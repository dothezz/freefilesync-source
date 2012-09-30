// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) Zenju (zenju AT gmx DOT de) - All Rights Reserved        *
// **************************************************************************

#include "dir_watcher.h"
#include <algorithm>
#include <set>
#include "thread.h" //includes <boost/thread.hpp>
#include "scope_guard.h"

#ifdef FFS_WIN
#include "notify_removal.h"
#include "win.h" //includes "windows.h"
#include "long_path_prefix.h"
//#include "privilege.h"

#elif defined FFS_LINUX
#include <sys/inotify.h>
#include <fcntl.h>
#include "file_traverser.h"
#endif

using namespace zen;


#ifdef FFS_WIN
namespace
{
class SharedData
{
public:
    //context of worker thread
    void addChanges(const char* buffer, DWORD bytesWritten, const Zstring& dirname) //throw ()
    {
        boost::lock_guard<boost::mutex> dummy(lockAccess);

        std::set<Zstring>& output = changedFiles;

        if (bytesWritten == 0) //according to docu this may happen in case of internal buffer overflow: report some "dummy" change
            output.insert(L"Overflow!");
        else
        {
            const char* bufPos = &buffer[0];
            for (;;)
            {
                const FILE_NOTIFY_INFORMATION& notifyInfo = reinterpret_cast<const FILE_NOTIFY_INFORMATION&>(*bufPos);

                const Zstring fullname = dirname + Zstring(notifyInfo.FileName, notifyInfo.FileNameLength / sizeof(WCHAR));

                //skip modifications sent by changed directories: reason for change, child element creation/deletion, will notify separately!
                [&]
                {
                    if (notifyInfo.Action == FILE_ACTION_RENAMED_OLD_NAME) //reporting FILE_ACTION_RENAMED_NEW_NAME should suffice
                        return;

                    if (notifyInfo.Action == FILE_ACTION_MODIFIED)
                    {
                        //note: this check will not work if top watched directory has been renamed
                        const DWORD ret = ::GetFileAttributes(applyLongPathPrefix(fullname).c_str());
                        if (ret != INVALID_FILE_ATTRIBUTES && (ret & FILE_ATTRIBUTE_DIRECTORY)) //returns true for (dir-)symlinks also
                            return;
                    }

                    output.insert(fullname);
                }();

                if (notifyInfo.NextEntryOffset == 0)
                    break;
                bufPos += notifyInfo.NextEntryOffset;
            }
        }
    }

    //context of main thread
    void addChange(const Zstring& dirname) //throw ()
    {
        boost::lock_guard<boost::mutex> dummy(lockAccess);
        changedFiles.insert(dirname);
    }


    //context of main thread
    void getChanges(std::vector<Zstring>& output) //throw FileError, ErrorNotExisting
    {
        boost::lock_guard<boost::mutex> dummy(lockAccess);

        //first check whether errors occured in thread
        if (!errorMsg.first.empty())
        {
            const std::wstring msg = errorMsg.first.c_str();
            const DWORD lastError = errorMsg.second;

            if (errorCodeForNotExisting(lastError))
                throw ErrorNotExisting(msg);
            throw FileError(msg);
        }

        output.assign(changedFiles.begin(), changedFiles.end());
        changedFiles.clear();
    }


    //context of worker thread
    void reportError(const std::wstring& msg, DWORD errorCode) //throw()
    {
        boost::lock_guard<boost::mutex> dummy(lockAccess);
        errorMsg = std::make_pair(copyStringTo<BasicWString>(msg), errorCode);
    }

private:
    typedef Zbase<wchar_t> BasicWString; //thread safe string class for UI texts

    boost::mutex lockAccess;
    std::set<Zstring> changedFiles; //get rid of duplicate entries (actually occur!)
    std::pair<BasicWString, DWORD> errorMsg; //non-empty if errors occured in thread
};


class ReadChangesAsync
{
public:
    //constructed in main thread!
    ReadChangesAsync(const Zstring& directory, //make sure to not leak in thread-unsafe types!
                     const std::shared_ptr<SharedData>& shared) :
        shared_(shared),
        dirnamePf(appendSeparator(directory)),
        hDir(INVALID_HANDLE_VALUE)
    {
        hDir = ::CreateFile(applyLongPathPrefix(dirnamePf).c_str(),
                            FILE_LIST_DIRECTORY,
                            FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                            nullptr,
                            OPEN_EXISTING,
                            FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OVERLAPPED,
                            nullptr);
        if (hDir == INVALID_HANDLE_VALUE)
        {
            const DWORD lastError = ::GetLastError();
            const std::wstring errorMsg = replaceCpy(_("Cannot monitor directory %x."), L"%x", fmtFileName(directory)) + L"\n\n" + getLastErrorFormatted(lastError);
            if (errorCodeForNotExisting(lastError))
                throw ErrorNotExisting(errorMsg);
            throw FileError(errorMsg);
        }

        //end of constructor, no need to start managing "hDir"
    }

    ~ReadChangesAsync()
    {
        if (hDir != INVALID_HANDLE_VALUE) //valid hDir is NOT an invariant, see move constructor!
            ::CloseHandle(hDir);
    }

    void operator()() //thread entry
    {
        try
        {
            std::vector<char> buffer(64 * 1024); //needs to be aligned on a DWORD boundary; maximum buffer size restricted by some networks protocols (according to docu)

            for (;;)
            {
                boost::this_thread::interruption_point();

                //actual work
                OVERLAPPED overlapped = {};
                overlapped.hEvent = ::CreateEvent(nullptr,  //__in_opt  LPSECURITY_ATTRIBUTES lpEventAttributes,
                                                  true,     //__in      BOOL bManualReset,
                                                  false,    //__in      BOOL bInitialState,
                                                  nullptr); //__in_opt  LPCTSTR lpName
                if (overlapped.hEvent == nullptr)
                    return shared_->reportError(replaceCpy(_("Cannot monitor directory %x."), L"%x", fmtFileName(dirnamePf)) + L" (CreateEvent)" L"\n\n" + getLastErrorFormatted(), ::GetLastError());

                ZEN_ON_SCOPE_EXIT(::CloseHandle(overlapped.hEvent));

                //asynchronous variant: runs on this thread's APC queue!
                if (!::ReadDirectoryChangesW(hDir,                              //  __in   HANDLE hDirectory,
                                             &buffer[0],                        //  __out  LPVOID lpBuffer,
                                             static_cast<DWORD>(buffer.size()), //  __in   DWORD nBufferLength,
                                             true,                              //  __in   BOOL bWatchSubtree,
                                             FILE_NOTIFY_CHANGE_FILE_NAME |
                                             FILE_NOTIFY_CHANGE_DIR_NAME  |
                                             FILE_NOTIFY_CHANGE_SIZE      |
                                             FILE_NOTIFY_CHANGE_LAST_WRITE, //  __in         DWORD dwNotifyFilter,
                                             nullptr,                       //  __out_opt    LPDWORD lpBytesReturned,
                                             &overlapped,                   //  __inout_opt  LPOVERLAPPED lpOverlapped,
                                             nullptr))                      //  __in_opt     LPOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine
                    return shared_->reportError(replaceCpy(_("Cannot monitor directory %x."), L"%x", fmtFileName(dirnamePf)) + L" (ReadDirectoryChangesW)" L"\n\n" + getLastErrorFormatted(), ::GetLastError());

                //async I/O is a resource that needs to be guarded since it will write to local variable "buffer"!
                zen::ScopeGuard guardAio = zen::makeGuard([&]
                {
                    //Canceling Pending I/O Operations: http://msdn.microsoft.com/en-us/library/aa363789(v=vs.85).aspx
                    //if (::CancelIoEx(hDir, &overlapped) /*!= FALSE*/ || ::GetLastError() != ERROR_NOT_FOUND) -> Vista only
                    if (::CancelIo(hDir) /*!= FALSE*/ || ::GetLastError() != ERROR_NOT_FOUND)
                    {
                        DWORD bytesWritten = 0;
                        ::GetOverlappedResult(hDir, &overlapped, &bytesWritten, true); //wait until cancellation is complete
                    }
                });

                //wait for results
                DWORD bytesWritten = 0;
                while (!::GetOverlappedResult(hDir,          //__in   HANDLE hFile,
                                              &overlapped,   //__in   LPOVERLAPPED lpOverlapped,
                                              &bytesWritten, //__out  LPDWORD lpNumberOfBytesTransferred,
                                              false))        //__in   BOOL bWait
                {
                    if (::GetLastError() != ERROR_IO_INCOMPLETE)
                        return shared_->reportError(replaceCpy(_("Cannot monitor directory %x."), L"%x", fmtFileName(dirnamePf)) + L" (GetOverlappedResult)" L"\n\n" + getLastErrorFormatted(), ::GetLastError());

                    //execute asynchronous procedure calls (APC) queued on this thread
                    ::SleepEx(50,    // __in  DWORD dwMilliseconds,
                              true); // __in  BOOL bAlertable

                    boost::this_thread::interruption_point();
                }
                guardAio.dismiss();

                shared_->addChanges(&buffer[0], bytesWritten, dirnamePf); //throw ()
            }
        }
        catch (boost::thread_interrupted&)
        {
            throw; //this is the only exception expected!
        }
    }

    ReadChangesAsync(ReadChangesAsync&& other) :
        hDir(INVALID_HANDLE_VALUE)
    {
        shared_   = std::move(other.shared_);
        dirnamePf = std::move(other.dirnamePf);
        std::swap(hDir, other.hDir);
    }

    HANDLE getDirHandle() const { return hDir; } //for reading/monitoring purposes only, don't abuse (e.g. close handle)!

private:
    ReadChangesAsync(const ReadChangesAsync&);
    ReadChangesAsync& operator=(const ReadChangesAsync&);

    //shared between main and worker:
    std::shared_ptr<SharedData> shared_;
    //worker thread only:
    Zstring dirnamePf; //thread safe!
    HANDLE hDir;
};


class HandleVolumeRemoval : public NotifyRequestDeviceRemoval
{
public:
    HandleVolumeRemoval(HANDLE hDir,
                        boost::thread& worker) :
        NotifyRequestDeviceRemoval(hDir), //throw FileError
        worker_(worker),
        removalRequested(false),
        operationComplete(false) {}

    //all functions are called by main thread!

    bool requestReceived() const { return removalRequested; }
    bool finished() const { return operationComplete; }

private:
    virtual void onRequestRemoval(HANDLE hnd)
    {
        //must release hDir immediately => stop monitoring!
        worker_.interrupt();
        worker_.join();
        //now hDir should have been released

        removalRequested = true;
    } //don't throw!

    virtual void onRemovalFinished(HANDLE hnd, bool successful) { operationComplete = true; } //throw()!

    boost::thread& worker_;
    bool removalRequested;
    bool operationComplete;
};
}


struct DirWatcher::Pimpl
{
    boost::thread worker;
    std::shared_ptr<SharedData> shared;

    Zstring dirname;
    std::unique_ptr<HandleVolumeRemoval> volRemoval;
};


DirWatcher::DirWatcher(const Zstring& directory) : //throw FileError
    pimpl_(new Pimpl)
{
    pimpl_->shared = std::make_shared<SharedData>();
    pimpl_->dirname = directory;

    ReadChangesAsync reader(directory, pimpl_->shared); //throw FileError
    pimpl_->volRemoval.reset(new HandleVolumeRemoval(reader.getDirHandle(), pimpl_->worker)); //throw FileError
    pimpl_->worker = boost::thread(std::move(reader));
}


DirWatcher::~DirWatcher()
{
    pimpl_->worker.interrupt();
    //pimpl_->worker.join(); -> we don't have time to wait... will take ~50ms anyway
    //caveat: exitting the app may simply kill this thread!
}


std::vector<Zstring> DirWatcher::getChanges(const std::function<void()>& processGuiMessages) //throw FileError
{
    std::vector<Zstring> output;

    //wait until device removal is confirmed, to prevent locking hDir again by some new watch!
    if (pimpl_->volRemoval->requestReceived())
    {
        const boost::system_time maxwait = boost::get_system_time() + boost::posix_time::seconds(15);
        //HandleVolumeRemoval::finished() not guaranteed! note: Windows gives unresponsive applications ca. 10 seconds until unmounting the usb stick in worst case

        while (!pimpl_->volRemoval->finished() && boost::get_system_time() < maxwait)
        {
            processGuiMessages(); //DBT_DEVICEREMOVECOMPLETE message is sent here!
            boost::thread::sleep(boost::get_system_time() + boost::posix_time::milliseconds(50));
        }

        output.push_back(pimpl_->dirname); //report removal as change to main directory
    }
    else //the normal case...
        pimpl_->shared->getChanges(output); //throw FileError
    return output;
}


#elif defined FFS_LINUX
struct DirWatcher::Pimpl
{
    Zstring dirname;
    int notifDescr;
    std::map<int, Zstring> watchDescrs; //watch descriptor and corresponding (sub-)directory name (postfixed with separator!)
};


namespace
{
class DirsOnlyTraverser : public zen::TraverseCallback
{
public:
    DirsOnlyTraverser(std::vector<Zstring>& dirs,
                      const std::shared_ptr<TraverseCallback>& otherMe) : otherMe_(otherMe), dirs_(dirs) {}

    virtual void onFile   (const Zchar* shortName, const Zstring& fullName, const FileInfo& details) {}
    virtual HandleLink onSymlink(const Zchar* shortName, const Zstring& fullName, const SymlinkInfo& details) { return LINK_SKIP; }
    virtual std::shared_ptr<TraverseCallback> onDir(const Zchar* shortName, const Zstring& fullName)
    {
        dirs_.push_back(fullName);
        return otherMe_;
    }
    virtual HandleError onError(const std::wstring& msg) { throw FileError(msg); }

private:
    const std::shared_ptr<TraverseCallback>& otherMe_; //lifetime management, two options: 1. use std::weak_ptr 2. ref to shared_ptr
    std::vector<Zstring>& dirs_;
};
}


DirWatcher::DirWatcher(const Zstring& directory) : //throw FileError
    pimpl_(new Pimpl)
{
    //still in main thread
    Zstring dirname = directory;
    if (endsWith(dirname, FILE_NAME_SEPARATOR))
        dirname.resize(dirname.size() - 1);

    //get all subdirectories
    std::vector<Zstring> fullDirList;
    fullDirList.push_back(dirname);

    std::shared_ptr<TraverseCallback> traverser;
    traverser = std::make_shared<DirsOnlyTraverser>(fullDirList, traverser); //throw FileError

    zen::traverseFolder(dirname, *traverser); //don't traverse into symlinks (analog to windows build)

    //init
    pimpl_->dirname    = directory;
    pimpl_->notifDescr = ::inotify_init();
    if (pimpl_->notifDescr == -1)
        throw FileError(replaceCpy(_("Cannot monitor directory %x."), L"%x", fmtFileName(dirname)) + L"\n\n" + getLastErrorFormatted());

    zen::ScopeGuard guardDescr = zen::makeGuard([&] { ::close(pimpl_->notifDescr); });

    //set non-blocking mode
    bool initSuccess = false;
    int flags = ::fcntl(pimpl_->notifDescr, F_GETFL);
    if (flags != -1)
        initSuccess = ::fcntl(pimpl_->notifDescr, F_SETFL, flags | O_NONBLOCK) != -1;

    if (!initSuccess)
        throw FileError(replaceCpy(_("Cannot monitor directory %x."), L"%x", fmtFileName(dirname)) + L"\n\n" + getLastErrorFormatted());

    //add watches
    std::for_each(fullDirList.begin(), fullDirList.end(),
                  [&](Zstring subdir)
    {
        int wd = ::inotify_add_watch(pimpl_->notifDescr, subdir.c_str(),
                                     IN_ONLYDIR     | //watch directories only
                                     IN_DONT_FOLLOW | //don't follow symbolic links
                                     IN_MODIFY 	    |
                                     IN_CLOSE_WRITE |
                                     IN_MOVE        |
                                     IN_CREATE   	|
                                     IN_DELETE 	    |
                                     IN_DELETE_SELF |
                                     IN_MOVE_SELF);
        if (wd == -1)
        {
            const std::wstring errorMsg = replaceCpy(_("Cannot monitor directory %x."), L"%x", fmtFileName(subdir)) + L"\n\n" + getLastErrorFormatted();
            if (errorCodeForNotExisting(errno))
                throw ErrorNotExisting(errorMsg);
            throw FileError(errorMsg);
        }

        pimpl_->watchDescrs.insert(std::make_pair(wd, appendSeparator(subdir)));
    });

    guardDescr.dismiss();
}


DirWatcher::~DirWatcher()
{
    ::close(pimpl_->notifDescr); //associated watches are removed automatically!
}


std::vector<Zstring> DirWatcher::getChanges(const std::function<void()>&) //throw FileError
{
    //non-blocking call, see O_NONBLOCK
    std::vector<char> buffer(1024 * (sizeof(struct inotify_event) + 16));
    ssize_t bytesRead = ::read(pimpl_->notifDescr, &buffer[0], buffer.size());

    if (bytesRead == -1)
    {
        if (errno == EINTR || //Interrupted function call; When this happens, you should try the call again.
            errno == EAGAIN)  //Non-blocking I/O has been selected using O_NONBLOCK and no data was immediately available for reading
            return std::vector<Zstring>();

        throw FileError(replaceCpy(_("Cannot monitor directory %x."), L"%x", fmtFileName(pimpl_->dirname)) + L"\n\n" + getLastErrorFormatted());
    }

    std::set<Zstring> tmp; //get rid of duplicate entries (actually occur!)

    ssize_t bytePos = 0;
    while (bytePos < bytesRead)
    {
        struct inotify_event& evt = reinterpret_cast<struct inotify_event&>(buffer[bytePos]);

        if (evt.len != 0) //exclude case: deletion of "self", already reported by parent directory watch
        {
            auto iter = pimpl_->watchDescrs.find(evt.wd);
            if (iter != pimpl_->watchDescrs.end())
            {
                //Note: evt.len is NOT the size of the evt.name c-string, but the array size including all padding 0 characters!
                //It may be even 0 in which case evt.name must not be used!
                tmp.insert(iter->second + evt.name);
            }
        }

        bytePos += sizeof(struct inotify_event) + evt.len;
    }

    return std::vector<Zstring>(tmp.begin(), tmp.end());
}

#endif
