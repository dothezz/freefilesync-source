// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) 2008-2011 ZenJu (zhnmju123 AT gmx.de)                    *
// **************************************************************************

#include "custom_tooltip.h"
#include <wx/stattext.h>
#include <wx/sizer.h>
#include <wx/statbmp.h>
#include <wx/settings.h>

class CustomTooltip::PopupFrameGenerated : public wxFrame
{
public:
    PopupFrameGenerated(wxWindow* parent,
                        wxWindowID id = wxID_ANY,
                        const wxString& title = wxEmptyString,
                        const wxPoint& pos = wxDefaultPosition,
                        const wxSize& size = wxSize( -1,-1 ),
                        long style = wxFRAME_NO_TASKBAR | wxSTAY_ON_TOP | wxSTATIC_BORDER);

    wxStaticText* m_staticTextMain;
    wxStaticBitmap* m_bitmapLeft;
};


CustomTooltip::PopupFrameGenerated::PopupFrameGenerated(
    wxWindow* parent,
    wxWindowID id,
    const wxString& title,
    const wxPoint& pos,
    const wxSize& size,
    long style ) : wxFrame(parent, id, title, pos, size, style)
{
    this->SetSizeHints( wxDefaultSize, wxDefaultSize );
    this->SetBackgroundColour(wxSystemSettings::GetColour(wxSYS_COLOUR_INFOBK));   //both required: on Ubuntu background is black, foreground white!
    this->SetForegroundColour(wxSystemSettings::GetColour(wxSYS_COLOUR_INFOTEXT)); //

    wxBoxSizer* bSizer158;
    bSizer158 = new wxBoxSizer( wxHORIZONTAL );

    m_bitmapLeft = new wxStaticBitmap( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, 0 );
    bSizer158->Add( m_bitmapLeft, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );

    m_staticTextMain = new wxStaticText( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
    bSizer158->Add( m_staticTextMain, 0, wxALL | wxALIGN_CENTER_HORIZONTAL | wxALIGN_CENTER_VERTICAL, 5 );

    this->SetSizer( bSizer158 );
    this->Layout();
    bSizer158->Fit( this );
}


CustomTooltip::CustomTooltip() : tipWindow(new PopupFrameGenerated(NULL)), lastBmp(NULL)
{
#ifdef FFS_WIN //neither looks good nor works at all on Linux
    tipWindow->Disable(); //prevent window stealing focus!
#endif

    hide();
}


CustomTooltip::~CustomTooltip()
{
    tipWindow->Destroy();
}


void CustomTooltip::show(const wxString& text, wxPoint pos, const wxBitmap* bmp)
{
    if (bmp != lastBmp)
    {
        lastBmp = bmp;
        tipWindow->m_bitmapLeft->SetBitmap(bmp == NULL ? wxNullBitmap : *bmp);
    }

    if (text != tipWindow->m_staticTextMain->GetLabel())
    {
        tipWindow->m_staticTextMain->SetLabel(text);
        tipWindow->m_staticTextMain->Wrap(600);
    }

#ifdef FFS_LINUX
    tipWindow->Fit(); //Alas Fit() seems to be somewhat broken => this needs to be called EVERY time inside show, not only if text or bmp change.
#endif

    if (pos != tipWindow->GetScreenPosition())
        tipWindow->SetSize(pos.x + 30, pos.y, wxDefaultCoord, wxDefaultCoord);
    //attention!!! possible endless loop: mouse pointer must NOT be within tipWindow!
    //Else it will trigger a wxEVT_LEAVE_WINDOW which will hide the window, causing the window to be shown again via this method, etc.

    if (!tipWindow->IsShown())
        tipWindow->Show();
}


void CustomTooltip::hide()
{
    tipWindow->Hide();
}
