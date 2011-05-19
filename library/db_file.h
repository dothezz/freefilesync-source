// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) 2008-2011 ZenJu (zhnmju123 AT gmx.de)                    *
// **************************************************************************
//
#ifndef DBFILE_H_INCLUDED
#define DBFILE_H_INCLUDED

#include "../shared/file_error.h"
#include "../file_hierarchy.h"

namespace zen
{
void saveToDisk(const BaseDirMapping& baseMapping); //throw (FileError)

struct DirInformation
{
    HardFilter::FilterRef filter; //filter settings (used when retrieving directory data)
    DirContainer baseDirContainer; //hierarchical directory information
};
typedef boost::shared_ptr<const DirInformation> DirInfoPtr;

DEFINE_NEW_FILE_ERROR(FileErrorDatabaseNotExisting);

std::pair<DirInfoPtr, DirInfoPtr> loadFromDisk(const BaseDirMapping& baseMapping); //throw (FileError, FileErrorDatabaseNotExisting) -> return value always bound!
}

#endif // DBFILE_H_INCLUDED
