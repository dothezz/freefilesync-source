#include "fileTraverser.h"
#include "systemConstants.h"
#include "systemFunctions.h"
#include <wx/intl.h>
#include "stringConv.h"

#ifdef FFS_WIN
#include <wx/msw/wrapwin.h> //includes "windows.h"

#elif defined FFS_LINUX
#include <sys/stat.h>
#include <dirent.h>
#include <errno.h>
#endif


#ifdef FFS_WIN
class CloseHandleOnExit
{
public:
    CloseHandleOnExit(HANDLE fileHandle) : fileHandle_(fileHandle) {}

    ~CloseHandleOnExit()
    {
        CloseHandle(fileHandle_);
    }

private:
    HANDLE fileHandle_;
};


class CloseFindHandleOnExit
{
public:
    CloseFindHandleOnExit(HANDLE searchHandle) : searchHandle_(searchHandle) {}

    ~CloseFindHandleOnExit()
    {
        FindClose(searchHandle_);
    }

private:
    HANDLE searchHandle_;
};


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
bool setWin32FileInformationFromSymlink(const Zstring linkName, FreeFileSync::TraverseCallback::FileInfo& output)
{
    //open handle to target of symbolic link
    HANDLE hFile = ::CreateFile(linkName.c_str(),
                                0,
                                FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                                NULL,
                                OPEN_EXISTING,
                                FILE_FLAG_BACKUP_SEMANTICS,
                                NULL);
    if (hFile == INVALID_HANDLE_VALUE)
        return false;

    CloseHandleOnExit dummy(hFile);

    BY_HANDLE_FILE_INFORMATION fileInfoByHandle;

    if (!::GetFileInformationByHandle(
                hFile,
                &fileInfoByHandle))
        return false;

    //write output
    setWin32FileInformation(fileInfoByHandle.ftLastWriteTime, fileInfoByHandle.nFileSizeHigh, fileInfoByHandle.nFileSizeLow, output);
    return true;
}

#elif defined FFS_LINUX
class CloseDirOnExit
{
public:
    CloseDirOnExit(DIR* dir) : m_dir(dir) {}

    ~CloseDirOnExit()
    {
        ::closedir(m_dir); //no error handling here
    }

private:
    DIR* m_dir;
};
#endif


template <bool traverseDirectorySymlinks>
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
    HANDLE searchHandle = FindFirstFile((directoryFormatted + DefaultChar('*')).c_str(), //__in   LPCTSTR lpFileName
                                        &fileMetaData);                                  //__out  LPWIN32_FIND_DATA lpFindFileData
    //no noticable performance difference compared to FindFirstFileEx with FindExInfoBasic, FIND_FIRST_EX_CASE_SENSITIVE and/or FIND_FIRST_EX_LARGE_FETCH

    if (searchHandle == INVALID_HANDLE_VALUE)
    {
        const DWORD lastError = ::GetLastError();
        if (lastError == ERROR_FILE_NOT_FOUND)
            return true;

        //else: we have a problem... report it:
        const wxString errorMessage = wxString(_("Error traversing directory:")) + wxT("\n\"") + zToWx(directory) + wxT("\"") ;
        switch (sink->onError(errorMessage + wxT("\n\n") + FreeFileSync::getLastErrorFormatted(lastError)))
        {
        case TraverseCallback::TRAVERSING_STOP:
            return false;
        case TraverseCallback::TRAVERSING_CONTINUE:
            return true;
        }
    }
    CloseFindHandleOnExit dummy(searchHandle);

    do
    {
        //don't return "." and ".."
        const wxChar* const shortName = fileMetaData.cFileName;
        if (    shortName[0] == wxChar('.') &&
                ((shortName[1] == wxChar('.') && shortName[2] == wxChar('\0')) ||
                 shortName[1] == wxChar('\0')))
            continue;

        const Zstring fullName = directoryFormatted + shortName;

        if (fileMetaData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) //a directory... (for directory symlinks this flag is set too!)
        {
            const TraverseCallback::ReturnValDir rv = sink->onDir(shortName, fullName);
            switch (rv.returnCode)
            {
            case TraverseCallback::ReturnValDir::TRAVERSING_STOP:
                return false;

            case TraverseCallback::ReturnValDir::TRAVERSING_IGNORE_DIR:
                break;

            case TraverseCallback::ReturnValDir::TRAVERSING_CONTINUE:
                //traverse into symbolic links, junctions, etc. if requested only:
                if (traverseDirectorySymlinks || (~fileMetaData.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT))
                    if (!traverseDirectory<traverseDirectorySymlinks>(fullName, rv.subDirCb, level + 1))
                        return false;
                break;
            }
        }
        else //a file...
        {
            TraverseCallback::FileInfo details;

            if (fileMetaData.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT) //dereference symlinks!
            {
                if (!setWin32FileInformationFromSymlink(fullName, details)) //broken symlink
                {
                    details.lastWriteTimeRaw = 0; //we are not interested in the modifiation time of the link
                    details.fileSize         = 0;
                }
            }
            else
                setWin32FileInformation(fileMetaData.ftLastWriteTime, fileMetaData.nFileSizeHigh, fileMetaData.nFileSizeLow, details);

            switch (sink->onFile(shortName, fullName, details))
            {
            case TraverseCallback::TRAVERSING_STOP:
                return false;
            case TraverseCallback::TRAVERSING_CONTINUE:
                break;
            }
        }
    }
    while (FindNextFile(searchHandle,	 // handle to search
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
    CloseDirOnExit dummy(dirObj);

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

                switch (sink->onFile(shortName, fullName, details))
                {
                case TraverseCallback::TRAVERSING_STOP:
                    return false;
                case TraverseCallback::TRAVERSING_CONTINUE:
                    break;
                }
                continue;
            }
        }


        if (S_ISDIR(fileInfo.st_mode)) //a directory... (note: symbolic links need to be dereferenced to test if they point to a directory!)
        {
            const TraverseCallback::ReturnValDir rv = sink->onDir(shortName, fullName);
            switch (rv.returnCode)
            {
            case TraverseCallback::ReturnValDir::TRAVERSING_STOP:
                return false;

            case TraverseCallback::ReturnValDir::TRAVERSING_IGNORE_DIR:
                break;

            case TraverseCallback::ReturnValDir::TRAVERSING_CONTINUE:
                //traverse into symbolic links, junctions, etc. if requested only:
                if (traverseDirectorySymlinks || !isSymbolicLink) //traverse into symbolic links if requested only
                    if (!traverseDirectory<traverseDirectorySymlinks>(fullName, rv.subDirCb, level + 1))
                        return false;
                break;
            }
        }
        else //a file...
        {
            TraverseCallback::FileInfo details;
            details.lastWriteTimeRaw = fileInfo.st_mtime; //UTC time(ANSI C format); unit: 1 second
            details.fileSize         = fileInfo.st_size;

            switch (sink->onFile(shortName, fullName, details))
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
                                  const bool traverseDirectorySymlinks,
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

    if (traverseDirectorySymlinks)
        traverseDirectory<true>(directoryFormatted, sink, 0);
    else
        traverseDirectory<false>(directoryFormatted, sink, 0);
}
