// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) ZenJu (zhnmju123 AT gmx DOT de) - All Rights Reserved    *
// **************************************************************************

#ifndef NO_FLICKER_HEADER_893421590321532
#define NO_FLICKER_HEADER_893421590321532

#include <wx/window.h>

namespace zen
{
inline
void setText(wxTextCtrl& control, const wxString& newText, bool* additionalLayoutChange = nullptr)
{
    if (additionalLayoutChange && !*additionalLayoutChange) //never revert from true to false!
        *additionalLayoutChange = control.GetValue().length() != newText.length(); //avoid screen flicker: update layout only when necessary

    if (control.GetValue() != newText)
        control.ChangeValue(newText);
}

inline
void setText(wxStaticText& control, const wxString& newText, bool* additionalLayoutChange = nullptr)
{
    if (additionalLayoutChange && !*additionalLayoutChange)
        *additionalLayoutChange = control.GetLabel().length() != newText.length(); //avoid screen flicker: update layout only when necessary

    if (control.GetLabel() != newText)
        control.SetLabel(newText);
}
}

#endif //NO_FLICKER_HEADER_893421590321532
