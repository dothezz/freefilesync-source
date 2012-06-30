// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) ZenJu (zhnmju123 AT gmx DOT de) - All Rights Reserved    *
// **************************************************************************

#include "gui_status_handler.h"
#include <wx/wupdlock.h>
#include <wx+/shell_execute.h>
#include "msg_popup.h"
#include "main_dlg.h"
#include "exec_finished_box.h"
#include "../lib/generate_logfile.h"

using namespace zen;
using namespace xmlAccess;


CompareStatusHandler::CompareStatusHandler(MainDialog& dlg) :
    mainDlg(dlg),
    ignoreErrors(false)
{
    {
        wxWindowUpdateLocker dummy(&mainDlg); //avoid display distortion

        //prevent user input during "compare", do not disable maindialog since abort-button would also be disabled
        mainDlg.disableAllElements(true);

        //display status panel during compare
        mainDlg.compareStatus->init(*this); //clear old values before showing panel
        mainDlg.auiMgr.GetPane(mainDlg.compareStatus->getAsWindow()).Show();
        mainDlg.auiMgr.Update();

        //register keys
        mainDlg.Connect(wxEVT_CHAR_HOOK, wxKeyEventHandler(CompareStatusHandler::OnKeyPressed), nullptr, this);
        mainDlg.m_buttonAbort->Connect(wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler(CompareStatusHandler::OnAbortCompare), nullptr, this);
    }
    mainDlg.Update(); //don't wait until idle event!
}


CompareStatusHandler::~CompareStatusHandler()
{
    updateUiNow(); //ui update before enabling buttons again: prevent strange behaviour of delayed button clicks

    //reenable complete main dialog
    mainDlg.enableAllElements();

    mainDlg.compareStatus->finalize();
    mainDlg.auiMgr.GetPane(mainDlg.compareStatus->getAsWindow()).Hide();
    mainDlg.auiMgr.Update();

    //unregister keys
    mainDlg.Disconnect(wxEVT_CHAR_HOOK, wxKeyEventHandler(CompareStatusHandler::OnKeyPressed), nullptr, this);
    mainDlg.m_buttonAbort->Disconnect(wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler(CompareStatusHandler::OnAbortCompare), nullptr, this);

    if (abortIsRequested())
        mainDlg.flashStatusInformation(_("Operation aborted!"));
}


void CompareStatusHandler::OnKeyPressed(wxKeyEvent& event)
{
    const int keyCode = event.GetKeyCode();
    if (keyCode == WXK_ESCAPE)
    {
        wxCommandEvent dummy;
        OnAbortCompare(dummy);
    }

    event.Skip();
}


void CompareStatusHandler::initNewPhase(int objectsTotal, Int64 dataTotal, Phase phaseID)
{
    StatusHandler::initNewPhase(objectsTotal, dataTotal, phaseID);

    switch (currentPhase())
    {
        case PHASE_NONE:
        case PHASE_SYNCHRONIZING:
            assert(false);
        case PHASE_SCANNING:
            break;
        case PHASE_COMPARING_CONTENT:
        {
            wxWindowUpdateLocker dummy(&mainDlg);
            mainDlg.compareStatus->switchToCompareBytewise();
            mainDlg.Layout();  //show progress bar...
            mainDlg.Refresh(); //remove distortion...
        }
        break;
    }
}


ProcessCallback::Response CompareStatusHandler::reportError(const std::wstring& message)
{
    if (ignoreErrors)
        return ProcessCallback::IGNORE_ERROR;

    forceUiRefresh();

    bool ignoreNextErrors = false;
    switch (showErrorDlg(&mainDlg,
                         ReturnErrorDlg::BUTTON_IGNORE | ReturnErrorDlg::BUTTON_RETRY | ReturnErrorDlg::BUTTON_CANCEL,
                         message, &ignoreNextErrors))
    {
        case ReturnErrorDlg::BUTTON_IGNORE:
            ignoreErrors = ignoreNextErrors;
            return ProcessCallback::IGNORE_ERROR;

        case ReturnErrorDlg::BUTTON_RETRY:
            return ProcessCallback::RETRY;

        case ReturnErrorDlg::BUTTON_CANCEL:
            abortThisProcess();
    }

    assert(false);
    return ProcessCallback::IGNORE_ERROR; //dummy return value
}


void CompareStatusHandler::reportFatalError(const std::wstring& errorMessage)
{
    forceUiRefresh();

    showFatalErrorDlg(&mainDlg, ReturnFatalErrorDlg::BUTTON_CANCEL, errorMessage, nullptr);
}


void CompareStatusHandler::reportWarning(const std::wstring& warningMessage, bool& warningActive)
{
    if (!warningActive || ignoreErrors) //if errors are ignored, then warnings should also
        return;

    forceUiRefresh();

    //show pop-up and ask user how to handle warning
    bool dontWarnAgain = false;
    switch (showWarningDlg(&mainDlg,
                           ReturnWarningDlg::BUTTON_IGNORE | ReturnWarningDlg::BUTTON_CANCEL,
                           warningMessage, dontWarnAgain))
    {
        case ReturnWarningDlg::BUTTON_IGNORE:
            warningActive = !dontWarnAgain;
            break;

        case ReturnWarningDlg::BUTTON_SWITCH:
            assert(false);
        case ReturnWarningDlg::BUTTON_CANCEL:
            abortThisProcess();
            break;
    }
}


void CompareStatusHandler::forceUiRefresh()
{
    mainDlg.compareStatus->updateStatusPanelNow();
}


void CompareStatusHandler::OnAbortCompare(wxCommandEvent& event)
{
    requestAbortion();
}


void CompareStatusHandler::abortThisProcess()
{
    requestAbortion(); //just make sure...
    throw GuiAbortProcess();
}
//########################################################################################################


SyncStatusHandler::SyncStatusHandler(MainDialog* parentDlg,
                                     OnGuiError handleError,
                                     const std::wstring& jobName,
                                     const std::wstring& execWhenFinished,
                                     std::vector<std::wstring>& execFinishedHistory) :
    parentDlg_(parentDlg),
    syncStatusFrame(*this, *this, parentDlg, true, jobName, execWhenFinished, execFinishedHistory),
    handleError_(handleError),
    jobName_(jobName)
{
    totalTime.Start(); //measure total time
}


SyncStatusHandler::~SyncStatusHandler()
{
    const int totalErrors = errorLog.getItemCount(TYPE_ERROR | TYPE_FATAL_ERROR); //evaluate before finalizing log

    //finalize error log
    //finalize error log
    std::wstring finalStatus;
    if (abortIsRequested())
    {
        finalStatus = _("Synchronization aborted!");
        errorLog.logMsg(finalStatus, TYPE_ERROR);
    }
    else if (totalErrors > 0)
    {
        finalStatus = _("Synchronization completed with errors!");
        errorLog.logMsg(finalStatus, TYPE_WARNING);
    }
    else
    {
        if (getObjectsTotal(PHASE_SYNCHRONIZING) == 0 && //we're past "initNewPhase(PHASE_SYNCHRONIZING)" at this point!
            getDataTotal   (PHASE_SYNCHRONIZING) == 0)
            finalStatus = _("Nothing to synchronize!"); //even if "ignored conflicts" occurred!
        else
            finalStatus = _("Synchronization completed successfully!");
        errorLog.logMsg(finalStatus, TYPE_INFO);
    }

    const Utf8String logStream = generateLogStream(errorLog, jobName_, finalStatus,
                                                   getObjectsCurrent(PHASE_SYNCHRONIZING), getDataCurrent(PHASE_SYNCHRONIZING),
                                                   getObjectsTotal  (PHASE_SYNCHRONIZING), getDataTotal  (PHASE_SYNCHRONIZING), totalTime.Time() / 1000);
    try
    {
        saveToLastSyncsLog(logStream); //throw FileError
    }
    catch (FileError&) {}

    bool showFinalResults = true;

    //execute "on completion" command (even in case of ignored errors)
    if (!abortIsRequested()) //if aborted (manually), we don't execute the command
    {
        const std::wstring finalCommand = syncStatusFrame.getExecWhenFinishedCommand(); //final value (after possible user modification)
        if (isCloseProgressDlgCommand(finalCommand))
            showFinalResults = false; //take precedence over current visibility status
        else if (!finalCommand.empty())
            shellExecute(finalCommand);
    }

    //notify to syncStatusFrame that current process has ended
    if (showFinalResults)
    {
        if (abortIsRequested())
            syncStatusFrame.processHasFinished(SyncStatus::RESULT_ABORTED, errorLog);  //enable okay and close events
        else if (totalErrors > 0)
            syncStatusFrame.processHasFinished(SyncStatus::RESULT_FINISHED_WITH_ERROR, errorLog);
        else
            syncStatusFrame.processHasFinished(SyncStatus::RESULT_FINISHED_WITH_SUCCESS, errorLog);
    }
    else
        syncStatusFrame.closeWindowDirectly(); //syncStatusFrame is main window => program will quit directly
}


void SyncStatusHandler::initNewPhase(int objectsTotal, Int64 dataTotal, Phase phaseID)
{
    assert(phaseID == PHASE_SYNCHRONIZING);
    StatusHandler::initNewPhase(objectsTotal, dataTotal, phaseID);
    syncStatusFrame.initNewPhase(); //call after "StatusHandler::initNewPhase"
}


void SyncStatusHandler::updateProcessedData(int objectsDelta, Int64 dataDelta)
{
    StatusHandler::updateProcessedData(objectsDelta, dataDelta);
    syncStatusFrame.reportCurrentBytes(getDataCurrent(currentPhase())); //throw ()
    //note: this method should NOT throw in order to properly allow undoing setting of statistics!
}


void SyncStatusHandler::reportInfo(const std::wstring& text)
{
    StatusHandler::reportInfo(text);
    errorLog.logMsg(text, TYPE_INFO);
}


ProcessCallback::Response SyncStatusHandler::reportError(const std::wstring& errorMessage)
{
    switch (handleError_)
    {
        case ON_GUIERROR_POPUP:
            break;
        case ON_GUIERROR_IGNORE:
            errorLog.logMsg(errorMessage, TYPE_ERROR);
            return ProcessCallback::IGNORE_ERROR;
    }

    PauseTimers dummy(syncStatusFrame);
    forceUiRefresh();

    bool ignoreNextErrors = false;
    switch (showErrorDlg(parentDlg_,
                         ReturnErrorDlg::BUTTON_IGNORE | ReturnErrorDlg::BUTTON_RETRY | ReturnErrorDlg::BUTTON_CANCEL,
                         errorMessage,
                         &ignoreNextErrors))
    {
        case ReturnErrorDlg::BUTTON_IGNORE:
            if (ignoreNextErrors) //falsify only
                handleError_ = ON_GUIERROR_IGNORE;
            errorLog.logMsg(errorMessage, TYPE_ERROR);
            return ProcessCallback::IGNORE_ERROR;

        case ReturnErrorDlg::BUTTON_RETRY:
            return ProcessCallback::RETRY;

        case ReturnErrorDlg::BUTTON_CANCEL:
            errorLog.logMsg(errorMessage, TYPE_ERROR);
            abortThisProcess();
            break;
    }

    assert(false);
    return ProcessCallback::IGNORE_ERROR; //dummy value
}


void SyncStatusHandler::reportFatalError(const std::wstring& errorMessage)
{
    errorLog.logMsg(errorMessage, TYPE_FATAL_ERROR);

    switch (handleError_)
    {
        case ON_GUIERROR_POPUP:
        {
            PauseTimers dummy(syncStatusFrame);
            forceUiRefresh();

            bool ignoreNextErrors = false;
            switch (showFatalErrorDlg(parentDlg_,
                                      ReturnFatalErrorDlg::BUTTON_IGNORE |  ReturnFatalErrorDlg::BUTTON_CANCEL,
                                      errorMessage, &ignoreNextErrors))
            {
                case ReturnFatalErrorDlg::BUTTON_IGNORE:
                    if (ignoreNextErrors) //falsify only
                        handleError_ = ON_GUIERROR_IGNORE;
                    break;

                case ReturnFatalErrorDlg::BUTTON_CANCEL:
                    abortThisProcess();
                    break;
            }
        }
        break;

        case ON_GUIERROR_IGNORE:
            break;
    }
}


void SyncStatusHandler::reportWarning(const std::wstring& warningMessage, bool& warningActive)
{
    errorLog.logMsg(warningMessage, TYPE_WARNING);

    if (!warningActive)
        return;

    switch (handleError_)
    {
        case ON_GUIERROR_POPUP:
        {
            PauseTimers dummy(syncStatusFrame);
            forceUiRefresh();

            bool dontWarnAgain = false;
            switch (showWarningDlg(parentDlg_,
                                   ReturnWarningDlg::BUTTON_IGNORE | ReturnWarningDlg::BUTTON_CANCEL,
                                   warningMessage, dontWarnAgain))
            {
                case ReturnWarningDlg::BUTTON_IGNORE: //no unhandled error situation!
                    warningActive = !dontWarnAgain;
                    break;

                case ReturnWarningDlg::BUTTON_SWITCH:
                    assert(false);
                case ReturnWarningDlg::BUTTON_CANCEL:
                    abortThisProcess();
                    break;
            }
        }
        break;

        case ON_GUIERROR_IGNORE:
            break; //if errors are ignored, then warnings should be, too
    }
}


void SyncStatusHandler::forceUiRefresh()
{
    syncStatusFrame.updateProgress();
}


void SyncStatusHandler::abortThisProcess()
{
    requestAbortion(); //just make sure...
    throw GuiAbortProcess();  //abort can be triggered by syncStatusFrame
}
