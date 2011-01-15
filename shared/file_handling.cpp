// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) 2008-2011 ZenJu (zhnmju123 AT gmx.de)                    *
// **************************************************************************
//
#include "file_handling.h"
#include <wx/intl.h>
#include "system_func.h"
#include "global_func.h"
#include "system_constants.h"
#include "file_traverser.h"
#include <boost/bind.hpp>
#include <algorithm>
#include <wx/datetime.h>
#include "string_conv.h"
#include <wx/utils.h>
#include <boost/scoped_array.hpp>
#include <boost/shared_ptr.hpp>
#include <stdexcept>
#include "loki/TypeManip.h"
#include "loki/ScopeGuard.h"
#include <map>
#include "symlink_target.h"

#ifdef FFS_WIN
#include "privilege.h"
#include "dll_loader.h"
#include <wx/msw/wrapwin.h> //includes "windows.h"
#include "long_path_prefix.h"
#include <Aclapi.h>
#include "dst_hack.h"

#elif defined FFS_LINUX
#include <sys/stat.h>
#include "file_io.h"
#include <time.h>
#include <utime.h>
#include <cerrno>
#include <sys/time.h>

#ifdef HAVE_SELINUX
#include <selinux/selinux.h>
#endif
#endif

using ffs3::FileError;


namespace
{
#ifdef FFS_WIN
Zstring resolveRelativePath(const Zstring& relativeName, DWORD proposedBufferSize = 1000)
{
    boost::scoped_array<Zchar> fullPath(new Zchar[proposedBufferSize]);
    const DWORD rv = ::GetFullPathName(
                         relativeName.c_str(), //__in   LPCTSTR lpFileName,
                         proposedBufferSize,   //__in   DWORD nBufferLength,
                         fullPath.get(),       //__out  LPTSTR lpBuffer,
                         NULL);                //__out  LPTSTR *lpFilePart
    if (rv == 0 || rv == proposedBufferSize)
        //ERROR! Don't do anything
        return relativeName;
    if (rv > proposedBufferSize)
        return resolveRelativePath(relativeName, rv);

    return fullPath.get();
}

#elif defined FFS_LINUX
Zstring resolveRelativePath(const Zstring& relativeName) //additional: resolves symbolic links!!!
{
    char absolutePath[PATH_MAX + 1];
    if (::realpath(relativeName.c_str(), absolutePath) == NULL)
        //ERROR! Don't do anything
        return relativeName;

    return Zstring(absolutePath);
}
#endif


bool replaceMacro(wxString& macro) //macro without %-characters, return true if replaced successfully
{
    if (macro.IsEmpty())
        return false;

    //there are equally named environment variables %TIME%, %DATE% existing, so replace these first!
    if (macro.CmpNoCase(wxT("time")) == 0)
    {
        macro = wxDateTime::Now().FormatISOTime();
        macro.Replace(wxT(":"), wxT(""));
        return true;
    }

    if (macro.CmpNoCase(wxT("date")) == 0)
    {
        macro = wxDateTime::Now().FormatISODate();
        return true;
    }

    if (macro.CmpNoCase(wxT("month")) == 0)
    {
        macro = wxDateTime::Now().Format(wxT("%B"));
        return true;
    }

    if (macro.CmpNoCase(wxT("week")) == 0)
    {
        macro = wxDateTime::Now().Format(wxT("%U"));
        return true;
    }

    if (macro.CmpNoCase(wxT("year")) == 0)
    {
        macro = wxDateTime::Now().Format(wxT("%Y"));
        return true;
    }

    //try to apply environment variables
    wxString envValue;
    if (wxGetEnv(macro, &envValue))
    {
        macro = envValue;

        //some postprocessing:
        macro.Trim(true);  //remove leading, trailing blanks
        macro.Trim(false); //

        //remove leading, trailing double-quotes
        if (    macro.StartsWith(wxT("\"")) &&
                macro.EndsWith(wxT("\"")) &&
                macro.length() >= 2)
            macro = wxString(macro.c_str() + 1, macro.length() - 2);
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
                rest = SEPARATOR + rest;
                expandMacros(rest);
                text = prefix + SEPARATOR + potentialMacro + rest;
            }
        }
    }
}
}


Zstring ffs3::getFormattedDirectoryName(const Zstring& dirname)
{
    //Formatting is needed since functions expect the directory to end with '\' to be able to split the relative names.
    //note: don't combine directory formatting with wxFileName, as it doesn't respect //?/ - prefix!

    wxString dirnameTmp = zToWx(dirname);
    expandMacros(dirnameTmp);

    Zstring output = wxToZ(dirnameTmp);

    output.Trim();

    if (output.empty()) //an empty string will later be returned as "\"; this is not desired
        return Zstring();

    /*
    resolve relative names; required by:
    WINDOWS:
     - \\?\-prefix which needs absolute names
     - Volume Shadow Copy: volume name needs to be part of each filename
     - file icon buffer (at least for extensions that are actually read from disk, e.g. "exe")
     - ::SHFileOperation(): Using relative path names is not thread safe
    WINDOWS/LINUX:
     - detection of dependent directories, e.g. "\" and "C:\test"
     */
    output = resolveRelativePath(output);

    if (!output.EndsWith(common::FILE_NAME_SEPARATOR))
        output += common::FILE_NAME_SEPARATOR;

    return output;
}


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

#include <wx/msgdlg.h>
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
    if (hFile != INVALID_HANDLE_VALUE)
    {
        boost::shared_ptr<void> dummy(hFile, ::CloseHandle);

        BY_HANDLE_FILE_INFORMATION fileInfoByHandle;
        if (::GetFileInformationByHandle(hFile, &fileInfoByHandle))
            return wxULongLong(fileInfoByHandle.nFileSizeHigh, fileInfoByHandle.nFileSizeLow);
    }

    const wxString errorMessage = wxString(_("Error reading file attributes:")) + wxT("\n\"") + ffs3::zToWx(linkName) + wxT("\"");
    throw FileError(errorMessage + wxT("\n\n") + ffs3::getLastErrorFormatted());
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

    //pathName need not exist!
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

    struct stat fileInfo;
    while (::lstat(volumePathName.c_str(), &fileInfo) != 0)
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
    //no error situation if file is not existing! manual deletion relies on it!
    if (!somethingExists(filename))
        return; //neither file nor any other object (e.g. broken symlink) with that name existing

#ifdef FFS_WIN
    const Zstring filenameFmt = applyLongPathPrefix(filename);

    //remove file, support for \\?\-prefix
    if (!::DeleteFile(filenameFmt.c_str()))
    {
        //optimization: change file attributes ONLY when necessary!
        if (::GetLastError() == ERROR_ACCESS_DENIED) //function fails if file is read-only
        {
            //initialize file attributes
            if (::SetFileAttributes(filenameFmt.c_str(),    //address of filename
                                    FILE_ATTRIBUTE_NORMAL)) //attributes to set
            {
                //now try again...
                if (::DeleteFile(filenameFmt.c_str()))
                    return;
            }
        }

        wxString errorMessage = wxString(_("Error deleting file:")) + wxT("\n\"") + zToWx(filename) + wxT("\"");
        throw FileError(errorMessage + wxT("\n\n") + ffs3::getLastErrorFormatted());
    }
#elif defined FFS_LINUX
    if (::unlink(filename.c_str()) != 0)
    {
        wxString errorMessage = wxString(_("Error deleting file:")) + wxT("\n\"") + zToWx(filename) + wxT("\"");
        throw FileError(errorMessage + wxT("\n\n") + ffs3::getLastErrorFormatted());
    }
#endif
}


namespace
{
struct ErrorDifferentVolume : public ffs3::FileError
{
    ErrorDifferentVolume(const wxString& message) : FileError(message) {}
};

/* Usage overview:

  renameFile()  -->  renameFileInternal()
      |               /|\
     \|/               |
      fix8Dot3NameClash()
*/
//wrapper for file system rename function:
//throw (FileError); ErrorDifferentVolume if it is due to moving file to another volume
void renameFileInternal(const Zstring& oldName, const Zstring& newName) //throw (FileError, ErrorDifferentVolume)
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
                if (::SetFileAttributes(oldNameFmt.c_str(),     //address of filename
                                        FILE_ATTRIBUTE_NORMAL)) //remove readonly-attribute
                {
                    //try again...
                    if (::MoveFileEx(oldNameFmt.c_str(), //__in      LPCTSTR lpExistingFileName,
                                     newNameFmt.c_str(), //__in_opt  LPCTSTR lpNewFileName,
                                     0))                 //__in      DWORD dwFlags
                    {
                        //(try to) restore file attributes
                        ::SetFileAttributes(newNameFmt.c_str(), //don't handle error
                                            oldNameAttrib);
                        return;
                    }
                    else
                    {
                        const DWORD errorCode = ::GetLastError();
                        //cleanup: (try to) restore file attributes: assume oldName is still existing
                        ::SetFileAttributes(oldNameFmt.c_str(),
                                            oldNameAttrib);

                        ::SetLastError(errorCode); //set error code from ::MoveFileEx()
                    }
                }
            }
        }

        const wxString errorMessage = wxString(_("Error moving file:")) + wxT("\n\"") + zToWx(oldName) +  wxT("\" ->\n\"") + zToWx(newName) + wxT("\"") +
                                      wxT("\n\n") + ffs3::getLastErrorFormatted();
        if (::GetLastError() == ERROR_NOT_SAME_DEVICE)
            throw ErrorDifferentVolume(errorMessage);
        else
            throw FileError(errorMessage);
    }

#elif defined FFS_LINUX
    //rename temporary file
    if (::rename(oldName.c_str(), newName.c_str()) != 0)
    {
        const wxString errorMessage = wxString(_("Error moving file:")) + wxT("\n\"") + zToWx(oldName) +  wxT("\" ->\n\"") + zToWx(newName) + wxT("\"") +
                                      wxT("\n\n") + ffs3::getLastErrorFormatted();
        if (errno == EXDEV)
            throw ErrorDifferentVolume(errorMessage);
        else
            throw FileError(errorMessage);
    }
#endif
}


void renameFileInternalNoThrow(const Zstring& oldName, const Zstring& newName) //throw ()
{
    try
    {
        ::renameFileInternal(oldName, newName);
    }
    catch (...) {}
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
        const Zstring fileNameLong  = getFilenameFmt(newName, ::GetLongPathName).AfterLast(common::FILE_NAME_SEPARATOR);  //throw() returns empty string on error

        if (    !fileNameShort.empty() &&
                !fileNameLong.empty()  &&
                EqualFilename()(fileNameOrig,  fileNameShort) &&
                !EqualFilename()(fileNameShort, fileNameLong))
        {
            //we detected an event where newName is in shortname format (although it is intended to be a long name) and
            //writing target file failed because another unrelated file happens to have the same short name

            const Zstring unrelatedPathLong = newName.BeforeLast(common::FILE_NAME_SEPARATOR) + common::FILE_NAME_SEPARATOR +
                                              fileNameLong;

            //find another name in short format: this ensures the actual short name WILL be renamed as well!
            const Zstring parkedTarget = createTemp8Dot3Name(newName);

            //move already existing short name out of the way for now
            renameFileInternal(unrelatedPathLong, parkedTarget); //throw (FileError, ErrorDifferentVolume);
            //DON'T call ffs3::renameFile() to avoid reentrance!

            //schedule cleanup; the file system should assign this unrelated file a new (unique) short name
            Loki::ScopeGuard guard = Loki::MakeGuard(renameFileInternalNoThrow, parkedTarget, unrelatedPathLong);//equivalent to Boost.ScopeExit in this case
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
void ffs3::renameFile(const Zstring& oldName, const Zstring& newName) //throw (FileError, ErrorDifferentVolume);
{
    try
    {
        renameFileInternal(oldName, newName); //throw (FileError, ErrorDifferentVolume)
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


using ffs3::MoveFileCallback;

class CopyCallbackImpl : public ffs3::CopyFileCallback //callback functionality
{
public:
    CopyCallbackImpl(const Zstring& sourceFile, MoveFileCallback& callback) : sourceFile_(sourceFile), moveCallback(callback) {}

    virtual Response updateCopyStatus(const wxULongLong& totalBytesTransferred)
    {
        switch (moveCallback.requestUiRefresh(sourceFile_))
        {
        case MoveFileCallback::CONTINUE:
            return CopyFileCallback::CONTINUE;

        case MoveFileCallback::CANCEL:
            return CopyFileCallback::CANCEL;
        }
        return CopyFileCallback::CONTINUE; //dummy return value
    }

private:
    const Zstring sourceFile_;
    MoveFileCallback& moveCallback;
};


void ffs3::moveFile(const Zstring& sourceFile, const Zstring& targetFile, MoveFileCallback* callback)   //throw (FileError);
{
    //call back once per file (moveFile() is called by moveDirectory())
    if (callback)
        switch (callback->requestUiRefresh(sourceFile))
        {
        case MoveFileCallback::CONTINUE:
            break;
        case MoveFileCallback::CANCEL: //a user aborted operation IS an error condition!
            throw FileError(wxString(_("Error moving file:")) + wxT("\n\"") + zToWx(sourceFile) +  wxT("\" ->\n\"") + zToWx(targetFile) + wxT("\"") +
                            wxT("\n\n") + _("Operation aborted!"));
        }

    //support case-sensitive renaming
    if (EqualFilename()(sourceFile, targetFile)) //difference in case only
        return renameFile(sourceFile, targetFile); //throw (FileError, ErrorDifferentVolume);

    if (somethingExists(targetFile)) //test file existence: e.g. Linux might silently overwrite existing symlinks
        throw FileError(wxString(_("Error moving file:")) + wxT("\n\"") + zToWx(sourceFile) +  wxT("\" ->\n\"") + zToWx(targetFile) + wxT("\"") +
                        wxT("\n\n") + _("Target file already existing!"));

    //moving of symbolic links should work correctly:

    //first try to move the file directly without copying
    try
    {
        renameFile(sourceFile, targetFile); //throw (FileError, ErrorDifferentVolume);
        return;
    }
    //if moving failed treat as error (except when it tried to move to a different volume: in this case we will copy the file)
    catch (const ErrorDifferentVolume&) {}


    //file is on a different volume: let's copy it
    std::auto_ptr<CopyCallbackImpl> copyCallback(callback != NULL ? new CopyCallbackImpl(sourceFile, *callback) : NULL);

    copyFile(sourceFile,
             targetFile,
             true, //copy symbolic links
             false, //dont copy filesystem permissions
#ifdef FFS_WIN
             NULL, //supply handler for making shadow copies
#endif
             copyCallback.get()); //throw (FileError);

    //attention: if copy-operation was cancelled an exception is thrown => sourcefile is not deleted, as we wish!

    removeFile(sourceFile);
}

namespace
{
class TraverseOneLevel : public ffs3::TraverseCallback
{
public:
    typedef std::vector<std::pair<Zstring, Zstring> > NamePair;

    TraverseOneLevel(NamePair& filesShort, NamePair& dirsShort) :
        m_files(filesShort),
        m_dirs(dirsShort) {}

    virtual void onFile(const Zchar* shortName, const Zstring& fullName, const FileInfo& details)
    {
        m_files.push_back(std::make_pair(Zstring(shortName), fullName));
    }
    virtual void onSymlink(const Zchar* shortName, const Zstring& fullName, const SymlinkInfo& details)
    {
        if (details.dirLink)
            m_dirs.push_back(std::make_pair(Zstring(shortName), fullName));
        else
            m_files.push_back(std::make_pair(Zstring(shortName), fullName));
    }

    virtual ReturnValDir onDir(const Zchar* shortName, const Zstring& fullName)
    {
        m_dirs.push_back(std::make_pair(Zstring(shortName), fullName));
        return Loki::Int2Type<ReturnValDir::TRAVERSING_DIR_IGNORE>(); //DON'T traverse into subdirs; moveDirectory works recursively!
    }
    virtual void onError(const wxString& errorText)
    {
        throw FileError(errorText);
    }

private:
    NamePair& m_files;
    NamePair& m_dirs;
};


struct RemoveCallbackImpl : public ffs3::RemoveDirCallback
{
    RemoveCallbackImpl(const Zstring& sourceDir,
                       const Zstring& targetDir,
                       MoveFileCallback& moveCallback) :
        sourceDir_(sourceDir),
        targetDir_(targetDir),
        moveCallback_(moveCallback) {}

    virtual void requestUiRefresh(const Zstring& currentObject)
    {
        switch (moveCallback_.requestUiRefresh(sourceDir_))
        {
        case MoveFileCallback::CONTINUE:
            break;
        case MoveFileCallback::CANCEL: //a user aborted operation IS an error condition!
            throw ffs3::FileError(wxString(_("Error moving directory:")) + wxT("\n\"") + ffs3::zToWx(sourceDir_) +  wxT("\" ->\n\"") +
                                  ffs3::zToWx(targetDir_) + wxT("\"") + wxT("\n\n") + _("Operation aborted!"));
        }
    }

private:
    const Zstring sourceDir_;
    const Zstring targetDir_;
    MoveFileCallback& moveCallback_;
};
}


void moveDirectoryImpl(const Zstring& sourceDir, const Zstring& targetDir, bool ignoreExistingDirs, MoveFileCallback* callback)   //throw (FileError);
{
    using namespace ffs3;

    //call back once per folder
    if (callback)
        switch (callback->requestUiRefresh(sourceDir))
        {
        case MoveFileCallback::CONTINUE:
            break;
        case MoveFileCallback::CANCEL: //a user aborted operation IS an error condition!
            throw FileError(wxString(_("Error moving directory:")) + wxT("\n\"") + zToWx(sourceDir) +  wxT("\" ->\n\"") +
                            zToWx(targetDir) + wxT("\"") + wxT("\n\n") + _("Operation aborted!"));
        }

    //handle symbolic links
    if (symlinkExists(sourceDir))
    {
        createDirectory(targetDir, sourceDir, true, false); //copy symbolic link, don't copy permissions
        removeDirectory(sourceDir, NULL);           //if target is already another symlink or directory, sourceDir-symlink is silently deleted
        return;
    }

    if (somethingExists(targetDir))
    {
        if (!ignoreExistingDirs) //directory or symlink exists (or even a file... this error will be caught later)
            throw FileError(wxString(_("Error moving directory:")) + wxT("\n\"") + zToWx(sourceDir) +  wxT("\" ->\n\"") + zToWx(targetDir) + wxT("\"") +
                            wxT("\n\n") + _("Target directory already existing!"));
    }
    else
    {
        //first try to move the directory directly without copying
        try
        {
            renameFile(sourceDir, targetDir); //throw (FileError, ErrorDifferentVolume);
            return;
        }
        //if moving failed treat as error (except when it tried to move to a different volume: in this case we will copy the directory)
        catch (const ErrorDifferentVolume&) {}

        //create target
        createDirectory(targetDir, sourceDir, false, false); //throw (FileError); don't copy permissions
    }

    //move files/folders recursively
    TraverseOneLevel::NamePair fileList; //list of names: 1. short 2.long
    TraverseOneLevel::NamePair dirList;  //

    //traverse source directory one level
    TraverseOneLevel traverseCallback(fileList, dirList);
    traverseFolder(sourceDir, false, traverseCallback); //traverse one level, don't follow symlinks

    const Zstring targetDirFormatted = targetDir.EndsWith(common::FILE_NAME_SEPARATOR) ? //ends with path separator
                                       targetDir :
                                       targetDir + common::FILE_NAME_SEPARATOR;

    //move files
    for (TraverseOneLevel::NamePair::const_iterator i = fileList.begin(); i != fileList.end(); ++i)
        ffs3::moveFile(i->second, targetDirFormatted + i->first, callback);

    //move directories
    for (TraverseOneLevel::NamePair::const_iterator i = dirList.begin(); i != dirList.end(); ++i)
        ::moveDirectoryImpl(i->second, targetDirFormatted + i->first, true, callback);

    //attention: if move-operation was cancelled an exception is thrown => sourceDir is not deleted, as we wish!

    //delete source
    std::auto_ptr<RemoveCallbackImpl> removeCallback(callback != NULL ? new RemoveCallbackImpl(sourceDir, targetDir, *callback) : NULL);
    removeDirectory(sourceDir, removeCallback.get()); //throw (FileError);
}


void ffs3::moveDirectory(const Zstring& sourceDir, const Zstring& targetDir, bool ignoreExistingDirs, MoveFileCallback* callback)   //throw (FileError);
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

    ::moveDirectoryImpl(sourceDirFormatted, targetDirFormatted, ignoreExistingDirs, callback);
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


void ffs3::removeDirectory(const Zstring& directory, RemoveDirCallback* callback)
{
    //no error situation if directory is not existing! manual deletion relies on it!
    if (!somethingExists(directory))
        return; //neither directory nor any other object (e.g. broken symlink) with that name existing

#ifdef FFS_WIN
    const Zstring directoryFmt = applyLongPathPrefix(directory); //support for \\?\-prefix

    //initialize file attributes
    if (!::SetFileAttributes(           // initialize file attributes: actually NEEDED for symbolic links also!
                directoryFmt.c_str(),   // address of directory name
                FILE_ATTRIBUTE_NORMAL)) // attributes to set
    {
        wxString errorMessage = wxString(_("Error deleting directory:")) + wxT("\n\"") + directory.c_str() + wxT("\"");
        throw FileError(errorMessage + wxT("\n\n") + ffs3::getLastErrorFormatted());
    }
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
    FILETIME lastAccessTime = {};
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
            boost::shared_ptr<void> dummy(hSource, ::CloseHandle);

            if (!::GetFileTime(hSource,        //__in       HANDLE hFile,
                               &creationTime,   //__out_opt  LPFILETIME lpCreationTime,
                               &lastAccessTime, //__out_opt  LPFILETIME lpLastAccessTime,
                               &lastWriteTime)) //__out_opt  LPFILETIME lpLastWriteTime
            {
                const wxString errorMessage = wxString(_("Error reading file attributes:")) + wxT("\n\"") + zToWx(sourceObj) + wxT("\"");
                throw FileError(errorMessage + wxT("\n\n") + ffs3::getLastErrorFormatted());
            }
        }
        else
        {
            creationTime   = sourceAttr.ftCreationTime;
            lastAccessTime = sourceAttr.ftLastAccessTime;
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
    boost::shared_ptr<void> dummy(hTarget, ::CloseHandle);

    if (!::SetFileTime(hTarget,
                       &creationTime,
                       &lastAccessTime,
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
        struct stat objInfo;
        if (::stat(sourceObj.c_str(), &objInfo) != 0) //read file attributes from source directory
        {
            const wxString errorMessage = wxString(_("Error reading file attributes:")) + wxT("\n\"") + zToWx(sourceObj) + wxT("\"");
            throw FileError(errorMessage + wxT("\n\n") + ffs3::getLastErrorFormatted());
        }

        struct utimbuf newTimes;
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
        struct stat objInfo;
        if (::lstat(sourceObj.c_str(), &objInfo) != 0) //read file attributes from source directory
        {
            const wxString errorMessage = wxString(_("Error reading file attributes:")) + wxT("\n\"") + zToWx(sourceObj) + wxT("\"");
            throw FileError(errorMessage + wxT("\n\n") + ffs3::getLastErrorFormatted());
        }

        struct timeval newTimes[2];
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

    boost::shared_ptr<void> dummy(hDir, ::CloseHandle);

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


#elif defined FFS_LINUX
void copySymlinkInternal(const Zstring& sourceLink, const Zstring& targetLink, bool copyFilePermissions) //throw (FileError)
{
    using namespace ffs3;

    //copy symbolic link
    const int BUFFER_SIZE = 10000;
    char buffer[BUFFER_SIZE];
    const int bytesWritten = ::readlink(sourceLink.c_str(), buffer, BUFFER_SIZE);
    if (bytesWritten < 0 || bytesWritten == BUFFER_SIZE)
    {
        wxString errorMessage = wxString(_("Error resolving symbolic link:")) + wxT("\n\"") + zToWx(sourceLink) + wxT("\"");
        if (bytesWritten < 0) errorMessage += wxString(wxT("\n\n")) + ffs3::getLastErrorFormatted();
        throw FileError(errorMessage);
    }
    //set null-terminating char
    buffer[bytesWritten] = 0;

    if (::symlink(buffer, targetLink.c_str()) != 0)
    {
        const wxString errorMessage = wxString(_("Error copying symbolic link:")) + wxT("\n\"") + zToWx(sourceLink) +  wxT("\" ->\n\"") + zToWx(targetLink) + wxT("\"");
        throw FileError(errorMessage + wxT("\n\n") + ffs3::getLastErrorFormatted());
    }

    //allow only consistent objects to be created -> don't place before ::symlink, targetLink may already exist
    Loki::ScopeGuard guardTargetLink = Loki::MakeGuard(::unlink, targetLink);

    copyFileTimes(sourceLink, targetLink, false); //throw (FileError)

    if (copyFilePermissions)
        copyObjectPermissions(sourceLink, targetLink, false); //throw FileError()

    guardTargetLink.Dismiss();
}
#endif
}


#ifdef HAVE_SELINUX
//copy SELinux security context
void copySecurityContext(const Zstring& source, const Zstring& target, bool derefSymlinks) //throw FileError()
{
    using ffs3::zToWx;

    security_context_t contextSource = NULL;
    const int rv = derefSymlinks ?
                   ::getfilecon (source.c_str(), &contextSource) :
                   ::lgetfilecon(source.c_str(), &contextSource);
    if (rv < 0)
    {
        if (    errno == ENODATA ||  //no security context (allegedly) is not an error condition on SELinux
                errno == EOPNOTSUPP) //extended attributes are not supported by the filesystem
            return;

        wxString errorMessage = wxString(_("Error reading security context:")) + wxT("\n\"") + zToWx(source) + wxT("\"");
        throw FileError(errorMessage + wxT("\n\n") + ffs3::getLastErrorFormatted());
    }
    boost::shared_ptr<char> dummy1(contextSource, ::freecon);

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
            boost::shared_ptr<char> dummy2(contextTarget, ::freecon);
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


//copy permissions for files, directories or symbolic links:
void ffs3::copyObjectPermissions(const Zstring& source, const Zstring& target, bool derefSymlinks) //throw FileError(); probably requires admin rights
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
    boost::shared_ptr<void> dummy(hSource, ::CloseHandle);

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
    boost::shared_ptr<void> dummy2(buffer, ::LocalFree);


    const Zstring targetFmt = ffs3::applyLongPathPrefix(target);

    //read-only file attribute may cause trouble: temporarily reset it
    const DWORD targetAttr = ::GetFileAttributes(targetFmt.c_str());
    Loki::ScopeGuard resetAttributes = Loki::MakeGuard(::SetFileAttributes, targetFmt, targetAttr);
    if (    targetAttr != INVALID_FILE_ATTRIBUTES &&
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
    boost::shared_ptr<void> dummy3(hTarget, ::CloseHandle);

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
    copySecurityContext(source, target, derefSymlinks); //throw FileError()
#endif

    if (derefSymlinks)
    {
        struct stat fileInfo;
        if (    ::stat(source.c_str(), &fileInfo) != 0                        ||
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
        if (    ::lstat(source.c_str(), &fileInfo) != 0                        ||
                ::lchown(target.c_str(), fileInfo.st_uid, fileInfo.st_gid) != 0 || // may require admin rights!
                (!symlinkExists(target) && ::chmod(target.c_str(), fileInfo.st_mode) != 0)) //setting access permissions doesn't make sense for symlinks on Linux: there is no lchmod()
        {
            const wxString errorMessage = wxString(_("Error copying file permissions:")) + wxT("\n\"") + zToWx(source) +  wxT("\" ->\n\"") + zToWx(target) + wxT("\"") + wxT("\n\n");
            throw FileError(errorMessage + ffs3::getLastErrorFormatted() + wxT(" (W)"));
        }
    }
#endif
}


void createDirectoryRecursively(const Zstring& directory, const Zstring& templateDir, bool copyDirectorySymLinks, bool copyFilePermissions, int level)
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
        createDirectoryRecursively(dirParent, templateParent, false, copyFilePermissions, level + 1); //don't create symbolic links in recursion!
    }

    struct TryCleanUp
    {
        static void tryDeleteDir(const Zstring& dirname) //throw ()
        {
            try
            {
                removeDirectory(dirname, NULL);
            }
            catch (...) {}
        }
    };

    //now creation should be possible
#ifdef FFS_WIN
    const DWORD templateAttr = templateDir.empty() ? INVALID_FILE_ATTRIBUTES :
                               ::GetFileAttributes(applyLongPathPrefix(templateDir).c_str()); //returns successful for broken symlinks
    if (templateAttr == INVALID_FILE_ATTRIBUTES) //fallback
    {
        if (!::CreateDirectory(applyLongPathPrefixCreateDir(directory).c_str(), // pointer to a directory path string
                               NULL))
        {
            if (level != 0) return;
            const wxString errorMessage = wxString(_("Error creating directory:")) + wxT("\n\"") + directory.c_str() + wxT("\"");
            throw FileError(errorMessage + wxT("\n\n") + ffs3::getLastErrorFormatted());
        }
    }
    else
    {
        const bool isSymlink = (templateAttr & FILE_ATTRIBUTE_REPARSE_POINT) != 0; //syntax required to shut MSVC up

        //symbolic link handling
        if (isSymlink)
        {
            if (!copyDirectorySymLinks) //create directory based on target of symbolic link
            {
                //get target directory of symbolic link
                Zstring linkPath;
                try
                {
                    linkPath = resolveDirectorySymlink(templateDir); //throw (FileError)
                }
                catch (FileError&)
                {
                    //dereferencing a symbolic link usually fails if it is located on network drive or client is XP: NOT really an error...
                    if (!::CreateDirectory(applyLongPathPrefixCreateDir(directory).c_str(), // pointer to a directory path string
                                           NULL))
                    {
                        if (level != 0) return;
                        const wxString errorMessage = wxString(_("Error creating directory:")) + wxT("\n\"") + directory.c_str() + wxT("\"");
                        throw FileError(errorMessage + wxT("\n\n") + ffs3::getLastErrorFormatted());
                    }
                    return;
                }

                if (!::CreateDirectoryEx(      // this function automatically copies symbolic links if encountered
                            applyLongPathPrefix(linkPath).c_str(),           // pointer to path string of template directory
                            applyLongPathPrefixCreateDir(directory).c_str(), // pointer to a directory path string
                            NULL))
                {
                    if (level != 0) return;
                    const wxString errorMessage = wxString(_("Error creating directory:")) + wxT("\n\"") + directory.c_str() + wxT("\"");
                    throw FileError(errorMessage + wxT("\n\n") + ffs3::getLastErrorFormatted());
                }
            }
            else //copy symbolic link
            {
                const Zstring linkPath = getSymlinkRawTargetString(templateDir); //accept broken symlinks; throw (FileError)

                //dynamically load windows API function
                typedef BOOLEAN (WINAPI *CreateSymbolicLinkFunc)(
                    LPCTSTR lpSymlinkFileName,
                    LPCTSTR lpTargetFileName,
                    DWORD dwFlags);
                static const CreateSymbolicLinkFunc createSymbolicLink = util::getDllFun<CreateSymbolicLinkFunc>(L"kernel32.dll", "CreateSymbolicLinkW");
                if (createSymbolicLink == NULL)
                    throw FileError(wxString(_("Error loading library function:")) + wxT("\n\"") + wxT("CreateSymbolicLinkW") + wxT("\""));

                if (!createSymbolicLink( //seems no long path prefix is required...
                            directory.c_str(),             //__in  LPTSTR lpSymlinkFileName,
                            linkPath.c_str(),              //__in  LPTSTR lpTargetFileName,
                            SYMBOLIC_LINK_FLAG_DIRECTORY)) //__in  DWORD dwFlags
                {
                    //if (level != 0) return;
                    const wxString errorMessage = wxString(_("Error copying symbolic link:")) + wxT("\n\"") + templateDir.c_str() +  wxT("\" ->\n\"") + directory.c_str() + wxT("\"");
                    throw FileError(errorMessage + wxT("\n\n") + ffs3::getLastErrorFormatted());
                }
            }
        }
        else //no symbolic link
        {
            //automatically copies symbolic links if encountered: unfortunately it doesn't copy symlinks over network shares but silently creates empty folders instead (on XP)!
            //also it isn't able to copy most junctions because of missing permissions (although target path can be retrieved!)
            if (!::CreateDirectoryEx(
                        applyLongPathPrefix(templateDir).c_str(),          // pointer to path string of template directory
                        applyLongPathPrefixCreateDir(directory).c_str(),   // pointer to a directory path string
                        NULL))
            {
                if (level != 0) return;
                const wxString errorMessage = wxString(_("Error creating directory:")) + wxT("\n\"") + directory.c_str() + wxT("\"");
                throw FileError(errorMessage + wxT("\n\n") + ffs3::getLastErrorFormatted());
            }
        }

        //ensure cleanup:
        Loki::ScopeGuard guardNewDir = Loki::MakeGuard(&TryCleanUp::tryDeleteDir, directory);

        if (copyDirectorySymLinks && isSymlink) //we need to copy the Symbolic Link's change date manually
            copyFileTimes(templateDir, directory, false); //throw (FileError)

        if (copyFilePermissions)
            copyObjectPermissions(templateDir, directory, !copyDirectorySymLinks); //throw FileError()

        guardNewDir.Dismiss(); //target has been created successfully!
    }

#elif defined FFS_LINUX
    //symbolic link handling
    if (    copyDirectorySymLinks &&
            !templateDir.empty()  &&
            symlinkExists(templateDir))
        //there is no directory-type symlink in Linux! => just copy as file
        return copySymlinkInternal(templateDir, directory, copyFilePermissions); //throw (FileError)

    //default directory creation
    if (::mkdir(directory.c_str(), 0755) != 0)
    {
        if (level != 0) return;
        wxString errorMessage = wxString(_("Error creating directory:")) + wxT("\n\"") + zToWx(directory) + wxT("\"");
        throw FileError(errorMessage + wxT("\n\n") + ffs3::getLastErrorFormatted());
    }

    //ensure cleanup:
    Loki::ScopeGuard guardNewDir = Loki::MakeGuard(&TryCleanUp::tryDeleteDir, directory);

    if (!templateDir.empty() && copyFilePermissions)
        copyObjectPermissions(templateDir, directory, true); //throw FileError()

    guardNewDir.Dismiss(); //target has been created successfully!
#endif
}


void ffs3::createDirectory(const Zstring& directory, const Zstring& templateDir, bool copyDirectorySymLinks, bool copyFilePermissions)
{
    //remove trailing separator
    const Zstring dirFormatted = directory.EndsWith(common::FILE_NAME_SEPARATOR) ?
                                 directory.BeforeLast(common::FILE_NAME_SEPARATOR) :
                                 directory;

    const Zstring templateFormatted = templateDir.EndsWith(common::FILE_NAME_SEPARATOR) ?
                                      templateDir.BeforeLast(common::FILE_NAME_SEPARATOR) :
                                      templateDir;

    createDirectoryRecursively(dirFormatted, templateFormatted, copyDirectorySymLinks, copyFilePermissions, 0);
}


void ffs3::createDirectory(const Zstring& directory)
{
    ffs3::createDirectory(directory, Zstring(), false, false);
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
}

#ifdef FFS_WIN
namespace
{
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
    using ffs3::CopyFileCallback;

    //small performance optimization: it seems this callback function is called for every 64 kB (depending on cluster size).
    static size_t callNr = 0;
    if (++callNr % 4 == 0) //executing callback for each 256 kB should suffice
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
            try
            {
                switch (callback->updateCopyStatus(wxULongLong(totalBytesTransferred.HighPart, totalBytesTransferred.LowPart)))
                {
                case CopyFileCallback::CONTINUE:
                    break;
                case CopyFileCallback::CANCEL:
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
    OSVERSIONINFO osvi;
    ZeroMemory(&osvi, sizeof(OSVERSIONINFO));
    osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);

    //encrypted destination is not supported with Windows 2000
    if (::GetVersionEx(&osvi))
        return osvi.dwMajorVersion > 5 ||
               (osvi.dwMajorVersion == 5 && osvi.dwMinorVersion > 0); //2000 has majorVersion == 5, minorVersion == 0
    //overview: http://msdn.microsoft.com/en-us/library/ms724834(VS.85).aspx
    return false;
}
}


void ffs3::copyFile(const Zstring& sourceFile,
                    const Zstring& targetFile,
                    bool copyFileSymLinks,
                    bool copyFilePermissions,
                    shadow::ShadowCopy* shadowCopyHandler,
                    ffs3::CopyFileCallback* callback)
{
    //ffs3::fileExists(targetFile) -> avoid this call, performance;
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

    struct TryCleanUp //ensure cleanup if working with temporary failed!
    {
        static void tryDeleteFile(const Zstring& filename) //throw ()
        {
            try
            {
                removeFile(filename);
            }
            catch (...) {}
        }
    };
    Loki::ScopeGuard guardTempFile = Loki::MakeGuard(&TryCleanUp::tryDeleteFile, temporary);


    if (!::CopyFileEx( //same performance as CopyFile()
                applyLongPathPrefix(sourceFile).c_str(),
                applyLongPathPrefix(temporary).c_str(),
                callback != NULL ? copyCallbackInternal : NULL,
                callback,
                NULL,
                copyFlags))
    {
        const DWORD lastError = ::GetLastError();

        //don't suppress "lastError == ERROR_REQUEST_ABORTED": a user aborted operation IS an error condition!

        //if file is locked (try to) use Windows Volume Shadow Copy Service
        if (shadowCopyHandler != NULL &&
                (lastError == ERROR_SHARING_VIOLATION ||
                 lastError == ERROR_LOCK_VIOLATION))
        {
            //shadowFilename already contains prefix: E.g. "\\?\GLOBALROOT\Device\HarddiskVolumeShadowCopy1\Program Files\FFS\sample.dat"

            Zstring shadowFilename;
            try
            {
                shadowFilename = shadowCopyHandler->makeShadowCopy(sourceFile); //throw (FileError)
            }
            catch (const FileError& e)
            {
                wxString errorMsg = _("Error copying locked file %x!");
                errorMsg.Replace(wxT("%x"), wxString(wxT("\"")) + zToWx(sourceFile) + wxT("\""));
                errorMsg += wxT("\n\n") + e.msg();
                throw FileError(errorMsg);
            }

            return copyFile(shadowFilename, //transferred bytes is automatically reset when new file is copied
                            targetFile,
                            copyFileSymLinks,
                            copyFilePermissions,
                            NULL,
                            callback);
        }
        //assemble error message...
        const wxString errorMessage = wxString(_("Error copying file:")) + wxT("\n\"") + sourceFile.c_str() +  wxT("\" ->\n\"") + targetFile.c_str() + wxT("\"") + wxT("\n\n");
        throw FileError(errorMessage + ffs3::getLastErrorFormatted(lastError));
    }

    //rename temporary file: do not add anything else here (note specific error handing)
    ffs3::renameFile(temporary, targetFile);

    guardTempFile.Dismiss(); //no need to delete temp file anymore

    Loki::ScopeGuard guardTargetFile = Loki::MakeGuard(&TryCleanUp::tryDeleteFile, targetFile);

    if (copyFilePermissions)
        copyObjectPermissions(sourceFile, targetFile, !copyFileSymLinks); //throw FileError()

    //copy creation date (last modification date is REDUNDANTLY written, too, even for symlinks)
    copyFileTimes(sourceFile, targetFile, !copyFileSymLinks); //throw (FileError)

    guardTargetFile.Dismiss();
}


#elif defined FFS_LINUX
void ffs3::copyFile(const Zstring& sourceFile,
                    const Zstring& targetFile,
                    bool copyFileSymLinks,
                    bool copyFilePermissions,
                    CopyFileCallback* callback)
{
    using ffs3::CopyFileCallback;

    //symbolic link handling
    if (    copyFileSymLinks &&
            symlinkExists(sourceFile))
    {
        return copySymlinkInternal(sourceFile, targetFile, copyFilePermissions); //throw (FileError)
    }

    //begin of regular file copy
    struct stat fileInfo;
    if (::stat(sourceFile.c_str(), &fileInfo) != 0) //read file attributes from source file (resolving symlinks)
    {
        const wxString errorMessage = wxString(_("Error reading file attributes:")) + wxT("\n\"") + zToWx(sourceFile) + wxT("\"");
        throw FileError(errorMessage + wxT("\n\n") + ffs3::getLastErrorFormatted());
    }

    //open sourceFile for reading
    FileInput fileIn(sourceFile); //throw FileError()

    //create targetFile and open it for writing
    const Zstring temporary = createTempName(targetFile); //use temporary file until a correct date has been set

    //ensure cleanup (e.g. network drop): call BEFORE creating fileOut object!
    Loki::ScopeGuard guardTempFile = Loki::MakeGuard(::unlink, temporary);

    FileOutput fileOut(temporary); //throw FileError()

    const size_t BUFFER_SIZE = 512 * 1024; //512 kb seems to be a reasonable buffer size
    static const boost::scoped_array<char> memory(new char[BUFFER_SIZE]);

    //copy contents of sourceFile to targetFile
    wxULongLong totalBytesTransferred;
    do
    {
        const size_t bytesRead = fileIn.read(memory.get(), BUFFER_SIZE); //throw FileError()

        fileOut.write(memory.get(), bytesRead); //throw FileError()

        totalBytesTransferred += bytesRead;

        //invoke callback method to update progress indicators
        if (callback != NULL)
            switch (callback->updateCopyStatus(totalBytesTransferred))
            {
            case CopyFileCallback::CONTINUE:
                break;

            case CopyFileCallback::CANCEL: //a user aborted operation IS an error condition!
                throw FileError(wxString(_("Error copying file:")) + wxT("\n\"") + zToWx(sourceFile) +  wxT("\" ->\n\"") +
                                zToWx(targetFile) + wxT("\"\n\n") + _("Operation aborted!"));
            }
    }
    while (!fileIn.eof());

    //close output stream before changing attributes
    fileOut.close();

    //rename temporary file
    ffs3::renameFile(temporary, targetFile);
    guardTempFile.Dismiss();

    //ensure cleanup:
    Loki::ScopeGuard guardTargetFile = Loki::MakeGuard(::unlink, targetFile.c_str());

    //adapt file modification time:
    copyFileTimes(sourceFile, targetFile, true); //throw (FileError)

    //set file permissions
    if (copyFilePermissions)
        copyObjectPermissions(sourceFile, targetFile, true); //throw FileError()

    guardTargetFile.Dismiss(); //target has been created successfully!
}
#endif
