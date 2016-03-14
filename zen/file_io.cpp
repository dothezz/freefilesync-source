// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl-3.0        *
// * Copyright (C) Zenju (zenju AT gmx DOT de) - All Rights Reserved        *
// **************************************************************************

#include "file_io.h"
#include "file_access.h"

    #include <sys/stat.h>
    #include <fcntl.h>  //open, close
    #include <unistd.h> //read, write

using namespace zen;


namespace
{
//- "filepath" could be a named pipe which *blocks* forever for open()!
//- open() with O_NONBLOCK avoids the block, but opens successfully
//- create sample pipe: "sudo mkfifo named_pipe"
void checkForUnsupportedType(const Zstring& filepath) //throw FileError
{
    struct ::stat fileInfo = {};
    if (::stat(filepath.c_str(), &fileInfo) != 0) //follows symlinks
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
        throw FileError(replaceCpy(_("Type of item %x is not supported:"), L"%x", fmtPath(filepath)) + L" " + getTypeName(fileInfo.st_mode));
    }
}

inline
FileHandle getInvalidHandle()
{
    return -1;
}
}


FileInput::FileInput(FileHandle handle, const Zstring& filepath) : FileBase(filepath), fileHandle(handle) {}


FileInput::FileInput(const Zstring& filepath) : //throw FileError, ErrorFileLocked
    FileBase(filepath), fileHandle(getInvalidHandle())
{
    checkForUnsupportedType(filepath); //throw FileError; opening a named pipe would block forever!

    //don't use O_DIRECT: http://yarchive.net/comp/linux/o_direct.html
    fileHandle = ::open(filepath.c_str(), O_RDONLY);
    if (fileHandle == -1) //don't check "< 0" -> docu seems to allow "-2" to be a valid file handle
        THROW_LAST_FILE_ERROR(replaceCpy(_("Cannot open file %x."), L"%x", fmtPath(filepath)), L"open");

    //------------------------------------------------------------------------------------------------------

    ZEN_ON_SCOPE_FAIL(::close(fileHandle));

    //optimize read-ahead on input file:
    if (::posix_fadvise(fileHandle, 0, 0, POSIX_FADV_SEQUENTIAL) != 0)
        THROW_LAST_FILE_ERROR(replaceCpy(_("Cannot read file %x."), L"%x", fmtPath(filepath)), L"posix_fadvise");

}


FileInput::~FileInput()
{
    if (fileHandle != getInvalidHandle())
        ::close(fileHandle);
}


size_t FileInput::tryRead(void* buffer, size_t bytesToRead) //throw FileError; may return short, only 0 means EOF!
{
    if (bytesToRead == 0) //"read() with a count of 0 returns zero" => indistinguishable from end of file! => check!
        throw std::logic_error("Contract violation! " + std::string(__FILE__) + ":" + numberTo<std::string>(__LINE__));

    ssize_t bytesRead = 0;
    do
    {
        bytesRead = ::read(fileHandle, buffer, bytesToRead);
    }
    while (bytesRead < 0 && errno == EINTR); //Compare copy_reg() in copy.c: ftp://ftp.gnu.org/gnu/coreutils/coreutils-8.23.tar.xz

    if (bytesRead < 0)
        THROW_LAST_FILE_ERROR(replaceCpy(_("Cannot read file %x."), L"%x", fmtPath(getFilePath())), L"read");
    if (static_cast<size_t>(bytesRead) > bytesToRead) //better safe than sorry
        throw FileError(replaceCpy(_("Cannot read file %x."), L"%x", fmtPath(getFilePath())), L"ReadFile: buffer overflow."); //user should never see this

    //if ::read is interrupted (EINTR) right in the middle, it will return successfully with "bytesRead < bytesToRead" => loop!

    return bytesRead; //"zero indicates end of file"
}

//----------------------------------------------------------------------------------------------------

FileOutput::FileOutput(FileHandle handle, const Zstring& filepath) : FileBase(filepath), fileHandle(handle) {}


FileOutput::FileOutput(const Zstring& filepath, AccessFlag access) : //throw FileError, ErrorTargetExisting
    FileBase(filepath), fileHandle(getInvalidHandle())
{
    //checkForUnsupportedType(filepath); -> not needed, open() + O_WRONLY should fail fast

    fileHandle = ::open(filepath.c_str(), O_WRONLY | O_CREAT | (access == FileOutput::ACC_CREATE_NEW ? O_EXCL : O_TRUNC),
                        S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH); //0666
    if (fileHandle == -1)
    {
        const int ec = errno; //copy before making other system calls!
        const std::wstring errorMsg = replaceCpy(_("Cannot write file %x."), L"%x", fmtPath(filepath));
        const std::wstring errorDescr = formatSystemError(L"open", ec);

        if (ec == EEXIST)
            throw ErrorTargetExisting(errorMsg, errorDescr);
        //if (ec == ENOENT) throw ErrorTargetPathMissing(errorMsg, errorDescr);

        throw FileError(errorMsg, errorDescr);
    }

    //------------------------------------------------------------------------------------------------------

    //ScopeGuard constructorGuard = zen::makeGuard

    //guard handle when adding code!!!

    //constructorGuard.dismiss();
}


FileOutput::FileOutput(FileOutput&& tmp) : FileBase(tmp.getFilePath()), fileHandle(tmp.fileHandle) { tmp.fileHandle = getInvalidHandle(); }


FileOutput::~FileOutput()
{
    if (fileHandle != getInvalidHandle())
        try
        {
            close(); //throw FileError
        }
        catch (FileError&) { assert(false); }
}


void FileOutput::close() //throw FileError
{
    if (fileHandle == getInvalidHandle())
        throw FileError(replaceCpy(_("Cannot write file %x."), L"%x", fmtPath(getFilePath())), L"Contract error: close() called more than once.");
    ZEN_ON_SCOPE_EXIT(fileHandle = getInvalidHandle());

    //no need to clean-up on failure here (just like there is no clean on FileOutput::write failure!) => FileOutput is not transactional!

    if (::close(fileHandle) != 0)
        THROW_LAST_FILE_ERROR(replaceCpy(_("Cannot write file %x."), L"%x", fmtPath(getFilePath())), L"close");
}


size_t FileOutput::tryWrite(const void* buffer, size_t bytesToWrite) //throw FileError; may return short! CONTRACT: bytesToWrite > 0
{
    if (bytesToWrite == 0)
        throw std::logic_error("Contract violation! " + std::string(__FILE__) + ":" + numberTo<std::string>(__LINE__));

    ssize_t bytesWritten = 0;
    do
    {
        bytesWritten = ::write(fileHandle, buffer, bytesToWrite);
    }
    while (bytesWritten < 0 && errno == EINTR);

    if (bytesWritten <= 0)
    {
        if (bytesWritten == 0) //comment in safe-read.c suggests to treat this as an error due to buggy drivers
            errno = ENOSPC;

        THROW_LAST_FILE_ERROR(replaceCpy(_("Cannot write file %x."), L"%x", fmtPath(getFilePath())), L"write");
    }
    if (bytesWritten > static_cast<ssize_t>(bytesToWrite)) //better safe than sorry
        throw FileError(replaceCpy(_("Cannot write file %x."), L"%x", fmtPath(getFilePath())), L"write: buffer overflow."); //user should never see this

    //if ::write() is interrupted (EINTR) right in the middle, it will return successfully with "bytesWritten < bytesToWrite"!
    return bytesWritten;
}
