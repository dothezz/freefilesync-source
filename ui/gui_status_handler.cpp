// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) 2008-2011 ZenJu (zhnmju123 AT gmx.de)                    *
// **************************************************************************

#include "gui_status_handler.h"
#include "small_dlgs.h"
#include "msg_popup.h"
#include "main_dlg.h"
#include <wx/wupdlock.h>
#include <wx+/string_conv.h>
#include "exec_finished_box.h"
#include <wx+/shell_execute.h>

using namespace zen;
using namespace xmlAccess;


CompareStatusHandler::CompareStatusHandler(MainDialog& dlg) :
    mainDlg(dlg),
    ignoreErrors(false),
    currentProcess(StatusHandler::PROCESS_NONE)
{
    wxWindowUpdateLocker dummy(&mainDlg); //avoid display distortion

    //prevent user input during "compare", do not disable maindialog since abort-button would also be disabled
    mainDlg.disableAllElements(true);
    mainDlg.compareStatus->init(); //clear old values

    //display status panel during compare
    mainDlg.auiMgr.GetPane(mainDlg.compareStatus->getAsWindow()).Show();
    mainDlg.auiMgr.Update();

    //register abort button
    mainDlg.m_buttonAbort->Connect(wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler(CompareStatusHandler::OnAbortCompare), NULL, this);
    //register key event
    mainDlg.Connect(wxEVT_CHAR_HOOK, wxKeyEventHandler(CompareStatusHandler::OnKeyPressed), NULL, this);
}


CompareStatusHandler::~CompareStatusHandler()
{
    updateUiNow(); //ui update before enabling buttons again: prevent strange behaviour of delayed button clicks

    //reenable complete main dialog
    mainDlg.enableAllElements();
    mainDlg.compareStatus->finalize();

    mainDlg.auiMgr.GetPane(mainDlg.compareStatus->getAsWindow()).Hide();
    mainDlg.auiMgr.Update();

    if (abortIsRequested())
        mainDlg.pushStatusInformation(_("Operation aborted!"));

    //de-register keys
    mainDlg.Disconnect(wxEVT_CHAR_HOOK, wxKeyEventHandler(CompareStatusHandler::OnKeyPressed), NULL, this);
    mainDlg.m_buttonAbort->Disconnect(wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler(CompareStatusHandler::OnAbortCompare), NULL, this);
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


void CompareStatusHandler::initNewProcess(int objectsTotal, zen::Int64 dataTotal, Process processID)
{
    currentProcess = processID;

    switch (currentProcess)
    {
        case StatusHandler::PROCESS_SCANNING:
            break;
        case StatusHandler::PROCESS_COMPARING_CONTENT:
        {
            wxWindowUpdateLocker dummy(&mainDlg);
            mainDlg.compareStatus->switchToCompareBytewise(objectsTotal, dataTotal);
            mainDlg.Layout();  //show progress bar...
            mainDlg.Refresh(); //remove distortion...
        }
        break;
        case StatusHandler::PROCESS_SYNCHRONIZING:
        case StatusHandler::PROCESS_NONE:
            assert(false);
            break;
    }
}


void CompareStatusHandler::updateProcessedData(int objectsDelta, zen::Int64 dataDelta)
{
    switch (currentProcess)
    {
        case StatusHandler::PROCESS_SCANNING:
            mainDlg.compareStatus->incScannedObjects_NoUpdate(objectsDelta); //throw ()
            break;
        case StatusHandler::PROCESS_COMPARING_CONTENT:
            mainDlg.compareStatus->incProcessedCmpData_NoUpdate(objectsDelta, dataDelta); //throw ()
            break;
        case StatusHandler::PROCESS_SYNCHRONIZING:
        case StatusHandler::PROCESS_NONE:
            assert(false);
            break;
    }

    //note: this method must NOT throw in order to properly allow undoing setting of statistics!
}


void CompareStatusHandler::updateTotalData(int objectsDelta, Int64 dataDelta)
{
    assert(currentProcess != PROCESS_SCANNING);
    mainDlg.compareStatus->incTotalCmpData_NoUpdate(objectsDelta, dataDelta);
}


void CompareStatusHandler::reportStatus(const std::wstring& text)
{
    mainDlg.compareStatus->setStatusText_NoUpdate(text);
    requestUiRefresh(); //throw AbortThisProcess
}


void CompareStatusHandler::reportInfo(const std::wstring& text)
{
    mainDlg.compareStatus->setStatusText_NoUpdate(text);
    requestUiRefresh(); //throw AbortThisProcess
}


ProcessCallback::Response CompareStatusHandler::reportError(const std::wstring& message)
{
    if (ignoreErrors)
        return ProcessCallback::IGNORE_ERROR;

    forceUiRefresh();

    bool ignoreNextErrors = false;
    switch (showErrorDlg(ReturnErrorDlg::BUTTON_IGNORE | ReturnErrorDlg::BUTTON_RETRY | ReturnErrorDlg::BUTTON_ABORT,
                         message, &ignoreNextErrors))
    {
        case ReturnErrorDlg::BUTTON_IGNORE:
            ignoreErrors = ignoreNextErrors;
            return ProcessCallback::IGNORE_ERROR;

        case ReturnErrorDlg::BUTTON_RETRY:
            return ProcessCallback::RETRY;

        case ReturnErrorDlg::BUTTON_ABORT:
            abortThisProcess();
    }

    assert(false);
    return ProcessCallback::IGNORE_ERROR; //dummy return value
}


void CompareStatusHandler::reportFatalError(const std::wstring& errorMessage)
{
    forceUiRefresh();

    showErrorDlg(ReturnErrorDlg::BUTTON_ABORT,
                 errorMessage, NULL);
}


void CompareStatusHandler::reportWarning(const std::wstring& warningMessage, bool& warningActive)
{
    if (!warningActive || ignoreErrors) //if errors are ignored, then warnings should also
        return;

    forceUiRefresh();

    //show pop-up and ask user how to handle warning
    bool dontWarnAgain = false;
    switch (showWarningDlg(ReturnWarningDlg::BUTTON_IGNORE | ReturnWarningDlg::BUTTON_ABORT,
                           warningMessage,
                           dontWarnAgain))
    {
        case ReturnWarningDlg::BUTTON_IGNORE:
            warningActive = !dontWarnAgain;
            break;

        case ReturnWarningDlg::BUTTON_SWITCH:
            assert(false);
        case ReturnWarningDlg::BUTTON_ABORT:
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
    requestAbortion();
    throw GuiAbortProcess();
}
//########################################################################################################


SyncStatusHandler::SyncStatusHandler(MainDialog* parentDlg,
                                     OnGuiError handleError,
                                     const wxString& jobName,
                                     const std::wstring& execWhenFinished,
                                     std::vector<std::wstring>& execFinishedHistory) :
    parentDlg_(parentDlg),
    syncStatusFrame(*this, parentDlg, SyncStatus::SYNCHRONIZING, true, jobName, execWhenFinished, execFinishedHistory),
    handleError_(handleError)
{
}


SyncStatusHandler::~SyncStatusHandler()
{
    const int totalErrors = errorLog.typeCount(TYPE_ERROR | TYPE_FATAL_ERROR); //evaluate before finalizing log

    //finalize error log
    if (abortIsRequested())
        errorLog.logMsg(_("Synchronization aborted!"), TYPE_ERROR);
    else if (totalErrors > 0)
        errorLog.logMsg(_("Synchronization completed with errors!"), TYPE_WARNING);
    else
        errorLog.logMsg(_("Synchronization completed successfully!"), TYPE_INFO);

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
            syncStatusFrame.processHasFinished(SyncStatus::ABORTED, errorLog);  //enable okay and close events
        else if (totalErrors > 0)
            syncStatusFrame.processHasFinished(SyncStatus::FINISHED_WITH_ERROR, errorLog);
        else
            syncStatusFrame.processHasFinished(SyncStatus::FINISHED_WITH_SUCCESS, errorLog);
    }
    else
        syncStatusFrame.closeWindowDirectly(); //syncStatusFrame is main window => program will quit directly
}


void SyncStatusHandler::initNewProcess(int objectsTotal, zen::Int64 dataTotal, Process processID)
{
    switch (processID)
    {
        case StatusHandler::PROCESS_SYNCHRONIZING:
            syncStatusFrame.initNewProcess(SyncStatus::SYNCHRONIZING, objectsTotal, dataTotal);
            break;
        case StatusHandler::PROCESS_SCANNING:
        case StatusHandler::PROCESS_COMPARING_CONTENT:
        case StatusHandler::PROCESS_NONE:
            assert(false);
            break;
    }
}


void SyncStatusHandler::updateProcessedData(int objectsDelta, Int64 dataDelta)
{
    syncStatusFrame.incProcessedData_NoUpdate(objectsDelta, dataDelta); //throw ()

    //note: this method should NOT throw in order to properly allow undoing setting of statistics!
}


void SyncStatusHandler::updateTotalData(int objectsDelta, Int64 dataDelta)
{
    syncStatusFrame.incTotalData_NoUpdate(objectsDelta, dataDelta); //throw ()
}


void SyncStatusHandler::reportStatus(const std::wstring& text)
{
    syncStatusFrame.setStatusText_NoUpdate(text); //throw ()
    requestUiRefresh(); //throw AbortThisProccess
}


void SyncStatusHandler::reportInfo(const std::wstring& text)
{
    errorLog.logMsg(text, TYPE_INFO);

    syncStatusFrame.setStatusText_NoUpdate(text); //throw ()
    requestUiRefresh(); //throw AbortThisProccess
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
    switch (showErrorDlg(ReturnErrorDlg::BUTTON_IGNORE | ReturnErrorDlg::BUTTON_RETRY | ReturnErrorDlg::BUTTON_ABORT,
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

        case ReturnErrorDlg::BUTTON_ABORT:
            errorLog.logMsg(errorMessage, TYPE_ERROR);
            abortThisProcess();
            break;
    }

    assert (false);
    errorLog.logMsg(errorMessage, TYPE_ERROR);
    return ProcessCallback::IGNORE_ERROR;
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
            switch (showErrorDlg(ReturnErrorDlg::BUTTON_IGNORE |  ReturnErrorDlg::BUTTON_ABORT,
                                 errorMessage,
                                 &ignoreNextErrors))
            {
                case ReturnErrorDlg::BUTTON_IGNORE:
                    if (ignoreNextErrors) //falsify only
                        handleError_ = ON_GUIERROR_IGNORE;
                    break;

                case ReturnErrorDlg::BUTTON_ABORT:
                    abortThisProcess();
                    break;

                case ReturnErrorDlg::BUTTON_RETRY:
                    assert(false);
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

    switch (handleError_)
    {
        case ON_GUIERROR_POPUP:
            break;
        case ON_GUIERROR_IGNORE:
            return; //if errors are ignored, then warnings should also
    }
    if (!warningActive)
        return;

    PauseTimers dummy(syncStatusFrame);
    forceUiRefresh();

    bool dontWarnAgain = false;
    switch (showWarningDlg(ReturnWarningDlg::BUTTON_IGNORE | ReturnWarningDlg::BUTTON_ABORT,
                           warningMessage,
                           dontWarnAgain))
    {
        case ReturnWarningDlg::BUTTON_IGNORE: //no unhandled error situation!
            warningActive = !dontWarnAgain;
            return;

        case ReturnWarningDlg::BUTTON_SWITCH:
            assert(false);
        case ReturnWarningDlg::BUTTON_ABORT:
            abortThisProcess();
            return;
    }

    assert(false);
}


void SyncStatusHandler::forceUiRefresh()
{
    syncStatusFrame.updateStatusDialogNow();
}


void SyncStatusHandler::abortThisProcess()
{
    requestAbortion();
    throw GuiAbortProcess();  //abort can be triggered by syncStatusFrame
}
