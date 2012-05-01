// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) ZenJu (zhnmju123 AT gmx DOT de) - All Rights Reserved    *
// **************************************************************************

#include "algorithm.h"
#include <set>
#include <iterator>
#include <stdexcept>
#include <tuple>
#include "lib/resources.h"
#include <zen/file_handling.h>
#include <zen/recycler.h>
#include <wx/msgdlg.h>
#include "lib/norm_filter.h"
#include <wx+/string_conv.h>
#include "lib/db_file.h"
#include <zen/scope_guard.h>
#include "lib/cmp_filetime.h"
#include <zen/stl_tools.h>
#include "lib/norm_filter.h"

using namespace zen;
using namespace std::rel_ops;


void zen::swapGrids(const MainConfiguration& config, FolderComparison& folderCmp)
{
    std::for_each(begin(folderCmp), end(folderCmp), std::mem_fun_ref(&BaseDirMapping::flip));
    redetermineSyncDirection(config, folderCmp, [](const std::wstring&) {});
}


//----------------------------------------------------------------------------------------------
class Redetermine
{
public:
    static void execute(const DirectionSet& dirCfgIn, HierarchyObject& hierObj)
    {
        Redetermine(dirCfgIn).recurse(hierObj);
    }

private:
    Redetermine(const DirectionSet& dirCfgIn) : dirCfg(dirCfgIn) {}

    void recurse(HierarchyObject& hierObj) const
    {
        std::for_each(hierObj.refSubFiles().begin(), hierObj.refSubFiles().end(), [&](FileMapping&    fileMap) { (*this)(fileMap); });
        std::for_each(hierObj.refSubLinks().begin(), hierObj.refSubLinks().end(), [&](SymLinkMapping& linkMap) { (*this)(linkMap); });
        std::for_each(hierObj.refSubDirs ().begin(), hierObj.refSubDirs ().end(), [&](DirMapping&      dirMap) { (*this)(dirMap); });
    }

    void operator()(FileMapping& fileObj) const
    {
        switch (fileObj.getCategory())
        {
            case FILE_LEFT_SIDE_ONLY:
                if (endsWith(fileObj.getShortName<LEFT_SIDE>(), zen::TEMP_FILE_ENDING))
                    fileObj.setSyncDir(SYNC_DIR_LEFT); //schedule potentially existing temporary files for deletion
                else
                    fileObj.setSyncDir(dirCfg.exLeftSideOnly);
                break;
            case FILE_RIGHT_SIDE_ONLY:
                if (endsWith(fileObj.getShortName<RIGHT_SIDE>(), zen::TEMP_FILE_ENDING))
                    fileObj.setSyncDir(SYNC_DIR_RIGHT); //schedule potentially existing temporary files for deletion
                else
                    fileObj.setSyncDir(dirCfg.exRightSideOnly);
                break;
            case FILE_RIGHT_NEWER:
                fileObj.setSyncDir(dirCfg.rightNewer);
                break;
            case FILE_LEFT_NEWER:
                fileObj.setSyncDir(dirCfg.leftNewer);
                break;
            case FILE_DIFFERENT:
                fileObj.setSyncDir(dirCfg.different);
                break;
            case FILE_CONFLICT:
                if (dirCfg.conflict == SYNC_DIR_NONE)
                    fileObj.setSyncDirConflict(fileObj.getCatConflict()); //take over category conflict
                else
                    fileObj.setSyncDir(dirCfg.conflict);
                break;
            case FILE_EQUAL:
                fileObj.setSyncDir(SYNC_DIR_NONE);
                break;
            case FILE_DIFFERENT_METADATA:
                fileObj.setSyncDir(dirCfg.conflict); //use setting from "conflict/cannot categorize"
                break;
        }
    }

    void operator()(SymLinkMapping& linkObj) const
    {
        switch (linkObj.getLinkCategory())
        {
            case SYMLINK_LEFT_SIDE_ONLY:
                linkObj.setSyncDir(dirCfg.exLeftSideOnly);
                break;
            case SYMLINK_RIGHT_SIDE_ONLY:
                linkObj.setSyncDir(dirCfg.exRightSideOnly);
                break;
            case SYMLINK_LEFT_NEWER:
                linkObj.setSyncDir(dirCfg.leftNewer);
                break;
            case SYMLINK_RIGHT_NEWER:
                linkObj.setSyncDir(dirCfg.rightNewer);
                break;
            case SYMLINK_CONFLICT:
                if (dirCfg.conflict == SYNC_DIR_NONE)
                    linkObj.setSyncDirConflict(linkObj.getCatConflict()); //take over category conflict
                else
                    linkObj.setSyncDir(dirCfg.conflict);
                break;
            case SYMLINK_DIFFERENT:
                linkObj.setSyncDir(dirCfg.different);
                break;
            case SYMLINK_EQUAL:
                linkObj.setSyncDir(SYNC_DIR_NONE);
                break;
            case SYMLINK_DIFFERENT_METADATA:
                linkObj.setSyncDir(dirCfg.conflict); //use setting from "conflict/cannot categorize"
                break;

        }
    }

    void operator()(DirMapping& dirObj) const
    {
        switch (dirObj.getDirCategory())
        {
            case DIR_LEFT_SIDE_ONLY:
                dirObj.setSyncDir(dirCfg.exLeftSideOnly);
                break;
            case DIR_RIGHT_SIDE_ONLY:
                dirObj.setSyncDir(dirCfg.exRightSideOnly);
                break;
            case DIR_EQUAL:
                dirObj.setSyncDir(SYNC_DIR_NONE);
                break;
            case DIR_DIFFERENT_METADATA:
                dirObj.setSyncDir(dirCfg.conflict); //use setting from "conflict/cannot categorize"
                break;
        }

        recurse(dirObj);
    }

    const DirectionSet dirCfg;
};


//---------------------------------------------------------------------------------------------------------------
struct AllEqual //test if non-equal items exist in scanned data
{
    bool operator()(const HierarchyObject& hierObj) const
    {
        return std::all_of(hierObj.refSubFiles().begin(), hierObj.refSubFiles().end(),
        [](const FileMapping& fileObj) { return fileObj.getCategory() == FILE_EQUAL; }) && //files

               std::all_of(hierObj.refSubLinks().begin(), hierObj.refSubLinks().end(),
        [](const SymLinkMapping& linkObj) { return linkObj.getLinkCategory() == SYMLINK_EQUAL; }) && //symlinks

               std::all_of(hierObj.refSubDirs(). begin(), hierObj.refSubDirs(). end(),
                           [](const DirMapping& dirObj) -> bool
        {
            if (dirObj.getDirCategory() != DIR_EQUAL)
                return false;
            return AllEqual()(dirObj); //recurse
        });    //directories
    }
};


bool zen::allElementsEqual(const FolderComparison& folderCmp)
{
    return std::all_of(begin(folderCmp), end(folderCmp), AllEqual());
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

    DataSetFile(const FileMapping& fileObj, Int2Type<LEFT_SIDE>)
    {
        init<LEFT_SIDE>(fileObj);
    }

    DataSetFile(const FileMapping& fileObj, Int2Type<RIGHT_SIDE>)
    {
        init<RIGHT_SIDE>(fileObj);
    }

    inline friend
    bool operator==(const DataSetFile& lhs, const DataSetFile& rhs)
    {
        if (lhs.shortName.empty())
            return rhs.shortName.empty();
        else if (rhs.shortName.empty())
            return false;

        return lhs.shortName == rhs.shortName && //detect changes in case (windows)
               //respect 2 second FAT/FAT32 precision! copying a file to a FAT32 drive changes it's modification date by up to 2 seconds
               sameFileTime(lhs.lastWriteTime, rhs.lastWriteTime, 2) &&
               lhs.fileSize == rhs.fileSize;
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
    zen::Int64  lastWriteTime;
    zen::UInt64 fileSize;

    //note: we do *not* consider FileId here, but are only interested in *visual* changes. Consider user moving data to some other medium, this is not a change!
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

    DataSetSymlink(const SymLinkMapping& linkObj, Int2Type<LEFT_SIDE>)
    {
        init<LEFT_SIDE>(linkObj);
    }

    DataSetSymlink(const SymLinkMapping& linkObj, Int2Type<RIGHT_SIDE>)
    {
        init<RIGHT_SIDE>(linkObj);
    }

    inline friend
    bool operator==(const DataSetSymlink& lhs, const DataSetSymlink& rhs)
    {
        if (lhs.shortName.empty()) //test if object is existing at all
            return rhs.shortName.empty();
        else if (rhs.shortName.empty())
            return false;

        return lhs.shortName == rhs.shortName &&
               //respect 2 second FAT/FAT32 precision! copying a file to a FAT32 drive changes it's modification date by up to 2 seconds
               sameFileTime(lhs.lastWriteTime, rhs.lastWriteTime, 2) &&
#ifdef FFS_WIN //comparison of symbolic link type is relevant for Windows only
               lhs.type == rhs.type &&
#endif
               lhs.targetPath == rhs.targetPath;
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
    zen::Int64 lastWriteTime;
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

    DataSetDir(const DirMapping& dirObj, Int2Type<LEFT_SIDE>) :
        shortName(dirObj.getShortName<LEFT_SIDE>()) {}

    DataSetDir(const DirMapping& dirObj, Int2Type<RIGHT_SIDE>) :
        shortName(dirObj.getShortName<RIGHT_SIDE>()) {}

    inline friend
    bool operator==(const DataSetDir& lhs, const DataSetDir& rhs)
    {
        return lhs.shortName == rhs.shortName;
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

    return std::make_pair(DataSetDir(), nullptr); //object not found
}

//----------------------------------------------------------------------------------------------
class RedetermineAuto
{
public:
    static void execute(BaseDirMapping& baseDirectory, std::function<void(const std::wstring&)> reportWarning)
    {
        RedetermineAuto(baseDirectory, reportWarning);
    }

private:
    RedetermineAuto(BaseDirMapping& baseDirectory, std::function<void(const std::wstring&)> reportWarning) :
        txtBothSidesChanged(_("Both sides have changed since last synchronization!")),
        txtNoSideChanged(_("Cannot determine sync-direction:") + L" \n" + _("No change since last synchronization!")),
        txtFilterChanged(_("Cannot determine sync-direction:") + L" \n" + _("Filter settings have changed!")),
        txtLastSyncFail (_("Cannot determine sync-direction:") + L" \n" + _("The file was not processed by last synchronization!")),
        reportWarning_(reportWarning)
    {
        if (AllEqual()(baseDirectory)) //nothing to do: abort and don't show any nag-screens
            return;

        //try to load sync-database files
        std::pair<DirInfoPtr, DirInfoPtr> dirInfo = loadDBFile(baseDirectory);
        if (dirInfo.first.get()  == nullptr ||
            dirInfo.second.get() == nullptr)
        {
            //set conservative "two-way" directions
            DirectionSet twoWayCfg = getTwoWaySet();

            Redetermine::execute(twoWayCfg, baseDirectory);
            return;
        }

        const DirInformation& dirInfoLeft  = *dirInfo.first;
        const DirInformation& dirInfoRight = *dirInfo.second;

        //-> considering filter not relevant:
        //if narrowing filter: all ok; if widening filter (if file ex on both sides -> conflict, fine; if file ex. on one side: copy to other side: fine)
        /*
                //save db filter (if it needs to be considered only):
                if (respectFiltering(baseDirectory, dirInfoLeft))
                    dbFilterLeft = dirInfoLeft.filter.get();

                if (respectFiltering(baseDirectory, dirInfoRight))
                    dbFilterRight = dirInfoRight.filter.get();
        */
        recurse(baseDirectory,
                &dirInfoLeft.baseDirContainer,
                &dirInfoRight.baseDirContainer);

        //----------- detect renamed files -----------------
        if (!exLeftOnly.empty() && !exRightOnly.empty())
        {
            findEqualDbEntries(dirInfoLeft .baseDirContainer, //fill map "onceEqual"
                               dirInfoRight.baseDirContainer);

            detectRenamedFiles();
        }
    }

    /*
    static bool respectFiltering(const BaseDirMapping& baseDirectory, const DirInformation& dirInfo)
    {
        //respect filtering if sync-DB filter is active && different from baseDir's filter:
        // in all other cases "view on files" is smaller for baseDirectory(current) than it was for dirInfo(old)
        // => dirInfo can be queried as if it were a scan without filters
        return !dirInfo.filter->isNull() && *dirInfo.filter != *baseDirectory.getFilter();
    }
    */

    std::pair<DirInfoPtr, DirInfoPtr> loadDBFile(const BaseDirMapping& baseDirectory) //return nullptr on failure
    {
        try
        {
            return loadFromDisk(baseDirectory);
        }
        catch (FileErrorDatabaseNotExisting&) {} //let's ignore these errors for now...
        catch (FileError& error) //e.g. incompatible database version
        {
            reportWarning_(error.toString() + L" \n\n" +
                           _("Setting default synchronization directions: Old files will be overwritten with newer files."));
        }
        return std::pair<DirInfoPtr, DirInfoPtr>();
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
            return (dbFilterLeft  && !dbFilterLeft ->passDirFilter(relativeName, nullptr)) ||
                   (dbFilterRight && !dbFilterRight->passDirFilter(relativeName, nullptr));
        }
    */

    void recurse(HierarchyObject& hierObj,
                 const DirContainer* dbDirectoryLeft,
                 const DirContainer* dbDirectoryRight)
    {
        std::for_each(hierObj.refSubFiles().begin(), hierObj.refSubFiles().end(),
        [&](FileMapping& fileMap) { processFile(fileMap, dbDirectoryLeft, dbDirectoryRight); });

        std::for_each(hierObj.refSubLinks().begin(), hierObj.refSubLinks().end(),
        [&](SymLinkMapping& linkMap) { processSymlink(linkMap, dbDirectoryLeft, dbDirectoryRight); });

        std::for_each(hierObj.refSubDirs().begin(), hierObj.refSubDirs().end(),
        [&](DirMapping& dirMap) { processDir(dirMap, dbDirectoryLeft, dbDirectoryRight); });
    }

    void processFile(FileMapping& fileObj,
                     const DirContainer* dbDirectoryLeft,
                     const DirContainer* dbDirectoryRight)
    {
        const CompareFilesResult cat = fileObj.getCategory();
        if (cat == FILE_EQUAL)
            return;

        //----------------- prepare detection of renamed files -----------------
        if (cat == FILE_LEFT_SIDE_ONLY)
        {
            if (fileObj.getFileId<LEFT_SIDE>() != FileId())
                exLeftOnly.push_back(&fileObj);
        }
        else if (cat == FILE_RIGHT_SIDE_ONLY)
        {
            if (fileObj.getFileId<RIGHT_SIDE>() != FileId())
                exRightOnly.insert(std::make_pair(getFileIdKey<RIGHT_SIDE>(fileObj), &fileObj));
        }
        //----------------------------------------------------------------------

        //##################### schedule potentially existing temporary files for deletion ####################
        if (cat == FILE_LEFT_SIDE_ONLY && endsWith(fileObj.getShortName<LEFT_SIDE>(), zen::TEMP_FILE_ENDING))
        {
            fileObj.setSyncDir(SYNC_DIR_LEFT);
            return;
        }
        else if (cat == FILE_RIGHT_SIDE_ONLY && endsWith(fileObj.getShortName<RIGHT_SIDE>(), zen::TEMP_FILE_ENDING))
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
                        fileObj.setSyncDirConflict(txtFilterChanged);
                    return;
                }
                */

        //determine datasets for change detection
        const DataSetFile dataDbLeft  = retrieveDataSetFile(fileObj.getObjShortName(), dbDirectoryLeft);
        const DataSetFile dataDbRight = retrieveDataSetFile(fileObj.getObjShortName(), dbDirectoryRight);

        const DataSetFile dataCurrentLeft (fileObj, Int2Type<LEFT_SIDE >());
        const DataSetFile dataCurrentRight(fileObj, Int2Type<RIGHT_SIDE>());

        //evaluation
        const bool changeOnLeft  = dataDbLeft  != dataCurrentLeft;
        const bool changeOnRight = dataDbRight != dataCurrentRight;

        if (dataDbLeft == dataDbRight) //we have a "last synchronous state" => last sync seems to have been successful
        {
            if (changeOnLeft)
            {
                if (changeOnRight)
                    fileObj.setSyncDirConflict(txtBothSidesChanged);
                else
                    fileObj.setSyncDir(SYNC_DIR_RIGHT);
            }
            else
            {
                if (changeOnRight)
                    fileObj.setSyncDir(SYNC_DIR_LEFT);
                else
                    fileObj.setSyncDirConflict(txtNoSideChanged);
            }
        }
        else //object did not complete last sync: important check: user may have changed comparison variant, so what was in sync according to last variant is not any longer!
        {
            if (changeOnLeft && changeOnRight)
                fileObj.setSyncDirConflict(txtBothSidesChanged);
            else
            {
                //                if (cat == FILE_LEFT_SIDE_ONLY)
                //                    fileObj.setSyncDir(SYNC_DIR_RIGHT);
                //                else if (cat == FILE_RIGHT_SIDE_ONLY)
                //                    fileObj.setSyncDir(SYNC_DIR_LEFT);
                //                else
                fileObj.setSyncDirConflict(txtLastSyncFail);
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
                        linkObj.setSyncDirConflict(txtFilterChanged);
                    return;
                }
                */

        //determine datasets for change detection
        const DataSetSymlink dataDbLeft  = retrieveDataSetSymlink(linkObj.getObjShortName(), dbDirectoryLeft);
        const DataSetSymlink dataDbRight = retrieveDataSetSymlink(linkObj.getObjShortName(), dbDirectoryRight);

        const DataSetSymlink dataCurrentLeft( linkObj, Int2Type<LEFT_SIDE>());
        const DataSetSymlink dataCurrentRight(linkObj, Int2Type<RIGHT_SIDE>());

        //evaluation
        const bool changeOnLeft  = dataDbLeft  != dataCurrentLeft;
        const bool changeOnRight = dataDbRight != dataCurrentRight;

        if (dataDbLeft == dataDbRight) //last sync seems to have been successful
        {
            if (changeOnLeft)
            {
                if (changeOnRight)
                    linkObj.setSyncDirConflict(txtBothSidesChanged);
                else
                    linkObj.setSyncDir(SYNC_DIR_RIGHT);
            }
            else
            {
                if (changeOnRight)
                    linkObj.setSyncDir(SYNC_DIR_LEFT);
                else
                    linkObj.setSyncDirConflict(txtNoSideChanged);
            }
        }
        else //object did not complete last sync
        {
            if (changeOnLeft && changeOnRight)
                linkObj.setSyncDirConflict(txtBothSidesChanged);
            else
                linkObj.setSyncDirConflict(txtLastSyncFail);
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

                    SetDirChangedFilter().recurse(dirObj); //filter issue for this directory => treat subfiles/-dirs the same
                    return;
                }
        */
        //determine datasets for change detection
        const std::pair<DataSetDir, const DirContainer*> dataDbLeftStuff  = retrieveDataSetDir(dirObj.getObjShortName(), dbDirectoryLeft);
        const std::pair<DataSetDir, const DirContainer*> dataDbRightStuff = retrieveDataSetDir(dirObj.getObjShortName(), dbDirectoryRight);

        if (cat != DIR_EQUAL)
        {
            const DataSetDir dataCurrentLeft( dirObj, Int2Type<LEFT_SIDE>());
            const DataSetDir dataCurrentRight(dirObj, Int2Type<RIGHT_SIDE>());

            //evaluation
            const bool changeOnLeft  = dataDbLeftStuff.first  != dataCurrentLeft;
            const bool changeOnRight = dataDbRightStuff.first != dataCurrentRight;

            if (dataDbLeftStuff.first == dataDbRightStuff.first) //last sync seems to have been successful
            {
                if (changeOnLeft)
                {
                    if (changeOnRight)
                        dirObj.setSyncDirConflict(txtBothSidesChanged);
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
                        dirObj.setSyncDirConflict(txtNoSideChanged);
                    }
                }
            }
            else //object did not complete last sync
            {
                if (changeOnLeft && changeOnRight)
                    dirObj.setSyncDirConflict(txtBothSidesChanged);
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

                    dirObj.setSyncDirConflict(txtLastSyncFail);
                }
            }
        }

        recurse(dirObj, dataDbLeftStuff.second, dataDbRightStuff.second); //recursion
    }


    void findEqualDbEntries(const DirContainer& dbDirectoryLeft,
                            const DirContainer& dbDirectoryRight)
    {
        //note: we cannot integrate this traversal into "recurse()" since it may take a *slightly* different path: e.g. file renamed on both sides

        std::for_each(dbDirectoryLeft.files.begin(), dbDirectoryLeft.files.end(),
                      [&](const DirContainer::FileList::value_type& entryLeft)
        {
            auto iterRight = dbDirectoryRight.files.find(entryLeft.first);
            if (iterRight != dbDirectoryRight.files.end())
            {
                if (entryLeft. second.id != FileId() &&
                    iterRight->second.id != FileId() &&
                    DataSetFile(entryLeft.first, entryLeft.second) == DataSetFile(iterRight->first, iterRight->second))
                    onceEqual.insert(std::make_pair(getFileIdKey(entryLeft.second), getFileIdKey(iterRight->second)));
            }
        });

        std::for_each(dbDirectoryLeft.dirs.begin(), dbDirectoryLeft.dirs.end(),
                      [&](const DirContainer::DirList::value_type& entryLeft)
        {
            auto iterRight = dbDirectoryRight.dirs.find(entryLeft.first);
            if (iterRight != dbDirectoryRight.dirs.end())
                findEqualDbEntries(entryLeft.second, iterRight->second);
        });
    }

    typedef std::tuple<Int64, UInt64, FileId> FileIdKey; //(date, size, file ID)


    //modification date is *not* considered as part of container key, so check here!
    template <class Container>
    static typename Container::const_iterator findValue(const Container& cnt, const FileIdKey& key)
    {
        auto iterPair = cnt.equal_range(key); //since file id is already unique, we expect a single-element range at most
        auto iter = std::find_if(iterPair.first, iterPair.second,
                                 [&](const typename Container::value_type& item)
        {
            return sameFileTime(std::get<0>(item.first), std::get<0>(key), 2); //respect 2 second FAT/FAT32 precision
        });
        return iter == iterPair.second ? cnt.end() : iter;
    }

    void detectRenamedFiles() const
    {
        std::for_each(exLeftOnly.begin(), exLeftOnly.end(),
                      [&](FileMapping* fileLeftOnly)
        {
            const FileIdKey& keyLeft = RedetermineAuto::getFileIdKey<LEFT_SIDE>(*fileLeftOnly);

            auto iter = findValue(onceEqual, keyLeft);
            if (iter != onceEqual.end())
            {
                const FileIdKey& keyRight = iter->second;

                auto iter2 = findValue(exRightOnly, keyRight);
                if (iter2 != exRightOnly.end())
                {
                    FileMapping* fileRightOnly = iter2->second;

                    //found a pair, mark it!
                    fileLeftOnly ->setMoveRef(fileRightOnly->getId());
                    fileRightOnly->setMoveRef(fileLeftOnly ->getId());
                }
            }
        });
    }


    const std::wstring txtBothSidesChanged;
    const std::wstring txtNoSideChanged;
    const std::wstring txtFilterChanged;
    const std::wstring txtLastSyncFail;

    std::function<void(const std::wstring&)> reportWarning_;

    //detection of renamed files
    template <SelectedSide side>
    static FileIdKey getFileIdKey(const FileMapping& fsObj) { return std::make_tuple(fsObj.getLastWriteTime<side>(), fsObj.getFileSize<side>(), fsObj.getFileId<side>()); }
    static FileIdKey getFileIdKey(const FileDescriptor& fileDescr) { return std::make_tuple(fileDescr.lastWriteTimeRaw, fileDescr.fileSize, fileDescr.id); }

    struct LessFileIdKey
    {
        bool operator()(const FileIdKey& lhs, const FileIdKey& rhs) const
        {
            //caveat: *don't* allow 2 sec tolerance as container predicate!!
            // => no strict weak ordering relation! reason: no transitivity of equivalence!

            //-> bad: if (!sameFileTime(std::get<0>(lhs), std::get<0>(rhs), 2))
            //    return std::get<0>(lhs) < std::get<0>(rhs);

            if (std::get<1>(lhs) != std::get<1>(rhs)) //file size
                return std::get<1>(lhs) < std::get<1>(rhs);

            return std::get<2>(lhs) < std::get<2>(rhs); //file id
        }
    };

    std::vector<FileMapping*> exLeftOnly;

    std::multimap<FileIdKey, FileIdKey, LessFileIdKey> onceEqual; //associates left and right database entries which are considered "equal" := "same name, size, date"

    std::multimap<FileIdKey, FileMapping*, LessFileIdKey> exRightOnly;

    /*
    detect renamed files

     X  ->  |_|      Create right
    |_| ->   Y       Delete right

    is detected as:

    Rename Y to X on right

    Algorithm:
    ----------
    DB-file left  --- (name, size, date) --->    DB-file right
      /|\                                             |
       |  (file ID, size, date)                       |  (file ID, size, date)
       |                                             \|/
    file left only                               file right only

       FAT caveat: File Ids are generally not stable when file is either moved or renamed!
       => 1. Move/rename operations on FAT cannot be detected reliably.
       => 2. database generally contains wrong file ID on FAT after renaming from .ffs_tmp files => correct file Ids in database after next sync
    */
};


//---------------------------------------------------------------------------------------------------------------
std::vector<DirectionConfig> zen::extractDirectionCfg(const MainConfiguration& mainCfg)
{
    //merge first and additional pairs
    std::vector<FolderPairEnh> allPairs;
    allPairs.push_back(mainCfg.firstPair);
    allPairs.insert(allPairs.end(),
                    mainCfg.additionalPairs.begin(), //add additional pairs
                    mainCfg.additionalPairs.end());

    std::vector<DirectionConfig> output;
    std::for_each(allPairs.begin(), allPairs.end(),
                  [&](const FolderPairEnh& fp)
    {
        output.push_back(fp.altSyncConfig.get() ? fp.altSyncConfig->directionCfg : mainCfg.syncCfg.directionCfg);
    });

    return output;
}


void zen::redetermineSyncDirection(const DirectionConfig& directConfig, BaseDirMapping& baseDirectory, std::function<void(const std::wstring&)> reportWarning)
{
    if (directConfig.var == DirectionConfig::AUTOMATIC)
        RedetermineAuto::execute(baseDirectory, reportWarning);
    else
    {
        DirectionSet dirCfg = extractDirections(directConfig);
        Redetermine::execute(dirCfg, baseDirectory);
    }
}


void zen::redetermineSyncDirection(const MainConfiguration& mainCfg, FolderComparison& folderCmp, std::function<void(const std::wstring&)> reportWarning)
{
    if (folderCmp.empty())
        return;

    std::vector<DirectionConfig> directCfgs = extractDirectionCfg(mainCfg);

    if (folderCmp.size() != directCfgs.size())
        throw std::logic_error("Programming Error: Contract violation!");

    for (auto iter = folderCmp.begin(); iter != folderCmp.end(); ++iter)
    {
        const DirectionConfig& cfg = directCfgs[iter - folderCmp.begin()];
        redetermineSyncDirection(cfg, **iter, reportWarning);
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
    void execute(HierarchyObject& hierObj) const
    {
        std::for_each(hierObj.refSubFiles().begin(), hierObj.refSubFiles().end(), [&](FileMapping&    fileMap) { (*this)(fileMap); });
        std::for_each(hierObj.refSubLinks().begin(), hierObj.refSubLinks().end(), [&](SymLinkMapping& linkMap) { (*this)(linkMap); });
        std::for_each(hierObj.refSubDirs ().begin(), hierObj.refSubDirs ().end(), [&](DirMapping&      dirMap) { (*this)(dirMap); });
    }

    const SyncDirection newDirection_;
};


void zen::setSyncDirectionRec(SyncDirection newDirection, FileSystemObject& fsObj)
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
    void operator()(zen::BaseDirMapping& baseDirectory) const //be careful with operator() to no get called by std::for_each!
    {
        execute(baseDirectory);
    }

    void execute(zen::HierarchyObject& hierObj) const //don't create ambiguity by replacing with operator()
    {
        std::for_each(hierObj.refSubFiles().begin(), hierObj.refSubFiles().end(), [&](FileMapping&    fileMap) { (*this)(fileMap); });
        std::for_each(hierObj.refSubLinks().begin(), hierObj.refSubLinks().end(), [&](SymLinkMapping& linkMap) { (*this)(linkMap); });
        std::for_each(hierObj.refSubDirs ().begin(), hierObj.refSubDirs ().end(), [&](DirMapping&      dirMap) { (*this)(dirMap); });
    }

private:
    void operator()(zen::FileMapping& fileObj) const
    {
        fileObj.setActive(include);
    }

    void operator()(zen::SymLinkMapping& linkObj) const
    {
        linkObj.setActive(include);
    }

    void operator()(zen::DirMapping& dirObj) const
    {
        dirObj.setActive(include);
        execute(dirObj); //recursion
    }
};


void zen::setActiveStatus(bool newStatus, zen::FolderComparison& folderCmp)
{
    if (newStatus)
        std::for_each(begin(folderCmp), end(folderCmp), InOrExcludeAllRows<true>());  //include all rows
    else
        std::for_each(begin(folderCmp), end(folderCmp), InOrExcludeAllRows<false>()); //exclude all rows
}


void zen::setActiveStatus(bool newStatus, zen::FileSystemObject& fsObj)
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
    STRATEGY_SET,
    STRATEGY_AND,
    STRATEGY_OR
};

template <FilterStrategy strategy> struct Eval;

template <>
struct Eval<STRATEGY_SET> //process all elements
{
    template <class T>
    bool process(const T& obj) const { return true; }
};

template <>
struct Eval<STRATEGY_AND>
{
    template <class T>
    bool process(const T& obj) const { return obj.isActive(); }
};

template <>
struct Eval<STRATEGY_OR>
{
    template <class T>
    bool process(const T& obj) const { return !obj.isActive(); }
};


template <FilterStrategy strategy>
class ApplyHardFilter
{
public:
    ApplyHardFilter(const HardFilter& filterProcIn) : filterProc(filterProcIn) {}

    void execute(zen::HierarchyObject& hierObj) const
    {
        std::for_each(hierObj.refSubFiles().begin(), hierObj.refSubFiles().end(), [&](FileMapping&    fileMap) { (*this)(fileMap); });
        std::for_each(hierObj.refSubLinks().begin(), hierObj.refSubLinks().end(), [&](SymLinkMapping& linkMap) { (*this)(linkMap); });
        std::for_each(hierObj.refSubDirs ().begin(), hierObj.refSubDirs ().end(), [&](DirMapping&      dirMap) { (*this)(dirMap); });
    };

private:
    void operator()(zen::FileMapping& fileObj) const
    {
        if (Eval<strategy>().process(fileObj))
            fileObj.setActive(filterProc.passFileFilter(fileObj.getObjRelativeName()));
    }

    void operator()(zen::SymLinkMapping& linkObj) const
    {
        if (Eval<strategy>().process(linkObj))
            linkObj.setActive(filterProc.passFileFilter(linkObj.getObjRelativeName()));
    }

    void operator()(zen::DirMapping& dirObj) const
    {
        bool subObjMightMatch = true;
        const bool filterPassed = filterProc.passDirFilter(dirObj.getObjRelativeName(), &subObjMightMatch);

        if (Eval<strategy>().process(dirObj))
            dirObj.setActive(filterPassed);

        if (!subObjMightMatch) //use same logic like directory traversing here: evaluate filter in subdirs only if objects could match
        {
            InOrExcludeAllRows<false>().execute(dirObj); //exclude all files dirs in subfolders
            return;
        }

        execute(dirObj);  //recursion
    }

    const HardFilter& filterProc;
};

template <>
class ApplyHardFilter<STRATEGY_OR>; //usage of InOrExcludeAllRows doesn't allow for strategy "or"


template <FilterStrategy strategy>
class ApplySoftFilter //falsify only! -> can run directly after "hard/base filter"
{
public:
    ApplySoftFilter(const SoftFilter& timeSizeFilter) : timeSizeFilter_(timeSizeFilter) {}

    void execute(zen::HierarchyObject& hierObj) const
    {
        std::for_each(hierObj.refSubFiles().begin(), hierObj.refSubFiles().end(), [&](FileMapping&    fileMap) { (*this)(fileMap); });
        std::for_each(hierObj.refSubLinks().begin(), hierObj.refSubLinks().end(), [&](SymLinkMapping& linkMap) { (*this)(linkMap); });
        std::for_each(hierObj.refSubDirs ().begin(), hierObj.refSubDirs ().end(), [&](DirMapping&      dirMap) { (*this)(dirMap); });
    };

private:
    void operator()(zen::FileMapping& fileObj) const
    {
        if (Eval<strategy>().process(fileObj))
        {
            if (fileObj.isEmpty<LEFT_SIDE>())
                fileObj.setActive(matchSize<RIGHT_SIDE>(fileObj) &&
                                  matchTime<RIGHT_SIDE>(fileObj));
            else if (fileObj.isEmpty<RIGHT_SIDE>())
                fileObj.setActive(matchSize<LEFT_SIDE>(fileObj) &&
                                  matchTime<LEFT_SIDE>(fileObj));
            else
            {
                //the only case with partially unclear semantics:
                //file and time filters may match or not match on each side, leaving a total of 16 combinations for both sides!
                /*
                               ST S T -         ST := match size and time
                               ---------         S := match size only
                            ST |X|X|X|X|         T := match time only
                            ------------         - := no match
                             S |X|O|?|O|
                            ------------         X := include row
                             T |X|?|O|O|         O := exclude row
                            ------------         ? := unclear
                             - |X|O|O|O|
                            ------------
                */
                //let's set ? := O
                fileObj.setActive((matchSize<RIGHT_SIDE>(fileObj) &&
                                   matchTime<RIGHT_SIDE>(fileObj)) ||
                                  (matchSize<LEFT_SIDE>(fileObj) &&
                                   matchTime<LEFT_SIDE>(fileObj)));
            }
        }
    }

    void operator()(zen::SymLinkMapping& linkObj) const
    {
        if (Eval<strategy>().process(linkObj))
        {
            if (linkObj.isEmpty<LEFT_SIDE>())
                linkObj.setActive(matchTime<RIGHT_SIDE>(linkObj));
            else if (linkObj.isEmpty<RIGHT_SIDE>())
                linkObj.setActive(matchTime<LEFT_SIDE>(linkObj));
            else
                linkObj.setActive(matchTime<RIGHT_SIDE>(linkObj) ||
                                  matchTime<LEFT_SIDE> (linkObj));
        }
    }

    void operator()(zen::DirMapping& dirObj) const
    {
        if (Eval<strategy>().process(dirObj))
            dirObj.setActive(timeSizeFilter_.matchFolder()); //if date filter is active we deactivate all folders: effectively gets rid of empty folders!

        execute(dirObj);  //recursion
    }

    template <SelectedSide side, class T>
    bool matchTime(const T& obj) const
    {
        return timeSizeFilter_.matchTime(obj.template getLastWriteTime<side>());
    }

    template <SelectedSide side, class T>
    bool matchSize(const T& obj) const
    {
        return timeSizeFilter_.matchSize(obj.template getFileSize<side>());
    }

    const SoftFilter timeSizeFilter_;
};
}


void zen::addHardFiltering(BaseDirMapping& baseMap, const Zstring& excludeFilter)
{
    ApplyHardFilter<STRATEGY_AND>(*HardFilter::FilterRef(
                                      new NameFilter(FilterConfig().includeFilter, excludeFilter))).execute(baseMap);
}


void zen::addSoftFiltering(BaseDirMapping& baseMap, const SoftFilter& timeSizeFilter)
{
    if (!timeSizeFilter.isNull()) //since we use STRATEGY_AND, we may skip a "null" filter
        ApplySoftFilter<STRATEGY_AND>(timeSizeFilter).execute(baseMap);
}


void zen::applyFiltering(FolderComparison& folderCmp, const MainConfiguration& mainCfg)
{
    if (folderCmp.empty())
        return;
    else if (folderCmp.size() != mainCfg.additionalPairs.size() + 1)
        throw std::logic_error("Programming Error: Contract violation!");

    //merge first and additional pairs
    std::vector<FolderPairEnh> allPairs;
    allPairs.push_back(mainCfg.firstPair);
    allPairs.insert(allPairs.end(),
                    mainCfg.additionalPairs.begin(), //add additional pairs
                    mainCfg.additionalPairs.end());


    for (auto iter = allPairs.begin(); iter != allPairs.end(); ++iter)
    {
        BaseDirMapping& baseDirectory = *folderCmp[iter - allPairs.begin()];

        const NormalizedFilter normFilter = normalizeFilters(mainCfg.globalFilter, iter->localFilter);

        //"set" hard filter
        ApplyHardFilter<STRATEGY_SET>(*normFilter.nameFilter).execute(baseDirectory);

        //"and" soft filter
        addSoftFiltering(baseDirectory, normFilter.timeSizeFilter);
    }
}


class FilterByTimeSpan
{
public:
    FilterByTimeSpan(const Int64& timeFrom,
                     const Int64& timeTo) :
        timeFrom_(timeFrom),
        timeTo_(timeTo) {}

    void execute(zen::HierarchyObject& hierObj) const
    {
        std::for_each(hierObj.refSubFiles().begin(), hierObj.refSubFiles().end(), [&](FileMapping&    fileMap) { (*this)(fileMap); });
        std::for_each(hierObj.refSubLinks().begin(), hierObj.refSubLinks().end(), [&](SymLinkMapping& linkMap) { (*this)(linkMap); });
        std::for_each(hierObj.refSubDirs ().begin(), hierObj.refSubDirs ().end(), [&](DirMapping&      dirMap) { (*this)(dirMap); });
    };

private:
    void operator()(zen::FileMapping& fileObj) const
    {
        if (fileObj.isEmpty<LEFT_SIDE>())
            fileObj.setActive(matchTime<RIGHT_SIDE>(fileObj));
        else if (fileObj.isEmpty<RIGHT_SIDE>())
            fileObj.setActive(matchTime<LEFT_SIDE>(fileObj));
        else
            fileObj.setActive(matchTime<RIGHT_SIDE>(fileObj) ||
                              matchTime<LEFT_SIDE>(fileObj));
    }

    void operator()(zen::SymLinkMapping& linkObj) const
    {
        if (linkObj.isEmpty<LEFT_SIDE>())
            linkObj.setActive(matchTime<RIGHT_SIDE>(linkObj));
        else if (linkObj.isEmpty<RIGHT_SIDE>())
            linkObj.setActive(matchTime<LEFT_SIDE>(linkObj));
        else
            linkObj.setActive(matchTime<RIGHT_SIDE>(linkObj) ||
                              matchTime<LEFT_SIDE> (linkObj));
    }

    void operator()(zen::DirMapping& dirObj) const
    {
        dirObj.setActive(false);
        execute(dirObj);  //recursion
    }

    template <SelectedSide side, class T>
    bool matchTime(const T& obj) const
    {
        return timeFrom_ <= obj.template getLastWriteTime<side>() &&
               obj.template getLastWriteTime<side>() <= timeTo_;
    }

    const Int64 timeFrom_;
    const Int64 timeTo_;
};


void zen::applyTimeSpanFilter(FolderComparison& folderCmp, const Int64& timeFrom, const Int64& timeTo)
{
    FilterByTimeSpan spanFilter(timeFrom, timeTo);

    std::for_each(begin(folderCmp), end(folderCmp), [&](BaseDirMapping& baseMap) { spanFilter.execute(baseMap); });
}


//############################################################################################################
std::pair<wxString, int> zen::deleteFromGridAndHDPreview(const std::vector<FileSystemObject*>& selectionLeft,
                                                         const std::vector<FileSystemObject*>& selectionRight,
                                                         bool deleteOnBothSides)
{
    //fast replacement for wxString modelling exponential growth
    typedef Zbase<wchar_t> zxString; //for use with UI texts

    zxString filesToDelete;
    int totalDelCount = 0;

    if (deleteOnBothSides)
    {
        //mix selected rows from left and right (without changing order)
        std::vector<FileSystemObject*> selection;
        {
            hash_set<FileSystemObject*> objectsUsed;
            std::copy_if(selectionLeft .begin(), selectionLeft .end(), std::back_inserter(selection), [&](FileSystemObject* fsObj) { return objectsUsed.insert(fsObj).second; });
            std::copy_if(selectionRight.begin(), selectionRight.end(), std::back_inserter(selection), [&](FileSystemObject* fsObj) { return objectsUsed.insert(fsObj).second; });
        }

        std::for_each(selection.begin(), selection.end(),
                      [&](const FileSystemObject* fsObj)
        {
            if (!fsObj->isEmpty<LEFT_SIDE>())
            {
                filesToDelete += utf8CvrtTo<zxString>(fsObj->getFullName<LEFT_SIDE>()) + L'\n';
                ++totalDelCount;
            }

            if (!fsObj->isEmpty<RIGHT_SIDE>())
            {
                filesToDelete += utf8CvrtTo<zxString>(fsObj->getFullName<RIGHT_SIDE>()) + L'\n';
                ++totalDelCount;
            }

            filesToDelete += L'\n';
        });
    }
    else //delete selected files only
    {
        std::for_each(selectionLeft.begin(), selectionLeft.end(),
                      [&](const FileSystemObject* fsObj)
        {
            if (!fsObj->isEmpty<LEFT_SIDE>())
            {
                filesToDelete += utf8CvrtTo<zxString>(fsObj->getFullName<LEFT_SIDE>()) + L'\n';
                ++totalDelCount;
            }
        });

        std::for_each(selectionRight.begin(), selectionRight.end(),
                      [&](const FileSystemObject* fsObj)
        {
            if (!fsObj->isEmpty<RIGHT_SIDE>())
            {
                filesToDelete += utf8CvrtTo<zxString>(fsObj->getFullName<RIGHT_SIDE>()) + L'\n';
                ++totalDelCount;
            }
        });
    }

    return std::make_pair(copyStringTo<wxString>(filesToDelete), totalDelCount);
}


namespace
{
struct RemoveCallbackImpl : public zen::CallbackRemoveDir
{
    RemoveCallbackImpl(DeleteFilesHandler& deleteCallback) : deleteCallback_(deleteCallback) {}

    virtual void notifyFileDeletion(const Zstring& filename) { deleteCallback_.notifyDeletion(filename); }
    virtual void notifyDirDeletion (const Zstring& dirname)  { deleteCallback_.notifyDeletion(dirname); }

private:
    DeleteFilesHandler& deleteCallback_;
};
}


template <SelectedSide side, class InputIterator>
void deleteFromGridAndHDOneSide(InputIterator first, InputIterator last,
                                bool useRecycleBin,
                                DeleteFilesHandler& statusHandler)
{
    for (auto iter = first; iter != last; ++iter) //VS 2010 bug prevents replacing this by std::for_each + lamba
    {
        FileSystemObject& fsObj = **iter; //all pointers are required(!) to be bound

        while (true)
        {
            try
            {
                if (!fsObj.isEmpty<side>()) //element may become implicitly delted, e.g. if parent folder was deleted first
                {
                    if (useRecycleBin)
                    {
                        if (zen::moveToRecycleBin(fsObj.getFullName<side>()))  //throw FileError
                            statusHandler.notifyDeletion(fsObj.getFullName<side>());
                    }
                    else
                    {
                        RemoveCallbackImpl removeCallback(statusHandler);

                        //del directories and symlinks
                        struct DeletePermanently : public FSObjectVisitor
                        {
                            DeletePermanently(RemoveCallbackImpl& remCallback) : remCallback_(remCallback) {}

                            virtual void visit(const FileMapping& fileObj)
                            {
                                if (zen::removeFile(fileObj.getFullName<side>()))
                                    remCallback_.notifyFileDeletion(fileObj.getFullName<side>());
                            }

                            virtual void visit(const SymLinkMapping& linkObj)
                            {
                                switch (linkObj.getLinkType<side>())
                                {
                                    case LinkDescriptor::TYPE_DIR:
                                        zen::removeDirectory(linkObj.getFullName<side>(), &remCallback_);
                                        break;
                                    case LinkDescriptor::TYPE_FILE:
                                        if (zen::removeFile(linkObj.getFullName<side>()))
                                            remCallback_.notifyFileDeletion(linkObj.getFullName<side>());
                                        break;
                                }
                            }

                            virtual void visit(const DirMapping& dirObj)
                            {
                                zen::removeDirectory(dirObj.getFullName<side>(), &remCallback_);
                            }

                        private:
                            RemoveCallbackImpl& remCallback_;
                        } delPerm(removeCallback);
                        fsObj.accept(delPerm);
                    }

                    fsObj.removeObject<side>(); //if directory: removes recursively!
                }
                break;
            }
            catch (const FileError& error)
            {
                DeleteFilesHandler::Response rv = statusHandler.reportError(error.toString());

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


void zen::deleteFromGridAndHD(const std::vector<FileSystemObject*>& rowsToDeleteOnLeft,  //refresh GUI grid after deletion to remove invalid rows
                              const std::vector<FileSystemObject*>& rowsToDeleteOnRight, //all pointers need to be bound!
                              FolderComparison& folderCmp,                         //attention: rows will be physically deleted!
                              const std::vector<DirectionConfig>& directCfgs,
                              bool deleteOnBothSides,
                              bool useRecycleBin,
                              DeleteFilesHandler& statusHandler)
{
    if (folderCmp.empty())
        return;
    else if (folderCmp.size() != directCfgs.size())
        throw std::logic_error("Programming Error: Contract violation!");

    //build up mapping from base directory to corresponding direction config
    std::map<const BaseDirMapping*, DirectionConfig> baseDirCfgs;
    for (auto iter = folderCmp.begin(); iter != folderCmp.end(); ++iter)
        baseDirCfgs[&** iter] = directCfgs[iter - folderCmp.begin()];

    std::set<FileSystemObject*> deleteLeft (rowsToDeleteOnLeft .begin(), rowsToDeleteOnLeft .end());
    std::set<FileSystemObject*> deleteRight(rowsToDeleteOnRight.begin(), rowsToDeleteOnRight.end());

    if (deleteOnBothSides)
    {
        deleteLeft.insert(deleteRight.begin(), deleteRight.end());
        deleteRight = deleteLeft;
    }

    set_remove_if(deleteLeft,  std::mem_fun(&FileSystemObject::isEmpty<LEFT_SIDE>));  //remove empty rows to ensure correct statistics
    set_remove_if(deleteRight, std::mem_fun(&FileSystemObject::isEmpty<RIGHT_SIDE>)); //

    //ensure cleanup: redetermination of sync-directions and removal of invalid rows
    auto updateDirection = [&]()
    {
        //update sync direction: we cannot do a full redetermination since the user may already have entered manual changes
        std::set<FileSystemObject*> deletedTotal = deleteLeft;
        deletedTotal.insert(deleteRight.begin(), deleteRight.end());

        for (auto iter = deletedTotal.begin(); iter != deletedTotal.end(); ++iter)
        {
            FileSystemObject& fsObj = **iter; //all pointers are required(!) to be bound

            if (fsObj.isEmpty<LEFT_SIDE>() != fsObj.isEmpty<RIGHT_SIDE>()) //make sure objects exists on one side only
            {
                auto cfgIter = baseDirCfgs.find(&fsObj.root());
                if (cfgIter != baseDirCfgs.end())
                {
                    SyncDirection newDir = SYNC_DIR_NONE;

                    if (cfgIter->second.var == DirectionConfig::AUTOMATIC)
                        newDir = fsObj.isEmpty<LEFT_SIDE>() ? SYNC_DIR_RIGHT : SYNC_DIR_LEFT;
                    else
                    {
                        DirectionSet dirCfg = extractDirections(cfgIter->second);
                        newDir = fsObj.isEmpty<LEFT_SIDE>() ? dirCfg.exRightSideOnly : dirCfg.exLeftSideOnly;
                    }

                    setSyncDirectionRec(newDir, fsObj); //set new direction (recursively)
                }
                else
                    assert(!"this should not happen!");
            }
        }

        //last step: cleanup empty rows: this one invalidates all pointers!
        std::for_each(begin(folderCmp), end(folderCmp), BaseDirMapping::removeEmpty);
    };
    ZEN_ON_SCOPE_EXIT(updateDirection()); //MSVC: assert is a macro and it doesn't play nice with ZEN_ON_SCOPE_EXIT, surprise... wasn't there something about macros being "evil"?

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


void zen::checkForDSTChange(const FileCompareResult& gridData,
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

