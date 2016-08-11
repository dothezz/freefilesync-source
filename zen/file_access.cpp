// *****************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under    *
// * GNU General Public License: http://www.gnu.org/licenses/gpl-3.0           *
// * Copyright (C) Zenju (zenju AT freefilesync DOT org) - All Rights Reserved *
// *****************************************************************************

#include "file_access.h"
#include <map>
#include <algorithm>
#include <stdexcept>
#include "file_traverser.h"
#include "scope_guard.h"
#include "symlink_target.h"
#include "file_id_def.h"
#include "file_io.h"

    #include <sys/vfs.h> //statfs
    #include <sys/time.h> //lutimes
    #ifdef HAVE_SELINUX
        #include <selinux/selinux.h>
    #endif


    #include <fcntl.h> //open, close, AT_SYMLINK_NOFOLLOW, UTIME_OMIT
    #include <sys/stat.h>

using namespace zen;


bool zen::fileExists(const Zstring& filePath)
{
    //symbolic links (broken or not) are also treated as existing files!
    struct ::stat fileInfo = {};
    if (::stat(filePath.c_str(), &fileInfo) == 0) //follow symlinks!
        return S_ISREG(fileInfo.st_mode);
    return false;
}


bool zen::dirExists(const Zstring& dirPath)
{
    //symbolic links (broken or not) are also treated as existing directories!
    struct ::stat dirInfo = {};
    if (::stat(dirPath.c_str(), &dirInfo) == 0) //follow symlinks!
        return S_ISDIR(dirInfo.st_mode);
    return false;
}


bool zen::symlinkExists(const Zstring& linkPath)
{
    struct ::stat linkInfo = {};
    if (::lstat(linkPath.c_str(), &linkInfo) == 0)
        return S_ISLNK(linkInfo.st_mode);
    return false;
}


bool zen::somethingExists(const Zstring& itemPath)
{
    struct ::stat fileInfo = {};
    if (::lstat(itemPath.c_str(), &fileInfo) == 0)
        return true;
    return false;
}


namespace
{
}


std::uint64_t zen::getFilesize(const Zstring& filePath) //throw FileError
{
    struct ::stat fileInfo = {};
    if (::stat(filePath.c_str(), &fileInfo) != 0)
        THROW_LAST_FILE_ERROR(replaceCpy(_("Cannot read file attributes of %x."), L"%x", fmtPath(filePath)), L"stat");

    return fileInfo.st_size;
}


std::uint64_t zen::getFreeDiskSpace(const Zstring& path) //throw FileError, returns 0 if not available
{
    struct ::statfs info = {};
    if (::statfs(path.c_str(), &info) != 0)
        THROW_LAST_FILE_ERROR(replaceCpy(_("Cannot determine free disk space for %x."), L"%x", fmtPath(path)), L"statfs");

    return static_cast<std::uint64_t>(info.f_bsize) * info.f_bavail;
}


VolumeId zen::getVolumeId(const Zstring& itemPath) //throw FileError
{
    struct ::stat fileInfo = {};
    if (::stat(itemPath.c_str(), &fileInfo) != 0)
        THROW_LAST_FILE_ERROR(replaceCpy(_("Cannot read file attributes of %x."), L"%x", fmtPath(itemPath)), L"stat");

    return fileInfo.st_dev;
}


Zstring zen::getTempFolderPath() //throw FileError
{
    const char* buf = ::getenv("TMPDIR"); //no extended error reporting
    if (!buf)
        throw FileError(_("Cannot get process information."), L"getenv: TMPDIR not found.");
    return buf;
}


bool zen::removeFile(const Zstring& filePath) //throw FileError
{
    const wchar_t functionName[] = L"unlink";
    if (::unlink(filePath.c_str()) != 0)
    {
        ErrorCode ec = getLastError(); //copy before directly/indirectly making other system calls!
        if (!somethingExists(filePath)) //warning: changes global error code!!
            return false; //neither file nor any other object (e.g. broken symlink) with that name existing - caveat: what if "access is denied"!?!??!?!?

        //begin of "regular" error reporting
        const std::wstring errorMsg = replaceCpy(_("Cannot delete file %x."), L"%x", fmtPath(filePath));
        std::wstring errorDescr = formatSystemError(functionName, ec);

        throw FileError(errorMsg, errorDescr);
    }
    return true;
}


void zen::removeDirectorySimple(const Zstring& dirPath) //throw FileError
{
    const wchar_t functionName[] = L"rmdir";
    if (::rmdir(dirPath.c_str()) != 0)
    {
        const ErrorCode ec = getLastError(); //copy before making other system calls!

        if (!somethingExists(dirPath)) //warning: changes global error code!!
            return;

        if (symlinkExists(dirPath))
        {
            if (::unlink(dirPath.c_str()) != 0)
                THROW_LAST_FILE_ERROR(replaceCpy(_("Cannot delete directory %x."), L"%x", fmtPath(dirPath)), L"unlink");
            return;
        }
        throw FileError(replaceCpy(_("Cannot delete directory %x."), L"%x", fmtPath(dirPath)), formatSystemError(functionName, ec));
    }
    /*
    Windows: may spuriously fail with ERROR_DIR_NOT_EMPTY(145) even though all child items have
    successfully been *marked* for deletion, but some application still has a handle open!
    e.g. Open "C:\Test\Dir1\Dir2" (filled with lots of files) in Explorer, then delete "C:\Test\Dir1" via ::RemoveDirectory() => Error 145
    Sample code: http://us.generation-nt.com/answer/createfile-directory-handles-removing-parent-help-29126332.html
    Alternatives: 1. move file/empty folder to some other location, then DeleteFile()/RemoveDirectory()
                  2. use CreateFile/FILE_FLAG_DELETE_ON_CLOSE *without* FILE_SHARE_DELETE instead of DeleteFile() => early failure
    */
}


namespace
{
void removeDirectoryImpl(const Zstring& folderPath) //throw FileError
{
    assert(dirExists(folderPath)); //[!] no symlinks in this context!!!
    //attention: check if folderPath is a symlink! Do NOT traverse into it deleting contained files!!!

    std::vector<Zstring> filePaths;
    std::vector<Zstring> folderSymlinkPaths;
    std::vector<Zstring> folderPaths;

    //get all files and directories from current directory (WITHOUT subdirectories!)
    traverseFolder(folderPath,
    [&](const FileInfo&    fi) { filePaths.push_back(fi.fullPath); },
    [&](const DirInfo&     di) { folderPaths .push_back(di.fullPath); }, //defer recursion => save stack space and allow deletion of extremely deep hierarchies!
    [&](const SymlinkInfo& si)
    {
            filePaths.push_back(si.fullPath);
    },
    [](const std::wstring& errorMsg) { throw FileError(errorMsg); });

    for (const Zstring& filePath : filePaths)
        removeFile(filePath); //throw FileError

    for (const Zstring& symlinkPath : folderSymlinkPaths)
        removeDirectorySimple(symlinkPath); //throw FileError

    //delete directories recursively
    for (const Zstring& subFolderPath : folderPaths)
        removeDirectoryImpl(subFolderPath); //throw FileError; call recursively to correctly handle symbolic links

    removeDirectorySimple(folderPath); //throw FileError
}
}


void zen::removeDirectoryRecursively(const Zstring& dirPath) //throw FileError
{
    if (symlinkExists(dirPath))
        removeDirectorySimple(dirPath); //throw FileError
    else if (somethingExists(dirPath))
        removeDirectoryImpl(dirPath); //throw FileError
}


namespace
{
/* Usage overview: (avoid circular pattern!)

  renameFile()  -->  renameFile_sub()
      |               /|\
     \|/               |
      Fix8Dot3NameClash()
*/
//wrapper for file system rename function:
void renameFile_sub(const Zstring& pathSource, const Zstring& pathTarget) //throw FileError, ErrorDifferentVolume, ErrorTargetExisting
{
    //rename() will never fail with EEXIST, but always (atomically) overwrite! => equivalent to SetFileInformationByHandle() + FILE_RENAME_INFO::ReplaceIfExists
    //=> Linux: renameat2() with RENAME_NOREPLACE -> still new, probably buggy
    //=> OS X: no solution

    auto throwException = [&](int ec)
    {
        const std::wstring errorMsg = replaceCpy(replaceCpy(_("Cannot move file %x to %y."), L"%x", L"\n" + fmtPath(pathSource)), L"%y", L"\n" + fmtPath(pathTarget));
        const std::wstring errorDescr = formatSystemError(L"rename", ec);

        if (ec == EXDEV)
            throw ErrorDifferentVolume(errorMsg, errorDescr);
        if (ec == EEXIST)
            throw ErrorTargetExisting(errorMsg, errorDescr);
        throw FileError(errorMsg, errorDescr);
    };

    if (!equalFilePath(pathSource, pathTarget)) //OS X: changing file name case is not an "already exists" error!
        if (somethingExists(pathTarget))
            throwException(EEXIST);

    if (::rename(pathSource.c_str(), pathTarget.c_str()) != 0)
        throwException(errno);
}


}


//rename file: no copying!!!
void zen::renameFile(const Zstring& pathSource, const Zstring& pathTarget) //throw FileError, ErrorDifferentVolume, ErrorTargetExisting
{
    try
    {
        renameFile_sub(pathSource, pathTarget); //throw FileError, ErrorDifferentVolume, ErrorTargetExisting
    }
    catch (const ErrorTargetExisting&)
    {
        throw;
    }
}


namespace
{

void setWriteTimeNative(const Zstring& itemPath, const struct ::timespec& modTime, ProcSymlink procSl) //throw FileError
{
    /*
    [2013-05-01] sigh, we can't use utimensat() on NTFS volumes on Ubuntu: silent failure!!! what morons are programming this shit???
    => fallback to "retarded-idiot version"! -- DarkByte

    [2015-03-09]
     - cannot reproduce issues with NTFS and utimensat() on Ubuntu
     - utimensat() is supposed to obsolete utime/utimes and is also used by "cp" and "touch"
		=> let's give utimensat another chance:
		using open()/futimens() for regular files and utimensat(AT_SYMLINK_NOFOLLOW) for symlinks is consistent with "cp" and "touch"!
    */
    struct ::timespec newTimes[2] = {};
    newTimes[0].tv_sec = ::time(nullptr); //access time; using UTIME_OMIT for tv_nsec would trigger even more bugs: http://www.freefilesync.org/forum/viewtopic.php?t=1701
    newTimes[1] = modTime; //modification time

    if (procSl == ProcSymlink::FOLLOW)
    {
		//hell knows why files on gvfs-mounted Samba shares fail to open(O_WRONLY) returning EOPNOTSUPP:
		//http://www.freefilesync.org/forum/viewtopic.php?t=2803 => utimensat() works
        if (::utimensat(AT_FDCWD, itemPath.c_str(), newTimes, 0) == 0)
			return;

		//in other cases utimensat() returns EINVAL for CIFS/NTFS drives, but open+futimens works: http://www.freefilesync.org/forum/viewtopic.php?t=387
        const int fdFile = ::open(itemPath.c_str(), O_WRONLY);
        if (fdFile == -1)
            THROW_LAST_FILE_ERROR(replaceCpy(_("Cannot write modification time of %x."), L"%x", fmtPath(itemPath)), L"open");
        ZEN_ON_SCOPE_EXIT(::close(fdFile));

        if (::futimens(fdFile, newTimes) != 0)
            THROW_LAST_FILE_ERROR(replaceCpy(_("Cannot write modification time of %x."), L"%x", fmtPath(itemPath)), L"futimens");
    }
    else
    {
        if (::utimensat(AT_FDCWD, itemPath.c_str(), newTimes, AT_SYMLINK_NOFOLLOW) != 0)
            THROW_LAST_FILE_ERROR(replaceCpy(_("Cannot write modification time of %x."), L"%x", fmtPath(itemPath)), L"utimensat");
    }
}


}


void zen::setFileTime(const Zstring& filePath, std::int64_t modTime, ProcSymlink procSl) //throw FileError
{
    struct ::timespec writeTime = {};
    writeTime.tv_sec = modTime;
    setWriteTimeNative(filePath, writeTime, procSl); //throw FileError

}


bool zen::supportsPermissions(const Zstring& dirPath) //throw FileError
{
    return true;
}


namespace
{
#ifdef HAVE_SELINUX
//copy SELinux security context
void copySecurityContext(const Zstring& source, const Zstring& target, ProcSymlink procSl) //throw FileError
{
    security_context_t contextSource = nullptr;
    const int rv = procSl == ProcSymlink::FOLLOW ?
                   ::getfilecon(source.c_str(), &contextSource) :
                   ::lgetfilecon(source.c_str(), &contextSource);
    if (rv < 0)
    {
        if (errno == ENODATA ||  //no security context (allegedly) is not an error condition on SELinux
            errno == EOPNOTSUPP) //extended attributes are not supported by the filesystem
            return;

        THROW_LAST_FILE_ERROR(replaceCpy(_("Cannot read security context of %x."), L"%x", fmtPath(source)), L"getfilecon");
    }
    ZEN_ON_SCOPE_EXIT(::freecon(contextSource));

    {
        security_context_t contextTarget = nullptr;
        const int rv2 = procSl == ProcSymlink::FOLLOW ?
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

    const int rv3 = procSl == ProcSymlink::FOLLOW ?
                    ::setfilecon(target.c_str(), contextSource) :
                    ::lsetfilecon(target.c_str(), contextSource);
    if (rv3 < 0)
        THROW_LAST_FILE_ERROR(replaceCpy(_("Cannot write security context of %x."), L"%x", fmtPath(target)), L"setfilecon");
}
#endif


//copy permissions for files, directories or symbolic links: requires admin rights
void copyItemPermissions(const Zstring& sourcePath, const Zstring& targetPath, ProcSymlink procSl) //throw FileError
{

#ifdef HAVE_SELINUX  //copy SELinux security context
    copySecurityContext(sourcePath, targetPath, procSl); //throw FileError
#endif

    struct ::stat fileInfo = {};
    if (procSl == ProcSymlink::FOLLOW)
    {
        if (::stat(sourcePath.c_str(), &fileInfo) != 0)
            THROW_LAST_FILE_ERROR(replaceCpy(_("Cannot read permissions of %x."), L"%x", fmtPath(sourcePath)), L"stat");

        if (::chown(targetPath.c_str(), fileInfo.st_uid, fileInfo.st_gid) != 0) // may require admin rights!
            THROW_LAST_FILE_ERROR(replaceCpy(_("Cannot write permissions of %x."), L"%x", fmtPath(targetPath)), L"chown");

        if (::chmod(targetPath.c_str(), fileInfo.st_mode) != 0)
            THROW_LAST_FILE_ERROR(replaceCpy(_("Cannot write permissions of %x."), L"%x", fmtPath(targetPath)), L"chmod");
    }
    else
    {
        if (::lstat(sourcePath.c_str(), &fileInfo) != 0)
            THROW_LAST_FILE_ERROR(replaceCpy(_("Cannot read permissions of %x."), L"%x", fmtPath(sourcePath)), L"lstat");

        if (::lchown(targetPath.c_str(), fileInfo.st_uid, fileInfo.st_gid) != 0) // may require admin rights!
            THROW_LAST_FILE_ERROR(replaceCpy(_("Cannot write permissions of %x."), L"%x", fmtPath(targetPath)), L"lchown");

        if (!symlinkExists(targetPath) && //setting access permissions doesn't make sense for symlinks on Linux: there is no lchmod()
            ::chmod(targetPath.c_str(), fileInfo.st_mode) != 0)
            THROW_LAST_FILE_ERROR(replaceCpy(_("Cannot write permissions of %x."), L"%x", fmtPath(targetPath)), L"chmod");
    }

}


void makeDirectoryRecursivelyImpl(const Zstring& directory) //FileError
{
    assert(!endsWith(directory, FILE_NAME_SEPARATOR)); //even "C:\" should be "C:" as input!

    try
    {
        copyNewDirectory(Zstring(), directory, false /*copyFilePermissions*/); //throw FileError, ErrorTargetExisting, ErrorTargetPathMissing
    }
    catch (const ErrorTargetExisting&) {} //*something* existing: folder or FILE!
    catch (const ErrorTargetPathMissing&)
    {
        //we need to create parent directories first
        const Zstring dirParent = beforeLast(directory, FILE_NAME_SEPARATOR, IF_MISSING_RETURN_NONE);
        if (!dirParent.empty())
        {
            //recurse...
            makeDirectoryRecursivelyImpl(dirParent); //throw FileError

            //now try again...
            copyNewDirectory(Zstring(), directory, false /*copyFilePermissions*/); //throw FileError, (ErrorTargetExisting), (ErrorTargetPathMissing)
            return;
        }
        throw;
    }
}
}


void zen::makeDirectoryRecursively(const Zstring& dirPath) //throw FileError
{
    //remove trailing separator (even for C:\ root directories)
    const Zstring dirFormatted = endsWith(dirPath, FILE_NAME_SEPARATOR) ?
                                 beforeLast(dirPath, FILE_NAME_SEPARATOR, IF_MISSING_RETURN_NONE) :
                                 dirPath;
    makeDirectoryRecursivelyImpl(dirFormatted); //FileError
}


//source path is optional (may be empty)
void zen::copyNewDirectory(const Zstring& sourcePath, const Zstring& targetPath, //throw FileError, ErrorTargetExisting, ErrorTargetPathMissing
                           bool copyFilePermissions)
{
    mode_t mode = S_IRWXU | S_IRWXG | S_IRWXO; //0777, default for newly created directories

    struct ::stat dirInfo = {};
    if (!sourcePath.empty())
        if (::stat(sourcePath.c_str(), &dirInfo) == 0)
        {
            mode = dirInfo.st_mode & (S_IRWXU | S_IRWXG | S_IRWXO); //analog to "cp" which copies "mode" (considering umask) by default
            mode |= S_IRWXU; //FFS only: we need full access to copy child items! "cp" seems to apply permissions *after* copying child items
        }
    //=> need copyItemPermissions() only for "chown" and umask-agnostic permissions

    if (::mkdir(targetPath.c_str(), mode) != 0)
    {
        const int lastError = errno; //copy before directly or indirectly making other system calls!
        const std::wstring errorMsg = replaceCpy(_("Cannot create directory %x."), L"%x", fmtPath(targetPath));
        const std::wstring errorDescr = formatSystemError(L"mkdir", lastError);

        if (lastError == EEXIST)
            throw ErrorTargetExisting(errorMsg, errorDescr);
        else if (lastError == ENOENT)
            throw ErrorTargetPathMissing(errorMsg, errorDescr);
        throw FileError(errorMsg, errorDescr);
    }

    if (!sourcePath.empty())
    {

        ZEN_ON_SCOPE_FAIL(try { removeDirectorySimple(targetPath); }
        catch (FileError&) {});   //ensure cleanup:

        //enforce copying file permissions: it's advertized on GUI...
        if (copyFilePermissions)
            copyItemPermissions(sourcePath, targetPath, ProcSymlink::FOLLOW); //throw FileError
    }
}


void zen::copySymlink(const Zstring& sourceLink, const Zstring& targetLink, bool copyFilePermissions) //throw FileError
{
    const Zstring linkPath = getSymlinkTargetRaw(sourceLink); //throw FileError; accept broken symlinks

    const wchar_t functionName[] = L"symlink";
    if (::symlink(linkPath.c_str(), targetLink.c_str()) != 0)
        THROW_LAST_FILE_ERROR(replaceCpy(replaceCpy(_("Cannot copy symbolic link %x to %y."), L"%x", L"\n" + fmtPath(sourceLink)), L"%y", L"\n" + fmtPath(targetLink)), functionName);

    //allow only consistent objects to be created -> don't place before ::symlink, targetLink may already exist!

    auto cleanUp = [&]
    {
        try
        {
                removeFile(targetLink); //throw FileError
        }
        catch (FileError&) {}
    };
    ZEN_ON_SCOPE_FAIL(cleanUp());

    //file times: essential for sync'ing a symlink: enforce this! (don't just try!)
    struct ::stat sourceInfo = {};
    if (::lstat(sourceLink.c_str(), &sourceInfo) != 0)
        THROW_LAST_FILE_ERROR(replaceCpy(_("Cannot read file attributes of %x."), L"%x", fmtPath(sourceLink)), L"lstat");

    setWriteTimeNative(targetLink, sourceInfo.st_mtim, ProcSymlink::DIRECT); //throw FileError

    if (copyFilePermissions)
        copyItemPermissions(sourceLink, targetLink, ProcSymlink::DIRECT); //throw FileError
}


namespace
{
InSyncAttributes copyFileOsSpecific(const Zstring& sourceFile, //throw FileError, ErrorTargetExisting
                                    const Zstring& targetFile,
                                    const std::function<void(std::int64_t bytesDelta)>& notifyProgress)
{
    FileInput fileIn(sourceFile); //throw FileError
    if (notifyProgress) notifyProgress(0); //throw X!

    struct ::stat sourceInfo = {};
    if (::fstat(fileIn.getHandle(), &sourceInfo) != 0)
        THROW_LAST_FILE_ERROR(replaceCpy(_("Cannot read file attributes of %x."), L"%x", fmtPath(sourceFile)), L"fstat");

    const mode_t mode = sourceInfo.st_mode & (S_IRWXU | S_IRWXG | S_IRWXO); //analog to "cp" which copies "mode" (considering umask) by default
    //it seems we don't need S_IWUSR, not even for the setFileTime() below! (tested with source file having different user/group!)

    //=> need copyItemPermissions() only for "chown" and umask-agnostic permissions
    const int fdTarget = ::open(targetFile.c_str(), O_WRONLY | O_CREAT | O_EXCL, mode);
    if (fdTarget == -1)
    {
        const int ec = errno; //copy before making other system calls!
        const std::wstring errorMsg = replaceCpy(_("Cannot write file %x."), L"%x", fmtPath(targetFile));
        const std::wstring errorDescr = formatSystemError(L"open", ec);

        if (ec == EEXIST)
            throw ErrorTargetExisting(errorMsg, errorDescr);

        throw FileError(errorMsg, errorDescr);
    }
    ZEN_ON_SCOPE_FAIL( try { removeFile(targetFile); }
    catch (FileError&) {} );
    //transactional behavior: place guard after ::open() and before lifetime of FileOutput:
    //=> don't delete file that existed previously!!!
    FileOutput fileOut(fdTarget, targetFile); //pass ownership
    if (notifyProgress) notifyProgress(0); //throw X!

    unbufferedStreamCopy(fileIn, fileOut, notifyProgress); //throw FileError, X

    struct ::stat targetInfo = {};
    if (::fstat(fileOut.getHandle(), &targetInfo) != 0)
        THROW_LAST_FILE_ERROR(replaceCpy(_("Cannot read file attributes of %x."), L"%x", fmtPath(targetFile)), L"fstat");

    //close output file handle before setting file time; also good place to catch errors when closing stream!
    fileOut.close(); //throw FileError
    if (notifyProgress) notifyProgress(0); //throw X!

    //we cannot set the target file times (::futimes) while the file descriptor is still open after a write operation:
    //this triggers bugs on samba shares where the modification time is set to current time instead.
    //Linux: http://bugs.debian.org/cgi-bin/bugreport.cgi?bug=340236
    //       http://comments.gmane.org/gmane.linux.file-systems.cifs/2854
    //OS X:  http://www.freefilesync.org/forum/viewtopic.php?t=356
    setWriteTimeNative(targetFile, sourceInfo.st_mtim, ProcSymlink::FOLLOW); //throw FileError

    InSyncAttributes newAttrib;
    newAttrib.fileSize         = sourceInfo.st_size;
    newAttrib.modificationTime = sourceInfo.st_mtim.tv_sec; //
    newAttrib.sourceFileId     = extractFileId(sourceInfo);
    newAttrib.targetFileId     = extractFileId(targetInfo);
    return newAttrib;
}

/*
                ------------------
                |File Copy Layers|
                ------------------
                   copyNewFile
                        |
               copyFileOsSpecific (solve 8.3 issue on Windows)
                        |
              copyFileWindowsSelectRoutine
              /                           \
copyFileWindowsDefault(::CopyFileEx)  copyFileWindowsBackupStream(::BackupRead/::BackupWrite)
*/
}


InSyncAttributes zen::copyNewFile(const Zstring& sourceFile, const Zstring& targetFile, bool copyFilePermissions, //throw FileError, ErrorTargetExisting, ErrorFileLocked
                                  const std::function<void(std::int64_t bytesDelta)>& notifyProgress)
{
    const InSyncAttributes attr = copyFileOsSpecific(sourceFile, targetFile, notifyProgress); //throw FileError, ErrorTargetExisting, ErrorFileLocked

    //at this point we know we created a new file, so it's fine to delete it for cleanup!
    ZEN_ON_SCOPE_FAIL(try { removeFile(targetFile); }
    catch (FileError&) {});

    if (copyFilePermissions)
        copyItemPermissions(sourceFile, targetFile, ProcSymlink::FOLLOW); //throw FileError

    return attr;
}
