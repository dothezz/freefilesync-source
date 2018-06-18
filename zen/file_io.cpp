// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) Zenju (zenju AT gmx DOT de) - All Rights Reserved        *
// **************************************************************************

#include "file_io.h"

#ifdef ZEN_WIN
#include "long_path_prefix.h"
#include "IFileOperation/file_op.h"
#include "win_ver.h"
#include "dll.h"

#elif defined ZEN_LINUX || defined ZEN_MAC
#include <fcntl.h>  //open, close
#include <unistd.h> //read, write
#endif

using namespace zen;


namespace
{
#ifdef ZEN_WIN
//(try to) enhance error messages by showing which processes lock the file
Zstring getLockingProcessNames(const Zstring& filename) //throw(), empty string if none found or error occurred
{
    if (vistaOrLater())
    {
        using namespace fileop;
        const DllFun<FunType_getLockingProcesses> getLockingProcesses(getDllName(), funName_getLockingProcesses);
        const DllFun<FunType_freeString>          freeString         (getDllName(), funName_freeString);

        if (getLockingProcesses && freeString)
        {
            const wchar_t* procList = nullptr;
            if (getLockingProcesses(filename.c_str(), procList))
            {
                ZEN_ON_SCOPE_EXIT(freeString(procList));
                return procList;
            }
        }
    }
    return Zstring();
}

#elif defined ZEN_LINUX || defined ZEN_MAC
//"filename" could be a named pipe which *blocks* forever during "open()"! https://sourceforge.net/p/freefilesync/bugs/221/
void checkForUnsupportedType(const Zstring& filename) //throw FileError
{
    struct ::stat fileInfo = {};
    if (::stat(filename.c_str(), &fileInfo) != 0) //follows symlinks
        return; //let the caller handle errors like "not existing"

    if (!S_ISREG(fileInfo.st_mode) &&
        !S_ISLNK(fileInfo.st_mode) &&
        !S_ISDIR(fileInfo.st_mode))
    {
        auto getTypeName = [](mode_t m) -> std::wstring
        {
            const wchar_t* name =
            S_ISCHR (m) ? L"character device":
            S_ISBLK (m) ? L"block device" :
            S_ISFIFO(m) ? L"FIFO, named pipe" :
            S_ISSOCK(m) ? L"socket" : nullptr;
            const std::wstring numFmt = printNumber<std::wstring>(L"0%06o", m & S_IFMT);
            return name ? numFmt + L", " + name : numFmt;
        };
        throw FileError(replaceCpy(_("Type of item %x is not supported:"), L"%x", fmtFileName(filename)) + L" " + getTypeName(fileInfo.st_mode));
    }
}
#endif
}


FileInput::FileInput(FileHandle handle, const Zstring& filename) : FileInputBase(filename), fileHandle(handle) {}


FileInput::FileInput(const Zstring& filename) : FileInputBase(filename) //throw FileError
{
#ifdef ZEN_WIN
    const wchar_t functionName[] = L"CreateFile";
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

                              Problem: bad XP implementation of prefetch makes flag FILE_FLAG_SEQUENTIAL_SCAN effectively load two files at the same time
                              from one drive, swapping every 64 kB (or similar). File access times explode!
                              => For XP it is critical to use FILE_FLAG_RANDOM_ACCESS (to disable prefetch) if reading two files on same disk and
                              FILE_FLAG_SEQUENTIAL_SCAN when reading from different disk (e.g. massive performance improvement compared to random access for DVD <-> HDD!)
                              => there is no compromise that satisfies all cases! (on XP)

                              for FFS most comparisons are probably between different disks => let's use FILE_FLAG_SEQUENTIAL_SCAN
                               */
                              nullptr);
    if (fileHandle == INVALID_HANDLE_VALUE)
#elif defined ZEN_LINUX || defined ZEN_MAC
    checkForUnsupportedType(filename); //throw FileError; reading a named pipe would block forever!
    const wchar_t functionName[] = L"fopen";
    fileHandle = ::fopen(filename.c_str(), "r,type=record,noseek"); //utilize UTF-8 filename
    if (!fileHandle)
#endif
    {
        const ErrorCode lastError = getLastError(); //copy before making other system calls!
        const std::wstring errorMsg = replaceCpy(_("Cannot open file %x."), L"%x", fmtFileName(filename));
        std::wstring errorDescr = formatSystemError(functionName, lastError);

#ifdef ZEN_WIN
        if (lastError == ERROR_SHARING_VIOLATION || //-> enhance error message!
            lastError == ERROR_LOCK_VIOLATION)
        {
            const Zstring procList = getLockingProcessNames(filename); //throw()
            if (!procList.empty())
                errorDescr = _("The file is locked by another process:") + L"\n" + procList;
        }
#endif
        throw FileError(errorMsg, errorDescr);
    }
}


FileInput::~FileInput()
{
#ifdef ZEN_WIN
    ::CloseHandle(fileHandle);
#elif defined ZEN_LINUX || defined ZEN_MAC
    ::fclose(fileHandle); //NEVER allow passing nullptr to fclose! -> crash!; fileHandle != nullptr in this context!
#endif
}


size_t FileInput::read(void* buffer, size_t bytesToRead) //returns actual number of bytes read; throw FileError
{
    assert(!eof());
    if (bytesToRead == 0) return 0;
#ifdef ZEN_WIN
    const wchar_t functionName[] = L"ReadFile";
    DWORD bytesRead = 0;
    if (!::ReadFile(fileHandle, //__in         HANDLE hFile,
                    buffer,     //__out        LPVOID lpBuffer,
                    static_cast<DWORD>(bytesToRead), //__in         DWORD nNumberOfBytesToRead,
                    &bytesRead, //__out_opt    LPDWORD lpNumberOfBytesRead,
                    nullptr))   //__inout_opt  LPOVERLAPPED lpOverlapped
#elif defined ZEN_LINUX || defined ZEN_MAC
    const wchar_t functionName[] = L"fread";
    const size_t bytesRead = ::fread(buffer, 1, bytesToRead, fileHandle);
    if (::ferror(fileHandle) != 0) //checks status of stream, not fread()!
#endif
        throw FileError(replaceCpy(_("Cannot read file %x."), L"%x", fmtFileName(getFilename())), formatSystemError(functionName, getLastError()));

#ifdef ZEN_WIN
    if (bytesRead < bytesToRead) //verify only!
        setEof();

#elif defined ZEN_LINUX || defined ZEN_MAC
    if (::feof(fileHandle) != 0)
        setEof();

    if (bytesRead < bytesToRead)
        if (!eof()) //pathologic!?
            throw FileError(replaceCpy(_("Cannot read file %x."), L"%x", fmtFileName(getFilename())), L"Incomplete read."); //user should never see this
#endif

    if (bytesRead > bytesToRead)
        throw FileError(replaceCpy(_("Cannot read file %x."), L"%x", fmtFileName(getFilename())), L"buffer overflow"); //user should never see this

    return bytesRead;
}


FileOutput::FileOutput(FileHandle handle, const Zstring& filename) : FileOutputBase(filename), fileHandle(handle) {}


FileOutput::FileOutput(const Zstring& filename, AccessFlag access) : //throw FileError, ErrorTargetPathMissing, ErrorTargetExisting
    FileOutputBase(filename)
{
#ifdef ZEN_WIN
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
        DWORD lastError = ::GetLastError(); //copy before making other system calls!

        //CREATE_ALWAYS fails with ERROR_ACCESS_DENIED if the existing file is hidden or "system" http://msdn.microsoft.com/en-us/library/windows/desktop/aa363858(v=vs.85).aspx
        if (lastError == ERROR_ACCESS_DENIED &&
            dwCreationDisposition == CREATE_ALWAYS)
        {
            const DWORD attrib = ::GetFileAttributes(applyLongPathPrefix(filename).c_str());
            if (attrib != INVALID_FILE_ATTRIBUTES)
            {
                fileHandle = getHandle(attrib); //retry: alas this may still fail for hidden file, e.g. accessing shared folder in XP as Virtual Box guest!
                lastError = ::GetLastError();
            }
        }

        //begin of "regular" error reporting
        if (fileHandle == INVALID_HANDLE_VALUE)
        {
            const std::wstring errorMsg = replaceCpy(_("Cannot write file %x."), L"%x", fmtFileName(filename));
            std::wstring errorDescr = formatSystemError(L"CreateFile", lastError);

            if (lastError == ERROR_SHARING_VIOLATION || //-> enhance error message!
                lastError == ERROR_LOCK_VIOLATION)
            {
                const Zstring procList = getLockingProcessNames(filename); //throw()
                if (!procList.empty())
                    errorDescr = _("The file is locked by another process:") + L"\n" + procList;
            }

            if (lastError == ERROR_FILE_EXISTS || //confirmed to be used
                lastError == ERROR_ALREADY_EXISTS) //comment on msdn claims, this one is used on Windows Mobile 6
                throw ErrorTargetExisting(errorMsg, errorDescr);

            if (lastError == ERROR_PATH_NOT_FOUND)
                throw ErrorTargetPathMissing(errorMsg, errorDescr);

            throw FileError(errorMsg, errorDescr);
        }
    }

#elif defined ZEN_LINUX || defined ZEN_MAC
    checkForUnsupportedType(filename); //throw FileError; writing a named pipe would block forever!
    fileHandle = ::fopen(filename.c_str(),
                         //GNU extension: https://www.securecoding.cert.org/confluence/display/cplusplus/FIO03-CPP.+Do+not+make+assumptions+about+fopen()+and+file+creation
                         access == ACC_OVERWRITE ? "w,type=record,noseek" : "wx,type=record,noseek");
    if (!fileHandle)
    {
        const ErrorCode lastError = getLastError(); //copy before making other system calls!
        const std::wstring errorMsg = replaceCpy(_("Cannot write file %x."), L"%x", fmtFileName(getFilename()));
        const std::wstring errorDescr = formatSystemError(L"fopen", lastError);

        if (lastError == EEXIST)
            throw ErrorTargetExisting(errorMsg, errorDescr);

        if (lastError == ENOENT)
            throw ErrorTargetPathMissing(errorMsg, errorDescr);

        throw FileError(errorMsg, errorDescr);
    }
#endif
}


FileOutput::~FileOutput()
{
#ifdef ZEN_WIN
    ::CloseHandle(fileHandle);
#elif defined ZEN_LINUX || defined ZEN_MAC
    ::fclose(fileHandle); //NEVER allow passing nullptr to fclose! -> crash!
#endif
}


void FileOutput::write(const void* buffer, size_t bytesToWrite) //throw FileError
{
#ifdef ZEN_WIN
    const wchar_t functionName[] = L"WriteFile";
    DWORD bytesWritten = 0; //this parameter is NOT optional: http://blogs.msdn.com/b/oldnewthing/archive/2013/04/04/10407417.aspx
    if (!::WriteFile(fileHandle,    //__in         HANDLE hFile,
                     buffer,        //__out        LPVOID lpBuffer,
                     static_cast<DWORD>(bytesToWrite),  //__in         DWORD nNumberOfBytesToWrite,
                     &bytesWritten, //__out_opt    LPDWORD lpNumberOfBytesWritten,
                     nullptr))      //__inout_opt  LPOVERLAPPED lpOverlapped
#elif defined ZEN_LINUX || defined ZEN_MAC
    const wchar_t functionName[] = L"fwrite";
    const size_t bytesWritten = ::fwrite(buffer, 1, bytesToWrite, fileHandle);
    if (::ferror(fileHandle) != 0) //checks status of stream, not fwrite()!
#endif
        throw FileError(replaceCpy(_("Cannot write file %x."), L"%x", fmtFileName(getFilename())), formatSystemError(functionName, getLastError()));

    if (bytesWritten != bytesToWrite) //must be fulfilled for synchronous writes!
        throw FileError(replaceCpy(_("Cannot write file %x."), L"%x", fmtFileName(getFilename())), L"Incomplete write."); //user should never see this
}


#if defined ZEN_LINUX || defined ZEN_MAC
//Compare copy_reg() in copy.c: ftp://ftp.gnu.org/gnu/coreutils/coreutils-5.0.tar.gz

FileInputUnbuffered::FileInputUnbuffered(const Zstring& filename) : FileInputBase(filename) //throw FileError
{
    checkForUnsupportedType(filename); //throw FileError; reading a named pipe would block forever!

    fdFile = ::open(filename.c_str(), O_RDONLY);
    if (fdFile == -1) //don't check "< 0" -> docu seems to allow "-2" to be a valid file handle
        throw FileError(replaceCpy(_("Cannot open file %x."), L"%x", fmtFileName(filename)), formatSystemError(L"open", getLastError()));
}


FileInputUnbuffered::~FileInputUnbuffered() { ::close(fdFile); }


size_t FileInputUnbuffered::read(void* buffer, size_t bytesToRead) //throw FileError; returns actual number of bytes read
{
    assert(!eof());
    if (bytesToRead == 0) return 0; //[!]

    ssize_t bytesRead = 0;
    do
    {
        bytesRead = ::read(fdFile, buffer, bytesToRead);
    }
    while (bytesRead < 0 && errno == EINTR);

    if (bytesRead < 0)
        throw FileError(replaceCpy(_("Cannot read file %x."), L"%x", fmtFileName(getFilename())), formatSystemError(L"read", getLastError()));
    else if (bytesRead == 0) //"zero indicates end of file"
        setEof();
    else if (bytesRead > static_cast<ssize_t>(bytesToRead)) //better safe than sorry
        throw FileError(replaceCpy(_("Cannot read file %x."), L"%x", fmtFileName(getFilename())), L"buffer overflow"); //user should never see this
    //if ::read is interrupted (EINTR) right in the middle, it will return successfully with "bytesRead < bytesToRead"!

    return bytesRead;
}


FileOutputUnbuffered::FileOutputUnbuffered(const Zstring& filename, mode_t mode) : FileOutputBase(filename) //throw FileError, ErrorTargetPathMissing, ErrorTargetExisting
{
    //checkForUnsupportedType(filename); -> not needed, open() + O_EXCL shoul fail fast

    //overwrite is: O_CREAT | O_WRONLY | O_TRUNC
    fdFile = ::open(filename.c_str(), O_CREAT | O_WRONLY | O_EXCL, mode & (S_IRWXU | S_IRWXG | S_IRWXO));
    if (fdFile == -1)
    {
        const int lastError = errno; //copy before making other system calls!
        const std::wstring errorMsg = replaceCpy(_("Cannot write file %x."), L"%x", fmtFileName(filename));
        const std::wstring errorDescr = formatSystemError(L"open", lastError);

        if (lastError == EEXIST)
            throw ErrorTargetExisting(errorMsg, errorDescr);

        if (lastError == ENOENT)
            throw ErrorTargetPathMissing(errorMsg, errorDescr);

        throw FileError(errorMsg, errorDescr);
    }
}

FileOutputUnbuffered::FileOutputUnbuffered(int fd, const Zstring& filename) : FileOutputBase(filename), fdFile(fd) {}

FileOutputUnbuffered::~FileOutputUnbuffered() { ::close(fdFile); }


void FileOutputUnbuffered::write(const void* buffer, size_t bytesToWrite) //throw FileError
{
    while (bytesToWrite > 0)
    {
        ssize_t bytesWritten = 0;
        do
        {
            bytesWritten = ::write(fdFile, buffer, bytesToWrite);
        }
        while (bytesWritten < 0 && errno == EINTR);

        if (bytesWritten <= 0)
        {
            if (bytesWritten == 0) //comment in safe-read.c suggests to treat this as an error due to buggy drivers
                errno = ENOSPC;

            throw FileError(replaceCpy(_("Cannot write file %x."), L"%x", fmtFileName(getFilename())), formatSystemError(L"write", getLastError()));
        }
        if (bytesWritten > static_cast<ssize_t>(bytesToWrite)) //better safe than sorry
            throw FileError(replaceCpy(_("Cannot write file %x."), L"%x", fmtFileName(getFilename())), L"buffer overflow"); //user should never see this

        //if ::write is interrupted (EINTR) right in the middle, it will return successfully with "bytesWritten < bytesToWrite"!
        buffer = static_cast<const char*>(buffer) + bytesWritten; //suppress warning about pointer arithmetics on void*
        bytesToWrite -= bytesWritten;
    }
}
#endif
