// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) Zenju (zenju AT gmx DOT de) - All Rights Reserved        *
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
        BUTTON_RETRY  = 1,
        BUTTON_IGNORE = 2,
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

class QuestConfig;
ReturnQuestionDlg::ButtonPressed showQuestionDlg(wxWindow* parent, int activeButtons, const wxString& messageText, const QuestConfig& cfg);

class QuestConfig
{
public:
    QuestConfig() : checkBoxValue() {}
    QuestConfig& setCaption (const wxString& label) { caption  = label; return *this; }
    QuestConfig& setLabelYes(const wxString& label) { labelYes = label; return *this; }
    QuestConfig& setLabelNo (const wxString& label) { labelNo  = label; return *this; }
    QuestConfig& showCheckBox(bool& value, const wxString& label) { checkBoxLabel = label; checkBoxValue = &value; return *this; }

private:
    friend ReturnQuestionDlg::ButtonPressed showQuestionDlg(wxWindow* parent, int activeButtons, const wxString& messageText, const QuestConfig& cfg);

    wxString caption;
    wxString labelYes; //overwrite default "Yes, No" labels
    wxString labelNo;  //
    //optional checkbox:
    bool* checkBoxValue;    //in/out
    wxString checkBoxLabel; //in
};

ReturnQuestionDlg::ButtonPressed showQuestionDlg(wxWindow* parent, int activeButtons, const wxString& messageText, const QuestConfig& cfg = QuestConfig());
}

#endif // MESSAGEPOPUP_H_INCLUDED
