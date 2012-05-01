// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) ZenJu (zhnmju123 AT gmx DOT de) - All Rights Reserved    *
// **************************************************************************

#include "parallel_scan.h"
#include <boost/detail/atomic_count.hpp>
#include "db_file.h"
#include "lock_holder.h"
#include <zen/file_traverser.h>
#include <zen/file_error.h>
#include <wx+/string_conv.h>
#include <zen/thread.h> //includes <boost/thread.hpp>
#include <zen/scope_guard.h>

using namespace zen;


#ifndef BOOST_HAS_THREADS
#error just some paranoia check...
#endif


namespace
{
/*
#ifdef FFS_WIN

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
    if (!::DeviceIoControl(hVolume,                              // handle to device
                           IOCTL_VOLUME_GET_VOLUME_DISK_EXTENTS, // dwIoControlCode
                           nullptr,                                 // lpInBuffer
                           0,                                    // nInBufferSize
                           &buffer[0],                           // output buffer
                           static_cast<DWORD>(buffer.size()),    // size of output buffer
                           &bytesReturned,                       // number of bytes returned
                           nullptr))                                // OVERLAPPED structure
        return output;

    const VOLUME_DISK_EXTENTS& volDisks = *reinterpret_cast<VOLUME_DISK_EXTENTS*>(&buffer[0]);

    if (volDisks.NumberOfDiskExtents != 1)
        return output;

    output.diskID = volDisks.Extents[0].DiskNumber;

    return output;
}

#elif defined FFS_LINUX
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
*/


std::vector<std::set<DirectoryKey>> separateByDistinctDisk(const std::set<DirectoryKey>& dirkeys)
{
    //see perf note: use one thread per dirkey:
    typedef std::map<int, std::set<DirectoryKey>> DiskKeyMapping;
    DiskKeyMapping tmp;
    int index = 0;
    std::for_each(dirkeys.begin(), dirkeys.end(),
    [&](const DirectoryKey& key) { tmp[++index].insert(key); });

    /*
    	//use one thread per physical disk:
    	typedef std::map<DiskInfo, std::set<DirectoryKey>> DiskKeyMapping;
        DiskKeyMapping tmp;
        std::for_each(dirkeys.begin(), dirkeys.end(),
        [&](const DirectoryKey& key) { tmp[retrieveDiskInfo(key.dirnameFull_)].insert(key); });
    */
    std::vector<std::set<DirectoryKey>> buckets;
    std::transform(tmp.begin(), tmp.end(), std::back_inserter(buckets),
    [&](const DiskKeyMapping::value_type& diskToKey) { return diskToKey.second; });
    return buckets;
}

//------------------------------------------------------------------------------------------
typedef Zbase<wchar_t, StorageRefCountThreadSafe> BasicWString; //thread safe string class for UI texts


class AsyncCallback //actor pattern
{
public:
    AsyncCallback() :
        notifyingThreadID(-1),
        textScanning(_("Scanning:")),
        itemsScanned(0),
        activeWorker(0) {}

    FillBufferCallback::HandleError reportError(const std::wstring& msg) //blocking call: context of worker thread
    {
        boost::unique_lock<boost::mutex> dummy(lockErrorMsg);
        while (!errorMsg.empty() || errorResponse.get())
            conditionCanReportError.timed_wait(dummy, boost::posix_time::milliseconds(50)); //interruption point!

        errorMsg = BasicWString(msg);

        while (!errorResponse.get())
            conditionGotResponse.timed_wait(dummy, boost::posix_time::milliseconds(50)); //interruption point!

        FillBufferCallback::HandleError rv = *errorResponse;

        errorMsg.clear();
        errorResponse.reset();

        dummy.unlock(); //optimization for condition_variable::notify_one()
        conditionCanReportError.notify_one();

        return rv;
    }

    void processErrors(FillBufferCallback& callback) //context of main thread, call repreatedly
    {
        boost::unique_lock<boost::mutex> dummy(lockErrorMsg);
        if (!errorMsg.empty() && !errorResponse.get())
        {
            FillBufferCallback::HandleError rv = callback.reportError(copyStringTo<std::wstring>(errorMsg)); //throw!
            errorResponse.reset(new FillBufferCallback::HandleError(rv));

            dummy.unlock(); //optimization for condition_variable::notify_one()
            conditionGotResponse.notify_one();
        }
    }

    void setNotifyingThread(int threadID) { notifyingThreadID = threadID; } //context of main thread

    void reportCurrentFile(const Zstring& filename, int threadID) //context of worker thread
    {
        if (threadID != notifyingThreadID) return; //only one thread may report status

        boost::lock_guard<boost::mutex> dummy(lockCurrentStatus);
        currentFile = filename;
        currentStatus.clear();
    }

    void reportCurrentStatus(const std::wstring& status, int threadID) //context of worker thread
    {
        if (threadID != notifyingThreadID) return; //only one thread may report status

        boost::lock_guard<boost::mutex> dummy(lockCurrentStatus);
        currentFile.clear();
        currentStatus = BasicWString(status); //we cannot assume std::wstring to be thread safe (yet)!
    }

    std::wstring getCurrentStatus() //context of main thread, call repreatedly
    {
        Zstring filename;
        std::wstring statusMsg;
        {
            boost::lock_guard<boost::mutex> dummy(lockCurrentStatus);
            if (!currentFile.empty())
                filename = currentFile;
            else if (!currentStatus.empty())
                statusMsg = copyStringTo<std::wstring>(currentStatus);
        }

        if (!filename.empty())
        {
            std::wstring statusText = copyStringTo<std::wstring>(textScanning);
            const long activeCount = activeWorker;
            if (activeCount >= 2)
                statusText += L" " + replaceCpy(_P("[1 Thread]", "[%x Threads]", activeCount), L"%x", numberTo<std::wstring>(activeCount));

            statusText += L" " + fmtFileName(filename);
            return statusText;
        }
        else
            return statusMsg;
    }

    void incItemsScanned() { ++itemsScanned; }
    long getItemsScanned() const { return itemsScanned; }

    void incActiveWorker() { ++activeWorker; }
    void decActiveWorker() { --activeWorker; }
    long getActiveWorker() const { return activeWorker; }

private:
    //---- error handling ----
    boost::mutex lockErrorMsg;
    boost::condition_variable conditionCanReportError;
    boost::condition_variable conditionGotResponse;
    BasicWString errorMsg;
    std::unique_ptr<FillBufferCallback::HandleError> errorResponse;

    //---- status updates ----
    volatile int notifyingThreadID; //theoretically racy, but there is nothing that could go wrong...
    //CAVEAT: do NOT use boost::thread::id as long as this showstopper exists: https://svn.boost.org/trac/boost/ticket/5754
    boost::mutex lockCurrentStatus; //use a different lock for current file: continue traversing while some thread may process an error
    Zstring currentFile;        //only one of these two is filled at a time!
    BasicWString currentStatus; //

    const BasicWString textScanning; //this one is (currently) not shared and could be made a std::wstring, but we stay consistent and use thread-safe variables in this class only!

    //---- status updates II (lock free) ----
    boost::detail::atomic_count itemsScanned;
    boost::detail::atomic_count activeWorker;
};
//-------------------------------------------------------------------------------------------------


struct TraverserShared
{
public:
    TraverserShared(int threadID,
                    SymLinkHandling handleSymlinks,
                    const HardFilter::FilterRef& filter,
                    std::set<Zstring>& failedReads,
                    AsyncCallback& acb) :
        handleSymlinks_(handleSymlinks),
        filterInstance(filter),
        failedReads_(failedReads),
        acb_(acb),
        threadID_(threadID) {}

    const SymLinkHandling handleSymlinks_;
    const HardFilter::FilterRef filterInstance; //always bound!

    std::set<Zstring>& failedReads_; //relative postfixed names of directories that could not be read (empty for root)

    AsyncCallback& acb_;
    int threadID_;
};


class DirCallback : public zen::TraverseCallback
{
public:
    DirCallback(TraverserShared& config,
                const Zstring& relNameParentPf, //postfixed with FILE_NAME_SEPARATOR!
                DirContainer& output) :
        cfg(config),
        relNameParentPf_(relNameParentPf),
        output_(output) {}

    virtual std::shared_ptr<TraverseCallback>
    onDir    (const Zchar* shortName, const Zstring& fullName);
    virtual void        onFile   (const Zchar* shortName, const Zstring& fullName, const FileInfo& details);
    virtual void        onSymlink(const Zchar* shortName, const Zstring& fullName, const SymlinkInfo& details);
    virtual HandleError onError  (const std::wstring& errorText);

private:
    TraverserShared& cfg;
    const Zstring relNameParentPf_;
    DirContainer& output_;
};


void DirCallback::onFile(const Zchar* shortName, const Zstring& fullName, const FileInfo& details)
{
    boost::this_thread::interruption_point();

    const Zstring fileNameShort(shortName);

    //do not list the database file(s) sync.ffs_db, sync.x64.ffs_db, etc. or lock files
    if (endsWith(fileNameShort, SYNC_DB_FILE_ENDING) ||
        endsWith(fileNameShort, LOCK_FILE_ENDING))
        return;

    //update status information no matter whether object is excluded or not!
    cfg.acb_.reportCurrentFile(fullName, cfg.threadID_);

    //------------------------------------------------------------------------------------
    //apply filter before processing (use relative name!)
    if (!cfg.filterInstance->passFileFilter(relNameParentPf_ + fileNameShort))
        return;

    //    std::string fileId = details.fileSize >=  1024 * 1024U ?
    //                                           util::retrieveFileID(fullName) :
    //                                          std::string();
    /*
    Perf test Windows 7, SSD, 350k files, 50k dirs, files > 1MB: 7000
    	regular:            6.9s
    	ID per file:       43.9s
    	ID per file > 1MB:  7.2s
    	ID per dir:         8.4s

    	Linux: retrieveFileID takes about 50% longer in VM! (avoidable because of redundant stat() call!)
    */

    output_.addSubFile(fileNameShort, FileDescriptor(details.lastWriteTimeRaw, details.fileSize, details.id));

    cfg.acb_.incItemsScanned(); //add 1 element to the progress indicator
}


void DirCallback::onSymlink(const Zchar* shortName, const Zstring& fullName, const SymlinkInfo& details)
{
    boost::this_thread::interruption_point();

    if (cfg.handleSymlinks_ == SYMLINK_IGNORE)
        return;

    //update status information no matter whether object is excluded or not!
    cfg.acb_.reportCurrentFile(fullName, cfg.threadID_);

    //------------------------------------------------------------------------------------
    const Zstring& relName = relNameParentPf_ + shortName;

    //apply filter before processing (use relative name!)
    if (!cfg.filterInstance->passFileFilter(relName)) //always use file filter: Link type may not be "stable" on Linux!
        return;

    output_.addSubLink(shortName, LinkDescriptor(details.lastWriteTimeRaw, details.targetPath, details.dirLink ? LinkDescriptor::TYPE_DIR : LinkDescriptor::TYPE_FILE));

    cfg.acb_.incItemsScanned(); //add 1 element to the progress indicator
}


std::shared_ptr<TraverseCallback> DirCallback::onDir(const Zchar* shortName, const Zstring& fullName)
{
    boost::this_thread::interruption_point();

    //update status information no matter whether object is excluded or not!
    cfg.acb_.reportCurrentFile(fullName, cfg.threadID_);

    //------------------------------------------------------------------------------------
    const Zstring& relName = relNameParentPf_ + shortName;

    //apply filter before processing (use relative name!)
    bool subObjMightMatch = true;
    const bool passFilter = cfg.filterInstance->passDirFilter(relName, &subObjMightMatch);
    if (!passFilter && !subObjMightMatch)
        return nullptr; //do NOT traverse subdirs
    //else: attention! ensure directory filtering is applied later to exclude actually filtered directories

    DirContainer& subDir = output_.addSubDir(shortName);
    if (passFilter)
        cfg.acb_.incItemsScanned(); //add 1 element to the progress indicator

    return std::make_shared<DirCallback>(cfg, relName + FILE_NAME_SEPARATOR, subDir);
}


DirCallback::HandleError DirCallback::onError(const std::wstring& errorText)
{
    switch (cfg.acb_.reportError(errorText))
    {
        case FillBufferCallback::TRAV_ERROR_IGNORE:
            cfg.failedReads_.insert(relNameParentPf_);
            return TRAV_ERROR_IGNORE;

        case FillBufferCallback::TRAV_ERROR_RETRY:
            return TRAV_ERROR_RETRY;
    }

    assert(false);
    return TRAV_ERROR_IGNORE;
}


#ifdef FFS_WIN
class DstHackCallbackImpl : public DstHackCallback
{
public:
    DstHackCallbackImpl(AsyncCallback& acb, int threadID) :
        acb_(acb),
        threadID_(threadID),
        textApplyingDstHack(replaceCpy(_("Encoding extended time information: %x"), L"%x", L"\n%x")) {}

private:
    virtual void requestUiRefresh(const Zstring& filename) //applying DST hack imposes significant one-time performance drawback => callback to inform user
    {
        acb_.reportCurrentStatus(replaceCpy(textApplyingDstHack, L"%x", fmtFileName(filename)), threadID_);
    }

    AsyncCallback& acb_;
    int threadID_;
    const std::wstring textApplyingDstHack;
};
#endif
//------------------------------------------------------------------------------------------


class WorkerThread
{
public:
    WorkerThread(int threadID,
                 const std::shared_ptr<AsyncCallback>& acb,
                 const std::vector<std::pair<DirectoryKey, DirectoryValue*>>& workload) :
        threadID_(threadID),
        acb_(acb),
        workload_(workload) {}

    void operator()() //thread entry
    {
        acb_->incActiveWorker();
        ZEN_ON_SCOPE_EXIT(acb_->decActiveWorker(););

        std::for_each(workload_.begin(), workload_.end(),
                      [&](std::pair<DirectoryKey, DirectoryValue*>& item)
        {
            const Zstring& directoryName = item.first.dirnameFull_;
            DirectoryValue& dirVal = *item.second;

            acb_->reportCurrentFile(directoryName, threadID_); //just in case first directory access is blocking

            TraverserShared travCfg(threadID_,
                                    item.first.handleSymlinks_, //shared by all(!) instances of DirCallback while traversing a folder hierarchy
                                    item.first.filter_,
                                    dirVal.failedReads,
                                    *acb_);

            DirCallback traverser(travCfg,
                                  Zstring(),
                                  dirVal.dirCont);

            bool followSymlinks = false;
            switch (item.first.handleSymlinks_)
            {
                case SYMLINK_IGNORE:
                    followSymlinks = false; //=> symlinks will be reported via onSymlink() where they are excluded
                    break;
                case SYMLINK_USE_DIRECTLY:
                    followSymlinks = false;
                    break;
                case SYMLINK_FOLLOW_LINK:
                    followSymlinks = true;
                    break;
            }

            DstHackCallback* dstCallbackPtr = nullptr;
#ifdef FFS_WIN
            DstHackCallbackImpl dstCallback(*acb_, threadID_);
            dstCallbackPtr = &dstCallback;
#endif

            //get all files and folders from directoryPostfixed (and subdirectories)
            traverseFolder(directoryName, followSymlinks, traverser, dstCallbackPtr); //exceptions may be thrown!
        });
    }

private:
    int threadID_;
    std::shared_ptr<AsyncCallback> acb_;
    std::vector<std::pair<DirectoryKey, DirectoryValue*>> workload_;
};
}


void zen::fillBuffer(const std::set<DirectoryKey>& keysToRead, //in
                     std::map<DirectoryKey, DirectoryValue>& buf, //out
                     FillBufferCallback& callback,
                     size_t updateInterval)
{
    buf.clear();

    std::vector<std::set<DirectoryKey>> buckets = separateByDistinctDisk(keysToRead); //one bucket per physical device

    std::vector<boost::thread> worker; //note: GCC doesn't allow to construct an array of empty threads since they would be initialized by const boost::thread&
    worker.reserve(buckets.size());

    zen::ScopeGuard guardWorker = zen::makeGuard([&]
    {
        std::for_each(worker.begin(), worker.end(), [](boost::thread& wt) { wt.interrupt(); }); //interrupt all at once, then join
        std::for_each(worker.begin(), worker.end(), [](boost::thread& wt) { wt.join(); });
    });

    std::shared_ptr<AsyncCallback> acb = std::make_shared<AsyncCallback>();

    //init worker threads
    for (auto iter = buckets.begin(); iter != buckets.end(); ++iter)
    {
        int threadID = iter - buckets.begin();
        const std::set<DirectoryKey>& bucket = *iter;

        std::vector<std::pair<DirectoryKey, DirectoryValue*>> workload;
        std::for_each(bucket.begin(), bucket.end(),
                      [&](const DirectoryKey& key)
        {
            auto rv = buf.insert(std::make_pair(key, DirectoryValue()));
            assert(rv.second);
            workload.push_back(std::make_pair(key, &rv.first->second));
        });

        worker.push_back(boost::thread(WorkerThread(threadID, acb, workload)));
    }

    //wait until done
    for (auto iter = worker.begin(); iter != worker.end(); ++iter)
    {
        boost::thread& wt = *iter;
        int threadID = iter - worker.begin();

        acb->setNotifyingThread(threadID); //process info messages of first (active) thread only

        do
        {
            //update status
            callback.reportStatus(acb->getCurrentStatus(), acb->getItemsScanned()); //throw!

            //process errors
            acb->processErrors(callback);
        }
        while (!wt.timed_join(boost::posix_time::milliseconds(updateInterval)));
    }

    guardWorker.dismiss();
}
