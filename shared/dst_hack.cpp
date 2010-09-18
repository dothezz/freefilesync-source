#include "dst_hack.h"
#include "system_constants.h"
#include <wx/intl.h>
#include "long_path_prefix.h"
#include "string_conv.h"
#include "system_func.h"
#include <wx/longlong.h>
#include "assert_static.h"
#include <bitset>
#include "global_func.h"
#include <limits>


bool dst::isFatDrive(const Zstring& fileName) //throw()
{
    using namespace ffs3;

    const size_t BUFFER_SIZE = MAX_PATH + 1;
    wchar_t buffer[BUFFER_SIZE];

//this call is expensive: ~1.5 ms!
//    if (!::GetVolumePathName(applyLongPathPrefix(fileName).c_str(), //__in   LPCTSTR lpszFileName,
//                             buffer,                                     //__out  LPTSTR lpszVolumePathName,
//                             BUFFER_SIZE))                               //__in   DWORD cchBufferLength
// ...
//    Zstring volumePath = buffer;
//    if (!volumePath.EndsWith(common::FILE_NAME_SEPARATOR)) //a trailing backslash is required
//        volumePath += common::FILE_NAME_SEPARATOR;

    //fast ::GetVolumePathName() clone: let's hope it's not too simple (doesn't honor mount points)
    const Zstring nameFmt = removeLongPathPrefix(fileName); //throw()
    const size_t pos = nameFmt.find(Zstr(":\\"));
    if (pos != 1) //expect single letter volume
        return false;
    const Zstring volumePath(nameFmt.c_str(), 3);

    //suprisingly fast: ca. 0.03 ms per call!
    if (!::GetVolumeInformation(volumePath.c_str(), //__in_opt   LPCTSTR lpRootPathName,
                                NULL,               //__out      LPTSTR lpVolumeNameBuffer,
                                0,                  //__in       DWORD nVolumeNameSize,
                                NULL,               //__out_opt  LPDWORD lpVolumeSerialNumber,
                                NULL,               //__out_opt  LPDWORD lpMaximumComponentLength,
                                NULL,               //__out_opt  LPDWORD lpFileSystemFlags,
                                buffer,             //__out      LPTSTR lpFileSystemNameBuffer,
                                BUFFER_SIZE))       //__in       DWORD nFileSystemNameSize
    {
        assert(false); //shouldn't happen
        return false;
    }
    const Zstring fileSystem = buffer;

    //DST hack seems to be working equally well for FAT and FAT32 (in particular creation time has 10^-2 s precision as advertised)
    return fileSystem == Zstr("FAT") ||
           fileSystem == Zstr("FAT32");
}


namespace
{
FILETIME utcToLocal(const FILETIME& utcTime) //throw (std::runtime_error)
{
    //treat binary local time representation (which is invariant under DST time zone shift) as logical UTC:
    FILETIME localTime = {};
    if (!::FileTimeToLocalFileTime(
                &utcTime,    //__in   const FILETIME *lpFileTime,
                &localTime)) //__out  LPFILETIME lpLocalFileTime
    {
        const wxString errorMessage = wxString(_("Conversion error:")) + wxT(" FILETIME -> local FILETIME: ") +
                                      wxT("(") + wxULongLong(utcTime.dwHighDateTime, utcTime.dwLowDateTime).ToString() + wxT(") ") + wxT("\n\n") + ffs3::getLastErrorFormatted();
        throw std::runtime_error(std::string((errorMessage).ToUTF8()));
    }
    return localTime;
}


FILETIME localToUtc(const FILETIME& localTime) //throw (std::runtime_error)
{
    //treat binary local time representation (which is invariant under DST time zone shift) as logical UTC:
    FILETIME utcTime = {};
    if (!::LocalFileTimeToFileTime(
                &localTime, //__in   const FILETIME *lpLocalFileTime,
                &utcTime))  //__out  LPFILETIME lpFileTime
    {
        const wxString errorMessage = wxString(_("Conversion error:")) + wxT(" local FILETIME -> FILETIME: ") +
                                      wxT("(") + wxULongLong(localTime.dwHighDateTime, localTime.dwLowDateTime).ToString() + wxT(") ") + wxT("\n\n") + ffs3::getLastErrorFormatted();
        throw std::runtime_error(std::string((errorMessage).ToUTF8()));
    }
    return utcTime;
}


int cmpFileTime(const FILETIME& a, const FILETIME& b)
{
    if (a.dwHighDateTime != b.dwHighDateTime)
        return a.dwHighDateTime - b.dwHighDateTime;
    return a.dwLowDateTime - b.dwLowDateTime;
}


template <class T> //convert wxULongLong and wxLongLong to FILETIME
inline
FILETIME toFiletime(const T& number)
{
    assert_static(sizeof(DWORD) == sizeof(long));
    assert_static(sizeof(long) == 4);
    assert(number.GetHi() >= 0); //for wxLongLong

    FILETIME output = {};
    output.dwHighDateTime = number.GetHi();
    output.dwLowDateTime  = number.GetLo();
    return output;
}


inline
wxULongLong toULonglong(const FILETIME& fileTime)
{
    assert_static(sizeof(DWORD) == sizeof(long));
    assert_static(sizeof(long) == 4);

    return wxULongLong(fileTime.dwHighDateTime, fileTime.dwLowDateTime);
}


inline
wxLongLong toLonglong(const FILETIME& fileTime)
{
    assert_static(sizeof(DWORD) == sizeof(long));
    assert_static(sizeof(long) == 4);
    assert(fileTime.dwHighDateTime <= static_cast<unsigned long>(std::numeric_limits<long>::max()));

    return wxLongLong(fileTime.dwHighDateTime, fileTime.dwLowDateTime);
}


//struct FILETIME {DWORD dwLowDateTime; DWORD dwHighDateTime;};
const FILETIME FAT_MIN_TIME = {13374976, 27846544}; //1980 \ both are valid max/min FAT dates for 2 second precision
const FILETIME FAT_MAX_TIME = {14487552, 37251238}; //2107 /

//http://en.wikipedia.org/wiki/File_Allocation_Table
const size_t PRECISION_WRITE_TIME  = 20000000; //number of 100 ns per step -> 2 s
const size_t PRECISION_CREATE_TIME =   100000; //                          -> 1/100 s

/*
Number of bits of information in create time: ln_2((FAT_MAX_TIME - FAT_MIN_TIME) / PRECISION_CREATE_TIME) = 38.55534023
Number of bits of information in write time: 30.91148404
*/
//total size available to store data:
const size_t CREATE_TIME_INFO_BITS = 38;
//   I. indicator that offset in II) is present
const size_t INDICATOR_EXISTING_BITS = 1;
//  II. local<->UTC time offset
const size_t UTC_LOCAL_OFFSET_BITS = 7;
// III. indicator that offset in II) corresponds to current local write time (this could be a hash of the write time)
const size_t WRITE_TIME_HASH_BITS = CREATE_TIME_INFO_BITS - INDICATOR_EXISTING_BITS - UTC_LOCAL_OFFSET_BITS;


template <size_t precision>
FILETIME encodeRawInformation(wxULongLong rawInfo)
{
    rawInfo *= precision;
    rawInfo += toULonglong(FAT_MIN_TIME);

    assert(rawInfo <= toULonglong(FAT_MAX_TIME));
    return toFiletime(rawInfo);
}


template <size_t precision>
wxULongLong extractRawInformation(const FILETIME& createTime)
{
    assert(cmpFileTime(FAT_MIN_TIME, createTime) <= 0);
    assert(cmpFileTime(createTime, FAT_MAX_TIME) <= 0);

    //FAT create time ranges from 1980 - 2107 (2^7 years) with 1/100 seconds precision
    wxULongLong rawInfo = toULonglong(createTime);
    assert_static(sizeof(DWORD) == sizeof(long));
    assert_static(sizeof(long) == 4);

    rawInfo -= toULonglong(FAT_MIN_TIME);
    rawInfo /= precision;        //reduce precision (FILETIME has unit 10^-7 s)

    assert(cmpFileTime(encodeRawInformation<precision>(rawInfo), createTime) == 0); //must be reversible
    return rawInfo;
}


//convert write time to it's minimal representation (no restriction to FAT range "1980 - 2107")
wxULongLong extractRawWriteTime(const FILETIME& writeTime)
{
    wxULongLong rawInfo = toULonglong(writeTime);
    assert(rawInfo % PRECISION_WRITE_TIME == 0);
    rawInfo /= PRECISION_WRITE_TIME;        //reduce precision (FILETIME has unit 10^-7 s)
    return rawInfo;
}


//files with different resolution than 2 seconds are rounded up when written to FAT
FILETIME roundToFatWriteTime(const FILETIME& writeTime)
{
    wxULongLong rawData = toULonglong(writeTime);

    if (rawData % PRECISION_WRITE_TIME != 0)
        rawData += PRECISION_WRITE_TIME;

    rawData /= PRECISION_WRITE_TIME;
    rawData *= PRECISION_WRITE_TIME;
    return toFiletime(rawData);
}


//get 7-bit value representing time shift in number of quarter-hours
std::bitset<UTC_LOCAL_OFFSET_BITS> getUtcLocalShift()
{
    FILETIME utcTime = FAT_MIN_TIME;
    utcTime.dwHighDateTime += 5000000; //some arbitrary valid time

    const FILETIME localTime = utcToLocal(utcTime);

    const int timeShiftSec = ((toLonglong(localTime) - toLonglong(utcTime)) / 10000000).ToLong(); //time shift in seconds

    const int timeShiftQuarter = timeShiftSec / (60 * 15); //time shift in quarter-hours

    const int absValue = common::abs(timeShiftQuarter); //MSVC C++0x bug: std::bitset<>(unsigned long) is ambiguous

    if (std::bitset<UTC_LOCAL_OFFSET_BITS - 1>(absValue).to_ulong() != static_cast<unsigned long>(absValue) || //time shifts that big shouldn't be possible!
            timeShiftSec % (60 * 15) != 0) //all known time shift have at least 15 minute granularity!
    {
        const wxString errorMessage = wxString(_("Conversion error:")) + wxT(" Unexpected UTC <-> local time shift: ") +
                                      wxT("(") + wxLongLong(timeShiftSec).ToString() + wxT(") ") + wxT("\n\n") + ffs3::getLastErrorFormatted();
        throw std::runtime_error(std::string((errorMessage).ToUTF8()));
    }

    std::bitset<UTC_LOCAL_OFFSET_BITS> output(absValue);
    output[UTC_LOCAL_OFFSET_BITS - 1] = timeShiftQuarter < 0; //sign bit
    return output;
}


//get time-zone shift in seconds
inline
int convertUtcLocalShift(std::bitset<UTC_LOCAL_OFFSET_BITS> rawShift)
{
    const bool hasSign = rawShift[UTC_LOCAL_OFFSET_BITS - 1];
    rawShift[UTC_LOCAL_OFFSET_BITS - 1] = false;

    assert_static(UTC_LOCAL_OFFSET_BITS <= sizeof(unsigned long) * 8);
    return hasSign ?
           rawShift.to_ulong() * 15 * 60 * -1 :
           rawShift.to_ulong() * 15 * 60;
}
}


bool dst::fatHasUtcEncoded(const RawTime& rawTime) //"createTimeRaw" as retrieved by ::FindFirstFile() and ::GetFileAttributesEx(); throw (std::runtime_error)
{
    if (    cmpFileTime(rawTime.createTimeRaw, FAT_MIN_TIME) < 0 ||
            cmpFileTime(FAT_MAX_TIME, rawTime.createTimeRaw) < 0)
    {
        assert(false); //shouldn't be possible according to FAT specification
        return false;
    }

    const wxULongLong rawInfo = extractRawInformation<PRECISION_CREATE_TIME>(utcToLocal(rawTime.createTimeRaw));

    assert_static(WRITE_TIME_HASH_BITS == 30);
    return (extractRawWriteTime(utcToLocal(rawTime.writeTimeRaw)) & 0x3FFFFFFF) == (rawInfo & 0x3FFFFFFF) && //ensure write time wasn't changed externally
           rawInfo >> (CREATE_TIME_INFO_BITS - INDICATOR_EXISTING_BITS) == 1; //extended data available
}


dst::RawTime dst::fatEncodeUtcTime(const FILETIME& writeTimeRealUtc) //throw (std::runtime_error)
{
    const FILETIME fatWriteTimeUtc = roundToFatWriteTime(writeTimeRealUtc); //writeTimeRealUtc may have incompatible precision (NTFS)

    //create time lets us store 40 bit of information

    //indicator that utc time is encoded -> hopefully results in a date long way in the future; but even if this bit is accidentally set, we still have the hash!
    wxULongLong data = 1;

    const std::bitset<UTC_LOCAL_OFFSET_BITS> utcShift = getUtcLocalShift();
    data <<= UTC_LOCAL_OFFSET_BITS;
    data |= utcShift.to_ulong();

    data <<= WRITE_TIME_HASH_BITS;
    data |= extractRawWriteTime(utcToLocal(fatWriteTimeUtc)) & 0x3FFFFFFF; //trim to last 30 bit of information
    assert_static(WRITE_TIME_HASH_BITS == 30);

    const FILETIME encodedData = localToUtc(encodeRawInformation<PRECISION_CREATE_TIME>(data)); //localToUtc: make sure data is physically saved as FAT local time
    assert(cmpFileTime(FAT_MIN_TIME, encodedData) <= 0);
    assert(cmpFileTime(encodedData, FAT_MAX_TIME) <= 0);

    return RawTime(encodedData, fatWriteTimeUtc); //keep compatible with other applications, at least until DST shift actually occurs
}


FILETIME dst::fatDecodeUtcTime(const RawTime& rawTime) //return real UTC time; throw (std::runtime_error)
{
    if (!fatHasUtcEncoded(rawTime))
        return rawTime.writeTimeRaw;

    const wxULongLong rawInfo = extractRawInformation<PRECISION_CREATE_TIME>(utcToLocal(rawTime.createTimeRaw));

    const std::bitset<UTC_LOCAL_OFFSET_BITS> rawShift(static_cast<int>(((rawInfo >> WRITE_TIME_HASH_BITS) & 0x7F).ToULong())); //static_cast<int>: a shame MSC...
    assert_static(UTC_LOCAL_OFFSET_BITS == 7);

    const int timeShiftSec = convertUtcLocalShift(rawShift);
    const FILETIME writeTimeLocal = utcToLocal(rawTime.writeTimeRaw);

    const wxLongLong realUTC = toLonglong(writeTimeLocal) - wxLongLong(timeShiftSec) * 10000000;
    return toFiletime(realUTC);
}
