// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) 2008-2011 ZenJu (zhnmju123 AT gmx.de)                    *
// **************************************************************************

#ifndef SYMLINK_WIN_H_INCLUDED
#define SYMLINK_WIN_H_INCLUDED

#include "loki/ScopeGuard.h"
#include "last_error.h"
#include "file_error.h"
#include "i18n.h"

#ifdef FFS_WIN
#include <wx/msw/wrapwin.h> //includes "windows.h"
#include "WinIoCtl.h"
#include "privilege.h"
#include "long_path_prefix.h"

#elif defined FFS_LINUX
#include <unistd.h>
#endif


#ifdef _MSC_VER //I don't have Windows Driver Kit at hands right now, so unfortunately we need to redefine this structures and cross fingers...

//from ntifs.h
typedef struct _REPARSE_DATA_BUFFER
{
    ULONG  ReparseTag;
    USHORT ReparseDataLength;
    USHORT Reserved;
    union
    {
        struct
        {
            USHORT SubstituteNameOffset;
            USHORT SubstituteNameLength;
            USHORT PrintNameOffset;
            USHORT PrintNameLength;
            ULONG  Flags;
            WCHAR  PathBuffer[1];
        } SymbolicLinkReparseBuffer;
        struct
        {
            USHORT SubstituteNameOffset;
            USHORT SubstituteNameLength;
            USHORT PrintNameOffset;
            USHORT PrintNameLength;
            WCHAR  PathBuffer[1];
        } MountPointReparseBuffer;
        struct
        {
            UCHAR DataBuffer[1];
        } GenericReparseBuffer;
    };
} REPARSE_DATA_BUFFER, *PREPARSE_DATA_BUFFER;
#define REPARSE_DATA_BUFFER_HEADER_SIZE   FIELD_OFFSET(REPARSE_DATA_BUFFER, GenericReparseBuffer)
#endif

namespace
{
//retrieve raw target data of symlink or junction
Zstring getSymlinkRawTargetString(const Zstring& linkPath) //throw FileError
{
    using namespace zen;
#ifdef FFS_WIN
    //FSCTL_GET_REPARSE_POINT: http://msdn.microsoft.com/en-us/library/aa364571(VS.85).aspx

    try //reading certain symlinks requires admin rights! This shall not cause an error in user mode!
    {
        //allow access to certain symbolic links/junctions
        Privileges::getInstance().ensureActive(SE_BACKUP_NAME); //throw FileError
    }
    catch (...) {}

    const HANDLE hLink = ::CreateFile(applyLongPathPrefix(linkPath).c_str(),
                                      GENERIC_READ,
                                      FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                                      0,
                                      OPEN_EXISTING,
                                      FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OPEN_REPARSE_POINT,
                                      NULL);
    if (hLink == INVALID_HANDLE_VALUE)
        throw FileError(_("Error resolving symbolic link:") + "\n\"" + linkPath + "\"" + "\n\n" + getLastErrorFormatted());
    LOKI_ON_BLOCK_EXIT2(::CloseHandle(hLink));

    //respect alignment issues...
    const DWORD bufferSize = REPARSE_DATA_BUFFER_HEADER_SIZE + MAXIMUM_REPARSE_DATA_BUFFER_SIZE;
    std::vector<char> buffer(bufferSize);

    DWORD bytesReturned; //dummy value required by FSCTL_GET_REPARSE_POINT!
    if (!::DeviceIoControl(hLink,                   //__in         HANDLE hDevice,
                           FSCTL_GET_REPARSE_POINT, //__in         DWORD dwIoControlCode,
                           NULL,                    //__in_opt     LPVOID lpInBuffer,
                           0,                       //__in         DWORD nInBufferSize,
                           &buffer[0],              //__out_opt    LPVOID lpOutBuffer,
                           bufferSize,              //__in         DWORD nOutBufferSize,
                           &bytesReturned,          //__out_opt    LPDWORD lpBytesReturned,
                           NULL))                   //__inout_opt  LPOVERLAPPED lpOverlapped
        throw FileError(_("Error resolving symbolic link:") + "\n\"" + linkPath + "\"" + "\n\n" + getLastErrorFormatted());

    REPARSE_DATA_BUFFER& reparseData = *reinterpret_cast<REPARSE_DATA_BUFFER*>(&buffer[0]); //REPARSE_DATA_BUFFER needs to be artificially enlarged!

    Zstring output;
    if (reparseData.ReparseTag == IO_REPARSE_TAG_SYMLINK)
    {
        output = Zstring(reparseData.SymbolicLinkReparseBuffer.PathBuffer + reparseData.SymbolicLinkReparseBuffer.SubstituteNameOffset / sizeof(WCHAR),
                         reparseData.SymbolicLinkReparseBuffer.SubstituteNameLength / sizeof(WCHAR));
    }
    else if (reparseData.ReparseTag == IO_REPARSE_TAG_MOUNT_POINT)
    {
        output = Zstring(reparseData.MountPointReparseBuffer.PathBuffer + reparseData.MountPointReparseBuffer.SubstituteNameOffset / sizeof(WCHAR),
                         reparseData.MountPointReparseBuffer.SubstituteNameLength / sizeof(WCHAR));
    }
    else
        throw FileError(_("Error resolving symbolic link:") + "\n\"" + linkPath + "\"" + "\n\n" + "Not a symbolic link or junction!");

    //absolute symlinks and junctions technically start with \??\ while relative ones do not
    if (output.StartsWith(Zstr("\\??\\")))
        output = Zstring(output.c_str() + 4, output.length() - 4);

    return output;

#elif defined FFS_LINUX
    const int BUFFER_SIZE = 10000;
    std::vector<char> buffer(BUFFER_SIZE);

    const int bytesWritten = ::readlink(linkPath.c_str(), &buffer[0], BUFFER_SIZE);
    if (bytesWritten < 0 || bytesWritten >= BUFFER_SIZE)
    {
        std::wstring errorMessage = _("Error resolving symbolic link:") + "\n\"" + linkPath + "\"";
        if (bytesWritten < 0)
            errorMessage += L"\n\n" + getLastErrorFormatted();
        throw FileError(errorMessage);
    }
    buffer[bytesWritten] = 0; //set null-terminating char

    return Zstring(&buffer[0], bytesWritten);
#endif
}
}

#endif // SYMLINK_WIN_H_INCLUDED
