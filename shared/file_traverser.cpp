// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) 2008-2011 ZenJu (zhnmju123 AT gmx.de)                    *
// **************************************************************************

#include "file_traverser.h"
#include <limits>
#include "last_error.h"
#include "assert_static.h"
#include "symlink_target.h"

#ifdef FFS_WIN
#include <wx/msw/wrapwin.h> //includes "windows.h"
#include "long_path_prefix.h"
#include "dst_hack.h"
#include "file_update_handle.h"

#elif defined FFS_LINUX
#include <sys/stat.h>
#include <dirent.h>
#include <cerrno>
#endif

using namespace zen;


namespace
{
//implement "retry" in a generic way:

//returns "true" if "cmd" was invoked successfully, "false" if error occured and was ignored
template <class Command> inline //function object with bool operator()(std::wstring& errorMsg), returns "true" on success, "false" on error and writes "errorMsg" in this case
bool tryReportingError(Command cmd, zen::TraverseCallback& callback)
{
    for (;;)
    {
        std::wstring errorMsg;
        if (cmd(errorMsg))
            return true;

        switch (callback.onError(errorMsg))
        {
            case TraverseCallback::TRAV_ERROR_RETRY:
                break;
            case TraverseCallback::TRAV_ERROR_IGNORE:
                return false;
            default:
                assert(false);
                break;
        }
    }
}


#ifdef FFS_WIN
inline
zen::Int64 filetimeToTimeT(const FILETIME& lastWriteTime)
{
    //convert UTC FILETIME to ANSI C format (number of seconds since Jan. 1st 1970 UTC)
    zen::Int64 writeTimeLong = to<zen::Int64>(zen::UInt64(lastWriteTime.dwLowDateTime, lastWriteTime.dwHighDateTime) / 10000000U); //reduce precision to 1 second (FILETIME has unit 10^-7 s)
    writeTimeLong -= zen::Int64(3054539008UL, 2); //timeshift between ansi C time and FILETIME in seconds == 11644473600s
    return writeTimeLong;
}


inline
bool setWin32FileInformationFromSymlink(const Zstring& linkName, zen::TraverseCallback::FileInfo& output)
{
    //open handle to target of symbolic link
    HANDLE hFile = ::CreateFile(zen::applyLongPathPrefix(linkName).c_str(),
                                0,
                                FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                                NULL,
                                OPEN_EXISTING,
                                FILE_FLAG_BACKUP_SEMANTICS,
                                NULL);
    if (hFile == INVALID_HANDLE_VALUE)
        return false;

    Loki::ScopeGuard dummy = Loki::MakeGuard(::CloseHandle, hFile);
    (void)dummy; //silence warning "unused variable"

    BY_HANDLE_FILE_INFORMATION fileInfoByHandle = {};
    if (!::GetFileInformationByHandle(hFile, &fileInfoByHandle))
        return false;

    //write output
    output.lastWriteTimeRaw = filetimeToTimeT(fileInfoByHandle.ftLastWriteTime);
    output.fileSize         = zen::UInt64(fileInfoByHandle.nFileSizeLow, fileInfoByHandle.nFileSizeHigh);
    return true;
}
#endif
}


class DirTraverser
{
public:
    DirTraverser(const Zstring& baseDirectory, bool followSymlinks, zen::TraverseCallback& sink, zen::DstHackCallback* dstCallback)
#ifdef FFS_WIN
        : isFatFileSystem(dst::isFatDrive(baseDirectory))
#endif
    {
        //format base directory name
#ifdef FFS_WIN
        const Zstring& directoryFormatted = baseDirectory;

#elif defined FFS_LINUX
        const Zstring directoryFormatted = //remove trailing slash
            baseDirectory.size() > 1 && baseDirectory.EndsWith(FILE_NAME_SEPARATOR) ?  //exception: allow '/'
            baseDirectory.BeforeLast(FILE_NAME_SEPARATOR) :
            baseDirectory;
#endif

        //traverse directories
        if (followSymlinks)
            traverse<true>(directoryFormatted, sink, 0);
        else
            traverse<false>(directoryFormatted, sink, 0);

        //apply daylight saving time hack AFTER file traversing, to give separate feedback to user
#ifdef FFS_WIN
        if (isFatFileSystem && dstCallback)
            applyDstHack(*dstCallback);
#endif
    }

private:
    DirTraverser(const DirTraverser&);
    DirTraverser& operator=(const DirTraverser&);

    template <bool followSymlinks>
    void traverse(const Zstring& directory, zen::TraverseCallback& sink, int level)
    {
        tryReportingError([&](std::wstring& errorMsg) -> bool
        {
            if (level == 100) //notify endless recursion
            {
                errorMsg = _("Endless loop when traversing directory:") + "\n\"" + directory + "\"";
                return false;
            }
            return true;
        }, sink);


#ifdef FFS_WIN
        //ensure directoryPf ends with backslash
        const Zstring& directoryPf = directory.EndsWith(FILE_NAME_SEPARATOR) ?
                                     directory :
                                     directory + FILE_NAME_SEPARATOR;
        WIN32_FIND_DATA fileInfo = {};

        HANDLE searchHandle = INVALID_HANDLE_VALUE;
        tryReportingError([&](std::wstring& errorMsg) -> bool
        {
            searchHandle = ::FindFirstFile(applyLongPathPrefix(directoryPf + L'*').c_str(), &fileInfo);
            //no noticable performance difference compared to FindFirstFileEx with FindExInfoBasic, FIND_FIRST_EX_CASE_SENSITIVE and/or FIND_FIRST_EX_LARGE_FETCH

            if (searchHandle == INVALID_HANDLE_VALUE)
            {
                const DWORD lastError = ::GetLastError();
                if (lastError == ERROR_FILE_NOT_FOUND)
                    return true; //fine: empty directory

                //else: we have a problem... report it:
                errorMsg = _("Error traversing directory:") + "\n\"" + directory + "\"" + "\n\n" + zen::getLastErrorFormatted(lastError);
                return false;
            }
            return true;
        }, sink);

        if (searchHandle == INVALID_HANDLE_VALUE)
            return; //empty dir or ignore error

        Loki::ScopeGuard dummy = Loki::MakeGuard(::FindClose, searchHandle);
        (void)dummy; //silence warning "unused variable"

        do
        {
            //don't return "." and ".."
            const Zchar* const shortName = fileInfo.cFileName;
            if (shortName[0] == L'.' &&
                (shortName[1] == L'\0' || (shortName[1] == L'.' && shortName[2] == L'\0')))
                continue;

            const Zstring& fullName = directoryPf + shortName;

            const bool isSymbolicLink = (fileInfo.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT) != 0;

            if (isSymbolicLink && !followSymlinks) //evaluate symlink directly
            {
                TraverseCallback::SymlinkInfo details;
                try
                {
                    details.targetPath = getSymlinkRawTargetString(fullName); //throw (FileError)
                }
                catch (FileError& e)
                {
                    (void)e;
#ifndef NDEBUG       //show broken symlink / access errors in debug build!
                    sink.onError(e.msg());
#endif
                }

                details.lastWriteTimeRaw = filetimeToTimeT(fileInfo.ftLastWriteTime);
                details.dirLink          = (fileInfo.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0; //directory symlinks have this flag on Windows
                sink.onSymlink(shortName, fullName, details);
            }
            else if (fileInfo.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) //a directory... or symlink that needs to be followed (for directory symlinks this flag is set too!)
            {
                const TraverseCallback::ReturnValDir rv = sink.onDir(shortName, fullName);
                switch (rv.returnCode)
                {
                    case TraverseCallback::ReturnValDir::TRAVERSING_DIR_IGNORE:
                        break;

                    case TraverseCallback::ReturnValDir::TRAVERSING_DIR_CONTINUE:
                        traverse<followSymlinks>(fullName, *rv.subDirCb, level + 1);
                        break;
                }
            }
            else //a file or symlink that is followed...
            {
                TraverseCallback::FileInfo details;

                if (isSymbolicLink) //dereference symlinks!
                {
                    if (!setWin32FileInformationFromSymlink(fullName, details))
                    {
                        //broken symlink...
                        details.lastWriteTimeRaw = 0; //we are not interested in the modification time of the link
                        details.fileSize         = 0U;
                    }
                }
                else
                {
                    //####################################### DST hack ###########################################
                    if (isFatFileSystem)
                    {
                        const dst::RawTime rawTime(fileInfo.ftCreationTime, fileInfo.ftLastWriteTime);

                        if (dst::fatHasUtcEncoded(rawTime)) //throw (std::runtime_error)
                            fileInfo.ftLastWriteTime = dst::fatDecodeUtcTime(rawTime); //return real UTC time; throw (std::runtime_error)
                        else
                            markForDstHack.push_back(std::make_pair(fullName, fileInfo.ftLastWriteTime));
                    }
                    //####################################### DST hack ###########################################
                    details.lastWriteTimeRaw = filetimeToTimeT(fileInfo.ftLastWriteTime);
                    details.fileSize         = zen::UInt64(fileInfo.nFileSizeLow, fileInfo.nFileSizeHigh);
                }

                sink.onFile(shortName, fullName, details);
            }
        }
        while ([&]() -> bool
    {
        bool moreData = false;

        tryReportingError([&](std::wstring& errorMsg) -> bool
        {
            if (!::FindNextFile(searchHandle, // handle to search
                &fileInfo)) // pointer to structure for data on found file
                {
                    if (::GetLastError() == ERROR_NO_MORE_FILES) //this is fine
                        return true;

                    //else we have a problem... report it:
                    errorMsg = _("Error traversing directory:") + "\n\"" + directory + "\"" + "\n\n" + zen::getLastErrorFormatted();
                    return false;
                }

                moreData = true;
                return true;
            }, sink);

            return moreData;
        }());


#elif defined FFS_LINUX
        DIR* dirObj = NULL;
        if (!tryReportingError([&](std::wstring& errorMsg) -> bool
    {
        dirObj = ::opendir(directory.c_str()); //directory must NOT end with path separator, except "/"
            if (dirObj == NULL)
            {
                errorMsg = _("Error traversing directory:") + "\n\"" + directory + "\"" + "\n\n" + zen::getLastErrorFormatted();
                return false;
            }
            return true;
        }, sink))
        return;

        Loki::ScopeGuard dummy = Loki::MakeGuard(::closedir, dirObj); //never close NULL handles! -> crash
        (void)dummy; //silence warning "unused variable"

        while (true)
        {
            struct dirent* dirEntry = NULL;
            tryReportingError([&](std::wstring& errorMsg) -> bool
            {
                errno = 0; //set errno to 0 as unfortunately this isn't done when readdir() returns NULL because it can't find any files
                dirEntry = ::readdir(dirObj);
                if (dirEntry == NULL)
                {
                    if (errno == 0)
                        return true; //everything okay, not more items

                    //else: we have a problem... report it:
                    errorMsg = _("Error traversing directory:") + "\n\"" + directory + "\"" + "\n\n" + zen::getLastErrorFormatted();
                    return false;
                }
                return true;
            }, sink);
            if (dirEntry == NULL) //no more items or ignore error
                return;


            //don't return "." and ".."
            const char* const shortName = dirEntry->d_name;
            if (shortName[0] == '.' &&
                (shortName[1] == '\0' || (shortName[1] == '.' && shortName[2] == '\0')))
                continue;

            const Zstring& fullName = directory.EndsWith(FILE_NAME_SEPARATOR) ? //e.g. "/"
                                      directory + shortName :
                                      directory + FILE_NAME_SEPARATOR + shortName;

            struct stat fileInfo = {};

            if (!tryReportingError([&](std::wstring& errorMsg) -> bool
        {
            if (::lstat(fullName.c_str(), &fileInfo) != 0) //lstat() does not resolve symlinks
                {
                    errorMsg = _("Error reading file attributes:") + "\n\"" + fullName + "\"" + "\n\n" + zen::getLastErrorFormatted();
                    return false;
                }
                return true;
            }, sink))
            continue; //ignore error: skip file

            const bool isSymbolicLink = S_ISLNK(fileInfo.st_mode);

            if (isSymbolicLink)
            {
                if (followSymlinks) //on Linux Symlinks need to be followed to evaluate whether they point to a file or directory
                {
                    if (::stat(fullName.c_str(), &fileInfo) != 0) //stat() resolves symlinks
                    {
                        sink.onFile(shortName, fullName, TraverseCallback::FileInfo()); //report broken symlink as file!
                        continue;
                    }
                }
                else //evaluate symlink directly
                {
                    TraverseCallback::SymlinkInfo details;
                    try
                    {
                        details.targetPath = getSymlinkRawTargetString(fullName); //throw (FileError)
                    }
                    catch (FileError& e)
                    {
#ifndef NDEBUG       //show broken symlink / access errors in debug build!
                        sink.onError(e.msg());
#endif
                    }

                    details.lastWriteTimeRaw = fileInfo.st_mtime; //UTC time(ANSI C format); unit: 1 second
                    details.dirLink          = ::stat(fullName.c_str(), &fileInfo) == 0 && S_ISDIR(fileInfo.st_mode); //S_ISDIR and S_ISLNK are mutually exclusive on Linux => need to follow link
                    sink.onSymlink(shortName, fullName, details);
                    continue;
                }
            }

            //fileInfo contains dereferenced data in any case from here on

            if (S_ISDIR(fileInfo.st_mode)) //a directory... cannot be a symlink on Linux in this case
            {
                const TraverseCallback::ReturnValDir rv = sink.onDir(shortName, fullName);
                switch (rv.returnCode)
                {
                    case TraverseCallback::ReturnValDir::TRAVERSING_DIR_IGNORE:
                        break;

                    case TraverseCallback::ReturnValDir::TRAVERSING_DIR_CONTINUE:
                        traverse<followSymlinks>(fullName, *rv.subDirCb, level + 1);
                        break;
                }
            }
            else //a file... (or symlink; pathological!)
            {
                TraverseCallback::FileInfo details;
                details.lastWriteTimeRaw = fileInfo.st_mtime; //UTC time(ANSI C format); unit: 1 second
                details.fileSize         = zen::UInt64(fileInfo.st_size);

                sink.onFile(shortName, fullName, details);
            }
        }
#endif
    }


#ifdef FFS_WIN
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
                                        GENERIC_WRITE, //just FILE_WRITE_ATTRIBUTES may not be enough for some NAS shares!
                                        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                                        NULL,
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
#endif
};


void zen::traverseFolder(const Zstring& directory, bool followSymlinks, TraverseCallback& sink, DstHackCallback* dstCallback)
{
#ifdef FFS_WIN
    try //traversing certain folders with restricted permissions requires this privilege! (but copying these files may still fail)
    {
        zen::Privileges::getInstance().ensureActive(SE_BACKUP_NAME); //throw FileError
    }
    catch (...) {} //don't cause issues in user mode
#endif

    DirTraverser(directory, followSymlinks, sink, dstCallback);
}
