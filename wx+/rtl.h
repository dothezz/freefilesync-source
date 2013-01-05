// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) Zenju (zenju AT gmx DOT de) - All Rights Reserved        *
// **************************************************************************

#ifndef RTL_H_0183487180058718273432148
#define RTL_H_0183487180058718273432148

#include <memory>
#include <wx/dcmemory.h>
#include <wx/dcmirror.h>
#include <wx/image.h>
#include <wx/icon.h>
#include <wx/app.h>
#include <wx/dcbuffer.h> //for macro: wxALWAYS_NATIVE_DOUBLE_BUFFER

namespace zen
{
//functions supporting right-to-left GUI layout

void drawBitmapRtlMirror(wxDC& dc,
                         const wxBitmap& image,
                         const wxRect& rect,
                         int alignment,
                         std::unique_ptr<wxBitmap>& buffer); //mirror image if layout is RTL + fix some strange wxDC::Blit bug on RTL

void drawBitmapRtlNoMirror(wxDC& dc,  //wxDC::DrawLabel does already NOT mirror by default (but does a crappy job at it, surprise)
                           const wxBitmap& image,
                           const wxRect& rect,
                           int alignment,
                           std::unique_ptr<wxBitmap>& buffer);

void drawIconRtlNoMirror(wxDC& dc, //wxDC::DrawIcon DOES mirror by default
                         const wxIcon& icon,
                         const wxPoint& pt,
                         std::unique_ptr<wxBitmap>& buffer);

wxBitmap mirrorIfRtl(const wxBitmap& bmp);


/*
class BufferedPaintDC
{
public:
    BufferedPaintDC(wxWindow& wnd, std::unique_ptr<wxBitmap>& buffer);
};
*/
//a fix for a poor wxWidgets implementation (wxAutoBufferedPaintDC skips one pixel on left side when RTL layout is active)


//manual text flow correction: http://www.w3.org/International/articles/inline-bidi-markup/








//---------------------- implementation ------------------------
namespace
{
template <class DrawImageFun>
void drawRtlImpl(wxDC& dc, const wxRect& rect, std::unique_ptr<wxBitmap>& buffer, bool doMirror, DrawImageFun draw)
{
    if (dc.GetLayoutDirection() == wxLayout_RightToLeft)
    {
        if (!buffer || buffer->GetWidth() != rect.width || buffer->GetHeight() < rect.height) //[!] since we do a mirror, width needs to match exactly!
            buffer.reset(new wxBitmap(rect.width, rect.height));

        wxMemoryDC memDc(*buffer);
        memDc.Blit(wxPoint(0, 0), rect.GetSize(), &dc, rect.GetTopLeft()); //blit in: background is mirrored due to memDc, dc having different layout direction!

        if (!doMirror)
        {
            *buffer = wxBitmap(buffer->ConvertToImage().Mirror());
            memDc.SelectObject(*buffer);
        }

        draw(memDc, wxRect(0, 0, rect.width, rect.height));
        //note: we cannot simply use memDc.SetLayoutDirection(wxLayout_RightToLeft) due to some strange 1 pixel bug! so it's a quadruple mirror! :>

        if (!doMirror)
        {
            *buffer = wxBitmap(buffer->ConvertToImage().Mirror());
            memDc.SelectObject(*buffer);
        }

        dc.Blit(rect.GetTopLeft(), rect.GetSize(), &memDc, wxPoint(0, 0)); //blit out: mirror once again
    }
    else
        draw(dc, rect);
}
}


inline
void drawBitmapRtlMirror(wxDC& dc, const wxBitmap& image, const wxRect& rect, int alignment, std::unique_ptr<wxBitmap>& buffer)
{
    return drawRtlImpl(dc, rect, buffer, true, [&](wxDC& dc2, const wxRect& rect2) { dc2.DrawLabel(wxString(), image, rect2, alignment); });
}

inline
void drawBitmapRtlNoMirror(wxDC& dc, const wxBitmap& image, const wxRect& rect, int alignment, std::unique_ptr<wxBitmap>& buffer)
{
    if (dc.GetLayoutDirection() == wxLayout_RightToLeft)
        if ((alignment & wxALIGN_CENTER_HORIZONTAL) == 0) //we still *do* want to mirror alignment!
            alignment ^= wxALIGN_RIGHT;
    static_assert(wxALIGN_LEFT == 0, "doh");
    return drawRtlImpl(dc, rect, buffer, false, [&](wxDC& dc2, const wxRect& rect2) { dc2.DrawLabel(wxString(), image, rect2, alignment); });
}

inline
void drawIconRtlNoMirror(wxDC& dc, const wxIcon& icon, const wxPoint& pt, std::unique_ptr<wxBitmap>& buffer)
{
    wxRect rect(pt.x, pt.y, icon.GetWidth(), icon.GetHeight());
    return drawRtlImpl(dc, rect, buffer, false, [&](wxDC& dc2, const wxRect& rect2) { dc2.DrawIcon(icon, rect2.GetTopLeft()); });
}


inline
wxBitmap mirrorIfRtl(const wxBitmap& bmp)
{
    if (wxTheApp->GetLayoutDirection() == wxLayout_RightToLeft)
        return bmp.ConvertToImage().Mirror();
    else
        return bmp;
}


#ifndef wxALWAYS_NATIVE_DOUBLE_BUFFER
#error we need this one!
#endif

#if wxALWAYS_NATIVE_DOUBLE_BUFFER
struct BufferedPaintDC : public wxPaintDC { BufferedPaintDC(wxWindow& wnd, std::unique_ptr<wxBitmap>& buffer) : wxPaintDC(&wnd) {} };

#else
class BufferedPaintDC : public wxMemoryDC
{
public:
    BufferedPaintDC(wxWindow& wnd, std::unique_ptr<wxBitmap>& buffer) : buffer_(buffer), paintDc(&wnd)
    {
        const wxSize clientSize = wnd.GetClientSize();
        if (!buffer_ || clientSize != wxSize(buffer->GetWidth(), buffer->GetHeight()))
            buffer.reset(new wxBitmap(clientSize.GetWidth(), clientSize.GetHeight()));

        SelectObject(*buffer);

        if (paintDc.IsOk() && paintDc.GetLayoutDirection() == wxLayout_RightToLeft)
            SetLayoutDirection(wxLayout_RightToLeft);
    }

    ~BufferedPaintDC()
    {
        if (GetLayoutDirection() == wxLayout_RightToLeft)
        {
            paintDc.SetLayoutDirection(wxLayout_LeftToRight); //workaround bug in wxDC::Blit()
            SetLayoutDirection(wxLayout_LeftToRight);         //
        }

        const wxPoint origin = GetDeviceOrigin();
        paintDc.Blit(0, 0, buffer_->GetWidth(), buffer_->GetHeight(), this, -origin.x, -origin.y);
    }

private:
    std::unique_ptr<wxBitmap>& buffer_;
    wxPaintDC paintDc;
};
#endif
}

#endif //RTL_H_0183487180058718273432148
