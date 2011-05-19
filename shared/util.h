// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) 2008-2011 ZenJu (zhnmju123 AT gmx.de)                    *
// **************************************************************************
//
#ifndef UTIL_H_INCLUDED
#define UTIL_H_INCLUDED

#include <wx/string.h>
#include <wx/textctrl.h>
#include <wx/filepicker.h>
#include <wx/combobox.h>
#include <wx/scrolwin.h>
#include <wx/choice.h>
#include "zstring.h"
#include "string_tools.h"
#include "int64.h"

namespace zen
{
wxString extractJobName(const wxString& configFilename);

wxString formatFilesizeToShortString(zen::UInt64 filesize);
wxString formatPercentage(zen::Int64 dividend, zen::Int64 divisor);

template <class NumberType>
wxString toStringSep(NumberType number); //convert number to wxString including thousands separator

void scrollToBottom(wxScrolledWindow* scrWindow);

wxString utcTimeToLocalString(zen::Int64 utcTime); //throw std::runtime_error


//handle mapping of enum values to wxChoice controls
template <class Enum>
struct EnumDescrList
{
    EnumDescrList& add(Enum value, const wxString& text, const wxString& tooltip = wxEmptyString)
    {
        descrList.push_back(std::make_pair(value, std::make_pair(text, tooltip)));
        return *this;
    }
    typedef std::vector<std::pair<Enum, std::pair<wxString, wxString> > > DescrList;
    DescrList descrList;
};
template <class Enum> void setEnumVal(const EnumDescrList<Enum>& mapping, wxChoice& ctrl, Enum value);
template <class Enum> Enum getEnumVal(const EnumDescrList<Enum>& mapping, const wxChoice& ctrl);
template <class Enum> void updateTooltipEnumVal(const EnumDescrList<Enum>& mapping, wxChoice& ctrl);
}


























//--------------- inline impelementation -------------------------------------------

//helper function! not to be used directly
namespace ffs_Impl
{
wxString includeNumberSeparator(const wxString& number);
}


namespace zen
{
template <class NumberType>
inline
wxString toStringSep(NumberType number)
{
    return ffs_Impl::includeNumberSeparator(zen::toString<wxString>(number));
}

template <class Enum>
void setEnumVal(const EnumDescrList<Enum>& mapping, wxChoice& ctrl, Enum value)
{
    ctrl.Clear();

    int selectedPos = 0;
    for (typename EnumDescrList<Enum>::DescrList::const_iterator i = mapping.descrList.begin(); i != mapping.descrList.end(); ++i)
    {
        ctrl.Append(i->second.first);
        if (i->first == value)
        {
            selectedPos = i - mapping.descrList.begin();

            if (!i->second.second.empty())
                ctrl.SetToolTip(i->second.second);
        }
    }

    ctrl.SetSelection(selectedPos);
}

template <class Enum>
Enum getEnumVal(const EnumDescrList<Enum>& mapping, const wxChoice& ctrl)
{
    const int selectedPos = ctrl.GetSelection();

    if (0 <= selectedPos && selectedPos < static_cast<int>(mapping.descrList.size()))
        return mapping.descrList[selectedPos].first;
    else
    {
        assert(false);
        return Enum(0);
    }
}

template <class Enum> void updateTooltipEnumVal(const EnumDescrList<Enum>& mapping, wxChoice& ctrl)
{
    const Enum value = getEnumVal(mapping, ctrl);

    for (typename EnumDescrList<Enum>::DescrList::const_iterator i = mapping.descrList.begin(); i != mapping.descrList.end(); ++i)
        if (i->first == value)
            ctrl.SetToolTip(i->second.second);
}

}


#endif // UTIL_H_INCLUDED
