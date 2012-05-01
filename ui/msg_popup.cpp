// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) ZenJu (zhnmju123 AT gmx DOT de) - All Rights Reserved    *
// **************************************************************************

#include "msg_popup.h"
#include "../lib/resources.h"
#include <wx+/mouse_move_dlg.h>
#include "gui_generated.h"


using namespace zen;


class ErrorDlg : public ErrorDlgGenerated
{
public:
    ErrorDlg(wxWindow* parent,
             int activeButtons,
             const wxString& messageText,
             bool* ignoreNextErrors);

private:
    void OnClose(wxCloseEvent& event);
    void OnIgnore(wxCommandEvent& event);
    void OnRetry(wxCommandEvent& event);
    void OnAbort(wxCommandEvent& event);

    bool* ignoreErrors;
};


ErrorDlg::ErrorDlg(wxWindow* parent, const int activeButtons, const wxString& messageText, bool* ignoreNextErrors) :
    ErrorDlgGenerated(parent),
    ignoreErrors(ignoreNextErrors)
{
#ifdef FFS_WIN
    new zen::MouseMoveWindow(*this); //allow moving main dialog by clicking (nearly) anywhere...; ownership passed to "this"
#endif

    m_bitmap10->SetBitmap(GlobalResources::getImage(L"error"));
    m_textCtrl8->SetValue(messageText);

    if (ignoreNextErrors)
        m_checkBoxIgnoreErrors->SetValue(*ignoreNextErrors);
    else
        m_checkBoxIgnoreErrors->Hide();

    if (~activeButtons & ReturnErrorDlg::BUTTON_IGNORE)
    {
        m_buttonIgnore->Hide();
        m_checkBoxIgnoreErrors->Hide();
    }

    if (~activeButtons & ReturnErrorDlg::BUTTON_RETRY)
        m_buttonRetry->Hide();

    if (~activeButtons & ReturnErrorDlg::BUTTON_ABORT)
        m_buttonAbort->Hide();

    //set button focus precedence
    if (activeButtons & ReturnErrorDlg::BUTTON_RETRY)
        m_buttonRetry->SetFocus();
    else if (activeButtons & ReturnErrorDlg::BUTTON_IGNORE)
        m_buttonIgnore->SetFocus();
    else if (activeButtons & ReturnErrorDlg::BUTTON_ABORT)
        m_buttonAbort->SetFocus();
}


void ErrorDlg::OnClose(wxCloseEvent& event)
{
    EndModal(ReturnErrorDlg::BUTTON_ABORT);
}


void ErrorDlg::OnIgnore(wxCommandEvent& event)
{
    if (ignoreErrors) *ignoreErrors = m_checkBoxIgnoreErrors->GetValue();
    EndModal(ReturnErrorDlg::BUTTON_IGNORE);
}


void ErrorDlg::OnRetry(wxCommandEvent& event)
{
    if (ignoreErrors) *ignoreErrors = m_checkBoxIgnoreErrors->GetValue();
    EndModal(ReturnErrorDlg::BUTTON_RETRY);
}


void ErrorDlg::OnAbort(wxCommandEvent& event)
{
    if (ignoreErrors) *ignoreErrors = m_checkBoxIgnoreErrors->GetValue();
    EndModal(ReturnErrorDlg::BUTTON_ABORT);
}


ReturnErrorDlg::ButtonPressed zen::showErrorDlg(wxWindow* parent, int activeButtons, const wxString& messageText, bool* ignoreNextErrors)
{
    ErrorDlg errorDlg(parent, activeButtons, messageText, ignoreNextErrors);
    errorDlg.Raise();
    return static_cast<ReturnErrorDlg::ButtonPressed>(errorDlg.ShowModal());
}
//########################################################################################


class WarningDlg : public WarningDlgGenerated
{
public:
    WarningDlg(wxWindow* parent, int activeButtons, const wxString& messageText, bool& dontShowAgain);

private:
    void OnClose(wxCloseEvent& event);
    void OnIgnore(wxCommandEvent& event);
    void OnSwitch(wxCommandEvent& event);
    void OnAbort(wxCommandEvent& event);
    bool& dontShowAgain;
};


WarningDlg::WarningDlg(wxWindow* parent,  int activeButtons, const wxString& messageText, bool& dontShowDlgAgain) :
    WarningDlgGenerated(parent),
    dontShowAgain(dontShowDlgAgain)
{
#ifdef FFS_WIN
    new zen::MouseMoveWindow(*this); //allow moving main dialog by clicking (nearly) anywhere...; ownership passed to "this"
#endif

    m_bitmap10->SetBitmap(GlobalResources::getImage(L"warning"));
    m_textCtrl8->SetValue(messageText);
    m_checkBoxDontShowAgain->SetValue(dontShowAgain);

    if (~activeButtons & ReturnWarningDlg::BUTTON_IGNORE)
    {
        m_buttonIgnore->Hide();
        m_checkBoxDontShowAgain->Hide();
    }

    if (~activeButtons & ReturnWarningDlg::BUTTON_SWITCH)
        m_buttonSwitch->Hide();

    if (~activeButtons & ReturnWarningDlg::BUTTON_ABORT)
        m_buttonAbort->Hide();

    //set button focus precedence
    if (activeButtons & ReturnWarningDlg::BUTTON_IGNORE)
        m_buttonIgnore->SetFocus();
    else if (activeButtons & ReturnWarningDlg::BUTTON_ABORT)
        m_buttonAbort->SetFocus();
}


void WarningDlg::OnClose(wxCloseEvent& event)
{
    EndModal(ReturnWarningDlg::BUTTON_ABORT);
}


void WarningDlg::OnIgnore(wxCommandEvent& event)
{
    dontShowAgain = m_checkBoxDontShowAgain->GetValue();
    EndModal(ReturnWarningDlg::BUTTON_IGNORE);
}


void WarningDlg::OnSwitch(wxCommandEvent& event)
{
    dontShowAgain = m_checkBoxDontShowAgain->GetValue();
    EndModal(ReturnWarningDlg::BUTTON_SWITCH);
}


void WarningDlg::OnAbort(wxCommandEvent& event)
{
    dontShowAgain = m_checkBoxDontShowAgain->GetValue();
    EndModal(ReturnWarningDlg::BUTTON_ABORT);
}


ReturnWarningDlg::ButtonPressed zen::showWarningDlg(wxWindow* parent, int activeButtons, const wxString& messageText, bool& dontShowAgain)
{
    WarningDlg warningDlg(parent, activeButtons, messageText, dontShowAgain);
    warningDlg.Raise();
    return static_cast<ReturnWarningDlg::ButtonPressed>(warningDlg.ShowModal());
}
//########################################################################################


class QuestionDlg : public QuestionDlgGenerated
{
public:
    QuestionDlg(wxWindow* parent, int activeButtons, const wxString& messageText, CheckBox* checkbox);

private:
    void OnClose (wxCloseEvent&   event) { EndModal(ReturnQuestionDlg::BUTTON_CANCEL); }
    void OnCancel(wxCommandEvent& event) { EndModal(ReturnQuestionDlg::BUTTON_CANCEL); }
    void OnYes(wxCommandEvent& event);
    void OnNo (wxCommandEvent& event);

    CheckBox* checkbox_; //optional
};


QuestionDlg::QuestionDlg(wxWindow* parent, int activeButtons, const wxString& messageText, CheckBox* checkbox) :
    QuestionDlgGenerated(parent),
    checkbox_(checkbox)
{
#ifdef FFS_WIN
    new zen::MouseMoveWindow(*this); //allow moving main dialog by clicking (nearly) anywhere...; ownership passed to "this"
#endif

    m_bitmap10->SetBitmap(GlobalResources::getImage(L"question"));
    m_textCtrl8->SetValue(messageText);

    if (checkbox_)
    {
        m_checkBox->SetValue(checkbox_->value_);
        m_checkBox->SetLabel(checkbox_->label_);
    }
    else
        m_checkBox->Hide();

    if (~activeButtons & ReturnQuestionDlg::BUTTON_YES)
        m_buttonYes->Hide();

    if (~activeButtons & ReturnQuestionDlg::BUTTON_NO)
        m_buttonNo->Hide();

    if (~activeButtons & ReturnQuestionDlg::BUTTON_CANCEL)
        m_buttonCancel->Hide();

    //set button focus precedence
    if (activeButtons & ReturnQuestionDlg::BUTTON_YES)
        m_buttonYes->SetFocus();
    else if (activeButtons & ReturnQuestionDlg::BUTTON_CANCEL)
        m_buttonCancel->SetFocus();
    else if (activeButtons & ReturnQuestionDlg::BUTTON_NO)
        m_buttonNo->SetFocus();
}

void QuestionDlg::OnYes(wxCommandEvent& event)
{
    if (checkbox_)
        checkbox_->value_ = m_checkBox->GetValue();
    EndModal(ReturnQuestionDlg::BUTTON_YES);
}

void QuestionDlg::OnNo(wxCommandEvent& event)
{
    if (checkbox_)
        checkbox_->value_ = m_checkBox->GetValue();
    EndModal(ReturnQuestionDlg::BUTTON_NO);
}

ReturnQuestionDlg::ButtonPressed zen::showQuestionDlg(wxWindow* parent, int activeButtons, const wxString& messageText, CheckBox* checkbox)
{
    QuestionDlg qtnDlg(parent, activeButtons, messageText, checkbox);
    qtnDlg.Raise();
    return static_cast<ReturnQuestionDlg::ButtonPressed>(qtnDlg.ShowModal());
}
