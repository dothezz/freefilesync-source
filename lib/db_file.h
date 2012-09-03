// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) ZenJu (zenju AT gmx DOT de) - All Rights Reserved        *
// **************************************************************************

#ifndef DBFILE_H_INCLUDED
#define DBFILE_H_INCLUDED

#include <zen/file_error.h>
#include "../file_hierarchy.h"

namespace zen
{
const Zstring SYNC_DB_FILE_ENDING = Zstr(".ffs_db");

//artificial hierarchy of last synchronous state:
struct InSyncFile
{
    enum InSyncType
    {
        IN_SYNC_BINARY_EQUAL,     //checked file content
        IN_SYNC_ATTRIBUTES_EQUAL, //only "looks" like they're equal
    };

    InSyncFile(const FileDescriptor& l, const FileDescriptor& r, InSyncType type) : left(l), right(r), inSyncType(type) {}
    FileDescriptor left;
    FileDescriptor right;
    InSyncType inSyncType;
};

struct InSyncSymlink
{
    InSyncSymlink(const LinkDescriptor& l, const LinkDescriptor& r) : left(l), right(r) {}
    LinkDescriptor left;
    LinkDescriptor right;
};

struct InSyncDir
{
    //for directories we have a logical problem: we cannot have "not existent" as an indicator for "no last synchronous state" since this precludes
    //child elements that may be in sync!
    enum InSyncStatus
    {
        STATUS_IN_SYNC,
        STATUS_STRAW_MAN //there is no last synchronous state, but used as container only
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
    InSyncDir& addDir(const Zstring& shortName, InSyncStatus statusIn)
    {
        //use C++11 emplace when available
        return dirs.insert(std::make_pair(shortName, InSyncDir(statusIn))).first->second;
    }

    void addFile(const Zstring& shortName, const FileDescriptor& dataL, const FileDescriptor& dataR, InSyncFile::InSyncType type)
    {
        files.insert(std::make_pair(shortName, InSyncFile(dataL, dataR, type)));
    }

    void addSymlink(const Zstring& shortName, const LinkDescriptor& dataL, const LinkDescriptor& dataR)
    {
        symlinks.insert(std::make_pair(shortName, InSyncSymlink(dataL, dataR)));
    }
};


DEFINE_NEW_FILE_ERROR(FileErrorDatabaseNotExisting);

std::shared_ptr<InSyncDir> loadLastSynchronousState(const BaseDirMapping& baseMapping); //throw FileError, FileErrorDatabaseNotExisting -> return value always bound!

void saveLastSynchronousState(const BaseDirMapping& baseMapping); //throw FileError
}

#endif // DBFILE_H_INCLUDED
