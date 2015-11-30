// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl-3.0        *
// * Copyright (C) Zenju (zenju AT gmx DOT de) - All Rights Reserved        *
// **************************************************************************

#ifndef FILE_ID_DEF_H_013287632486321493
#define FILE_ID_DEF_H_013287632486321493

#include <utility>

    #include <sys/stat.h>


namespace zen
{
namespace impl { typedef struct ::stat StatDummy; } //sigh...

typedef decltype(impl::StatDummy::st_dev) DeviceId;
typedef decltype(impl::StatDummy::st_ino) FileIndex;

typedef std::pair<DeviceId, FileIndex> FileId;

inline
FileId extractFileId(const struct ::stat& fileInfo)
{
    return fileInfo.st_dev != 0 && fileInfo.st_ino != 0 ?
           FileId(fileInfo.st_dev, fileInfo.st_ino) : FileId();
}
}

#endif //FILE_ID_DEF_H_013287632486321493
