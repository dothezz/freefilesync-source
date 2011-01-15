// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) 2008-2011 ZenJu (zhnmju123 AT gmx.de)                    *
// **************************************************************************
//
#ifndef UTIL_H_INCLUDED
#define UTIL_H_INCLUDED

#include <wx/string.h>
#include <wx/longlong.h>
#include <wx/textctrl.h>
#include <wx/filepicker.h>
#include <wx/combobox.h>
#include <wx/scrolwin.h>
#include "zstring.h"
#include "global_func.h"


namespace ffs3
{
wxString extractJobName(const wxString& configFilename);

wxString formatFilesizeToShortString(const wxLongLong& filesize);
wxString formatFilesizeToShortString(const wxULongLong& filesize);
wxString formatFilesizeToShortString(double filesize);

wxString formatPercentage(const wxLongLong& dividend, const wxLongLong& divisor);

template <class NumberType>
wxString numberToStringSep(NumberType number); //convert number to wxString including thousands separator

void scrollToBottom(wxScrolledWindow* scrWindow);

wxString utcTimeToLocalString(const wxLongLong& utcTime); //throw std::runtime_error
}


























//--------------- inline impelementation -------------------------------------------

//helper function! not to be used directly
namespace ffs_Impl
{
wxString includeNumberSeparator(const wxString& number);
}


namespace ffs3
{
//wxULongLongNative doesn't support operator<<(std::ostream&, wxULongLongNative)
template <>
inline
wxString numberToStringSep(wxULongLongNative number)
{
    return ffs_Impl::includeNumberSeparator(number.ToString());
}


template <class NumberType>
inline
wxString numberToStringSep(NumberType number)
{
    return ffs_Impl::includeNumberSeparator(common::numberToString(number));
}
}


#endif // UTIL_H_INCLUDED
