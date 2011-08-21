// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) 2008-2011 ZenJu (zhnmju123 AT gmx.de)                    *
// **************************************************************************

#ifndef CUSTOMTOOLTIP_H_INCLUDED
#define CUSTOMTOOLTIP_H_INCLUDED

#include <wx/frame.h>

class CustomTooltip
{
public:
    CustomTooltip();
    ~CustomTooltip();

    void show(const wxString& text, wxPoint pos, const wxBitmap* bmp = NULL); //absolute screen coordinates
    void hide();

private:
    class PopupFrameGenerated;
    PopupFrameGenerated* tipWindow;
    const wxBitmap* lastBmp; //buffer last used bitmap pointer
};

#endif // CUSTOMTOOLTIP_H_INCLUDED
