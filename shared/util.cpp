// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) 2008-2010 ZenJu (zhnmju123 AT gmx.de)                    *
// **************************************************************************
//
#include "util.h"
#include <wx/scrolwin.h>
#include <wx/textctrl.h>
#include <wx/combobox.h>
#include <wx/filepicker.h>
#include "localization.h"
#include "fileHandling.h"
#include "stringConv.h"
#include <stdexcept>
#include "systemFunctions.h"
#include "checkExist.h"

#ifdef FFS_WIN
#include <wx/msw/wrapwin.h> //includes "windows.h"
#endif


wxString FreeFileSync::formatFilesizeToShortString(const wxLongLong& filesize)
{
    return FreeFileSync::formatFilesizeToShortString(filesize.ToDouble());
}


wxString FreeFileSync::formatFilesizeToShortString(const wxULongLong& filesize)
{
    return FreeFileSync::formatFilesizeToShortString(filesize.ToDouble());
}


wxString FreeFileSync::formatFilesizeToShortString(double filesize)
{
    if (filesize < 0)
        return _("Error");

    wxString output = _("%x Byte");

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
        const size_t leadDigitCount = globalFunctions::getDigitCount(static_cast<size_t>(filesize)); //number of digits before decimal point
        if (leadDigitCount == 0 || leadDigitCount > 3)
            return _("Error");

        output.Replace(wxT("%x"), wxString::Format(wxT("%.*f"), static_cast<int>(3 - leadDigitCount), filesize));
    }
    else
    {
        output.Replace(wxT("%x"), globalFunctions::numberToString(static_cast<int>(filesize))); //no decimal places in case of bytes
    }

    return output;
}


wxString FreeFileSync::formatPercentage(const wxLongLong& dividend, const wxLongLong& divisor)
{
    const double ratio = divisor != 0 ? dividend.ToDouble() * 100 / divisor.ToDouble() : 0;
    wxString output = _("%x%");
    output.Replace(wxT("%x"), wxString::Format(wxT("%3.2f"), ratio), false);
    return output;
}


wxString FreeFileSync_Impl::includeNumberSeparator(const wxString& number)
{
    wxString output(number);
    for (size_t i = output.size(); i > 3; i -= 3)
        output.insert(i - 3, FreeFileSync::getThousandsSeparator());

    return output;
}


template <class T>
void setDirectoryNameImpl(const wxString& dirname, T* txtCtrl, wxDirPickerCtrl* dirPicker)
{
    using namespace FreeFileSync;

    txtCtrl->SetValue(dirname);
    const Zstring dirFormatted = FreeFileSync::getFormattedDirectoryName(wxToZ(dirname));

    if (Utility::dirExists(dirFormatted, 200) == Utility::EXISTING_TRUE) //potentially slow network access: wait 200ms at most
        dirPicker->SetPath(zToWx(dirFormatted));
}


void FreeFileSync::setDirectoryName(const wxString& dirname, wxTextCtrl* txtCtrl, wxDirPickerCtrl* dirPicker)
{
    setDirectoryNameImpl(dirname, txtCtrl, dirPicker);
}


void FreeFileSync::setDirectoryName(const wxString& dirname, wxComboBox* txtCtrl, wxDirPickerCtrl* dirPicker)
{
    txtCtrl->SetSelection(wxNOT_FOUND);
    setDirectoryNameImpl(dirname, txtCtrl, dirPicker);
}


void FreeFileSync::scrollToBottom(wxScrolledWindow* scrWindow)
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
inline
void writeTwoDigitNumber(size_t number, wxString& string)
{
    assert (number < 100);

    string += wxChar('0' + number / 10);
    string += wxChar('0' + number % 10);
}


inline
void writeFourDigitNumber(size_t number, wxString& string)
{
    assert (number < 10000);

    string += wxChar('0' + number / 1000);
    number %= 1000;
    string += wxChar('0' + number / 100);
    number %= 100;
    string += wxChar('0' + number / 10);
    number %= 10;
    string += wxChar('0' + number);
}
}

wxString FreeFileSync::utcTimeToLocalString(const wxLongLong& utcTime)
{
#ifdef FFS_WIN
    //convert ansi C time to FILETIME
    wxLongLong fileTimeLong(utcTime);

    fileTimeLong += wxLongLong(2, 3054539008UL); //timeshift between ansi C time and FILETIME in seconds == 11644473600s
    fileTimeLong *= 10000000;

    FILETIME lastWriteTimeUtc;
    lastWriteTimeUtc.dwLowDateTime  = fileTimeLong.GetLo();             //GetLo() returns unsigned
    lastWriteTimeUtc.dwHighDateTime = unsigned(fileTimeLong.GetHi());   //GetHi() returns signed


    FILETIME localFileTime;
    if (::FileTimeToLocalFileTime( //convert to local time
                &lastWriteTimeUtc, //pointer to UTC file time to convert
                &localFileTime 	   //pointer to converted file time
            ) == 0)
        throw std::runtime_error(std::string((wxString(_("Conversion error:")) + wxT(" FILETIME -> local FILETIME: ") +
                                              wxT("(") + wxULongLong(lastWriteTimeUtc.dwHighDateTime, lastWriteTimeUtc.dwLowDateTime).ToString() + wxT(") ") +
                                              wxT("\n\n") + getLastErrorFormatted()).ToAscii()));

    if (localFileTime.dwHighDateTime > 0x7fffffff)
        return _("Error");  //this actually CAN happen if UTC time is just below this border and ::FileTimeToLocalFileTime() adds 2 hours due to DST or whatever!
    //Testcase (UTC): dateHigh = 2147483647 (=0x7fffffff) -> year 30000
    //                dateLow  = 4294967295

    SYSTEMTIME time;
    if (::FileTimeToSystemTime(
                &localFileTime, //pointer to file time to convert
                &time 	        //pointer to structure to receive system time
            ) == 0)
        throw std::runtime_error(std::string((wxString(_("Conversion error:")) + wxT(" local FILETIME -> SYSTEMTIME: ") +
                                              wxT("(") + wxULongLong(localFileTime.dwHighDateTime, localFileTime.dwLowDateTime).ToString() + wxT(") ") +
                                              wxT("\n\n") + getLastErrorFormatted()).ToAscii()));

    //assemble time string (performance optimized) -> note: performance argument may not be valid any more
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

    return formattedTime;

#elif defined FFS_LINUX
    tm* timeinfo;
    const time_t fileTime = utcTime.ToLong();
    timeinfo = localtime(&fileTime); //convert to local time
    char buffer[50];
    strftime(buffer, 50, "%Y-%m-%d  %H:%M:%S", timeinfo);

    return zToWx(buffer);
#endif
}
