// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) Zenju (zenju AT gmx DOT de) - All Rights Reserved        *
// **************************************************************************

#ifndef RECYCLER_H_INCLUDED
#define RECYCLER_H_INCLUDED

#include <vector>
#include <zen/file_error.h>
#include <zen/zstring.h>

namespace zen
{
/*
--------------------
|Recycle Bin Access|
--------------------

Windows
-------
Recycler API always available: during runtime either SHFileOperation or IFileOperation (since Vista) will be dynamically selected

Linux
-----
Compiler flags: `pkg-config --cflags gio-2.0`
Linker   flags: `pkg-config --libs gio-2.0`

Already included in package "gtk+-2.0"!
*/

//move a file or folder to Recycle Bin (deletes permanently if recycler is not available) -> crappy semantics, but we have no choice thanks to Windows' design
bool recycleOrDelete(const Zstring& filename); //throw FileError, return "true" if file/dir was actually deleted


#ifdef FFS_WIN
enum StatusRecycler
{
    STATUS_REC_EXISTS,
    STATUS_REC_MISSING,
    STATUS_REC_UNKNOWN
};
StatusRecycler recycleBinStatus(const Zstring& pathName); //test existence of Recycle Bin API for certain path
//Win: blocks heavily if recycle bin is really full and drive is slow!!!

struct CallbackRecycling
{
    virtual ~CallbackRecycling() {}

    //may throw: first exception is swallowed, updateStatus() is then called again where it should throw again and the exception will propagate as expected
    virtual void updateStatus(const Zstring& currentItem) = 0; //currentItem may be empty
};

void recycleOrDelete(const std::vector<Zstring>& filenames, //throw FileError, return "true" if file/dir was actually deleted
                     CallbackRecycling* callback); //optional
#endif
}

#endif // RECYCLER_H_INCLUDED
