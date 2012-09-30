// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) Zenju (zenju AT gmx DOT de) - All Rights Reserved        *
// **************************************************************************

#include "msg_popup.h"
#include "../lib/resources.h"
#include <wx+/mouse_move_dlg.h>
#include "gui_generated.h"

using namespace zen;


class ErrorDlg : public MessageDlgGenerated
{
public:
    ErrorDlg(wxWindow* parent,
             int activeButtons,
             const wxString& messageText,
             const wxString& caption, //optional
             bool* ignoreNextErrors);

private:
    void OnClose (wxCloseEvent&   event) { EndModal(ReturnErrorDlg::BUTTON_CANCEL); }
    void OnCancel(wxCommandEvent& event) { EndModal(ReturnErrorDlg::BUTTON_CANCEL); }
    void OnButton1(wxCommandEvent& event);
    void OnButton2(wxCommandEvent& event);

    bool* ignoreErrors;
    wxButton& buttonIgnore;           //
    wxButton& buttonRetry;            // map generic controls
    wxCheckBox& checkBoxIgnoreErrors; //
};


ErrorDlg::ErrorDlg(wxWindow* parent, int activeButtons, const wxString& messageText, const wxString& caption, bool* ignoreNextErrors) :
    MessageDlgGenerated(parent),
    ignoreErrors(ignoreNextErrors),
    buttonIgnore(*m_buttonCustom1),
    buttonRetry (*m_buttonCustom2),
    checkBoxIgnoreErrors(*m_checkBoxCustom)
{
#ifdef FFS_WIN
    new zen::MouseMoveWindow(*this); //allow moving main dialog by clicking (nearly) anywhere...; ownership passed to "this"
#endif

    SetTitle(!caption.empty() ? caption : _("Error"));
    m_bitmapMsgType->SetBitmap(GlobalResources::getImage(L"msg_error"));
    m_textCtrlMessage->SetValue(messageText);
    checkBoxIgnoreErrors.SetLabel(_("Ignore further errors"));
    buttonIgnore.SetLabel(_("&Ignore"));
    buttonRetry .SetLabel(_("&Retry"));
    buttonIgnore.SetId(wxID_IGNORE);
    buttonRetry .SetId(wxID_RETRY);

    if (ignoreNextErrors)
        checkBoxIgnoreErrors.SetValue(*ignoreNextErrors);
    else
        checkBoxIgnoreErrors.Hide();

    if (~activeButtons & ReturnErrorDlg::BUTTON_IGNORE)
    {
        buttonIgnore.Hide();
        checkBoxIgnoreErrors.Hide();
    }

    if (~activeButtons & ReturnErrorDlg::BUTTON_RETRY)
        buttonRetry.Hide();

    if (~activeButtons & ReturnErrorDlg::BUTTON_CANCEL)
        m_buttonCancel->Hide();

    //set button focus precedence
    if (activeButtons & ReturnErrorDlg::BUTTON_RETRY)
        buttonRetry.SetFocus();
    else if (activeButtons & ReturnErrorDlg::BUTTON_IGNORE)
        buttonIgnore.SetFocus();
    else if (activeButtons & ReturnErrorDlg::BUTTON_CANCEL)
        m_buttonCancel->SetFocus();

    Fit(); //child-element widths have changed: image was set
}


void ErrorDlg::OnButton1(wxCommandEvent& event) //ignore
{
    if (ignoreErrors)
        *ignoreErrors = checkBoxIgnoreErrors.GetValue();
    EndModal(ReturnErrorDlg::BUTTON_IGNORE);
}


void ErrorDlg::OnButton2(wxCommandEvent& event) //retry
{
    if (ignoreErrors)
        *ignoreErrors = checkBoxIgnoreErrors.GetValue();
    EndModal(ReturnErrorDlg::BUTTON_RETRY);
}


ReturnErrorDlg::ButtonPressed zen::showErrorDlg(wxWindow* parent, int activeButtons, const wxString& messageText, bool* ignoreNextErrors)
{
    ErrorDlg errorDlg(parent, activeButtons, messageText, wxString(), ignoreNextErrors);
    errorDlg.Raise();
    return static_cast<ReturnErrorDlg::ButtonPressed>(errorDlg.ShowModal());
}

//########################################################################################

ReturnFatalErrorDlg::ButtonPressed zen::showFatalErrorDlg(wxWindow* parent, int activeButtons, const wxString& messageText, bool* ignoreNextErrors)
{
    ErrorDlg errorDlg(parent, activeButtons, messageText, _("Fatal Error"), ignoreNextErrors);
    errorDlg.Raise();
    return static_cast<ReturnFatalErrorDlg::ButtonPressed>(errorDlg.ShowModal());
}

//########################################################################################

class WarningDlg : public MessageDlgGenerated
{
public:
    WarningDlg(wxWindow* parent, int activeButtons, const wxString& messageText, bool& dontShowAgain);

private:
    void OnClose (wxCloseEvent&   event) { EndModal(ReturnWarningDlg::BUTTON_CANCEL); }
    void OnCancel(wxCommandEvent& event) { EndModal(ReturnWarningDlg::BUTTON_CANCEL); }
    void OnButton1(wxCommandEvent& event);
    void OnButton2(wxCommandEvent& event);

    bool& dontShowAgain;
    wxButton& buttonIgnore;            //
    wxButton& buttonSwitch;            // map generic controls
    wxCheckBox& checkBoxDontShowAgain; //
};


WarningDlg::WarningDlg(wxWindow* parent,  int activeButtons, const wxString& messageText, bool& dontShowDlgAgain) :
    MessageDlgGenerated(parent),
    dontShowAgain(dontShowDlgAgain),
    buttonIgnore(*m_buttonCustom1),
    buttonSwitch(*m_buttonCustom2),
    checkBoxDontShowAgain(*m_checkBoxCustom)

{
#ifdef FFS_WIN
    new zen::MouseMoveWindow(*this); //allow moving main dialog by clicking (nearly) anywhere...; ownership passed to "this"
#endif

    SetTitle(_("Warning"));
    m_bitmapMsgType->SetBitmap(GlobalResources::getImage(L"msg_warning"));
    m_textCtrlMessage->SetValue(messageText);
    checkBoxDontShowAgain.SetLabel(_("Don't show this dialog again"));
    buttonIgnore.SetLabel(_("&Ignore"));
    buttonSwitch.SetLabel(_("&Switch"));
    buttonIgnore.SetId(wxID_IGNORE);
    buttonSwitch.SetId(wxID_MORE);

    checkBoxDontShowAgain.SetValue(dontShowAgain);

    if (~activeButtons & ReturnWarningDlg::BUTTON_IGNORE)
    {
        buttonIgnore.Hide();
        checkBoxDontShowAgain.Hide();
    }

    if (~activeButtons & ReturnWarningDlg::BUTTON_SWITCH)
        buttonSwitch.Hide();

    if (~activeButtons & ReturnWarningDlg::BUTTON_CANCEL)
        m_buttonCancel->Hide();

    //set button focus precedence
    if (activeButtons & ReturnWarningDlg::BUTTON_IGNORE)
        buttonIgnore.SetFocus();
    else if (activeButtons & ReturnWarningDlg::BUTTON_CANCEL)
        m_buttonCancel->SetFocus();

    Fit(); //child-element widths have changed: image was set
}


void WarningDlg::OnButton1(wxCommandEvent& event) //ignore
{
    dontShowAgain = checkBoxDontShowAgain.GetValue();
    EndModal(ReturnWarningDlg::BUTTON_IGNORE);
}


void WarningDlg::OnButton2(wxCommandEvent& event) //switch
{
    dontShowAgain = checkBoxDontShowAgain.GetValue();
    EndModal(ReturnWarningDlg::BUTTON_SWITCH);
}


ReturnWarningDlg::ButtonPressed zen::showWarningDlg(wxWindow* parent, int activeButtons, const wxString& messageText, bool& dontShowAgain)
{
    WarningDlg warningDlg(parent, activeButtons, messageText, dontShowAgain);
    warningDlg.Raise();
    return static_cast<ReturnWarningDlg::ButtonPressed>(warningDlg.ShowModal());
}
//########################################################################################


class QuestionDlg : public MessageDlgGenerated
{
public:
    QuestionDlg(wxWindow* parent, int activeButtons, const wxString& messageText, const wxString& caption, const wxString& labelYes, const wxString& labelNo, CheckBox* checkbox);

private:
    void OnClose (wxCloseEvent&   event) { EndModal(ReturnQuestionDlg::BUTTON_CANCEL); }
    void OnCancel(wxCommandEvent& event) { EndModal(ReturnQuestionDlg::BUTTON_CANCEL); }
    void OnButton1(wxCommandEvent& event);
    void OnButton2(wxCommandEvent& event);

    CheckBox* checkbox_; //optional
    wxButton& buttonYes; // map generic controls
    wxButton& buttonNo;  //
};


QuestionDlg::QuestionDlg(wxWindow* parent,
                         int activeButtons,
                         const wxString& messageText,
                         const wxString& caption, //optional
                         const wxString& labelYes,
                         const wxString& labelNo,
                         CheckBox* checkbox) :
    MessageDlgGenerated(parent),
    checkbox_(checkbox),
    buttonYes(*m_buttonCustom1),
    buttonNo (*m_buttonCustom2)
{
#ifdef FFS_WIN
    new zen::MouseMoveWindow(*this); //allow moving main dialog by clicking (nearly) anywhere...; ownership passed to "this"
#endif

    SetTitle(!caption.empty()? caption : _("Question"));
    m_bitmapMsgType->SetBitmap(GlobalResources::getImage(L"msg_question"));
    m_textCtrlMessage->SetValue(messageText);
    buttonYes.SetLabel(!labelYes.empty() ? labelYes : _("&Yes"));
    buttonNo .SetLabel(!labelNo .empty() ? labelNo  : _("&No"));
    buttonYes.SetId(wxID_YES);
    buttonNo .SetId(wxID_NO);

    if (checkbox_)
    {
        m_checkBoxCustom->SetValue(checkbox_->value_);
        m_checkBoxCustom->SetLabel(checkbox_->label_);
    }
    else
        m_checkBoxCustom->Hide();

    if (~activeButtons & ReturnQuestionDlg::BUTTON_YES)
        buttonYes.Hide();

    if (~activeButtons & ReturnQuestionDlg::BUTTON_NO)
        buttonNo.Hide();

    if (~activeButtons & ReturnQuestionDlg::BUTTON_CANCEL)
        m_buttonCancel->Hide();

    //set button focus precedence
    if (activeButtons & ReturnQuestionDlg::BUTTON_YES)
        buttonYes.SetFocus();
    else if (activeButtons & ReturnQuestionDlg::BUTTON_CANCEL)
        m_buttonCancel->SetFocus();
    else if (activeButtons & ReturnQuestionDlg::BUTTON_NO)
        buttonNo.SetFocus();

    Fit(); //child-element widths have changed: image was set
}

void QuestionDlg::OnButton1(wxCommandEvent& event) //yes
{
    if (checkbox_)
        checkbox_->value_ = m_checkBoxCustom->GetValue();
    EndModal(ReturnQuestionDlg::BUTTON_YES);
}

void QuestionDlg::OnButton2(wxCommandEvent& event) //no
{
    if (checkbox_)
        checkbox_->value_ = m_checkBoxCustom->GetValue();
    EndModal(ReturnQuestionDlg::BUTTON_NO);
}


ReturnQuestionDlg::ButtonPressed zen::showQuestionDlg(wxWindow* parent,
                                                      int activeButtons,
                                                      const wxString& messageText,
                                                      const wxString& caption, //optional
                                                      const wxString& labelYes, const wxString& labelNo,
                                                      CheckBox* checkbox)
{
    QuestionDlg qtnDlg(parent, activeButtons, messageText, caption, labelYes, labelNo, checkbox);
    qtnDlg.Raise();
    return static_cast<ReturnQuestionDlg::ButtonPressed>(qtnDlg.ShowModal());
}
