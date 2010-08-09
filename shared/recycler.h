// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) 2008-2010 ZenJu (zhnmju123 AT gmx.de)                    *
// **************************************************************************
//
#ifndef RECYCLER_H_INCLUDED
#define RECYCLER_H_INCLUDED

#include "file_error.h"
#include "zstring.h"

namespace ffs3
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
Include compilation flag:
`pkg-config --cflags gtkmm-2.4`

Linker flag:
`pkg-config --libs gtkmm-2.4`
*/

bool recycleBinExists(); //test existence of Recycle Bin API on current system

//move a file or folder to Recycle Bin
void moveToRecycleBin(const Zstring& fileToDelete);  //throw (FileError)
}

#endif // RECYCLER_H_INCLUDED
