// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) ZenJu (zhnmju123 AT gmx DOT de) - All Rights Reserved    *
// **************************************************************************

#include "file_id.h"

#ifdef FFS_WIN
#include "win.h" //includes "windows.h"
#include "long_path_prefix.h"
#include "scope_guard.h"

#elif defined FFS_LINUX
#include <sys/stat.h>
#endif


zen::FileId zen::getFileID(const Zstring& filename)
{
#ifdef FFS_WIN
    //WARNING: CreateFile() is SLOW, while GetFileInformationByHandle() is cheap!
    //http://msdn.microsoft.com/en-us/library/aa363788(VS.85).aspx

    //privilege SE_BACKUP_NAME doesn't seem to be required here at all

    const HANDLE hFile = ::CreateFile(zen::applyLongPathPrefix(filename).c_str(),
                                      0,
                                      FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                                      nullptr,
                                      OPEN_EXISTING,
                                      FILE_FLAG_OPEN_REPARSE_POINT | FILE_FLAG_BACKUP_SEMANTICS, //FILE_FLAG_BACKUP_SEMANTICS needed to open a directory
                                      nullptr);
    if (hFile != INVALID_HANDLE_VALUE)
    {
        ZEN_ON_SCOPE_EXIT(::CloseHandle(hFile));

        BY_HANDLE_FILE_INFORMATION fileInfo = {};
        if (::GetFileInformationByHandle(hFile, &fileInfo))
            return extractFileID(fileInfo);
    }

#elif defined FFS_LINUX
    struct ::stat fileInfo = {};
    if (::stat(filename.c_str(), &fileInfo) == 0) //stat() follows symlinks
        return extractFileID(fileInfo);
#endif

    return zen::FileId();
}


bool zen::samePhysicalFile(const Zstring& file1, const Zstring& file2)
{
    if (EqualFilename()(file1, file2)) //quick check
        return true;

    const auto id1 = getFileID(file1);
    const auto id2 = getFileID(file2);

    if (id1 == zen::FileId() || id2 == zen::FileId())
        return false;

    return id1 == id2;
}
