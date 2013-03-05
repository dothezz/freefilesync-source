// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) Zenju (zenju AT gmx DOT de) - All Rights Reserved        *
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

    void setBitmapFront(const wxBitmap& bitmap, int spaceAfter = 0); //...and enlarge button if required!
    void setBitmapBack (const wxBitmap& bitmap, int spaceBefore = 0); //

    void setInnerBorderSize(int sz) { innerBorderSize = sz; refreshButtonLabel(); }

    virtual void SetLabel(const wxString& label);

    void refreshButtonLabel(); //e.g. after font change

private:
    wxBitmap createBitmapFromText(const wxString& text);

    wxBitmap bitmapFront;
    int spaceAfter_;
    ///wxString textLabel;
    int spaceBefore_;
    wxBitmap bitmapBack;

    int innerBorderSize;
};

//set bitmap label flicker free!
void setImage(wxBitmapButton& button, const wxBitmap& bmp);
}


#endif // CUSTOMBUTTON_H_INCLUDED
