// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) Zenju (zenju AT gmx DOT de) - All Rights Reserved        *
// **************************************************************************

#ifndef FONT_SIZE_HEADER_23849632846734343234532
#define FONT_SIZE_HEADER_23849632846734343234532

#include <zen/basic_math.h>
#include <wx/window.h>

namespace zen
{
//set portable font size in multiples of the operating system's default font size
inline
void setRelativeFontSize(wxWindow& control, double factor)
{
    wxFont fnt = control.GetFont();
    fnt.SetPointSize(numeric::round(wxNORMAL_FONT->GetPointSize() * factor));
    control.SetFont(fnt);
};
}

#endif //FONT_SIZE_HEADER_23849632846734343234532
