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

#ifdef FFS_WIN
#include <wx/msw/wrapwin.h> //includes "windows.h"
#include "longPathPrefix.h"

#elif defined FFS_LINUX
#include <sys/stat.h>
#include <dirent.h>
#include <errno.h>
#endif


#ifdef FFS_WIN
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


inline
void setWin32FileInformation(const FILETIME& lastWriteTime,
                             const DWORD fileSizeHigh,
                             const DWORD fileSizeLow,
                             FreeFileSync::TraverseCallback::FileInfo& output)
{
    //convert UTC FILETIME to ANSI C format (number of seconds since Jan. 1st 1970 UTC)
    wxLongLong writeTimeLong(wxInt32(lastWriteTime.dwHighDateTime), lastWriteTime.dwLowDateTime);
    writeTimeLong /= 10000000;                    //reduce precision to 1 second (FILETIME has unit 10^-7 s)
    writeTimeLong -= wxLongLong(2, 3054539008UL); //timeshift between ansi C time and FILETIME in seconds == 11644473600s
    output.lastWriteTimeRaw = writeTimeLong;

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


bool traverseDirectory(const Zstring& directory, FreeFileSync::TraverseCallback* sink, const int level)
{
    using namespace FreeFileSync;

    if (level == 100) //catch endless recursion
    {
        switch (sink->onError(wxString(_("Endless loop when traversing directory:")) + wxT("\n\"") + zToWx(directory) + wxT("\"")))
        {
        case TraverseCallback::TRAVERSING_STOP:
            return false;
        case TraverseCallback::TRAVERSING_CONTINUE:
            return true;
        }
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
            return true;

        //else: we have a problem... report it:
        const wxString errorMessage = wxString(_("Error traversing directory:")) + wxT("\n\"") + zToWx(directory) + wxT("\"") + wxT("\n\n") +
                                      FreeFileSync::getLastErrorFormatted(lastError);

        switch (sink->onError(errorMessage))
        {
        case TraverseCallback::TRAVERSING_STOP:
            return false;
        case TraverseCallback::TRAVERSING_CONTINUE:
            return true;
        }
    }

    boost::shared_ptr<void> dummy(searchHandle, ::FindClose);

    do
    {
        //don't return "." and ".."
        const wxChar* const shortName = fileMetaData.cFileName;
        if (    shortName[0] == wxChar('.') &&
                ((shortName[1] == wxChar('.') && shortName[2] == wxChar('\0')) ||
                 shortName[1] == wxChar('\0')))
            continue;

        const Zstring fullName = directoryFormatted + shortName;
        const bool isSymbolicLink = (fileMetaData.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT) != 0;

        if (fileMetaData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) //a directory... (for directory symlinks this flag is set too!)
        {
            const TraverseCallback::ReturnValDir rv = sink->onDir(shortName, fullName, isSymbolicLink);
            switch (rv.returnCode)
            {
            case TraverseCallback::ReturnValDir::TRAVERSING_DIR_STOP:
                return false;

            case TraverseCallback::ReturnValDir::TRAVERSING_DIR_IGNORE:
                break;

            case TraverseCallback::ReturnValDir::TRAVERSING_DIR_CONTINUE:
                if (!traverseDirectory(fullName, rv.subDirCb, level + 1))
                    return false;
                break;
            }
        }
        else //a file...
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

            switch (sink->onFile(shortName, fullName, isSymbolicLink, details))
            {
            case TraverseCallback::TRAVERSING_STOP:
                return false;
            case TraverseCallback::TRAVERSING_CONTINUE:
                break;
            }
        }
    }
    while (::FindNextFile(searchHandle,	   // handle to search
                          &fileMetaData)); // pointer to structure for data on found file

    const DWORD lastError = ::GetLastError();
    if (lastError == ERROR_NO_MORE_FILES)
        return true; //everything okay

    //else: we have a problem... report it:
    const wxString errorMessage = wxString(_("Error traversing directory:")) + wxT("\n\"") + zToWx(directory) + wxT("\"") ;
    switch (sink->onError(errorMessage + wxT("\n\n") + FreeFileSync::getLastErrorFormatted(lastError)))
    {
    case TraverseCallback::TRAVERSING_STOP:
        return false;
    case TraverseCallback::TRAVERSING_CONTINUE:
        return true;
    }

#elif defined FFS_LINUX
    DIR* dirObj = ::opendir(directory.c_str()); //directory must NOT end with path separator, except "/"
    if (dirObj == NULL)
    {
        const wxString errorMessage = wxString(_("Error traversing directory:")) + wxT("\n\"") + zToWx(directory) + wxT("\"") ;
        switch (sink->onError(errorMessage + wxT("\n\n") + FreeFileSync::getLastErrorFormatted()))
        {
        case TraverseCallback::TRAVERSING_STOP:
            return false;
        case TraverseCallback::TRAVERSING_CONTINUE:
            return true;
        }
    }

    boost::shared_ptr<DIR> dummy(dirObj, &::closedir); //never close NULL handles! -> crash

    while (true)
    {
        errno = 0; //set errno to 0 as unfortunately this isn't done when readdir() returns NULL because it can't find any files
        struct dirent* dirEntry = ::readdir(dirObj);
        if (dirEntry == NULL)
        {
            if (errno == 0)
                return true; //everything okay

            //else: we have a problem... report it:
            const wxString errorMessage = wxString(_("Error traversing directory:")) + wxT("\n\"") + zToWx(directory) + wxT("\"") ;
            switch (sink->onError(errorMessage + wxT("\n\n") + FreeFileSync::getLastErrorFormatted()))
            {
            case TraverseCallback::TRAVERSING_STOP:
                return false;
            case TraverseCallback::TRAVERSING_CONTINUE:
                return true;
            }
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
        if (lstat(fullName.c_str(), &fileInfo) != 0) //lstat() does not resolve symlinks
        {
            const wxString errorMessage = wxString(_("Error reading file attributes:")) + wxT("\n\"") + zToWx(fullName) + wxT("\"");
            switch (sink->onError(errorMessage + wxT("\n\n") + FreeFileSync::getLastErrorFormatted()))
            {
            case TraverseCallback::TRAVERSING_STOP:
                return false;
            case TraverseCallback::TRAVERSING_CONTINUE:
                break;
            }
            continue;
        }

        const bool isSymbolicLink = S_ISLNK(fileInfo.st_mode);
        if (isSymbolicLink) //dereference symbolic links
        {
            if (stat(fullName.c_str(), &fileInfo) != 0) //stat() resolves symlinks
            {
                //a broken symbolic link
                TraverseCallback::FileInfo details;
                details.lastWriteTimeRaw = 0; //we are not interested in the modifiation time of the link
                details.fileSize         = 0;

                switch (sink->onFile(shortName, fullName, isSymbolicLink, details))
                {
                case TraverseCallback::TRAVERSING_STOP:
                    return false;
                case TraverseCallback::TRAVERSING_CONTINUE:
                    break;
                }
                continue;
            }
        }


        if (S_ISDIR(fileInfo.st_mode)) //a directory... (note: symbolic links need to be dereferenced to test whether they point to a directory!)
        {
            const TraverseCallback::ReturnValDir rv = sink->onDir(shortName, fullName, isSymbolicLink);
            switch (rv.returnCode)
            {
            case TraverseCallback::ReturnValDir::TRAVERSING_DIR_STOP:
                return false;

            case TraverseCallback::ReturnValDir::TRAVERSING_DIR_IGNORE:
                break;

            case TraverseCallback::ReturnValDir::TRAVERSING_DIR_CONTINUE:
                if (!traverseDirectory(fullName, rv.subDirCb, level + 1))
                    return false;
                break;
            }
        }
        else //a file...
        {
            TraverseCallback::FileInfo details;
            details.lastWriteTimeRaw = fileInfo.st_mtime; //UTC time(ANSI C format); unit: 1 second
            details.fileSize         = fileInfo.st_size;

            switch (sink->onFile(shortName, fullName, isSymbolicLink, details))
            {
            case TraverseCallback::TRAVERSING_STOP:
                return false;
            case TraverseCallback::TRAVERSING_CONTINUE:
                break;
            }
        }
    }
#endif

    return true; //dummy value
}


void FreeFileSync::traverseFolder(const Zstring& directory,
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

    traverseDirectory(directoryFormatted, sink, 0);
}
