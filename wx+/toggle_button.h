// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) Zenju (zenju AT gmx DOT de) - All Rights Reserved        *
// **************************************************************************

#ifndef TOGGLEBUTTON_H_INCLUDED
#define TOGGLEBUTTON_H_INCLUDED

#include <wx/bmpbuttn.h>

class ToggleButton : public wxBitmapButton
{
public:
    ToggleButton(wxWindow*          parent,
                 wxWindowID         id,
                 const wxBitmap&    bitmap,
                 const wxPoint&     pos = wxDefaultPosition,
                 const wxSize&      size = wxDefaultSize,
                 long               style = 0,
                 const wxValidator& validator = wxDefaultValidator,
                 const wxString&    name = wxButtonNameStr) :
        wxBitmapButton(parent, id, bitmap, pos, size, style, validator, name),
        active(false)
    {
        SetLayoutDirection(wxLayout_LeftToRight); //avoid mirroring RTL languages like Hebrew or Arabic
    }

    void init(const wxBitmap& activeBmp,
              const wxBitmap& inactiveBmp,
              const wxString& activeTooltip,
              const wxString& inactiveTooltip = wxString());

    void setActive(bool value);
    bool isActive() const { return active; }
    void toggle()         { setActive(!active); }

private:
    bool active;

    wxBitmap m_activeBmp;
    wxString m_activeTooltip;

    wxBitmap m_inactiveBmp;
    wxString m_inactiveTooltip;
};













//######################## implementation ########################
inline
void ToggleButton::init(const wxBitmap& activeBmp,
                        const wxBitmap& inactiveBmp,
                        const wxString& activeTooltip,
                        const wxString& inactiveTooltip)
{
    m_activeBmp       = activeBmp;
    m_activeTooltip   = activeTooltip;
    m_inactiveBmp     = inactiveBmp;
    m_inactiveTooltip = inactiveTooltip.empty() ? activeTooltip : inactiveTooltip;

    //load resources
    setActive(active);
}


inline
void ToggleButton::setActive(bool value)
{
    active = value;

    if (active)
    {
        SetBitmapLabel(m_activeBmp);
        SetToolTip(m_activeTooltip);
    }
    else
    {
        SetBitmapLabel(m_inactiveBmp);
        SetToolTip(m_inactiveTooltip);
    }
}

#endif // TOGGLEBUTTON_H_INCLUDED
