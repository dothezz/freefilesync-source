// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) ZenJu (zenju AT gmx DOT de) - All Rights Reserved        *
// **************************************************************************

#ifndef MESSAGEPOPUP_H_INCLUDED
#define MESSAGEPOPUP_H_INCLUDED

#include <wx/window.h>
#include <wx/string.h>


namespace zen
{
//parent window, optional: support correct dialog placement above parent on multiple monitor systems

struct ReturnErrorDlg
{
    enum ButtonPressed
    {
        BUTTON_IGNORE = 1,
        BUTTON_RETRY  = 2,
        BUTTON_CANCEL = 4
    };
};
ReturnErrorDlg::ButtonPressed showErrorDlg(wxWindow* parent, int activeButtons, const wxString& messageText, bool* ignoreNextErrors); //ignoreNextErrors may be nullptr


struct ReturnFatalErrorDlg
{
    enum ButtonPressed
    {
        BUTTON_IGNORE = ReturnErrorDlg::BUTTON_IGNORE,
        BUTTON_CANCEL = ReturnErrorDlg::BUTTON_CANCEL
    };
};
ReturnFatalErrorDlg::ButtonPressed showFatalErrorDlg(wxWindow* parent, int activeButtons, const wxString& messageText, bool* ignoreNextErrors); //ignoreNextErrors may be nullptr


struct ReturnWarningDlg
{
    enum ButtonPressed
    {
        BUTTON_IGNORE  = 1,
        BUTTON_SWITCH  = 2,
        BUTTON_CANCEL  = 4
    };
};
ReturnWarningDlg::ButtonPressed showWarningDlg(wxWindow* parent, int activeButtons, const wxString& messageText, bool& dontShowAgain);


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

ReturnQuestionDlg::ButtonPressed showQuestionDlg(wxWindow* parent,
                                                 int activeButtons,
                                                 const wxString& messageText,
                                                 const wxString& caption  = wxString(),
                                                 const wxString& labelYes = wxString(), //overwrite default "Yes, No" labels
                                                 const wxString& labelNo  = wxString(), //
                                                 CheckBox* checkbox = nullptr);
}

#endif // MESSAGEPOPUP_H_INCLUDED
