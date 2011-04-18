// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) 2008-2011 ZenJu (zhnmju123 AT gmx.de)                    *
// **************************************************************************
//
#include "algorithm.h"
#include <iterator>
#include <stdexcept>
#include <wx/log.h>
#include "library/resources.h"
#include "shared/file_handling.h"
#include "shared/recycler.h"
#include <wx/msgdlg.h>
#include "library/filter.h"
#include <boost/bind.hpp>
#include "shared/string_conv.h"
#include "shared/global_func.h"
#include "shared/i18n.h"
#include "shared/loki/TypeManip.h"
#include "library/db_file.h"
#include "library/cmp_filetime.h"

using namespace ffs3;


void ffs3::swapGrids(const MainConfiguration& config, FolderComparison& folderCmp)
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
        util::ProxyForEach<const Redetermine> prx(*this); //grant std::for_each access to private parts of this class

        std::for_each(hierObj.useSubFiles().begin(), hierObj.useSubFiles().end(), prx); //process files
        std::for_each(hierObj.useSubLinks().begin(), hierObj.useSubLinks().end(), prx); //process links
        std::for_each(hierObj.useSubDirs(). begin(), hierObj.useSubDirs(). end(), prx); //process directories
    }

    /*
        void execute2(FileSystemObject& fsObj) const
        {
            struct RedetermineObject : public FSObjectVisitor
            {
                RedetermineObject(const Redetermine& parent,
                                  FileSystemObject& fsObject) : parent_(parent), fsObj_(fsObject) {}

                virtual void visit(const FileMapping& fileObj)
                {
                    parent_(static_cast<FileMapping&>(fsObj_));
                }

                virtual void visit(const SymLinkMapping& linkObj)
                {
                    parent_(static_cast<SymLinkMapping&>(fsObj_));
                }

                virtual void visit(const DirMapping& dirObj)
                {
                    parent_(static_cast<DirMapping&>(fsObj_));
                }

            private:
                const Redetermine& parent_;
                FileSystemObject& fsObj_; //hack
            } redetObj(*this, fsObj);
            fsObj.accept(redetObj);
        }
    */

private:
    friend class util::ProxyForEach<const Redetermine>; //friend declaration of std::for_each is NOT sufficient as implementation is compiler dependent!

    void operator()(FileMapping& fileObj) const
    {
        switch (fileObj.getCategory())
        {
            case FILE_LEFT_SIDE_ONLY:
                if (fileObj.getFullName<LEFT_SIDE>().EndsWith(ffs3::TEMP_FILE_ENDING))
                    fileObj.setSyncDir(SYNC_DIR_LEFT); //schedule potentially existing temporary files for deletion
                else
                    fileObj.setSyncDir(config.exLeftSideOnly);
                break;
            case FILE_RIGHT_SIDE_ONLY:
                if (fileObj.getFullName<RIGHT_SIDE>().EndsWith(ffs3::TEMP_FILE_ENDING))
                    fileObj.setSyncDir(SYNC_DIR_RIGHT); //schedule potentially existing temporary files for deletion
                else
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
            case FILE_DIFFERENT_METADATA:
                fileObj.setSyncDir(config.conflict); //use setting from "conflict/cannot categorize"
                break;
        }
    }

    void operator()(SymLinkMapping& linkObj) const
    {
        switch (linkObj.getLinkCategory())
        {
            case SYMLINK_LEFT_SIDE_ONLY:
                linkObj.setSyncDir(config.exLeftSideOnly);
                break;
            case SYMLINK_RIGHT_SIDE_ONLY:
                linkObj.setSyncDir(config.exRightSideOnly);
                break;
            case SYMLINK_LEFT_NEWER:
                linkObj.setSyncDir(config.leftNewer);
                break;
            case SYMLINK_RIGHT_NEWER:
                linkObj.setSyncDir(config.rightNewer);
                break;
            case SYMLINK_CONFLICT:
                linkObj.setSyncDir(config.conflict);
                break;
            case SYMLINK_DIFFERENT:
                linkObj.setSyncDir(config.different);
                break;
            case SYMLINK_EQUAL:
                linkObj.setSyncDir(SYNC_DIR_NONE);
                break;
            case SYMLINK_DIFFERENT_METADATA:
                linkObj.setSyncDir(config.conflict); //use setting from "conflict/cannot categorize"
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
            case DIR_DIFFERENT_METADATA:
                dirObj.setSyncDir(config.conflict); //use setting from "conflict/cannot categorize"
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
        return std::find_if(hierObj.useSubFiles().begin(), hierObj.useSubFiles().end(), *this) != hierObj.useSubFiles().end()  || //files
               std::find_if(hierObj.useSubLinks().begin(), hierObj.useSubLinks().end(), *this) != hierObj.useSubLinks(). end() || //symlinks
               std::find_if(hierObj.useSubDirs(). begin(), hierObj.useSubDirs(). end(), *this) != hierObj.useSubDirs(). end();    //directories
    }

    //logical private! => __find_if (used by std::find_if) needs public access
    bool operator()(const FileMapping& fileObj) const
    {
        return fileObj.getCategory() != FILE_EQUAL;
    }

    bool operator()(const SymLinkMapping& linkObj) const
    {
        return linkObj.getLinkCategory() != SYMLINK_EQUAL;
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


bool ffs3::allElementsEqual(const FolderComparison& folderCmp)
{
    return std::find_if(folderCmp.begin(), folderCmp.end(), std::not1(AllElementsEqual())) == folderCmp.end();
}
//---------------------------------------------------------------------------------------------------------------

class DataSetFile
{
public:
    DataSetFile() {}

    DataSetFile(const Zstring& name, const FileDescriptor& fileDescr)
    {
        shortName     = name;
        lastWriteTime = fileDescr.lastWriteTimeRaw;
        fileSize      = fileDescr.fileSize;
    }

    DataSetFile(const FileMapping& fileObj, Loki::Int2Type<LEFT_SIDE>)
    {
        init<LEFT_SIDE>(fileObj);
    }

    DataSetFile(const FileMapping& fileObj, Loki::Int2Type<RIGHT_SIDE>)
    {
        init<RIGHT_SIDE>(fileObj);
    }

    bool operator==(const DataSetFile& other) const
    {
        if (shortName.empty())
            return other.shortName.empty();
        else
        {
            if (other.shortName.empty())
                return false;
            else
            {
                return shortName == other.shortName && //detect changes in case (windows)
                       //respect 2 second FAT/FAT32 precision! copying a file to a FAT32 drive changes it's modification date by up to 2 seconds
                       sameFileTime(lastWriteTime, other.lastWriteTime, 2) &&
                       fileSize == other.fileSize;
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
            shortName     = fileObj.getShortName<side>();
            lastWriteTime = fileObj.getLastWriteTime<side>();
            fileSize      = fileObj.getFileSize<side>();
        }
    }

    Zstring     shortName;     //empty if object not existing
    wxLongLong  lastWriteTime;
    wxULongLong fileSize;
};


//--------------------------------------------------------------------
class DataSetSymlink
{
public:
    DataSetSymlink()
#ifdef FFS_WIN
        : type(LinkDescriptor::TYPE_FILE) //dummy value
#endif
    {}

    DataSetSymlink(const Zstring& name, const LinkDescriptor& linkDescr)
    {
        shortName     = name;
        lastWriteTime = linkDescr.lastWriteTimeRaw;
        targetPath    = linkDescr.targetPath;
#ifdef FFS_WIN //type of symbolic link is relevant for Windows only
        type          = linkDescr.type;
#endif
    }

    DataSetSymlink(const SymLinkMapping& linkObj, Loki::Int2Type<LEFT_SIDE>)
    {
        init<LEFT_SIDE>(linkObj);
    }

    DataSetSymlink(const SymLinkMapping& linkObj, Loki::Int2Type<RIGHT_SIDE>)
    {
        init<RIGHT_SIDE>(linkObj);
    }

    bool operator==(const DataSetSymlink& other) const
    {
        if (shortName.empty()) //test if object is existing at all
            return other.shortName.empty();
        else
        {
            if (other.shortName.empty())
                return false;
            else
            {
                return shortName == other.shortName &&
                       //respect 2 second FAT/FAT32 precision! copying a file to a FAT32 drive changes it's modification date by up to 2 seconds
                       sameFileTime(lastWriteTime, other.lastWriteTime, 2) &&
#ifdef FFS_WIN //comparison of symbolic link type is relevant for Windows only
                       type == other.type &&
#endif
                       targetPath == other.targetPath;
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
    void init(const SymLinkMapping& linkObj)
    {
#ifdef FFS_WIN
        type = LinkDescriptor::TYPE_FILE; //always initialize
#endif

        if (!linkObj.isEmpty<side>())
        {
            shortName     = linkObj.getShortName<side>();
            lastWriteTime = linkObj.getLastWriteTime<side>();
            targetPath    = linkObj.getTargetPath<side>();
#ifdef FFS_WIN //type of symbolic link is relevant for Windows only
            type          = linkObj.getLinkType<side>();
#endif
        }
    }

    Zstring    shortName;     //empty if object not existing
    wxLongLong lastWriteTime;
    Zstring    targetPath;
#ifdef FFS_WIN
    LinkDescriptor::LinkType type;
#endif
};


//--------------------------------------------------------------------
class DataSetDir
{
public:
    DataSetDir() {}

    DataSetDir(const Zstring& name) :
        shortName(name) {}

    DataSetDir(const DirMapping& dirObj, Loki::Int2Type<LEFT_SIDE>) :
        shortName(dirObj.getShortName<LEFT_SIDE>()) {}

    DataSetDir(const DirMapping& dirObj, Loki::Int2Type<RIGHT_SIDE>) :
        shortName(dirObj.getShortName<RIGHT_SIDE>()) {}

    bool operator==(const DataSetDir& other) const
    {
        return shortName == other.shortName;
    }

    template <class T>
    bool operator!=(const T& other) const
    {
        return !(*this == other);
    }

private:
    Zstring shortName; //empty if object not existing
};


//--------------------------------------------------------------------------------------------------------
DataSetFile retrieveDataSetFile(const Zstring& objShortName, const DirContainer* dbDirectory)
{
    if (dbDirectory)
    {
        DirContainer::FileList::const_iterator iter = dbDirectory->files.find(objShortName);
        if (iter != dbDirectory->files.end())
            return DataSetFile(iter->first, iter->second);
    }

    return DataSetFile(); //object not found
}

DataSetSymlink retrieveDataSetSymlink(const Zstring& objShortName, const DirContainer* dbDirectory)
{
    if (dbDirectory)
    {
        DirContainer::LinkList::const_iterator iter = dbDirectory->links.find(objShortName);
        if (iter != dbDirectory->links.end())
            return DataSetSymlink(iter->first, iter->second);
    }

    return DataSetSymlink(); //object not found
}


std::pair<DataSetDir, const DirContainer*> retrieveDataSetDir(const Zstring& objShortName, const DirContainer* dbDirectory)
{
    if (dbDirectory)
    {
        DirContainer::DirList::const_iterator iter = dbDirectory->dirs.find(objShortName);
        if (iter != dbDirectory->dirs.end())
            return std::make_pair(DataSetDir(iter->first), &iter->second);
    }

    return std::make_pair(DataSetDir(), static_cast<const DirContainer*>(NULL)); //object not found
}


//--------------------------------------------------------------------------------------------------------
/*
class SetDirChangedFilter
{
public:
    SetDirChangedFilter() :
        txtFilterChanged(wxString(_("Cannot determine sync-direction:")) + wxT(" \n") + _("Filter settings have changed!")) {}

    void execute(HierarchyObject& hierObj) const
    {
        util::ProxyForEach<const SetDirChangedFilter> prx(*this); //grant std::for_each access to private parts of this class

        std::for_each(hierObj.useSubFiles().begin(), hierObj.useSubFiles().end(), prx); //process files
        std::for_each(hierObj.useSubLinks().begin(), hierObj.useSubLinks().end(), prx); //process links
        std::for_each(hierObj.useSubDirs().begin(),  hierObj.useSubDirs().end(),  prx); //process directories
    }

private:
    friend class util::ProxyForEach<const SetDirChangedFilter>; //friend declaration of std::for_each is NOT sufficient as implementation is compiler dependent!

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

    void operator()(SymLinkMapping& linkObj) const
    {
        const CompareSymlinkResult cat = linkObj.getLinkCategory();

        if (cat == SYMLINK_EQUAL)
            return;

        if (cat == SYMLINK_LEFT_SIDE_ONLY)
            linkObj.setSyncDir(SYNC_DIR_RIGHT);
        else if (cat == SYMLINK_RIGHT_SIDE_ONLY)
            linkObj.setSyncDir(SYNC_DIR_LEFT);
        else
            linkObj.setSyncDirConflict(txtFilterChanged);   //set syncDir = SYNC_DIR_INT_CONFLICT
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
*/

//test whether planned deletion of a directory is in conflict with (direct!) sub-elements that are not categorized for deletion (e.g. shall be copied or are in conflict themselves)
class FindDeleteDirConflictNonRec
{
public:
    bool conflictFound(const HierarchyObject& hierObj) const
    {
        return std::find_if(hierObj.useSubFiles().begin(), hierObj.useSubFiles().end(), *this) != hierObj.useSubFiles().end() || //files
               std::find_if(hierObj.useSubLinks().begin(), hierObj.useSubLinks().end(), *this) != hierObj.useSubLinks().end() || //symlinks
               std::find_if(hierObj.useSubDirs(). begin(), hierObj.useSubDirs(). end(), *this) != hierObj.useSubDirs(). end();   //directories
    }

    //logical private! => __find_if (used by std::find_if) needs public access
    bool operator()(const FileSystemObject& fsObj) const
    {
        switch (fsObj.getSyncOperation())
        {
            case SO_CREATE_NEW_LEFT:
            case SO_CREATE_NEW_RIGHT:
            case SO_UNRESOLVED_CONFLICT:
                return true;

            case SO_DELETE_LEFT:
            case SO_DELETE_RIGHT:
            case SO_OVERWRITE_LEFT:
            case SO_OVERWRITE_RIGHT:
            case SO_DO_NOTHING:
            case SO_EQUAL:
            case SO_COPY_METADATA_TO_LEFT:
            case SO_COPY_METADATA_TO_RIGHT:
                ;
        }
        return false;
    }
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
        txtLastSyncFail(wxString(_("Cannot determine sync-direction:")) + wxT(" \n") + _("The file was not processed by last synchronization!")),
        txtDirDeleteConflict(_("Planned directory deletion is in conflict with its subdirectories and -files!")),
        dbFilterLeft(NULL),
        dbFilterRight(NULL),
        handler_(handler)
    {
        if (AllElementsEqual()(baseDirectory)) //nothing to do: abort and don't show any nag-screens
            return;

        //try to load sync-database files
        std::pair<DirInfoPtr, DirInfoPtr> dirInfo = loadDBFile(baseDirectory);
        if (dirInfo.first.get()  == NULL ||
            dirInfo.second.get() == NULL)
        {
            //use standard settings:
            SyncConfiguration defaultSync;
            ffs3::setTwoWay(defaultSync);
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
        catch (FileErrorDatabaseNotExisting&) {} //let's ignore these errors for now...
        catch (FileError& error) //e.g. incompatible database version
        {
            if (handler_) handler_->reportWarning(error.msg() + wxT(" \n\n") +
                                                      _("Setting default synchronization directions: Old files will be overwritten with newer files."));
        }
        return std::pair<DirInfoPtr, DirInfoPtr>(); //NULL
    }

    /*
        bool filterFileConflictFound(const Zstring& relativeName) const
        {
            //if filtering would have excluded file during database creation, then we can't say anything about its former state
            return (dbFilterLeft  && !dbFilterLeft ->passFileFilter(relativeName)) ||
                   (dbFilterRight && !dbFilterRight->passFileFilter(relativeName));
        }


        bool filterDirConflictFound(const Zstring& relativeName) const
        {
            //if filtering would have excluded directory during database creation, then we can't say anything about its former state
            return (dbFilterLeft  && !dbFilterLeft ->passDirFilter(relativeName, NULL)) ||
                   (dbFilterRight && !dbFilterRight->passDirFilter(relativeName, NULL));
        }
    */

    template<typename Iterator, typename Function>
    friend Function std::for_each(Iterator, Iterator, Function);


    void execute(HierarchyObject& hierObj,
                 const DirContainer* dbDirectoryLeft,
                 const DirContainer* dbDirectoryRight)
    {
        //process files
        std::for_each(hierObj.useSubFiles().begin(), hierObj.useSubFiles().end(),
                      boost::bind(&RedetermineAuto::processFile, this, _1, dbDirectoryLeft, dbDirectoryRight));
        //process symbolic links
        std::for_each(hierObj.useSubLinks().begin(), hierObj.useSubLinks().end(),
                      boost::bind(&RedetermineAuto::processSymlink, this, _1, dbDirectoryLeft, dbDirectoryRight));
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


        //##################### schedule potentially existing temporary files for deletion ####################
        if (cat == FILE_LEFT_SIDE_ONLY && fileObj.getFullName<LEFT_SIDE>().EndsWith(ffs3::TEMP_FILE_ENDING))
        {
            fileObj.setSyncDir(SYNC_DIR_LEFT);
            return;
        }
        else if (cat == FILE_RIGHT_SIDE_ONLY && fileObj.getFullName<RIGHT_SIDE>().EndsWith(ffs3::TEMP_FILE_ENDING))
        {
            fileObj.setSyncDir(SYNC_DIR_RIGHT);
            return;
        }
        //#####################################################################################################

        /*
                if (filterFileConflictFound(fileObj.getObjRelativeName()))
                {
                    if (cat == FILE_LEFT_SIDE_ONLY)
                        fileObj.setSyncDir(SYNC_DIR_RIGHT);
                    else if (cat == FILE_RIGHT_SIDE_ONLY)
                        fileObj.setSyncDir(SYNC_DIR_LEFT);
                    else
                        fileObj.setSyncDirConflict(txtFilterChanged);   //set syncDir = SYNC_DIR_INT_CONFLICT
                    return;
                }
                */

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
                    fileObj.setSyncDirConflict(txtNoSideChanged);   //set syncDir = SYNC_DIR_INT_CONFLICT
            }
        }
        else //object did not complete last sync
        {
            if (changeOnLeft && changeOnRight)
                fileObj.setSyncDirConflict(txtBothSidesChanged);   //set syncDir = SYNC_DIR_INT_CONFLICT
            else
            {
                //                if (cat == FILE_LEFT_SIDE_ONLY)
                //                    fileObj.setSyncDir(SYNC_DIR_RIGHT);
                //                else if (cat == FILE_RIGHT_SIDE_ONLY)
                //                    fileObj.setSyncDir(SYNC_DIR_LEFT);
                //                else
                fileObj.setSyncDirConflict(txtLastSyncFail);   //set syncDir = SYNC_DIR_INT_CONFLICT
            }
        }
    }


    void processSymlink(SymLinkMapping& linkObj,
                        const DirContainer* dbDirectoryLeft,
                        const DirContainer* dbDirectoryRight)
    {
        const CompareSymlinkResult cat = linkObj.getLinkCategory();
        if (cat == SYMLINK_EQUAL)
            return;

        /*
                if (filterFileConflictFound(linkObj.getObjRelativeName())) //always use file filter: Link type may not be "stable" on Linux!
                {
                    if (cat == SYMLINK_LEFT_SIDE_ONLY)
                        linkObj.setSyncDir(SYNC_DIR_RIGHT);
                    else if (cat == SYMLINK_RIGHT_SIDE_ONLY)
                        linkObj.setSyncDir(SYNC_DIR_LEFT);
                    else
                        linkObj.setSyncDirConflict(txtFilterChanged);   //set syncDir = SYNC_DIR_INT_CONFLICT
                    return;
                }
                */

        //determine datasets for change detection
        const DataSetSymlink dataDbLeft  = retrieveDataSetSymlink(linkObj.getObjShortName(), dbDirectoryLeft);
        const DataSetSymlink dataDbRight = retrieveDataSetSymlink(linkObj.getObjShortName(), dbDirectoryRight);

        const DataSetSymlink dataCurrentLeft( linkObj, Loki::Int2Type<LEFT_SIDE>());
        const DataSetSymlink dataCurrentRight(linkObj, Loki::Int2Type<RIGHT_SIDE>());

        //evaluation
        const bool changeOnLeft  = dataDbLeft  != dataCurrentLeft;
        const bool changeOnRight = dataDbRight != dataCurrentRight;

        if (dataDbLeft == dataDbRight) //last sync seems to have been successful
        {
            if (changeOnLeft)
            {
                if (changeOnRight)
                    linkObj.setSyncDirConflict(txtBothSidesChanged);   //set syncDir = SYNC_DIR_INT_CONFLICT
                else
                    linkObj.setSyncDir(SYNC_DIR_RIGHT);
            }
            else
            {
                if (changeOnRight)
                    linkObj.setSyncDir(SYNC_DIR_LEFT);
                else
                    linkObj.setSyncDirConflict(txtNoSideChanged);   //set syncDir = SYNC_DIR_INT_CONFLICT
            }
        }
        else //object did not complete last sync
        {
            if (changeOnLeft && changeOnRight)
                linkObj.setSyncDirConflict(txtBothSidesChanged);   //set syncDir = SYNC_DIR_INT_CONFLICT
            else
                linkObj.setSyncDirConflict(txtLastSyncFail);   //set syncDir = SYNC_DIR_INT_CONFLICT
        }
    }


    void processDir(DirMapping& dirObj,
                    const DirContainer* dbDirectoryLeft,
                    const DirContainer* dbDirectoryRight)
    {
        const CompareDirResult cat = dirObj.getDirCategory();

        /*
                if (filterDirConflictFound(dirObj.getObjRelativeName()))
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
                        ;
                    }

                    SetDirChangedFilter().execute(dirObj); //filter issue for this directory => treat subfiles/-dirs the same
                    return;
                }
        */
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
                        assert(false);
                        dirObj.setSyncDirConflict(txtNoSideChanged);   //set syncDir = SYNC_DIR_INT_CONFLICT
                    }
                }
            }
            else //object did not complete last sync
            {
                if (changeOnLeft && changeOnRight)
                    dirObj.setSyncDirConflict(txtBothSidesChanged);   //set syncDir = SYNC_DIR_INT_CONFLICT
                else
                {
                    //                    switch (cat)
                    //                    {
                    //                    case DIR_LEFT_SIDE_ONLY:
                    //                        dirObj.setSyncDir(SYNC_DIR_RIGHT);
                    //                        break;
                    //                    case DIR_RIGHT_SIDE_ONLY:
                    //                        dirObj.setSyncDir(SYNC_DIR_LEFT);
                    //                        break;
                    //                    case DIR_EQUAL:
                    //                        assert(false);
                    //                    }

                    dirObj.setSyncDirConflict(txtLastSyncFail);   //set syncDir = SYNC_DIR_INT_CONFLICT
                }
            }
        }

        execute(dirObj, dataDbLeftStuff.second, dataDbRightStuff.second); //recursion
        //###################################################################################################

        //if a directory is to be deleted on one side, ensure that directions of sub-elements are "d’accord"
        const SyncOperation syncOp = dirObj.getSyncOperation();
        if (syncOp == SO_DELETE_LEFT ||
            syncOp == SO_DELETE_RIGHT)
        {
            if (FindDeleteDirConflictNonRec().conflictFound(dirObj))
                dirObj.setSyncDirConflict(txtDirDeleteConflict);
        }
    }

    const wxString txtBothSidesChanged;
    const wxString txtNoSideChanged;
    const wxString txtFilterChanged;
    const wxString txtLastSyncFail;
    const wxString txtDirDeleteConflict;

    const BaseFilter* dbFilterLeft;  //optional
    const BaseFilter* dbFilterRight; //optional

    DeterminationProblem* const handler_;
};


//---------------------------------------------------------------------------------------------------------------
void ffs3::redetermineSyncDirection(const SyncConfiguration& config, BaseDirMapping& baseDirectory, DeterminationProblem* handler)
{
    if (config.automatic)
        RedetermineAuto(baseDirectory, handler);
    else
        Redetermine(config).execute(baseDirectory);
}


void ffs3::redetermineSyncDirection(const MainConfiguration& currentMainCfg, FolderComparison& folderCmp, DeterminationProblem* handler)
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
    SetNewDirection(SyncDirection newDirection) : newDirection_(newDirection) {}

    void operator()(FileMapping& fileObj) const
    {
        if (fileObj.getCategory() != FILE_EQUAL)
            fileObj.setSyncDir(newDirection_);
    }

    void operator()(SymLinkMapping& linkObj) const
    {
        if (linkObj.getLinkCategory() != SYMLINK_EQUAL)
            linkObj.setSyncDir(newDirection_);
    }

    void operator()(DirMapping& dirObj) const
    {
        if (dirObj.getDirCategory() != DIR_EQUAL)
            dirObj.setSyncDir(newDirection_);
        execute(dirObj); //recursion
    }

private:
    friend class util::ProxyForEach<const SetNewDirection>; //friend declaration of std::for_each is NOT sufficient as implementation is compiler dependent!

    void execute(HierarchyObject& hierObj) const
    {
        util::ProxyForEach<const SetNewDirection> prx(*this); //grant std::for_each access to private parts of this class

        std::for_each(hierObj.useSubFiles().begin(), hierObj.useSubFiles().end(), prx); //process files
        std::for_each(hierObj.useSubLinks().begin(), hierObj.useSubLinks().end(), prx); //process links
        std::for_each(hierObj.useSubDirs().begin(),  hierObj.useSubDirs().end(),  prx); //process directories
    }

    const SyncDirection newDirection_;
};


void ffs3::setSyncDirectionRec(SyncDirection newDirection, FileSystemObject& fsObj)
{
    SetNewDirection dirSetter(newDirection);

    //process subdirectories also!
    struct Recurse: public FSObjectVisitor
    {
        Recurse(const SetNewDirection& ds) : dirSetter_(ds) {}
        virtual void visit(const FileMapping& fileObj)
        {
            dirSetter_(const_cast<FileMapping&>(fileObj)); //phyiscal object is not const in this method anyway
        }
        virtual void visit(const SymLinkMapping& linkObj)
        {
            dirSetter_(const_cast<SymLinkMapping&>(linkObj)); //phyiscal object is not const in this method anyway
        }
        virtual void visit(const DirMapping& dirObj)
        {
            dirSetter_(const_cast<DirMapping&>(dirObj)); //phyiscal object is not const in this method anyway
        }
    private:
        const SetNewDirection& dirSetter_;
    } recurse(dirSetter);
    fsObj.accept(recurse);
}


//--------------- functions related to filtering ------------------------------------------------------------------------------------

template <bool include>
class InOrExcludeAllRows
{
public:
    void operator()(ffs3::BaseDirMapping& baseDirectory) const //be careful with operator() to no get called by std::for_each!
    {
        execute(baseDirectory);
    }

    void execute(ffs3::HierarchyObject& hierObj) const //don't create ambiguity by replacing with operator()
    {
        util::ProxyForEach<const InOrExcludeAllRows<include> > prx(*this); //grant std::for_each access to private parts of this class

        std::for_each(hierObj.useSubFiles().begin(), hierObj.useSubFiles().end(), prx); //process files
        std::for_each(hierObj.useSubLinks().begin(), hierObj.useSubLinks().end(), prx); //process links
        std::for_each(hierObj.useSubDirs().begin(),  hierObj.useSubDirs().end(),  prx); //process directories
    }

private:
    friend class util::ProxyForEach<const InOrExcludeAllRows<include> >; //friend declaration of std::for_each is NOT sufficient as implementation is compiler dependent!

    void operator()(ffs3::FileMapping& fileObj) const
    {
        fileObj.setActive(include);
    }

    void operator()(ffs3::SymLinkMapping& linkObj) const
    {
        linkObj.setActive(include);
    }

    void operator()(ffs3::DirMapping& dirObj) const
    {
        dirObj.setActive(include);
        execute(dirObj); //recursion
    }
};


void ffs3::setActiveStatus(bool newStatus, ffs3::FolderComparison& folderCmp)
{
    if (newStatus)
        std::for_each(folderCmp.begin(), folderCmp.end(), InOrExcludeAllRows<true>());  //include all rows
    else
        std::for_each(folderCmp.begin(), folderCmp.end(), InOrExcludeAllRows<false>()); //exclude all rows
}


void ffs3::setActiveStatus(bool newStatus, ffs3::FileSystemObject& fsObj)
{
    fsObj.setActive(newStatus);

    //process subdirectories also!
        struct Recurse: public FSObjectVisitor
        {
            Recurse(bool newStat) : newStatus_(newStat) {}
            virtual void visit(const FileMapping& fileObj) {}
            virtual void visit(const SymLinkMapping& linkObj) {}
            virtual void visit(const DirMapping& dirObj)
            {
                if (newStatus_)
                    InOrExcludeAllRows<true>().execute(const_cast<DirMapping&>(dirObj)); //object is not physically const here anyway
                else
                    InOrExcludeAllRows<false>().execute(const_cast<DirMapping&>(dirObj)); //
            }
        private:
            const bool newStatus_;
        } recurse(newStatus);
        fsObj.accept(recurse);
}

namespace
{
enum FilterStrategy
{
    STRATEGY_ALL,
    STRATEGY_ACTIVE_ONLY
    //STRATEGY_INACTIVE_ONLY -> logical conflict with InOrExcludeAllRows<false>
};


template <FilterStrategy strategy>
class FilterData
{
public:
    FilterData(const BaseFilter& filterProcIn) : filterProc(filterProcIn) {}

    void execute(ffs3::HierarchyObject& hierObj) const
    {
        util::ProxyForEach<const FilterData> prx(*this); //grant std::for_each access to private parts of this class

        std::for_each(hierObj.useSubFiles().begin(), hierObj.useSubFiles().end(), prx); //files
        std::for_each(hierObj.useSubLinks().begin(), hierObj.useSubLinks().end(), prx); //symlinks
        std::for_each(hierObj.useSubDirs(). begin(), hierObj.useSubDirs(). end(), prx); //directories
    };

private:
    friend class util::ProxyForEach<const FilterData>; //friend declaration of std::for_each is NOT sufficient as implementation is compiler dependent!

    bool processObject(ffs3::FileSystemObject& fsObj) const;

    void operator()(ffs3::FileMapping& fileObj) const
    {
        if (processObject(fileObj))
            fileObj.setActive(filterProc.passFileFilter(fileObj.getObjRelativeName()));
    }

    void operator()(ffs3::SymLinkMapping& linkObj) const
    {
        if (processObject(linkObj))
            linkObj.setActive(filterProc.passFileFilter(linkObj.getObjRelativeName()));
    }

    void operator()(ffs3::DirMapping& dirObj) const
    {
        bool subObjMightMatch = true;
        const bool filterPassed = filterProc.passDirFilter(dirObj.getObjRelativeName(), &subObjMightMatch);

        if (processObject(dirObj))
            dirObj.setActive(filterPassed);

        if (!subObjMightMatch) //use same logic like directory traversing here: evaluate filter in subdirs only if objects could match
        {
            InOrExcludeAllRows<false>().execute(dirObj); //exclude all files dirs in subfolders
            return;
        }

        execute(dirObj);  //recursion
    }

    const BaseFilter& filterProc;
};


template <> //process all elements
inline
bool FilterData<STRATEGY_ALL>::processObject(ffs3::FileSystemObject& fsObj) const
{
    return true;
}

template <>
inline
bool FilterData<STRATEGY_ACTIVE_ONLY>::processObject(ffs3::FileSystemObject& fsObj) const
{
    return fsObj.isActive();
}
}


void ffs3::addExcludeFiltering(const Zstring& excludeFilter, FolderComparison& folderCmp)
{
    for (std::vector<BaseDirMapping>::iterator i = folderCmp.begin(); i != folderCmp.end(); ++i)
        FilterData<STRATEGY_ACTIVE_ONLY>(*BaseFilter::FilterRef(new NameFilter(Zstr("*"), excludeFilter))).execute(*i);
}


void ffs3::applyFiltering(const MainConfiguration& currentMainCfg, FolderComparison& folderCmp)
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


    const BaseFilter::FilterRef globalFilter(new NameFilter(currentMainCfg.globalFilter.includeFilter, currentMainCfg.globalFilter.excludeFilter));

    for (std::vector<FolderPairEnh>::const_iterator i = allPairs.begin(); i != allPairs.end(); ++i)
    {
        BaseDirMapping& baseDirectory = folderCmp[i - allPairs.begin()];

        FilterData<STRATEGY_ALL>(*combineFilters(globalFilter,
                                                 BaseFilter::FilterRef(new NameFilter(
                                                                           i->localFilter.includeFilter,
                                                                           i->localFilter.excludeFilter)))).execute(baseDirectory);
    }
}


//############################################################################################################
std::pair<wxString, int> ffs3::deleteFromGridAndHDPreview( //assemble message containing all files to be deleted
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


namespace
{
struct RemoveCallbackImpl : public ffs3::CallbackRemoveDir
{
    RemoveCallbackImpl(DeleteFilesHandler& deleteCallback) : deleteCallback_(deleteCallback) {}

    virtual void requestUiRefresh(const Zstring& currentObject)
    {
        deleteCallback_.deletionSuccessful(0);
    }

private:
    DeleteFilesHandler& deleteCallback_;
};
}


template <SelectedSide side, class InputIterator>
void deleteFromGridAndHDOneSide(InputIterator first, InputIterator last,
                                const bool useRecycleBin,
                                DeleteFilesHandler& statusHandler)
{
    for (InputIterator i = first; i != last; ++i)
    {
        while (true)
        {
            try
            {
                FileSystemObject* const fsObj = *i; //all pointers are required(!) to be bound
                if (!fsObj->isEmpty<side>()) //element may become implicitly delted, e.g. if parent folder was deleted first
                {
                    if (useRecycleBin)
                        ffs3::moveToRecycleBin(fsObj->getFullName<side>());  //throw (FileError)
                    else
                    {
                        RemoveCallbackImpl removeCallback(statusHandler);

                        //del directories and symlinks
                        struct DeletePermanently : public FSObjectVisitor
                        {
                            DeletePermanently(RemoveCallbackImpl* remCallback) : remCallback_(remCallback) {}

                            virtual void visit(const FileMapping& fileObj)
                            {
                                ffs3::removeFile(fileObj.getFullName<side>());
                            }

                            virtual void visit(const SymLinkMapping& linkObj)
                            {
                                switch (linkObj.getLinkType<side>())
                                {
                                    case LinkDescriptor::TYPE_DIR:
                                        ffs3::removeDirectory(linkObj.getFullName<side>(), NULL);
                                        break;
                                    case LinkDescriptor::TYPE_FILE:
                                        ffs3::removeFile(linkObj.getFullName<side>());
                                        break;
                                }
                            }

                            virtual void visit(const DirMapping& dirObj)
                            {
                                ffs3::removeDirectory(dirObj.getFullName<side>(), remCallback_);
                            }

                        private:
                            RemoveCallbackImpl* remCallback_;
                        } delPerm(&removeCallback);
                        fsObj->accept(delPerm);
                    }

                    fsObj->removeObject<side>(); //if directory: removes recursively!

                    //update sync direction: as this is a synchronization tool, the user most likely wants to delete the other side, too!
                    if (side == LEFT_SIDE)
                        setSyncDirectionRec(SYNC_DIR_RIGHT, *fsObj); //set new direction (recursively)
                    else
                        setSyncDirectionRec(SYNC_DIR_LEFT, *fsObj);
                }

                //notify successful file/folder deletion: make sure [first, last) contains to be deleted entries only, to avoid miscalculation
                statusHandler.deletionSuccessful(1);
                break;
            }
            catch (const FileError& error)
            {
                DeleteFilesHandler::Response rv = statusHandler.reportError(error.msg());

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
    FinalizeDeletion(FolderComparison& folderCmp) : folderCmp_(folderCmp) {}

    ~FinalizeDeletion()
    {
        std::for_each(folderCmp_.begin(), folderCmp_.end(), FileSystemObject::removeEmpty);

        //redetermineSyncDirection(mainConfig_, folderCmp_, NULL);
    }

private:
    FolderComparison& folderCmp_;
};


void ffs3::deleteFromGridAndHD(FolderComparison& folderCmp,                         //attention: rows will be physically deleted!
                               std::vector<FileSystemObject*>& rowsToDeleteOnLeft,  //refresh GUI grid after deletion to remove invalid rows
                               std::vector<FileSystemObject*>& rowsToDeleteOnRight, //all pointers need to be bound!
                               const bool deleteOnBothSides,
                               const bool useRecycleBin,
                               DeleteFilesHandler& statusHandler)
{
    FinalizeDeletion dummy(folderCmp); //ensure cleanup: redetermination of sync-directions and removal of invalid rows

    std::set<FileSystemObject*> deleteLeft;
    std::set<FileSystemObject*> deleteRight;

    if (deleteOnBothSides)
    {
        //mix selected rows from left and right (and remove duplicates)
        std::set<FileSystemObject*> tmp(rowsToDeleteOnLeft.begin(), rowsToDeleteOnLeft.end());
        tmp.insert(rowsToDeleteOnRight.begin(), rowsToDeleteOnRight.end());

        std::remove_copy_if(tmp.begin(), tmp.end(),
                            std::inserter(deleteLeft, deleteLeft.begin()), std::mem_fun(&FileSystemObject::isEmpty<LEFT_SIDE>)); //remove empty rows to ensure correct statistics

        std::remove_copy_if(tmp.begin(), tmp.end(),
                            std::inserter(deleteRight, deleteRight.begin()), std::mem_fun(&FileSystemObject::isEmpty<RIGHT_SIDE>));
    }
    else
    {
        std::remove_copy_if(rowsToDeleteOnLeft.begin(), rowsToDeleteOnLeft.end(),
                            std::inserter(deleteLeft, deleteLeft.begin()), std::mem_fun(&FileSystemObject::isEmpty<LEFT_SIDE>)); //remove empty rows to ensure correct statistics

        std::remove_copy_if(rowsToDeleteOnRight.begin(), rowsToDeleteOnRight.end(),
                            std::inserter(deleteRight, deleteRight.begin()), std::mem_fun(&FileSystemObject::isEmpty<RIGHT_SIDE>));
    }

    deleteFromGridAndHDOneSide<LEFT_SIDE>(deleteLeft.begin(), deleteLeft.end(),
                                          useRecycleBin,
                                          statusHandler);

    deleteFromGridAndHDOneSide<RIGHT_SIDE>(deleteRight.begin(), deleteRight.end(),
                                           useRecycleBin,
                                           statusHandler);
}


//############################################################################################################
/*Statistical theory: detect daylight saving time (DST) switch by comparing files that exist on both sides (and have same filesizes). If there are "enough"
that have a shift by +-1h then assert that DST switch occurred.
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


void ffs3::checkForDSTChange(const FileCompareResult& gridData,
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

                if (leftFile.objType == FileDescrLine::TYPE_FILE && rightFile.objType == FileDescrLine::TYPE_FILE &&
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

