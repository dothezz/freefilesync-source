#include "fileHandling.h"
#include <wx/intl.h>
#include "systemFunctions.h"
#include "globalFunctions.h"
#include "fileTraverser.h"

#ifdef FFS_WIN
#include <wx/msw/wrapwin.h> //includes "windows.h"
#include "shadow.h"

#elif defined FFS_LINUX
#include <sys/stat.h>
#include <stdio.h> //for rename()
#include <time.h>
#include <utime.h>
#include <fstream>
#include <unistd.h>
#include <dirent.h>
#include <errno.h>
#endif

using FreeFileSync::FileError;


Zstring FreeFileSync::getFormattedDirectoryName(const Zstring& dirname)
{   //Formatting is needed since functions expect the directory to end with '\' to be able to split the relative names.
    //Also improves usability.

    Zstring dirnameTmp = dirname;
    dirnameTmp.Trim(true);  //remove whitespace characters from right
    dirnameTmp.Trim(false); //remove whitespace characters from left

    if (dirnameTmp.empty()) //an empty string is interpreted as "\"; this is not desired
        return Zstring();

    if (!dirnameTmp.EndsWith(globalFunctions::FILE_NAME_SEPARATOR))
        dirnameTmp += globalFunctions::FILE_NAME_SEPARATOR;

    //don't do directory formatting with wxFileName, as it doesn't respect //?/ - prefix
    return dirnameTmp;
}


class RecycleBin
{
public:
    static const RecycleBin& getInstance()
    {
        static RecycleBin instance; //lazy creation of RecycleBin
        return instance;
    }

    bool recycleBinExists() const
    {
        return recycleBinAvailable;
    }

    bool moveToRecycleBin(const Zstring& filename) const; //throw (RuntimeException)

private:
    RecycleBin() :
            recycleBinAvailable(false)
    {
#ifdef FFS_WIN
        recycleBinAvailable = true;
#endif  // FFS_WIN
    }

    ~RecycleBin() {}

private:
    bool recycleBinAvailable;
};


bool RecycleBin::moveToRecycleBin(const Zstring& filename) const //throw (RuntimeException)
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

    if (SHFileOperation(&fileOp) != 0 || fileOp.fAnyOperationsAborted) return false;
#endif

    return true;
}


bool FreeFileSync::recycleBinExists()
{
    return RecycleBin::getInstance().recycleBinExists();
}


inline
bool moveToRecycleBin(const Zstring& filename) //throw (RuntimeException)
{
    return RecycleBin::getInstance().moveToRecycleBin(filename);
}


bool FreeFileSync::fileExists(const DefaultChar* filename)
{ //symbolic links (broken or not) are also treated as existing files!
#ifdef FFS_WIN
    // we must use GetFileAttributes() instead of the ANSI C functions because
    // it can cope with network (UNC) paths unlike them
    const DWORD ret = ::GetFileAttributes(filename);

    return (ret != INVALID_FILE_ATTRIBUTES) && !(ret & FILE_ATTRIBUTE_DIRECTORY); //returns true for (file-)symlinks also

#elif defined FFS_LINUX
    struct stat fileInfo;
    return (::lstat(filename, &fileInfo) == 0 &&
            (S_ISLNK(fileInfo.st_mode) || S_ISREG(fileInfo.st_mode))); //in Linux a symbolic link is neither file nor directory
#endif
}


bool FreeFileSync::dirExists(const DefaultChar* dirname)
{ //symbolic links (broken or not) are also treated as existing directories!
#ifdef FFS_WIN
    // we must use GetFileAttributes() instead of the ANSI C functions because
    // it can cope with network (UNC) paths unlike them
    const DWORD ret = ::GetFileAttributes(dirname);

    return (ret != INVALID_FILE_ATTRIBUTES) && (ret & FILE_ATTRIBUTE_DIRECTORY); //returns true for (dir-)symlinks also

#elif defined FFS_LINUX
    struct stat dirInfo;
    return (::lstat(dirname, &dirInfo) == 0 &&
            (S_ISLNK(dirInfo.st_mode) || S_ISDIR(dirInfo.st_mode))); //in Linux a symbolic link is neither file nor directory
#endif
}


bool FreeFileSync::symlinkExists(const DefaultChar* objname)
{
#ifdef FFS_WIN
    const DWORD ret = ::GetFileAttributes(objname);
    return (ret != INVALID_FILE_ATTRIBUTES) && (ret & FILE_ATTRIBUTE_REPARSE_POINT);

#elif defined FFS_LINUX
    struct stat fileInfo;
    return (::lstat(objname, &fileInfo) == 0 &&
            S_ISLNK(fileInfo.st_mode)); //symbolic link
#endif
}


void FreeFileSync::removeFile(const Zstring& filename, const bool useRecycleBin) //throw (FileError, ::RuntimeException);
{
    //no error situation if file is not existing! manual deletion relies on it!
#ifdef FFS_WIN
    if (::GetFileAttributes(filename.c_str()) == INVALID_FILE_ATTRIBUTES)
        return; //neither file nor any other object with that name existing

#elif defined FFS_LINUX
    struct stat fileInfo;
    if (::lstat(filename.c_str(), &fileInfo) != 0)
        return; //neither file nor any other object (e.g. broken symlink) with that name existing
#endif

    if (useRecycleBin)
    {
        if (!moveToRecycleBin(filename))
            throw FileError(wxString(_("Error moving to Recycle Bin:")) + wxT("\n\"") + filename.c_str() + wxT("\""));
        return;
    }

#ifdef FFS_WIN
    //initialize file attributes
    if (!::SetFileAttributes(
                filename.c_str(),       //address of filename
                FILE_ATTRIBUTE_NORMAL)) //attributes to set
    {
        wxString errorMessage = wxString(_("Error deleting file:")) + wxT("\n\"") + filename.c_str() + wxT("\"");
        throw FileError(errorMessage + wxT("\n\n") + FreeFileSync::getLastErrorFormatted());
    }

    //remove file, support for \\?\-prefix
    if (!::DeleteFile(filename.c_str()))
    {
        wxString errorMessage = wxString(_("Error deleting file:")) + wxT("\n\"") + filename.c_str() + wxT("\"");
        throw FileError(errorMessage + wxT("\n\n") + FreeFileSync::getLastErrorFormatted());
    }
#elif defined FFS_LINUX
    if (::unlink(filename.c_str()) != 0)
    {
        wxString errorMessage = wxString(_("Error deleting file:")) + wxT("\n\"") + filename.c_str() + wxT("\"");
        throw FileError(errorMessage + wxT("\n\n") + FreeFileSync::getLastErrorFormatted());
    }
#endif
}


using FreeFileSync::MoveFileCallback;

class CopyCallbackImpl : public FreeFileSync::CopyFileCallback //callback functionality
{
public:
    CopyCallbackImpl(MoveFileCallback* callback) : moveCallback(callback) {}

    virtual Response updateCopyStatus(const wxULongLong& totalBytesTransferred)
    {
        switch (moveCallback->requestUiRefresh())
        {
        case MoveFileCallback::CONTINUE:
            return CopyFileCallback::CONTINUE;

        case MoveFileCallback::CANCEL:
            return CopyFileCallback::CANCEL;
        }
        return CopyFileCallback::CONTINUE; //dummy return value
    }

private:
    MoveFileCallback* moveCallback;
};


void FreeFileSync::moveFile(const Zstring& sourceFile, const Zstring& targetFile, MoveFileCallback* callback)   //throw (FileError);
{
    if (fileExists(targetFile)) //test file existence: e.g. Linux might silently overwrite existing symlinks
    {
        const wxString errorMessage = wxString(_("Error moving file:")) + wxT("\n\"") + sourceFile +  wxT("\" ->\n\"") + targetFile + wxT("\"");
        throw FileError(errorMessage + wxT("\n\n") + _("Target file already existing!"));
    }

    //moving of symbolic links should work correctly:

#ifdef FFS_WIN
    //first try to move the file directly without copying
    if (::MoveFileEx(sourceFile.c_str(), //__in      LPCTSTR lpExistingFileName,
                     targetFile.c_str(), //__in_opt  LPCTSTR lpNewFileName,
                     0))                 //__in      DWORD dwFlags
        return;

    //if moving failed treat as error (except when it tried to move to a different volume: in this case we will copy the file)
    const DWORD lastError = ::GetLastError();
    if (lastError != ERROR_NOT_SAME_DEVICE)
    {
        const wxString errorMessage = wxString(_("Error moving file:")) + wxT("\n\"") + sourceFile.c_str() +  wxT("\" ->\n\"") + targetFile.c_str() + wxT("\"");
        throw FileError(errorMessage + wxT("\n\n") + FreeFileSync::getLastErrorFormatted(lastError));
    }

    //file is on a different volume: let's copy it
    std::auto_ptr<CopyCallbackImpl> copyCallback(callback != NULL ? new CopyCallbackImpl(callback) : NULL);

    copyFile(sourceFile,
             targetFile,
             true, //copy symbolic links
             NULL, //supply handler for making shadow copies
             copyCallback.get()); //throw (FileError);

#elif defined FFS_LINUX
    //first try to move the file directly without copying
    if (rename(sourceFile.c_str(), targetFile.c_str()) == 0)
        return;

    //if moving failed treat as error (except when it tried to move to a different volume: in this case we will copy the file)
    if (errno != EXDEV)
    {
        const wxString errorMessage = wxString(_("Error moving file:")) + wxT("\n\"") + sourceFile.c_str() +  wxT("\" ->\n\"") + targetFile.c_str() + wxT("\"");
        throw FileError(errorMessage + wxT("\n\n") + FreeFileSync::getLastErrorFormatted(errno));
    }

    //file is on a different volume: let's copy it
    std::auto_ptr<CopyCallbackImpl> copyCallback(callback != NULL ? new CopyCallbackImpl(callback) : NULL);

    copyFile(sourceFile,
             targetFile,
             true, //copy symbolic links
             copyCallback.get()); //throw (FileError);
#endif

    //attention: if copy-operation was cancelled an exception is thrown => sourcefile is not deleted, as we wish!

    removeFile(sourceFile, false);
}


class TraverseOneLevel : public FreeFileSync::TraverseCallback
{
public:
    typedef std::vector<std::pair<Zstring, Zstring> > NamePair;

    TraverseOneLevel(NamePair& filesShort, NamePair& dirsShort) :
            m_files(filesShort),
            m_dirs(dirsShort) {}

    virtual ReturnValue onFile(const DefaultChar* shortName, const Zstring& fullName, const FileInfo& details)
    {
        m_files.push_back(std::make_pair(Zstring(shortName), fullName));
        return TRAVERSING_CONTINUE;
    }
    virtual ReturnValDir onDir(const DefaultChar* shortName, const Zstring& fullName)
    {
        m_dirs.push_back(std::make_pair(Zstring(shortName), fullName));
        return ReturnValDir::Ignore(); //DON'T traverse into subdirs; moveDirectory works recursively!
    }
    virtual ReturnValue onError(const wxString& errorText)
    {
        throw FileError(errorText);
    }

private:
    NamePair& m_files;
    NamePair& m_dirs;
};


void moveDirectoryImpl(const Zstring& sourceDir, const Zstring& targetDir, bool ignoreExistingDirs, MoveFileCallback* callback)   //throw (FileError);
{
    //handle symbolic links
    if (FreeFileSync::symlinkExists(sourceDir))
    {
        FreeFileSync::createDirectory(targetDir, sourceDir, true); //copy symbolic link
        FreeFileSync::removeDirectory(sourceDir, false);           //if target is already another symlink or directory, sourceDir-symlink is silently deleted
        return;
    }

    if (FreeFileSync::dirExists(targetDir))
    {
        if (!ignoreExistingDirs) //directory or symlink exists
        {
            const wxString errorMessage = wxString(_("Error moving directory:")) + wxT("\n\"") + sourceDir +  wxT("\" ->\n\"") + targetDir + wxT("\"");
            throw FileError(errorMessage + wxT("\n\n") + _("Target directory already existing!"));
        }
    }
    else
    {
        //first try to move the directory directly without copying
#ifdef FFS_WIN
        if (::MoveFileEx(sourceDir.c_str(), //__in      LPCTSTR lpExistingFileName,
                         targetDir.c_str(), //__in_opt  LPCTSTR lpNewFileName,
                         0))                //__in      DWORD dwFlags
            return;

        //if moving failed treat as error (except when it tried to move to a different volume: in this case we will copy the directory)
        const DWORD lastError = ::GetLastError();
        if (lastError != ERROR_NOT_SAME_DEVICE)
        {
            const wxString errorMessage = wxString(_("Error moving directory:")) + wxT("\n\"") + sourceDir.c_str() +  wxT("\" ->\n\"") + targetDir.c_str() + wxT("\"");
            throw FileError(errorMessage + wxT("\n\n") + FreeFileSync::getLastErrorFormatted(lastError));
        }
#elif defined FFS_LINUX
        if (::rename(sourceDir.c_str(), targetDir.c_str()) == 0)
            return;

        //if moving failed treat as error (except when it tried to move to a different volume: in this case we will copy the directory)
        if (errno != EXDEV)
        {
            const wxString errorMessage = wxString(_("Error moving directory:")) + wxT("\n\"") + sourceDir.c_str() +  wxT("\" ->\n\"") + targetDir.c_str() + wxT("\"");
            throw FileError(errorMessage + wxT("\n\n") + FreeFileSync::getLastErrorFormatted(errno));
        }
#endif

        //create target
        FreeFileSync::createDirectory(targetDir, sourceDir, false); //throw (FileError);
    }

    //call back once per folder
    if (callback)
        switch (callback->requestUiRefresh())
        {
        case MoveFileCallback::CONTINUE:
            break;
        case MoveFileCallback::CANCEL:
            //an user aborted operation IS an error condition!
            throw FileError(wxString(_("Error moving directory:")) + wxT("\n\"") + sourceDir.c_str() +  wxT("\" ->\n\"") +
                            targetDir.c_str() + wxT("\"") + wxT("\n\n") + _("Operation aborted!"));
        }

    //move files/folders recursively
    TraverseOneLevel::NamePair fileList; //list of names: 1. short 2.long
    TraverseOneLevel::NamePair dirList;  //

    //traverse source directory one level
    TraverseOneLevel traverseCallback(fileList, dirList);
    FreeFileSync::traverseFolder(sourceDir, false, &traverseCallback); //traverse one level

    const Zstring targetDirFormatted = targetDir.EndsWith(globalFunctions::FILE_NAME_SEPARATOR) ? //ends with path separator
                                       targetDir :
                                       targetDir + globalFunctions::FILE_NAME_SEPARATOR;

    //move files
    for (TraverseOneLevel::NamePair::const_iterator i = fileList.begin(); i != fileList.end(); ++i)
        FreeFileSync::moveFile(i->second, targetDirFormatted + i->first, callback);

    //move directories
    for (TraverseOneLevel::NamePair::const_iterator i = dirList.begin(); i != dirList.end(); ++i)
        ::moveDirectoryImpl(i->second, targetDirFormatted + i->first, true, callback);

    //attention: if move-operation was cancelled an exception is thrown => sourceDir is not deleted, as we wish!

    //delete source
    FreeFileSync::removeDirectory(sourceDir, false); //throw (FileError);

    return;
}


void FreeFileSync::moveDirectory(const Zstring& sourceDir, const Zstring& targetDir, bool ignoreExistingDirs, MoveFileCallback* callback)   //throw (FileError);
{
#ifdef FFS_WIN
    const Zstring& sourceDirFormatted = sourceDir;
    const Zstring& targetDirFormatted = targetDir;
#elif defined FFS_LINUX
    const Zstring sourceDirFormatted = //remove trailing slash
        sourceDir.size() > 1 && sourceDir.EndsWith(globalFunctions::FILE_NAME_SEPARATOR) ?  //exception: allow '/'
        sourceDir.BeforeLast(globalFunctions::FILE_NAME_SEPARATOR) :
        sourceDir;
    const Zstring targetDirFormatted = //remove trailing slash
        targetDir.size() > 1 && targetDir.EndsWith(globalFunctions::FILE_NAME_SEPARATOR) ?  //exception: allow '/'
        targetDir.BeforeLast(globalFunctions::FILE_NAME_SEPARATOR) :
        targetDir;
#endif

    ::moveDirectoryImpl(sourceDirFormatted, targetDirFormatted, ignoreExistingDirs, callback);
}


class FilesDirsOnlyTraverser : public FreeFileSync::TraverseCallback
{
public:
    FilesDirsOnlyTraverser(std::vector<Zstring>& files, std::vector<Zstring>& dirs) :
            m_files(files),
            m_dirs(dirs) {}

    virtual ReturnValue onFile(const DefaultChar* shortName, const Zstring& fullName, const FileInfo& details)
    {
        m_files.push_back(fullName);
        return TRAVERSING_CONTINUE;
    }
    virtual ReturnValDir onDir(const DefaultChar* shortName, const Zstring& fullName)
    {
        m_dirs.push_back(fullName);
        return ReturnValDir::Ignore(); //DON'T traverse into subdirs; removeDirectory works recursively!
    }
    virtual ReturnValue onError(const wxString& errorText)
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
            throw FileError(wxString(_("Error moving to Recycle Bin:")) + wxT("\n\"") + directory.c_str() + wxT("\""));
        return;
    }

//attention: check if directory is a symlink! Do NOT traverse into it deleting contained files!!!
#ifdef FFS_WIN
    if (dirAttr & FILE_ATTRIBUTE_REPARSE_POINT) //remove symlink directly, support for \\?\-prefix
    {
        if (!::SetFileAttributes(           // initialize file attributes: actually NEEDED for symbolic links also!
                    directory.c_str(),      // address of directory name
                    FILE_ATTRIBUTE_NORMAL)) // attributes to set
        {
            wxString errorMessage = wxString(_("Error deleting directory:")) + wxT("\n\"") + directory.c_str() + wxT("\"");
            throw FileError(errorMessage + wxT("\n\n") + FreeFileSync::getLastErrorFormatted());
        }

        if (!::RemoveDirectory(directory.c_str()))
        {
            wxString errorMessage = wxString(_("Error deleting directory:")) + wxT("\n\"") + directory.c_str() + wxT("\"");
            throw FileError(errorMessage + wxT("\n\n") + FreeFileSync::getLastErrorFormatted());
        }
        return;
    }

#elif defined FFS_LINUX
    if (S_ISLNK(dirInfo.st_mode))
    {
        if (::unlink(directory.c_str()) != 0)
        {
            wxString errorMessage = wxString(_("Error deleting directory:")) + wxT("\n\"") + directory.c_str() + wxT("\"");
            throw FileError(errorMessage + wxT("\n\n") + FreeFileSync::getLastErrorFormatted());
        }
        return;
    }
#endif

    std::vector<Zstring> fileList;
    std::vector<Zstring> dirList;

    //get all files and directories from current directory (WITHOUT subdirectories!)
    FilesDirsOnlyTraverser traverser(fileList, dirList);
    FreeFileSync::traverseFolder(directory, false, &traverser);

    //delete files
    for (std::vector<Zstring>::const_iterator j = fileList.begin(); j != fileList.end(); ++j)
        FreeFileSync::removeFile(*j, false);

    //delete directories recursively
    for (std::vector<Zstring>::const_iterator j = dirList.begin(); j != dirList.end(); ++j)
        FreeFileSync::removeDirectory(*j, false); //call recursively to correctly handle symbolic links

    //parent directory is deleted last
#ifdef FFS_WIN
    //initialize file attributes
    if (!::SetFileAttributes(
                directory.c_str(),      // address of directory name
                FILE_ATTRIBUTE_NORMAL)) // attributes to set
    {
        wxString errorMessage = wxString(_("Error deleting directory:")) + wxT("\n\"") + directory.c_str() + wxT("\"");
        throw FileError(errorMessage + wxT("\n\n") + FreeFileSync::getLastErrorFormatted());
    }

    //remove directory, support for \\?\-prefix
    if (!::RemoveDirectory(directory.c_str()))
    {
        wxString errorMessage = wxString(_("Error deleting directory:")) + wxT("\n\"") + directory.c_str() + wxT("\"");
        throw FileError(errorMessage + wxT("\n\n") + FreeFileSync::getLastErrorFormatted());
    }
#else
    if (::rmdir(directory.c_str()) != 0)
    {
        wxString errorMessage = wxString(_("Error deleting directory:")) + wxT("\n\"") + directory.c_str() + wxT("\"");
        throw FileError(errorMessage + wxT("\n\n") + FreeFileSync::getLastErrorFormatted());
    }
#endif
}


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


class KernelDllHandler //dynamically load windows API functions
{
    typedef DWORD (WINAPI *GetFinalPath)(
        HANDLE hFile,
        LPTSTR lpszFilePath,
        DWORD cchFilePath,
        DWORD dwFlags);

public:
    static const KernelDllHandler& getInstance() //lazy creation of KernelDllHandler
    {
        static KernelDllHandler instance;
        return instance;
    }

    GetFinalPath getFinalPathNameByHandle;

private:
    KernelDllHandler() :
            getFinalPathNameByHandle(NULL),
            hKernel(NULL)
    {
        //get a handle to the DLL module containing required functionality
        hKernel = ::LoadLibrary(wxT("kernel32.dll"));
        if (hKernel)
            getFinalPathNameByHandle = reinterpret_cast<GetFinalPath>(::GetProcAddress(hKernel, "GetFinalPathNameByHandleW")); //load unicode version!
    }

    ~KernelDllHandler()
    {
        if (hKernel) ::FreeLibrary(hKernel);
    }

    HINSTANCE hKernel;
};


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

    if (KernelDllHandler::getInstance().getFinalPathNameByHandle == NULL )
        throw FileError(wxString(_("Error loading library function:")) + wxT("\n\"") + wxT("GetFinalPathNameByHandleW") + wxT("\""));

    const unsigned int BUFFER_SIZE = 10000;
    TCHAR targetPath[BUFFER_SIZE];

    const DWORD rv = KernelDllHandler::getInstance().getFinalPathNameByHandle(
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
    if (FreeFileSync::dirExists(directory))
        return;

    if (level == 100) //catch endless recursion
        return;

    //try to create parent folders first
    const Zstring dirParent = directory.BeforeLast(globalFunctions::FILE_NAME_SEPARATOR);
    if (!dirParent.empty() && !FreeFileSync::dirExists(dirParent))
    {
        //call function recursively
        const Zstring templateParent = templateDir.BeforeLast(globalFunctions::FILE_NAME_SEPARATOR);
        createDirectoryRecursively(dirParent, templateParent, false, level + 1); //don't create symbolic links in recursion!
    }

    //now creation should be possible
#ifdef FFS_WIN
    const DWORD templateAttr = ::GetFileAttributes(templateDir.c_str()); //replaces wxDirExists(): also returns successful for broken symlinks
    if (templateAttr == INVALID_FILE_ATTRIBUTES) //fallback
    {
        if (!::CreateDirectory(directory.c_str(), // pointer to a directory path string
                               NULL) && level == 0)
        {
            const wxString errorMessage = wxString(_("Error creating directory:")) + wxT("\n\"") + directory.c_str() + wxT("\"");
            throw FileError(errorMessage + wxT("\n\n") + FreeFileSync::getLastErrorFormatted());
        }
    }
    else
    {
        //symbolic link handling
        if (!copyDirectorySymLinks && templateAttr & FILE_ATTRIBUTE_REPARSE_POINT) //create directory based on target of symbolic link
        {
            //get target directory of symbolic link
            const Zstring linkPath = resolveDirectorySymlink(templateDir);
            if (linkPath.empty())
            {
                if (level == 0)
                    throw FileError(wxString(_("Error resolving symbolic link:")) + wxT("\n\"") + templateDir.c_str() + wxT("\""));
            }
            else
            {
                if (!::CreateDirectoryEx(      // this function automatically copies symbolic links if encountered
                            linkPath.c_str(),  // pointer to path string of template directory
                            directory.c_str(), // pointer to a directory path string
                            NULL) && level == 0)
                {
                    const wxString errorMessage = wxString(_("Error creating directory:")) + wxT("\n\"") + directory.c_str() + wxT("\"");
                    throw FileError(errorMessage + wxT("\n\n") + FreeFileSync::getLastErrorFormatted());
                }
            }
        }
        else //in all other cases
        {
            if (!::CreateDirectoryEx(        // this function automatically copies symbolic links if encountered
                        templateDir.c_str(), // pointer to path string of template directory
                        directory.c_str(),	 // pointer to a directory path string
                        NULL) && level == 0)
            {
                const wxString errorMessage = wxString(_("Error creating directory:")) + wxT("\n\"") + directory.c_str() + wxT("\"");
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
                wxString errorMessage = wxString(_("Error resolving symbolic link:")) + wxT("\n\"") + templateDir.c_str() + wxT("\"");
                if (bytesWritten < 0) errorMessage += wxString(wxT("\n\n")) + FreeFileSync::getLastErrorFormatted();
                throw FileError(errorMessage);
            }
            //set null-terminating char
            buffer[bytesWritten] = 0;

            if (symlink(buffer, directory.c_str()) != 0)
            {
                wxString errorMessage = wxString(_("Error creating directory:")) + wxT("\n\"") + directory.c_str() + wxT("\"");
                throw FileError(errorMessage + wxT("\n\n") + FreeFileSync::getLastErrorFormatted());
            }
            return; //symlink created successfully
        }
    }

    //default directory creation
    if (mkdir(directory.c_str(), 0755) != 0 && level == 0)
    {
        wxString errorMessage = wxString(_("Error creating directory:")) + wxT("\n\"") + directory.c_str() + wxT("\"");
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
    const Zstring dirFormatted = directory.EndsWith(globalFunctions::FILE_NAME_SEPARATOR) ?
                                 directory.BeforeLast(globalFunctions::FILE_NAME_SEPARATOR) :
                                 directory;

    const Zstring templateFormatted = templateDir.EndsWith(globalFunctions::FILE_NAME_SEPARATOR) ?
                                      templateDir.BeforeLast(globalFunctions::FILE_NAME_SEPARATOR) :
                                      templateDir;

    createDirectoryRecursively(dirFormatted, templateFormatted, copyDirectorySymLinks, 0);
}


#ifdef FFS_WIN

#ifndef COPY_FILE_COPY_SYMLINK
const DWORD COPY_FILE_COPY_SYMLINK = 0x00000800;
#endif

DWORD CALLBACK copyCallbackInternal(
    LARGE_INTEGER totalFileSize,
    LARGE_INTEGER totalBytesTransferred,
    LARGE_INTEGER streamSize,
    LARGE_INTEGER streamBytesTransferred,
    DWORD dwStreamNumber,
    DWORD dwCallbackReason,
    HANDLE hSourceFile,
    HANDLE hDestinationFile,
    LPVOID lpData)
{
    using FreeFileSync::CopyFileCallback;

    //small performance optimization: it seems this callback function is called for every 64 kB (depending on cluster size).
    static unsigned int callNr = 0;
    if (++callNr % 50 == 0) //reduce by factor of 50 =^ 10-20 calls/sec
    {
        if (lpData != NULL)
        {
            //some odd check for some possible(?) error condition
            if (totalBytesTransferred.HighPart < 0) //let's see if someone answers the call...
                ::MessageBox(NULL, wxT("You've just discovered a bug in WIN32 API function \"CopyFileEx\"! \n\n\
            Please write a mail to the author of FreeFileSync at zhnmju123@gmx.de and simply state that\n\
            \"totalBytesTransferred.HighPart can be below zero\"!\n\n\
            This will then be handled in future versions of FreeFileSync.\n\nThanks -ZenJu"),
                             NULL, 0);


            CopyFileCallback* callback = static_cast<CopyFileCallback*>(lpData);

            switch (callback->updateCopyStatus(wxULongLong(totalBytesTransferred.HighPart, totalBytesTransferred.LowPart)))
            {
            case CopyFileCallback::CONTINUE:
                break;
            case CopyFileCallback::CANCEL:
                return PROGRESS_CANCEL;
            }
        }
    }

    return PROGRESS_CONTINUE;
}


bool supportForSymbolicLinks()
{
    OSVERSIONINFO osvi;
    ZeroMemory(&osvi, sizeof(OSVERSIONINFO));
    osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);

    //symbolic links are supported starting with Vista
    if (GetVersionEx(&osvi))
        return osvi.dwMajorVersion > 5; //XP has majorVersion == 5, minorVersion == 1, Vista majorVersion == 6

    return false;
}


void FreeFileSync::copyFile(const Zstring& sourceFile,
                            const Zstring& targetFile,
                            const bool copyFileSymLinks,
                            ShadowCopy* shadowCopyHandler,
                            CopyFileCallback* callback)
{
    DWORD copyFlags = COPY_FILE_FAIL_IF_EXISTS;

    //copy symbolic links instead of the files pointed at
    static const bool symlinksSupported = supportForSymbolicLinks(); //only set "true" if supported by OS: else copying in Windows XP fails
    if (copyFileSymLinks && symlinksSupported)
        copyFlags |= COPY_FILE_COPY_SYMLINK;

    if (!::CopyFileEx( //same performance as CopyFile()
                sourceFile.c_str(),
                targetFile.c_str(),
                copyCallbackInternal,
                callback,
                NULL,
                copyFlags))
    {
        const DWORD lastError = ::GetLastError();

        //don't suppress "lastError == ERROR_REQUEST_ABORTED": an user aborted operation IS an error condition!

        //if file is locked (try to) use Windows Volume Shadow Copy Service
        if (lastError == ERROR_SHARING_VIOLATION && shadowCopyHandler != NULL)
        {
            const Zstring shadowFilename(shadowCopyHandler->makeShadowCopy(sourceFile));
            FreeFileSync::copyFile(shadowFilename, //transferred bytes is automatically reset when new file is copied
                                   targetFile,
                                   copyFileSymLinks,
                                   shadowCopyHandler,
                                   callback);
            return;
        }

        const wxString errorMessage = wxString(_("Error copying file:")) + wxT("\n\"") + sourceFile.c_str() +  wxT("\" ->\n\"") + targetFile.c_str() + wxT("\"");
        throw FileError(errorMessage + wxT("\n\n") + FreeFileSync::getLastErrorFormatted(lastError));
    }
}


#elif defined FFS_LINUX
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
                            CopyFileCallback* callback)
{
    using FreeFileSync::CopyFileCallback;

    if (FreeFileSync::fileExists(targetFile.c_str()))
        throw FileError(wxString(_("Error copying file:")) + wxT("\n\"") + sourceFile.c_str() +  wxT("\" ->\n\"") + targetFile.c_str() + wxT("\"\n\n")
                        + _("Target file already existing!"));

    //symbolic link handling
    if (copyFileSymLinks)
    {
        //test if sourceFile is a symbolic link
        struct stat linkInfo;
        if (lstat(sourceFile.c_str(), &linkInfo) != 0)
        {
            const wxString errorMessage = wxString(_("Error reading file attributes:")) + wxT("\n\"") + sourceFile.c_str() + wxT("\"");
            throw FileError(errorMessage + wxT("\n\n") + FreeFileSync::getLastErrorFormatted());
        }

        if (S_ISLNK(linkInfo.st_mode))
        {
            //copy symbolic link
            const int BUFFER_SIZE = 10000;
            char buffer[BUFFER_SIZE];
            const int bytesWritten = readlink(sourceFile.c_str(), buffer, BUFFER_SIZE);
            if (bytesWritten < 0 || bytesWritten == BUFFER_SIZE)
            {
                wxString errorMessage = wxString(_("Error resolving symbolic link:")) + wxT("\n\"") + sourceFile.c_str() + wxT("\"");
                if (bytesWritten < 0) errorMessage += wxString(wxT("\n\n")) + FreeFileSync::getLastErrorFormatted();
                throw FileError(errorMessage);
            }
            //set null-terminating char
            buffer[bytesWritten] = 0;

            if (symlink(buffer, targetFile.c_str()) != 0)
            {
                const wxString errorMessage = wxString(_("Error writing file:")) + wxT("\n\"") + targetFile.c_str() + wxT("\"");
                throw FileError(errorMessage + wxT("\n\n") + FreeFileSync::getLastErrorFormatted());
            }

            return; //symlink created successfully
        }
    }

    //begin of regular file copy
    struct stat fileInfo;
    if (stat(sourceFile.c_str(), &fileInfo) != 0) //read file attributes from source file (resolving symlinks)
    {
        const wxString errorMessage = wxString(_("Error reading file attributes:")) + wxT("\n\"") + sourceFile.c_str() + wxT("\"");
        throw FileError(errorMessage + wxT("\n\n") + FreeFileSync::getLastErrorFormatted());
    }

    //open sourceFile for reading
    std::ifstream fileIn(sourceFile.c_str(), std::ios_base::binary);
    if (fileIn.fail())
        throw FileError(wxString(_("Error opening file:")) + wxT("\n\"") + sourceFile.c_str() +  wxT("\""));

    //create targetFile and open it for writing
    std::ofstream fileOut(targetFile.c_str(), std::ios_base::binary);
    if (fileOut.fail())
        throw FileError(wxString(_("Error opening file:")) + wxT("\n\"") + targetFile.c_str() + wxT("\""));

    try
    {
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
                    throw FileError(wxString(_("Error writing file:")) + wxT("\n\"") + targetFile.c_str() + wxT("\""));
                break;
            }
            else if (fileIn.fail())
                throw FileError(wxString(_("Error reading file:")) + wxT("\n\"") + sourceFile.c_str() + wxT("\""));


            fileOut.write(memory.buffer, memory.bufferSize);
            if (fileOut.bad())
                throw FileError(wxString(_("Error writing file:")) + wxT("\n\"") + targetFile.c_str() + wxT("\""));

            totalBytesTransferred += memory.bufferSize;

            //invoke callback method to update progress indicators
            if (callback != NULL)
            {
                switch (callback->updateCopyStatus(totalBytesTransferred))
                {
                case CopyFileCallback::CONTINUE:
                    break;

                case CopyFileCallback::CANCEL:
                    //an user aborted operation IS an error condition!
                    throw FileError(wxString(_("Error copying file:")) + wxT("\n\"") + sourceFile.c_str() +  wxT("\" ->\n\"") +
                                    targetFile.c_str() + wxT("\"\n\n") + _("Operation aborted!"));
                }
            }
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
            wxString errorMessage = wxString(_("Error changing modification time:")) + wxT("\n\"") + targetFile.c_str() + wxT("\"");
            throw FileError(errorMessage + wxT("\n\n") + FreeFileSync::getLastErrorFormatted());
        }

        //set file access rights
        if (chmod(targetFile.c_str(), fileInfo.st_mode) != 0)
        {
            wxString errorMessage = wxString(_("Error writing file attributes:")) + wxT("\n\"") + targetFile.c_str() + wxT("\"");
            throw FileError(errorMessage + wxT("\n\n") + FreeFileSync::getLastErrorFormatted());
        }
    }
    catch (...)
    {   //try to delete target file if error occured, or exception was thrown in callback function
        if (FreeFileSync::fileExists(targetFile))
            ::unlink(targetFile); //don't handle error situations!

        throw;
    }
}
#endif



/*
#ifdef FFS_WIN
inline
Zstring getDriveName(const Zstring& directoryName) //GetVolume() doesn't work under Linux!
{
    const Zstring volumeName = wxFileName(directoryName.c_str()).GetVolume().c_str();
    if (volumeName.empty())
        return Zstring();

    return volumeName + wxFileName::GetVolumeSeparator().c_str() + globalFunctions::FILE_NAME_SEPARATOR;
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