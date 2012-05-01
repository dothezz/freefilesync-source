// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) ZenJu (zhnmju123 AT gmx DOT de) - All Rights Reserved    *
// **************************************************************************

#include "tooltip.h"
#include <wx/stattext.h>
#include <wx/sizer.h>
#include <wx/statbmp.h>
#include <wx/settings.h>
#include <wx/app.h>
#include <wx+/image_tools.h>

using namespace zen;


class Tooltip::PopupFrameGenerated : public wxFrame
{
public:
    PopupFrameGenerated(wxWindow* parent,
                        wxWindowID id = wxID_ANY,
                        const wxString& title = wxEmptyString,
                        const wxPoint& pos = wxDefaultPosition,
                        const wxSize& size = wxSize( -1, -1 ),
                        long style = wxFRAME_NO_TASKBAR | wxSTAY_ON_TOP | wxSTATIC_BORDER) : wxFrame(parent, id, title, pos, size, style)
    {
        this->SetSizeHints(wxDefaultSize, wxDefaultSize);
        this->SetBackgroundColour(wxSystemSettings::GetColour(wxSYS_COLOUR_INFOBK));   //both required: on Ubuntu background is black, foreground white!
        this->SetForegroundColour(wxSystemSettings::GetColour(wxSYS_COLOUR_INFOTEXT)); //

        wxBoxSizer* bSizer158;
        bSizer158 = new wxBoxSizer(wxHORIZONTAL);

        m_bitmapLeft = new wxStaticBitmap(this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, 0);
        bSizer158->Add(m_bitmapLeft, 0, wxALL | wxALIGN_CENTER_VERTICAL, 5);

        m_staticTextMain = new wxStaticText(this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0);
        bSizer158->Add(m_staticTextMain, 0, wxALL | wxALIGN_CENTER_HORIZONTAL | wxALIGN_CENTER_VERTICAL, 5);

        this->SetSizer(bSizer158);
        this->Layout();
        bSizer158->Fit(this);
    }

    wxStaticText* m_staticTextMain;
    wxStaticBitmap* m_bitmapLeft;
};


Tooltip::Tooltip() : tipWindow(new PopupFrameGenerated(nullptr))
{
#ifdef FFS_WIN //neither looks good nor works at all on Linux
    tipWindow->Disable(); //prevent window stealing focus!
#endif
    hide();
}


Tooltip::~Tooltip()
{
    tipWindow->Destroy();
}


void Tooltip::show(const wxString& text, wxPoint mousePos, const wxBitmap* bmp)
{
    const wxBitmap& newBmp = bmp ? *bmp : wxNullBitmap;

    if (!isEqual(tipWindow->m_bitmapLeft->GetBitmap(), newBmp))
        tipWindow->m_bitmapLeft->SetBitmap(newBmp);

    if (text != tipWindow->m_staticTextMain->GetLabel())
    {
        tipWindow->m_staticTextMain->SetLabel(text);
        tipWindow->m_staticTextMain->Wrap(600);
    }

    tipWindow->Fit(); //Linux: Fit() seems to be somewhat broken => this needs to be called EVERY time inside show, not only if text or bmp change

    const wxPoint newPos = wxTheApp->GetLayoutDirection() == wxLayout_RightToLeft ?
                           mousePos - wxPoint(30 + tipWindow->GetSize().GetWidth(), 0) :
                           mousePos + wxPoint(30, 0);

    if (newPos != tipWindow->GetScreenPosition())
        tipWindow->SetSize(newPos.x, newPos.y, wxDefaultCoord, wxDefaultCoord);
    //attention!!! possible endless loop: mouse pointer must NOT be within tipWindow!
    //else it will trigger a wxEVT_LEAVE_WINDOW on middle grid which will hide the window, causing the window to be shown again via this method, etc.

    if (!tipWindow->IsShown())
        tipWindow->Show();
}


void Tooltip::hide()
{
#ifdef FFS_LINUX
    //on wxGTK the tip window occassionally goes blank and stays that way. This is somehow triggered by wxWindow::Hide() and doesn't seem to be a wxWidgets bug (=> GTK?)
    //apply brute force:
    tipWindow->Destroy();
    tipWindow = new PopupFrameGenerated(nullptr);
#endif

    tipWindow->Hide();
}