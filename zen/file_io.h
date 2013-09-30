// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) Zenju (zenju AT gmx DOT de) - All Rights Reserved        *
// **************************************************************************

#ifndef FILEIO_89578342758342572345
#define FILEIO_89578342758342572345

#include "file_io_base.h"
#include "file_error.h"

#ifdef ZEN_WIN
#include "win.h" //includes "windows.h"
#elif defined ZEN_LINUX || defined ZEN_MAC
#include <cstdio>
#include <sys/stat.h>
#endif


namespace zen
{
#ifdef ZEN_WIN
static const char LINE_BREAK[] = "\r\n";
#elif defined ZEN_LINUX || defined ZEN_MAC
static const char LINE_BREAK[] = "\n"; //since OS X apple uses newline, too
#endif

//buffered file IO optimized for sequential read/write accesses + better error reporting + long path support (following symlinks)

#ifdef ZEN_WIN
typedef HANDLE FileHandle;
#elif defined ZEN_LINUX || defined ZEN_MAC
typedef FILE* FileHandle;
#endif

class FileInput : public FileInputBase
{
public:
    FileInput(const Zstring& filename);                    //throw FileError
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

#if defined ZEN_LINUX || defined ZEN_MAC
class FileInputUnbuffered : public FileInputBase
{
public:
    FileInputUnbuffered(const Zstring& filename); //throw FileError
    ~FileInputUnbuffered();

    //considering safe-read.c it seems buffer size should be a multiple of 8192
    virtual size_t read(void* buffer, size_t bytesToRead); //throw FileError; returns actual number of bytes read
    //do NOT rely on partially filled buffer meaning EOF!

    int getDescriptor() { return fdFile;}

private:
    int fdFile;
};

class FileOutputUnbuffered : public FileOutputBase
{
public:
    //creates a new file (no overwrite allowed!)
    FileOutputUnbuffered(const Zstring& filename, mode_t mode); //throw FileError, ErrorTargetPathMissing, ErrorTargetExisting
    FileOutputUnbuffered(int fd, const Zstring& filename); //takes ownership!
    ~FileOutputUnbuffered();

    virtual void write(const void* buffer, size_t bytesToWrite); //throw FileError
    int getDescriptor() { return fdFile;}

private:
    int fdFile;
};
#endif
}

#endif //FILEIO_89578342758342572345
