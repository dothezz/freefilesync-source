#ifndef DST_HACK_H_INCLUDED
#define DST_HACK_H_INCLUDED

#include <wx/msw/wrapwin.h> //includes "windows.h"
#include "zstring.h"
#include <stdexcept>


namespace dst
{
/*
Solve DST +-1h and time zone shift issues on FAT drives
-------------------------------------------------------
- (local) last write time is not touched!
- all additional metadata is encoded in local create time:
   I. indicator that offset in II) is present
  II. local<->UTC time offset
 III. indicator that offset in II) corresponds to current local write time (a hash of local last write time)
*/

bool isFatDrive(const Zstring& fileName); //throw ()

//all subsequent functions may throw the std::runtime_error exception!

struct RawTime //time as retrieved by ::FindFirstFile() and ::GetFileAttributesEx()
{
    RawTime(const FILETIME& create, const FILETIME& lastWrite) : createTimeRaw(create), writeTimeRaw(lastWrite) {}
    FILETIME createTimeRaw;
    FILETIME writeTimeRaw;
};
//save UTC time resistant against DST/time zone shifts
bool fatHasUtcEncoded(const RawTime& rawTime);      //as retrieved by ::FindFirstFile() and ::GetFileAttributesEx(); throw (std::runtime_error)

RawTime fatEncodeUtcTime(const FILETIME& writeTimeRealUtc); //throw (std::runtime_error)
FILETIME fatDecodeUtcTime(const RawTime& rawTime); //return last write time in real UTC; throw (std::runtime_error)
}

#endif // DST_HACK_H_INCLUDED
