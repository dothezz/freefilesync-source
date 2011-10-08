// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) 2008-2011 ZenJu (zhnmju123 AT gmx.de)                    *
// **************************************************************************

#include "format_unit.h"
#include <zen/basic_math.h>
#include <zen/i18n.h>
#include <cwchar>  //swprintf
#include <ctime>
#include <cstdio>

#ifdef FFS_WIN
#include <zen/win.h> //includes "windows.h"
#include <zen/win_ver.h>
#endif


namespace
{
inline
size_t getDigitCount(size_t number)
{
    return number == 0 ? 1 : static_cast<size_t>(std::log10(static_cast<double>(number))) + 1;
} //count number of digits
}


std::wstring zen::filesizeToShortString(UInt64 size)
{
    if (to<Int64>(size) < 0) return _("Error");

    if (size <= 999U)
        return replaceCpy(_P("1 Byte", "%x Bytes", to<int>(size)),
                          L"%x",
                          toString<std::wstring>(size));
    else
    {
        double filesize = to<double>(size);

        filesize /= 1024;
        std::wstring output = _("%x KB");
        if (filesize > 999)
        {
            filesize /= 1024;
            output = _("%x MB");
            if (filesize > 999)
            {
                filesize /= 1024;
                output = _("%x GB");
                if (filesize > 999)
                {
                    filesize /= 1024;
                    output = _("%x TB");
                    if (filesize > 999)
                    {
                        filesize /= 1024;
                        output = _("%x PB");
                    }
                }
            }
        }
        //print just three significant digits: 0,01 | 0,11 | 1,11 | 11,1 | 111
        const size_t leadDigitCount = getDigitCount(static_cast<size_t>(filesize)); //number of digits before decimal point
        if (leadDigitCount == 0 || leadDigitCount > 3)
            return _("Error");

        wchar_t buffer[50];
#ifdef __MINGW32__
        int charsWritten = ::snwprintf(buffer, 50, L"%.*f", static_cast<int>(3 - leadDigitCount), filesize); //MinGW does not comply to the C standard here
#else
        int charsWritten = std::swprintf(buffer, 50, L"%.*f", static_cast<int>(3 - leadDigitCount), filesize);
#endif
        return charsWritten > 0 ? replaceCpy(output, L"%x", std::wstring(buffer, charsWritten)) : _("Error");
    }
}


enum UnitRemTime
{
    URT_SEC,
    URT_MIN,
    URT_HOUR,
    URT_DAY
};

std::wstring zen::remainingTimeToShortString(double timeInSec)
{
    double remainingTime = timeInSec;

    //determine preferred unit
    UnitRemTime unit = URT_SEC;
    if (remainingTime > 59)
    {
        unit = URT_MIN;
        remainingTime /= 60;
        if (remainingTime > 59)
        {
            unit = URT_HOUR;
            remainingTime /= 60;
            if (remainingTime > 23)
            {
                unit = URT_DAY;
                remainingTime /= 24;
            }
        }
    }

    int formattedTime = numeric::round(remainingTime);

    //reduce precision to 5 seconds
    if (unit == URT_SEC)
        formattedTime = static_cast<int>(std::ceil(formattedTime / 5.0) * 5);

    //generate output message
    std::wstring output;
    switch (unit)
    {
        case URT_SEC:
            output = _P("1 sec", "%x sec", formattedTime);
            break;
        case URT_MIN:
            output = _P("1 min", "%x min", formattedTime);
            break;
        case URT_HOUR:
            output = _P("1 hour", "%x hours", formattedTime);
            break;
        case URT_DAY:
            output = _P("1 day", "%x days", formattedTime);
            break;
    }
    return replaceCpy(output, L"%x", zen::toString<std::wstring>(formattedTime));
}


std::wstring zen::percentageToShortString(double fraction)
{
    return replaceCpy(_("%x%"), L"%x", printNumber<std::wstring>(L"%3.2f", fraction * 100.0), false);
}


std::wstring zen::ffs_Impl::includeNumberSeparator(const std::wstring& number)
{
    std::wstring output(number);
    for (size_t i = output.size(); i > 3; i -= 3)
        output.insert(i - 3, zen::getThousandsSeparator());

    return output;
}

/*
#include <wx/scrolwin.h>

void zen::scrollToBottom(wxScrolledWindow* scrWindow)
{
    int height = 0;
    scrWindow->GetClientSize(NULL, &height);

    int pixelPerLine = 0;
    scrWindow->GetScrollPixelsPerUnit(NULL, &pixelPerLine);

    if (height > 0 && pixelPerLine > 0)
    {
        const int scrollLinesTotal    = scrWindow->GetScrollLines(wxVERTICAL);
        const int scrollLinesOnScreen = height / pixelPerLine;
        const int scrollPosBottom     = scrollLinesTotal - scrollLinesOnScreen;

        if (0 <= scrollPosBottom)
            scrWindow->Scroll(0, scrollPosBottom);
    }
}
*/

#ifdef FFS_WIN
namespace
{
const bool useNewLocalTimeCalculation = zen::vistaOrLater();
}
#endif


std::wstring zen::utcToLocalTimeString(Int64 utcTime)
{
#ifdef FFS_WIN
    FILETIME lastWriteTimeUtc = tofiletime(utcTime); //convert ansi C time to FILETIME

    SYSTEMTIME systemTimeLocal = {};

    if (useNewLocalTimeCalculation) //use DST setting from source date (like in Windows 7, see http://msdn.microsoft.com/en-us/library/ms724277(VS.85).aspx)
    {
        SYSTEMTIME systemTimeUtc = {};
        if (!::FileTimeToSystemTime(&lastWriteTimeUtc, //__in   const FILETIME *lpFileTime,
                                    &systemTimeUtc))   //__out  LPSYSTEMTIME lpSystemTime
            return _("Error");

        if (!::SystemTimeToTzSpecificLocalTime(NULL,              //__in_opt  LPTIME_ZONE_INFORMATION lpTimeZone,
                                               &systemTimeUtc,    //__in      LPSYSTEMTIME lpUniversalTime,
                                               &systemTimeLocal)) //__out     LPSYSTEMTIME lpLocalTime
            return _("Error");
    }
    else //use DST setting (like in Windows 2000 and XP)
    {
        FILETIME fileTimeLocal = {};
        if (!::FileTimeToLocalFileTime(&lastWriteTimeUtc,  //pointer to UTC file time to convert
                                       &fileTimeLocal))    //pointer to converted file time
            return _("Error");

        if (!::FileTimeToSystemTime(&fileTimeLocal,  //pointer to file time to convert
                                    &systemTimeLocal)) //pointer to structure to receive system time
            return _("Error");
    }

    struct tm loc = {};
    loc.tm_year = systemTimeLocal.wYear - 1900;
    loc.tm_mon  = systemTimeLocal.wMonth - 1;
    loc.tm_mday = systemTimeLocal.wDay;
    loc.tm_hour = systemTimeLocal.wHour;
    loc.tm_min  = systemTimeLocal.wMinute;
    loc.tm_sec  = systemTimeLocal.wSecond;
    const struct tm* timePtr = &loc;

#elif defined FFS_LINUX
    const time_t fileTime = to<time_t>(utcTime);
    const struct tm* timePtr = ::localtime(&fileTime); //convert to local time
#endif

    wchar_t buffer[1000];
    size_t charsWritten = std::wcsftime(buffer, 1000, L"%x  %X", timePtr);

    return std::wstring(buffer, charsWritten);
}
