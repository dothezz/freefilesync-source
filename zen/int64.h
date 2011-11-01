// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) 2008-2011 ZenJu (zhnmju123 AT gmx.de)                    *
// **************************************************************************

#ifndef FFS_LARGE_64_BIT_INTEGER_H_INCLUDED
#define FFS_LARGE_64_BIT_INTEGER_H_INCLUDED

#include <cassert>
#include <limits>
#include <cstdint>
#include <cstdint>
#include <ostream>
#include "assert_static.h"
#include "type_tools.h"

#ifdef FFS_WIN
#include "win.h"
#endif


/*
zen::Int64/zen::UInt64: wrapper classes around std::int64_t/std::uint64_t

    - default initialization with 0
    - debug runtime overflow/underflow checks
    - safe and explicit semantics: no unsafe type conversions
    - safe conversion to and from Windows 64-bit integers
    - specializes std::numeric_limits
    - support stream operator<< and operator>>
*/

namespace zen
{
template <class T, class U> inline void checkRange(U value)
{
    //caveat: std::numeric_limits<T>::min returns minimum positive(!) number for T = double, while behaving correctly for integer types... sigh
    assert(double(std::numeric_limits<T>::lowest()) <= double(value) && //new with C++11!
           double(std::numeric_limits<T>::max()   ) >= double(value));

    //    assert(double(boost::numeric::bounds<T>::lowest ()) <= double(value) &&
    //           double(boost::numeric::bounds<T>::highest()) >= double(value));
}

class Int64
{
    struct DummyClass { operator int() { return 0; } };
public:
    //safe implicit conversions
    Int64()                   : value(0) {}
    Int64(const Int64&   rhs) : value(rhs.value) {}
    Int64(int            rhs) : value(rhs) {} //ambiguity intentional for types other than these
    Int64(long           rhs) : value(rhs) {}
    Int64(SelectIf<IsSameType<std::int64_t, long>::result, DummyClass, std::int64_t>::Result rhs) :
        value(rhs) {} //-> std::int64_t equals long int on x64 Linux! Still we want implicit behavior for all other systems!

    //unsafe explicit but checked conversion from arbitrary integer type
    template <class T> explicit Int64(T rhs) : value(rhs) { checkRange<std::int64_t>(rhs); }

    Int64& operator=(const Int64& rhs) { value = rhs.value; return *this; }

#ifdef FFS_WIN
    Int64(DWORD low, LONG high)
    {
        assert_static(sizeof(low) + sizeof(high) == sizeof(value));

        LARGE_INTEGER cvt = {};
        cvt.LowPart  = low;
        cvt.HighPart = high;
        value = cvt.QuadPart;
    }
    LONG getHi() const
    {
        LARGE_INTEGER cvt = {};
        cvt.QuadPart = value;
        return cvt.HighPart;
    }
    DWORD getLo() const
    {
        LARGE_INTEGER cvt = {};
        cvt.QuadPart = value;
        return cvt.LowPart;
    }
#endif

    Int64& operator+=(const Int64& rhs) { checkRange<std::int64_t>(double(value) + rhs.value); value += rhs.value; return *this; }
    Int64& operator-=(const Int64& rhs) { checkRange<std::int64_t>(double(value) - rhs.value); value -= rhs.value; return *this; }
    Int64& operator*=(const Int64& rhs) { checkRange<std::int64_t>(double(value) * rhs.value); value *= rhs.value; return *this; }
    Int64& operator/=(const Int64& rhs) { assert(rhs.value != 0);                              value /= rhs.value; return *this; }
    Int64& operator%=(const Int64& rhs) { assert(rhs.value != 0);                              value %= rhs.value; return *this; }
    Int64& operator&=(const Int64& rhs) {                                                      value &= rhs.value; return *this;}
    Int64& operator|=(const Int64& rhs) {                                                      value |= rhs.value; return *this;}
    Int64& operator<<=(int rhs) { assert(rhs < 0 || (value << rhs) >> rhs == value);           value <<= rhs; return *this; }
    Int64& operator>>=(int rhs) { assert(rhs > 0 || (value >> rhs) << rhs == value);           value >>= rhs; return *this; }

    inline friend bool operator==(const Int64& lhs, const Int64& rhs) { return lhs.value == rhs.value; }
    inline friend bool operator!=(const Int64& lhs, const Int64& rhs) { return lhs.value != rhs.value; }
    inline friend bool operator< (const Int64& lhs, const Int64& rhs) { return lhs.value <  rhs.value; }
    inline friend bool operator> (const Int64& lhs, const Int64& rhs) { return lhs.value >  rhs.value; }
    inline friend bool operator<=(const Int64& lhs, const Int64& rhs) { return lhs.value <= rhs.value; }
    inline friend bool operator>=(const Int64& lhs, const Int64& rhs) { return lhs.value >= rhs.value; }

    //checked conversion to arbitrary target integer type
    template <class T> inline friend T to(Int64 number) { checkRange<T>(number.value); return static_cast<T>(number.value); }

    template <class T> inline friend std::basic_istream<T>& operator>>(std::basic_istream<T>& lhs, Int64& rhs) { lhs >> rhs.value; return lhs; }
    template <class T> inline friend std::basic_ostream<T>& operator<<(std::basic_ostream<T>& lhs, const Int64& rhs) { lhs << rhs.value; return lhs; }

private:
    std::int64_t value;
};

inline Int64 operator+ (const Int64& lhs, const Int64& rhs) { return Int64(lhs) += rhs; }
inline Int64 operator- (const Int64& lhs, const Int64& rhs) { return Int64(lhs) -= rhs; }
inline Int64 operator* (const Int64& lhs, const Int64& rhs) { return Int64(lhs) *= rhs; }
inline Int64 operator/ (const Int64& lhs, const Int64& rhs) { return Int64(lhs) /= rhs; }
inline Int64 operator% (const Int64& lhs, const Int64& rhs) { return Int64(lhs) %= rhs; }
inline Int64 operator& (const Int64& lhs, const Int64& rhs) { return Int64(lhs) &= rhs; }
inline Int64 operator| (const Int64& lhs, const Int64& rhs) { return Int64(lhs) |= rhs; }
inline Int64 operator<<(const Int64& lhs, int rhs) { return Int64(lhs) <<= rhs; }
inline Int64 operator>>(const Int64& lhs, int rhs) { return Int64(lhs) >>= rhs; }


class UInt64
{
    struct DummyClass { operator size_t() { return 0U; } };
public:
    //safe implicit conversions
    UInt64()                    : value(0) {}
    UInt64(const UInt64&   rhs) : value(rhs.value) {}
    UInt64(unsigned int    rhs) : value(rhs) {} //ambiguity intentional for types other than these
    UInt64(unsigned long   rhs) : value(rhs) {}
    UInt64(SelectIf<IsSameType<std::uint64_t, unsigned long>::result, DummyClass, std::uint64_t>::Result rhs) :
        value(rhs) {} //-> std::uint64_t equals unsigned long int on x64 Linux! Still we want implicit behavior for all other systems!

    //unsafe explicit but checked conversion from arbitrary integer type
    template <class T> explicit UInt64(T rhs) : value(rhs) { checkRange<std::uint64_t>(rhs); }

    UInt64& operator=(const UInt64& rhs) { value = rhs.value; return *this; }

#ifdef FFS_WIN
    UInt64(DWORD low, DWORD high)
    {
        assert_static(sizeof(low) + sizeof(high) == sizeof(value));

        ULARGE_INTEGER cvt = {};
        cvt.LowPart  = low;
        cvt.HighPart = high;
        value = cvt.QuadPart;
    }
    DWORD getHi() const
    {
        ULARGE_INTEGER cvt = {};
        cvt.QuadPart = value;
        return cvt.HighPart;
    }
    DWORD getLo() const
    {
        ULARGE_INTEGER cvt = {};
        cvt.QuadPart = value;
        return cvt.LowPart;
    }
#endif

    UInt64& operator+=(const UInt64& rhs) { checkRange<std::uint64_t>(double(value) + rhs.value); value += rhs.value; return *this; }
    UInt64& operator-=(const UInt64& rhs) { checkRange<std::uint64_t>(double(value) - rhs.value); value -= rhs.value; return *this; }
    UInt64& operator*=(const UInt64& rhs) { checkRange<std::uint64_t>(double(value) * rhs.value); value *= rhs.value; return *this; }
    UInt64& operator/=(const UInt64& rhs) { assert(rhs.value != 0);                                 value /= rhs.value; return *this; }
    UInt64& operator%=(const UInt64& rhs) { assert(rhs.value != 0);                                 value %= rhs.value; return *this; }
    UInt64& operator&=(const UInt64& rhs) {                                                         value &= rhs.value; return *this;}
    UInt64& operator|=(const UInt64& rhs) {                                                         value |= rhs.value; return *this;}
    UInt64& operator<<=(int rhs) { assert(rhs < 0 || (value << rhs) >> rhs == value);               value <<= rhs; return *this; }
    UInt64& operator>>=(int rhs) { assert(rhs > 0 || (value >> rhs) << rhs == value);               value >>= rhs; return *this; }

    inline friend bool operator==(const UInt64& lhs, const UInt64& rhs) { return lhs.value == rhs.value; }
    inline friend bool operator!=(const UInt64& lhs, const UInt64& rhs) { return lhs.value != rhs.value; }
    inline friend bool operator< (const UInt64& lhs, const UInt64& rhs) { return lhs.value <  rhs.value; }
    inline friend bool operator> (const UInt64& lhs, const UInt64& rhs) { return lhs.value >  rhs.value; }
    inline friend bool operator<=(const UInt64& lhs, const UInt64& rhs) { return lhs.value <= rhs.value; }
    inline friend bool operator>=(const UInt64& lhs, const UInt64& rhs) { return lhs.value >= rhs.value; }

    //checked conversion to arbitrary target integer type
    template <class T> inline friend T to(UInt64 number) { checkRange<T>(number.value); return static_cast<T>(number.value); }

    template <class T> inline friend std::basic_istream<T>& operator>>(std::basic_istream<T>& lhs, UInt64& rhs) { lhs >> rhs.value; return lhs; }
    template <class T> inline friend std::basic_ostream<T>& operator<<(std::basic_ostream<T>& lhs, const UInt64& rhs) { lhs << rhs.value; return lhs; }

private:
    std::uint64_t value;
};

inline UInt64 operator+ (const UInt64& lhs, const UInt64& rhs) { return UInt64(lhs) += rhs; }
inline UInt64 operator- (const UInt64& lhs, const UInt64& rhs) { return UInt64(lhs) -= rhs; }
inline UInt64 operator* (const UInt64& lhs, const UInt64& rhs) { return UInt64(lhs) *= rhs; }
inline UInt64 operator/ (const UInt64& lhs, const UInt64& rhs) { return UInt64(lhs) /= rhs; }
inline UInt64 operator% (const UInt64& lhs, const UInt64& rhs) { return UInt64(lhs) %= rhs; }
inline UInt64 operator& (const UInt64& lhs, const UInt64& rhs) { return UInt64(lhs) &= rhs; }
inline UInt64 operator| (const UInt64& lhs, const UInt64& rhs) { return UInt64(lhs) |= rhs; }
inline UInt64 operator<<(const UInt64& lhs, int rhs) { return UInt64(lhs) <<= rhs; }
inline UInt64 operator>>(const UInt64& lhs, int rhs) { return UInt64(lhs) >>= rhs; }

template <> inline UInt64 to(Int64 number) { checkRange<std::uint64_t>(number.value); return UInt64(number.value); }
template <> inline Int64 to(UInt64 number) { checkRange<std:: int64_t>(number.value); return  Int64(number.value); }


#ifdef FFS_WIN
//convert FILETIME (number of 100-nanosecond intervals since January 1, 1601 UTC)
//       to time_t (number of seconds since Jan. 1st 1970 UTC)
//
//FAT32 time is preserved exactly: FAT32 -> toTimeT -> tofiletime -> FAT32
inline
Int64 toTimeT(const FILETIME& ft)
{
    return to<Int64>(UInt64(ft.dwLowDateTime, ft.dwHighDateTime) / 10000000U) - Int64(3054539008UL, 2);
    //timeshift between ansi C time and FILETIME in seconds == 11644473600s
}

inline
FILETIME tofiletime(const Int64& utcTime)
{
    const UInt64 fileTimeLong = to<UInt64>(utcTime + Int64(3054539008UL, 2)) * 10000000U;
    const FILETIME output = { fileTimeLong.getLo(), fileTimeLong.getHi() };
    return output;
}
#endif
}

//specialize numeric limits
namespace std
{
assert_static(std::numeric_limits<std:: int64_t>::is_specialized);
assert_static(std::numeric_limits<std::uint64_t>::is_specialized);

template <> class numeric_limits<zen::Int64>  : public numeric_limits<std::int64_t>
{
public:
    static zen::Int64 min() throw() { return numeric_limits<std::int64_t>::min(); }
    static zen::Int64 max() throw() { return numeric_limits<std::int64_t>::max(); }
};

template <> class numeric_limits<zen::UInt64> : public numeric_limits<std::uint64_t>
{
public:
    static zen::UInt64 min() throw() { return numeric_limits<std::uint64_t>::min(); }
    static zen::UInt64 max() throw() { return numeric_limits<std::uint64_t>::max(); }
};
}

/*
//specialize zen type trait
namespace zen -> we cannot mix signed/unsigned in general arithmetic operations -> we'll use the ostream-approach
{
template <> struct IsUnsignedInt<UInt64> { enum { result = true }; };
template <> struct IsSignedInt  <Int64> { enum { result = true }; };
}
*/
#endif //FFS_LARGE_64_BIT_INTEGER_H_INCLUDED
