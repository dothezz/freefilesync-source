// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) 2008-2010 ZenJu (zhnmju123 AT gmx.de)                    *
// **************************************************************************
//
#include "file_id.h"

#ifdef FFS_WIN
#include "assert_static.h"
#include <wx/msw/wrapwin.h> //includes "windows.h"
#include "long_path_prefix.h"
#include <boost/shared_ptr.hpp>

#elif defined FFS_LINUX
#endif



#ifdef FFS_WIN
util::FileID util::retrieveFileID(const Zstring& filename)
{
    //ensure our DWORD_FFS really is the same as DWORD
    assert_static(sizeof(util::FileID::DWORD_FFS) == sizeof(DWORD));

//WARNING: CreateFile() is SLOW, while GetFileInformationByHandle() is quite cheap!
//http://msdn.microsoft.com/en-us/library/aa363788(VS.85).aspx

    const HANDLE hFile = ::CreateFile(ffs3::applyLongPathPrefix(filename).c_str(),
                                      0,
                                      FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                                      NULL,
                                      OPEN_EXISTING,
                                      FILE_FLAG_OPEN_REPARSE_POINT | FILE_FLAG_BACKUP_SEMANTICS, //FILE_FLAG_BACKUP_SEMANTICS needed to open a directory
                                      NULL);
    if (hFile != INVALID_HANDLE_VALUE)
    {
        boost::shared_ptr<void> dummy(hFile, ::CloseHandle);

        BY_HANDLE_FILE_INFORMATION info;
        if (::GetFileInformationByHandle(hFile, &info))
        {
            return util::FileID(info.dwVolumeSerialNumber,
                                   info.nFileIndexHigh,
                                   info.nFileIndexLow);
        }
    }
    return util::FileID(); //empty ID
}


#elif defined FFS_LINUX
util::FileID util::retrieveFileID(const Zstring& filename)
{
    struct stat fileInfo;
    if (::lstat(filename.c_str(), &fileInfo) == 0) //lstat() does not resolve symlinks
        return util::FileID(fileInfo.st_dev, fileInfo.st_ino);

    return util::FileID(); //empty ID
}
#endif


bool util::sameFileSpecified(const Zstring& file1, const Zstring& file2)
{
    const util::FileID id1 = retrieveFileID(file1);
    const util::FileID id2 = retrieveFileID(file2);

    if (id1 != FileID() && id2 != FileID())
        return id1 == id2;

    return false;
}
