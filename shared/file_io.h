// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) 2008-2011 ZenJu (zhnmju123 AT gmx.de)                    *
// **************************************************************************
//
#ifndef FILEIO_H_INCLUDED
#define FILEIO_H_INCLUDED

#include <wx/stream.h>

#ifdef FFS_WIN
#include <wx/msw/wrapwin.h> //includes "windows.h"
#elif defined FFS_LINUX
#include <cstdio>
#endif

#include "zstring.h"
#include "file_error.h"

namespace ffs3
{
//file IO optimized for sequential read/write accesses + better error reporting + long path support (following symlinks)

class FileInput
{
public:
    FileInput(const Zstring& filename); //throw (FileError, ErrorNotExisting)
    ~FileInput();

    size_t read(void* buffer, size_t bytesToRead); //throw FileError(); returns actual number of bytes read
    bool eof(); //end of file reached

private:
#ifdef FFS_WIN
    HANDLE fileHandle;
    bool eofReached;
#elif defined FFS_LINUX
    FILE* fileHandle;
#endif
    const Zstring filename_;
};


class  FileOutput
{
public:
    FileOutput(const Zstring& filename); //throw FileError()
    ~FileOutput();

    void write(const void* buffer, size_t bytesToWrite); //throw FileError()
    void close(); //close file stream
private:
#ifdef FFS_WIN
    HANDLE fileHandle;
#elif defined FFS_LINUX
    FILE* fileHandle;
#endif
    const Zstring filename_;
};


//############# wxWidgets stream adapter #############
// can be used as base classes (have virtual destructors)
class FileInputStream : public wxInputStream
{
public:
    FileInputStream(const Zstring& filename) : //throw FileError()
        fileObj(filename) {}

private:
    virtual size_t OnSysRead(void* buffer, size_t bufsize) //throw FileError()
    {
        return fileObj.read(buffer, bufsize);
    }

    FileInput fileObj;
};


class FileOutputStream : public wxOutputStream
{
public:
    FileOutputStream(const Zstring& filename) : //throw FileError()
        fileObj(filename) {}

private:
    virtual size_t OnSysWrite(const void* buffer, size_t bufsize) //throw FileError()
    {
        fileObj.write(buffer, bufsize);
        return bufsize;
    }

    FileOutput fileObj;
};
}

#endif // FILEIO_H_INCLUDED
