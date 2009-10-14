#include "customButton.h"
#include <wx/dcmemory.h>
#include <wx/image.h>

wxButtonWithImage::wxButtonWithImage(wxWindow *parent,
                                     wxWindowID id,
                                     const wxString& label,
                                     const wxPoint& pos,
                                     const wxSize& size,
                                     long style,
                                     const wxValidator& validator,
                                     const wxString& name) :
    wxBitmapButton(parent, id, wxNullBitmap, pos, size, style | wxBU_AUTODRAW, validator, name),
    m_spaceAfter(0),
    m_spaceBefore(0)
{
    setTextLabel(label);
}


void wxButtonWithImage::setBitmapFront(const wxBitmap& bitmap, unsigned spaceAfter)
{
    bitmapFront  = bitmap;
    m_spaceAfter = spaceAfter;
    refreshButtonLabel();
}


void wxButtonWithImage::setTextLabel(const wxString& text)
{
    textLabel = text;
    wxBitmapButton::SetLabel(text);
    refreshButtonLabel();
}


void wxButtonWithImage::setBitmapBack(const wxBitmap& bitmap, unsigned spaceBefore)
{
    bitmapBack    = bitmap;
    m_spaceBefore = spaceBefore;
    refreshButtonLabel();
}


void makeWhiteTransparent(const wxColor exceptColor, wxImage& image)
{
    unsigned char* alphaData = image.GetAlpha();
    if (alphaData)
    {
        assert(exceptColor.Red() != 255);

        unsigned char exCol = exceptColor.Red(); //alpha value can be extracted from any one of (red/green/blue)
        unsigned char* imageStart = image.GetData();

        unsigned char* j = alphaData;
        const unsigned char* const rowEnd = j + image.GetWidth() * image.GetHeight();
        while (j != rowEnd)
        {
            const unsigned char* imagePixel = imageStart + (j - alphaData) * 3; //each pixel consists of three chars
            //exceptColor(red,green,blue) becomes fully opaque(255), while white(255,255,255) becomes transparent(0)
            *(j++) = ((255 - imagePixel[0]) * wxIMAGE_ALPHA_OPAQUE) / (255 - exCol);
        }
    }
}


wxSize getSizeNeeded(const wxString& text, wxFont& font)
{
    wxCoord width, height;
    wxMemoryDC dc;

    wxString textFormatted = text;
    textFormatted.Replace(wxT("&"), wxT(""), false); //remove accelerator
    dc.GetMultiLineTextExtent(textFormatted, &width, &height , NULL, &font);
    return wxSize(width, height);
}

/*
inline
void linearInterpolationHelper(const int shift, wxImage& img)
{
    unsigned char* const data = img.GetData();
    const int width = img.GetWidth();
    if (width < 2)
        return;

    const float intensity = 0.25;

    for (int y = 1; y < img.GetHeight() - 1; ++y)
    {
        float back   = 0;
        float middle = 0;
        float front  = 0;

        unsigned char* location           = data + 3 * (y * width) + shift;
        const unsigned char* const endPos = location + 3 * width;

        middle = (*location + *(location - 3 * width) + *(location + 3 * width)) / 3;;
        front  = (*(location + 3) + *(location + 3 * (1 - width)) + *(location + 3 * (1 + width))) / 3;
        *location += ((middle + front) / 2 - *location) * intensity;
        location += 3;

        while (location < endPos - 3)
        {
            back   = middle;
            middle = front;
            front  = (*(location + 3) + *(location + 3 * (1 - width)) + *(location + 3 * (1 + width))) / 3;
            *location += ((back + middle + front) / 3 - *location) * intensity;
            location += 3;
        }

        back   = middle;
        middle = front;
        *location += ((back + middle) / 2 - *location) * intensity;
    }
}


void linearInterpolation(wxImage& img)
{
    linearInterpolationHelper(0, img); //red channel
    linearInterpolationHelper(1, img); //green channel
    linearInterpolationHelper(2, img); //blue channel
}
*/

wxBitmap wxButtonWithImage::createBitmapFromText(const wxString& text)
{
    if (text.empty())
        return wxBitmap();

    wxFont currentFont = wxBitmapButton::GetFont();
    wxColor textColor  = wxBitmapButton::GetForegroundColour();

    wxSize sizeNeeded = getSizeNeeded(text, currentFont);
    wxBitmap newBitmap(sizeNeeded.GetWidth(), sizeNeeded.GetHeight());

    wxMemoryDC dc;
    dc.SelectObject(newBitmap);

    //set up white background
    dc.SetBackground(*wxWHITE_BRUSH);
    dc.Clear();

    //find position of accelerator
    int indexAccel = -1;
    size_t accelPos;
    wxString textLabelFormatted = text;
    if ((accelPos = text.find(wxT("&"))) != wxString::npos)
    {
        textLabelFormatted.Replace(wxT("&"), wxT(""), false); //remove accelerator
        indexAccel = accelPos;
    }

    dc.SetTextForeground(textColor);
    dc.SetTextBackground(*wxWHITE);
    dc.SetFont(currentFont);
    dc.DrawLabel(textLabelFormatted, wxNullBitmap, wxRect(0, 0, newBitmap.GetWidth(), newBitmap.GetHeight()), wxALIGN_LEFT | wxALIGN_CENTER_VERTICAL, indexAccel);

    dc.SelectObject(wxNullBitmap);

    //add alpha channel to image
    wxImage finalImage(newBitmap.ConvertToImage());
    finalImage.SetAlpha();

    //linearInterpolation(finalImage);

    //make white background transparent
    makeWhiteTransparent(textColor, finalImage);

    return wxBitmap(finalImage);
}


//copy one image into another, allowing arbitrary overlapping! (pos may contain negative numbers)
void writeToImage(const wxImage& source, const wxPoint pos, wxImage& target)
{
    //determine startpositions in source and target image, as well as width and height to be copied
    wxPoint posSrc, posTrg;
    int width, height;

    //X-axis
    if (pos.x < 0)
    {
        posSrc.x = -pos.x;
        posTrg.x = 0;
        width = std::min(pos.x + source.GetWidth(), target.GetWidth());
    }
    else
    {
        posSrc.x = 0;
        posTrg.x = pos.x;
        width = std::min(target.GetWidth() - pos.x, source.GetWidth());
    }

    //Y-axis
    if (pos.y < 0)
    {
        posSrc.y = -pos.y;
        posTrg.y = 0;
        height = std::min(pos.y + source.GetHeight(), target.GetHeight());
    }
    else
    {
        posSrc.y = 0;
        posTrg.y = pos.y;
        height = std::min(target.GetHeight() - pos.y, source.GetHeight());
    }


    if (width > 0 && height > 0)
    {
        {
            //copy source to target respecting overlapping parts
            const unsigned char*       sourcePtr    = source.GetData() + 3 * (posSrc.x + posSrc.y * source.GetWidth());
            const unsigned char* const sourcePtrEnd = source.GetData() + 3 * (posSrc.x + (posSrc.y + height) * source.GetWidth());
            unsigned char*             targetPtr    = target.GetData() + 3 * (posTrg.x + posTrg.y * target.GetWidth());

            while (sourcePtr < sourcePtrEnd)
            {
                memcpy(targetPtr, sourcePtr, 3 * width);
                sourcePtr += 3 * source.GetWidth();
                targetPtr += 3 * target.GetWidth();
            }
        }

        //handle different cases concerning alpha channel
        if (source.HasAlpha())
        {
            if (!target.HasAlpha())
            {
                target.SetAlpha();
                unsigned char* alpha = target.GetAlpha();
                memset(alpha, wxIMAGE_ALPHA_OPAQUE, target.GetWidth() * target.GetHeight());
            }

            //copy alpha channel
            const unsigned char*       sourcePtr    = source.GetAlpha() + (posSrc.x + posSrc.y * source.GetWidth());
            const unsigned char* const sourcePtrEnd = source.GetAlpha() + (posSrc.x + (posSrc.y + height) * source.GetWidth());
            unsigned char*             targetPtr    = target.GetAlpha() + (posTrg.x + posTrg.y * target.GetWidth());

            while (sourcePtr < sourcePtrEnd)
            {
                memcpy(targetPtr, sourcePtr, width);
                sourcePtr += source.GetWidth();
                targetPtr += target.GetWidth();
            }
        }
        else if (target.HasAlpha())
        {
            unsigned char*             targetPtr    = target.GetAlpha() + (posTrg.x + posTrg.y * target.GetWidth());
            const unsigned char* const targetPtrEnd = target.GetAlpha() + (posTrg.x + (posTrg.y + height) * target.GetWidth());

            while (targetPtr < targetPtrEnd)
            {
                memset(targetPtr, wxIMAGE_ALPHA_OPAQUE, width);
                targetPtr += target.GetWidth();
            }
        }
    }
}


void wxButtonWithImage::refreshButtonLabel()
{
    wxBitmap bitmapText = createBitmapFromText(textLabel);

    //calculate dimensions of new button
    const int height = std::max(std::max(bitmapFront.GetHeight(), bitmapText.GetHeight()), bitmapBack.GetHeight());
    const int width  = bitmapFront.GetWidth() + m_spaceAfter + bitmapText.GetWidth() + m_spaceBefore + bitmapBack.GetWidth();

    //create a transparent image
    wxImage transparentImage(width, height, false);
    transparentImage.SetAlpha();
    unsigned char* alpha = transparentImage.GetAlpha();
    memset(alpha, wxIMAGE_ALPHA_TRANSPARENT, width * height);

    //wxDC::DrawLabel() unfortunately isn't working for transparent images on Linux, so we need to use custom image-concatenation
    if (bitmapFront.IsOk())
        writeToImage(wxImage(bitmapFront.ConvertToImage()),
                     wxPoint(0, (transparentImage.GetHeight() - bitmapFront.GetHeight()) / 2),
                     transparentImage);

    if (bitmapText.IsOk())
        writeToImage(wxImage(bitmapText.ConvertToImage()),
                     wxPoint(bitmapFront.GetWidth() + m_spaceAfter, (transparentImage.GetHeight() - bitmapText.GetHeight()) / 2),
                     transparentImage);

    if (bitmapBack.IsOk())
        writeToImage(wxImage(bitmapBack.ConvertToImage()),
                     wxPoint(bitmapFront.GetWidth() + m_spaceAfter + bitmapText.GetWidth() + m_spaceBefore, (transparentImage.GetHeight() - bitmapBack.GetHeight()) / 2),
                     transparentImage);

    //adjust button size
    wxSize minSize = GetMinSize();

    //SetMinSize() instead of SetSize() is needed here for wxWindows layout determination to work corretly
    wxBitmapButton::SetMinSize(wxSize(std::max(width + 10, minSize.GetWidth()), std::max(height + 5, minSize.GetHeight())));

    //finally set bitmap
    wxBitmapButton::SetBitmapLabel(wxBitmap(transparentImage));
}
