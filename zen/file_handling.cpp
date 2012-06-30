// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) ZenJu (zhnmju123 AT gmx DOT de) - All Rights Reserved    *
// **************************************************************************

#include "file_handling.h"
#include <map>
#include <algorithm>
#include <stdexcept>
#include "file_traverser.h"
#include "scope_guard.h"
#include "symlink_target.h"
#include "file_io.h"
#include "assert_static.h"
#include <boost/thread/tss.hpp>
#include "file_id_def.h"

#ifdef FFS_WIN
#include "privilege.h"
#include "dll.h"
#include "win.h" //includes "windows.h"
#include "long_path_prefix.h"
#include <Aclapi.h>
#include "dst_hack.h"
#include "win_ver.h"
#include "IFileOperation/file_op.h"

#elif defined FFS_LINUX
#include <sys/stat.h>
#include <time.h>
#include <utime.h>
#include <sys/time.h>
#include <sys/vfs.h>

#ifdef HAVE_SELINUX
#include <selinux/selinux.h>
#endif
#endif

using namespace zen;


bool zen::fileExists(const Zstring& filename)
{
    //symbolic links (broken or not) are also treated as existing files!
#ifdef FFS_WIN
    const DWORD ret = ::GetFileAttributes(applyLongPathPrefix(filename).c_str());
    return ret != INVALID_FILE_ATTRIBUTES && (ret & FILE_ATTRIBUTE_DIRECTORY) == 0; //returns true for (file-)symlinks also

#elif defined FFS_LINUX
    struct stat fileInfo = {};
    return ::lstat(filename.c_str(), &fileInfo) == 0 &&
           (S_ISLNK(fileInfo.st_mode) || S_ISREG(fileInfo.st_mode)); //in Linux a symbolic link is neither file nor directory
#endif
}


bool zen::dirExists(const Zstring& dirname)
{
    //symbolic links (broken or not) are also treated as existing directories!
#ifdef FFS_WIN
    const DWORD ret = ::GetFileAttributes(applyLongPathPrefix(dirname).c_str());
    return ret != INVALID_FILE_ATTRIBUTES && (ret & FILE_ATTRIBUTE_DIRECTORY) != 0; //returns true for (dir-)symlinks also

#elif defined FFS_LINUX
    struct stat dirInfo = {};
    return ::lstat(dirname.c_str(), &dirInfo) == 0 &&
           (S_ISLNK(dirInfo.st_mode) || S_ISDIR(dirInfo.st_mode)); //in Linux a symbolic link is neither file nor directory
#endif
}


bool zen::symlinkExists(const Zstring& linkname)
{
#ifdef FFS_WIN
    const DWORD ret = ::GetFileAttributes(applyLongPathPrefix(linkname).c_str());
    return ret != INVALID_FILE_ATTRIBUTES && (ret & FILE_ATTRIBUTE_REPARSE_POINT) != 0;

#elif defined FFS_LINUX
    struct stat fileInfo = {};
    return ::lstat(linkname.c_str(), &fileInfo) == 0 &&
           S_ISLNK(fileInfo.st_mode); //symbolic link
#endif
}


bool zen::somethingExists(const Zstring& objname) //throw()       check whether any object with this name exists
{
#ifdef FFS_WIN
    const DWORD rv = ::GetFileAttributes(applyLongPathPrefix(objname).c_str());
    return rv != INVALID_FILE_ATTRIBUTES || ::GetLastError() == ERROR_SHARING_VIOLATION; //"C:\pagefile.sys"

#elif defined FFS_LINUX
    struct stat fileInfo = {};
    return ::lstat(objname.c_str(), &fileInfo) == 0;
#endif
}


namespace
{
void getFileAttrib(const Zstring& filename, FileAttrib& attr, ProcSymlink procSl) //throw FileError
{
#ifdef FFS_WIN
    WIN32_FIND_DATA fileInfo = {};
    {
        const HANDLE searchHandle = ::FindFirstFile(applyLongPathPrefix(filename).c_str(), &fileInfo);
        if (searchHandle == INVALID_HANDLE_VALUE)
            throw FileError(replaceCpy(_("Cannot read file attributes of %x."), L"%x", fmtFileName(filename)) + L"\n\n" + getLastErrorFormatted());
        ::FindClose(searchHandle);
    }
    //        WIN32_FILE_ATTRIBUTE_DATA sourceAttr = {};
    //        if (!::GetFileAttributesEx(applyLongPathPrefix(sourceObj).c_str(), //__in   LPCTSTR lpFileName,
    //                                   GetFileExInfoStandard,                  //__in   GET_FILEEX_INFO_LEVELS fInfoLevelId,
    //                                   &sourceAttr))                           //__out  LPVOID lpFileInformation

    const bool isSymbolicLink = (fileInfo.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT) != 0;
    if (!isSymbolicLink || procSl == SYMLINK_DIRECT)
    {
        //####################################### DST hack ###########################################
        const bool isDirectory = (fileInfo.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
        if (!isDirectory && dst::isFatDrive(filename)) //throw()
        {
            const dst::RawTime rawTime(fileInfo.ftCreationTime, fileInfo.ftLastWriteTime);
            if (dst::fatHasUtcEncoded(rawTime)) //throw std::runtime_error
            {
                fileInfo.ftLastWriteTime = dst::fatDecodeUtcTime(rawTime); //return last write time in real UTC, throw (std::runtime_error)
                ::GetSystemTimeAsFileTime(&fileInfo.ftCreationTime); //real creation time information is not available...
            }
        }
        //####################################### DST hack ###########################################

        attr.fileSize         = UInt64(fileInfo.nFileSizeLow, fileInfo.nFileSizeHigh);
        attr.modificationTime = toTimeT(fileInfo.ftLastWriteTime);
    }
    else
    {
        const HANDLE hFile = ::CreateFile(applyLongPathPrefix(filename).c_str(), //open handle to target of symbolic link
                                          0,
                                          FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                                          nullptr,
                                          OPEN_EXISTING,
                                          FILE_FLAG_BACKUP_SEMANTICS, //needed to open a directory
                                          nullptr);
        if (hFile == INVALID_HANDLE_VALUE)
            throw FileError(replaceCpy(_("Cannot read file attributes of %x."), L"%x", fmtFileName(filename)) + L"\n\n" + getLastErrorFormatted());

        ZEN_ON_SCOPE_EXIT(::CloseHandle(hFile));

        BY_HANDLE_FILE_INFORMATION fileInfoHnd = {};
        if (!::GetFileInformationByHandle(hFile, &fileInfoHnd))
            throw FileError(replaceCpy(_("Cannot read file attributes of %x."), L"%x", fmtFileName(filename)) + L"\n\n" + getLastErrorFormatted());

        attr.fileSize         = UInt64(fileInfoHnd.nFileSizeLow, fileInfoHnd.nFileSizeHigh);
        attr.modificationTime = toTimeT(fileInfoHnd.ftLastWriteTime);
    }

#elif defined FFS_LINUX
    struct stat fileInfo = {};

    const int rv = procSl == SYMLINK_FOLLOW ?
                   :: stat(filename.c_str(), &fileInfo) :
                   ::lstat(filename.c_str(), &fileInfo);
    if (rv != 0) //follow symbolic links
        throw FileError(replaceCpy(_("Cannot read file attributes of %x."), L"%x", fmtFileName(filename)) + L"\n\n" + getLastErrorFormatted());

    attr.fileSize         = UInt64(fileInfo.st_size);
    attr.modificationTime = fileInfo.st_mtime;
#endif
}
}


UInt64 zen::getFilesize(const Zstring& filename) //throw FileError
{
    FileAttrib attr;
    getFileAttrib(filename, attr, SYMLINK_FOLLOW); //throw FileError
    return attr.fileSize;
}


Int64 zen::getFileTime(const Zstring& filename, ProcSymlink procSl) //throw FileError
{
    FileAttrib attr;
    getFileAttrib(filename, attr, procSl); //throw FileError
    return attr.modificationTime;
}


UInt64 zen::getFreeDiskSpace(const Zstring& path) //throw FileError
{
#ifdef FFS_WIN
    ULARGE_INTEGER bytesFree = {};
    if (!::GetDiskFreeSpaceEx(appendSeparator(path).c_str(), //__in_opt   LPCTSTR lpDirectoryName, -> "UNC name [...] must include a trailing backslash, for example, "\\MyServer\MyShare\"
                              &bytesFree,                    //__out_opt  PULARGE_INTEGER lpFreeBytesAvailable,
                              nullptr,                       //__out_opt  PULARGE_INTEGER lpTotalNumberOfBytes,
                              nullptr))                      //__out_opt  PULARGE_INTEGER lpTotalNumberOfFreeBytes
        throw FileError(replaceCpy(_("Cannot read file attributes of %x."), L"%x", fmtFileName(path)) + L"\n\n" + getLastErrorFormatted());

    return UInt64(bytesFree.LowPart, bytesFree.HighPart);

#elif defined FFS_LINUX
    struct statfs info = {};
    if (::statfs(path.c_str(), &info) != 0)
        throw FileError(replaceCpy(_("Cannot read file attributes of %x."), L"%x", fmtFileName(path)) + L"\n\n" + getLastErrorFormatted());

    return UInt64(info.f_bsize) * info.f_bavail;
#endif
}


namespace
{
#ifdef FFS_WIN
//(try to) enhance error messages by showing which processed lock the file
Zstring getLockingProcessNames(const Zstring& filename) //throw(), empty string if none found or error occurred
{
    if (vistaOrLater())
    {
        using namespace fileop;
        const DllFun<FunType_getLockingProcesses> getLockingProcesses(getDllName(), funName_getLockingProcesses);
        const DllFun<FunType_freeString>          freeString         (getDllName(), funName_freeString);

        if (getLockingProcesses && freeString)
        {
            const wchar_t* procList = nullptr;
            if (getLockingProcesses(filename.c_str(), procList))
            {
                ZEN_ON_SCOPE_EXIT(freeString(procList));
                return procList;
            }
        }
    }
    return Zstring();
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

    Zstring volumePath = appendSeparator(&buffer[0]);

    DWORD volumeSerial = 0;
    if (!::GetVolumeInformation(volumePath.c_str(), //__in_opt   LPCTSTR lpRootPathName,
                                nullptr,            //__out      LPTSTR lpVolumeNameBuffer,
                                0,                  //__in       DWORD nVolumeNameSize,
                                &volumeSerial,      //__out_opt  LPDWORD lpVolumeSerialNumber,
                                nullptr,            //__out_opt  LPDWORD lpMaximumComponentLength,
                                nullptr,            //__out_opt  LPDWORD lpFileSystemFlags,
                                nullptr,            //__out      LPTSTR lpFileSystemNameBuffer,
                                0))                 //__in       DWORD nFileSystemNameSize
        return 0;

    return volumeSerial;
}
#elif defined FFS_LINUX

dev_t retrieveVolumeSerial(const Zstring& pathName) //return 0 on error!
{
    Zstring volumePathName = pathName;

    //remove trailing slash
    if (volumePathName.size() > 1 && endsWith(volumePathName, FILE_NAME_SEPARATOR))  //exception: allow '/'
        volumePathName = beforeLast(volumePathName, FILE_NAME_SEPARATOR);

    struct stat fileInfo = {};
    while (::lstat(volumePathName.c_str(), &fileInfo) != 0) //go up in folder hierarchy until existing folder is found
    {
        volumePathName = beforeLast(volumePathName, FILE_NAME_SEPARATOR); //returns empty string if ch not found
        if (volumePathName.empty())
            return 0;  //this includes path "/" also!
    }

    return fileInfo.st_dev;
}
#endif
}


zen::ResponseSame zen::onSameVolume(const Zstring& folderLeft, const Zstring& folderRight) //throw()
{
    const auto serialLeft  = retrieveVolumeSerial(folderLeft);  //returns 0 on error!
    const auto serialRight = retrieveVolumeSerial(folderRight); //returns 0 on error!
    if (serialLeft == 0 || serialRight == 0)
        return IS_SAME_CANT_SAY;

    return serialLeft == serialRight ? IS_SAME_YES : IS_SAME_NO;
}


bool zen::removeFile(const Zstring& filename) //throw FileError
{
#ifdef FFS_WIN
    const Zstring& filenameFmt = applyLongPathPrefix(filename);
    if (!::DeleteFile(filenameFmt.c_str()))
#elif defined FFS_LINUX
    if (::unlink(filename.c_str()) != 0)
#endif
    {
        ErrorCode lastError = getLastError();
        if (errorCodeForNotExisting(lastError)) //no error situation if file is not existing! manual deletion relies on it!
            return false;

        const std::wstring shortMsg = replaceCpy(_("Cannot delete file %x."), L"%x", fmtFileName(filename));

#ifdef FFS_WIN
        if (lastError == ERROR_ACCESS_DENIED) //function fails if file is read-only
        {
            ::SetFileAttributes(filenameFmt.c_str(), FILE_ATTRIBUTE_NORMAL); //(try to) normalize file attributes

            if (::DeleteFile(filenameFmt.c_str())) //now try again...
                return true;
            lastError = ::GetLastError();
        }

        if (lastError == ERROR_SHARING_VIOLATION || //-> enhance error message!
            lastError == ERROR_LOCK_VIOLATION)
        {
            const Zstring procList = getLockingProcessNames(filename); //throw()
            if (!procList.empty())
                throw FileError(shortMsg + L"\n\n" + _("The file is locked by another process:") + L"\n" + procList);
        }
#endif
        //after "lastError" evaluation it *may* be redundant to check existence again, but better be safe than sorry:
        if (!somethingExists(filename)) //warning: changes global error code!!
            return false; //neither file nor any other object (e.g. broken symlink) with that name existing

        throw FileError(shortMsg + L"\n\n" + getLastErrorFormatted(lastError));
    }
    return true;
}


namespace
{
DEFINE_NEW_FILE_ERROR(ErrorDifferentVolume);

/* Usage overview: (avoid circular pattern!)

  renameFile()  -->  renameFile_sub()
      |               /|\
     \|/               |
      Fix8Dot3NameClash()
*/
//wrapper for file system rename function:
void renameFile_sub(const Zstring& oldName, const Zstring& newName) //throw FileError, ErrorDifferentVolume, ErrorTargetExisting
{
#ifdef FFS_WIN
    const Zstring oldNameFmt = applyLongPathPrefix(oldName);
    const Zstring newNameFmt = applyLongPathPrefix(newName);

    if (!::MoveFileEx(oldNameFmt.c_str(), //__in      LPCTSTR lpExistingFileName,
                      newNameFmt.c_str(), //__in_opt  LPCTSTR lpNewFileName,
                      0))                 //__in      DWORD dwFlags
    {
        DWORD lastError = ::GetLastError();

        const std::wstring shortMsg = replaceCpy(replaceCpy(_("Cannot move file %x to %y."), L"%x", fmtFileName(oldName)), L"%y", fmtFileName(newName));

        if (lastError == ERROR_SHARING_VIOLATION || //-> enhance error message!
            lastError == ERROR_LOCK_VIOLATION)
        {
            const Zstring procList = getLockingProcessNames(oldName); //throw()
            if (!procList.empty())
                throw FileError(shortMsg + L"\n\n" + _("The file is locked by another process:") + L"\n" + procList);
        }

        if (lastError == ERROR_ACCESS_DENIED) //MoveFileEx may fail to rename a read-only file on a SAMBA-share -> (try to) handle this
        {
            const DWORD oldAttr = ::GetFileAttributes(oldNameFmt.c_str());
            if (oldAttr != INVALID_FILE_ATTRIBUTES && (oldAttr & FILE_ATTRIBUTE_READONLY))
            {
                if (::SetFileAttributes(oldNameFmt.c_str(), FILE_ATTRIBUTE_NORMAL)) //remove readonly-attribute
                {
                    //try again...
                    if (::MoveFileEx(oldNameFmt.c_str(), //__in      LPCTSTR lpExistingFileName,
                                     newNameFmt.c_str(), //__in_opt  LPCTSTR lpNewFileName,
                                     0))                 //__in      DWORD dwFlags
                    {
                        //(try to) restore file attributes
                        ::SetFileAttributes(newNameFmt.c_str(), oldAttr); //don't handle error
                        return;
                    }
                    else
                    {
                        lastError = ::GetLastError(); //use error code from second call to ::MoveFileEx()

                        //cleanup: (try to) restore file attributes: assume oldName is still existing
                        ::SetFileAttributes(oldNameFmt.c_str(), oldAttr);
                    }
                }
            }
        }

        std::wstring errorMessage = shortMsg + L"\n\n" + getLastErrorFormatted(lastError);

        if (lastError == ERROR_NOT_SAME_DEVICE)
            throw ErrorDifferentVolume(errorMessage);

        else if (lastError == ERROR_ALREADY_EXISTS || //-> used on Win7 x64
                 lastError == ERROR_FILE_EXISTS)      //-> used by XP???
            throw ErrorTargetExisting(errorMessage);
        else
            throw FileError(errorMessage);
    }

#elif defined FFS_LINUX
    if (::rename(oldName.c_str(), newName.c_str()) != 0)
    {
        const int lastError = errno;
        std::wstring errorMessage = replaceCpy(replaceCpy(_("Cannot move file %x to %y."), L"%x", fmtFileName(oldName)), L"%y", fmtFileName(newName)) +
                                    L"\n\n" + getLastErrorFormatted(lastError);

        if (lastError == EXDEV)
            throw ErrorDifferentVolume(errorMessage);
        else if (lastError == EEXIST)
            throw ErrorTargetExisting(errorMessage);
        else
            throw FileError(errorMessage);
    }
#endif
}


#ifdef FFS_WIN
/*small wrapper around
::GetShortPathName()
::GetLongPathName() */
template <typename Function>
Zstring getFilenameFmt(const Zstring& filename, Function fun) //throw(); returns empty string on error
{
    const Zstring filenameFmt = applyLongPathPrefix(filename);

    const DWORD bufferSize = fun(filenameFmt.c_str(), nullptr, 0);
    if (bufferSize == 0)
        return Zstring();

    std::vector<wchar_t> buffer(bufferSize);

    const DWORD rv = fun(filenameFmt.c_str(), //__in   LPCTSTR lpszShortPath,
                         &buffer[0],          //__out  LPTSTR  lpszLongPath,
                         bufferSize);         //__in   DWORD   cchBuffer
    if (rv == 0 || rv >= bufferSize)
        return Zstring();

    return &buffer[0];
}


Zstring findUnused8Dot3Name(const Zstring& filename) //find a unique 8.3 short name
{
    const Zstring pathPrefix = contains(filename, FILE_NAME_SEPARATOR) ?
                               (beforeLast(filename, FILE_NAME_SEPARATOR) + FILE_NAME_SEPARATOR) : Zstring();

    Zstring extension = afterLast(afterLast(filename, FILE_NAME_SEPARATOR), Zchar('.')); //extension needn't contain reasonable data
    if (extension.empty())
        extension = Zstr("FFS");
    else if (extension.length() > 3)
        extension.resize(3);

    for (int index = 0; index < 100000000; ++index) //filename must be representable by <= 8 characters
    {
        const Zstring output = pathPrefix + numberTo<Zstring>(index) + Zchar('.') + extension;
        if (!somethingExists(output)) //ensure uniqueness
            return output;
    }

    throw std::runtime_error(std::string("100000000 files, one for each number, exist in this directory? You're kidding...\n") + utfCvrtTo<std::string>(pathPrefix));
}


bool have8dot3NameClash(const Zstring& filename)
{
    if (!contains(filename, FILE_NAME_SEPARATOR))
        return false;

    if (somethingExists(filename)) //name OR directory!
    {
        const Zstring origName  = afterLast(filename, FILE_NAME_SEPARATOR); //returns the whole string if ch not found
        const Zstring shortName = afterLast(getFilenameFmt(filename, ::GetShortPathName), FILE_NAME_SEPARATOR); //throw() returns empty string on error
        const Zstring longName  = afterLast(getFilenameFmt(filename, ::GetLongPathName) , FILE_NAME_SEPARATOR); //

        if (!shortName.empty() &&
            !longName.empty()  &&
            EqualFilename()(origName,  shortName) &&
            !EqualFilename()(shortName, longName))
        {
            //for filename short and long file name are equal and another unrelated file happens to have the same short name
            //e.g. filename == "TESTWE~1", but another file is existing named "TestWeb" with short name ""TESTWE~1"
            return true;
        }
    }
    return false;
}

class Fix8Dot3NameClash
{
public:
    Fix8Dot3NameClash(const Zstring& filename)
    {
        const Zstring longName = afterLast(getFilenameFmt(filename, ::GetLongPathName), FILE_NAME_SEPARATOR); //throw() returns empty string on error

        unrelatedFile = beforeLast(filename, FILE_NAME_SEPARATOR) + FILE_NAME_SEPARATOR + longName;

        //find another name in short format: this ensures the actual short name WILL be renamed as well!
        unrelatedFileParked = findUnused8Dot3Name(filename);

        //move already existing short name out of the way for now
        renameFile_sub(unrelatedFile, unrelatedFileParked); //throw FileError, ErrorDifferentVolume
        //DON'T call renameFile() to avoid reentrance!
    }

    ~Fix8Dot3NameClash()
    {
        //the file system should assign this unrelated file a new (unique) short name
        try
        {
            renameFile_sub(unrelatedFileParked, unrelatedFile); //throw FileError, ErrorDifferentVolume
        }
        catch (...) {}
    }
private:
    Zstring unrelatedFile;
    Zstring unrelatedFileParked;
};
#endif
}


//rename file: no copying!!!
void zen::renameFile(const Zstring& oldName, const Zstring& newName) //throw FileError, ErrorDifferentVolume, ErrorTargetExisting
{
    try
    {
        renameFile_sub(oldName, newName); //throw FileError, ErrorDifferentVolume, ErrorTargetExisting
    }
    catch (const ErrorTargetExisting&)
    {
#ifdef FFS_WIN
        //try to handle issues with already existing short 8.3 file names on Windows
        if (have8dot3NameClash(newName))
        {
            Fix8Dot3NameClash dummy(newName); //move clashing filename to the side
            //now try again...
            renameFile_sub(oldName, newName); //throw FileError
            return;
        }
#endif
        throw;
    }
}


class CopyCallbackImpl : public zen::CallbackCopyFile //callback functionality
{
public:
    CopyCallbackImpl(const Zstring& sourceFile,
                     const Zstring& targetFile,
                     CallbackMoveFile* callback) : sourceFile_(sourceFile),
        targetFile_(targetFile),
        moveCallback(callback) {}

    virtual void deleteTargetFile(const Zstring& targetFile) { assert(!fileExists(targetFile)); }

    virtual void updateCopyStatus(Int64 bytesDelta)
    {
        if (moveCallback)
            moveCallback->updateStatus(bytesDelta);
    }

private:
    CopyCallbackImpl(const CopyCallbackImpl&);
    CopyCallbackImpl& operator=(const CopyCallbackImpl&);

    const Zstring sourceFile_;
    const Zstring targetFile_;
    CallbackMoveFile* moveCallback; //optional
};


void zen::moveFile(const Zstring& sourceFile, const Zstring& targetFile, CallbackMoveFile* callback) //throw FileError
{
    if (callback) callback->onBeforeFileMove(sourceFile, targetFile); //call back once *after* work was done

    //first try to move the file directly without copying
    try
    {
        renameFile(sourceFile, targetFile); //throw FileError, ErrorDifferentVolume, ErrorTargetExisting
        //great, we get away cheaply!
        if (callback) callback->objectProcessed();
        return;
    }
    //if moving failed treat as error (except when it tried to move to a different volume: in this case we will copy the file)
    catch (const ErrorDifferentVolume&) {}
    catch (const ErrorTargetExisting&) {}

    //create target
    if (!fileExists(targetFile)) //check even if ErrorTargetExisting: me may have clashed with a directory of the same name!!!
    {
        //file is on a different volume: let's copy it
        if (symlinkExists(sourceFile))
            copySymlink(sourceFile, targetFile, false); //throw FileError; don't copy filesystem permissions
        else
        {
            CopyCallbackImpl copyCallback(sourceFile, targetFile, callback);
            copyFile(sourceFile, targetFile, false, true, &copyCallback); //throw FileError - permissions "false", transactional copy "true"
        }
    }

    //delete source
    removeFile(sourceFile); //throw FileError

    //note: newly copied file is NOT deleted in case of exception: currently this function is called in context of user-defined deletion dir, where this behavior is fine
    if (callback) callback->objectProcessed();
}

namespace
{
class TraverseOneLevel : public zen::TraverseCallback
{
public:
    typedef std::pair<Zstring, Zstring> ShortLongNames;
    typedef std::vector<ShortLongNames> NameList;

    TraverseOneLevel(NameList& files, NameList& dirs) :
        files_(files),
        dirs_(dirs) {}

    virtual void onFile(const Zchar* shortName, const Zstring& fullName, const FileInfo& details)
    {
        files_.push_back(std::make_pair(shortName, fullName));
    }

    virtual HandleLink onSymlink(const Zchar* shortName, const Zstring& fullName, const SymlinkInfo& details)
    {
        if (details.dirLink)
            dirs_.push_back(std::make_pair(shortName, fullName));
        else
            files_.push_back(std::make_pair(shortName, fullName));
        return LINK_SKIP;
    }

    virtual std::shared_ptr<TraverseCallback> onDir(const Zchar* shortName, const Zstring& fullName)
    {
        dirs_.push_back(std::make_pair(shortName, fullName));
        return nullptr; //DON'T traverse into subdirs; moveDirectory works recursively!
    }

    virtual HandleError onError(const std::wstring& errorText) { throw FileError(errorText); }

private:
    TraverseOneLevel(const TraverseOneLevel&);
    TraverseOneLevel& operator=(const TraverseOneLevel&);

    NameList& files_;
    NameList& dirs_;
};


struct RemoveCallbackImpl : public CallbackRemoveDir
{
    RemoveCallbackImpl(CallbackMoveFile* moveCallback) : moveCallback_(moveCallback) {}

    virtual void notifyFileDeletion(const Zstring& filename) { if (moveCallback_) moveCallback_->updateStatus(0); }
    virtual void notifyDirDeletion (const Zstring& dirname ) { if (moveCallback_) moveCallback_->updateStatus(0); }

private:
    RemoveCallbackImpl(const RemoveCallbackImpl&);
    RemoveCallbackImpl& operator=(const RemoveCallbackImpl&);

    CallbackMoveFile* moveCallback_; //optional
};
}


void moveDirectoryImpl(const Zstring& sourceDir, const Zstring& targetDir, CallbackMoveFile* callback) //throw FileError
{
    //note: we cannot support "throw exception if target already exists": If we did, we would have to do a full cleanup
    //removing all newly created directories in case of an exception so that subsequent tries would not fail with "target already existing".
    //However an exception may also happen during final deletion of source folder, in which case cleanup effectively leads to data loss!

    if (callback) callback->onBeforeDirMove(sourceDir, targetDir); //call back once *after* work was done

    //first try to move the directory directly without copying
    try
    {
        renameFile(sourceDir, targetDir); //throw FileError, ErrorDifferentVolume, ErrorTargetExisting
        //great, we get away cheaply!
        if (callback) callback->objectProcessed();
        return;
    }
    //if moving failed treat as error (except when it tried to move to a different volume: in this case we will copy the directory)
    catch (const ErrorDifferentVolume&) {}
    catch (const ErrorTargetExisting& ) {}

    //create target
    if (symlinkExists(sourceDir))
    {
        if (!symlinkExists(targetDir))
            copySymlink(sourceDir, targetDir, false); //throw FileError -> don't copy permissions
    }
    else
    {
        try
        {
            makeNewDirectory(targetDir, sourceDir, false); //FileError, ErrorTargetExisting
        }
        catch (const ErrorTargetExisting&)
        {
            if (!dirExists(targetDir))
                throw; //clashed with a file or symlink of the same name!!!
        }

        //move files/folders recursively
        TraverseOneLevel::NameList fileList; //list of names: 1. short 2.long
        TraverseOneLevel::NameList dirList;  //

        //traverse source directory one level
        TraverseOneLevel traverseCallback(fileList, dirList);
        traverseFolder(sourceDir, traverseCallback); //traverse one level

        const Zstring targetDirPf = appendSeparator(targetDir);

        //move files
        for (TraverseOneLevel::NameList::const_iterator i = fileList.begin(); i != fileList.end(); ++i)
            moveFile(i->second, targetDirPf + i->first, callback); //throw FileError

        //move directories
        for (TraverseOneLevel::NameList::const_iterator i = dirList.begin(); i != dirList.end(); ++i)
            ::moveDirectoryImpl(i->second, targetDirPf + i->first, callback);
    }

    //delete source
    RemoveCallbackImpl removeCallback(callback);
    removeDirectory(sourceDir, &removeCallback); //throw FileError

    if (callback) callback->objectProcessed();
}


void zen::moveDirectory(const Zstring& sourceDir, const Zstring& targetDir, CallbackMoveFile* callback) //throw FileError
{
#ifdef FFS_WIN
    const Zstring& sourceDirFormatted = sourceDir;
    const Zstring& targetDirFormatted = targetDir;

#elif defined FFS_LINUX
    const Zstring sourceDirFormatted = //remove trailing slash
        sourceDir.size() > 1 && endsWith(sourceDir, FILE_NAME_SEPARATOR) ?  //exception: allow '/'
        beforeLast(sourceDir, FILE_NAME_SEPARATOR) :
        sourceDir;
    const Zstring targetDirFormatted = //remove trailing slash
        targetDir.size() > 1 && endsWith(targetDir, FILE_NAME_SEPARATOR) ?  //exception: allow '/'
        beforeLast(targetDir, FILE_NAME_SEPARATOR) :
        targetDir;
#endif

    ::moveDirectoryImpl(sourceDirFormatted, targetDirFormatted, callback);
}


class FilesDirsOnlyTraverser : public zen::TraverseCallback
{
public:
    FilesDirsOnlyTraverser(std::vector<Zstring>& files, std::vector<Zstring>& dirs) :
        m_files(files),
        m_dirs(dirs) {}

    virtual void onFile(const Zchar* shortName, const Zstring& fullName, const FileInfo& details)
    {
        m_files.push_back(fullName);
    }
    virtual HandleLink onSymlink(const Zchar* shortName, const Zstring& fullName, const SymlinkInfo& details)
    {
        if (details.dirLink)
            m_dirs.push_back(fullName);
        else
            m_files.push_back(fullName);
        return LINK_SKIP;
    }
    virtual std::shared_ptr<TraverseCallback> onDir(const Zchar* shortName, const Zstring& fullName)
    {
        m_dirs.push_back(fullName);
        return nullptr; //DON'T traverse into subdirs; removeDirectory works recursively!
    }
    virtual HandleError onError(const std::wstring& errorText) { throw FileError(errorText); }

private:
    FilesDirsOnlyTraverser(const FilesDirsOnlyTraverser&);
    FilesDirsOnlyTraverser& operator=(const FilesDirsOnlyTraverser&);

    std::vector<Zstring>& m_files;
    std::vector<Zstring>& m_dirs;
};


void zen::removeDirectory(const Zstring& directory, CallbackRemoveDir* callback)
{
    //no error situation if directory is not existing! manual deletion relies on it!
    if (!somethingExists(directory))
        return; //neither directory nor any other object (e.g. broken symlink) with that name existing

#ifdef FFS_WIN
    const Zstring directoryFmt = applyLongPathPrefix(directory); //support for \\?\-prefix

    //(try to) normalize file attributes: actually NEEDED for symbolic links also!
    ::SetFileAttributes(directoryFmt.c_str(), FILE_ATTRIBUTE_NORMAL);
#endif

    //attention: check if directory is a symlink! Do NOT traverse into it deleting contained files!!!
    if (symlinkExists(directory)) //remove symlink directly
    {
#ifdef FFS_WIN
        if (!::RemoveDirectory(directoryFmt.c_str()))
#elif defined FFS_LINUX
        if (::unlink(directory.c_str()) != 0)
#endif
            throw FileError(replaceCpy(_("Cannot delete directory %x."), L"%x", fmtFileName(directory)) + L"\n\n" + getLastErrorFormatted());

        if (callback)
            callback->notifyDirDeletion(directory); //once per symlink
        return;
    }

    std::vector<Zstring> fileList;
    std::vector<Zstring> dirList;
    {
        //get all files and directories from current directory (WITHOUT subdirectories!)
        FilesDirsOnlyTraverser traverser(fileList, dirList);
        traverseFolder(directory, traverser); //don't follow symlinks
    }

    //delete directories recursively
    for (auto iter = dirList.begin(); iter != dirList.end(); ++iter)
        removeDirectory(*iter, callback); //call recursively to correctly handle symbolic links

    //delete files
    for (auto iter = fileList.begin(); iter != fileList.end(); ++iter)
    {
        const bool workDone = removeFile(*iter);
        if (callback && workDone)
            callback->notifyFileDeletion(*iter); //call once per file
    }

    //parent directory is deleted last
#ifdef FFS_WIN
    if (!::RemoveDirectory(directoryFmt.c_str()))
#else
    if (::rmdir(directory.c_str()) != 0)
#endif
        throw FileError(replaceCpy(_("Cannot delete directory %x."), L"%x", fmtFileName(directory)) + L"\n\n" + getLastErrorFormatted());

    if (callback)
        callback->notifyDirDeletion(directory); //and once per folder
}


void zen::setFileTime(const Zstring& filename, const Int64& modificationTime, ProcSymlink procSl) //throw FileError
{
#ifdef FFS_WIN
    FILETIME creationTime  = {};
    FILETIME lastWriteTime = tofiletime(modificationTime);

    //####################################### DST hack ###########################################
    if (dst::isFatDrive(filename)) //throw()
    {
        const dst::RawTime encodedTime = dst::fatEncodeUtcTime(lastWriteTime); //throw std::runtime_error
        creationTime  = encodedTime.createTimeRaw;
        lastWriteTime = encodedTime.writeTimeRaw;
    }
    //####################################### DST hack ###########################################

    //privilege SE_BACKUP_NAME doesn't seem to be required here for symbolic links
    //note: setting privileges requires admin rights!

    //opening newly created target file may fail due to some AV-software scanning it: no error, we will wait!
    //http://support.microsoft.com/?scid=kb%3Ben-us%3B316609&x=17&y=20
    //-> enable as soon it turns out it is required!

    /*const int retryInterval = 50;
    const int maxRetries = 2000 / retryInterval;
    for (int i = 0; i < maxRetries; ++i)
    {
    */

    /*
    if (hTarget == INVALID_HANDLE_VALUE && ::GetLastError() == ERROR_SHARING_VIOLATION)
        ::Sleep(retryInterval); //wait then retry
    else //success or unknown failure
        break;
    }
    */

    //temporarily reset read-only flag if required
    DWORD attribs = INVALID_FILE_ATTRIBUTES;
    ZEN_ON_SCOPE_EXIT(
        if (attribs != INVALID_FILE_ATTRIBUTES)
        ::SetFileAttributes(applyLongPathPrefix(filename).c_str(), attribs);
    );

    auto removeReadonly = [&]() -> bool //may need to remove the readonly-attribute (e.g. on FAT usb drives)
    {
        if (attribs == INVALID_FILE_ATTRIBUTES)
        {
            const DWORD tmpAttr = ::GetFileAttributes(applyLongPathPrefix(filename).c_str());
            if (tmpAttr == INVALID_FILE_ATTRIBUTES)
                throw FileError(replaceCpy(_("Cannot read file attributes of %x."), L"%x", fmtFileName(filename)) + L"\n\n" + getLastErrorFormatted());

            if (tmpAttr & FILE_ATTRIBUTE_READONLY)
            {
                if (!::SetFileAttributes(applyLongPathPrefix(filename).c_str(), FILE_ATTRIBUTE_NORMAL))
                    throw FileError(replaceCpy(_("Cannot write file attributes of %x."), L"%x", fmtFileName(filename)) + L"\n\n" + getLastErrorFormatted());

                attribs = tmpAttr; //reapplied on scope exit
                return true;
            }
        }
        return false;
    };

    auto openFile = [&](bool conservativeApproach)
    {
        return ::CreateFile(applyLongPathPrefix(filename).c_str(),
                            (conservativeApproach ?
                             //some NAS seem to have issues with FILE_WRITE_ATTRIBUTES, even worse, they may fail silently!
                             //http://sourceforge.net/tracker/?func=detail&atid=1093081&aid=3536680&group_id=234430
                             //Citrix shares seem to have this issue, too, but at least fail with "access denied" => try generic access first:
                             GENERIC_READ | GENERIC_WRITE :
                             //avoids mysterious "access denied" when using "GENERIC_READ | GENERIC_WRITE" on a read-only file, even *after* read-only was removed directly before the call!
                             //http://sourceforge.net/tracker/?func=detail&atid=1093080&aid=3514569&group_id=234430
                             //since former gives an error notification we may very well try FILE_WRITE_ATTRIBUTES second.
                             FILE_READ_ATTRIBUTES | FILE_WRITE_ATTRIBUTES),
                            FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                            nullptr,
                            OPEN_EXISTING,
                            FILE_FLAG_BACKUP_SEMANTICS | //needed to open a directory
                            (procSl == SYMLINK_DIRECT ? FILE_FLAG_OPEN_REPARSE_POINT : 0), //process symlinks
                            nullptr);
    };

    HANDLE hFile = INVALID_HANDLE_VALUE;
    for (int i = 0; i < 2; ++i) //we will get this handle, no matter what! :)
    {
        //1. be conservative
        hFile = openFile(true);
        if (hFile == INVALID_HANDLE_VALUE)
        {
            if (::GetLastError() == ERROR_ACCESS_DENIED) //fails if file is read-only (or for "other" reasons)
                if (removeReadonly())
                    continue;

            //2. be a *little* fancy
            hFile = openFile(false);
            if (hFile == INVALID_HANDLE_VALUE)
            {
                if (::GetLastError() == ERROR_ACCESS_DENIED)
                    if (removeReadonly())
                        continue;

                //3. after these herculean stunts we give up...
                throw FileError(replaceCpy(_("Cannot write modification time of %x."), L"%x", fmtFileName(filename)) + L"\n\n" + getLastErrorFormatted());
            }
        }
        break;
    }
    ZEN_ON_SCOPE_EXIT(::CloseHandle(hFile));


    auto isNullTime = [](const FILETIME& ft) { return ft.dwLowDateTime == 0 && ft.dwHighDateTime == 0; };

    if (!::SetFileTime(hFile,           //__in      HANDLE hFile,
                       !isNullTime(creationTime) ? &creationTime : nullptr, //__in_opt  const FILETIME *lpCreationTime,
                       nullptr,         //__in_opt  const FILETIME *lpLastAccessTime,
                       &lastWriteTime)) //__in_opt  const FILETIME *lpLastWriteTime
    {
        auto lastErr = ::GetLastError();

        //function may fail if file is read-only: https://sourceforge.net/tracker/?func=detail&atid=1093080&aid=3514569&group_id=234430
        if (lastErr == ERROR_ACCESS_DENIED)
        {
            //dynamically load windows API function: available with Windows Vista and later
            typedef BOOL (WINAPI* SetFileInformationByHandleFunc)(HANDLE hFile, FILE_INFO_BY_HANDLE_CLASS FileInformationClass, LPVOID lpFileInformation, DWORD dwBufferSize);

            const SysDllFun<SetFileInformationByHandleFunc> setFileInformationByHandle(L"kernel32.dll", "SetFileInformationByHandle");
            if (setFileInformationByHandle) //if not: let the original error propagate!
            {
                auto setFileInfo = [&](FILE_BASIC_INFO basicInfo) //throw FileError; no const& since SetFileInformationByHandle() requires non-const parameter!
                {
                    if (!setFileInformationByHandle(hFile,              //__in  HANDLE hFile,
                                                    FileBasicInfo,      //__in  FILE_INFO_BY_HANDLE_CLASS FileInformationClass,
                                                    &basicInfo,         //__in  LPVOID lpFileInformation,
                                                    sizeof(basicInfo))) //__in  DWORD dwBufferSize
                        throw FileError(replaceCpy(_("Cannot write file attributes of %x."), L"%x", fmtFileName(filename)) + L"\n\n" + getLastErrorFormatted());
                };

                auto toLargeInteger = [](const FILETIME& ft) -> LARGE_INTEGER
                {
                    LARGE_INTEGER tmp = {};
                    tmp.LowPart  = ft.dwLowDateTime;
                    tmp.HighPart = ft.dwHighDateTime;
                    return tmp;
                };
                //---------------------------------------------------------------------------

                BY_HANDLE_FILE_INFORMATION fileInfo = {};
                if (::GetFileInformationByHandle(hFile, &fileInfo))
                    if (fileInfo.dwFileAttributes & FILE_ATTRIBUTE_READONLY)
                    {
                        FILE_BASIC_INFO basicInfo = {}; //undocumented: file times of "0" stand for "don't change"
                        basicInfo.FileAttributes = FILE_ATTRIBUTE_NORMAL;        //[!] the bug in the ticket above requires we set this together with file times!!!
                        basicInfo.LastWriteTime = toLargeInteger(lastWriteTime); //
                        if (!isNullTime(creationTime))
                            basicInfo.CreationTime = toLargeInteger(creationTime);

                        //set file time + attributes
                        setFileInfo(basicInfo); //throw FileError

                        try //... to restore original file attributes
                        {
                            FILE_BASIC_INFO basicInfo2 = {};
                            basicInfo2.FileAttributes = fileInfo.dwFileAttributes;
                            setFileInfo(basicInfo2); //throw FileError
                        }
                        catch (FileError&) {}

                        lastErr = ERROR_SUCCESS;
                    }
            }
        }

        if (lastErr != ERROR_SUCCESS)
            throw FileError(replaceCpy(_("Cannot write modification time of %x."), L"%x", fmtFileName(filename)) + L"\n\n" + getLastErrorFormatted(lastErr));
    }

#ifndef NDEBUG //dst hack: verify data written
    if (dst::isFatDrive(filename) && !dirExists(filename)) //throw()
    {
        WIN32_FILE_ATTRIBUTE_DATA debugeAttr = {};
        assert(::GetFileAttributesEx(applyLongPathPrefix(filename).c_str(), //__in   LPCTSTR lpFileName,
                                     GetFileExInfoStandard,                 //__in   GET_FILEEX_INFO_LEVELS fInfoLevelId,
                                     &debugeAttr));                         //__out  LPVOID lpFileInformation

        assert(::CompareFileTime(&debugeAttr.ftCreationTime,  &creationTime)  == 0);
        assert(::CompareFileTime(&debugeAttr.ftLastWriteTime, &lastWriteTime) == 0);
    }
#endif

#elif defined FFS_LINUX
    if (procSl == SYMLINK_FOLLOW)
    {
        struct utimbuf newTimes = {};
        newTimes.actime  = ::time(nullptr);
        newTimes.modtime = to<time_t>(modificationTime);

        // set new "last write time"
        if (::utime(filename.c_str(), &newTimes) != 0)
            throw FileError(replaceCpy(_("Cannot write modification time of %x."), L"%x", fmtFileName(filename)) + L"\n\n" + getLastErrorFormatted());
    }
    else
    {
        struct timeval newTimes[2] = {};
        newTimes[0].tv_sec  = ::time(nullptr); //seconds
        newTimes[0].tv_usec = 0;	           //microseconds

        newTimes[1].tv_sec  = to<time_t>(modificationTime);
        newTimes[1].tv_usec = 0;

        if (::lutimes(filename.c_str(), newTimes) != 0)
            throw FileError(replaceCpy(_("Cannot write modification time of %x."), L"%x", fmtFileName(filename)) + L"\n\n" + getLastErrorFormatted());
    }
#endif
}


bool zen::supportsPermissions(const Zstring& dirname) //throw FileError
{
#ifdef FFS_WIN
    const DWORD bufferSize = MAX_PATH + 1;
    std::vector<wchar_t> buffer(bufferSize);
    if (!::GetVolumePathName(dirname.c_str(), //__in   LPCTSTR lpszFileName,
                             &buffer[0],      //__out  LPTSTR lpszVolumePathName,
                             bufferSize))     //__in   DWORD cchBufferLength
        throw FileError(replaceCpy(_("Cannot read file attributes of %x."), L"%x", fmtFileName(dirname)) + L"\n\n" + getLastErrorFormatted());

    DWORD fsFlags = 0;
    if (!::GetVolumeInformation(&buffer[0], //__in_opt   LPCTSTR lpRootPathName,
                                nullptr,    //__out      LPTSTR  lpVolumeNameBuffer,
                                0,          //__in       DWORD   nVolumeNameSize,
                                nullptr,    //__out_opt  LPDWORD lpVolumeSerialNumber,
                                nullptr,    //__out_opt  LPDWORD lpMaximumComponentLength,
                                &fsFlags,   //__out_opt  LPDWORD lpFileSystemFlags,
                                nullptr,    //__out      LPTSTR  lpFileSystemNameBuffer,
                                0))         //__in       DWORD   nFileSystemNameSize
        throw FileError(replaceCpy(_("Cannot read file attributes of %x."), L"%x", fmtFileName(dirname)) + L"\n\n" + getLastErrorFormatted());

    return (fsFlags & FILE_PERSISTENT_ACLS) != 0;

#elif defined FFS_LINUX
    return true;
#endif
}


namespace
{
#ifdef FFS_WIN
Zstring getSymlinkTargetPath(const Zstring& symlink) //throw FileError
{
    //open handle to target of symbolic link
    const HANDLE hDir = ::CreateFile(applyLongPathPrefix(symlink).c_str(),
                                     0,
                                     FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                                     nullptr,
                                     OPEN_EXISTING,
                                     FILE_FLAG_BACKUP_SEMANTICS,  //needed to open a directory
                                     nullptr);
    if (hDir == INVALID_HANDLE_VALUE)
        throw FileError(replaceCpy(_("Cannot resolve symbolic link %x."), L"%x", fmtFileName(symlink)) + L"\n\n" + getLastErrorFormatted());
    ZEN_ON_SCOPE_EXIT(::CloseHandle(hDir));

    //dynamically load windows API function
    typedef DWORD (WINAPI* GetFinalPathNameByHandleWFunc)(HANDLE hFile,
                                                          LPTSTR lpszFilePath,
                                                          DWORD cchFilePath,
                                                          DWORD dwFlags);
    const SysDllFun<GetFinalPathNameByHandleWFunc> getFinalPathNameByHandle(L"kernel32.dll", "GetFinalPathNameByHandleW");
    if (!getFinalPathNameByHandle)
        throw FileError(replaceCpy(_("Cannot find system function %x."), L"%x", L"\"GetFinalPathNameByHandleW\""));

    const DWORD BUFFER_SIZE = 10000;
    std::vector<wchar_t> targetPath(BUFFER_SIZE);
    const DWORD charsWritten = getFinalPathNameByHandle(hDir,                  //__in   HANDLE hFile,
                                                        &targetPath[0],        //__out  LPTSTR lpszFilePath,
                                                        BUFFER_SIZE,           //__in   DWORD cchFilePath,
                                                        FILE_NAME_NORMALIZED); //__in   DWORD dwFlags
    if (charsWritten >= BUFFER_SIZE || charsWritten == 0)
    {
        std::wstring errorMessage = replaceCpy(_("Cannot resolve symbolic link %x."), L"%x", fmtFileName(symlink));
        if (charsWritten == 0)
            errorMessage += L"\n\n" + getLastErrorFormatted();
        throw FileError(errorMessage);
    }

    return Zstring(&targetPath[0], charsWritten);
}
#endif


#ifdef HAVE_SELINUX
//copy SELinux security context
void copySecurityContext(const Zstring& source, const Zstring& target, ProcSymlink procSl) //throw FileError
{
    security_context_t contextSource = nullptr;
    const int rv = procSl == SYMLINK_FOLLOW ?
                   ::getfilecon(source.c_str(), &contextSource) :
                   ::lgetfilecon(source.c_str(), &contextSource);
    if (rv < 0)
    {
        if (errno == ENODATA ||  //no security context (allegedly) is not an error condition on SELinux
            errno == EOPNOTSUPP) //extended attributes are not supported by the filesystem
            return;

        throw FileError(replaceCpy(_("Cannot read security context of %x."), L"%x", fmtFileName(source)) + L"\n\n" + getLastErrorFormatted());
    }
    ZEN_ON_SCOPE_EXIT(::freecon(contextSource));

    {
        security_context_t contextTarget = nullptr;
        const int rv2 = procSl == SYMLINK_FOLLOW ?
                        ::getfilecon(target.c_str(), &contextTarget) :
                        ::lgetfilecon(target.c_str(), &contextTarget);
        if (rv2 < 0)
        {
            if (errno == EOPNOTSUPP)
                return;
            //else: still try to set security context
        }
        else
        {
            ZEN_ON_SCOPE_EXIT(::freecon(contextTarget));

            if (::strcmp(contextSource, contextTarget) == 0) //nothing to do
                return;
        }
    }

    const int rv3 = procSl == SYMLINK_FOLLOW ?
                    ::setfilecon(target.c_str(), contextSource) :
                    ::lsetfilecon(target.c_str(), contextSource);
    if (rv3 < 0)
        throw FileError(replaceCpy(_("Cannot write security context of %x."), L"%x", fmtFileName(target)) + L"\n\n" + getLastErrorFormatted());
}
#endif //HAVE_SELINUX


//copy permissions for files, directories or symbolic links: requires admin rights
void copyObjectPermissions(const Zstring& source, const Zstring& target, ProcSymlink procSl) //throw FileError
{
#ifdef FFS_WIN
    //setting privileges requires admin rights!

    //enable privilege: required to read/write SACL information (only)
    activatePrivilege(SE_SECURITY_NAME); //throw FileError
    //Note: trying to copy SACL (SACL_SECURITY_INFORMATION) may return ERROR_PRIVILEGE_NOT_HELD (1314) on Samba shares. This is not due to missing privileges!
    //However, this is okay, since copying NTFS permissions doesn't make sense in this case anyway

    //enable privilege: required to copy owner information
    activatePrivilege(SE_RESTORE_NAME); //throw FileError

    //the following privilege may be required according to http://msdn.microsoft.com/en-us/library/aa364399(VS.85).aspx (although not needed nor active in my tests)
    activatePrivilege(SE_BACKUP_NAME); //throw FileError


    //in contrast to ::SetSecurityInfo(), ::SetFileSecurity() seems to honor the "inherit DACL/SACL" flags
    //CAVEAT: if a file system does not support ACLs, GetFileSecurity() will return successfully with a *valid* security descriptor containing *no* ACL entries!

    //NOTE: ::GetFileSecurity()/::SetFileSecurity() do NOT follow Symlinks!
    const Zstring sourceResolved = procSl == SYMLINK_FOLLOW && symlinkExists(source) ? getSymlinkTargetPath(source) : source;
    const Zstring targetResolved = procSl == SYMLINK_FOLLOW && symlinkExists(target) ? getSymlinkTargetPath(target) : target;

    std::vector<char> buffer(10000); //example of actually required buffer size: 192 bytes
    for (;;)
    {
        DWORD bytesNeeded = 0;
        if (::GetFileSecurity(applyLongPathPrefix(sourceResolved).c_str(), //__in LPCTSTR lpFileName, -> long path prefix IS needed, although it is NOT mentioned on MSDN!!!
                              OWNER_SECURITY_INFORMATION | GROUP_SECURITY_INFORMATION |
                              DACL_SECURITY_INFORMATION  | SACL_SECURITY_INFORMATION, //__in       SECURITY_INFORMATION RequestedInformation,
                              reinterpret_cast<PSECURITY_DESCRIPTOR>(&buffer[0]),     //__out_opt  PSECURITY_DESCRIPTOR pSecurityDescriptor,
                              static_cast<DWORD>(buffer.size()), //__in       DWORD nLength,
                              &bytesNeeded))                     //__out      LPDWORD lpnLengthNeeded
            break;
        //failure: ...
        if (bytesNeeded > buffer.size())
            buffer.resize(bytesNeeded);
        else
            throw FileError(replaceCpy(_("Cannot read permissions of %x."), L"%x", fmtFileName(sourceResolved)) + L"\n\n" + getLastErrorFormatted());
    }
    SECURITY_DESCRIPTOR& secDescr = reinterpret_cast<SECURITY_DESCRIPTOR&>(buffer[0]);

    /*
        SECURITY_DESCRIPTOR_CONTROL secCtrl = 0;
        {
            DWORD ctrlRev = 0;
            if (!::GetSecurityDescriptorControl(&secDescr, // __in   PSECURITY_DESCRIPTOR pSecurityDescriptor,
                                                &secCtrl,  // __out  PSECURITY_DESCRIPTOR_CONTROL pControl,
                                                &ctrlRev)) //__out  LPDWORD lpdwRevision
                throw FileErro
       }
    //interesting flags:
    //#define SE_DACL_PRESENT                  (0x0004)
    //#define SE_SACL_PRESENT                  (0x0010)
    //#define SE_DACL_PROTECTED                (0x1000)
    //#define SE_SACL_PROTECTED                (0x2000)
    */

    if (!::SetFileSecurity(applyLongPathPrefix(targetResolved).c_str(), //__in  LPCTSTR lpFileName, -> long path prefix IS needed, although it is NOT mentioned on MSDN!!!
                           OWNER_SECURITY_INFORMATION | GROUP_SECURITY_INFORMATION |
                           DACL_SECURITY_INFORMATION  | SACL_SECURITY_INFORMATION, //__in  SECURITY_INFORMATION SecurityInformation,
                           &secDescr)) //__in  PSECURITY_DESCRIPTOR pSecurityDescriptor
        throw FileError(replaceCpy(_("Cannot write permissions of %x."), L"%x", fmtFileName(targetResolved)) + L"\n\n" + getLastErrorFormatted());

    /*
    PSECURITY_DESCRIPTOR buffer = nullptr;
    PSID owner                  = nullptr;
    PSID group                  = nullptr;
    PACL dacl                   = nullptr;
    PACL sacl                   = nullptr;

    //File Security and Access Rights:    http://msdn.microsoft.com/en-us/library/aa364399(v=VS.85).aspx
    //SECURITY_INFORMATION Access Rights: http://msdn.microsoft.com/en-us/library/windows/desktop/aa379573(v=vs.85).aspx
    const HANDLE hSource = ::CreateFile(applyLongPathPrefix(source).c_str(),
                                        READ_CONTROL | ACCESS_SYSTEM_SECURITY, //ACCESS_SYSTEM_SECURITY required for SACL access
                                        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                                        nullptr,
                                        OPEN_EXISTING,
                                        FILE_FLAG_BACKUP_SEMANTICS | (procSl == SYMLINK_DIRECT ? FILE_FLAG_OPEN_REPARSE_POINT : 0), //FILE_FLAG_BACKUP_SEMANTICS needed to open a directory
                                        nullptr);
    if (hSource == INVALID_HANDLE_VALUE)
        throw FileError
    ZEN_ON_SCOPE_EXIT(::CloseHandle(hSource));

    //  DWORD rc = ::GetNamedSecurityInfo(const_cast<WCHAR*>(applyLongPathPrefix(source).c_str()), -> does NOT dereference symlinks!
    DWORD rc = ::GetSecurityInfo(hSource,        //__in       LPTSTR pObjectName,
                                 SE_FILE_OBJECT, //__in       SE_OBJECT_TYPE ObjectType,
                                 OWNER_SECURITY_INFORMATION | GROUP_SECURITY_INFORMATION |
                                 DACL_SECURITY_INFORMATION | SACL_SECURITY_INFORMATION,  //__in       SECURITY_INFORMATION SecurityInfo,
                                 &owner,   //__out_opt  PSID *ppsidOwner,
                                 &group,   //__out_opt  PSID *ppsidGroup,
                                 &dacl,    //__out_opt  PACL *ppDacl,
                                 &sacl,    //__out_opt  PACL *ppSacl,
                                 &buffer); //__out_opt  PSECURITY_DESCRIPTOR *ppSecurityDescriptor
    if (rc != ERROR_SUCCESS)
        throw FileError
    ZEN_ON_SCOPE_EXIT(::LocalFree(buffer));

    SECURITY_DESCRIPTOR_CONTROL secCtrl = 0;
    {
    DWORD ctrlRev = 0;
    if (!::GetSecurityDescriptorControl(buffer, // __in   PSECURITY_DESCRIPTOR pSecurityDescriptor,
    &secCtrl, // __out  PSECURITY_DESCRIPTOR_CONTROL pControl,
    &ctrlRev))//__out  LPDWORD lpdwRevision
        throw FileError
    }

    //may need to remove the readonly-attribute
    FileUpdateHandle targetHandle(target, [=]
    {
        return ::CreateFile(applyLongPathPrefix(target).c_str(),                              // lpFileName
                            GENERIC_WRITE | WRITE_OWNER | WRITE_DAC | ACCESS_SYSTEM_SECURITY, // dwDesiredAccess: all four seem to be required!!!
                            FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,           // dwShareMode
                            nullptr,                       // lpSecurityAttributes
                            OPEN_EXISTING,              // dwCreationDisposition
                            FILE_FLAG_BACKUP_SEMANTICS | (procSl == SYMLINK_DIRECT ? FILE_FLAG_OPEN_REPARSE_POINT : 0), // dwFlagsAndAttributes
                            nullptr);                        // hTemplateFile
    });

    if (targetHandle.get() == INVALID_HANDLE_VALUE)
        throw FileError

    	SECURITY_INFORMATION secFlags = OWNER_SECURITY_INFORMATION | GROUP_SECURITY_INFORMATION | DACL_SECURITY_INFORMATION | SACL_SECURITY_INFORMATION;

    	//SACL/DACL inheritence flag is NOT copied by default: we have to tell ::SetSecurityInfo(() to enable/disable it manually!
    	//if (secCtrl & SE_DACL_PRESENT)
    	secFlags |= (secCtrl & SE_DACL_PROTECTED) ? PROTECTED_DACL_SECURITY_INFORMATION : UNPROTECTED_DACL_SECURITY_INFORMATION;
    	//if (secCtrl & SE_SACL_PRESENT)
    	secFlags |= (secCtrl & SE_SACL_PROTECTED) ? PROTECTED_SACL_SECURITY_INFORMATION : UNPROTECTED_SACL_SECURITY_INFORMATION;


    //  rc = ::SetNamedSecurityInfo(const_cast<WCHAR*>(applyLongPathPrefix(target).c_str()), //__in      LPTSTR pObjectName, -> does NOT dereference symlinks!
    rc = ::SetSecurityInfo(targetHandle.get(), //__in      LPTSTR pObjectName,
                           SE_FILE_OBJECT,     //__in      SE_OBJECT_TYPE ObjectType,
                           secFlags, //__in      SECURITY_INFORMATION SecurityInfo,
                           owner, //__in_opt  PSID psidOwner,
                           group, //__in_opt  PSID psidGroup,
                           dacl,  //__in_opt  PACL pDacl,
                           sacl); //__in_opt  PACL pSacl

    if (rc != ERROR_SUCCESS)
        throw FileError
    		*/

#elif defined FFS_LINUX

#ifdef HAVE_SELINUX  //copy SELinux security context
    copySecurityContext(source, target, procSl); //throw FileError
#endif

    struct stat fileInfo = {};
    if (procSl == SYMLINK_FOLLOW)
    {
        if (::stat(source.c_str(), &fileInfo) != 0)
            throw FileError(replaceCpy(_("Cannot read permissions of %x."), L"%x", fmtFileName(source)) + L"\n\n" + getLastErrorFormatted());

        if (::chown(target.c_str(), fileInfo.st_uid, fileInfo.st_gid) != 0 || // may require admin rights!
            ::chmod(target.c_str(), fileInfo.st_mode) != 0)
            throw FileError(replaceCpy(_("Cannot write permissions of %x."), L"%x", fmtFileName(target)) + L"\n\n" + getLastErrorFormatted());
    }
    else
    {
        if (::lstat(source.c_str(), &fileInfo) != 0)
            throw FileError(replaceCpy(_("Cannot read permissions of %x."), L"%x", fmtFileName(source)) + L"\n\n" + getLastErrorFormatted());

        if (::lchown(target.c_str(), fileInfo.st_uid, fileInfo.st_gid) != 0 || // may require admin rights!
            (!symlinkExists(target) && ::chmod(target.c_str(), fileInfo.st_mode) != 0)) //setting access permissions doesn't make sense for symlinks on Linux: there is no lchmod()
            throw FileError(replaceCpy(_("Cannot write permissions of %x."), L"%x", fmtFileName(target)) + L"\n\n" + getLastErrorFormatted());
    }
#endif
}


void createDirectoryStraight(const Zstring& directory, //throw FileError, ErrorTargetExisting, ErrorTargetPathMissing
                             const Zstring& templateDir,
                             bool copyFilePermissions)
{
#ifdef FFS_WIN
    //don't use ::CreateDirectoryEx:
    //- it may fail with "wrong parameter (error code 87)" when source is on mapped online storage
    //- automatically copies symbolic links if encountered: unfortunately it doesn't copy symlinks over network shares but silently creates empty folders instead (on XP)!
    //- it isn't able to copy most junctions because of missing permissions (although target path can be retrieved alternatively!)
    if (!::CreateDirectory(applyLongPathPrefixCreateDir(directory).c_str(), //__in      LPCTSTR lpPathName,
                           nullptr))                                        //__in_opt  LPSECURITY_ATTRIBUTES lpSecurityAttributes
    {
        const std::wstring msg = replaceCpy(_("Cannot create directory %x."), L"%x", fmtFileName(directory)) + L"\n\n" + getLastErrorFormatted();
        const ErrorCode lastError = getLastError();

        if (lastError == ERROR_ALREADY_EXISTS)
            throw ErrorTargetExisting(msg);
        else if (lastError == ERROR_PATH_NOT_FOUND)
            throw ErrorTargetPathMissing(msg);
        throw FileError(msg);
    }

#elif defined FFS_LINUX
    if (::mkdir(directory.c_str(), 0755) != 0) //mode: drwxr-xr-x
    {
        const std::wstring msg = replaceCpy(_("Cannot create directory %x."), L"%x", fmtFileName(directory)) + L"\n\n" + getLastErrorFormatted();
        const ErrorCode lastError = getLastError();

        if (lastError == EEXIST)
            throw ErrorTargetExisting(msg);
        else if (lastError == ENOENT)
            throw ErrorTargetPathMissing(msg);
        throw FileError(msg);
    }
#endif

    if (!templateDir.empty())
    {
#ifdef FFS_WIN
        //try to copy file attributes
        Zstring sourcePath;

        if (symlinkExists(templateDir)) //dereference symlink!
        {
            try
            {
                //get target directory of symbolic link
                sourcePath = getSymlinkTargetPath(templateDir); //throw FileError
            }
            catch (FileError&) {} //dereferencing a symbolic link usually fails if it is located on network drive or client is XP: NOT really an error...
        }
        else     //no symbolic link
            sourcePath = templateDir;

        //try to copy file attributes
        if (!sourcePath.empty())
        {
            const DWORD sourceAttr = ::GetFileAttributes(applyLongPathPrefix(sourcePath).c_str());
            if (sourceAttr != INVALID_FILE_ATTRIBUTES)
            {
                ::SetFileAttributes(applyLongPathPrefix(directory).c_str(), sourceAttr);
                //copy "read-only and system attributes": http://blogs.msdn.com/b/oldnewthing/archive/2003/09/30/55100.aspx

                const bool isCompressed = (sourceAttr & FILE_ATTRIBUTE_COMPRESSED)  != 0;
                const bool isEncrypted  = (sourceAttr & FILE_ATTRIBUTE_ENCRYPTED)   != 0;

                if (isEncrypted)
                    ::EncryptFile(directory.c_str()); //seems no long path is required (check passed!)

                if (isCompressed)
                {
                    HANDLE hDir = ::CreateFile(applyLongPathPrefix(directory).c_str(),
                                               GENERIC_READ | GENERIC_WRITE, //read access required for FSCTL_SET_COMPRESSION
                                               FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                                               nullptr,
                                               OPEN_EXISTING,
                                               FILE_FLAG_BACKUP_SEMANTICS,
                                               nullptr);
                    if (hDir != INVALID_HANDLE_VALUE)
                    {
                        ZEN_ON_SCOPE_EXIT(::CloseHandle(hDir));

                        USHORT cmpState = COMPRESSION_FORMAT_DEFAULT;
                        DWORD bytesReturned = 0;
                        ::DeviceIoControl(hDir,                  //handle to file or directory
                                          FSCTL_SET_COMPRESSION, //dwIoControlCode
                                          &cmpState,             //input buffer
                                          sizeof(cmpState),      //size of input buffer
                                          nullptr,               //lpOutBuffer
                                          0,                     //OutBufferSize
                                          &bytesReturned,        //number of bytes returned
                                          nullptr);              //OVERLAPPED structure
                    }
                }
            }
        }
#endif

        zen::ScopeGuard guardNewDir = zen::makeGuard([&] { try { removeDirectory(directory); } catch (...) {} }); //ensure cleanup:

        //enforce copying file permissions: it's advertized on GUI...
        if (copyFilePermissions)
            copyObjectPermissions(templateDir, directory, SYMLINK_FOLLOW); //throw FileError

        guardNewDir.dismiss(); //target has been created successfully!
    }
}


void createDirectoryRecursively(const Zstring& directory, const Zstring& templateDir, bool copyFilePermissions) //FileError, ErrorTargetExisting
{
    try
    {
        createDirectoryStraight(directory, templateDir, copyFilePermissions); //throw FileError, ErrorTargetExisting, ErrorTargetPathMissing
    }
    catch (const ErrorTargetExisting&)
    {
#ifdef FFS_WIN
        //handle issues with already existing short 8.3 file names on Windows
        if (have8dot3NameClash(directory))
        {
            Fix8Dot3NameClash dummy(directory); //move clashing object to the side

            //now try again...
            createDirectoryStraight(directory, templateDir, copyFilePermissions); //throw FileError, ErrorTargetExisting, ErrorTargetPathMissing
            return;
        }
#endif
        throw;
    }
    catch (const ErrorTargetPathMissing&)
    {
        //we need to create parent directories first
        const Zstring dirParent = beforeLast(directory, FILE_NAME_SEPARATOR);
        if (!dirParent.empty())
        {
            //call function recursively
            const Zstring templateParent = beforeLast(templateDir, FILE_NAME_SEPARATOR); //returns empty string if ch not found
            createDirectoryRecursively(dirParent, templateParent, copyFilePermissions); //throw

            //now try again...
            createDirectoryStraight(directory, templateDir, copyFilePermissions); //throw FileError, ErrorTargetExisting, ErrorTargetPathMissing
            return;
        }
        throw;
    }
}
}


void zen::makeNewDirectory(const Zstring& directory, const Zstring& templateDir, bool copyFilePermissions) //FileError, ErrorTargetExisting
{
    //remove trailing separator
    const Zstring dirFormatted = endsWith(directory, FILE_NAME_SEPARATOR) ?
                                 beforeLast(directory, FILE_NAME_SEPARATOR) :
                                 directory;

    const Zstring templateFormatted = endsWith(templateDir, FILE_NAME_SEPARATOR) ?
                                      beforeLast(templateDir, FILE_NAME_SEPARATOR) :
                                      templateDir;

    createDirectoryRecursively(dirFormatted, templateFormatted, copyFilePermissions); //FileError, ErrorTargetExisting
}


void zen::makeDirectory(const Zstring& directory)
{
    try
    {
        makeNewDirectory(directory, Zstring(), false); //FileError, ErrorTargetExisting
    }
    catch (const ErrorTargetExisting&)
    {
        if (dirExists(directory))
            return;
        throw; //clash with file (dir symlink is okay)
    }
}


void zen::copySymlink(const Zstring& sourceLink, const Zstring& targetLink, bool copyFilePermissions) //throw FileError
{
    const Zstring linkPath = getSymlinkRawTargetString(sourceLink); //accept broken symlinks; throw FileError

#ifdef FFS_WIN
    const bool isDirLink = [&]() -> bool
    {
        const DWORD ret = ::GetFileAttributes(applyLongPathPrefix(sourceLink).c_str());
        return ret != INVALID_FILE_ATTRIBUTES && (ret& FILE_ATTRIBUTE_DIRECTORY);
    }();

    //dynamically load windows API function
    typedef BOOLEAN (WINAPI* CreateSymbolicLinkFunc)(LPCTSTR lpSymlinkFileName, LPCTSTR lpTargetFileName, DWORD dwFlags);

    const SysDllFun<CreateSymbolicLinkFunc> createSymbolicLink(L"kernel32.dll", "CreateSymbolicLinkW");
    if (!createSymbolicLink)
        throw FileError(replaceCpy(_("Cannot find system function %x."), L"%x", L"\"CreateSymbolicLinkW\""));

    if (!createSymbolicLink(targetLink.c_str(), //__in  LPTSTR lpSymlinkFileName, - seems no long path prefix is required...
                            linkPath.c_str(),   //__in  LPTSTR lpTargetFileName,
                            (isDirLink ? SYMBOLIC_LINK_FLAG_DIRECTORY : 0))) //__in  DWORD dwFlags
#elif defined FFS_LINUX
    if (::symlink(linkPath.c_str(), targetLink.c_str()) != 0)
#endif
        throw FileError(replaceCpy(replaceCpy(_("Cannot copy symbolic link %x to %y."), L"%x", fmtFileName(sourceLink)), L"%y", fmtFileName(targetLink)) +
                        L"\n\n" + getLastErrorFormatted());

    //allow only consistent objects to be created -> don't place before ::symlink, targetLink may already exist
    zen::ScopeGuard guardNewDir = zen::makeGuard([&]
    {
        try
        {
#ifdef FFS_WIN
            if (isDirLink)
                removeDirectory(targetLink);
            else
#endif
                removeFile(targetLink);
        }
        catch (...) {}
    });

    //file times: essential for a symlink: enforce this! (don't just try!)
    {
        const Int64 modTime = getFileTime(sourceLink, SYMLINK_DIRECT); //throw FileError
        setFileTime(targetLink, modTime, SYMLINK_DIRECT); //throw FileError
    }

    if (copyFilePermissions)
        copyObjectPermissions(sourceLink, targetLink, SYMLINK_DIRECT); //throw FileError

    guardNewDir.dismiss(); //target has been created successfully!
}


namespace
{
#ifdef FFS_WIN
/*
            CopyFileEx()    BackupRead()      FileRead()
			--------------------------------------------
Attributes       YES            NO			     NO
create time      NO             NO               NO
ADS			     YES            YES				 NO
Encrypted	     YES            NO(silent fail!) NO
Compressed	     NO             NO		         NO
Sparse		     NO             YES				 NO
Nonstandard FS   YES                  UNKNOWN    -> issues writing ADS to Samba, issues reading from NAS, error copying files having "blocked" state... ect.
PERF		       -                 6% faster

Mark stream as compressed: FSCTL_SET_COMPRESSION - compatible with both BackupRead() and FileRead()


Current support for combinations of NTFS extended attributes:

source attr | tf normal | tf compressed | tf encrypted | handled by
============|==================================================================
    ---     |    ---           -C-             E--       copyFileWindowsDefault
    --S     |    --S           -CS             E-S       copyFileWindowsSparse
    -C-     |    -C-           -C-             E--       copyFileWindowsDefault
    -CS     |    -CS           -CS             E-S       copyFileWindowsSparse
    E--     |    E--           E--             E--       copyFileWindowsDefault
    E-S     |    E-- (NOK)     E-- (NOK)       E-- (NOK) copyFileWindowsDefault -> may fail with ERROR_DISK_FULL!!

tf  := target folder
E   := encrypted
C   := compressed
S   := sparse
NOK := current behavior is not optimal/OK yet.

Note: - if target parent folder is compressed or encrypted, both attributes are added automatically during file creation!
      - "compressed" and "encrypted" are mutually exclusive: http://support.microsoft.com/kb/223093/en-us
*/


//due to issues on non-NTFS volumes, we should use the copy-as-sparse routine only if required and supported!
bool canCopyAsSparse(DWORD fileAttrSource, const Zstring& targetFile) //throw ()
{
    const bool sourceIsEncrypted = (fileAttrSource & FILE_ATTRIBUTE_ENCRYPTED)   != 0;
    const bool sourceIsSparse    = (fileAttrSource & FILE_ATTRIBUTE_SPARSE_FILE) != 0;

    if (sourceIsEncrypted || !sourceIsSparse) //BackupRead() silently fails reading encrypted files!
        return false; //small perf optimization: don't check "targetFile" if not needed

    //------------------------------------------------------------------------------------
    const DWORD bufferSize = 10000;
    std::vector<wchar_t> buffer(bufferSize);

    //full pathName need not yet exist!
    if (!::GetVolumePathName(targetFile.c_str(), //__in   LPCTSTR lpszFileName,
                             &buffer[0],         //__out  LPTSTR lpszVolumePathName,
                             bufferSize))        //__in   DWORD cchBufferLength
        return false;

    DWORD fsFlagsTarget = 0;
    if (!::GetVolumeInformation(&buffer[0],     //__in_opt   LPCTSTR lpRootPathName
                                nullptr,        //__out_opt  LPTSTR lpVolumeNameBuffer,
                                0,              //__in       DWORD nVolumeNameSize,
                                nullptr,        //__out_opt  LPDWORD lpVolumeSerialNumber,
                                nullptr,        //__out_opt  LPDWORD lpMaximumComponentLength,
                                &fsFlagsTarget, //__out_opt  LPDWORD lpFileSystemFlags,
                                nullptr,        //__out      LPTSTR lpFileSystemNameBuffer,
                                0))             //__in       DWORD nFileSystemNameSize
        return false;

    const bool targetSupportSparse = (fsFlagsTarget & FILE_SUPPORTS_SPARSE_FILES) != 0;

    return targetSupportSparse;
    //both source and target must not be FAT since copyFileWindowsSparse() does no DST hack! implicitly guaranteed at this point!
}


bool canCopyAsSparse(const Zstring& sourceFile, const Zstring& targetFile) //throw ()
{
    //follow symlinks!
    HANDLE hSource = ::CreateFile(applyLongPathPrefix(sourceFile).c_str(),
                                  0,
                                  FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, //all shared modes are required to read files that are open in other applications
                                  nullptr,
                                  OPEN_EXISTING,
                                  0,
                                  nullptr);
    if (hSource == INVALID_HANDLE_VALUE)
        return false;
    ZEN_ON_SCOPE_EXIT(::CloseHandle(hSource));

    BY_HANDLE_FILE_INFORMATION fileInfoSource = {};
    if (!::GetFileInformationByHandle(hSource, &fileInfoSource))
        return false;

    return canCopyAsSparse(fileInfoSource.dwFileAttributes, targetFile); //throw ()
}


//precondition: canCopyAsSparse() must return "true"!
void copyFileWindowsSparse(const Zstring& sourceFile,
                           const Zstring& targetFile,
                           CallbackCopyFile* callback,
                           FileAttrib* newAttrib) //throw FileError, ErrorTargetPathMissing, ErrorTargetExisting, ErrorFileLocked
{
    assert(canCopyAsSparse(sourceFile, targetFile));

    //try to get backup read and write privileges: who knows, maybe this helps solve some obscure "access denied" errors
    try { activatePrivilege(SE_BACKUP_NAME); }
    catch (const FileError&) {}
    try { activatePrivilege(SE_RESTORE_NAME); }
    catch (const FileError&) {}

    //open sourceFile for reading
    HANDLE hFileSource = ::CreateFile(applyLongPathPrefix(sourceFile).c_str(),
                                      GENERIC_READ,
                                      FILE_SHARE_READ | FILE_SHARE_DELETE,
                                      nullptr,
                                      OPEN_EXISTING, //FILE_FLAG_OVERLAPPED must not be used!
                                      FILE_FLAG_SEQUENTIAL_SCAN | FILE_FLAG_BACKUP_SEMANTICS, //FILE_FLAG_NO_BUFFERING should not be used!
                                      nullptr);
    if (hFileSource == INVALID_HANDLE_VALUE)
    {
        const DWORD lastError = ::GetLastError();

        const std::wstring shortMsg = replaceCpy(_("Cannot read file %x."), L"%x", fmtFileName(sourceFile));

        //if file is locked throw "ErrorFileLocked" instead!
        if (lastError == ERROR_SHARING_VIOLATION ||
            lastError == ERROR_LOCK_VIOLATION)
        {
            const Zstring procList = getLockingProcessNames(sourceFile); //throw()
            throw ErrorFileLocked(shortMsg + L"\n\n" + (!procList.empty() ? _("The file is locked by another process:") + L"\n" + procList : getLastErrorFormatted(lastError)));
        }

        throw FileError(shortMsg + L"\n\n" + getLastErrorFormatted(lastError) + L" (open)");
    }
    ZEN_ON_SCOPE_EXIT(::CloseHandle(hFileSource));

    //----------------------------------------------------------------------
    BY_HANDLE_FILE_INFORMATION fileInfoSource = {};
    if (!::GetFileInformationByHandle(hFileSource, &fileInfoSource))
        throw FileError(replaceCpy(_("Cannot read file attributes of %x."), L"%x", fmtFileName(sourceFile)) + L"\n\n" + getLastErrorFormatted());

    //----------------------------------------------------------------------
    const DWORD validAttribs = FILE_ATTRIBUTE_NORMAL   | //"This attribute is valid only if used alone."
                               FILE_ATTRIBUTE_READONLY |
                               FILE_ATTRIBUTE_HIDDEN   |
                               FILE_ATTRIBUTE_SYSTEM   |
                               FILE_ATTRIBUTE_ARCHIVE  |           //those two are not set properly (not worse than ::CopyFileEx())
                               FILE_ATTRIBUTE_NOT_CONTENT_INDEXED; //
    //FILE_ATTRIBUTE_ENCRYPTED -> no!

    //create targetFile and open it for writing
    HANDLE hFileTarget = ::CreateFile(applyLongPathPrefix(targetFile).c_str(),
                                      GENERIC_READ | GENERIC_WRITE, //read access required for FSCTL_SET_COMPRESSION
                                      FILE_SHARE_DELETE, //FILE_SHARE_DELETE is required to rename file while handle is open!
                                      nullptr,
                                      CREATE_NEW,
                                      FILE_FLAG_SEQUENTIAL_SCAN | FILE_FLAG_BACKUP_SEMANTICS | (fileInfoSource.dwFileAttributes & validAttribs),
                                      //FILE_FLAG_OVERLAPPED must not be used! FILE_FLAG_NO_BUFFERING should not be used!
                                      nullptr);
    if (hFileTarget == INVALID_HANDLE_VALUE)
    {
        const DWORD lastError = ::GetLastError();
        const std::wstring errorMessage = replaceCpy(_("Cannot write file %x."), L"%x", fmtFileName(targetFile)) + L"\n\n" + getLastErrorFormatted(lastError) + L" (open)";

        if (lastError == ERROR_FILE_EXISTS || //confirmed to be used
            lastError == ERROR_ALREADY_EXISTS) //comment on msdn claims, this one is used on Windows Mobile 6
            throw ErrorTargetExisting(errorMessage);

        if (lastError == ERROR_PATH_NOT_FOUND)
            throw ErrorTargetPathMissing(errorMessage);

        throw FileError(errorMessage);
    }
    ScopeGuard guardTarget = makeGuard([&] { try { removeFile(targetFile); } catch (...) {} }); //transactional behavior: guard just after opening target and before managing hFileOut
    ZEN_ON_SCOPE_EXIT(::CloseHandle(hFileTarget));

    //----------------------------------------------------------------------
    BY_HANDLE_FILE_INFORMATION fileInfoTarget = {};
    if (!::GetFileInformationByHandle(hFileTarget, &fileInfoTarget))
        throw FileError(replaceCpy(_("Cannot read file attributes of %x."), L"%x", fmtFileName(targetFile)) + L"\n\n" + getLastErrorFormatted());

    //return up-to-date file attributes
    if (newAttrib)
    {
        newAttrib->fileSize         = UInt64(fileInfoSource.nFileSizeLow, fileInfoSource.nFileSizeHigh);
        newAttrib->modificationTime = toTimeT(fileInfoSource.ftLastWriteTime); //no DST hack (yet)
        newAttrib->sourceFileId     = extractFileID(fileInfoSource);
        newAttrib->targetFileId     = extractFileID(fileInfoTarget);
    }

    //#################### copy NTFS compressed attribute #########################
    const bool sourceIsCompressed = (fileInfoSource.dwFileAttributes & FILE_ATTRIBUTE_COMPRESSED) != 0;
    const bool targetIsCompressed = (fileInfoTarget.dwFileAttributes & FILE_ATTRIBUTE_COMPRESSED) != 0; //already set by CreateFile if target parent folder is compressed!
    if (sourceIsCompressed && !targetIsCompressed)
    {
        USHORT cmpState = COMPRESSION_FORMAT_DEFAULT;
        DWORD bytesReturned = 0;
        if (!::DeviceIoControl(hFileTarget,           //handle to file or directory
                               FSCTL_SET_COMPRESSION, //dwIoControlCode
                               &cmpState,             //input buffer
                               sizeof(cmpState),      //size of input buffer
                               nullptr,               //lpOutBuffer
                               0,                     //OutBufferSize
                               &bytesReturned,        //number of bytes returned
                               nullptr))              //OVERLAPPED structure
        {} //may legitimately fail with ERROR_INVALID_FUNCTION if:
        // - target folder is encrypted
        // - target volume does not support compressed attribute -> unlikely in this context
    }
    //#############################################################################

    //although it seems the sparse attribute is set automatically by BackupWrite, we are required to do this manually: http://support.microsoft.com/kb/271398/en-us
    //Quote: It is the responsibility of the backup utility to apply file attributes to a file after it is restored by using BackupWrite.
    //The application should retrieve the attributes by using GetFileAttributes prior to creating a backup with BackupRead.
    //If a file originally had the sparse attribute (FILE_ATTRIBUTE_SPARSE_FILE), the backup utility must explicitly set the
    //attribute on the restored file.

    //if (sourceIsSparse && targetSupportsSparse) -> no need to check, this is our precondition!
    {
        DWORD bytesReturned = 0;
        if (!::DeviceIoControl(hFileTarget,      //handle to file
                               FSCTL_SET_SPARSE, //dwIoControlCode
                               nullptr,          //input buffer
                               0,                //size of input buffer
                               nullptr,          //lpOutBuffer
                               0,                //OutBufferSize
                               &bytesReturned,   //number of bytes returned
                               nullptr))         //OVERLAPPED structure
            throw FileError(replaceCpy(_("Cannot write file attributes of %x."), L"%x", fmtFileName(targetFile)) +
                            L"\n\n" + zen::getLastErrorFormatted() + L" (NTFS sparse)");
    }

    //----------------------------------------------------------------------
    const DWORD BUFFER_SIZE = 512 * 1024; //512 kb seems to be a reasonable buffer size - must be greater than sizeof(WIN32_STREAM_ID)
    static boost::thread_specific_ptr<std::vector<BYTE>> cpyBuf;
    if (!cpyBuf.get())
        cpyBuf.reset(new std::vector<BYTE>(BUFFER_SIZE));
    std::vector<BYTE>& buffer = *cpyBuf;

    LPVOID contextRead  = nullptr; //manage context for BackupRead()/BackupWrite()
    LPVOID contextWrite = nullptr; //

    ZEN_ON_SCOPE_EXIT(
        if (contextRead ) ::BackupRead (0, nullptr, 0, nullptr, true, false, &contextRead); //lpContext must be passed [...] all other parameters are ignored.
        if (contextWrite) ::BackupWrite(0, nullptr, 0, nullptr, true, false, &contextWrite); );

    //stream-copy sourceFile to targetFile
    bool eof = false;
    bool silentFailure = true; //try to detect failure reading encrypted files
    do
    {
        DWORD bytesRead = 0;
        if (!::BackupRead(hFileSource,   //__in   HANDLE hFile,
                          &buffer[0],    //__out  LPBYTE lpBuffer,
                          BUFFER_SIZE,   //__in   DWORD nNumberOfBytesToRead,
                          &bytesRead,    //__out  LPDWORD lpNumberOfBytesRead,
                          false,         //__in   BOOL bAbort,
                          false,         //__in   BOOL bProcessSecurity,
                          &contextRead)) //__out  LPVOID *lpContext
            throw FileError(replaceCpy(_("Cannot read file %x."), L"%x", fmtFileName(sourceFile)) + L"\n\n" + getLastErrorFormatted()); //better use fine-granular error messages "reading/writing"!

        if (bytesRead > BUFFER_SIZE)
            throw FileError(replaceCpy(_("Cannot read file %x."), L"%x", fmtFileName(sourceFile)) + L"\n\n" + L"(buffer overflow)");

        if (bytesRead < BUFFER_SIZE)
            eof = true;

        DWORD bytesWritten = 0;
        if (!::BackupWrite(hFileTarget,    //__in   HANDLE hFile,
                           &buffer[0],     //__in   LPBYTE lpBuffer,
                           bytesRead,      //__in   DWORD nNumberOfBytesToWrite,
                           &bytesWritten,  //__out  LPDWORD lpNumberOfBytesWritten,
                           false,          //__in   BOOL bAbort,
                           false,          //__in   BOOL bProcessSecurity,
                           &contextWrite)) //__out  LPVOID *lpContext
            throw FileError(replaceCpy(_("Cannot write file %x."), L"%x", fmtFileName(targetFile)) + L"\n\n" + getLastErrorFormatted());

        if (bytesWritten != bytesRead)
            throw FileError(replaceCpy(_("Cannot write file %x."), L"%x", fmtFileName(targetFile)) + L"\n\n" + L"(incomplete write)");

        //total bytes transferred may be larger than file size! context information + ADS or smaller (sparse, compressed)!

        //invoke callback method to update progress indicators
        if (callback)
            callback->updateCopyStatus(Int64(bytesRead)); //throw X!

        if (bytesRead > 0)
            silentFailure = false;
    }
    while (!eof);

    //DST hack not required, since both source and target volumes cannot be FAT!

    //::BackupRead() silently fails reading encrypted files -> double check!
    if (silentFailure && UInt64(fileInfoSource.nFileSizeLow, fileInfoSource.nFileSizeHigh) != 0U)
        //note: there is no guaranteed ordering relation beween bytes transferred and file size! Consider ADS (>) and compressed/sparse files (<)!
        throw FileError(replaceCpy(_("Cannot read file %x."), L"%x", fmtFileName(sourceFile)) + L"\n\n" + L"(unknown error)");

    //time needs to be set at the end: BackupWrite() changes modification time
    if (!::SetFileTime(hFileTarget,
                       &fileInfoSource.ftCreationTime,
                       nullptr,
                       &fileInfoSource.ftLastWriteTime))
        throw FileError(replaceCpy(_("Cannot write modification time of %x."), L"%x", fmtFileName(targetFile)) + L"\n\n" + getLastErrorFormatted());

    guardTarget.dismiss();

    /*
        //create sparse file for testing:
        HANDLE hSparse = ::CreateFile(L"C:\\sparse.file",
                                      GENERIC_READ | GENERIC_WRITE, //read access required for FSCTL_SET_COMPRESSION
                                      FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                                      nullptr,
                                      CREATE_NEW,
                                      FILE_FLAG_SEQUENTIAL_SCAN,
                                      nullptr);
        if (hFileTarget == INVALID_HANDLE_VALUE)
            throw 1;
        ZEN_ON_SCOPE_EXIT(::CloseHandle(hSparse));

        DWORD br = 0;
        if (!::DeviceIoControl(hSparse, FSCTL_SET_SPARSE, nullptr, 0, nullptr, 0, &br,nullptr))
            throw 1;

        LARGE_INTEGER liDistanceToMove =  {};
        liDistanceToMove.QuadPart = 1024 * 1024 * 1024; //create 5 TB sparse file
        liDistanceToMove.QuadPart *= 5 * 1024;          //maximum file size on NTFS: 16 TB - 64 kB
        if (!::SetFilePointerEx(hSparse, liDistanceToMove, nullptr, FILE_BEGIN))
            throw 1;

        if (!SetEndOfFile(hSparse))
            throw 1;

        FILE_ZERO_DATA_INFORMATION zeroInfo = {};
        zeroInfo.BeyondFinalZero.QuadPart = liDistanceToMove.QuadPart;
        if (!::DeviceIoControl(hSparse, FSCTL_SET_ZERO_DATA, &zeroInfo, sizeof(zeroInfo), nullptr, 0, &br, nullptr))
            throw 1;
    */
}


DEFINE_NEW_FILE_ERROR(ErrorShouldCopyAsSparse);

class ErrorHandling
{
public:
    ErrorHandling() : shouldCopyAsSparse(false), exceptionInUserCallback(nullptr) {}

    //call context: copyCallbackInternal()
    void reportErrorShouldCopyAsSparse() { shouldCopyAsSparse = true; }

    void reportUserException(CallbackCopyFile& userCallback) { exceptionInUserCallback = &userCallback; }

    void reportError(const std::wstring& message) { errorMsg = message; }

    //call context: copyFileWindowsDefault()
    void evaluateErrors() //throw X
    {
        if (shouldCopyAsSparse)
            throw ErrorShouldCopyAsSparse(L"sparse dummy value");

        if (exceptionInUserCallback)
            exceptionInUserCallback->updateCopyStatus(0); //rethrow (hopefully!)

        if (!errorMsg.empty())
            throw FileError(errorMsg);
    }

private:
    bool shouldCopyAsSparse;                   //
    std::wstring errorMsg;                     //these are exclusive!
    CallbackCopyFile* exceptionInUserCallback; //
};


struct CallbackData
{
    CallbackData(CallbackCopyFile* cb, //may be nullptr
                 const Zstring& sourceFile,
                 const Zstring& targetFile) :
        sourceFile_(sourceFile),
        targetFile_(targetFile),
        userCallback(cb) {}

    const Zstring& sourceFile_;
    const Zstring& targetFile_;

    CallbackCopyFile* const userCallback; //optional!
    ErrorHandling errorHandler;
    FileAttrib newAttrib; //modified by CopyFileEx() at beginning

    Int64 bytesReported; //used internally to calculate bytes transferred delta
};


DWORD CALLBACK copyCallbackInternal(LARGE_INTEGER totalFileSize,
                                    LARGE_INTEGER totalBytesTransferred,
                                    LARGE_INTEGER streamSize,
                                    LARGE_INTEGER streamBytesTransferred,
                                    DWORD dwStreamNumber,
                                    DWORD dwCallbackReason,
                                    HANDLE hSourceFile,
                                    HANDLE hDestinationFile,
                                    LPVOID lpData)
{
    /*
    this callback is invoked for block sizes managed by Windows, these may vary from e.g. 64 kB up to 1MB. It seems this depends on file size amongst others.

    symlink handling:
        if source is a symlink and COPY_FILE_COPY_SYMLINK is     specified, this callback is NOT invoked!
        if source is a symlink and COPY_FILE_COPY_SYMLINK is NOT specified, this callback is called and hSourceFile is a handle to the *target* of the link!

    file time handling:
        ::CopyFileEx() will copy file modification time (only) over from source file AFTER the last invokation of this callback
        => it is possible to adapt file creation time of target in here, but NOT file modification time!
    	CAVEAT: if ::CopyFileEx() fails to set modification time, it silently ignores this error and returns success!!!
    	see procmon log in: https://sourceforge.net/tracker/?func=detail&atid=1093080&aid=3514569&group_id=234430

    alternate data stream handling:
        CopyFileEx() processes multiple streams one after another, stream 1 is the file data stream and always available!
        Each stream is initialized with CALLBACK_STREAM_SWITCH and provides *new* hSourceFile, hDestinationFile.
        Calling GetFileInformationByHandle() on hDestinationFile for stream > 1 results in ERROR_ACCESS_DENIED!
    */

    CallbackData& cbd = *static_cast<CallbackData*>(lpData);

    if (dwCallbackReason == CALLBACK_STREAM_SWITCH &&  //called up-front for every file (even if 0-sized)
        dwStreamNumber == 1) //consider ADS!
    {
        //#################### return source file attributes ################################
        BY_HANDLE_FILE_INFORMATION fileInfoSrc = {};
        if (!::GetFileInformationByHandle(hSourceFile, &fileInfoSrc))
        {
            cbd.errorHandler.reportError(replaceCpy(_("Cannot read file attributes of %x."), L"%x", fmtFileName(cbd.sourceFile_)) + L"\n\n" + getLastErrorFormatted());
            return PROGRESS_CANCEL;
        }

        BY_HANDLE_FILE_INFORMATION fileInfoTrg = {};
        if (!::GetFileInformationByHandle(hDestinationFile, &fileInfoTrg))
        {
            cbd.errorHandler.reportError(replaceCpy(_("Cannot read file attributes of %x."), L"%x", fmtFileName(cbd.targetFile_)) + L"\n\n" + getLastErrorFormatted());
            return PROGRESS_CANCEL;
        }

        cbd.newAttrib.fileSize         = UInt64(fileInfoSrc.nFileSizeLow, fileInfoSrc.nFileSizeHigh);
        cbd.newAttrib.modificationTime = toTimeT(fileInfoSrc.ftLastWriteTime); //no DST hack (yet)
        cbd.newAttrib.sourceFileId     = extractFileID(fileInfoSrc);
        cbd.newAttrib.targetFileId     = extractFileID(fileInfoTrg);

        //#################### switch to sparse file copy if req. #######################
        if (canCopyAsSparse(fileInfoSrc.dwFileAttributes, cbd.targetFile_)) //throw ()
        {
            cbd.errorHandler.reportErrorShouldCopyAsSparse(); //use a different copy routine!
            return PROGRESS_CANCEL;
        }

        //#################### copy file creation time ################################
        ::SetFileTime(hDestinationFile, &fileInfoSrc.ftCreationTime, nullptr, nullptr); //no error handling!

        //#################### copy NTFS compressed attribute #########################
        const bool sourceIsCompressed = (fileInfoSrc.dwFileAttributes & FILE_ATTRIBUTE_COMPRESSED) != 0;
        const bool targetIsCompressed = (fileInfoTrg.dwFileAttributes & FILE_ATTRIBUTE_COMPRESSED) != 0; //already set by CopyFileEx if target parent folder is compressed!
        if (sourceIsCompressed && !targetIsCompressed)
        {
            USHORT cmpState = COMPRESSION_FORMAT_DEFAULT;
            DWORD bytesReturned = 0;
            if (!::DeviceIoControl(hDestinationFile,      //handle to file or directory
                                   FSCTL_SET_COMPRESSION, //dwIoControlCode
                                   &cmpState,             //input buffer
                                   sizeof(cmpState),      //size of input buffer
                                   nullptr,               //lpOutBuffer
                                   0,                     //OutBufferSize
                                   &bytesReturned,        //number of bytes returned
                                   nullptr))              //OVERLAPPED structure
            {} //may legitimately fail with ERROR_INVALID_FUNCTION if
            // - if target folder is encrypted
            // - target volume does not support compressed attribute
            //#############################################################################
        }
    }

    //called after copy operation is finished - note: for 0-sized files this callback is invoked just ONCE!
    //if (totalFileSize.QuadPart == totalBytesTransferred.QuadPart && dwStreamNumber == 1) {}
    if (cbd.userCallback)
    {
        //some odd check for some possible(?) error condition
        if (totalBytesTransferred.QuadPart < 0) //let's see if someone answers the call...
            ::MessageBox(nullptr, L"You've just discovered a bug in WIN32 API function \"CopyFileEx\"! \n\n\
            Please write a mail to the author of FreeFileSync at zhnmju123@gmx.de and simply state that\n\
            \"totalBytesTransferred.HighPart can be below zero\"!\n\n\
            This will then be handled in future versions of FreeFileSync.\n\nThanks -ZenJu",
                         nullptr, 0);
        try
        {
            cbd.userCallback->updateCopyStatus(totalBytesTransferred.QuadPart - cbd.bytesReported); //throw X!
            cbd.bytesReported = totalBytesTransferred.QuadPart;
        }
        catch (...)
        {
            //#warning migrate to std::exception_ptr when available

            cbd.errorHandler.reportUserException(*cbd.userCallback);
            return PROGRESS_CANCEL;
        }
    }
    return PROGRESS_CONTINUE;
}


const bool supportNonEncryptedDestination = winXpOrLater(); //encrypted destination is not supported with Windows 2000
//const bool supportUnbufferedCopy          = vistaOrLater();
//caveat: function scope static initialization is not thread-safe in VS 2010!


void copyFileWindowsDefault(const Zstring& sourceFile,
                            const Zstring& targetFile,
                            CallbackCopyFile* callback,
                            FileAttrib* newAttrib) //throw FileError, ErrorTargetPathMissing, ErrorTargetExisting, ErrorFileLocked, ErrorShouldCopyAsSparse
{
    //try to get backup read and write privileges: who knows, maybe this helps solve some obscure "access denied" errors
    try { activatePrivilege(SE_BACKUP_NAME); }
    catch (const FileError&) {}
    try { activatePrivilege(SE_RESTORE_NAME); }
    catch (const FileError&) {}

    zen::ScopeGuard guardTarget = zen::makeGuard([&] { try { removeFile(targetFile); } catch (...) {} });
    //transactional behavior: guard just before starting copy, we don't trust ::CopyFileEx(), do we? ;)

    DWORD copyFlags = COPY_FILE_FAIL_IF_EXISTS;

    if (supportNonEncryptedDestination)
        copyFlags |= COPY_FILE_ALLOW_DECRYPTED_DESTINATION; //allow copying from encrypted to non-encrytped location

    //if (supportUnbufferedCopy) //see http://blogs.technet.com/b/askperf/archive/2007/05/08/slow-large-file-copy-issues.aspx
    //  copyFlags |= COPY_FILE_NO_BUFFERING; //no perf difference at worst, huge improvement for large files (20% in test NTFS -> NTFS)
    //It's a shame this flag causes file corruption! https://sourceforge.net/tracker/?func=detail&atid=1093080&aid=3529683&group_id=234430

    CallbackData cbd(callback, sourceFile, targetFile);

    const bool success = ::CopyFileEx( //same performance like CopyFile()
                             applyLongPathPrefix(sourceFile).c_str(), //__in      LPCTSTR lpExistingFileName,
                             applyLongPathPrefix(targetFile).c_str(), //__in      LPCTSTR lpNewFileName,
                             copyCallbackInternal, //__in_opt  LPPROGRESS_ROUTINE lpProgressRoutine,
                             &cbd,                 //__in_opt  LPVOID lpData,
                             nullptr,              //__in_opt  LPBOOL pbCancel,
                             copyFlags) != FALSE;  //__in      DWORD dwCopyFlags

    cbd.errorHandler.evaluateErrors(); //throw ?, process errors in callback first!
    if (!success)
    {
        const DWORD lastError = ::GetLastError();

        //don't suppress "lastError == ERROR_REQUEST_ABORTED": a user aborted operation IS an error condition!

        //trying to copy huge sparse files may fail with ERROR_DISK_FULL
        if (canCopyAsSparse(sourceFile, targetFile)) //throw ()
            throw ErrorShouldCopyAsSparse(L"sparse dummy value2");

        //assemble error message...
        std::wstring errorMessage = replaceCpy(replaceCpy(_("Cannot copy file %x to %y."), L"%x", fmtFileName(sourceFile)), L"%y", fmtFileName(targetFile)) +
                                    L"\n\n" + getLastErrorFormatted(lastError);

        //if file is locked throw "ErrorFileLocked" instead!
        if (lastError == ERROR_SHARING_VIOLATION ||
            lastError == ERROR_LOCK_VIOLATION)
        {
            const Zstring procList = getLockingProcessNames(sourceFile); //throw() -> enhance error message!
            throw ErrorFileLocked(replaceCpy(_("Cannot read file %x."), L"%x", fmtFileName(sourceFile)) + L"\n\n" +
                                  (!procList.empty() ? _("The file is locked by another process:") + L"\n" + procList : getLastErrorFormatted(lastError)));
        }

        //if target is existing this functions is expected to throw ErrorTargetExisting!!!
        if (lastError == ERROR_FILE_EXISTS || //confirmed to be used
            lastError == ERROR_ALREADY_EXISTS) //not sure if used -> better be safe than sorry!!!
        {
            guardTarget.dismiss(); //don't delete file that existed previously!
            throw ErrorTargetExisting(errorMessage);
        }

        if (lastError == ERROR_PATH_NOT_FOUND)
        {
            guardTarget.dismiss(); //not relevant
            throw ErrorTargetPathMissing(errorMessage);
        }

        try //add more meaningful message
        {
            //trying to copy > 4GB file to FAT/FAT32 volume gives obscure ERROR_INVALID_PARAMETER (FAT can indeed handle files up to 4 Gig, tested!)
            if (lastError == ERROR_INVALID_PARAMETER &&
                dst::isFatDrive(targetFile) &&
                getFilesize(sourceFile) >= 4U * UInt64(1024U * 1024 * 1024)) //throw FileError
                errorMessage += L"\nFAT volume cannot store files larger than 4 gigabyte!";

            //note: ERROR_INVALID_PARAMETER can also occur when copying to a SharePoint server or MS SkyDrive and the target filename is of a restricted type.
        }
        catch (...) {}

        throw FileError(errorMessage);
    }

    if (newAttrib)
        *newAttrib = cbd.newAttrib;

    {
        //DST hack
        const Int64 modTime = getFileTime(sourceFile, SYMLINK_FOLLOW); //throw FileError
        setFileTime(targetFile, modTime, SYMLINK_FOLLOW); //throw FileError

        if (newAttrib)
            newAttrib->modificationTime = modTime;

        //note: this sequence leads to a loss of precision of up to 1 sec!
        //perf-loss on USB sticks with many small files of about 30%! damn!
    }

    guardTarget.dismiss(); //target has been created successfully!
}


//another layer to support copying sparse files
inline
void copyFileWindowsSelectRoutine(const Zstring& sourceFile, const Zstring& targetFile, CallbackCopyFile* callback, FileAttrib* sourceAttr)
{
    try
    {
        copyFileWindowsDefault(sourceFile, targetFile, callback, sourceAttr); //throw ErrorShouldCopyAsSparse et al.
    }
    catch (ErrorShouldCopyAsSparse&) //we cheaply check for this condition within callback of ::CopyFileEx()!
    {
        copyFileWindowsSparse(sourceFile, targetFile, callback, sourceAttr);
    }
}


//another layer of indirection solving 8.3 name clashes
inline
void copyFileWindows(const Zstring& sourceFile, const Zstring& targetFile, CallbackCopyFile* callback, FileAttrib* sourceAttr)
{
    try
    {
        copyFileWindowsSelectRoutine(sourceFile, targetFile, callback, sourceAttr); //throw FileError, ErrorTargetPathMissing, ErrorTargetExisting, ErrorFileLocked
    }
    catch (const ErrorTargetExisting&)
    {
        //try to handle issues with already existing short 8.3 file names on Windows
        if (have8dot3NameClash(targetFile))
        {
            Fix8Dot3NameClash dummy(targetFile); //move clashing filename to the side
            copyFileWindowsSelectRoutine(sourceFile, targetFile, callback, sourceAttr); //throw FileError; the short filename name clash is solved, this should work now
            return;
        }
        throw;
    }
}

#elif defined FFS_LINUX
void copyFileLinux(const Zstring& sourceFile,
                   const Zstring& targetFile,
                   CallbackCopyFile* callback,
                   FileAttrib* newAttrib) //throw FileError, ErrorTargetPathMissing, ErrorTargetExisting
{
    zen::ScopeGuard guardTarget = zen::makeGuard([&] { try { removeFile(targetFile); } catch (...) {} }); //transactional behavior: place guard before lifetime of FileOutput
    try
    {
        //open sourceFile for reading
        FileInput fileIn(sourceFile); //throw FileError

        //create targetFile and open it for writing
        FileOutput fileOut(targetFile, FileOutput::ACC_CREATE_NEW); //throw FileError, ErrorTargetPathMissing, ErrorTargetExisting

        std::vector<char>& buffer = []() -> std::vector<char>&
        {
            static boost::thread_specific_ptr<std::vector<char>> cpyBuf;
            if (!cpyBuf.get())
                cpyBuf.reset(new std::vector<char>(512 * 1024)); //512 kb seems to be a reasonable buffer size
            return *cpyBuf;
        }();

        //copy contents of sourceFile to targetFile
        do
        {
            const size_t bytesRead = fileIn.read(&buffer[0], buffer.size()); //throw FileError

            fileOut.write(&buffer[0], bytesRead); //throw FileError

            //invoke callback method to update progress indicators
            if (callback)
                callback->updateCopyStatus(Int64(bytesRead)); //throw X!
        }
        while (!fileIn.eof());
    }
    catch (ErrorTargetExisting&)
    {
        guardTarget.dismiss(); //don't delete file that existed previously!
        throw;
    }

    //adapt file modification time:
    {
        struct ::stat srcInfo = {};
        if (::stat(sourceFile.c_str(), &srcInfo) != 0) //read file attributes from source directory
            throw FileError(replaceCpy(_("Cannot read file attributes of %x."), L"%x", fmtFileName(sourceFile)) + L"\n\n" + getLastErrorFormatted());

        struct ::utimbuf newTimes = {};
        newTimes.actime  = srcInfo.st_atime;
        newTimes.modtime = srcInfo.st_mtime;

        //set new "last write time"
        if (::utime(targetFile.c_str(), &newTimes) != 0)
            throw FileError(replaceCpy(_("Cannot write modification time of %x."), L"%x", fmtFileName(targetFile)) + L"\n\n" + getLastErrorFormatted());

        if (newAttrib)
        {
            struct ::stat trgInfo = {};
            if (::stat(targetFile.c_str(), &trgInfo) != 0) //read file attributes from source directory
                throw FileError(replaceCpy(_("Cannot read file attributes of %x."), L"%x", fmtFileName(targetFile)) + L"\n\n" + getLastErrorFormatted());

            newAttrib->fileSize         = UInt64(srcInfo.st_size);
            newAttrib->modificationTime = srcInfo.st_mtime;
            newAttrib->sourceFileId     = extractFileID(srcInfo);
            newAttrib->targetFileId     = extractFileID(trgInfo);
        }
    }

    guardTarget.dismiss(); //target has been created successfully!
}
#endif


Zstring findUnusedTempName(const Zstring& filename)
{
    Zstring output = filename + zen::TEMP_FILE_ENDING;

    //ensure uniqueness (+ minor file system race condition!)
    for (int i = 1; somethingExists(output); ++i)
        output = filename + Zchar('_') + numberTo<Zstring>(i) + zen::TEMP_FILE_ENDING;

    return output;
}


/*
      File Copy Layers
      ================

         copyFile (setup transactional behavior)
             |
      copyFileSelectOs
      /               \
copyFileLinux  copyFileWindows (solve 8.3 issue)
                       |
			  copyFileWindowsSelectRoutine
	          /                           \
copyFileWindowsDefault(::CopyFileEx)  copyFileWindowsSparse(::BackupRead/::BackupWrite)
*/

inline
void copyFileSelectOs(const Zstring& sourceFile, const Zstring& targetFile, CallbackCopyFile* callback, FileAttrib* sourceAttr)
{
#ifdef FFS_WIN
    copyFileWindows(sourceFile, targetFile, callback, sourceAttr); //throw FileError, ErrorTargetPathMissing, ErrorTargetExisting, ErrorFileLocked

#elif defined FFS_LINUX
    copyFileLinux(sourceFile, targetFile, callback, sourceAttr); //throw FileError, ErrorTargetPathMissing, ErrorTargetExisting
#endif
}
}


void zen::copyFile(const Zstring& sourceFile, //throw FileError, ErrorTargetPathMissing, ErrorFileLocked
                   const Zstring& targetFile,
                   bool copyFilePermissions,
                   bool transactionalCopy,
                   CallbackCopyFile* callback,
                   FileAttrib* sourceAttr)
{
    if (transactionalCopy)
    {
        Zstring temporary = targetFile + zen::TEMP_FILE_ENDING; //use temporary file until a correct date has been set
        zen::ScopeGuard guardTempFile = zen::makeGuard([&] { try { removeFile(temporary); } catch (...) {} }); //transactional behavior: ensure cleanup (e.g. network drop) -> ref to temporary[!]

        //raw file copy
        try
        {
            copyFileSelectOs(sourceFile, temporary, callback, sourceAttr); //throw FileError: ErrorTargetPathMissing, ErrorTargetExisting, ErrorFileLocked
        }
        catch (ErrorTargetExisting&)
        {
            //determine non-used temp file name "first":
            //using optimistic strategy: assume everything goes well, but recover on error -> minimize file accesses
            temporary = findUnusedTempName(targetFile);

            //retry
            copyFileSelectOs(sourceFile, temporary, callback, sourceAttr); //throw FileError
        }

        //have target file deleted (after read access on source and target has been confirmed) => allow for almost transactional overwrite
        if (callback)
            callback->deleteTargetFile(targetFile);

        //rename temporary file:
        //perf: this call is REALLY expensive on unbuffered volumes! ~40% performance decrease on FAT USB stick!
        renameFile(temporary, targetFile); //throw FileError

        guardTempFile.dismiss();
    }
    else
    {
        //have target file deleted
        if (callback) callback->deleteTargetFile(targetFile);

        copyFileSelectOs(sourceFile, targetFile, callback, sourceAttr); //throw FileError: ErrorTargetPathMissing, ErrorTargetExisting, ErrorFileLocked
    }
    /*
       Note: non-transactional file copy solves at least four problems:
        	-> skydrive - doesn't allow for .ffs_tmp extension and returns ERROR_INVALID_PARAMETER
        	-> network renaming issues
        	-> allow for true delete before copy to handle low disk space problems
        	-> higher performance on non-buffered drives (e.g. usb sticks)
    */

    //file permissions
    if (copyFilePermissions)
    {
        zen::ScopeGuard guardTargetFile = zen::makeGuard([&] { try { removeFile(targetFile); } catch (...) {}});

        copyObjectPermissions(sourceFile, targetFile, SYMLINK_FOLLOW); //throw FileError

        guardTargetFile.dismiss(); //target has been created successfully!
    }
}
