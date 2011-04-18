// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) 2008-2011 ZenJu (zhnmju123 AT gmx.de)                    *
// **************************************************************************
//
#include "file_handling.h"
#include <map>
#include <algorithm>
#include <boost/scoped_array.hpp>
#include <boost/bind.hpp>
#include <stdexcept>
#include "system_func.h"
#include "global_func.h"
#include "system_constants.h"
#include "file_traverser.h"
#include "string_conv.h"
#include "loki/TypeManip.h"
#include "loki/ScopeGuard.h"
#include "symlink_target.h"
#include "file_io.h"
#include "i18n.h"

#ifdef FFS_WIN
#include "privilege.h"
#include "dll_loader.h"
#include <wx/msw/wrapwin.h> //includes "windows.h"
#include "long_path_prefix.h"
#include <Aclapi.h>
#include "dst_hack.h"

#elif defined FFS_LINUX
#include <sys/stat.h>
#include <time.h>
#include <utime.h>
#include <cerrno>
#include <sys/time.h>

#ifdef HAVE_SELINUX
#include <selinux/selinux.h>
#endif
#endif

using ffs3::FileError;
using namespace ffs3;


bool ffs3::fileExists(const Zstring& filename)
{
    //symbolic links (broken or not) are also treated as existing files!
#ifdef FFS_WIN
    // we must use GetFileAttributes() instead of the ANSI C functions because
    // it can cope with network (UNC) paths unlike them
    const DWORD ret = ::GetFileAttributes(applyLongPathPrefix(filename).c_str());
    return ret != INVALID_FILE_ATTRIBUTES && !(ret & FILE_ATTRIBUTE_DIRECTORY); //returns true for (file-)symlinks also

#elif defined FFS_LINUX
    struct stat fileInfo;
    return ::lstat(filename.c_str(), &fileInfo) == 0 &&
           (S_ISLNK(fileInfo.st_mode) || S_ISREG(fileInfo.st_mode)); //in Linux a symbolic link is neither file nor directory
#endif
}


bool ffs3::dirExists(const Zstring& dirname)
{
    //symbolic links (broken or not) are also treated as existing directories!
#ifdef FFS_WIN
    // we must use GetFileAttributes() instead of the ANSI C functions because
    // it can cope with network (UNC) paths unlike them
    const DWORD ret = ::GetFileAttributes(applyLongPathPrefix(dirname).c_str());
    return (ret != INVALID_FILE_ATTRIBUTES) && (ret & FILE_ATTRIBUTE_DIRECTORY); //returns true for (dir-)symlinks also

#elif defined FFS_LINUX
    struct stat dirInfo;
    return ::lstat(dirname.c_str(), &dirInfo) == 0 &&
           (S_ISLNK(dirInfo.st_mode) || S_ISDIR(dirInfo.st_mode)); //in Linux a symbolic link is neither file nor directory
#endif
}


bool ffs3::symlinkExists(const Zstring& objname)
{
#ifdef FFS_WIN
    const DWORD ret = ::GetFileAttributes(applyLongPathPrefix(objname).c_str());
    return ret != INVALID_FILE_ATTRIBUTES && (ret & FILE_ATTRIBUTE_REPARSE_POINT);

#elif defined FFS_LINUX
    struct stat fileInfo;
    return ::lstat(objname.c_str(), &fileInfo) == 0 &&
           S_ISLNK(fileInfo.st_mode); //symbolic link
#endif
}


bool ffs3::somethingExists(const Zstring& objname) //throw()       check whether any object with this name exists
{
#ifdef FFS_WIN
    return ::GetFileAttributes(applyLongPathPrefix(objname).c_str()) != INVALID_FILE_ATTRIBUTES;

#elif defined FFS_LINUX
    struct stat fileInfo;
    return ::lstat(objname.c_str(), &fileInfo) == 0;
#endif
}


#ifdef FFS_WIN
namespace
{
wxULongLong getFileSizeSymlink(const Zstring& linkName) //throw (FileError)
{
    //open handle to target of symbolic link
    const HANDLE hFile = ::CreateFile(ffs3::applyLongPathPrefix(linkName).c_str(),
                                      0,
                                      FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                                      NULL,
                                      OPEN_EXISTING,
                                      FILE_FLAG_BACKUP_SEMANTICS,
                                      NULL);
    if (hFile == INVALID_HANDLE_VALUE)
    {
        const wxString errorMessage = wxString(_("Error reading file attributes:")) + wxT("\n\"") + ffs3::zToWx(linkName) + wxT("\"");
        throw FileError(errorMessage + wxT("\n\n") + ffs3::getLastErrorFormatted());
    }
    Loki::ScopeGuard dummy = Loki::MakeGuard(::CloseHandle, hFile);
    (void)dummy; //silence warning "unused variable"

    BY_HANDLE_FILE_INFORMATION fileInfoByHandle = {};
    if (!::GetFileInformationByHandle(hFile, &fileInfoByHandle))
    {
        const wxString errorMessage = wxString(_("Error reading file attributes:")) + wxT("\n\"") + ffs3::zToWx(linkName) + wxT("\"");
        throw FileError(errorMessage + wxT("\n\n") + ffs3::getLastErrorFormatted());
    }

    return wxULongLong(fileInfoByHandle.nFileSizeHigh, fileInfoByHandle.nFileSizeLow);
}
}
#endif


wxULongLong ffs3::getFilesize(const Zstring& filename) //throw (FileError)
{
#ifdef FFS_WIN
    WIN32_FIND_DATA fileMetaData;
    const HANDLE searchHandle = ::FindFirstFile(applyLongPathPrefix(filename).c_str(), &fileMetaData);
    if (searchHandle == INVALID_HANDLE_VALUE)
    {
        const wxString errorMessage = wxString(_("Error reading file attributes:")) + wxT("\n\"") + zToWx(filename) + wxT("\"");
        throw FileError(errorMessage + wxT("\n\n") + ffs3::getLastErrorFormatted());
    }
    ::FindClose(searchHandle);

    const bool isSymbolicLink = (fileMetaData.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT) != 0;
    if (isSymbolicLink)
        return getFileSizeSymlink(filename); //throw (FileError)

    return wxULongLong(fileMetaData.nFileSizeHigh, fileMetaData.nFileSizeLow);

#elif defined FFS_LINUX
    struct stat fileInfo;
    if (::stat(filename.c_str(), &fileInfo) != 0) //follow symbolic links
    {
        const wxString errorMessage = wxString(_("Error reading file attributes:")) + wxT("\n\"") + zToWx(filename) + wxT("\"");
        throw FileError(errorMessage + wxT("\n\n") + ffs3::getLastErrorFormatted());
    }

    return fileInfo.st_size;
#endif
}


namespace
{
#ifdef FFS_WIN
DWORD retrieveVolumeSerial(const Zstring& pathName) //return 0 on error!
{
    const size_t bufferSize = std::max(pathName.size(), static_cast<size_t>(10000));
    boost::scoped_array<wchar_t> buffer(new wchar_t[bufferSize]);

    //full pathName need not yet exist!
    if (!::GetVolumePathName(pathName.c_str(), //__in   LPCTSTR lpszFileName,
                             buffer.get(),     //__out  LPTSTR lpszVolumePathName,
                             static_cast<DWORD>(bufferSize))) //__in   DWORD cchBufferLength
        return 0;

    Zstring volumePath = buffer.get();
    if (!volumePath.EndsWith(common::FILE_NAME_SEPARATOR))
        volumePath += common::FILE_NAME_SEPARATOR;

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
#elif defined FFS_LINUX

dev_t retrieveVolumeSerial(const Zstring& pathName) //return 0 on error!
{
    Zstring volumePathName = pathName;

    //remove trailing slash
    if (volumePathName.size() > 1 && volumePathName.EndsWith(common::FILE_NAME_SEPARATOR))  //exception: allow '/'
        volumePathName = volumePathName.BeforeLast(common::FILE_NAME_SEPARATOR);

    struct stat fileInfo = {};
    while (::lstat(volumePathName.c_str(), &fileInfo) != 0) //go up in folder hierarchy until existing folder is found
    {
        volumePathName = volumePathName.BeforeLast(common::FILE_NAME_SEPARATOR); //returns empty string if ch not found
        if (volumePathName.empty())
            return 0;  //this includes path "/" also!
    }

    return fileInfo.st_dev;
}
#endif
}


ffs3::ResponseSameVol ffs3::onSameVolume(const Zstring& folderLeft, const Zstring& folderRight) //throw()
{
#ifdef FFS_WIN
    typedef DWORD VolSerial;
#elif defined FFS_LINUX
    typedef dev_t VolSerial;
#endif
    const VolSerial serialLeft  = retrieveVolumeSerial(folderLeft);  //returns 0 on error!
    const VolSerial serialRight = retrieveVolumeSerial(folderRight); //returns 0 on error!
    if (serialLeft == 0 || serialRight == 0)
        return VOLUME_CANT_SAY;

    return serialLeft == serialRight ? VOLUME_SAME : VOLUME_DIFFERENT;
}


void ffs3::removeFile(const Zstring& filename) //throw (FileError);
{
#ifdef FFS_WIN
    //remove file, support for \\?\-prefix
    const Zstring filenameFmt = applyLongPathPrefix(filename);
    if (!::DeleteFile(filenameFmt.c_str()))
#elif defined FFS_LINUX
    if (::unlink(filename.c_str()) != 0)
#endif
    {
#ifdef FFS_WIN
        //perf: apply ONLY when necessary!
        if (::GetLastError() == ERROR_ACCESS_DENIED) //function fails if file is read-only
        {
            //(try to) normalize file attributes
            ::SetFileAttributes(filenameFmt.c_str(), FILE_ATTRIBUTE_NORMAL);

            //now try again...
            if (::DeleteFile(filenameFmt.c_str()))
                return;
        }
#endif
        //eval error code before next call
        const wxString errorMessage = wxString(_("Error deleting file:")) + wxT("\n\"") + zToWx(filename) + wxT("\"") + wxT("\n\n") + ffs3::getLastErrorFormatted();

        //no error situation if file is not existing! manual deletion relies on it!
        //perf: place check in error handling block
        //warning: this call changes error code!!
        if (!somethingExists(filename))
            return; //neither file nor any other object (e.g. broken symlink) with that name existing

        throw FileError(errorMessage);
    }
}


namespace
{
DEFINE_NEW_FILE_ERROR(ErrorDifferentVolume);

/* Usage overview:

  renameFile()  -->  renameFileInternal()
      |               /|\
     \|/               |
      fix8Dot3NameClash()
*/
//wrapper for file system rename function:
//throw (FileError); ErrorDifferentVolume if it is due to moving file to another volume
void renameFileInternal(const Zstring& oldName, const Zstring& newName) //throw (FileError: ErrorDifferentVolume, ErrorTargetExisting)
{
    using namespace ffs3; //for zToWx()

#ifdef FFS_WIN
    const Zstring oldNameFmt = applyLongPathPrefix(oldName);
    const Zstring newNameFmt = applyLongPathPrefix(newName);

    if (!::MoveFileEx(oldNameFmt.c_str(), //__in      LPCTSTR lpExistingFileName,
                      newNameFmt.c_str(), //__in_opt  LPCTSTR lpNewFileName,
                      0))                 //__in      DWORD dwFlags
    {
        if (::GetLastError() == ERROR_ACCESS_DENIED) //MoveFileEx may fail to rename a read-only file on a SAMBA-share -> (try to) handle this
        {
            const DWORD oldNameAttrib = ::GetFileAttributes(oldNameFmt.c_str());
            if (oldNameAttrib != INVALID_FILE_ATTRIBUTES)
            {
                if (::SetFileAttributes(oldNameFmt.c_str(), FILE_ATTRIBUTE_NORMAL)) //remove readonly-attribute
                {
                    //try again...
                    if (::MoveFileEx(oldNameFmt.c_str(), //__in      LPCTSTR lpExistingFileName,
                                     newNameFmt.c_str(), //__in_opt  LPCTSTR lpNewFileName,
                                     0))                 //__in      DWORD dwFlags
                    {
                        //(try to) restore file attributes
                        ::SetFileAttributes(newNameFmt.c_str(), oldNameAttrib); //don't handle error
                        return;
                    }
                    else
                    {
                        Loki::ScopeGuard dummy = Loki::MakeGuard(::SetLastError, ::GetLastError()); //use error code from ::MoveFileEx()
                        (void)dummy;

                        //cleanup: (try to) restore file attributes: assume oldName is still existing
                        ::SetFileAttributes(oldNameFmt.c_str(),
                                            oldNameAttrib);
                    }
                }
            }
        }

        const DWORD lastError = ::GetLastError();
        const wxString errorMessage = wxString(_("Error moving file:")) + wxT("\n\"") + zToWx(oldName) +  wxT("\" ->\n\"") + zToWx(newName) + wxT("\"") +
                                      wxT("\n\n") + ffs3::getLastErrorFormatted(lastError);
        if (lastError == ERROR_NOT_SAME_DEVICE)
            throw ErrorDifferentVolume(errorMessage);
        else if (lastError == ERROR_FILE_EXISTS)
            throw ErrorTargetExisting(errorMessage);
        else
            throw FileError(errorMessage);
    }

#elif defined FFS_LINUX
    if (::rename(oldName.c_str(), newName.c_str()) != 0)
    {
        const int lastError = errno;
        const wxString errorMessage = wxString(_("Error moving file:")) + wxT("\n\"") + zToWx(oldName) +  wxT("\" ->\n\"") + zToWx(newName) + wxT("\"") +
                                      wxT("\n\n") + ffs3::getLastErrorFormatted(lastError);
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
    const Zstring filenameFmt = ffs3::applyLongPathPrefix(filename);

    const DWORD bufferSize = fun(filenameFmt.c_str(), NULL, 0);
    if (bufferSize == 0)
        return Zstring();

    boost::scoped_array<wchar_t> buffer(new wchar_t[bufferSize]);

    const DWORD rv = fun(filenameFmt.c_str(), //__in   LPCTSTR lpszShortPath,
                         buffer.get(),        //__out  LPTSTR  lpszLongPath,
                         bufferSize);         //__in   DWORD   cchBuffer
    if (rv == 0 || rv >= bufferSize)
        return Zstring();

    return buffer.get();
}


Zstring createTemp8Dot3Name(const Zstring& fileName) //find a unique 8.3 short name
{
    const Zstring pathPrefix = fileName.find(common::FILE_NAME_SEPARATOR) != Zstring::npos ?
                               (fileName.BeforeLast(common::FILE_NAME_SEPARATOR) + common::FILE_NAME_SEPARATOR) : Zstring();

    Zstring extension = fileName.AfterLast(common::FILE_NAME_SEPARATOR).AfterLast(Zchar('.')); //extension needn't contain reasonable data
    if (extension.empty())
        extension = Zstr("FFS");
    extension.Truncate(3);

    for (int index = 0; index < 100000000; ++index) //filename must be representable by <= 8 characters
    {
        const Zstring output = pathPrefix + Zstring::fromNumber(index) + Zchar('.') + extension;
        if (!ffs3::somethingExists(output)) //ensure uniqueness
            return output;
    }

    throw std::runtime_error(std::string("100000000 files, one for each number, exist in this directory? You're kidding...\n") + std::string(wxString(pathPrefix.c_str()).ToUTF8()));
}


//try to handle issues with already existing short 8.3 file names on Windows 7
bool fix8Dot3NameClash(const Zstring& oldName, const Zstring& newName)  //throw (FileError); return "true" if rename operation succeeded
{
    using namespace ffs3;

    if (newName.find(common::FILE_NAME_SEPARATOR) == Zstring::npos)
        return false;

    if (ffs3::somethingExists(newName)) //name OR directory!
    {
        const Zstring fileNameOrig  = newName.AfterLast(common::FILE_NAME_SEPARATOR); //returns the whole string if ch not found
        const Zstring fileNameShort = getFilenameFmt(newName, ::GetShortPathName).AfterLast(common::FILE_NAME_SEPARATOR); //throw() returns empty string on error
        const Zstring fileNameLong  = getFilenameFmt(newName, ::GetLongPathName) .AfterLast(common::FILE_NAME_SEPARATOR); //throw() returns empty string on error

        if (!fileNameShort.empty() &&
            !fileNameLong.empty()  &&
            EqualFilename() (fileNameOrig,  fileNameShort) &&
            !EqualFilename()(fileNameShort, fileNameLong))
        {
            //we detected an event where newName is in shortname format (although it is intended to be a long name) and
            //writing target file failed because another unrelated file happens to have the same short name

            const Zstring unrelatedPathLong = newName.BeforeLast(common::FILE_NAME_SEPARATOR) + common::FILE_NAME_SEPARATOR +
                                              fileNameLong;

            //find another name in short format: this ensures the actual short name WILL be renamed as well!
            const Zstring parkedTarget = createTemp8Dot3Name(newName);

            //move already existing short name out of the way for now
            renameFileInternal(unrelatedPathLong, parkedTarget); //throw (FileError: ErrorDifferentVolume);
            //DON'T call ffs3::renameFile() to avoid reentrance!

            //schedule cleanup; the file system should assign this unrelated file a new (unique) short name
            Loki::ScopeGuard guard = Loki::MakeGuard(renameFileInternal, parkedTarget, unrelatedPathLong);//equivalent to Boost.ScopeExit in this case
            (void)guard; //silence warning "unused variable"

            renameFileInternal(oldName, newName); //the short filename name clash is solved, this should work now
            return true;
        }
    }
    return false; //issue not fixed
}
#endif
}


//rename file: no copying!!!
void ffs3::renameFile(const Zstring& oldName, const Zstring& newName) //throw (FileError: ErrorDifferentVolume, ErrorTargetExisting);
{
    try
    {
        renameFileInternal(oldName, newName); //throw (FileError: ErrorDifferentVolume, ErrorTargetExisting)
    }
    catch (const FileError&)
    {
#ifdef FFS_WIN
        if (fix8Dot3NameClash(oldName, newName)) //throw (FileError); try to handle issues with already existing short 8.3 file names on Windows 7
            return;
#endif
        throw;
    }
}


using ffs3::CallbackMoveFile;

class CopyCallbackImpl : public ffs3::CallbackCopyFile //callback functionality
{
public:
    CopyCallbackImpl(const Zstring& sourceFile, CallbackMoveFile& callback) : sourceFile_(sourceFile), moveCallback(callback) {}

    virtual void deleteTargetFile(const Zstring& targetFile) { assert(!fileExists(targetFile)); }

    virtual Response updateCopyStatus(const wxULongLong& totalBytesTransferred)
    {
        switch (moveCallback.requestUiRefresh(sourceFile_))
        {
            case CallbackMoveFile::CONTINUE:
                return CallbackCopyFile::CONTINUE;

            case CallbackMoveFile::CANCEL:
                return CallbackCopyFile::CANCEL;
        }
        return CallbackCopyFile::CONTINUE; //dummy return value
    }

private:
    const Zstring sourceFile_;
    CallbackMoveFile& moveCallback;
};


void ffs3::moveFile(const Zstring& sourceFile, const Zstring& targetFile, bool ignoreExisting, CallbackMoveFile* callback)   //throw (FileError);
{
    //call back once per file (moveFile() is called by moveDirectory())
    if (callback)
        switch (callback->requestUiRefresh(sourceFile))
        {
            case CallbackMoveFile::CONTINUE:
                break;
            case CallbackMoveFile::CANCEL: //a user aborted operation IS an error condition!
                throw FileError(wxString(_("Error moving file:")) + wxT("\n\"") + zToWx(sourceFile) +  wxT("\" ->\n\"") + zToWx(targetFile) + wxT("\"") +
                                wxT("\n\n") + _("Operation aborted!"));
        }

    const bool targetExisting = fileExists(targetFile);

    if (targetExisting && !ignoreExisting) //test file existence: e.g. Linux might silently overwrite existing symlinks
        throw FileError(wxString(_("Error moving file:")) + wxT("\n\"") + zToWx(sourceFile) +  wxT("\" ->\n\"") + zToWx(targetFile) + wxT("\"") +
                        wxT("\n\n") + _("Target file already existing!"));

    if (!targetExisting)
    {
        //try to move the file directly without copying
        try
        {
            renameFile(sourceFile, targetFile); //throw (FileError: ErrorDifferentVolume);
            return; //great, we get away cheaply!
        }
        //if moving failed treat as error (except when it tried to move to a different volume: in this case we will copy the file)
        catch (const ErrorDifferentVolume&) {}

        //file is on a different volume: let's copy it
        std::auto_ptr<CopyCallbackImpl> copyCallback(callback != NULL ? new CopyCallbackImpl(sourceFile, *callback) : NULL);

        if (symlinkExists(sourceFile))
            copySymlink(sourceFile, targetFile, SYMLINK_TYPE_FILE, false); //throw (FileError) dont copy filesystem permissions
        else
            copyFile(sourceFile, targetFile, false, copyCallback.get()); //throw (FileError);

        //attention: if copy-operation was cancelled an exception is thrown => sourcefile is not deleted, as we wish!
    }

    removeFile(sourceFile); //throw (FileError)
    //note: copying file is NOT undone in case of exception: currently this function is called in context of user-defined deletion dir, where this behavior is fine
}

namespace
{
class TraverseOneLevel : public ffs3::TraverseCallback
{
public:
    typedef std::pair<Zstring, Zstring> NamePair;
    typedef std::vector<NamePair> NameList;

    TraverseOneLevel(NameList& files, NameList& dirs) :
        files_(files),
        dirs_(dirs) {}

    virtual void onFile(const Zchar* shortName, const Zstring& fullName, const FileInfo& details)
    {
        files_.push_back(NamePair(shortName, fullName));
    }

    virtual void onSymlink(const Zchar* shortName, const Zstring& fullName, const SymlinkInfo& details)
    {
        if (details.dirLink)
            dirs_.push_back(NamePair(shortName, fullName));
        else
            files_.push_back(NamePair(shortName, fullName));
    }

    virtual ReturnValDir onDir(const Zchar* shortName, const Zstring& fullName)
    {
        dirs_.push_back(NamePair(shortName, fullName));
        return Loki::Int2Type<ReturnValDir::TRAVERSING_DIR_IGNORE>(); //DON'T traverse into subdirs; moveDirectory works recursively!
    }

    virtual void onError(const wxString& errorText)
    {
        throw FileError(errorText);
    }

private:
    NameList& files_;
    NameList& dirs_;
};


struct RemoveCallbackImpl : public ffs3::CallbackRemoveDir
{
    RemoveCallbackImpl(const Zstring& sourceDir,
                       const Zstring& targetDir,
                       CallbackMoveFile& moveCallback) :
        sourceDir_(sourceDir),
        targetDir_(targetDir),
        moveCallback_(moveCallback) {}

    virtual void requestUiRefresh(const Zstring& currentObject)
    {
        switch (moveCallback_.requestUiRefresh(sourceDir_))
        {
            case CallbackMoveFile::CONTINUE:
                break;
            case CallbackMoveFile::CANCEL: //a user aborted operation IS an error condition!
                throw ffs3::FileError(wxString(_("Error moving directory:")) + wxT("\n\"") + ffs3::zToWx(sourceDir_) +  wxT("\" ->\n\"") +
                                      ffs3::zToWx(targetDir_) + wxT("\"") + wxT("\n\n") + _("Operation aborted!"));
        }
    }

private:
    const Zstring sourceDir_;
    const Zstring targetDir_;
    CallbackMoveFile& moveCallback_;
};
}


void moveDirectoryImpl(const Zstring& sourceDir, const Zstring& targetDir, bool ignoreExisting, CallbackMoveFile* callback)   //throw (FileError);
{
    using namespace ffs3;

    //call back once per folder
    if (callback)
        switch (callback->requestUiRefresh(sourceDir))
        {
            case CallbackMoveFile::CONTINUE:
                break;
            case CallbackMoveFile::CANCEL: //a user aborted operation IS an error condition!
                throw FileError(wxString(_("Error moving directory:")) + wxT("\n\"") + zToWx(sourceDir) +  wxT("\" ->\n\"") +
                                zToWx(targetDir) + wxT("\"") + wxT("\n\n") + _("Operation aborted!"));
        }

    const bool targetExisting = dirExists(targetDir);

    if (targetExisting && !ignoreExisting) //directory or symlink exists (or even a file... this error will be caught later)
        throw FileError(wxString(_("Error moving directory:")) + wxT("\n\"") + zToWx(sourceDir) +  wxT("\" ->\n\"") + zToWx(targetDir) + wxT("\"") +
                        wxT("\n\n") + _("Target directory already existing!"));

    const bool isSymlink = symlinkExists(sourceDir);

    if (!targetExisting)
    {
        //first try to move the directory directly without copying
        try
        {
            renameFile(sourceDir, targetDir); //throw (FileError: ErrorDifferentVolume, ErrorTargetExisting);
            return; //great, we get away cheaply!
        }
        //if moving failed treat as error (except when it tried to move to a different volume: in this case we will copy the directory)
        catch (const ErrorDifferentVolume&) {}

        //create target
        if (isSymlink)
            copySymlink(sourceDir, targetDir, SYMLINK_TYPE_DIR, false); //throw (FileError) -> don't copy permissions
        else
            createDirectory(targetDir, sourceDir, false); //throw (FileError)
    }

    if (!isSymlink) //handle symbolic links
    {
        //move files/folders recursively
        TraverseOneLevel::NameList fileList; //list of names: 1. short 2.long
        TraverseOneLevel::NameList dirList;  //

        //traverse source directory one level
        TraverseOneLevel traverseCallback(fileList, dirList);
        traverseFolder(sourceDir, false, traverseCallback); //traverse one level, don't follow symlinks

        const Zstring targetDirFormatted = targetDir.EndsWith(common::FILE_NAME_SEPARATOR) ? //ends with path separator
                                           targetDir :
                                           targetDir + common::FILE_NAME_SEPARATOR;

        //move files
        for (TraverseOneLevel::NameList::const_iterator i = fileList.begin(); i != fileList.end(); ++i)
            ffs3::moveFile(i->second, targetDirFormatted + i->first, ignoreExisting, callback); //throw (FileError: ErrorTargetExisting);

        //move directories
        for (TraverseOneLevel::NameList::const_iterator i = dirList.begin(); i != dirList.end(); ++i)
            ::moveDirectoryImpl(i->second, targetDirFormatted + i->first, ignoreExisting, callback);

        //attention: if move-operation was cancelled an exception is thrown => sourceDir is not deleted, as we wish!
    }

    //delete source
    std::auto_ptr<RemoveCallbackImpl> removeCallback(callback != NULL ? new RemoveCallbackImpl(sourceDir, targetDir, *callback) : NULL);
    removeDirectory(sourceDir, removeCallback.get()); //throw (FileError);
}


void ffs3::moveDirectory(const Zstring& sourceDir, const Zstring& targetDir, bool ignoreExisting, CallbackMoveFile* callback)   //throw (FileError);
{
#ifdef FFS_WIN
    const Zstring& sourceDirFormatted = sourceDir;
    const Zstring& targetDirFormatted = targetDir;

#elif defined FFS_LINUX
    const Zstring sourceDirFormatted = //remove trailing slash
        sourceDir.size() > 1 && sourceDir.EndsWith(common::FILE_NAME_SEPARATOR) ?  //exception: allow '/'
        sourceDir.BeforeLast(common::FILE_NAME_SEPARATOR) :
        sourceDir;
    const Zstring targetDirFormatted = //remove trailing slash
        targetDir.size() > 1 && targetDir.EndsWith(common::FILE_NAME_SEPARATOR) ?  //exception: allow '/'
        targetDir.BeforeLast(common::FILE_NAME_SEPARATOR) :
        targetDir;
#endif

    ::moveDirectoryImpl(sourceDirFormatted, targetDirFormatted, ignoreExisting, callback);
}


class FilesDirsOnlyTraverser : public ffs3::TraverseCallback
{
public:
    FilesDirsOnlyTraverser(std::vector<Zstring>& files, std::vector<Zstring>& dirs) :
        m_files(files),
        m_dirs(dirs) {}

    virtual void onFile(const Zchar* shortName, const Zstring& fullName, const FileInfo& details)
    {
        m_files.push_back(fullName);
    }
    virtual void onSymlink(const Zchar* shortName, const Zstring& fullName, const SymlinkInfo& details)
    {
        if (details.dirLink)
            m_dirs.push_back(fullName);
        else
            m_files.push_back(fullName);
    }
    virtual ReturnValDir onDir(const Zchar* shortName, const Zstring& fullName)
    {
        m_dirs.push_back(fullName);
        return Loki::Int2Type<ReturnValDir::TRAVERSING_DIR_IGNORE>(); //DON'T traverse into subdirs; removeDirectory works recursively!
    }
    virtual void onError(const wxString& errorText)
    {
        throw FileError(errorText);
    }

private:
    std::vector<Zstring>& m_files;
    std::vector<Zstring>& m_dirs;
};


void ffs3::removeDirectory(const Zstring& directory, CallbackRemoveDir* callback)
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
        {
            wxString errorMessage = wxString(_("Error deleting directory:")) + wxT("\n\"") + zToWx(directory) + wxT("\"");
            throw FileError(errorMessage + wxT("\n\n") + ffs3::getLastErrorFormatted());
        }
        return;
    }

    std::vector<Zstring> fileList;
    std::vector<Zstring> dirList;

    //get all files and directories from current directory (WITHOUT subdirectories!)
    FilesDirsOnlyTraverser traverser(fileList, dirList);
    ffs3::traverseFolder(directory, false, traverser); //don't follow symlinks


    //delete files
    for (std::vector<Zstring>::const_iterator i = fileList.begin(); i != fileList.end(); ++i)
    {
        if (callback) callback->requestUiRefresh(*i); //call once per file
        removeFile(*i);
    }

    //delete directories recursively
    for (std::vector<Zstring>::const_iterator i = dirList.begin(); i != dirList.end(); ++i)
    {
        if (callback) callback->requestUiRefresh(*i); //and once per folder
        removeDirectory(*i, callback); //call recursively to correctly handle symbolic links
    }


    //parent directory is deleted last
#ifdef FFS_WIN
    if (!::RemoveDirectory(directoryFmt.c_str())) //remove directory, support for \\?\-prefix
#else
    if (::rmdir(directory.c_str()) != 0)
#endif
    {
        wxString errorMessage = wxString(_("Error deleting directory:")) + wxT("\n\"") + zToWx(directory) + wxT("\"");
        throw FileError(errorMessage + wxT("\n\n") + ffs3::getLastErrorFormatted());
    }
}


void ffs3::copyFileTimes(const Zstring& sourceObj, const Zstring& targetObj, bool deRefSymlinks) //throw (FileError)
{
#ifdef FFS_WIN
    FILETIME creationTime   = {};
    FILETIME lastWriteTime  = {};

    {
        WIN32_FILE_ATTRIBUTE_DATA sourceAttr = {};
        if (!::GetFileAttributesEx(applyLongPathPrefix(sourceObj).c_str(), //__in   LPCTSTR lpFileName,
                                   GetFileExInfoStandard,                  //__in   GET_FILEEX_INFO_LEVELS fInfoLevelId,
                                   &sourceAttr))                           //__out  LPVOID lpFileInformation
        {
            const wxString errorMessage = wxString(_("Error reading file attributes:")) + wxT("\n\"") + zToWx(sourceObj) + wxT("\"");
            throw FileError(errorMessage + wxT("\n\n") + ffs3::getLastErrorFormatted());
        }

        const bool isReparsePoint = (sourceAttr.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT) != 0;
        const bool isDirectory    = (sourceAttr.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)     != 0;

        if (isReparsePoint && deRefSymlinks) //we have a symlink AND need to dereference...
        {
            HANDLE hSource = ::CreateFile(applyLongPathPrefix(sourceObj).c_str(),
                                          FILE_READ_ATTRIBUTES,
                                          FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                                          NULL,
                                          OPEN_EXISTING,
                                          FILE_FLAG_BACKUP_SEMANTICS, //needed to open a directory; no FILE_FLAG_OPEN_REPARSE_POINT => deref symlinks
                                          NULL);
            if (hSource == INVALID_HANDLE_VALUE)
            {
                const wxString errorMessage = wxString(_("Error reading file attributes:")) + wxT("\n\"") + zToWx(sourceObj) + wxT("\"");
                throw FileError(errorMessage + wxT("\n\n") + ffs3::getLastErrorFormatted());
            }

            Loki::ScopeGuard dummy = Loki::MakeGuard(::CloseHandle, hSource);
            (void)dummy; //silence warning "unused variable"

            if (!::GetFileTime(hSource,        //__in       HANDLE hFile,
                               &creationTime,   //__out_opt  LPFILETIME lpCreationTime,
                               NULL, //__out_opt  LPFILETIME lpLastAccessTime,
                               &lastWriteTime)) //__out_opt  LPFILETIME lpLastWriteTime
            {
                const wxString errorMessage = wxString(_("Error reading file attributes:")) + wxT("\n\"") + zToWx(sourceObj) + wxT("\"");
                throw FileError(errorMessage + wxT("\n\n") + ffs3::getLastErrorFormatted());
            }
        }
        else
        {
            creationTime   = sourceAttr.ftCreationTime;
            lastWriteTime  = sourceAttr.ftLastWriteTime;
        }

        //####################################### DST hack ###########################################
        if (!isDirectory) //dst hack not (yet) required for directories (symlinks implicitly checked by isFatDrive())
        {
            if (dst::isFatDrive(sourceObj)) //throw()
            {
                const dst::RawTime rawTime(creationTime, lastWriteTime);
                if (dst::fatHasUtcEncoded(rawTime)) //throw (std::runtime_error)
                {
                    lastWriteTime = dst::fatDecodeUtcTime(rawTime); //return last write time in real UTC, throw (std::runtime_error)
                    ::GetSystemTimeAsFileTime(&creationTime); //real creation time information is not available...
                }
            }

            if (dst::isFatDrive(targetObj)) //throw()
            {
                const dst::RawTime encodedTime = dst::fatEncodeUtcTime(lastWriteTime); //throw (std::runtime_error)
                creationTime  = encodedTime.createTimeRaw;
                lastWriteTime = encodedTime.writeTimeRaw;
            }
        }
        //####################################### DST hack ###########################################
    }


    //privilege SE_BACKUP_NAME doesn't seem to be required here for symbolic links
    //note: setting privileges requires admin rights!

    HANDLE hTarget = ::CreateFile(applyLongPathPrefix(targetObj).c_str(),
                                  FILE_WRITE_ATTRIBUTES,
                                  FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                                  NULL,
                                  OPEN_EXISTING,
                                  FILE_FLAG_BACKUP_SEMANTICS | //needed to open a directory
                                  (deRefSymlinks ? 0 : FILE_FLAG_OPEN_REPARSE_POINT), //process symlinks
                                  NULL);
    if (hTarget == INVALID_HANDLE_VALUE)
    {
        wxString errorMessage = wxString(_("Error changing modification time:")) + wxT("\n\"") + zToWx(targetObj) + wxT("\"");
        throw FileError(errorMessage + wxT("\n\n") + ffs3::getLastErrorFormatted());
    }
    Loki::ScopeGuard dummy = Loki::MakeGuard(::CloseHandle, hTarget);
    (void)dummy; //silence warning "unused variable"

    if (!::SetFileTime(hTarget,
                       &creationTime,
                       NULL,
                       &lastWriteTime))
    {
        wxString errorMessage = wxString(_("Error changing modification time:")) + wxT("\n\"") + zToWx(targetObj) + wxT("\"");
        throw FileError(errorMessage + wxT("\n\n") + ffs3::getLastErrorFormatted());
    }

#ifndef NDEBUG //dst hack: verify data written
    if (dst::isFatDrive(targetObj) && !ffs3::dirExists(targetObj)) //throw()
    {
        WIN32_FILE_ATTRIBUTE_DATA debugeAttr = {};
        assert(::GetFileAttributesEx(applyLongPathPrefix(targetObj).c_str(), //__in   LPCTSTR lpFileName,
                                     GetFileExInfoStandard,                  //__in   GET_FILEEX_INFO_LEVELS fInfoLevelId,
                                     &debugeAttr));                          //__out  LPVOID lpFileInformation

        assert(::CompareFileTime(&debugeAttr.ftCreationTime,  &creationTime)  == 0);
        assert(::CompareFileTime(&debugeAttr.ftLastWriteTime, &lastWriteTime) == 0);
    }
#endif

#elif defined FFS_LINUX
    if (deRefSymlinks)
    {
        struct stat objInfo = {};
        if (::stat(sourceObj.c_str(), &objInfo) != 0) //read file attributes from source directory
        {
            const wxString errorMessage = wxString(_("Error reading file attributes:")) + wxT("\n\"") + zToWx(sourceObj) + wxT("\"");
            throw FileError(errorMessage + wxT("\n\n") + ffs3::getLastErrorFormatted());
        }

        struct utimbuf newTimes = {};
        newTimes.actime  = objInfo.st_atime;
        newTimes.modtime = objInfo.st_mtime;

        //(try to) set new "last write time"
        if (::utime(targetObj.c_str(), &newTimes) != 0) //return value not evaluated!
        {
            wxString errorMessage = wxString(_("Error changing modification time:")) + wxT("\n\"") + zToWx(targetObj) + wxT("\"");
            throw FileError(errorMessage + wxT("\n\n") + ffs3::getLastErrorFormatted());
        }
    }
    else
    {
        struct stat objInfo = {};
        if (::lstat(sourceObj.c_str(), &objInfo) != 0) //read file attributes from source directory
        {
            const wxString errorMessage = wxString(_("Error reading file attributes:")) + wxT("\n\"") + zToWx(sourceObj) + wxT("\"");
            throw FileError(errorMessage + wxT("\n\n") + ffs3::getLastErrorFormatted());
        }

        struct timeval newTimes[2] = {};
        newTimes[0].tv_sec  = objInfo.st_atime;	/* seconds */
        newTimes[0].tv_usec = 0;	            /* microseconds */

        newTimes[1].tv_sec  = objInfo.st_mtime;	/* seconds */
        newTimes[1].tv_usec = 0;            	/* microseconds */

        if (::lutimes(targetObj.c_str(), newTimes) != 0) //return value not evaluated!
        {
            wxString errorMessage = wxString(_("Error changing modification time:")) + wxT("\n\"") + zToWx(targetObj) + wxT("\"");
            throw FileError(errorMessage + wxT("\n\n") + ffs3::getLastErrorFormatted());
        }
    }
#endif
}


namespace
{
struct TryCleanUp
{
    static void tryDeleteDir(const Zstring& dirname) //throw ()
    {
        try
        {
            ffs3::removeDirectory(dirname, NULL);
        }
        catch (...) {}
    }

    static void tryDeleteFile(const Zstring& filename) //throw ()
    {
        try
        {
            ffs3::removeFile(filename);
        }
        catch (...) {}
    }
};


#ifdef FFS_WIN
Zstring resolveDirectorySymlink(const Zstring& dirLinkName) //get full target path of symbolic link to a directory; throw (FileError)
{
    //open handle to target of symbolic link
    const HANDLE hDir = ::CreateFile(ffs3::applyLongPathPrefix(dirLinkName).c_str(),
                                     0,
                                     FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                                     NULL,
                                     OPEN_EXISTING,
                                     FILE_FLAG_BACKUP_SEMANTICS,  //needed to open a directory
                                     NULL);
    if (hDir == INVALID_HANDLE_VALUE)
    {
        wxString errorMessage = wxString(_("Error resolving symbolic link:")) + wxT("\n\"") + ffs3::zToWx(dirLinkName) + wxT("\"");
        throw FileError(errorMessage + wxT("\n\n") + ffs3::getLastErrorFormatted());
    }

    Loki::ScopeGuard dummy = Loki::MakeGuard(::CloseHandle, hDir);
    (void)dummy; //silence warning "unused variable"

    const size_t BUFFER_SIZE = 10000;
    TCHAR targetPath[BUFFER_SIZE];

    //dynamically load windows API function
    typedef DWORD (WINAPI *GetFinalPathNameByHandleWFunc)(
        HANDLE hFile,
        LPTSTR lpszFilePath,
        DWORD cchFilePath,
        DWORD dwFlags);
    static const GetFinalPathNameByHandleWFunc getFinalPathNameByHandle =
        util::getDllFun<GetFinalPathNameByHandleWFunc>(L"kernel32.dll", "GetFinalPathNameByHandleW");

    if (getFinalPathNameByHandle == NULL)
        throw FileError(wxString(_("Error loading library function:")) + wxT("\n\"") + wxT("GetFinalPathNameByHandleW") + wxT("\""));

    const DWORD rv = getFinalPathNameByHandle(
                         hDir,       //__in   HANDLE hFile,
                         targetPath, //__out  LPTSTR lpszFilePath,
                         BUFFER_SIZE,//__in   DWORD cchFilePath,
                         0);         //__in   DWORD dwFlags
    if (rv >= BUFFER_SIZE || rv == 0)
    {
        wxString errorMessage = wxString(_("Error resolving symbolic link:")) + wxT("\n\"") + ffs3::zToWx(dirLinkName) + wxT("\"");
        if (rv == 0) errorMessage += wxT("\n\n") + ffs3::getLastErrorFormatted();
        throw FileError(errorMessage);
    }

    return targetPath;
}
#endif


#ifdef HAVE_SELINUX
//copy SELinux security context
void copySecurityContext(const Zstring& source, const Zstring& target, bool derefSymlinks) //throw (FileError)
{
    using ffs3::zToWx;

    security_context_t contextSource = NULL;
    const int rv = derefSymlinks ?
                   ::getfilecon (source.c_str(), &contextSource) :
                   ::lgetfilecon(source.c_str(), &contextSource);
    if (rv < 0)
    {
        if (errno == ENODATA ||  //no security context (allegedly) is not an error condition on SELinux
            errno == EOPNOTSUPP) //extended attributes are not supported by the filesystem
            return;

        wxString errorMessage = wxString(_("Error reading security context:")) + wxT("\n\"") + zToWx(source) + wxT("\"");
        throw FileError(errorMessage + wxT("\n\n") + ffs3::getLastErrorFormatted());
    }
    Loki::ScopeGuard dummy1 = Loki::MakeGuard(::freecon, contextSource);
    (void)dummy1; //silence warning "unused variable"

    {
        security_context_t contextTarget = NULL;
        const int rv2 = derefSymlinks ?
                        ::getfilecon (target.c_str(), &contextTarget) :
                        ::lgetfilecon(target.c_str(), &contextTarget);
        if (rv2 < 0)
        {
            if (errno == EOPNOTSUPP)
                return;
            //else: still try to set security context
        }
        else
        {
            Loki::ScopeGuard dummy2 = Loki::MakeGuard(::freecon, contextTarget);
            (void)dummy2; //silence warning "unused variable"

            if (::strcmp(contextSource, contextTarget) == 0) //nothing to do
                return;
        }
    }

    const int rv3 = derefSymlinks ?
                    ::setfilecon (target.c_str(), contextSource) :
                    ::lsetfilecon(target.c_str(), contextSource);
    if (rv3 < 0)
    {
        wxString errorMessage = wxString(_("Error writing security context:")) + wxT("\n\"") + zToWx(target) + wxT("\"");
        throw FileError(errorMessage + wxT("\n\n") + ffs3::getLastErrorFormatted());
    }
}
#endif //HAVE_SELINUX


//copy permissions for files, directories or symbolic links: requires admin rights
void copyObjectPermissions(const Zstring& source, const Zstring& target, bool derefSymlinks) //throw (FileError);
{
#ifdef FFS_WIN
    //setting privileges requires admin rights!

    //enable privilege: required to read/write SACL information
    Privileges::getInstance().ensureActive(SE_SECURITY_NAME); //polling allowed...

    //enable privilege: required to copy owner information
    Privileges::getInstance().ensureActive(SE_RESTORE_NAME);

    //the following privilege may be required according to http://msdn.microsoft.com/en-us/library/aa364399(VS.85).aspx (although not needed nor active in my tests)
    Privileges::getInstance().ensureActive(SE_BACKUP_NAME);

    PSECURITY_DESCRIPTOR buffer = NULL;
    PSID owner                  = NULL;
    PSID group                  = NULL;
    PACL dacl                   = NULL;
    PACL sacl                   = NULL;

    //http://msdn.microsoft.com/en-us/library/aa364399(v=VS.85).aspx
    const HANDLE hSource = ::CreateFile(ffs3::applyLongPathPrefix(source).c_str(),
                                        READ_CONTROL | ACCESS_SYSTEM_SECURITY, //ACCESS_SYSTEM_SECURITY required for SACL access
                                        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                                        NULL,
                                        OPEN_EXISTING,
                                        FILE_FLAG_BACKUP_SEMANTICS | (derefSymlinks ? 0 : FILE_FLAG_OPEN_REPARSE_POINT), //FILE_FLAG_BACKUP_SEMANTICS needed to open a directory
                                        NULL);
    if (hSource == INVALID_HANDLE_VALUE)
    {
        const wxString errorMessage = wxString(_("Error copying file permissions:")) + wxT("\n\"") + zToWx(source) +  wxT("\" ->\n\"") + zToWx(target) + wxT("\"") + wxT("\n\n");
        throw FileError(errorMessage + ffs3::getLastErrorFormatted() + wxT(" (OR)"));
    }
    Loki::ScopeGuard dummy = Loki::MakeGuard(::CloseHandle, hSource);
    (void)dummy; //silence warning "unused variable"

    //  DWORD rc = ::GetNamedSecurityInfo(const_cast<WCHAR*>(applyLongPathPrefix(source).c_str()), -> does NOT dereference symlinks!
    DWORD rc = ::GetSecurityInfo(
                   hSource,        //__in       LPTSTR pObjectName,
                   SE_FILE_OBJECT, //__in       SE_OBJECT_TYPE ObjectType,
                   OWNER_SECURITY_INFORMATION | GROUP_SECURITY_INFORMATION | DACL_SECURITY_INFORMATION | SACL_SECURITY_INFORMATION,  //__in       SECURITY_INFORMATION SecurityInfo,
                   &owner,   //__out_opt  PSID *ppsidOwner,
                   &group,   //__out_opt  PSID *ppsidGroup,
                   &dacl,    //__out_opt  PACL *ppDacl,
                   &sacl,    //__out_opt  PACL *ppSacl,
                   &buffer); //__out_opt  PSECURITY_DESCRIPTOR *ppSecurityDescriptor
    if (rc != ERROR_SUCCESS)
    {
        const wxString errorMessage = wxString(_("Error copying file permissions:")) + wxT("\n\"") + zToWx(source) +  wxT("\" ->\n\"") + zToWx(target) + wxT("\"") + wxT("\n\n");
        throw FileError(errorMessage + ffs3::getLastErrorFormatted(rc) + wxT(" (R)"));
    }

    Loki::ScopeGuard dummy4 = Loki::MakeGuard(::LocalFree, buffer);
    (void)dummy4; //silence warning "unused variable"


    const Zstring targetFmt = ffs3::applyLongPathPrefix(target);

    //read-only file attribute may cause trouble: temporarily reset it
    const DWORD targetAttr = ::GetFileAttributes(targetFmt.c_str());
    Loki::ScopeGuard resetAttributes = Loki::MakeGuard(::SetFileAttributes, targetFmt, targetAttr);
    if (targetAttr != INVALID_FILE_ATTRIBUTES &&
        (targetAttr & FILE_ATTRIBUTE_READONLY))
        ::SetFileAttributes(targetFmt.c_str(), targetAttr & (~FILE_ATTRIBUTE_READONLY)); //try to...
    else
        resetAttributes.Dismiss();


    const HANDLE hTarget = ::CreateFile( targetFmt.c_str(),                                                     // lpFileName
                                         FILE_GENERIC_WRITE | WRITE_OWNER | WRITE_DAC | ACCESS_SYSTEM_SECURITY, // dwDesiredAccess: all four seem to be required!!!
                                         FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,                // dwShareMode
                                         NULL,                       // lpSecurityAttributes
                                         OPEN_EXISTING,              // dwCreationDisposition
                                         FILE_FLAG_BACKUP_SEMANTICS | (derefSymlinks ? 0 : FILE_FLAG_OPEN_REPARSE_POINT), // dwFlagsAndAttributes
                                         NULL);                        // hTemplateFile
    if (hTarget == INVALID_HANDLE_VALUE)
    {
        const wxString errorMessage = wxString(_("Error copying file permissions:")) + wxT("\n\"") + zToWx(source) +  wxT("\" ->\n\"") + zToWx(target) + wxT("\"") + wxT("\n\n");
        throw FileError(errorMessage + ffs3::getLastErrorFormatted() + wxT(" (OW)"));
    }
    Loki::ScopeGuard dummy2 = Loki::MakeGuard(::CloseHandle, hTarget);
    (void)dummy2; //silence warning "unused variable"

    //  rc = ::SetNamedSecurityInfo(const_cast<WCHAR*>(applyLongPathPrefix(target).c_str()), //__in      LPTSTR pObjectName, -> does NOT dereference symlinks!
    rc = ::SetSecurityInfo(
             hTarget,        //__in      LPTSTR pObjectName,
             SE_FILE_OBJECT, //__in      SE_OBJECT_TYPE ObjectType,
             OWNER_SECURITY_INFORMATION | GROUP_SECURITY_INFORMATION | DACL_SECURITY_INFORMATION | SACL_SECURITY_INFORMATION,  //__in      SECURITY_INFORMATION SecurityInfo,
             owner, //__in_opt  PSID psidOwner,
             group, //__in_opt  PSID psidGroup,
             dacl,  //__in_opt  PACL pDacl,
             sacl); //__in_opt  PACL pSacl

    if (rc != ERROR_SUCCESS)
    {
        const wxString errorMessage = wxString(_("Error copying file permissions:")) + wxT("\n\"") + zToWx(source) +  wxT("\" ->\n\"") + zToWx(target) + wxT("\"") + wxT("\n\n");
        throw FileError(errorMessage + ffs3::getLastErrorFormatted(rc) + wxT(" (W)"));
    }

#elif defined FFS_LINUX

#ifdef HAVE_SELINUX  //copy SELinux security context
    copySecurityContext(source, target, derefSymlinks); //throw (FileError)
#endif

    if (derefSymlinks)
    {
        struct stat fileInfo;
        if (::stat (source.c_str(), &fileInfo) != 0                        ||
            ::chown(target.c_str(), fileInfo.st_uid, fileInfo.st_gid) != 0 || // may require admin rights!
            ::chmod(target.c_str(), fileInfo.st_mode) != 0)
        {
            const wxString errorMessage = wxString(_("Error copying file permissions:")) + wxT("\n\"") + zToWx(source) +  wxT("\" ->\n\"") + zToWx(target) + wxT("\"") + wxT("\n\n");
            throw FileError(errorMessage + ffs3::getLastErrorFormatted() + wxT(" (R)"));
        }
    }
    else
    {
        struct stat fileInfo;
        if (::lstat (source.c_str(), &fileInfo) != 0                        ||
            ::lchown(target.c_str(), fileInfo.st_uid, fileInfo.st_gid) != 0 || // may require admin rights!
            (!symlinkExists(target) && ::chmod(target.c_str(), fileInfo.st_mode) != 0)) //setting access permissions doesn't make sense for symlinks on Linux: there is no lchmod()
        {
            const wxString errorMessage = wxString(_("Error copying file permissions:")) + wxT("\n\"") + zToWx(source) +  wxT("\" ->\n\"") + zToWx(target) + wxT("\"") + wxT("\n\n");
            throw FileError(errorMessage + ffs3::getLastErrorFormatted() + wxT(" (W)"));
        }
    }
#endif
}


void createDirectoryRecursively(const Zstring& directory, const Zstring& templateDir, bool copyFilePermissions, int level)
{
    using namespace ffs3;

    if (ffs3::dirExists(directory))
        return;

    if (level == 100) //catch endless recursion
        return;

    //try to create parent folders first
    const Zstring dirParent = directory.BeforeLast(common::FILE_NAME_SEPARATOR);
    if (!dirParent.empty() && !ffs3::dirExists(dirParent))
    {
        //call function recursively
        const Zstring templateParent = templateDir.BeforeLast(common::FILE_NAME_SEPARATOR);
        createDirectoryRecursively(dirParent, templateParent, copyFilePermissions, level + 1);
    }

    //now creation should be possible

    //default directory creation
#ifdef FFS_WIN
    //don't use ::CreateDirectoryEx:
    //- it may fail with "wrong parameter (error code 87)" when source is on mapped online storage
    //- automatically copies symbolic links if encountered: unfortunately it doesn't copy symlinks over network shares but silently creates empty folders instead (on XP)!
    //- it isn't able to copy most junctions because of missing permissions (although target path can be retrieved alternatively!)
    if (!::CreateDirectory(applyLongPathPrefixCreateDir(directory).c_str(), NULL))
#elif defined FFS_LINUX
    if (::mkdir(directory.c_str(), 0755) != 0)
#endif
    {
        if (level != 0) return;
        wxString errorMessage = wxString(_("Error creating directory:")) + wxT("\n\"") + zToWx(directory) + wxT("\"");
        throw FileError(errorMessage + wxT("\n\n") + ffs3::getLastErrorFormatted());
    }

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
                sourcePath = resolveDirectorySymlink(templateDir); //throw (FileError)
            }
            catch (FileError&) {} //dereferencing a symbolic link usually fails if it is located on network drive or client is XP: NOT really an error...
        }
        else //no symbolic link
            sourcePath = templateDir;

        //try to copy file attributes
        if (!sourcePath.empty())
        {
            const DWORD sourceAttr = ::GetFileAttributes(applyLongPathPrefix(sourcePath).c_str());
            if (sourceAttr != INVALID_FILE_ATTRIBUTES)
            {
                const bool isCompressed = (sourceAttr & FILE_ATTRIBUTE_COMPRESSED)  != 0;
                const bool isEncrypted  = (sourceAttr & FILE_ATTRIBUTE_ENCRYPTED)   != 0;

                ::SetFileAttributes(applyLongPathPrefix(directory), sourceAttr);

                if (isEncrypted)
                    ::EncryptFile(directory.c_str()); //seems no long path is required (check passed!)

                if (isCompressed)
                {
                    HANDLE hDir = ::CreateFile(applyLongPathPrefix(directory).c_str(),
                                               GENERIC_READ | GENERIC_WRITE, //read access required for FSCTL_SET_COMPRESSION
                                               FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                                               NULL,
                                               OPEN_EXISTING,
                                               FILE_FLAG_BACKUP_SEMANTICS,
                                               NULL);
                    if (hDir != INVALID_HANDLE_VALUE)
                    {
                        Loki::ScopeGuard dummy = Loki::MakeGuard(::CloseHandle, hDir);
                        (void)dummy; //silence warning "unused variable"

                        USHORT cmpState = COMPRESSION_FORMAT_DEFAULT;

                        DWORD bytesReturned = 0;
                        ::DeviceIoControl(hDir,                  //handle to file or directory
                                          FSCTL_SET_COMPRESSION, //dwIoControlCode
                                          &cmpState,             //input buffer
                                          sizeof(cmpState),      //size of input buffer
                                          NULL,                  //lpOutBuffer
                                          0,                     //OutBufferSize
                                          &bytesReturned,        //number of bytes returned
                                          NULL);                 //OVERLAPPED structure
                    }
                }
            }
        }
#endif

        //try to copy file times: NOT mission critical for a directory
        try
        {
            copyFileTimes(templateDir, directory, true); //throw (FileError)
        }
        catch (FileError&) {}

        Loki::ScopeGuard guardNewDir = Loki::MakeGuard(&TryCleanUp::tryDeleteDir, directory); //ensure cleanup:

        //enforce copying file permissions: it's advertized on GUI...
        if (copyFilePermissions)
            copyObjectPermissions(templateDir, directory, true); //throw (FileError)

        guardNewDir.Dismiss(); //target has been created successfully!
    }
}
}


void ffs3::createDirectory(const Zstring& directory, const Zstring& templateDir, bool copyFilePermissions)
{
    //remove trailing separator
    const Zstring dirFormatted = directory.EndsWith(common::FILE_NAME_SEPARATOR) ?
                                 directory.BeforeLast(common::FILE_NAME_SEPARATOR) :
                                 directory;

    const Zstring templateFormatted = templateDir.EndsWith(common::FILE_NAME_SEPARATOR) ?
                                      templateDir.BeforeLast(common::FILE_NAME_SEPARATOR) :
                                      templateDir;

    createDirectoryRecursively(dirFormatted, templateFormatted, copyFilePermissions, 0);
}


void ffs3::createDirectory(const Zstring& directory)
{
    ffs3::createDirectory(directory, Zstring(), false);
}


void ffs3::copySymlink(const Zstring& sourceLink, const Zstring& targetLink, ffs3::SymlinkType type, bool copyFilePermissions) //throw (FileError)
{
    const Zstring linkPath = getSymlinkRawTargetString(sourceLink); //accept broken symlinks; throw (FileError)

#ifdef FFS_WIN
    //dynamically load windows API function
    typedef BOOLEAN (WINAPI *CreateSymbolicLinkFunc)(LPCTSTR lpSymlinkFileName, LPCTSTR lpTargetFileName, DWORD dwFlags);
    static const CreateSymbolicLinkFunc createSymbolicLink = util::getDllFun<CreateSymbolicLinkFunc>(L"kernel32.dll", "CreateSymbolicLinkW");
    if (createSymbolicLink == NULL)
        throw FileError(wxString(_("Error loading library function:")) + wxT("\n\"") + wxT("CreateSymbolicLinkW") + wxT("\""));

    if (!createSymbolicLink(    //seems no long path prefix is required...
            targetLink.c_str(), //__in  LPTSTR lpSymlinkFileName,
            linkPath.c_str(),   //__in  LPTSTR lpTargetFileName,
            (type == SYMLINK_TYPE_DIR ? SYMBOLIC_LINK_FLAG_DIRECTORY : 0))) //__in  DWORD dwFlags
    {
        const wxString errorMessage = wxString(_("Error copying symbolic link:")) + wxT("\n\"") + zToWx(sourceLink) +  wxT("\" ->\n\"") + zToWx(targetLink) + wxT("\"");
        throw FileError(errorMessage + wxT("\n\n") + ffs3::getLastErrorFormatted());
    }

#elif defined FFS_LINUX
    if (::symlink(linkPath.c_str(), targetLink.c_str()) != 0)
    {
        const wxString errorMessage = wxString(_("Error copying symbolic link:")) + wxT("\n\"") + zToWx(sourceLink) +  wxT("\" ->\n\"") + zToWx(targetLink) + wxT("\"");
        throw FileError(errorMessage + wxT("\n\n") + ffs3::getLastErrorFormatted());
    }
#endif

    //allow only consistent objects to be created -> don't place before ::symlink, targetLink may already exist
    Loki::ScopeGuard guardNewDir = type == SYMLINK_TYPE_DIR ?
                                   Loki::MakeGuard(&TryCleanUp::tryDeleteDir,  targetLink) :
                                   Loki::MakeGuard(&TryCleanUp::tryDeleteFile, targetLink);

    //file times: essential for a symlink: enforce this! (don't just try!)
    copyFileTimes(sourceLink, targetLink, false); //throw (FileError)

    if (copyFilePermissions)
        copyObjectPermissions(sourceLink, targetLink, false); //throw (FileError)

    guardNewDir.Dismiss(); //target has been created successfully!
}


namespace
{
Zstring createTempName(const Zstring& filename)
{
    Zstring output = filename + ffs3::TEMP_FILE_ENDING;

    //ensure uniqueness
    for (int i = 1; ffs3::somethingExists(output); ++i)
        output = filename + Zchar('_') + Zstring::fromNumber(i) + ffs3::TEMP_FILE_ENDING;

    return output;
}

#ifdef FFS_WIN
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
    using ffs3::CallbackCopyFile;

    //small performance optimization: it seems this callback function is called for every 64 kB (depending on cluster size).
    static size_t callNr = 0;
    if (++callNr % 4 == 0) //executing callback every 256 kB should suffice
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

            CallbackCopyFile* callback = static_cast<CallbackCopyFile*>(lpData);
            try
            {
                switch (callback->updateCopyStatus(wxULongLong(totalBytesTransferred.HighPart, totalBytesTransferred.LowPart)))
                {
                    case CallbackCopyFile::CONTINUE:
                        break;
                    case CallbackCopyFile::CANCEL:
                        return PROGRESS_CANCEL;
                }
            }
            catch (...)
            {
                ::MessageBox(NULL, wxT("Exception in callback ffs3::copyFile! Please contact the author of FFS."), NULL, 0);
            }
        }
    }

    return PROGRESS_CONTINUE;
}


#ifndef COPY_FILE_COPY_SYMLINK
#define COPY_FILE_COPY_SYMLINK 0x00000800
#endif

bool supportForSymbolicLinks()
{
    OSVERSIONINFO osvi = {};
    osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);

    //symbolic links are supported starting with Vista
    if (::GetVersionEx(&osvi))
        return osvi.dwMajorVersion > 5; //XP has majorVersion == 5, minorVersion == 1; Vista majorVersion == 6, dwMinorVersion == 0
    //overview: http://msdn.microsoft.com/en-us/library/ms724834(VS.85).aspx
    return false;
}


#ifndef COPY_FILE_ALLOW_DECRYPTED_DESTINATION
#define COPY_FILE_ALLOW_DECRYPTED_DESTINATION 0x00000008
#endif

bool supportForNonEncryptedDestination()
{
    OSVERSIONINFO osvi = {};
    osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);

    //encrypted destination is not supported with Windows 2000
    if (::GetVersionEx(&osvi))
        return osvi.dwMajorVersion > 5 ||
               (osvi.dwMajorVersion == 5 && osvi.dwMinorVersion > 0); //2000 has majorVersion == 5, minorVersion == 0
    //overview: http://msdn.microsoft.com/en-us/library/ms724834(VS.85).aspx
    return false;
}


void rawCopyWinApi(const Zstring& sourceFile,
                   const Zstring& targetFile,
                   CallbackCopyFile* callback) //throw (FileError: ErrorTargetPathMissing, ErrorTargetExisting, ErrorFileLocked)
{
    Loki::ScopeGuard guardTarget = Loki::MakeGuard(&removeFile, targetFile); //transactional behavior: guard just before starting copy!

    DWORD copyFlags = COPY_FILE_FAIL_IF_EXISTS;

    //    static const bool symlinksSupported = supportForSymbolicLinks(); //only set "true" if supported by OS: else copying in Windows XP fails
    //    if (copyFileSymLinks && symlinksSupported)
    //        copyFlags |= COPY_FILE_COPY_SYMLINK;

    //allow copying from encrypted to non-encrytped location
    static const bool nonEncSupported = supportForNonEncryptedDestination();
    if (nonEncSupported)
        copyFlags |= COPY_FILE_ALLOW_DECRYPTED_DESTINATION;

    if (!::CopyFileEx( //same performance like CopyFile()
            applyLongPathPrefix(sourceFile).c_str(),
            applyLongPathPrefix(targetFile).c_str(),
            callback != NULL ? copyCallbackInternal : NULL,
            callback,
            NULL,
            copyFlags))
    {
        const DWORD lastError = ::GetLastError();

        //don't suppress "lastError == ERROR_REQUEST_ABORTED": a user aborted operation IS an error condition!

        //assemble error message...
        wxString errorMessage = wxString(_("Error copying file:")) + wxT("\n\"") + sourceFile.c_str() +  wxT("\" ->\n\"") + targetFile.c_str() + wxT("\"") +
                                wxT("\n\n") + ffs3::getLastErrorFormatted(lastError);

        //if file is locked (try to) use Windows Volume Shadow Copy Service
        if (lastError == ERROR_SHARING_VIOLATION ||
            lastError == ERROR_LOCK_VIOLATION)
            throw ErrorFileLocked(errorMessage);

        if (lastError == ERROR_FILE_EXISTS) //if target is existing this functions is expected to throw ErrorTargetExisting!!!
        {
            guardTarget.Dismiss(); //don't delete file that existed previously!
            throw ErrorTargetExisting(errorMessage);
        }

        if (lastError == ERROR_PATH_NOT_FOUND)
            throw ErrorTargetPathMissing(errorMessage);

        try //add more meaningful message
        {
            //trying to copy > 4GB file to FAT/FAT32 volume gives obscure ERROR_INVALID_PARAMETER (FAT can indeed handle files up to 4 Gig, tested!)
            if (lastError == ERROR_INVALID_PARAMETER &&
                dst::isFatDrive(targetFile) &&
                getFilesize(sourceFile) >= wxULongLong(1024 * 1024 * 1024) * 4) //throw (FileError)
                errorMessage += wxT("\nFAT volume cannot store file larger than 4 gigabyte!");
        }
        catch(...) {}

        throw FileError(errorMessage);
    }

    //adapt file modification time:
    copyFileTimes(sourceFile, targetFile, true); //throw (FileError)

    guardTarget.Dismiss(); //target has been created successfully!
}


void rawCopyWinOptimized(const Zstring& sourceFile,
                         const Zstring& targetFile,
                         CallbackCopyFile* callback) //throw (FileError: ErrorTargetPathMissing, ErrorTargetExisting, ErrorFileLocked)
{
    /*
                BackupRead()	FileRead()		CopyFileEx()
    			--------------------------------------------
    Attributes  NO			    NO              YES
    create time NO              NO              NO
    ADS			YES				NO				YES
    Encrypted	NO(silent fail)	NO				YES
    Compressed	NO				NO				NO
    Sparse		YES				NO				NO
    PERF		    6% faster		            -

    Mark stream as compressed: FSCTL_SET_COMPRESSION
        compatible with: BackupRead() FileRead()
    */

    //open sourceFile for reading
    HANDLE hFileIn = ::CreateFile(ffs3::applyLongPathPrefix(sourceFile).c_str(),
                                  GENERIC_READ,
                                  FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, //all shared modes are required to read files that are open in other applications
                                  NULL,
                                  OPEN_EXISTING,
                                  FILE_FLAG_SEQUENTIAL_SCAN,
                                  NULL);
    if (hFileIn == INVALID_HANDLE_VALUE)
    {
        const DWORD lastError = ::GetLastError();
        const wxString& errorMessage = wxString(_("Error opening file:")) + wxT("\n\"") + zToWx(sourceFile) + wxT("\"") + wxT("\n\n") + ffs3::getLastErrorFormatted(lastError);

        //if file is locked (try to) use Windows Volume Shadow Copy Service
        if (lastError == ERROR_SHARING_VIOLATION ||
            lastError == ERROR_LOCK_VIOLATION)
            throw ErrorFileLocked(errorMessage);

        throw FileError(errorMessage);
    }
    Loki::ScopeGuard dummy1 = Loki::MakeGuard(::CloseHandle, hFileIn);
    (void)dummy1; //silence warning "unused variable"


    BY_HANDLE_FILE_INFORMATION infoFileIn = {};
    if (!::GetFileInformationByHandle(hFileIn, &infoFileIn))
    {
        const wxString errorMessage = wxString(_("Error reading file attributes:")) + wxT("\n\"") + zToWx(sourceFile) + wxT("\"");
        throw FileError(errorMessage + wxT("\n\n") + ffs3::getLastErrorFormatted());
    }

    const bool sourceIsCompressed = (infoFileIn.dwFileAttributes & FILE_ATTRIBUTE_COMPRESSED)  != 0;
    const bool sourceIsSparse     = (infoFileIn.dwFileAttributes & FILE_ATTRIBUTE_SPARSE_FILE) != 0;
    const bool sourceIsEncrypted  = (infoFileIn.dwFileAttributes & FILE_ATTRIBUTE_ENCRYPTED)   != 0;

    bool targetSupportSparse     = false;
    bool targetSupportCompressed = false;
    bool targetSupportEncryption = false;

    if (sourceIsCompressed || sourceIsSparse || sourceIsEncrypted)
    {
        DWORD fsFlags = 0;

        {
            const size_t bufferSize = std::max(targetFile.size(), static_cast<size_t>(10000));
            boost::scoped_array<wchar_t> buffer(new wchar_t[bufferSize]);

            //suprisingly slow: ca. 1.5 ms per call!
            if (!::GetVolumePathName(targetFile.c_str(), //__in   LPCTSTR lpszFileName, -> seems to be no need for path prefix (check passed)
                                     buffer.get(),       //__out  LPTSTR lpszVolumePathName,
                                     static_cast<DWORD>(bufferSize))) //__in   DWORD cchBufferLength
                throw FileError(wxString(_("Error reading file attributes:")) + wxT("\n\"") + zToWx(targetFile) + wxT("\"") +
                                wxT("\n\n") + ffs3::getLastErrorFormatted() +
                                wxT("\nFailed to determine volume name!"));

            Zstring volumePath = buffer.get();
            if (!volumePath.EndsWith(common::FILE_NAME_SEPARATOR))
                volumePath += common::FILE_NAME_SEPARATOR;

            //suprisingly fast: ca. 0.03 ms per call!
            if (!::GetVolumeInformation(volumePath.c_str(), //__in_opt   LPCTSTR lpRootPathName,
                                        NULL,               //__out      LPTSTR lpVolumeNameBuffer,
                                        0,                  //__in       DWORD nVolumeNameSize,
                                        NULL,               //__out_opt  LPDWORD lpVolumeSerialNumber,
                                        NULL,               //__out_opt  LPDWORD lpMaximumComponentLength,
                                        &fsFlags,           //__out_opt  LPDWORD lpFileSystemFlags,
                                        NULL,               //__out      LPTSTR lpFileSystemNameBuffer,
                                        0))                 //__in       DWORD nFileSystemNameSize
                throw FileError(wxString(_("Error reading file attributes:")) + wxT("\n\"") + zToWx(volumePath) + wxT("\"") +
                                wxT("\n\n") + ffs3::getLastErrorFormatted() +
                                wxT("\nFailed to determine volume information!"));
        }

        targetSupportSparse     = (fsFlags & FILE_SUPPORTS_SPARSE_FILES) != 0;
        targetSupportCompressed = (fsFlags & FILE_FILE_COMPRESSION     ) != 0;
        targetSupportEncryption = (fsFlags & FILE_SUPPORTS_ENCRYPTION  ) != 0;
    }

    //####################################### DST hack ###########################################
    if (dst::isFatDrive(sourceFile)) //throw()
    {
        const dst::RawTime rawTime(infoFileIn.ftCreationTime, infoFileIn.ftLastWriteTime);
        if (dst::fatHasUtcEncoded(rawTime)) //throw (std::runtime_error)
        {
            infoFileIn.ftLastWriteTime = dst::fatDecodeUtcTime(rawTime); //return last write time in real UTC, throw (std::runtime_error)
            ::GetSystemTimeAsFileTime(&infoFileIn.ftCreationTime);       //real creation time information is not available...
        }
    }

    if (dst::isFatDrive(targetFile)) //throw()
    {
        const dst::RawTime encodedTime = dst::fatEncodeUtcTime(infoFileIn.ftLastWriteTime); //throw (std::runtime_error)
        infoFileIn.ftCreationTime  = encodedTime.createTimeRaw;
        infoFileIn.ftLastWriteTime = encodedTime.writeTimeRaw;
    }
    //####################################### DST hack ###########################################

    const DWORD validAttribs = FILE_ATTRIBUTE_READONLY |
                               FILE_ATTRIBUTE_HIDDEN   |
                               FILE_ATTRIBUTE_SYSTEM   |
                               FILE_ATTRIBUTE_ARCHIVE  |            //those two are not set properly (not worse than ::CopyFileEx())
                               FILE_ATTRIBUTE_NOT_CONTENT_INDEXED | //
                               (targetSupportEncryption ? FILE_ATTRIBUTE_ENCRYPTED : 0);

    //create targetFile and open it for writing
    HANDLE hFileOut = ::CreateFile(ffs3::applyLongPathPrefix(targetFile).c_str(),
                                   GENERIC_READ | GENERIC_WRITE, //read access required for FSCTL_SET_COMPRESSION
                                   FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                                   NULL,
                                   CREATE_NEW,
                                   (infoFileIn.dwFileAttributes & validAttribs) | FILE_FLAG_SEQUENTIAL_SCAN,
                                   NULL);
    if (hFileOut == INVALID_HANDLE_VALUE)
    {
        const DWORD lastError = ::GetLastError();
        const wxString& errorMessage = wxString(_("Error writing file:")) + wxT("\n\"") + zToWx(targetFile) + wxT("\"") +
                                       wxT("\n\n") + ffs3::getLastErrorFormatted(lastError);

        if (lastError == ERROR_FILE_EXISTS)
            throw ErrorTargetExisting(errorMessage);

        if (lastError == ERROR_PATH_NOT_FOUND)
            throw ErrorTargetPathMissing(errorMessage);

        throw FileError(errorMessage);
    }
    Loki::ScopeGuard guardTarget = Loki::MakeGuard(&removeFile, targetFile); //transactional behavior: guard just after opening target and before managing hFileOut

    Loki::ScopeGuard dummy = Loki::MakeGuard(::CloseHandle, hFileOut);
    (void)dummy; //silence warning "unused variable"


    const bool useBackupFun = !sourceIsEncrypted; //http://msdn.microsoft.com/en-us/library/aa362509(v=VS.85).aspx

    if (sourceIsCompressed && targetSupportCompressed)
    {
        USHORT cmpState = COMPRESSION_FORMAT_DEFAULT;

        DWORD bytesReturned = 0;
        if (!DeviceIoControl(hFileOut,              //handle to file or directory
                             FSCTL_SET_COMPRESSION, //dwIoControlCode
                             &cmpState,             //input buffer
                             sizeof(cmpState),      //size of input buffer
                             NULL,                  //lpOutBuffer
                             0,                     //OutBufferSize
                             &bytesReturned,        //number of bytes returned
                             NULL))                 //OVERLAPPED structure
            throw FileError(wxString(_("Error writing file:")) + wxT("\n\"") + zToWx(targetFile) + wxT("\"") +
                            wxT("\n\n") + ffs3::getLastErrorFormatted() +
                            wxT("\nFailed to write NTFS compressed attribute!"));
    }

    //although it seems the sparse attribute is set automatically by BackupWrite, we are required to do this manually: http://support.microsoft.com/kb/271398/en-us
    if (sourceIsSparse && targetSupportSparse)
    {
        if (useBackupFun)
        {
            DWORD bytesReturned = 0;
            if (!DeviceIoControl(hFileOut,         //handle to file
                                 FSCTL_SET_SPARSE, //dwIoControlCode
                                 NULL,             //input buffer
                                 0,                //size of input buffer
                                 NULL,             //lpOutBuffer
                                 0,                //OutBufferSize
                                 &bytesReturned,   //number of bytes returned
                                 NULL))            //OVERLAPPED structure
                throw FileError(wxString(_("Error writing file:")) + wxT("\n\"") + zToWx(targetFile) + wxT("\"") +
                                wxT("\n\n") + ffs3::getLastErrorFormatted() +
                                wxT("\nFailed to write NTFS sparse attribute!"));
        }
    }


    const DWORD BUFFER_SIZE = 512 * 1024; //512 kb seems to be a reasonable buffer size
    static const boost::scoped_array<BYTE> memory(new BYTE[BUFFER_SIZE]);

    struct ManageCtxt //manage context for BackupRead()/BackupWrite()
    {
        ManageCtxt() : read(NULL), write(NULL) {}
        ~ManageCtxt()
        {
            if (read != NULL)
                ::BackupRead (0, NULL, 0, NULL, true, false, &read);
            if (write != NULL)
                ::BackupWrite(0, NULL, 0, NULL, true, false, &write);
        }

        LPVOID read;
        LPVOID write;
    } context;

    //copy contents of sourceFile to targetFile
    wxULongLong totalBytesTransferred;

    bool eof = false;
    do
    {
        DWORD bytesRead = 0;

        if (useBackupFun)
        {
            if (!::BackupRead(hFileIn,        //__in   HANDLE hFile,
                              memory.get(),   //__out  LPBYTE lpBuffer,
                              BUFFER_SIZE,    //__in   DWORD nNumberOfBytesToRead,
                              &bytesRead,     //__out  LPDWORD lpNumberOfBytesRead,
                              false,          //__in   BOOL bAbort,
                              false,          //__in   BOOL bProcessSecurity,
                              &context.read)) //__out  LPVOID *lpContext
                throw FileError(wxString(_("Error reading file:")) + wxT("\n\"") + zToWx(sourceFile) + wxT("\"") +
                                wxT("\n\n") + ffs3::getLastErrorFormatted());
        }
        else if (!::ReadFile(hFileIn,      //__in         HANDLE hFile,
                             memory.get(), //__out        LPVOID lpBuffer,
                             BUFFER_SIZE,  //__in         DWORD nNumberOfBytesToRead,
                             &bytesRead,   //__out_opt    LPDWORD lpNumberOfBytesRead,
                             NULL))        //__inout_opt  LPOVERLAPPED lpOverlapped
            throw FileError(wxString(_("Error reading file:")) + wxT("\n\"") + zToWx(sourceFile) + wxT("\"") +
                            wxT("\n\n") + ffs3::getLastErrorFormatted());

        if (bytesRead > BUFFER_SIZE)
            throw FileError(wxString(_("Error reading file:")) + wxT("\n\"") + zToWx(sourceFile) + wxT("\"") +
                            wxT("\n\n") + wxT("buffer overflow"));

        if (bytesRead < BUFFER_SIZE)
            eof = true;

        DWORD bytesWritten = 0;

        if (useBackupFun)
        {
            if (!::BackupWrite(hFileOut,        //__in   HANDLE hFile,
                               memory.get(),    //__in   LPBYTE lpBuffer,
                               bytesRead,       //__in   DWORD nNumberOfBytesToWrite,
                               &bytesWritten,   //__out  LPDWORD lpNumberOfBytesWritten,
                               false,           //__in   BOOL bAbort,
                               false,           //__in   BOOL bProcessSecurity,
                               &context.write)) //__out  LPVOID *lpContext
                throw FileError(wxString(_("Error writing file:")) + wxT("\n\"") + zToWx(targetFile) + wxT("\"") +
                                wxT("\n\n") + ffs3::getLastErrorFormatted() + wxT(" (w)")); //w -> distinguish from fopen error message!
        }
        else if (!::WriteFile(hFileOut,      //__in         HANDLE hFile,
                              memory.get(),  //__out        LPVOID lpBuffer,
                              bytesRead,     //__in         DWORD nNumberOfBytesToWrite,
                              &bytesWritten, //__out_opt    LPDWORD lpNumberOfBytesWritten,
                              NULL))         //__inout_opt  LPOVERLAPPED lpOverlapped
            throw FileError(wxString(_("Error writing file:")) + wxT("\n\"") + zToWx(targetFile) + wxT("\"") +
                            wxT("\n\n") + ffs3::getLastErrorFormatted() + wxT(" (w)")); //w -> distinguish from fopen error message!

        if (bytesWritten != bytesRead)
            throw FileError(wxString(_("Error writing file:")) + wxT("\n\"") + zToWx(targetFile) + wxT("\"") +
                            wxT("\n\n") + wxT("incomplete write"));

        totalBytesTransferred += bytesRead;

        //invoke callback method to update progress indicators
        if (callback != NULL)
            switch (callback->updateCopyStatus(totalBytesTransferred))
            {
                case CallbackCopyFile::CONTINUE:
                    break;

                case CallbackCopyFile::CANCEL: //a user aborted operation IS an error condition!
                    throw FileError(wxString(_("Error copying file:")) + wxT("\n\"") + zToWx(sourceFile) +  wxT("\" ->\n\"") +
                                    zToWx(targetFile) + wxT("\"\n\n") + _("Operation aborted!"));
            }
    }
    while (!eof);


    if (totalBytesTransferred == 0) //BackupRead silently fails reading encrypted files -> double check!
    {
        LARGE_INTEGER inputSize = {};
        if (!::GetFileSizeEx(hFileIn, &inputSize))
            throw FileError(wxString(_("Error reading file attributes:")) + wxT("\n\"") + zToWx(sourceFile) + wxT("\"") +
                            wxT("\n\n") + ffs3::getLastErrorFormatted());

        if (inputSize.QuadPart != 0)
            throw FileError(wxString(_("Error reading file:")) + wxT("\n\"") + zToWx(sourceFile) + wxT("\"") +
                            wxT("\n\n") + wxT("unknown error"));
    }

    //time needs to be set at the end: BackupWrite() changes file time
    if (!::SetFileTime(hFileOut,
                       &infoFileIn.ftCreationTime,
                       NULL,
                       &infoFileIn.ftLastWriteTime))
        throw FileError(wxString(_("Error changing modification time:")) + wxT("\n\"") + zToWx(targetFile) + wxT("\"") +
                        wxT("\n\n") + ffs3::getLastErrorFormatted());


#ifndef NDEBUG //dst hack: verify data written
    if (dst::isFatDrive(targetFile)) //throw()
    {
        WIN32_FILE_ATTRIBUTE_DATA debugeAttr = {};
        assert(::GetFileAttributesEx(applyLongPathPrefix(targetFile).c_str(), //__in   LPCTSTR lpFileName,
                                     GetFileExInfoStandard,                   //__in   GET_FILEEX_INFO_LEVELS fInfoLevelId,
                                     &debugeAttr));                           //__out  LPVOID lpFileInformation

        assert(::CompareFileTime(&debugeAttr.ftCreationTime,  &infoFileIn.ftCreationTime)  == 0);
        assert(::CompareFileTime(&debugeAttr.ftLastWriteTime, &infoFileIn.ftLastWriteTime) == 0);
    }
#endif

    guardTarget.Dismiss();

    /*
        //create test sparse file
        size_t sparseSize = 50 * 1024 * 1024;
        HANDLE hSparse = ::CreateFile(L"C:\\sparse.file",
                                      GENERIC_READ | GENERIC_WRITE, //read access required for FSCTL_SET_COMPRESSION
                                      FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                                      NULL,
                                      CREATE_NEW,
                                      FILE_FLAG_SEQUENTIAL_SCAN,
                                      NULL);
        DWORD br = 0;
        if (!::DeviceIoControl(hSparse, FSCTL_SET_SPARSE, NULL, 0, NULL, 0, &br,NULL))
            throw 1;

        LARGE_INTEGER liDistanceToMove =  {};
        liDistanceToMove.QuadPart = sparseSize;
        if (!::SetFilePointerEx(hSparse, liDistanceToMove, NULL, FILE_BEGIN))
            throw 1;

        if (!SetEndOfFile(hSparse))
            throw 1;

        FILE_ZERO_DATA_INFORMATION zeroInfo = {};
        zeroInfo.BeyondFinalZero.QuadPart = sparseSize;
        if (!::DeviceIoControl(hSparse, FSCTL_SET_ZERO_DATA, &zeroInfo, sizeof(zeroInfo), NULL, 0, &br, NULL))
            throw 1;

        ::CloseHandle(hSparse);
        */
}
#endif


void rawCopyStream(const Zstring& sourceFile,
                   const Zstring& targetFile,
                   CallbackCopyFile* callback) //throw (FileError: ErrorTargetPathMissing, ErrorTargetExisting)
{
    Loki::ScopeGuard guardTarget = Loki::MakeGuard(&removeFile, targetFile); //transactional behavior: place guard before lifetime of FileOutput
    try
    {
        //open sourceFile for reading
        FileInput fileIn(sourceFile); //throw (FileError)

        //create targetFile and open it for writing
        FileOutput fileOut(targetFile, FileOutput::ACC_CREATE_NEW); //throw (FileError: ErrorTargetPathMissing, ErrorTargetExisting)

        const size_t BUFFER_SIZE = 512 * 1024; //512 kb seems to be a reasonable buffer size
        static const boost::scoped_array<char> memory(new char[BUFFER_SIZE]);

        //copy contents of sourceFile to targetFile
        wxULongLong totalBytesTransferred;
        do
        {
            const size_t bytesRead = fileIn.read(memory.get(), BUFFER_SIZE); //throw (FileError)

            fileOut.write(memory.get(), bytesRead); //throw (FileError)

            totalBytesTransferred += bytesRead;

            //invoke callback method to update progress indicators
            if (callback != NULL)
                switch (callback->updateCopyStatus(totalBytesTransferred))
                {
                    case CallbackCopyFile::CONTINUE:
                        break;

                    case CallbackCopyFile::CANCEL: //a user aborted operation IS an error condition!
                        throw FileError(wxString(_("Error copying file:")) + wxT("\n\"") + zToWx(sourceFile) +  wxT("\" ->\n\"") +
                                        zToWx(targetFile) + wxT("\"\n\n") + _("Operation aborted!"));
                }
        }
        while (!fileIn.eof());
    }
    catch(ErrorTargetExisting&)
    {
        guardTarget.Dismiss(); //don't delete file that existed previously!
        throw;
    }

    //adapt file modification time:
    copyFileTimes(sourceFile, targetFile, true); //throw (FileError)

    guardTarget.Dismiss(); //target has been created successfully!
}


inline
void copyFileImpl(const Zstring& sourceFile,
                  const Zstring& targetFile,
                  CallbackCopyFile* callback) //throw (FileError: ErrorTargetPathMissing, ErrorTargetExisting, ErrorFileLocked)
{
#ifdef FFS_WIN
    /*
                    rawCopyWinApi()	rawCopyWinOptimized()
        			-------------------------------------
        Attributes  YES			    YES
        Filetimes   YES			    YES
        ADS			YES			    YES
        Encrypted	YES			    YES
        Compressed	NO              YES
        Sparse		NO              YES
        PERF		-               6% faster
    */

    //rawCopyWinApi(sourceFile, targetFile, callback);     //throw (FileError: ErrorTargetPathMissing, ErrorTargetExisting, ErrorFileLocked)
    rawCopyWinOptimized(sourceFile, targetFile, callback); //throw (FileError: ErrorTargetPathMissing, ErrorTargetExisting, ErrorFileLocked) ->about 8% faster

#elif defined FFS_LINUX
    rawCopyStream(sourceFile, targetFile, callback); //throw (FileError: ErrorTargetPathMissing, ErrorTargetExisting)
#endif
}
}


void ffs3::copyFile(const Zstring& sourceFile, //throw (FileError: ErrorTargetPathMissing, ErrorFileLocked);
                    const Zstring& targetFile,
                    bool copyFilePermissions,
                    CallbackCopyFile* callback)
{
    Zstring temporary = targetFile + ffs3::TEMP_FILE_ENDING; //use temporary file until a correct date has been set
    Loki::ScopeGuard guardTempFile = Loki::MakeGuard(&removeFile, boost::cref(temporary)); //transactional behavior: ensure cleanup (e.g. network drop) -> cref [!]

    //raw file copy
    try
    {
        copyFileImpl(sourceFile, temporary, callback); //throw (FileError: ErrorTargetPathMissing, ErrorTargetExisting, ErrorFileLocked)
    }
    catch (ErrorTargetExisting&)
    {
        //determine non-used temp file name "first":
        //using optimistic strategy: assume everything goes well, but recover on error -> minimize file accesses
        temporary = createTempName(targetFile);

        //retry
        copyFileImpl(sourceFile, temporary, callback); //throw (FileError)
    }

    //have target file deleted (after read access on source and target has been confirmed) => allow for almost transactional overwrite
    if (callback) callback->deleteTargetFile(targetFile);

    //rename temporary file:
    //perf: this call is REALLY expensive on unbuffered volumes! ~40% performance decrease on FAT USB stick!
    renameFile(temporary, targetFile); //throw (FileError)

    guardTempFile.Dismiss();
    Loki::ScopeGuard guardTargetFile = Loki::MakeGuard(&removeFile, targetFile);

    //perf: interestingly it is much faster to apply file times BEFORE renaming temporary!

    //set file permissions
    if (copyFilePermissions)
        copyObjectPermissions(sourceFile, targetFile, true); //throw (FileError)

    guardTargetFile.Dismiss(); //target has been created successfully!
}
