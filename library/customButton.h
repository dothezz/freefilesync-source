/***************************************************************
 * Purpose:   wxButton with bitmap label
 * Author:    ZenJu (zhnmju123@gmx.de)
 * Created:   Feb. 2009
 **************************************************************/

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

    void setBitmapFront(const wxBitmap& bitmap);
    void setTextLabel(  const wxString& text);
    void setBitmapBack( const wxBitmap& bitmap);

private:
    wxBitmap createBitmapFromText(const wxString& text);
    void refreshButtonLabel();

    wxBitmap bitmapFront;
    wxString textLabel;
    wxBitmap bitmapBack;
};


#endif // CUSTOMBUTTON_H_INCLUDED
