// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) 2008-2010 ZenJu (zhnmju123 AT gmx.de)                    *
// **************************************************************************
//
#include "fileTraverser.h"
#include "systemConstants.h"
#include "systemFunctions.h"
#include <wx/intl.h>
#include "stringConv.h"
#include <boost/shared_ptr.hpp>
#include <boost/scoped_array.hpp>

#ifdef FFS_WIN
#include <wx/msw/wrapwin.h> //includes "windows.h"
#include "WinIoCtl.h"
#include "longPathPrefix.h"

#elif defined FFS_LINUX
#include <sys/stat.h>
#include <dirent.h>
#include <cerrno>
#endif


//Note: this class is superfluous for 64 bit applications!
//class DisableWow64Redirection
//{
//public:
//    DisableWow64Redirection() :
//        wow64DisableWow64FsRedirection(Utility::loadDllFunction<Wow64DisableWow64FsRedirectionFunc>(L"kernel32.dll", "Wow64DisableWow64FsRedirection")),
//        wow64RevertWow64FsRedirection(Utility::loadDllFunction<Wow64RevertWow64FsRedirectionFunc>(L"kernel32.dll", "Wow64RevertWow64FsRedirection")),
//        oldValue(NULL)
//    {
//        if (    wow64DisableWow64FsRedirection &&
//                wow64RevertWow64FsRedirection)
//            (*wow64DisableWow64FsRedirection)(&oldValue); //__out  PVOID *OldValue
//    }
//
//    ~DisableWow64Redirection()
//    {
//        if (    wow64DisableWow64FsRedirection &&
//                wow64RevertWow64FsRedirection)
//            (*wow64RevertWow64FsRedirection)(oldValue); //__in  PVOID OldValue
//    }
//
//private:
//    typedef BOOL (WINAPI *Wow64DisableWow64FsRedirectionFunc)(PVOID* OldValue);
//    typedef BOOL (WINAPI *Wow64RevertWow64FsRedirectionFunc)(PVOID OldValue);
//
//    const Wow64DisableWow64FsRedirectionFunc wow64DisableWow64FsRedirection;
//    const Wow64RevertWow64FsRedirectionFunc  wow64RevertWow64FsRedirection;
//
//    PVOID oldValue;
//};

#ifdef _MSC_VER //I don't have Windows Driver Kit at hands right now, so unfortunately we need to redefine this structures and cross fingers...
typedef struct _REPARSE_DATA_BUFFER
{
    ULONG  ReparseTag;
    USHORT ReparseDataLength;
    USHORT Reserved;
    union
    {
        struct
        {
            USHORT SubstituteNameOffset;
            USHORT SubstituteNameLength;
            USHORT PrintNameOffset;
            USHORT PrintNameLength;
            ULONG  Flags;
            WCHAR  PathBuffer[1];
        } SymbolicLinkReparseBuffer;
        struct
        {
            USHORT SubstituteNameOffset;
            USHORT SubstituteNameLength;
            USHORT PrintNameOffset;
            USHORT PrintNameLength;
            WCHAR  PathBuffer[1];
        } MountPointReparseBuffer;
        struct
        {
            UCHAR DataBuffer[1];
        } GenericReparseBuffer;
    };
} REPARSE_DATA_BUFFER, *PREPARSE_DATA_BUFFER;
#define REPARSE_DATA_BUFFER_HEADER_SIZE   FIELD_OFFSET(REPARSE_DATA_BUFFER, GenericReparseBuffer)
#endif


Zstring getSymlinkTarget(const Zstring& linkPath) //throw(); returns empty string on error
{
#ifdef FFS_WIN
//FSCTL_GET_REPARSE_POINT: http://msdn.microsoft.com/en-us/library/aa364571(VS.85).aspx

    const HANDLE hLink = ::CreateFile(linkPath.c_str(),
                                      GENERIC_READ,
                                      FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                                      NULL,
                                      OPEN_EXISTING,
                                      FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OPEN_REPARSE_POINT,
                                      NULL);
    if (hLink == INVALID_HANDLE_VALUE)
        return Zstring();

    boost::shared_ptr<void> dummy(hLink, ::CloseHandle);

    //respect alignment issues...
    const size_t bufferSize = REPARSE_DATA_BUFFER_HEADER_SIZE + MAXIMUM_REPARSE_DATA_BUFFER_SIZE;
    boost::scoped_array<char> buffer(new char[bufferSize]);

    DWORD bytesReturned; //dummy value required by FSCTL_GET_REPARSE_POINT!
    if (!::DeviceIoControl(hLink,                   //__in         HANDLE hDevice,
                           FSCTL_GET_REPARSE_POINT, //__in         DWORD dwIoControlCode,
                           NULL,                    //__in_opt     LPVOID lpInBuffer,
                           0,                       //__in         DWORD nInBufferSize,
                           buffer.get(),            //__out_opt    LPVOID lpOutBuffer,
                           bufferSize,              //__in         DWORD nOutBufferSize,
                           &bytesReturned,          //__out_opt    LPDWORD lpBytesReturned,
                           NULL))                   //__inout_opt  LPOVERLAPPED lpOverlapped
        return Zstring();

    REPARSE_DATA_BUFFER& reparseData = *reinterpret_cast<REPARSE_DATA_BUFFER*>(buffer.get()); //REPARSE_DATA_BUFFER needs to be artificially enlarged!

    if (reparseData.ReparseTag == IO_REPARSE_TAG_SYMLINK)
        return Zstring(reparseData.SymbolicLinkReparseBuffer.PathBuffer + reparseData.SymbolicLinkReparseBuffer.SubstituteNameOffset / sizeof(WCHAR),
                       reparseData.SymbolicLinkReparseBuffer.SubstituteNameLength / sizeof(WCHAR));
    else if (reparseData.ReparseTag == IO_REPARSE_TAG_MOUNT_POINT)
        return Zstring(reparseData.MountPointReparseBuffer.PathBuffer + reparseData.MountPointReparseBuffer.SubstituteNameOffset / sizeof(WCHAR),
                       reparseData.MountPointReparseBuffer.SubstituteNameLength / sizeof(WCHAR));
    else
        return Zstring(); //unknown reparse point

#elif defined FFS_LINUX
    const int BUFFER_SIZE = 10000;
    char buffer[BUFFER_SIZE];

    const int bytesWritten = ::readlink(linkPath.c_str(), buffer, BUFFER_SIZE);
    if (bytesWritten < 0 || bytesWritten >= BUFFER_SIZE)
        return Zstring(); //error

    buffer[bytesWritten] = 0; //set null-terminating char

    return buffer;
#endif
}


#ifdef FFS_WIN
inline
wxLongLong getWin32TimeInformation(const FILETIME& lastWriteTime)
{
    //convert UTC FILETIME to ANSI C format (number of seconds since Jan. 1st 1970 UTC)
    wxLongLong writeTimeLong(wxInt32(lastWriteTime.dwHighDateTime), lastWriteTime.dwLowDateTime);
    writeTimeLong /= 10000000;                    //reduce precision to 1 second (FILETIME has unit 10^-7 s)
    writeTimeLong -= wxLongLong(2, 3054539008UL); //timeshift between ansi C time and FILETIME in seconds == 11644473600s
    return writeTimeLong;
}


inline
void setWin32FileInformation(const FILETIME& lastWriteTime,
                             const DWORD fileSizeHigh,
                             const DWORD fileSizeLow,
                             FreeFileSync::TraverseCallback::FileInfo& output)
{
    output.lastWriteTimeRaw = getWin32TimeInformation(lastWriteTime);
    output.fileSize = wxULongLong(fileSizeHigh, fileSizeLow);
}


inline
bool setWin32FileInformationFromSymlink(const Zstring& linkName, FreeFileSync::TraverseCallback::FileInfo& output)
{
    //open handle to target of symbolic link
    HANDLE hFile = ::CreateFile(FreeFileSync::applyLongPathPrefix(linkName).c_str(),
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


template <bool followSymlinks>
void traverseDirectory(const Zstring& directory, FreeFileSync::TraverseCallback* sink, int level)
{
    using namespace FreeFileSync;

    if (level == 100) //catch endless recursion
    {
        sink->onError(wxString(_("Endless loop when traversing directory:")) + wxT("\n\"") + zToWx(directory) + wxT("\""));
        return;
    }

#ifdef FFS_WIN
    //ensure directoryFormatted ends with backslash
    const Zstring directoryFormatted = directory.EndsWith(globalFunctions::FILE_NAME_SEPARATOR) ?
                                       directory :
                                       directory + globalFunctions::FILE_NAME_SEPARATOR;

    WIN32_FIND_DATA fileMetaData;
    HANDLE searchHandle = ::FindFirstFile(applyLongPathPrefix(directoryFormatted + DefaultChar('*')).c_str(), //__in   LPCTSTR lpFileName
                                          &fileMetaData);                                                     //__out  LPWIN32_FIND_DATA lpFindFileData
    //no noticable performance difference compared to FindFirstFileEx with FindExInfoBasic, FIND_FIRST_EX_CASE_SENSITIVE and/or FIND_FIRST_EX_LARGE_FETCH

    if (searchHandle == INVALID_HANDLE_VALUE)
    {
        const DWORD lastError = ::GetLastError();
        if (lastError == ERROR_FILE_NOT_FOUND)
            return;

        //else: we have a problem... report it:
        const wxString errorMessage = wxString(_("Error traversing directory:")) + wxT("\n\"") + zToWx(directory) + wxT("\"") + wxT("\n\n") +
                                      FreeFileSync::getLastErrorFormatted(lastError);

        sink->onError(errorMessage);
        return;
    }

    boost::shared_ptr<void> dummy(searchHandle, ::FindClose);

    do
    {
        //don't return "." and ".."
        const wxChar* const shortName = fileMetaData.cFileName;
        if (     shortName[0] == wxChar('.') &&
                 ((shortName[1] == wxChar('.') && shortName[2] == wxChar('\0')) ||
                  shortName[1] == wxChar('\0')))
            continue;

        const Zstring fullName = directoryFormatted + shortName;

        const bool isSymbolicLink = (fileMetaData.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT) != 0;

        if (isSymbolicLink && !followSymlinks) //evaluate symlink directly
        {
            TraverseCallback::SymlinkInfo details;
            details.lastWriteTimeRaw = getWin32TimeInformation(fileMetaData.ftLastWriteTime);
            details.targetPath       = getSymlinkTarget(fullName); //throw(); returns empty string on error
            details.dirLink          = (fileMetaData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0; //directory symlinks have this flag on Windows
            sink->onSymlink(shortName, fullName, details);
        }
        else if (fileMetaData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) //a directory... or symlink that needs to be followed (for directory symlinks this flag is set too!)
        {
            const TraverseCallback::ReturnValDir rv = sink->onDir(shortName, fullName);
            switch (rv.returnCode)
            {
            case TraverseCallback::ReturnValDir::TRAVERSING_DIR_IGNORE:
                break;

            case TraverseCallback::ReturnValDir::TRAVERSING_DIR_CONTINUE:
                traverseDirectory<followSymlinks>(fullName, rv.subDirCb, level + 1);
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
                    details.lastWriteTimeRaw = 0; //we are not interested in the modifiation time of the link
                    details.fileSize         = 0;
                }
            }
            else
                setWin32FileInformation(fileMetaData.ftLastWriteTime, fileMetaData.nFileSizeHigh, fileMetaData.nFileSizeLow, details);

            sink->onFile(shortName, fullName, details);
        }
    }
    while (::FindNextFile(searchHandle,	   // handle to search
                          &fileMetaData)); // pointer to structure for data on found file

    const DWORD lastError = ::GetLastError();
    if (lastError == ERROR_NO_MORE_FILES)
        return; //everything okay

    //else: we have a problem... report it:
    const wxString errorMessage = wxString(_("Error traversing directory:")) + wxT("\n\"") + zToWx(directory) + wxT("\"") ;
    sink->onError(errorMessage + wxT("\n\n") + FreeFileSync::getLastErrorFormatted(lastError));
    return;

#elif defined FFS_LINUX
    DIR* dirObj = ::opendir(directory.c_str()); //directory must NOT end with path separator, except "/"
    if (dirObj == NULL)
    {
        const wxString errorMessage = wxString(_("Error traversing directory:")) + wxT("\n\"") + zToWx(directory) + wxT("\"") ;
        sink->onError(errorMessage + wxT("\n\n") + FreeFileSync::getLastErrorFormatted());
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
            sink->onError(errorMessage + wxT("\n\n") + FreeFileSync::getLastErrorFormatted());
            return;
        }

        //don't return "." and ".."
        const DefaultChar* const shortName = dirEntry->d_name;
        if (      shortName[0] == wxChar('.') &&
                  ((shortName[1] == wxChar('.') && shortName[2] == wxChar('\0')) ||
                   shortName[1] == wxChar('\0')))
            continue;

        const Zstring fullName = directory.EndsWith(globalFunctions::FILE_NAME_SEPARATOR) ? //e.g. "/"
                                 directory + shortName :
                                 directory + globalFunctions::FILE_NAME_SEPARATOR + shortName;

        struct stat fileInfo;
        if (::lstat(fullName.c_str(), &fileInfo) != 0) //lstat() does not resolve symlinks
        {
            const wxString errorMessage = wxString(_("Error reading file attributes:")) + wxT("\n\"") + zToWx(fullName) + wxT("\"");
            sink->onError(errorMessage + wxT("\n\n") + FreeFileSync::getLastErrorFormatted());
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
                    sink->onFile(shortName, fullName, details); //report broken symlink as file!
                    continue;
                }
            }
            else //evaluate symlink directly
            {
                TraverseCallback::SymlinkInfo details;
                details.lastWriteTimeRaw = fileInfo.st_mtime; //UTC time(ANSI C format); unit: 1 second
                details.targetPath       = getSymlinkTarget(fullName); //throw(); returns empty string on error
                details.dirLink          = ::stat(fullName.c_str(), &fileInfo) == 0 && S_ISDIR(fileInfo.st_mode); //S_ISDIR and S_ISLNK are mutually exclusive on Linux => need to follow link
                sink->onSymlink(shortName, fullName, details);
                continue;
            }
        }

        //fileInfo contains dereferenced data in any case from here on

        if (S_ISDIR(fileInfo.st_mode)) //a directory... cannot be a symlink on Linux in this case
        {
            const TraverseCallback::ReturnValDir rv = sink->onDir(shortName, fullName);
            switch (rv.returnCode)
            {
            case TraverseCallback::ReturnValDir::TRAVERSING_DIR_IGNORE:
                break;

            case TraverseCallback::ReturnValDir::TRAVERSING_DIR_CONTINUE:
                traverseDirectory<followSymlinks>(fullName, rv.subDirCb, level + 1);
                break;
            }
        }
        else //a file... (or symlink; pathological!)
        {
            TraverseCallback::FileInfo details;
            details.lastWriteTimeRaw = fileInfo.st_mtime; //UTC time(ANSI C format); unit: 1 second
            details.fileSize         = fileInfo.st_size;

            sink->onFile(shortName, fullName, details);
        }
    }
#endif
}


void FreeFileSync::traverseFolder(const Zstring& directory,
                                  bool followSymlinks,
                                  TraverseCallback* sink)
{
#ifdef FFS_WIN
    const Zstring& directoryFormatted = directory;
#elif defined FFS_LINUX
    const Zstring directoryFormatted = //remove trailing slash
        directory.size() > 1 && directory.EndsWith(globalFunctions::FILE_NAME_SEPARATOR) ?  //exception: allow '/'
        directory.BeforeLast(globalFunctions::FILE_NAME_SEPARATOR) :
        directory;
#endif

    if (followSymlinks)
        traverseDirectory<true>(directoryFormatted, sink, 0);
    else
        traverseDirectory<false>(directoryFormatted, sink, 0);
}
