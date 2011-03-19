// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) 2008-2011 ZenJu (zhnmju123 AT gmx.de)                    *
// **************************************************************************
//
#ifndef DEBUG_PERF_HEADER
#define DEBUG_PERF_HEADER

#include <sstream>
//#define WIN32_LEAN_AND_MEAN -> not in a header
/*
#include <windows.h>
#undef max
#undef min
*/
#include <wx/msw/wrapwin.h> //includes "windows.h"


#ifdef __MINGW32__
#define DEPRECATED(x) x __attribute__ ((deprecated))
#elif defined _MSC_VER
#define DEPRECATED(x) __declspec(deprecated) x
#endif

class CpuTimer
{
public:
    class TimerError {};

    DEPRECATED(CpuTimer()) : resultWasShown(false), startTime(), frequency()
    {
        if (!::QueryPerformanceFrequency(&frequency)) throw TimerError();
        if (!::QueryPerformanceCounter  (&startTime)) throw TimerError();
    }

    ~CpuTimer()
    {
        if (!resultWasShown)
            showResult();
    }

    void showResult()
    {
        LARGE_INTEGER currentTime = {};
        if (!::QueryPerformanceCounter(&currentTime)) throw TimerError();

        const long delta = 1000.0 * (currentTime.QuadPart - startTime.QuadPart) / frequency.QuadPart;
        std::ostringstream ss;
        ss << delta << " ms";

        ::MessageBoxA(NULL, ss.str().c_str(), "Timer", 0);
        resultWasShown = true;

        if (!::QueryPerformanceCounter(&startTime)) throw TimerError(); //don't include call to MessageBox()!
    }

private:
    bool resultWasShown;
    LARGE_INTEGER startTime;
    LARGE_INTEGER frequency;
};

//two macros for quick performance measurements
#define PERF_START CpuTimer perfTest;
#define PERF_STOP  perfTest.showResult();

#endif //DEBUG_PERF_HEADER
