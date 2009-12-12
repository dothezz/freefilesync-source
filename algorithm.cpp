#include "algorithm.h"
#include <wx/intl.h>
#include <stdexcept>
#include <wx/log.h>
#include "library/resources.h"
#include "shared/fileHandling.h"
#include <wx/msgdlg.h>
#include "library/filter.h"
#include <boost/bind.hpp>
#include "shared/stringConv.h"
#include "shared/globalFunctions.h"
#include "shared/loki/TypeManip.h"
#include "shared/loki/NullType.h"

using namespace FreeFileSync;


void FreeFileSync::swapGrids(const MainConfiguration& config, FolderComparison& folderCmp)
{
    std::for_each(folderCmp.begin(), folderCmp.end(), boost::bind(&BaseDirMapping::swap, _1));
    redetermineSyncDirection(config, folderCmp, NULL);
}


//----------------------------------------------------------------------------------------------
class Redetermine
{
public:
    Redetermine(const SyncConfiguration& configIn) : config(configIn) {}

    void execute(HierarchyObject& hierObj) const
    {
        //process files
        std::for_each(hierObj.subFiles.begin(), hierObj.subFiles.end(), *this);
        //process directories
        std::for_each(hierObj.subDirs.begin(), hierObj.subDirs.end(), *this);
    }

private:
    template<typename Iterator, typename Function>
    friend Function std::for_each(Iterator, Iterator, Function);

    void operator()(FileMapping& fileObj) const
    {
        switch (fileObj.getCategory())
        {
        case FILE_LEFT_SIDE_ONLY:
            fileObj.setSyncDir(config.exLeftSideOnly);
            break;
        case FILE_RIGHT_SIDE_ONLY:
            fileObj.setSyncDir(config.exRightSideOnly);
            break;
        case FILE_RIGHT_NEWER:
            fileObj.setSyncDir(config.rightNewer);
            break;
        case FILE_LEFT_NEWER:
            fileObj.setSyncDir(config.leftNewer);
            break;
        case FILE_DIFFERENT:
            fileObj.setSyncDir(config.different);
            break;
        case FILE_CONFLICT:
            fileObj.setSyncDir(config.conflict);
            break;
        case FILE_EQUAL:
            fileObj.setSyncDir(SYNC_DIR_NONE);
            break;
        }
    }

    void operator()(DirMapping& dirObj) const
    {
        switch (dirObj.getDirCategory())
        {
        case DIR_LEFT_SIDE_ONLY:
            dirObj.setSyncDir(config.exLeftSideOnly);
            break;
        case DIR_RIGHT_SIDE_ONLY:
            dirObj.setSyncDir(config.exRightSideOnly);
            break;
        case DIR_EQUAL:
            dirObj.setSyncDir(SYNC_DIR_NONE);
            break;
        }

        //recursion
        execute(dirObj);
    }

    const SyncConfiguration& config;
};


//---------------------------------------------------------------------------------------------------------------
class FindNonEqual //test if non-equal items exist in scanned data
{
public:
    bool findNonEqual(const HierarchyObject& hierObj) const
    {
        //files
        if (std::find_if(hierObj.subFiles.begin(), hierObj.subFiles.end(), *this) != hierObj.subFiles.end())
            return true;

        //directories
        return std::find_if(hierObj.subDirs.begin(), hierObj.subDirs.end(), *this) != hierObj.subDirs.end();
    }

    bool operator()(const FileMapping& fileObj) const
    {
        return fileObj.getCategory() != FILE_EQUAL;
    }

    bool operator()(const DirMapping& dirObj) const
    {
        if (dirObj.getDirCategory() != DIR_EQUAL)
            return true;

        return findNonEqual(dirObj); //recursion
    }
};


struct AllElementsEqual : public std::unary_function<BaseDirMapping, bool>
{
    bool operator()(const BaseDirMapping& baseMapping) const
    {
        return !FindNonEqual().findNonEqual(baseMapping);
    }
};


bool FreeFileSync::allElementsEqual(const FolderComparison& folderCmp)
{
    return std::find_if(folderCmp.begin(), folderCmp.end(), std::not1(AllElementsEqual())) == folderCmp.end();
}

//---------------------------------------------------------------------------------------------------------------
enum Answer
{
    CHANGE_DETECTED,
    NO_CHANGE,
    CANT_SAY_FILTERING_CHANGED
};


template <bool respectFiltering>
class DetectChanges
{
public:
    DetectChanges(const DirContainer* dirCont, //DirContainer in sense of a HierarchyObject
                  const FilterProcess::FilterRef& dbFilter);

    DetectChanges(const DirContainer* dirCont); //DirContainer in sense of a HierarchyObject

    template <SelectedSide side>
    Answer detectFileChange(const FileMapping& fileObj) const;

    struct DirAnswer
    {
        DirAnswer(Answer status, const DetectChanges instance) : dirStatus(status), subDirInstance(instance) {}
        Answer dirStatus;
        DetectChanges subDirInstance; //not valid if dirStatus == CANT_SAY_FILTERING_CHANGED
    };
    template <SelectedSide side>
    DirAnswer detectDirChange(const DirMapping& dirObj) const;

private:
    const DirContainer* const dirCont_; //if NULL: did not exist during db creation
    typename Loki::Select<respectFiltering, const FilterProcess::FilterRef, Loki::NullType>::Result dbFilter_; //filter setting that was used when db was created
};


template <>
inline
DetectChanges<true>::DetectChanges(const DirContainer* dirCont, //DirContainer in sense of a HierarchyObject
                                   const FilterProcess::FilterRef& dbFilter) :
    dirCont_(dirCont),
    dbFilter_(dbFilter) {}


template <>
inline
DetectChanges<false>::DetectChanges(const DirContainer* dirCont) : //DirContainer in sense of a HierarchyObject
    dirCont_(dirCont) {}



template <SelectedSide side>
Answer detectFileChangeSub(const FileMapping& fileObj, const DirContainer* dbDirectory)
{
    if (dbDirectory)
    {
        DirContainer::SubFileList::const_iterator j = dbDirectory->getSubFiles().find(fileObj.getObjShortName());
        if (j == dbDirectory->getSubFiles().end())
        {
            if (fileObj.isEmpty<side>())
                return NO_CHANGE;
            else
                return CHANGE_DETECTED; //->create
        }
        else
        {
            if (fileObj.isEmpty<side>())
                return CHANGE_DETECTED; //->delete
            else
            {
                const FileDescriptor& dbData = j->second.getData();
                if (    fileObj.getLastWriteTime<side>() == dbData.lastWriteTimeRaw &&
                        fileObj.getFileSize<side>()      == dbData.fileSize)
                    return NO_CHANGE;
                else
                    return CHANGE_DETECTED; //->update
            }
        }
    }
    else
    {
        if (fileObj.isEmpty<side>())
            return NO_CHANGE;
        else
            return CHANGE_DETECTED; //->create
    }
}


template <>
template <SelectedSide side>
inline
Answer DetectChanges<false>::detectFileChange(const FileMapping& fileObj) const
{
    return detectFileChangeSub<side>(fileObj, dirCont_);
}


template <>
template <SelectedSide side>
inline
Answer DetectChanges<true>::detectFileChange(const FileMapping& fileObj) const
{
    //if filtering would have excluded file during database creation, then we can't say anything about its former state
    if (!dbFilter_->passFileFilter(fileObj.getObjShortName()))
        return CANT_SAY_FILTERING_CHANGED;

    return detectFileChangeSub<side>(fileObj, dirCont_);
}


template <SelectedSide side>
Answer detectDirChangeSub(const DirMapping& dirObj, const DirContainer* dbDirectory, const DirContainer*& dbSubDirectory)
{
    if (dbDirectory)
    {
        DirContainer::SubDirList::const_iterator j = dbDirectory->getSubDirs().find(dirObj.getObjShortName());
        if (j == dbDirectory->getSubDirs().end())
        {
            if (dirObj.isEmpty<side>())
                return NO_CHANGE;
            else
                return CHANGE_DETECTED; //->create
        }
        else
        {
            dbSubDirectory = &j->second;
            if (dirObj.isEmpty<side>())
                return CHANGE_DETECTED; //->delete
            else
                return NO_CHANGE;
        }
    }
    else
    {
        if (dirObj.isEmpty<side>())
            return NO_CHANGE;
        else
            return CHANGE_DETECTED; //->create
    }
}


template <>
template <SelectedSide side>
DetectChanges<false>::DirAnswer DetectChanges<false>::detectDirChange(const DirMapping& dirObj) const
{
    const DirContainer* dbSubDir = NULL;
    const Answer answer = detectDirChangeSub<side>(dirObj, dirCont_, dbSubDir);

    return DirAnswer(answer, DetectChanges<false>(dbSubDir));
}


template <>
template <SelectedSide side>
DetectChanges<true>::DirAnswer DetectChanges<true>::detectDirChange(const DirMapping& dirObj) const
{
    //if filtering would have excluded file during database creation, then we can't say anything about its former state
    if (!dbFilter_->passDirFilter(dirObj.getObjShortName(), NULL))
        return DirAnswer(CANT_SAY_FILTERING_CHANGED, DetectChanges<true>(NULL, dbFilter_));

    const DirContainer* dbSubDir = NULL;
    const Answer answer = detectDirChangeSub<side>(dirObj, dirCont_, dbSubDir);

    return DirAnswer(answer, DetectChanges<true>(dbSubDir, dbFilter_));
}


//----------------------------------------------------------------------------------------------
class SetDirChangedFilter
{
public:
    SetDirChangedFilter() :
        txtFilterChanged(_("Cannot determine sync-direction: Changed filter settings!")) {}

    void execute(HierarchyObject& hierObj) const
    {
        //files
        std::for_each(hierObj.subFiles.begin(), hierObj.subFiles.end(), *this);
        //directories
        std::for_each(hierObj.subDirs.begin(), hierObj.subDirs.end(), *this);
    }

private:
    template<typename Iterator, typename Function>
    friend Function std::for_each(Iterator, Iterator, Function);

    void operator()(FileMapping& fileObj) const
    {
        const CompareFilesResult cat = fileObj.getCategory();
        if (cat == FILE_LEFT_SIDE_ONLY)
            fileObj.setSyncDir(SYNC_DIR_RIGHT);
        else if (cat == FILE_RIGHT_SIDE_ONLY)
            fileObj.setSyncDir(SYNC_DIR_LEFT);
        else
            fileObj.setSyncDirConflict(txtFilterChanged);   //set syncDir = SYNC_DIR_INT_CONFLICT
    }

    void operator()(DirMapping& dirObj) const
    {
        switch (dirObj.getDirCategory())
        {
        case DIR_LEFT_SIDE_ONLY:
            dirObj.setSyncDir(SYNC_DIR_RIGHT);
            break;
        case DIR_RIGHT_SIDE_ONLY:
            dirObj.setSyncDir(SYNC_DIR_LEFT);
            break;
        case DIR_EQUAL:
            break;
        }

        execute(dirObj); //recursion
    }

    const wxString txtFilterChanged;
};


//----------------------------------------------------------------------------------------------
class RedetermineAuto
{
public:
    RedetermineAuto(BaseDirMapping& baseDirectory,
                    DeterminationProblem* handler) :
        txtBothSidesChanged(_("Both sides have changed since last synchronization!")),
        txtNoSideChanged(_("Cannot determine sync-direction: No change since last synchronization!")),
        txtFilterChanged(_("Cannot determine sync-direction: Changed filter settings!")),
        handler_(handler)
    {
        if (AllElementsEqual()(baseDirectory)) //nothing to do: abort and don't show any nag-screens
            return;

        //try to load sync-database files
        std::pair<DirInfoPtr, DirInfoPtr> dirInfo = loadDBFile(baseDirectory);
        if (    dirInfo.first.get()  == NULL ||
                dirInfo.second.get() == NULL)
        {
            //use standard settings:
            SyncConfiguration defaultSync;
            defaultSync.setVariant(SyncConfiguration::TWOWAY);
            Redetermine(defaultSync).execute(baseDirectory);
            return;
        }

        const DirInformation& dirInfoLeft  = *dirInfo.first;
        const DirInformation& dirInfoRight = *dirInfo.second;

        if (    respectFiltering(baseDirectory, dirInfoLeft) &&
                respectFiltering(baseDirectory, dirInfoRight))
        {
            execute(baseDirectory,
                    DetectChanges<true>(&dirInfoLeft.baseDirContainer,  dirInfoLeft.filter),
                    DetectChanges<true>(&dirInfoRight.baseDirContainer, dirInfoRight.filter));
        }
        else if (   !respectFiltering(baseDirectory, dirInfoLeft) &&
                    respectFiltering( baseDirectory, dirInfoRight))
        {
            execute(baseDirectory,
                    DetectChanges<false>(&dirInfoLeft.baseDirContainer),
                    DetectChanges<true>( &dirInfoRight.baseDirContainer, dirInfoRight.filter));
        }
        else if (   respectFiltering( baseDirectory, dirInfoLeft) &&
                    !respectFiltering(baseDirectory, dirInfoRight))
        {
            execute(baseDirectory,
                    DetectChanges<true>( &dirInfoLeft.baseDirContainer, dirInfoLeft.filter),
                    DetectChanges<false>(&dirInfoRight.baseDirContainer));
        }
        else
        {
            execute(baseDirectory,
                    DetectChanges<false>(&dirInfoLeft.baseDirContainer),
                    DetectChanges<false>(&dirInfoRight.baseDirContainer));
        }
    }


private:
    static bool respectFiltering(const BaseDirMapping& baseDirectory, const DirInformation& dirInfo)
    {
        //respect filtering if sync-DB filter is active && different from baseDir's filter:
        // in all other cases "view on files" is smaller for baseDirectory(current) than it was for dirInfo(old)
        // => dirInfo can be queried as if it were a scan without filters
        return !dirInfo.filter->isNull() && *dirInfo.filter != *baseDirectory.getFilter();
    }

    std::pair<DirInfoPtr, DirInfoPtr> loadDBFile(const BaseDirMapping& baseDirectory) //return NULL on failure
    {
        try
        {
            return loadFromDisk(baseDirectory);
        }
        catch (FileError& error) //e.g. incompatible database version
        {
            if (handler_) handler_->reportWarning(error.show() + wxT(" \n\n") +
                                                      _("Setting default synchronization directions. Please check whether they are appropriate for you."));
        }
        return std::pair<DirInfoPtr, DirInfoPtr>(); //NULL
    }

    template<typename Iterator, typename Function>
    friend Function std::for_each(Iterator, Iterator, Function);

    template <bool dbLeftFilterActive, bool dbRightFilterActive>
    void execute(HierarchyObject& hierObj,
                 const DetectChanges<dbLeftFilterActive>& dbLeft,
                 const DetectChanges<dbRightFilterActive>& dbRight)
    {
        //process files
        std::for_each(hierObj.subFiles.begin(), hierObj.subFiles.end(),
                      boost::bind(&RedetermineAuto::processFile<dbLeftFilterActive, dbRightFilterActive>, this, _1, boost::ref(dbLeft), boost::ref(dbRight)));
        //process directories
        std::for_each(hierObj.subDirs.begin(), hierObj.subDirs.end(),
                      boost::bind(&RedetermineAuto::processDir<dbLeftFilterActive, dbRightFilterActive>, this, _1, boost::ref(dbLeft), boost::ref(dbRight)));
    }

    template <bool dbLeftFilterActive, bool dbRightFilterActive>
    void processFile(FileMapping& fileObj,
                     const DetectChanges<dbLeftFilterActive>& dbLeft,
                     const DetectChanges<dbRightFilterActive>& dbRight)
    {
        const CompareFilesResult cat = fileObj.getCategory();
        if (cat == FILE_EQUAL)
            return;

        const Answer statusLeft  = dbLeft. template detectFileChange<LEFT_SIDE>(fileObj);
        const Answer statusRight = dbRight.template detectFileChange<RIGHT_SIDE>(fileObj);

        if (    statusLeft  == CANT_SAY_FILTERING_CHANGED ||
                statusRight == CANT_SAY_FILTERING_CHANGED)
        {
            if (cat == FILE_LEFT_SIDE_ONLY)
                fileObj.setSyncDir(SYNC_DIR_RIGHT);
            else if (cat == FILE_RIGHT_SIDE_ONLY)
                fileObj.setSyncDir(SYNC_DIR_LEFT);
            else
                fileObj.setSyncDirConflict(txtFilterChanged);   //set syncDir = SYNC_DIR_INT_CONFLICT
            return;
        }


        if (    statusLeft  == CHANGE_DETECTED &&
                statusRight == NO_CHANGE)
            fileObj.setSyncDir(SYNC_DIR_RIGHT);

        else if (    statusLeft  == NO_CHANGE &&
                     statusRight == CHANGE_DETECTED)
            fileObj.setSyncDir(SYNC_DIR_LEFT);

        else if (    statusLeft  == CHANGE_DETECTED &&
                     statusRight == CHANGE_DETECTED)
            fileObj.setSyncDirConflict(txtBothSidesChanged);   //set syncDir = SYNC_DIR_INT_CONFLICT

        else if (    statusLeft  == NO_CHANGE &&
                     statusRight == NO_CHANGE)
        {
            if (cat == FILE_LEFT_SIDE_ONLY)
                fileObj.setSyncDir(SYNC_DIR_RIGHT);
            else if (cat == FILE_RIGHT_SIDE_ONLY)
                fileObj.setSyncDir(SYNC_DIR_LEFT);
            else
                fileObj.setSyncDirConflict(txtNoSideChanged);   //set syncDir = SYNC_DIR_INT_CONFLICT
        }
    }


    template <bool dbLeftFilterActive, bool dbRightFilterActive>
    void processDir(DirMapping& dirObj,
                    const DetectChanges<dbLeftFilterActive>& dbLeft,
                    const DetectChanges<dbRightFilterActive>& dbRight)
    {
        const typename DetectChanges<dbLeftFilterActive> ::DirAnswer statusLeft  = dbLeft. template detectDirChange<LEFT_SIDE>(dirObj);
        const typename DetectChanges<dbRightFilterActive>::DirAnswer statusRight = dbRight.template detectDirChange<RIGHT_SIDE>(dirObj);

        const CompareDirResult cat = dirObj.getDirCategory();
        if (cat != DIR_EQUAL)
        {
            if (    (statusLeft.dirStatus  == CANT_SAY_FILTERING_CHANGED ||
                     statusRight.dirStatus == CANT_SAY_FILTERING_CHANGED))
            {
                switch (cat)
                {
                case DIR_LEFT_SIDE_ONLY:
                    dirObj.setSyncDir(SYNC_DIR_RIGHT);
                    break;
                case DIR_RIGHT_SIDE_ONLY:
                    dirObj.setSyncDir(SYNC_DIR_LEFT);
                    break;
                case DIR_EQUAL:
                    assert(false);
                }

                SetDirChangedFilter().execute(dirObj); //filter issue for this directory => treat subfiles/-dirs the same
                return;
            }
            else if (    statusLeft.dirStatus  == CHANGE_DETECTED &&
                         statusRight.dirStatus == NO_CHANGE)
                dirObj.setSyncDir(SYNC_DIR_RIGHT);

            else if (    statusLeft.dirStatus  == NO_CHANGE &&
                         statusRight.dirStatus == CHANGE_DETECTED)
                dirObj.setSyncDir(SYNC_DIR_LEFT);

            else if (    statusLeft.dirStatus  == CHANGE_DETECTED &&
                         statusRight.dirStatus == CHANGE_DETECTED)
                dirObj.setSyncDirConflict(txtBothSidesChanged);   //set syncDir = SYNC_DIR_INT_CONFLICT

            else if (    statusLeft.dirStatus  == NO_CHANGE &&
                         statusRight.dirStatus == NO_CHANGE)
            {
                switch (cat)
                {
                case DIR_LEFT_SIDE_ONLY:
                    dirObj.setSyncDir(SYNC_DIR_RIGHT);
                    break;
                case DIR_RIGHT_SIDE_ONLY:
                    dirObj.setSyncDir(SYNC_DIR_LEFT);
                    break;
                case DIR_EQUAL:
                    assert(false);
                }
            }
        }

        execute(dirObj, statusLeft.subDirInstance, statusRight.subDirInstance); //recursion
    }

    const wxString txtBothSidesChanged;
    const wxString txtNoSideChanged;
    const wxString txtFilterChanged;

    DeterminationProblem* const handler_;
};


//---------------------------------------------------------------------------------------------------------------
void FreeFileSync::redetermineSyncDirection(const SyncConfiguration& config, BaseDirMapping& baseDirectory, DeterminationProblem* handler)
{
    if (config.automatic)
        RedetermineAuto(baseDirectory, handler);
    else
        Redetermine(config).execute(baseDirectory);
}


void FreeFileSync::redetermineSyncDirection(const MainConfiguration& currentMainCfg, FolderComparison& folderCmp, DeterminationProblem* handler)
{
    if (folderCmp.size() == 0)
        return;
    else if (folderCmp.size() != currentMainCfg.additionalPairs.size() + 1)
        throw std::logic_error("Programming Error: Contract violation!");

    //merge first and additional pairs
    std::vector<FolderPairEnh> allPairs;
    allPairs.push_back(currentMainCfg.firstPair);
    allPairs.insert(allPairs.end(),
                    currentMainCfg.additionalPairs.begin(), //add additional pairs
                    currentMainCfg.additionalPairs.end());

    for (std::vector<FolderPairEnh>::const_iterator i = allPairs.begin(); i != allPairs.end(); ++i)
    {
        redetermineSyncDirection(i->altSyncConfig.get() ? i->altSyncConfig->syncConfiguration : currentMainCfg.syncConfiguration,
                                 folderCmp[i - allPairs.begin()], handler);
    }
}


//---------------------------------------------------------------------------------------------------------------
class SetNewDirection
{
public:
    SetNewDirection(SyncDirection newDirection) :
        newDirection_(newDirection) {}

    void execute(HierarchyObject& hierObj) const
    {
        //directories
        std::for_each(hierObj.subDirs.begin(), hierObj.subDirs.end(), *this);
        //files
        std::for_each(hierObj.subFiles.begin(), hierObj.subFiles.end(), *this);
    }

private:
    template<typename Iterator, typename Function>
    friend Function std::for_each(Iterator, Iterator, Function);

    void operator()(FileMapping& fileObj) const
    {
        fileObj.setSyncDir(newDirection_);
    }

    void operator()(DirMapping& dirObj) const
    {
        dirObj.setSyncDir(newDirection_);
        execute(dirObj); //recursion
    }

    const SyncDirection newDirection_;
};


void FreeFileSync::setSyncDirectionRec(SyncDirection newDirection, FileSystemObject& fsObj)
{
    fsObj.setSyncDir(newDirection);

    DirMapping* dirObj = dynamic_cast<DirMapping*>(&fsObj);
    if (dirObj) //process subdirectories also!
        SetNewDirection(newDirection).execute(*dirObj);
}


//--------------- functions related to filtering ------------------------------------------------------------------------------------

template <bool include>
class InOrExcludeAllRows
{
public:
    void operator()(FreeFileSync::BaseDirMapping& baseDirectory) const //be careful with operator() to no get called by std::for_each!
    {
        execute(baseDirectory);
    }

    void execute(FreeFileSync::HierarchyObject& hierObj) const //don't create ambiguity by replacing with operator()
    {
        std::for_each(hierObj.subFiles.begin(), hierObj.subFiles.end(), *this); //files
        std::for_each(hierObj.subDirs.begin(),  hierObj.subDirs.end(),  *this); //directories
    }

private:
    template<typename Iterator, typename Function>
    friend Function std::for_each(Iterator, Iterator, Function);

    void operator()(FreeFileSync::FileMapping& fileObj) const
    {
        fileObj.setActive(include);
    }

    void operator()(FreeFileSync::DirMapping& dirObj) const
    {
        dirObj.setActive(include);
        execute(dirObj); //recursion
    }
};


void FreeFileSync::setActiveStatus(bool newStatus, FreeFileSync::FolderComparison& folderCmp)
{
    if (newStatus)
        std::for_each(folderCmp.begin(), folderCmp.end(), InOrExcludeAllRows<true>());  //include all rows
    else
        std::for_each(folderCmp.begin(), folderCmp.end(), InOrExcludeAllRows<false>()); //exclude all rows
}


void FreeFileSync::setActiveStatus(bool newStatus, FreeFileSync::FileSystemObject& fsObj)
{
    fsObj.setActive(newStatus);

    DirMapping* dirObj = dynamic_cast<DirMapping*>(&fsObj);
    if (dirObj) //process subdirectories also!
    {
        if (newStatus)
            InOrExcludeAllRows<true>().execute(*dirObj);
        else
            InOrExcludeAllRows<false>().execute(*dirObj);
    }
}


class FilterData
{
public:
    FilterData(const FilterProcess& filterProcIn) : filterProc(filterProcIn) {}

    void execute(FreeFileSync::HierarchyObject& hierObj)
    {
        //files
        std::for_each(hierObj.subFiles.begin(), hierObj.subFiles.end(), *this);

        //directories
        std::for_each(hierObj.subDirs.begin(), hierObj.subDirs.end(), *this);
    };

private:
    template<typename Iterator, typename Function>
    friend Function std::for_each(Iterator, Iterator, Function);


    void operator()(FreeFileSync::FileMapping& fileObj)
    {
        fileObj.setActive(filterProc.passFileFilter(fileObj.getObjRelativeName()));
    }

    void operator()(FreeFileSync::DirMapping& dirObj)
    {
        bool subObjMightMatch = true;
        dirObj.setActive(filterProc.passDirFilter(dirObj.getObjRelativeName(), &subObjMightMatch));

        if (subObjMightMatch) //use same logic as within directory traversing here: evaluate filter in subdirs only if objects could match
            execute(dirObj);  //recursion
        else
            InOrExcludeAllRows<false>().execute(dirObj); //exclude all files dirs in subfolders
    }

    const FilterProcess& filterProc;
};


void FreeFileSync::applyFiltering(const FilterProcess& filter, FreeFileSync::BaseDirMapping& baseDirectory)
{
    FilterData(filter).execute(baseDirectory);
}


void FreeFileSync::applyFiltering(const MainConfiguration& currentMainCfg, FolderComparison& folderCmp)
{
    if (folderCmp.size() == 0)
        return;
    else if (folderCmp.size() != currentMainCfg.additionalPairs.size() + 1)
        throw std::logic_error("Programming Error: Contract violation!");


    //merge first and additional pairs
    std::vector<FolderPairEnh> allPairs;
    allPairs.push_back(currentMainCfg.firstPair);
    allPairs.insert(allPairs.end(),
                    currentMainCfg.additionalPairs.begin(), //add additional pairs
                    currentMainCfg.additionalPairs.end());


    const FilterProcess::FilterRef globalFilter(new NameFilter(currentMainCfg.includeFilter, currentMainCfg.excludeFilter));

    for (std::vector<FolderPairEnh>::const_iterator i = allPairs.begin(); i != allPairs.end(); ++i)
    {
        BaseDirMapping& baseDirectory = folderCmp[i - allPairs.begin()];

        applyFiltering(*combineFilters(globalFilter,
                                       FilterProcess::FilterRef(new NameFilter(
                                               i->localFilter.includeFilter,
                                               i->localFilter.excludeFilter))),
                       baseDirectory);
    }
}


//############################################################################################################
std::pair<wxString, int> FreeFileSync::deleteFromGridAndHDPreview( //assemble message containing all files to be deleted
    const std::vector<FileSystemObject*>& rowsToDeleteOnLeft,
    const std::vector<FileSystemObject*>& rowsToDeleteOnRight,
    const bool deleteOnBothSides)
{
    wxString filesToDelete;
    int totalDelCount = 0;

    if (deleteOnBothSides)
    {
        //mix selected rows from left and right
        std::set<FileSystemObject*> rowsToDelete(rowsToDeleteOnLeft.begin(), rowsToDeleteOnLeft.end());
        rowsToDelete.insert(rowsToDeleteOnRight.begin(), rowsToDeleteOnRight.end());

        for (std::set<FileSystemObject*>::const_iterator i = rowsToDelete.begin(); i != rowsToDelete.end(); ++i)
        {
            const FileSystemObject& currObj = *(*i);

            if (!currObj.isEmpty<LEFT_SIDE>())
            {
                filesToDelete += zToWx(currObj.getFullName<LEFT_SIDE>()) + wxT("\n");
                ++totalDelCount;
            }

            if (!currObj.isEmpty<RIGHT_SIDE>())
            {
                filesToDelete += zToWx(currObj.getFullName<RIGHT_SIDE>()) + wxT("\n");
                ++totalDelCount;
            }

            filesToDelete += wxT("\n");
        }
    }
    else //delete selected files only
    {
        for (std::vector<FileSystemObject*>::const_iterator i = rowsToDeleteOnLeft.begin(); i != rowsToDeleteOnLeft.end(); ++i)
        {
            const FileSystemObject& currObj = *(*i);

            if (!currObj.isEmpty<LEFT_SIDE>())
            {
                filesToDelete += zToWx(currObj.getFullName<LEFT_SIDE>()) + wxT("\n");
                ++totalDelCount;
            }
        }

        for (std::vector<FileSystemObject*>::const_iterator i = rowsToDeleteOnRight.begin(); i != rowsToDeleteOnRight.end(); ++i)
        {
            const FileSystemObject& currObj = *(*i);

            if (!currObj.isEmpty<RIGHT_SIDE>())
            {
                filesToDelete += zToWx(currObj.getFullName<RIGHT_SIDE>()) + wxT("\n");
                ++totalDelCount;
            }
        }
    }

    return std::make_pair(filesToDelete, totalDelCount);
}


template <SelectedSide side>
void deleteFromGridAndHDOneSide(std::vector<FileSystemObject*>& rowsToDeleteOneSide,
                                const bool useRecycleBin,
                                DeleteFilesHandler* statusHandler)
{
    for (std::vector<FileSystemObject*>::const_iterator i = rowsToDeleteOneSide.begin(); i != rowsToDeleteOneSide.end(); ++i)
    {
        if (!(*i)->isEmpty<side>())
        {
            while (true)
            {
                try
                {
                    FileMapping* fileObj = dynamic_cast<FileMapping*>(*i);
                    if (fileObj != NULL)
                    {
                        FreeFileSync::removeFile(fileObj->getFullName<side>(), useRecycleBin);
                        fileObj->removeObject<side>();
                        statusHandler->deletionSuccessful(); //notify successful file/folder deletion
                    }
                    else
                    {
                        DirMapping* dirObj = dynamic_cast<DirMapping*>(*i);
                        if (dirObj != NULL)
                        {
                            FreeFileSync::removeDirectory(dirObj->getFullName<side>(), useRecycleBin);
                            dirObj->removeObject<side>(); //directory: removes recursively!
                            statusHandler->deletionSuccessful(); //notify successful file/folder deletion
                        }
                        else
                            assert(!"It's no file, no dir, what is it then?");
                    }

                    break;
                }
                catch (const FileError& error)
                {
                    DeleteFilesHandler::Response rv = statusHandler->reportError(error.show());

                    if (rv == DeleteFilesHandler::IGNORE_ERROR)
                        break;

                    else if (rv == DeleteFilesHandler::RETRY)
                        ;   //continue in loop
                    else
                        assert (false);
                }
            }
        }
    }
}


class FinalizeDeletion
{
public:
    FinalizeDeletion(FolderComparison& folderCmp, const MainConfiguration& mainConfig) :
        folderCmp_(folderCmp),
        mainConfig_(mainConfig) {}

    ~FinalizeDeletion()
    {
        std::for_each(folderCmp_.begin(), folderCmp_.end(), FileSystemObject::removeEmpty);
        redetermineSyncDirection(mainConfig_, folderCmp_, NULL);
    }

private:
    FolderComparison& folderCmp_;
    const MainConfiguration& mainConfig_;
};


void FreeFileSync::deleteFromGridAndHD(FolderComparison& folderCmp,                        //attention: rows will be physically deleted!
                                       std::vector<FileSystemObject*>& rowsToDeleteOnLeft,  //refresh GUI grid after deletion to remove invalid rows
                                       std::vector<FileSystemObject*>& rowsToDeleteOnRight, //all pointers need to be bound!
                                       const bool deleteOnBothSides,
                                       const bool useRecycleBin,
                                       const MainConfiguration& mainConfig,
                                       DeleteFilesHandler* statusHandler)
{
    if (folderCmp.size() == 0)
        return;
    else if (folderCmp.size() != mainConfig.additionalPairs.size() + 1)
        throw std::logic_error("Programming Error: Contract violation!");

    FinalizeDeletion dummy(folderCmp, mainConfig); //ensure cleanup: redetermination of sync-directions and removal of invalid rows


    if (deleteOnBothSides)
    {
        //mix selected rows from left and right (and remove duplicates)
        std::set<FileSystemObject*> temp(rowsToDeleteOnLeft.begin(), rowsToDeleteOnLeft.end());
        temp.insert(rowsToDeleteOnRight.begin(), rowsToDeleteOnRight.end());

        std::vector<FileSystemObject*> rowsToDeleteBothSides(temp.begin(), temp.end());

        deleteFromGridAndHDOneSide<LEFT_SIDE>(rowsToDeleteBothSides,
                                              useRecycleBin,
                                              statusHandler);

        deleteFromGridAndHDOneSide<RIGHT_SIDE>(rowsToDeleteBothSides,
                                               useRecycleBin,
                                               statusHandler);
    }
    else
    {
        deleteFromGridAndHDOneSide<LEFT_SIDE>(rowsToDeleteOnLeft,
                                              useRecycleBin,
                                              statusHandler);

        deleteFromGridAndHDOneSide<RIGHT_SIDE>(rowsToDeleteOnRight,
                                               useRecycleBin,
                                               statusHandler);
    }
}


//############################################################################################################
/*Statistical theory: detect daylight saving time (DST) switch by comparing files that exist on both sides (and have same filesizes). If there are "enough"
that have a shift by +-1h then assert that DST switch occured.
What is "enough" =: N? N should be large enough AND small enough that the following two errors remain small:

Error 1: A DST switch is detected although there was none
Error 2: A DST switch is not detected although it took place

Error 1 results in lower bound, error 2 in upper bound for N.

Target: Choose N such that probability of error 1 and error 2 is lower than 0.001 (0.1%)

Definitions:
p1: probability that a file with same filesize on both sides was changed nevertheless
p2: probability that a changed file has +1h shift in filetime due to a change

M: number of files with same filesize on both sides in total
N: number of files with same filesize and time-diff +1h when DST check shall detect "true"

X: number of files with same filesize that have a +1h difference after change

Error 1 ("many files have +1h shift by chance") imposes:
Probability of error 1: (binomial distribution)

P(X >= N) = 1 - P(X <= N - 1) =
1 - sum_i=0^N-1 p3^i * (1 - p3)^(M - i) (M above i)   shall be   <= 0.0005

with p3 := p1 * p2

Probability of error 2 also will be <= 0.0005 if we choose N as lowest number that satisfies the preceding formula. Proof is left to the reader.

The function M |-> N behaves almost linearly and can be empirically approximated by:

N(M) =

2                   for      0 <= M <= 500
125/1000000 * M + 5 for    500 <  M <= 50000
77/1000000 * M + 10 for  50000 <  M <= 400000
60/1000000 * M + 35 for 400000 <  M


inline
bool sameFileTime(const time_t a, const time_t b)
{
    if (a < b)
        return b - a <= FILE_TIME_PRECISION;
    else
        return a - b <= FILE_TIME_PRECISION;
}


#ifdef FFS_WIN
unsigned int getThreshold(const unsigned filesWithSameSizeTotal)
{
    if (filesWithSameSizeTotal <= 500)
        return 2;
    else if (filesWithSameSizeTotal <= 50000)
        return unsigned(125.0/1000000 * filesWithSameSizeTotal + 5.0);
    else if (filesWithSameSizeTotal <= 400000)
        return unsigned(77.0/1000000 * filesWithSameSizeTotal + 10.0);
    else
        return unsigned(60.0/1000000 * filesWithSameSizeTotal + 35.0);
}


void FreeFileSync::checkForDSTChange(const FileCompareResult& gridData,
                                     const std::vector<FolderPair>& directoryPairsFormatted,
                                     int& timeShift, wxString& driveName)
{
    driveName.Clear();
    timeShift = 0;

    TIME_ZONE_INFORMATION dummy;
    DWORD rv = GetTimeZoneInformation(&dummy);
    if (rv == TIME_ZONE_ID_UNKNOWN) return;
    bool dstActive = rv == TIME_ZONE_ID_DAYLIGHT;

    for (std::vector<FolderPair>::const_iterator i = directoryPairsFormatted.begin(); i != directoryPairsFormatted.end(); ++i)
    {
        bool leftDirIsFat = isFatDrive(i->leftDirectory);
        bool rightDirIsFat = isFatDrive(i->rightDirectory);

        if (leftDirIsFat || rightDirIsFat)
        {
            unsigned int filesTotal        = 0; //total number of files (with same size on both sides)
            unsigned int plusOneHourCount  = 0; //number of files with +1h time shift
            unsigned int minusOneHourCount = 0; // "

            for (FileCompareResult::const_iterator j = gridData.begin(); j != gridData.end(); ++j)
            {
                const FileDescrLine& leftFile  = j->fileDescrLeft;
                const FileDescrLine& rightFile = j->fileDescrRight;

                if (    leftFile.objType == FileDescrLine::TYPE_FILE && rightFile.objType == FileDescrLine::TYPE_FILE &&
                        leftFile.fileSize == rightFile.fileSize &&
                        leftFile.directory.CmpNoCase(i->leftDirectory.c_str()) == 0 && //Windows does NOT distinguish between upper/lower-case
                        rightFile.directory.CmpNoCase(i->rightDirectory.c_str()) == 0) //
                {
                    ++filesTotal;

                    if (sameFileTime(leftFile.lastWriteTimeRaw - 3600, rightFile.lastWriteTimeRaw))
                        ++plusOneHourCount;
                    else if (sameFileTime(leftFile.lastWriteTimeRaw + 3600, rightFile.lastWriteTimeRaw))
                        ++minusOneHourCount;
                }
            }

            unsigned int threshold = getThreshold(filesTotal);
            if (plusOneHourCount >= threshold)
            {
                if (dstActive)
                {
                    if (rightDirIsFat) //it should be FAT; else this were some kind of error
                    {
                        timeShift = 3600;
                        driveName = getDriveName(i->rightDirectory);
                    }
                }
                else
                {
                    if (leftDirIsFat) //it should be FAT; else this were some kind of error
                    {
                        timeShift = -3600;
                        driveName = getDriveName(i->leftDirectory);
                    }
                }
                return;
            }
            else if (minusOneHourCount >= threshold)
            {
                if (dstActive)
                {
                    if (leftDirIsFat) //it should be FAT; else this were some kind of error
                    {
                        timeShift = 3600;
                        driveName = getDriveName(i->leftDirectory);
                    }
                }
                else
                {
                    if (rightDirIsFat) //it should be FAT; else this were some kind of error
                    {
                        timeShift = -3600;
                        driveName = getDriveName(i->rightDirectory);
                    }
                }
                return;
            }
        }
    }
}
#endif  //FFS_WIN
*/


