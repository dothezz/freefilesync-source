// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl-3.0        *
// * Copyright (C) Zenju (zenju AT gmx DOT de) - All Rights Reserved        *
// **************************************************************************

#include "binary.h"
#include <zen/tick_count.h>
#include <vector>
#include <zen/file_io.h>
#include <boost/thread/tss.hpp>

using namespace zen;
using ABF = AbstractBaseFolder;

namespace
{
/*
1. there seems to be no perf improvement possible when using file mappings instad of ::ReadFile() calls on Windows:
	=> buffered   access: same perf
	=> unbuffered access: same perf on USB stick, file mapping 30% slower on local disk

2. Tests on Win7 x64 show that buffer size does NOT matter if files are located on different physical disks!
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

class BufferSize
{
public:
    BufferSize(size_t initialSize) : bufSize(initialSize) {}

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

    size_t get() const { return bufSize; }

private:
    static const size_t BUFFER_SIZE_MIN   =            8 * 1024; //slow FTP transfer!
    static const size_t BUFFER_SIZE_MAX   =  1024 * 1024 * 1024;

    size_t bufSize;
};


inline
void setMinSize(std::vector<char>& buffer, size_t minSize)
{
    if (buffer.size() < minSize) //this is similar to reserve(), but we need a "properly initialized" array here
        buffer.resize(minSize);
}


const std::int64_t TICKS_PER_SEC = ticksPerSec();
}


bool zen::filesHaveSameContent(const AbstractPathRef& filePath1, const AbstractPathRef& filePath2, const std::function<void(std::int64_t bytesDelta)>& onUpdateStatus) //throw FileError
{
    static boost::thread_specific_ptr<std::vector<char>> cpyBuf1;
    static boost::thread_specific_ptr<std::vector<char>> cpyBuf2;
    if (!cpyBuf1.get())
        cpyBuf1.reset(new std::vector<char>());
    if (!cpyBuf2.get())
        cpyBuf2.reset(new std::vector<char>());

    std::vector<char>& memory1 = *cpyBuf1;
    std::vector<char>& memory2 = *cpyBuf2;

    const std::unique_ptr<ABF::InputStream> inStream1 = ABF::getInputStream(filePath1); //throw FileError, (ErrorFileLocked)
    const std::unique_ptr<ABF::InputStream> inStream2 = ABF::getInputStream(filePath2); //

    BufferSize bufferSize(std::min(inStream1->optimalBlockSize(),
                                   inStream2->optimalBlockSize()));

    TickVal lastDelayViolation = getTicks();

    for (;;)
    {
        setMinSize(memory1, bufferSize.get());
        setMinSize(memory2, bufferSize.get());

        const TickVal startTime = getTicks();

        const size_t length1 = inStream1->read(&memory1[0], bufferSize.get()); //throw FileError
        const size_t length2 = inStream2->read(&memory2[0], bufferSize.get()); //returns actual number of bytes read
        //send progress updates immediately after reading to reliably allow speed calculations for our clients!
        if (onUpdateStatus)
            onUpdateStatus(std::max(length1, length2));

        if (length1 != length2 || ::memcmp(&memory1[0], &memory2[0], length1) != 0)
            return false;

        //-------- dynamically set buffer size to keep callback interval between 100 - 500ms ---------------------
        if (TICKS_PER_SEC > 0)
        {
            const TickVal now = getTicks();

            const std::int64_t loopTime = dist(startTime, now) * 1000 / TICKS_PER_SEC; //unit: [ms]
            if (loopTime < 100)
            {
                if (dist(lastDelayViolation, now) / TICKS_PER_SEC > 2) //avoid "flipping back": e.g. DVD-Roms read 32MB at once, so first read may be > 500 ms, but second one will be 0ms!
                {
                    lastDelayViolation = now;
                    bufferSize.inc();
                }
            }
            else if (loopTime > 500)
            {
                lastDelayViolation = now;
                bufferSize.dec();
            }
        }
        //------------------------------------------------------------------------------------------------

        if (length1 != bufferSize.get()) //end of file
            return true;
    }
}
