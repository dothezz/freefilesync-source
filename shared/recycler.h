// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) 2008-2010 ZenJu (zhnmju123 AT gmx.de)                    *
// **************************************************************************
//
#ifndef RECYCLER_H_INCLUDED
#define RECYCLER_H_INCLUDED

#include "fileError.h"
#include "zstring.h"

namespace FreeFileSync
{
/*
--------------------
|Recycle Bin Access|
--------------------

Windows
-------
Recycler always available: during runtime dynamically either SHFileOperation or (since Vista) IFileOperation will be selected

Linux
-----
During compilation set:
RECYCLER_GIO if available ("pkg-config --exists gio-2.0")
or
RECYCLER_NONE to disable the recycler
*/

bool recycleBinExists(); //test existence of Recycle Bin API on current system

//move a file or folder to Recycle Bin
void moveToRecycleBin(const Zstring& fileToDelete);  //throw (FileError)
}

#endif // RECYCLER_H_INCLUDED
