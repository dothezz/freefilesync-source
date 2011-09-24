// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) 2008-2011 ZenJu (zhnmju123 AT gmx.de)                    *
// **************************************************************************

#include "tray_icon.h"
#include "../library/resources.h"
#include "small_dlgs.h"
#include "../shared/i18n.h"
#include <wx/taskbar.h>
#include <cmath>
#include <wx/image.h>
#include <wx/menu.h>
#include <wx/icon.h> //req. by Linux
#include "../shared/image_tools.h"


const wxEventType FFS_REQUEST_RESUME_TRAY_EVENT = wxNewEventType();


namespace
{
inline
int roundNum(double d) //little rounding function
{
    return static_cast<int>(d < 0 ? d - .5 : d + .5);
}

void fillRange(wxImage& img, int pixelFirst, int pixelLast, const wxColor& col)
{
    const int pixelCount = img.GetWidth() >= 0 ? img.GetWidth() * img.GetHeight() : -1;

    if (0 <= pixelFirst && pixelFirst < pixelLast && pixelLast <= pixelCount)
    {
        unsigned char* const bytesBegin = img.GetData() + pixelFirst * 3;
        unsigned char* const bytesEnd   = img.GetData() + pixelLast  * 3;

        for (unsigned char* bytePos = bytesBegin; bytePos < bytesEnd; bytePos += 3)
        {
            bytePos[0] = col.Red  ();
            bytePos[1] = col.Green();
            bytePos[2] = col.Blue ();
        }

        if (img.HasAlpha()) //make progress indicator fully opaque:
            std::fill(img.GetAlpha() + pixelFirst, img.GetAlpha() + pixelLast, wxIMAGE_ALPHA_OPAQUE);
    }
}

wxIcon generateIcon(double fraction) //generate icon with progress indicator
{
#ifdef FFS_WIN
    static const wxBitmap trayIcon = GlobalResources::getImage(wxT("FFS_tray_win.png"));
#elif defined FFS_LINUX
    static const wxBitmap trayIcon = GlobalResources::getImage(wxT("FFS_tray_linux.png"));
#endif

    const int pixelCount = trayIcon.GetWidth() * trayIcon.GetHeight();
    const int startFillPixel = std::min(roundNum(fraction * pixelCount), pixelCount);

    //minor optimization
    static std::pair<int, wxIcon> buffer = std::make_pair(-1, wxNullIcon);
    if (buffer.first == startFillPixel)
        return buffer.second;

    wxIcon genIcon;

    {
        wxImage genImage(trayIcon.ConvertToImage());

        //gradually make FFS icon brighter while nearing completion
        zen::brighten(genImage, -200 * (1 - fraction));

        //progress bar
        if (genImage.GetWidth()  > 0 &&
            genImage.GetHeight() > 0)
        {
            //fill black border row
            if (startFillPixel <= pixelCount - genImage.GetWidth())
            {
                /*
                        --------
                        ---bbbbb
                        bbbbSyyy  S : start yellow remainder
                        yyyyyyyy
                */
                int bStart = startFillPixel - genImage.GetWidth();
                if (bStart % genImage.GetWidth() != 0) //add one more black pixel, see ascii-art
                    --bStart;
                fillRange(genImage, std::max(bStart, 0), startFillPixel, *wxBLACK);
            }
            else if (startFillPixel != pixelCount)
            {
                //special handling for last row
                /*
                        --------
                        --------
                        ---bbbbb
                        ---bSyyy  S : start yellow remainder
                */
                int bStart = startFillPixel - genImage.GetWidth() - 1;
                int bEnd = (bStart / genImage.GetWidth() + 1) * (genImage.GetWidth());

                fillRange(genImage, std::max(bStart, 0), bEnd, *wxBLACK);
                fillRange(genImage, startFillPixel - 1, startFillPixel, *wxBLACK);
            }

            //fill yellow remainder
            fillRange(genImage, startFillPixel, pixelCount, wxColour(240, 200, 0));

            /*
                    const int indicatorWidth  = genImage.GetWidth() * .4;
                    const int indicatorXBegin = std::ceil((genImage.GetWidth() - indicatorWidth) / 2.0);
                    const int indicatorYBegin = genImage.GetHeight() - indicatorHeight;

                    //draw progress indicator: do NOT use wxDC::DrawRectangle! Doesn't respect alpha in Windows, but does in Linux!
                    //We need a simple, working solution:

                    for (int row = indicatorYBegin;  row < genImage.GetHeight(); ++row)
                    {
                        for (int col = indicatorXBegin;  col < indicatorXBegin + indicatorWidth; ++col)
                        {
                            unsigned char* const pixelBegin = data + (row * genImage.GetWidth() + col) * 3;
                            pixelBegin[0] = 240; //red
                            pixelBegin[1] = 200; //green
                            pixelBegin[2] = 0;   //blue
                        }
                    }

                    if (genImage.HasAlpha())
                    {
                        unsigned char* const alpha = genImage.GetAlpha();
                        //make progress indicator fully opaque:
                        for (int row = indicatorYBegin;  row < genImage.GetHeight(); ++row)
                            ::memset(alpha + row * genImage.GetWidth() + indicatorXBegin, wxIMAGE_ALPHA_OPAQUE, indicatorWidth);
                    }
            */
            genIcon.CopyFromBitmap(wxBitmap(genImage));
        }
        else //fallback
            genIcon.CopyFromBitmap(trayIcon);
    }

    //fill buffer
    buffer.first  = startFillPixel;
    buffer.second = genIcon;
    return genIcon;
}
}


//------------------------------------------------------------------------------------------------
enum Selection
{
    CONTEXT_RESTORE,
    CONTEXT_ABOUT
};


class FfsTrayIcon::TaskBarImpl : public wxTaskBarIcon
{
public:
    TaskBarImpl(FfsTrayIcon& parent) : parent_(&parent) {}

    void parentHasDied() { parent_ = NULL; }

private:
    virtual wxMenu* CreatePopupMenu()
    {
        if (!parent_)
            return NULL;

        wxMenu* contextMenu = new wxMenu;
        contextMenu->Append(CONTEXT_ABOUT, _("&About..."));
        contextMenu->AppendSeparator();
        contextMenu->Append(CONTEXT_RESTORE, _("&Restore"));
        //event handling
        contextMenu->Connect(wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(FfsTrayIcon::OnContextMenuSelection), NULL, parent_);

        return contextMenu; //ownership transferred to caller
    }

    FfsTrayIcon* parent_;
};


FfsTrayIcon::FfsTrayIcon() :
    trayIcon(new TaskBarImpl(*this))
{
    trayIcon->SetIcon(generateIcon(0), wxT("FreeFileSync"));
    trayIcon->Connect(wxEVT_TASKBAR_LEFT_DCLICK, wxCommandEventHandler(FfsTrayIcon::OnDoubleClick), NULL, this); //register double-click
}


FfsTrayIcon::~FfsTrayIcon()
{
    trayIcon->RemoveIcon(); //hide icon until final deletion takes place
    trayIcon->Disconnect(wxEVT_TASKBAR_LEFT_DCLICK, wxCommandEventHandler(FfsTrayIcon::OnDoubleClick), NULL, this);
    trayIcon->parentHasDied(); //TaskBarImpl (potentially) has longer lifetime than FfsTrayIcon: avoid callback!

    //use wxWidgets delayed destruction: delete during next idle loop iteration (handle late window messages, e.g. when double-clicking)
    if (!wxPendingDelete.Member(trayIcon))
        wxPendingDelete.Append(trayIcon);
}


void FfsTrayIcon::setToolTip2(const wxString& toolTipText, double fraction)
{
    trayIcon->SetIcon(generateIcon(fraction), toolTipText);
}


void FfsTrayIcon::OnContextMenuSelection(wxCommandEvent& event)
{
    const Selection eventId = static_cast<Selection>(event.GetId());
    switch (eventId)
    {
        case CONTEXT_ABOUT:
        {
            //ATTENTION: the modal dialog below does NOT disable all GUI input, e.g. user may still double-click on tray icon
            //which will implicitly destroy the tray icon while still showing the modal dialog
            trayIcon->SetEvtHandlerEnabled(false);
            zen::showAboutDialog();
            trayIcon->SetEvtHandlerEnabled(true);
        }
        break;
        case CONTEXT_RESTORE:
        {
            wxCommandEvent dummy(FFS_REQUEST_RESUME_TRAY_EVENT);
            ProcessEvent(dummy);
        }
    }
}


void FfsTrayIcon::OnDoubleClick(wxCommandEvent& event)
{
    wxCommandEvent dummy(FFS_REQUEST_RESUME_TRAY_EVENT);
    ProcessEvent(dummy);
}

