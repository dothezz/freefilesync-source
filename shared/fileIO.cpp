// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) 2008-2010 ZenJu (zhnmju123 AT gmx.de)                    *
// **************************************************************************
//
#include "fileIO.h"
#include <wx/intl.h>
#include "stringConv.h"
#include "systemFunctions.h"

#ifdef FFS_WIN
#include "longPathPrefix.h"
#endif

using namespace FreeFileSync;


FileInput::FileInput(const Zstring& filename)  : //throw FileError()
#ifdef FFS_WIN
    eofReached(false),
#endif
    filename_(filename)
{
#ifdef FFS_WIN
    fileHandle = ::CreateFile(FreeFileSync::applyLongPathPrefix(filename).c_str(),
                              GENERIC_READ,
                              FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, //all shared modes are required to read files that are open in other applications
                              NULL,
                              OPEN_EXISTING,
                              FILE_FLAG_SEQUENTIAL_SCAN,
                              /* possible values: (Reference http://msdn.microsoft.com/en-us/library/aa363858(VS.85).aspx#caching_behavior)
                                FILE_FLAG_NO_BUFFERING
                                FILE_FLAG_RANDOM_ACCESS
                                FILE_FLAG_SEQUENTIAL_SCAN

                                tests on Win7 x64 show that FILE_FLAG_SEQUENTIAL_SCAN provides best performance for binary comparison in all cases:
                                - comparing different physical disks (DVD <-> HDD and HDD <-> HDD)
                                - even on same physical disk! (HDD <-> HDD)
                                - independent from client buffer size!

                                tests on XP show that FILE_FLAG_SEQUENTIAL_SCAN provides best performance for binary comparison when
                                - comparing different physical disks (DVD <-> HDD)

                                while FILE_FLAG_RANDOM_ACCESS offers best performance for
                                - same physical disk (HDD <-> HDD)

                              Problem: bad XP implementation of prefetch makes flag FILE_FLAG_SEQUENTIAL_SCAN effectively load two files at once from one drive
                              swapping every 64 kB (or similar). File access times explode!
                              => For XP it is critical to use FILE_FLAG_RANDOM_ACCESS (to disable prefetch) if reading two files on same disk and
                              FILE_FLAG_SEQUENTIAL_SCAN when reading from different disk (e.g. massive performance improvement compared to random access for DVD <-> HDD!)
                              => there is no compromise that satisfies all cases! (on XP)

                              for FFS most comparisons are probably between different disks => let's use FILE_FLAG_SEQUENTIAL_SCAN
                               */
                              NULL);
    if (fileHandle == INVALID_HANDLE_VALUE)
        throw FileError(wxString(_("Error opening file:")) + wxT("\n\"") + zToWx(filename_) + wxT("\"") +
                        wxT("\n\n") + FreeFileSync::getLastErrorFormatted());
#elif defined FFS_LINUX
    fileHandle = ::fopen(filename.c_str(), "rb,type=record,noseek"); //utilize UTF-8 filename
    if (!fileHandle)
        throw FileError(wxString(_("Error opening file:")) + wxT("\n\"") + zToWx(filename_) + wxT("\"") +
                        wxT("\n\n") + FreeFileSync::getLastErrorFormatted());
#endif
}


FileInput::~FileInput()
{
#ifdef FFS_WIN
    ::CloseHandle(fileHandle);
#elif defined FFS_LINUX
    ::fclose(fileHandle); //NEVER allow passing NULL to fclose! -> crash!; fileHandle != NULL in this context!
#endif
}


size_t FileInput::read(void* buffer, size_t bytesToRead) //returns actual number of bytes read; throw FileError()
{
#ifdef FFS_WIN
    DWORD bytesRead = 0;

    if (!::ReadFile(
                fileHandle,    //__in         HANDLE hFile,
                buffer,        //__out        LPVOID lpBuffer,
                static_cast<DWORD>(bytesToRead),   //__in         DWORD nNumberOfBytesToRead,
                &bytesRead,    //__out_opt    LPDWORD lpNumberOfBytesRead,
                NULL)          //__inout_opt  LPOVERLAPPED lpOverlapped
            || bytesRead > bytesToRead) //must be fulfilled
        throw FileError(wxString(_("Error reading file:")) + wxT("\n\"") + zToWx(filename_) + wxT("\"") +
                        wxT("\n\n") + FreeFileSync::getLastErrorFormatted());

    if (bytesRead < bytesToRead)
        eofReached = true;

    return bytesRead;
#elif defined FFS_LINUX
    const size_t bytesRead = ::fread(buffer, 1, bytesToRead, fileHandle);
    if (::ferror(fileHandle) != 0)
        throw FileError(wxString(_("Error reading file:")) + wxT("\n\"") + zToWx(filename_) + wxT("\"") +
                        wxT("\n\n") + FreeFileSync::getLastErrorFormatted());
    return bytesRead;
#endif
}


bool FileInput::eof() //end of file reached
{
#ifdef FFS_WIN
    return eofReached;
#elif defined FFS_LINUX
    return ::feof(fileHandle) != 0;
#endif
}


FileOutput::FileOutput(const Zstring& filename) : //throw FileError()
    filename_(filename)
{
#ifdef FFS_WIN
    fileHandle = ::CreateFile(FreeFileSync::applyLongPathPrefix(filename).c_str(),
                              GENERIC_WRITE,
                              FILE_SHARE_READ,
                              NULL,
                              CREATE_ALWAYS,
                              FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN,
                              NULL);
    if (fileHandle == INVALID_HANDLE_VALUE)
        throw FileError(wxString(_("Error writing file:")) + wxT("\n\"") + zToWx(filename_) + wxT("\"") +
                        wxT("\n\n") + FreeFileSync::getLastErrorFormatted());
#elif defined FFS_LINUX
    fileHandle = ::fopen(filename.c_str(), "wb,type=record,noseek"); //utilize UTF-8 filename
    if (!fileHandle)
        throw FileError(wxString(_("Error writing file:")) + wxT("\n\"") + zToWx(filename_) + wxT("\"") +
                        wxT("\n\n") + FreeFileSync::getLastErrorFormatted());
#endif
}


FileOutput::~FileOutput()
{
    close(); //may be called more than once
}


void FileOutput::write(const void* buffer, size_t bytesToWrite) //throw FileError()
{
#ifdef FFS_WIN
    DWORD bytesWritten = 0;

    if (!::WriteFile(
                fileHandle,    //__in         HANDLE hFile,
                buffer,        //__out        LPVOID lpBuffer,
                static_cast<DWORD>(bytesToWrite),  //__in         DWORD nNumberOfBytesToRead,
                &bytesWritten, //__out_opt    LPDWORD lpNumberOfBytesWritten,
                NULL)          //__inout_opt  LPOVERLAPPED lpOverlapped
            || bytesWritten != bytesToWrite) //must be fulfilled for synchronous writes!
        throw FileError(wxString(_("Error writing file:")) + wxT("\n\"") + zToWx(filename_) + wxT("\"") +
                        wxT("\n\n") + FreeFileSync::getLastErrorFormatted());
#elif defined FFS_LINUX
    const size_t bytesWritten = ::fwrite(buffer, 1, bytesToWrite, fileHandle);
    if (::ferror(fileHandle) != 0 || bytesWritten != bytesToWrite)
        throw FileError(wxString(_("Error writing file:")) + wxT("\n\"") + zToWx(filename_) + wxT("\"") +
                        wxT("\n\n") + FreeFileSync::getLastErrorFormatted() + wxT(" (w)")); //w -> distinguish from fopen error message!
#endif
}


void FileOutput::close() //close file stream
{
    if (fileHandle != NULL) //NEVER allow passing NULL to fclose! -> crash!
    {
#ifdef FFS_WIN
        ::CloseHandle(fileHandle);
#elif defined FFS_LINUX
        ::fclose(fileHandle);
#endif
        fileHandle = NULL;
    }
}
