// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) ZenJu (zhnmju123 AT gmx DOT de) - All Rights Reserved    *
// **************************************************************************

#ifndef MESSAGEPOPUP_H_INCLUDED
#define MESSAGEPOPUP_H_INCLUDED

#include <wx/string.h>

namespace zen
{
struct ReturnErrorDlg
{
    enum ButtonPressed
    {
        BUTTON_IGNORE = 1,
        BUTTON_RETRY  = 2,
        BUTTON_ABORT  = 4
    };
};
ReturnErrorDlg::ButtonPressed showErrorDlg(int activeButtons, const wxString& messageText, bool* ignoreNextErrors); //ignoreNextErrors may be nullptr


struct ReturnWarningDlg
{
    enum ButtonPressed
    {
        BUTTON_IGNORE  = 1,
        BUTTON_SWITCH  = 2,
        BUTTON_ABORT   = 4
    };
};
ReturnWarningDlg::ButtonPressed showWarningDlg(int activeButtons, const wxString& messageText, bool& dontShowAgain);


struct ReturnQuestionDlg
{
    enum ButtonPressed
    {
        BUTTON_YES    = 1,
        BUTTON_NO     = 2,
        BUTTON_CANCEL = 4
    };
};

struct CheckBox
{
    CheckBox(const wxString& label, bool& value) : label_(label), value_(value) {}

    wxString label_; //in
    bool& value_;    //in/out
};

ReturnQuestionDlg::ButtonPressed showQuestionDlg(int activeButtons, const wxString& messageText, CheckBox* checkbox = nullptr);
}


#endif // MESSAGEPOPUP_H_INCLUDED
