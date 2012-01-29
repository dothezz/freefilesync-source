// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) ZenJu (zhnmju123 AT gmx DOT de) - All Rights Reserved    *
// **************************************************************************

#ifndef CUSTOMTOOLTIP_H_INCLUDED
#define CUSTOMTOOLTIP_H_INCLUDED

#include <wx/frame.h>

namespace zen
{
class Tooltip
{
public:
    Tooltip();
    ~Tooltip();

    void show(const wxString& text, wxPoint mousePos, const wxBitmap* bmp = NULL); //absolute screen coordinates
    void hide();

private:
    class PopupFrameGenerated;
    PopupFrameGenerated* tipWindow;
};
}

#endif // CUSTOMTOOLTIP_H_INCLUDED
