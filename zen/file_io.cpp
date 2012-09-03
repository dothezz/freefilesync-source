// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) ZenJu (zenju AT gmx DOT de) - All Rights Reserved        *
// **************************************************************************

#include "file_io.h"

#ifdef FFS_WIN
#include "long_path_prefix.h"
#endif

using namespace zen;


FileInput::FileInput(FileHandle handle, const Zstring& filename) :
    eofReached(false),
    fileHandle(handle),
    filename_(filename) {}


FileInput::FileInput(const Zstring& filename)  : //throw FileError, ErrorNotExisting
    eofReached(false),
    filename_(filename)
{
#ifdef FFS_WIN
    fileHandle = ::CreateFile(applyLongPathPrefix(filename).c_str(),
                              GENERIC_READ,
                              FILE_SHARE_READ | FILE_SHARE_DELETE,
                              nullptr,
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
                              nullptr);
    if (fileHandle == INVALID_HANDLE_VALUE)
#elif defined FFS_LINUX
    fileHandle = ::fopen(filename.c_str(), "r,type=record,noseek"); //utilize UTF-8 filename
    if (!fileHandle)
#endif
    {
        const ErrorCode lastError = getLastError();

        if (errorCodeForNotExisting(lastError))
            throw ErrorNotExisting(replaceCpy(_("Cannot find file %x."), L"%x", fmtFileName(filename_)) + L"\n\n" + getLastErrorFormatted(lastError));

        throw FileError(replaceCpy(_("Cannot read file %x."), L"%x", fmtFileName(filename_)) + L"\n\n" + getLastErrorFormatted(lastError) + L" (open)");
    }
}


FileInput::~FileInput()
{
#ifdef FFS_WIN
    ::CloseHandle(fileHandle);
#elif defined FFS_LINUX
    ::fclose(fileHandle); //NEVER allow passing nullptr to fclose! -> crash!; fileHandle != nullptr in this context!
#endif
}


size_t FileInput::read(void* buffer, size_t bytesToRead) //returns actual number of bytes read; throw FileError
{
#ifdef FFS_WIN
    DWORD bytesRead = 0;
    if (!::ReadFile(fileHandle,    //__in         HANDLE hFile,
                    buffer,        //__out        LPVOID lpBuffer,
                    static_cast<DWORD>(bytesToRead), //__in         DWORD nNumberOfBytesToRead,
                    &bytesRead,    //__out_opt    LPDWORD lpNumberOfBytesRead,
                    nullptr))      //__inout_opt  LPOVERLAPPED lpOverlapped
#elif defined FFS_LINUX
    const size_t bytesRead = ::fread(buffer, 1, bytesToRead, fileHandle);
    if (::ferror(fileHandle) != 0) //checks status of stream, not fread()!
#endif
        throw FileError(replaceCpy(_("Cannot read file %x."), L"%x", fmtFileName(filename_)) + L"\n\n" + getLastErrorFormatted() + L" (read)");

#ifdef FFS_WIN
    if (bytesRead < bytesToRead) //verify only!
        eofReached = true;

#elif defined FFS_LINUX
    if (::feof(fileHandle) != 0)
        eofReached = true;

    if (bytesRead < bytesToRead)
        if (!eofReached) //pathologic!?
            throw FileError(replaceCpy(_("Cannot read file %x."), L"%x", fmtFileName(filename_)) + L"\n\n" + L"Incomplete read!");
#endif

    if (bytesRead > bytesToRead)
        throw FileError(replaceCpy(_("Cannot read file %x."), L"%x", fmtFileName(filename_)) + L"\n\n" + L"buffer overflow");

    return bytesRead;
}


FileOutput::FileOutput(FileHandle handle, const Zstring& filename) : fileHandle(handle), filename_(filename) {}


FileOutput::FileOutput(const Zstring& filename, AccessFlag access) : //throw FileError, ErrorTargetPathMissing, ErrorTargetExisting
    filename_(filename)
{
#ifdef FFS_WIN
    const DWORD dwCreationDisposition = access == FileOutput::ACC_OVERWRITE ? CREATE_ALWAYS : CREATE_NEW;

    auto getHandle = [&](DWORD dwFlagsAndAttributes)
    {
        return ::CreateFile(applyLongPathPrefix(filename).c_str(),
                            GENERIC_READ | GENERIC_WRITE,
                            /*  http://msdn.microsoft.com/en-us/library/aa363858(v=vs.85).aspx
                                   quote: When an application creates a file across a network, it is better
                                   to use GENERIC_READ | GENERIC_WRITE for dwDesiredAccess than to use GENERIC_WRITE alone.
                                   The resulting code is faster, because the redirector can use the cache manager and send fewer SMBs with more data.
                                   This combination also avoids an issue where writing to a file across a network can occasionally return ERROR_ACCESS_DENIED. */
                            FILE_SHARE_DELETE, //FILE_SHARE_DELETE is required to rename file while handle is open!
                            nullptr,
                            dwCreationDisposition,
                            dwFlagsAndAttributes | FILE_FLAG_SEQUENTIAL_SCAN,
                            nullptr);
    };

    fileHandle = getHandle(FILE_ATTRIBUTE_NORMAL);
    if (fileHandle == INVALID_HANDLE_VALUE)
    {
        DWORD lastError = ::GetLastError();

        //CREATE_ALWAYS fails with ERROR_ACCESS_DENIED if the existing file is hidden or "system" http://msdn.microsoft.com/en-us/library/windows/desktop/aa363858(v=vs.85).aspx
        if (lastError == ERROR_ACCESS_DENIED &&
            dwCreationDisposition == CREATE_ALWAYS)
        {
            const DWORD attrib = ::GetFileAttributes(applyLongPathPrefix(filename).c_str());
            if (attrib != INVALID_FILE_ATTRIBUTES)
            {
                fileHandle = getHandle(attrib); //retry
                lastError = ::GetLastError();
            }
        }
        //"regular" error handling
        if (fileHandle == INVALID_HANDLE_VALUE)
        {
            const std::wstring errorMessage = replaceCpy(_("Cannot write file %x."), L"%x", fmtFileName(filename_)) + L"\n\n" + zen::getLastErrorFormatted(lastError);

            if (lastError == ERROR_FILE_EXISTS || //confirmed to be used
                lastError == ERROR_ALREADY_EXISTS) //comment on msdn claims, this one is used on Windows Mobile 6
                throw ErrorTargetExisting(errorMessage);

            if (lastError == ERROR_PATH_NOT_FOUND)
                throw ErrorTargetPathMissing(errorMessage);

            throw FileError(errorMessage);
        }
    }

#elif defined FFS_LINUX
    fileHandle = ::fopen(filename.c_str(),
                         //GNU extension: https://www.securecoding.cert.org/confluence/display/cplusplus/FIO03-CPP.+Do+not+make+assumptions+about+fopen()+and+file+creation
                         access == ACC_OVERWRITE ? "w,type=record,noseek" : "wx,type=record,noseek");
    if (!fileHandle)
    {
        const int lastError = errno;
        const std::wstring errorMessage = replaceCpy(_("Cannot write file %x."), L"%x", fmtFileName(filename_)) + L"\n\n" + zen::getLastErrorFormatted(lastError);
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
    ::fclose(fileHandle); //NEVER allow passing nullptr to fclose! -> crash!
#endif
}


void FileOutput::write(const void* buffer, size_t bytesToWrite) //throw FileError
{
#ifdef FFS_WIN
    DWORD bytesWritten = 0;
    if (!::WriteFile(fileHandle,    //__in         HANDLE hFile,
                     buffer,        //__out        LPVOID lpBuffer,
                     static_cast<DWORD>(bytesToWrite),  //__in         DWORD nNumberOfBytesToWrite,
                     &bytesWritten, //__out_opt    LPDWORD lpNumberOfBytesWritten,
                     nullptr))      //__inout_opt  LPOVERLAPPED lpOverlapped
#elif defined FFS_LINUX
    const size_t bytesWritten = ::fwrite(buffer, 1, bytesToWrite, fileHandle);
    if (::ferror(fileHandle) != 0) //checks status of stream, not fwrite()!
#endif
        throw FileError(replaceCpy(_("Cannot write file %x."), L"%x", fmtFileName(filename_)) + L"\n\n" + getLastErrorFormatted() + L" (w)"); //w -> distinguish from fopen error message!

    if (bytesWritten != bytesToWrite) //must be fulfilled for synchronous writes!
        throw FileError(replaceCpy(_("Cannot write file %x."), L"%x", fmtFileName(filename_)) + L"\n\n" + L"Incomplete write!");
}
