// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl-3.0        *
// * Copyright (C) Zenju (zenju AT gmx DOT de) - All Rights Reserved        *
// **************************************************************************

#ifndef TICK_COUNT_H_3807326223463457
#define TICK_COUNT_H_3807326223463457

#include <cstdint>
#include "type_traits.h"
#include "basic_math.h"

    #include <time.h> //Posix ::clock_gettime()


namespace zen
{
//a portable "GetTickCount()" using "wall time equivalent" - e.g. no jumps due to ntp time corrections
class TickVal;
int64_t dist(const TickVal& lhs, const TickVal& rhs); //use absolute difference for paranoid security: even QueryPerformanceCounter "wraps-around" at *some* time

int64_t ticksPerSec(); //return 0 on error
TickVal getTicks();    //return invalid value on error: !TickVal::isValid()









//############################ implementation ##############################
class TickVal
{
public:
    typedef timespec NativeVal;

    TickVal() {}
    explicit TickVal(const NativeVal& val) : val_(val) {}

    inline friend
    int64_t dist(const TickVal& lhs, const TickVal& rhs)
    {
        //structure timespec documented with members:
        //  time_t  tv_sec    seconds
        //  long    tv_nsec   nanoseconds
        const int64_t deltaSec  = lhs.val_.tv_sec  - rhs.val_.tv_sec;
        const int64_t deltaNsec = lhs.val_.tv_nsec - rhs.val_.tv_nsec;
        return numeric::abs(deltaSec * 1000000000 + deltaNsec);
    }

    inline friend
    bool operator<(const TickVal& lhs, const TickVal& rhs)
    {
        if (lhs.val_.tv_sec != rhs.val_.tv_sec)
            return lhs.val_.tv_sec < rhs.val_.tv_sec;
        return lhs.val_.tv_nsec < rhs.val_.tv_nsec;
    }

    bool isValid() const { return dist(*this, TickVal()) != 0; }

private:
    NativeVal val_ {};
};


inline
int64_t ticksPerSec() //return 0 on error
{
    return 1000000000; //precision: nanoseconds

}


inline
TickVal getTicks() //return !isValid() on error
{
    //gettimeofday() seems fine but is deprecated
    timespec now = {};
    if (::clock_gettime(CLOCK_MONOTONIC_RAW, &now) != 0) //CLOCK_MONOTONIC measures time reliably across processors!
        return TickVal();

    return TickVal(now);
}
}

#endif //TICK_COUNT_H_3807326223463457
