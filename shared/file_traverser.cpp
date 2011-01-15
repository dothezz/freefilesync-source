// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) 2008-2011 ZenJu (zhnmju123 AT gmx.de)                    *
// **************************************************************************
//
#include "file_traverser.h"
#include <limits>
#include "system_constants.h"
#include "system_func.h"
#include <wx/intl.h>
#include "string_conv.h"
#include "assert_static.h"
#include "symlink_target.h"

#ifdef FFS_WIN
#include <wx/msw/wrapwin.h> //includes "windows.h"
#include "long_path_prefix.h"
#include "dst_hack.h"

#elif defined FFS_LINUX
#include <sys/stat.h>
#include <dirent.h>
#include <cerrno>
#endif


#ifdef FFS_WIN
inline
wxLongLong getWin32TimeInformation(const FILETIME& lastWriteTime)
{
    //convert UTC FILETIME to ANSI C format (number of seconds since Jan. 1st 1970 UTC)
    wxLongLong writeTimeLong(lastWriteTime.dwHighDateTime, lastWriteTime.dwLowDateTime);
    writeTimeLong /= 10000000;                    //reduce precision to 1 second (FILETIME has unit 10^-7 s)
    writeTimeLong -= wxLongLong(2, 3054539008UL); //timeshift between ansi C time and FILETIME in seconds == 11644473600s

    assert(lastWriteTime.dwHighDateTime <= static_cast<unsigned long>(std::numeric_limits<long>::max()));
    assert_static(sizeof(DWORD) == sizeof(long));
    assert_static(sizeof(long) == 4);
    return writeTimeLong;
}


inline
void setWin32FileInformation(const FILETIME& lastWriteTime,
                             const DWORD fileSizeHigh,
                             const DWORD fileSizeLow,
                             ffs3::TraverseCallback::FileInfo& output)
{
    output.lastWriteTimeRaw = getWin32TimeInformation(lastWriteTime);
    output.fileSize = wxULongLong(fileSizeHigh, fileSizeLow);
}


inline
bool setWin32FileInformationFromSymlink(const Zstring& linkName, ffs3::TraverseCallback::FileInfo& output)
{
    //open handle to target of symbolic link
    HANDLE hFile = ::CreateFile(ffs3::applyLongPathPrefix(linkName).c_str(),
                                0,
                                FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                                NULL,
                                OPEN_EXISTING,
                                FILE_FLAG_BACKUP_SEMANTICS,
                                NULL);
    if (hFile == INVALID_HANDLE_VALUE)
        return false;

    boost::shared_ptr<void> dummy(hFile, ::CloseHandle);

    BY_HANDLE_FILE_INFORMATION fileInfoByHandle;
    if (!::GetFileInformationByHandle(hFile, &fileInfoByHandle))
        return false;

    //write output
    setWin32FileInformation(fileInfoByHandle.ftLastWriteTime, fileInfoByHandle.nFileSizeHigh, fileInfoByHandle.nFileSizeLow, output);
    return true;
}
#endif


class DirTraverser
{
public:
    DirTraverser(const Zstring& baseDirectory, bool followSymlinks, ffs3::TraverseCallback& sink, ffs3::DstHackCallback* dstCallback)
#ifdef FFS_WIN
        : isFatFileSystem(dst::isFatDrive(baseDirectory))
#endif
    {
        //format base directory name
#ifdef FFS_WIN
        const Zstring& directoryFormatted = baseDirectory;

#elif defined FFS_LINUX
        const Zstring directoryFormatted = //remove trailing slash
            baseDirectory.size() > 1 && baseDirectory.EndsWith(common::FILE_NAME_SEPARATOR) ?  //exception: allow '/'
            baseDirectory.BeforeLast(common::FILE_NAME_SEPARATOR) :
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
    template <bool followSymlinks>
    void traverse(const Zstring& directory, ffs3::TraverseCallback& sink, int level)
    {
        using namespace ffs3;

        if (level == 100) //catch endless recursion
        {
            sink.onError(wxString(_("Endless loop when traversing directory:")) + wxT("\n\"") + zToWx(directory) + wxT("\""));
            return;
        }

#ifdef FFS_WIN
        //ensure directoryFormatted ends with backslash
        const Zstring& directoryFormatted = directory.EndsWith(common::FILE_NAME_SEPARATOR) ?
                                            directory :
                                            directory + common::FILE_NAME_SEPARATOR;

        WIN32_FIND_DATA fileMetaData;
        HANDLE searchHandle = ::FindFirstFile(applyLongPathPrefix(directoryFormatted + Zchar('*')).c_str(), //__in   LPCTSTR lpFileName
                                              &fileMetaData);                                                     //__out  LPWIN32_FIND_DATA lpFindFileData
        //no noticable performance difference compared to FindFirstFileEx with FindExInfoBasic, FIND_FIRST_EX_CASE_SENSITIVE and/or FIND_FIRST_EX_LARGE_FETCH

        if (searchHandle == INVALID_HANDLE_VALUE)
        {
            const DWORD lastError = ::GetLastError();
            if (lastError == ERROR_FILE_NOT_FOUND)
                return;

            //else: we have a problem... report it:
            const wxString errorMessage = wxString(_("Error traversing directory:")) + wxT("\n\"") + zToWx(directory) + wxT("\"") + wxT("\n\n") +
                                          ffs3::getLastErrorFormatted(lastError);

            sink.onError(errorMessage);
            return;
        }

        boost::shared_ptr<void> dummy(searchHandle, ::FindClose);

        do
        {
            //don't return "." and ".."
            const Zchar* const shortName = fileMetaData.cFileName;
            if (     shortName[0] == Zstr('.') &&
                     ((shortName[1] == Zstr('.') && shortName[2] == Zstr('\0')) ||
                      shortName[1] == Zstr('\0')))
                continue;

            const Zstring& fullName = directoryFormatted + shortName;

            const bool isSymbolicLink = (fileMetaData.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT) != 0;

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

                details.lastWriteTimeRaw = getWin32TimeInformation(fileMetaData.ftLastWriteTime);
                details.dirLink          = (fileMetaData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0; //directory symlinks have this flag on Windows
                sink.onSymlink(shortName, fullName, details);
            }
            else if (fileMetaData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) //a directory... or symlink that needs to be followed (for directory symlinks this flag is set too!)
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
                        details.fileSize         = 0;
                    }
                }
                else
                {
//####################################### DST hack ###########################################
                    if (isFatFileSystem)
                    {
                        const dst::RawTime rawTime(fileMetaData.ftCreationTime, fileMetaData.ftLastWriteTime);

                        if (dst::fatHasUtcEncoded(rawTime)) //throw (std::runtime_error)
                            fileMetaData.ftLastWriteTime = dst::fatDecodeUtcTime(rawTime); //return real UTC time; throw (std::runtime_error)
                        else
                            markForDstHack.push_back(std::make_pair(fullName, fileMetaData.ftLastWriteTime));
                    }
//####################################### DST hack ###########################################
                    setWin32FileInformation(fileMetaData.ftLastWriteTime, fileMetaData.nFileSizeHigh, fileMetaData.nFileSizeLow, details);
                }

                sink.onFile(shortName, fullName, details);
            }
        }
        while (::FindNextFile(searchHandle,	   // handle to search
                              &fileMetaData)); // pointer to structure for data on found file

        const DWORD lastError = ::GetLastError();
        if (lastError != ERROR_NO_MORE_FILES) //this is fine
        {
            //else we have a problem... report it:
            const wxString errorMessage = wxString(_("Error traversing directory:")) + wxT("\n\"") + zToWx(directory) + wxT("\"") ;
            sink.onError(errorMessage + wxT("\n\n") + ffs3::getLastErrorFormatted(lastError));
        }

#elif defined FFS_LINUX
        DIR* dirObj = ::opendir(directory.c_str()); //directory must NOT end with path separator, except "/"
        if (dirObj == NULL)
        {
            const wxString errorMessage = wxString(_("Error traversing directory:")) + wxT("\n\"") + zToWx(directory) + wxT("\"") ;
            sink.onError(errorMessage + wxT("\n\n") + ffs3::getLastErrorFormatted());
            return;
        }

        boost::shared_ptr<DIR> dummy(dirObj, &::closedir); //never close NULL handles! -> crash

        while (true)
        {
            errno = 0; //set errno to 0 as unfortunately this isn't done when readdir() returns NULL because it can't find any files
            struct dirent* dirEntry = ::readdir(dirObj);
            if (dirEntry == NULL)
            {
                if (errno == 0)
                    return; //everything okay

                //else: we have a problem... report it:
                const wxString errorMessage = wxString(_("Error traversing directory:")) + wxT("\n\"") + zToWx(directory) + wxT("\"") ;
                sink.onError(errorMessage + wxT("\n\n") + ffs3::getLastErrorFormatted());
                return;
            }

            //don't return "." and ".."
            const Zchar* const shortName = dirEntry->d_name;
            if (      shortName[0] == Zstr('.') &&
                      ((shortName[1] == Zstr('.') && shortName[2] == Zstr('\0')) ||
                       shortName[1] == Zstr('\0')))
                continue;

            const Zstring& fullName = directory.EndsWith(common::FILE_NAME_SEPARATOR) ? //e.g. "/"
                                      directory + shortName :
                                      directory + common::FILE_NAME_SEPARATOR + shortName;

            struct stat fileInfo;
            if (::lstat(fullName.c_str(), &fileInfo) != 0) //lstat() does not resolve symlinks
            {
                const wxString errorMessage = wxString(_("Error reading file attributes:")) + wxT("\n\"") + zToWx(fullName) + wxT("\"");
                sink.onError(errorMessage + wxT("\n\n") + ffs3::getLastErrorFormatted());
                continue;
            }

            const bool isSymbolicLink = S_ISLNK(fileInfo.st_mode);

            if (isSymbolicLink)
            {
                if (followSymlinks) //on Linux Symlinks need to be followed to evaluate whether they point to a file or directory
                {
                    if (::stat(fullName.c_str(), &fileInfo) != 0) //stat() resolves symlinks
                    {
                        //a broken symbolic link
                        TraverseCallback::FileInfo details;
                        details.lastWriteTimeRaw = 0; //we are not interested in the modifiation time of the link
                        details.fileSize         = 0;
                        sink.onFile(shortName, fullName, details); //report broken symlink as file!
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
                details.fileSize         = fileInfo.st_size;

                sink.onFile(shortName, fullName, details);
            }
        }
#endif
    }


#ifdef FFS_WIN
//####################################### DST hack ###########################################
    void applyDstHack(ffs3::DstHackCallback& dstCallback)
    {
        for (FilenameTimeList::const_iterator i = markForDstHack.begin(); i != markForDstHack.end(); ++i)
        {
            dstCallback.requestUiRefresh(i->first);

            HANDLE hTarget = ::CreateFile(ffs3::applyLongPathPrefix(i->first).c_str(),
                                          FILE_WRITE_ATTRIBUTES,
                                          FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                                          NULL,
                                          OPEN_EXISTING,
                                          FILE_FLAG_BACKUP_SEMANTICS,
                                          NULL);
            if (hTarget == INVALID_HANDLE_VALUE)
                assert(false); //don't throw exceptions due to dst hack here
            else
            {
                boost::shared_ptr<void> dummy2(hTarget, ::CloseHandle);

                const dst::RawTime encodedTime = dst::fatEncodeUtcTime(i->second); //throw (std::runtime_error)

                if (!::SetFileTime(hTarget,
                                   &encodedTime.createTimeRaw,
                                   NULL,
                                   &encodedTime.writeTimeRaw))
                    assert(false); //don't throw exceptions due to dst hack here

#ifndef NDEBUG //dst hack: verify data written; attention: this check may fail for "sync.ffs_lock"
                WIN32_FILE_ATTRIBUTE_DATA debugeAttr = {};
                assert(::GetFileAttributesEx(ffs3::applyLongPathPrefix(i->first).c_str(), //__in   LPCTSTR lpFileName,
                                             GetFileExInfoStandard,                 //__in   GET_FILEEX_INFO_LEVELS fInfoLevelId,
                                             &debugeAttr));                         //__out  LPVOID lpFileInformation

                assert(::CompareFileTime(&debugeAttr.ftCreationTime,  &encodedTime.createTimeRaw) == 0);
                assert(::CompareFileTime(&debugeAttr.ftLastWriteTime, &encodedTime.writeTimeRaw)  == 0);
#endif
            }
        }
    }

    const bool isFatFileSystem;
    typedef std::vector<std::pair<Zstring, FILETIME> > FilenameTimeList;
    FilenameTimeList markForDstHack;
//####################################### DST hack ###########################################
#endif
};


void ffs3::traverseFolder(const Zstring& directory, bool followSymlinks, TraverseCallback& sink, DstHackCallback* dstCallback)
{
    DirTraverser(directory, followSymlinks, sink, dstCallback);
}
