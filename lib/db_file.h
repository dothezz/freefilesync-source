// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) 2008-2011 ZenJu (zhnmju123 AT gmx.de)                    *
// **************************************************************************

#ifndef DBFILE_H_INCLUDED
#define DBFILE_H_INCLUDED

#include <zen/file_error.h>
#include "../file_hierarchy.h"

namespace zen
{
const Zstring SYNC_DB_FILE_ENDING = Zstr(".ffs_db");

struct DirInformation
{
    DirContainer baseDirContainer; //hierarchical directory information
};
typedef std::shared_ptr<const DirInformation> DirInfoPtr;

DEFINE_NEW_FILE_ERROR(FileErrorDatabaseNotExisting);

std::pair<DirInfoPtr, DirInfoPtr> loadFromDisk(const BaseDirMapping& baseMapping); //throw FileError, FileErrorDatabaseNotExisting -> return value always bound!

void saveToDisk(const BaseDirMapping& baseMapping); //throw FileError
}

#endif // DBFILE_H_INCLUDED
