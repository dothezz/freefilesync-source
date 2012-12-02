// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) Zenju (zenju AT gmx DOT de) - All Rights Reserved        *
// **************************************************************************
#include "dir_lock.h"
#include <utility>
#include <wx/log.h>
#include <memory>
#include <zen/last_error.h>
#include <zen/thread.h> //includes <boost/thread.hpp>
#include <zen/scope_guard.h>
#include <zen/guid.h>
#include <zen/tick_count.h>
#include <zen/assert_static.h>
#include <zen/int64.h>
#include <zen/file_handling.h>
#include <zen/serialize.h>

#ifdef FFS_WIN
#include <tlhelp32.h>
#include <zen/win.h> //includes "windows.h"
#include <zen/long_path_prefix.h>
#include <Sddl.h> //login sid
#include <Lmcons.h> //UNLEN

#elif defined FFS_LINUX
#include <fcntl.h>    //::open()
#include <sys/stat.h> //
#include <unistd.h>
#endif

using namespace zen;
using namespace std::rel_ops;


namespace
{
const int EMIT_LIFE_SIGN_INTERVAL   =  5; //show life sign;        unit: [s]
const int POLL_LIFE_SIGN_INTERVAL   =  4; //poll for life sign;    unit: [s]
const int DETECT_ABANDONED_INTERVAL = 30; //assume abandoned lock; unit: [s]

const char LOCK_FORMAT_DESCR[] = "FreeFileSync";
const int LOCK_FORMAT_VER = 2; //lock file format version
}

//worker thread
class LifeSigns
{
public:
    LifeSigns(const Zstring& lockfilename) : //throw()!!! siehe SharedDirLock()
        lockfilename_(lockfilename) {} //thread safety: make deep copy!

    void operator()() const //thread entry
    {
        try
        {
            while (true)
            {
                boost::this_thread::sleep(boost::posix_time::seconds(EMIT_LIFE_SIGN_INTERVAL)); //interruption point!

                //actual work
                emitLifeSign(); //throw ()
            }
        }
        catch (const std::exception& e) //exceptions must be catched per thread
        {
            wxSafeShowMessage(_("An exception occurred!") + L" (Dirlock)", utfCvrtTo<wxString>(e.what())); //simple wxMessageBox won't do for threads
        }
    }

    void emitLifeSign() const //try to append one byte...; throw()
    {
        const char buffer[1] = {' '};

#ifdef FFS_WIN
        //ATTENTION: setting file pointer IS required! => use CreateFile/GENERIC_WRITE + SetFilePointerEx!
        //although CreateFile/FILE_APPEND_DATA without SetFilePointerEx works locally, it MAY NOT work on some network shares creating a 4 gig file!!!

        const HANDLE fileHandle = ::CreateFile(applyLongPathPrefix(lockfilename_).c_str(),
                                               GENERIC_READ | GENERIC_WRITE, //use both when writing over network, see comment in file_io.cpp
                                               FILE_SHARE_READ,
                                               nullptr,
                                               OPEN_EXISTING,
                                               FILE_ATTRIBUTE_NORMAL,
                                               nullptr);
        if (fileHandle == INVALID_HANDLE_VALUE)
            return;
        ZEN_ON_SCOPE_EXIT(::CloseHandle(fileHandle));

        const LARGE_INTEGER moveDist = {};
        if (!::SetFilePointerEx(fileHandle, //__in       HANDLE hFile,
                                moveDist,   //__in       LARGE_INTEGER liDistanceToMove,
                                nullptr,    //__out_opt  PLARGE_INTEGER lpNewFilePointer,
                                FILE_END))  //__in       DWORD dwMoveMethod
            return;

        DWORD bytesWritten = 0;
        /*bool rv = */
        ::WriteFile(fileHandle,    //__in         HANDLE hFile,
                    buffer,        //__out        LPVOID lpBuffer,
                    1,             //__in         DWORD nNumberOfBytesToWrite,
                    &bytesWritten, //__out_opt    LPDWORD lpNumberOfBytesWritten,
                    nullptr);      //__inout_opt  LPOVERLAPPED lpOverlapped

#elif defined FFS_LINUX
        const int fileHandle = ::open(lockfilename_.c_str(), O_WRONLY | O_APPEND);
        if (fileHandle < 0)
            return;
        ZEN_ON_SCOPE_EXIT(::close(fileHandle));

        const ssize_t bytesWritten = ::write(fileHandle, buffer, 1);
        (void)bytesWritten;
#endif
    }

private:
    const Zstring lockfilename_; //thread local! atomic ref-count => binary value-type semantics!
};


namespace
{
UInt64 getLockFileSize(const Zstring& filename) //throw FileError, ErrorNotExisting
{
#ifdef FFS_WIN
    WIN32_FIND_DATA fileInfo = {};
    const HANDLE searchHandle = ::FindFirstFile(applyLongPathPrefix(filename).c_str(), &fileInfo);
    if (searchHandle != INVALID_HANDLE_VALUE)
    {
        ::FindClose(searchHandle);
        return UInt64(fileInfo.nFileSizeLow, fileInfo.nFileSizeHigh);
    }

#elif defined FFS_LINUX
    struct ::stat fileInfo = {};
    if (::stat(filename.c_str(), &fileInfo) == 0) //follow symbolic links
        return UInt64(fileInfo.st_size);
#endif

    const ErrorCode lastError = getLastError();
    const std::wstring errorMessage = replaceCpy(_("Cannot read file attributes of %x."), L"%x", fmtFileName(filename)) + L"\n\n" + getLastErrorFormatted(lastError);

    if (errorCodeForNotExisting(lastError))
        throw ErrorNotExisting(errorMessage);
    else
        throw FileError(errorMessage);
}


Zstring deleteAbandonedLockName(const Zstring& lockfilename) //make sure to NOT change file ending!
{
    const size_t pos = lockfilename.rfind(FILE_NAME_SEPARATOR); //search from end
    return pos == Zstring::npos ? Zstr("Del.") + lockfilename :
           Zstring(lockfilename.c_str(), pos + 1) + //include path separator
           Zstr("Del.") +
           afterLast(lockfilename, FILE_NAME_SEPARATOR); //returns the whole string if ch not found
}


#ifdef FFS_WIN
std::wstring getLoginSid() //throw FileError
{
    HANDLE hToken = 0;
    if (!::OpenProcessToken(::GetCurrentProcess(), //__in   HANDLE ProcessHandle,
                            TOKEN_ALL_ACCESS,      //__in   DWORD DesiredAccess,
                            &hToken))              //__out  PHANDLE TokenHandle
        throw FileError(_("Cannot get process information.") + L"\n\n" + getLastErrorFormatted());
    ZEN_ON_SCOPE_EXIT(::CloseHandle(hToken));

    DWORD bufferSize = 0;
    ::GetTokenInformation(hToken, TokenGroups, nullptr, 0, &bufferSize);

    std::vector<char> buffer(bufferSize);
    if (!::GetTokenInformation(hToken,       //__in       HANDLE TokenHandle,
                               TokenGroups,  //__in       TOKEN_INFORMATION_CLASS TokenInformationClass,
                               &buffer[0],   //__out_opt  LPVOID TokenInformation,
                               bufferSize,   //__in       DWORD TokenInformationLength,
                               &bufferSize)) //__out      PDWORD ReturnLength
        throw FileError(_("Cannot get process information.") + L"\n\n" + getLastErrorFormatted());

    auto groups = reinterpret_cast<const TOKEN_GROUPS*>(&buffer[0]);

    for (DWORD i = 0; i < groups->GroupCount; ++i)
        if ((groups->Groups[i].Attributes & SE_GROUP_LOGON_ID) != 0)
        {
            LPTSTR sidStr = nullptr;
            if (!::ConvertSidToStringSid(groups->Groups[i].Sid, //__in   PSID Sid,
                                         &sidStr))              //__out  LPTSTR *StringSid
                throw FileError(_("Cannot get process information.") + L"\n\n" + getLastErrorFormatted());
            ZEN_ON_SCOPE_EXIT(::LocalFree(sidStr));

            return sidStr;
        }
    throw FileError(_("Cannot get process information.") + L"\n\n" + getLastErrorFormatted() + L"(no login found)"); //shouldn't happen
}
#endif


class FromCurrentProcess {}; //tag

struct LockInformation //throw FileError
{
    explicit LockInformation(FromCurrentProcess) :
        lockId(zen::generateGUID()),
#ifdef FFS_WIN
        sessionId(utfCvrtTo<std::string>(getLoginSid())), //throw FileError
        processId(::GetCurrentProcessId()) //never fails
    {
        DWORD bufferSize = 0;
        ::GetComputerNameEx(ComputerNameDnsFullyQualified, nullptr, &bufferSize); //get required buffer size

        std::vector<wchar_t> buffer(bufferSize);
        if (!GetComputerNameEx(ComputerNameDnsFullyQualified, //__in     COMPUTER_NAME_FORMAT NameType,
                               &buffer[0],                    //__out    LPTSTR lpBuffer,
                               &bufferSize))                  //__inout  LPDWORD lpnSize
            throw FileError(_("Cannot get process information.") + L"\n\n" + getLastErrorFormatted());
        computerName = "Windows." + utfCvrtTo<std::string>(&buffer[0]);

        bufferSize = UNLEN + 1;
        buffer.resize(bufferSize);
        if (!::GetUserName(&buffer[0],   //__out    LPTSTR lpBuffer,
                           &bufferSize)) //__inout  LPDWORD lpnSize
            throw FileError(_("Cannot get process information.") + L"\n\n" + getLastErrorFormatted());
        userId = utfCvrtTo<std::string>(&buffer[0]);
    }
#elif defined FFS_LINUX
        processId(::getpid()) //never fails
    {
        std::vector<char> buffer(10000);
        if (::gethostname(&buffer[0], buffer.size()) != 0)
            throw FileError(_("Cannot get process information.") + L"\n\n" + getLastErrorFormatted());

        computerName += "Linux."; //distinguish linux/windows lock files
        computerName += &buffer[0];

        if (::getdomainname(&buffer[0], buffer.size()) != 0)
            throw FileError(_("Cannot get process information.") + L"\n\n" + getLastErrorFormatted());
        computerName += ".";
        computerName += &buffer[0];

        uid_t userIdTmp = ::getuid(); //never fails
        userId.assign(reinterpret_cast<const char*>(&userIdTmp), sizeof(userIdTmp));

        pid_t sessionIdTmp = ::getsid(0); //new after each restart!
        if (sessionIdTmp == -1)
            throw FileError(_("Cannot get process information.") + L"\n\n" + getLastErrorFormatted());
        sessionId.assign(reinterpret_cast<const char*>(&sessionIdTmp), sizeof(sessionIdTmp));
    }
#endif

    explicit LockInformation(BinStreamIn& stream) //throw UnexpectedEndOfStreamError
    {
        char tmp[sizeof(LOCK_FORMAT_DESCR)] = {};
        readArray(stream, &tmp, sizeof(tmp));                           //file format header
        const int lockFileVersion = readNumber<boost::int32_t>(stream); //

        if (!std::equal(std::begin(tmp), std::end(tmp), std::begin(LOCK_FORMAT_DESCR)) ||
            lockFileVersion != LOCK_FORMAT_VER)
            throw UnexpectedEndOfStreamError(); //well, not really...!?

        lockId       = readContainer<std::string>(stream); //
        computerName = readContainer<std::string>(stream); //UnexpectedEndOfStreamError
        userId       = readContainer<std::string>(stream); //
        sessionId    = readContainer<std::string>(stream); //
        processId = static_cast<decltype(processId)>(readNumber<std::uint64_t>(stream)); //[!] conversion
    }

    void toStream(BinStreamOut& stream) const //throw ()
    {
        writeArray(stream, LOCK_FORMAT_DESCR, sizeof(LOCK_FORMAT_DESCR));
        writeNumber<boost::int32_t>(stream, LOCK_FORMAT_VER);

        assert_static(sizeof(processId) <= sizeof(std::uint64_t)); //ensure portability

        writeContainer(stream, lockId);
        writeContainer(stream, computerName);
        writeContainer(stream, userId);
        writeContainer(stream, sessionId);
        writeNumber<std::uint64_t>(stream, processId);
    }

    std::string lockId; //16 byte GUID - a universal identifier for this lock (no matter what the path is, considering symlinks, distributed network, etc.)

    std::string computerName; //format: HostName.DomainName
    std::string userId;    //non-readable!
    std::string sessionId; //
#ifdef FFS_WIN
    DWORD processId;
#elif defined FFS_LINUX
    pid_t processId;
#endif
};


//wxGetFullHostName() is a performance killer for some users, so don't touch!


void writeLockInfo(const Zstring& lockfilename) //throw FileError
{
    BinStreamOut streamOut;
    LockInformation(FromCurrentProcess()).toStream(streamOut);
    saveBinStream(lockfilename, streamOut.get()); //throw FileError
}


LockInformation retrieveLockInfo(const Zstring& lockfilename) //throw FileError, ErrorNotExisting
{
    BinStreamIn streamIn = loadBinStream<BinaryStream>(lockfilename); //throw FileError, ErrorNotExisting
    try
    {
        return LockInformation(streamIn); //throw UnexpectedEndOfStreamError
    }
    catch (UnexpectedEndOfStreamError&)
    {
        throw FileError(replaceCpy(_("Cannot read file %x."), L"%x", fmtFileName(lockfilename)));
    }
}


inline
std::string retrieveLockId(const Zstring& lockfilename) //throw FileError, ErrorNotExisting
{
    return retrieveLockInfo(lockfilename).lockId;
}


//true: process not available, false: cannot say anything
enum ProcessStatus
{
    PROC_STATUS_NOT_RUNNING,
    PROC_STATUS_RUNNING,
    PROC_STATUS_ITS_US,
    PROC_STATUS_NO_IDEA
};
ProcessStatus getProcessStatus(const LockInformation& lockInfo) //throw FileError
{
    const LockInformation localInfo((FromCurrentProcess())); //throw FileError

    if (lockInfo.computerName != localInfo.computerName ||
        lockInfo.userId != localInfo.userId) //another user may run a session right now!
        return PROC_STATUS_NO_IDEA; //lock owned by different computer in this network

    if (lockInfo.sessionId != localInfo.sessionId)
        return PROC_STATUS_NOT_RUNNING; //different session but same user? there can be only one

    if (lockInfo.processId == localInfo.processId) //obscure, but possible: deletion failed or a lock file is "stolen" and put back while the program is running
        return PROC_STATUS_ITS_US;

#ifdef FFS_WIN
    //note: ::OpenProcess() is no alternative as it may successfully return for crashed processes! -> remark: "WaitForSingleObject" may identify this case!
    HANDLE snapshot = ::CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, //__in  DWORD dwFlags,
                                                 0);                 //__in  DWORD th32ProcessID
    if (snapshot == INVALID_HANDLE_VALUE)
        return PROC_STATUS_NO_IDEA;
    ZEN_ON_SCOPE_EXIT(::CloseHandle(snapshot));

    PROCESSENTRY32 processEntry = {};
    processEntry.dwSize = sizeof(processEntry);

    if (!::Process32First(snapshot,       //__in     HANDLE hSnapshot,
                          &processEntry)) //__inout  LPPROCESSENTRY32 lppe
        return PROC_STATUS_NO_IDEA; //ERROR_NO_MORE_FILES not possible
    do
    {
        if (processEntry.th32ProcessID == lockInfo.processId)
            return PROC_STATUS_RUNNING; //process still running
    }
    while (::Process32Next(snapshot, &processEntry));
    if (::GetLastError() != ERROR_NO_MORE_FILES) //yes, they call it "files"
        return PROC_STATUS_NO_IDEA;

    return PROC_STATUS_NOT_RUNNING;

#elif defined FFS_LINUX
    if (lockInfo.processId <= 0 || lockInfo.processId >= 65536)
        return PROC_STATUS_NO_IDEA; //invalid process id

    return zen::dirExists("/proc/" + zen::numberTo<Zstring>(lockInfo.processId)) ? PROC_STATUS_RUNNING : PROC_STATUS_NOT_RUNNING;
#endif
}


const std::int64_t TICKS_PER_SEC = ticksPerSec(); //= 0 on error


void waitOnDirLock(const Zstring& lockfilename, DirLockCallback* callback) //throw FileError
{
    const std::wstring infoMsg = replaceCpy(_("Waiting while directory is locked (%x)..."), L"%x", fmtFileName(lockfilename));

    if (callback)
        callback->reportInfo(infoMsg);
    //---------------------------------------------------------------
    try
    {
        //convenience optimization only: if we know the owning process crashed, we needn't wait DETECT_ABANDONED_INTERVAL sec
        bool lockOwnderDead = false;
        std::string originalLockId; //empty if it cannot be retrieved
        try
        {
            const LockInformation& lockInfo = retrieveLockInfo(lockfilename); //throw FileError, ErrorNotExisting
            originalLockId = lockInfo.lockId;
            switch (getProcessStatus(lockInfo)) //throw FileError
            {
                case PROC_STATUS_ITS_US: //since we've already passed LockAdmin, the lock file seems abandoned ("stolen"?) although it's from this process
                case PROC_STATUS_NOT_RUNNING:
                    lockOwnderDead = true;
                    break;
                case PROC_STATUS_RUNNING:
                case PROC_STATUS_NO_IDEA:
                    break;
            }
        }
        catch (FileError&) {} //logfile may be only partly written -> this is no error!

        UInt64 fileSizeOld;
        TickVal lastLifeSign = getTicks();

        while (true)
        {
            const TickVal now = getTicks();
            const UInt64 fileSizeNew = ::getLockFileSize(lockfilename); //throw FileError, ErrorNotExisting

            if (TICKS_PER_SEC <= 0 || !lastLifeSign.isValid() || !now.isValid())
                throw FileError(L"System Timer failed!"); //no i18n: "should" never throw ;)

            if (fileSizeNew != fileSizeOld) //received life sign from lock
            {
                fileSizeOld  = fileSizeNew;
                lastLifeSign = now;
            }

            if (lockOwnderDead || //no need to wait any longer...
                dist(lastLifeSign, now) / TICKS_PER_SEC > DETECT_ABANDONED_INTERVAL)
            {
                DirLock dummy(deleteAbandonedLockName(lockfilename), callback); //throw FileError

                //now that the lock is in place check existence again: meanwhile another process may have deleted and created a new lock!

                if (!originalLockId.empty())
                    if (retrieveLockId(lockfilename) != originalLockId) //throw FileError, ErrorNotExisting -> since originalLockId is filled, we are not expecting errors!
                        return; //another process has placed a new lock, leave scope: the wait for the old lock is technically over...

                if (::getLockFileSize(lockfilename) != fileSizeOld) //throw FileError, ErrorNotExisting
                    continue; //late life sign

                removeFile(lockfilename); //throw FileError
                return;
            }

            //wait some time...
            assert_static(1000 * POLL_LIFE_SIGN_INTERVAL % GUI_CALLBACK_INTERVAL == 0);
            for (size_t i = 0; i < 1000 * POLL_LIFE_SIGN_INTERVAL / GUI_CALLBACK_INTERVAL; ++i)
            {
                if (callback) callback->requestUiRefresh();
                boost::this_thread::sleep(boost::posix_time::milliseconds(GUI_CALLBACK_INTERVAL));

                if (callback)
                {
                    //one signal missed: it's likely this is an abandoned lock => show countdown
                    if (dist(lastLifeSign, now) / TICKS_PER_SEC > EMIT_LIFE_SIGN_INTERVAL)
                    {
                        const int remainingSeconds = std::max<int>(0, DETECT_ABANDONED_INTERVAL - dist(lastLifeSign, getTicks()) / TICKS_PER_SEC);
                        const std::wstring remSecMsg = replaceCpy(_P("1 sec", "%x sec", remainingSeconds), L"%x", numberTo<std::wstring>(remainingSeconds));
                        callback->reportInfo(infoMsg + L" " + remSecMsg);
                    }
                    else
                        callback->reportInfo(infoMsg); //emit a message in any case (might clear other one)
                }
            }
        }
    }
    catch (const ErrorNotExisting&)
    {
        return; //what we are waiting for...
    }
}


void releaseLock(const Zstring& lockfilename) //throw ()
{
    try
    {
        removeFile(lockfilename); //throw FileError
    }
    catch (...) {}
}


bool tryLock(const Zstring& lockfilename) //throw FileError
{
#ifdef FFS_WIN
    const HANDLE fileHandle = ::CreateFile(applyLongPathPrefix(lockfilename).c_str(),
                                           GENERIC_READ | GENERIC_WRITE, //use both when writing over network, see comment in file_io.cpp
                                           FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                                           nullptr,
                                           CREATE_NEW,
                                           FILE_ATTRIBUTE_NORMAL,
                                           nullptr);
    if (fileHandle == INVALID_HANDLE_VALUE)
    {
        const DWORD lastError = ::GetLastError();
        if (lastError == ERROR_FILE_EXISTS || //confirmed to be used
            lastError == ERROR_ALREADY_EXISTS) //comment on msdn claims, this one is used on Windows Mobile 6
            return false;
        else
            throw FileError(replaceCpy(_("Cannot set directory lock %x."), L"%x", fmtFileName(lockfilename)) + L"\n\n" + getLastErrorFormatted());
    }
    ::CloseHandle(fileHandle);

    ::SetFileAttributes(applyLongPathPrefix(lockfilename).c_str(), FILE_ATTRIBUTE_HIDDEN); //(try to) hide it

#elif defined FFS_LINUX
    //O_EXCL contains a race condition on NFS file systems: http://linux.die.net/man/2/open
    ::umask(0); //important! -> why?
    const int fileHandle = ::open(lockfilename.c_str(), O_CREAT | O_WRONLY | O_EXCL, S_IRWXU | S_IRWXG | S_IRWXO);
    if (fileHandle == -1)
    {
        if (errno == EEXIST)
            return false;
        else
            throw FileError(replaceCpy(_("Cannot set directory lock %x."), L"%x", fmtFileName(lockfilename)) + L"\n\n" + getLastErrorFormatted());
    }
    ::close(fileHandle);
#endif

    ScopeGuard guardLockFile = zen::makeGuard([&] { removeFile(lockfilename); });

    //write housekeeping info: user, process info, lock GUID
    writeLockInfo(lockfilename); //throw FileError

    guardLockFile.dismiss(); //lockfile created successfully
    return true;
}
}


class DirLock::SharedDirLock
{
public:
    SharedDirLock(const Zstring& lockfilename, DirLockCallback* callback = nullptr) : //throw FileError
        lockfilename_(lockfilename)
    {
        while (!::tryLock(lockfilename))             //throw FileError
            ::waitOnDirLock(lockfilename, callback); //

        threadObj = boost::thread(LifeSigns(lockfilename));
    }

    ~SharedDirLock()
    {
        threadObj.interrupt(); //thread lifetime is subset of this instances's life
        threadObj.join(); //we assume precondition "threadObj.joinable()"!!!

        ::releaseLock(lockfilename_); //throw ()
    }

private:
    SharedDirLock(const DirLock&);
    SharedDirLock& operator=(const DirLock&);

    const Zstring lockfilename_;

    boost::thread threadObj;
};


class DirLock::LockAdmin //administrate all locks held by this process to avoid deadlock by recursion
{
public:
    static LockAdmin& instance()
    {
        static LockAdmin inst;
        return inst;
    }

    //create or retrieve a SharedDirLock
    std::shared_ptr<SharedDirLock> retrieve(const Zstring& lockfilename, DirLockCallback* callback) //throw FileError
    {
        tidyUp();

        //optimization: check if we already own a lock for this path
        auto iterGuid = fileToGuid.find(lockfilename);
        if (iterGuid != fileToGuid.end())
            if (const std::shared_ptr<SharedDirLock>& activeLock = getActiveLock(iterGuid->second)) //returns null-lock if not found
                return activeLock; //SharedDirLock is still active -> enlarge circle of shared ownership

        try //check based on lock GUID, deadlock prevention: "lockfilename" may be an alternative name for a lock already owned by this process
        {
            const std::string lockId = retrieveLockId(lockfilename); //throw FileError, ErrorNotExisting
            if (const std::shared_ptr<SharedDirLock>& activeLock = getActiveLock(lockId)) //returns null-lock if not found
            {
                fileToGuid[lockfilename] = lockId; //found an alias for one of our active locks
                return activeLock;
            }
        }
        catch (FileError&) {} //catch everything, let SharedDirLock constructor deal with errors, e.g. 0-sized/corrupted lock files

        //lock not owned by us => create a new one
        auto newLock = std::make_shared<SharedDirLock>(lockfilename, callback); //throw FileError
        const std::string& newLockGuid = retrieveLockId(lockfilename); //throw FileError, ErrorNotExisting

        //update registry
        fileToGuid[lockfilename] = newLockGuid; //throw()
        guidToLock[newLockGuid]  = newLock;     //

        return newLock;
    }

private:
    LockAdmin() {}

    typedef std::string UniqueId;
    typedef std::map<Zstring, UniqueId, LessFilename>        FileToGuidMap; //n:1 handle uppper/lower case correctly
    typedef std::map<UniqueId, std::weak_ptr<SharedDirLock>> GuidToLockMap; //1:1

    std::shared_ptr<SharedDirLock> getActiveLock(const UniqueId& lockId) //returns null if none found
    {
        auto iterLock = guidToLock.find(lockId);
        return iterLock != guidToLock.end() ? iterLock->second.lock() : nullptr; //try to get shared_ptr; throw()
    }

    void tidyUp() //remove obsolete lock entries
    {
        map_remove_if(guidToLock, [ ](const GuidToLockMap::value_type& v) { return !v.second.lock(); });
        map_remove_if(fileToGuid, [&](const FileToGuidMap::value_type& v) { return guidToLock.find(v.second) == guidToLock.end(); });
    }

    FileToGuidMap fileToGuid; //lockname |-> GUID; locks can be referenced by a lockfilename or alternatively a GUID
    GuidToLockMap guidToLock; //GUID |-> "shared lock ownership"
};


DirLock::DirLock(const Zstring& lockfilename, DirLockCallback* callback) //throw FileError
{
#ifdef FFS_WIN
    const DWORD bufferSize = 10000;
    std::vector<wchar_t> volName(bufferSize);
    if (::GetVolumePathName(lockfilename.c_str(), //__in   LPCTSTR lpszFileName,
                            &volName[0],          //__out  LPTSTR lpszVolumePathName,
                            bufferSize))          //__in   DWORD cchBufferLength
    {
        DWORD dt = ::GetDriveType(&volName[0]);
        if (dt == DRIVE_CDROM)
            return; //we don't need a lock for a CD ROM
    }
#endif

    sharedLock = LockAdmin::instance().retrieve(lockfilename, callback); //throw FileError
}
