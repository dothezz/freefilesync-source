// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) Zenju (zenju AT gmx DOT de) - All Rights Reserved        *
// **************************************************************************

#ifndef FILE_HANDLING_H_INCLUDED
#define FILE_HANDLING_H_INCLUDED

#include "zstring.h"
#include "file_error.h"
#include "int64.h"
#include "file_id_def.h"

namespace zen
{
struct CallbackRemoveDir;
struct CallbackCopyFile;


bool fileExists     (const Zstring& filename); //throw()       check whether file      or symlink exists
bool dirExists      (const Zstring& dirname);  //throw()       check whether directory or symlink exists
bool symlinkExists  (const Zstring& linkname); //throw()       check whether a symbolic link exists
bool somethingExists(const Zstring& objname);  //throw()       check whether any object with this name exists

//check whether two folders are located on the same (logical) volume
//left and right directories NEED NOT yet exist! volume prefix is sufficient! path may end with PATH_SEPARATOR
enum ResponseSame
{
    IS_SAME_YES,
    IS_SAME_NO,
    IS_SAME_CANT_SAY
};
ResponseSame onSameVolume(const Zstring& folderLeft, const Zstring& folderRight); //throw()

enum ProcSymlink
{
    SYMLINK_DIRECT,
    SYMLINK_FOLLOW
};

Int64 getFileTime(const Zstring& filename, ProcSymlink procSl); //throw FileError
void setFileTime(const Zstring& filename, const Int64& modificationTime, ProcSymlink procSl); //throw FileError

//symlink handling: always evaluate target
UInt64 getFilesize(const Zstring& filename); //throw FileError
UInt64 getFreeDiskSpace(const Zstring& path); //throw FileError

//file handling
bool removeFile(const Zstring& filename); //throw FileError; return "false" if file is not existing
void removeDirectory(const Zstring& directory, CallbackRemoveDir* callback = nullptr); //throw FileError

//rename file or directory: no copying!!!
void renameFile(const Zstring& oldName, const Zstring& newName); //throw FileError, ErrorDifferentVolume, ErrorTargetExisting

bool supportsPermissions(const Zstring& dirname); //throw FileError, derefernces symlinks

//if parent directory not existing: create recursively:
void makeDirectory(const Zstring& directory, bool failIfExists = false); //throw FileError, ErrorTargetExisting

//fail if already existing or parent not existing:
//directory should not end with path separator
//templateDir may be empty
void makeDirectoryPlain(const Zstring& directory, const Zstring& templateDir, bool copyFilePermissions); //throw FileError, ErrorTargetExisting, ErrorTargetPathMissing

struct FileAttrib
{
    UInt64 fileSize;
    Int64 modificationTime; //time_t UTC compatible
    FileId sourceFileId;
    FileId targetFileId;
};

void copyFile(const Zstring& sourceFile, //throw FileError, ErrorTargetPathMissing, ErrorFileLocked (Windows-only)
              const Zstring& targetFile, //symlink handling: dereference source
              bool copyFilePermissions,
              bool transactionalCopy,
              CallbackCopyFile* callback, //may be nullptr
              FileAttrib* newAttrib = nullptr);  //return current attributes at the time of copy
//Note: it MAY happen that copyFile() leaves temp files behind, e.g. temporary network drop.
// => clean them up at an appropriate time (automatically set sync directions to delete them). They have the following ending:
const Zstring TEMP_FILE_ENDING = Zstr(".ffs_tmp");

void copySymlink(const Zstring& sourceLink, const Zstring& targetLink, bool copyFilePermissions); //throw FileError



//----------- callbacks ---------------
struct CallbackRemoveDir
{
    virtual ~CallbackRemoveDir() {}
    virtual void onBeforeFileDeletion(const Zstring& filename) = 0; //one call for each *existing* object!
    virtual void onBeforeDirDeletion (const Zstring& dirname ) = 0; //
};

struct CallbackCopyFile
{
    virtual ~CallbackCopyFile() {}

    //if target is existing user needs to implement deletion: copyFile() NEVER overwrites target if already existing!
    //if transactionalCopy == true, full read access on source had been proven at this point, so it's safe to delete it.
    virtual void deleteTargetFile(const Zstring& targetFile) = 0; //may throw exceptions

    //may throw:
    //Linux:   unconditionally
    //Windows: first exception is swallowed, updateCopyStatus() is then called again where it should throw again and the exception will propagate as expected
    virtual void updateCopyStatus(Int64 bytesDelta) = 0; //accummulated delta != file size! consider ADS, sparse, compressed files
};
}

#endif //FILE_HANDLING_H_INCLUDED
