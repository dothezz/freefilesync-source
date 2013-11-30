#ifndef CMP_FILETIME_H_INCLUDED
#define CMP_FILETIME_H_INCLUDED

#include <ctime>
#include <zen/int64.h>

namespace zen
{
//---------------------------------------------------------------------------------------------------------------
inline
bool sameFileTime(const Int64& lhs, const Int64& rhs, int tolerance)
{
    if (tolerance < 0) //= unlimited tolerance by convention!
        return true;

    if (lhs < rhs)
        return rhs - lhs <= tolerance;
    else
        return lhs - rhs <= tolerance;
}
//---------------------------------------------------------------------------------------------------------------

//number of seconds since Jan 1st 1970 + 1 year (needn't be too precise)
static const Int64 oneYearFromNow = std::time(nullptr) + 365 * 24 * 3600; //init at program startup in *each* compilation untit -> avoid MT issues
//refactor when C++11 thread-safe static initialization is availalbe in VS (already in GCC)

class CmpFileTime
{
public:
    enum Result
    {
        TIME_EQUAL,
        TIME_LEFT_NEWER,
        TIME_RIGHT_NEWER,
        TIME_LEFT_INVALID,
        TIME_RIGHT_INVALID
    };

    static Result getResult(const Int64& lhs, const Int64& rhs, int tolerance)
    {
        if (sameFileTime(lhs, rhs, tolerance)) //last write time may differ by up to 2 seconds (NTFS vs FAT32)
            return TIME_EQUAL;

        //check for erroneous dates
        if (lhs < 0 || lhs > oneYearFromNow) //earlier than Jan 1st 1970 or more than one year in future
            return TIME_LEFT_INVALID;

        if (rhs < 0 || rhs > oneYearFromNow)
            return TIME_RIGHT_INVALID;

        //regular time comparison
        if (lhs < rhs)
            return TIME_RIGHT_NEWER;
        else
            return TIME_LEFT_NEWER;
    }
};
}

#endif // CMP_FILETIME_H_INCLUDED
