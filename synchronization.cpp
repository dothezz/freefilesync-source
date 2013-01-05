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
    //conflict    = 0;
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
            //++conflict;
            //if (conflictMsgs.size() < MAX_CONFLICTS) //save the first conflict texts
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
            //++conflict;
            //if (conflictMsgs.size() < MAX_CONFLICTS) //save the first conflict texts
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

        case SO_OVERWRITE_LEFT:
        case SO_OVERWRITE_RIGHT:
            assert(false);
            break;

        case SO_UNRESOLVED_CONFLICT:
            //++conflict;
            //if (conflictMsgs.size() < MAX_CONFLICTS) //save the first conflict texts
            conflictMsgs.push_back(std::make_pair(dirObj.getObjRelativeName(), dirObj.getSyncOpConflict()));
            break;

        case SO_COPY_METADATA_TO_LEFT:
            ++updateLeft;
            break;

        case SO_COPY_METADATA_TO_RIGHT:
            ++updateRight;
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
    for (auto i = allPairs.begin(); i != allPairs.end(); ++i)
    {
        SyncConfig syncCfg = i->altSyncConfig.get() ? *i->altSyncConfig : mainCfg.syncCfg;

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
                                //folderPairStat.getUpdate() +  -> not relevant when testing for "wrong folder selected"
                                folderPairStat.getDelete  () +
                                folderPairStat.getConflict(); //?

    return nonMatchingRows >= 10 && nonMatchingRows > 0.5 * folderPairStat.getRowCount();
}

//#################################################################################################################

/*
class PhysicalStatistics //counts *physical* operations, actual items processed (NOT disk accesses) and bytes transferred
{
public:
    PhysicalStatistics(const FolderComparison& folderCmp) : items(0)
    {
        delType =;
        std::for_each(begin(folderCmp), end(folderCmp), [&](const BaseDirMapping& baseMap) { recurse(baseMap); });
    }

    int   getItems() const { return items; }
    Int64 getBytes() const { return bytes; }

private:
    enum DeletionType
{
    DEL_PERMANENTLY,
    DEL_RECYCLE_BIN,
    VERSIONING_SAME_VOL,
    VERSIONING_DIFF_VOL,
};

    void recurse(const HierarchyObject& hierObj)
    {
        std::for_each(hierObj.refSubDirs ().begin(), hierObj.refSubDirs ().end(), [&](const DirMapping&     dirObj ) { calcStats(dirObj ); });
        std::for_each(hierObj.refSubFiles().begin(), hierObj.refSubFiles().end(), [&](const FileMapping&    fileObj) { calcStats(fileObj); });
        std::for_each(hierObj.refSubLinks().begin(), hierObj.refSubLinks().end(), [&](const SymLinkMapping& linkObj) { calcStats(linkObj); });
    }

    void calcStats(const FileMapping& fileObj)
    {
        switch (fileObj.getSyncOperation()) //evaluate comparison result and sync direction
        {
            case SO_CREATE_NEW_LEFT:
                items += 2;
                bytes += to<Int64>(fileObj.getFileSize<RIGHT_SIDE>());
                break;

            case SO_CREATE_NEW_RIGHT:
                ++items;
                bytes += to<Int64>(fileObj.getFileSize<LEFT_SIDE>());
                break;

            case SO_DELETE_LEFT:
                switch (delType)
                    {
                    case DEL_INSTANTLY:
                        ++items;
                        break;
                        case DEL_COPY_DELETE:
                        break;
                    }
                break;

            case SO_DELETE_RIGHT:
                ++items;
                break;

            case SO_MOVE_LEFT_TARGET:
            case SO_MOVE_RIGHT_TARGET:
                ++items;
                break;

            case SO_MOVE_LEFT_SOURCE:  //ignore; already counted
            case SO_MOVE_RIGHT_SOURCE: //
                break;

            case SO_OVERWRITE_LEFT:
                //todo: delete
                items += 2; //read + write
                bytes += to<Int64>(fileObj.getFileSize<RIGHT_SIDE>());
                break;

            case SO_OVERWRITE_RIGHT:
                //todo: delete
                items += 2; //read + write
                bytes += to<Int64>(fileObj.getFileSize<LEFT_SIDE>());
                break;

            case SO_COPY_METADATA_TO_LEFT:
            case SO_COPY_METADATA_TO_RIGHT:
                ++items;
                break;

            case SO_UNRESOLVED_CONFLICT:
            case SO_DO_NOTHING:
            case SO_EQUAL:
                break;
        }
    }

    void calcStats(const SymLinkMapping& linkObj)
    {

    }

    void calcStats(const DirMapping& dirObj)
    {
        //since we model physical stats, we recurse only if deletion variant is "permanently" or "user-defined + different volume",
        //else deletion is done as a single physical operation
    }

    int items;
    Int64 bytes;

DeletionType delType;
};
*/

//--------------------------------------------------------------------------------------------------------------

class DeletionHandling //e.g. generate name of alternate deletion directory (unique for session AND folder pair)
{
public:
    DeletionHandling(DeletionPolicy handleDel, //nothrow!
                     const Zstring& versioningDir,
                     VersioningStyle versioningStyle,
                     const TimeComp& timeStamp,
                     size_t folderIndex,
                     const Zstring& baseDirPf, //with separator postfix
                     ProcessCallback& procCallback);
    ~DeletionHandling()
    {
        try { tryCleanup(false); } //always (try to) clean up, even if synchronization is aborted!
        catch (...) {}
        /*
        do not allow user callback:
        - make sure this stays non-blocking!
        - avoid throwing user abort exception again, leading to incomplete clean-up!
        */
    }

    //clean-up temporary directory (recycle bin optimization)
    void tryCleanup(bool allowUserCallback = true); //throw FileError -> call this in non-exceptional coding, i.e. somewhere after sync!

    void removeFile  (const Zstring& relativeName); //throw FileError
    void removeFolder(const Zstring& relativeName) { removeFolderInt(relativeName, nullptr, nullptr); }; //throw FileError
    void removeFolderUpdateStatistics(const Zstring& relativeName, int objectsExpected, Int64 dataExpected) { removeFolderInt(relativeName, &objectsExpected, &dataExpected); }; //throw FileError
    //in contrast to "removeFolder()" this function will update statistics!

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

    void removeFolderInt(const Zstring& relativeName, const int* objectsExpected, const Int64* dataExpected); //throw FileError
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
    Zstring recyclerTmpDirPf; //temporary folder holding files/folders for *deferred* recycling (postfixed with file name separator)
    std::vector<Zstring> toBeRecycled; //full path of files located in temporary folder, waiting to be recycled

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


DeletionHandling::DeletionHandling(DeletionPolicy handleDel, //nothrow!
                                   const Zstring& versioningDir,
                                   VersioningStyle versioningStyle,
                                   const TimeComp& timeStamp,
                                   size_t folderIndex,
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
        if (handleDel == DELETE_TO_RECYCLER && recycleBinStatus(baseDirPf) != STATUS_REC_EXISTS)
        {
            handleDel = DELETE_PERMANENTLY; //Windows' ::SHFileOperation() will do this anyway, but we have a better and faster deletion routine (e.g. on networks)
            recFallbackDelPermantently = true;
        }

    //assemble temporary recycler bin directory
    if (!baseDirPf_.empty())
    {
        Zstring tempDir = baseDirPf_ + Zstr("FFS ") + formatTime<Zstring>(Zstr("%Y-%m-%d %H%M%S"), timeStamp);
        if (folderIndex > 0)
            tempDir += Zstr(" [") + numberTo<Zstring>(folderIndex + 1) + Zstr("]"); //e.g. C:\Source\FFS 2012-05-15 131513 [2]

        recyclerTmpDirPf = appendSeparator(tempDir);
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
                if (!recyclerTmpDirPf.empty())
                {
                    //move content of temporary directory to recycle bin in a single call
                    CallbackMassRecycling cbmr(procCallback_);
                    recycleOrDelete(toBeRecycled, allowUserCallback ? &cbmr : nullptr); //throw FileError

                    //clean up temp directory itself (should contain remnant empty directories only)
                    removeDirectory(beforeLast(recyclerTmpDirPf, FILE_NAME_SEPARATOR)); //throw FileError
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
class CallbackMoveFileImpl : public CallbackMoveFile
{
public:
    CallbackMoveFileImpl(ProcessCallback& statusHandler,
                         int* objectsReported) :
        statusHandler_  (statusHandler),
        objectsReported_(objectsReported),
        txtMovingFile   (_("Moving file %x to %y")),
        txtMovingFolder (_("Moving folder %x to %y")) {}

private:
    virtual void onBeforeFileMove(const Zstring& fileFrom, const Zstring& fileTo) { reportStatus(txtMovingFile, fileFrom, fileTo); }
    virtual void onBeforeDirMove (const Zstring& dirFrom,  const Zstring& dirTo ) { reportStatus(txtMovingFolder, dirFrom, dirTo); }
    virtual void objectProcessed() //one call after each processed move
    {
        if (objectsReported_)
        {
            statusHandler_.updateProcessedData(1, 0);
            ++*objectsReported_;
        }
    }

    virtual void updateStatus(Int64 bytesDelta)
    {
        //statusHandler_.updateProcessedData(0, bytesDelta);
        //bytesReported_ += bytesDelta; -> statistics model *logical* operations! as such a file delete is only (1 obj/0 bytes)! Doesn't matter if it's actually copy + delete

        statusHandler_.requestUiRefresh();
    }

    void reportStatus(const std::wstring& statusText, const Zstring& fileFrom, const Zstring& fileTo) const
    {
        statusHandler_.reportStatus(replaceCpy(replaceCpy(statusText, L"%x", fmtFileName(fileFrom)), L"%y", fmtFileName(fileTo)));
    };

    ProcessCallback& statusHandler_;
    int* objectsReported_; //optional

    const std::wstring txtMovingFile;
    const std::wstring txtMovingFolder;
};


struct CallbackRemoveDirImpl : public CallbackRemoveDir
{
    CallbackRemoveDirImpl(ProcessCallback& statusHandler,
                          const DeletionHandling& delHandling,
                          int* objectsReported) :
        statusHandler_(statusHandler),
        delHandling_(delHandling),
        objectsReported_(objectsReported) {}

private:
    virtual void notifyFileDeletion(const Zstring& filename) { processSingleObject(delHandling_.getTxtRemovingFile(), filename); }
    virtual void notifyDirDeletion (const Zstring& dirname ) { processSingleObject(delHandling_.getTxtRemovingDir (), dirname ); }

    void processSingleObject(const std::wstring& statusText, const Zstring& objName)
    {
        statusHandler_.reportStatus(replaceCpy(statusText, L"%x", fmtFileName(objName)));

        if (objectsReported_)
        {
            statusHandler_.updateProcessedData(1, 0);
            ++*objectsReported_;
        }
    }

    ProcessCallback& statusHandler_;
    const DeletionHandling& delHandling_;
    int* objectsReported_; //optional
};
}


void DeletionHandling::removeFile(const Zstring& relativeName)
{
    const Zstring fullName = baseDirPf_ + relativeName;

    switch (deletionPolicy_)
    {
        case DELETE_PERMANENTLY:
            zen::removeFile(fullName);
            //[!] scope specifier resolves nameclash!
            break;

        case DELETE_TO_RECYCLER:
#ifdef FFS_WIN
            {
                const Zstring targetFile = recyclerTmpDirPf + relativeName; //ends with path separator

                auto moveToTempDir = [&]
                {
                    //performance optimization: Instead of moving each object into recycle bin separately,
                    //we rename them one by one into a temporary directory and batch-recycle this directory after sync
                    renameFile(fullName, targetFile); //throw FileError
                    toBeRecycled.push_back(targetFile);
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
#elif defined FFS_LINUX
            recycleOrDelete(fullName); //throw FileError
#endif
            break;

        case DELETE_TO_VERSIONING:
        {
            CallbackMoveFileImpl callback(procCallback_, nullptr); //we do *not* report statistics in this method
            getOrCreateVersioner().revisionFile(fullName, relativeName, callback); //throw FileError
        }
        break;
    }
}


void DeletionHandling::removeFolderInt(const Zstring& relativeName, const int* objectsExpected, const Int64* dataExpected) //throw FileError
{
    const Zstring fullName = baseDirPf_ + relativeName;

    int objectsReported = 0; //use *only* if "objectsExpected" is bound!
    //in error situation: undo communication of processed amount of data
    ScopeGuard guardStatistics = makeGuard([&] { procCallback_.updateProcessedData(-objectsReported, 0); });

    switch (deletionPolicy_)
    {
        case DELETE_PERMANENTLY:
        {
            CallbackRemoveDirImpl remDirCallback(procCallback_, *this, objectsExpected ? &objectsReported : nullptr);
            removeDirectory(fullName, &remDirCallback);
        }
        break;

        case DELETE_TO_RECYCLER:
#ifdef FFS_WIN
            {
                const Zstring targetDir = recyclerTmpDirPf + relativeName;

                auto moveToTempDir = [&]
                {
                    //performance optimization: Instead of moving each object into recycle bin separately,
                    //we rename them one by one into a temporary directory and batch-recycle this directory after sync
                    renameFile(fullName, targetDir); //throw FileError
                    toBeRecycled.push_back(targetDir);
                };

                try
                {
                    moveToTempDir(); //throw FileError
                }
                catch (FileError&)
                {
                    if (somethingExists(fullName))
                    {
                        const Zstring targetSuperDir = beforeLast(targetDir, FILE_NAME_SEPARATOR);
                        if (!dirExists(targetSuperDir))
                        {
                            makeDirectory(targetSuperDir); //throw FileError -> may legitimately fail on Linux if permissions are missing
                            moveToTempDir(); //throw FileError -> this should work now!
                        }
                        else
                            throw;
                    }
                }
            }
#elif defined FFS_LINUX
            recycleOrDelete(fullName); //throw FileError
#endif

            if (objectsExpected) //even though we have only one disk access, we completed "objectsExpected" logical operations!
            {
                procCallback_.updateProcessedData(*objectsExpected, 0);
                objectsReported += *objectsExpected;
            }
            break;

        case DELETE_TO_VERSIONING:
        {
            CallbackMoveFileImpl callback(procCallback_, objectsExpected ? &objectsReported : nullptr);
            getOrCreateVersioner().revisionDir(fullName, relativeName, callback); //throw FileError
        }
        break;
    }

    //inform about the (remaining) processed amount of data
    if (objectsExpected && dataExpected)
    {
        guardStatistics.dismiss();

        if (*objectsExpected != objectsReported || *dataExpected != 0) //adjust total: may have changed after comparison!
            procCallback_.updateTotalData(objectsReported - *objectsExpected, -*dataExpected);
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

    void reportInfo  (const std::wstring& rawText, const Zstring& objname) const { procCallback_.reportInfo  (replaceCpy(rawText, L"%x", fmtFileName(objname))); };
    void reportStatus(const std::wstring& rawText, const Zstring& objname) const { procCallback_.reportStatus(replaceCpy(rawText, L"%x", fmtFileName(objname))); };

    //more low level helper
    template <SelectedSide side, class DelTargetCommand>
    void copyFileUpdatingTo(const FileMapping& fileObj, const DelTargetCommand& cmd, FileAttrib& newAttr) const;
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

    //ensure uniqueness
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

    reportInfo(replaceCpy(txtMovingFile, L"%y", fmtFileName(tmpTarget)), source);

    renameFile(source, tmpTarget); //throw FileError

    //update file hierarchy
    const FileDescriptor descrSource(sourceObj.getLastWriteTime<side>(),
                                     sourceObj.getFileSize     <side>(),
                                     sourceObj.getFileId       <side>());

    sourceObj.removeObject<side>();

    FileMapping& tempFile = sourceObj.root().addSubFile<side>(afterLast(tmpTarget, FILE_NAME_SEPARATOR), descrSource);

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
    //note: this case may include "move sources" from two-step sub-routine!
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
        case SO_MOVE_RIGHT_TARGET: //to be processed in second pass, after "move target" parent directory was created!
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
bool getTargetDirection(SyncOperation syncOp, SelectedSide* side)
{
    switch (syncOp)
    {
        case SO_CREATE_NEW_LEFT:
        case SO_DELETE_LEFT:
        case SO_OVERWRITE_LEFT:
        case SO_COPY_METADATA_TO_LEFT:
        case SO_MOVE_LEFT_SOURCE:
        case SO_MOVE_LEFT_TARGET:
            *side = LEFT_SIDE;
            return true;

        case SO_CREATE_NEW_RIGHT:
        case SO_DELETE_RIGHT:
        case SO_OVERWRITE_RIGHT:
        case SO_COPY_METADATA_TO_RIGHT:
        case SO_MOVE_RIGHT_SOURCE:
        case SO_MOVE_RIGHT_TARGET:
            *side = RIGHT_SIDE;
            return true;

        case SO_DO_NOTHING:
        case SO_EQUAL:
        case SO_UNRESOLVED_CONFLICT:
            break; //nothing to do
    }
    return false;
}
}


inline
void SynchronizeFolderPair::synchronizeFile(FileMapping& fileObj)
{
    const SyncOperation syncOp = fileObj.getSyncOperation();

    SelectedSide sideTrg = LEFT_SIDE;

    if (getTargetDirection(syncOp, &sideTrg))
    {
        if (sideTrg == LEFT_SIDE)
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
            const Zstring& target = fileObj.getBaseDirPf<sideTrg>() + fileObj.getRelativeName<sideSrc>(); //can't use "getFullName" as target is not yet existing

            reportInfo(txtCreatingFile, target);

            try
            {
                FileAttrib newAttr;
                copyFileUpdatingTo<sideTrg>(fileObj, [] {}, /*no target to delete*/ newAttr); //throw FileError
                procCallback_.updateProcessedData(1, 0); //processed data is communicated in copyFileUpdatingTo()!

                const FileDescriptor descrSource(newAttr.modificationTime, newAttr.fileSize, newAttr.sourceFileId);
                const FileDescriptor descrTarget(newAttr.modificationTime, newAttr.fileSize, newAttr.targetFileId);
                fileObj.syncTo<sideTrg>(descrTarget, &descrSource); //update FileMapping
            }
            catch (FileError&)
            {
                if (somethingExists(fileObj.getFullName<sideSrc>())) //do not check on type (symlink, file, folder) -> if there is a type change, FFS should not be quiet about it!
                    throw;
                //source deleted meanwhile...nothing was done (logical point of view!)
                procCallback_.updateTotalData(-1, -to<zen::Int64>(fileObj.getFileSize<sideSrc>()));
                fileObj.removeObject<sideSrc>();
            }
        }
        break;

        case SO_DELETE_LEFT:
        case SO_DELETE_RIGHT:
            reportInfo(getDelHandling<sideTrg>().getTxtRemovingFile(), fileObj.getFullName<sideTrg>());

            getDelHandling<sideTrg>().removeFile(fileObj.getObjRelativeName()); //throw FileError
            fileObj.removeObject<sideTrg>(); //update FileMapping

            procCallback_.updateProcessedData(1, 0);
            break;

        case SO_MOVE_LEFT_SOURCE:
        case SO_MOVE_LEFT_TARGET:
        case SO_MOVE_RIGHT_SOURCE:
        case SO_MOVE_RIGHT_TARGET:
            if (FileMapping* targetObj = dynamic_cast<FileMapping*>(FileSystemObject::retrieve(fileObj.getMoveRef())))
            {
                FileMapping* sourceObj = &fileObj;

                if (syncOp != SO_MOVE_LEFT_SOURCE && syncOp != SO_MOVE_RIGHT_SOURCE)
                    std::swap(sourceObj, targetObj);

                assert((sourceObj->getSyncOperation() == SO_MOVE_LEFT_SOURCE  && targetObj->getSyncOperation() == SO_MOVE_LEFT_TARGET  && sideTrg == LEFT_SIDE) ||
                       (sourceObj->getSyncOperation() == SO_MOVE_RIGHT_SOURCE && targetObj->getSyncOperation() == SO_MOVE_RIGHT_TARGET && sideTrg == RIGHT_SIDE));

                const Zstring& source = sourceObj->getFullName<sideTrg>();
                const Zstring& target = targetObj->getBaseDirPf<sideTrg>() + targetObj->getRelativeName<sideSrc>();

                reportInfo(replaceCpy(txtMovingFile, L"%y", fmtFileName(target)), source);

                renameFile(source, target); //throw FileError

                const FileDescriptor descrTarget(sourceObj->getLastWriteTime<sideTrg>(),
                                                 sourceObj->getFileSize     <sideTrg>(),
                                                 sourceObj->getFileId       <sideTrg>());

                sourceObj->removeObject<sideTrg>();      //
                targetObj->syncTo<sideTrg>(descrTarget); //update FileMapping

                procCallback_.updateProcessedData(1, 0);
            }
            break;

        case SO_OVERWRITE_LEFT:
        case SO_OVERWRITE_RIGHT:
        {
            const Zstring& target = fileObj.getBaseDirPf<sideTrg>() + fileObj.getRelativeName<sideSrc>(); //respect differences in case of source object

            reportInfo(txtOverwritingFile, target);

            FileAttrib newAttr;
            copyFileUpdatingTo<sideTrg>(fileObj, [&] //delete target at appropriate time
            {
                reportStatus(this->getDelHandling<sideTrg>().getTxtRemovingFile(), fileObj.getFullName<sideTrg>());

                this->getDelHandling<sideTrg>().removeFile(fileObj.getObjRelativeName()); //throw FileError
                fileObj.removeObject<sideTrg>(); //remove file from FileMapping, to keep in sync (if subsequent copying fails!!)

                reportStatus(txtOverwritingFile, target); //restore status text copy file
            }, newAttr);

            const FileDescriptor descrSource(newAttr.modificationTime, newAttr.fileSize, newAttr.sourceFileId);
            const FileDescriptor descrTarget(newAttr.modificationTime, newAttr.fileSize, newAttr.targetFileId);
            fileObj.syncTo<sideTrg>(descrTarget, &descrSource); //update FileMapping

            procCallback_.updateProcessedData(1, 0);
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

    SelectedSide sideTrg = LEFT_SIDE;

    if (getTargetDirection(syncOp, &sideTrg))
    {
        if (sideTrg == LEFT_SIDE)
            synchronizeLinkInt<LEFT_SIDE>(linkObj, syncOp);
        else
            synchronizeLinkInt<RIGHT_SIDE>(linkObj, syncOp);
    }
}


template <SelectedSide sideTrg>
void SynchronizeFolderPair::synchronizeLinkInt(SymLinkMapping& linkObj, SyncOperation syncOp)
{
    static const SelectedSide sideSrc = OtherSide<sideTrg>::result;

    auto deleteSymlink = [&]
    {
        switch (linkObj.getLinkType<sideTrg>())
        {
            case LinkDescriptor::TYPE_DIR:
                this->getDelHandling<sideTrg>().removeFolder(linkObj.getObjRelativeName()); //throw FileError
                break;

            case LinkDescriptor::TYPE_FILE: //Windows: true file symlink; Linux: file-link or broken link
                this->getDelHandling<sideTrg>().removeFile(linkObj.getObjRelativeName()); //throw FileError
                break;
        }
    };

    switch (syncOp)
    {
        case SO_CREATE_NEW_LEFT:
        case SO_CREATE_NEW_RIGHT:
        {
            const Zstring& target = linkObj.getBaseDirPf<sideTrg>() + linkObj.getRelativeName<sideSrc>();

            reportInfo(txtCreatingLink, target);

            try
            {
                zen::copySymlink(linkObj.getFullName<sideSrc>(), target, copyFilePermissions_); //throw FileError
                procCallback_.updateProcessedData(1, 0);

                linkObj.copyTo<sideTrg>(); //update SymLinkMapping
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

            deleteSymlink(); //throw FileError

            linkObj.removeObject<sideTrg>(); //update SymLinkMapping

            procCallback_.updateProcessedData(1, 0);
            break;

        case SO_OVERWRITE_LEFT:
        case SO_OVERWRITE_RIGHT:
        {
            const Zstring& target = linkObj.getBaseDirPf<sideTrg>() + linkObj.getRelativeName<sideSrc>(); //respect differences in case of source object

            reportInfo(txtOverwritingLink, target);

            reportStatus(getDelHandling<sideTrg>().getTxtRemovingSymLink(), linkObj.getFullName<sideTrg>());
            deleteSymlink(); //throw FileError
            linkObj.removeObject<sideTrg>(); //remove file from FileMapping, to keep in sync (if subsequent copying fails!!)

            reportStatus(txtOverwritingLink, target); //restore status text
            zen::copySymlink(linkObj.getFullName<sideSrc>(), target, copyFilePermissions_); //throw FileError
            linkObj.copyTo<sideTrg>(); //update SymLinkMapping

            procCallback_.updateProcessedData(1, 0);
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

    SelectedSide sideTrg = LEFT_SIDE;

    if (getTargetDirection(syncOp, &sideTrg))
    {
        if (sideTrg == LEFT_SIDE)
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
            if (somethingExists(dirObj.getFullName<sideSrc>())) //do not check on type (symlink, file, folder) -> if there is a type change, FFS should not be quiet about it!
            {
                const Zstring& target = dirObj.getBaseDirPf<sideTrg>() + dirObj.getRelativeName<sideSrc>();

                reportInfo(txtCreatingFolder, target);
                try
                {
                    makeNewDirectory(target, dirObj.getFullName<sideSrc>(), copyFilePermissions_); //no symlink copying!
                }
                catch (const ErrorTargetExisting&) { if (!dirExists(target)) throw; } //clash with file (dir-symlink is okay)

                dirObj.copyTo<sideTrg>(); //update DirMapping

                procCallback_.updateProcessedData(1, 0);
            }
            else //source deleted meanwhile...nothing was done (logical point of view!) -> uh....what about a temporary network drop???
            {
                // throw FileError
                const SyncStatistics subStats(dirObj);
                procCallback_.updateTotalData(-getCUD(subStats) - 1, -subStats.getDataToProcess());

                dirObj.refSubFiles().clear(); //...then remove sub-objects
                dirObj.refSubLinks().clear(); //
                dirObj.refSubDirs ().clear(); //
                dirObj.removeObject<sideSrc>();
            }
            break;

        case SO_COPY_METADATA_TO_LEFT:
        case SO_COPY_METADATA_TO_RIGHT:
            reportInfo(txtWritingAttributes, dirObj.getFullName<sideTrg>());

            if (dirObj.getShortName<sideTrg>() != dirObj.getShortName<sideSrc>()) //adapt difference in case (windows only)
                renameFile(dirObj.getFullName<sideTrg>(),
                           beforeLast(dirObj.getFullName<sideTrg>(), FILE_NAME_SEPARATOR) + FILE_NAME_SEPARATOR + dirObj.getShortName<sideSrc>()); //throw FileError
            //copyFileTimes -> useless at this time: modification time changes with each child-object creation/deletion

            dirObj.copyTo<sideTrg>(); //-> both sides *should* be completely equal now...

            procCallback_.updateProcessedData(1, 0);
            break;

        case SO_DELETE_LEFT:
        case SO_DELETE_RIGHT:
        {
            reportInfo(getDelHandling<sideTrg>().getTxtRemovingDir(), dirObj.getFullName<sideTrg>());

            const SyncStatistics subStats(dirObj); //counts sub-objects only!
            getDelHandling<sideTrg>().removeFolderUpdateStatistics(dirObj.getObjRelativeName(), 1 + getCUD(subStats), subStats.getDataToProcess()); //throw FileError
            //this call covers progress indicator updates for dir + sub-objects!

            dirObj.refSubFiles().clear();   //...then remove everything
            dirObj.refSubLinks().clear();   //
            dirObj.refSubDirs ().clear();   //
            dirObj.removeObject<sideTrg>(); //
        }
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
    const Zstring dirname = beforeLast(baseMap.getBaseDirPf<side>(), FILE_NAME_SEPARATOR);
    if (!dirname.empty())
    {
        if (baseMap.isExisting<side>()) //atomicity: do NOT check directory existence again!
        {
            //just convenience: exit sync right here instead of showing tons of error messages during file copy
            return tryReportingError([&]
            {
                if (!dirExistsUpdating(dirname, false, callback))
                    throw FileError(replaceCpy(_("Cannot find folder %x."), L"%x", fmtFileName(dirname))); //this should really be a "fatal error" if not recoverable
            }, callback); //may throw in error-callback!
        }
        else //create target directory: user presumably ignored error "dir existing" in order to have it created automatically
        {
            bool temporaryNetworkDrop = false;
            bool rv = tryReportingError([&]
            {
                try
                {
                    makeNewDirectory(dirname, Zstring(), false); //FileError, ErrorTargetExisting
                    //a nice race-free check and set operation!
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
    return true;
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
        backgroundPrio.reset(new ScheduleForBackgroundProcessing);

    //prevent operating system going into sleep state
    PreventStandby dummy;
    (void)dummy;

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
                                 folderIndex,
                                 j->getBaseDirPf<LEFT_SIDE>(),
                                 callback);

        delHandlerR.emplace_back(folderPairCfg.handleDeletion,
                                 folderPairCfg.versioningFolder,
                                 folderPairCfg.versioningStyle_,
                                 timeStamp,
                                 folderIndex,
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
                const Zstring dirname = beforeLast(baseDirPf, FILE_NAME_SEPARATOR);
                if (!dirname.empty())
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
        std::wstring warningMessage = _("The following items have unresolved conflicts and will not be synchronized:") + L"\n\n";

        const auto& conflictMsgs = statisticsTotal.getConflictMessages(); //get *all* sync conflicts
        for (auto it = conflictMsgs.begin(); it != conflictMsgs.end(); ++it)
            warningMessage += fmtFileName(it->first) + L": " + it->second + L"\n\n";

        callback.reportWarning(warningMessage, warnings.warningUnresolvedConflicts);
    }


    //check if user accidentally selected wrong directories for sync
    if (!significantDiff.empty())
    {
        std::wstring warningMessage = _("Significant difference detected:");

        for (auto it = significantDiff.begin(); it != significantDiff.end(); ++it)
            warningMessage += std::wstring(L"\n\n") +
                              it->first + L" <-> " + L"\n" +
                              it->second;
        warningMessage += L"\n\n";
        warningMessage += _("More than 50% of the total number of files will be copied or deleted!");

        callback.reportWarning(warningMessage, warnings.warningSignificantDifference);
    }


    //check for sufficient free diskspace
    if (!diskSpaceMissing.empty())
    {
        std::wstring warningMessage = _("Not enough free disk space available in:");

        for (auto i = diskSpaceMissing.begin(); i != diskSpaceMissing.end(); ++i)
            warningMessage += std::wstring(L"\n\n") +
                              fmtFileName(i->first) + L"\n" +
                              _("Required:")  + L" " + filesizeToShortString(i->second.first)  + L"\n" +
                              _("Available:") + L" " + filesizeToShortString(i->second.second);

        callback.reportWarning(warningMessage, warnings.warningNotEnoughDiskSpace);
    }


    //windows: check if recycle bin really exists; if not, Windows will silently delete, which is wrong
#ifdef FFS_WIN
    if (!recyclMissing.empty())
    {
        std::wstring warningMessage = _("Recycle Bin is not available for the following paths! Files will be deleted permanently instead:");
        warningMessage += L"\n";

        std::for_each(recyclMissing.begin(), recyclMissing.end(),
        [&](const Zstring& path) { warningMessage += L"\n" + utfCvrtTo<std::wstring>(path); });

        callback.reportWarning(warningMessage, warnings.warningRecyclerMissing);
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
        std::wstring warningMessage = _("A folder will be modified which is part of multiple folder pairs. Please review synchronization settings.") + L"\n";
        for (auto i = conflictDirs.begin(); i != conflictDirs.end(); ++i)
            warningMessage += L"\n" + fmtFileName(*i);
        callback.reportWarning(warningMessage, warnings.warningFolderPairRaceCondition);
    }

    //-------------------end of basic checks------------------------------------------

#ifdef FFS_WIN
    //shadow copy buffer: per sync-instance, not folder pair
    std::unique_ptr<shadow::ShadowCopy> shadowCopyHandler(copyLockedFiles ? new shadow::ShadowCopy : nullptr);
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
                                L"    " + left  + fmtFileName(j->getBaseDirPf<LEFT_SIDE >()) + L"\n" +
                                L"    " + right + fmtFileName(j->getBaseDirPf<RIGHT_SIDE>()));
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
//callback functionality for smooth progress indicators

template <class DelTargetCommand>
class WhileCopying : public zen::CallbackCopyFile
{
public:
    WhileCopying(Int64& bytesReported,
                 ProcessCallback& statusHandler,
                 const DelTargetCommand& cmd) :
        bytesReported_(bytesReported),
        statusHandler_(statusHandler),
        cmd_(cmd) {}

    virtual void deleteTargetFile(const Zstring& targetFile) { cmd_(); }

    virtual void updateCopyStatus(Int64 bytesDelta)
    {
        //inform about the (differential) processed amount of data
        statusHandler_.updateProcessedData(0, bytesDelta); //throw()! -> this ensures client and service provider are in sync!
        bytesReported_ += bytesDelta;                      //

        statusHandler_.requestUiRefresh(); //may throw
    }

private:
    Int64&          bytesReported_;
    ProcessCallback& statusHandler_;
    DelTargetCommand cmd_;
};


//copy file while refreshing UI
template <SelectedSide side, class DelTargetCommand>
void SynchronizeFolderPair::copyFileUpdatingTo(const FileMapping& fileObj, const DelTargetCommand& cmd, FileAttrib& newAttr) const
{
    const Int64 expectedBytesToCpy = to<Int64>(fileObj.getFileSize<OtherSide<side>::result>());
    Zstring source = fileObj.getFullName<OtherSide<side>::result>();
    const Zstring& target = fileObj.getBaseDirPf<side>() + fileObj.getRelativeName<OtherSide<side>::result>();

    Int64 bytesReported;

    auto copyOperation = [&]
    {
        //start of (possibly) long-running copy process: ensure status updates are performed regularly

        //in error situation: undo communication of processed amount of data
        auto guardStatistics = makeGuard([&]
        {
            procCallback_.updateProcessedData(0, -bytesReported);
            bytesReported = 0;
        });

        WhileCopying<DelTargetCommand> callback(bytesReported, procCallback_, cmd);

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

        //update statistics to consider the real amount of data, e.g. more than the "file size" for ADS streams or file changed in the meantime!
        if (bytesReported != expectedBytesToCpy)
            procCallback_.updateTotalData(0, bytesReported - expectedBytesToCpy);

        //we model physical statistic numbers => adjust total: consider ADS, sparse, compressed files -> transferred bytes may differ from file size (which is just a rough guess)!

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
#elif defined FFS_LINUX
    wxFile file1(::open(source.c_str(), O_RDONLY)); //utilize UTF-8 filename
#endif
    if (!file1.IsOpened())
        throw FileError(replaceCpy(_("Cannot read file %x."), L"%x", fmtFileName(source)));

#ifdef FFS_WIN
    wxFile file2(applyLongPathPrefix(target).c_str(), wxFile::read); //don't use buffered file input for verification!
#elif defined FFS_LINUX
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
