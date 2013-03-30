// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) Zenju (zenju AT gmx DOT de) - All Rights Reserved        *
// **************************************************************************

#include "synchronization.h"
#include <memory>
#include <random>
#include <deque>
#include <stdexcept>
#include <wx/file.h> //get rid!?
#include <zen/format_unit.h>
#include <zen/scope_guard.h>
#include <zen/process_priority.h>
#include <zen/file_handling.h>
#include <zen/recycler.h>
#include <zen/optional.h>
#include "lib/resolve_path.h"
#include "lib/db_file.h"
#include "lib/dir_exist_async.h"
#include "lib/cmp_filetime.h"
#include <zen/file_io.h>
#include <zen/time.h>
#include "lib/status_handler_impl.h"
#include "lib/versioning.h"

#ifdef FFS_WIN
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
    std::for_each(begin(folderCmp), end(folderCmp), [&](const BaseDirMapping& baseMap) { recurse(baseMap); });
}


SyncStatistics::SyncStatistics(const HierarchyObject&  hierObj)
{
    init();
    recurse(hierObj);
}


SyncStatistics::SyncStatistics(const FileMapping& fileObj)
{
    init();
    calcStats(fileObj);
    rowsTotal += 1;
}


inline
void SyncStatistics::recurse(const HierarchyObject& hierObj)
{
    std::for_each(hierObj.refSubDirs ().begin(), hierObj.refSubDirs ().end(), [&](const DirMapping&     dirObj ) { calcStats(dirObj ); });
    std::for_each(hierObj.refSubFiles().begin(), hierObj.refSubFiles().end(), [&](const FileMapping&    fileObj) { calcStats(fileObj); });
    std::for_each(hierObj.refSubLinks().begin(), hierObj.refSubLinks().end(), [&](const SymLinkMapping& linkObj) { calcStats(linkObj); });

    rowsTotal += hierObj.refSubDirs(). size();
    rowsTotal += hierObj.refSubFiles().size();
    rowsTotal += hierObj.refSubLinks().size();
}


inline
void SyncStatistics::calcStats(const FileMapping& fileObj)
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
void SyncStatistics::calcStats(const SymLinkMapping& linkObj)
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
void SyncStatistics::calcStats(const DirMapping& dirObj)
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
    for (auto it = allPairs.begin(); it != allPairs.end(); ++it)
    {
        SyncConfig syncCfg = it->altSyncConfig.get() ? *it->altSyncConfig : mainCfg.syncCfg;

        output.push_back(
            FolderPairSyncCfg(syncCfg.directionCfg.var == DirectionConfig::AUTOMATIC,
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
        try { tryCleanup(false); }
        catch (...) {} //always (try to) clean up, even if synchronization is aborted!
        /*
        do not allow user callback:
        - make sure this stays non-blocking!
        - avoid throwing user abort exception again, leading to incomplete clean-up!
        */
    }

    //clean-up temporary directory (recycle bin optimization)
    void tryCleanup(bool allowUserCallback = true); //throw FileError -> call this in non-exceptional coding, i.e. somewhere after sync!

    template <class Function> void removeFileUpdating(const Zstring& relativeName, Int64 bytesExpected, Function notifyItemDeletion); //throw FileError
    template <class Function> void removeDirUpdating (const Zstring& relativeName, Int64 bytesExpected, Function notifyItemDeletion); //reports ONLY data delta via updateProcessedData()!
    template <class Function> void removeLinkUpdating(const Zstring& relativeName, Int64 bytesExpected, Function notifyItemDeletion, LinkDescriptor::LinkType lt); //

    const std::wstring& getTxtRemovingFile   () const { return txtRemovingFile;      } //
    const std::wstring& getTxtRemovingSymLink() const { return txtRemovingSymlink;   } //buffered status texts
    const std::wstring& getTxtRemovingDir    () const { return txtRemovingDirectory; } //

    //evaluate whether a deletion will actually free space within a volume
    bool deletionFreesSpace() const;
#ifdef FFS_WIN
    bool recyclerFallbackOnDelete() const { return recFallbackDelPermantently; }
#endif

private:
    DeletionHandling(const DeletionHandling&);
    DeletionHandling& operator=(const DeletionHandling&);

    void setDeletionPolicy(DeletionPolicy newPolicy);

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

#ifdef FFS_WIN
    Zstring getOrCreateRecyclerTempDirPf(); //throw FileError
    Zstring recyclerTmpDir; //temporary folder holding files/folders for *deferred* recycling
    std::vector<Zstring> toBeRecycled; //full path of files located in temporary folder, waiting for batch-recycling

    bool recFallbackDelPermantently;
#endif

    //magage three states: allow dynamic fallback from recycler to permanent deletion
    DeletionPolicy deletionPolicy_;
    std::unique_ptr<FileVersioner> versioner; //used for DELETE_TO_VERSIONING; throw FileError in constructor => create on demand!

    //buffer status texts:
    std::wstring txtRemovingFile;
    std::wstring txtRemovingSymlink;
    std::wstring txtRemovingDirectory;

    bool cleanedUp;
};

namespace
{
#ifdef FFS_WIN
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
}


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
#ifdef FFS_WIN
    recFallbackDelPermantently(false),
#endif
    deletionPolicy_(DELETE_TO_RECYCLER),
    cleanedUp(false)
{
#ifdef FFS_WIN
    if (!baseDirPf.empty())
        if (handleDel == DELETE_TO_RECYCLER && recycleBinStatusUpdating(baseDirPf, procCallback_) != STATUS_REC_EXISTS)
        {
            handleDel = DELETE_PERMANENTLY; //Windows' ::SHFileOperation() will do this anyway, but we have a better and faster deletion routine (e.g. on networks)
            recFallbackDelPermantently = true;
        }
#endif

    setDeletionPolicy(handleDel);
}


void DeletionHandling::setDeletionPolicy(DeletionPolicy newPolicy)
{
    deletionPolicy_ = newPolicy;

    switch (deletionPolicy_)
    {
        case DELETE_PERMANENTLY:
            txtRemovingFile      = _("Deleting file %x"         );
            txtRemovingDirectory = _("Deleting folder %x"       );
            txtRemovingSymlink   = _("Deleting symbolic link %x");
            break;

        case DELETE_TO_RECYCLER:
            txtRemovingFile      = _("Moving file %x to recycle bin"         );
            txtRemovingDirectory = _("Moving folder %x to recycle bin"       );
            txtRemovingSymlink   = _("Moving symbolic link %x to recycle bin");
            break;

        case DELETE_TO_VERSIONING:
            txtRemovingFile      = replaceCpy(_("Moving file %x to %y"         ), L"%y", fmtFileName(versioningDir_));
            txtRemovingDirectory = replaceCpy(_("Moving folder %x to %y"       ), L"%y", fmtFileName(versioningDir_));
            txtRemovingSymlink   = replaceCpy(_("Moving symbolic link %x to %y"), L"%y", fmtFileName(versioningDir_));
            break;
    }
}

#ifdef FFS_WIN
namespace
{
class CallbackMassRecycling : public CallbackRecycling
{
public:
    CallbackMassRecycling(ProcessCallback& statusHandler) :
        statusHandler_(statusHandler),
        txtRecyclingFile(_("Moving file %x to recycle bin")) {}

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

Zstring createUniqueRandomTempDir(const Zstring& baseDirPf) //throw FileError
{
    assert(endsWith(baseDirPf, FILE_NAME_SEPARATOR));

    //1. generate random directory name
    static std::default_random_engine rng(std::time(nullptr)); //a pseudo-random number engine with seconds-precision seed is sufficient!
    //the alternative std::random_device may not always be available and can even throw an exception!

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

    //2. ensure uniqueness (at least for this base directory)
    for (;;)
        try
        {
            Zstring dirname = generatePath();
            makeDirectory(dirname, /*bool failIfExists*/ true); //throw FileError, ErrorTargetExisting
            return dirname;
        }
        catch (const ErrorTargetExisting&) {}
}
}

//create + returns temporary directory postfixed with file name separator
//to support later cleanup if automatic deletion fails for whatever reason
Zstring DeletionHandling::getOrCreateRecyclerTempDirPf() //throw FileError
{
    assert(!baseDirPf_.empty());
    if (baseDirPf_.empty())
        return Zstring();

    if (recyclerTmpDir.empty())
        recyclerTmpDir = createUniqueRandomTempDir(baseDirPf_); //throw FileError
    //assemble temporary recycle bin directory with random name and .ffs_tmp ending
    return appendSeparator(recyclerTmpDir);
}
#endif

void DeletionHandling::tryCleanup(bool allowUserCallback) //throw FileError
{
    if (!cleanedUp)
    {
        switch (deletionPolicy_)
        {
            case DELETE_PERMANENTLY:
                break;

            case DELETE_TO_RECYCLER:
#ifdef FFS_WIN
                if (!recyclerTmpDir.empty())
                {
                    //move content of temporary directory to recycle bin in a single call
                    CallbackMassRecycling cbmr(procCallback_);
                    recycleOrDelete(toBeRecycled, allowUserCallback ? &cbmr : nullptr); //throw FileError

                    //clean up temp directory itself (should contain remnant empty directories only)
                    removeDirectory(recyclerTmpDir); //throw FileError
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

        cleanedUp = true;
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
void DeletionHandling::removeDirUpdating(const Zstring& relativeName, Int64 bytesExpected, Function notifyItemDeletion) //throw FileError
{
    assert(!baseDirPf_.empty());
    const Zstring fullName = baseDirPf_ + relativeName;

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
#ifdef FFS_WIN
            const Zstring targetDir = getOrCreateRecyclerTempDirPf() + relativeName; //throw FileError
            bool deleted = false;

            auto moveToTempDir = [&]
            {
                //performance optimization: Instead of moving each object into recycle bin separately,
                //we rename them one by one into a temporary directory and batch-recycle this directory after sync
                renameFile(fullName, targetDir); //throw FileError
                this->toBeRecycled.push_back(targetDir);
                deleted = true;
            };

            try
            {
                moveToTempDir(); //throw FileError
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
#elif defined FFS_LINUX || defined FFS_MAC
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
void DeletionHandling::removeFileUpdating(const Zstring& relativeName, Int64 bytesExpected, Function notifyItemDeletion) //throw FileError
{
    assert(!baseDirPf_.empty());
    const Zstring fullName = baseDirPf_ + relativeName;

    Int64 bytesReported;
    auto guardStatistics = makeGuard([&] { procCallback_.updateTotalData(0, bytesReported); }); //error = unexpected increase of total workload
    bool deleted = false;

    switch (deletionPolicy_)
    {
        case DELETE_PERMANENTLY:
            deleted = zen::removeFile(fullName); //[!] scope specifier resolves nameclash!
            break;

        case DELETE_TO_RECYCLER:
#ifdef FFS_WIN
            {
                const Zstring targetFile = getOrCreateRecyclerTempDirPf() + relativeName; //throw FileError

                auto moveToTempDir = [&]
                {
                    //performance optimization: Instead of moving each object into recycle bin separately,
                    //we rename them one by one into a temporary directory and batch-recycle this directory after sync
                    renameFile(fullName, targetFile); //throw FileError
                    this->toBeRecycled.push_back(targetFile);
                    deleted = true;
                };

                try
                {
                    moveToTempDir(); //throw FileError
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
#elif defined FFS_LINUX || defined FFS_MAC
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
void DeletionHandling::removeLinkUpdating(const Zstring& relativeName, Int64 bytesExpected, Function notifyItemDeletion, LinkDescriptor::LinkType lt) //throw FileError
{
    switch (lt)
    {
        case LinkDescriptor::TYPE_DIR:
            return removeDirUpdating(relativeName, bytesExpected, notifyItemDeletion); //throw FileError

        case LinkDescriptor::TYPE_FILE: //Windows: true file symlink; Linux: file-link or broken link
            return removeFileUpdating(relativeName, bytesExpected, notifyItemDeletion); //throw FileError
    }
}


//evaluate whether a deletion will actually free space within a volume
bool DeletionHandling::deletionFreesSpace() const
{
    switch (deletionPolicy_)
    {
        case DELETE_PERMANENTLY:
            return true;
        case DELETE_TO_RECYCLER:
            return false; //in general... (unless Recycle Bin is full)
        case DELETE_TO_VERSIONING:
            switch (zen::onSameVolume(baseDirPf_, versioningDir_))
            {
                case IS_SAME_YES:
                    return false;
                case IS_SAME_NO:
                    return true; //but other volume (versioningDir) may become full...
                case IS_SAME_CANT_SAY:
                    return true; //a rough guess!
            }
    }
    assert(false);
    return true;
}

//------------------------------------------------------------------------------------------------------------

namespace
{
class DiskSpaceNeeded
{
public:
    static std::pair<Int64, Int64> calculate(const BaseDirMapping& baseObj, bool freeSpaceDelLeft, bool freeSpaceDelRight)
    {
        DiskSpaceNeeded inst(baseObj, freeSpaceDelLeft, freeSpaceDelRight);
        return std::make_pair(inst.spaceNeededLeft, inst.spaceNeededRight);
    }

private:
    DiskSpaceNeeded(const BaseDirMapping& baseObj, bool freeSpaceDelLeft, bool freeSpaceDelRight) :
        freeSpaceDelLeft_(freeSpaceDelLeft),
        freeSpaceDelRight_(freeSpaceDelRight)
    {
        recurse(baseObj);
    }

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
                    if (freeSpaceDelLeft_)
                        spaceNeededLeft -= to<Int64>(it->getFileSize<LEFT_SIDE>());
                    break;

                case SO_DELETE_RIGHT:
                    if (freeSpaceDelRight_)
                        spaceNeededRight -= to<Int64>(it->getFileSize<RIGHT_SIDE>());
                    break;

                case SO_OVERWRITE_LEFT:
                    if (freeSpaceDelLeft_)
                        spaceNeededLeft -= to<Int64>(it->getFileSize<LEFT_SIDE>());
                    spaceNeededLeft += to<Int64>(it->getFileSize<RIGHT_SIDE>());
                    break;

                case SO_OVERWRITE_RIGHT:
                    if (freeSpaceDelRight_)
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
        std::for_each(hierObj.refSubDirs().begin(), hierObj.refSubDirs().end(), [&](const HierarchyObject& subDir) { this->recurse(subDir); });
        //for (auto& subDir : hierObj.refSubDirs())
        //      recurse(subDir);
    }

    const bool freeSpaceDelLeft_;
    const bool freeSpaceDelRight_;

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
#ifdef FFS_WIN
                          shadow::ShadowCopy* shadowCopyHandler,
#endif
                          DeletionHandling& delHandlingLeft,
                          DeletionHandling& delHandlingRight) :
        procCallback_(procCallback),
#ifdef FFS_WIN
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

    void startSync(BaseDirMapping& baseMap)
    {
        runZeroPass(baseMap);       //first process file moves
        runPass<PASS_ONE>(baseMap); //delete files (or overwrite big ones with smaller ones)
        runPass<PASS_TWO>(baseMap); //copy rest
    }

private:
    enum PassId
    {
        PASS_ONE, //delete files
        PASS_TWO, //create, modify
        PASS_NEVER //skip
    };

    static PassId getPass(const FileMapping&    fileObj);
    static PassId getPass(const SymLinkMapping& linkObj);
    static PassId getPass(const DirMapping&     dirObj);

    template <SelectedSide side>
    void prepare2StepMove(FileMapping& sourceObj, FileMapping& targetObj); //throw FileError
    bool createParentDir(FileSystemObject& fsObj); //throw FileError
    template <SelectedSide side>
    void manageFileMove(FileMapping& sourceObj, FileMapping& targetObj); //throw FileError

    void runZeroPass(HierarchyObject& hierObj);
    template <PassId pass>
    void runPass(HierarchyObject& hierObj);

    void synchronizeFile(FileMapping& fileObj);
    template <SelectedSide side> void synchronizeFileInt(FileMapping& fileObj, SyncOperation syncOp);

    void synchronizeLink(SymLinkMapping& linkObj);
    template <SelectedSide sideTrg> void synchronizeLinkInt(SymLinkMapping& linkObj, SyncOperation syncOp);

    void synchronizeFolder(DirMapping& dirObj);
    template <SelectedSide sideTrg> void synchronizeFolderInt(DirMapping& dirObj, SyncOperation syncOp);

    void reportStatus(const std::wstring& rawText, const Zstring& objname) const { procCallback_.reportStatus(replaceCpy(rawText, L"%x", fmtFileName(objname))); };
    void reportInfo  (const std::wstring& rawText, const Zstring& objname) const { procCallback_.reportInfo  (replaceCpy(rawText, L"%x", fmtFileName(objname))); };
    void reportInfo  (const std::wstring& rawText,
                      const Zstring& objname1,
                      const Zstring& objname2) const
    {
        procCallback_.reportInfo(replaceCpy(replaceCpy(rawText, L"%x", L"\n" + fmtFileName(objname1)), L"%y", L"\n" + fmtFileName(objname2)));
    };

    template <SelectedSide sideTrg, class Function>
    FileAttrib copyFileUpdatingTo(const FileMapping& fileObj, Function delTargetCommand) const; //throw FileError; reports data delta via updateProcessedData()
    void verifyFileCopy(const Zstring& source, const Zstring& target) const;

    template <SelectedSide side>
    DeletionHandling& getDelHandling();

    ProcessCallback& procCallback_;
#ifdef FFS_WIN
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
template <class Mapping> inline
bool haveNameClash(const Zstring& shortname, Mapping& m)
{
    return std::any_of(m.begin(), m.end(),
    [&](const typename Mapping::value_type& obj) { return EqualFilename()(obj.getObjShortName(), shortname); });
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
void SynchronizeFolderPair::prepare2StepMove(FileMapping& sourceObj,
                                             FileMapping& targetObj) //throw FileError
{
    const Zstring& source = sourceObj.getFullName<side>();
    const Zstring& tmpTarget = findUnusedTempName(sourceObj.getBaseDirPf<side>() + sourceObj.getShortName<side>());
    //this could still lead to a name-clash in obscure cases, if some file exists on the other side with
    //the very same (.ffs_tmp) name and is copied before the second step of the move is executed
    //good news: even in this pathologic case, this may only prevent the copy of the other file, but not the move

    reportInfo(txtMovingFile, source, tmpTarget);

    renameFile(source, tmpTarget); //throw FileError

    //update file hierarchy
    const FileDescriptor descrSource(sourceObj.getLastWriteTime<side>(),
                                     sourceObj.getFileSize     <side>(),
                                     sourceObj.getFileId       <side>());

    sourceObj.removeObject<side>();

    FileMapping& tempFile = sourceObj.root().addSubFile<side>(afterLast(tmpTarget, FILE_NAME_SEPARATOR), descrSource);
    static_assert(IsSameType<FixedList<FileMapping>, HierarchyObject::SubFileVec>::value,
                  "ATTENTION: we're adding to the file list WHILE looping over it! This is only working because FixedList iterators are not invalidated by this!");

    //prepare move in second pass
    tempFile.setSyncDir(side == LEFT_SIDE ? SYNC_DIR_LEFT : SYNC_DIR_RIGHT);

    targetObj.setMoveRef(tempFile .getId());
    tempFile .setMoveRef(targetObj.getId());

    //NO statistics update!
    procCallback_.requestUiRefresh(); //may throw
}


bool SynchronizeFolderPair::createParentDir(FileSystemObject& fsObj) //throw FileError, "false" on name clash
{
    if (DirMapping* parentDir = dynamic_cast<DirMapping*>(&fsObj.parent()))
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

        synchronizeFolder(*parentDir);  //throw FileError
    }
    return true;
}


template <SelectedSide side>
void SynchronizeFolderPair::manageFileMove(FileMapping& sourceObj,
                                           FileMapping& targetObj) //throw FileError
{
    assert((sourceObj.getSyncOperation() == SO_MOVE_LEFT_SOURCE  && targetObj.getSyncOperation() == SO_MOVE_LEFT_TARGET  && side == LEFT_SIDE) ||
           (sourceObj.getSyncOperation() == SO_MOVE_RIGHT_SOURCE && targetObj.getSyncOperation() == SO_MOVE_RIGHT_TARGET && side == RIGHT_SIDE));

    const bool sourceWillBeDeleted = [&]() -> bool
    {
        if (DirMapping* parentDir = dynamic_cast<DirMapping*>(&sourceObj.parent()))
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

    auto haveNameClash = [](const FileMapping& fileObj)
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
        synchronizeFile(sourceObj); //throw FileError
    }
    //else: sourceObj will not be deleted, and is not standing in the way => delay to second pass
    //note: this case may include new "move sources" from two-step sub-routine!!!
}


void SynchronizeFolderPair::runZeroPass(HierarchyObject& hierObj)
{
    //search for file move-operations

    for (auto it = hierObj.refSubFiles().begin(); it != hierObj.refSubFiles().end(); ++it) //VS 2010 crashes if we use for_each + lambda here...
    {
        FileMapping& fileObj = *it;

        const SyncOperation syncOp = fileObj.getSyncOperation();
        switch (syncOp) //evaluate comparison result and sync direction
        {
            case SO_MOVE_LEFT_SOURCE:
            case SO_MOVE_RIGHT_SOURCE:
            {
                FileMapping* sourceObj = &fileObj;
                if (FileMapping* targetObj = dynamic_cast<FileMapping*>(FileSystemObject::retrieve(fileObj.getMoveRef())))
                {
                    if (!tryReportingError([&]
                {
                    if (syncOp == SO_MOVE_LEFT_SOURCE)
                            this->manageFileMove<LEFT_SIDE>(*sourceObj, *targetObj);
                        else
                            this->manageFileMove<RIGHT_SIDE>(*sourceObj, *targetObj);
                    }, procCallback_))
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
    [&](DirMapping& dirObj) { this->runZeroPass(dirObj); /*recurse */ });
}

//---------------------------------------------------------------------------------------------------------------

//1st, 2nd pass requirements:
// - avoid disk space shortage: 1. delete files, 2. overwrite big with small files first
// - support change in type: overwrite file by directory, symlink by file, ect.

inline
SynchronizeFolderPair::PassId SynchronizeFolderPair::getPass(const FileMapping& fileObj)
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
        case SO_DO_NOTHING:
        case SO_EQUAL:
        case SO_UNRESOLVED_CONFLICT:
        case SO_COPY_METADATA_TO_LEFT:
        case SO_COPY_METADATA_TO_RIGHT:
            return PASS_TWO;
    }
    assert(false);
    return PASS_TWO; //dummy
}


inline
SynchronizeFolderPair::PassId SynchronizeFolderPair::getPass(const SymLinkMapping& linkObj)
{
    switch (linkObj.getSyncOperation()) //evaluate comparison result and sync direction
    {
        case SO_DELETE_LEFT:
        case SO_DELETE_RIGHT:
            return PASS_ONE; //make sure to delete symlinks in first pass, and equally named file or dir in second pass: usecase "overwrite symlink with regular file"!

        case SO_MOVE_LEFT_SOURCE:
        case SO_MOVE_RIGHT_SOURCE:
        case SO_MOVE_LEFT_TARGET:
        case SO_MOVE_RIGHT_TARGET:
            assert(false);
        case SO_OVERWRITE_LEFT:
        case SO_OVERWRITE_RIGHT:
        case SO_CREATE_NEW_LEFT:
        case SO_CREATE_NEW_RIGHT:
        case SO_DO_NOTHING:
        case SO_EQUAL:
        case SO_UNRESOLVED_CONFLICT:
        case SO_COPY_METADATA_TO_LEFT:
        case SO_COPY_METADATA_TO_RIGHT:
            return PASS_TWO;
    }
    assert(false);
    return PASS_TWO; //dummy
}


inline
SynchronizeFolderPair::PassId SynchronizeFolderPair::getPass(const DirMapping& dirObj)
{
    switch (dirObj.getSyncOperation()) //evaluate comparison result and sync direction
    {
        case SO_DELETE_LEFT:
        case SO_DELETE_RIGHT:
            return PASS_ONE;

        case SO_OVERWRITE_LEFT:
        case SO_OVERWRITE_RIGHT:
        case SO_MOVE_LEFT_SOURCE:
        case SO_MOVE_RIGHT_SOURCE:
        case SO_MOVE_LEFT_TARGET:
        case SO_MOVE_RIGHT_TARGET:
            assert(false);
        case SO_CREATE_NEW_LEFT:
        case SO_CREATE_NEW_RIGHT:
        case SO_DO_NOTHING:
        case SO_EQUAL:
        case SO_UNRESOLVED_CONFLICT:
        case SO_COPY_METADATA_TO_LEFT:
        case SO_COPY_METADATA_TO_RIGHT:
            return PASS_TWO;
    }
    assert(false);
    return PASS_TWO; //dummy
}


template <SynchronizeFolderPair::PassId pass>
void SynchronizeFolderPair::runPass(HierarchyObject& hierObj)
{
    //synchronize files:
    std::for_each(hierObj.refSubFiles().begin(), hierObj.refSubFiles().end(),
                  [&](FileMapping& fileObj)
    {
        if (pass == this->getPass(fileObj)) //"this->" required by two-pass lookup as enforced by GCC 4.7
            tryReportingError([&] { synchronizeFile(fileObj); }, procCallback_);
    });

    //synchronize symbolic links:
    std::for_each(hierObj.refSubLinks().begin(), hierObj.refSubLinks().end(),
                  [&](SymLinkMapping& linkObj)
    {
        if (pass == this->getPass(linkObj))
            tryReportingError([&] { synchronizeLink(linkObj); }, procCallback_);
    });

    //synchronize folders:
    std::for_each(hierObj.refSubDirs().begin(), hierObj.refSubDirs().end(),
                  [&](DirMapping& dirObj)
    {
        if (pass == this->getPass(dirObj))
            tryReportingError([&] { synchronizeFolder(dirObj); }, procCallback_);

        this->runPass<pass>(dirObj); //recurse
    });
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
void SynchronizeFolderPair::synchronizeFile(FileMapping& fileObj)
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
void SynchronizeFolderPair::synchronizeFileInt(FileMapping& fileObj, SyncOperation syncOp)
{
    static const SelectedSide sideSrc = OtherSide<sideTrg>::result;

    switch (syncOp)
    {
        case SO_CREATE_NEW_LEFT:
        case SO_CREATE_NEW_RIGHT:
        {
            if (const DirMapping* parentDir = dynamic_cast<DirMapping*>(&fileObj.parent()))
                if (parentDir->isEmpty<sideTrg>()) //BaseDirMapping OTOH is always non-empty and existing in this context => else: fatal error in zen::synchronize()
                    return; //if parent directory creation failed, there's no reason to show more errors!

            const Zstring& target = fileObj.getBaseDirPf<sideTrg>() + fileObj.getRelativeName<sideSrc>(); //can't use "getFullName" as target is not yet existing
            reportInfo(txtCreatingFile, target);

            try
            {
                const FileAttrib newAttr = copyFileUpdatingTo<sideTrg>(fileObj, [] {} /*no target to delete*/); //throw FileError

                const FileDescriptor descrSource(newAttr.modificationTime, newAttr.fileSize, newAttr.sourceFileId);
                const FileDescriptor descrTarget(newAttr.modificationTime, newAttr.fileSize, newAttr.targetFileId);
                fileObj.syncTo<sideTrg>(descrTarget, &descrSource); //update FileMapping

                procCallback_.updateProcessedData(1, 0); //processed bytes are reported in copyFileUpdatingTo()!
            }
            catch (FileError&)
            {
                if (somethingExists(fileObj.getFullName<sideSrc>())) //do not check on type (symlink, file, folder) -> if there is a type change, FFS should error out!
                    throw;
                //source deleted meanwhile...nothing was done (logical point of view!)
                fileObj.removeObject<sideSrc>();
                procCallback_.updateTotalData(-1, -to<zen::Int64>(fileObj.getFileSize<sideSrc>()));
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

                getDelHandling<sideTrg>().removeFileUpdating(fileObj.getObjRelativeName(), bytesExpected, [&] //throw FileError
                {
                    procCallback_.updateProcessedData(1, 0); //noexcept
                    ++objectsReported;
                });

                guardStatistics.dismiss(); //update statistics to consider the real amount of data
                if (objectsReported != objectsExpected)
                    procCallback_.updateTotalData(objectsReported - objectsExpected, 0); //noexcept!
            }
            fileObj.removeObject<sideTrg>(); //update FileMapping
            break;

        case SO_MOVE_LEFT_SOURCE:
        case SO_MOVE_LEFT_TARGET:
        case SO_MOVE_RIGHT_SOURCE:
        case SO_MOVE_RIGHT_TARGET:
            if (FileMapping* targetObj = dynamic_cast<FileMapping*>(FileSystemObject::retrieve(fileObj.getMoveRef())))
            {
                FileMapping* sourceObj = &fileObj;

                if (syncOp != SO_MOVE_LEFT_SOURCE &&
                    syncOp != SO_MOVE_RIGHT_SOURCE)
                    std::swap(sourceObj, targetObj);

                assert((sourceObj->getSyncOperation() == SO_MOVE_LEFT_SOURCE  && targetObj->getSyncOperation() == SO_MOVE_LEFT_TARGET  && sideTrg == LEFT_SIDE) ||
                       (sourceObj->getSyncOperation() == SO_MOVE_RIGHT_SOURCE && targetObj->getSyncOperation() == SO_MOVE_RIGHT_TARGET && sideTrg == RIGHT_SIDE));

                const Zstring& source = sourceObj->getFullName<sideTrg>();
                const Zstring& target = targetObj->getBaseDirPf<sideTrg>() + targetObj->getRelativeName<sideSrc>();

                reportInfo(txtMovingFile, source, target);

                renameFile(source, target); //throw FileError

                const FileDescriptor descrTarget(sourceObj->getLastWriteTime<sideTrg>(),
                                                 sourceObj->getFileSize     <sideTrg>(),
                                                 sourceObj->getFileId       <sideTrg>());

                sourceObj->removeObject<sideTrg>();      //update FileMapping
                targetObj->syncTo<sideTrg>(descrTarget); //

                procCallback_.updateProcessedData(1, 0);
            }
            break;

        case SO_OVERWRITE_LEFT:
        case SO_OVERWRITE_RIGHT:
        {
            const Zstring& target = fileObj.getBaseDirPf<sideTrg>() + fileObj.getRelativeName<sideSrc>(); //respect differences in case of source object
            reportInfo(txtOverwritingFile, target);

            const FileAttrib newAttr = copyFileUpdatingTo<sideTrg>(fileObj, [&] //delete target at appropriate time
            {
                reportStatus(this->getDelHandling<sideTrg>().getTxtRemovingFile(), fileObj.getFullName<sideTrg>());

                this->getDelHandling<sideTrg>().removeFileUpdating(fileObj.getObjRelativeName(), 0, []{}); //throw FileError;
                //no (logical) item count update desired - but total byte count may change, e.g. move(copy) deleted file to versioning dir
                fileObj.removeObject<sideTrg>(); //update FileMapping

                reportStatus(txtOverwritingFile, target); //restore status text copy file
            });

            const FileDescriptor descrSource(newAttr.modificationTime, newAttr.fileSize, newAttr.sourceFileId);
            const FileDescriptor descrTarget(newAttr.modificationTime, newAttr.fileSize, newAttr.targetFileId);
            fileObj.syncTo<sideTrg>(descrTarget, &descrSource); //update FileMapping

            procCallback_.updateProcessedData(1, 0); //we model "delete + copy" as ONE logical operation
        }
        break;

        case SO_COPY_METADATA_TO_LEFT:
        case SO_COPY_METADATA_TO_RIGHT:
        {
            reportInfo(txtWritingAttributes, fileObj.getFullName<sideTrg>());

            if (fileObj.getShortName<sideTrg>() != fileObj.getShortName<sideSrc>()) //adapt difference in case (windows only)
                renameFile(fileObj.getFullName<sideTrg>(),
                           beforeLast(fileObj.getFullName<sideTrg>(), FILE_NAME_SEPARATOR) + FILE_NAME_SEPARATOR + fileObj.getShortName<sideSrc>()); //throw FileError

            if (!sameFileTime(fileObj.getLastWriteTime<sideTrg>(), fileObj.getLastWriteTime<sideSrc>(), 2)) //respect 2 second FAT/FAT32 precision
                setFileTime(fileObj.getFullName<sideTrg>(), fileObj.getLastWriteTime<sideSrc>(), SYMLINK_FOLLOW); //throw FileError
            //do NOT read *current* source file time, but use buffered value which corresponds to time of comparison!

            const FileDescriptor descrTarget(fileObj.getLastWriteTime<sideSrc>(),
                                             fileObj.getFileSize     <sideTrg >(),
                                             fileObj.getFileId       <sideTrg >());
            fileObj.syncTo<sideTrg>(descrTarget); //-> both sides *should* be completely equal now...

            procCallback_.updateProcessedData(1, 0);
        }
        break;

        case SO_DO_NOTHING:
        case SO_EQUAL:
        case SO_UNRESOLVED_CONFLICT:
            return; //no update on processed data!
    }

    procCallback_.requestUiRefresh(); //may throw
}


inline
void SynchronizeFolderPair::synchronizeLink(SymLinkMapping& linkObj)
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
void SynchronizeFolderPair::synchronizeLinkInt(SymLinkMapping& linkObj, SyncOperation syncOp)
{
    static const SelectedSide sideSrc = OtherSide<sideTrg>::result;

    switch (syncOp)
    {
        case SO_CREATE_NEW_LEFT:
        case SO_CREATE_NEW_RIGHT:
        {
            if (const DirMapping* parentDir = dynamic_cast<DirMapping*>(&linkObj.parent()))
                if (parentDir->isEmpty<sideTrg>()) //BaseDirMapping OTOH is always non-empty and existing in this context => else: fatal error in zen::synchronize()
                    return; //if parent directory creation failed, there's no reason to show more errors!

            const Zstring& target = linkObj.getBaseDirPf<sideTrg>() + linkObj.getRelativeName<sideSrc>();

            reportInfo(txtCreatingLink, target);

            try
            {
                zen::copySymlink(linkObj.getFullName<sideSrc>(), target, copyFilePermissions_); //throw FileError
                linkObj.copyTo<sideTrg>(); //update SymLinkMapping

                procCallback_.updateProcessedData(1, 0);
            }
            catch (FileError&)
            {
                if (somethingExists(linkObj.getFullName<sideSrc>())) //do not check on type (symlink, file, folder) -> if there is a type change, FFS should not be quiet about it!
                    throw;
                //source deleted meanwhile...nothing was done (logical point of view!)
                linkObj.removeObject<sideSrc>();
                procCallback_.updateTotalData(-1, 0);
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

                getDelHandling<sideTrg>().removeLinkUpdating(linkObj.getObjRelativeName(), bytesExpected, [&] //throw FileError
                {
                    procCallback_.updateProcessedData(1, 0); //noexcept
                    ++objectsReported;
                }, linkObj.getLinkType<sideTrg>());

                guardStatistics.dismiss(); //update statistics to consider the real amount of data
                if (objectsReported != objectsExpected)
                    procCallback_.updateTotalData(objectsReported - objectsExpected, 0); //noexcept!
            }
            linkObj.removeObject<sideTrg>(); //update SymLinkMapping
            break;

        case SO_OVERWRITE_LEFT:
        case SO_OVERWRITE_RIGHT:
        {
            const Zstring& target = linkObj.getBaseDirPf<sideTrg>() + linkObj.getRelativeName<sideSrc>(); //respect differences in case of source object

            reportInfo(txtOverwritingLink, target);

            reportStatus(getDelHandling<sideTrg>().getTxtRemovingSymLink(), linkObj.getFullName<sideTrg>());
            getDelHandling<sideTrg>().removeLinkUpdating(linkObj.getObjRelativeName(), 0, [] {}, linkObj.getLinkType<sideTrg>()); //throw FileError
            linkObj.removeObject<sideTrg>(); //remove file from FileMapping, to keep in sync (if subsequent copying fails!!)

            reportStatus(txtOverwritingLink, target); //restore status text
            zen::copySymlink(linkObj.getFullName<sideSrc>(), target, copyFilePermissions_); //throw FileError
            linkObj.copyTo<sideTrg>(); //update SymLinkMapping

            procCallback_.updateProcessedData(1, 0); //we model "delete + copy" as ONE logical operation
        }
        break;

        case SO_COPY_METADATA_TO_LEFT:
        case SO_COPY_METADATA_TO_RIGHT:
            reportInfo(txtWritingAttributes, linkObj.getFullName<sideTrg>());

            if (linkObj.getShortName<sideTrg>() != linkObj.getShortName<sideSrc>()) //adapt difference in case (windows only)
                renameFile(linkObj.getFullName<sideTrg>(),
                           beforeLast(linkObj.getFullName<sideTrg>(), FILE_NAME_SEPARATOR) + FILE_NAME_SEPARATOR + linkObj.getShortName<sideSrc>()); //throw FileError

            if (!sameFileTime(linkObj.getLastWriteTime<sideTrg>(), linkObj.getLastWriteTime<sideSrc>(), 2)) //respect 2 second FAT/FAT32 precision
                setFileTime(linkObj.getFullName<sideTrg>(), linkObj.getLastWriteTime<sideSrc>(), SYMLINK_DIRECT); //throw FileError
            linkObj.copyTo<sideTrg>(); //-> both sides *should* be completely equal now...

            procCallback_.updateProcessedData(1, 0);
            break;

        case SO_MOVE_LEFT_SOURCE:
        case SO_MOVE_RIGHT_SOURCE:
        case SO_MOVE_LEFT_TARGET:
        case SO_MOVE_RIGHT_TARGET:
            assert(false);
        case SO_DO_NOTHING:
        case SO_EQUAL:
        case SO_UNRESOLVED_CONFLICT:
            return; //no update on processed data!
    }

    procCallback_.requestUiRefresh(); //may throw
}


inline
void SynchronizeFolderPair::synchronizeFolder(DirMapping& dirObj)
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
void SynchronizeFolderPair::synchronizeFolderInt(DirMapping& dirObj, SyncOperation syncOp)
{
    static const SelectedSide sideSrc = OtherSide<sideTrg>::result;

    switch (syncOp)
    {
        case SO_CREATE_NEW_LEFT:
        case SO_CREATE_NEW_RIGHT:
            if (const DirMapping* parentDir = dynamic_cast<DirMapping*>(&dirObj.parent()))
                if (parentDir->isEmpty<sideTrg>()) //BaseDirMapping OTOH is always non-empty and existing in this context => else: fatal error in zen::synchronize()
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
                dirObj.copyTo<sideTrg>(); //update DirMapping

                procCallback_.updateProcessedData(1, 0);
            }
            else //source deleted meanwhile...nothing was done (logical point of view!) -> uh....what about a temporary network drop???
            {
                dirObj.refSubFiles().clear();   //
                dirObj.refSubLinks().clear();   //update DirMapping
                dirObj.refSubDirs ().clear();   //
                dirObj.removeObject<sideSrc>(); //

                const SyncStatistics subStats(dirObj);
                procCallback_.updateTotalData(-getCUD(subStats) - 1, -subStats.getDataToProcess());
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

                getDelHandling<sideTrg>().removeDirUpdating(dirObj.getObjRelativeName(), bytesExpected, [&] //throw FileError
                {
                    procCallback_.updateProcessedData(1, 0); //noexcept
                    ++objectsReported;
                });

                guardStatistics.dismiss(); //update statistics to consider the real amount of data
                if (objectsReported != objectsExpected)
                    procCallback_.updateTotalData(objectsReported - objectsExpected, 0); //noexcept!
            }
            dirObj.refSubFiles().clear();   //
            dirObj.refSubLinks().clear();   //update DirMapping
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
            dirObj.copyTo<sideTrg>(); //-> both sides *should* be completely equal now...

            procCallback_.updateProcessedData(1, 0);
            break;

        case SO_OVERWRITE_RIGHT:
        case SO_OVERWRITE_LEFT:
        case SO_MOVE_LEFT_SOURCE:
        case SO_MOVE_RIGHT_SOURCE:
        case SO_MOVE_LEFT_TARGET:
        case SO_MOVE_RIGHT_TARGET:
            assert(false);
        case SO_UNRESOLVED_CONFLICT:
        case SO_DO_NOTHING:
        case SO_EQUAL:
            return; //no update on processed data!
    }

    procCallback_.requestUiRefresh(); //may throw
}


namespace
{
void makeSameLength(std::wstring& first, std::wstring& second)
{
    const size_t maxPref = std::max(first.length(), second.length());
    first .resize(maxPref, L' ');
    second.resize(maxPref, L' ');
}


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
bool createBaseDirectory(BaseDirMapping& baseMap, ProcessCallback& callback) //nothrow; return false if fatal error occurred
{
    const Zstring dirname = beforeLast(baseMap.getBaseDirPf<side>(), FILE_NAME_SEPARATOR); //what about C:\ ???
    if (dirname.empty())
        return true;

    if (baseMap.isExisting<side>()) //atomicity: do NOT check directory existence again!
    {
        //just convenience: exit sync right here instead of showing tons of error messages during file copy
        return tryReportingError([&]
        {
            if (!dirExistsUpdating(dirname, false, callback))
                throw FileError(replaceCpy(_("Cannot find %x."), L"%x", fmtFileName(dirname))); //this should really be a "fatal error" if not recoverable
        }, callback); //may throw in error-callback!
    }
    else //create target directory: user presumably ignored error "dir existing" in order to have it created automatically
    {
        bool temporaryNetworkDrop = false;
        bool rv = tryReportingError([&]
        {
            try
            {
                //a nice race-free check and set operation:
                makeDirectory(dirname, /*bool failIfExists*/ true); //throw FileError, ErrorTargetExisting
                baseMap.setExisting<side>(true); //update our model!
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
        return rv && !temporaryNetworkDrop;
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


    //initialize deletion handling: already required when checking for warnings
    FixedList<DeletionHandling> delHandlerL; //we can't use a FixedList<std::pair<>> because DeletionHandling is not copy-constructable
    FixedList<DeletionHandling> delHandlerR;
    for (auto j = begin(folderCmp); j != end(folderCmp); ++j)
    {
        const size_t folderIndex = j - folderCmp.begin();
        const FolderPairSyncCfg& folderPairCfg = syncConfig[folderIndex];

        delHandlerL.emplace_back(folderPairCfg.handleDeletion,
                                 folderPairCfg.versioningFolder,
                                 folderPairCfg.versioningStyle_,
                                 timeStamp,
                                 j->getBaseDirPf<LEFT_SIDE>(),
                                 callback);

        delHandlerR.emplace_back(folderPairCfg.handleDeletion,
                                 folderPairCfg.versioningFolder,
                                 folderPairCfg.versioningStyle_,
                                 timeStamp,
                                 j->getBaseDirPf<RIGHT_SIDE>(),
                                 callback);
    }

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

#ifdef FFS_WIN
    std::set<Zstring, LessFilename> recyclMissing;
#endif

    //start checking folder pairs
    {
        auto iterDelHandlerL = delHandlerL.cbegin();
        auto iterDelHandlerR = delHandlerR.cbegin();
        for (auto j = begin(folderCmp); j != end(folderCmp); ++j, ++iterDelHandlerL, ++iterDelHandlerR)
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

            //skip folder pair if there is nothing to do (except for automatic mode, where data base needs to be written even in this case)
            if (!writeLeft && !writeRight &&
                !folderPairCfg.inAutomaticMode)
            {
                skipFolderPair[folderIndex] = true; //skip creating (not yet existing) base directories in particular if there's no need
                continue;
            }


            //check empty input fields: basically this only makes sense if empty field is not target (and not automatic mode: because of db file creation)
            if ((j->getBaseDirPf<LEFT_SIDE >().empty() && (writeLeft  || folderPairCfg.inAutomaticMode)) ||
                (j->getBaseDirPf<RIGHT_SIDE>().empty() && (writeRight || folderPairCfg.inAutomaticMode)))
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
                    callback.reportFatalError(_("Folder input field for versioning must not be empty."));
                    skipFolderPair[folderIndex] = true;
                    continue;
                }
            }

            //the following scenario is covered by base directory creation below in case source directory exists (accessible or not), but latter doesn't cover not-yet-created source!!!
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
            auto checkSpace = [&](const Zstring& baseDirPf, const Int64& spaceRequired)
            {
                try
                {
                    Int64 freeSpace = to<Int64>(getFreeDiskSpace(baseDirPf)); //throw FileError

                    if (0 < freeSpace && //zero disk space is either an error or not: in both cases this warning message is obsolete (WebDav seems to report 0)
                        freeSpace < spaceRequired)
                        diskSpaceMissing.push_back(std::make_pair(baseDirPf, std::make_pair(spaceRequired, freeSpace)));
                }
                catch (FileError&) {}
            };
            const std::pair<Int64, Int64> spaceNeeded = DiskSpaceNeeded::calculate(*j,
                                                                                   iterDelHandlerL->deletionFreesSpace(),
                                                                                   iterDelHandlerR->deletionFreesSpace());
            checkSpace(j->getBaseDirPf<LEFT_SIDE >(), spaceNeeded.first);
            checkSpace(j->getBaseDirPf<RIGHT_SIDE>(), spaceNeeded.second);

#ifdef FFS_WIN
            //windows: check if recycle bin really exists; if not, Windows will silently delete, which is wrong
            if (folderPairCfg.handleDeletion == DELETE_TO_RECYCLER)
            {
                if (folderPairStat.getUpdate<LEFT_SIDE>() +
                    folderPairStat.getDelete<LEFT_SIDE>() > 0 &&
                    iterDelHandlerL->recyclerFallbackOnDelete())
                    recyclMissing.insert(j->getBaseDirPf<LEFT_SIDE>());

                if (folderPairStat.getUpdate<RIGHT_SIDE>() +
                    folderPairStat.getDelete<RIGHT_SIDE>() > 0 &&
                    iterDelHandlerR->recyclerFallbackOnDelete())
                    recyclMissing.insert(j->getBaseDirPf<RIGHT_SIDE>());
            }
#endif
        }
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
        std::wstring msg = _("Significant difference detected:");

        for (auto it = significantDiff.begin(); it != significantDiff.end(); ++it)
            msg += std::wstring(L"\n\n") +
                   it->first + L" <-> " + L"\n" +
                   it->second;
        msg += L"\n\n";
        msg += _("More than 50% of the total number of files will be copied or deleted!");

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

#ifdef FFS_WIN
    //windows: check if recycle bin really exists; if not, Windows will silently delete, which is wrong
    if (!recyclMissing.empty())
    {
        std::wstring msg = _("Recycle Bin is not available for the following paths! Files will be deleted permanently instead:") + L"\n";
        std::for_each(recyclMissing.begin(), recyclMissing.end(),
        [&](const Zstring& path) { msg += std::wstring(L"\n") + path; });

        callback.reportWarning(msg, warnings.warningRecyclerMissing);
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

#ifdef FFS_WIN
    //shadow copy buffer: per sync-instance, not folder pair
    std::unique_ptr<shadow::ShadowCopy> shadowCopyHandler;
    if (copyLockedFiles)
        shadowCopyHandler = make_unique<shadow::ShadowCopy>();
#endif

    try
    {
        //loop through all directory pairs
        auto iterDelHandlerL = delHandlerL.begin();
        auto iterDelHandlerR = delHandlerR.begin();
        for (auto j = begin(folderCmp); j != end(folderCmp); ++j, ++iterDelHandlerL, ++iterDelHandlerR)
        {
            //exclude some pathological case (leftdir, rightdir are empty)
            if (EqualFilename()(j->getBaseDirPf<LEFT_SIDE>(), j->getBaseDirPf<RIGHT_SIDE>()))
                continue;

            //------------------------------------------------------------------------------------------
            //always report folder pairs for log file, even if there is no work to do
            std::wstring left  = _("Left")  + L": ";
            std::wstring right = _("Right") + L": ";
            makeSameLength(left, right);

            callback.reportInfo(_("Synchronizing folder pair:") + L"\n" +
                                L"    " + left  + j->getBaseDirPf<LEFT_SIDE >() + L"\n" +
                                L"    " + right + j->getBaseDirPf<RIGHT_SIDE>());
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
                if (folderPairCfg.inAutomaticMode)
                    try { zen::saveLastSynchronousState(*j); } //throw FileError
                    catch (...) {}
            });

            //guarantee removal of invalid entries (where element on both sides is empty)
            ZEN_ON_SCOPE_EXIT(BaseDirMapping::removeEmpty(*j););

            bool copyPermissionsFp = false;
            tryReportingError([&]
            {
                copyPermissionsFp = copyFilePermissions && //copy permissions only if asked for and supported by *both* sides!
                !j->getBaseDirPf<LEFT_SIDE >().empty() && //scenario: directory selected on one side only
                !j->getBaseDirPf<RIGHT_SIDE>().empty() && //
                supportsPermissions(beforeLast(j->getBaseDirPf<LEFT_SIDE >(), FILE_NAME_SEPARATOR)) && //throw FileError
                supportsPermissions(beforeLast(j->getBaseDirPf<RIGHT_SIDE>(), FILE_NAME_SEPARATOR));
            }, callback); //show error dialog if necessary

            SynchronizeFolderPair syncFP(callback, verifyCopiedFiles, copyPermissionsFp, transactionalFileCopy,
#ifdef FFS_WIN
                                         shadowCopyHandler.get(),
#endif
                                         *iterDelHandlerL, *iterDelHandlerR);
            syncFP.startSync(*j);

            //(try to gracefully) cleanup temporary Recycle bin folders and versioning -> will be done in ~DeletionHandling anyway...
            tryReportingError([&] { iterDelHandlerL->tryCleanup(); }, callback); //show error dialog if necessary
            tryReportingError([&] { iterDelHandlerR->tryCleanup(); }, callback); //

            //(try to gracefully) write database file (will be done in ~EnforceUpdateDatabase anyway...)
            if (folderPairCfg.inAutomaticMode)
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
template <SelectedSide sideTrg, class Function>
FileAttrib SynchronizeFolderPair::copyFileUpdatingTo(const FileMapping& fileObj, Function delTargetCommand) const //returns current attributes of source file
{
    static const SelectedSide sideSrc = OtherSide<sideTrg>::result;

    FileAttrib newAttr;
    const Int64 bytesExpected = to<Int64>(fileObj.getFileSize<sideSrc>());
    Zstring source = fileObj.getFullName<sideSrc>();
    const Zstring& target = fileObj.getBaseDirPf<sideTrg>() + fileObj.getRelativeName<sideSrc>();

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
        target,
        copyFilePermissions_,
        transactionalFileCopy_,
        &callback,
        &newAttr); //throw FileError, ErrorFileLocked

        //#################### Verification #############################
        if (verifyCopiedFiles_)
        {
            auto guardTarget = makeGuard([&] { removeFile(target); }); //delete target if verification fails
            verifyFileCopy(source, target); //throw FileError
            guardTarget.dismiss();
        }
        //#################### /Verification #############################

        //update statistics to consider the real amount of data, e.g. more than the "file size" for ADS streams,
        //less for sparse and compressed files,  or file changed in the meantime!
        if (bytesReported != bytesExpected)
            procCallback_.updateTotalData(0, bytesReported - bytesExpected); //noexcept!

        guardStatistics.dismiss();
    };

#ifdef FFS_WIN
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
            source = shadowCopyHandler_->makeShadowCopy(source); //throw FileError
        }
        catch (const FileError& e2)
        {
            throw FileError(e1.toString() + L"\n\n" + e2.toString());
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

void verifyFiles(const Zstring& source, const Zstring& target, VerifyCallback& callback) // throw (FileError)
{
    static std::vector<char> memory1(1024 * 1024); //1024 kb seems to be a reasonable buffer size
    static std::vector<char> memory2(1024 * 1024);

#ifdef FFS_WIN
    wxFile file1(applyLongPathPrefix(source).c_str(), wxFile::read); //don't use buffered file input for verification!
#elif defined FFS_LINUX || defined FFS_MAC
    wxFile file1(::open(source.c_str(), O_RDONLY)); //utilize UTF-8 filename
#endif
    if (!file1.IsOpened())
        throw FileError(replaceCpy(_("Cannot read file %x."), L"%x", fmtFileName(source)));

#ifdef FFS_WIN
    wxFile file2(applyLongPathPrefix(target).c_str(), wxFile::read); //don't use buffered file input for verification!
#elif defined FFS_LINUX || defined FFS_MAC
    wxFile file2(::open(target.c_str(), O_RDONLY)); //utilize UTF-8 filename
#endif
    if (!file2.IsOpened()) //NO cleanup necessary for (wxFile) file1
        throw FileError(replaceCpy(_("Cannot read file %x."), L"%x", fmtFileName(target)));

    do
    {
        const size_t length1 = file1.Read(&memory1[0], memory1.size());
        if (file1.Error())
            throw FileError(replaceCpy(_("Cannot read file %x."), L"%x", fmtFileName(source)) + L" (r)");
        callback.updateStatus(); //send progress updates

        const size_t length2 = file2.Read(&memory2[0], memory2.size());
        if (file2.Error())
            throw FileError(replaceCpy(_("Cannot read file %x."), L"%x", fmtFileName(target)) + L" (r)");
        callback.updateStatus(); //send progress updates

        if (length1 != length2 || ::memcmp(&memory1[0], &memory2[0], length1) != 0)
            throw FileError(_("Data verification error: Source and target file have different content!") + L"\n" + fmtFileName(source) + L" -> \n" + fmtFileName(target));
    }
    while (!file1.Eof());

    if (!file2.Eof())
        throw FileError(_("Data verification error: Source and target file have different content!") + L"\n" + fmtFileName(source) + L" -> \n" + fmtFileName(target));
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
