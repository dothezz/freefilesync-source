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
              const wxString& tooltip);

    void setActive(bool value);
    bool isActive() const { return active; }
    void toggle()         { setActive(!active); }

private:
    bool active;

    wxBitmap activeBmp_;
    wxBitmap inactiveBmp_;
};













//######################## implementation ########################
inline
void ToggleButton::init(const wxBitmap& activeBmp,
                        const wxBitmap& inactiveBmp,
                        const wxString& tooltip)
{
    SetToolTip(tooltip);

    activeBmp_   = activeBmp;
    inactiveBmp_ = inactiveBmp;

    setActive(active);
}


inline
void ToggleButton::setActive(bool value)
{
    active = value;
    SetBitmapLabel(active ? activeBmp_ : inactiveBmp_);
}

#endif // TOGGLEBUTTON_H_INCLUDED
