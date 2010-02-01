#include "fileHandling.h"
#include <wx/intl.h>
#include "systemFunctions.h"
#include "globalFunctions.h"
#include "systemConstants.h"
#include "fileTraverser.h"
#include <wx/file.h>
#include <stdexcept>
#include <boost/bind.hpp>
#include <algorithm>
#include <wx/log.h>
#include <wx/datetime.h>
#include "stringConv.h"
#include <wx/utils.h>

#ifdef FFS_WIN
#include "recycler.h"
#include "dllLoader.h"
#include <wx/msw/wrapwin.h> //includes "windows.h"
#include "shadow.h"
#include "longPathPrefix.h"

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


bool replaceMacro(wxString& macro) //macro without %-characters, return true if replaced successfully
{
    if (macro.IsEmpty())
        return false;

    //there are equally named environment variables %TIME%, %DATE% existing, so replace these first!
    if (macro.CmpNoCase(wxT("time")) == 0)
    {
        macro = wxDateTime::Now().FormatISOTime();
        macro.Replace(wxT(":"), wxT("-"));
        return true;
    }

    if (macro.CmpNoCase(wxT("date")) == 0)
    {
        macro = wxDateTime::Now().FormatISODate();
        return true;
    }

    //try to apply environment variables
    wxString envValue;
    if (wxGetEnv(macro, &envValue))
    {
        macro = envValue;
        return true;
    }

    return false;
}


void expandMacros(wxString& text)
{
    const wxChar SEPARATOR = '%';

    if (text.Find(SEPARATOR) != wxNOT_FOUND)
    {
        wxString prefix  = text.BeforeFirst(SEPARATOR);
        wxString postfix = text.AfterFirst(SEPARATOR);
        if (postfix.Find(SEPARATOR) != wxNOT_FOUND)
        {
            wxString potentialMacro = postfix.BeforeFirst(SEPARATOR);
            wxString rest           = postfix.AfterFirst(SEPARATOR); //text == prefix + SEPARATOR + potentialMacro + SEPARATOR + rest

            if (replaceMacro(potentialMacro))
            {
                expandMacros(rest);
                text = prefix + potentialMacro + rest;
            }
            else
            {
                rest = wxString() + SEPARATOR + rest;
                expandMacros(rest);
                text = prefix + SEPARATOR + potentialMacro + rest;
            }
        }
    }
}


Zstring FreeFileSync::getFormattedDirectoryName(const Zstring& dirname)
{
    //Formatting is needed since functions expect the directory to end with '\' to be able to split the relative names.
    //note: don't do directory formatting with wxFileName, as it doesn't respect //?/ - prefix!

    wxString dirnameTmp = zToWx(dirname);
    dirnameTmp.Trim(true);  //remove whitespace characters from right
    dirnameTmp.Trim(false); //remove whitespace characters from left

    if (dirnameTmp.empty()) //an empty string is interpreted as "\"; this is not desired
        return Zstring();

    //replace macros
    expandMacros(dirnameTmp);

#ifdef FFS_WIN
    /*
    resolve relative names; required by:
     - \\?\-prefix which needs absolute names
     - Volume Shadow Copy: volume name needs to be part of each filename
     - file icon buffer (at least for extensions that are acutally read from disk, e.g. "exe")
     - detection of dependent directories, e.g. "\" and "C:\test"
     */
    dirnameTmp = resolveRelativePath(dirnameTmp.c_str()).c_str();
#endif

    if (!dirnameTmp.EndsWith(zToWx(globalFunctions::FILE_NAME_SEPARATOR)))
        dirnameTmp += zToWx(globalFunctions::FILE_NAME_SEPARATOR);

    return wxToZ(dirnameTmp);
}


bool FreeFileSync::recycleBinExists()
{
#ifdef FFS_WIN
    return true;
#else
    return false;
#endif  // FFS_WIN
}


inline
void moveToRecycleBin(const Zstring& filename) //throw (std::logic_error), throw (FileError)
{
    if (!FreeFileSync::recycleBinExists())   //this method should ONLY be called if recycle bin is available
        throw std::logic_error("Initialization of Recycle Bin failed!");

#ifdef FFS_WIN
    FreeFileSync::moveToWindowsRecycler(filename);  //throw (FileError)
#else
    throw std::logic_error("No Recycler for Linux available at the moment!");
#endif
}


bool FreeFileSync::fileExists(const DefaultChar* filename)
{
    //symbolic links (broken or not) are also treated as existing files!
#ifdef FFS_WIN
    // we must use GetFileAttributes() instead of the ANSI C functions because
    // it can cope with network (UNC) paths unlike them
    const DWORD ret = ::GetFileAttributes(applyLongPathPrefix(filename).c_str());

    return (ret != INVALID_FILE_ATTRIBUTES) && !(ret & FILE_ATTRIBUTE_DIRECTORY); //returns true for (file-)symlinks also

#elif defined FFS_LINUX
    struct stat fileInfo;
    return (::lstat(filename, &fileInfo) == 0 &&
            (S_ISLNK(fileInfo.st_mode) || S_ISREG(fileInfo.st_mode))); //in Linux a symbolic link is neither file nor directory
#endif
}


bool FreeFileSync::dirExists(const DefaultChar* dirname)
{
    //symbolic links (broken or not) are also treated as existing directories!
#ifdef FFS_WIN
    // we must use GetFileAttributes() instead of the ANSI C functions because
    // it can cope with network (UNC) paths unlike them
    const DWORD ret = ::GetFileAttributes(applyLongPathPrefix(dirname).c_str());

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
    const DWORD ret = ::GetFileAttributes(applyLongPathPrefix(objname).c_str());
    return (ret != INVALID_FILE_ATTRIBUTES) && (ret & FILE_ATTRIBUTE_REPARSE_POINT);

#elif defined FFS_LINUX
    struct stat fileInfo;
    return (::lstat(objname, &fileInfo) == 0 &&
            S_ISLNK(fileInfo.st_mode)); //symbolic link
#endif
}


bool FreeFileSync::isMovable(const Zstring& pathFrom, const Zstring& pathTo)
{
    wxLogNull noWxLogs; //prevent wxWidgets logging if dummy file creation failed

    const Zstring dummyFileSource = pathFrom.EndsWith(globalFunctions::FILE_NAME_SEPARATOR) ?
                                    pathFrom + DefaultStr("DeleteMe.tmp") :
                                    pathFrom + globalFunctions::FILE_NAME_SEPARATOR + DefaultStr("DeleteMe.tmp");

    const Zstring dummyFileTarget = pathTo.EndsWith(globalFunctions::FILE_NAME_SEPARATOR) ?
                                    pathTo + DefaultStr("DeleteMe.tmp") :
                                    pathTo + globalFunctions::FILE_NAME_SEPARATOR + DefaultStr("DeleteMe.tmp");
    try
    {
        removeFile(dummyFileSource, false);
        removeFile(dummyFileTarget, false);
    }
    catch  (...) {}

    //create dummy file
    {
        wxFile dummy(zToWx(dummyFileSource), wxFile::write);
        if (!dummy.IsOpened())
            return false; //if there's no write access, files can't be moved neither
        dummy.Write(wxT("FreeFileSync dummy file. May be deleted safely.\n"));
    }

    const bool result =
        //try to move the file
#ifdef FFS_WIN
        ::MoveFileEx(applyLongPathPrefix(dummyFileSource).c_str(), //__in      LPCTSTR lpExistingFileName,
                     applyLongPathPrefix(dummyFileTarget).c_str(), //__in_opt  LPCTSTR lpNewFileName,
                     0) != 0;                 //__in      DWORD dwFlags
#elif defined FFS_LINUX
        ::rename(dummyFileSource.c_str(), dummyFileTarget.c_str()) == 0;
#endif

    try
    {
        removeFile(dummyFileSource, false);
        removeFile(dummyFileTarget, false);
    }
    catch  (...) {}

    return result;
}


void FreeFileSync::removeFile(const Zstring& filename, const bool useRecycleBin) //throw (FileError, std::logic_error);
{
    //no error situation if file is not existing! manual deletion relies on it!
#ifdef FFS_WIN

    const Zstring filenameFmt = applyLongPathPrefix(filename);
    if (::GetFileAttributes(filenameFmt.c_str()) == INVALID_FILE_ATTRIBUTES)
        return; //neither file nor any other object with that name existing

#elif defined FFS_LINUX
    struct stat fileInfo;
    if (::lstat(filename.c_str(), &fileInfo) != 0)
        return; //neither file nor any other object (e.g. broken symlink) with that name existing
#endif

    if (useRecycleBin)
    {
        ::moveToRecycleBin(filename);
        return;
    }

#ifdef FFS_WIN
    //initialize file attributes
    if (!::SetFileAttributes(
                filenameFmt.c_str(),    //address of filename
                FILE_ATTRIBUTE_NORMAL)) //attributes to set
    {
        wxString errorMessage = wxString(_("Error deleting file:")) + wxT("\n\"") + zToWx(filename) + wxT("\"");
        throw FileError(errorMessage + wxT("\n\n") + FreeFileSync::getLastErrorFormatted());
    }

    //remove file, support for \\?\-prefix
    if (!::DeleteFile(filenameFmt.c_str()))
    {
        wxString errorMessage = wxString(_("Error deleting file:")) + wxT("\n\"") + zToWx(filename) + wxT("\"");
        throw FileError(errorMessage + wxT("\n\n") + FreeFileSync::getLastErrorFormatted());
    }
#elif defined FFS_LINUX
    if (::unlink(filename.c_str()) != 0)
    {
        wxString errorMessage = wxString(_("Error deleting file:")) + wxT("\n\"") + zToWx(filename) + wxT("\"");
        throw FileError(errorMessage + wxT("\n\n") + FreeFileSync::getLastErrorFormatted());
    }
#endif
}


//rename file: no copying!!!
void FreeFileSync::renameFile(const Zstring& oldName, const Zstring& newName) //throw (FileError);
{
#ifdef FFS_WIN
    if (!::MoveFileEx(applyLongPathPrefix(oldName).c_str(), //__in      LPCTSTR lpExistingFileName,
                      applyLongPathPrefix(newName).c_str(), //__in_opt  LPCTSTR lpNewFileName,
                      0))              //__in      DWORD dwFlags
    {
        const wxString errorMessage = wxString(_("Error moving file:")) + wxT("\n\"") + zToWx(oldName) +  wxT("\" ->\n\"") + zToWx(newName) + wxT("\"");
        throw FileError(errorMessage + wxT("\n\n") + FreeFileSync::getLastErrorFormatted());
    }
#elif defined FFS_LINUX
    //rename temporary file
    if (::rename(oldName.c_str(), newName.c_str()) != 0)
    {
        const wxString errorMessage = wxString(_("Error moving file:")) + wxT("\n\"") + zToWx(oldName) +  wxT("\" ->\n\"") + zToWx(newName) + wxT("\"");
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
    if (fileExists(targetFile.c_str())) //test file existence: e.g. Linux might silently overwrite existing symlinks
    {
        const wxString errorMessage = wxString(_("Error moving file:")) + wxT("\n\"") + zToWx(sourceFile) +  wxT("\" ->\n\"") +
                                      zToWx(targetFile) + wxT("\"");
        throw FileError(errorMessage + wxT("\n\n") + _("Target file already existing!"));
    }

    //moving of symbolic links should work correctly:

#ifdef FFS_WIN
    //first try to move the file directly without copying
    if (::MoveFileEx(applyLongPathPrefix(sourceFile).c_str(), //__in      LPCTSTR lpExistingFileName,
                     applyLongPathPrefix(targetFile).c_str(), //__in_opt  LPCTSTR lpNewFileName,
                     0))                 //__in      DWORD dwFlags
        return;

    //if moving failed treat as error (except when it tried to move to a different volume: in this case we will copy the file)
    const DWORD lastError = ::GetLastError();
    if (lastError != ERROR_NOT_SAME_DEVICE)
    {
        const wxString errorMessage = wxString(_("Error moving file:")) + wxT("\n\"") + zToWx(sourceFile) +  wxT("\" ->\n\"") +
                                      zToWx(targetFile) + wxT("\"");
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
    if (::rename(sourceFile.c_str(), targetFile.c_str()) == 0)
        return;

    //if moving failed treat as error (except when it tried to move to a different volume: in this case we will copy the file)
    if (errno != EXDEV)
    {
        const wxString errorMessage = wxString(_("Error moving file:")) + wxT("\n\"") + zToWx(sourceFile) +  wxT("\" ->\n\"") +
                                      zToWx(targetFile) + wxT("\"");
        throw FileError(errorMessage + wxT("\n\n") + FreeFileSync::getLastErrorFormatted());
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
    using namespace FreeFileSync;

    //handle symbolic links
    if (symlinkExists(sourceDir))
    {
        createDirectory(targetDir, sourceDir, true); //copy symbolic link
        removeDirectory(sourceDir, false);           //if target is already another symlink or directory, sourceDir-symlink is silently deleted
        return;
    }

    if (dirExists(targetDir))
    {
        if (!ignoreExistingDirs) //directory or symlink exists
        {
            const wxString errorMessage = wxString(_("Error moving directory:")) + wxT("\n\"") +
                                          zToWx(sourceDir) +  wxT("\" ->\n\"") + zToWx(targetDir) + wxT("\"");
            throw FileError(errorMessage + wxT("\n\n") + _("Target directory already existing!"));
        }
    }
    else
    {
        //first try to move the directory directly without copying
#ifdef FFS_WIN
        if (::MoveFileEx(applyLongPathPrefix(sourceDir).c_str(), //__in      LPCTSTR lpExistingFileName,
                         applyLongPathPrefix(targetDir).c_str(), //__in_opt  LPCTSTR lpNewFileName,
                         0))                //__in      DWORD dwFlags
            return;

        //if moving failed treat as error (except when it tried to move to a different volume: in this case we will copy the directory)
        const DWORD lastError = ::GetLastError();
        if (lastError != ERROR_NOT_SAME_DEVICE)
        {
            const wxString errorMessage = wxString(_("Error moving directory:")) + wxT("\n\"") + zToWx(sourceDir) +  wxT("\" ->\n\"") +
                                          zToWx(targetDir) + wxT("\"");
            throw FileError(errorMessage + wxT("\n\n") + FreeFileSync::getLastErrorFormatted(lastError));
        }
#elif defined FFS_LINUX
        if (::rename(sourceDir.c_str(), targetDir.c_str()) == 0)
            return;

        //if moving failed treat as error (except when it tried to move to a different volume: in this case we will copy the directory)
        if (errno != EXDEV)
        {
            const wxString errorMessage = wxString(_("Error moving directory:")) + wxT("\n\"") +
                                          zToWx(sourceDir) +  wxT("\" ->\n\"") + zToWx(targetDir) + wxT("\"");
            throw FileError(errorMessage + wxT("\n\n") + FreeFileSync::getLastErrorFormatted());
        }
#endif

        //create target
        createDirectory(targetDir, sourceDir, false); //throw (FileError);
    }

    //call back once per folder
    if (callback)
        switch (callback->requestUiRefresh())
        {
        case MoveFileCallback::CONTINUE:
            break;
        case MoveFileCallback::CANCEL:
            //an user aborted operation IS an error condition!
            throw FileError(wxString(_("Error moving directory:")) + wxT("\n\"") + zToWx(sourceDir) +  wxT("\" ->\n\"") +
                            zToWx(targetDir) + wxT("\"") + wxT("\n\n") + _("Operation aborted!"));
        }

    //move files/folders recursively
    TraverseOneLevel::NamePair fileList; //list of names: 1. short 2.long
    TraverseOneLevel::NamePair dirList;  //

    //traverse source directory one level
    TraverseOneLevel traverseCallback(fileList, dirList);
    traverseFolder(sourceDir, false, &traverseCallback); //traverse one level

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
    removeDirectory(sourceDir, false); //throw (FileError);
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
    const Zstring directoryFmt = applyLongPathPrefix(directory); //support for \\?\-prefix

    const DWORD dirAttr = ::GetFileAttributes(directoryFmt.c_str()); //name of a file or directory
    if (dirAttr == INVALID_FILE_ATTRIBUTES)
        return; //neither directory nor any other object with that name existing

#elif defined FFS_LINUX
    struct stat dirInfo;
    if (lstat(directory.c_str(), &dirInfo) != 0)
        return; //neither directory nor any other object (e.g. broken symlink) with that name existing
#endif

    if (useRecycleBin)
    {
        ::moveToRecycleBin(directory);
        return;
    }


#ifdef FFS_WIN
    //initialize file attributes
    if (!::SetFileAttributes(           // initialize file attributes: actually NEEDED for symbolic links also!
                directoryFmt.c_str(),      // address of directory name
                FILE_ATTRIBUTE_NORMAL)) // attributes to set
    {
        wxString errorMessage = wxString(_("Error deleting directory:")) + wxT("\n\"") + directory.c_str() + wxT("\"");
        throw FileError(errorMessage + wxT("\n\n") + FreeFileSync::getLastErrorFormatted());
    }


//attention: check if directory is a symlink! Do NOT traverse into it deleting contained files!!!
    if (dirAttr & FILE_ATTRIBUTE_REPARSE_POINT) //remove symlink directly
    {
        if (!::RemoveDirectory(directoryFmt.c_str()))
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
            wxString errorMessage = wxString(_("Error deleting directory:")) + wxT("\n\"") + zToWx(directory) + wxT("\"");
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
    std::for_each(fileList.begin(), fileList.end(), boost::bind(removeFile, _1, false));

    //delete directories recursively
    std::for_each(dirList.begin(), dirList.end(), boost::bind(removeDirectory, _1, false)); //call recursively to correctly handle symbolic links

    //parent directory is deleted last
#ifdef FFS_WIN
    //remove directory, support for \\?\-prefix
    if (!::RemoveDirectory(directoryFmt.c_str()))
#else
    if (::rmdir(directory.c_str()) != 0)
#endif
    {
        wxString errorMessage = wxString(_("Error deleting directory:")) + wxT("\n\"") + zToWx(directory) + wxT("\"");
        throw FileError(errorMessage + wxT("\n\n") + FreeFileSync::getLastErrorFormatted());
    }
}


#ifdef FFS_WIN
class CloseHandleOnExit
{
public:
    CloseHandleOnExit(HANDLE fileHandle) : fileHandle_(fileHandle) {}

    ~CloseHandleOnExit()
    {
        ::CloseHandle(fileHandle_);
    }

private:
    HANDLE fileHandle_;
};
#endif


//optionally: copy directory last change date, DO NOTHING if something fails
void FreeFileSync::copyFileTimes(const Zstring& sourceDir, const Zstring& targetDir)
{
    if (symlinkExists(sourceDir)) //don't handle symlinks (yet)
        return;

#ifdef FFS_WIN
    HANDLE hDirRead = ::CreateFile(applyLongPathPrefix(sourceDir).c_str(),
                                   0,
                                   FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                                   NULL,
                                   OPEN_EXISTING,
                                   FILE_FLAG_BACKUP_SEMANTICS, //needed for directories
                                   NULL);
    if (hDirRead == INVALID_HANDLE_VALUE)
        return;
    CloseHandleOnExit dummy(hDirRead);

    FILETIME creationTime;
    FILETIME accessTime;
    FILETIME lastWriteTime;
    if (::GetFileTime(hDirRead,
                      &creationTime,
                      &accessTime,
                      &lastWriteTime))
    {
        HANDLE hDirWrite = ::CreateFile(applyLongPathPrefix(targetDir).c_str(),
                                        FILE_WRITE_ATTRIBUTES,
                                        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                                        NULL,
                                        OPEN_EXISTING,
                                        FILE_FLAG_BACKUP_SEMANTICS, //needed for directories
                                        NULL);
        if (hDirWrite == INVALID_HANDLE_VALUE)
            return;
        CloseHandleOnExit dummy2(hDirWrite);

        //(try to) set new "last write time"
        ::SetFileTime(hDirWrite,
                      &creationTime,
                      &accessTime,
                      &lastWriteTime); //return value not evalutated!
    }
#elif defined FFS_LINUX

    struct stat dirInfo;
    if (::stat(sourceDir.c_str(), &dirInfo) == 0) //read file attributes from source directory
    {
        //adapt file modification time:
        struct utimbuf newTimes;
        //::time(&newTimes.actime); //set file access time to current time
        newTimes.actime  = dirInfo.st_atime;
        newTimes.modtime = dirInfo.st_mtime;

        //(try to) set new "last write time"
        ::utime(targetDir.c_str(), &newTimes); //return value not evalutated!
    }
#endif
}


#ifdef FFS_WIN
Zstring resolveDirectorySymlink(const Zstring& dirLinkName) //get full target path of symbolic link to a directory
{
    //open handle to target of symbolic link
    const HANDLE hDir = ::CreateFile(FreeFileSync::applyLongPathPrefix(dirLinkName).c_str(),
                                     0,
                                     FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                                     NULL,
                                     OPEN_EXISTING,
                                     FILE_FLAG_BACKUP_SEMANTICS,
                                     NULL);
    if (hDir == INVALID_HANDLE_VALUE)
        return Zstring();

    CloseHandleOnExit dummy(hDir);

    const unsigned int BUFFER_SIZE = 10000;
    TCHAR targetPath[BUFFER_SIZE];


    //dynamically load windows API function
    typedef DWORD (WINAPI *GetFinalPathNameByHandleWFunc)(
        HANDLE hFile,
        LPTSTR lpszFilePath,
        DWORD cchFilePath,
        DWORD dwFlags);
    static const GetFinalPathNameByHandleWFunc getFinalPathNameByHandle =
        Utility::loadDllFunction<GetFinalPathNameByHandleWFunc>(L"kernel32.dll", "GetFinalPathNameByHandleW");

    if (getFinalPathNameByHandle == NULL)
        throw FileError(wxString(_("Error loading library function:")) + wxT("\n\"") + wxT("GetFinalPathNameByHandleW") + wxT("\""));

    const DWORD rv = (*getFinalPathNameByHandle)(
                         hDir,
                         targetPath,
                         BUFFER_SIZE,
                         0);

    if (rv >= BUFFER_SIZE || rv == 0)
        return Zstring();

    return targetPath;
}

//#warning teste
//#include <aclapi.h>
////optionally: copy additional metadata, DO NOTHING if something fails
//void copyAdditionalMetadata(const Zstring& sourceDir, const Zstring& targetDir)
//{
//    //copy NTFS permissions
//
//    PSECURITY_DESCRIPTOR pSD;
//
//    PACL pDacl;
//    if (::GetNamedSecurityInfo(
//                const_cast<DefaultChar*>(sourceDir.c_str()),
//                SE_FILE_OBJECT, //file or directory
//                DACL_SECURITY_INFORMATION,
//                NULL,
//                NULL,
//                &pDacl,
//                NULL,
//                &pSD
//            ) == ERROR_SUCCESS)
//    {
//        //(try to) set new security information
//        if (::SetNamedSecurityInfo(
//                    const_cast<DefaultChar*>(targetDir.c_str()),
//                    SE_FILE_OBJECT, //file or directory
//                    DACL_SECURITY_INFORMATION,
//                    NULL,
//                    NULL,
//                    pDacl,
//                    NULL) != ERROR_SUCCESS) //return value not evalutated!
//        {
//            const wxString errorMessage = wxString(wxT("Error 2:")) + wxT("\n\"") + targetDir.c_str() + wxT("\"");
//            throw FileError(errorMessage + wxT("\n\n") + FreeFileSync::getLastErrorFormatted());
//        }
//
//#warning BUG!
//        LocalFree(pSD);
//    }
//    else
//    {
//        const wxString errorMessage = wxString(wxT("Error 1:")) + wxT("\n\"") + sourceDir.c_str() + wxT("\"");
//        throw FileError(errorMessage + wxT("\n\n") + FreeFileSync::getLastErrorFormatted());
//    }
//
//}
#endif


void createDirectoryRecursively(const Zstring& directory, const Zstring& templateDir, const bool copyDirectorySymLinks, const int level)
{
    using namespace FreeFileSync;

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
    const DWORD templateAttr = ::GetFileAttributes(applyLongPathPrefix(templateDir).c_str()); //replaces wxDirExists(): also returns successful for broken symlinks
    if (templateAttr == INVALID_FILE_ATTRIBUTES) //fallback
    {
        if (!::CreateDirectory(applyLongPathPrefixCreateDir(directory).c_str(), // pointer to a directory path string
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
                            applyLongPathPrefix(linkPath).c_str(),           // pointer to path string of template directory
                            applyLongPathPrefixCreateDir(directory).c_str(), // pointer to a directory path string
                            NULL) && level == 0)
                {
                    const wxString errorMessage = wxString(_("Error creating directory:")) + wxT("\n\"") + directory.c_str() + wxT("\"");
                    throw FileError(errorMessage + wxT("\n\n") + FreeFileSync::getLastErrorFormatted());
                }

                //(try to) copy additional metadata like last modification time: no measurable performance drawback
                //copyAdditionalMetadata(linkPath, directory);
            }
        }
        else //in all other cases
        {
            if (!::CreateDirectoryEx( // this function automatically copies symbolic links if encountered
                        applyLongPathPrefix(templateDir).c_str(),          // pointer to path string of template directory
                        applyLongPathPrefixCreateDir(directory).c_str(),   // pointer to a directory path string
                        NULL) && level == 0)
            {
                const wxString errorMessage = templateAttr & FILE_ATTRIBUTE_REPARSE_POINT ?
                                              //give a more meaningful errormessage if copying a symbolic link failed, e.g. "C:\Users\ZenJu\Application Data"
                                              (wxString(_("Error copying symbolic link:")) + wxT("\n\"") + templateDir.c_str() +  wxT("\" ->\n\"") + directory.c_str() + wxT("\"")) :

                                              (wxString(_("Error creating directory:")) + wxT("\n\"") + directory.c_str() + wxT("\""));
                throw FileError(errorMessage + wxT("\n\n") + FreeFileSync::getLastErrorFormatted());
            }

            //(try to) copy additional metadata like last modification time: no measurable performance drawback
            //copyAdditionalMetadata(templateDir, directory);
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
                wxString errorMessage = wxString(_("Error resolving symbolic link:")) + wxT("\n\"") + zToWx(templateDir) + wxT("\"");
                if (bytesWritten < 0) errorMessage += wxString(wxT("\n\n")) + FreeFileSync::getLastErrorFormatted();
                throw FileError(errorMessage);
            }
            //set null-terminating char
            buffer[bytesWritten] = 0;

            if (symlink(buffer, directory.c_str()) != 0)
            {
                const wxString errorMessage = wxString(_("Error copying symbolic link:")) + wxT("\n\"") + zToWx(templateDir) +  wxT("\" ->\n\"") + zToWx(directory) + wxT("\"");
                throw FileError(errorMessage + wxT("\n\n") + FreeFileSync::getLastErrorFormatted());
            }
            return; //symlink created successfully
        }
    }

    //default directory creation
    if (::mkdir(directory.c_str(), 0755) != 0 && level == 0)
    {
        wxString errorMessage = wxString(_("Error creating directory:")) + wxT("\n\"") + zToWx(directory) + wxT("\"");
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


Zstring createTempName(const Zstring& filename)
{
    Zstring output = filename + DefaultStr(".ffs_tmp");

    //ensure uniqueness
    if (FreeFileSync::fileExists(output))
    {
        //if it's not unique, add a postfix number
        int postfix = 1;
        while (FreeFileSync::fileExists(output + DefaultStr("_") + numberToZstring(postfix)))
            ++postfix;

        output += Zstring(DefaultStr("_")) + numberToZstring(postfix);
    }

    return output;
}


#ifdef FFS_WIN

#ifndef COPY_FILE_COPY_SYMLINK
#define COPY_FILE_COPY_SYMLINK                0x00000800
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
    //overview: http://msdn.microsoft.com/en-us/library/ms724834(VS.85).aspx
    return false;
}


#ifndef COPY_FILE_ALLOW_DECRYPTED_DESTINATION
#define COPY_FILE_ALLOW_DECRYPTED_DESTINATION 0x00000008
#endif


bool supportForNonEncryptedDestination()
{
    OSVERSIONINFO osvi;
    ZeroMemory(&osvi, sizeof(OSVERSIONINFO));
    osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);

    //encrypted destination is not supported with Windows 2000
    if (GetVersionEx(&osvi))
        return osvi.dwMajorVersion > 5 ||
               (osvi.dwMajorVersion == 5 && osvi.dwMinorVersion > 0); //2000 has majorVersion == 5, minorVersion == 0
    //overview: http://msdn.microsoft.com/en-us/library/ms724834(VS.85).aspx
    return false;
}


void FreeFileSync::copyFile(const Zstring& sourceFile,
                            const Zstring& targetFile,
                            const bool copyFileSymLinks,
                            FreeFileSync::ShadowCopy* shadowCopyHandler,
                            FreeFileSync::CopyFileCallback* callback)
{
    //FreeFileSync::fileExists(targetFile.c_str()) -> avoid this call, performance;
    //if target exists (very unlikely, because sync-algorithm deletes it) renaming below will fail!


    DWORD copyFlags = COPY_FILE_FAIL_IF_EXISTS;

    //copy symbolic links instead of the files pointed at
    static const bool symlinksSupported = supportForSymbolicLinks(); //only set "true" if supported by OS: else copying in Windows XP fails
    if (copyFileSymLinks && symlinksSupported)
        copyFlags |= COPY_FILE_COPY_SYMLINK;

    //allow copying from encrypted to non-encrytped location
    static const bool nonEncSupported = supportForNonEncryptedDestination();
    if (nonEncSupported)
        copyFlags |= COPY_FILE_ALLOW_DECRYPTED_DESTINATION;

    const Zstring temporary = createTempName(targetFile); //use temporary file until a correct date has been set

    if (!::CopyFileEx( //same performance as CopyFile()
                applyLongPathPrefix(sourceFile).c_str(),
                applyLongPathPrefix(temporary).c_str(),
                copyCallbackInternal,
                callback,
                NULL,
                copyFlags))
    {
        const DWORD lastError = ::GetLastError();

        //don't suppress "lastError == ERROR_REQUEST_ABORTED": an user aborted operation IS an error condition!

        //if file is locked (try to) use Windows Volume Shadow Copy Service
        if (shadowCopyHandler != NULL &&
                (lastError == ERROR_SHARING_VIOLATION ||
                 lastError == ERROR_LOCK_VIOLATION))
        {
            //shadowFilename already contains prefix: E.g. "\\?\GLOBALROOT\Device\HarddiskVolumeShadowCopy1\Program Files\FFS\sample.dat"
            const Zstring shadowFilename(shadowCopyHandler->makeShadowCopy(sourceFile));
            copyFile(shadowFilename, //transferred bytes is automatically reset when new file is copied
                     targetFile,
                     copyFileSymLinks,
                     NULL,
                     callback);
            return;
        }

        //assemble error message...
        const wxString errorMessage = wxString(_("Error copying file:")) + wxT("\n\"") + sourceFile.c_str() +  wxT("\" ->\n\"") + targetFile.c_str() + wxT("\"") +
                                      wxT("\n\n") + FreeFileSync::getLastErrorFormatted(lastError);

        throw FileError(errorMessage);
    }

    try
    {
        //rename temporary file
        FreeFileSync::renameFile(temporary, targetFile);
    }
    catch (...) //if renaming temporary failed: cleanup
    {
        try
        {
            removeFile(temporary, false); //throw (FileError, std::logic_error);
        }
        catch(...) {}

        //this can only happen in very obscure situations: while scanning, target didn't exist, but while sync'ing it suddenly does (e.g. network drop?)
        if (FreeFileSync::fileExists(targetFile.c_str()))
            throw FileError(wxString(_("Error copying file:")) + wxT("\n\"") + zToWx(sourceFile) +  wxT("\" ->\n\"") + zToWx(targetFile) + wxT("\"\n\n")
                            + _("Target file already existing!"));

        throw;
    }

    //copy creation date (last modification date is redundantly written, too)
    copyFileTimes(sourceFile, targetFile); //throw()
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
        throw FileError(wxString(_("Error copying file:")) + wxT("\n\"") + zToWx(sourceFile) +  wxT("\" ->\n\"") + zToWx(targetFile) + wxT("\"\n\n")
                        + _("Target file already existing!"));

    //symbolic link handling
    if (copyFileSymLinks)
    {
        //test if sourceFile is a symbolic link
        struct stat linkInfo;
        if (lstat(sourceFile.c_str(), &linkInfo) != 0)
        {
            const wxString errorMessage = wxString(_("Error reading file attributes:")) + wxT("\n\"") + zToWx(sourceFile) + wxT("\"");
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
                wxString errorMessage = wxString(_("Error resolving symbolic link:")) + wxT("\n\"") + zToWx(sourceFile) + wxT("\"");
                if (bytesWritten < 0) errorMessage += wxString(wxT("\n\n")) + FreeFileSync::getLastErrorFormatted();
                throw FileError(errorMessage);
            }
            //set null-terminating char
            buffer[bytesWritten] = 0;

            if (symlink(buffer, targetFile.c_str()) != 0)
            {
                const wxString errorMessage = wxString(_("Error copying symbolic link:")) + wxT("\n\"") + zToWx(sourceFile) +  wxT("\" ->\n\"") + zToWx(targetFile) + wxT("\"");
                throw FileError(errorMessage + wxT("\n\n") + FreeFileSync::getLastErrorFormatted());
            }

            return; //symlink created successfully
        }
    }

    //begin of regular file copy
    struct stat fileInfo;
    if (stat(sourceFile.c_str(), &fileInfo) != 0) //read file attributes from source file (resolving symlinks; but cannot be one in this context)
    {
        const wxString errorMessage = wxString(_("Error reading file attributes:")) + wxT("\n\"") + zToWx(sourceFile) + wxT("\"");
        throw FileError(errorMessage + wxT("\n\n") + FreeFileSync::getLastErrorFormatted());
    }

    //open sourceFile for reading
    std::ifstream fileIn(sourceFile.c_str(), std::ios_base::binary);
    if (fileIn.fail())
        throw FileError(wxString(_("Error opening file:")) + wxT("\n\"") + zToWx(sourceFile) +  wxT("\""));

    //create targetFile and open it for writing
    const Zstring temporary = createTempName(targetFile); //use temporary file until a correct date has been set

    try
    {
        std::ofstream fileOut(temporary.c_str(), std::ios_base::binary);
        if (fileOut.fail())
            throw FileError(wxString(_("Error opening file:")) + wxT("\n\"") + zToWx(targetFile) + wxT("\""));

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
                    throw FileError(wxString(_("Error writing file:")) + wxT("\n\"") + zToWx(targetFile) + wxT("\""));
                break;
            }
            else if (fileIn.fail())
                throw FileError(wxString(_("Error reading file:")) + wxT("\n\"") + zToWx(sourceFile) + wxT("\""));


            fileOut.write(memory.buffer, memory.bufferSize);
            if (fileOut.bad())
                throw FileError(wxString(_("Error writing file:")) + wxT("\n\"") + zToWx(targetFile) + wxT("\""));

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
                    throw FileError(wxString(_("Error copying file:")) + wxT("\n\"") + zToWx(sourceFile) +  wxT("\" ->\n\"") +
                                    zToWx(targetFile) + wxT("\"\n\n") + _("Operation aborted!"));
                }
            }
        }

        //close streams before changing attributes
        fileIn.close();
        fileOut.close();

        //adapt file modification time:
        struct utimbuf newTimes;
        ::time(&newTimes.actime); //set file access time to current time
        newTimes.modtime = fileInfo.st_mtime;
        if (utime(temporary.c_str(), &newTimes) != 0)
        {
            wxString errorMessage = wxString(_("Error changing modification time:")) + wxT("\n\"") + zToWx(targetFile) + wxT("\"");
            throw FileError(errorMessage + wxT("\n\n") + FreeFileSync::getLastErrorFormatted());
        }

        //rename temporary file
        FreeFileSync::renameFile(temporary, targetFile);

        //set file access rights
        if (::chmod(targetFile.c_str(), fileInfo.st_mode) != 0)
        {
            const wxString errorMessage = wxString(_("Error writing file attributes:")) + wxT("\n\"") + zToWx(targetFile) + wxT("\"");
            throw FileError(errorMessage + wxT("\n\n") + FreeFileSync::getLastErrorFormatted());
        }
    }
    catch (...)
    {
        //try to delete target file if error occured, or exception was thrown in callback function
        //no data-loss, because of "fileExists(targetFile))" check at the beginning!
        if (FreeFileSync::fileExists(targetFile))
            ::unlink(targetFile); //don't handle error situations!

        //clean-up temporary
        if (FreeFileSync::fileExists(temporary))
            ::unlink(temporary); //don't handle error situations!

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

