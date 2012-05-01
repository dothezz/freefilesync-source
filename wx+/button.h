// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) ZenJu (zhnmju123 AT gmx DOT de) - All Rights Reserved    *
// **************************************************************************

#ifndef CUSTOMBUTTON_H_INCLUDED
#define CUSTOMBUTTON_H_INCLUDED

#include <wx/bmpbuttn.h>

namespace zen
{
//zen::BitmapButton behaves like wxButton but optionally adds bitmap labels
class BitmapButton : public wxBitmapButton
{
public:
    BitmapButton(wxWindow* parent,
                 wxWindowID id,
                 const wxString& label,
                 const wxPoint& pos           = wxDefaultPosition,
                 const wxSize& size           = wxDefaultSize,
                 long style                   = 0,
                 const wxValidator& validator = wxDefaultValidator,
                 const wxString& name         = wxButtonNameStr);

    void setBitmapFront(const wxBitmap& bitmap, size_t spaceAfter = 0);
    void setTextLabel(  const wxString& text);
    void setBitmapBack( const wxBitmap& bitmap, size_t spaceBefore = 0);

private:
    wxBitmap createBitmapFromText(const wxString& text);
    void refreshButtonLabel();

    wxBitmap bitmapFront;
    unsigned int m_spaceAfter;
    wxString textLabel;
    unsigned int m_spaceBefore;
    wxBitmap bitmapBack;
};

//set bitmap label flicker free!
void setImage(wxBitmapButton& button, const wxBitmap& bmp);
}


#endif // CUSTOMBUTTON_H_INCLUDED