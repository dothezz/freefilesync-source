#include "SmallDialogs.h"

AboutDlg::AboutDlg(MainDialog* window) : AboutDlgGenerated(window), mainDialog(window)
{
    m_bitmap9->SetBitmap(*mainDialog->bitmapWebsite);
    m_bitmap10->SetBitmap(*mainDialog->bitmapEmail);

    m_animationControl1->SetAnimation(*mainDialog->animationMoney);
    m_animationControl1->Play();

    //build
    wxString build = wxString(_("(Build: ")) + __TDATE__ + ")";
    m_build->SetLabel(build);
}

AboutDlg::~AboutDlg() {}

void AboutDlg::OnClose(wxCloseEvent& event)
{
    Destroy();
}

void AboutDlg::OnOK(wxCommandEvent& event)
{
    Destroy();
}

//########################################################################################

HelpDlg::HelpDlg(MainDialog* window) : HelpDlgGenerated(window) {}

HelpDlg::~HelpDlg() {}

void HelpDlg::OnClose(wxCloseEvent& event)
{
    Destroy();
}
void HelpDlg::OnOK(wxCommandEvent& event)
{
    Destroy();
}

//########################################################################################


FilterDlg::FilterDlg(MainDialog* window) : FilterDlgGenerated(window), mainDialog(window)
{

    m_bitmap8->SetBitmap(*mainDialog->bitmapInclude);
    m_bitmap9->SetBitmap(*mainDialog->bitmapExclude);

    m_textCtrlInclude->SetValue(mainDialog->includeFilter);
    m_textCtrlExclude->SetValue(mainDialog->excludeFilter);
}

FilterDlg::~FilterDlg() {}

void FilterDlg::OnClose(wxCloseEvent& event)
{
    EndModal(0);
}

void FilterDlg::OnOK(wxCommandEvent& event)
{
    //only if user presses ApplyFilter, he wants the changes to be committed
    mainDialog->includeFilter = m_textCtrlInclude->GetValue();
    mainDialog->excludeFilter = m_textCtrlExclude->GetValue();

    //when leaving dialog: filter and redraw grid, if filter is active
    EndModal(OkayButtonPressed);
}


void FilterDlg::OnDefault(wxCommandEvent& event)
{
    m_textCtrlInclude->SetValue("*");
    m_textCtrlExclude->SetValue("");

    //changes to mainDialog are only committed when the OK button is pressed
    event.Skip();
}

//########################################################################################

SyncErrorDlg::SyncErrorDlg(MainDialog* window, wxString messageText, bool& suppressErrormessages)
        : SyncErrorDlgGenerated(window), mainDialog(window), suppressErrors(suppressErrormessages)
{
    m_bitmap10->SetBitmap(*mainDialog->bitmapWarning);
    m_textCtrl8->SetValue(messageText);
}

SyncErrorDlg::~SyncErrorDlg() {}

void SyncErrorDlg::OnClose(wxCloseEvent& event)
{
    //suppressErrors = m_checkBoxSuppress->GetValue(); -> not needed here
    EndModal(AbortButtonPressed);
}

void SyncErrorDlg::OnContinue(wxCommandEvent& event)
{
    suppressErrors = m_checkBoxSuppress->GetValue();
    EndModal(ContinueButtonPressed);
}

void SyncErrorDlg::OnAbort(wxCommandEvent& event)
{
    //suppressErrors = m_checkBoxSuppress->GetValue(); -> not needed here
    EndModal(AbortButtonPressed);
}

