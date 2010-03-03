// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) 2008-2010 ZenJu (zhnmju123 AT gmx.de)                    *
// **************************************************************************
//
#ifndef MESSAGEPOPUP_H_INCLUDED
#define MESSAGEPOPUP_H_INCLUDED

#include "guiGenerated.h"


class ErrorDlg : public ErrorDlgGenerated
{
public:
    ErrorDlg(wxWindow* parentWindow, const int activeButtons, const wxString messageText, bool& ignoreNextErrors);

    enum ReturnCodes
    {
        BUTTON_IGNORE = 1,
        BUTTON_RETRY  = 2,
        BUTTON_ABORT  = 4
    };

private:
    void OnClose(wxCloseEvent& event);
    void OnIgnore(wxCommandEvent& event);
    void OnRetry(wxCommandEvent& event);
    void OnAbort(wxCommandEvent& event);

    bool& ignoreErrors;
};


class WarningDlg : public WarningDlgGenerated
{
public:
    WarningDlg(wxWindow* parentWindow, int activeButtons, const wxString messageText, bool& dontShowAgain);
    ~WarningDlg();

    enum Response
    {
        BUTTON_IGNORE  = 1,
        BUTTON_ABORT   = 2
    };

private:
    void OnClose(wxCloseEvent& event);
    void OnIgnore(wxCommandEvent& event);
    void OnResolve(wxCommandEvent& event);
    void OnAbort(wxCommandEvent& event);
    void OnOkay(wxCommandEvent& event);

    bool& dontShowAgain;
};


class QuestionDlg : public QuestionDlgGenerated
{
public:
    QuestionDlg(wxWindow* parentWindow, int activeButtons, const wxString messageText, bool* dontShowAgain = NULL);

    enum
    {
        BUTTON_YES    = 1,
        BUTTON_NO     = 2,
        BUTTON_CANCEL = 4
    };

private:
    void OnClose(wxCloseEvent& event);
    void OnCancel(wxCommandEvent& event);
    void OnYes(wxCommandEvent& event);
    void OnNo(wxCommandEvent& event);

    bool* dontShowAgain; //optional
};


#endif // MESSAGEPOPUP_H_INCLUDED
