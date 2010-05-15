// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) 2008-2010 ZenJu (zhnmju123 AT gmx.de)                    *
// **************************************************************************
//
#ifndef UTIL_H_INCLUDED
#define UTIL_H_INCLUDED

#include "../shared/zstring.h"
#include <wx/string.h>
#include <wx/longlong.h>

class wxComboBox;
class wxTextCtrl;
class wxDirPickerCtrl;
class wxScrolledWindow;


namespace FreeFileSync
{
wxString formatFilesizeToShortString(const wxLongLong& filesize);
wxString formatFilesizeToShortString(const wxULongLong& filesize);
wxString formatFilesizeToShortString(double filesize);

wxString formatPercentage(const wxLongLong& dividend, const wxLongLong& divisor);

wxString numberToWxString(size_t      number, bool includeNumberSep); //convert number to wxString
wxString numberToWxString(int         number, bool includeNumberSep); //convert number to wxString
wxString numberToWxString(const wxULongLong& number, bool includeNumberSep); //convert number to wxString

void setDirectoryName(const wxString& dirname, wxTextCtrl* txtCtrl, wxDirPickerCtrl* dirPicker);
void setDirectoryName(const wxString& dirname, wxComboBox* txtCtrl, wxDirPickerCtrl* dirPicker);
void scrollToBottom(wxScrolledWindow* scrWindow);

wxString utcTimeToLocalString(const wxLongLong& utcTime); //throw std::runtime_error
}


#endif // UTIL_H_INCLUDED
