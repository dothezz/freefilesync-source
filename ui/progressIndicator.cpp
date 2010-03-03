// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) 2008-2010 ZenJu (zhnmju123 AT gmx.de)                    *
// **************************************************************************
//
#include "progressIndicator.h"
#include <memory>
#include "guiGenerated.h"
#include <wx/stopwatch.h>
#include "../library/resources.h"
#include "../shared/stringConv.h"
#include "util.h"
#include "../library/statistics.h"
#include "../library/statusHandler.h"
#include <wx/wupdlock.h>
#include "../shared/globalFunctions.h"
#include "trayIcon.h"
#include <boost/shared_ptr.hpp>

using namespace FreeFileSync;


class CompareStatusImpl : public CompareStatusGenerated
{
public:
    CompareStatusImpl(wxWindow& parentWindow);

    void init(); //initialize all status values

    void switchToCompareBytewise(int totalObjectsToProcess, wxLongLong totalDataToProcess);
    void incScannedObjects_NoUpdate(int number);
    void incProcessedCmpData_NoUpdate(int objectsProcessed, wxLongLong dataProcessed);
    void setStatusText_NoUpdate(const Zstring& text);
    void updateStatusPanelNow();

private:
    //status variables
    unsigned int scannedObjects;
    Zstring currentStatusText;

    wxStopWatch timeElapsed;

    //gauge variables
    int        totalObjects;
    wxLongLong totalData;      //each data element represents one byte for proper progress indicator scaling
    int        currentObjects; //each object represents a file or directory processed
    wxLongLong currentData;
    double     scalingFactor;  //nr of elements has to be normalized to smaller nr. because of range of int limitation

    //remaining time
    std::auto_ptr<Statistics> statistics;
    long lastStatCallSpeed;   //used for calculating intervals between statistics update
    long lastStatCallRemTime; //
};

//redirect to implementation
CompareStatus::CompareStatus(wxWindow& parentWindow) :
    pimpl(new CompareStatusImpl(parentWindow)) {}

CompareStatus::~CompareStatus()
{
    //DON'T delete pimpl! it relies on wxWidgets destruction (parent window destroys child windows!)
}

wxWindow* CompareStatus::getAsWindow()
{
    return pimpl;
}

void CompareStatus::init()
{
    pimpl->init();
}

void CompareStatus::switchToCompareBytewise(int totalObjectsToProcess, wxLongLong totalDataToProcess)
{
    pimpl->switchToCompareBytewise(totalObjectsToProcess, totalDataToProcess);
}

void CompareStatus::incScannedObjects_NoUpdate(int number)
{
    pimpl->incScannedObjects_NoUpdate(number);
}

void CompareStatus::incProcessedCmpData_NoUpdate(int objectsProcessed, wxLongLong dataProcessed)
{
    pimpl->incProcessedCmpData_NoUpdate(objectsProcessed, dataProcessed);
}

void CompareStatus::setStatusText_NoUpdate(const Zstring& text)
{
    pimpl->setStatusText_NoUpdate(text);
}

void CompareStatus::updateStatusPanelNow()
{
    pimpl->updateStatusPanelNow();
}
//########################################################################################


CompareStatusImpl::CompareStatusImpl(wxWindow& parentWindow) :
    CompareStatusGenerated(&parentWindow),
    scannedObjects(0),
    totalObjects(0),
    totalData(0),
    currentObjects(0),
    currentData(0),
    scalingFactor(0),
    statistics(NULL),
    lastStatCallSpeed(-1000000), //some big number
    lastStatCallRemTime(-1000000)
{
    init();
}


void CompareStatusImpl::init()
{
    //initialize gauge
    m_gauge2->SetRange(50000);
    m_gauge2->SetValue(0);

    //initially hide status that's relevant for comparing bytewise only
    bSizer42->Hide(sbSizer13);
    m_gauge2->Hide();
    bSizer42->Layout();

    scannedObjects = 0;
    currentStatusText.clear();

    totalObjects   = 0;
    totalData      = 0;
    currentObjects = 0;
    currentData    = 0;
    scalingFactor  = 0;

    statistics.reset();

    timeElapsed.Start(); //measure total time

    updateStatusPanelNow();
}


void CompareStatusImpl::switchToCompareBytewise(int totalObjectsToProcess, wxLongLong totalDataToProcess)
{
    currentData = 0;
    totalData = totalDataToProcess;

    currentObjects = 0;
    totalObjects   = totalObjectsToProcess;

    if (totalData != 0)
        scalingFactor = 50000 / totalData.ToDouble(); //let's normalize to 50000
    else
        scalingFactor = 0;

    //set new statistics handler: 10 seconds "window" for remaining time, 5 seconds for speed
    statistics.reset(new Statistics(totalObjectsToProcess, totalDataToProcess.ToDouble(), 10000, 5000));
    lastStatCallSpeed   = -1000000; //some big number
    lastStatCallRemTime = -1000000;

    //show status for comparing bytewise
    bSizer42->Show(sbSizer13);
    m_gauge2->Show();
    bSizer42->Layout();
}


void CompareStatusImpl::incScannedObjects_NoUpdate(int number)
{
    scannedObjects += number;
}


void CompareStatusImpl::incProcessedCmpData_NoUpdate(int objectsProcessed, wxLongLong dataProcessed)
{
    currentData    +=    dataProcessed;
    currentObjects += objectsProcessed;
}


void CompareStatusImpl::setStatusText_NoUpdate(const Zstring& text)
{
    currentStatusText = text;
}


void CompareStatusImpl::updateStatusPanelNow()
{
    //static RetrieveStatistics statistic;
    //statistic.writeEntry(currentData, currentObjects);
    {
        wxWindowUpdateLocker dummy(this); //reduce display distortion

        bool screenChanged = false; //avoid screen flicker by calling layout() only if necessary

        //remove linebreaks from currentStatusText
        wxString formattedStatusText = zToWx(currentStatusText);
        for (wxString::iterator i = formattedStatusText.begin(); i != formattedStatusText.end(); ++i)
            if (*i == wxChar('\n'))
                *i = wxChar(' ');

        //status texts
        if (m_textCtrlStatus->GetValue() != formattedStatusText && (screenChanged = true)) //avoid screen flicker
            m_textCtrlStatus->SetValue(formattedStatusText);

        //nr of scanned objects
        const wxString scannedObjTmp = globalFunctions::numberToWxString(scannedObjects);
        if (m_staticTextScanned->GetLabel() != scannedObjTmp && (screenChanged = true)) //avoid screen flicker
            m_staticTextScanned->SetLabel(scannedObjTmp);

        //progress indicator for "compare file content"
        m_gauge2->SetValue(int(currentData.ToDouble() * scalingFactor));

        //remaining files left for file comparison
        const wxString filesToCompareTmp = globalFunctions::numberToWxString(totalObjects - currentObjects);
        if (m_staticTextFilesRemaining->GetLabel() != filesToCompareTmp && (screenChanged = true)) //avoid screen flicker
            m_staticTextFilesRemaining->SetLabel(filesToCompareTmp);

        //remaining bytes left for file comparison
        const wxString remainingBytesTmp = FreeFileSync::formatFilesizeToShortString(totalData - currentData);
        if (m_staticTextDataRemaining->GetLabel() != remainingBytesTmp && (screenChanged = true)) //avoid screen flicker
            m_staticTextDataRemaining->SetLabel(remainingBytesTmp);

        if (statistics.get())
        {
            if (timeElapsed.Time() - lastStatCallSpeed >= 500) //call method every 500 ms
            {
                lastStatCallSpeed = timeElapsed.Time();

                statistics->addMeasurement(currentObjects, currentData.ToDouble());

                //current speed
                const wxString speedTmp = statistics->getBytesPerSecond();
                if (m_staticTextSpeed->GetLabel() != speedTmp && (screenChanged = true)) //avoid screen flicker
                    m_staticTextSpeed->SetLabel(speedTmp);

                if (timeElapsed.Time() - lastStatCallRemTime >= 2000) //call method every two seconds only
                {
                    lastStatCallRemTime = timeElapsed.Time();

                    //remaining time
                    const wxString timeRemainingTmp = statistics->getRemainingTime();
                    if (m_staticTextTimeRemaining->GetLabel() != timeRemainingTmp && (screenChanged = true)) //avoid screen flicker
                        m_staticTextTimeRemaining->SetLabel(timeRemainingTmp);
                }
            }
        }

        //time elapsed
        const wxString timeElapsedTmp = (wxTimeSpan::Milliseconds(timeElapsed.Time())).Format();
        if (m_staticTextTimeElapsed->GetLabel() != timeElapsedTmp && (screenChanged = true)) //avoid screen flicker
            m_staticTextTimeElapsed->SetLabel(timeElapsedTmp);

        //do the ui update
        if (screenChanged)
            bSizer42->Layout();
    }
    updateUiNow();
}
//########################################################################################


class SyncStatusImpl : public SyncStatusDlgGenerated
{
public:
    SyncStatusImpl(StatusHandler& updater, wxWindow* parentWindow);
    ~SyncStatusImpl();

    void resetGauge(int totalObjectsToProcess, wxLongLong totalDataToProcess);
    void incProgressIndicator_NoUpdate(int objectsProcessed, wxLongLong dataProcessed);
    void setStatusText_NoUpdate(const Zstring& text);
    void updateStatusDialogNow();

    void setCurrentStatus(SyncStatus::SyncStatusID id);
    void processHasFinished(SyncStatus::SyncStatusID id, const wxString& finalMessage);  //essential to call this in StatusUpdater derived class destructor at the LATEST(!) to prevent access to currentStatusUpdater

    void minimizeToTray();

private:
    void OnKeyPressed(wxKeyEvent& event);
    virtual void OnOkay(wxCommandEvent& event);
    virtual void OnPause(wxCommandEvent& event);
    virtual void OnAbort(wxCommandEvent& event);
    virtual void OnClose(wxCloseEvent& event);
    virtual void OnIconize(wxIconizeEvent& event);

    void resumeFromSystray();
    bool currentProcessIsRunning();

    wxStopWatch timeElapsed;

    StatusHandler* processStatusHandler;
    wxWindow* mainDialog;

    //gauge variables
    int        totalObjects;
    wxLongLong totalData;
    int        currentObjects; //each object represents a file or directory processed
    wxLongLong currentData;    //each data element represents one byte for proper progress indicator scaling
    double     scalingFactor;  //nr of elements has to be normalized to smaller nr. because of range of int limitation

    Zstring currentStatusText;
    bool processPaused;
    SyncStatus::SyncStatusID currentStatus;

    //remaining time
    std::auto_ptr<Statistics> statistics;
    long lastStatCallSpeed;   //used for calculating intervals between statistics update
    long lastStatCallRemTime; //

    boost::shared_ptr<MinimizeToTray> minimizedToSysTray; //optional: if filled, hides all visible windows, shows again if destroyed
};


//redirect to implementation
SyncStatus::SyncStatus(StatusHandler& updater, wxWindow* parentWindow, bool startSilent) :
    pimpl(new SyncStatusImpl(updater, parentWindow))
{
    if (startSilent)
        pimpl->minimizeToTray();
    else
    {
        pimpl->Show();
        pimpl->updateStatusDialogNow(); //update visual statistics to get rid of "dummy" texts
    }
}

SyncStatus::~SyncStatus()
{
    //DON'T delete pimpl! it will be deleted by the user clicking "OK/Cancel" -> (wxWindow::Destroy())
}

wxWindow* SyncStatus::getAsWindow()
{
    return pimpl;
}

void SyncStatus::closeWindowDirectly() //don't wait for user (silent mode)
{
    pimpl->Destroy();
}

void SyncStatus::resetGauge(int totalObjectsToProcess, wxLongLong totalDataToProcess)
{
    pimpl->resetGauge(totalObjectsToProcess, totalDataToProcess);
}

void SyncStatus::incProgressIndicator_NoUpdate(int objectsProcessed, wxLongLong dataProcessed)
{
    pimpl->incProgressIndicator_NoUpdate(objectsProcessed, dataProcessed);
}

void SyncStatus::setStatusText_NoUpdate(const Zstring& text)
{
    pimpl->setStatusText_NoUpdate(text);
}

void SyncStatus::updateStatusDialogNow()
{
    pimpl->updateStatusDialogNow();
}

void SyncStatus::setCurrentStatus(SyncStatusID id)
{
    pimpl->setCurrentStatus(id);
}

void SyncStatus::processHasFinished(SyncStatusID id, const wxString& finalMessage)
{
    pimpl->processHasFinished(id, finalMessage);
}
//########################################################################################


SyncStatusImpl::SyncStatusImpl(StatusHandler& updater, wxWindow* parentWindow) :
    SyncStatusDlgGenerated(parentWindow,
                           wxID_ANY,
                           parentWindow ? wxEmptyString : _("FreeFileSync - Folder Comparison and Synchronization"),
                           wxDefaultPosition, wxSize(638, 376),
                           parentWindow ?
                           wxDEFAULT_FRAME_STYLE | wxFRAME_NO_TASKBAR | wxFRAME_FLOAT_ON_PARENT :
                           wxDEFAULT_FRAME_STYLE | wxTAB_TRAVERSAL),
    processStatusHandler(&updater),
    mainDialog(parentWindow),
    totalObjects(0),
    totalData(0),
    currentObjects(0),
    currentData(0),
    scalingFactor(0),
    processPaused(false),
    currentStatus(SyncStatus::ABORTED),
    statistics(NULL),
    lastStatCallSpeed(-1000000), //some big number
    lastStatCallRemTime(-1000000)
{
    m_animationControl1->SetAnimation(*GlobalResources::getInstance().animationSync);
    m_animationControl1->Play();

    m_staticTextSpeed->SetLabel(wxT("-"));
    m_staticTextTimeRemaining->SetLabel(wxT("-"));

    //initialize gauge
    m_gauge1->SetRange(50000);
    m_gauge1->SetValue(0);

    m_buttonAbort->SetFocus();

    if (mainDialog)    //disable (main) window while this status dialog is shown
        mainDialog->Disable();

    timeElapsed.Start(); //measure total time

    //hide "processed" statistics until end of process
    bSizerObjectsProcessed->Show(false);
    bSizerDataProcessed->Show(false);


    SetIcon(*GlobalResources::getInstance().programIcon); //set application icon

    //register key event
    Connect(wxEVT_CHAR_HOOK, wxKeyEventHandler(SyncStatusImpl::OnKeyPressed), NULL, this);
}


SyncStatusImpl::~SyncStatusImpl()
{
    if (mainDialog)
    {
        mainDialog->Enable();
        mainDialog->Raise();
        mainDialog->SetFocus();
    }

    if (minimizedToSysTray.get())
        minimizedToSysTray->keepHidden(); //avoid window flashing shortly before it is destroyed
}


void SyncStatusImpl::OnKeyPressed(wxKeyEvent& event)
{
    const int keyCode = event.GetKeyCode();
    if (keyCode == WXK_ESCAPE)
        Close(); //generate close event: do NOT destroy window unconditionally!

    event.Skip();
}


void SyncStatusImpl::resetGauge(int totalObjectsToProcess, wxLongLong totalDataToProcess)
{
    currentData = 0;
    totalData = totalDataToProcess;

    currentObjects = 0;
    totalObjects   = totalObjectsToProcess;

    if (totalData != 0)
        scalingFactor = 50000 / totalData.ToDouble(); //let's normalize to 50000
    else
        scalingFactor = 0;

    //set new statistics handler: 10 seconds "window" for remaining time, 5 seconds for speed
    statistics.reset(new Statistics(totalObjectsToProcess, totalDataToProcess.ToDouble(), 10000, 5000));
    lastStatCallSpeed   = -1000000; //some big number
    lastStatCallRemTime = -1000000;
}


void SyncStatusImpl::incProgressIndicator_NoUpdate(int objectsProcessed, wxLongLong dataProcessed)
{
    currentData    +=    dataProcessed;
    currentObjects += objectsProcessed;
}


void SyncStatusImpl::setStatusText_NoUpdate(const Zstring& text)
{
    currentStatusText = text;
}


void SyncStatusImpl::updateStatusDialogNow()
{
    //static RetrieveStatistics statistic;
    //statistic.writeEntry(currentData, currentObjects);

    //write status information to systray, if window is minimized
    if (minimizedToSysTray.get())
        switch (currentStatus)
        {
        case SyncStatus::SCANNING:
            minimizedToSysTray->setToolTip(wxString(wxT("FreeFileSync - ")) + wxString(_("Scanning...")));
            //+ wxT(" ") + globalFunctions::numberToWxString(currentObjects));
            break;
        case SyncStatus::COMPARING_CONTENT:
            minimizedToSysTray->setToolTip(wxString(wxT("FreeFileSync - ")) + wxString(_("Comparing content...")) + wxT(" ") +
                                           formatPercentage(currentData, totalData), currentData.ToDouble() * 100 / totalData.ToDouble());
            break;
        case SyncStatus::SYNCHRONIZING:
            minimizedToSysTray->setToolTip(wxString(wxT("FreeFileSync - ")) + wxString(_("Synchronizing...")) + wxT(" ") +
                                           formatPercentage(currentData, totalData), currentData.ToDouble() * 100 / totalData.ToDouble());
            break;
        case SyncStatus::ABORTED:
        case SyncStatus::FINISHED_WITH_SUCCESS:
        case SyncStatus::FINISHED_WITH_ERROR:
        case SyncStatus::PAUSE:
            minimizedToSysTray->setToolTip(wxT("FreeFileSync"));
        }

    //write regular status information (if dialog is visible or not)
    {
        wxWindowUpdateLocker dummy(this); //reduce display distortion

        bool screenChanged = false; //avoid screen flicker by calling layout() only if necessary

        //progress indicator
        if (currentStatus == SyncStatus::SCANNING)
            m_gauge1->Pulse();
        else
            m_gauge1->SetValue(globalFunctions::round(currentData.ToDouble() * scalingFactor));

        //status text
        const wxString statusTxt = zToWx(currentStatusText);
        if (m_textCtrlInfo->GetValue() != statusTxt && (screenChanged = true)) //avoid screen flicker
            m_textCtrlInfo->SetValue(statusTxt);

        //remaining objects
        const wxString remainingObjTmp = globalFunctions::numberToWxString(totalObjects - currentObjects);
        if (m_staticTextRemainingObj->GetLabel() != remainingObjTmp && (screenChanged = true)) //avoid screen flicker
            m_staticTextRemainingObj->SetLabel(remainingObjTmp);

        //remaining bytes left for copy
        const wxString remainingBytesTmp = FreeFileSync::formatFilesizeToShortString(totalData - currentData);
        if (m_staticTextDataRemaining->GetLabel() != remainingBytesTmp && (screenChanged = true)) //avoid screen flicker
            m_staticTextDataRemaining->SetLabel(remainingBytesTmp);

        if (statistics.get())
        {
            if (timeElapsed.Time() - lastStatCallSpeed >= 500) //call method every 500 ms
            {
                lastStatCallSpeed = timeElapsed.Time();

                statistics->addMeasurement(currentObjects, currentData.ToDouble());

                //current speed
                const wxString speedTmp = statistics->getBytesPerSecond();
                if (m_staticTextSpeed->GetLabel() != speedTmp && (screenChanged = true)) //avoid screen flicker
                    m_staticTextSpeed->SetLabel(speedTmp);

                if (timeElapsed.Time() - lastStatCallRemTime >= 2000) //call method every two seconds only
                {
                    lastStatCallRemTime = timeElapsed.Time();

                    //remaining time
                    const wxString timeRemainingTmp = statistics->getRemainingTime();
                    if (m_staticTextTimeRemaining->GetLabel() != timeRemainingTmp && (screenChanged = true)) //avoid screen flicker
                        m_staticTextTimeRemaining->SetLabel(timeRemainingTmp);
                }
            }
        }

        //time elapsed
        const wxString timeElapsedTmp = wxTimeSpan::Milliseconds(timeElapsed.Time()).Format();
        if (m_staticTextTimeElapsed->GetLabel() != timeElapsedTmp && (screenChanged = true)) //avoid screen flicker
            m_staticTextTimeElapsed->SetLabel(timeElapsedTmp);


        //do the ui update
        if (screenChanged)
        {
            bSizer28->Layout();
            bSizer31->Layout();
        }
    }
    updateUiNow();

//support for pause button
    while (processPaused && currentProcessIsRunning())
    {
        wxMilliSleep(UI_UPDATE_INTERVAL);
        updateUiNow();
    }
}


bool SyncStatusImpl::currentProcessIsRunning()
{
    return processStatusHandler != NULL;
}


void SyncStatusImpl::setCurrentStatus(SyncStatus::SyncStatusID id)
{
    switch (id)
    {
    case SyncStatus::ABORTED:
        m_bitmapStatus->SetBitmap(GlobalResources::getInstance().getImageByName(wxT("statusError")));
        m_staticTextStatus->SetLabel(_("Aborted"));
        break;

    case SyncStatus::FINISHED_WITH_SUCCESS:
        m_bitmapStatus->SetBitmap(GlobalResources::getInstance().getImageByName(wxT("statusSuccess")));
        m_staticTextStatus->SetLabel(_("Completed"));
        break;

    case SyncStatus::FINISHED_WITH_ERROR:
        m_bitmapStatus->SetBitmap(GlobalResources::getInstance().getImageByName(wxT("statusWarning")));
        m_staticTextStatus->SetLabel(_("Completed"));
        break;

    case SyncStatus::PAUSE:
        m_bitmapStatus->SetBitmap(GlobalResources::getInstance().getImageByName(wxT("statusPause")));
        m_staticTextStatus->SetLabel(_("Paused"));
        break;

    case SyncStatus::SCANNING:
        m_bitmapStatus->SetBitmap(GlobalResources::getInstance().getImageByName(wxT("statusScanning")));
        m_staticTextStatus->SetLabel(_("Scanning..."));
        break;

    case SyncStatus::COMPARING_CONTENT:
        m_bitmapStatus->SetBitmap(GlobalResources::getInstance().getImageByName(wxT("statusBinaryCompare")));
        m_staticTextStatus->SetLabel(_("Comparing content..."));
        break;

    case SyncStatus::SYNCHRONIZING:
        m_bitmapStatus->SetBitmap(GlobalResources::getInstance().getImageByName(wxT("statusSyncing")));
        m_staticTextStatus->SetLabel(_("Synchronizing..."));
        break;
    }

    currentStatus = id;
    Layout();
}


void SyncStatusImpl::processHasFinished(SyncStatus::SyncStatusID id, const wxString& finalMessage) //essential to call this in StatusHandler derived class destructor
{
    //at the LATEST(!) to prevent access to currentStatusHandler
    //enable okay and close events; may be set in this method ONLY

    processStatusHandler = NULL; //avoid callback to (maybe) deleted parent process

    setCurrentStatus(id);

    resumeFromSystray(); //if in tray mode...

    m_buttonAbort->Disable();
    m_buttonAbort->Hide();
    m_buttonPause->Disable();
    m_buttonPause->Hide();
    m_buttonOK->Show();
    m_buttonOK->SetFocus();

    m_animationControl1->Stop();
    m_animationControl1->Hide();

    //hide speed and remaining time
    bSizerSpeed  ->Show(false);
    bSizerRemTime->Show(false);

    //if everything was processed successfully, hide remaining statistics (is 0 anyway)
    if (    totalObjects == currentObjects &&
            totalData    == currentData)
    {
        bSizerObjectsRemaining->Show(false);
        bSizerDataRemaining   ->Show(false);

        //show processed statistics at the end (but only if there was some work to be done)
        if (totalObjects != 0 || totalData != 0)
        {
            bSizerObjectsProcessed->Show(true);
            bSizerDataProcessed   ->Show(true);

            m_staticTextProcessedObj->SetLabel(globalFunctions::numberToWxString(currentObjects));
            m_staticTextDataProcessed->SetLabel(FreeFileSync::formatFilesizeToShortString(currentData));
        }
    }

    updateStatusDialogNow(); //keep this sequence to avoid display distortion, if e.g. only 1 item is sync'ed
    m_textCtrlInfo->SetValue(finalMessage); //
    Layout();                               //
}


void SyncStatusImpl::OnOkay(wxCommandEvent& event)
{
    if (!currentProcessIsRunning()) Destroy();
}


void SyncStatusImpl::OnPause(wxCommandEvent& event)
{
    static SyncStatus::SyncStatusID previousStatus = SyncStatus::ABORTED;

    if (processPaused)
    {
        setCurrentStatus(previousStatus);
        processPaused = false;
        m_buttonPause->SetLabel(_("Pause"));
        m_animationControl1->Play();

        //resume timers
        timeElapsed.Resume();
        if (statistics.get())
            statistics->resumeTimer();
    }
    else
    {
        previousStatus = currentStatus; //save current status

        setCurrentStatus(SyncStatus::PAUSE);
        processPaused = true;
        m_buttonPause->SetLabel(_("Continue"));
        m_animationControl1->Stop();

        //pause timers
        timeElapsed.Pause();
        if (statistics.get())
            statistics->pauseTimer();
    }
}


void SyncStatusImpl::OnAbort(wxCommandEvent& event)
{
    processPaused = false;
    if (currentProcessIsRunning())
    {
        m_buttonAbort->Disable();
        m_buttonAbort->Hide();
        m_buttonPause->Disable();
        m_buttonPause->Hide();

        setStatusText_NoUpdate(wxToZ(_("Abort requested: Waiting for current operation to finish...")));
        //no Layout() or UI-update here to avoid cascaded Yield()-call

        processStatusHandler->requestAbortion();
    }
}


void SyncStatusImpl::OnClose(wxCloseEvent& event)
{
    processPaused = false;
    if (processStatusHandler)
        processStatusHandler->requestAbortion();
    else
        Destroy();
}


void SyncStatusImpl::OnIconize(wxIconizeEvent& event)
{
    if (event.Iconized()) //ATTENTION: iconize event is also triggered on "Restore"! (at least under Linux)
        minimizeToTray();
}


void SyncStatusImpl::minimizeToTray()
{
    minimizedToSysTray.reset(new MinimizeToTray(this, mainDialog));
}


void SyncStatusImpl::resumeFromSystray()
{
    minimizedToSysTray.reset();
}
