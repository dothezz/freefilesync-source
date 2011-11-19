// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) 2008-2011 ZenJu (zhnmju123 AT gmx.de)                    *
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

#elif defined FFS_LINUX
#include <sys/stat.h>
#include <dirent.h>
#include <cerrno>
#endif

using namespace zen;


namespace
{
//implement "retry" in a generic way:

template <class Command> inline //function object expecting to throw FileError if operation fails
void tryReportingError(Command cmd, zen::TraverseCallback& callback)
{
    for (;;)
        try
        {
            cmd();
            return;
        }
        catch (const FileError& e)
        {
            switch (callback.onError(e.toString()))
            {
                case TraverseCallback::TRAV_ERROR_RETRY:
                    break;
                case TraverseCallback::TRAV_ERROR_IGNORE:
                    return;
                default:
                    assert(false);
                    break;
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
                                NULL);
    if (hFile == INVALID_HANDLE_VALUE)
        return false;
    ZEN_ON_BLOCK_EXIT(::CloseHandle(hFile));

    BY_HANDLE_FILE_INFORMATION fileInfoByHandle = {};
    if (!::GetFileInformationByHandle(hFile, &fileInfoByHandle))
        return false;

    //write output
    output.fileSize         = zen::UInt64(fileInfoByHandle.nFileSizeLow, fileInfoByHandle.nFileSizeHigh);
    output.lastWriteTimeRaw = toTimeT(fileInfoByHandle.ftLastWriteTime);
    //output.id               = extractFileID(fileInfoByHandle); -> id from dereferenced symlink is problematic, since renaming will consider the link, not the target!
    return true;
}


DWORD retrieveVolumeSerial(const Zstring& pathName) //return 0 on error!
{
    //note: this even works for network shares: \\share\dirname

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
                                NULL,               //__out      LPTSTR lpVolumeNameBuffer,
                                0,                  //__in       DWORD nVolumeNameSize,
                                &volumeSerial,      //__out_opt  LPDWORD lpVolumeSerialNumber,
                                NULL,               //__out_opt  LPDWORD lpMaximumComponentLength,
                                NULL,               //__out_opt  LPDWORD lpFileSystemFlags,
                                NULL,               //__out      LPTSTR lpFileSystemNameBuffer,
                                0))                 //__in       DWORD nFileSystemNameSize
        return 0;

    return volumeSerial;
}


const DllFun<findplus::OpenDirFunc>  openDir (findplus::getDllName(), findplus::openDirFuncName ); //
const DllFun<findplus::ReadDirFunc>  readDir (findplus::getDllName(), findplus::readDirFuncName ); //load at startup: avoid pre C++11 static initialization MT issues
const DllFun<findplus::CloseDirFunc> closeDir(findplus::getDllName(), findplus::closeDirFuncName); //


/*
Common C-style interface for Win32 FindFirstFile(), FindNextFile() and FileFilePlus openDir(), closeDir():
struct X //see "policy based design"
{
typedef ... Handle;
typedef ... FindData;
static Handle create(const Zstring& directoryPf, FindData& fileInfo); //throw FileError
static void destroy(Handle hnd); //throw()
static bool next(Handle hnd, const Zstring& directory, WIN32_FIND_DATA& fileInfo) //throw FileError

//helper routines
static void extractFileInfo            (const FindData& fileInfo, const DWORD* volumeSerial, TraverseCallback::FileInfo& output);
static Int64 getModTime                (const FindData& fileInfo);
static const FILETIME& getModTimeRaw   (const FindData& fileInfo); //yet another concession to DST hack
static const FILETIME& getCreateTimeRaw(const FindData& fileInfo); //
static const wchar_t* getShortName     (const FindData& fileInfo);
static bool isDirectory                (const FindData& fileInfo);
static bool isSymlink                  (const FindData& fileInfo);
}

Note: Win32 FindFirstFile(), FindNextFile() is a weaker abstraction than FileFilePlus openDir(), readDir(), closeDir() and Unix opendir(), closedir(), stat(),
      so unfortunately we have to use former as a greatest common divisor
*/


struct Win32Traverser
{
    typedef HANDLE Handle;
    typedef WIN32_FIND_DATA FindData;

    static Handle create(const Zstring& directory, FindData& fileInfo) //throw FileError
    {
        const Zstring& directoryPf = endsWith(directory, FILE_NAME_SEPARATOR) ?
                                     directory :
                                     directory + FILE_NAME_SEPARATOR;

        HANDLE output = ::FindFirstFile(applyLongPathPrefix(directoryPf + L'*').c_str(), &fileInfo);
        //no noticable performance difference compared to FindFirstFileEx with FindExInfoBasic, FIND_FIRST_EX_CASE_SENSITIVE and/or FIND_FIRST_EX_LARGE_FETCH
        if (output == INVALID_HANDLE_VALUE)
            throw FileError(_("Error traversing directory:") + L"\n\"" + directory + L"\"" + L"\n\n" + zen::getLastErrorFormatted());
        //::GetLastError() == ERROR_FILE_NOT_FOUND -> actually NOT okay, even for an empty directory this should not occur (., ..)
        return output;
    }

    static void destroy(Handle hnd) { ::FindClose(hnd); } //throw()

    static bool next(Handle hnd, const Zstring& directory, FindData& fileInfo) //throw FileError
    {
        if (!::FindNextFile(hnd, &fileInfo))
        {
            if (::GetLastError() == ERROR_NO_MORE_FILES) //not an error situation
                return false;
            //else we have a problem... report it:
            throw FileError(_("Error traversing directory:") + L"\n\"" + directory + L"\"" + L"\n\n" + zen::getLastErrorFormatted());
        }
        return true;
    }

    //helper routines
    template <class FindData>
    static void extractFileInfo(const FindData& fileInfo, const DWORD* volumeSerial, TraverseCallback::FileInfo& output)
    {
        output.lastWriteTimeRaw = getModTime(fileInfo);
        output.fileSize         = UInt64(fileInfo.nFileSizeLow, fileInfo.nFileSizeHigh);
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
    typedef findplus::FindHandle Handle;
    typedef findplus::FileInformation FindData;

    static Handle create(const Zstring& directory, FindData& fileInfo) //throw FileError
    {
        Handle output = ::openDir(applyLongPathPrefix(directory).c_str());
        if (output == NULL)
            throw FileError(_("Error traversing directory:") + L"\n\"" + directory + L"\"" + L"\n\n" + zen::getLastErrorFormatted());

        bool rv = next(output, directory, fileInfo); //throw FileError
        if (!rv) //we expect at least two successful reads, even for an empty directory: ., ..
            throw FileError(_("Error traversing directory:") + L"\n\"" + directory + L"\"" + L"\n\n" + zen::getLastErrorFormatted(ERROR_NO_MORE_FILES));

        return output;
    }

    static void destroy(Handle hnd) { ::closeDir(hnd); } //throw()

    static bool next(Handle hnd, const Zstring& directory, FindData& fileInfo) //throw FileError
    {
        if (!::readDir(hnd, fileInfo))
        {
            if (::GetLastError() == ERROR_NO_MORE_FILES) //not an error situation
                return false;
            //else we have a problem... report it:
            throw FileError(_("Error traversing directory:") + L"\n\"" + directory + L"\"" + L"\n\n" + zen::getLastErrorFormatted());
        }
        return true;
    }

    //helper routines
    template <class FindData>
    static void extractFileInfo(const FindData& fileInfo, const DWORD* volumeSerial, TraverseCallback::FileInfo& output)
    {
        output.fileSize         = UInt64(fileInfo.fileSize.QuadPart);
        output.lastWriteTimeRaw = getModTime(fileInfo);
        if (volumeSerial)
            output.id = extractFileID(*volumeSerial, fileInfo.fileId);
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
        catch (...) {} //don't cause issues in user mode

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

        typename Trav::FindData fileInfo = {};

        typename Trav::Handle searchHandle = 0;

        tryReportingError([&]
        {
            typedef Trav Trav; //f u VS!
            searchHandle = Trav::create(directory, fileInfo); //throw FileError
        }, sink);

        if (searchHandle == 0)
            return; //ignored error
        ZEN_ON_BLOCK_EXIT(typedef Trav Trav; Trav::destroy(searchHandle));

        do
        {
            //don't return "." and ".."
            const Zchar* const shortName = Trav::getShortName(fileInfo);
            if (shortName[0] == L'.' &&
                (shortName[1] == L'\0' || (shortName[1] == L'.' && shortName[2] == L'\0')))
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
                const std::shared_ptr<TraverseCallback> rv = sink.onDir(shortName, fullName);
                if (rv)
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
                    Trav::extractFileInfo(fileInfo, volumeSerial != 0 ? &volumeSerial : nullptr, details); //make optional character of volumeSerial explicit in the interface

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
        while ([&]() -> bool
    {
        bool moreData = false;

        typedef Trav Trav1; //f u VS!
        tryReportingError([&]
        {
            typedef Trav1 Trav; //f u VS!
            moreData = Trav::next(searchHandle, directory, fileInfo); //throw FileError
            }, sink);

            return moreData;
        }());
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
                FileUpdateHandle updateHandle(i->first, [=]()
                {
                    return ::CreateFile(zen::applyLongPathPrefix(i->first).c_str(),
                                        GENERIC_READ | GENERIC_WRITE, //use both when writing over network, see comment in file_io.cpp
                                        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                                        0,
                                        OPEN_EXISTING,
                                        FILE_FLAG_BACKUP_SEMANTICS,
                                        NULL);
                });
                if (updateHandle.get() == INVALID_HANDLE_VALUE)
                {
                    ++failedAttempts;
                    assert(false); //don't throw exceptions due to dst hack here
                    continue;
                }

                if (!::SetFileTime(updateHandle.get(),
                                   &encodedTime.createTimeRaw,
                                   NULL,
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


        DIR* dirObj = NULL;
        tryReportingError([&]
        {
            dirObj = ::opendir(directory.c_str()); //directory must NOT end with path separator, except "/"
            if (dirObj == NULL)
                throw FileError(_("Error traversing directory:") + L"\n\"" + directory + L"\"" + L"\n\n" + zen::getLastErrorFormatted());
        }, sink);

        if (dirObj == NULL)
            return; //ignored error
        ZEN_ON_BLOCK_EXIT(::closedir(dirObj)); //never close NULL handles! -> crash

        while (true)
        {
            struct ::dirent* dirEntry = NULL;
            tryReportingError([&]
            {
                if (::readdir_r(dirObj, reinterpret_cast< ::dirent*>(&buffer[0]), &dirEntry) != 0)
                    throw FileError(_("Error traversing directory:") + L"\n\"" + directory + L"\"" + L"\n\n" + zen::getLastErrorFormatted());
            }, sink);
            if (dirEntry == NULL) //no more items or ignore error
                return;


            //don't return "." and ".."
            const char* const shortName = dirEntry->d_name; //evaluate dirEntry *before* going into recursion => we use a single "buffer"!
            if (shortName[0] == '.' &&
                (shortName[1] == '\0' || (shortName[1] == '.' && shortName[2] == '\0')))
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


bool zen::supportForFileId() //Linux: always; Windows: if FindFilePlus_Win32.dll was loaded correctly
{
#ifdef FFS_WIN
    return ::openDir && ::readDir && ::closeDir;

#elif defined FFS_LINUX
    return true;
#endif
}
