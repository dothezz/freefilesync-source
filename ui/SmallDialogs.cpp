#include "SmallDialogs.h"
#include "..\library\globalFunctions.h"

AboutDlg::AboutDlg(MainDialog* window) : AboutDlgGenerated(window)
{
    m_bitmap9->SetBitmap(*GlobalResources::bitmapWebsite);
    m_bitmap10->SetBitmap(*GlobalResources::bitmapEmail);

    m_animationControl1->SetAnimation(*GlobalResources::animationMoney);
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

    m_bitmap8->SetBitmap(*GlobalResources::bitmapInclude);
    m_bitmap9->SetBitmap(*GlobalResources::bitmapExclude);

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


ErrorDlg::ErrorDlg(const wxString messageText, bool& suppressErrormessages) :
        ErrorDlgGenerated(0),
        suppressErrors(suppressErrormessages)
{
    m_bitmap10->SetBitmap(*GlobalResources::bitmapWarning);
    m_textCtrl8->SetValue(messageText);
}

ErrorDlg::~ErrorDlg() {}

void ErrorDlg::OnClose(wxCloseEvent& event)
{
    //suppressErrors = m_checkBoxSuppress->GetValue(); -> not needed here
    EndModal(AbortButtonPressed);
}

void ErrorDlg::OnContinue(wxCommandEvent& event)
{
    suppressErrors = m_checkBoxSuppress->GetValue();
    EndModal(ContinueButtonPressed);
}

void ErrorDlg::OnRetry(wxCommandEvent& event)
{
    //suppressErrors = m_checkBoxSuppress->GetValue(); -> not needed here
    EndModal(RetryButtonPressed);
}

void ErrorDlg::OnAbort(wxCommandEvent& event)
{
    //suppressErrors = m_checkBoxSuppress->GetValue(); -> not needed here
    EndModal(AbortButtonPressed);
}
//########################################################################################


SyncStatus::SyncStatus(StatusUpdater* updater, double gaugeTotalElements, wxWindow* parentWindow) :
        SyncStatusGenerated(parentWindow),
        currentStatusUpdater(updater),
        windowToDis(parentWindow),
        currentProcessIsRunning(true),
        numberOfProcessedObjects(0)
{
    //initialize gauge
    m_gauge1->SetRange(50000);
    m_gauge1->SetValue(0);

    resetGauge(gaugeTotalElements);

    if (windowToDis)    //disable (main) window while this status dialog is shown
        windowToDis->Enable(false);
}


SyncStatus::~SyncStatus()
{
    if (windowToDis)
        windowToDis->Enable(true);
}


void SyncStatus::resetGauge(double totalNrOfElements)
{
    currentElements = 0;
    totalElements = totalNrOfElements;

    if (totalElements != 0)
        scalingFactor = 50000 / totalElements; //let's normalize to 50000
    else
        scalingFactor = 0;
}


void SyncStatus::incProgressIndicator_NoUpdate(double number)
{
    currentElements+= number;
    numberOfProcessedObjects++;
}


void SyncStatus::setStatusText_NoUpdate(const wxString& text)
{
    currentStatusText = text;
}


void SyncStatus::updateStatusDialogNow()
{
    //progress indicator
    m_gauge1->SetValue(int(currentElements * scalingFactor));

    //status text
    m_textCtrlInfo->SetValue(currentStatusText);

    //processed objects
    m_staticTextProcessedObj->SetLabel(GlobalFunctions::numberToWxString(numberOfProcessedObjects));

    //remaining bytes left for copy
    m_staticTextBytesCurrent->SetLabel(FreeFileSync::formatFilesizeToShortString(mpz_class(currentElements)));

    static double totalElementsLast = -1;
    if (totalElementsLast != totalElements)
    {
        totalElementsLast = totalElements;
        m_staticTextBytesTotal->SetLabel(FreeFileSync::formatFilesizeToShortString(mpz_class(totalElements)));
    }

    //do the ui update
    updateUI_Now();
}


void SyncStatus::processHasFinished(const wxString& finalStatusText) //essential to call this in StatusUpdater derived class destructor
{                                                                    //at the LATEST(!) to prevent access to currentStatusUpdater
    currentProcessIsRunning = false;   //enable okay and close events

    m_staticTextStatus->SetLabel(finalStatusText);
    m_buttonAbort->Hide();
    m_buttonOK->Show();

    updateStatusDialogNow(); //keep this sequence to avoid display distortion, if e.g. only 1 item is sync'ed
    Layout();                //
}


void SyncStatus::OnOkay(wxCommandEvent& event)
{
    if (!currentProcessIsRunning) Destroy();
}


void SyncStatus::OnAbort(wxCommandEvent& event)
{
    if (currentProcessIsRunning) currentStatusUpdater->requestAbortion();
}


void SyncStatus::OnClose(wxCloseEvent& event)
{
    if (!currentProcessIsRunning) Destroy();
}

