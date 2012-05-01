// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) ZenJu (zhnmju123 AT gmx DOT de) - All Rights Reserved    *
// **************************************************************************

#ifndef FILE_ID_INTERNAL_HEADER_013287632486321493
#define FILE_ID_INTERNAL_HEADER_013287632486321493

#include <utility>
#include "assert_static.h"

#ifdef FFS_WIN
#include "win.h" //includes "windows.h"

#elif defined FFS_LINUX
#include <sys/stat.h>
#endif


namespace zen
{
#ifdef FFS_WIN
typedef std::pair<DWORD, ULONGLONG> FileId; //(volume serial number, file ID)

inline
FileId extractFileID(const BY_HANDLE_FILE_INFORMATION& fileInfo)
{
    ULARGE_INTEGER uint = {};
    uint.HighPart = fileInfo.nFileIndexHigh;
    uint.LowPart  = fileInfo.nFileIndexLow;

    return fileInfo.dwVolumeSerialNumber != 0 && uint.QuadPart != 0 ?
           FileId(fileInfo.dwVolumeSerialNumber, uint.QuadPart) : FileId();
}

inline
FileId extractFileID(DWORD dwVolumeSerialNumber, ULARGE_INTEGER fileId)
{
    return dwVolumeSerialNumber != 0 && fileId.QuadPart != 0 ?
           FileId(dwVolumeSerialNumber, fileId.QuadPart) : FileId();
}

assert_static(sizeof(FileId().first ) == sizeof(BY_HANDLE_FILE_INFORMATION().dwVolumeSerialNumber));
assert_static(sizeof(FileId().second) == sizeof(BY_HANDLE_FILE_INFORMATION().nFileIndexHigh) + sizeof(BY_HANDLE_FILE_INFORMATION().nFileIndexLow));
assert_static(sizeof(FileId().second) == sizeof(ULARGE_INTEGER));


#elif defined FFS_LINUX
namespace impl { typedef struct ::stat StatDummy; } //sigh...

typedef std::pair<decltype(impl::StatDummy::st_dev), decltype(impl::StatDummy::st_ino)> FileId; //(device id, inode)

inline
FileId extractFileID(const struct ::stat& fileInfo)
{
    return fileInfo.st_dev != 0 && fileInfo.st_ino != 0 ?
           FileId(fileInfo.st_dev, fileInfo.st_ino) : FileId();
}
#endif
}

#endif //FILE_ID_INTERNAL_HEADER_013287632486321493