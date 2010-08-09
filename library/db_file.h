// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) 2008-2010 ZenJu (zhnmju123 AT gmx.de)                    *
// **************************************************************************
//
#ifndef DBFILE_H_INCLUDED
#define DBFILE_H_INCLUDED

#include "../file_hierarchy.h"

namespace ffs3
{
void saveToDisk(const BaseDirMapping& baseMapping); //throw (FileError)

struct DirInformation
{
    BaseFilter::FilterRef filter; //filter settings (used when retrieving directory data)
    DirContainer baseDirContainer; //hierarchical directory information
};
typedef boost::shared_ptr<const DirInformation> DirInfoPtr;

std::pair<DirInfoPtr, DirInfoPtr> loadFromDisk(const BaseDirMapping& baseMapping); //throw (FileError) -> return value always bound!
}

#endif // DBFILE_H_INCLUDED