// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) ZenJu (zhnmju123 AT gmx DOT de) - All Rights Reserved    *
// **************************************************************************

#ifndef DEBUG_PERF_HEADER
#define DEBUG_PERF_HEADER

#include "deprecate.h"

#ifdef FFS_WIN
#include <sstream>
#include "win.h" //includes "windows.h"
#else
#include <iostream>
#include <time.h>
#endif

//two macros for quick performance measurements
#define PERF_START CpuTimer perfTest;
#define PERF_STOP  perfTest.showResult();

#ifdef FFS_WIN
class CpuTimer
{
public:
    class TimerError {};

    ZEN_DEPRECATE
    CpuTimer() : frequency(), startTime(), resultShown(false)
    {
        SetThreadAffinity dummy;
        if (!::QueryPerformanceFrequency(&frequency))
            throw TimerError();
        if (!::QueryPerformanceCounter  (&startTime))
            throw TimerError();
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
        if (!::QueryPerformanceCounter(&currentTime))
            throw TimerError();

        const auto delta = static_cast<long>(1000.0 * (currentTime.QuadPart - startTime.QuadPart) / frequency.QuadPart);
        std::ostringstream ss;
        ss << delta << " ms";

        ::MessageBoxA(nullptr, ss.str().c_str(), "Timer", 0);
        resultShown = true;

        if (!::QueryPerformanceCounter(&startTime))
            throw TimerError(); //don't include call to MessageBox()!
    }

private:
    class SetThreadAffinity
    {
    public:
        SetThreadAffinity() : oldmask(::SetThreadAffinityMask(::GetCurrentThread(), 1)) { if (oldmask == 0) throw TimerError(); }
        ~SetThreadAffinity() { ::SetThreadAffinityMask(::GetCurrentThread(), oldmask); }
    private:
        SetThreadAffinity(const SetThreadAffinity&);
        SetThreadAffinity& operator=(const SetThreadAffinity&);
        const DWORD_PTR oldmask;
    };

    LARGE_INTEGER frequency;
    LARGE_INTEGER startTime;
    bool resultShown;
};


#else
class CpuTimer
{
public:
    class TimerError {};

    ZEN_DEPRECATE
    CpuTimer() : startTime(), resultShown(false)
    {
        //clock() seems to give grossly incorrect results: multi core issue?
        //gettimeofday() seems fine but is deprecated
        if (::clock_gettime(CLOCK_MONOTONIC_RAW, &startTime) != 0) //CLOCK_MONOTONIC measures time reliably across processors!
            throw TimerError();
    }

    ~CpuTimer()
    {
        if (!resultShown)
            showResult();
    }

    void showResult()
    {
        timespec currentTime = {};
        if (::clock_gettime(CLOCK_MONOTONIC_RAW, &currentTime) != 0)
            throw TimerError();

        const auto delta = static_cast<long>((currentTime.tv_sec - startTime.tv_sec) * 1000.0 + (currentTime.tv_nsec - startTime.tv_nsec) / 1000000.0);
        std::clog << "Perf: duration: " << delta << " ms\n";
        resultShown = true;

        if (::clock_gettime(CLOCK_MONOTONIC_RAW, &startTime) != 0)
            throw TimerError();
    }

private:
    timespec startTime;
    bool resultShown;
};
#endif

#endif //DEBUG_PERF_HEADER
