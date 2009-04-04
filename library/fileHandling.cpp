#include "fileHandling.h"
#include <wx/intl.h>
#include <wx/msgdlg.h>
#include "../algorithm.h"
#include <wx/filename.h>

#ifdef FFS_WIN
#include <wx/msw/wrapwin.h> //includes "windows.h"

#elif defined FFS_LINUX
#include <sys/stat.h>
#include <time.h>
#include <utime.h>
#include <fstream>
#include <unistd.h>
#include <dirent.h>
#include <errno.h>
#endif


class RecycleBin
{
public:
    RecycleBin() :
            recycleBinAvailable(false)
    {
#ifdef FFS_WIN
        recycleBinAvailable = true;
#endif  // FFS_WIN
    }

    ~RecycleBin() {}

    bool recycleBinExists()
    {
        return recycleBinAvailable;
    }

    bool moveToRecycleBin(const Zstring& filename)
    {
        if (!recycleBinAvailable)   //this method should ONLY be called if recycle bin is available
            throw RuntimeException(_("Initialization of Recycle Bin failed!"));

#ifdef FFS_WIN
        Zstring filenameDoubleNull = filename + wxChar(0);

        SHFILEOPSTRUCT fileOp;
        fileOp.hwnd   = NULL;
        fileOp.wFunc  = FO_DELETE;
        fileOp.pFrom  = filenameDoubleNull.c_str();
        fileOp.pTo    = NULL;
        fileOp.fFlags = FOF_ALLOWUNDO | FOF_NOCONFIRMATION | FOF_SILENT | FOF_NOERRORUI;
        fileOp.fAnyOperationsAborted = false;
        fileOp.hNameMappings         = NULL;
        fileOp.lpszProgressTitle     = NULL;

        if (SHFileOperation(&fileOp   //pointer to an SHFILEOPSTRUCT structure that contains information the function needs to carry out
                           ) != 0 || fileOp.fAnyOperationsAborted) return false;
#else
        assert(false);
#endif

        return true;
    }

private:
    bool recycleBinAvailable;
};

//global instance of recycle bin
RecycleBin recyclerInstance;


bool FreeFileSync::recycleBinExists()
{
    return recyclerInstance.recycleBinExists();
}


inline
bool moveToRecycleBin(const Zstring& filename) throw(RuntimeException)
{
    return recyclerInstance.moveToRecycleBin(filename);
}


bool FreeFileSync::fileExists(const Zstring& filename)
{ //symbolic links (broken or not) are also treated as existing files!
#ifdef FFS_WIN
    // we must use GetFileAttributes() instead of the ANSI C functions because
    // it can cope with network (UNC) paths unlike them
    const DWORD ret = ::GetFileAttributes(filename.c_str());

    return (ret != INVALID_FILE_ATTRIBUTES) && !(ret & FILE_ATTRIBUTE_DIRECTORY);

#elif defined FFS_LINUX
    struct stat fileInfo;
    return (lstat(filename.c_str(), &fileInfo) == 0 &&
            (S_ISLNK(fileInfo.st_mode) || S_ISREG(fileInfo.st_mode)));
#endif
}


void FreeFileSync::removeFile(const Zstring& filename, const bool useRecycleBin)
{
    //no error situation if file is not existing! manual deletion relies on it!
#ifdef FFS_WIN
    if (GetFileAttributes(filename.c_str()) == INVALID_FILE_ATTRIBUTES)
        return; //neither file nor any other object with that name existing

#elif defined FFS_LINUX
    struct stat fileInfo;
    if (lstat(filename.c_str(), &fileInfo) != 0)
        return; //neither file nor any other object (e.g. broken symlink) with that name existing
#endif

    if (useRecycleBin)
    {
        if (!moveToRecycleBin(filename))
            throw FileError(Zstring(_("Error moving to Recycle Bin:")) + wxT("\n\"") + filename + wxT("\""));
        return;
    }

#ifdef FFS_WIN
    //initialize file attributes
    if (!SetFileAttributes(
                filename.c_str(),       //address of filename
                FILE_ATTRIBUTE_NORMAL)) //attributes to set
    {
        Zstring errorMessage = Zstring(_("Error deleting file:")) + wxT("\n\"") + filename + wxT("\"");
        throw FileError(errorMessage + wxT("\n\n") + FreeFileSync::getLastErrorFormatted());
    }

    //remove file, support for \\?\-prefix
    if (DeleteFile(filename.c_str()) == 0)
    {
        Zstring errorMessage = Zstring(_("Error deleting file:")) + wxT("\n\"") + filename + wxT("\"");
        throw FileError(errorMessage + wxT("\n\n") + FreeFileSync::getLastErrorFormatted());
    }
#elif defined FFS_LINUX
    if (unlink(filename.c_str()) != 0)
    {
        Zstring errorMessage = Zstring(_("Error deleting file:")) + wxT("\n\"") + filename + wxT("\"");
        throw FileError(errorMessage + wxT("\n\n") + FreeFileSync::getLastErrorFormatted());
    }
#endif
}


class FilesDirsOnlyTraverser : public FreeFileSync::FullDetailFileTraverser
{
public:
    FilesDirsOnlyTraverser(std::vector<Zstring>& files, std::vector<Zstring>& dirs) :
            m_files(files),
            m_dirs(dirs) {}

    virtual wxDirTraverseResult OnFile(const Zstring& filename, const FreeFileSync::FileInfo& details)
    {
        m_files.push_back(filename);
        return wxDIR_CONTINUE;
    }
    virtual wxDirTraverseResult OnDir(const Zstring& dirname)
    {
        m_dirs.push_back(dirname);
        return wxDIR_IGNORE; //DON'T traverse into subdirs, removeDirectory works recursively!
    }
    virtual wxDirTraverseResult OnError(const Zstring& errorText)
    {
        throw FileError(errorText);
    }

private:
    std::vector<Zstring>& m_files;
    std::vector<Zstring>& m_dirs;
};


void FreeFileSync::removeDirectory(const Zstring& directory, const bool useRecycleBin)
{
    //no error situation if directory is not existing! manual deletion relies on it!
#ifdef FFS_WIN
    const DWORD dirAttr = GetFileAttributes(directory.c_str()); //name of a file or directory
    if (dirAttr == INVALID_FILE_ATTRIBUTES)
        return; //neither directory nor any other object with that name existing

#elif defined FFS_LINUX
    struct stat dirInfo;
    if (lstat(directory.c_str(), &dirInfo) != 0)
        return; //neither directory nor any other object (e.g. broken symlink) with that name existing
#endif

    if (useRecycleBin)
    {
        if (!moveToRecycleBin(directory))
            throw FileError(Zstring(_("Error moving to Recycle Bin:")) + wxT("\n\"") + directory + wxT("\""));
        return;
    }

//attention: check if directory is a symlink! Do NOT traverse into it deleting contained files!!!
#ifdef FFS_WIN
    if (dirAttr & FILE_ATTRIBUTE_REPARSE_POINT)
    {   //remove symlink directly, support for \\?\-prefix
        if (RemoveDirectory(directory.c_str()) == 0)
        {
            Zstring errorMessage = Zstring(_("Error deleting directory:")) + wxT("\n\"") + directory + wxT("\"");
            throw FileError(errorMessage + wxT("\n\n") + FreeFileSync::getLastErrorFormatted());
        }
        return;
    }

#elif defined FFS_LINUX
    if (S_ISLNK(dirInfo.st_mode))
    {
        if (unlink(directory.c_str()) != 0)
        {
            Zstring errorMessage = Zstring(_("Error deleting directory:")) + wxT("\n\"") + directory + wxT("\"");
            throw FileError(errorMessage + wxT("\n\n") + FreeFileSync::getLastErrorFormatted());
        }
        return;
    }
#endif

    std::vector<Zstring> fileList;
    std::vector<Zstring> dirList;

    //get all files and directories from current directory (WITHOUT subdirectories!)
    FilesDirsOnlyTraverser traverser(fileList, dirList);
    FreeFileSync::traverseInDetail(directory, false, &traverser); //don't traverse into symlinks to directories

    //delete files
    for (std::vector<Zstring>::const_iterator j = fileList.begin(); j != fileList.end(); ++j)
        FreeFileSync::removeFile(*j, false);

    //delete directories recursively
    for (std::vector<Zstring>::const_iterator j = dirList.begin(); j != dirList.end(); ++j)
        FreeFileSync::removeDirectory(*j, false); //call recursively to correctly handle symbolic links

    //parent directory is deleted last
#ifdef FFS_WIN
    //initialize file attributes
    if (!SetFileAttributes(
                directory.c_str(),      // address of directory name
                FILE_ATTRIBUTE_NORMAL)) // attributes to set
    {
        Zstring errorMessage = Zstring(_("Error deleting directory:")) + wxT("\n\"") + directory + wxT("\"");
        throw FileError(errorMessage + wxT("\n\n") + FreeFileSync::getLastErrorFormatted());
    }

    //remove directory, support for \\?\-prefix
    if (!RemoveDirectory(directory.c_str()))
    {
        Zstring errorMessage = Zstring(_("Error deleting directory:")) + wxT("\n\"") + directory + wxT("\"");
        throw FileError(errorMessage + wxT("\n\n") + FreeFileSync::getLastErrorFormatted());
    }
#else
    if (rmdir(directory.c_str()) != 0)
    {
        Zstring errorMessage = Zstring(_("Error deleting directory:")) + wxT("\n\"") + directory + wxT("\"");
        throw FileError(errorMessage + wxT("\n\n") + FreeFileSync::getLastErrorFormatted());
    }
#endif
}


#ifdef FFS_WIN
class CloseHandleOnExit
{
public:
    CloseHandleOnExit(HANDLE searchHandle) : m_searchHandle(searchHandle) {}

    ~CloseHandleOnExit()
    {
        FindClose(m_searchHandle);
    }

private:
    HANDLE m_searchHandle;
};


typedef DWORD WINAPI (*GetFinalPath)(
    HANDLE hFile,
    LPTSTR lpszFilePath,
    DWORD cchFilePath,
    DWORD dwFlags);


class DllHandler //dynamically load windows API functions
{
public:
    DllHandler() :
            getFinalPathNameByHandle(NULL),
            hKernel(NULL)
    {
        //get a handle to the DLL module containing required functionality
        hKernel = ::LoadLibrary(wxT("kernel32.dll"));
        if (hKernel)
            getFinalPathNameByHandle = (GetFinalPath)(::GetProcAddress(hKernel, "GetFinalPathNameByHandleW")); //load unicode version!
    }

    ~DllHandler()
    {
        if (hKernel) ::FreeLibrary(hKernel);
    }

    GetFinalPath getFinalPathNameByHandle;

private:
    HINSTANCE hKernel;
};

//global instance
DllHandler dynamicWinApi;


Zstring resolveDirectorySymlink(const Zstring& dirLinkName) //get full target path of symbolic link to a directory
{
    //open handle to target of symbolic link
    HANDLE hDir = CreateFile(dirLinkName.c_str(),
                             0,
                             FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                             NULL,
                             OPEN_EXISTING,
                             FILE_FLAG_BACKUP_SEMANTICS,
                             NULL);
    if (hDir == INVALID_HANDLE_VALUE)
        return Zstring();

    CloseHandleOnExit dummy(hDir);

    if (dynamicWinApi.getFinalPathNameByHandle == NULL )
        throw FileError(Zstring(_("Error loading library function:")) + wxT("\n\"") + wxT("GetFinalPathNameByHandleW") + wxT("\""));

    const unsigned BUFFER_SIZE = 10000;
    TCHAR targetPath[BUFFER_SIZE];

    const DWORD rv = dynamicWinApi.getFinalPathNameByHandle(
                         hDir,
                         targetPath,
                         BUFFER_SIZE,
                         0);

    if (rv >= BUFFER_SIZE || rv == 0)
        return Zstring();

    return targetPath;
}
#endif


void createDirectoryRecursively(const Zstring& directory, const Zstring& templateDir, const bool copyDirectorySymLinks, const int level)
{
    if (wxDirExists(directory.c_str()))
        return;

    if (level == 50) //catch endless recursion
        return;

    //try to create parent folders first
    const Zstring dirParent = directory.BeforeLast(GlobalResources::FILE_NAME_SEPARATOR);
    if (!dirParent.empty() && !wxDirExists(dirParent))
    {
        //call function recursively
        const Zstring templateParent = templateDir.BeforeLast(GlobalResources::FILE_NAME_SEPARATOR);
        createDirectoryRecursively(dirParent, templateParent, false, level + 1); //don't create symbolic links in recursion!
    }

    //now creation should be possible
#ifdef FFS_WIN
    const DWORD templateAttr = ::GetFileAttributes(templateDir.c_str()); //replaces wxDirExists(): also returns successful for broken symlinks
    if (templateAttr == INVALID_FILE_ATTRIBUTES) //fallback
    {
        if (CreateDirectory(
                    directory.c_str(), // pointer to a directory path string
                    NULL) == 0 && level == 0)
        {
            const Zstring errorMessage = Zstring(_("Error creating directory:")) + wxT("\n\"") + directory + wxT("\"");
            throw FileError(errorMessage + wxT("\n\n") + FreeFileSync::getLastErrorFormatted());
        }
    }
    else
    {
        //symbolic link handling
        if (!copyDirectorySymLinks && templateAttr & FILE_ATTRIBUTE_REPARSE_POINT) //create directory based on target of symbolic link
        {
            //get target directory of symbolic link
            const Zstring targetPath = resolveDirectorySymlink(templateDir);
            if (targetPath.empty())
            {
                if (level == 0)
                    throw FileError(Zstring(_("Error resolving symbolic link:")) + wxT("\n\"") + templateDir + wxT("\""));
            }
            else
            {
                if (CreateDirectoryEx(          // this function automatically copies symbolic links if encountered
                            targetPath.c_str(), // pointer to path string of template directory
                            directory.c_str(),	// pointer to a directory path string
                            NULL) == 0 && level == 0)
                {
                    const Zstring errorMessage = Zstring(_("Error creating directory:")) + wxT("\n\"") + directory + wxT("\"");
                    throw FileError(errorMessage + wxT("\n\n") + FreeFileSync::getLastErrorFormatted());
                }
            }
        }
        else //in all other cases
        {
            if (CreateDirectoryEx(           // this function automatically copies symbolic links if encountered
                        templateDir.c_str(), // pointer to path string of template directory
                        directory.c_str(),	 // pointer to a directory path string
                        NULL) == 0 && level == 0)
            {
                const Zstring errorMessage = Zstring(_("Error creating directory:")) + wxT("\n\"") + directory + wxT("\"");
                throw FileError(errorMessage + wxT("\n\n") + FreeFileSync::getLastErrorFormatted());
            }
        }
    }
#elif defined FFS_LINUX
    //symbolic link handling
    if (copyDirectorySymLinks)
    {
        //test if templateDir is a symbolic link
        struct stat linkInfo;
        if (lstat(templateDir.c_str(), &linkInfo) == 0 && S_ISLNK(linkInfo.st_mode))
        {
            //copy symbolic link
            const int BUFFER_SIZE = 10000;
            char buffer[BUFFER_SIZE];
            const int bytesWritten = readlink(templateDir.c_str(), buffer, BUFFER_SIZE);
            if (bytesWritten < 0 || bytesWritten == BUFFER_SIZE)
            {
                Zstring errorMessage = Zstring(_("Error resolving symbolic link:")) + wxT("\n\"") + templateDir + wxT("\"");
                if (bytesWritten < 0) errorMessage += Zstring(wxT("\n\n")) + FreeFileSync::getLastErrorFormatted();
                throw FileError(errorMessage);
            }
            //set null-terminating char
            buffer[bytesWritten] = 0;

            if (symlink(buffer, directory.c_str()) != 0)
            {
                Zstring errorMessage = Zstring(_("Error creating directory:")) + wxT("\n\"") + directory + wxT("\"");
                throw FileError(errorMessage + wxT("\n\n") + FreeFileSync::getLastErrorFormatted());
            }
            return; //symlink created successfully
        }
    }

    //default directory creation
    if (mkdir(directory.c_str(), 0755) != 0 && level == 0)
    {
        Zstring errorMessage = Zstring(_("Error creating directory:")) + wxT("\n\"") + directory + wxT("\"");
        throw FileError(errorMessage + wxT("\n\n") + FreeFileSync::getLastErrorFormatted());
    }

//copy directory permissions: not sure if this is a good idea: if a directory is read-only copying/sync'ing of files will fail...
    /*
        if (templateDirExists)
        {
            struct stat fileInfo;
            if (stat(templateDir.c_str(), &fileInfo) != 0) //read permissions from template directory
                throw FileError(Zstring(_("Error reading file attributes:")) + wxT("\n\"") + templateDir + wxT("\""));

            // reset the umask as we want to create the directory with exactly the same permissions as the template
            wxCHANGE_UMASK(0);

            if (mkdir(directory.c_str(), fileInfo.st_mode) != 0 && level == 0)
                throw FileError(Zstring(_("Error creating directory:")) + wxT("\n\"") + directory + wxT("\""));
        }
        else
        {
            if (mkdir(directory.c_str(), 0777) != 0 && level == 0)
                throw FileError(Zstring(_("Error creating directory:")) + wxT("\n\"") + directory + wxT("\""));
        }
    */
#endif
}


void FreeFileSync::createDirectory(const Zstring& directory, const Zstring& templateDir, const bool copyDirectorySymLinks)
{
    //remove trailing separator
    Zstring dirFormatted;
    if (FreeFileSync::endsWithPathSeparator(directory))
        dirFormatted = directory.BeforeLast(GlobalResources::FILE_NAME_SEPARATOR);
    else
        dirFormatted = directory;

    Zstring templateFormatted;
    if (FreeFileSync::endsWithPathSeparator(templateDir))
        templateFormatted = templateDir.BeforeLast(GlobalResources::FILE_NAME_SEPARATOR);
    else
        templateFormatted = templateDir;

    createDirectoryRecursively(dirFormatted, templateFormatted, copyDirectorySymLinks, 0);
}


#ifdef FFS_LINUX
struct MemoryAllocator
{
    MemoryAllocator()
    {
        buffer = new char[bufferSize];
    }

    ~MemoryAllocator()
    {
        delete [] buffer;
    }

    static const unsigned int bufferSize = 512 * 1024;
    char* buffer;
};


void FreeFileSync::copyFile(const Zstring& sourceFile,
                            const Zstring& targetFile,
                            const bool copyFileSymLinks,
                            CopyFileCallback callback,
                            void* data)
{
    try
    {
        if (FreeFileSync::fileExists(targetFile.c_str()))
            throw FileError(Zstring(_("Error copying file:")) + wxT("\n\"") + sourceFile +  wxT("\" -> \"") + targetFile + wxT("\"\n")
                            + _("Target file already existing!"));

        //symbolic link handling
        if (copyFileSymLinks)
        {
            //test if sourceFile is a symbolic link
            struct stat linkInfo;
            if (lstat(sourceFile.c_str(), &linkInfo) != 0)
            {
                Zstring errorMessage = Zstring(_("Error reading file attributes:")) + wxT("\n\"") + sourceFile + wxT("\"");
                throw FileError(errorMessage + wxT("\n\n") + FreeFileSync::getLastErrorFormatted());
            }

            if (S_ISLNK(linkInfo.st_mode))
            {
                //copy symbolic link
                const unsigned BUFFER_SIZE = 10000;
                char buffer[BUFFER_SIZE];
                const int bytesWritten = readlink(sourceFile.c_str(), buffer, BUFFER_SIZE - 1);
                if (bytesWritten < 0)
                {
                    Zstring errorMessage = Zstring(_("Error resolving symbolic link:")) + wxT("\n\"") + sourceFile + wxT("\"");
                    throw FileError(errorMessage + wxT("\n\n") + FreeFileSync::getLastErrorFormatted());
                }
                //set null-terminating char
                buffer[bytesWritten] = 0;

                if (symlink(buffer, targetFile.c_str()) != 0)
                {
                    Zstring errorMessage = Zstring(_("Error writing file:")) + wxT("\n\"") + targetFile + wxT("\"");
                    throw FileError(errorMessage + wxT("\n\n") + FreeFileSync::getLastErrorFormatted());
                }

                return; //symlink created successfully
            }
        }

        //begin of regular file copy
        struct stat fileInfo;
        if (stat(sourceFile.c_str(), &fileInfo) != 0) //read file attributes from source file (resolve symlinks)
        {
            Zstring errorMessage = Zstring(_("Error reading file attributes:")) + wxT("\n\"") + sourceFile + wxT("\"");
            throw FileError(errorMessage + wxT("\n\n") + FreeFileSync::getLastErrorFormatted());
        }

        //open sourceFile for reading
        std::ifstream fileIn(sourceFile.c_str(), std::ios_base::binary);
        if (fileIn.fail())
            throw FileError(Zstring(_("Error opening file:")) + wxT("\n\"") + sourceFile +  wxT("\""));

        //create targetFile and open it for writing
        std::ofstream fileOut(targetFile.c_str(), std::ios_base::binary);
        if (fileOut.fail())
            throw FileError(Zstring(_("Error opening file:")) + wxT("\n\"") + targetFile + wxT("\""));

        //copy contents of sourceFile to targetFile
        wxULongLong totalBytesTransferred;
        static MemoryAllocator memory;
        while (true)
        {
            fileIn.read(memory.buffer, memory.bufferSize);
            if (fileIn.eof())  //end of file? fail bit is set in this case also!
            {
                fileOut.write(memory.buffer, fileIn.gcount());
                if (fileOut.bad())
                    throw FileError(Zstring(_("Error writing file:")) + wxT("\n\"") + targetFile + wxT("\""));
                break;
            }
            else if (fileIn.fail())
                throw FileError(Zstring(_("Error reading file:")) + wxT("\n\"") + sourceFile + wxT("\""));


            fileOut.write(memory.buffer, memory.bufferSize);
            if (fileOut.bad())
                throw FileError(Zstring(_("Error writing file:")) + wxT("\n\"") + targetFile + wxT("\""));

            totalBytesTransferred += memory.bufferSize;

            //invoke callback method to update progress indicators
            if (callback)
                callback(totalBytesTransferred, data);
        }

        //close streams before changing attributes
        fileIn.close();
        fileOut.close();

        //adapt file modification time:
        struct utimbuf newTimes;
        time(&newTimes.actime); //set file access time to current time
        newTimes.modtime = fileInfo.st_mtime;
        if (utime(targetFile.c_str(), &newTimes) != 0)
        {
            Zstring errorMessage = Zstring(_("Error changing modification time:")) + wxT("\n\"") + targetFile + wxT("\"");
            throw FileError(errorMessage + wxT("\n\n") + FreeFileSync::getLastErrorFormatted());
        }

        //set file access rights
        if (chmod(targetFile.c_str(), fileInfo.st_mode) != 0)
        {
            Zstring errorMessage = Zstring(_("Error writing file attributes:")) + wxT("\n\"") + targetFile + wxT("\"");
            throw FileError(errorMessage + wxT("\n\n") + FreeFileSync::getLastErrorFormatted());
        }
    }
    catch (...)
    {   //try to delete target file if error occured, or exception was thrown in callback function
        if (FreeFileSync::fileExists(targetFile.c_str()))
            wxRemoveFile(targetFile.c_str()); //don't handle error situations!

        throw;
    }
}
#endif


#ifdef FFS_WIN
inline
void setWin32FileInformation(const FILETIME& lastWriteTime, const DWORD fileSizeHigh, const DWORD fileSizeLow, FreeFileSync::FileInfo& output)
{
    //convert UTC FILETIME to ANSI C format (number of seconds since Jan. 1st 1970 UTC)
    wxLongLong writeTimeLong(lastWriteTime.dwHighDateTime, lastWriteTime.dwLowDateTime);
    writeTimeLong /= 10000000;                     //reduce precision to 1 second (FILETIME has unit 10^-7 s)
    writeTimeLong -= wxLongLong(2, 3054539008UL); //timeshift between ansi C time and FILETIME in seconds == 11644473600s
    output.lastWriteTimeRaw = writeTimeLong;

    output.fileSize = wxULongLong(fileSizeHigh, fileSizeLow);
}

inline
bool setWin32FileInformationFromSymlink(const Zstring linkName, FreeFileSync::FileInfo& output)
{
    //open handle to target of symbolic link
    HANDLE hFile = CreateFile(linkName.c_str(),
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

    if (!GetFileInformationByHandle(
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
        closedir(m_dir); //no error handling here
    }

private:
    DIR* m_dir;
};
#endif


template <bool traverseDirectorySymlinks>
class TraverseRecursively
{
public:
    TraverseRecursively(FreeFileSync::FullDetailFileTraverser* sink) : m_sink(sink) {}

    bool traverse(const Zstring& directory, const int level)
    {
        if (level == 100) //catch endless recursion
        {
            if (m_sink->OnError(Zstring(_("Error traversing directory:")) + wxT("\n\"") + directory + wxT("\"")) == wxDIR_STOP)
                return false;
            else
                return true;
        }

#ifdef FFS_WIN
        Zstring directoryFormatted = directory;
        if (!FreeFileSync::endsWithPathSeparator(directoryFormatted))
            directoryFormatted += GlobalResources::FILE_NAME_SEPARATOR;

        const Zstring filespec = directoryFormatted + DefaultChar('*');

        WIN32_FIND_DATA fileMetaData;
        HANDLE searchHandle = FindFirstFile(filespec.c_str(), //pointer to name of file to search for
                                            &fileMetaData);   //pointer to returned information

        if (searchHandle == INVALID_HANDLE_VALUE)
        {
            const DWORD lastError = GetLastError();
            if (lastError == ERROR_FILE_NOT_FOUND)
                return true;

            //else: we have a problem... report it:
            Zstring errorMessage = Zstring(_("Error traversing directory:")) + wxT("\n\"") + directory + wxT("\"") ;
            if (m_sink->OnError(errorMessage + wxT("\n\n") + FreeFileSync::getLastErrorFormatted(lastError)) == wxDIR_STOP)
                return false;
            else
                return true;
        }
        CloseHandleOnExit dummy(searchHandle);

        do
        {   //don't return "." and ".."
            const wxChar* name = fileMetaData.cFileName;
            if (    name[0] == wxChar('.') &&
                    ((name[1] == wxChar('.') && name[2] == wxChar('\0')) ||
                     name[1] == wxChar('\0')))
                continue;

            const Zstring fullName = directoryFormatted + name;

            if (fileMetaData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) //a directory... (for directory symlinks this flag is set too!)
            {
                switch (m_sink->OnDir(fullName))
                {
                case wxDIR_IGNORE:
                    break;
                case wxDIR_CONTINUE:
                    //traverse into symbolic links, junctions, etc. if requested only:
                    if (traverseDirectorySymlinks || (~fileMetaData.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT))
                        if (!this->traverse(fullName, level + 1))
                            return false;
                    break;
                case wxDIR_STOP:
                    return false;
                default:
                    assert(false);
                }
            }
            else //a file...
            {
                FreeFileSync::FileInfo details;

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

                if (m_sink->OnFile(fullName, details) == wxDIR_STOP)
                    return false;
            }
        }
        while (FindNextFile(searchHandle,	 // handle to search
                            &fileMetaData)); // pointer to structure for data on found file

        const DWORD lastError = GetLastError();
        if (lastError == ERROR_NO_MORE_FILES)
            return true; //everything okay

        //else: we have a problem... report it:
        Zstring errorMessage = Zstring(_("Error traversing directory:")) + wxT("\n\"") + directory + wxT("\"") ;
        if (m_sink->OnError(errorMessage + wxT("\n\n") + FreeFileSync::getLastErrorFormatted(lastError)) == wxDIR_STOP)
            return false;
        else
            return true;

#elif defined FFS_LINUX
        Zstring directoryFormatted = directory;
        if (FreeFileSync::endsWithPathSeparator(directoryFormatted))
            directoryFormatted = directoryFormatted.BeforeLast(GlobalResources::FILE_NAME_SEPARATOR);

        DIR* dirObj = opendir(directoryFormatted.c_str());
        if (dirObj == NULL)
        {
            Zstring errorMessage = Zstring(_("Error traversing directory:")) + wxT("\n\"") + directoryFormatted + wxT("\"") ;
            if (m_sink->OnError(errorMessage + wxT("\n\n") + FreeFileSync::getLastErrorFormatted()) == wxDIR_STOP)
                return false;
            else
                return true;
        }
        CloseDirOnExit dummy(dirObj);

        struct dirent* dirEntry;
        while (!(errno = 0) && (dirEntry = readdir(dirObj)) != NULL) //set errno to 0 as unfortunately this isn't done when readdir() returns NULL when it is finished
        {
            //don't return "." and ".."
            const wxChar* name = dirEntry->d_name;
            if (      name[0] == wxChar('.') &&
                      ((name[1] == wxChar('.') && name[2] == wxChar('\0')) ||
                       name[1] == wxChar('\0')))
                continue;

            const Zstring fullName = directoryFormatted + GlobalResources::FILE_NAME_SEPARATOR + name;

            struct stat fileInfo;
            if (lstat(fullName.c_str(), &fileInfo) != 0) //lstat() does not resolve symlinks
            {
                const Zstring errorMessage = Zstring(_("Error reading file attributes:")) + wxT("\n\"") + fullName + wxT("\"");
                if (m_sink->OnError(errorMessage + wxT("\n\n") + FreeFileSync::getLastErrorFormatted()) == wxDIR_STOP)
                    return false;
                continue;
            }

            const bool isSymbolicLink = S_ISLNK(fileInfo.st_mode);
            if (isSymbolicLink) //dereference symbolic links
            {
                if (stat(fullName.c_str(), &fileInfo) != 0) //stat() resolves symlinks
                {
                    //a broken symbolic link
                    FreeFileSync::FileInfo details;
                    details.lastWriteTimeRaw = 0; //we are not interested in the modifiation time of the link
                    details.fileSize         = 0;

                    if (m_sink->OnFile(fullName, details) == wxDIR_STOP)
                        return false;
                    continue;
                }
            }


            if (S_ISDIR(fileInfo.st_mode)) //a directory... (note: symbolic links need to be dereferenced to test if they point to a directory!)
            {
                switch (m_sink->OnDir(fullName))
                {
                case wxDIR_IGNORE:
                    break;
                case wxDIR_CONTINUE:
                    if (traverseDirectorySymlinks || !isSymbolicLink) //traverse into symbolic links if requested only
                    {
                        if (!this->traverse(fullName, level + 1))
                            return false;
                    }
                    break;
                case wxDIR_STOP:
                    return false;
                default:
                    assert(false);
                }
            }
            else //a file...
            {
                FreeFileSync::FileInfo details;
                details.lastWriteTimeRaw = fileInfo.st_mtime; //UTC time(ANSI C format); unit: 1 second
                details.fileSize         = fileInfo.st_size;

                if (m_sink->OnFile(fullName, details) == wxDIR_STOP)
                    return false;
            }
        }

        if (errno == 0)
            return true; //everything okay

        //else: we have a problem... report it:
        const Zstring errorMessage = Zstring(_("Error traversing directory:")) + wxT("\n\"") + directoryFormatted + wxT("\"") ;
        if (m_sink->OnError(errorMessage + wxT("\n\n") + FreeFileSync::getLastErrorFormatted()) == wxDIR_STOP)
            return false;
        else
            return true;
#endif
    }

private:
    FreeFileSync::FullDetailFileTraverser* m_sink;
};


void FreeFileSync::traverseInDetail(const Zstring& directory,
                                    const bool traverseDirectorySymlinks,
                                    FullDetailFileTraverser* sink)
{
    if (traverseDirectorySymlinks)
    {
        TraverseRecursively<true> filewalker(sink);
        filewalker.traverse(directory, 0);
    }
    else
    {
        TraverseRecursively<false> filewalker(sink);
        filewalker.traverse(directory, 0);
    }
}


/*
#ifdef FFS_WIN
inline
Zstring getDriveName(const Zstring& directoryName) //GetVolume() doesn't work under Linux!
{
    const Zstring volumeName = wxFileName(directoryName.c_str()).GetVolume().c_str();
    if (volumeName.empty())
        return Zstring();

    return volumeName + wxFileName::GetVolumeSeparator().c_str() + GlobalResources::FILE_NAME_SEPARATOR;
}


bool FreeFileSync::isFatDrive(const Zstring& directoryName)
{
    const Zstring driveName = getDriveName(directoryName);
    if (driveName.empty())
        return false;

    wxChar fileSystem[32];
    if (!GetVolumeInformation(driveName.c_str(), NULL, 0, NULL, NULL, NULL, fileSystem, 32))
        return false;

    return Zstring(fileSystem).StartsWith(wxT("FAT"));
}
#endif  //FFS_WIN
*/
