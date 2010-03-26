// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) 2008-2010 ZenJu (zhnmju123 AT gmx.de)                    *
// **************************************************************************
//
#include "algorithm.h"
#include <wx/intl.h>
#include <stdexcept>
#include <wx/log.h>
#include "library/resources.h"
#include "shared/fileHandling.h"
#include "shared/recycler.h"
#include <wx/msgdlg.h>
#include "library/filter.h"
#include <boost/bind.hpp>
#include "shared/stringConv.h"
#include "shared/globalFunctions.h"
#include "shared/loki/TypeManip.h"
#include "library/dbFile.h"
//#include "shared/loki/NullType.h"

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
        std::for_each(hierObj.useSubFiles().begin(), hierObj.useSubFiles().end(), *this);
        //process directories
        std::for_each(hierObj.useSubDirs().begin(), hierObj.useSubDirs().end(), *this);
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
        if (std::find_if(hierObj.useSubFiles().begin(), hierObj.useSubFiles().end(), *this) != hierObj.useSubFiles().end())
            return true;

        //directories
        return std::find_if(hierObj.useSubDirs().begin(), hierObj.useSubDirs().end(), *this) != hierObj.useSubDirs().end();
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
inline
bool sameFileTime(const wxLongLong& a, const wxLongLong& b, const unsigned int tolerance)
{
    if (a < b)
        return b <= a + tolerance;
    else
        return a <= b + tolerance;
}
//---------------------------------------------------------------------------------------------------------------

class DataSetFile
{
public:
    DataSetFile(const FileMapping& fileObj, Loki::Int2Type<LEFT_SIDE>) :
        lastWriteTime_(NULL),
        fileSize_(NULL)
    {
        init<LEFT_SIDE>(fileObj);
    }

    DataSetFile(const FileMapping& fileObj, Loki::Int2Type<RIGHT_SIDE>) :
        lastWriteTime_(NULL),
        fileSize_(NULL)
    {
        init<RIGHT_SIDE>(fileObj);
    }


    DataSetFile(const FileContainer* fileCont) :
        lastWriteTime_(NULL),
        fileSize_(NULL)
    {
        if (fileCont)
        {
            const FileDescriptor& dbData = fileCont->getData();
            lastWriteTime_ = &dbData.lastWriteTimeRaw;
            fileSize_      = &dbData.fileSize;
        }
    }

    bool operator==(const DataSetFile& other) const
    {
        if (lastWriteTime_ == NULL)
            return other.lastWriteTime_ == NULL;
        else
        {
            if (other.lastWriteTime_ == NULL)
                return false;
            else
            {
                //respect 2 second FAT/FAT32 precision! copying a file to a FAT32 drive changes it's modification date by up to 2 seconds
                return ::sameFileTime(*lastWriteTime_, *other.lastWriteTime_, 2) &&
                       *fileSize_ == *other.fileSize_;
            }
        }
    }

    template <class T>
    bool operator!=(const T& other) const
    {
        return !(*this == other);
    }

private:
    template <SelectedSide side>
    void init(const FileMapping& fileObj)
    {
        if (!fileObj.isEmpty<side>())
        {
            lastWriteTime_ = &fileObj.getLastWriteTime<side>();
            fileSize_      = &fileObj.getFileSize<side>();
        }
    }

    const wxLongLong*  lastWriteTime_; //optional
    const wxULongLong* fileSize_;      //optional
};


//-----------------------------------
class DataSetDir
{
public:
    DataSetDir(const DirMapping& dirObj, Loki::Int2Type<LEFT_SIDE>) :
        existing(!dirObj.isEmpty<LEFT_SIDE>()) {}

    DataSetDir(const DirMapping& dirObj, Loki::Int2Type<RIGHT_SIDE>) :
        existing(!dirObj.isEmpty<RIGHT_SIDE>()) {}

    DataSetDir(bool isExising) :
        existing(isExising) {}

    bool operator==(const DataSetDir& other) const
    {
        return existing == other.existing;
    }

    template <class T>
    bool operator!=(const T& other) const
    {
        return !(*this == other);
    }

private:
    bool existing;
};


//--------------------------------------------------------------------------------------------------------
DataSetFile retrieveDataSetFile(const Zstring& objShortName, const DirContainer* dbDirectory)
{
    if (dbDirectory)
        return dbDirectory->findFile(objShortName); //return value may be NULL

    //object not found
    return DataSetFile(NULL);
}


std::pair<DataSetDir, const DirContainer*> retrieveDataSetDir(const Zstring& objShortName, const DirContainer* dbDirectory)
{
    if (dbDirectory)
    {
        const DirContainer* dbDir = dbDirectory->findDir(objShortName);
        if (dbDir)
            return std::make_pair(DataSetDir(true), dbDir);
    }

    //object not found
    return std::make_pair(DataSetDir(false), static_cast<const DirContainer*>(NULL));
}


//--------------------------------------------------------------------------------------------------------
class SetDirChangedFilter
{
public:
    SetDirChangedFilter() :
        txtFilterChanged(wxString(_("Cannot determine sync-direction:")) + wxT(" \n") + _("Filter settings have changed!")) {}

    void execute(HierarchyObject& hierObj) const
    {
        //files
        std::for_each(hierObj.useSubFiles().begin(), hierObj.useSubFiles().end(), *this);
        //directories
        std::for_each(hierObj.useSubDirs().begin(), hierObj.useSubDirs().end(), *this);
    }

private:
    template<typename Iterator, typename Function>
    friend Function std::for_each(Iterator, Iterator, Function);

    void operator()(FileMapping& fileObj) const
    {
        const CompareFilesResult cat = fileObj.getCategory();

        if (cat == FILE_EQUAL)
            return;

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
        txtNoSideChanged(wxString(_("Cannot determine sync-direction:")) + wxT(" \n") + _("No change since last synchronization!")),
        txtFilterChanged(wxString(_("Cannot determine sync-direction:")) + wxT(" \n") + _("Filter settings have changed!")),
        txtLastSyncFail(wxString(_("Cannot determine sync-direction:")) + wxT(" \n") + _("Last synchronization not completed!")),
        dbFilterLeft(NULL),
        dbFilterRight(NULL),
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
            FreeFileSync::setTwoWay(defaultSync);
            Redetermine(defaultSync).execute(baseDirectory);
            return;
        }

        const DirInformation& dirInfoLeft  = *dirInfo.first;
        const DirInformation& dirInfoRight = *dirInfo.second;

        //save db filter (if it needs to be considered only):
        if (respectFiltering(baseDirectory, dirInfoLeft))
            dbFilterLeft = dirInfoLeft.filter.get();

        if (respectFiltering(baseDirectory, dirInfoRight))
            dbFilterRight = dirInfoRight.filter.get();

        execute(baseDirectory,
                &dirInfoLeft.baseDirContainer,
                &dirInfoRight.baseDirContainer);
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
                                                      _("Setting default synchronization directions: Old files will be overwritten by newer files."));
        }
        return std::pair<DirInfoPtr, DirInfoPtr>(); //NULL
    }


    bool filterConflictFound(const FileMapping& fileObj) const
    {
        //if filtering would have excluded file during database creation, then we can't say anything about its former state
        return (dbFilterLeft  && !dbFilterLeft ->passFileFilter(fileObj.getObjShortName())) ||
               (dbFilterRight && !dbFilterRight->passFileFilter(fileObj.getObjShortName()));
    }


    bool filterConflictFound(const DirMapping& dirObj) const
    {
        //if filtering would have excluded directory during database creation, then we can't say anything about its former state
        return (dbFilterLeft  && !dbFilterLeft ->passDirFilter(dirObj.getObjShortName(), NULL)) ||
               (dbFilterRight && !dbFilterRight->passDirFilter(dirObj.getObjShortName(), NULL));
    }


    template<typename Iterator, typename Function>
    friend Function std::for_each(Iterator, Iterator, Function);


    void execute(HierarchyObject& hierObj,
                 const DirContainer* dbDirectoryLeft,
                 const DirContainer* dbDirectoryRight)
    {
        //process files
        std::for_each(hierObj.useSubFiles().begin(), hierObj.useSubFiles().end(),
                      boost::bind(&RedetermineAuto::processFile, this, _1, dbDirectoryLeft, dbDirectoryRight));
        //process directories
        std::for_each(hierObj.useSubDirs().begin(), hierObj.useSubDirs().end(),
                      boost::bind(&RedetermineAuto::processDir, this, _1, dbDirectoryLeft, dbDirectoryRight));
    }


    void processFile(FileMapping& fileObj,
                     const DirContainer* dbDirectoryLeft,
                     const DirContainer* dbDirectoryRight)
    {
        const CompareFilesResult cat = fileObj.getCategory();
        if (cat == FILE_EQUAL)
            return;

        if (filterConflictFound(fileObj))
        {
            if (cat == FILE_LEFT_SIDE_ONLY)
                fileObj.setSyncDir(SYNC_DIR_RIGHT);
            else if (cat == FILE_RIGHT_SIDE_ONLY)
                fileObj.setSyncDir(SYNC_DIR_LEFT);
            else
                fileObj.setSyncDirConflict(txtFilterChanged);   //set syncDir = SYNC_DIR_INT_CONFLICT
            return;
        }

        //determine datasets for change detection
        const DataSetFile dataDbLeft  = retrieveDataSetFile(fileObj.getObjShortName(), dbDirectoryLeft);
        const DataSetFile dataDbRight = retrieveDataSetFile(fileObj.getObjShortName(), dbDirectoryRight);

        const DataSetFile dataCurrentLeft( fileObj, Loki::Int2Type<LEFT_SIDE>());
        const DataSetFile dataCurrentRight(fileObj, Loki::Int2Type<RIGHT_SIDE>());

        //evaluation
        const bool changeOnLeft  = dataDbLeft  != dataCurrentLeft;
        const bool changeOnRight = dataDbRight != dataCurrentRight;

        if (dataDbLeft == dataDbRight) //last sync seems to have been successful
        {
            if (changeOnLeft)
            {
                if (changeOnRight)
                    fileObj.setSyncDirConflict(txtBothSidesChanged);   //set syncDir = SYNC_DIR_INT_CONFLICT
                else
                    fileObj.setSyncDir(SYNC_DIR_RIGHT);
            }
            else
            {
                if (changeOnRight)
                    fileObj.setSyncDir(SYNC_DIR_LEFT);
                else
                {
                    if (cat == FILE_LEFT_SIDE_ONLY)
                        fileObj.setSyncDir(SYNC_DIR_RIGHT);
                    else if (cat == FILE_RIGHT_SIDE_ONLY)
                        fileObj.setSyncDir(SYNC_DIR_LEFT);
                    else
                        fileObj.setSyncDirConflict(txtNoSideChanged);   //set syncDir = SYNC_DIR_INT_CONFLICT
                }
            }
        }
        else //object did not complete last sync
        {
            if (changeOnLeft && changeOnRight)
                fileObj.setSyncDirConflict(txtBothSidesChanged);   //set syncDir = SYNC_DIR_INT_CONFLICT
            else
            {
                if (cat == FILE_LEFT_SIDE_ONLY)
                    fileObj.setSyncDir(SYNC_DIR_RIGHT);
                else if (cat == FILE_RIGHT_SIDE_ONLY)
                    fileObj.setSyncDir(SYNC_DIR_LEFT);
                else
                    fileObj.setSyncDirConflict(txtLastSyncFail);   //set syncDir = SYNC_DIR_INT_CONFLICT
            }
        }
    }


    void processDir(DirMapping& dirObj,
                    const DirContainer* dbDirectoryLeft,
                    const DirContainer* dbDirectoryRight)
    {
        const CompareDirResult cat = dirObj.getDirCategory();

        if (filterConflictFound(dirObj))
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
                ; //assert(false);
            }

            SetDirChangedFilter().execute(dirObj); //filter issue for this directory => treat subfiles/-dirs the same
            return;
        }

        //determine datasets for change detection
        const std::pair<DataSetDir, const DirContainer*> dataDbLeftStuff  = retrieveDataSetDir(dirObj.getObjShortName(), dbDirectoryLeft);
        const std::pair<DataSetDir, const DirContainer*> dataDbRightStuff = retrieveDataSetDir(dirObj.getObjShortName(), dbDirectoryRight);

        if (cat != DIR_EQUAL)
        {
            const DataSetDir dataCurrentLeft( dirObj, Loki::Int2Type<LEFT_SIDE>());
            const DataSetDir dataCurrentRight(dirObj, Loki::Int2Type<RIGHT_SIDE>());

            //evaluation
            const bool changeOnLeft  = dataDbLeftStuff.first  != dataCurrentLeft;
            const bool changeOnRight = dataDbRightStuff.first != dataCurrentRight;

            if (dataDbLeftStuff.first == dataDbRightStuff.first) //last sync seems to have been successful
            {
                if (changeOnLeft)
                {
                    if (changeOnRight)
                        dirObj.setSyncDirConflict(txtBothSidesChanged);   //set syncDir = SYNC_DIR_INT_CONFLICT
                    else
                        dirObj.setSyncDir(SYNC_DIR_RIGHT);
                }
                else
                {
                    if (changeOnRight)
                        dirObj.setSyncDir(SYNC_DIR_LEFT);
                    else
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
            }
            else //object did not complete last sync
            {
                if (changeOnLeft && changeOnRight)
                    dirObj.setSyncDirConflict(txtBothSidesChanged);   //set syncDir = SYNC_DIR_INT_CONFLICT
                else
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
        }

        execute(dirObj, dataDbLeftStuff.second, dataDbRightStuff.second); //recursion
    }

    const wxString txtBothSidesChanged;
    const wxString txtNoSideChanged;
    const wxString txtFilterChanged;
    const wxString txtLastSyncFail;

    const BaseFilter* dbFilterLeft;  //optional
    const BaseFilter* dbFilterRight; //optional

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
        std::for_each(hierObj.useSubDirs().begin(), hierObj.useSubDirs().end(), *this);
        //files
        std::for_each(hierObj.useSubFiles().begin(), hierObj.useSubFiles().end(), *this);
    }

private:
    template<typename Iterator, typename Function>
    friend Function std::for_each(Iterator, Iterator, Function);

    void operator()(FileMapping& fileObj) const
    {
        if (fileObj.getCategory() != FILE_EQUAL)
            fileObj.setSyncDir(newDirection_);
    }

    void operator()(DirMapping& dirObj) const
    {
        if (dirObj.getDirCategory() != DIR_EQUAL)
            dirObj.setSyncDir(newDirection_);
        execute(dirObj); //recursion
    }

    const SyncDirection newDirection_;
};


void FreeFileSync::setSyncDirectionRec(SyncDirection newDirection, FileSystemObject& fsObj)
{
    if (fsObj.getCategory() != FILE_EQUAL)
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
        std::for_each(hierObj.useSubFiles().begin(), hierObj.useSubFiles().end(), *this); //files
        std::for_each(hierObj.useSubDirs(). begin(), hierObj.useSubDirs().end(),  *this); //directories
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
    FilterData(const BaseFilter& filterProcIn) : filterProc(filterProcIn) {}

    void execute(FreeFileSync::HierarchyObject& hierObj) const
    {
        //files
        std::for_each(hierObj.useSubFiles().begin(), hierObj.useSubFiles().end(), *this);

        //directories
        std::for_each(hierObj.useSubDirs().begin(), hierObj.useSubDirs().end(), *this);
    };

private:
    template<typename Iterator, typename Function>
    friend Function std::for_each(Iterator, Iterator, Function);


    void operator()(FreeFileSync::FileMapping& fileObj) const
    {
        fileObj.setActive(filterProc.passFileFilter(fileObj.getObjRelativeName()));
    }

    void operator()(FreeFileSync::DirMapping& dirObj) const
    {
        bool subObjMightMatch = true;
        dirObj.setActive(filterProc.passDirFilter(dirObj.getObjRelativeName(), &subObjMightMatch));

        if (subObjMightMatch) //use same logic as within directory traversing here: evaluate filter in subdirs only if objects could match
            execute(dirObj);  //recursion
        else
            InOrExcludeAllRows<false>().execute(dirObj); //exclude all files dirs in subfolders
    }

    const BaseFilter& filterProc;
};


void FreeFileSync::applyFiltering(const BaseFilter& filter, FreeFileSync::BaseDirMapping& baseDirectory)
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


    const BaseFilter::FilterRef globalFilter(new NameFilter(currentMainCfg.includeFilter, currentMainCfg.excludeFilter));

    for (std::vector<FolderPairEnh>::const_iterator i = allPairs.begin(); i != allPairs.end(); ++i)
    {
        BaseDirMapping& baseDirectory = folderCmp[i - allPairs.begin()];

        applyFiltering(*combineFilters(globalFilter,
                                       BaseFilter::FilterRef(new NameFilter(
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
        while (true)
        {
            try
            {
                FileSystemObject* const fsObj = *i; //all pointers are required(!) to be bound
                if (!fsObj->isEmpty<side>())
                {
                    if (useRecycleBin)
                        FreeFileSync::moveToRecycleBin(fsObj->getFullName<side>());  //throw (FileError)
                    else
                    {
                        if (isDirectoryMapping(*fsObj))
                            FreeFileSync::removeDirectory(fsObj->getFullName<side>());
                        else
                            FreeFileSync::removeFile(fsObj->getFullName<side>());
                    }

                    fsObj->removeObject<side>(); //if directory: removes recursively!
                }
                statusHandler->deletionSuccessful(); //notify successful file/folder deletion

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


void FreeFileSync::deleteFromGridAndHD(FolderComparison& folderCmp,                         //attention: rows will be physically deleted!
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

