// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) 2008-2010 ZenJu (zhnmju123 AT gmx.de)                    *
// **************************************************************************
//
#include "fileHandling.h"
#include <wx/intl.h>
#include "systemFunctions.h"
#include "globalFunctions.h"
#include "systemConstants.h"
#include "fileTraverser.h"
#include <boost/bind.hpp>
#include <algorithm>
#include <wx/datetime.h>
#include "stringConv.h"
#include <wx/utils.h>
#include <boost/scoped_array.hpp>
#include <boost/shared_ptr.hpp>
#include <stdexcept>
#include "loki/TypeManip.h"
#include "loki/ScopeGuard.h"

#ifdef FFS_WIN
#include "dllLoader.h"
#include <wx/msw/wrapwin.h> //includes "windows.h"
#include "longPathPrefix.h"

#elif defined FFS_LINUX
#include <sys/stat.h>
#include "fileIO.h"
#include <time.h>
#include <utime.h>
#include <errno.h>
#endif

using FreeFileSync::FileError;


namespace
{
#ifdef FFS_WIN
Zstring resolveRelativePath(const Zstring& relativeName, DWORD proposedBufferSize = 1000)
{
    boost::scoped_array<DefaultChar> fullPath(new DefaultChar[proposedBufferSize]);
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
                rest = wxString() + SEPARATOR + rest;
                expandMacros(rest);
                text = prefix + SEPARATOR + potentialMacro + rest;
            }
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

    if (dirnameTmp.empty()) //an empty string will later be returned as "\"; this is not desired
        return Zstring();

    //replace macros
    expandMacros(dirnameTmp);

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
    dirnameTmp = zToWx(resolveRelativePath(wxToZ(dirnameTmp)));

    if (!dirnameTmp.EndsWith(zToWx(globalFunctions::FILE_NAME_SEPARATOR)))
        dirnameTmp += zToWx(globalFunctions::FILE_NAME_SEPARATOR);

    return wxToZ(dirnameTmp);
}


bool FreeFileSync::fileExists(const Zstring& filename)
{
    //symbolic links (broken or not) are also treated as existing files!
#ifdef FFS_WIN
    // we must use GetFileAttributes() instead of the ANSI C functions because
    // it can cope with network (UNC) paths unlike them
    const DWORD ret = ::GetFileAttributes(applyLongPathPrefix(filename).c_str());
    return (ret != INVALID_FILE_ATTRIBUTES) && !(ret & FILE_ATTRIBUTE_DIRECTORY); //returns true for (file-)symlinks also

#elif defined FFS_LINUX
    struct stat fileInfo;
    return (::lstat(filename.c_str(), &fileInfo) == 0 &&
            (S_ISLNK(fileInfo.st_mode) || S_ISREG(fileInfo.st_mode))); //in Linux a symbolic link is neither file nor directory
#endif
}


bool FreeFileSync::dirExists(const Zstring& dirname)
{
    //symbolic links (broken or not) are also treated as existing directories!
#ifdef FFS_WIN
    // we must use GetFileAttributes() instead of the ANSI C functions because
    // it can cope with network (UNC) paths unlike them
    const DWORD ret = ::GetFileAttributes(applyLongPathPrefix(dirname).c_str());

    return (ret != INVALID_FILE_ATTRIBUTES) && (ret & FILE_ATTRIBUTE_DIRECTORY); //returns true for (dir-)symlinks also

#elif defined FFS_LINUX
    struct stat dirInfo;
    return (::lstat(dirname.c_str(), &dirInfo) == 0 &&
            (S_ISLNK(dirInfo.st_mode) || S_ISDIR(dirInfo.st_mode))); //in Linux a symbolic link is neither file nor directory
#endif
}


bool FreeFileSync::symlinkExists(const Zstring& objname)
{
#ifdef FFS_WIN
    const DWORD ret = ::GetFileAttributes(applyLongPathPrefix(objname).c_str());
    return (ret != INVALID_FILE_ATTRIBUTES) && (ret & FILE_ATTRIBUTE_REPARSE_POINT);

#elif defined FFS_LINUX
    struct stat fileInfo;
    return (::lstat(objname.c_str(), &fileInfo) == 0 &&
            S_ISLNK(fileInfo.st_mode)); //symbolic link
#endif
}


bool FreeFileSync::somethingExists(const Zstring& objname) //throw()       check whether any object with this name exists
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
    const HANDLE hFile = ::CreateFile(FreeFileSync::applyLongPathPrefix(linkName).c_str(),
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
        {
            return wxULongLong(fileInfoByHandle.nFileSizeHigh, fileInfoByHandle.nFileSizeLow);
        }
    }

    const wxString errorMessage = wxString(_("Error reading file attributes:")) + wxT("\n\"") + FreeFileSync::zToWx(linkName) + wxT("\"");
    throw FileError(errorMessage + wxT("\n\n") + FreeFileSync::getLastErrorFormatted());
}
}
#endif


wxULongLong FreeFileSync::getFilesize(const Zstring& filename) //throw (FileError)
{
#ifdef FFS_WIN
    WIN32_FIND_DATA fileMetaData;
    const HANDLE searchHandle = ::FindFirstFile(applyLongPathPrefix(filename).c_str(), &fileMetaData);
    if (searchHandle == INVALID_HANDLE_VALUE)
    {
        const wxString errorMessage = wxString(_("Error reading file attributes:")) + wxT("\n\"") + zToWx(filename) + wxT("\"");
        throw FileError(errorMessage + wxT("\n\n") + FreeFileSync::getLastErrorFormatted());
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
        throw FileError(errorMessage + wxT("\n\n") + FreeFileSync::getLastErrorFormatted());
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
    if (!volumePath.EndsWith(globalFunctions::FILE_NAME_SEPARATOR))
        volumePath += globalFunctions::FILE_NAME_SEPARATOR;

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
    if (volumePathName.size() > 1 && volumePathName.EndsWith(globalFunctions::FILE_NAME_SEPARATOR))  //exception: allow '/'
        volumePathName = volumePathName.BeforeLast(globalFunctions::FILE_NAME_SEPARATOR);

    struct stat fileInfo;
    while (::lstat(volumePathName.c_str(), &fileInfo) != 0)
    {
        volumePathName = volumePathName.BeforeLast(globalFunctions::FILE_NAME_SEPARATOR); //returns empty string if ch not found
        if (volumePathName.empty())
            return 0;  //this includes path "/" also!
    }

    return fileInfo.st_dev;
}
#endif
}


FreeFileSync::ResponseSameVol FreeFileSync::onSameVolume(const Zstring& folderLeft, const Zstring& folderRight) //throw()
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


void FreeFileSync::removeFile(const Zstring& filename) //throw (FileError);
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


namespace
{
struct ErrorDifferentVolume : public FreeFileSync::FileError
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
    using namespace FreeFileSync; //for zToWx()

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
                                      wxT("\n\n") + FreeFileSync::getLastErrorFormatted();
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
                                      wxT("\n\n") + FreeFileSync::getLastErrorFormatted();
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
    const Zstring filenameFmt = FreeFileSync::applyLongPathPrefix(filename);

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
    const Zstring pathPrefix = fileName.Find(globalFunctions::FILE_NAME_SEPARATOR) != Zstring::npos ?
                               (fileName.BeforeLast(globalFunctions::FILE_NAME_SEPARATOR) + globalFunctions::FILE_NAME_SEPARATOR) : Zstring();

    Zstring extension = fileName.AfterLast(globalFunctions::FILE_NAME_SEPARATOR).AfterLast(DefaultChar('.')); //extension needn't contain reasonable data
    if (extension.empty())
        extension = DefaultStr("FFS");
    extension.Truncate(3);

    for (int index = 0; index < 100000000; ++index) //filename must be representable by <= 8 characters
    {
        const Zstring output = pathPrefix + numberToZstring(index) + DefaultChar('.') + extension;
        if (!FreeFileSync::somethingExists(output)) //ensure uniqueness
            return output;
    }

    throw std::runtime_error(std::string("100000000 files, one for each number, exist in this directory? You're kidding...\n") + std::string(wxString(pathPrefix.c_str()).ToUTF8()));
}


//try to handle issues with already existing short 8.3 file names on Windows 7
bool fix8Dot3NameClash(const Zstring& oldName, const Zstring& newName)  //throw (FileError); return "true" if rename operation succeeded
{
    using namespace FreeFileSync;

    if (newName.Find(globalFunctions::FILE_NAME_SEPARATOR) == Zstring::npos)
        return false;

    if (FreeFileSync::somethingExists(newName)) //name OR directory!
    {
        const Zstring fileNameOrig  = newName.AfterLast(globalFunctions::FILE_NAME_SEPARATOR); //returns the whole string if ch not found
        const Zstring fileNameShort = getFilenameFmt(newName, ::GetShortPathName).AfterLast(globalFunctions::FILE_NAME_SEPARATOR); //throw() returns empty string on error
        const Zstring fileNameLong  = getFilenameFmt(newName, ::GetLongPathName).AfterLast(globalFunctions::FILE_NAME_SEPARATOR);  //throw() returns empty string on error

        if (    !fileNameShort.empty() &&
                !fileNameLong.empty()  &&
                fileNameOrig.cmpFileName(fileNameShort) == 0 &&
                fileNameShort.cmpFileName(fileNameLong) != 0)
        {
            //we detected an event where newName is in shortname format (although it is intended to be a long name) and
            //writing target file failed because another unrelated file happens to have the same short name

            const Zstring newNameFullPathLong = newName.BeforeLast(globalFunctions::FILE_NAME_SEPARATOR) + globalFunctions::FILE_NAME_SEPARATOR +
                                                fileNameLong;

            //find another name in short format: this ensures the actual short name WILL be renamed as well!
            const Zstring parkedTarget = createTemp8Dot3Name(newName);

            //move already existing short name out of the way for now
            renameFileInternal(newNameFullPathLong, parkedTarget); //throw (FileError, ErrorDifferentVolume);
            //DON'T call FreeFileSync::renameFile() to avoid reentrance!

            //schedule cleanup; the file system should assign this unrelated file a new (unique) short name
            Loki::ScopeGuard guard = Loki::MakeGuard(renameFileInternalNoThrow, parkedTarget, newNameFullPathLong);//equivalent to Boost.ScopeExit in this case
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
void FreeFileSync::renameFile(const Zstring& oldName, const Zstring& newName) //throw (FileError, ErrorDifferentVolume);
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
    if (somethingExists(targetFile)) //test file existence: e.g. Linux might silently overwrite existing symlinks
    {
        const wxString errorMessage = wxString(_("Error moving file:")) + wxT("\n\"") + zToWx(sourceFile) +  wxT("\" ->\n\"") + zToWx(targetFile) + wxT("\"");
        throw FileError(errorMessage + wxT("\n\n") + _("Target file already existing!"));
    }

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
    std::auto_ptr<CopyCallbackImpl> copyCallback(callback != NULL ? new CopyCallbackImpl(callback) : NULL);

    copyFile(sourceFile,
             targetFile,
             true, //copy symbolic links
#ifdef FFS_WIN
             NULL, //supply handler for making shadow copies
#endif
             copyCallback.get()); //throw (FileError);

    //attention: if copy-operation was cancelled an exception is thrown => sourcefile is not deleted, as we wish!

    removeFile(sourceFile);
}


class TraverseOneLevel : public FreeFileSync::TraverseCallback
{
public:
    typedef std::vector<std::pair<Zstring, Zstring> > NamePair;

    TraverseOneLevel(NamePair& filesShort, NamePair& dirsShort) :
        m_files(filesShort),
        m_dirs(dirsShort) {}

    virtual ReturnValue onFile(const DefaultChar* shortName, const Zstring& fullName, bool isSymlink, const FileInfo& details)
    {
        m_files.push_back(std::make_pair(Zstring(shortName), fullName));
        return TRAVERSING_CONTINUE;
    }
    virtual ReturnValDir onDir(const DefaultChar* shortName, const Zstring& fullName, bool isSymlink)
    {
        m_dirs.push_back(std::make_pair(Zstring(shortName), fullName));
        return Loki::Int2Type<ReturnValDir::TRAVERSING_DIR_IGNORE>(); //DON'T traverse into subdirs; moveDirectory works recursively!
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
        removeDirectory(sourceDir);           //if target is already another symlink or directory, sourceDir-symlink is silently deleted
        return;
    }

    if (dirExists(targetDir))
    {
        if (!ignoreExistingDirs) //directory or symlink exists
        {
            const wxString errorMessage = wxString(_("Error moving directory:")) + wxT("\n\"") + zToWx(sourceDir) +  wxT("\" ->\n\"") + zToWx(targetDir) + wxT("\"");
            throw FileError(errorMessage + wxT("\n\n") + _("Target directory already existing!"));
        }
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
    traverseFolder(sourceDir, &traverseCallback); //traverse one level

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
    removeDirectory(sourceDir); //throw (FileError);
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

    virtual ReturnValue onFile(const DefaultChar* shortName, const Zstring& fullName, bool isSymlink, const FileInfo& details)
    {
        m_files.push_back(fullName);
        return TRAVERSING_CONTINUE;
    }
    virtual ReturnValDir onDir(const DefaultChar* shortName, const Zstring& fullName, bool isSymlink)
    {
        m_dirs.push_back(fullName);
        return Loki::Int2Type<ReturnValDir::TRAVERSING_DIR_IGNORE>(); //DON'T traverse into subdirs; removeDirectory works recursively!
    }
    virtual ReturnValue onError(const wxString& errorText)
    {
        throw FileError(errorText);
    }

private:
    std::vector<Zstring>& m_files;
    std::vector<Zstring>& m_dirs;
};


void FreeFileSync::removeDirectory(const Zstring& directory)
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
        throw FileError(errorMessage + wxT("\n\n") + FreeFileSync::getLastErrorFormatted());
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
            throw FileError(errorMessage + wxT("\n\n") + FreeFileSync::getLastErrorFormatted());
        }
        return;
    }

    std::vector<Zstring> fileList;
    std::vector<Zstring> dirList;

    //get all files and directories from current directory (WITHOUT subdirectories!)
    FilesDirsOnlyTraverser traverser(fileList, dirList);
    FreeFileSync::traverseFolder(directory, &traverser);

    //delete files
    std::for_each(fileList.begin(), fileList.end(), removeFile);

    //delete directories recursively
    std::for_each(dirList.begin(), dirList.end(), removeDirectory); //call recursively to correctly handle symbolic links

    //parent directory is deleted last
#ifdef FFS_WIN
    if (!::RemoveDirectory(directoryFmt.c_str())) //remove directory, support for \\?\-prefix
#else
    if (::rmdir(directory.c_str()) != 0)
#endif
    {
        wxString errorMessage = wxString(_("Error deleting directory:")) + wxT("\n\"") + zToWx(directory) + wxT("\"");
        throw FileError(errorMessage + wxT("\n\n") + FreeFileSync::getLastErrorFormatted());
    }
}


//optionally: copy directory last change date, DO NOTHING if something fails
void FreeFileSync::copyFileTimes(const Zstring& sourceDir, const Zstring& targetDir)
{
    if (symlinkExists(sourceDir)) //don't handle symlinks (yet)
        return;

#ifdef FFS_WIN
    WIN32_FILE_ATTRIBUTE_DATA sourceAttr;
    if (!::GetFileAttributesEx(applyLongPathPrefix(sourceDir).c_str(), //__in   LPCTSTR lpFileName,
                               GetFileExInfoStandard,                  //__in   GET_FILEEX_INFO_LEVELS fInfoLevelId,
                               &sourceAttr))                           //__out  LPVOID lpFileInformation
        return;

    HANDLE hDirWrite = ::CreateFile(applyLongPathPrefix(targetDir).c_str(),
                                    FILE_WRITE_ATTRIBUTES,
                                    FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                                    NULL,
                                    OPEN_EXISTING,
                                    FILE_FLAG_BACKUP_SEMANTICS, //needed to open a directory
                                    NULL);
    if (hDirWrite == INVALID_HANDLE_VALUE)
        return;

    boost::shared_ptr<void> dummy(hDirWrite, ::CloseHandle);

    //(try to) set new "last write time"
    ::SetFileTime(hDirWrite,
                  &sourceAttr.ftCreationTime,
                  &sourceAttr.ftLastAccessTime,
                  &sourceAttr.ftLastWriteTime); //return value not evalutated!
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
                                     FILE_FLAG_BACKUP_SEMANTICS,  //needed to open a directory
                                     NULL);
    if (hDir == INVALID_HANDLE_VALUE)
        return Zstring();

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
        Utility::loadDllFunction<GetFinalPathNameByHandleWFunc>(L"kernel32.dll", "GetFinalPathNameByHandleW");

    if (getFinalPathNameByHandle == NULL)
        throw FileError(wxString(_("Error loading library function:")) + wxT("\n\"") + wxT("GetFinalPathNameByHandleW") + wxT("\""));

    const DWORD rv = (*getFinalPathNameByHandle)(
                         hDir,       //__in   HANDLE hFile,
                         targetPath, //__out  LPTSTR lpszFilePath,
                         BUFFER_SIZE,//__in   DWORD cchFilePath,
                         0);         //__in   DWORD dwFlags
    if (rv >= BUFFER_SIZE || rv == 0)
        return Zstring();

    return targetPath;
}

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
    const DWORD templateAttr = templateDir.empty() ?
                               INVALID_FILE_ATTRIBUTES :
                               ::GetFileAttributes(applyLongPathPrefix(templateDir).c_str()); //returns successful for broken symlinks
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
        if (    !templateDir.empty() && //test if templateDir is a symbolic link
                symlinkExists(templateDir))
        {
            //copy symbolic link
            const int BUFFER_SIZE = 10000;
            char buffer[BUFFER_SIZE];
            const int bytesWritten = readlink(templateDir.c_str(), buffer, BUFFER_SIZE);
            if (bytesWritten < 0 || bytesWritten >= BUFFER_SIZE)
            {
                wxString errorMessage = wxString(_("Error resolving symbolic link:")) + wxT("\n\"") + zToWx(templateDir) + wxT("\"");
                if (bytesWritten < 0) errorMessage += wxString(wxT("\n\n")) + FreeFileSync::getLastErrorFormatted();
                throw FileError(errorMessage);
            }
            //set null-terminating char
            buffer[bytesWritten] = 0;

            if (::symlink(buffer, directory.c_str()) != 0)
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


namespace
{
Zstring createTempName(const Zstring& filename)
{
    Zstring output = filename + FreeFileSync::TEMP_FILE_ENDING;

    //ensure uniqueness
    for (int i = 1; FreeFileSync::somethingExists(output); ++i)
        output = filename + DefaultChar('_') + numberToZstring(i) + FreeFileSync::TEMP_FILE_ENDING;

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
    using FreeFileSync::CopyFileCallback;

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
}


void FreeFileSync::copyFile(const Zstring& sourceFile,
                            const Zstring& targetFile,
                            bool copyFileSymLinks,
                            FreeFileSync::ShadowCopy* shadowCopyHandler,
                            FreeFileSync::CopyFileCallback* callback)
{
    //FreeFileSync::fileExists(targetFile) -> avoid this call, performance;
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
                copyCallbackInternal,
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
            const Zstring shadowFilename(shadowCopyHandler->makeShadowCopy(sourceFile));
            return copyFile(shadowFilename, //transferred bytes is automatically reset when new file is copied
                            targetFile,
                            copyFileSymLinks,
                            NULL,
                            callback);
        }
        //assemble error message...
        const wxString errorMessage = wxString(_("Error copying file:")) + wxT("\n\"") + sourceFile.c_str() +  wxT("\" ->\n\"") + targetFile.c_str() + wxT("\"") + wxT("\n\n");
        throw FileError(errorMessage + FreeFileSync::getLastErrorFormatted(lastError));
    }

    //rename temporary file: do not add anything else here (note specific error handing)
    FreeFileSync::renameFile(temporary, targetFile);

    guardTempFile.Dismiss(); //no need to delete temp file anymore

    //copy creation date (last modification date is redundantly written, too)
    copyFileTimes(sourceFile, targetFile); //throw()
}


#elif defined FFS_LINUX
void FreeFileSync::copyFile(const Zstring& sourceFile,
                            const Zstring& targetFile,
                            bool copyFileSymLinks,
                            CopyFileCallback* callback)
{
    using FreeFileSync::CopyFileCallback;

    //symbolic link handling
    if (copyFileSymLinks)
    {
        if (symlinkExists(sourceFile))
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
    if (::stat(sourceFile.c_str(), &fileInfo) != 0) //read file attributes from source file (resolving symlinks)
    {
        const wxString errorMessage = wxString(_("Error reading file attributes:")) + wxT("\n\"") + zToWx(sourceFile) + wxT("\"");
        throw FileError(errorMessage + wxT("\n\n") + FreeFileSync::getLastErrorFormatted());
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
        {
            switch (callback->updateCopyStatus(totalBytesTransferred))
            {
            case CopyFileCallback::CONTINUE:
                break;

            case CopyFileCallback::CANCEL: //a user aborted operation IS an error condition!
                throw FileError(wxString(_("Error copying file:")) + wxT("\n\"") + zToWx(sourceFile) +  wxT("\" ->\n\"") +
                                zToWx(targetFile) + wxT("\"\n\n") + _("Operation aborted!"));
            }
        }
    }
    while (!fileIn.eof());

    //close output stream before changing attributes
    fileOut.close();

    //adapt file modification time:
    struct utimbuf newTimes;
    ::time(&newTimes.actime); //set file access time to current time
    newTimes.modtime = fileInfo.st_mtime;
    if (::utime(temporary.c_str(), &newTimes) != 0)
    {
        wxString errorMessage = wxString(_("Error changing modification time:")) + wxT("\n\"") + zToWx(targetFile) + wxT("\"");
        throw FileError(errorMessage + wxT("\n\n") + FreeFileSync::getLastErrorFormatted());
    }

    //rename temporary file
    FreeFileSync::renameFile(temporary, targetFile);
    guardTempFile.Dismiss();

    //ensure cleanup:
    Loki::ScopeGuard guardTargetFile = Loki::MakeGuard(::unlink, targetFile.c_str()); //don't use Utility::CleanUp here

    //set file access rights
    if (::chmod(targetFile.c_str(), fileInfo.st_mode) != 0)
    {
        const wxString errorMessage = wxString(_("Error writing file attributes:")) + wxT("\n\"") + zToWx(targetFile) + wxT("\"");
        throw FileError(errorMessage + wxT("\n\n") + FreeFileSync::getLastErrorFormatted());
    }

    guardTargetFile.Dismiss(); //target has been created successfully!
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
