// **************************************************************************
// * This file is part of the zenXML project. It is distributed under the   *
// * Boost Software License: http://www.boost.org/LICENSE_1_0.txt           *
// * Copyright (C) Zenju (zenju AT gmx DOT de) - All Rights Reserved        *
// **************************************************************************

#ifndef ZEN_TICK_COUNT_HEADER_3807326
#define ZEN_TICK_COUNT_HEADER_3807326

#include <cstdint>
//#include <algorithm>
#include "type_traits.h"
#include "basic_math.h"
//#include "assert_static.h"
//#include <cmath>
//template <class T> inline
//T dist(T a, T b)
//{
//    return a > b ? a - b : b - a;
//}


#ifdef FFS_WIN
#include "win.h" //includes "windows.h"
#elif defined FFS_LINUX
#include <time.h> //Posix ::clock_gettime()
#endif

namespace zen
{
//a portable "GetTickCount()" using "wall time equivalent" - e.g. no jumps due to ntp time corrections
class TickVal;
std::int64_t dist(const TickVal& lhs, const TickVal& rhs); //use absolute difference for paranoid security: even QueryPerformanceCounter "wraps-around" at *some* time

std::int64_t ticksPerSec(); //return 0 on error
TickVal getTicks();         //return invalid value on error: !TickVal::isValid()















//############################ implementation ##############################
class TickVal
{
public:
#ifdef FFS_WIN
    typedef LARGE_INTEGER NativeVal;
#elif defined FFS_LINUX
    typedef timespec NativeVal;
#endif

    TickVal() : val_() {}
    explicit TickVal(const NativeVal& val) : val_(val) {}

    inline friend
    std::int64_t dist(const TickVal& lhs, const TickVal& rhs)
    {
#ifdef FFS_WIN
        return numeric::dist(lhs.val_.QuadPart, rhs.val_.QuadPart); //std::abs(a - b) can lead to overflow!

#elif defined FFS_LINUX
        const auto distSec  = numeric::dist(lhs.val_.tv_sec,  rhs.val_.tv_sec);
        const auto distNsec = numeric::dist(lhs.val_.tv_nsec, rhs.val_.tv_nsec);

        if (distSec > (std::numeric_limits<std::int64_t>::max() - distNsec) / 1000000000) //truncate instead of overflow!
            return std::numeric_limits<std::int64_t>::max();
        return distSec * 1000000000 + distNsec;
#endif
    }

    inline friend
    bool operator<(const TickVal& lhs, const TickVal& rhs) //evaluate directly rather than reuse operator-
    {
#ifdef FFS_WIN
        return lhs.val_.QuadPart < rhs.val_.QuadPart;
#elif defined FFS_LINUX
        if (lhs.val_.tv_sec != rhs.val_.tv_sec)
            return lhs.val_.tv_sec < rhs.val_.tv_sec;
        return lhs.val_.tv_nsec < rhs.val_.tv_nsec;
#endif
    }

    bool isValid() const { return dist(*this, TickVal()) != 0; }

private:
    NativeVal val_;
};


inline
std::int64_t ticksPerSec() //return 0 on error
{
#ifdef FFS_WIN
    LARGE_INTEGER frequency = {};
    if (!::QueryPerformanceFrequency(&frequency)) //MSDN promises: "The frequency cannot change while the system is running."
        return 0;
    static_assert(sizeof(std::int64_t) >= sizeof(frequency.QuadPart), "");
    return frequency.QuadPart;

#elif defined FFS_LINUX
    return 1000000000; //precision: nanoseconds
#endif
}


inline
TickVal getTicks() //return 0 on error
{
#ifdef FFS_WIN
    LARGE_INTEGER now = {};
    if (!::QueryPerformanceCounter(&now)) //msdn: SetThreadAffinityMask() may be required if there are bugs in BIOS or HAL"
        return TickVal();

#elif defined FFS_LINUX
    //gettimeofday() seems fine but is deprecated
    timespec now = {};
    if (::clock_gettime(CLOCK_MONOTONIC_RAW, &now) != 0) //CLOCK_MONOTONIC measures time reliably across processors!
        return TickVal();
#endif
    return TickVal(now);
}
}

#endif //ZEN_TICK_COUNT_HEADER_3807326
