// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) ZenJu (zhnmju123 AT gmx DOT de) - All Rights Reserved    *
// **************************************************************************

#ifndef FILEIO_H_INCLUDED
#define FILEIO_H_INCLUDED

#ifdef FFS_WIN
#include "win.h" //includes "windows.h"

#elif defined FFS_LINUX
#include <cstdio>
#endif

#include "zstring.h"
#include "file_error.h"

namespace zen
{
#ifdef FFS_WIN
static const char LINE_BREAK[] = "\r\n";
#elif defined FFS_LINUX
static const char LINE_BREAK[] = "\n";
#endif

//file IO optimized for sequential read/write accesses + better error reporting + long path support (following symlinks)

#ifdef FFS_WIN
typedef HANDLE FileHandle;
#elif defined FFS_LINUX
typedef FILE* FileHandle;
#endif

class FileInput
{
public:
    FileInput(const Zstring& filename);                    //throw FileError, ErrorNotExisting
    FileInput(FileHandle handle, const Zstring& filename); //takes ownership!
    ~FileInput();

    size_t read(void* buffer, size_t bytesToRead); //throw FileError; returns actual number of bytes read
    bool eof() { return eofReached; } //end of file reached

    const Zstring& getFilename() const { return filename_; }

private:
    FileInput(const FileInput&);
    FileInput& operator=(const FileInput&);

    bool eofReached;
    FileHandle fileHandle;
    const Zstring filename_;
};


class  FileOutput
{
public:
    enum AccessFlag
    {
        ACC_OVERWRITE,
        ACC_CREATE_NEW
    };
    FileOutput(const Zstring& filename, AccessFlag access); //throw FileError, ErrorTargetPathMissing, ErrorTargetExisting
    FileOutput(FileHandle handle, const Zstring& filename); //takes ownership!
    ~FileOutput();

    void write(const void* buffer, size_t bytesToWrite); //throw FileError

    const Zstring& getFilename() const { return filename_; }

private:
    FileOutput(const FileOutput&);
    FileOutput& operator=(const FileOutput&);

    FileHandle fileHandle;
    const Zstring filename_;
};

}

#endif // FILEIO_H_INCLUDED
