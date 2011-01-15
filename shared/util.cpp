// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) 2008-2011 ZenJu (zhnmju123 AT gmx.de)                    *
// **************************************************************************
//
#include "util.h"
#include <wx/scrolwin.h>
#include <wx/textctrl.h>
#include <wx/combobox.h>
#include <wx/filepicker.h>
#include "localization.h"
#include "file_handling.h"
#include "string_conv.h"
#include <stdexcept>
#include "system_func.h"
#include "check_exist.h"
#include "assert_static.h"
#include "system_constants.h"


#ifdef FFS_WIN
#include <wx/msw/wrapwin.h> //includes "windows.h"
#endif

wxString ffs3::extractJobName(const wxString& configFilename)
{
    using namespace common;

    const wxString shortName = configFilename.AfterLast(FILE_NAME_SEPARATOR); //returns the whole string if seperator not found
    const wxString jobName = shortName.BeforeLast(wxChar('.')); //returns empty string if seperator not found
    return jobName.IsEmpty() ? shortName : jobName;
}


wxString ffs3::formatFilesizeToShortString(const wxLongLong& filesize)
{
    return ffs3::formatFilesizeToShortString(filesize.ToDouble());
}


wxString ffs3::formatFilesizeToShortString(const wxULongLong& filesize)
{
    return ffs3::formatFilesizeToShortString(filesize.ToDouble());
}


wxString ffs3::formatFilesizeToShortString(double filesize)
{
    if (filesize < 0)
        return _("Error");

    wxString output = _("%x Bytes");

    if (filesize > 999)
    {
        filesize /= 1024;
        output = _("%x kB");
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
    }
    else
    {
        output.Replace(wxT("%x"), common::numberToString(static_cast<int>(filesize))); //no decimal places in case of bytes
    }

    return output;
}


wxString ffs3::formatPercentage(const wxLongLong& dividend, const wxLongLong& divisor)
{
    const double ratio = divisor != 0 ? dividend.ToDouble() * 100 / divisor.ToDouble() : 0;
    wxString output = _("%x%");
    output.Replace(wxT("%x"), wxString::Format(wxT("%3.2f"), ratio), false);
    return output;
}


wxString ffs_Impl::includeNumberSeparator(const wxString& number)
{
    wxString output(number);
    for (size_t i = output.size(); i > 3; i -= 3)
        output.insert(i - 3, ffs3::getThousandsSeparator());

    return output;
}


void ffs3::scrollToBottom(wxScrolledWindow* scrWindow)
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


wxString ffs3::utcTimeToLocalString(const wxLongLong& utcTime)
{
#ifdef FFS_WIN
    //convert ansi C time to FILETIME
    wxLongLong fileTimeLong(utcTime);
    fileTimeLong += wxLongLong(2, 3054539008UL); //timeshift between ansi C time and FILETIME in seconds == 11644473600s
    fileTimeLong *= 10000000;

    FILETIME lastWriteTimeUtc;
    lastWriteTimeUtc.dwLowDateTime  = fileTimeLong.GetLo();             //GetLo() returns unsigned
    lastWriteTimeUtc.dwHighDateTime = static_cast<DWORD>(fileTimeLong.GetHi());   //GetHi() returns signed

    assert(fileTimeLong.GetHi() >= 0);
    assert_static(sizeof(DWORD) == sizeof(long));
    assert_static(sizeof(long) == 4);

    SYSTEMTIME systemTimeLocal = {};

    static const bool useNewLocalTimeCalculation = isVistaOrLater();
    if (useNewLocalTimeCalculation) //use DST setting from source date (like in Windows 7, see http://msdn.microsoft.com/en-us/library/ms724277(VS.85).aspx)
    {
        if (lastWriteTimeUtc.dwHighDateTime > 0x7FFFFFFF)
            return _("Error");

        SYSTEMTIME systemTimeUtc = {};
        if (!::FileTimeToSystemTime(
                    &lastWriteTimeUtc, //__in   const FILETIME *lpFileTime,
                    &systemTimeUtc))   //__out  LPSYSTEMTIME lpSystemTime
            throw std::runtime_error(std::string((wxString(_("Conversion error:")) + wxT(" FILETIME -> SYSTEMTIME: ") +
                                                  wxT("(") + wxULongLong(lastWriteTimeUtc.dwHighDateTime, lastWriteTimeUtc.dwLowDateTime).ToString() + wxT(") ") +
                                                  wxT("\n\n") + getLastErrorFormatted()).ToAscii()));

        if (!::SystemTimeToTzSpecificLocalTime(
                    NULL,              //__in_opt  LPTIME_ZONE_INFORMATION lpTimeZone,
                    &systemTimeUtc,    //__in      LPSYSTEMTIME lpUniversalTime,
                    &systemTimeLocal)) //__out     LPSYSTEMTIME lpLocalTime
            throw std::runtime_error(std::string((wxString(_("Conversion error:")) + wxT(" SYSTEMTIME -> local SYSTEMTIME: ") +
                                                  wxT("(") + wxULongLong(lastWriteTimeUtc.dwHighDateTime, lastWriteTimeUtc.dwLowDateTime).ToString() + wxT(") ") +
                                                  wxT("\n\n") + getLastErrorFormatted()).ToAscii()));
    }
    else //use current DST setting (like in Windows 2000 and XP)
    {
        FILETIME fileTimeLocal = {};
        if (!::FileTimeToLocalFileTime( //convert to local time
                    &lastWriteTimeUtc,  //pointer to UTC file time to convert
                    &fileTimeLocal))    //pointer to converted file time
            throw std::runtime_error(std::string((wxString(_("Conversion error:")) + wxT(" FILETIME -> local FILETIME: ") +
                                                  wxT("(") + wxULongLong(lastWriteTimeUtc.dwHighDateTime, lastWriteTimeUtc.dwLowDateTime).ToString() + wxT(") ") +
                                                  wxT("\n\n") + getLastErrorFormatted()).ToAscii()));

        if (fileTimeLocal.dwHighDateTime > 0x7FFFFFFF)
            return _("Error");  //this actually CAN happen if UTC time is just below this border and ::FileTimeToLocalFileTime() adds 2 hours due to DST or whatever!
        //Testcase (UTC): dateHigh = 2147483647 (=0x7fffffff) -> year 30000
        //                dateLow  = 4294967295

        if (!::FileTimeToSystemTime(
                    &fileTimeLocal,  //pointer to file time to convert
                    &systemTimeLocal)) //pointer to structure to receive system time
            throw std::runtime_error(std::string((wxString(_("Conversion error:")) + wxT(" local FILETIME -> local SYSTEMTIME: ") +
                                                  wxT("(") + wxULongLong(fileTimeLocal.dwHighDateTime, fileTimeLocal.dwLowDateTime).ToString() + wxT(") ") +
                                                  wxT("\n\n") + getLastErrorFormatted()).ToAscii()));
    }

    /*
         //assemble time string (performance optimized) -> note: performance argument is not valid anymore

         wxString formattedTime;
         formattedTime.reserve(20);

         writeFourDigitNumber(time.wYear, formattedTime);
         formattedTime += wxChar('-');
         writeTwoDigitNumber(time.wMonth, formattedTime);
         formattedTime += wxChar('-');
         writeTwoDigitNumber(time.wDay, formattedTime);
         formattedTime += wxChar(' ');
         formattedTime += wxChar(' ');
         writeTwoDigitNumber(time.wHour, formattedTime);
         formattedTime += wxChar(':');
         writeTwoDigitNumber(time.wMinute, formattedTime);
         formattedTime += wxChar(':');
         writeTwoDigitNumber(time.wSecond, formattedTime);
     */
    const wxDateTime localTime(systemTimeLocal.wDay,
                               wxDateTime::Month(systemTimeLocal.wMonth - 1),
                               systemTimeLocal.wYear,
                               systemTimeLocal.wHour,
                               systemTimeLocal.wMinute,
                               systemTimeLocal.wSecond);

#elif defined FFS_LINUX
    tm* timeinfo;
    const time_t fileTime = utcTime.ToLong();
    timeinfo = localtime(&fileTime); //convert to local time

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
