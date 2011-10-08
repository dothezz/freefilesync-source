// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) 2008-2011 ZenJu (zhnmju123 AT gmx.de)                    *
// **************************************************************************

#ifndef FILEID_H_INCLUDED
#define FILEID_H_INCLUDED

#include "zstring.h"
#include <string>

//unique file identifier

namespace zen
{
//get unique file id (symbolic link handling: opens the link!!!)
//returns empty string on error!
std::string getFileID(const Zstring& filename);

//test whether two distinct paths point to the same file or directory:
//      true: paths point to same files/dirs
//      false: error occurred OR point to different files/dirs
bool samePhysicalFile(const Zstring& file1, const Zstring& file2);
}

#endif // FILEID_H_INCLUDED
