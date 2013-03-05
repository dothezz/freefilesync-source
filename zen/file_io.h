// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) Zenju (zenju AT gmx DOT de) - All Rights Reserved        *
// **************************************************************************

#ifndef FILEIO_H_INCLUDED
#define FILEIO_H_INCLUDED

#include "file_io_base.h"
#include "file_error.h"

#ifdef FFS_WIN
#include "win.h" //includes "windows.h"
#elif defined FFS_LINUX || defined FFS_MAC
#include <cstdio>
#include <sys/stat.h>
#endif


namespace zen
{
#ifdef FFS_WIN
static const char LINE_BREAK[] = "\r\n";
#elif defined FFS_LINUX
static const char LINE_BREAK[] = "\n";
#elif defined FFS_MAC
static const char LINE_BREAK[] = "\r";
#endif

//buffered file IO optimized for sequential read/write accesses + better error reporting + long path support (following symlinks)

#ifdef FFS_WIN
typedef HANDLE FileHandle;
#elif defined FFS_LINUX || defined FFS_MAC
typedef FILE* FileHandle;
#endif

class FileInput : public FileInputBase
{
public:
    FileInput(const Zstring& filename);                    //throw FileError, ErrorNotExisting
    FileInput(FileHandle handle, const Zstring& filename); //takes ownership!
    ~FileInput();

    virtual size_t read(void* buffer, size_t bytesToRead); //throw FileError; returns actual number of bytes read
    //expected to fill buffer completely unless "end of file"

private:
    FileHandle fileHandle;
};


class FileOutput : public FileOutputBase
{
public:
    FileOutput(const Zstring& filename, AccessFlag access); //throw FileError, ErrorTargetPathMissing, ErrorTargetExisting
    FileOutput(FileHandle handle, const Zstring& filename); //takes ownership!
    ~FileOutput();

    virtual void write(const void* buffer, size_t bytesToWrite); //throw FileError

private:
    FileHandle fileHandle;
};

#if defined FFS_LINUX || defined FFS_MAC
class FileInputUnbuffered : public FileInputBase
{
public:
    FileInputUnbuffered(const Zstring& filename); //throw FileError, ErrorNotExisting
    ~FileInputUnbuffered();

    //considering safe-read.c it seems buffer size should be a multiple of 8192
    virtual size_t read(void* buffer, size_t bytesToRead); //throw FileError; returns actual number of bytes read
    //we should not rely on buffer being filled completely!

    int getDescriptor() { return fdFile;}

private:
    int fdFile;
};

class FileOutputUnbuffered : public FileOutputBase
{
public:
    //creates a new file (no overwrite allowed!)
    FileOutputUnbuffered(const Zstring& filename, mode_t mode); //throw FileError, ErrorTargetPathMissing, ErrorTargetExisting
    ~FileOutputUnbuffered();

    virtual void write(const void* buffer, size_t bytesToWrite); //throw FileError
    int getDescriptor() { return fdFile;}

private:
    int fdFile;
};
#endif
}

#endif // FILEIO_H_INCLUDED
