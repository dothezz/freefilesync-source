// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) ZenJu (zhnmju123 AT gmx DOT de) - All Rights Reserved    *
// **************************************************************************

#include "file_traverser.h"
#include "last_error.h"
#include "assert_static.h"
#include "symlink_target.h"

#ifdef FFS_WIN
#include "win.h" //includes "windows.h"
#include "long_path_prefix.h"
#include "dst_hack.h"
#include "file_update_handle.h"
#include "dll.h"
#include "FindFilePlus/find_file_plus.h"
#include <zen/win_ver.h>

#elif defined FFS_LINUX
#include <sys/stat.h>
#include <dirent.h>
#endif

using namespace zen;


namespace
{
//implement "retry" in a generic way:

template <class Command> inline //function object expecting to throw FileError if operation fails
bool tryReportingError(Command cmd, zen::TraverseCallback& callback) //return "true" on success, "false" if error was ignored
{
    for (;;)
        try
        {
            cmd(); //throw FileError
            return true;
        }
        catch (const FileError& e)
        {
            switch (callback.onError(e.toString()))
            {
                case TraverseCallback::TRAV_ERROR_RETRY:
                    break;
                case TraverseCallback::TRAV_ERROR_IGNORE:
                    return false;
                    //default:
                    //  assert(false);
                    //break;
            }
        }
}


#ifdef FFS_WIN
inline
bool extractFileInfoFromSymlink(const Zstring& linkName, zen::TraverseCallback::FileInfo& output)
{
    //open handle to target of symbolic link
    HANDLE hFile = ::CreateFile(zen::applyLongPathPrefix(linkName).c_str(),
                                0,
                                FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                                0,
                                OPEN_EXISTING,
                                FILE_FLAG_BACKUP_SEMANTICS,
                                nullptr);
    if (hFile == INVALID_HANDLE_VALUE)
        return false;
    ZEN_ON_SCOPE_EXIT(::CloseHandle(hFile));

    BY_HANDLE_FILE_INFORMATION fileInfoByHandle = {};
    if (!::GetFileInformationByHandle(hFile, &fileInfoByHandle))
        return false;

    //write output
    output.fileSize         = zen::UInt64(fileInfoByHandle.nFileSizeLow, fileInfoByHandle.nFileSizeHigh);
    output.lastWriteTimeRaw = toTimeT(fileInfoByHandle.ftLastWriteTime);
    output.id = FileId(); //= extractFileID(fileInfoByHandle); -> id from dereferenced symlink is problematic, since renaming will consider the link, not the target!
    return true;
}


DWORD retrieveVolumeSerial(const Zstring& pathName) //returns 0 on error or if serial is not supported!
{
    //this works for:
    //- root paths "C:\", "D:\"
    //- network shares: \\share\dirname
    //- indirection: subst S: %USERPROFILE%
    //                  -> GetVolumePathName() on the other hand resolves "S:\Desktop\somedir" to "S:\Desktop\" - nice try...

    typedef BOOL (WINAPI* GetFileInformationByHandleFunc)(HANDLE hFile,
                                                          LPBY_HANDLE_FILE_INFORMATION lpFileInformation);

    const SysDllFun<GetFileInformationByHandleFunc> getFileInformationByHandle(L"kernel32.dll", "GetFileInformationByHandle"); //available since Windows XP
    if (!getFileInformationByHandle)
    {
        assert(false);
        return 0;
    }

    const HANDLE hDir = ::CreateFile(zen::applyLongPathPrefix(pathName).c_str(),
                                     0,
                                     FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                                     nullptr,
                                     OPEN_EXISTING,
                                     FILE_FLAG_BACKUP_SEMANTICS /*needed to open a directory*/ /*| FILE_FLAG_OPEN_REPARSE_POINT -> no, we follow symlinks!*/ ,
                                     nullptr);
    if (hDir == INVALID_HANDLE_VALUE)
        return 0;
    ZEN_ON_SCOPE_EXIT(::CloseHandle(hDir));

    BY_HANDLE_FILE_INFORMATION fileInfo = {};
    if (!getFileInformationByHandle(hDir,       //__in   HANDLE hFile,
                                    &fileInfo)) //__out  LPBY_HANDLE_FILE_INFORMATION lpFileInformation
        return 0;

    return fileInfo.dwVolumeSerialNumber;
}


/*
DWORD retrieveVolumeSerial(const Zstring& pathName) //returns 0 on error!
{
    //note: this works for network shares: \\share\dirname, but not "subst"!

    const DWORD BUFFER_SIZE = 10000;
    std::vector<wchar_t> buffer(BUFFER_SIZE);

    //full pathName need not yet exist!
    if (!::GetVolumePathName(pathName.c_str(), //__in   LPCTSTR lpszFileName,
                             &buffer[0],       //__out  LPTSTR lpszVolumePathName,
                             BUFFER_SIZE))     //__in   DWORD cchBufferLength
        return 0;

    Zstring volumePath = &buffer[0];
    if (!endsWith(volumePath, FILE_NAME_SEPARATOR))
        volumePath += FILE_NAME_SEPARATOR;

    DWORD volumeSerial = 0;
    if (!::GetVolumeInformation(volumePath.c_str(), //__in_opt   LPCTSTR lpRootPathName,
                                nullptr,               //__out      LPTSTR lpVolumeNameBuffer,
                                0,                  //__in       DWORD nVolumeNameSize,
                                &volumeSerial,      //__out_opt  LPDWORD lpVolumeSerialNumber,
                                nullptr,               //__out_opt  LPDWORD lpMaximumComponentLength,
                                nullptr,               //__out_opt  LPDWORD lpFileSystemFlags,
                                nullptr,               //__out      LPTSTR lpFileSystemNameBuffer,
                                0))                 //__in       DWORD nFileSystemNameSize
        return 0;

    return volumeSerial;
}
*/


const bool isXpOrLater = winXpOrLater(); //VS2010 compiled DLLs are not supported on Win 2000: Popup dialog "DecodePointer not found"

const DllFun<findplus::OpenDirFunc>  openDir = isXpOrLater ? DllFun<findplus::OpenDirFunc >(findplus::getDllName(), findplus::openDirFuncName ) : DllFun<findplus::OpenDirFunc >(); //
const DllFun<findplus::ReadDirFunc>  readDir = isXpOrLater ? DllFun<findplus::ReadDirFunc >(findplus::getDllName(), findplus::readDirFuncName ) : DllFun<findplus::ReadDirFunc >(); //load at startup: avoid pre C++11 static initialization MT issues
const DllFun<findplus::CloseDirFunc> closeDir= isXpOrLater ? DllFun<findplus::CloseDirFunc>(findplus::getDllName(), findplus::closeDirFuncName) : DllFun<findplus::CloseDirFunc>(); //

/*
Common C-style interface for Win32 FindFirstFile(), FindNextFile() and FileFilePlus openDir(), closeDir():
struct TraverserPolicy //see "policy based design"
{
typedef ... DirHandle;
typedef ... FindData;

static void create(const Zstring& directory, DirHandle& hnd); //throw FileError - *no* concession to FindFirstFile(): open handle only, *no* return of data!
static void destroy(DirHandle hnd); //throw()

template <class FallbackFun>
static bool getEntry(DirHandle hnd, const Zstring& directory, FindData& fileInfo, FallbackFun fb) //throw FileError -> fb: fallback to FindFirstFile()/FindNextFile()

//FindData "member" functions
static void extractFileInfo            (const FindData& fileInfo, DWORD volumeSerial, TraverseCallback::FileInfo& output); //volumeSerial may be 0 if not available!
static Int64 getModTime                (const FindData& fileInfo);
static const FILETIME& getModTimeRaw   (const FindData& fileInfo); //yet another concession to DST hack
static const FILETIME& getCreateTimeRaw(const FindData& fileInfo); //
static const wchar_t* getShortName     (const FindData& fileInfo);
static bool isDirectory                (const FindData& fileInfo);
static bool isSymlink                  (const FindData& fileInfo);
}

Note: Win32 FindFirstFile(), FindNextFile() is a weaker abstraction than FileFilePlus openDir(), readDir(), closeDir() and Unix opendir(), closedir(), stat()
*/


struct Win32Traverser
{
    struct DirHandle
    {
        DirHandle() : searchHandle(nullptr), firstRead(true) {}

        HANDLE searchHandle;
        bool firstRead;
        WIN32_FIND_DATA firstData;
    };

    typedef WIN32_FIND_DATA FindData;

    static void create(const Zstring& directory, DirHandle& hnd) //throw FileError
    {
        const Zstring& directoryPf = endsWith(directory, FILE_NAME_SEPARATOR) ?
                                     directory :
                                     directory + FILE_NAME_SEPARATOR;

        hnd.searchHandle = ::FindFirstFile(applyLongPathPrefix(directoryPf + L'*').c_str(), &hnd.firstData);
        //no noticable performance difference compared to FindFirstFileEx with FindExInfoBasic, FIND_FIRST_EX_CASE_SENSITIVE and/or FIND_FIRST_EX_LARGE_FETCH
        if (hnd.searchHandle == INVALID_HANDLE_VALUE)
            throw FileError(_("Error traversing directory:") + L"\n\"" + directory + L"\"" + L"\n\n" + zen::getLastErrorFormatted());

        //::GetLastError() == ERROR_FILE_NOT_FOUND -> *usually* NOT okay:
        //however it is unclear whether this indicates a missing directory or a completely empty directory
        //      note: not all directories contain "., .." entries! E.g. a drive's root directory or NetDrive + ftp.gnu.org\CRYPTO.README"
        //      -> addon: this is NOT a directory, it looks like one in NetDrive, but it's a file in Opera!
        //we have to guess it's former and let the error propagate
        // -> FindFirstFile() is a nice example of violation of API design principle of single responsibility and its consequences
    }

    static void destroy(const DirHandle& hnd) { ::FindClose(hnd.searchHandle); } //throw()

    template <class FallbackFun>
    static bool getEntry(DirHandle& hnd, const Zstring& directory, FindData& fileInfo, FallbackFun) //throw FileError
    {
        if (hnd.firstRead)
        {
            hnd.firstRead = false;
            ::memcpy(&fileInfo, &hnd.firstData, sizeof(fileInfo));
            return true;
        }

        if (!::FindNextFile(hnd.searchHandle, &fileInfo))
        {
            if (::GetLastError() == ERROR_NO_MORE_FILES) //not an error situation
                return false;
            //else we have a problem... report it:
            throw FileError(_("Error traversing directory:") + L"\n\"" + directory + L"\"" + L"\n\n" + zen::getLastErrorFormatted());
        }
        return true;
    }

    template <class FindData>
    static void extractFileInfo(const FindData& fileInfo, DWORD volumeSerial, TraverseCallback::FileInfo& output)
    {
        output.fileSize         = UInt64(fileInfo.nFileSizeLow, fileInfo.nFileSizeHigh);
        output.lastWriteTimeRaw = getModTime(fileInfo);
        output.id = FileId();
    }

    template <class FindData>
    static Int64 getModTime(const FindData& fileInfo) { return toTimeT(fileInfo.ftLastWriteTime); }

    template <class FindData>
    static const FILETIME& getModTimeRaw(const FindData& fileInfo) { return fileInfo.ftLastWriteTime; }

    template <class FindData>
    static const FILETIME& getCreateTimeRaw(const FindData& fileInfo) { return fileInfo.ftCreationTime; }

    template <class FindData>
    static const wchar_t* getShortName(const FindData& fileInfo) { return fileInfo.cFileName; }

    template <class FindData>
    static bool isDirectory(const FindData& fileInfo) { return (fileInfo.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0; }

    template <class FindData>
    static bool isSymlink(const FindData& fileInfo) { return (fileInfo.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT) != 0; }
};


struct FilePlusTraverser
{
    struct DirHandle
    {
        DirHandle() : searchHandle(nullptr) {}

        findplus::FindHandle searchHandle;
    };

    typedef findplus::FileInformation FindData;

    static void create(const Zstring& directory, DirHandle& hnd) //throw FileError
    {
        hnd.searchHandle = ::openDir(applyLongPathPrefix(directory).c_str());
        if (hnd.searchHandle == nullptr)
            throw FileError(_("Error traversing directory:") + L"\n\"" + directory + L"\"" + L"\n\n" + zen::getLastErrorFormatted());
    }

    static void destroy(DirHandle hnd) { ::closeDir(hnd.searchHandle); } //throw()

    template <class FallbackFun>
    static bool getEntry(DirHandle hnd, const Zstring& directory, FindData& fileInfo, FallbackFun fb) //throw FileError
    {
        if (!::readDir(hnd.searchHandle, fileInfo))
        {
            const DWORD lastError = ::GetLastError();
            if (lastError == ERROR_NO_MORE_FILES) //not an error situation
                return false;

            /*
            fallback to default directory query method, if FileIdBothDirectoryInformation is not properly implemented
            this is required for NetDrive mounted Webdav, e.g. www.box.net and NT4, 2000 remote drives, et al.

            NT status code                  |  Win32 error code
            -----------------------------------------------------------
            STATUS_INVALID_LEVEL            | ERROR_INVALID_LEVEL
            STATUS_NOT_SUPPORTED            | ERROR_NOT_SUPPORTED
            STATUS_INVALID_PARAMETER        | ERROR_INVALID_PARAMETER
            STATUS_INVALID_NETWORK_RESPONSE | ERROR_BAD_NET_RESP
            STATUS_INVALID_INFO_CLASS       | ERROR_INVALID_PARAMETER
            STATUS_UNSUCCESSFUL             | ERROR_GEN_FAILURE
            STATUS_ACCESS_VIOLATION         | ERROR_NOACCESS       ->FileIdBothDirectoryInformation on XP accessing UDF
            */

            if (lastError == ERROR_INVALID_LEVEL     ||
                lastError == ERROR_NOT_SUPPORTED     ||
                lastError == ERROR_INVALID_PARAMETER ||
                lastError == ERROR_BAD_NET_RESP      ||
                lastError == ERROR_UNEXP_NET_ERR     || //traverse network drive hosted by Win98
                lastError == ERROR_GEN_FAILURE       ||
                lastError == ERROR_NOACCESS)
            {
                fb(); //fallback should apply to whole directory sub-tree!
                return false;
            }

            //else we have a problem... report it:
            throw FileError(_("Error traversing directory:") + L"\n\"" + directory + L"\"" + L"\n\n" + zen::getLastErrorFormatted());
        }
        return true;
    }

    template <class FindData>
    static void extractFileInfo(const FindData& fileInfo, DWORD volumeSerial, TraverseCallback::FileInfo& output)
    {
        output.fileSize         = fileInfo.fileSize.QuadPart;
        output.lastWriteTimeRaw = getModTime(fileInfo);
        output.id               = extractFileID(volumeSerial, fileInfo.fileId);
    }

    template <class FindData>
    static Int64 getModTime(const FindData& fileInfo) { return toTimeT(fileInfo.lastWriteTime); }

    template <class FindData>
    static const FILETIME& getModTimeRaw(const FindData& fileInfo) { return fileInfo.lastWriteTime; }

    template <class FindData>
    static const FILETIME& getCreateTimeRaw(const FindData& fileInfo) { return fileInfo.creationTime; }

    template <class FindData>
    static const wchar_t* getShortName(const FindData& fileInfo) { return fileInfo.shortName; }

    template <class FindData>
    static bool isDirectory(const FindData& fileInfo) { return (fileInfo.fileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0; }

    template <class FindData>
    static bool isSymlink(const FindData& fileInfo) { return (fileInfo.fileAttributes & FILE_ATTRIBUTE_REPARSE_POINT) != 0; }
};


class DirTraverser
{
public:
    DirTraverser(const Zstring& baseDirectory, bool followSymlinks, zen::TraverseCallback& sink, zen::DstHackCallback* dstCallback) :
        isFatFileSystem(dst::isFatDrive(baseDirectory)),
        followSymlinks_(followSymlinks),
        volumeSerial(retrieveVolumeSerial(baseDirectory)) //return 0 on error
    {
        try //traversing certain folders with restricted permissions requires this privilege! (but copying these files may still fail)
        {
            activatePrivilege(SE_BACKUP_NAME); //throw FileError
        }
        catch (FileError&) {} //don't cause issues in user mode

        if (::openDir && ::readDir && ::closeDir)
            traverse<FilePlusTraverser>(baseDirectory, sink, 0);
        else //fallback
            traverse<Win32Traverser>(baseDirectory, sink, 0);

        //apply daylight saving time hack AFTER file traversing, to give separate feedback to user
        if (dstCallback && isFatFileSystem)
            applyDstHack(*dstCallback);
    }

private:
    DirTraverser(const DirTraverser&);
    DirTraverser& operator=(const DirTraverser&);

    template <class Trav>
    void traverse(const Zstring& directory, zen::TraverseCallback& sink, int level)
    {
        tryReportingError([&]
        {
            if (level == 100) //notify endless recursion
                throw FileError(_("Endless loop when traversing directory:") + L"\n\"" + directory + L"\"");
        }, sink);

        typename Trav::DirHandle searchHandle;

        const bool openSuccess = tryReportingError([&]
        {
            typedef Trav Trav; //f u VS!
            Trav::create(directory, searchHandle); //throw FileError
        }, sink);

        if (!openSuccess)
            return; //ignored error
        ZEN_ON_SCOPE_EXIT(typedef Trav Trav; Trav::destroy(searchHandle));

        typename Trav::FindData fileInfo = {};

        auto fallback = [&] { this->traverse<Win32Traverser>(directory, sink, level); }; //help VS2010 a little by avoiding too deeply nested lambdas

        while ([&]() -> bool
    {
        bool gotEntry = false;

        typedef Trav Trav1; //f u VS!
        tryReportingError([&]
        {
            typedef Trav1 Trav; //f u VS!
            gotEntry = Trav::getEntry(searchHandle, directory, fileInfo, fallback); //throw FileError
            }, sink);

            return gotEntry;
        }())
        {
            //skip "." and ".."
            const Zchar* const shortName = Trav::getShortName(fileInfo);
            if (shortName[0] == L'.' &&
            (shortName[1] == 0 || (shortName[1] == L'.' && shortName[2] == 0)))
                continue;

            const Zstring& fullName = endsWith(directory, FILE_NAME_SEPARATOR) ?
            directory + shortName :
            directory + FILE_NAME_SEPARATOR + shortName;

            if (Trav::isSymlink(fileInfo) && !followSymlinks_) //evaluate symlink directly
            {
                TraverseCallback::SymlinkInfo details;
                try
                {
                    details.targetPath = getSymlinkRawTargetString(fullName); //throw FileError
                }
#ifdef NDEBUG //Release
                catch (FileError&) {}
#else
                catch (FileError& e) { sink.onError(e.toString()); } //show broken symlink / access errors in debug build!
#endif

                details.lastWriteTimeRaw = Trav::getModTime (fileInfo);
                details.dirLink          = Trav::isDirectory(fileInfo); //directory symlinks have this flag on Windows
                sink.onSymlink(shortName, fullName, details);
            }
            else if (Trav::isDirectory(fileInfo)) //a directory... or symlink that needs to be followed (for directory symlinks this flag is set too!)
            {
                if (const std::shared_ptr<TraverseCallback> rv = sink.onDir(shortName, fullName))
                    traverse<Trav>(fullName, *rv, level + 1);
            }
            else //a file or symlink that is followed...
            {
                TraverseCallback::FileInfo details;

                if (Trav::isSymlink(fileInfo)) //dereference symlinks!
                {
                    extractFileInfoFromSymlink(fullName, details); //try to...
                    //keep details initial if symlink is broken
                }
                else
                {
                    Trav::extractFileInfo(fileInfo, volumeSerial, details);

                    //####################################### DST hack ###########################################
                    if (isFatFileSystem)
                    {
                        const dst::RawTime rawTime(Trav::getCreateTimeRaw(fileInfo), Trav::getModTimeRaw(fileInfo));

                        if (dst::fatHasUtcEncoded(rawTime)) //throw (std::runtime_error)
                            details.lastWriteTimeRaw = toTimeT(dst::fatDecodeUtcTime(rawTime)); //return real UTC time; throw (std::runtime_error)
                        else
                            markForDstHack.push_back(std::make_pair(fullName, Trav::getModTimeRaw(fileInfo)));
                    }
                    //####################################### DST hack ###########################################
                }

                sink.onFile(shortName, fullName, details);
            }
        }
    }


    //####################################### DST hack ###########################################
    void applyDstHack(zen::DstHackCallback& dstCallback)
    {
        int failedAttempts  = 0;
        int filesToValidate = 50; //don't let data verification become a performance issue

        for (FilenameTimeList::const_iterator i = markForDstHack.begin(); i != markForDstHack.end(); ++i)
        {
            if (failedAttempts >= 10) //some cloud storages don't support changing creation/modification times => don't waste (a lot of) time trying to
                return;

            dstCallback.requestUiRefresh(i->first);

            const dst::RawTime encodedTime = dst::fatEncodeUtcTime(i->second); //throw (std::runtime_error)
            {
                //may need to remove the readonly-attribute (e.g. FAT usb drives)
                FileUpdateHandle updateHandle(i->first, [=]
                {
                    return ::CreateFile(zen::applyLongPathPrefix(i->first).c_str(),
                    GENERIC_READ | GENERIC_WRITE, //use both when writing over network, see comment in file_io.cpp
                    FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                    0,
                    OPEN_EXISTING,
                    FILE_FLAG_BACKUP_SEMANTICS,
                    nullptr);
                });
                if (updateHandle.get() == INVALID_HANDLE_VALUE)
                {
                    ++failedAttempts;
                    assert(false); //don't throw exceptions due to dst hack here
                    continue;
                }

                if (!::SetFileTime(updateHandle.get(),
                                   &encodedTime.createTimeRaw,
                                   nullptr,
                                   &encodedTime.writeTimeRaw))
                {
                    ++failedAttempts;
                    assert(false); //don't throw exceptions due to dst hack here
                    continue;
                }
            }

            //even at this point it's not sure whether data was written correctly, again cloud storages tend to lie about success status
            if (filesToValidate > 0)
            {
                --filesToValidate; //don't change during check!

                //dst hack: verify data written; attention: this check may fail for "sync.ffs_lock"
                WIN32_FILE_ATTRIBUTE_DATA debugeAttr = {};
                ::GetFileAttributesEx(zen::applyLongPathPrefix(i->first).c_str(), //__in   LPCTSTR lpFileName,
                                      GetFileExInfoStandard,                //__in   GET_FILEEX_INFO_LEVELS fInfoLevelId,
                                      &debugeAttr);                         //__out  LPVOID lpFileInformation

                if (::CompareFileTime(&debugeAttr.ftCreationTime,  &encodedTime.createTimeRaw) != 0 ||
                    ::CompareFileTime(&debugeAttr.ftLastWriteTime, &encodedTime.writeTimeRaw)  != 0)
                {
                    ++failedAttempts;
                    assert(false); //don't throw exceptions due to dst hack here
                    continue;
                }
            }
        }
    }

    const bool isFatFileSystem;
    typedef std::vector<std::pair<Zstring, FILETIME> > FilenameTimeList;
    FilenameTimeList markForDstHack;
    //####################################### DST hack ###########################################

    const bool followSymlinks_;
    const DWORD volumeSerial; //may be 0!
};


#elif defined FFS_LINUX
class DirTraverser
{
public:
    DirTraverser(const Zstring& baseDirectory, bool followSymlinks, zen::TraverseCallback& sink, zen::DstHackCallback* dstCallback) :
        followSymlinks_(followSymlinks)
    {
        const Zstring directoryFormatted = //remove trailing slash
            baseDirectory.size() > 1 && endsWith(baseDirectory, FILE_NAME_SEPARATOR) ?  //exception: allow '/'
            beforeLast(baseDirectory, FILE_NAME_SEPARATOR) :
            baseDirectory;

        /* quote: "Since POSIX.1 does not specify the size of the d_name field, and other nonstandard fields may precede
                   that field within the dirent structure, portable applications that use readdir_r() should allocate
                   the buffer whose address is passed in entry as follows:
                       len = offsetof(struct dirent, d_name) + pathconf(dirpath, _PC_NAME_MAX) + 1
                       entryp = malloc(len); */
        const long maxPath = std::max<long>(::pathconf(directoryFormatted.c_str(), _PC_NAME_MAX), 10000); //::pathconf may return -1
        buffer.resize(offsetof(struct ::dirent, d_name) + maxPath + 1);

        traverse(directoryFormatted, sink, 0);
    }

private:
    DirTraverser(const DirTraverser&);
    DirTraverser& operator=(const DirTraverser&);

    void traverse(const Zstring& directory, zen::TraverseCallback& sink, int level)
    {
        tryReportingError([&]
        {
            if (level == 100) //notify endless recursion
                throw FileError(_("Endless loop when traversing directory:") + L"\n\"" + directory + L"\"");
        }, sink);


        DIR* dirObj = nullptr;
        tryReportingError([&]
        {
            dirObj = ::opendir(directory.c_str()); //directory must NOT end with path separator, except "/"
            if (!dirObj)
                throw FileError(_("Error traversing directory:") + L"\n\"" + directory + L"\"" + L"\n\n" + zen::getLastErrorFormatted());
        }, sink);

        if (!dirObj)
            return; //ignored error
        ZEN_ON_SCOPE_EXIT(::closedir(dirObj)); //never close nullptr handles! -> crash

        while (true)
        {
            struct ::dirent* dirEntry = nullptr;
            tryReportingError([&]
            {
                if (::readdir_r(dirObj, reinterpret_cast< ::dirent*>(&buffer[0]), &dirEntry) != 0)
                    throw FileError(_("Error traversing directory:") + L"\n\"" + directory + L"\"" + L"\n\n" + zen::getLastErrorFormatted());
            }, sink);
            if (!dirEntry) //no more items or ignore error
                return;


            //don't return "." and ".."
            const char* const shortName = dirEntry->d_name; //evaluate dirEntry *before* going into recursion => we use a single "buffer"!
            if (shortName[0] == '.' &&
                (shortName[1] == 0 || (shortName[1] == '.' && shortName[2] == 0)))
                continue;

            const Zstring& fullName = endsWith(directory, FILE_NAME_SEPARATOR) ? //e.g. "/"
                                      directory + shortName :
                                      directory + FILE_NAME_SEPARATOR + shortName;

            struct ::stat fileInfo = {};
            bool haveData = false;
            tryReportingError([&]
            {
                if (::lstat(fullName.c_str(), &fileInfo) != 0) //lstat() does not resolve symlinks
                    throw FileError(_("Error reading file attributes:") + L"\n\"" + fullName + L"\"" + L"\n\n" + zen::getLastErrorFormatted());
                haveData = true;
            }, sink);
            if (!haveData)
                continue; //ignore error: skip file

            if (S_ISLNK(fileInfo.st_mode))
            {
                if (followSymlinks_) //on Linux Symlinks need to be followed to evaluate whether they point to a file or directory
                {
                    if (::stat(fullName.c_str(), &fileInfo) != 0) //stat() resolves symlinks
                    {
                        sink.onFile(shortName, fullName, TraverseCallback::FileInfo()); //report broken symlink as file!
                        continue;
                    }

                    fileInfo.st_dev = 0; //id from dereferenced symlink is problematic, since renaming will consider the link, not the target!
                    fileInfo.st_ino = 0; //
                }
                else //evaluate symlink directly
                {
                    TraverseCallback::SymlinkInfo details;
                    try
                    {
                        details.targetPath = getSymlinkRawTargetString(fullName); //throw FileError
                    }
                    catch (FileError& e)
                    {
#ifndef NDEBUG       //show broken symlink / access errors in debug build!
                        sink.onError(e.toString());
#endif
                    }

                    details.lastWriteTimeRaw = fileInfo.st_mtime; //UTC time(ANSI C format); unit: 1 second
                    details.dirLink          = ::stat(fullName.c_str(), &fileInfo) == 0 && S_ISDIR(fileInfo.st_mode);
                    //S_ISDIR and S_ISLNK are mutually exclusive on Linux => explicitly need to follow link
                    sink.onSymlink(shortName, fullName, details);
                    continue;
                }
            }

            //fileInfo contains dereferenced data in any case from here on

            if (S_ISDIR(fileInfo.st_mode)) //a directory... cannot be a symlink on Linux in this case
            {
                const std::shared_ptr<TraverseCallback> rv = sink.onDir(shortName, fullName);
                if (rv)
                    traverse(fullName, *rv, level + 1);
            }
            else //a file... (or symlink; pathological!)
            {
                TraverseCallback::FileInfo details;
                details.fileSize         = zen::UInt64(fileInfo.st_size);
                details.lastWriteTimeRaw = fileInfo.st_mtime; //UTC time (time_t format); unit: 1 second
                details.id               = extractFileID(fileInfo);

                sink.onFile(shortName, fullName, details);
            }
        }
    }

    std::vector<char> buffer;
    const bool followSymlinks_;
};
#endif
}


void zen::traverseFolder(const Zstring& directory, bool followSymlinks, TraverseCallback& sink, DstHackCallback* dstCallback)
{
    DirTraverser(directory, followSymlinks, sink, dstCallback);
}
