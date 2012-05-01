// **************************************************************************
// * This file is part of the zenXML project. It is distributed under the   *
// * Boost Software License, Version 1.0. See accompanying file             *
// * LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt.       *
// * Copyright (C) ZenJu (zhnmju123 AT gmx DOT de) - All Rights Reserved    *
// **************************************************************************

#ifndef ZEN_TIME_HEADER_845709281432434
#define ZEN_TIME_HEADER_845709281432434

#include <ctime>
#include "string_tools.h"


namespace zen
{
struct TimeComp //replaces "struct std::tm" and SYSTEMTIME
{
    TimeComp() : year(0), month(0), day(0), hour(0), minute(0), second(0) {}

    int year;   // -
    int month;  //1-12
    int day;    //1-31
    int hour;   //0-23
    int minute; //0-59
    int second; //0-61
};

TimeComp localTime   (time_t utc = std::time(nullptr)); //convert time_t (UTC) to local time components
time_t   localToTimeT(const TimeComp& comp);            //convert local time components to time_t (UTC), returns -1 on error

//----------------------------------------------------------------------------------------------------------------------------------

/*
format (current) date and time; example:
        formatTime<std::wstring>(L"%Y*%m*%d");     -> "2011*10*29"
        formatTime<std::wstring>(FORMAT_DATE);     -> "2011-10-29"
        formatTime<std::wstring>(FORMAT_TIME);     -> "17:55:34"
*/
template <class String, class String2>
String formatTime(const String2& format, const TimeComp& comp = localTime()); //format as specified by "std::strftime", returns empty string on failure

//the "format" parameter of formatTime() is partially specialized with the following type tags:
const struct FormatDateTag     {} FORMAT_DATE      = {}; //%x - locale dependent date representation: e.g. 08/23/01
const struct FormatTimeTag     {} FORMAT_TIME      = {}; //%X - locale dependent time representation: e.g. 14:55:02
const struct FormatDateTimeTag {} FORMAT_DATE_TIME = {}; //%c - locale dependent date and time:       e.g. Thu Aug 23 14:55:02 2001

const struct FormatIsoDateTag     {} FORMAT_ISO_DATE      = {}; //%Y-%m-%d          - e.g. 2001-08-23
const struct FormatIsoTimeTag     {} FORMAT_ISO_TIME      = {}; //%H:%M:%S          - e.g. 14:55:02
const struct FormatIsoDateTimeTag {} FORMAT_ISO_DATE_TIME = {}; //%Y-%m-%d %H:%M:%S - e.g. 2001-08-23 14:55:02

//----------------------------------------------------------------------------------------------------------------------------------

template <class String>
bool parseTime(const String& format, const String& str, TimeComp& comp); //similar to ::strptime(), return true on success

//----------------------------------------------------------------------------------------------------------------------------------

























//############################ implementation ##############################
namespace implementation
{
inline
struct std::tm toClibTimeComponents(const TimeComp& comp)
{
    assert(1 <= comp.month  && comp.month  <= 12 &&
           1 <= comp.day    && comp.day    <= 31 &&
           0 <= comp.hour   && comp.hour   <= 23 &&
           0 <= comp.minute && comp.minute <= 59 &&
           0 <= comp.second && comp.second <= 61);

    struct std::tm ctc = {};
    ctc.tm_year  = comp.year - 1900; //years since 1900
    ctc.tm_mon   = comp.month - 1;   //0-11
    ctc.tm_mday  = comp.day;         //1-31
    ctc.tm_hour  = comp.hour;        //0-23
    ctc.tm_min   = comp.minute;      //0-59
    ctc.tm_sec   = comp.second;      //0-61
    ctc.tm_isdst = -1;               //> 0 if DST is active, == 0 if DST is not active, < 0 if the information is not available

    return ctc;
}

inline
TimeComp toZenTimeComponents(const struct std::tm& ctc)
{
    TimeComp comp;
    comp.year   = ctc.tm_year + 1900;
    comp.month  = ctc.tm_mon + 1;
    comp.day    = ctc.tm_mday;
    comp.hour   = ctc.tm_hour;
    comp.minute = ctc.tm_min;
    comp.second = ctc.tm_sec;
    return comp;
}


template <class T>
struct GetFormat; //get default time formats as char* or wchar_t*

template <>
struct GetFormat<FormatDateTag> //%x - locale dependent date representation: e.g. 08/23/01
{
    const char*    format(char)    const { return  "%x"; }
    const wchar_t* format(wchar_t) const { return L"%x"; }
};

template <>
struct GetFormat<FormatTimeTag> //%X - locale dependent time representation: e.g. 14:55:02
{
    const char*    format(char)    const { return  "%X"; }
    const wchar_t* format(wchar_t) const { return L"%X"; }
};

template <>
struct GetFormat<FormatDateTimeTag> //%c - locale dependent date and time:       e.g. Thu Aug 23 14:55:02 2001
{
    const char*    format(char)    const { return  "%c"; }
    const wchar_t* format(wchar_t) const { return L"%c"; }
};

template <>
struct GetFormat<FormatIsoDateTag> //%Y-%m-%d - e.g. 2001-08-23
{
    const char*    format(char)    const { return  "%Y-%m-%d"; }
    const wchar_t* format(wchar_t) const { return L"%Y-%m-%d"; }
};

template <>
struct GetFormat<FormatIsoTimeTag> //%H:%M:%S - e.g. 14:55:02
{
    const char*    format(char)    const { return  "%H:%M:%S"; }
    const wchar_t* format(wchar_t) const { return L"%H:%M:%S"; }
};

template <>
struct GetFormat<FormatIsoDateTimeTag> //%Y-%m-%d %H:%M:%S - e.g. 2001-08-23 14:55:02
{
    const char*    format(char)    const { return  "%Y-%m-%d %H:%M:%S"; }
    const wchar_t* format(wchar_t) const { return L"%Y-%m-%d %H:%M:%S"; }
};


inline
size_t strftimeWrap(char* buffer, size_t bufferSize, const char* format, const struct std::tm* timeptr)
{
    return std::strftime(buffer, bufferSize, format, timeptr);
}


inline
size_t strftimeWrap(wchar_t* buffer, size_t bufferSize, const wchar_t* format, const struct std::tm* timeptr)
{
    return std::wcsftime(buffer, bufferSize, format, timeptr);
}


struct UserDefinedFormatTag {};
struct PredefinedFormatTag  {};

template <class String, class String2> inline
String formatTime(const String2& format, const TimeComp& comp, UserDefinedFormatTag) //format as specified by "std::strftime", returns empty string on failure
{
    typedef typename GetCharType<String>::Type CharType;

    struct std::tm ctc = toClibTimeComponents(comp);
    std::mktime(&ctc); // unfortunately std::strftime() needs all elements of "struct tm" filled, e.g. tm_wday, tm_yday
    //note: although std::mktime() explicitly expects "local time", calculating weekday and day of year *should* be time-zone and DST independent

    CharType buffer[256];
    const size_t charsWritten = strftimeWrap(buffer, 256, strBegin(format), &ctc);
    return String(buffer, charsWritten);
}

template <class String, class FormatType> inline
String formatTime(FormatType, const TimeComp& comp, PredefinedFormatTag)
{
    typedef typename GetCharType<String>::Type CharType;
    return formatTime<String>(GetFormat<FormatType>().format(CharType()), comp, UserDefinedFormatTag());
}
}


inline
TimeComp localTime(time_t utc)
{
#ifdef _MSC_VER
    struct tm lt = {};
    /*errno_t rv = */
    ::localtime_s(&lt, &utc); //more secure?
    return implementation::toZenTimeComponents(lt);
#else
    return implementation::toZenTimeComponents(*std::localtime(&utc));
#endif
}


inline
time_t localToTimeT(const TimeComp& comp) //returns -1 on error
{
    struct std::tm ctc = implementation::toClibTimeComponents(comp);
    return std::mktime(&ctc);
}


template <class String, class String2> inline
String formatTime(const String2& format, const TimeComp& comp)
{
    typedef typename SelectIf<
    IsSameType<String2, FormatDateTag       >::value ||
    IsSameType<String2, FormatTimeTag       >::value ||
    IsSameType<String2, FormatDateTimeTag   >::value ||
    IsSameType<String2, FormatIsoDateTag    >::value ||
    IsSameType<String2, FormatIsoTimeTag    >::value ||
    IsSameType<String2, FormatIsoDateTimeTag>::value, implementation::PredefinedFormatTag, implementation::UserDefinedFormatTag>::Type FormatTag;

    return implementation::formatTime<String>(format, comp, FormatTag());
}


template <class String>
bool parseTime(const String& format, const String& str, TimeComp& comp) //return true on success
{
    typedef typename GetCharType<String>::Type CharType;

    const CharType*       iterFmt = strBegin(format);
    const CharType* const fmtLast = iterFmt + strLength(format);

    const CharType*       iterStr = strBegin(str);
    const CharType* const strLast = iterStr + strLength(str);

    auto extractNumber = [&](int& result, size_t digitCount) -> bool
    {
        if (strLast - iterStr < digitCount)
            return false;

        if (std::any_of(iterStr, iterStr + digitCount, [](CharType c) { return !isDigit(c); }))
        return false;

        result = zen::stringTo<int>(StringProxy<CharType>(iterStr, digitCount));
        iterStr += digitCount;
        return true;
    };

    for (; iterFmt != fmtLast; ++iterFmt)
    {
        const CharType fmt = *iterFmt;

        if (fmt == '%')
        {
            ++iterFmt;
            if (iterFmt == fmtLast)
                return false;

            switch (*iterFmt)
            {
                case 'Y':
                    if (!extractNumber(comp.year, 4))
                        return false;
                    break;
                case 'm':
                    if (!extractNumber(comp.month, 2))
                        return false;
                    break;
                case 'd':
                    if (!extractNumber(comp.day, 2))
                        return false;
                    break;
                case 'H':
                    if (!extractNumber(comp.hour, 2))
                        return false;
                    break;
                case 'M':
                    if (!extractNumber(comp.minute, 2))
                        return false;
                    break;
                case 'S':
                    if (!extractNumber(comp.second, 2))
                        return false;
                    break;
                default:
                    return false;
            }
        }
        else if (isWhiteSpace(fmt)) //single whitespace in format => skip 0..n whitespace chars
        {
            while (iterStr != strLast && isWhiteSpace(*iterStr))
                ++iterStr;
        }
        else
        {
            if (iterStr == strLast || *iterStr != fmt)
                return false;
            ++iterStr;
        }
    }

    return iterStr == strLast;
}
}

#endif //ZEN_TIME_HEADER_845709281432434