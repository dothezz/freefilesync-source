// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) 2008-2010 ZenJu (zhnmju123 AT gmx.de)                    *
// **************************************************************************
//
#include "msg_popup.h"
#include "../library/resources.h"
#include "mouse_move_dlg.h"


ErrorDlg::ErrorDlg(wxWindow* parentWindow, const int activeButtons, const wxString messageText, bool& ignoreNextErrors) :
    ErrorDlgGenerated(parentWindow),
    ignoreErrors(ignoreNextErrors)
{
#ifdef FFS_WIN
    new ffs3::MouseMoveWindow(*this, //allow moving main dialog by clicking (nearly) anywhere...
                        this, m_bitmap10); //ownership passed to "this"
#endif

    m_bitmap10->SetBitmap(GlobalResources::getInstance().getImageByName(wxT("error")));
    m_textCtrl8->SetValue(messageText);
    m_checkBoxIgnoreErrors->SetValue(ignoreNextErrors);

    if (~activeButtons & BUTTON_IGNORE)
    {
        m_buttonIgnore->Hide();
        m_checkBoxIgnoreErrors->Hide();
    }

    if (~activeButtons & BUTTON_RETRY)
        m_buttonRetry->Hide();

    if (~activeButtons & BUTTON_ABORT)
        m_buttonAbort->Hide();

    //set button focus precedence
    if (activeButtons & BUTTON_RETRY)
        m_buttonRetry->SetFocus();
    else if (activeButtons & BUTTON_IGNORE)
        m_buttonIgnore->SetFocus();
    else if (activeButtons & BUTTON_ABORT)
        m_buttonAbort->SetFocus();
}


void ErrorDlg::OnClose(wxCloseEvent& event)
{
    ignoreErrors = m_checkBoxIgnoreErrors->GetValue();
    EndModal(BUTTON_ABORT);
}


void ErrorDlg::OnIgnore(wxCommandEvent& event)
{
    ignoreErrors = m_checkBoxIgnoreErrors->GetValue();
    EndModal(BUTTON_IGNORE);
}


void ErrorDlg::OnRetry(wxCommandEvent& event)
{
    ignoreErrors = m_checkBoxIgnoreErrors->GetValue();
    EndModal(BUTTON_RETRY);
}


void ErrorDlg::OnAbort(wxCommandEvent& event)
{
    ignoreErrors = m_checkBoxIgnoreErrors->GetValue();
    EndModal(BUTTON_ABORT);
}
//########################################################################################


WarningDlg::WarningDlg(wxWindow* parentWindow,  int activeButtons, const wxString messageText, bool& dontShowDlgAgain) :
    WarningDlgGenerated(parentWindow),
    dontShowAgain(dontShowDlgAgain)
{
#ifdef FFS_WIN
    new ffs3::MouseMoveWindow(*this, //allow moving main dialog by clicking (nearly) anywhere...
                        this, m_bitmap10); //ownership passed to "this"
#endif

    m_bitmap10->SetBitmap(GlobalResources::getInstance().getImageByName(wxT("warning")));
    m_textCtrl8->SetValue(messageText);
    m_checkBoxDontShowAgain->SetValue(dontShowAgain);

    if (~activeButtons & BUTTON_IGNORE)
    {
        m_buttonIgnore->Hide();
        m_checkBoxDontShowAgain->Hide();
    }

    if (~activeButtons & BUTTON_SWITCH)
        m_buttonSwitch->Hide();

    if (~activeButtons & BUTTON_ABORT)
        m_buttonAbort->Hide();

    //set button focus precedence
    if (activeButtons & BUTTON_IGNORE)
        m_buttonIgnore->SetFocus();
    else if (activeButtons & BUTTON_ABORT)
        m_buttonAbort->SetFocus();
}


void WarningDlg::OnClose(wxCloseEvent& event)
{
    dontShowAgain = m_checkBoxDontShowAgain->GetValue();
    EndModal(BUTTON_ABORT);
}


void WarningDlg::OnIgnore(wxCommandEvent& event)
{
    dontShowAgain = m_checkBoxDontShowAgain->GetValue();
    EndModal(BUTTON_IGNORE);
}


void WarningDlg::OnSwitch(wxCommandEvent& event)
{
    dontShowAgain = m_checkBoxDontShowAgain->GetValue();
    EndModal(BUTTON_SWITCH);
}


void WarningDlg::OnAbort(wxCommandEvent& event)
{
    dontShowAgain = m_checkBoxDontShowAgain->GetValue();
    EndModal(BUTTON_ABORT);
}
//########################################################################################


QuestionDlg::QuestionDlg(wxWindow* parentWindow, int activeButtons, const wxString messageText, bool* dontShowDlgAgain) :
    QuestionDlgGenerated(parentWindow),
    dontShowAgain(dontShowDlgAgain)
{
#ifdef FFS_WIN
    new ffs3::MouseMoveWindow(*this, //allow moving main dialog by clicking (nearly) anywhere...
                        this, m_bitmap10); //ownership passed to "this"
#endif

    m_bitmap10->SetBitmap(GlobalResources::getInstance().getImageByName(wxT("question")));
    m_textCtrl8->SetValue(messageText);
    if (dontShowAgain)
        m_checkBoxDontAskAgain->SetValue(*dontShowAgain);
    else
        m_checkBoxDontAskAgain->Hide();

    if (~activeButtons & BUTTON_YES)
        m_buttonYes->Hide();

    if (~activeButtons & BUTTON_NO)
    {
        m_buttonNo->Hide();
        m_checkBoxDontAskAgain->Hide();
    }

    if (~activeButtons & BUTTON_CANCEL)
        m_buttonCancel->Hide();

    //set button focus precedence
    if (activeButtons & BUTTON_YES)
        m_buttonYes->SetFocus();
    else if (activeButtons & BUTTON_CANCEL)
        m_buttonCancel->SetFocus();
    else if (activeButtons & BUTTON_NO)
        m_buttonNo->SetFocus();
}


void QuestionDlg::OnClose(wxCloseEvent& event)
{
    if (dontShowAgain)
        *dontShowAgain = m_checkBoxDontAskAgain->GetValue();
    EndModal(BUTTON_CANCEL);
}


void QuestionDlg::OnCancel(wxCommandEvent& event)
{
    if (dontShowAgain)
        *dontShowAgain = m_checkBoxDontAskAgain->GetValue();
    EndModal(BUTTON_CANCEL);
}


void QuestionDlg::OnYes(wxCommandEvent& event)
{
    if (dontShowAgain)
        *dontShowAgain = m_checkBoxDontAskAgain->GetValue();
    EndModal(BUTTON_YES);
}

void QuestionDlg::OnNo(wxCommandEvent& event)
{
    if (dontShowAgain)
        *dontShowAgain = m_checkBoxDontAskAgain->GetValue();
    EndModal(BUTTON_NO);
}
