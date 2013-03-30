// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) Zenju (zenju AT gmx DOT de) - All Rights Reserved        *
// **************************************************************************

#include "tray_icon.h"
#include <zen/basic_math.h>
#include <wx/taskbar.h>
#include <wx/menu.h>
#include <wx/icon.h> //req. by Linux
#include <wx+/image_tools.h>
#include "small_dlgs.h"
#include "../lib/resources.h"


const wxEventType FFS_REQUEST_RESUME_TRAY_EVENT = wxNewEventType();

namespace
{
void fillRange(wxImage& img, int pixelFirst, int pixelLast, const wxColor& col) //tolerant input range
{
    if (img.IsOk())
    {
        const int width  = img.GetWidth ();
        const int height = img.GetHeight();

        if (width > 0 && height > 0)
        {
            pixelFirst = std::max(pixelFirst, 0);
            pixelLast  = std::min(pixelLast, width * height);

            if (pixelFirst < pixelLast)
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
    }
}


wxIcon generateProgressIcon(const wxImage& logo, double fraction) //generate icon with progress indicator
{
    if (!logo.IsOk())
        return wxIcon();

    const int pixelCount = logo.GetWidth() * logo.GetHeight();
    const int startFillPixel = std::min(numeric::round(fraction * pixelCount), pixelCount);

    //minor optimization
    static std::pair<int, wxIcon> buffer = std::make_pair(-1, wxNullIcon);

    if (buffer.first != startFillPixel)
    {
        //progress bar
        if (logo.GetWidth()  > 0 &&
            logo.GetHeight() > 0)
        {
            wxImage genImage(logo.Copy()); //workaround wxWidgets' screwed-up design from hell: their copy-construction implements reference-counting WITHOUT copy-on-write!

            //gradually make FFS icon brighter while nearing completion
            zen::brighten(genImage, -200 * (1 - fraction));

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
                fillRange(genImage, bStart, startFillPixel, *wxBLACK);
            }
            else if (startFillPixel < pixelCount)
            {
                //special handling for last row
                /*
                        --------
                        --------
                        ---bbbbb
                        ---bSyyy  S : start yellow remainder
                */
                int bStart = startFillPixel - genImage.GetWidth() - 1;
                int bEnd = (bStart / genImage.GetWidth() + 1) * genImage.GetWidth();

                fillRange(genImage, bStart, bEnd, *wxBLACK);
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
            buffer.second.CopyFromBitmap(wxBitmap(genImage));
        }
        else
            buffer.second = wxIcon();
    }

    return buffer.second;
}

//------------------------------------------------------------------------------------------------

enum Selection
{
    CONTEXT_RESTORE = 1, //wxWidgets: "A MenuItem ID of Zero does not work under Mac"
    CONTEXT_ABOUT = wxID_ABOUT
};
}


class FfsTrayIcon::TaskBarImpl : public wxTaskBarIcon
{
public:
    TaskBarImpl(FfsTrayIcon& parent) : parent_(&parent) {}

    void parentHasDied() { parent_ = nullptr; }

private:
    virtual wxMenu* CreatePopupMenu()
    {
        if (!parent_)
            return nullptr;

        wxMenu* contextMenu = new wxMenu;
        contextMenu->Append(CONTEXT_ABOUT, _("&About"));
        contextMenu->AppendSeparator();
        contextMenu->Append(CONTEXT_RESTORE, _("&Restore"));
        //event handling
        contextMenu->Connect(wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(FfsTrayIcon::OnContextMenuSelection), nullptr, parent_);

        return contextMenu; //ownership transferred to caller
    }

    FfsTrayIcon* parent_;
};


FfsTrayIcon::FfsTrayIcon() :
    trayIcon(new TaskBarImpl(*this)),
    fractionLast(1), //show FFS logo by default
#if defined FFS_WIN || defined FFS_MAC //16x16 seems to be the only size that is shown correctly on OS X
    logo(getResourceImage(L"FFS_tray_16x16").ConvertToImage())
#elif defined FFS_LINUX
    logo(getResourceImage(L"FFS_tray_24x24").ConvertToImage())
#endif
{
    trayIcon->SetIcon(generateProgressIcon(logo, fractionLast), L"FreeFileSync");
    trayIcon->Connect(wxEVT_TASKBAR_LEFT_DCLICK, wxCommandEventHandler(FfsTrayIcon::OnDoubleClick), nullptr, this); //register double-click
}


FfsTrayIcon::~FfsTrayIcon()
{
    trayIcon->RemoveIcon(); //hide icon until final deletion takes place
    trayIcon->Disconnect(wxEVT_TASKBAR_LEFT_DCLICK, wxCommandEventHandler(FfsTrayIcon::OnDoubleClick), nullptr, this);
    trayIcon->parentHasDied(); //TaskBarImpl (potentially) has longer lifetime than FfsTrayIcon: avoid callback!

    //use wxWidgets delayed destruction: delete during next idle loop iteration (handle late window messages, e.g. when double-clicking)
    if (!wxPendingDelete.Member(trayIcon))
        wxPendingDelete.Append(trayIcon);
}


void FfsTrayIcon::setToolTip(const wxString& toolTip)
{
    toolTipLast = toolTip;
    trayIcon->SetIcon(generateProgressIcon(logo, fractionLast), toolTip); //another wxWidgets design bug: non-orthogonal method!
}


void FfsTrayIcon::setProgress(double fraction)
{
    fractionLast = fraction;
    trayIcon->SetIcon(generateProgressIcon(logo, fraction), toolTipLast);
}


void FfsTrayIcon::OnContextMenuSelection(wxCommandEvent& event)
{
    switch (static_cast<Selection>(event.GetId()))
    {
        case CONTEXT_ABOUT:
        {
            //ATTENTION: the modal dialog below does NOT disable all GUI input, e.g. user may still double-click on tray icon
            //which will implicitly destroy the tray icon while still showing the modal dialog
            trayIcon->SetEvtHandlerEnabled(false);
            zen::showAboutDialog(nullptr);
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

