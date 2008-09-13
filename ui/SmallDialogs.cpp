#include "smallDialogs.h"
#include "../library/globalFunctions.h"

using namespace globalFunctions;

AboutDlg::AboutDlg(MainDialog* window) : AboutDlgGenerated(window)
{
    m_bitmap9->SetBitmap(*GlobalResources::bitmapWebsite);
    m_bitmap10->SetBitmap(*GlobalResources::bitmapEmail);
    m_bitmap11->SetBitmap(*GlobalResources::bitmapFFS);

    m_animationControl1->SetAnimation(*GlobalResources::animationMoney);
    m_animationControl1->Play();

    //build
    wxString build = wxString(_("(Build: ")) + __TDATE__ + ")";
    m_build->SetLabel(build);

    m_button8->SetFocus();
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


HelpDlg::HelpDlg(MainDialog* window) : HelpDlgGenerated(window)
{
    m_button8->SetFocus();
}

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


FilterDlg::FilterDlg(MainDialog* window) :
        FilterDlgGenerated(window),
        mainDialog(window)
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
    EndModal(okayButtonPressed);
}


void FilterDlg::OnCancel(wxCommandEvent& event)
{
    EndModal(0);
}


void FilterDlg::OnDefault(wxCommandEvent& event)
{
    m_textCtrlInclude->SetValue("*");
    m_textCtrlExclude->SetValue("");

    //changes to mainDialog are only committed when the OK button is pressed
    event.Skip();
}
//########################################################################################

DeleteDialog::DeleteDialog(const wxString& headerText, const wxString& messageText, wxWindow* main) :
        DeleteDialogGenerated(main)
{
    m_staticTextHeader->SetLabel(headerText);
    m_textCtrlMessage->SetValue(messageText);
    m_bitmap12->SetBitmap(*GlobalResources::bitmapDeleteFile);

    m_buttonOK->SetFocus();
}

DeleteDialog::~DeleteDialog() {}


void DeleteDialog::OnOK(wxCommandEvent& event)
{
    EndModal(okayButtonPressed);
}

void DeleteDialog::OnCancel(wxCommandEvent& event)
{
    EndModal(cancelButtonPressed);
}

void DeleteDialog::OnClose(wxCloseEvent& event)
{
    EndModal(cancelButtonPressed);
}
//########################################################################################


ErrorDlg::ErrorDlg(const wxString messageText, bool& suppressErrormessages) :
        ErrorDlgGenerated(0),
        suppressErrors(suppressErrormessages)
{
    m_bitmap10->SetBitmap(*GlobalResources::bitmapWarning);
    m_textCtrl8->SetValue(messageText);

    m_buttonContinue->SetFocus();
}

ErrorDlg::~ErrorDlg() {}


void ErrorDlg::OnClose(wxCloseEvent& event)
{
    //suppressErrors = m_checkBoxSuppress->GetValue(); -> not needed here
    EndModal(abortButtonPressed);
}


void ErrorDlg::OnContinue(wxCommandEvent& event)
{
    suppressErrors = m_checkBoxSuppress->GetValue();
    EndModal(continueButtonPressed);
}


void ErrorDlg::OnRetry(wxCommandEvent& event)
{
    //suppressErrors = m_checkBoxSuppress->GetValue(); -> not needed here
    EndModal(retryButtonPressed);
}


void ErrorDlg::OnAbort(wxCommandEvent& event)
{
    //suppressErrors = m_checkBoxSuppress->GetValue(); -> not needed here
    EndModal(abortButtonPressed);
}
//########################################################################################


SyncStatus::SyncStatus(StatusUpdater* updater, wxWindow* parentWindow) :
        SyncStatusGenerated(parentWindow),
        currentStatusUpdater(updater),
        windowToDis(parentWindow),
        currentProcessIsRunning(true),
        totalData(0),
        currentData(0),
        scalingFactor(0),
        currentObjects(0),
        totalObjects(0)
{
    m_animationControl1->SetAnimation(*GlobalResources::animationSync);
    m_animationControl1->Play();

    //initialize gauge
    m_gauge1->SetRange(50000);
    m_gauge1->SetValue(0);

    m_buttonAbort->SetFocus();

    if (windowToDis)    //disable (main) window while this status dialog is shown
        windowToDis->Disable();
}


SyncStatus::~SyncStatus()
{
    if (windowToDis)
    {
        windowToDis->Enable();
        windowToDis->Raise();
    }
}


void SyncStatus::resetGauge(int totalObjectsToProcess, double totalDataToProcess)
{
    currentData = 0;
    totalData = totalDataToProcess;

    currentObjects = 0;
    totalObjects   = totalObjectsToProcess;

    if (totalData != 0)
        scalingFactor = 50000 / totalData; //let's normalize to 50000
    else
        scalingFactor = 0;
}


void SyncStatus::incProgressIndicator_NoUpdate(int objectsProcessed, double dataProcessed)
{
    currentData+=    dataProcessed;
    currentObjects+= objectsProcessed;
}


void SyncStatus::setStatusText_NoUpdate(const wxString& text)
{
    currentStatusText = text;
}


void SyncStatus::updateStatusDialogNow()
{
    //progress indicator
    m_gauge1->SetValue(int(currentData * scalingFactor));

    //status text
    m_textCtrlInfo->SetValue(currentStatusText);

    //remaining objects
    m_staticTextRemainingObj->SetLabel(numberToWxString(totalObjects - currentObjects));

    //remaining bytes left for copy
    const wxString remainingBytes = FreeFileSync::formatFilesizeToShortString(mpz_class(totalData - currentData));
    m_staticTextDataRemaining->SetLabel(remainingBytes);

    //do the ui update
    updateUI_Now();
}


void SyncStatus::processHasFinished(const wxString& finalStatusText) //essential to call this in StatusUpdater derived class destructor
{                                                                    //at the LATEST(!) to prevent access to currentStatusUpdater
    currentProcessIsRunning = false;   //enable okay and close events

    m_staticTextStatus->SetLabel(finalStatusText);
    m_buttonAbort->Hide();
    m_buttonOK->Show();
    m_buttonOK->SetFocus();

    m_animationControl1->Stop();

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
    if (currentProcessIsRunning) currentStatusUpdater->requestAbortion();
    else
        Destroy();
}
//########################################################################################


CompareStatus::CompareStatus(wxWindow* parentWindow) :
        CompareStatusGenerated(parentWindow),
        scannedFiles(0),
        totalMD5Data(0),
        currentMD5Data(0),
        scalingFactorMD5(0),
        currentMD5Objects(0),
        totalMD5Objects(0)
{
    //initialize gauge
    m_gauge2->SetRange(50000);
    m_gauge2->SetValue(0);
}


CompareStatus::~CompareStatus() {}


void CompareStatus::resetMD5Gauge(int totalMD5ObjectsToProcess, double totalMD5DataToProcess)
{
    currentMD5Data = 0;
    totalMD5Data = totalMD5DataToProcess;

    currentMD5Objects = 0;
    totalMD5Objects   = totalMD5ObjectsToProcess;

    if (totalMD5Data != 0)
        scalingFactorMD5 = 50000 / totalMD5Data; //let's normalize to 50000
    else
        scalingFactorMD5 = 0;
}


void CompareStatus::incScannedFiles_NoUpdate(int number)
{
    scannedFiles+= number;
}


void CompareStatus::incProcessedMD5Data_NoUpdate(int objectsProcessed, double dataProcessed)
{
    currentMD5Data+=    dataProcessed;
    currentMD5Objects+= objectsProcessed;
}


void CompareStatus::setStatusText_NoUpdate(const wxString& text)
{
    currentStatusText = text;
}


void CompareStatus::updateStatusPanelNow()
{
    //status texts
    m_textCtrlFilename->SetValue(currentStatusText);

    m_staticTextScanned->SetLabel(numberToWxString(scannedFiles));

    //progress indicator for MD5
    m_gauge2->SetValue(int(currentMD5Data * scalingFactorMD5));

    //remaining MD5 objects
    m_staticTextFilesToCompare->SetLabel(numberToWxString(totalMD5Objects - currentMD5Objects));

    //remaining bytes left for MD5 calculation
    const wxString remainingBytes = FreeFileSync::formatFilesizeToShortString(mpz_class(totalMD5Data - currentMD5Data));
    m_staticTextDataToCompare->SetLabel(remainingBytes);

    //do the ui update
    updateUI_Now();
}
