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
#include "../shared/globalFunctions.h"
#include "../shared/localization.h"
#include "../shared/fileHandling.h"
#include "../shared/stringConv.h"
#include <stdexcept>
#include "../shared/systemFunctions.h"

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

    if (filesize <= 999)
        return wxString::Format(wxT("%i"), static_cast<int>(filesize)) + _(" Byte"); //no decimal places in case of bytes

    filesize /= 1024;
    wxString unit = _(" kB");
    if (filesize > 999)
    {
        filesize /= 1024;
        unit = _(" MB");
        if (filesize > 999)
        {
            filesize /= 1024;
            unit = _(" GB");
            if (filesize > 999)
            {
                filesize /= 1024;
                unit = _(" TB");
                if (filesize > 999)
                {
                    filesize /= 1024;
                    unit = _(" PB");
                }
            }
        }
    }

    //print just three significant digits: 0,01 | 0,11 | 1,11 | 11,1 | 111

    const unsigned int leadDigitCount = globalFunctions::getDigitCount(static_cast<unsigned int>(filesize)); //number of digits before decimal point
    if (leadDigitCount == 0 || leadDigitCount > 3)
        return _("Error");

    if (leadDigitCount == 3)
        return wxString::Format(wxT("%i"), static_cast<int>(filesize)) + unit;
    else if (leadDigitCount == 2)
    {
        wxString output = wxString::Format(wxT("%i"), static_cast<int>(filesize * 10));
        output.insert(leadDigitCount, getDecimalPoint());
        return output + unit;
    }
    else //leadDigitCount == 1
    {
        wxString output = wxString::Format(wxT("%03i"), static_cast<int>(filesize * 100));
        output.insert(leadDigitCount, getDecimalPoint());
        return output + unit;
    }

    //return wxString::Format(wxT("%.*f"), 3 - leadDigitCount, filesize) + unit;
}


wxString FreeFileSync::formatPercentage(const wxLongLong& dividend, const wxLongLong& divisor)
{
    const double ratio = divisor != 0 ? dividend.ToDouble() * 100 / divisor.ToDouble() : 0;
    wxString output = _("%x%");
    output.Replace(wxT("%x"), wxString::Format(wxT("%3.2f"), ratio), false);
    return output;
}


namespace
{
wxString includeNumberSeparator(const wxString& number)
{
    wxString output(number);
	for (size_t i = output.size(); i > 3; i -= 3)
        output.insert(i - 3, FreeFileSync::getThousandsSeparator());

    return output;
}
}


wxString FreeFileSync::numberToWxString(size_t number, bool includeNumberSep)
{
    const wxString output = wxString::Format(wxT("%u"), number);
    return includeNumberSep ? includeNumberSeparator(output) : output;
}



wxString FreeFileSync::numberToWxString(int number, bool includeNumberSep)
{
    const wxString output = wxString::Format(wxT("%i"), number);
    return includeNumberSep ? includeNumberSeparator(output) : output;
}


wxString FreeFileSync::numberToWxString(const wxULongLong& number, bool includeNumberSep)
{
    return includeNumberSep ? includeNumberSeparator(number.ToString()) : number.ToString();
}


template <class T>
void setDirectoryNameImpl(const wxString& dirname, T* txtCtrl, wxDirPickerCtrl* dirPicker)
{
    using namespace FreeFileSync;

    txtCtrl->SetValue(dirname);
    const Zstring leftDirFormatted = FreeFileSync::getFormattedDirectoryName(wxToZ(dirname));
    if (FreeFileSync::dirExists(leftDirFormatted))
        dirPicker->SetPath(zToWx(leftDirFormatted));
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


inline
void writeTwoDigitNumber(unsigned int number, wxString& string)
{
    assert (number < 100);

    string += '0' + number / 10;
    string += '0' + number % 10;
}


inline
void writeFourDigitNumber(unsigned int number, wxString& string)
{
    assert (number < 10000);

    string += '0' + number / 1000;
    number %= 1000;
    string += '0' + number / 100;
    number %= 100;
    string += '0' + number / 10;
    number %= 10;
    string += '0' + number;
}


wxString FreeFileSync::utcTimeToLocalString(const wxLongLong& utcTime, const Zstring& filename)
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
                                              filename.c_str() + wxT("\n\n") + getLastErrorFormatted()).ToAscii()));

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
                                              filename.c_str()  + wxT("\n\n") + getLastErrorFormatted()).ToAscii()));

    //assemble time string (performance optimized)
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
