// **************************************************************************
// * This file is part of the zenXML project. It is distributed under the   *
// * Boost Software License, Version 1.0. See accompanying file             *
// * LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt.       *
// * Copyright (C) ZenJu (zhnmju123 AT gmx DOT de) - All Rights Reserved    *
// **************************************************************************

#ifndef ZEN_TICK_COUNT_HEADER_3807326
#define ZEN_TICK_COUNT_HEADER_3807326

#include <cstdint>
#include "type_traits.h"
#include "assert_static.h"

#ifdef FFS_WIN
#include "win.h" //includes "windows.h"
#elif defined FFS_LINUX
#include <time.h> //Posix ::clock_gettime()
#endif

namespace zen
{
//a portable "GetTickCount()" using "wall time equivalent" - e.g. no jumps due to ntp time corrections
class TickVal;
std::int64_t operator-(const TickVal& lhs, const TickVal& rhs);

std::int64_t ticksPerSec(); //return 0 on error
TickVal getTicks();         //return invalid value on error



















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
    std::int64_t operator-(const TickVal& lhs, const TickVal& rhs)
    {
#ifdef FFS_WIN
        assert_static(IsSignedInt<decltype(lhs.val_.QuadPart)>::value);
        return lhs.val_.QuadPart - rhs.val_.QuadPart;
#elif defined FFS_LINUX
        assert_static(IsSignedInt<decltype(lhs.val_.tv_sec)>::value);
        assert_static(IsSignedInt<decltype(lhs.val_.tv_nsec)>::value);
        return static_cast<std::int64_t>(lhs.val_.tv_sec - rhs.val_.tv_sec) * 1000000000.0 + lhs.val_.tv_nsec - rhs.val_.tv_nsec;
#endif
    }

    bool isValid() const { return *this - TickVal() != 0; }

private:
    NativeVal val_;
};


inline
std::int64_t ticksPerSec() //return 0 on error
{
#ifdef FFS_WIN
    LARGE_INTEGER frequency = {};
    if (!::QueryPerformanceFrequency(&frequency))
        return 0;
    assert_static(sizeof(std::int64_t) >= sizeof(frequency.QuadPart));
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
