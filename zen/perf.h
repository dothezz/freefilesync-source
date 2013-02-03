// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) Zenju (zenju AT gmx DOT de) - All Rights Reserved        *
// **************************************************************************

#ifndef DEBUG_PERF_HEADER
#define DEBUG_PERF_HEADER

#include "deprecate.h"
#include "tick_count.h"

#ifdef FFS_WIN
#include <sstream>
#else
#include <iostream>
#endif

//############# two macros for quick performance measurements ###############
#define PERF_START zen::PerfTimer perfTest;
#define PERF_STOP  perfTest.showResult();
//###########################################################################

namespace zen
{
class PerfTimer
{
public:
    class TimerError {};

    ZEN_DEPRECATE
    PerfTimer() : //throw TimerError
        ticksPerSec_(ticksPerSec()), startTime(), resultShown(false)
    {
        //std::clock() - "counts CPU time in C and wall time in VC++" - WTF!???
#ifdef FFS_WIN
        if (::SetThreadAffinityMask(::GetCurrentThread(), 1) == 0) //"should not be required unless there are bugs in BIOS or HAL" - msdn, QueryPerformanceCounter
            throw TimerError();
#endif
        startTime = getTicks();
        if (ticksPerSec_ == 0 || !startTime.isValid())
            throw TimerError();
    }

    ~PerfTimer() { if (!resultShown) showResult(); }

    void showResult()
    {
        const TickVal now = getTicks();
        if (!now.isValid())
            throw TimerError();

        const auto delta = static_cast<long>(1000.0 * dist(startTime, now) / ticksPerSec_);
#ifdef FFS_WIN
        std::ostringstream ss;
        ss << delta << " ms";
        ::MessageBoxA(nullptr, ss.str().c_str(), "Timer", 0);
#else
        std::clog << "Perf: duration: " << delta << " ms\n";
#endif
        resultShown = true;

        startTime = getTicks(); //don't include call to MessageBox()!
        if (!startTime.isValid())
            throw TimerError();
    }

private:
    const std::int64_t ticksPerSec_;
    TickVal startTime;
    bool resultShown;
};
}

#endif //DEBUG_PERF_HEADER
