// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl-3.0        *
// * Copyright (C) Zenju (zenju AT gmx DOT de) - All Rights Reserved        *
// **************************************************************************

#include "parallel_scan.h"
#include <zen/file_error.h>
#include <zen/thread.h>
#include <zen/scope_guard.h>
#include <zen/fixed_list.h>
#include "db_file.h"
#include "lock_holder.h"

using namespace zen;


namespace
{
/*
#ifdef ZEN_WIN

struct DiskInfo
{
    DiskInfo() :
        driveType(DRIVE_UNKNOWN),
        diskID(-1) {}

    UINT driveType;
    int diskID; // -1 if id could not be determined, this one is filled if driveType == DRIVE_FIXED or DRIVE_REMOVABLE;
};

inline
bool operator<(const DiskInfo& lhs, const DiskInfo& rhs)
{
    if (lhs.driveType != rhs.driveType)
        return lhs.driveType < rhs.driveType;

    if (lhs.diskID < 0 || rhs.diskID < 0)
        return false;
    //consider "same", reason: one volume may be uniquely associated with one disk, while the other volume is associated to the same disk AND another one!
    //volume <-> disk is 0..N:1..N

    return lhs.diskID < rhs.diskID ;
}


DiskInfo retrieveDiskInfo(const Zstring& pathName)
{
    std::vector<wchar_t> volName(std::max(pathName.size(), static_cast<size_t>(10000)));

    DiskInfo output;

    //full pathName need not yet exist!
    if (!::GetVolumePathName(pathName.c_str(),  //__in   LPCTSTR lpszFileName,
                             &volName[0],       //__out  LPTSTR lpszVolumePathName,
                             static_cast<DWORD>(volName.size()))) //__in   DWORD cchBufferLength
        return output;

    const Zstring rootPathName = &volName[0];

    output.driveType = ::GetDriveType(rootPathName.c_str());

    if (output.driveType == DRIVE_NO_ROOT_DIR) //these two should be the same error category
        output.driveType = DRIVE_UNKNOWN;

    if (output.driveType != DRIVE_FIXED && output.driveType != DRIVE_REMOVABLE)
        return output; //no reason to get disk ID

    //go and find disk id:

    //format into form: "\\.\C:"
    Zstring volnameFmt = rootPathName;
    if (endsWith(volnameFmt, FILE_NAME_SEPARATOR))
        volnameFmt.resize(volnameFmt.size() - 1);
    volnameFmt = L"\\\\.\\" + volnameFmt;

    HANDLE hVolume = ::CreateFile(volnameFmt.c_str(),
                                  0,
                                  FILE_SHARE_READ | FILE_SHARE_WRITE,
                                  nullptr,
                                  OPEN_EXISTING,
                                  0,
                                  nullptr);
    if (hVolume == INVALID_HANDLE_VALUE)
        return output;
    ZEN_ON_SCOPE_EXIT(::CloseHandle(hVolume));

    std::vector<char> buffer(sizeof(VOLUME_DISK_EXTENTS) + sizeof(DISK_EXTENT)); //reserve buffer for at most one disk! call below will then fail if volume spans multiple disks!

    DWORD bytesReturned = 0;
    if (!::DeviceIoControl(hVolume,                              //_In_         HANDLE hDevice,
                           IOCTL_VOLUME_GET_VOLUME_DISK_EXTENTS, //_In_         DWORD dwIoControlCode,
                           nullptr,                              //_In_opt_     LPVOID lpInBuffer,
                           0,                                    //_In_         DWORD nInBufferSize,
                           &buffer[0],                           //_Out_opt_    LPVOID lpOutBuffer,
                           static_cast<DWORD>(buffer.size()),    //_In_         DWORD nOutBufferSize,
                           &bytesReturned,                       //_Out_opt_    LPDWORD lpBytesReturned
                           nullptr))                             //_Inout_opt_  LPOVERLAPPED lpOverlapped
        return output;

    const VOLUME_DISK_EXTENTS& volDisks = *reinterpret_cast<VOLUME_DISK_EXTENTS*>(&buffer[0]);

    if (volDisks.NumberOfDiskExtents != 1)
        return output;

    output.diskID = volDisks.Extents[0].DiskNumber;

    return output;
}
#endif
*/

/*
PERF NOTE

--------------------------------------------
|Testcase: Reading from two different disks|
--------------------------------------------
Windows 7:
            1st(unbuffered) |2nd (OS buffered)
			----------------------------------
1 Thread:          57s      |        8s
2 Threads:         39s      |        7s

--------------------------------------------------
|Testcase: Reading two directories from same disk|
--------------------------------------------------
Windows 7:                                        Windows XP:
            1st(unbuffered) |2nd (OS buffered)                   1st(unbuffered) |2nd (OS buffered)
			----------------------------------                   ----------------------------------
1 Thread:          41s      |        13s             1 Thread:          45s      |        13s
2 Threads:         42s      |        11s             2 Threads:         38s      |         8s

=> Traversing does not take any advantage of file locality so that even multiple threads operating on the same disk impose no performance overhead! (even faster on XP)

std::vector<std::set<DirectoryKey>> separateByDistinctDisk(const std::set<DirectoryKey>& dirkeys)
{
    //use one thread per physical disk:
    typedef std::map<DiskInfo, std::set<DirectoryKey>> DiskKeyMapping;
    DiskKeyMapping tmp;
    std::for_each(dirkeys.begin(), dirkeys.end(),
    [&](const DirectoryKey& key) { tmp[retrieveDiskInfo(key.dirpathFull_)].insert(key); });

    std::vector<std::set<DirectoryKey>> buckets;
    std::transform(tmp.begin(), tmp.end(), std::back_inserter(buckets),
    [&](const DiskKeyMapping::value_type& diskToKey) { return diskToKey.second; });
    return buckets;
}
*/

//------------------------------------------------------------------------------------------
typedef Zbase<wchar_t, StorageRefCountThreadSafe> BasicWString; //thread-safe string class for UI texts


class AsyncCallback //actor pattern
{
public:
    //blocking call: context of worker thread
    FillBufferCallback::HandleError reportError(const std::wstring& msg, size_t retryNumber) //throw ThreadInterruption
    {
        std::unique_lock<std::mutex> dummy(lockErrorInfo);
        interruptibleWait(conditionCanReportError, dummy, [this] { return !errorInfo && !errorResponse; }); //throw ThreadInterruption

        errorInfo = make_unique<std::pair<BasicWString, size_t>>(copyStringTo<BasicWString>(msg), retryNumber);

        interruptibleWait(conditionGotResponse, dummy, [this] { return static_cast<bool>(errorResponse); }); //throw ThreadInterruption

        FillBufferCallback::HandleError rv = *errorResponse;

        errorInfo    .reset();
        errorResponse.reset();

        dummy.unlock(); //optimization for condition_variable::notify_all()
        conditionCanReportError.notify_all(); //instead of notify_one(); workaround bug: https://svn.boost.org/trac/boost/ticket/7796

        return rv;
    }

    void processErrors(FillBufferCallback& callback) //context of main thread, call repreatedly
    {
        std::unique_lock<std::mutex> dummy(lockErrorInfo);
        if (errorInfo.get() && !errorResponse.get())
        {
            FillBufferCallback::HandleError rv = callback.reportError(copyStringTo<std::wstring>(errorInfo->first), errorInfo->second); //throw!
            errorResponse = make_unique<FillBufferCallback::HandleError>(rv);

            dummy.unlock(); //optimization for condition_variable::notify_all()
            conditionGotResponse.notify_all(); //instead of notify_one(); workaround bug: https://svn.boost.org/trac/boost/ticket/7796
        }
    }

    void incrementNotifyingThreadId() { ++notifyingThreadID; } //context of main thread

    //perf optimization?:
    bool mayReportCurrentFile(int threadID) const { return threadID == notifyingThreadID; }

    void reportCurrentFile(const std::wstring& filepath, int threadID) //context of worker thread
    {
        if (threadID != notifyingThreadID) return; //only one thread at a time may report status

        std::lock_guard<std::mutex> dummy(lockCurrentStatus);
        currentFile = copyStringTo<BasicWString>(filepath);
    }

    std::wstring getCurrentStatus() //context of main thread, call repreatedly
    {
        std::wstring filepath;
        {
            std::lock_guard<std::mutex> dummy(lockCurrentStatus);
            filepath = copyStringTo<std::wstring>(currentFile);
        }

        if (filepath.empty())
            return std::wstring();

        std::wstring statusText = copyStringTo<std::wstring>(textScanning);

        const long activeCount = activeWorker;
        if (activeCount >= 2)
            statusText += L" [" + replaceCpy(_P("1 thread", "%x threads", activeCount), L"%x", numberTo<std::wstring>(activeCount)) + L"]";

        statusText += L" ";
        statusText += filepath;
        return statusText;
    }

    void incItemsScanned() { ++itemsScanned; } //perf: irrelevant! scanning is almost entirely file I/O bound, not CPU bound! => no prob having multiple threads poking at the same variable!
    long getItemsScanned() const { return itemsScanned; }

    void incActiveWorker() { ++activeWorker; }
    void decActiveWorker() { --activeWorker; }
    long getActiveWorker() const { return activeWorker; }

private:
    //---- error handling ----
    std::mutex lockErrorInfo;
    std::condition_variable conditionCanReportError;
    std::condition_variable conditionGotResponse;
    std::unique_ptr<std::pair<BasicWString, size_t>> errorInfo; //error message + retry number
    std::unique_ptr<FillBufferCallback::HandleError> errorResponse;

    //---- status updates ----
    std::atomic<int> notifyingThreadID { 0 }; //CAVEAT: do NOT use boost::thread::id: https://svn.boost.org/trac/boost/ticket/5754

    std::mutex lockCurrentStatus; //use a different lock for current file: continue traversing while some thread may process an error
    BasicWString currentFile;

    const BasicWString textScanning { copyStringTo<BasicWString>(_("Scanning:")) }; //this one is (currently) not shared and could be made a std::wstring, but we stay consistent and use thread-safe variables in this class only!

    //---- status updates II (lock free) ----
    std::atomic<int> itemsScanned{ 0 }; //std:atomic is uninitialized by default!
    std::atomic<int> activeWorker{ 0 }; //
};

//-------------------------------------------------------------------------------------------------

struct TraverserShared
{
public:
    TraverserShared(int threadID,
                    const ABF& abf,
                    const HardFilter::FilterRef& filter,
                    SymLinkHandling handleSymlinks,
                    std::map<Zstring, std::wstring, LessFilePath>& failedDirReads,
                    std::map<Zstring, std::wstring, LessFilePath>& failedItemReads,
                    AsyncCallback& acb) :
        abstractBaseFolder(abf),
        filterInstance(filter),
        handleSymlinks_(handleSymlinks),
        failedDirReads_ (failedDirReads),
        failedItemReads_(failedItemReads),
        acb_(acb),
        threadID_(threadID) {}

    const ABF& abstractBaseFolder;
    const HardFilter::FilterRef filterInstance; //always bound!
    const SymLinkHandling handleSymlinks_;

    std::map<Zstring, std::wstring, LessFilePath>& failedDirReads_;
    std::map<Zstring, std::wstring, LessFilePath>& failedItemReads_;

    AsyncCallback& acb_;
    const int threadID_;
};


class DirCallback : public ABF::TraverserCallback
{
public:
    DirCallback(TraverserShared& config,
                const Zstring& relNameParentPf, //postfixed with FILE_NAME_SEPARATOR!
                DirContainer& output) :
        cfg(config),
        relNameParentPf_(relNameParentPf),
        output_(output) {}

    virtual void                               onFile   (const FileInfo&    fi) override; //
    virtual std::unique_ptr<TraverserCallback> onDir    (const DirInfo&     di) override; //throw ThreadInterruption
    virtual HandleLink                         onSymlink(const SymlinkInfo& li) override; //

    HandleError reportDirError (const std::wstring& msg, size_t retryNumber)                           override; //throw ThreadInterruption
    HandleError reportItemError(const std::wstring& msg, size_t retryNumber, const Zchar* shortName)   override; //

private:
    TraverserShared& cfg;
    const Zstring relNameParentPf_;
    DirContainer& output_;
};


void DirCallback::onFile(const FileInfo& fi) //throw ThreadInterruption
{
    interruptionPoint(); //throw ThreadInterruption

    const Zstring fileNameShort(fi.shortName);

    //do not list the database file(s) sync.ffs_db, sync.x64.ffs_db, etc. or lock files
    if (endsWith(fileNameShort, SYNC_DB_FILE_ENDING) ||
        endsWith(fileNameShort, LOCK_FILE_ENDING))
        return;

    const Zstring relFilePath = relNameParentPf_ + fileNameShort;

    //update status information no matter whether object is excluded or not!
    if (cfg.acb_.mayReportCurrentFile(cfg.threadID_))
        cfg.acb_.reportCurrentFile(ABF::getDisplayPath(cfg.abstractBaseFolder.getAbstractPath(relFilePath)), cfg.threadID_);

    //------------------------------------------------------------------------------------
    //apply filter before processing (use relative name!)
    if (!cfg.filterInstance->passFileFilter(relFilePath))
        return;

    //    std::string fileId = details.fileSize >=  1024 * 1024U ? util::retrieveFileID(filepath) : std::string();
    /*
    Perf test Windows 7, SSD, 350k files, 50k dirs, files > 1MB: 7000
    	regular:            6.9s
    	ID per file:       43.9s
    	ID per file > 1MB:  7.2s
    	ID per dir:         8.4s

    	Linux: retrieveFileID takes about 50% longer in VM! (avoidable because of redundant stat() call!)
    */

    output_.addSubFile(fileNameShort, FileDescriptor(fi.lastWriteTime, fi.fileSize, fi.id, fi.symlinkInfo != nullptr));

    cfg.acb_.incItemsScanned(); //add 1 element to the progress indicator
}


std::unique_ptr<ABF::TraverserCallback> DirCallback::onDir(const DirInfo& di) //throw ThreadInterruption
{
    interruptionPoint(); //throw ThreadInterruption

    const Zstring& relDirPath = relNameParentPf_ + di.shortName;

    //update status information no matter whether object is excluded or not!
    if (cfg.acb_.mayReportCurrentFile(cfg.threadID_))
        cfg.acb_.reportCurrentFile(ABF::getDisplayPath(cfg.abstractBaseFolder.getAbstractPath(relDirPath)), cfg.threadID_);

    //------------------------------------------------------------------------------------
    //apply filter before processing (use relative name!)
    bool subObjMightMatch = true;
    const bool passFilter = cfg.filterInstance->passDirFilter(relDirPath, &subObjMightMatch);
    if (!passFilter && !subObjMightMatch)
        return nullptr; //do NOT traverse subdirs
    //else: attention! ensure directory filtering is applied later to exclude actually filtered directories

    DirContainer& subDir = output_.addSubDir(di.shortName);
    if (passFilter)
        cfg.acb_.incItemsScanned(); //add 1 element to the progress indicator

    return make_unique<DirCallback>(cfg, relDirPath + FILE_NAME_SEPARATOR, subDir); //releaseDirTraverser() is guaranteed to be called in any case
}


DirCallback::HandleLink DirCallback::onSymlink(const SymlinkInfo& si) //throw ThreadInterruption
{
    interruptionPoint(); //throw ThreadInterruption

    const Zstring& relLinkPath = relNameParentPf_ + si.shortName;

    //update status information no matter whether object is excluded or not!
    if (cfg.acb_.mayReportCurrentFile(cfg.threadID_))
        cfg.acb_.reportCurrentFile(ABF::getDisplayPath(cfg.abstractBaseFolder.getAbstractPath(relLinkPath)), cfg.threadID_);

    switch (cfg.handleSymlinks_)
    {
        case SYMLINK_EXCLUDE:
            return LINK_SKIP;

        case SYMLINK_DIRECT:
            if (cfg.filterInstance->passFileFilter(relLinkPath)) //always use file filter: Link type may not be "stable" on Linux!
            {
                output_.addSubLink(si.shortName, LinkDescriptor(si.lastWriteTime));
                cfg.acb_.incItemsScanned(); //add 1 element to the progress indicator
            }
            return LINK_SKIP;

        case SYMLINK_FOLLOW:
            //filter symlinks before trying to follow them: handle user-excluded broken symlinks!
            //since we don't know what type the symlink will resolve to, only do this when both variants agree:
            if (!cfg.filterInstance->passFileFilter(relLinkPath))
            {
                bool subObjMightMatch = true;
                if (!cfg.filterInstance->passDirFilter(relLinkPath, &subObjMightMatch))
                    if (!subObjMightMatch)
                        return LINK_SKIP;
            }
            return LINK_FOLLOW;
    }

    assert(false);
    return LINK_SKIP;
}


DirCallback::HandleError DirCallback::reportDirError(const std::wstring& msg, size_t retryNumber) //throw ThreadInterruption
{
    switch (cfg.acb_.reportError(msg, retryNumber)) //throw ThreadInterruption
    {
        case FillBufferCallback::ON_ERROR_IGNORE:
            cfg.failedDirReads_[beforeLast(relNameParentPf_, FILE_NAME_SEPARATOR, IF_MISSING_RETURN_NONE)] = msg;
            return ON_ERROR_IGNORE;

        case FillBufferCallback::ON_ERROR_RETRY:
            return ON_ERROR_RETRY;
    }
    assert(false);
    return ON_ERROR_IGNORE;
}


DirCallback::HandleError DirCallback::reportItemError(const std::wstring& msg, size_t retryNumber, const Zchar* shortName) //throw ThreadInterruption
{
    switch (cfg.acb_.reportError(msg, retryNumber)) //throw ThreadInterruption
    {
        case FillBufferCallback::ON_ERROR_IGNORE:
            cfg.failedItemReads_[relNameParentPf_ + shortName] =  msg;
            return ON_ERROR_IGNORE;

        case FillBufferCallback::ON_ERROR_RETRY:
            return ON_ERROR_RETRY;
    }
    assert(false);
    return ON_ERROR_IGNORE;
}

//------------------------------------------------------------------------------------------

class WorkerThread
{
public:
    WorkerThread(int threadID,
                 const std::shared_ptr<AsyncCallback>& acb,
                 const DirectoryKey& dirKey,
                 DirectoryValue& dirOutput) :
        threadID_(threadID),
        acb_(acb),
        dirKey_(dirKey),
        dirOutput_(dirOutput) {}

    void operator()() const //thread entry
    {
        acb_->incActiveWorker();
        ZEN_ON_SCOPE_EXIT(acb_->decActiveWorker(););

        const AbstractPathRef& baseFolderItem = dirKey_.baseFolder_->getAbstractPath();

        if (acb_->mayReportCurrentFile(threadID_))
            acb_->reportCurrentFile(ABF::getDisplayPath(baseFolderItem), threadID_); //just in case first directory access is blocking

        TraverserShared travCfg(threadID_,
                                *dirKey_.baseFolder_,
                                dirKey_.filter_,
                                dirKey_.handleSymlinks_, //shared by all(!) instances of DirCallback while traversing a folder hierarchy
                                dirOutput_.failedDirReads,
                                dirOutput_.failedItemReads,
                                *acb_);

        DirCallback traverser(travCfg,
                              Zstring(),
                              dirOutput_.dirCont);

        ABF::traverseFolder(baseFolderItem, traverser); //throw X
    }

private:
    const int threadID_;
    std::shared_ptr<AsyncCallback> acb_;
    const DirectoryKey dirKey_;
    DirectoryValue& dirOutput_;
};
}


void zen::fillBuffer(const std::set<DirectoryKey>& keysToRead, //in
                     std::map<DirectoryKey, DirectoryValue>& buf, //out
                     FillBufferCallback& callback,
                     size_t updateInterval)
{
    buf.clear();

    FixedList<InterruptibleThread> worker;

    zen::ScopeGuard guardWorker = zen::makeGuard([&]
    {
        for (InterruptibleThread& wt : worker)
            wt.interrupt(); //interrupt all at once first, then join
        for (InterruptibleThread& wt : worker)
            if (wt.joinable()) //= precondition of thread::join(), which throws an exception if violated!
                wt.join();     //in this context it is possible a thread is *not* joinable anymore due to the thread::try_join_for() below!
    });

    auto acb = std::make_shared<AsyncCallback>();

    //init worker threads
    for (const DirectoryKey& key : keysToRead)
    {
        assert(buf.find(key) == buf.end());
        DirectoryValue& dirOutput = buf[key];

        const int threadId = static_cast<int>(worker.size());
        worker.emplace_back(WorkerThread(threadId, acb, key, dirOutput));
    }

    //wait until done
    for (InterruptibleThread& wt : worker)
    {
        do
        {
            //update status
            callback.reportStatus(acb->getCurrentStatus(), acb->getItemsScanned()); //throw!

            //process errors
            acb->processErrors(callback);
        }
        while (!wt.tryJoinFor(std::chrono::milliseconds(updateInterval)));

        acb->incrementNotifyingThreadId(); //process info messages of one thread at a time only
    }

    guardWorker.dismiss();
}
