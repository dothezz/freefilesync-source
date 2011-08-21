// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) 2008-2011 ZenJu (zhnmju123 AT gmx.de)                    *
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
ReturnErrorDlg::ButtonPressed showErrorDlg(int activeButtons, const wxString& messageText, bool& ignoreNextErrors);


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
ReturnQuestionDlg::ButtonPressed showQuestionDlg(int activeButtons, const wxString& messageText, bool* dontShowAgain = NULL);
}


#endif // MESSAGEPOPUP_H_INCLUDED
