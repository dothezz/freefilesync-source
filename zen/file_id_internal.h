// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) 2008-2011 ZenJu (zhnmju123 AT gmx.de)                    *
// **************************************************************************

#ifndef FILE_ID_INTERNAL_HEADER_013287632486321493
#define FILE_ID_INTERNAL_HEADER_013287632486321493

#include <string>

#ifdef FFS_WIN
#include "win.h" //includes "windows.h"

#elif defined FFS_LINUX
#include <sys/stat.h>
#endif

namespace
{
template <class T> inline
std::string numberToBytes(T number)
{
    const char* rawBegin = reinterpret_cast<const char*>(&number);
    return std::string(rawBegin, rawBegin + sizeof(number));
}

#ifdef FFS_WIN
inline
std::string extractFileID(const BY_HANDLE_FILE_INFORMATION& fileInfo)
{
    return numberToBytes(fileInfo.dwVolumeSerialNumber) +
           numberToBytes(fileInfo.nFileIndexHigh) +
           numberToBytes(fileInfo.nFileIndexLow);
}
#elif defined FFS_LINUX
inline
std::string extractFileID(const struct stat& fileInfo)
{
    return numberToBytes(fileInfo.st_dev) +
           numberToBytes(fileInfo.st_ino);
}
#endif

}


#endif //FILE_ID_INTERNAL_HEADER_013287632486321493
