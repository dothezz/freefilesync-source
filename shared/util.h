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
#include "../shared/globalFunctions.h"

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

template <class NumberType>
wxString numberToStringSep(NumberType number); //convert number to wxString including thousands separator

void setDirectoryName(const wxString& dirname, wxTextCtrl* txtCtrl, wxDirPickerCtrl* dirPicker);
void setDirectoryName(const wxString& dirname, wxComboBox* txtCtrl, wxDirPickerCtrl* dirPicker);
void scrollToBottom(wxScrolledWindow* scrWindow);

wxString utcTimeToLocalString(const wxLongLong& utcTime); //throw std::runtime_error
}


























//--------------- inline impelementation -------------------------------------------

//helper function! not to be used directly
namespace FreeFileSync_Impl
{
wxString includeNumberSeparator(const wxString& number);
}


namespace FreeFileSync
{
//wxULongLongNative doesn't support operator<<(std::ostream&, wxULongLongNative)
template <>
inline
wxString numberToStringSep(wxULongLongNative number)
{
    return FreeFileSync_Impl::includeNumberSeparator(number.ToString());
}


template <class NumberType>
inline
wxString numberToStringSep(NumberType number)
{
    return FreeFileSync_Impl::includeNumberSeparator(globalFunctions::numberToString(number));
}
}


#endif // UTIL_H_INCLUDED
