// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) 2008-2010 ZenJu (zhnmju123 AT gmx.de)                    *
// **************************************************************************
//
#include "binary.h"
#include <boost/scoped_array.hpp>
#include <wx/intl.h>
#include "../shared/stringConv.h"

#ifdef FFS_WIN
#include "../shared/longPathPrefix.h"
#include <wx/msw/wrapwin.h> //includes "windows.h"
#include <boost/shared_ptr.hpp>

#elif defined FFS_LINUX
#include <wx/ffile.h>
#endif


bool FreeFileSync::filesHaveSameContent(const Zstring& filename1, const Zstring& filename2, CompareCallback* callback)
{
    const size_t BUFFER_SIZE = 512 * 1024; //512 kb seems to be the perfect buffer size
    static boost::scoped_array<unsigned char> memory1(new unsigned char[BUFFER_SIZE]);
    static boost::scoped_array<unsigned char> memory2(new unsigned char[BUFFER_SIZE]);

#ifdef FFS_WIN
    const HANDLE hFile1 = ::CreateFile(FreeFileSync::applyLongPathPrefix(filename1).c_str(),
                                       GENERIC_READ,
                                       FILE_SHARE_READ,
                                       NULL,
                                       OPEN_EXISTING,
                                       FILE_FLAG_OPEN_REPARSE_POINT | FILE_FLAG_SEQUENTIAL_SCAN,
                                       NULL);
    if (hFile1 == INVALID_HANDLE_VALUE)
        throw FileError(wxString(_("Error opening file:")) + wxT(" \"") + zToWx(filename1) + wxT("\""));

    boost::shared_ptr<void> dummy1(hFile1, &::CloseHandle);

    const HANDLE hFile2 = ::CreateFile(FreeFileSync::applyLongPathPrefix(filename2).c_str(),
                                       GENERIC_READ,
                                       FILE_SHARE_READ,
                                       NULL,
                                       OPEN_EXISTING,
                                       FILE_FLAG_OPEN_REPARSE_POINT | FILE_FLAG_SEQUENTIAL_SCAN,
                                       NULL);
    if (hFile2 == INVALID_HANDLE_VALUE)
        throw FileError(wxString(_("Error opening file:")) + wxT(" \"") + zToWx(filename2) + wxT("\""));

    boost::shared_ptr<void> dummy2(hFile2, &::CloseHandle);

    wxLongLong bytesCompared;
    DWORD length1 = 0;
    do
    {
        if (!::ReadFile(
                    hFile1,        //__in         HANDLE hFile,
                    memory1.get(), //__out        LPVOID lpBuffer,
                    BUFFER_SIZE,   //__in         DWORD nNumberOfBytesToRead,
                    &length1,      //__out_opt    LPDWORD lpNumberOfBytesRead,
                    NULL))         //__inout_opt  LPOVERLAPPED lpOverlapped
            throw FileError(wxString(_("Error reading file:")) + wxT(" \"") + zToWx(filename1) + wxT("\""));

        DWORD length2 = 0;
        if (!::ReadFile(
                    hFile2,        //__in         HANDLE hFile,
                    memory2.get(), //__out        LPVOID lpBuffer,
                    BUFFER_SIZE,   //__in         DWORD nNumberOfBytesToRead,
                    &length2,      //__out_opt    LPDWORD lpNumberOfBytesRead,
                    NULL))         //__inout_opt  LPOVERLAPPED lpOverlapped
            throw FileError(wxString(_("Error reading file:")) + wxT(" \"") + zToWx(filename2) + wxT("\""));

        if (length1 != length2 || ::memcmp(memory1.get(), memory2.get(), length1) != 0)
            return false;

        bytesCompared += length1 * 2;

        //send progress updates
        callback->updateCompareStatus(bytesCompared);
    }
    while (length1 == BUFFER_SIZE);

    return true;


#elif defined FFS_LINUX
    wxFFile file1(::fopen(filename1.c_str(), DefaultStr("rb,type=record,noseek"))); //utilize UTF-8 filename
    if (!file1.IsOpened())
        throw FileError(wxString(_("Error opening file:")) + wxT(" \"") + zToWx(filename1) + wxT("\""));

    wxFFile file2(::fopen(filename2.c_str(), DefaultStr("rb,type=record,noseek"))); //utilize UTF-8 filename
    if (!file2.IsOpened())
        throw FileError(wxString(_("Error opening file:")) + wxT(" \"") + zToWx(filename2) + wxT("\""));

    wxLongLong bytesCompared;
    do
    {
        const size_t length1 = file1.Read(memory1.get(), BUFFER_SIZE);
        if (file1.Error())
            throw FileError(wxString(_("Error reading file:")) + wxT(" \"") + zToWx(filename1) + wxT("\""));

        const size_t length2 = file2.Read(memory2.get(), BUFFER_SIZE);
        if (file2.Error())
            throw FileError(wxString(_("Error reading file:")) + wxT(" \"") + zToWx(filename2) + wxT("\""));

        if (length1 != length2 || ::memcmp(memory1.get(), memory2.get(), length1) != 0)
            return false;

        bytesCompared += length1 * 2;

        //send progress updates
        callback->updateCompareStatus(bytesCompared);
    }
    while (!file1.Eof());

    if (!file2.Eof()) //highly unlikely, but theoretically possible! (but then again, not in this context where both files have same size...)
        return false;

    return true;
#endif
}
