#include "smallDialogs.h"
#include "../library/globalFunctions.h"
//#include <fstream>
#include "../library/resources.h"

using namespace globalFunctions;


AboutDlg::AboutDlg(wxWindow* window) : AboutDlgGenerated(window)
{
    m_bitmap9->SetBitmap(*GlobalResources::bitmapWebsite);
    m_bitmap10->SetBitmap(*GlobalResources::bitmapEmail);
    m_bitmap11->SetBitmap(*GlobalResources::bitmapLogo);
    m_bitmap13->SetBitmap(*GlobalResources::bitmapGPL);

    //build information
    wxString build = wxString(_("(Build: ")) + __TDATE__;
#if wxUSE_UNICODE
    build+= wxT(" - Unicode");
#else
    build+= wxT(" - ANSI");
#endif //wxUSE_UNICODE
    build+= + wxT(")");
    m_build->SetLabel(build);

    m_animationControl1->SetAnimation(*GlobalResources::animationMoney);
    m_animationControl1->Play(); //Note: The animation is created hidden(!) to not disturb constraint based window creation;
    m_animationControl1->Show(); //an empty animation consumes a lot of space that later is NOT removed anymore.

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


HelpDlg::HelpDlg(wxWindow* window) : HelpDlgGenerated(window)
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


FilterDlg::FilterDlg(wxWindow* window, wxString& filterIncl, wxString& filterExcl) :
        FilterDlgGenerated(window),
        includeFilter(filterIncl),
        excludeFilter(filterExcl)
{

    m_bitmap8->SetBitmap(*GlobalResources::bitmapInclude);
    m_bitmap9->SetBitmap(*GlobalResources::bitmapExclude);

    m_textCtrlInclude->SetValue(includeFilter);
    m_textCtrlExclude->SetValue(excludeFilter);
}

FilterDlg::~FilterDlg() {}


void FilterDlg::OnClose(wxCloseEvent& event)
{
    EndModal(0);
}


void FilterDlg::OnOK(wxCommandEvent& event)
{
    //only if user presses ApplyFilter, he wants the changes to be committed
    includeFilter = m_textCtrlInclude->GetValue();
    excludeFilter = m_textCtrlExclude->GetValue();

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
        DeleteDlgGenerated(main)
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


ErrorDlg::ErrorDlg(const wxString messageText, bool& continueError) :
        ErrorDlgGenerated(0),
        continueOnError(continueError)
{
    m_bitmap10->SetBitmap(*GlobalResources::bitmapWarning);
    m_textCtrl8->SetValue(messageText);

    m_buttonContinue->SetFocus();
}

ErrorDlg::~ErrorDlg() {}


void ErrorDlg::OnClose(wxCloseEvent& event)
{
    //continueOnError = m_checkBoxContinueError->GetValue(); -> not needed here
    EndModal(abortButtonPressed);
}


void ErrorDlg::OnContinue(wxCommandEvent& event)
{
    continueOnError = m_checkBoxContinueError->GetValue();
    EndModal(continueButtonPressed);
}


void ErrorDlg::OnRetry(wxCommandEvent& event)
{
    //continueOnError = m_checkBoxContinueError->GetValue(); -> not needed here
    EndModal(retryButtonPressed);
}


void ErrorDlg::OnAbort(wxCommandEvent& event)
{
    //continueOnError = m_checkBoxContinueError->GetValue(); -> not needed here
    EndModal(abortButtonPressed);
}
//########################################################################################

/*
class for calculation of remaining time:
----------------------------------------
"filesize |-> time" is an affine linear function f(x) = z_1 + z_2 x

For given n measurements, sizes x_0, ..., x_n and times f_0, ..., f_n, the function f (as a polynom of degree 1) can be lineary approximated by

z_1 = (r - s * q / p) / ((n + 1) - s * s / p)
z_2 = (q - s * z_1) / p = (r - (n + 1) z_1) / s

with
p := x_0^2 + ... + x_n^2
q := f_0 x_0 + ... + f_n x_n
r := f_0 + ... + f_n
s := x_0 + ... + x_n

=> the time to process N files with amount of data D is:    N * z_1 + D * z_2

Problem:
--------
Times f_0, ..., f_n can be very small so that precision of the PC clock is poor.
=> Times have to be accumulated to enhance precision:
Copying of m files with sizes x_i and times f_i (i = 1, ..., m) takes sum_i f(x_i) := m * z_1 + z_2 * sum x_i = sum f_i
With X defined as the accumulated sizes and F the accumulated times this gives: (in theory...)
m * z_1 + z_2 * X = F   <=>
z_1 + z_2 * X / m = F / m

=> we optain a new (artificial) measurement with size X / m and time F / m to be used in the linear approximation above


RemainingTime::RemainingTime() : n(0), m(0), X(0), F(0), p(0), q(0), r(0), s(0), z_1(0), z_2(0), lastExec(0) {}

RemainingTime::~RemainingTime()
{
    ofstream output("test.txt");
    for (unsigned i = 0; i < x.size(); ++i)
    {
        output<<x[i]<<" "<<f[i]<<'\n';
    }
    output<<'\n'<<z_1<<" "<<z_2<<'\n';
    output.close();
}


wxLongLong RemainingTime::getRemainingTime(double processedDataSinceLastCall, int remainingFiles, double remainingData) //returns the remaining time in seconds
{
    wxLongLong newExec = wxGetLocalTimeMillis();

    if (lastExec != 0)
    {
        X+= processedDataSinceLastCall;
        F = (newExec - lastExec).ToDouble();
        ++m;

        if (F > 1000)  //add new measurement only if F is accumulated to a certain degree
        {
            lastExec = newExec;
            ++n;

            double x_i = X / m;
            double f_i = F / m;
            X = 0;
            F = 0;
            m = 0;

            x.push_back(x_i);
            f.push_back(f_i);

            p+= x_i * x_i;
            q+= f_i * x_i;
            r+= f_i;
            s+= x_i;

            if (p != 0)
            {
                double tmp = (n - s * s / p);
                if (tmp != 0 && s != 0)
                {   //recalculate coefficients for affine-linear function
                    z_1 = (r - s * q / p) / tmp;
                    z_2 = (r - n * z_1) / s;    //not (n + 1) here, since n already is the number of measurements
                }
            }
        }

        return int(remainingFiles * z_1 + remainingData * z_2);
    }
    //else
    lastExec = newExec;
    return 0;
}*/

//########################################################################################

SyncStatus::SyncStatus(StatusUpdater* updater, wxWindow* parentWindow) :
        SyncStatusDlgGenerated(parentWindow),
        currentStatusUpdater(updater),
        windowToDis(parentWindow),
        currentProcessIsRunning(true),
        totalData(0),
        currentData(0),
        scalingFactor(0),
        currentObjects(0),
        totalObjects(0),
        processPaused(false)
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
        windowToDis->SetFocus();
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
    const wxString remainingBytes = FreeFileSync::formatFilesizeToShortString(totalData - currentData);
    m_staticTextDataRemaining->SetLabel(remainingBytes);

    //do the ui update
    bSizer28->Layout();
    updateUI_Now();

    //support for pause button
    while (processPaused && currentProcessIsRunning)
    {
        wxMilliSleep(UI_UPDATE_INTERVAL);
        updateUI_Now();
    }
}


void SyncStatus::setCurrentStatus(SyncStatusID id)
{
    switch (id)
    {
    case ABORTED:
        m_bitmapStatus->SetBitmap(*GlobalResources::bitmapStatusError);
        m_staticTextStatus->SetLabel(_("Aborted"));
        break;

    case FINISHED_WITH_SUCCESS:
        m_bitmapStatus->SetBitmap(*GlobalResources::bitmapStatusSuccess);
        m_staticTextStatus->SetLabel(_("Completed"));
        break;

    case FINISHED_WITH_ERROR:
        m_bitmapStatus->SetBitmap(*GlobalResources::bitmapStatusWarning);
        m_staticTextStatus->SetLabel(_("Completed"));
        break;

    case PAUSE:
        m_bitmapStatus->SetBitmap(*GlobalResources::bitmapStatusPause);
        m_staticTextStatus->SetLabel(_("Pause"));
        break;

    case SCANNING:
        m_bitmapStatus->SetBitmap(*GlobalResources::bitmapStatusComparing);
        m_staticTextStatus->SetLabel(_("Scanning..."));
        break;

    case COMPARING:
        m_bitmapStatus->SetBitmap(*GlobalResources::bitmapStatusComparing);
        m_staticTextStatus->SetLabel(_("Comparing..."));
        break;

    case SYNCHRONIZING:
        m_bitmapStatus->SetBitmap(*GlobalResources::bitmapStatusSyncing);
        m_staticTextStatus->SetLabel(_("Synchronizing..."));
        break;
    }
}


void SyncStatus::processHasFinished(SyncStatusID id) //essential to call this in StatusUpdater derived class destructor
{                                                    //at the LATEST(!) to prevent access to currentStatusUpdater
    currentProcessIsRunning = false; //enable okay and close events; may be set ONLY in this method

    setCurrentStatus(id);

    m_buttonAbort->Disable();
    m_buttonAbort->Hide();
    m_buttonPause->Disable();
    m_buttonPause->Hide();
    m_buttonOK->Show();
    m_buttonOK->SetFocus();

    m_animationControl1->Stop();

    //m_animationControl1->SetInactiveBitmap(*GlobalResources::bitmapFinished);
    m_animationControl1->Hide();

    updateStatusDialogNow(); //keep this sequence to avoid display distortion, if e.g. only 1 item is sync'ed
    Layout();                //
}


void SyncStatus::OnOkay(wxCommandEvent& event)
{
    if (!currentProcessIsRunning) Destroy();
}


void SyncStatus::OnPause(wxCommandEvent& event)
{
    if (processPaused)
    {
        processPaused = false;
        m_buttonPause->SetLabel(_("Pause"));
        m_animationControl1->Play();

    }
    else
    {
        processPaused = true;
        m_buttonPause->SetLabel(_("Continue"));
        m_animationControl1->Stop();
    }
}


void SyncStatus::OnAbort(wxCommandEvent& event)
{
    processPaused = false;
    if (currentProcessIsRunning) currentStatusUpdater->requestAbortion();
}


void SyncStatus::OnClose(wxCloseEvent& event)
{
    processPaused = false;
    if (currentProcessIsRunning) currentStatusUpdater->requestAbortion();
    else
        Destroy();
}
//########################################################################################


CompareStatus::CompareStatus(wxWindow* parentWindow) :
        CompareStatusGenerated(parentWindow),
        scannedFiles(0),
        totalCmpData(0),
        processedCmpData(0),
        scalingFactorCmp(0),
        processedCmpObjects(0),
        totalCmpObjects(0)
        /*timeRemaining(0),
        timeRemainingTimeStamp(0)*/

{   //initialize gauge
    m_gauge2->SetRange(50000);
    m_gauge2->SetValue(0);

    //initially hide status that's relevant for comparing bytewise only
    bSizer42->Hide(sbSizer13);
    bSizer42->Hide(sbSizer11);
    bSizer42->Layout();
}


CompareStatus::~CompareStatus() {}


void CompareStatus::resetCmpGauge(int totalCmpObjectsToProcess, double totalCmpDataToProcess)
{
    processedCmpData = 0;
    totalCmpData = totalCmpDataToProcess;

    processedCmpObjects = 0;
    totalCmpObjects   = totalCmpObjectsToProcess;

    if (totalCmpData != 0)
        scalingFactorCmp = 50000 / totalCmpData; //let's normalize to 50000
    else
        scalingFactorCmp = 0;

    //show status for comparing bytewise
    bSizer42->Show(sbSizer13);
    bSizer42->Show(sbSizer11);
    bSizer42->Layout();
}


void CompareStatus::incScannedFiles_NoUpdate(int number)
{
    scannedFiles+= number;
}


void CompareStatus::incProcessedCmpData_NoUpdate(int objectsProcessed, double dataProcessed)
{
    processedCmpData+=    dataProcessed;
    processedCmpObjects+= objectsProcessed;

    /*    timeRemaining = calcTimeLeft.getRemainingTime(dataProcessed, totalCmpObjects - processedCmpObjects, totalCmpData - processedCmpData);
        timeRemainingTimeStamp = wxGetLocalTimeMillis();*/
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

    //progress indicator for "compare file content"
    m_gauge2->SetValue(int(processedCmpData * scalingFactorCmp));

    //remaining file to compare
    m_staticTextFilesToCompare->SetLabel(numberToWxString(totalCmpObjects - processedCmpObjects));

    //remaining bytes left for file comparison
    const wxString remainingBytes = FreeFileSync::formatFilesizeToShortString(totalCmpData - processedCmpData);
    m_staticTextDataToCompare->SetLabel(remainingBytes);
    /*
        //remaining time in seconds
        if (timeRemaining != 0)
        {
            int time = ((timeRemaining - (wxGetLocalTimeMillis() - timeRemainingTimeStamp)) / 1000).GetLo();
            m_staticTextRemainingTime->SetLabel(numberToWxString(time) + " s");
        }
    */
    //do the ui update
    bSizer42->Layout();
    updateUI_Now();
}
