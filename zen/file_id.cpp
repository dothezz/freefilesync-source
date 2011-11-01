// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) 2008-2011 ZenJu (zhnmju123 AT gmx.de)                    *
// **************************************************************************

#include "file_id.h"
#include "file_id_internal.h"

#ifdef FFS_WIN
#include "win.h" //includes "windows.h"
#include "long_path_prefix.h"
#include "scope_guard.h"

#elif defined FFS_LINUX
#include <sys/stat.h>
#endif


std::string zen::getFileID(const Zstring& filename)
{
#ifdef FFS_WIN
    //WARNING: CreateFile() is SLOW, while GetFileInformationByHandle() is cheap!
    //http://msdn.microsoft.com/en-us/library/aa363788(VS.85).aspx

    //privilege SE_BACKUP_NAME doesn't seem to be required here at all

    const HANDLE hFile = ::CreateFile(zen::applyLongPathPrefix(filename).c_str(),
                                      0,
                                      FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                                      0,
                                      OPEN_EXISTING,
                                      FILE_FLAG_OPEN_REPARSE_POINT | FILE_FLAG_BACKUP_SEMANTICS, //FILE_FLAG_BACKUP_SEMANTICS needed to open a directory
                                      NULL);
    if (hFile != INVALID_HANDLE_VALUE)
    {
        ZEN_ON_BLOCK_EXIT(::CloseHandle(hFile));

        BY_HANDLE_FILE_INFORMATION fileInfo = {};
        if (::GetFileInformationByHandle(hFile, &fileInfo))
            return extractFileID(fileInfo);
    }

#elif defined FFS_LINUX
    struct ::stat fileInfo = {};
    if (::lstat(filename.c_str(), &fileInfo) == 0) //lstat() does not follow symlinks
        return extractFileID(fileInfo);
#endif

    return std::string();
}


bool zen::samePhysicalFile(const Zstring& file1, const Zstring& file2)
{
    if (EqualFilename()(file1, file2)) //quick check
        return true;

    const std::string id1 = getFileID(file1);
    const std::string id2 = getFileID(file2);

    if (id1.empty() || id2.empty())
        return false;

    return id1 == id2;
}
