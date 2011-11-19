// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) 2008-2011 ZenJu (zhnmju123 AT gmx.de)                    *
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
typedef std::pair<decltype(BY_HANDLE_FILE_INFORMATION().dwVolumeSerialNumber), decltype(ULARGE_INTEGER().QuadPart)> FileId; //(volume serial number, file ID)

inline
FileId extractFileID(const BY_HANDLE_FILE_INFORMATION& fileInfo)
{
    ULARGE_INTEGER uint = {};
    uint.HighPart = fileInfo.nFileIndexHigh;
    uint.LowPart  = fileInfo.nFileIndexLow;
    return std::make_pair(fileInfo.dwVolumeSerialNumber, uint.QuadPart);
}

inline
FileId extractFileID(DWORD dwVolumeSerialNumber, ULARGE_INTEGER fileId)
{
    return std::make_pair(dwVolumeSerialNumber, fileId.QuadPart);
}

namespace impl
{
inline
void validate(const FileId& id, const BY_HANDLE_FILE_INFORMATION& fileInfo)
{
    assert_static(sizeof(id.second) == sizeof(fileInfo.nFileIndexHigh) + sizeof(fileInfo.nFileIndexLow));
    assert_static(sizeof(id.first ) == sizeof(DWORD));
    assert_static(sizeof(id.second) == sizeof(ULARGE_INTEGER));
}
}

#elif defined FFS_LINUX
namespace impl { typedef struct ::stat StatDummy; } //sigh...

typedef std::pair<decltype(impl::StatDummy::st_dev), decltype(impl::StatDummy::st_ino)> FileId; //(device id, inode)

inline
FileId extractFileID(const struct stat& fileInfo)
{
    return std::make_pair(fileInfo.st_dev, fileInfo.st_ino);
}
#endif
}

#endif //FILE_ID_INTERNAL_HEADER_013287632486321493
