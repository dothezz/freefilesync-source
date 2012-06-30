#ifndef CMP_FILETIME_H_INCLUDED
#define CMP_FILETIME_H_INCLUDED

#include <wx/stopwatch.h>
#include <zen/int64.h>

namespace zen
{
//---------------------------------------------------------------------------------------------------------------
inline
bool sameFileTime(const Int64& a, const Int64& b, size_t tolerance)
{
    if (a < b)
        return b <= a + static_cast<int>(tolerance);
    else
        return a <= b + static_cast<int>(tolerance);
}
//---------------------------------------------------------------------------------------------------------------

//number of seconds since Jan 1st 1970 + 1 year (needn't be too precise)
static const long oneYearFromNow = wxGetUTCTime() + 365 * 24 * 3600; //init at program startup alas in *each* compilation untit -> avoid MT issues
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

    static Result getResult(const Int64& lhs, const Int64& rhs, size_t tolerance)
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
