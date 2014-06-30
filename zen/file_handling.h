// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) Zenju (zenju AT gmx DOT de) - All Rights Reserved        *
// **************************************************************************

#ifndef FILE_HANDLING_H_8017341345614857
#define FILE_HANDLING_H_8017341345614857

#include <functional>
#include "zstring.h"
#include "file_error.h"
#include "file_id_def.h"
#include "int64.h"

namespace zen
{
bool fileExists     (const Zstring& filename); //noexcept; check whether file      or file-symlink exists
bool dirExists      (const Zstring& dirname ); //noexcept; check whether directory or dir-symlink exists
bool symlinkExists  (const Zstring& linkname); //noexcept; check whether a symbolic link exists
bool somethingExists(const Zstring& objname ); //noexcept; check whether any object with this name exists

enum class ProcSymlink
{
    DIRECT,
    FOLLOW
};

void setFileTime(const Zstring& filename, const Int64& modificationTime, ProcSymlink procSl); //throw FileError

//symlink handling: always evaluate target
UInt64 getFilesize(const Zstring& filename); //throw FileError
UInt64 getFreeDiskSpace(const Zstring& path); //throw FileError

bool removeFile(const Zstring& filename); //throw FileError; return "false" if file is not existing
void removeDirectory(const Zstring& directory, //throw FileError
                     const std::function<void (const Zstring& filename)>& onBeforeFileDeletion = nullptr,  //optional;
                     const std::function<void (const Zstring& dirname)>&  onBeforeDirDeletion  = nullptr); //one call for each *existing* object!

//rename file or directory: no copying!!!
void renameFile(const Zstring& oldName, const Zstring& newName); //throw FileError, ErrorDifferentVolume, ErrorTargetExisting

bool supportsPermissions(const Zstring& dirname); //throw FileError, dereferences symlinks

//if parent directory not existing: create recursively:
void makeDirectory(const Zstring& directory, bool failIfExists = false); //throw FileError, ErrorTargetExisting

//fail if already existing or parent directory not existing:
//directory should not end with path separator
//templateDir may be empty
void makeDirectoryPlain(const Zstring& directory, const Zstring& templateDir, bool copyFilePermissions); //throw FileError, ErrorTargetExisting, ErrorTargetPathMissing

struct InSyncAttributes
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
              //if target is existing user needs to implement deletion: copyFile() NEVER overwrites target if already existing!
              //if transactionalCopy == true, full read access on source had been proven at this point, so it's safe to delete it.
              const std::function<void()>& onDeleteTargetFile, //may be nullptr; throw X!
              //Linux:   unconditionally
              //Windows: first exception is swallowed, updateCopyStatus() is then called again where it should throw again and the exception will propagate as expected
              //accummulated delta != file size! consider ADS, sparse, compressed files
              const std::function<void(Int64 bytesDelta)>& onUpdateCopyStatus, //may be nullptr; throw X!

              InSyncAttributes* newAttrib = nullptr);  //return current attributes at the time of copy

//Note: it MAY happen that copyFile() leaves temp files behind, e.g. temporary network drop.
// => clean them up at an appropriate time (automatically set sync directions to delete them). They have the following ending:

const Zchar TEMP_FILE_ENDING[] = Zstr(".ffs_tmp"); //don't use Zstring as global constant: avoid static initialization order problem in global namespace!

void copySymlink(const Zstring& sourceLink, const Zstring& targetLink, bool copyFilePermissions); //throw FileError
}

#endif //FILE_HANDLING_H_8017341345614857
