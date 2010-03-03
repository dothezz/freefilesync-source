// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) 2008-2010 ZenJu (zhnmju123 AT gmx.de)                    *
// **************************************************************************
//
#include "fileID.h"

#ifdef FFS_WIN
#include "staticAssert.h"
#include <wx/msw/wrapwin.h> //includes "windows.h"
#include "longPathPrefix.h"
#include <boost/shared_ptr.hpp>

#elif defined FFS_LINUX

#endif



#ifdef FFS_WIN
Utility::FileID Utility::retrieveFileID(const Zstring& filename)
{
    //ensure our DWORD_FFS really is the same as DWORD
    assert_static(sizeof(Utility::FileID::DWORD_FFS) == sizeof(DWORD));

//WARNING: CreateFile() is SLOW, while GetFileInformationByHandle() is quite cheap!
//http://msdn.microsoft.com/en-us/library/aa363788(VS.85).aspx

    const HANDLE hFile = ::CreateFile(FreeFileSync::applyLongPathPrefix(filename).c_str(),
                                      0,
                                      FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                                      NULL,
                                      OPEN_EXISTING,
                                      FILE_FLAG_OPEN_REPARSE_POINT | FILE_FLAG_BACKUP_SEMANTICS, //FILE_FLAG_BACKUP_SEMANTICS needed to open directories
                                      NULL);
    if (hFile != INVALID_HANDLE_VALUE)
    {
         boost::shared_ptr<void> dummy(hFile, ::CloseHandle);

        BY_HANDLE_FILE_INFORMATION info;
        if (::GetFileInformationByHandle(hFile, &info))
        {
            return Utility::FileID(info.dwVolumeSerialNumber,
                                   info.nFileIndexHigh,
                                   info.nFileIndexLow);
        }
    }
    return Utility::FileID(); //empty ID
}


#elif defined FFS_LINUX
Utility::FileID Utility::retrieveFileID(const Zstring& filename)
{
    struct stat fileInfo;
    if (::lstat(filename.c_str(), &fileInfo) == 0) //lstat() does not resolve symlinks
        return Utility::FileID(fileInfo.st_dev, fileInfo.st_ino);

    return Utility::FileID(); //empty ID
}
#endif


bool Utility::sameFileSpecified(const Zstring& file1, const Zstring& file2)
{
    const Utility::FileID id1 = retrieveFileID(file1);
    const Utility::FileID id2 = retrieveFileID(file2);

    if (id1 != FileID() && id2 != FileID())
        return id1 == id2;

    return false;
}
