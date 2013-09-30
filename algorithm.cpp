// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) Zenju (zenju AT gmx DOT de) - All Rights Reserved        *
// **************************************************************************

#include "algorithm.h"
#include <set>
#include <stdexcept>
#include <zen/file_handling.h>
#include <zen/recycler.h>
#include <zen/stl_tools.h>
#include <zen/scope_guard.h>
#include <zen/thread.h>
#include <wx+/image_resources.h>
#include "lib/norm_filter.h"
#include "lib/db_file.h"
#include "lib/cmp_filetime.h"
#include "lib/norm_filter.h"
#include "process_callback.h" //for UI_UPDATE_INTERVAL

using namespace zen;
using namespace std::rel_ops;


void zen::swapGrids(const MainConfiguration& config, FolderComparison& folderCmp)
{
    std::for_each(begin(folderCmp), end(folderCmp), [](BaseDirPair& baseObj) { baseObj.flip(); });
    redetermineSyncDirection(config, folderCmp, [](const std::wstring&) {});
}

//----------------------------------------------------------------------------------------------

namespace
{
class Redetermine
{
public:
    static void execute(const DirectionSet& dirCfgIn, HierarchyObject& hierObj) { Redetermine(dirCfgIn).recurse(hierObj); }

private:
    Redetermine(const DirectionSet& dirCfgIn) : dirCfg(dirCfgIn) {}

    void recurse(HierarchyObject& hierObj) const
    {
        for (FilePair& fileObj : hierObj.refSubFiles())
            processFile(fileObj);
        for (SymlinkPair& linkObj : hierObj.refSubLinks())
            processLink(linkObj);
        for (DirPair& dirObj : hierObj.refSubDirs())
            processDir(dirObj);
    }

    void processFile(FilePair& fileObj) const
    {
        const CompareFilesResult cat = fileObj.getCategory();

        //##################### schedule old temporary files for deletion ####################
        if (cat == FILE_LEFT_SIDE_ONLY && endsWith(fileObj.getShortName<LEFT_SIDE>(), TEMP_FILE_ENDING))
            return fileObj.setSyncDir(SyncDirection::LEFT);
        else if (cat == FILE_RIGHT_SIDE_ONLY && endsWith(fileObj.getShortName<RIGHT_SIDE>(), TEMP_FILE_ENDING))
            return fileObj.setSyncDir(SyncDirection::RIGHT);
        //####################################################################################

        switch (cat)
        {
            case FILE_LEFT_SIDE_ONLY:
                fileObj.setSyncDir(dirCfg.exLeftSideOnly);
                break;
            case FILE_RIGHT_SIDE_ONLY:
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
            case FILE_DIFFERENT_METADATA: //use setting from "conflict/cannot categorize"
                if (dirCfg.conflict == SyncDirection::NONE)
                    fileObj.setSyncDirConflict(fileObj.getCatExtraDescription()); //take over category conflict
                else
                    fileObj.setSyncDir(dirCfg.conflict);
                break;
            case FILE_EQUAL:
                fileObj.setSyncDir(SyncDirection::NONE);
                break;
        }
    }

    void processLink(SymlinkPair& linkObj) const
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
            case SYMLINK_DIFFERENT_METADATA: //use setting from "conflict/cannot categorize"
                if (dirCfg.conflict == SyncDirection::NONE)
                    linkObj.setSyncDirConflict(linkObj.getCatExtraDescription()); //take over category conflict
                else
                    linkObj.setSyncDir(dirCfg.conflict);
                break;
            case SYMLINK_DIFFERENT:
                linkObj.setSyncDir(dirCfg.different);
                break;
            case SYMLINK_EQUAL:
                linkObj.setSyncDir(SyncDirection::NONE);
                break;
        }
    }

    void processDir(DirPair& dirObj) const
    {
        const CompareDirResult cat = dirObj.getDirCategory();

        //########### schedule abandoned temporary recycle bin directory for deletion  ##########
        if (cat == DIR_LEFT_SIDE_ONLY && endsWith(dirObj.getShortName<LEFT_SIDE>(), TEMP_FILE_ENDING))
            return setSyncDirectionRec(SyncDirection::LEFT, dirObj); //
        else if (cat == DIR_RIGHT_SIDE_ONLY && endsWith(dirObj.getShortName<RIGHT_SIDE>(), TEMP_FILE_ENDING))
            return setSyncDirectionRec(SyncDirection::RIGHT, dirObj); //don't recurse below!
        //#######################################################################################

        switch (cat)
        {
            case DIR_LEFT_SIDE_ONLY:
                dirObj.setSyncDir(dirCfg.exLeftSideOnly);
                break;
            case DIR_RIGHT_SIDE_ONLY:
                dirObj.setSyncDir(dirCfg.exRightSideOnly);
                break;
            case DIR_EQUAL:
                dirObj.setSyncDir(SyncDirection::NONE);
                break;
            case DIR_DIFFERENT_METADATA: //use setting from "conflict/cannot categorize"
                if (dirCfg.conflict == SyncDirection::NONE)
                    dirObj.setSyncDirConflict(dirObj.getCatExtraDescription()); //take over category conflict
                else
                    dirObj.setSyncDir(dirCfg.conflict);
                break;
        }

        recurse(dirObj);
    }

    const DirectionSet dirCfg;
};

//---------------------------------------------------------------------------------------------------------------

//test if non-equal items exist in scanned data
bool allItemsCategoryEqual(const HierarchyObject& hierObj)
{
    return std::all_of(hierObj.refSubFiles().begin(), hierObj.refSubFiles().end(),
    [](const FilePair& fileObj) { return fileObj.getCategory() == FILE_EQUAL; })&&   //files

    std::all_of(hierObj.refSubLinks().begin(), hierObj.refSubLinks().end(),
    [](const SymlinkPair& linkObj) { return linkObj.getLinkCategory() == SYMLINK_EQUAL; })&&   //symlinks

    std::all_of(hierObj.refSubDirs(). begin(), hierObj.refSubDirs().end(),
                [](const DirPair& dirObj)
    {
        return dirObj.getDirCategory() == DIR_EQUAL && allItemsCategoryEqual(dirObj); //short circuit-behavior!
    });    //directories
}
}

bool zen::allElementsEqual(const FolderComparison& folderCmp)
{
    return std::all_of(begin(folderCmp), end(folderCmp), [](const BaseDirPair& baseObj) { return allItemsCategoryEqual(baseObj); });
}

//---------------------------------------------------------------------------------------------------------------

namespace
{
template <SelectedSide side> inline
const InSyncDescrFile& getDescriptor(const InSyncFile& dbFile) { return dbFile.left; }

template <> inline
const InSyncDescrFile& getDescriptor<RIGHT_SIDE>(const InSyncFile& dbFile) { return dbFile.right; }


//check whether database entry and current item match: *irrespective* of current comparison settings
template <SelectedSide side> inline
bool isEqual(const FilePair& fileObj, const InSyncDir::FileList::value_type* dbFile)
{
    if (fileObj.isEmpty<side>())
        return !dbFile;
    else if (!dbFile)
        return false;

    const Zstring&    shortNameDb = dbFile->first;
    const InSyncDescrFile& descrDb = getDescriptor<side>(dbFile->second);

    return fileObj.getShortName<side>() == shortNameDb && //detect changes in case (windows)
           //respect 2 second FAT/FAT32 precision! copying a file to a FAT32 drive changes it's modification date by up to 2 seconds
           sameFileTime(fileObj.getLastWriteTime<side>(), descrDb.lastWriteTimeRaw, 2) &&
           fileObj.getFileSize<side>() == dbFile->second.fileSize;
    //note: we do *not* consider FileId here, but are only interested in *visual* changes. Consider user moving data to some other medium, this is not a change!
}


//check whether database entry is in sync considering *current* comparison settings
inline
bool stillInSync(const InSyncFile& dbFile, CompareVariant compareVar, size_t fileTimeTolerance)
{
    switch (compareVar)
    {
        case CMP_BY_TIME_SIZE:
            if (dbFile.cmpVar == CMP_BY_CONTENT) return true; //special rule: this is already "good enough" for CMP_BY_TIME_SIZE!

            return //case-sensitive short name match is a database invariant!
                CmpFileTime::getResult(dbFile.left.lastWriteTimeRaw, dbFile.right.lastWriteTimeRaw, fileTimeTolerance) == CmpFileTime::TIME_EQUAL;
            //dbFile.left.fileSize == dbFile.right.fileSize;

        case CMP_BY_CONTENT:
            //case-sensitive short name match is a database invariant!
            return dbFile.cmpVar == CMP_BY_CONTENT;
            //in contrast to comparison, we don't care about modification time here!
    }
    assert(false);
    return false;
}

//--------------------------------------------------------------------

template <SelectedSide side> inline
const InSyncDescrLink& getDescriptor(const InSyncSymlink& dbLink) { return dbLink.left; }

template <> inline
const InSyncDescrLink& getDescriptor<RIGHT_SIDE>(const InSyncSymlink& dbLink) { return dbLink.right; }


//check whether database entry and current item match: *irrespective* of current comparison settings
template <SelectedSide side> inline
bool isEqual(const SymlinkPair& linkObj, const InSyncDir::LinkList::value_type* dbLink)
{
    if (linkObj.isEmpty<side>())
        return !dbLink;
    else if (!dbLink)
        return false;

    const Zstring&    shortNameDb = dbLink->first;
    const InSyncDescrLink& descrDb = getDescriptor<side>(dbLink->second);

    return linkObj.getShortName<side>() == shortNameDb &&
           //respect 2 second FAT/FAT32 precision! copying a file to a FAT32 drive changes its modification date by up to 2 seconds
           sameFileTime(linkObj.getLastWriteTime<side>(), descrDb.lastWriteTimeRaw, 2);
}


//check whether database entry is in sync considering *current* comparison settings
inline
bool stillInSync(const InSyncSymlink& dbLink, CompareVariant compareVar, size_t fileTimeTolerance)
{
    switch (compareVar)
    {
        case CMP_BY_TIME_SIZE:
            if (dbLink.cmpVar == CMP_BY_CONTENT) return true; //special rule: this is already "good enough" for CMP_BY_TIME_SIZE!

            return //case-sensitive short name match is a database invariant!
                CmpFileTime::getResult(dbLink.left.lastWriteTimeRaw, dbLink.right.lastWriteTimeRaw, fileTimeTolerance) == CmpFileTime::TIME_EQUAL;

        case CMP_BY_CONTENT:
            //case-sensitive short name match is a database invariant!
            return dbLink.cmpVar == CMP_BY_CONTENT;
            //in contrast to comparison, we don't care about modification time here!
    }
    assert(false);
    return false;
}

//--------------------------------------------------------------------

//check whether database entry and current item match: *irrespective* of current comparison settings
template <SelectedSide side> inline
bool isEqual(const DirPair& dirObj, const InSyncDir::DirList::value_type* dbDir)
{
    if (dirObj.isEmpty<side>())
        return !dbDir || dbDir->second.status == InSyncDir::DIR_STATUS_STRAW_MAN;
    else if (!dbDir || dbDir->second.status == InSyncDir::DIR_STATUS_STRAW_MAN)
        return false;

    const Zstring& shortNameDb = dbDir->first;

    return dirObj.getShortName<side>() == shortNameDb;
}


inline
bool stillInSync(const InSyncDir& dbDir)
{
    //case-sensitive short name match is a database invariant!
    //InSyncDir::DIR_STATUS_STRAW_MAN considered
    return true;
}

//----------------------------------------------------------------------------------------------

class DetectMovedFiles
{
public:
    static void execute(BaseDirPair& baseDirectory, const InSyncDir& dbContainer) { DetectMovedFiles(baseDirectory, dbContainer); }

private:
    DetectMovedFiles(BaseDirPair& baseDirectory, const InSyncDir& dbContainer) :
        cmpVar(baseDirectory.getCompVariant()),
        fileTimeTolerance(baseDirectory.getFileTimeTolerance())
    {
        recurse(baseDirectory);

        if (!exLeftOnly.empty() && !exRightOnly.empty())
            detectFilePairs(dbContainer);
    }

    void recurse(HierarchyObject& hierObj)
    {
        for (FilePair& fileObj : hierObj.refSubFiles())
        {
            const CompareFilesResult cat = fileObj.getCategory();

            if (cat == FILE_LEFT_SIDE_ONLY)
            {
                if (fileObj.getFileId<LEFT_SIDE>() != FileId())
                {
                    auto rv = exLeftOnly.insert(std::make_pair(fileObj.getFileId<LEFT_SIDE>(), &fileObj));
                    assert(rv.second);
                    if (!rv.second) //duplicate file ID!
                        rv.first->second = nullptr;
                }
            }
            else if (cat == FILE_RIGHT_SIDE_ONLY)
            {
                if (fileObj.getFileId<RIGHT_SIDE>() != FileId())
                {
                    auto rv = exRightOnly.insert(std::make_pair(fileObj.getFileId<RIGHT_SIDE>(), &fileObj));
                    assert(rv.second);
                    if (!rv.second) //duplicate file ID!
                        rv.first->second = nullptr;
                }
            }
        }
        for (DirPair& dirObj : hierObj.refSubDirs())
            recurse(dirObj);
    }

    void detectFilePairs(const InSyncDir& container) const
    {
        for (auto& dbFile : container.files)
            findAndSetMovePair(dbFile.second);

        for (auto& dbDir : container.dirs)
            detectFilePairs(dbDir.second);
    }

    static bool sameSizeAndDateLeft(const FilePair& fsObj, const InSyncFile& dbEntry)
    {
        return fsObj.getFileSize<LEFT_SIDE>() == dbEntry.fileSize &&
               sameFileTime(fsObj.getLastWriteTime<LEFT_SIDE>(), dbEntry.left.lastWriteTimeRaw, 2); //respect 2 second FAT/FAT32 precision!
        //PS: *never* allow 2 sec tolerance as container predicate!!
        // => no strict weak ordering relation! reason: no transitivity of equivalence!
    }
    static bool sameSizeAndDateRight(const FilePair& fsObj, const InSyncFile& dbEntry)
    {
        return fsObj.getFileSize<RIGHT_SIDE>() == dbEntry.fileSize &&
               sameFileTime(fsObj.getLastWriteTime<RIGHT_SIDE>(), dbEntry.right.lastWriteTimeRaw, 2);
    }

    void findAndSetMovePair(const InSyncFile& dbEntry) const
    {
        const FileId idLeft  = dbEntry.left .fileId;
        const FileId idRight = dbEntry.right.fileId;

        if (idLeft  != FileId() &&
            idRight != FileId() &&
            stillInSync(dbEntry, cmpVar, fileTimeTolerance))
        {
            auto itL = exLeftOnly.find(idLeft);
            if (itL != exLeftOnly.end())
                if (FilePair* fileLeftOnly = itL->second) //= nullptr, if duplicate ID!
                    if (sameSizeAndDateLeft(*fileLeftOnly, dbEntry))
                    {
                        auto itR = exRightOnly.find(idRight);
                        if (itR != exRightOnly.end())
                            if (FilePair* fileRightOnly = itR->second) //= nullptr, if duplicate ID!
                                if (sameSizeAndDateRight(*fileRightOnly, dbEntry))
                                    if (fileLeftOnly ->getMoveRef() == nullptr && //the db may contain duplicate file ids on left or right side: e.g. consider aliasing through symlinks
                                        fileRightOnly->getMoveRef() == nullptr)   //=> should not be a problem (same id, size, date => alias!) but don't let a row participate in two move pairs!
                                    {
                                        fileLeftOnly ->setMoveRef(fileRightOnly->getId()); //found a pair, mark it!
                                        fileRightOnly->setMoveRef(fileLeftOnly ->getId()); //
                                    }
                    }
        }
    }

    const CompareVariant cmpVar;
    const size_t fileTimeTolerance;

    std::map<FileId, FilePair*> exLeftOnly;  //FilePair* == nullptr for duplicate ids! => consider aliasing through symlinks!
    std::map<FileId, FilePair*> exRightOnly; //=> avoid ambiguity for mixtures of files/symlinks on one side and allow 1-1 mapping only!

    /*
    detect renamed files:

     X  ->  |_|      Create right
    |_| ->   Y       Delete right

    is detected as:

    Rename Y to X on right

    Algorithm:
    ----------
    DB-file left  <--- (name, size, date) --->  DB-file right
       |                                             |
       |  (file ID, size, date)                      |  (file ID, size, date)
      \|/                                           \|/
    file left only                             file right only

       FAT caveat: File Ids are generally not stable when file is either moved or renamed!
       => 1. Move/rename operations on FAT cannot be detected reliably.
       => 2. database generally contains wrong file ID on FAT after renaming from .ffs_tmp files => correct file Ids in database only after next sync
       => 3. even exFAT screws up (but less than FAT) and changes IDs after file move. Did they learn nothing from the past?

    Possible refinement
    -------------------
    If the file ID is wrong (FAT) or not available, we could at least allow direct association by name, instead of breaking the chain completely: support NTFS -> FAT
    */
};

//----------------------------------------------------------------------------------------------

class RedetermineTwoWay
{
public:
    static void execute(BaseDirPair& baseDirectory, const InSyncDir& dbContainer) { RedetermineTwoWay(baseDirectory, dbContainer); }

private:
    RedetermineTwoWay(BaseDirPair& baseDirectory, const InSyncDir& dbContainer) :
        txtBothSidesChanged(_("Both sides have changed since last synchronization.")),
        txtNoSideChanged(_("Cannot determine sync-direction:") + L" \n" + _("No change since last synchronization.")),
        txtDbNotInSync(_("Cannot determine sync-direction:") + L" \n" + _("The database entry is not in sync considering current settings.")),
        cmpVar(baseDirectory.getCompVariant()),
        fileTimeTolerance(baseDirectory.getFileTimeTolerance())
    {
        //-> considering filter not relevant:
        //if narrowing filter: all ok; if widening filter (if file ex on both sides -> conflict, fine; if file ex. on one side: copy to other side: fine)

        recurse(baseDirectory, &dbContainer);
    }

    void recurse(HierarchyObject& hierObj, const InSyncDir* dbContainer) const
    {
        for (FilePair& fileObj : hierObj.refSubFiles())
            processFile(fileObj, dbContainer);
        for (SymlinkPair& linkObj : hierObj.refSubLinks())
            processSymlink(linkObj, dbContainer);
        for (DirPair& dirObj : hierObj.refSubDirs())
            processDir(dirObj, dbContainer);
    }

    void processFile(FilePair& fileObj, const InSyncDir* dbContainer) const
    {
        const CompareFilesResult cat = fileObj.getCategory();
        if (cat == FILE_EQUAL)
            return;

        //##################### schedule old temporary files for deletion ####################
        if (cat == FILE_LEFT_SIDE_ONLY && endsWith(fileObj.getShortName<LEFT_SIDE>(), TEMP_FILE_ENDING))
            return fileObj.setSyncDir(SyncDirection::LEFT);
        else if (cat == FILE_RIGHT_SIDE_ONLY && endsWith(fileObj.getShortName<RIGHT_SIDE>(), TEMP_FILE_ENDING))
            return fileObj.setSyncDir(SyncDirection::RIGHT);
        //####################################################################################

        //try to find corresponding database entry
        const InSyncDir::FileList::value_type* dbEntry = nullptr;
        if (dbContainer)
        {
            auto it = dbContainer->files.find(fileObj.getObjShortName());
            if (it != dbContainer->files.end())
                dbEntry = &*it;
        }

        //evaluation
        const bool changeOnLeft  = !isEqual<LEFT_SIDE >(fileObj, dbEntry);
        const bool changeOnRight = !isEqual<RIGHT_SIDE>(fileObj, dbEntry);

        if (changeOnLeft != changeOnRight)
        {
            //if database entry not in sync according to current settings! -> do not set direction based on async status!
            if (dbEntry && !stillInSync(dbEntry->second, cmpVar, fileTimeTolerance))
                fileObj.setSyncDirConflict(txtDbNotInSync);
            else
                fileObj.setSyncDir(changeOnLeft ? SyncDirection::RIGHT : SyncDirection::LEFT);
        }
        else
        {
            if (changeOnLeft)
                fileObj.setSyncDirConflict(txtBothSidesChanged);
            else
                fileObj.setSyncDirConflict(txtNoSideChanged);
        }
    }

    void processSymlink(SymlinkPair& linkObj, const InSyncDir* dbContainer) const
    {
        const CompareSymlinkResult cat = linkObj.getLinkCategory();
        if (cat == SYMLINK_EQUAL)
            return;

        //try to find corresponding database entry
        const InSyncDir::LinkList::value_type* dbEntry = nullptr;
        if (dbContainer)
        {
            auto it = dbContainer->symlinks.find(linkObj.getObjShortName());
            if (it != dbContainer->symlinks.end())
                dbEntry = &*it;
        }

        //evaluation
        const bool changeOnLeft  = !isEqual<LEFT_SIDE >(linkObj, dbEntry);
        const bool changeOnRight = !isEqual<RIGHT_SIDE>(linkObj, dbEntry);

        if (changeOnLeft != changeOnRight)
        {
            //if database entry not in sync according to current settings! -> do not set direction based on async status!
            if (dbEntry && !stillInSync(dbEntry->second, cmpVar, fileTimeTolerance))
                linkObj.setSyncDirConflict(txtDbNotInSync);
            else
                linkObj.setSyncDir(changeOnLeft ? SyncDirection::RIGHT : SyncDirection::LEFT);
        }
        else
        {
            if (changeOnLeft)
                linkObj.setSyncDirConflict(txtBothSidesChanged);
            else
                linkObj.setSyncDirConflict(txtNoSideChanged);
        }
    }

    void processDir(DirPair& dirObj, const InSyncDir* dbContainer) const
    {
        const CompareDirResult cat = dirObj.getDirCategory();

        //########### schedule abandoned temporary recycle bin directory for deletion  ##########
        if (cat == DIR_LEFT_SIDE_ONLY && endsWith(dirObj.getShortName<LEFT_SIDE>(), TEMP_FILE_ENDING))
            return setSyncDirectionRec(SyncDirection::LEFT, dirObj); //
        else if (cat == DIR_RIGHT_SIDE_ONLY && endsWith(dirObj.getShortName<RIGHT_SIDE>(), TEMP_FILE_ENDING))
            return setSyncDirectionRec(SyncDirection::RIGHT, dirObj); //don't recurse below!
        //#######################################################################################

        //try to find corresponding database entry
        const InSyncDir::DirList::value_type* dbEntry = nullptr;
        if (dbContainer)
        {
            auto it = dbContainer->dirs.find(dirObj.getObjShortName());
            if (it != dbContainer->dirs.end())
                dbEntry = &*it;
        }

        if (cat != DIR_EQUAL)
        {
            //evaluation
            const bool changeOnLeft  = !isEqual<LEFT_SIDE >(dirObj, dbEntry);
            const bool changeOnRight = !isEqual<RIGHT_SIDE>(dirObj, dbEntry);

            if (changeOnLeft != changeOnRight)
            {
                //if database entry not in sync according to current settings! -> do not set direction based on async status!
                if (dbEntry && !stillInSync(dbEntry->second))
                    dirObj.setSyncDirConflict(txtDbNotInSync);
                else
                    dirObj.setSyncDir(changeOnLeft ? SyncDirection::RIGHT : SyncDirection::LEFT);
            }
            else
            {
                if (changeOnLeft)
                    dirObj.setSyncDirConflict(txtBothSidesChanged);
                else
                    dirObj.setSyncDirConflict(txtNoSideChanged);
            }
        }

        recurse(dirObj, dbEntry ? &dbEntry->second : nullptr);
    }

    const std::wstring txtBothSidesChanged;
    const std::wstring txtNoSideChanged;
    const std::wstring txtDbNotInSync;

    const CompareVariant cmpVar;
    const size_t fileTimeTolerance;
};
}

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


void zen::redetermineSyncDirection(const DirectionConfig& dirCfg, BaseDirPair& baseDirectory, std::function<void(const std::wstring&)> reportWarning)
{
    //try to load sync-database files
    std::shared_ptr<InSyncDir> lastSyncState;
    if (dirCfg.var == DirectionConfig::TWOWAY || detectMovedFilesEnabled(dirCfg))
        try
        {
            if (allItemsCategoryEqual(baseDirectory))
                return; //nothing to do: abort and don't even try to open db files

            lastSyncState = loadLastSynchronousState(baseDirectory); //throw FileError, FileErrorDatabaseNotExisting
        }
        catch (FileErrorDatabaseNotExisting&) {} //let's ignore this error, there's no value in reporting it other than confuse users
        catch (FileError& error) //e.g. incompatible database version
        {
            reportWarning(error.toString() +
                          (dirCfg.var == DirectionConfig::TWOWAY ?
                           L" \n\n" + _("Setting default synchronization directions: Old files will be overwritten with newer files.") : std::wstring()));
        }

    //set sync directions
    if (dirCfg.var == DirectionConfig::TWOWAY)
    {
        if (lastSyncState)
            RedetermineTwoWay::execute(baseDirectory, *lastSyncState);
        else //default fallback
            Redetermine::execute(getTwoWayUpdateSet(), baseDirectory);
    }
    else
        Redetermine::execute(extractDirections(dirCfg), baseDirectory);

    //detect renamed files
    if (lastSyncState)
        DetectMovedFiles::execute(baseDirectory, *lastSyncState);
}


void zen::redetermineSyncDirection(const MainConfiguration& mainCfg, FolderComparison& folderCmp, std::function<void(const std::wstring&)> reportWarning)
{
    if (folderCmp.empty())
        return;

    std::vector<DirectionConfig> directCfgs = extractDirectionCfg(mainCfg);

    if (folderCmp.size() != directCfgs.size())
        throw std::logic_error("Programming Error: Contract violation! " + std::string(__FILE__) + ":" + numberTo<std::string>(__LINE__));

    for (auto it = folderCmp.begin(); it != folderCmp.end(); ++it)
    {
        const DirectionConfig& cfg = directCfgs[it - folderCmp.begin()];
        redetermineSyncDirection(cfg, **it, reportWarning);
    }
}

//---------------------------------------------------------------------------------------------------------------

struct SetNewDirection
{
    static void execute(FilePair& fileObj, SyncDirection newDirection)
    {
        if (fileObj.getCategory() != FILE_EQUAL)
            fileObj.setSyncDir(newDirection);
    }

    static void execute(SymlinkPair& linkObj, SyncDirection newDirection)
    {
        if (linkObj.getLinkCategory() != SYMLINK_EQUAL)
            linkObj.setSyncDir(newDirection);
    }

    static void execute(DirPair& dirObj, SyncDirection newDirection)
    {
        if (dirObj.getDirCategory() != DIR_EQUAL)
            dirObj.setSyncDir(newDirection);

        //recurse:
        for (FilePair& fileObj : dirObj.refSubFiles())
            execute(fileObj, newDirection);
        for (SymlinkPair& linkObj : dirObj.refSubLinks())
            execute(linkObj, newDirection);
        for (DirPair& dirObj2 : dirObj.refSubDirs())
            execute(dirObj2, newDirection);
    }
};


void zen::setSyncDirectionRec(SyncDirection newDirection, FileSystemObject& fsObj)
{
    //process subdirectories also!
    struct Recurse: public FSObjectVisitor
    {
        Recurse(SyncDirection newDir) : newDir_(newDir) {}
        virtual void visit(const FilePair& fileObj)
        {
            SetNewDirection::execute(const_cast<FilePair&>(fileObj), newDir_); //phyiscal object is not const in this method anyway
        }
        virtual void visit(const SymlinkPair& linkObj)
        {
            SetNewDirection::execute(const_cast<SymlinkPair&>(linkObj), newDir_); //
        }
        virtual void visit(const DirPair& dirObj)
        {
            SetNewDirection::execute(const_cast<DirPair&>(dirObj), newDir_); //
        }
    private:
        SyncDirection newDir_;
    } setDirVisitor(newDirection);
    fsObj.accept(setDirVisitor);
}

//--------------- functions related to filtering ------------------------------------------------------------------------------------

namespace
{
template <bool include>
void inOrExcludeAllRows(zen::HierarchyObject& hierObj)
{
    for (FilePair& fileObj : hierObj.refSubFiles())
        fileObj.setActive(include);
    for (SymlinkPair& linkObj : hierObj.refSubLinks())
        linkObj.setActive(include);
    for (DirPair& dirObj : hierObj.refSubDirs())
    {
        dirObj.setActive(include);
        inOrExcludeAllRows<include>(dirObj); //recurse
    }
}
}


void zen::setActiveStatus(bool newStatus, zen::FolderComparison& folderCmp)
{
    if (newStatus)
        std::for_each(begin(folderCmp), end(folderCmp), [](BaseDirPair& baseDirObj) { inOrExcludeAllRows<true>(baseDirObj); }); //include all rows
    else
        std::for_each(begin(folderCmp), end(folderCmp), [](BaseDirPair& baseDirObj) { inOrExcludeAllRows<false>(baseDirObj); }); //exclude all rows
}


void zen::setActiveStatus(bool newStatus, zen::FileSystemObject& fsObj)
{
    fsObj.setActive(newStatus);

    //process subdirectories also!
    struct Recurse: public FSObjectVisitor
    {
        Recurse(bool newStat) : newStatus_(newStat) {}
        virtual void visit(const FilePair& fileObj) {}
        virtual void visit(const SymlinkPair& linkObj) {}
        virtual void visit(const DirPair& dirObj)
        {
            if (newStatus_)
                inOrExcludeAllRows<true>(const_cast<DirPair&>(dirObj)); //object is not physically const here anyway
            else
                inOrExcludeAllRows<false>(const_cast<DirPair&>(dirObj)); //
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
    static void execute(HierarchyObject& hierObj, const HardFilter& filterProcIn) { ApplyHardFilter(hierObj, filterProcIn); }

private:
    ApplyHardFilter(HierarchyObject& hierObj, const HardFilter& filterProcIn) : filterProc(filterProcIn)  { recurse(hierObj); }

    void recurse(HierarchyObject& hierObj) const
    {
        for (FilePair& fileObj : hierObj.refSubFiles())
            processFile(fileObj);
        for (SymlinkPair& linkObj : hierObj.refSubLinks())
            processLink(linkObj);
        for (DirPair& dirObj : hierObj.refSubDirs())
            processDir(dirObj);
    };

    void processFile(FilePair& fileObj) const
    {
        if (Eval<strategy>().process(fileObj))
            fileObj.setActive(filterProc.passFileFilter(fileObj.getObjRelativeName()));
    }

    void processLink(SymlinkPair& linkObj) const
    {
        if (Eval<strategy>().process(linkObj))
            linkObj.setActive(filterProc.passFileFilter(linkObj.getObjRelativeName()));
    }

    void processDir(DirPair& dirObj) const
    {
        bool subObjMightMatch = true;
        const bool filterPassed = filterProc.passDirFilter(dirObj.getObjRelativeName(), &subObjMightMatch);

        if (Eval<strategy>().process(dirObj))
            dirObj.setActive(filterPassed);

        if (!subObjMightMatch) //use same logic like directory traversing here: evaluate filter in subdirs only if objects could match
        {
            inOrExcludeAllRows<false>(dirObj); //exclude all files dirs in subfolders
            return;
        }

        recurse(dirObj);
    }

    const HardFilter& filterProc;
};

template <>
class ApplyHardFilter<STRATEGY_OR>; //usage of InOrExcludeAllRows doesn't allow for strategy "or"


template <FilterStrategy strategy>
class ApplySoftFilter //falsify only! -> can run directly after "hard/base filter"
{
public:
    static void execute(HierarchyObject& hierObj, const SoftFilter& timeSizeFilter) { ApplySoftFilter(hierObj, timeSizeFilter); }

private:
    ApplySoftFilter(HierarchyObject& hierObj, const SoftFilter& timeSizeFilter) : timeSizeFilter_(timeSizeFilter) { recurse(hierObj); }

    void recurse(zen::HierarchyObject& hierObj) const
    {
        for (FilePair& fileObj : hierObj.refSubFiles())
            processFile(fileObj);
        for (SymlinkPair& linkObj : hierObj.refSubLinks())
            processLink(linkObj);
        for (DirPair& dirObj : hierObj.refSubDirs())
            processDir(dirObj);
    };

    void processFile(FilePair& fileObj) const
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
                               ST S T -       ST := match size and time
                               ---------       S := match size only
                            ST |X|X|X|X|       T := match time only
                            ------------       - := no match
                             S |X|O|?|O|
                            ------------       X := include row
                             T |X|?|O|O|       O := exclude row
                            ------------       ? := unclear
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

    void processLink(SymlinkPair& linkObj) const
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

    void processDir(DirPair& dirObj) const
    {
        if (Eval<strategy>().process(dirObj))
            dirObj.setActive(timeSizeFilter_.matchFolder()); //if date filter is active we deactivate all folders: effectively gets rid of empty folders!

        recurse(dirObj);
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


void zen::addHardFiltering(BaseDirPair& baseDirObj, const Zstring& excludeFilter)
{
    ApplyHardFilter<STRATEGY_AND>::execute(baseDirObj, NameFilter(FilterConfig().includeFilter, excludeFilter));
}


void zen::addSoftFiltering(BaseDirPair& baseDirObj, const SoftFilter& timeSizeFilter)
{
    if (!timeSizeFilter.isNull()) //since we use STRATEGY_AND, we may skip a "null" filter
        ApplySoftFilter<STRATEGY_AND>::execute(baseDirObj, timeSizeFilter);
}


void zen::applyFiltering(FolderComparison& folderCmp, const MainConfiguration& mainCfg)
{
    if (folderCmp.empty())
        return;
    else if (folderCmp.size() != mainCfg.additionalPairs.size() + 1)
        throw std::logic_error("Programming Error: Contract violation! " + std::string(__FILE__) + ":" + numberTo<std::string>(__LINE__));

    //merge first and additional pairs
    std::vector<FolderPairEnh> allPairs;
    allPairs.push_back(mainCfg.firstPair);
    allPairs.insert(allPairs.end(),
                    mainCfg.additionalPairs.begin(), //add additional pairs
                    mainCfg.additionalPairs.end());

    for (auto it = allPairs.begin(); it != allPairs.end(); ++it)
    {
        BaseDirPair& baseDirectory = *folderCmp[it - allPairs.begin()];

        const NormalizedFilter normFilter = normalizeFilters(mainCfg.globalFilter, it->localFilter);

        //"set" hard filter
        ApplyHardFilter<STRATEGY_SET>::execute(baseDirectory, *normFilter.nameFilter);

        //"and" soft filter
        addSoftFiltering(baseDirectory, normFilter.timeSizeFilter);
    }
}


class FilterByTimeSpan
{
public:
    static void execute(HierarchyObject& hierObj, const Int64& timeFrom, const Int64& timeTo) { FilterByTimeSpan(hierObj, timeFrom, timeTo); }

private:
    FilterByTimeSpan(HierarchyObject& hierObj,
                     const Int64& timeFrom,
                     const Int64& timeTo) :
        timeFrom_(timeFrom),
        timeTo_(timeTo) { recurse(hierObj); }

    void recurse(HierarchyObject& hierObj) const
    {
        for (FilePair& fileObj : hierObj.refSubFiles())
            processFile(fileObj);
        for (SymlinkPair& linkObj : hierObj.refSubLinks())
            processLink(linkObj);
        for (DirPair& dirObj : hierObj.refSubDirs())
            processDir(dirObj);
    };

    void processFile(FilePair& fileObj) const
    {
        if (fileObj.isEmpty<LEFT_SIDE>())
            fileObj.setActive(matchTime<RIGHT_SIDE>(fileObj));
        else if (fileObj.isEmpty<RIGHT_SIDE>())
            fileObj.setActive(matchTime<LEFT_SIDE>(fileObj));
        else
            fileObj.setActive(matchTime<RIGHT_SIDE>(fileObj) ||
                              matchTime<LEFT_SIDE>(fileObj));
    }

    void processLink(SymlinkPair& linkObj) const
    {
        if (linkObj.isEmpty<LEFT_SIDE>())
            linkObj.setActive(matchTime<RIGHT_SIDE>(linkObj));
        else if (linkObj.isEmpty<RIGHT_SIDE>())
            linkObj.setActive(matchTime<LEFT_SIDE>(linkObj));
        else
            linkObj.setActive(matchTime<RIGHT_SIDE>(linkObj) ||
                              matchTime<LEFT_SIDE> (linkObj));
    }

    void processDir(DirPair& dirObj) const
    {
        dirObj.setActive(false);
        recurse(dirObj);
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
    std::for_each(begin(folderCmp), end(folderCmp), [&](BaseDirPair& baseDirObj) { FilterByTimeSpan::execute(baseDirObj, timeFrom, timeTo); });
}


//############################################################################################################
std::pair<Zstring, int> zen::deleteFromGridAndHDPreview(const std::vector<FileSystemObject*>& selectionLeft,
                                                        const std::vector<FileSystemObject*>& selectionRight,
                                                        bool deleteOnBothSides)
{
    //don't use wxString here, it's linear allocation strategy would bring perf down to a crawl; Zstring: exponential growth!
    Zstring fileList;
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
                fileList += fsObj->getFullName<LEFT_SIDE>() + Zstr('\n');
                ++totalDelCount;
            }

            if (!fsObj->isEmpty<RIGHT_SIDE>())
            {
                fileList += fsObj->getFullName<RIGHT_SIDE>() + Zstr('\n');
                ++totalDelCount;
            }

            fileList += Zstr('\n');
        });
    }
    else //delete selected files only
    {
        std::for_each(selectionLeft.begin(), selectionLeft.end(),
                      [&](const FileSystemObject* fsObj)
        {
            if (!fsObj->isEmpty<LEFT_SIDE>())
            {
                fileList += fsObj->getFullName<LEFT_SIDE>() + Zstr('\n');
                ++totalDelCount;
            }
        });

        std::for_each(selectionRight.begin(), selectionRight.end(),
                      [&](const FileSystemObject* fsObj)
        {
            if (!fsObj->isEmpty<RIGHT_SIDE>())
            {
                fileList += fsObj->getFullName<RIGHT_SIDE>() + Zstr('\n');
                ++totalDelCount;
            }
        });
    }

    return std::make_pair(fileList, totalDelCount);
}


namespace
{
template <typename Function> inline
bool tryReportingError(Function cmd, DeleteFilesHandler& handler) //return "true" on success, "false" if error was ignored
{
    for (;;)
        try
        {
            cmd(); //throw FileError
            return true;
        }
        catch (FileError& error)
        {
            switch (handler.reportError(error.toString())) //may throw!
            {
                case DeleteFilesHandler::IGNORE_ERROR:
                    return false;
                case DeleteFilesHandler::RETRY:
                    break; //continue with loop
                default:
                    assert(false);
                    break;
            }
        }
}

#ifdef ZEN_WIN
//recycleBinStatus() blocks seriously if recycle bin is really full and drive is slow
StatusRecycler recycleBinStatusUpdating(const Zstring& dirname, DeleteFilesHandler& callback)
{
    const std::wstring msg = replaceCpy(_("Checking recycle bin availability for folder %x..."), L"%x", fmtFileName(dirname), false);

    auto ft = async([=] { return recycleBinStatus(dirname); });
    while (!ft.timed_wait(boost::posix_time::milliseconds(UI_UPDATE_INTERVAL / 2)))
        callback.reportStatus(msg); //may throw!
    return ft.get();
}
#endif


template <SelectedSide side>
void categorize(const std::set<FileSystemObject*>& rowsIn,
                std::vector<FileSystemObject*>& deletePermanent,
                std::vector<FileSystemObject*>& deleteRecyler,
                bool useRecycleBin,
                std::map<Zstring, bool, LessFilename>& hasRecyclerBuffer,
                DeleteFilesHandler& callback)
{
    auto hasRecycler = [&](const FileSystemObject& fsObj) -> bool
    {
#ifdef ZEN_WIN
        const Zstring& baseDirPf = fsObj.root().getBaseDirPf<side>();

        auto it = hasRecyclerBuffer.find(baseDirPf);
        if (it != hasRecyclerBuffer.end())
            return it->second;
        return hasRecyclerBuffer.insert(std::make_pair(baseDirPf, recycleBinStatusUpdating(baseDirPf, callback) == STATUS_REC_EXISTS)).first->second;
#elif defined ZEN_LINUX || defined ZEN_MAC
        return true;
#endif
    };

    for (auto it = rowsIn.begin(); it != rowsIn.end(); ++it)
        if (!(*it)->isEmpty<side>())
        {
            if (useRecycleBin && hasRecycler(**it)) //Windows' ::SHFileOperation() will delete permanently anyway, but we have a superior deletion routine
                deleteRecyler.push_back(*it);
            else
                deletePermanent.push_back(*it);
        }
}


template <SelectedSide side>
struct ItemDeleter : public FSObjectVisitor  //throw FileError, but nothrow constructor!!!
{
    ItemDeleter(bool useRecycleBin, DeleteFilesHandler& handler) :
        handler_(handler), useRecycleBin_(useRecycleBin), remCallback(*this)
    {
        if (useRecycleBin_)
        {
            txtRemovingFile      = _("Moving file %x to the recycle bin"         );
            txtRemovingDirectory = _("Moving folder %x to the recycle bin"       );
            txtRemovingSymlink   = _("Moving symbolic link %x to the recycle bin");
        }
        else
        {
            txtRemovingFile      = _("Deleting file %x"         );
            txtRemovingDirectory = _("Deleting folder %x"       );
            txtRemovingSymlink   = _("Deleting symbolic link %x");
        }
    }

    virtual void visit(const FilePair& fileObj)
    {
        notifyFileDeletion(fileObj.getFullName<side>());

        if (useRecycleBin_)
            zen::recycleOrDelete(fileObj.getFullName<side>()); //throw FileError
        else
            zen::removeFile(fileObj.getFullName<side>()); //throw FileError
    }

    virtual void visit(const SymlinkPair& linkObj)
    {
        notifySymlinkDeletion(linkObj.getFullName<side>());

        if (useRecycleBin_)
            zen::recycleOrDelete(linkObj.getFullName<side>()); //throw FileError
        else
        {
            if (dirExists(linkObj.getFullName<side>())) //dir symlink
                zen::removeDirectory(linkObj.getFullName<side>()); //throw FileError
            else //file symlink, broken symlink
                zen::removeFile(linkObj.getFullName<side>()); //throw FileError
        }
    }

    virtual void visit(const DirPair& dirObj)
    {
        notifyDirectoryDeletion(dirObj.getFullName<side>()); //notfied twice! see RemoveCallbackImpl -> no big deal

        if (useRecycleBin_)
            zen::recycleOrDelete(dirObj.getFullName<side>()); //throw FileError
        else
            zen::removeDirectory(dirObj.getFullName<side>(), &remCallback); //throw FileError
    }

private:
    struct RemoveCallbackImpl : public zen::CallbackRemoveDir
    {
        RemoveCallbackImpl(ItemDeleter& itemDeleter) : itemDeleter_(itemDeleter) {}

        virtual void onBeforeFileDeletion(const Zstring& filename) { itemDeleter_.notifyFileDeletion     (filename); }
        virtual void onBeforeDirDeletion (const Zstring& dirname)  { itemDeleter_.notifyDirectoryDeletion(dirname ); }

    private:
        ItemDeleter& itemDeleter_;
    };

    void notifyFileDeletion     (const Zstring& objName) { notifyItemDeletion(txtRemovingFile     , objName); }
    void notifyDirectoryDeletion(const Zstring& objName) { notifyItemDeletion(txtRemovingDirectory, objName); }
    void notifySymlinkDeletion  (const Zstring& objName) { notifyItemDeletion(txtRemovingSymlink  , objName); }

    void notifyItemDeletion(const std::wstring& statusText, const Zstring& objName)
    {
        handler_.reportStatus(replaceCpy(statusText, L"%x", fmtFileName(objName)));
    }

    DeleteFilesHandler& handler_;
    const bool useRecycleBin_;
    RemoveCallbackImpl remCallback;

    std::wstring txtRemovingFile;
    std::wstring txtRemovingDirectory;
    std::wstring txtRemovingSymlink;
};


template <SelectedSide side>
void deleteFromGridAndHDOneSide(std::vector<FileSystemObject*>& ptrList,
                                bool useRecycleBin,
                                DeleteFilesHandler& handler)
{
    ItemDeleter<side> deleter(useRecycleBin, handler);

    for (auto it = ptrList.begin(); it != ptrList.end(); ++it) //VS 2010 bug prevents replacing this by std::for_each + lamba
    {
        FileSystemObject& fsObj = **it; //all pointers are required(!) to be bound
        if (!fsObj.isEmpty<side>()) //element may be implicitly deleted, e.g. if parent folder was deleted first
            tryReportingError([&]
        {
            fsObj.accept(deleter); //throw FileError
            fsObj.removeObject<side>(); //if directory: removes recursively!
        }, handler);
    }
}
}

void zen::deleteFromGridAndHD(const std::vector<FileSystemObject*>& rowsToDeleteOnLeft,  //refresh GUI grid after deletion to remove invalid rows
                              const std::vector<FileSystemObject*>& rowsToDeleteOnRight, //all pointers need to be bound!
                              FolderComparison& folderCmp,                         //attention: rows will be physically deleted!
                              const std::vector<DirectionConfig>& directCfgs,
                              bool deleteOnBothSides,
                              bool useRecycleBin,
                              DeleteFilesHandler& statusHandler,
                              bool& warningRecyclerMissing)
{
    if (folderCmp.empty())
        return;
    else if (folderCmp.size() != directCfgs.size())
        throw std::logic_error("Programming Error: Contract violation! " + std::string(__FILE__) + ":" + numberTo<std::string>(__LINE__));

    //build up mapping from base directory to corresponding direction config
    hash_map<const BaseDirPair*, DirectionConfig> baseDirCfgs;
    for (auto it = folderCmp.begin(); it != folderCmp.end(); ++it)
        baseDirCfgs[&** it] = directCfgs[it - folderCmp.begin()];

    std::set<FileSystemObject*> deleteLeft (rowsToDeleteOnLeft .begin(), rowsToDeleteOnLeft .end());
    std::set<FileSystemObject*> deleteRight(rowsToDeleteOnRight.begin(), rowsToDeleteOnRight.end());
    if (deleteOnBothSides)
    {
        deleteLeft.insert(deleteRight.begin(), deleteRight.end());
        deleteRight = deleteLeft;
    }

    set_remove_if(deleteLeft,  [](const FileSystemObject* fsObj) { return fsObj->isEmpty<LEFT_SIDE >(); }); //still needed?
    set_remove_if(deleteRight, [](const FileSystemObject* fsObj) { return fsObj->isEmpty<RIGHT_SIDE>(); }); //

    //ensure cleanup: redetermination of sync-directions and removal of invalid rows
    auto updateDirection = [&]()
    {
        //update sync direction: we cannot do a full redetermination since the user may already have entered manual changes
        std::set<FileSystemObject*> deletedTotal = deleteLeft;
        deletedTotal.insert(deleteRight.begin(), deleteRight.end());

        for (auto it = deletedTotal.begin(); it != deletedTotal.end(); ++it)
        {
            FileSystemObject& fsObj = **it; //all pointers are required(!) to be bound

            if (fsObj.isEmpty<LEFT_SIDE>() != fsObj.isEmpty<RIGHT_SIDE>()) //make sure objects exists on one side only
            {
                auto cfgIter = baseDirCfgs.find(&fsObj.root());
                if (cfgIter != baseDirCfgs.end())
                {
                    SyncDirection newDir = SyncDirection::NONE;

                    if (cfgIter->second.var == DirectionConfig::TWOWAY)
                        newDir = fsObj.isEmpty<LEFT_SIDE>() ? SyncDirection::RIGHT : SyncDirection::LEFT;
                    else
                    {
                        const DirectionSet& dirCfg = extractDirections(cfgIter->second);
                        newDir = fsObj.isEmpty<LEFT_SIDE>() ? dirCfg.exRightSideOnly : dirCfg.exLeftSideOnly;
                    }

                    setSyncDirectionRec(newDir, fsObj); //set new direction (recursively)
                }
                else
                    assert(!"this should not happen!");
            }
        }

        //last step: cleanup empty rows: this one invalidates all pointers!
        std::for_each(begin(folderCmp), end(folderCmp), BaseDirPair::removeEmpty);
    };
    ZEN_ON_SCOPE_EXIT(updateDirection()); //MSVC: assert is a macro and it doesn't play nice with ZEN_ON_SCOPE_EXIT, surprise... wasn't there something about macros being "evil"?

    //categorize rows into permanent deletion and recycle bin
    std::vector<FileSystemObject*> deletePermanentLeft;
    std::vector<FileSystemObject*> deletePermanentRight;
    std::vector<FileSystemObject*> deleteRecylerLeft;
    std::vector<FileSystemObject*> deleteRecylerRight;

    std::map<Zstring, bool, LessFilename> hasRecyclerBuffer;
    categorize<LEFT_SIDE >(deleteLeft,  deletePermanentLeft,  deleteRecylerLeft,  useRecycleBin, hasRecyclerBuffer, statusHandler);
    categorize<RIGHT_SIDE>(deleteRight, deletePermanentRight, deleteRecylerRight, useRecycleBin, hasRecyclerBuffer, statusHandler);

    //windows: check if recycle bin really exists; if not, Windows will silently delete, which is wrong
    if (useRecycleBin &&
    std::any_of(hasRecyclerBuffer.begin(), hasRecyclerBuffer.end(), [](std::pair<Zstring, bool> item) { return !item.second; }))
    {
        std::wstring msg = _("The recycle bin is not available for the following folders. Files will be deleted permanently instead:") + L"\n";

        for (auto it = hasRecyclerBuffer.begin(); it != hasRecyclerBuffer.end(); ++it)
            if (!it->second)
                msg += std::wstring(L"\n") + it->first;

        statusHandler.reportWarning(msg, warningRecyclerMissing);
    }

    deleteFromGridAndHDOneSide<LEFT_SIDE>(deleteRecylerLeft,   true,  statusHandler);
    deleteFromGridAndHDOneSide<LEFT_SIDE>(deletePermanentLeft, false, statusHandler);

    deleteFromGridAndHDOneSide<RIGHT_SIDE>(deleteRecylerRight,   true,  statusHandler);
    deleteFromGridAndHDOneSide<RIGHT_SIDE>(deletePermanentRight, false, statusHandler);
}
