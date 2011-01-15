// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) 2008-2011 ZenJu (zhnmju123 AT gmx.de)                    *
// **************************************************************************
//
#include "binary.h"
#include "../shared/file_io.h"
#include <vector>
#include <wx/stopwatch.h>


inline
void setMinSize(std::vector<char>& buffer, size_t minSize)
{
    if (buffer.size() < minSize) //this is similar to reserve(), but we need a "properly initialized" array here
        buffer.resize(minSize);
}


namespace
{
class BufferSize
{
public:
    BufferSize() : bufSize(BUFFER_SIZE_START) {}

    void inc()
    {
        if (bufSize < BUFFER_SIZE_MAX)
            bufSize *= 2;
    }

    void dec()
    {
        if (bufSize > BUFFER_SIZE_MIN)
            bufSize /= 2;
    }

    operator size_t() const
    {
        return bufSize;
    }

private:
    static const size_t BUFFER_SIZE_MIN   =       128 * 1024;
    static const size_t BUFFER_SIZE_START =       512 * 1024; //512 kb seems to be a reasonable initial buffer size
    static const size_t BUFFER_SIZE_MAX   = 16 * 1024 * 1024;

    /*Tests on Win7 x64 show that buffer size does NOT matter if files are located on different physical disks!
    Impact of buffer size when files are on same disk:

    buffer  MB/s
    ------------
    64      10
    128     19
    512     40
    1024    48
    2048    56
    4096    56
    8192    56
    */

    size_t bufSize;
};
}


bool ffs3::filesHaveSameContent(const Zstring& filename1, const Zstring& filename2, CompareCallback& callback)
{
    FileInput file1(filename1); //throw FileError()
    FileInput file2(filename2); //throw FileError()

    BufferSize bufferSize;

    static std::vector<char> memory1;
    static std::vector<char> memory2;

    wxLongLong bytesCompared;

    wxLongLong lastDelayViolation = wxGetLocalTimeMillis();

    do
    {
        setMinSize(memory1, bufferSize);
        setMinSize(memory2, bufferSize);

        const wxLongLong startTime = wxGetLocalTimeMillis();

        const size_t length1 = file1.read(&memory1[0], bufferSize); //returns actual number of bytes read; throw FileError()
        const size_t length2 = file2.read(&memory2[0], bufferSize); //

        const wxLongLong stopTime = wxGetLocalTimeMillis();

        //-------- dynamically set buffer size to keep callback interval between 200 - 500ms ---------------------
        const wxLongLong loopTime = stopTime - startTime;
        if (loopTime < 200 && stopTime - lastDelayViolation > 2000) //avoid "flipping back": e.g. DVD-Roms read 32MB at once, so first read may be > 300 ms, but second one will be 0ms!
        {
            lastDelayViolation = stopTime;
            bufferSize.inc(); //practically no costs!
        }
        else if (loopTime > 500)
        {
            lastDelayViolation = stopTime;
            bufferSize.dec(); //
        }
        //------------------------------------------------------------------------------------------------

        if (length1 != length2 || ::memcmp(&memory1[0], &memory2[0], length1) != 0)
            return false;

        bytesCompared += length1 * 2;
        callback.updateCompareStatus(bytesCompared); //send progress updates
    }
    while (!file1.eof());

    if (!file2.eof()) //highly unlikely, but theoretically possible! (but then again, not in this context where both files have same size...)
        return false;

    return true;
}
