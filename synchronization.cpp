// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) Zenju (zenju AT gmx DOT de) - All Rights Reserved        *
// **************************************************************************

#include "synchronization.h"
#include <memory>
#include <deque>
#include <stdexcept>
#include <wx/file.h> //get rid!?
#include <zen/format_unit.h>
#include <zen/scope_guard.h>
#include <zen/process_priority.h>
#include <zen/file_handling.h>
#include <zen/recycler.h>
#include <zen/optional.h>
#include <zen/symlink_target.h>
#include <zen/file_io.h>
#include <zen/time.h>
#include "lib/resolve_path.h"
#include "lib/db_file.h"
#include "lib/dir_exist_async.h"
#include "lib/cmp_filetime.h"
#include "lib/status_handler_impl.h"
#include "lib/versioning.h"

#ifdef ZEN_WIN
#include <zen/long_path_prefix.h>
#include <zen/perf.h>
#include "lib/shadow.h"
#endif

using namespace zen;


namespace
{
inline
int getCUD(const SyncStatistics& stat)
{
    return stat.getCreate() +
           stat.getUpdate() +
           stat.getDelete();
}
}

void SyncStatistics::init()
{
    createLeft  = 0;
    createRight = 0;
    updateLeft  = 0;
    updateRight = 0;
    deleteLeft  = 0;
    deleteRight = 0;
    rowsTotal   = 0;
}


SyncStatistics::SyncStatistics(const FolderComparison& folderCmp)
{
    init();
    std::for_each(begin(folderCmp), end(folderCmp), [&](const BaseDirPair& baseDirObj) { recurse(baseDirObj); });
}


SyncStatistics::SyncStatistics(const HierarchyObject&  hierObj)
{
    init();
    recurse(hierObj);
}


SyncStatistics::SyncStatistics(const FilePair& fileObj)
{
    init();
    processFile(fileObj);
    rowsTotal += 1;
}


inline
void SyncStatistics::recurse(const HierarchyObject& hierObj)
{
    for (const FilePair& fileObj : hierObj.refSubFiles())
        processFile(fileObj);
    for (const SymlinkPair& linkObj : hierObj.refSubLinks())
        processLink(linkObj);
    for (const DirPair& dirObj : hierObj.refSubDirs())
        processDir(dirObj);

    rowsTotal += hierObj.refSubDirs(). size();
    rowsTotal += hierObj.refSubFiles().size();
    rowsTotal += hierObj.refSubLinks().size();
}


inline
void SyncStatistics::processFile(const FilePair& fileObj)
{
    switch (fileObj.getSyncOperation()) //evaluate comparison result and sync direction
    {
        case SO_CREATE_NEW_LEFT:
            ++createLeft;
            dataToProcess += to<Int64>(fileObj.getFileSize<RIGHT_SIDE>());
            break;

        case SO_CREATE_NEW_RIGHT:
            ++createRight;
            dataToProcess += to<Int64>(fileObj.getFileSize<LEFT_SIDE>());
            break;

        case SO_DELETE_LEFT:
            ++deleteLeft;
            break;

        case SO_DELETE_RIGHT:
            ++deleteRight;
            break;

        case SO_MOVE_LEFT_TARGET:
            ++updateLeft;
            break;

        case SO_MOVE_RIGHT_TARGET:
            ++updateRight;
            break;

        case SO_MOVE_LEFT_SOURCE:  //ignore; already counted
        case SO_MOVE_RIGHT_SOURCE: //
            break;

        case SO_OVERWRITE_LEFT:
            ++updateLeft;
            dataToProcess += to<Int64>(fileObj.getFileSize<RIGHT_SIDE>());
            break;

        case SO_OVERWRITE_RIGHT:
            ++updateRight;
            dataToProcess += to<Int64>(fileObj.getFileSize<LEFT_SIDE>());
            break;

        case SO_UNRESOLVED_CONFLICT:
            conflictMsgs.push_back(std::make_pair(fileObj.getObjRelativeName(), fileObj.getSyncOpConflict()));
            break;

        case SO_COPY_METADATA_TO_LEFT:
            ++updateLeft;
            break;

        case SO_COPY_METADATA_TO_RIGHT:
            ++updateRight;
            break;

        case SO_DO_NOTHING:
        case SO_EQUAL:
            break;
    }
}


inline
void SyncStatistics::processLink(const SymlinkPair& linkObj)
{
    switch (linkObj.getSyncOperation()) //evaluate comparison result and sync direction
    {
        case SO_CREATE_NEW_LEFT:
            ++createLeft;
            break;

        case SO_CREATE_NEW_RIGHT:
            ++createRight;
            break;

        case SO_DELETE_LEFT:
            ++deleteLeft;
            break;

        case SO_DELETE_RIGHT:
            ++deleteRight;
            break;

        case SO_OVERWRITE_LEFT:
        case SO_COPY_METADATA_TO_LEFT:
            ++updateLeft;
            break;

        case SO_OVERWRITE_RIGHT:
        case SO_COPY_METADATA_TO_RIGHT:
            ++updateRight;
            break;

        case SO_UNRESOLVED_CONFLICT:
            conflictMsgs.push_back(std::make_pair(linkObj.getObjRelativeName(), linkObj.getSyncOpConflict()));
            break;

        case SO_MOVE_LEFT_SOURCE:
        case SO_MOVE_RIGHT_SOURCE:
        case SO_MOVE_LEFT_TARGET:
        case SO_MOVE_RIGHT_TARGET:
            assert(false);
        case SO_DO_NOTHING:
        case SO_EQUAL:
            break;
    }
}


inline
void SyncStatistics::processDir(const DirPair& dirObj)
{
    switch (dirObj.getSyncOperation()) //evaluate comparison result and sync direction
    {
        case SO_CREATE_NEW_LEFT:
            ++createLeft;
            break;

        case SO_CREATE_NEW_RIGHT:
            ++createRight;
            break;

        case SO_DELETE_LEFT: //if deletion variant == user-defined directory existing on other volume, this results in a full copy + delete operation!
            ++deleteLeft;    //however we cannot (reliably) anticipate this situation, fortunately statistics can be adapted during sync!
            break;

        case SO_DELETE_RIGHT:
            ++deleteRight;
            break;

        case SO_UNRESOLVED_CONFLICT:
            conflictMsgs.push_back(std::make_pair(dirObj.getObjRelativeName(), dirObj.getSyncOpConflict()));
            break;

        case SO_COPY_METADATA_TO_LEFT:
            ++updateLeft;
            break;

        case SO_COPY_METADATA_TO_RIGHT:
            ++updateRight;
            break;

        case SO_OVERWRITE_LEFT:
        case SO_OVERWRITE_RIGHT:
        case SO_MOVE_LEFT_SOURCE:
        case SO_MOVE_RIGHT_SOURCE:
        case SO_MOVE_LEFT_TARGET:
        case SO_MOVE_RIGHT_TARGET:
            assert(false);
        case SO_DO_NOTHING:
        case SO_EQUAL:
            break;
    }

    recurse(dirObj); //since we model logical stats, we recurse, even if deletion variant is "recycler" or "versioning + same volume", which is a single physical operation!
}

//-----------------------------------------------------------------------------------------------------------

std::vector<zen::FolderPairSyncCfg> zen::extractSyncCfg(const MainConfiguration& mainCfg)
{
    //merge first and additional pairs
    std::vector<FolderPairEnh> allPairs;
    allPairs.push_back(mainCfg.firstPair);
    allPairs.insert(allPairs.end(),
                    mainCfg.additionalPairs.begin(), //add additional pairs
                    mainCfg.additionalPairs.end());

    std::vector<FolderPairSyncCfg> output;

    //process all pairs
    for (const FolderPairEnh& fp : allPairs)
    {
        SyncConfig syncCfg = fp.altSyncConfig.get() ? *fp.altSyncConfig : mainCfg.syncCfg;

        output.push_back(
            FolderPairSyncCfg(syncCfg.directionCfg.var == DirectionConfig::TWOWAY || detectMovedFilesEnabled(syncCfg.directionCfg),
                              syncCfg.handleDeletion,
                              syncCfg.versioningStyle,
                              getFormattedDirectoryName(syncCfg.versioningDirectory)));
    }
    return output;
}

//------------------------------------------------------------------------------------------------------------

//test if user accidentally selected the wrong folders to sync
bool significantDifferenceDetected(const SyncStatistics& folderPairStat)
{
    //initial file copying shall not be detected as major difference
    if ((folderPairStat.getCreate<LEFT_SIDE >() == 0 ||
         folderPairStat.getCreate<RIGHT_SIDE>() == 0) &&
        folderPairStat.getUpdate  () == 0 &&
        folderPairStat.getDelete  () == 0 &&
        folderPairStat.getConflict() == 0)
        return false;

    const int nonMatchingRows = folderPairStat.getCreate() +
                                folderPairStat.getDelete();
    //folderPairStat.getUpdate() +  -> not relevant when testing for "wrong folder selected"
    //folderPairStat.getConflict();

    return nonMatchingRows >= 10 && nonMatchingRows > 0.5 * folderPairStat.getRowCount();
}

//#################################################################################################################

class DeletionHandling //abstract deletion variants: permanently, recycle bin, user-defined directory
{
public:
    DeletionHandling(DeletionPolicy handleDel, //nothrow!
                     const Zstring& versioningDir,
                     VersioningStyle versioningStyle,
                     const TimeComp& timeStamp,
                     const Zstring& baseDirPf, //with separator postfix
                     ProcessCallback& procCallback);
    ~DeletionHandling()
    {
        //always (try to) clean up, even if synchronization is aborted!
        try
        {
            tryCleanup(false); //throw FileError, (throw X)
        }
        catch (FileError&) {}
        catch (...) { assert(false); }  //what is this?
        /*
        may block heavily, but still do not allow user callback:
        -> avoid throwing user cancel exception again, leading to incomplete clean-up!
        */
    }

    //clean-up temporary directory (recycle bin optimization)
    void tryCleanup(bool allowUserCallback = true); //throw FileError; throw X -> call this in non-exceptional coding, i.e. somewhere after sync!

    template <class Function> void removeFileUpdating(const Zstring& fullName, const Zstring& relativeName, const Int64& bytesExpected, Function notifyItemDeletion); //throw FileError
    template <class Function> void removeDirUpdating (const Zstring& fullName, const Zstring& relativeName, const Int64& bytesExpected, Function notifyItemDeletion); //reports ONLY data delta via updateProcessedData()!
    template <class Function> void removeLinkUpdating(const Zstring& fullName, const Zstring& relativeName, const Int64& bytesExpected, Function notifyItemDeletion); //

    const std::wstring& getTxtRemovingFile   () const { return txtRemovingFile;      } //
    const std::wstring& getTxtRemovingSymLink() const { return txtRemovingSymlink;   } //buffered status texts
    const std::wstring& getTxtRemovingDir    () const { return txtRemovingDirectory; } //

private:
    DeletionHandling(const DeletionHandling&);
    DeletionHandling& operator=(const DeletionHandling&);

    FileVersioner& getOrCreateVersioner() //throw FileError! => dont create in DeletionHandling()!!!
    {
        if (!versioner.get())
            versioner = make_unique<FileVersioner>(versioningDir_, versioningStyle_, timeStamp_); //throw FileError
        return *versioner;
    };

    ProcessCallback& procCallback_;
    const Zstring baseDirPf_;  //ends with path separator
    const Zstring versioningDir_;
    const VersioningStyle versioningStyle_;
    const TimeComp timeStamp_;

#ifdef ZEN_WIN
    Zstring getOrCreateRecyclerTempDirPf(); //throw FileError
    Zstring recyclerTmpDir; //temporary folder holding files/folders for *deferred* recycling
    std::vector<Zstring> toBeRecycled; //full path of files located in temporary folder, waiting for batch-recycling
#endif

    //magage three states: allow dynamic fallback from recycler to permanent deletion
    const DeletionPolicy deletionPolicy_;
    std::unique_ptr<FileVersioner> versioner; //used for DELETE_TO_VERSIONING; throw FileError in constructor => create on demand!

    //buffer status texts:
    std::wstring txtRemovingFile;
    std::wstring txtRemovingSymlink;
    std::wstring txtRemovingDirectory;
};


DeletionHandling::DeletionHandling(DeletionPolicy handleDel, //nothrow!
                                   const Zstring& versioningDir,
                                   VersioningStyle versioningStyle,
                                   const TimeComp& timeStamp,
                                   const Zstring& baseDirPf, //with separator postfix
                                   ProcessCallback& procCallback) :
    procCallback_(procCallback),
    baseDirPf_(baseDirPf),
    versioningDir_(versioningDir),
    versioningStyle_(versioningStyle),
    timeStamp_(timeStamp),
    deletionPolicy_(handleDel)
{
    switch (deletionPolicy_)
    {
        case DELETE_PERMANENTLY:
            txtRemovingFile      = _("Deleting file %x"         );
            txtRemovingDirectory = _("Deleting folder %x"       );
            txtRemovingSymlink   = _("Deleting symbolic link %x");
            break;

        case DELETE_TO_RECYCLER:
            txtRemovingFile      = _("Moving file %x to the recycle bin"         );
            txtRemovingDirectory = _("Moving folder %x to the recycle bin"       );
            txtRemovingSymlink   = _("Moving symbolic link %x to the recycle bin");
            break;

        case DELETE_TO_VERSIONING:
            txtRemovingFile      = replaceCpy(_("Moving file %x to %y"         ), L"%y", fmtFileName(versioningDir_));
            txtRemovingDirectory = replaceCpy(_("Moving folder %x to %y"       ), L"%y", fmtFileName(versioningDir_));
            txtRemovingSymlink   = replaceCpy(_("Moving symbolic link %x to %y"), L"%y", fmtFileName(versioningDir_));
            break;
    }
}


#ifdef ZEN_WIN
namespace
{
class CallbackMassRecycling : public CallbackRecycling
{
public:
    CallbackMassRecycling(ProcessCallback& statusHandler) :
        statusHandler_(statusHandler),
        txtRecyclingFile(_("Moving file %x to the recycle bin")) {}

    //may throw: first exception is swallowed, updateStatus() is then called again where it should throw again and the exception will propagate as expected
    virtual void updateStatus(const Zstring& currentItem)
    {
        if (!currentItem.empty())
            statusHandler_.reportStatus(replaceCpy(txtRecyclingFile, L"%x", fmtFileName(currentItem))); //throw ?
        else
            statusHandler_.requestUiRefresh(); //throw ?
    }

private:
    ProcessCallback& statusHandler_;
    const std::wstring txtRecyclingFile;
};
}

//create + returns temporary directory postfixed with file name separator
//to support later cleanup if automatic deletion fails for whatever reason
Zstring DeletionHandling::getOrCreateRecyclerTempDirPf() //throw FileError
{
    assert(!baseDirPf_.empty());
    if (baseDirPf_.empty())
        return Zstring();

    if (recyclerTmpDir.empty())
    {
        recyclerTmpDir = [&]
        {
            assert(endsWith(baseDirPf_, FILE_NAME_SEPARATOR));
            /*
            -> this naming convention is too cute and confusing for end users:

            //1. generate random directory name
            static std::mt19937 rng(std::time(nullptr)); //don't use std::default_random_engine and leave the choice to the STL implementer!
            //- the alternative std::random_device may not always be available and can even throw an exception!
            //- seed with second precision is sufficient: collisions are handled below

            const Zstring chars(Zstr("abcdefghijklmnopqrstuvwxyz")
                                Zstr("1234567890"));
            std::uniform_int_distribution<size_t> distrib(0, chars.size() - 1); //takes closed range

            auto generatePath = [&]() -> Zstring //e.g. C:\Source\3vkf74fq.ffs_tmp
            {
                Zstring path = baseDirPf;
                for (int i = 0; i < 8; ++i)
                    path += chars[distrib(rng)];
                return path + TEMP_FILE_ENDING;
            };
            */

            //ensure unique ownership:
            Zstring dirname = baseDirPf_ + Zstr("RecycleBin") + TEMP_FILE_ENDING;
            for (int i = 1;; ++i)
                try
                {
                    makeDirectory(dirname, /*bool failIfExists*/ true); //throw FileError, ErrorTargetExisting
                    return dirname;
                }
                catch (const ErrorTargetExisting&)
                {
                    dirname = baseDirPf_ + Zstr("RecycleBin") + Zchar('_') + numberTo<Zstring>(i) + TEMP_FILE_ENDING;
                }
        }();
    }
    //assemble temporary recycle bin directory with random name and .ffs_tmp ending
    return appendSeparator(recyclerTmpDir);
}
#endif


void DeletionHandling::tryCleanup(bool allowUserCallback) //throw FileError; throw X
{
    switch (deletionPolicy_)
    {
        case DELETE_PERMANENTLY:
            break;

        case DELETE_TO_RECYCLER:
#ifdef ZEN_WIN
            if (!toBeRecycled.empty())
            {
                //move content of temporary directory to recycle bin in a single call
                CallbackMassRecycling cbmr(procCallback_);
                recycleOrDelete(toBeRecycled, allowUserCallback ? &cbmr : nullptr); //throw FileError
                toBeRecycled.clear();
            }

            //clean up temp directory itself (should contain remnant empty directories only)
            if (!recyclerTmpDir.empty())
            {
                removeDirectory(recyclerTmpDir); //throw FileError
                recyclerTmpDir.clear();
            }
#endif
            break;

        case DELETE_TO_VERSIONING:
            //if (versioner.get())
            //{
            //    if (allowUserCallback)
            //    {
            //        procCallback_.reportStatus(_("Removing old versions...")); //throw ?
            //        versioner->limitVersions([&] { procCallback_.requestUiRefresh(); /*throw ? */ }); //throw FileError
            //    }
            //    else
            //        versioner->limitVersions([] {}); //throw FileError
            //}
            break;
    }
}


namespace
{
template <class Function>
struct CallbackRemoveDirImpl : public CallbackRemoveDir
{
    CallbackRemoveDirImpl(ProcessCallback& statusHandler,
                          const DeletionHandling& delHandling,
                          Function notifyItemDeletion) :
        statusHandler_(statusHandler),
        notifyItemDeletion_(notifyItemDeletion),
        txtDeletingFile  (delHandling.getTxtRemovingFile()),
        txtDeletingFolder(delHandling.getTxtRemovingDir ()) {}

private:
    virtual void onBeforeFileDeletion(const Zstring& filename) { notifyDeletion(txtDeletingFile,   filename); }
    virtual void onBeforeDirDeletion (const Zstring& dirname ) { notifyDeletion(txtDeletingFolder, dirname ); }

    void notifyDeletion(const std::wstring& statusText, const Zstring& objName)
    {
        notifyItemDeletion_(); //it would be more correct to report *after* work was done!
        statusHandler_.reportStatus(replaceCpy(statusText, L"%x", fmtFileName(objName)));
    }

    ProcessCallback& statusHandler_;
    Function notifyItemDeletion_;
    const std::wstring txtDeletingFile;
    const std::wstring txtDeletingFolder;
};


template <class Function>
class CallbackMoveDirImpl : public CallbackMoveDir
{
public:
    CallbackMoveDirImpl(ProcessCallback& callback,
                        Int64& bytesReported,
                        Function notifyItemDeletion) :
        callback_       (callback),
        bytesReported_(bytesReported),
        notifyItemDeletion_(notifyItemDeletion),
        txtMovingFile   (_("Moving file %x to %y")),
        txtMovingFolder (_("Moving folder %x to %y")) {}

private:
    virtual void onBeforeFileMove(const Zstring& fileFrom, const Zstring& fileTo) { notifyMove(txtMovingFile, fileFrom, fileTo); }
    virtual void onBeforeDirMove (const Zstring& dirFrom,  const Zstring& dirTo ) { notifyMove(txtMovingFolder, dirFrom, dirTo); }

    void notifyMove(const std::wstring& statusText, const Zstring& fileFrom, const Zstring& fileTo) const
    {
        notifyItemDeletion_(); //it would be more correct to report *after* work was done!
        callback_.reportStatus(replaceCpy(replaceCpy(statusText, L"%x", L"\n" + fmtFileName(fileFrom)), L"%y", L"\n" + fmtFileName(fileTo)));
    };

    virtual void updateStatus(Int64 bytesDelta)
    {
        callback_.updateProcessedData(0, bytesDelta); //throw()! -> ensure client and service provider are in sync!
        bytesReported_ += bytesDelta;                 //

        callback_.requestUiRefresh(); //may throw
    }

    ProcessCallback& callback_;
    Int64& bytesReported_;
    Function notifyItemDeletion_;
    const std::wstring txtMovingFile;
    const std::wstring txtMovingFolder;
};
}


template <class Function>
void DeletionHandling::removeDirUpdating(const Zstring& fullName,
                                         const Zstring& relativeName,
                                         const Int64& bytesExpected, Function notifyItemDeletion) //throw FileError
{
    Int64 bytesReported;
    ScopeGuard guardStatistics = makeGuard([&] { procCallback_.updateTotalData(0, bytesReported); }); //error = unexpected increase of total workload

    switch (deletionPolicy_)
    {
        case DELETE_PERMANENTLY:
        {
            CallbackRemoveDirImpl<Function> remDirCallback(procCallback_, *this, notifyItemDeletion);
            removeDirectory(fullName, &remDirCallback);
        }
        break;

        case DELETE_TO_RECYCLER:
        {
#ifdef ZEN_WIN
            const Zstring targetDir = getOrCreateRecyclerTempDirPf() + relativeName; //throw FileError
            bool deleted = false;

            auto moveToTempDir = [&]
            {
                try
                {
                    //performance optimization: Instead of moving each object into recycle bin separately,
                    //we rename them one by one into a temporary directory and batch-recycle this directory after sync
                    renameFile(fullName, targetDir); //throw FileError, ErrorDifferentVolume
                    this->toBeRecycled.push_back(targetDir);
                    deleted = true;
                }
                catch (ErrorDifferentVolume&) //MoveFileEx() returns ERROR_PATH_NOT_FOUND *before* considering ERROR_NOT_SAME_DEVICE! => we have to create targetDir in any case!
                {
                    deleted = recycleOrDelete(fullName); //throw FileError
                }
            };

            try
            {
                moveToTempDir(); //throw FileError, ErrorDifferentVolume
            }
            catch (FileError&)
            {
                if (somethingExists(fullName))
                {
                    const Zstring targetSuperDir = beforeLast(targetDir, FILE_NAME_SEPARATOR); //what if C:\ ?
                    if (!dirExists(targetSuperDir))
                    {
                        makeDirectory(targetSuperDir); //throw FileError -> may legitimately fail on Linux if permissions are missing
                        moveToTempDir(); //throw FileError -> this should work now!
                    }
                    else
                        throw;
                }
            }
#elif defined ZEN_LINUX || defined ZEN_MAC
            const bool deleted = recycleOrDelete(fullName); //throw FileError
#endif
            if (deleted)
                notifyItemDeletion(); //moving to recycler is ONE logical operation, irrespective of the number of child elements!
        }
        break;

        case DELETE_TO_VERSIONING:
        {
            CallbackMoveDirImpl<Function> callback(procCallback_, bytesReported, notifyItemDeletion);
            getOrCreateVersioner().revisionDir(fullName, relativeName, callback); //throw FileError
        }
        break;
    }

    //update statistics to consider the real amount of data
    guardStatistics.dismiss();
    if (bytesReported != bytesExpected)
        procCallback_.updateTotalData(0, bytesReported - bytesExpected); //noexcept!
}


template <class Function>
void DeletionHandling::removeFileUpdating(const Zstring& fullName,
                                          const Zstring& relativeName,
                                          const Int64& bytesExpected, Function notifyItemDeletion) //throw FileError
{
    Int64 bytesReported;
    auto guardStatistics = makeGuard([&] { procCallback_.updateTotalData(0, bytesReported); }); //error = unexpected increase of total workload
    bool deleted = false;

    if (endsWith(relativeName, TEMP_FILE_ENDING)) //special rule for .ffs_tmp files: always delete permanently!
        deleted = zen::removeFile(fullName);
    else
        switch (deletionPolicy_)
        {
            case DELETE_PERMANENTLY:
                deleted = zen::removeFile(fullName); //[!] scope specifier resolves nameclash!
                break;

            case DELETE_TO_RECYCLER:
#ifdef ZEN_WIN
                {
                    const Zstring targetFile = getOrCreateRecyclerTempDirPf() + relativeName; //throw FileError

                    auto moveToTempDir = [&]
                    {
                        try
                        {
                            //performance optimization: Instead of moving each object into recycle bin separately,
                            //we rename them one by one into a temporary directory and batch-recycle this directory after sync
                            renameFile(fullName, targetFile); //throw FileError, ErrorDifferentVolume
                            this->toBeRecycled.push_back(targetFile);
                            deleted = true;
                        }
                        catch (ErrorDifferentVolume&) //MoveFileEx() returns ERROR_PATH_NOT_FOUND *before* considering ERROR_NOT_SAME_DEVICE! => we have to create targetDir in any case!
                        {
                            deleted = recycleOrDelete(fullName); //throw FileError
                        }
                    };

                    try
                    {
                        moveToTempDir(); //throw FileError, ErrorDifferentVolume
                    }
                    catch (FileError&)
                    {
                        if (somethingExists(fullName))
                        {
                            const Zstring targetDir = beforeLast(targetFile, FILE_NAME_SEPARATOR);
                            if (!dirExists(targetDir))
                            {
                                makeDirectory(targetDir); //throw FileError -> may legitimately fail on Linux if permissions are missing
                                moveToTempDir(); //throw FileError -> this should work now!
                            }
                            else
                                throw;
                        }
                    }
                }
#elif defined ZEN_LINUX || defined ZEN_MAC
                deleted = recycleOrDelete(fullName); //throw FileError
#endif
                break;

            case DELETE_TO_VERSIONING:
            {
                struct CallbackMoveFileImpl : public CallbackMoveFile
                {
                    CallbackMoveFileImpl(ProcessCallback& callback, Int64& bytes) : callback_(callback), bytesReported_(bytes)  {}

                private:
                    virtual void updateStatus(Int64 bytesDelta)
                    {
                        callback_.updateProcessedData(0, bytesDelta); //throw()! -> ensure client and service provider are in sync!
                        bytesReported_ += bytesDelta;                 //

                        callback_.requestUiRefresh(); //may throw
                    }
                    ProcessCallback& callback_;
                    Int64& bytesReported_;
                } cb(procCallback_, bytesReported);

                deleted = getOrCreateVersioner().revisionFile(fullName, relativeName, cb); //throw FileError
            }
            break;
        }
    if (deleted)
        notifyItemDeletion();

    //update statistics to consider the real amount of data
    guardStatistics.dismiss();
    if (bytesReported != bytesExpected)
        procCallback_.updateTotalData(0, bytesReported - bytesExpected); //noexcept!
}


template <class Function> inline
void DeletionHandling::removeLinkUpdating(const Zstring& fullName, const Zstring& relativeName, const Int64& bytesExpected, Function notifyItemDeletion) //throw FileError
{
    if (dirExists(fullName)) //dir symlink
        return removeDirUpdating(fullName, relativeName, bytesExpected, notifyItemDeletion); //throw FileError
    else //file symlink, broken symlink
        return removeFileUpdating(fullName, relativeName, bytesExpected, notifyItemDeletion); //throw FileError
}

//------------------------------------------------------------------------------------------------------------

namespace
{
/*
  DELETE_PERMANENTLY:   deletion frees space
  DELETE_TO_RECYCLER:   won't free space until recycler is full, but then frees space
  DELETE_TO_VERSIONING: depends on whether versioning folder is on a different volume
-> if deleted item is a followed symlink, no space is freed
-> created/updated/deleted item may be on a different volume than base directory: consider symlinks, junctions!

=> generally assume deletion frees space; may avoid false positive disk space warnings for recycler and versioning
*/
class MinimumDiskSpaceNeeded
{
public:
    static std::pair<Int64, Int64> calculate(const BaseDirPair& baseObj)
    {
        MinimumDiskSpaceNeeded inst;
        inst.recurse(baseObj);
        return std::make_pair(inst.spaceNeededLeft, inst.spaceNeededRight);
    }

private:
    MinimumDiskSpaceNeeded() {}

    void recurse(const HierarchyObject& hierObj)
    {
        //don't process directories

        //process files
        for (auto it = hierObj.refSubFiles().begin(); it != hierObj.refSubFiles().end(); ++it)
            switch (it->getSyncOperation()) //evaluate comparison result and sync direction
            {
                case SO_CREATE_NEW_LEFT:
                    spaceNeededLeft += to<Int64>(it->getFileSize<RIGHT_SIDE>());
                    break;

                case SO_CREATE_NEW_RIGHT:
                    spaceNeededRight += to<Int64>(it->getFileSize<LEFT_SIDE>());
                    break;

                case SO_DELETE_LEFT:
                    //if (freeSpaceDelLeft_)
                    spaceNeededLeft -= to<Int64>(it->getFileSize<LEFT_SIDE>());
                    break;

                case SO_DELETE_RIGHT:
                    //if (freeSpaceDelRight_)
                    spaceNeededRight -= to<Int64>(it->getFileSize<RIGHT_SIDE>());
                    break;

                case SO_OVERWRITE_LEFT:
                    //if (freeSpaceDelLeft_)
                    spaceNeededLeft -= to<Int64>(it->getFileSize<LEFT_SIDE>());
                    spaceNeededLeft += to<Int64>(it->getFileSize<RIGHT_SIDE>());
                    break;

                case SO_OVERWRITE_RIGHT:
                    //if (freeSpaceDelRight_)
                    spaceNeededRight -= to<Int64>(it->getFileSize<RIGHT_SIDE>());
                    spaceNeededRight += to<Int64>(it->getFileSize<LEFT_SIDE>());
                    break;

                case SO_DO_NOTHING:
                case SO_EQUAL:
                case SO_UNRESOLVED_CONFLICT:
                case SO_COPY_METADATA_TO_LEFT:
                case SO_COPY_METADATA_TO_RIGHT:
                case SO_MOVE_LEFT_SOURCE:
                case SO_MOVE_RIGHT_SOURCE:
                case SO_MOVE_LEFT_TARGET:
                case SO_MOVE_RIGHT_TARGET:
                    break;
            }

        //symbolic links
        //[...]

        //recurse into sub-dirs
        for (auto& subDir : hierObj.refSubDirs())
            recurse(subDir);
    }

    Int64 spaceNeededLeft;
    Int64 spaceNeededRight;
};

//----------------------------------------------------------------------------------------

class SynchronizeFolderPair
{
public:
    SynchronizeFolderPair(ProcessCallback& procCallback,
                          bool verifyCopiedFiles,
                          bool copyFilePermissions,
                          bool transactionalFileCopy,
#ifdef ZEN_WIN
                          shadow::ShadowCopy* shadowCopyHandler,
#endif
                          DeletionHandling& delHandlingLeft,
                          DeletionHandling& delHandlingRight) :
        procCallback_(procCallback),
#ifdef ZEN_WIN
        shadowCopyHandler_(shadowCopyHandler),
#endif
        delHandlingLeft_(delHandlingLeft),
        delHandlingRight_(delHandlingRight),
        verifyCopiedFiles_(verifyCopiedFiles),
        copyFilePermissions_(copyFilePermissions),
        transactionalFileCopy_(transactionalFileCopy),
        txtCreatingFile     (_("Creating file %x"            )),
        txtCreatingLink     (_("Creating symbolic link %x"   )),
        txtCreatingFolder   (_("Creating folder %x"          )),
        txtOverwritingFile  (_("Overwriting file %x"         )),
        txtOverwritingLink  (_("Overwriting symbolic link %x")),
        txtVerifying        (_("Verifying file %x"           )),
        txtWritingAttributes(_("Updating attributes of %x"   )),
        txtMovingFile       (_("Moving file %x to %y"))
    {}

    void startSync(BaseDirPair& baseDirObj)
    {
        runZeroPass(baseDirObj);       //first process file moves
        runPass<PASS_ONE>(baseDirObj); //delete files (or overwrite big ones with smaller ones)
        runPass<PASS_TWO>(baseDirObj); //copy rest
    }

private:
    enum PassId
    {
        PASS_ONE, //delete files
        PASS_TWO, //create, modify
        PASS_NEVER //skip
    };

    static PassId getPass(const FilePair&    fileObj);
    static PassId getPass(const SymlinkPair& linkObj);
    static PassId getPass(const DirPair&     dirObj);

    template <SelectedSide side>
    void prepare2StepMove(FilePair& sourceObj, FilePair& targetObj); //throw FileError
    bool createParentDir(FileSystemObject& fsObj); //throw FileError
    template <SelectedSide side>
    void manageFileMove(FilePair& sourceObj, FilePair& targetObj); //throw FileError

    void runZeroPass(HierarchyObject& hierObj);
    template <PassId pass>
    void runPass(HierarchyObject& hierObj);

    void synchronizeFile(FilePair& fileObj);
    template <SelectedSide side> void synchronizeFileInt(FilePair& fileObj, SyncOperation syncOp);

    void synchronizeLink(SymlinkPair& linkObj);
    template <SelectedSide sideTrg> void synchronizeLinkInt(SymlinkPair& linkObj, SyncOperation syncOp);

    void synchronizeFolder(DirPair& dirObj);
    template <SelectedSide sideTrg> void synchronizeFolderInt(DirPair& dirObj, SyncOperation syncOp);

    void reportStatus(const std::wstring& rawText, const Zstring& objname) const { procCallback_.reportStatus(replaceCpy(rawText, L"%x", fmtFileName(objname))); };
    void reportInfo  (const std::wstring& rawText, const Zstring& objname) const { procCallback_.reportInfo  (replaceCpy(rawText, L"%x", fmtFileName(objname))); };
    void reportInfo  (const std::wstring& rawText,
                      const Zstring& objname1,
                      const Zstring& objname2) const
    {
        procCallback_.reportInfo(replaceCpy(replaceCpy(rawText, L"%x", L"\n" + fmtFileName(objname1)), L"%y", L"\n" + fmtFileName(objname2)));
    };

    template <class Function>
    FileAttrib copyFileUpdating(const Zstring& sourceFile, const Zstring& targetFile, const Int64& bytesExpected, Function delTargetCommand) const; //throw FileError; reports data delta via updateProcessedData()
    void verifyFileCopy(const Zstring& source, const Zstring& target) const;

    template <SelectedSide side>
    DeletionHandling& getDelHandling();

    ProcessCallback& procCallback_;
#ifdef ZEN_WIN
    shadow::ShadowCopy* shadowCopyHandler_; //optional!
#endif
    DeletionHandling& delHandlingLeft_;
    DeletionHandling& delHandlingRight_;

    const bool verifyCopiedFiles_;
    const bool copyFilePermissions_;
    const bool transactionalFileCopy_;

    //preload status texts
    const std::wstring txtCreatingFile;
    const std::wstring txtCreatingLink;
    const std::wstring txtCreatingFolder;
    const std::wstring txtOverwritingFile;
    const std::wstring txtOverwritingLink;
    const std::wstring txtVerifying;
    const std::wstring txtWritingAttributes;
    const std::wstring txtMovingFile;
};

//---------------------------------------------------------------------------------------------------------------

template <> inline
DeletionHandling& SynchronizeFolderPair::getDelHandling<LEFT_SIDE>() { return delHandlingLeft_; }

template <> inline
DeletionHandling& SynchronizeFolderPair::getDelHandling<RIGHT_SIDE>() { return delHandlingRight_; }
}

/*
__________________________
|Move algorithm, 0th pass|
--------------------------
1. loop over hierarchy and find "move source"

2. check whether parent directory of "move source" is going to be deleted or location of "move source" may lead to name clash with other dir/symlink
   -> no:  delay move until 2nd pass

3. create move target's parent directory recursively + execute move
   do we have name clash?
   -> prepare a 2-step move operation: 1. move source to root and update "move target" accordingly 2. delay move until 2nd pass

4. If any of the operations above did not succeed (even after retry), update statistics and revert to "copy + delete"
   Note: first pass may delete "move source"!!!

__________________
|killer-scenarios|
------------------
propagate the following move sequences:
I) a -> a/a      caveat sync'ing parent directory first leads to circular dependency!

II) a/a -> a     caveat: fixing name clash will remove source!

III) c -> d      caveat: move-sequence needs to be processed in correct order!
     b -> c/b
     a -> b/a
*/

namespace
{
template <class List> inline
bool haveNameClash(const Zstring& shortname, List& m)
{
    return std::any_of(m.begin(), m.end(),
    [&](const typename List::value_type& obj) { return EqualFilename()(obj.getObjShortName(), shortname); });
}


Zstring findUnusedTempName(const Zstring& filename)
{
    Zstring output = filename + zen::TEMP_FILE_ENDING;

    //ensure uniqueness (+ minor file system race condition!)
    for (int i = 1; somethingExists(output); ++i)
        output = filename + Zchar('_') + numberTo<Zstring>(i) + zen::TEMP_FILE_ENDING;

    return output;
}
}


template <SelectedSide side>
void SynchronizeFolderPair::prepare2StepMove(FilePair& sourceObj,
                                             FilePair& targetObj) //throw FileError
{
    const Zstring& source = sourceObj.getFullName<side>();
    const Zstring& tmpTarget = findUnusedTempName(sourceObj.getBaseDirPf<side>() + sourceObj.getShortName<side>());
    //this could still lead to a name-clash in obscure cases, if some file exists on the other side with
    //the very same (.ffs_tmp) name and is copied before the second step of the move is executed
    //good news: even in this pathologic case, this may only prevent the copy of the other file, but not the move

    reportInfo(txtMovingFile, source, tmpTarget);

    warn_static("was wenn diff volume: symlink aliasing!") //throw FileError, ErrorDifferentVolume, ErrorTargetExisting

    renameFile(source, tmpTarget); //throw FileError

    //update file hierarchy
    const FileDescriptor descrSource(sourceObj.getLastWriteTime <side>(),
                                     sourceObj.getFileSize      <side>(),
                                     sourceObj.getFileId        <side>(),
                                     sourceObj.isFollowedSymlink<side>());

    FilePair& tempFile = sourceObj.root().addSubFile<side>(afterLast(tmpTarget, FILE_NAME_SEPARATOR), descrSource);
    static_assert(IsSameType<FixedList<FilePair>, HierarchyObject::SubFileVec>::value,
                  "ATTENTION: we're adding to the file list WHILE looping over it! This is only working because FixedList iterators are not invalidated by insertion!");
    sourceObj.removeObject<side>(); //remove only *after* evaluating "sourceObj, side"!

    //prepare move in second pass
    tempFile.setSyncDir(side == LEFT_SIDE ? SyncDirection::LEFT : SyncDirection::RIGHT);

    targetObj.setMoveRef(tempFile .getId());
    tempFile .setMoveRef(targetObj.getId());

    //NO statistics update!
    procCallback_.requestUiRefresh(); //may throw
}


bool SynchronizeFolderPair::createParentDir(FileSystemObject& fsObj) //throw FileError, "false" on name clash
{
    if (DirPair* parentDir = dynamic_cast<DirPair*>(&fsObj.parent()))
    {
        if (!createParentDir(*parentDir))
            return false;

        //detect (and try to resolve) file type conflicts: 1. symlinks 2. files
        const Zstring& shortname = parentDir->getObjShortName();
        if (haveNameClash(shortname, parentDir->parent().refSubLinks()) ||
            haveNameClash(shortname, parentDir->parent().refSubFiles()))
            return false;

        //in this context "parentDir" cannot be scheduled for deletion since it contains a "move target"!
        //note: if parentDir were deleted, we'd end up destroying "fsObj"!
        assert(parentDir->getSyncOperation() != SO_DELETE_LEFT &&
               parentDir->getSyncOperation() != SO_DELETE_RIGHT);

        synchronizeFolder(*parentDir); //throw FileError
    }
    return true;
}


template <SelectedSide side>
void SynchronizeFolderPair::manageFileMove(FilePair& sourceObj,
                                           FilePair& targetObj) //throw FileError
{
    assert((sourceObj.getSyncOperation() == SO_MOVE_LEFT_SOURCE  && targetObj.getSyncOperation() == SO_MOVE_LEFT_TARGET  && side == LEFT_SIDE) ||
           (sourceObj.getSyncOperation() == SO_MOVE_RIGHT_SOURCE && targetObj.getSyncOperation() == SO_MOVE_RIGHT_TARGET && side == RIGHT_SIDE));

    const bool sourceWillBeDeleted = [&]() -> bool
    {
        if (DirPair* parentDir = dynamic_cast<DirPair*>(&sourceObj.parent()))
        {
            switch (parentDir->getSyncOperation()) //evaluate comparison result and sync direction
            {
                case SO_DELETE_LEFT:
                case SO_DELETE_RIGHT:
                    return true; //we need to do something about it
                case SO_MOVE_LEFT_SOURCE:
                case SO_MOVE_RIGHT_SOURCE:
                case SO_MOVE_LEFT_TARGET:
                case SO_MOVE_RIGHT_TARGET:
                case SO_OVERWRITE_LEFT:
                case SO_OVERWRITE_RIGHT:
                case SO_CREATE_NEW_LEFT:
                case SO_CREATE_NEW_RIGHT:
                case SO_DO_NOTHING:
                case SO_EQUAL:
                case SO_UNRESOLVED_CONFLICT:
                case SO_COPY_METADATA_TO_LEFT:
                case SO_COPY_METADATA_TO_RIGHT:
                    break;
            }
        }
        return false;
    }();

    auto haveNameClash = [](const FilePair& fileObj)
    {
        return ::haveNameClash(fileObj.getObjShortName(), fileObj.parent().refSubLinks()) ||
               ::haveNameClash(fileObj.getObjShortName(), fileObj.parent().refSubDirs());
    };

    if (sourceWillBeDeleted || haveNameClash(sourceObj))
    {
        //prepare for move now: - revert to 2-step move on name clashes
        if (haveNameClash(targetObj) ||
            !createParentDir(targetObj)) //throw FileError
            return prepare2StepMove<side>(sourceObj, targetObj); //throw FileError

        //finally start move! this should work now:
        synchronizeFile(targetObj); //throw FileError
        //SynchronizeFolderPair::synchronizeFileInt() is *not* expecting SO_MOVE_LEFT_SOURCE/SO_MOVE_RIGHT_SOURCE => start move from targetObj, not sourceObj!
    }
    //else: sourceObj will not be deleted, and is not standing in the way => delay to second pass
    //note: this case may include new "move sources" from two-step sub-routine!!!
}


//search for file move-operations
void SynchronizeFolderPair::runZeroPass(HierarchyObject& hierObj)
{
    for (auto it = hierObj.refSubFiles().begin(); it != hierObj.refSubFiles().end(); ++it) //VS 2010 crashes if we use for_each + lambda here...
    {
        FilePair& fileObj = *it;

        const SyncOperation syncOp = fileObj.getSyncOperation();
        switch (syncOp) //evaluate comparison result and sync direction
        {
            case SO_MOVE_LEFT_SOURCE:
            case SO_MOVE_RIGHT_SOURCE:
            {
                FilePair* sourceObj = &fileObj;
                if (FilePair* targetObj = dynamic_cast<FilePair*>(FileSystemObject::retrieve(fileObj.getMoveRef())))
                {
                    zen::Opt<std::wstring> errMsg = tryReportingError([&]
                    {
                        if (syncOp == SO_MOVE_LEFT_SOURCE)
                            this->manageFileMove<LEFT_SIDE>(*sourceObj, *targetObj); //throw FileError
                        else
                            this->manageFileMove<RIGHT_SIDE>(*sourceObj, *targetObj); //
                    }, procCallback_);

                    if (errMsg)
                    {
                        //move operation has failed! We cannot allow to continue and have move source's parent directory deleted, messing up statistics!
                        // => revert to ordinary "copy + delete"

                        auto getStat = [&]() -> std::pair<int, Int64>
                        {
                            SyncStatistics statSrc(*sourceObj);
                            SyncStatistics statTrg(*targetObj);

                            return std::make_pair(getCUD(statSrc) + getCUD(statTrg),
                            statSrc.getDataToProcess() + statTrg.getDataToProcess());
                        };

                        const auto statBefore = getStat();
                        sourceObj->setMoveRef(nullptr);
                        targetObj->setMoveRef(nullptr);
                        const auto statAfter = getStat();
                        //fix statistics to total to match "copy + delete"
                        procCallback_.updateTotalData(statAfter.first - statBefore.first, statAfter.second - statBefore.second);
                    }
                }
            }
            break;
            case SO_MOVE_LEFT_TARGET:  //it's enough to try each move-pair *once*
            case SO_MOVE_RIGHT_TARGET: //
            case SO_DELETE_LEFT:
            case SO_DELETE_RIGHT:
            case SO_OVERWRITE_LEFT:
            case SO_OVERWRITE_RIGHT:
            case SO_CREATE_NEW_LEFT:
            case SO_CREATE_NEW_RIGHT:
            case SO_DO_NOTHING:
            case SO_EQUAL:
            case SO_UNRESOLVED_CONFLICT:
            case SO_COPY_METADATA_TO_LEFT:
            case SO_COPY_METADATA_TO_RIGHT:
                break;
        }
    }

    std::for_each(hierObj.refSubDirs().begin(), hierObj.refSubDirs().end(),
    [&](DirPair& dirObj) { this->runZeroPass(dirObj); /*recurse */ });
}

//---------------------------------------------------------------------------------------------------------------

//1st, 2nd pass requirements:
// - avoid disk space shortage: 1. delete files, 2. overwrite big with small files first
// - support change in type: overwrite file by directory, symlink by file, ect.

inline
SynchronizeFolderPair::PassId SynchronizeFolderPair::getPass(const FilePair& fileObj)
{
    switch (fileObj.getSyncOperation()) //evaluate comparison result and sync direction
    {
        case SO_DELETE_LEFT:
        case SO_DELETE_RIGHT:
            return PASS_ONE;

        case SO_OVERWRITE_LEFT:
            return fileObj.getFileSize<LEFT_SIDE>() > fileObj.getFileSize<RIGHT_SIDE>() ? PASS_ONE : PASS_TWO;

        case SO_OVERWRITE_RIGHT:
            return fileObj.getFileSize<LEFT_SIDE>() < fileObj.getFileSize<RIGHT_SIDE>() ? PASS_ONE : PASS_TWO;

        case SO_MOVE_LEFT_SOURCE:  //
        case SO_MOVE_RIGHT_SOURCE: // [!]
            return PASS_NEVER;
        case SO_MOVE_LEFT_TARGET:  //
        case SO_MOVE_RIGHT_TARGET: //make sure 2-step move is processed in second pass, after move *target* parent directory was created!
            return PASS_TWO;

        case SO_CREATE_NEW_LEFT:
        case SO_CREATE_NEW_RIGHT:
        case SO_COPY_METADATA_TO_LEFT:
        case SO_COPY_METADATA_TO_RIGHT:
            return PASS_TWO;

        case SO_DO_NOTHING:
        case SO_EQUAL:
        case SO_UNRESOLVED_CONFLICT:
            return PASS_NEVER;
    }
    assert(false);
    return PASS_TWO; //dummy
}


inline
SynchronizeFolderPair::PassId SynchronizeFolderPair::getPass(const SymlinkPair& linkObj)
{
    switch (linkObj.getSyncOperation()) //evaluate comparison result and sync direction
    {
        case SO_DELETE_LEFT:
        case SO_DELETE_RIGHT:
            return PASS_ONE; //make sure to delete symlinks in first pass, and equally named file or dir in second pass: usecase "overwrite symlink with regular file"!

        case SO_OVERWRITE_LEFT:
        case SO_OVERWRITE_RIGHT:
        case SO_CREATE_NEW_LEFT:
        case SO_CREATE_NEW_RIGHT:
        case SO_COPY_METADATA_TO_LEFT:
        case SO_COPY_METADATA_TO_RIGHT:
            return PASS_TWO;

        case SO_MOVE_LEFT_SOURCE:
        case SO_MOVE_RIGHT_SOURCE:
        case SO_MOVE_LEFT_TARGET:
        case SO_MOVE_RIGHT_TARGET:
            assert(false);
        case SO_DO_NOTHING:
        case SO_EQUAL:
        case SO_UNRESOLVED_CONFLICT:
            return PASS_NEVER;
    }
    assert(false);
    return PASS_TWO; //dummy
}


inline
SynchronizeFolderPair::PassId SynchronizeFolderPair::getPass(const DirPair& dirObj)
{
    switch (dirObj.getSyncOperation()) //evaluate comparison result and sync direction
    {
        case SO_DELETE_LEFT:
        case SO_DELETE_RIGHT:
            return PASS_ONE;

        case SO_CREATE_NEW_LEFT:
        case SO_CREATE_NEW_RIGHT:
        case SO_COPY_METADATA_TO_LEFT:
        case SO_COPY_METADATA_TO_RIGHT:
            return PASS_TWO;

        case SO_OVERWRITE_LEFT:
        case SO_OVERWRITE_RIGHT:
        case SO_MOVE_LEFT_SOURCE:
        case SO_MOVE_RIGHT_SOURCE:
        case SO_MOVE_LEFT_TARGET:
        case SO_MOVE_RIGHT_TARGET:
            assert(false);
        case SO_DO_NOTHING:
        case SO_EQUAL:
        case SO_UNRESOLVED_CONFLICT:
            return PASS_NEVER;
    }
    assert(false);
    return PASS_TWO; //dummy
}


template <SynchronizeFolderPair::PassId pass>
void SynchronizeFolderPair::runPass(HierarchyObject& hierObj)
{
    //synchronize files:
    for (FilePair& fileObj : hierObj.refSubFiles())
        if (pass == this->getPass(fileObj)) //"this->" required by two-pass lookup as enforced by GCC 4.7
            tryReportingError([&] { synchronizeFile(fileObj); }, procCallback_);

    //synchronize symbolic links:
    for (SymlinkPair& linkObj : hierObj.refSubLinks())
        if (pass == this->getPass(linkObj))
            tryReportingError([&] { synchronizeLink(linkObj); }, procCallback_);

    //synchronize folders:
    for (DirPair& dirObj : hierObj.refSubDirs())
    {
        if (pass == this->getPass(dirObj))
            tryReportingError([&] { synchronizeFolder(dirObj); }, procCallback_);

        this->runPass<pass>(dirObj); //recurse
    }
}

//---------------------------------------------------------------------------------------------------------------

namespace
{
inline
Opt<SelectedSide> getTargetDirection(SyncOperation syncOp)
{
    switch (syncOp)
    {
        case SO_CREATE_NEW_LEFT:
        case SO_DELETE_LEFT:
        case SO_OVERWRITE_LEFT:
        case SO_COPY_METADATA_TO_LEFT:
        case SO_MOVE_LEFT_SOURCE:
        case SO_MOVE_LEFT_TARGET:
            return LEFT_SIDE;

        case SO_CREATE_NEW_RIGHT:
        case SO_DELETE_RIGHT:
        case SO_OVERWRITE_RIGHT:
        case SO_COPY_METADATA_TO_RIGHT:
        case SO_MOVE_RIGHT_SOURCE:
        case SO_MOVE_RIGHT_TARGET:
            return RIGHT_SIDE;

        case SO_DO_NOTHING:
        case SO_EQUAL:
        case SO_UNRESOLVED_CONFLICT:
            break; //nothing to do
    }
    return NoValue();
}
}


inline
void SynchronizeFolderPair::synchronizeFile(FilePair& fileObj)
{
    const SyncOperation syncOp = fileObj.getSyncOperation();

    if (Opt<SelectedSide> sideTrg = getTargetDirection(syncOp))
    {
        if (*sideTrg == LEFT_SIDE)
            synchronizeFileInt<LEFT_SIDE>(fileObj, syncOp);
        else
            synchronizeFileInt<RIGHT_SIDE>(fileObj, syncOp);
    }
}


template <SelectedSide sideTrg>
void SynchronizeFolderPair::synchronizeFileInt(FilePair& fileObj, SyncOperation syncOp)
{
    static const SelectedSide sideSrc = OtherSide<sideTrg>::result;

    switch (syncOp)
    {
        case SO_CREATE_NEW_LEFT:
        case SO_CREATE_NEW_RIGHT:
        {
            if (const DirPair* parentDir = dynamic_cast<DirPair*>(&fileObj.parent()))
                if (parentDir->isEmpty<sideTrg>()) //BaseDirPair OTOH is always non-empty and existing in this context => else: fatal error in zen::synchronize()
                    return; //if parent directory creation failed, there's no reason to show more errors!

            const Zstring& target = fileObj.getBaseDirPf<sideTrg>() + fileObj.getRelativeName<sideSrc>(); //can't use "getFullName" as target is not yet existing
            reportInfo(txtCreatingFile, target);

            try
            {
                const FileAttrib newAttr = copyFileUpdating(fileObj.getFullName<sideSrc>(),
                                                            target,
                                                            to<Int64>(fileObj.getFileSize<sideSrc>()), [] {} /*no target to delete*/); //throw FileError
                //update FilePair
                fileObj.setSyncedTo<sideTrg>(fileObj.getShortName<sideSrc>(), newAttr.fileSize,
                                             newAttr.modificationTime, //target time set from source
                                             newAttr.modificationTime,
                                             newAttr.targetFileId,
                                             newAttr.sourceFileId,
                                             false, fileObj.isFollowedSymlink<sideSrc>());

                procCallback_.updateProcessedData(1, 0); //processed bytes are reported in copyFileUpdating()!
            }
            catch (FileError&)
            {
                if (somethingExists(fileObj.getFullName<sideSrc>())) //do not check on type (symlink, file, folder) -> if there is a type change, FFS should error out!
                    throw;
                //source deleted meanwhile...nothing was done (logical point of view!)
                procCallback_.updateTotalData(-1, -to<zen::Int64>(fileObj.getFileSize<sideSrc>()));
                fileObj.removeObject<sideSrc>(); //remove only *after* evaluating "fileObj, sideSrc"!
            }
        }
        break;

        case SO_DELETE_LEFT:
        case SO_DELETE_RIGHT:
            reportInfo(getDelHandling<sideTrg>().getTxtRemovingFile(), fileObj.getFullName<sideTrg>());
            {
                int objectsReported = 0;
                auto guardStatistics = makeGuard([&] { procCallback_.updateTotalData(objectsReported, 0); }); //error = unexpected increase of total workload
                const int objectsExpected = 1;
                const Int64 bytesExpected = 0;

                getDelHandling<sideTrg>().removeFileUpdating(fileObj.getFullName<sideTrg>(), fileObj.getObjRelativeName(), bytesExpected, [&] //throw FileError
                {
                    procCallback_.updateProcessedData(1, 0); //noexcept
                    ++objectsReported;
                });

                guardStatistics.dismiss(); //update statistics to consider the real amount of data
                if (objectsReported != objectsExpected)
                    procCallback_.updateTotalData(objectsReported - objectsExpected, 0); //noexcept!
            }
            fileObj.removeObject<sideTrg>(); //update FilePair
            break;

        case SO_MOVE_LEFT_TARGET:
        case SO_MOVE_RIGHT_TARGET:
            if (FilePair* moveSource = dynamic_cast<FilePair*>(FileSystemObject::retrieve(fileObj.getMoveRef())))
            {
                FilePair* moveTarget = &fileObj;

                assert((moveSource->getSyncOperation() == SO_MOVE_LEFT_SOURCE  && moveTarget->getSyncOperation() == SO_MOVE_LEFT_TARGET  && sideTrg == LEFT_SIDE) ||
                       (moveSource->getSyncOperation() == SO_MOVE_RIGHT_SOURCE && moveTarget->getSyncOperation() == SO_MOVE_RIGHT_TARGET && sideTrg == RIGHT_SIDE));

                const Zstring& oldName = moveSource->getFullName<sideTrg>();
                const Zstring& newName = moveSource->getBaseDirPf<sideTrg>() + moveTarget->getRelativeName<sideSrc>();

                reportInfo(txtMovingFile, oldName, newName);
                warn_static("was wenn diff volume: symlink aliasing!") //throw FileError, ErrorDifferentVolume, ErrorTargetExisting
                renameFile(oldName, newName); //throw FileError

                //update FilePair
                assert(moveSource->getFileSize<sideTrg>() == moveTarget->getFileSize<sideSrc>());
                moveTarget->setSyncedTo<sideTrg>(moveTarget->getShortName<sideSrc>(), moveTarget->getFileSize<sideSrc>(),
                                                 moveSource->getLastWriteTime<sideTrg>(), //awkward naming! moveSource is renamed on "sideTrg" side!
                                                 moveTarget->getLastWriteTime<sideSrc>(),
                                                 moveSource->getFileId<sideTrg>(),
                                                 moveTarget->getFileId<sideSrc>(),
                                                 moveSource->isFollowedSymlink<sideTrg>(),
                                                 moveTarget->isFollowedSymlink<sideSrc>());
                moveSource->removeObject<sideTrg>(); //remove only *after* evaluating "moveSource, sideTrg"!

                procCallback_.updateProcessedData(1, 0);
            }
            else (assert(false));
            break;

        case SO_OVERWRITE_LEFT:
        case SO_OVERWRITE_RIGHT:
        {
            const Zstring targetFile = fileObj.isFollowedSymlink<sideTrg>() ? //follow link when updating file rather than delete it and replace with regular file!!!
                                       zen::getResolvedFilePath(fileObj.getFullName<sideTrg>()) : //throw FileError
                                       fileObj.getBaseDirPf<sideTrg>() + fileObj.getRelativeName<sideSrc>(); //respect differences in case of source object

            reportInfo(txtOverwritingFile, targetFile);

            if (fileObj.isFollowedSymlink<sideTrg>()) //since we follow the link, we need to handle case sensitivity of the link manually!
                if (fileObj.getShortName<sideTrg>() != fileObj.getShortName<sideSrc>()) //adapt difference in case (windows only)
                    renameFile(fileObj.getFullName<sideTrg>(),
                               beforeLast(fileObj.getFullName<sideTrg>(), FILE_NAME_SEPARATOR) + FILE_NAME_SEPARATOR + fileObj.getShortName<sideSrc>()); //throw FileError

            const FileAttrib newAttr = copyFileUpdating(fileObj.getFullName<sideSrc>(),
                                                        targetFile,
                                                        to<Int64>(fileObj.getFileSize<sideSrc>()),
                                                        [&] //delete target at appropriate time
            {
                reportStatus(this->getDelHandling<sideTrg>().getTxtRemovingFile(), targetFile);

                this->getDelHandling<sideTrg>().removeFileUpdating(targetFile, fileObj.getObjRelativeName(), 0, [] {}); //throw FileError;
                //no (logical) item count update desired - but total byte count may change, e.g. move(copy) deleted file to versioning dir

                //fileObj.removeObject<sideTrg>(); -> doesn't make sense for isFollowedSymlink(); "fileObj, sideTrg" evaluated below!

                //if fail-safe file copy is active, then the next operation will be a simple "rename"
                //=> don't risk reportStatus() throwing GuiAbortProcess() leaving the target deleted rather than updated!
                if (!transactionalFileCopy_)
                    reportStatus(txtOverwritingFile, targetFile); //restore status text copy file
            }); //throw FileError

            //update FilePair
            fileObj.setSyncedTo<sideTrg>(fileObj.getShortName<sideSrc>(), newAttr.fileSize,
                                         newAttr.modificationTime, //target time set from source
                                         newAttr.modificationTime,
                                         newAttr.targetFileId,
                                         newAttr.sourceFileId,
                                         fileObj.isFollowedSymlink<sideTrg>(),
                                         fileObj.isFollowedSymlink<sideSrc>());

            procCallback_.updateProcessedData(1, 0); //we model "delete + copy" as ONE logical operation
        }
        break;

        case SO_COPY_METADATA_TO_LEFT:
        case SO_COPY_METADATA_TO_RIGHT:
        {
            //harmonize with file_hierarchy.cpp::getSyncOpDescription!!

            reportInfo(txtWritingAttributes, fileObj.getFullName<sideTrg>());

            if (fileObj.getShortName<sideTrg>() != fileObj.getShortName<sideSrc>()) //adapt difference in case (windows only)
                renameFile(fileObj.getFullName<sideTrg>(),
                           beforeLast(fileObj.getFullName<sideTrg>(), FILE_NAME_SEPARATOR) + FILE_NAME_SEPARATOR + fileObj.getShortName<sideSrc>()); //throw FileError

            if (!sameFileTime(fileObj.getLastWriteTime<sideTrg>(), fileObj.getLastWriteTime<sideSrc>(), 2)) //respect 2 second FAT/FAT32 precision
                setFileTime(fileObj.getFullName<sideTrg>(), fileObj.getLastWriteTime<sideSrc>(), SYMLINK_FOLLOW); //throw FileError
            //do NOT read *current* source file time, but use buffered value which corresponds to time of comparison!

            //-> both sides *should* be completely equal now...
            assert(fileObj.getFileSize<sideTrg>() == fileObj.getFileSize<sideSrc>());
            fileObj.setSyncedTo<sideTrg>(fileObj.getShortName<sideSrc>(), fileObj.getFileSize<sideSrc>(),
                                         fileObj.getLastWriteTime<sideSrc>(), //target time set from source
                                         fileObj.getLastWriteTime<sideSrc>(),
                                         fileObj.getFileId       <sideTrg>(),
                                         fileObj.getFileId       <sideSrc>(),
                                         fileObj.isFollowedSymlink<sideTrg>(),
                                         fileObj.isFollowedSymlink<sideSrc>());

            procCallback_.updateProcessedData(1, 0);
        }
        break;

        case SO_MOVE_LEFT_SOURCE:
        case SO_MOVE_RIGHT_SOURCE:
        case SO_DO_NOTHING:
        case SO_EQUAL:
        case SO_UNRESOLVED_CONFLICT:
            assert(false); //should have been filtered out by SynchronizeFolderPair::getPass()
            return; //no update on processed data!
    }

    procCallback_.requestUiRefresh(); //may throw
}


inline
void SynchronizeFolderPair::synchronizeLink(SymlinkPair& linkObj)
{
    const SyncOperation syncOp = linkObj.getSyncOperation();

    if (Opt<SelectedSide> sideTrg = getTargetDirection(syncOp))
    {
        if (*sideTrg == LEFT_SIDE)
            synchronizeLinkInt<LEFT_SIDE>(linkObj, syncOp);
        else
            synchronizeLinkInt<RIGHT_SIDE>(linkObj, syncOp);
    }
}


template <SelectedSide sideTrg>
void SynchronizeFolderPair::synchronizeLinkInt(SymlinkPair& linkObj, SyncOperation syncOp)
{
    static const SelectedSide sideSrc = OtherSide<sideTrg>::result;

    switch (syncOp)
    {
        case SO_CREATE_NEW_LEFT:
        case SO_CREATE_NEW_RIGHT:
        {
            if (const DirPair* parentDir = dynamic_cast<DirPair*>(&linkObj.parent()))
                if (parentDir->isEmpty<sideTrg>()) //BaseDirPair OTOH is always non-empty and existing in this context => else: fatal error in zen::synchronize()
                    return; //if parent directory creation failed, there's no reason to show more errors!

            const Zstring& target = linkObj.getBaseDirPf<sideTrg>() + linkObj.getRelativeName<sideSrc>();

            reportInfo(txtCreatingLink, target);

            try
            {
                zen::copySymlink(linkObj.getFullName<sideSrc>(), target, copyFilePermissions_); //throw FileError
                //update SymlinkPair
                linkObj.setSyncedTo<sideTrg>(linkObj.getShortName<sideSrc>(),
                                             linkObj.getLastWriteTime<sideSrc>(), //target time set from source
                                             linkObj.getLastWriteTime<sideSrc>());

                procCallback_.updateProcessedData(1, 0);
            }
            catch (FileError&)
            {
                if (somethingExists(linkObj.getFullName<sideSrc>())) //do not check on type (symlink, file, folder) -> if there is a type change, FFS should not be quiet about it!
                    throw;
                //source deleted meanwhile...nothing was done (logical point of view!)
                procCallback_.updateTotalData(-1, 0);
                linkObj.removeObject<sideSrc>();
            }
        }
        break;

        case SO_DELETE_LEFT:
        case SO_DELETE_RIGHT:
            reportInfo(getDelHandling<sideTrg>().getTxtRemovingSymLink(), linkObj.getFullName<sideTrg>());
            {
                int objectsReported = 0;
                auto guardStatistics = makeGuard([&] { procCallback_.updateTotalData(objectsReported, 0); }); //error = unexpected increase of total workload
                const int objectsExpected = 1;
                const Int64 bytesExpected = 0;

                getDelHandling<sideTrg>().removeLinkUpdating(linkObj.getFullName<sideTrg>(), linkObj.getObjRelativeName(), bytesExpected, [&] //throw FileError
                {
                    procCallback_.updateProcessedData(1, 0); //noexcept
                    ++objectsReported;
                });

                guardStatistics.dismiss(); //update statistics to consider the real amount of data
                if (objectsReported != objectsExpected)
                    procCallback_.updateTotalData(objectsReported - objectsExpected, 0); //noexcept!
            }
            linkObj.removeObject<sideTrg>(); //update SymlinkPair
            break;

        case SO_OVERWRITE_LEFT:
        case SO_OVERWRITE_RIGHT:
            reportInfo(txtOverwritingLink, linkObj.getFullName<sideTrg>());

            //reportStatus(getDelHandling<sideTrg>().getTxtRemovingSymLink(), linkObj.getFullName<sideTrg>());
            getDelHandling<sideTrg>().removeLinkUpdating(linkObj.getFullName<sideTrg>(), linkObj.getObjRelativeName(), 0, [] {}); //throw FileError

            //linkObj.removeObject<sideTrg>(); -> "linkObj, sideTrg" evaluated below!

            //=> don't risk reportStatus() throwing GuiAbortProcess() leaving the target deleted rather than updated:

            //reportStatus(txtOverwritingLink, linkObj.getFullName<sideTrg>()); //restore status text
            zen::copySymlink(linkObj.getFullName<sideSrc>(),
                             linkObj.getBaseDirPf<sideTrg>() + linkObj.getRelativeName<sideSrc>(), //respect differences in case of source object
                             copyFilePermissions_); //throw FileError

            //update SymlinkPair
            linkObj.setSyncedTo<sideTrg>(linkObj.getShortName<sideSrc>(),
                                         linkObj.getLastWriteTime<sideSrc>(), //target time set from source
                                         linkObj.getLastWriteTime<sideSrc>());

            procCallback_.updateProcessedData(1, 0); //we model "delete + copy" as ONE logical operation
            break;

        case SO_COPY_METADATA_TO_LEFT:
        case SO_COPY_METADATA_TO_RIGHT:
            reportInfo(txtWritingAttributes, linkObj.getFullName<sideTrg>());

            if (linkObj.getShortName<sideTrg>() != linkObj.getShortName<sideSrc>()) //adapt difference in case (windows only)
                renameFile(linkObj.getFullName<sideTrg>(),
                           beforeLast(linkObj.getFullName<sideTrg>(), FILE_NAME_SEPARATOR) + FILE_NAME_SEPARATOR + linkObj.getShortName<sideSrc>()); //throw FileError

            if (!sameFileTime(linkObj.getLastWriteTime<sideTrg>(), linkObj.getLastWriteTime<sideSrc>(), 2)) //respect 2 second FAT/FAT32 precision
                setFileTime(linkObj.getFullName<sideTrg>(), linkObj.getLastWriteTime<sideSrc>(), SYMLINK_DIRECT); //throw FileError

            //-> both sides *should* be completely equal now...
            linkObj.setSyncedTo<sideTrg>(linkObj.getShortName<sideSrc>(),
                                         linkObj.getLastWriteTime<sideSrc>(), //target time set from source
                                         linkObj.getLastWriteTime<sideSrc>());

            procCallback_.updateProcessedData(1, 0);
            break;

        case SO_MOVE_LEFT_SOURCE:
        case SO_MOVE_RIGHT_SOURCE:
        case SO_MOVE_LEFT_TARGET:
        case SO_MOVE_RIGHT_TARGET:
        case SO_DO_NOTHING:
        case SO_EQUAL:
        case SO_UNRESOLVED_CONFLICT:
            assert(false); //should have been filtered out by SynchronizeFolderPair::getPass()
            return; //no update on processed data!
    }

    procCallback_.requestUiRefresh(); //may throw
}


inline
void SynchronizeFolderPair::synchronizeFolder(DirPair& dirObj)
{
    const SyncOperation syncOp = dirObj.getSyncOperation();

    if (Opt<SelectedSide> sideTrg = getTargetDirection(syncOp))
    {
        if (*sideTrg == LEFT_SIDE)
            synchronizeFolderInt<LEFT_SIDE>(dirObj, syncOp);
        else
            synchronizeFolderInt<RIGHT_SIDE>(dirObj, syncOp);
    }
}


template <SelectedSide sideTrg>
void SynchronizeFolderPair::synchronizeFolderInt(DirPair& dirObj, SyncOperation syncOp)
{
    static const SelectedSide sideSrc = OtherSide<sideTrg>::result;

    switch (syncOp)
    {
        case SO_CREATE_NEW_LEFT:
        case SO_CREATE_NEW_RIGHT:
            if (const DirPair* parentDir = dynamic_cast<DirPair*>(&dirObj.parent()))
                if (parentDir->isEmpty<sideTrg>()) //BaseDirPair OTOH is always non-empty and existing in this context => else: fatal error in zen::synchronize()
                    return; //if parent directory creation failed, there's no reason to show more errors!

            if (somethingExists(dirObj.getFullName<sideSrc>())) //do not check on type (symlink, file, folder) -> if there is a type change, FFS should error out!
            {
                const Zstring& target = dirObj.getBaseDirPf<sideTrg>() + dirObj.getRelativeName<sideSrc>();

                reportInfo(txtCreatingFolder, target);
                try
                {
                    makeDirectoryPlain(target, dirObj.getFullName<sideSrc>(), copyFilePermissions_); //throw FileError, ErrorTargetExisting, (ErrorTargetPathMissing)
                }
                catch (const ErrorTargetExisting&) { if (!dirExists(target)) throw; } //detect clash with file (dir-symlink OTOH is okay)

                //update DirPair
                dirObj.setSyncedTo(dirObj.getShortName<sideSrc>());

                procCallback_.updateProcessedData(1, 0);
            }
            else //source deleted meanwhile...nothing was done (logical point of view!) -> uh....what about a temporary network drop???
            {
                const SyncStatistics subStats(dirObj);
                procCallback_.updateTotalData(-getCUD(subStats) - 1, -subStats.getDataToProcess());

                //remove only *after* evaluating dirObj!!
                dirObj.refSubFiles().clear();   //
                dirObj.refSubLinks().clear();   //update DirPair
                dirObj.refSubDirs ().clear();   //
                dirObj.removeObject<sideSrc>(); //
            }
            break;

        case SO_DELETE_LEFT:
        case SO_DELETE_RIGHT:
            reportInfo(getDelHandling<sideTrg>().getTxtRemovingDir(), dirObj.getFullName<sideTrg>());
            {
                int objectsReported = 0;
                auto guardStatistics = makeGuard([&] { procCallback_.updateTotalData(objectsReported, 0); }); //error = unexpected increase of total workload
                const SyncStatistics subStats(dirObj); //counts sub-objects only!
                const int objectsExpected = 1 + getCUD(subStats);
                const Int64 bytesExpected = subStats.getDataToProcess();
                assert(bytesExpected == 0);

                getDelHandling<sideTrg>().removeDirUpdating(dirObj.getFullName<sideTrg>(), dirObj.getObjRelativeName(), bytesExpected, [&] //throw FileError
                {
                    procCallback_.updateProcessedData(1, 0); //noexcept
                    ++objectsReported;
                });

                guardStatistics.dismiss(); //update statistics to consider the real amount of data
                if (objectsReported != objectsExpected)
                    procCallback_.updateTotalData(objectsReported - objectsExpected, 0); //noexcept!
            }
            dirObj.refSubFiles().clear();   //
            dirObj.refSubLinks().clear();   //update DirPair
            dirObj.refSubDirs ().clear();   //
            dirObj.removeObject<sideTrg>(); //
            break;

        case SO_COPY_METADATA_TO_LEFT:
        case SO_COPY_METADATA_TO_RIGHT:
            reportInfo(txtWritingAttributes, dirObj.getFullName<sideTrg>());

            if (dirObj.getShortName<sideTrg>() != dirObj.getShortName<sideSrc>()) //adapt difference in case (windows only)
                renameFile(dirObj.getFullName<sideTrg>(),
                           beforeLast(dirObj.getFullName<sideTrg>(), FILE_NAME_SEPARATOR) + FILE_NAME_SEPARATOR + dirObj.getShortName<sideSrc>()); //throw FileError
            //copyFileTimes -> useless: modification time changes with each child-object creation/deletion

            //-> both sides *should* be completely equal now...
            dirObj.setSyncedTo(dirObj.getShortName<sideSrc>());

            procCallback_.updateProcessedData(1, 0);
            break;

        case SO_OVERWRITE_LEFT:
        case SO_OVERWRITE_RIGHT:
        case SO_MOVE_LEFT_SOURCE:
        case SO_MOVE_RIGHT_SOURCE:
        case SO_MOVE_LEFT_TARGET:
        case SO_MOVE_RIGHT_TARGET:
        case SO_DO_NOTHING:
        case SO_EQUAL:
        case SO_UNRESOLVED_CONFLICT:
            assert(false); //should have been filtered out by SynchronizeFolderPair::getPass()
            return; //no update on processed data!
    }

    procCallback_.requestUiRefresh(); //may throw
}


namespace
{
#ifdef ZEN_WIN
//recycleBinStatus() blocks seriously if recycle bin is really full and drive is slow
StatusRecycler recycleBinStatusUpdating(const Zstring& dirname, ProcessCallback& procCallback)
{
    procCallback.reportStatus(replaceCpy(_("Checking recycle bin availability for folder %x..."), L"%x", fmtFileName(dirname), false));

    auto ft = async([=] { return recycleBinStatus(dirname); });

    while (!ft.timed_wait(boost::posix_time::milliseconds(UI_UPDATE_INTERVAL / 2)))
        procCallback.requestUiRefresh(); //may throw!
    return ft.get();
}
#endif


/*
struct LessDependentDirectory : public std::binary_function<Zstring, Zstring, bool>
{
->  a *very* bad idea: this is NOT a strict weak ordering! No transitivity of equivalence!

    bool operator()(const Zstring& lhs, const Zstring& rhs) const
    {
        return LessFilename()(Zstring(lhs.c_str(), std::min(lhs.length(), rhs.length())),
                              Zstring(rhs.c_str(), std::min(lhs.length(), rhs.length())));
    }
};
*/

template <SelectedSide side> //create base directories first (if not yet existing) -> no symlink or attribute copying!
bool createBaseDirectory(BaseDirPair& baseDirObj, ProcessCallback& callback) //nothrow; return false if fatal error occurred
{
    const Zstring dirname = beforeLast(baseDirObj.getBaseDirPf<side>(), FILE_NAME_SEPARATOR); //what about C:\ ???
    if (dirname.empty())
        return true;

    if (baseDirObj.isExisting<side>()) //atomicity: do NOT check directory existence again!
    {
        //just convenience: exit sync right here instead of showing tons of error messages during file copy
        zen::Opt<std::wstring> errMsg = tryReportingError([&]
        {
            if (!dirExistsUpdating(dirname, false, callback))
                throw FileError(replaceCpy(_("Cannot find %x."), L"%x", fmtFileName(dirname))); //should be logged as a "fatal error" if ignored by the user...
        }, callback); //may throw in error-callback!

        return !errMsg;
    }
    else //create target directory: user presumably ignored error "dir existing" in order to have it created automatically
    {
        bool temporaryNetworkDrop = false;
        zen::Opt<std::wstring> errMsg = tryReportingError([&]
        {
            try
            {
                //a nice race-free check and set operation:
                makeDirectory(dirname, /*bool failIfExists*/ true); //throw FileError, ErrorTargetExisting
                baseDirObj.setExisting<side>(true); //update our model!
            }
            catch (const ErrorTargetExisting&)
            {
                //TEMPORARY network drop: base directory not found during comparison, but reappears during synchronization
                //=> sync-directions are based on false assumptions! Abort.
                callback.reportFatalError(replaceCpy(_("Target folder %x already existing."), L"%x", fmtFileName(dirname)));
                temporaryNetworkDrop = true;

                //Is it possible we're catching a "false-positive" here, could FFS have created the directory indirectly after comparison?
                //	1. deletion handling: recycler       -> no, temp directory created only at first deletion
                //	2. deletion handling: versioning     -> "
                //	3. log file creates containing folder -> no, log only created in batch mode, and only *before* comparison
            }
        }, callback); //may throw in error-callback!
        return !errMsg && !temporaryNetworkDrop;
    }
}
}


void zen::synchronize(const TimeComp& timeStamp,
                      xmlAccess::OptionalDialogs& warnings,
                      bool verifyCopiedFiles,
                      bool copyLockedFiles,
                      bool copyFilePermissions,
                      bool transactionalFileCopy,
                      bool runWithBackgroundPriority,
                      const std::vector<FolderPairSyncCfg>& syncConfig,
                      FolderComparison& folderCmp,
                      ProcessCallback& callback)
{
    //specify process and resource handling priorities
    std::unique_ptr<ScheduleForBackgroundProcessing> backgroundPrio;
    if (runWithBackgroundPriority)
        try
        {
            backgroundPrio = make_unique<ScheduleForBackgroundProcessing>(); //throw FileError
        }
        catch (const FileError& e)
        {
            //not an error in this context
            callback.reportInfo(e.toString()); //may throw!
        }

    //prevent operating system going into sleep state
    std::unique_ptr<PreventStandby> noStandby;
    try
    {
        noStandby = make_unique<PreventStandby>(); //throw FileError
    }
    catch (const FileError& e)
    {
        //not an error in this context
        callback.reportInfo(e.toString()); //may throw!
    }

    //PERF_START;

    if (syncConfig.size() != folderCmp.size())
        throw std::logic_error("Programming Error: Contract violation! " + std::string(__FILE__) + ":" + numberTo<std::string>(__LINE__));

    //inform about the total amount of data that will be processed from now on
    const SyncStatistics statisticsTotal(folderCmp);

    //keep at beginning so that all gui elements are initialized properly
    callback.initNewPhase(getCUD(statisticsTotal),
                          statisticsTotal.getDataToProcess(),
                          ProcessCallback::PHASE_SYNCHRONIZING);


    std::deque<bool> skipFolderPair(folderCmp.size()); //folder pairs may be skipped after fatal errors were found

    //-------------------execute basic checks all at once before starting sync--------------------------------------

    auto dependentDir = [](const Zstring& lhs, const Zstring& rhs) //note: this is NOT an equivalence relation!
    {
        return EqualFilename()(Zstring(lhs.c_str(), std::min(lhs.length(), rhs.length())),
                               Zstring(rhs.c_str(), std::min(lhs.length(), rhs.length())));
    };

    //aggregate information
    std::map<Zstring, std::pair<size_t, size_t>> dirReadWriteCount; //count read/write accesses
    auto incReadCount = [&](const Zstring& baseDir)
    {
        dirReadWriteCount[baseDir]; //create entry
        for (auto it = dirReadWriteCount.begin(); it != dirReadWriteCount.end(); ++it)
        {
            auto& countRef = it->second;
            if (dependentDir(baseDir, it->first))
                ++countRef.first;
        }
    };
    auto incWriteCount = [&](const Zstring& baseDir)
    {
        dirReadWriteCount[baseDir]; //create entry
        for (auto it = dirReadWriteCount.begin(); it != dirReadWriteCount.end(); ++it)
        {
            auto& countRef = it->second;
            if (dependentDir(baseDir, it->first))
                ++countRef.second;
        }
    };

    typedef std::vector<std::pair<Zstring, Zstring>> DirPairList;
    DirPairList significantDiff;

    typedef std::vector<std::pair<Zstring, std::pair<Int64, Int64>>> DirSpaceRequAvailList; //dirname / space required / space available
    DirSpaceRequAvailList diskSpaceMissing;

#ifdef ZEN_WIN
    //status of base directories which are set to DELETE_TO_RECYCLER (and contain actual items to be deleted)
    std::map<Zstring, bool, LessFilename> baseDirHasRecycler; //might be expensive to determine => buffer + check recycle bin existence only once per base directory!
#endif

    //start checking folder pairs
    for (auto j = begin(folderCmp); j != end(folderCmp); ++j)
    {
        const size_t folderIndex = j - begin(folderCmp);

        //exclude some pathological case (leftdir, rightdir are empty)
        if (EqualFilename()(j->getBaseDirPf<LEFT_SIDE>(), j->getBaseDirPf<RIGHT_SIDE>()))
            continue;

        const FolderPairSyncCfg& folderPairCfg = syncConfig[folderIndex];

        const SyncStatistics folderPairStat(*j);

        //aggregate basic information
        const bool writeLeft = folderPairStat.getCreate<LEFT_SIDE>() +
                               folderPairStat.getUpdate<LEFT_SIDE>() +
                               folderPairStat.getDelete<LEFT_SIDE>() > 0;

        const bool writeRight = folderPairStat.getCreate<RIGHT_SIDE>() +
                                folderPairStat.getUpdate<RIGHT_SIDE>() +
                                folderPairStat.getDelete<RIGHT_SIDE>() > 0;

        //skip folder pair if there is nothing to do (except for two-way mode and move-detection, where DB files need to be written)
        if (!writeLeft && !writeRight &&
            !folderPairCfg.saveSyncDB_)
        {
            skipFolderPair[folderIndex] = true; //skip creating (not yet existing) base directories in particular if there's no need
            continue;
        }

        //check empty input fields: this only makes sense if empty field is source (and no DB files need to be created)
        if ((j->getBaseDirPf<LEFT_SIDE >().empty() && (writeLeft  || folderPairCfg.saveSyncDB_)) ||
            (j->getBaseDirPf<RIGHT_SIDE>().empty() && (writeRight || folderPairCfg.saveSyncDB_)))
        {
            callback.reportFatalError(_("Target folder input field must not be empty."));
            skipFolderPair[folderIndex] = true;
            continue;
        }

        //aggregate information of folders used by multiple pairs in read/write access
        if (!dependentDir(j->getBaseDirPf<LEFT_SIDE>(), j->getBaseDirPf<RIGHT_SIDE>())) //true in general
        {
            if (writeLeft && writeRight)
            {
                incWriteCount(j->getBaseDirPf<LEFT_SIDE >());
                incWriteCount(j->getBaseDirPf<RIGHT_SIDE>());
            }
            else if (writeLeft)
            {
                incWriteCount(j->getBaseDirPf<LEFT_SIDE>());
                incReadCount (j->getBaseDirPf<RIGHT_SIDE>());
            }
            else if (writeRight)
            {
                incReadCount (j->getBaseDirPf<LEFT_SIDE>());
                incWriteCount(j->getBaseDirPf<RIGHT_SIDE>());
            }
        }
        else //if folder pair contains two dependent folders, a warning was already issued after comparison; in this context treat as one write access at most
        {
            if (writeLeft || writeRight)
                incWriteCount(j->getBaseDirPf<LEFT_SIDE>());
        }


        if (folderPairStat.getUpdate() + folderPairStat.getDelete() > 0 &&
            folderPairCfg.handleDeletion == zen::DELETE_TO_VERSIONING)
        {
            //check if user-defined directory for deletion was specified
            if (folderPairCfg.versioningFolder.empty()) //already trimmed by getFormattedDirectoryName()
            {
                //should never arrive here: already checked in SyncCfgDialog
                callback.reportFatalError(_("Please enter a target folder for versioning."));
                skipFolderPair[folderIndex] = true;
                continue;
            }
        }

        //the following scenario is covered by base directory creation below in case source directory exists (accessible or not), but latter doesn't cover source created after comparison, but before sync!!!
        auto checkSourceMissing = [&](const Zstring& baseDirPf, bool wasExisting) -> bool //avoid race-condition: we need to evaluate existence status from time of comparison!
        {
            if (!baseDirPf.empty())
            {
                //PERMANENT network drop: avoid data loss when source directory is not found AND user chose to ignore errors (else we wouldn't arrive here)
                if (folderPairStat.getCreate() +
                folderPairStat.getUpdate() == 0 &&
                folderPairStat.getDelete() > 0) //deletions only... (respect filtered items!)
                    //folderPairStat.getConflict() == 0 && -> there COULD be conflicts for <automatic> if directory existence check fails, but loading sync.ffs_db succeeds
                    //https://sourceforge.net/tracker/?func=detail&atid=1093080&aid=3531351&group_id=234430 -> fixed, but still better not consider conflicts
                {
                    if (!wasExisting) //avoid race-condition: we need to evaluate existence status from time of comparison!
                    {
                        callback.reportFatalError(replaceCpy(_("Source folder %x not found."), L"%x", fmtFileName(baseDirPf)));
                        skipFolderPair[folderIndex] = true;
                        return false;
                    }
                }
            }
            return true;
        };
        if (!checkSourceMissing(j->getBaseDirPf<LEFT_SIDE >(), j->isExisting<LEFT_SIDE >()) ||
            !checkSourceMissing(j->getBaseDirPf<RIGHT_SIDE>(), j->isExisting<RIGHT_SIDE>()))
            continue;

        //check if more than 50% of total number of files/dirs are to be created/overwritten/deleted
        if (significantDifferenceDetected(folderPairStat))
            significantDiff.push_back(std::make_pair(j->getBaseDirPf<LEFT_SIDE>(), j->getBaseDirPf<RIGHT_SIDE>()));

        //check for sufficient free diskspace
        auto checkSpace = [&](const Zstring& baseDirPf, const Int64& minSpaceNeeded)
        {
            try
            {
                const Int64 freeSpace = to<Int64>(getFreeDiskSpace(baseDirPf)); //throw FileError

                if (0 < freeSpace && //zero disk space probably means "request not supported" (e.g. see WebDav)
                    freeSpace < minSpaceNeeded)
                    diskSpaceMissing.push_back(std::make_pair(baseDirPf, std::make_pair(minSpaceNeeded, freeSpace)));
            }
            catch (FileError&) {}
        };
        const std::pair<Int64, Int64> spaceNeeded = MinimumDiskSpaceNeeded::calculate(*j);
        checkSpace(j->getBaseDirPf<LEFT_SIDE >(), spaceNeeded.first);
        checkSpace(j->getBaseDirPf<RIGHT_SIDE>(), spaceNeeded.second);

#ifdef ZEN_WIN
        //windows: check if recycle bin really exists; if not, Windows will silently delete, which is wrong
        auto checkRecycler = [&](const Zstring& baseDirPf)
        {
            if (!baseDirPf.empty()) //should be
                if (baseDirHasRecycler.find(baseDirPf) == baseDirHasRecycler.end()) //perf: avoid duplicate checks!
                    baseDirHasRecycler[baseDirPf] = recycleBinStatusUpdating(baseDirPf, callback) == STATUS_REC_EXISTS;
        };

        if (folderPairCfg.handleDeletion == DELETE_TO_RECYCLER)
        {
            if (folderPairStat.getUpdate<LEFT_SIDE>() +
                folderPairStat.getDelete<LEFT_SIDE>() > 0)
                checkRecycler(j->getBaseDirPf<LEFT_SIDE>());

            if (folderPairStat.getUpdate<RIGHT_SIDE>() +
                folderPairStat.getDelete<RIGHT_SIDE>() > 0)
                checkRecycler(j->getBaseDirPf<RIGHT_SIDE>());
        }
#endif
    }

    //check if unresolved conflicts exist
    if (statisticsTotal.getConflict() > 0)
    {
        //show the first few conflicts in warning message also:
        std::wstring msg = _("The following items have unresolved conflicts and will not be synchronized:");

        const auto& conflictMsgs = statisticsTotal.getConflictMessages(); //get *all* sync conflicts
        for (auto it = conflictMsgs.begin(); it != conflictMsgs.end(); ++it)
            msg += L"\n\n" + fmtFileName(it->first) + L": " + it->second;

        callback.reportWarning(msg, warnings.warningUnresolvedConflicts);
    }


    //check if user accidentally selected wrong directories for sync
    if (!significantDiff.empty())
    {
        std::wstring msg = _("The following folders are significantly different. Make sure you are matching the correct folders for synchronization.");

        for (auto it = significantDiff.begin(); it != significantDiff.end(); ++it)
            msg += std::wstring(L"\n\n") +
                   it->first + L" <-> " + L"\n" +
                   it->second;

        callback.reportWarning(msg, warnings.warningSignificantDifference);
    }


    //check for sufficient free diskspace
    if (!diskSpaceMissing.empty())
    {
        std::wstring msg = _("Not enough free disk space available in:");

        for (auto it = diskSpaceMissing.begin(); it != diskSpaceMissing.end(); ++it)
            msg += std::wstring(L"\n\n") +
                   it->first + L"\n" +
                   _("Required:")  + L" " + filesizeToShortString(it->second.first)  + L"\n" +
                   _("Available:") + L" " + filesizeToShortString(it->second.second);

        callback.reportWarning(msg, warnings.warningNotEnoughDiskSpace);
    }

#ifdef ZEN_WIN
    //windows: check if recycle bin really exists; if not, Windows will silently delete, which is wrong
    {
        std::wstring dirListMissingRecycler;
        for (auto it = baseDirHasRecycler.begin(); it != baseDirHasRecycler.end(); ++it)
            if (!it->second)
                dirListMissingRecycler += std::wstring(L"\n") + it->first;

        if (!dirListMissingRecycler.empty())
            callback.reportWarning(_("The recycle bin is not available for the following folders. Files will be deleted permanently instead:") + L"\n" + dirListMissingRecycler, warnings.warningRecyclerMissing);
    }
#endif

    //check if folders are used by multiple pairs in read/write access
    std::vector<Zstring> conflictDirs;
    for (auto it = dirReadWriteCount.cbegin(); it != dirReadWriteCount.cend(); ++it)
    {
        const std::pair<size_t, size_t>& countRef = it->second; //# read/write accesses

        if (countRef.first + countRef.second >= 2 && countRef.second >= 1) //race condition := multiple accesses of which at least one is a write
            conflictDirs.push_back(it->first);
    }

    if (!conflictDirs.empty())
    {
        std::wstring msg = _("A folder will be modified which is part of multiple folder pairs. Please review synchronization settings.") + L"\n";
        std::for_each(conflictDirs.begin(), conflictDirs.end(),
        [&](const Zstring& dirname) { msg += std::wstring(L"\n") + dirname; });

        callback.reportWarning(msg, warnings.warningFolderPairRaceCondition);
    }

    //-------------------end of basic checks------------------------------------------

#ifdef ZEN_WIN
    //shadow copy buffer: per sync-instance, not folder pair
    std::unique_ptr<shadow::ShadowCopy> shadowCopyHandler;
    if (copyLockedFiles)
        shadowCopyHandler = make_unique<shadow::ShadowCopy>();
#endif

    try
    {
        //loop through all directory pairs
        for (auto j = begin(folderCmp); j != end(folderCmp); ++j)
        {
            //exclude pathological cases (e.g. leftdir, rightdir are empty)
            if (EqualFilename()(j->getBaseDirPf<LEFT_SIDE>(), j->getBaseDirPf<RIGHT_SIDE>()))
                continue;

            //------------------------------------------------------------------------------------------
            callback.reportInfo(_("Synchronizing folder pair:") + L"\n" +
                                L"    " + j->getBaseDirPf<LEFT_SIDE >() + L"\n" +
                                L"    " + j->getBaseDirPf<RIGHT_SIDE>());
            //------------------------------------------------------------------------------------------

            const size_t folderIndex = j - begin(folderCmp);
            const FolderPairSyncCfg& folderPairCfg = syncConfig[folderIndex];

            if (skipFolderPair[folderIndex]) //folder pairs may be skipped after fatal errors were found
                continue;

            //create base directories first (if not yet existing) -> no symlink or attribute copying!
            if (!createBaseDirectory<LEFT_SIDE >(*j, callback) ||
                !createBaseDirectory<RIGHT_SIDE>(*j, callback))
                continue; //skip this folder pair

            //------------------------------------------------------------------------------------------
            //execute synchronization recursively

            //update synchronization database (automatic sync only)
            ScopeGuard guardUpdateDb = makeGuard([&]
            {
                if (folderPairCfg.saveSyncDB_)
                    try { zen::saveLastSynchronousState(*j); } //throw FileError
                    catch (FileError&) {}
            });

            //guarantee removal of invalid entries (where element on both sides is empty)
            ZEN_ON_SCOPE_EXIT(BaseDirPair::removeEmpty(*j););

            bool copyPermissionsFp = false;
            tryReportingError([&]
            {
                copyPermissionsFp = copyFilePermissions && //copy permissions only if asked for and supported by *both* sides!
                !j->getBaseDirPf<LEFT_SIDE >().empty() && //scenario: directory selected on one side only
                !j->getBaseDirPf<RIGHT_SIDE>().empty() && //
                supportsPermissions(beforeLast(j->getBaseDirPf<LEFT_SIDE >(), FILE_NAME_SEPARATOR)) && //throw FileError
                supportsPermissions(beforeLast(j->getBaseDirPf<RIGHT_SIDE>(), FILE_NAME_SEPARATOR));
            }, callback); //show error dialog if necessary


            auto getEffectiveDeletionPolicy = [&](const Zstring& baseDirPf) -> DeletionPolicy
            {
#ifdef ZEN_WIN
                if (folderPairCfg.handleDeletion == DELETE_TO_RECYCLER)
                {
                    auto it = baseDirHasRecycler.find(baseDirPf);
                    if (it != baseDirHasRecycler.end())
                        if (!it->second)
                            return DELETE_PERMANENTLY; //Windows' ::SHFileOperation() will do this anyway, but we have a better and faster deletion routine (e.g. on networks)
                }
#endif
                return folderPairCfg.handleDeletion;
            };


            DeletionHandling delHandlerL(getEffectiveDeletionPolicy(j->getBaseDirPf<LEFT_SIDE>()),
                                         folderPairCfg.versioningFolder,
                                         folderPairCfg.versioningStyle_,
                                         timeStamp,
                                         j->getBaseDirPf<LEFT_SIDE>(),
                                         callback);

            DeletionHandling delHandlerR(getEffectiveDeletionPolicy(j->getBaseDirPf<RIGHT_SIDE>()),
                                         folderPairCfg.versioningFolder,
                                         folderPairCfg.versioningStyle_,
                                         timeStamp,
                                         j->getBaseDirPf<RIGHT_SIDE>(),
                                         callback);


            SynchronizeFolderPair syncFP(callback, verifyCopiedFiles, copyPermissionsFp, transactionalFileCopy,
#ifdef ZEN_WIN
                                         shadowCopyHandler.get(),
#endif
                                         delHandlerL, delHandlerR);
            syncFP.startSync(*j);

            //(try to gracefully) cleanup temporary Recycle bin folders and versioning -> will be done in ~DeletionHandling anyway...
            tryReportingError([&] { delHandlerL.tryCleanup(); }, callback); //show error dialog if necessary
            tryReportingError([&] { delHandlerR.tryCleanup(); }, callback); //

            //(try to gracefully) write database file
            if (folderPairCfg.saveSyncDB_)
            {
                callback.reportStatus(_("Generating database..."));
                callback.forceUiRefresh();

                tryReportingError([&] { zen::saveLastSynchronousState(*j); }, callback); //throw FileError
                guardUpdateDb.dismiss();
            }
        }
    }
    catch (const std::exception& e)
    {
        callback.reportFatalError(utfCvrtTo<std::wstring>(e.what()));
    }
}

//###########################################################################################

template <class Function>
class WhileCopying : public zen::CallbackCopyFile
{
public:
    WhileCopying(Int64& bytesReported,
                 ProcessCallback& statusHandler,
                 Function delTargetCmd) :
        bytesReported_(bytesReported),
        statusHandler_(statusHandler),
        delTargetCmd_(std::move(delTargetCmd)) {}

    virtual void deleteTargetFile(const Zstring& targetFile) { delTargetCmd_(); }

    virtual void updateCopyStatus(Int64 bytesDelta)
    {
        statusHandler_.updateProcessedData(0, bytesDelta); //throw()! -> ensure client and service provider are in sync!
        bytesReported_ += bytesDelta;                      //

        statusHandler_.requestUiRefresh(); //may throw
    }

private:
    Int64&           bytesReported_;
    ProcessCallback& statusHandler_;
    Function delTargetCmd_;
};


//throw FileError; reports data delta via updateProcessedData()
template <class Function>
FileAttrib SynchronizeFolderPair::copyFileUpdating(const Zstring& sourceFile,
                                                   const Zstring& targetFile, const Int64& bytesExpected, Function delTargetCommand) const //returns current attributes of source file
{
    Zstring source = sourceFile;
    FileAttrib newAttr;
    Int64 bytesReported;

    auto copyOperation = [&]
    {
        auto guardStatistics = makeGuard([&]
        {
            procCallback_.updateTotalData(0, bytesReported); //error = unexpected increase of total workload
            bytesReported = 0;
        });

        WhileCopying<Function> callback(bytesReported, procCallback_, delTargetCommand);

        copyFile(source, //type File implicitly means symlinks need to be dereferenced!
        targetFile,
        copyFilePermissions_,
        transactionalFileCopy_,
        &callback,
        &newAttr); //throw FileError, ErrorFileLocked

        //#################### Verification #############################
        if (verifyCopiedFiles_)
        {
            auto guardTarget = makeGuard([&] { removeFile(targetFile); }); //delete target if verification fails
            verifyFileCopy(source, targetFile); //throw FileError
            guardTarget.dismiss();
        }
        //#################### /Verification #############################

        //update statistics to consider the real amount of data, e.g. more than the "file size" for ADS streams,
        //less for sparse and compressed files,  or file changed in the meantime!
        if (bytesReported != bytesExpected)
            procCallback_.updateTotalData(0, bytesReported - bytesExpected); //noexcept!

        guardStatistics.dismiss();
    };

#ifdef ZEN_WIN
    try
    {
        copyOperation();
    }
    catch (ErrorFileLocked& e1)
    {
        //if file is locked (try to) use Windows Volume Shadow Copy Service
        if (!shadowCopyHandler_)
            throw;
        try
        {
            //contains prefix: E.g. "\\?\GLOBALROOT\Device\HarddiskVolumeShadowCopy1\Program Files\FFS\sample.dat"
            source = shadowCopyHandler_->makeShadowCopy(source,  //throw FileError
                                                        [&](const Zstring& volumeName)
            {
                procCallback_.reportStatus(replaceCpy(_("Creating a Volume Shadow Copy for %x..."), L"%x", fmtFileName(volumeName)));
                procCallback_.forceUiRefresh();
            });
        }
        catch (const FileError& e2)
        {
            throw FileError(e1.toString(), e2.toString());
        }

        //now try again
        copyOperation();
    }
#else
    copyOperation();
#endif

    return newAttr;
}


//--------------------- data verification -------------------------
struct VerifyCallback
{
    virtual ~VerifyCallback() {}
    virtual void updateStatus() = 0;
};

void verifyFiles(const Zstring& source, const Zstring& target, VerifyCallback& callback) //throw FileError
{
    static std::vector<char> memory1(1024 * 1024); //1024 kb seems to be a reasonable buffer size
    static std::vector<char> memory2(1024 * 1024);

#ifdef ZEN_WIN
    wxFile file1(applyLongPathPrefix(source).c_str(), wxFile::read); //don't use buffered file input for verification!
#elif defined ZEN_LINUX || defined ZEN_MAC
    wxFile file1(::open(source.c_str(), O_RDONLY)); //utilize UTF-8 filename
#endif
    if (!file1.IsOpened())
        throw FileError(replaceCpy(_("Cannot read file %x."), L"%x", fmtFileName(source)) + L" (open)");

#ifdef ZEN_WIN
    wxFile file2(applyLongPathPrefix(target).c_str(), wxFile::read); //don't use buffered file input for verification!
#elif defined ZEN_LINUX || defined ZEN_MAC
    wxFile file2(::open(target.c_str(), O_RDONLY)); //utilize UTF-8 filename
#endif
    if (!file2.IsOpened()) //NO cleanup necessary for (wxFile) file1
        throw FileError(replaceCpy(_("Cannot read file %x."), L"%x", fmtFileName(target)) + L" (open)");

    do
    {
        const size_t length1 = file1.Read(&memory1[0], memory1.size());
        if (file1.Error())
            throw FileError(replaceCpy(_("Cannot read file %x."), L"%x", fmtFileName(source)));
        callback.updateStatus();

        const size_t length2 = file2.Read(&memory2[0], memory2.size());
        if (file2.Error())
            throw FileError(replaceCpy(_("Cannot read file %x."), L"%x", fmtFileName(target)));
        callback.updateStatus();

        if (length1 != length2 || ::memcmp(&memory1[0], &memory2[0], length1) != 0)
            throw FileError(replaceCpy(replaceCpy(_("Data verification error: %x and %y have different content."), L"%x", L"\n" + fmtFileName(source)), L"%y", L"\n" + fmtFileName(target)));
    }
    while (!file1.Eof());

    if (!file2.Eof())
        throw FileError(replaceCpy(replaceCpy(_("Data verification error: %x and %y have different content."), L"%x", L"\n" + fmtFileName(source)), L"%y", L"\n" + fmtFileName(target)));
}


class VerifyStatusUpdater : public VerifyCallback
{
public:
    VerifyStatusUpdater(ProcessCallback& statusHandler) : statusHandler_(statusHandler) {}

    virtual void updateStatus() { statusHandler_.requestUiRefresh(); } //trigger display refresh

private:
    ProcessCallback& statusHandler_;
};


void SynchronizeFolderPair::verifyFileCopy(const Zstring& source, const Zstring& target) const
{
    procCallback_.reportInfo(replaceCpy(txtVerifying, L"%x", fmtFileName(target)));

    VerifyStatusUpdater callback(procCallback_);
    tryReportingError([&] { ::verifyFiles(source, target, callback); }, procCallback_);
}
