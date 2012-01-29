// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) ZenJu (zhnmju123 AT gmx DOT de) - All Rights Reserved    *
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
struct CallbackMoveFile;
struct CallbackCopyFile;


bool fileExists     (const Zstring& filename); //throw()       check whether file      or symlink exists
bool dirExists      (const Zstring& dirname);  //throw()       check whether directory or symlink exists
bool symlinkExists  (const Zstring& objname);  //throw()       check whether a symbolic link exists
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

//file handling
bool removeFile(const Zstring& filename); //return "true" if file was actually deleted; throw FileError
void removeDirectory(const Zstring& directory, CallbackRemoveDir* callback = NULL); //throw FileError


//rename file or directory: no copying!!!
void renameFile(const Zstring& oldName, const Zstring& newName); //throw FileError;

//move source to target; expectations: all super-directories of target exist
//"ignoreExisting": if target already exists, source is deleted
void moveFile(const Zstring& sourceFile, const Zstring& targetFile, bool ignoreExisting, CallbackMoveFile* callback); //throw FileError;

//move source to target including subdirectories
//"ignoreExisting": existing directories and files will be enriched
void moveDirectory(const Zstring& sourceDir, const Zstring& targetDir, bool ignoreExisting, CallbackMoveFile* callback); //throw FileError;

bool supportsPermissions(const Zstring& dirname); //throw FileError, derefernces symlinks

//creates superdirectories automatically:
void createDirectory(const Zstring& directory, const Zstring& templateDir, bool copyFilePermissions); //throw FileError;
void createDirectory(const Zstring& directory); //throw FileError; -> function overload avoids default parameter ambiguity issues!

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
              CallbackCopyFile* callback, //may be NULL
              FileAttrib* newAttrib = NULL);  //return current attributes at the time of copy
//Note: it MAY happen that copyFile() leaves temp files behind, e.g. temporary network drop.
// => clean them up at an appropriate time (automatically set sync directions to delete them). They have the following ending:
const Zstring TEMP_FILE_ENDING = Zstr(".ffs_tmp");

void copySymlink(const Zstring& sourceLink, const Zstring& targetLink, bool copyFilePermissions); //throw FileError



//----------- callbacks ---------------
struct CallbackRemoveDir
{
    virtual ~CallbackRemoveDir() {}
    virtual void notifyFileDeletion(const Zstring& filename) = 0; //one call for each (existing) object!
    virtual void notifyDirDeletion (const Zstring& dirname ) = 0; //
};


struct CallbackCopyFile //callback functionality
{
    virtual ~CallbackCopyFile() {}

    //if target is existing user needs to implement deletion: copyFile() NEVER deletes target if already existing!
    //if transactionalCopy == true, full read access on source had been proven at this point, so it's safe to delete it.
    virtual void deleteTargetFile(const Zstring& targetFile) = 0; //may throw exceptions

    //may throw:
    //Linux:   unconditionally
    //Windows: first exception is swallowed, requestUiRefresh() is then called again where it should throw again and exception will propagate as expected
    virtual void updateCopyStatus(UInt64 totalBytesTransferred) = 0;
};


struct CallbackMoveFile //callback functionality
{
    virtual ~CallbackMoveFile() {} //see CallbackCopyFile for limitations when trowing exceptions!

    virtual void onBeforeFileMove(const Zstring& fileFrom, const Zstring& fileTo) = 0; //one call before each (planned) move
    virtual void onBeforeDirMove (const Zstring& dirFrom,  const Zstring& dirTo ) = 0; //
    virtual void objectProcessed() = 0; //one call after each completed move (count objects total)

    //called frequently if move has to revert to copy + delete:
    virtual void updateStatus(Int64 bytesDelta) = 0;
};
}

#endif //FILE_HANDLING_H_INCLUDED
