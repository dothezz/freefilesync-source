#include "dst_hack.h"
#include <bitset>
#include "basic_math.h"
#include "long_path_prefix.h"
#include "utf.h"
#include "assert_static.h"
#include "int64.h"
#include "file_error.h"
#include "dll.h"

using namespace zen;


namespace
{
//fast ::GetVolumePathName() clone: let's hope it's not too simple (doesn't honor mount points)
Zstring getVolumeName(const Zstring& filepath)
{
    //this call is expensive: ~1.5 ms!
    //    if (!::GetVolumePathName(filepath.c_str(), //__in   LPCTSTR lpszFileName,
    //                             fsName,           //__out  LPTSTR lpszVolumePathName,
    //                             BUFFER_SIZE))     //__in   DWORD cchBufferLength
    // ...
    //    Zstring volumePath = appendSeparator(fsName);

    const Zstring nameFmt = appendSeparator(removeLongPathPrefix(filepath)); //throw()

    if (startsWith(nameFmt, Zstr("\\\\"))) //UNC path: "\\ComputerName\SharedFolder\"
    {
        size_t nameSize = nameFmt.size();
        const size_t posFirstSlash = nameFmt.find(Zstr("\\"), 2);
        if (posFirstSlash != Zstring::npos)
        {
            nameSize = posFirstSlash + 1;
            const size_t posSecondSlash = nameFmt.find(Zstr("\\"), posFirstSlash + 1);
            if (posSecondSlash != Zstring::npos)
                nameSize = posSecondSlash + 1;
        }
        return Zstring(nameFmt.c_str(), nameSize); //include trailing backslash!
    }
    else //local path: "C:\Folder\"
    {
        const size_t pos = nameFmt.find(Zstr(":\\"));
        if (pos == 1) //expect single letter volume
            return Zstring(nameFmt.c_str(), 3);
    }

    return Zstring();
}
}


bool dst::isFatDrive(const Zstring& filepath) //throw()
{
    const Zstring volumePath = getVolumeName(filepath);
    if (volumePath.empty())
        return false;

    const DWORD bufferSize = MAX_PATH + 1;
	wchar_t fsName[bufferSize] = {};

    //suprisingly fast: ca. 0.03 ms per call!
    if (!::GetVolumeInformation(appendSeparator(volumePath).c_str(), //__in_opt   LPCTSTR lpRootPathName,
                                nullptr,     //__out      LPTSTR lpVolumeNameBuffer,
                                0,           //__in       DWORD nVolumeNameSize,
                                nullptr,     //__out_opt  LPDWORD lpVolumeSerialNumber,
                                nullptr,     //__out_opt  LPDWORD lpMaximumComponentLength,
                                nullptr,     //__out_opt  LPDWORD lpFileSystemFlags,
                                fsName,      //__out      LPTSTR lpFileSystemNameBuffer,
                                bufferSize)) //__in       DWORD nFileSystemNameSize
    {
        assert(false); //shouldn't happen
        return false;
    }
    //DST hack seems to be working equally well for FAT and FAT32 (in particular creation time has 10^-2 s precision as advertised)
    return fsName == Zstring(L"FAT") ||
           fsName == Zstring(L"FAT32");
}


/*
bool dst::isFatDrive(HANDLE hFile) //throw()
{
Requires Windows Vista!
    //dynamically load windows API function
    typedef BOOL (WINAPI* GetVolumeInformationByHandleWFunc)(HANDLE  hFile,
                                                             LPWSTR  lpVolumeNameBuffer,
                                                             DWORD   nVolumeNameSize,
                                                             LPDWORD lpVolumeSerialNumber,
                                                             LPDWORD lpMaximumComponentLength,
                                                             LPDWORD lpFileSystemFlags,
                                                             LPWSTR  lpFileSystemNameBuffer,
                                                             DWORD   nFileSystemNameSize);

    const SysDllFun<GetVolumeInformationByHandleWFunc> getVolumeInformationByHandle(L"kernel32.dll", "GetVolumeInformationByHandleW");
    if (!getVolumeInformationByHandle)
    {
        assert(false);
        return false;
    }

    const size_t BUFFER_SIZE = MAX_PATH + 1;
    wchar_t fsName[BUFFER_SIZE];

    if (!getVolumeInformationByHandle(hFile,        //__in       HANDLE hFile,
                                      nullptr,      //__out      LPTSTR lpVolumeNameBuffer,
                                      0,            //__in       DWORD nVolumeNameSize,
                                      nullptr,      //__out_opt  LPDWORD lpVolumeSerialNumber,
                                      nullptr,      //__out_opt  LPDWORD lpMaximumComponentLength,
                                      nullptr,      //__out_opt  LPDWORD lpFileSystemFlags,
                                      fsName,       //__out      LPTSTR lpFileSystemNameBuffer,
                                      BUFFER_SIZE)) //__in       DWORD nFileSystemNameSize
    {
        assert(false); //shouldn't happen
        return false;
    }
    //DST hack seems to be working equally well for FAT and FAT32 (in particular creation time has 10^-2 s precision as advertised)
    return fsName == Zstring(L"FAT") ||
           fsName == Zstring(L"FAT32");
}
*/


namespace
{
//convert std::uint64_t and std::int64_t to FILETIME
inline
FILETIME toFiletime(std::uint64_t number)
{
       ULARGE_INTEGER cvt = {};
        cvt.QuadPart = number;

    const FILETIME output = { cvt.LowPart, cvt.HighPart };
    return output;
}

inline
FILETIME toFiletime(std::int64_t number)
{
	return toFiletime(static_cast<std::uint64_t>(number));
}

inline
std::uint64_t toUInt64(const FILETIME& fileTime)
{
    return get64BitUInt(fileTime.dwLowDateTime, fileTime.dwHighDateTime);
}


inline
std::int64_t toInt64(const FILETIME& fileTime)
{
    return get64BitUInt(fileTime.dwLowDateTime, fileTime.dwHighDateTime); //convert unsigned to signed in return
}


inline
FILETIME utcToLocal(const FILETIME& utcTime) //throw std::runtime_error
{
    //treat binary local time representation (which is invariant under DST time zone shift) as logical UTC:
    FILETIME localTime = {};
    if (!::FileTimeToLocalFileTime(
            &utcTime,    //__in   const FILETIME *lpFileTime,
            &localTime)) //__out  LPFILETIME lpLocalFileTime
    {
        const DWORD lastError = ::GetLastError(); //copy before directly or indirectly making other system calls!
        throw std::runtime_error(utfCvrtTo<std::string>(_("Conversion error:") + L" FILETIME -> local FILETIME: " + L"(" +
                                                        L"High: " + numberTo<std::wstring>(utcTime.dwHighDateTime) + L" " +
                                                        L"Low: "  + numberTo<std::wstring>(utcTime.dwLowDateTime) + L") " + L"\n\n" + formatSystemError(L"FileTimeToLocalFileTime", lastError)));
    }
    return localTime;
}


inline
FILETIME localToUtc(const FILETIME& localTime) //throw std::runtime_error
{
    //treat binary local time representation (which is invariant under DST time zone shift) as logical UTC:
    FILETIME utcTime = {};
    if (!::LocalFileTimeToFileTime(
            &localTime, //__in   const FILETIME *lpLocalFileTime,
            &utcTime))  //__out  LPFILETIME lpFileTime
    {
        const DWORD lastError = ::GetLastError(); //copy before directly or indirectly making other system calls!
        throw std::runtime_error(utfCvrtTo<std::string>(_("Conversion error:") + L" local FILETIME -> FILETIME: " + L"(" +
                                                        L"High: " + numberTo<std::wstring>(localTime.dwHighDateTime) + L" " +
                                                        L"Low: "  + numberTo<std::wstring>(localTime.dwLowDateTime) + L") " + L"\n\n" + formatSystemError(L"LocalFileTimeToFileTime", lastError)));
    }
    return utcTime;
}


//struct FILETIME {DWORD dwLowDateTime; DWORD dwHighDateTime;};
const FILETIME FAT_MIN_TIME = { 13374976, 27846544 }; //1980 \ both are valid max/min FAT dates for 2 second precision
const FILETIME FAT_MAX_TIME = { 14487552, 37251238 }; //2107 /

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


template <size_t precision> inline
FILETIME encodeRawInformation(std::uint64_t rawInfo)
{
    rawInfo *= precision;
    rawInfo += toUInt64(FAT_MIN_TIME);

    assert(rawInfo <= toUInt64(FAT_MAX_TIME));
    return toFiletime(rawInfo);
}


template <size_t precision> inline
std::uint64_t extractRawInformation(const FILETIME& createTime)
{
    assert(toUInt64(FAT_MIN_TIME) <= toUInt64(createTime));
    assert(toUInt64(createTime) <= toUInt64(FAT_MAX_TIME));

    //FAT create time ranges from 1980 - 2107 (2^7 years) with 1/100 seconds precision
    std::uint64_t rawInfo = toUInt64(createTime);

    rawInfo -= toUInt64(FAT_MIN_TIME);
    rawInfo /= precision;        //reduce precision (FILETIME has unit 10^-7 s)

    assert(toUInt64(encodeRawInformation<precision>(rawInfo)) == toUInt64(createTime)); //must be reversible
    return rawInfo;
}


//convert write time to it's minimal representation (no restriction to FAT range "1980 - 2107")
inline
std::uint64_t extractRawWriteTime(const FILETIME& writeTime)
{
    std::uint64_t rawInfo = toUInt64(writeTime);
    assert(rawInfo % PRECISION_WRITE_TIME == 0U);
    rawInfo /= PRECISION_WRITE_TIME;        //reduce precision (FILETIME has unit 10^-7 s)
    return rawInfo;
}


//files with different resolution than 2 seconds are rounded up when written to FAT
inline
FILETIME roundToFatWriteTime(const FILETIME& writeTime)
{
    std::uint64_t rawData = toUInt64(writeTime);

    if (rawData % PRECISION_WRITE_TIME != 0U)
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

    const int timeShiftSec = static_cast<int>((toInt64(localTime) - toInt64(utcTime)) / 10000000); //time shift in seconds

    const int timeShiftQuarter = timeShiftSec / (60 * 15); //time shift in quarter-hours

    const int absValue = numeric::abs(timeShiftQuarter); //MSVC C++0x bug: std::bitset<>(unsigned long) is ambiguous

    if (std::bitset < UTC_LOCAL_OFFSET_BITS - 1 > (absValue).to_ulong() != static_cast<unsigned long>(absValue) || //time shifts that big shouldn't be possible!
        timeShiftSec % (60 * 15) != 0) //all known time shift have at least 15 minute granularity!
    {
        const std::wstring errorMsg = _("Conversion error:") + L" Unexpected UTC <-> local time shift: " + L"(" + numberTo<std::wstring>(timeShiftSec) + L")";
        throw std::runtime_error(utfCvrtTo<std::string>(errorMsg));
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
           static_cast<int>(rawShift.to_ulong()) * 15 * 60 * -1 :
           static_cast<int>(rawShift.to_ulong()) * 15 * 60;
}
}


bool dst::fatHasUtcEncoded(const RawTime& rawTime) //"createTimeRaw" as retrieved by ::FindFirstFile() and ::GetFileAttributesEx(); throw std::runtime_error
{
    if (toUInt64(rawTime.createTimeRaw) < toUInt64(FAT_MIN_TIME) ||
        toUInt64(FAT_MAX_TIME) < toUInt64(rawTime.createTimeRaw))
    {
        assert(false); //shouldn't be possible according to FAT specification
        return false;
    }

    const std::uint64_t rawInfo = extractRawInformation<PRECISION_CREATE_TIME>(utcToLocal(rawTime.createTimeRaw));

    assert_static(WRITE_TIME_HASH_BITS == 30);
    return (extractRawWriteTime(utcToLocal(rawTime.writeTimeRaw)) & 0x3FFFFFFFU) == (rawInfo & 0x3FFFFFFFU) && //ensure write time wasn't changed externally
           rawInfo >> (CREATE_TIME_INFO_BITS - INDICATOR_EXISTING_BITS) == 1U; //extended data available
}


dst::RawTime dst::fatEncodeUtcTime(const FILETIME& writeTimeRealUtc) //throw std::runtime_error
{
    const FILETIME fatWriteTimeUtc = roundToFatWriteTime(writeTimeRealUtc); //writeTimeRealUtc may have incompatible precision (NTFS)

    //create time lets us store 40 bit of information

    //indicator that utc time is encoded -> hopefully results in a date long way in the future; but even if this bit is accidentally set, we still have the hash!
    std::uint64_t data = 1U;

    const std::bitset<UTC_LOCAL_OFFSET_BITS> utcShift = getUtcLocalShift();
    data <<= UTC_LOCAL_OFFSET_BITS;
    data |= utcShift.to_ulong();

    data <<= WRITE_TIME_HASH_BITS;
    data |= extractRawWriteTime(utcToLocal(fatWriteTimeUtc)) & 0x3FFFFFFFU; //trim to last 30 bit of information
    assert_static(WRITE_TIME_HASH_BITS == 30);

    const FILETIME encodedData = localToUtc(encodeRawInformation<PRECISION_CREATE_TIME>(data)); //localToUtc: make sure data is physically saved as FAT local time
    assert(toUInt64(FAT_MIN_TIME) <= toUInt64(encodedData));
    assert(toUInt64(encodedData) <= toUInt64(FAT_MAX_TIME));

    return RawTime(encodedData, fatWriteTimeUtc); //keep compatible with other applications, at least until DST shift actually occurs
}


FILETIME dst::fatDecodeUtcTime(const RawTime& rawTime) //return real UTC time; throw (std::runtime_error)
{
    if (!fatHasUtcEncoded(rawTime))
        return rawTime.writeTimeRaw;

    const std::uint64_t rawInfo = extractRawInformation<PRECISION_CREATE_TIME>(utcToLocal(rawTime.createTimeRaw));

    const std::bitset<UTC_LOCAL_OFFSET_BITS> rawShift(static_cast<int>((rawInfo >> WRITE_TIME_HASH_BITS) & 0x7FU)); //static_cast<int>: a shame MSC... "unsigned long long" should be supported instead!
    assert_static(UTC_LOCAL_OFFSET_BITS == 7);

    const std::int64_t timeShiftSec = convertUtcLocalShift(rawShift);
    const FILETIME writeTimeLocal = utcToLocal(rawTime.writeTimeRaw);

    const std::int64_t realUTC = toInt64(writeTimeLocal) - timeShiftSec * 10000000;
    return toFiletime(realUTC);
}
