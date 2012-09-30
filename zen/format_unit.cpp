// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) Zenju (zenju AT gmx DOT de) - All Rights Reserved        *
// **************************************************************************

#include "format_unit.h"
#include <zen/basic_math.h>
#include <zen/i18n.h>
#include <zen/time.h>
#include <cwchar>  //swprintf
#include <ctime>
#include <cstdio>

#ifdef FFS_WIN
#include <zen/win.h> //includes "windows.h"
#include <zen/win_ver.h>

#elif defined FFS_LINUX
#include <clocale>    //thousands separator
#include <zen/utf.h> //
#endif

using namespace zen;


std::wstring zen::filesizeToShortString(Int64 size)
{
    //if (size < 0) return _("Error"); -> really? there's at least one exceptional case: a failed rename operation falls-back to copy + delete, reducing "bytes transferred" to potentially < 0!

    if (numeric::abs(size) <= 999)
        return replaceCpy(_P("1 Byte", "%x Bytes", to<int>(size)),
                          L"%x",
                          numberTo<std::wstring>(size));
    else
    {
        double filesize = to<double>(size);

        filesize /= 1024;
        std::wstring output = _("%x KB");
        if (numeric::abs(filesize) > 999)
        {
            filesize /= 1024;
            output = _("%x MB");
            if (numeric::abs(filesize) > 999)
            {
                filesize /= 1024;
                output = _("%x GB");
                if (numeric::abs(filesize) > 999)
                {
                    filesize /= 1024;
                    output = _("%x TB");
                    if (numeric::abs(filesize) > 999)
                    {
                        filesize /= 1024;
                        output = _("%x PB");
                    }
                }
            }
        }
        //print just three significant digits: 0,01 | 0,11 | 1,11 | 11,1 | 111
        const size_t fullunits = static_cast<size_t>(numeric::abs(filesize));
        const int precisionDigits = fullunits < 10 ? 2 : fullunits < 100 ? 1 : 0; //sprintf requires "int"

        wchar_t buffer[50];
#ifdef __MINGW32__
        int charsWritten =   ::snwprintf(buffer, 50, L"%.*f", precisionDigits, filesize); //MinGW does not comply to the C standard here
#else
        int charsWritten = std::swprintf(buffer, 50, L"%.*f", precisionDigits, filesize);
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
    return replaceCpy(output, L"%x", zen::numberTo<std::wstring>(formattedTime));
}


std::wstring zen::fractionToShortString(double fraction)
{
    //return replaceCpy(_("%x%"), L"%x", printNumber<std::wstring>(L"%3.2f", fraction * 100.0), false);
    return printNumber<std::wstring>(L"%3.2f", fraction * 100.0) + L'%'; //no need to internationalize fraction!?
}


#ifdef FFS_WIN
namespace
{
bool getUserSetting(LCTYPE lt, UINT& setting)
{
    return ::GetLocaleInfo(LOCALE_USER_DEFAULT,                  //__in   LCID Locale,
                           lt | LOCALE_RETURN_NUMBER,            //__in   LCTYPE LCType,
                           reinterpret_cast<LPTSTR>(&setting),   //__out  LPTSTR lpLCData,
                           sizeof(setting) / sizeof(TCHAR)) > 0; //__in   int cchData
}


bool getUserSetting(LCTYPE lt, std::wstring& setting)
{
    int bufferSize = ::GetLocaleInfo(LOCALE_USER_DEFAULT, lt, nullptr, 0);
    if (bufferSize > 0)
    {
        std::vector<wchar_t> buffer(bufferSize);
        if (::GetLocaleInfo(LOCALE_USER_DEFAULT, //__in   LCID Locale,
                            lt,                  //__in   LCTYPE LCType,
                            &buffer[0],          //__out  LPTSTR lpLCData,
                            bufferSize) > 0)     //__in   int cchData
        {
            setting = &buffer[0]; //GetLocaleInfo() returns char count *including* 0-termination!
            return true;
        }
    }
    return false;
}

class IntegerFormat
{
public:
    static const NUMBERFMT& get() { return getInst().fmt; }
    static bool isValid() { return getInst().valid_; }

private:
    static const IntegerFormat& getInst()
    {
        static IntegerFormat inst; //not threadsafe in MSVC until C++11, but not required right now
        return inst;
    }

    IntegerFormat() : fmt(), valid_(false)
    {
        //all we want is default NUMBERFMT, but set NumDigits to 0
        fmt.NumDigits = 0;

        //what a disgrace:
        std::wstring grouping;
        if (getUserSetting(LOCALE_ILZERO,     fmt.LeadingZero) &&
            getUserSetting(LOCALE_SGROUPING,  grouping)        &&
            getUserSetting(LOCALE_SDECIMAL,   decimalSep)      &&
            getUserSetting(LOCALE_STHOUSAND,  thousandSep)     &&
            getUserSetting(LOCALE_INEGNUMBER, fmt.NegativeOrder))
        {
            fmt.lpDecimalSep  = &decimalSep[0]; //not used
            fmt.lpThousandSep = &thousandSep[0];

            //convert LOCALE_SGROUPING to Grouping: http://blogs.msdn.com/b/oldnewthing/archive/2006/04/18/578251.aspx
            replace(grouping, L';', L"");
            if (endsWith(grouping, L'0'))
                grouping.resize(grouping.size() - 1);
            else
                grouping += L'0';
            fmt.Grouping = stringTo<UINT>(grouping);
            valid_ = true;
        }
    }

    NUMBERFMT fmt;
    std::wstring thousandSep;
    std::wstring decimalSep;
    bool valid_;
};
}
#endif


std::wstring zen::ffs_Impl::includeNumberSeparator(const std::wstring& number)
{
#ifdef FFS_WIN
    if (IntegerFormat::isValid())
    {
        int bufferSize = ::GetNumberFormat(LOCALE_USER_DEFAULT, 0, number.c_str(), &IntegerFormat::get(), nullptr, 0);
        if (bufferSize > 0)
        {
            std::vector<wchar_t> buffer(bufferSize);
            if (::GetNumberFormat(LOCALE_USER_DEFAULT,   //__in       LCID Locale,
                                  0,                     //__in       DWORD dwFlags,
                                  number.c_str(),        //__in       LPCTSTR lpValue,
                                  &IntegerFormat::get(), //__in_opt   const NUMBERFMT *lpFormat,
                                  &buffer[0],            //__out_opt  LPTSTR lpNumberStr,
                                  bufferSize) > 0)       //__in       int cchNumber
                return &buffer[0]; //GetNumberFormat() returns char count *including* 0-termination!
        }
    }
    return number;

#else
    //we have to include thousands separator ourselves; this doesn't work for all countries (e.g india), but is better than nothing

    //::setlocale (LC_ALL, ""); -> implicitly called by wxLocale
    const lconv* localInfo = ::localeconv(); //always bound according to doc
    const std::wstring& thousandSep = utfCvrtTo<std::wstring>(localInfo->thousands_sep);

    // THOUSANDS_SEPARATOR = std::use_facet<std::numpunct<wchar_t> >(std::locale("")).thousands_sep(); - why not working?
    // DECIMAL_POINT       = std::use_facet<std::numpunct<wchar_t> >(std::locale("")).decimal_point();

    std::wstring output(number);
    size_t i = output.size();
    for (;;)
    {
        if (i <= 3)
            break;
        i -= 3;
        if (!isDigit(output[i - 1]))
            break;
        output.insert(i, thousandSep);
    }
    return output;
#endif
}

/*
#include <wx/scrolwin.h>

void zen::scrollToBottom(wxScrolledWindow* scrWindow)
{
    int height = 0;
    scrWindow->GetClientSize(nullptr, &height);

    int pixelPerLine = 0;
    scrWindow->GetScrollPixelsPerUnit(nullptr, &pixelPerLine);

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
    auto errorMsg = [&] { return _("Error") + L" (time_t: " + numberTo<std::wstring>(utcTime) + L")"; };

#ifdef FFS_WIN
    FILETIME lastWriteTimeUtc = tofiletime(utcTime); //convert ansi C time to FILETIME

    SYSTEMTIME systemTimeLocal = {};

    if (useNewLocalTimeCalculation) //use DST setting from source date (like in Windows 7, see http://msdn.microsoft.com/en-us/library/ms724277(VS.85).aspx
    {
        SYSTEMTIME systemTimeUtc = {};
        if (!::FileTimeToSystemTime(&lastWriteTimeUtc, //__in   const FILETIME *lpFileTime,
                                    &systemTimeUtc))   //__out  LPSYSTEMTIME lpSystemTime
            return errorMsg();

        if (!::SystemTimeToTzSpecificLocalTime(nullptr,           //__in_opt  LPTIME_ZONE_INFORMATION lpTimeZone,
                                               &systemTimeUtc,    //__in      LPSYSTEMTIME lpUniversalTime,
                                               &systemTimeLocal)) //__out     LPSYSTEMTIME lpLocalTime
            return errorMsg();
    }
    else //use DST setting (like in Windows 2000 and XP)
    {
        FILETIME fileTimeLocal = {};
        if (!::FileTimeToLocalFileTime(&lastWriteTimeUtc, //_In_   const FILETIME *lpFileTime,
                                       &fileTimeLocal))   //_Out_  LPFILETIME lpLocalFileTime
            return errorMsg();

        if (!::FileTimeToSystemTime(&fileTimeLocal,    //__in   const FILETIME *lpFileTime,
                                    &systemTimeLocal)) //__out  LPSYSTEMTIME lpSystemTime
            return errorMsg();
    }

    zen::TimeComp loc;
    loc.year   = systemTimeLocal.wYear;
    loc.month  = systemTimeLocal.wMonth;
    loc.day    = systemTimeLocal.wDay;
    loc.hour   = systemTimeLocal.wHour;
    loc.minute = systemTimeLocal.wMinute;
    loc.second = systemTimeLocal.wSecond;

#elif defined FFS_LINUX
    zen::TimeComp loc = zen::localTime(to<time_t>(utcTime));
#endif

    std::wstring dateString = formatTime<std::wstring>(L"%x  %X", loc);
    return !dateString.empty() ? dateString : errorMsg();
}
