// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) 2008-2011 ZenJu (zhnmju123 AT gmx.de)                    *
// **************************************************************************

#include "dir_watcher.h"
#include "last_error.h"
#include "i18n.h"
#include <algorithm>
#include "boost_thread_wrap.h" //include <boost/thread.hpp>
#include "loki/ScopeGuard.h"
#include <set>
#include <wx/log.h> //wxSafeShowMessage

#ifdef FFS_WIN
#include "notify_removal.h"
#include <wx/msw/wrapwin.h> //includes "windows.h"
#include "long_path_prefix.h"
#include "privilege.h"

#elif defined FFS_LINUX
#include <sys/inotify.h>
#include <fcntl.h>
#include "file_traverser.h"
#endif


using namespace zen;


#ifdef FFS_WIN
namespace
{
typedef Zbase<wchar_t> BasicWString; //thread safe string class for UI texts


struct SharedData
{
    boost::mutex lockAccess;
    std::set<Zstring> changedFiles; //get rid of duplicate entries (actually occur!)
    BasicWString errorMsg; //non-empty if errors occured in thread
};


void addChanges(SharedData& shared, const char* buffer, DWORD bytesWritten, const Zstring& dirname) //throw ()
{
    boost::lock_guard<boost::mutex> dummy(shared.lockAccess);

    std::set<Zstring>& output = shared.changedFiles;

    if (bytesWritten == 0) //according to docu this may happen in case of internal buffer overflow: report some "dummy" change
        output.insert(L"Overflow!");
    else
    {
        const char* bufPos = &buffer[0];
        while (true)
        {
            const FILE_NOTIFY_INFORMATION& notifyInfo = reinterpret_cast<const FILE_NOTIFY_INFORMATION&>(*bufPos);

            const Zstring fullname = dirname + Zstring(notifyInfo.FileName, notifyInfo.FileNameLength / sizeof(WCHAR));

            //skip modifications sent by changed directories: reason for change, child element creation/deletion, will notify separately!
            bool skip = false;
            if (notifyInfo.Action == FILE_ACTION_MODIFIED)
            {
                //note: this check will not work if top watched directory has been renamed
                const DWORD ret = ::GetFileAttributes(applyLongPathPrefix(fullname).c_str());
                bool isDir = ret != INVALID_FILE_ATTRIBUTES && (ret & FILE_ATTRIBUTE_DIRECTORY); //returns true for (dir-)symlinks also
                skip = isDir;
            }

            if (!skip)
                output.insert(fullname);

            if (notifyInfo.NextEntryOffset == 0)
                break;
            bufPos += notifyInfo.NextEntryOffset;
        }
    }
}


void getChanges(SharedData& shared, std::vector<Zstring>& output) //throw FileError
{
    boost::lock_guard<boost::mutex> dummy(shared.lockAccess);

    //first check whether errors occured in thread
    if (!shared.errorMsg.empty())
        throw zen::FileError(shared.errorMsg.c_str());

    output.assign(shared.changedFiles.begin(), shared.changedFiles.end());
    shared.changedFiles.clear();
}


void reportError(SharedData& shared, const BasicWString& errorMsg) //throw ()
{
    boost::lock_guard<boost::mutex> dummy(shared.lockAccess);
    shared.errorMsg = errorMsg;
}


class ReadChangesAsync
{
public:
    ReadChangesAsync(const Zstring& directory, //make sure to not leak in thread-unsafe types!
                     const std::shared_ptr<SharedData>& shared) :
        shared_(shared),
        dirname(directory)
    {
        //still in main thread
        if (!endsWith(dirname, FILE_NAME_SEPARATOR))
            dirname += FILE_NAME_SEPARATOR;

        //these two privileges are required by ::CreateFile FILE_FLAG_BACKUP_SEMANTICS according to
        //http://msdn.microsoft.com/en-us/library/aa363858(v=vs.85).aspx
        try
        {
            Privileges::getInstance().ensureActive(SE_BACKUP_NAME);  //throw FileError
            Privileges::getInstance().ensureActive(SE_RESTORE_NAME); //
        }
        catch (const FileError&) {}

        hDir = ::CreateFile(applyLongPathPrefix(dirname.c_str()).c_str(),
                            FILE_LIST_DIRECTORY,
                            FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                            NULL,
                            OPEN_EXISTING,
                            FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OVERLAPPED,
                            NULL);
        if(hDir == INVALID_HANDLE_VALUE )
            throw FileError(_("Could not initialize directory monitoring:") + "\n\"" + utf8CvrtTo<std::wstring>(dirname) + "\"" + "\n\n" + zen::getLastErrorFormatted());

        //Loki::ScopeGuard guardDir = Loki::MakeGuard(::CloseHandle, hDir);
        //guardDir.Dismiss();
    }

    ~ReadChangesAsync()
    {
        if (hDir != INVALID_HANDLE_VALUE)
            ::CloseHandle(hDir);
    }

    void operator()() //thread entry
    {
        try
        {
            std::vector<char> buffer(64 * 1024); //maximum buffer size restricted by some networks protocols (according to docu)

            while (true)
            {
                boost::this_thread::interruption_point();

                //actual work
                OVERLAPPED overlapped = {};
                overlapped.hEvent = ::CreateEvent(NULL,  //__in_opt  LPSECURITY_ATTRIBUTES lpEventAttributes,
                                                  true,  //__in      BOOL bManualReset,
                                                  false, //__in      BOOL bInitialState,
                                                  NULL); //__in_opt  LPCTSTR lpName
                if (overlapped.hEvent == NULL)
                    return ::reportError(*shared_, BasicWString(_("Error when monitoring directories.") + "\n\n" + getLastErrorFormatted())); //throw () + quit thread

                Loki::ScopeGuard dummy = Loki::MakeGuard(::CloseHandle, overlapped.hEvent);
                (void)dummy;

                //asynchronous variant: runs on this thread's APC queue!
                if (!::ReadDirectoryChangesW(hDir,                          //  __in         HANDLE hDirectory,
                                             &buffer[0],                    //  __out        LPVOID lpBuffer,
                                             static_cast<DWORD>(buffer.size()), //  __in         DWORD nBufferLength,
                                             true,                          //  __in         BOOL bWatchSubtree,
                                             FILE_NOTIFY_CHANGE_FILE_NAME |
                                             FILE_NOTIFY_CHANGE_DIR_NAME  |
                                             FILE_NOTIFY_CHANGE_SIZE      |
                                             FILE_NOTIFY_CHANGE_LAST_WRITE, //  __in         DWORD dwNotifyFilter,
                                             NULL,                          //  __out_opt    LPDWORD lpBytesReturned,
                                             &overlapped,                   //  __inout_opt  LPOVERLAPPED lpOverlapped,
                                             NULL))                    //  __in_opt     LPOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine
                    return ::reportError(*shared_, BasicWString(_("Error when monitoring directories.") + "\n\n" + getLastErrorFormatted())); //throw () + quit thread

                //async I/O is a resource that needs to be guarded since it will write to local variable "buffer"!
                Loki::ScopeGuard lockAio = Loki::MakeGuard([&]()
                {
                    //http://msdn.microsoft.com/en-us/library/aa363789(v=vs.85).aspx
                    bool cancelSuccess = ::CancelIo(hDir) == TRUE; //cancel all async I/O related to this handle and thread
                    if (cancelSuccess)
                    {
                        DWORD bytesWritten = 0;
                        ::GetOverlappedResult(hDir, &overlapped, &bytesWritten, true); //wait until cancellation is complete
                    }
                });

                DWORD bytesWritten = 0;

                //wait for results
                while (!::GetOverlappedResult(hDir,          //__in   HANDLE hFile,
                                              &overlapped,   //__in   LPOVERLAPPED lpOverlapped,
                                              &bytesWritten, //__out  LPDWORD lpNumberOfBytesTransferred,
                                              false))        //__in   BOOL bWait
                {
                    if (::GetLastError() != ERROR_IO_INCOMPLETE)
                        return ::reportError(*shared_, BasicWString(_("Error when monitoring directories.") + "\n\n" + getLastErrorFormatted())); //throw () + quit thread

                    //execute asynchronous procedure calls (APC) queued on this thread
                    ::SleepEx(50,    // __in  DWORD dwMilliseconds,
                              true); // __in  BOOL bAlertable

                    boost::this_thread::interruption_point();
                }
                lockAio.Dismiss();

                ::addChanges(*shared_, &buffer[0], bytesWritten, dirname); //throw ()
            }
        }
        catch (boost::thread_interrupted&)
        {
            throw; //this is the only reasonable exception!
        }
        catch (...) //exceptions must be catched per thread
        {
            wxSafeShowMessage(wxString(_("An exception occurred!")) + wxT("(Dir Watcher)"), wxT("Unknown exception in worker thread!")); //simple wxMessageBox won't do for threads
        }
    }

    ReadChangesAsync(ReadChangesAsync&& other) :
        hDir(INVALID_HANDLE_VALUE)
    {
        shared_ = std::move(other.shared_);
        dirname = std::move(other.dirname);
        std::swap(hDir, other.hDir);
    }

    HANDLE getDirHandle() const { return hDir; } //for reading purposes only, don't abuse (e.g. close handle)!

private:
    //shared between main and worker:
    std::shared_ptr<SharedData> shared_;
    //worker thread only:
    Zstring dirname; //thread safe!
    HANDLE hDir;
};


class HandleVolumeRemoval : public NotifyRequestDeviceRemoval
{
public:
    HandleVolumeRemoval(HANDLE hDir,
                        boost::thread& worker,
                        const std::shared_ptr<SharedData>& shared,
                        const Zstring& dirname) :
        NotifyRequestDeviceRemoval(hDir), //throw FileError
        worker_(worker),
        shared_(shared),
        dirname_(dirname),
        removalRequested(false),
        operationComplete(false) {}

    //all functions are called by main thread!

    bool requestReceived() const { return removalRequested; }
    bool finished() const { return operationComplete; }

private:
    virtual void onRequestRemoval(HANDLE hnd)
    {
        //must release hDir immediately!
        worker_.interrupt();
        worker_.join();
        //now hDir should have been released

        //report removal as change to main directory
        {
            boost::lock_guard<boost::mutex> dummy(shared_->lockAccess);
            shared_->changedFiles.insert(dirname_);
        }

        removalRequested = true;
    } //don't throw!
    virtual void onRemovalFinished(HANDLE hnd, bool successful) { operationComplete = true; } //throw()!

    boost::thread& worker_;
    std::shared_ptr<SharedData> shared_;
    Zstring dirname_;
    bool removalRequested;
    bool operationComplete;
};
}


struct DirWatcher::Pimpl
{
    boost::thread worker;
    std::shared_ptr<SharedData> shared;

    std::unique_ptr<HandleVolumeRemoval> volRemoval;
};


DirWatcher::DirWatcher(const Zstring& directory) : //throw FileError
    pimpl_(new Pimpl)
{
    pimpl_->shared = std::make_shared<SharedData>();

    ReadChangesAsync reader(directory, pimpl_->shared); //throw FileError
    pimpl_->volRemoval.reset(new HandleVolumeRemoval(reader.getDirHandle(), pimpl_->worker, pimpl_->shared, directory)); //throw FileError
    pimpl_->worker = boost::thread(std::move(reader));
}


DirWatcher::~DirWatcher()
{
    pimpl_->worker.interrupt();
    //pimpl_->worker.join(); -> we don't have time to wait... will take ~50ms anyway
    //caveat: exitting the app may simply kill this thread!

    //wait until device removal is confirmed, to (hopefully!) prevent locking hDir again by new watch!
    if (pimpl_->volRemoval->requestReceived())
    {
        const boost::system_time maxwait = boost::get_system_time() + boost::posix_time::seconds(3); //HandleVolumeRemoval::finished() not guaranteed!

        while (!pimpl_->volRemoval->finished() && boost::get_system_time() < maxwait)
            boost::thread::sleep(boost::get_system_time() + boost::posix_time::milliseconds(50));
    }
}


std::vector<Zstring> DirWatcher::getChanges() //throw FileError
{
    std::vector<Zstring> output;
    ::getChanges(*pimpl_->shared, output); //throw FileError
    return output;
}


#elif defined FFS_LINUX
struct DirWatcher::Pimpl
{
    int notifDescr;
    std::map<int, Zstring> watchDescrs; //watch descriptor and corresponding directory name (postfixed with separator!)
};


namespace
{
class DirsOnlyTraverser : public zen::TraverseCallback
{
public:
    DirsOnlyTraverser(std::vector<Zstring>& dirs) : dirs_(dirs) {}

    virtual         void onFile   (const Zchar* shortName, const Zstring& fullName, const FileInfo& details) {}
    virtual         void onSymlink(const Zchar* shortName, const Zstring& fullName, const SymlinkInfo& details) {}
    virtual ReturnValDir onDir    (const Zchar* shortName, const Zstring& fullName)
    {
        dirs_.push_back(fullName);
        return ReturnValDir(Loki::Int2Type<ReturnValDir::TRAVERSING_DIR_CONTINUE>(), *this);
    }
    virtual HandleError onError(const std::wstring& errorText) { throw FileError(errorText); }

private:
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

    DirsOnlyTraverser traverser(fullDirList); //throw FileError
    zen::traverseFolder(dirname, false, traverser); //don't traverse into symlinks (analog to windows build)

    //init
    pimpl_->notifDescr = ::inotify_init();
    if (pimpl_->notifDescr == -1)
        throw FileError(_("Could not initialize directory monitoring:") + "\n\"" + dirname + "\"" + "\n\n" + getLastErrorFormatted());

    Loki::ScopeGuard guardDescr = Loki::MakeGuard(::close, pimpl_->notifDescr);

    //set non-blocking mode
    bool initSuccess = false;
    int flags = ::fcntl(pimpl_->notifDescr, F_GETFL);
    if (flags != -1)
        initSuccess = ::fcntl(pimpl_->notifDescr, F_SETFL, flags | O_NONBLOCK) != -1;

    if (!initSuccess)
        throw FileError(_("Could not initialize directory monitoring:") + "\n\"" + dirname + "\"" + "\n\n" + getLastErrorFormatted());

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
            throw FileError(_("Could not initialize directory monitoring:") + "\n\"" + subdir + "\"" + "\n\n" + getLastErrorFormatted());

        if (!endsWith(subdir, FILE_NAME_SEPARATOR))
            subdir += FILE_NAME_SEPARATOR;
        pimpl_->watchDescrs.insert(std::make_pair(wd, subdir));
    });

    guardDescr.Dismiss();
}


DirWatcher::~DirWatcher()
{
    ::close(pimpl_->notifDescr); //associated watches are removed automatically!
}


std::vector<Zstring> DirWatcher::getChanges() //throw FileError
{
    std::vector<char> buffer(1024 * (sizeof(struct inotify_event) + 16));

    //non-blocking call, see O_NONBLOCK
    ssize_t bytesRead = ::read(pimpl_->notifDescr, &buffer[0], buffer.size());

    if (bytesRead == -1)
    {
        if (errno == EINTR || //Interrupted function call; When this happens, you should try the call again.
            errno == EAGAIN)  //Non-blocking I/O has been selected using O_NONBLOCK and no data was immediately available for reading
            return std::vector<Zstring>();

        throw FileError(_("Error when monitoring directories.") + "\n\n" + getLastErrorFormatted());
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
