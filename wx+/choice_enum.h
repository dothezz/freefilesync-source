// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) ZenJu (zenju AT gmx DOT de) - All Rights Reserved        *
// **************************************************************************

#ifndef WX_CHOICE_ENUM_H_INCLUDED
#define WX_CHOICE_ENUM_H_INCLUDED

#include <vector>
#include <wx/choice.h>

//handle mapping of enum values to wxChoice controls
/*
Example:

Member variable:
    zen::EnumDescrList<EnumOnError> enumDescrMap;

Constructor code:
    enumDescrMap.
    add(ON_ERROR_POPUP , "Show pop-up"   , "Show pop-up on errors or warnings"). <- add localization
    add(ON_ERROR_IGNORE, "Ignore errors" , "Hide all error and warning messages").
    add(ON_ERROR_EXIT  , "Exit instantly", "Abort synchronization immediately");

Set enum value:
    setEnumVal(enumDescrMap, *m_choiceHandleError, value);

Get enum value:
	value = getEnumVal(enumDescrMap, *m_choiceHandleError)

Update enum tooltips (after user changed selection):
	updateTooltipEnumVal(enumDescrMap, *m_choiceHandleError);
*/


namespace zen
{
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



















//--------------- inline impelementation -------------------------------------------
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


#endif //WX_CHOICE_ENUM_H_INCLUDED
