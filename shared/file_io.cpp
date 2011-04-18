// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) 2008-2011 ZenJu (zhnmju123 AT gmx.de)                    *
// **************************************************************************
//
#include "file_io.h"
#include "string_conv.h"
#include "system_func.h"
#include "i18n.h"

#ifdef FFS_WIN
#include "long_path_prefix.h"
#elif defined FFS_LINUX
#include <cerrno>
#endif

using namespace ffs3;


FileInput::FileInput(FileHandle handle, const Zstring& filename) :
#ifdef FFS_WIN
    eofReached(false),
#endif
    fileHandle(handle),
    filename_(filename) {}


FileInput::FileInput(const Zstring& filename)  : //throw (FileError, ErrorNotExisting)
#ifdef FFS_WIN
    eofReached(false),
#endif
    filename_(filename)
{
#ifdef FFS_WIN
    fileHandle = ::CreateFile(ffs3::applyLongPathPrefix(filename).c_str(),
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
    {
        const DWORD lastError = ::GetLastError();
        const wxString& errorMessage = wxString(_("Error opening file:")) + wxT("\n\"") + zToWx(filename_) + wxT("\"") + wxT("\n\n") + ffs3::getLastErrorFormatted(lastError);
        if (lastError == ERROR_FILE_NOT_FOUND ||
            lastError == ERROR_PATH_NOT_FOUND)
            throw ErrorNotExisting(errorMessage);

        throw FileError(errorMessage);
    }
#elif defined FFS_LINUX
    fileHandle = ::fopen(filename.c_str(), "r,type=record,noseek"); //utilize UTF-8 filename
    if (fileHandle == NULL)
    {
        const int lastError = errno;
        const wxString& errorMessage = wxString(_("Error opening file:")) + wxT("\n\"") + zToWx(filename_) + wxT("\"") + wxT("\n\n") + ffs3::getLastErrorFormatted(lastError);
        if (lastError == ENOENT)
            throw ErrorNotExisting(errorMessage);

        throw FileError(errorMessage);
    }
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


size_t FileInput::read(void* buffer, size_t bytesToRead) //returns actual number of bytes read; throw (FileError)
{
#ifdef FFS_WIN
    DWORD bytesRead = 0;

    if (!::ReadFile(fileHandle,    //__in         HANDLE hFile,
                    buffer,        //__out        LPVOID lpBuffer,
                    static_cast<DWORD>(bytesToRead), //__in         DWORD nNumberOfBytesToRead,
                    &bytesRead,    //__out_opt    LPDWORD lpNumberOfBytesRead,
                    NULL))          //__inout_opt  LPOVERLAPPED lpOverlapped
        throw FileError(wxString(_("Error reading file:")) + wxT("\n\"") + zToWx(filename_) + wxT("\"") +
                        wxT("\n\n") + ffs3::getLastErrorFormatted());

    if (bytesRead > bytesToRead)
        throw FileError(wxString(_("Error reading file:")) + wxT("\n\"") + zToWx(filename_) + wxT("\"") +
                        wxT("\n\n") + wxT("buffer overflow"));

    if (bytesRead < bytesToRead)
        eofReached = bytesRead < bytesToRead;

    return bytesRead;

#elif defined FFS_LINUX
    const size_t bytesRead = ::fread(buffer, 1, bytesToRead, fileHandle);
    if (::ferror(fileHandle) != 0)
        throw FileError(wxString(_("Error reading file:")) + wxT("\n\"") + zToWx(filename_) + wxT("\"") +
                        wxT("\n\n") + ffs3::getLastErrorFormatted());
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


FileOutput::FileOutput(FileHandle handle, const Zstring& filename) : fileHandle(handle), filename_(filename) {}


FileOutput::FileOutput(const Zstring& filename, AccessFlag access) : //throw (FileError, ErrorTargetPathMissing, ErrorTargetExisting)
    filename_(filename)
{
#ifdef FFS_WIN
    fileHandle = ::CreateFile(ffs3::applyLongPathPrefix(filename).c_str(),
                              GENERIC_WRITE,
                              FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, //note: FILE_SHARE_DELETE is required to rename file while handle is open!
                              NULL,
                              access == ACC_OVERWRITE ? CREATE_ALWAYS : CREATE_NEW,
                              FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN,
                              NULL);
    if (fileHandle == INVALID_HANDLE_VALUE)
    {
        const DWORD lastError = ::GetLastError();
        const wxString& errorMessage = wxString(_("Error writing file:")) + wxT("\n\"") + zToWx(filename_) + wxT("\"") +
                                       wxT("\n\n") + ffs3::getLastErrorFormatted(lastError);

        if (lastError == ERROR_FILE_EXISTS)
            throw ErrorTargetExisting(errorMessage);

        if (lastError == ERROR_PATH_NOT_FOUND)
            throw ErrorTargetPathMissing(errorMessage);

        throw FileError(errorMessage);
    }

#elif defined FFS_LINUX
    fileHandle = ::fopen(filename.c_str(),
                         //GNU extension: https://www.securecoding.cert.org/confluence/display/cplusplus/FIO03-CPP.+Do+not+make+assumptions+about+fopen()+and+file+creation
                         access == ACC_OVERWRITE ? "w,type=record,noseek" : "wx,type=record,noseek");
    if (fileHandle == NULL)
    {
        const int lastError = errno;
        const wxString& errorMessage = wxString(_("Error writing file:")) + wxT("\n\"") + zToWx(filename_) + wxT("\"") +
                                       wxT("\n\n") + ffs3::getLastErrorFormatted(lastError);
        if (lastError == EEXIST)
            throw ErrorTargetExisting(errorMessage);

        if (lastError == ENOENT)
            throw ErrorTargetPathMissing(errorMessage);

        throw FileError(errorMessage);
    }
#endif
}


FileOutput::~FileOutput()
{
#ifdef FFS_WIN
    ::CloseHandle(fileHandle);
#elif defined FFS_LINUX
    ::fclose(fileHandle); //NEVER allow passing NULL to fclose! -> crash!
#endif
}


void FileOutput::write(const void* buffer, size_t bytesToWrite) //throw (FileError)
{
#ifdef FFS_WIN
    DWORD bytesWritten = 0;

    if (!::WriteFile(fileHandle,    //__in         HANDLE hFile,
                     buffer,        //__out        LPVOID lpBuffer,
                     static_cast<DWORD>(bytesToWrite),  //__in         DWORD nNumberOfBytesToWrite,
                     &bytesWritten, //__out_opt    LPDWORD lpNumberOfBytesWritten,
                     NULL))          //__inout_opt  LPOVERLAPPED lpOverlapped
        throw FileError(wxString(_("Error writing file:")) + wxT("\n\"") + zToWx(filename_) + wxT("\"") +
                        wxT("\n\n") + ffs3::getLastErrorFormatted() + wxT(" (w)")); //w -> distinguish from fopen error message!

    if (bytesWritten != bytesToWrite) //must be fulfilled for synchronous writes!
        throw FileError(wxString(_("Error writing file:")) + wxT("\n\"") + zToWx(filename_) + wxT("\"") +
                        wxT("\n\n") + wxT("incomplete write"));

#elif defined FFS_LINUX
    const size_t bytesWritten = ::fwrite(buffer, 1, bytesToWrite, fileHandle);
    if (::ferror(fileHandle) != 0 || bytesWritten != bytesToWrite)
        throw FileError(wxString(_("Error writing file:")) + wxT("\n\"") + zToWx(filename_) + wxT("\"") +
                        wxT("\n\n") + ffs3::getLastErrorFormatted() + wxT(" (w)")); //w -> distinguish from fopen error message!
#endif
}
