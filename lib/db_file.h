// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) Zenju (zenju AT gmx DOT de) - All Rights Reserved        *
// **************************************************************************

#ifndef DBFILE_H_834275398588021574
#define DBFILE_H_834275398588021574

#include <zen/file_error.h>
#include "../file_hierarchy.h"

namespace zen
{
const Zstring SYNC_DB_FILE_ENDING = Zstr(".ffs_db");

enum InSyncType
{
    IN_SYNC_BINARY_EQUAL,     //checked file content
    IN_SYNC_ATTRIBUTES_EQUAL, //only "looks" like they're equal
};

struct InSyncDescrFile //subset of FileDescriptor
{
    InSyncDescrFile(const Int64& lastWriteTimeRawIn,
                    const FileId& idIn) :
        lastWriteTimeRaw(lastWriteTimeRawIn),
        fileId(idIn) {}

    Int64  lastWriteTimeRaw;
    FileId fileId; // == file id: optional! (however, always set on Linux, and *generally* available on Windows)
};

struct InSyncDescrLink
{
    explicit InSyncDescrLink(const Int64& lastWriteTimeRawIn) : lastWriteTimeRaw(lastWriteTimeRawIn) {}
    Int64 lastWriteTimeRaw;
};


//artificial hierarchy of last synchronous state:
struct InSyncFile
{
    InSyncFile(const InSyncDescrFile& l, const InSyncDescrFile& r, InSyncType type, const UInt64& fileSizeIn) : left(l), right(r), inSyncType(type), fileSize(fileSizeIn) {}
    InSyncDescrFile left;
    InSyncDescrFile right;
    InSyncType inSyncType;
    UInt64 fileSize; //file size must be identical on both sides!
};

struct InSyncSymlink
{
    InSyncSymlink(const InSyncDescrLink& l, const InSyncDescrLink& r, InSyncType type) : left(l), right(r), inSyncType(type) {}
    InSyncDescrLink left;
    InSyncDescrLink right;
    InSyncType inSyncType;
};

struct InSyncDir
{
    //for directories we have a logical problem: we cannot have "not existent" as an indicator for
    //"no last synchronous state" since this precludes child elements that may be in sync!
    enum InSyncStatus
    {
        DIR_STATUS_IN_SYNC,
        DIR_STATUS_STRAW_MAN //there is no last synchronous state, but used as container only
    };
    InSyncDir(InSyncStatus statusIn) : status(statusIn) {}

    InSyncStatus status;

    //------------------------------------------------------------------
    typedef std::map<Zstring, InSyncDir,     LessFilename> DirList;  //
    typedef std::map<Zstring, InSyncFile,    LessFilename> FileList; // key: shortName
    typedef std::map<Zstring, InSyncSymlink, LessFilename> LinkList; //
    //------------------------------------------------------------------

    DirList  dirs;
    FileList files;
    LinkList symlinks; //non-followed symlinks

    //convenience
    InSyncDir& addDir(const Zstring& shortName, InSyncStatus st)
    {
        //use C++11 emplace when available
        return dirs.insert(std::make_pair(shortName, InSyncDir(st))).first->second;
    }

    void addFile(const Zstring& shortName, const InSyncDescrFile& dataL, const InSyncDescrFile& dataR, InSyncType type, const UInt64& fileSize)
    {
        files.insert(std::make_pair(shortName, InSyncFile(dataL, dataR, type, fileSize)));
    }

    void addSymlink(const Zstring& shortName, const InSyncDescrLink& dataL, const InSyncDescrLink& dataR, InSyncType type)
    {
        symlinks.insert(std::make_pair(shortName, InSyncSymlink(dataL, dataR, type)));
    }
};


DEFINE_NEW_FILE_ERROR(FileErrorDatabaseNotExisting);

std::shared_ptr<InSyncDir> loadLastSynchronousState(const BaseDirPair& baseDirObj); //throw FileError, FileErrorDatabaseNotExisting -> return value always bound!

void saveLastSynchronousState(const BaseDirPair& baseDirObj); //throw FileError
}

#endif //DBFILE_H_834275398588021574
