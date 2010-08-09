// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) 2008-2010 ZenJu (zhnmju123 AT gmx.de)                    *
// **************************************************************************
//
#ifndef CUSTOMBUTTON_H_INCLUDED
#define CUSTOMBUTTON_H_INCLUDED

#include <wx/bmpbuttn.h>


//wxButtonWithImage behaves like wxButton but optionally adds bitmap labels
class wxButtonWithImage : public wxBitmapButton
{
public:
    wxButtonWithImage(wxWindow *parent,
                      wxWindowID id,
                      const wxString& label,
                      const wxPoint& pos           = wxDefaultPosition,
                      const wxSize& size           = wxDefaultSize,
                      long style                   = 0,
                      const wxValidator& validator = wxDefaultValidator,
                      const wxString& name         = wxButtonNameStr);

    void setBitmapFront(const wxBitmap& bitmap, unsigned spaceAfter = 0);
    void setTextLabel(  const wxString& text);
    void setBitmapBack( const wxBitmap& bitmap, unsigned spaceBefore = 0);

private:
    wxBitmap createBitmapFromText(const wxString& text);
    void refreshButtonLabel();

    wxBitmap bitmapFront;
    unsigned m_spaceAfter;
    wxString textLabel;
    unsigned m_spaceBefore;
    wxBitmap bitmapBack;
};


#endif // CUSTOMBUTTON_H_INCLUDED
