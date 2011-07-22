// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) 2008-2011 ZenJu (zhnmju123 AT gmx.de)                    *
// **************************************************************************
//
#include "file_id.h"

#ifdef FFS_WIN
#include <wx/msw/wrapwin.h> //includes "windows.h"
#include "long_path_prefix.h"
#include "loki/ScopeGuard.h"

#elif defined FFS_LINUX
#include <sys/stat.h>
#endif


namespace
{
template <class T>
inline
std::string numberToBytes(T number)
{
    const char* rawBegin = reinterpret_cast<const char*>(&number);
    return std::string(rawBegin, rawBegin + sizeof(number));
}
}


std::string util::retrieveFileID(const Zstring& filename)
{
    std::string fileID;

#ifdef FFS_WIN
    //WARNING: CreateFile() is SLOW, while GetFileInformationByHandle() is cheap!
    //http://msdn.microsoft.com/en-us/library/aa363788(VS.85).aspx

    //privilege SE_BACKUP_NAME doesn't seem to be required here at all

    const HANDLE hFile = ::CreateFile(zen::applyLongPathPrefix(filename).c_str(),
                                      0,
                                      FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                                      NULL,
                                      OPEN_EXISTING,
                                      FILE_FLAG_OPEN_REPARSE_POINT | FILE_FLAG_BACKUP_SEMANTICS, //FILE_FLAG_BACKUP_SEMANTICS needed to open a directory
                                      NULL);
    if (hFile != INVALID_HANDLE_VALUE)
    {
        Loki::ScopeGuard dummy = Loki::MakeGuard(::CloseHandle, hFile);
        (void)dummy; //silence warning "unused variable"

        BY_HANDLE_FILE_INFORMATION fileInfo = {};
        if (::GetFileInformationByHandle(hFile, &fileInfo))
        {
            fileID += numberToBytes(fileInfo.dwVolumeSerialNumber);
            fileID += numberToBytes(fileInfo.nFileIndexHigh);
            fileID += numberToBytes(fileInfo.nFileIndexLow);
        }
    }

#elif defined FFS_LINUX
    struct stat fileInfo = {};
    if (::lstat(filename.c_str(), &fileInfo) == 0) //lstat() does not follow symlinks
    {
        fileID += numberToBytes(fileInfo.st_dev);
        fileID += numberToBytes(fileInfo.st_ino);
    }
#endif
    assert(!fileID.empty());
    return fileID;
}


bool util::sameFileSpecified(const Zstring& file1, const Zstring& file2)
{
    if (EqualFilename()(file1, file2)) //quick check
        return true;

    const std::string id1 = retrieveFileID(file1);
    const std::string id2 = retrieveFileID(file2);

    if (id1.empty() || id2.empty())
        return false;

    return id1 == id2;
}
