#ifndef CUSTOMTOOLTIP_H_INCLUDED
#define CUSTOMTOOLTIP_H_INCLUDED

#include <wx/frame.h>

class PopupFrameGenerated;


class CustomTooltip
{
public:
    CustomTooltip();
    ~CustomTooltip();

    void show(const wxString& text, wxPoint pos, const wxBitmap* bmp = NULL); //absolute screen coordinates
    void hide();

private:
    PopupFrameGenerated* tipWindow;
    const wxBitmap* lastBmp; //buffer last used bitmap pointer
};

#endif // CUSTOMTOOLTIP_H_INCLUDED
