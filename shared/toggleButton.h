// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) 2008-2010 ZenJu (zhnmju123 AT gmx.de)                    *
// **************************************************************************
//
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
        active(false) {}

    void init(const wxBitmap& activeBmp,
              const wxString& activeTooltip,
              const wxBitmap& inactiveBmp,
              const wxString& inactiveTooltip);

    void setActive(bool value);
    bool isActive() const;
    void toggle();

private:
    bool active;

    wxBitmap m_activeBmp;
    wxString m_activeTooltip;

    wxBitmap m_inactiveBmp;
    wxString m_inactiveTooltip;
};


#endif // TOGGLEBUTTON_H_INCLUDED
