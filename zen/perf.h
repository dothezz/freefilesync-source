// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) 2008-2011 ZenJu (zhnmju123 AT gmx.de)                    *
// **************************************************************************

#ifndef DEBUG_PERF_HEADER
#define DEBUG_PERF_HEADER

#include <sstream>
#include "win.h" //includes "windows.h"

#ifdef __MINGW32__
#define DEPRECATED(x) x __attribute__ ((deprecated))
#elif defined _MSC_VER
#define DEPRECATED(x) __declspec(deprecated) x
#endif


//two macros for quick performance measurements
#define PERF_START CpuTimer perfTest;
#define PERF_STOP  perfTest.showResult();

class CpuTimer
{
public:
    class TimerError {};

    DEPRECATED(CpuTimer()) : frequency(), startTime(), resultShown(false)
    {
        SetThreadAffinity dummy;
        if (!::QueryPerformanceFrequency(&frequency)) throw TimerError();
        if (!::QueryPerformanceCounter  (&startTime)) throw TimerError();
    }

    ~CpuTimer()
    {
        if (!resultShown)
            showResult();
    }

    void showResult()
    {
        SetThreadAffinity dummy;
        LARGE_INTEGER currentTime = {};
        if (!::QueryPerformanceCounter(&currentTime)) throw TimerError();

        const long delta = static_cast<long>(1000.0 * (currentTime.QuadPart - startTime.QuadPart) / frequency.QuadPart);
        std::ostringstream ss;
        ss << delta << " ms";

        ::MessageBoxA(NULL, ss.str().c_str(), "Timer", 0);
        resultShown = true;

        if (!::QueryPerformanceCounter(&startTime)) throw TimerError(); //don't include call to MessageBox()!
    }

private:
    class SetThreadAffinity
    {
    public:
        SetThreadAffinity() : oldmask(::SetThreadAffinityMask(::GetCurrentThread(), 1)) { if (oldmask == 0) throw TimerError(); }
        ~SetThreadAffinity() { ::SetThreadAffinityMask(::GetCurrentThread(), oldmask); }
    private:
        const DWORD_PTR oldmask;
    };

    LARGE_INTEGER frequency;
    LARGE_INTEGER startTime;
    bool resultShown;
};

#endif //DEBUG_PERF_HEADER
