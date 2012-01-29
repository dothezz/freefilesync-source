// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) ZenJu (zhnmju123 AT gmx DOT de) - All Rights Reserved    *
// **************************************************************************

#ifndef IMAGE_TOOLS_HEADER_45782456427634254
#define IMAGE_TOOLS_HEADER_45782456427634254

#include <numeric>
#include <wx/bitmap.h>
#include <wx/image.h>
#include <wx/dcmemory.h>
#include <zen/basic_math.h>

namespace zen
{
wxBitmap greyScale(const wxBitmap& bmp); //greyscale + brightness adaption
wxBitmap layOver(const wxBitmap& foreground, const wxBitmap& background); //merge

void move(wxImage& img, int up, int left = 0);
void adjustBrightness(wxImage& img, int targetLevel);
double getAvgBrightness(const wxImage& img); //in [0, 255]
void brighten(wxImage& img, int level); //level: delta per channel in points

bool isEqual(const wxBitmap& lhs, const wxBitmap& rhs); //pixel-wise equality (respecting alpha channel)

wxColor gradient(const wxColor& from, const wxColor& to, double fraction); //maps fraction within [0, 1] to an intermediate color















//################################### implementation ###################################
inline
void move(wxImage& img, int up, int left)
{
    img = img.GetSubImage(wxRect(std::max(0, left), std::max(0, up), img.GetWidth() - abs(left), img.GetHeight() - abs(up)));
    img.Resize(wxSize(img.GetWidth() + abs(left), img.GetHeight() + abs(up)), wxPoint(-std::min(0, left), -std::min(0, up)));
}


inline
wxBitmap greyScale(const wxBitmap& bmp)
{
    wxImage output = bmp.ConvertToImage().ConvertToGreyscale(1.0/3, 1.0/3, 1.0/3); //treat all channels equally!
    //wxImage output = bmp.ConvertToImage().ConvertToGreyscale();
    adjustBrightness(output, 170);
    return output;
}


inline
double getAvgBrightness(const wxImage& img)
{
    const int pixelCount = img.GetWidth() * img.GetHeight();
    auto pixBegin = img.GetData();

    if (pixelCount > 0 && pixBegin)
    {
        auto pixEnd = pixBegin + 3 * pixelCount; //RGB

        if (img.HasAlpha())
        {
            const unsigned char* alphaFirst = img.GetAlpha();

            //calculate average weighted by alpha channel
            double dividend = 0;
            for (auto iter = pixBegin; iter != pixEnd; ++iter)
                dividend += *iter * static_cast<double>(alphaFirst[(iter - pixBegin) / 3]);

            const double divisor = 3.0 * std::accumulate(alphaFirst, alphaFirst + pixelCount, 0.0);

            return numeric::isNull(divisor) ? 0 : dividend / divisor;
        }
        else
            return std::accumulate(pixBegin, pixEnd, 0.0) / (3.0 * pixelCount);
    }
    return 0;
}


inline
void brighten(wxImage& img, int level)
{
    const int pixelCount = img.GetWidth() * img.GetHeight();
    auto pixBegin = img.GetData();
    if (pixBegin)
    {
        auto pixEnd = pixBegin + 3 * pixelCount; //RGB
        if (level > 0)
            std::for_each(pixBegin, pixEnd, [&](unsigned char& c) { c = std::min(255, c + level); });
        else
            std::for_each(pixBegin, pixEnd, [&](unsigned char& c) { c = std::max(0, c + level); });
    }
}


inline
void adjustBrightness(wxImage& img, int targetLevel)
{
    brighten(img, targetLevel - getAvgBrightness(img));
}


inline
wxBitmap layOver(const wxBitmap& foreground, const wxBitmap& background)
{
    wxBitmap output = background;
    {
        wxMemoryDC dc;
        dc.SelectObject(output);
        dc.DrawBitmap(foreground, 0, 0, true);
        dc.SelectObject(wxNullBitmap);
    }
    return output;
}


inline
bool isEqual(const wxBitmap& lhs, const wxBitmap& rhs)
{
    if (lhs.IsOk() != rhs.IsOk())
        return false;
    if (!lhs.IsOk())
        return true;

    const int pixelCount = lhs.GetWidth() * lhs.GetHeight();
    if (pixelCount != rhs.GetWidth() * rhs.GetHeight())
        return false;

    wxImage imLhs = lhs.ConvertToImage();
    wxImage imRhs = rhs.ConvertToImage();

    if (imLhs.HasAlpha() != imRhs.HasAlpha())
        return false;

    if (imLhs.HasAlpha())
    {
        if (!std::equal(imLhs.GetAlpha(), imLhs.GetAlpha() + pixelCount, imRhs.GetAlpha()))
            return false;
    }

    return std::equal(imLhs.GetData(), imLhs.GetData() + pixelCount * 3, imRhs.GetData());
}

inline
wxColor gradient(const wxColor& from, const wxColor& to, double fraction)
{
    fraction = std::max(0.0, fraction);
    fraction = std::min(1.0, fraction);
    return wxColor(from.Red  () + (to.Red  () - from.Red  ()) * fraction,
                   from.Green() + (to.Green() - from.Green()) * fraction,
                   from.Blue () + (to.Blue () - from.Blue ()) * fraction,
                   from.Alpha() + (to.Alpha() - from.Alpha()) * fraction);
}
}


#endif //IMAGE_TOOLS_HEADER_45782456427634254
