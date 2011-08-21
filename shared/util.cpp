// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) 2008-2011 ZenJu (zhnmju123 AT gmx.de)                    *
// **************************************************************************

#include "util.h"
#include "zstring.h"
#include "i18n.h"
#include "last_error.h"
#include "global_func.h"

#ifdef FFS_WIN
#include <wx/msw/wrapwin.h> //includes "windows.h"
#endif


wxString zen::extractJobName(const wxString& configFilename)
{
    const wxString shortName = configFilename.AfterLast(FILE_NAME_SEPARATOR); //returns the whole string if seperator not found
    const wxString jobName = shortName.BeforeLast(wxChar('.')); //returns empty string if seperator not found
    return jobName.IsEmpty() ? shortName : jobName;
}


wxString zen::formatFilesizeToShortString(UInt64 size)
{
    if (to<Int64>(size) < 0) return _("Error");

    if (size <= 999U)
    {
        wxString output = _P("1 Byte", "%x Bytes", to<int>(size));
        output.Replace(wxT("%x"), toStringSep(size)); //no decimal places in case of bytes
        return output;
    }
    else
    {
        double filesize = to<double>(size);

        filesize /= 1024;
        wxString output = _("%x KB");
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
        const size_t leadDigitCount = common::getDigitCount(static_cast<size_t>(filesize)); //number of digits before decimal point
        if (leadDigitCount == 0 || leadDigitCount > 3)
            return _("Error");

        output.Replace(wxT("%x"), wxString::Format(wxT("%.*f"), static_cast<int>(3 - leadDigitCount), filesize));
        return output;
    }
}


wxString zen::formatPercentage(zen::Int64 dividend, zen::Int64 divisor)
{
    const double ratio = divisor != 0 ? to<double>(dividend) * 100.0 / to<double>(divisor) : 0;
    wxString output = _("%x%");
    output.Replace(wxT("%x"), wxString::Format(wxT("%3.2f"), ratio), false);
    return output;
}


wxString ffs_Impl::includeNumberSeparator(const wxString& number)
{
    wxString output(number);
    for (size_t i = output.size(); i > 3; i -= 3)
        output.insert(i - 3, zen::getThousandsSeparator());

    return output;
}


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


namespace
{
#ifdef FFS_WIN
bool isVistaOrLater()
{
    OSVERSIONINFO osvi = {};
    osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);

    //symbolic links are supported starting with Vista
    if (::GetVersionEx(&osvi))
        return osvi.dwMajorVersion > 5; //XP has majorVersion == 5, minorVersion == 1; Vista majorVersion == 6, dwMinorVersion == 0
    //overview: http://msdn.microsoft.com/en-us/library/ms724834(VS.85).aspx
    return false;
}
#endif
}


wxString zen::utcTimeToLocalString(zen::Int64 utcTime)
{
#ifdef FFS_WIN
    //convert ansi C time to FILETIME
    zen::UInt64 fileTimeLong = to<zen::UInt64>(utcTime + //may be < 0
                                               zen::Int64(3054539008UL, 2)); //timeshift between ansi C time and FILETIME in seconds == 11644473600s
    fileTimeLong *= 10000000U;

    FILETIME lastWriteTimeUtc = {};
    lastWriteTimeUtc.dwLowDateTime  = fileTimeLong.getLo();
    lastWriteTimeUtc.dwHighDateTime = fileTimeLong.getHi();

    SYSTEMTIME systemTimeLocal = {};

    static const bool useNewLocalTimeCalculation = isVistaOrLater();
    if (useNewLocalTimeCalculation) //use DST setting from source date (like in Windows 7, see http://msdn.microsoft.com/en-us/library/ms724277(VS.85).aspx)
    {
        SYSTEMTIME systemTimeUtc = {};
        if (!::FileTimeToSystemTime(
                &lastWriteTimeUtc, //__in   const FILETIME *lpFileTime,
                &systemTimeUtc))   //__out  LPSYSTEMTIME lpSystemTime
            return _("Error");

        if (!::SystemTimeToTzSpecificLocalTime(
                NULL,              //__in_opt  LPTIME_ZONE_INFORMATION lpTimeZone,
                &systemTimeUtc,    //__in      LPSYSTEMTIME lpUniversalTime,
                &systemTimeLocal)) //__out     LPSYSTEMTIME lpLocalTime
            return _("Error");
    }
    else //use DST setting (like in Windows 2000 and XP)
    {
        FILETIME fileTimeLocal = {};
        if (!::FileTimeToLocalFileTime( //convert to local time
                &lastWriteTimeUtc,  //pointer to UTC file time to convert
                &fileTimeLocal))    //pointer to converted file time
            return _("Error");

        if (!::FileTimeToSystemTime(&fileTimeLocal,  //pointer to file time to convert
                                    &systemTimeLocal)) //pointer to structure to receive system time
            return _("Error");
    }

    const wxDateTime localTime(systemTimeLocal.wDay,
                               wxDateTime::Month(systemTimeLocal.wMonth - 1),
                               systemTimeLocal.wYear,
                               systemTimeLocal.wHour,
                               systemTimeLocal.wMinute,
                               systemTimeLocal.wSecond);

#elif defined FFS_LINUX
    const time_t fileTime = to<time_t>(utcTime);
    const tm* timeinfo = ::localtime(&fileTime); //convert to local time

    /*
        char buffer[50];
        ::strftime(buffer, 50, "%Y-%m-%d  %H:%M:%S", timeinfo);
        return zToWx(buffer);
    */
    const wxDateTime localTime(timeinfo->tm_mday,
                               wxDateTime::Month(timeinfo->tm_mon),
                               1900 + timeinfo->tm_year,
                               timeinfo->tm_hour,
                               timeinfo->tm_min,
                               timeinfo->tm_sec);
#endif

    return localTime.FormatDate() + wxT("  ") + localTime.FormatTime();
}
