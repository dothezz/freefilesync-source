// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) Zenju (zenju AT gmx DOT de) - All Rights Reserved        *
// **************************************************************************

#include "msg_popup.h"
#include <wx+/mouse_move_dlg.h>
#include <wx+/std_button_order.h>
#include "gui_generated.h"
#include "../lib/resources.h"

using namespace zen;

namespace
{
void setAsStandard(wxButton& btn)
{
    btn.SetDefault();
    btn.SetFocus();
}
}


class ErrorDlg : public MessageDlgGenerated
{
public:
    ErrorDlg(wxWindow* parent,
             int activeButtons,
             const wxString& messageText,
             const wxString& caption, //optional
             bool* ignoreNextErrors);

private:
    virtual void OnClose (wxCloseEvent&   event) { EndModal(ReturnErrorDlg::BUTTON_CANCEL); }
    virtual void OnCancel(wxCommandEvent& event) { EndModal(ReturnErrorDlg::BUTTON_CANCEL); }
    virtual void OnButtonAffirmative(wxCommandEvent& event);
    virtual void OnButtonNegative   (wxCommandEvent& event);
    virtual void OnCheckBoxClick(wxCommandEvent& event) { updateGui(); event.Skip(); }
    void updateGui();

    bool* ignoreErrors;
    wxButton& buttonRetry;            //
    wxButton& buttonIgnore;           // map generic controls
    wxCheckBox& checkBoxIgnoreErrors; //
};


ErrorDlg::ErrorDlg(wxWindow* parent, int activeButtons, const wxString& messageText, const wxString& caption, bool* ignoreNextErrors) :
    MessageDlgGenerated(parent),
    ignoreErrors(ignoreNextErrors),
    buttonRetry (*m_buttonAffirmative),
    buttonIgnore(*m_buttonNegative),
    checkBoxIgnoreErrors(*m_checkBoxCustom)
{
#ifdef ZEN_WIN
    new zen::MouseMoveWindow(*this); //allow moving main dialog by clicking (nearly) anywhere...; ownership passed to "this"
#endif

    SetTitle(!caption.empty() ? caption : _("Error"));
    m_bitmapMsgType->SetBitmap(getResourceImage(L"msg_error"));
    m_textCtrlMessage->SetValue(messageText);
    checkBoxIgnoreErrors.SetLabel(_("&Ignore subsequent errors"));
    buttonIgnore.SetLabel(_("&Ignore"));
    buttonRetry .SetLabel(_("&Retry"));
    //buttonIgnore.SetId(wxID_IGNORE); -> setting id after button creation breaks "mouse snap to" functionality
    //buttonRetry .SetId(wxID_RETRY);  -> also wxWidgets docs seem to hide some info: "Normally, the identifier should be provided on creation and should not be modified subsequently."

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
        setAsStandard(buttonRetry);
    else if (activeButtons & ReturnErrorDlg::BUTTON_IGNORE)
        setAsStandard(buttonIgnore);
    else if (activeButtons & ReturnErrorDlg::BUTTON_CANCEL)
        setAsStandard(*m_buttonCancel);

    //set std order after button visibility was set
    setStandardButtonOrder(*bSizerStdButtons, StdButtons().setAffirmative(&buttonRetry).setNegative(&buttonIgnore).setCancel(m_buttonCancel));

    updateGui();
    Fit(); //child-element widths have changed: image was set
}


void ErrorDlg::updateGui()
{
    //button doesn't make sense when checkbox is set!
    buttonRetry.Enable(!checkBoxIgnoreErrors.GetValue());
}


void ErrorDlg::OnButtonAffirmative(wxCommandEvent& event) //retry
{
    if (ignoreErrors)
        *ignoreErrors = checkBoxIgnoreErrors.GetValue();
    EndModal(ReturnErrorDlg::BUTTON_RETRY);
}


void ErrorDlg::OnButtonNegative(wxCommandEvent& event) //ignore
{
    if (ignoreErrors)
        *ignoreErrors = checkBoxIgnoreErrors.GetValue();
    EndModal(ReturnErrorDlg::BUTTON_IGNORE);
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
    virtual void OnClose (wxCloseEvent&   event) { EndModal(ReturnWarningDlg::BUTTON_CANCEL); }
    virtual void OnCancel(wxCommandEvent& event) { EndModal(ReturnWarningDlg::BUTTON_CANCEL); }
    virtual void OnButtonAffirmative(wxCommandEvent& event);
    virtual void OnButtonNegative   (wxCommandEvent& event);
    virtual void OnCheckBoxClick(wxCommandEvent& event) { updateGui(); event.Skip(); }
    void updateGui();

    bool& dontShowAgain;
    wxButton& buttonSwitch;            //
    wxButton& buttonIgnore;            //map generic controls
    wxCheckBox& checkBoxDontShowAgain; //
};


WarningDlg::WarningDlg(wxWindow* parent,  int activeButtons, const wxString& messageText, bool& dontShowDlgAgain) :
    MessageDlgGenerated(parent),
    dontShowAgain(dontShowDlgAgain),
    buttonSwitch(*m_buttonAffirmative),
    buttonIgnore(*m_buttonNegative),
    checkBoxDontShowAgain(*m_checkBoxCustom)
{
#ifdef ZEN_WIN
    new zen::MouseMoveWindow(*this); //allow moving main dialog by clicking (nearly) anywhere...; ownership passed to "this"
#endif

    SetTitle(_("Warning"));
    m_bitmapMsgType->SetBitmap(getResourceImage(L"msg_warning"));
    m_textCtrlMessage->SetValue(messageText);
    checkBoxDontShowAgain.SetLabel(_("&Don't show this warning again"));
    buttonIgnore.SetLabel(_("&Ignore"));
    buttonSwitch.SetLabel(_("&Switch"));
    //buttonIgnore.SetId(wxID_IGNORE); -> see comment in ErrorDlg
    //buttonSwitch.SetId(wxID_MORE);

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
        setAsStandard(buttonIgnore);
    else if (activeButtons & ReturnWarningDlg::BUTTON_CANCEL)
        setAsStandard(*m_buttonCancel);

    //set std order after button visibility was set
    setStandardButtonOrder(*bSizerStdButtons, StdButtons().setAffirmative(&buttonSwitch).setNegative(&buttonIgnore).setCancel(m_buttonCancel));

    updateGui();
    Fit(); //child-element widths have changed: image was set
}


void WarningDlg::updateGui()
{
    //button doesn't make sense when checkbox is set!
    buttonSwitch.Enable(!checkBoxDontShowAgain.GetValue());
}


void WarningDlg::OnButtonAffirmative(wxCommandEvent& event) //switch
{
    dontShowAgain = checkBoxDontShowAgain.GetValue();
    EndModal(ReturnWarningDlg::BUTTON_SWITCH);
}


void WarningDlg::OnButtonNegative(wxCommandEvent& event) //ignore
{
    dontShowAgain = checkBoxDontShowAgain.GetValue();
    EndModal(ReturnWarningDlg::BUTTON_IGNORE);
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
    QuestionDlg(wxWindow* parent, int activeButtons, const wxString& messageText, const QuestConfig& cfg);

private:
    virtual void OnClose (wxCloseEvent&   event) { EndModal(ReturnQuestionDlg::BUTTON_CANCEL); }
    virtual void OnCancel(wxCommandEvent& event) { EndModal(ReturnQuestionDlg::BUTTON_CANCEL); }
    virtual void OnButtonAffirmative(wxCommandEvent& event);
    virtual void OnButtonNegative   (wxCommandEvent& event);
    virtual void OnCheckBoxClick(wxCommandEvent& event) { updateGui(); event.Skip(); }
    void updateGui();

    wxButton& buttonYes; // map generic controls
    wxButton& buttonNo;  //

    bool* const checkBoxValue_; //optional
    const int disabledButtonsWhenChecked_;
};


QuestionDlg::QuestionDlg(wxWindow* parent,
                         int activeButtons,
                         const wxString& messageText,
                         const QuestConfig& cfg) :
    MessageDlgGenerated(parent),
    buttonYes(*m_buttonAffirmative),
    buttonNo (*m_buttonNegative),
    checkBoxValue_(cfg.checkBoxValue),
    disabledButtonsWhenChecked_(cfg.disabledButtonsWhenChecked_)
{
#ifdef ZEN_WIN
    new zen::MouseMoveWindow(*this); //allow moving main dialog by clicking (nearly) anywhere...; ownership passed to "this"
#endif
    assert(!cfg.caption.empty()); //"Question" is not a good caption!
    SetTitle(!cfg.caption.empty()? cfg.caption : _("Question"));
    m_bitmapMsgType->SetBitmap(getResourceImage(L"msg_question"));
    m_textCtrlMessage->SetValue(messageText);
    buttonYes.SetLabel(!cfg.labelYes.empty() ? cfg.labelYes : _("&Yes"));
    buttonNo .SetLabel(!cfg.labelNo .empty() ? cfg.labelNo  : _("&No"));
    //buttonYes.SetId(wxID_YES); -> see comment in ErrorDlg
    //buttonNo .SetId(wxID_NO);

    if (cfg.checkBoxValue)
    {
        m_checkBoxCustom->SetValue(*cfg.checkBoxValue);
        m_checkBoxCustom->SetLabel(cfg.checkBoxLabel);
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
        setAsStandard(buttonYes);
    else if (activeButtons & ReturnQuestionDlg::BUTTON_NO)
        setAsStandard(buttonNo);
    else if (activeButtons & ReturnQuestionDlg::BUTTON_CANCEL)
        setAsStandard(*m_buttonCancel);

    //set std order after button visibility was set
    setStandardButtonOrder(*bSizerStdButtons, StdButtons().setAffirmative(&buttonYes).setNegative(&buttonNo).setCancel(m_buttonCancel));

    updateGui();
    Fit(); //child-element widths have changed: image was set
}


void QuestionDlg::updateGui()
{
    auto updateEnabledStatus =  [&](wxButton& btn, ReturnQuestionDlg::ButtonPressed btnId)
    {
        if (disabledButtonsWhenChecked_ & btnId)
            btn.Enable(!m_checkBoxCustom->GetValue());
    };
    updateEnabledStatus(buttonYes,       ReturnQuestionDlg::BUTTON_YES);
    updateEnabledStatus(buttonNo,        ReturnQuestionDlg::BUTTON_NO);
    updateEnabledStatus(*m_buttonCancel, ReturnQuestionDlg::BUTTON_CANCEL);
}


void QuestionDlg::OnButtonAffirmative(wxCommandEvent& event) //yes
{
    if (checkBoxValue_)
        *checkBoxValue_ = m_checkBoxCustom->GetValue();
    EndModal(ReturnQuestionDlg::BUTTON_YES);
}


void QuestionDlg::OnButtonNegative(wxCommandEvent& event) //no
{
    if (checkBoxValue_)
        *checkBoxValue_ = m_checkBoxCustom->GetValue();
    EndModal(ReturnQuestionDlg::BUTTON_NO);
}


ReturnQuestionDlg::ButtonPressed zen::showQuestionDlg(wxWindow* parent,
                                                      int activeButtons,
                                                      const wxString& messageText,
                                                      const QuestConfig& cfg)
{
    QuestionDlg qtnDlg(parent, activeButtons, messageText, cfg);
    qtnDlg.Raise();
    return static_cast<ReturnQuestionDlg::ButtonPressed>(qtnDlg.ShowModal());
}
