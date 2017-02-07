// *****************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under    *
// * GNU General Public License: http://www.gnu.org/licenses/gpl-3.0           *
// * Copyright (C) Zenju (zenju AT freefilesync DOT org) - All Rights Reserved *
// *****************************************************************************

#ifndef FILE_ACCESS_H_8017341345614857
#define FILE_ACCESS_H_8017341345614857

#include <functional>
#include "zstring.h"
#include "file_error.h"
#include "file_id_def.h"
#include "serialize.h"

namespace zen
{
//note: certain functions require COM initialization! (vista_file_op.h)

struct PathComponents
{
    Zstring rootPath; //itemPath = rootPath + (FILE_NAME_SEPARATOR?) + relPath
    Zstring relPath;  //
};
Opt<PathComponents> getPathComponents(const Zstring& itemPath); //no value on failure

Opt<Zstring> getParentFolderPath(const Zstring& itemPath);

//POSITIVE existence checks; if false: 1. item not existing 2. different type 3.device access error or similar
bool fileAvailable(const Zstring& filePath); //noexcept
bool dirAvailable (const Zstring& dirPath ); //
//NEGATIVE existence checks; if false: 1. item existing 2.device access error or similar
bool itemNotExisting(const Zstring& itemPath);

enum class ItemType
{
    FILE,
    FOLDER,
    SYMLINK,
};
//(hopefully) fast: does not distinguish between error/not existing
ItemType      getItemType        (const Zstring& itemPath); //throw FileError
//execute potentially SLOW folder traversal but distinguish error/not existing
Opt<ItemType> getItemTypeIfExists(const Zstring& itemPath); //throw FileError

struct PathDetails
{
    ItemType existingType;
    Zstring existingPath;         //itemPath =: existingPath + relPath
    std::vector<Zstring> relPath; //
};
PathDetails getPathDetails(const Zstring& itemPath); //throw FileError

enum class ProcSymlink
{
    DIRECT,
    FOLLOW
};
void setFileTime(const Zstring& filePath, int64_t modificationTime, ProcSymlink procSl); //throw FileError

//symlink handling: always evaluate target
uint64_t getFileSize(const Zstring& filePath); //throw FileError
uint64_t getFreeDiskSpace(const Zstring& path); //throw FileError, returns 0 if not available
VolumeId getVolumeId(const Zstring& itemPath); //throw FileError
//get per-user directory designated for temporary files:
Zstring getTempFolderPath(); //throw FileError

void removeFilePlain     (const Zstring& filePath);         //throw FileError; ERROR if not existing
void removeSymlinkPlain  (const Zstring& linkPath);         //throw FileError; ERROR if not existing
void removeDirectoryPlain(const Zstring& dirPath );         //throw FileError; ERROR if not existing
void removeDirectoryPlainRecursion(const Zstring& dirPath); //throw FileError; ERROR if not existing

//rename file or directory: no copying!!!
void renameFile(const Zstring& itemPathOld, const Zstring& itemPathNew); //throw FileError, ErrorDifferentVolume, ErrorTargetExisting

bool supportsPermissions(const Zstring& dirPath); //throw FileError, dereferences symlinks

//- no error if already existing
//- create recursively if parent directory is not existing
void createDirectoryIfMissingRecursion(const Zstring& dirPath); //throw FileError

//fail if already existing or parent directory not existing:
//source path is optional (may be empty)
void copyNewDirectory(const Zstring& sourcePath, const Zstring& targetPath, bool copyFilePermissions); //throw FileError, ErrorTargetExisting

void copySymlink(const Zstring& sourceLink, const Zstring& targetLink, bool copyFilePermissions); //throw FileError

struct InSyncAttributes
{
    uint64_t fileSize = 0;
    int64_t modificationTime = 0; //time_t UTC compatible
    FileId sourceFileId;
    FileId targetFileId;
};

InSyncAttributes copyNewFile(const Zstring& sourceFile, const Zstring& targetFile, bool copyFilePermissions, //throw FileError, ErrorTargetExisting, ErrorFileLocked
                             //accummulated delta != file size! consider ADS, sparse, compressed files
                             const IOCallback& notifyUnbufferedIO); //may be nullptr; throw X!
}

#endif //FILE_ACCESS_H_8017341345614857
