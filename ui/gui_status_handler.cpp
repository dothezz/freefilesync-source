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
#include "../shared/global_func.h"
#include "../shared/string_conv.h"
#include "../shared/util.h"

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


void CompareStatusHandler::updateProcessedData(int objectsProcessed, zen::Int64 dataProcessed)
{
    switch (currentProcess)
    {
        case StatusHandler::PROCESS_SCANNING:
            mainDlg.compareStatus->incScannedObjects_NoUpdate(objectsProcessed); //throw ()
            break;
        case StatusHandler::PROCESS_COMPARING_CONTENT:
            mainDlg.compareStatus->incProcessedCmpData_NoUpdate(objectsProcessed, dataProcessed); //throw ()
            break;
        case StatusHandler::PROCESS_SYNCHRONIZING:
        case StatusHandler::PROCESS_NONE:
            assert(false);
            break;
    }

    //note: this method should NOT throw in order to properly allow undoing setting of statistics!
}


void CompareStatusHandler::reportInfo(const wxString& text)
{
    mainDlg.compareStatus->setStatusText_NoUpdate(text);
    requestUiRefresh(); //throw AbortThisProcess
}


ProcessCallback::Response CompareStatusHandler::reportError(const wxString& message)
{
    if (ignoreErrors)
        return ProcessCallback::IGNORE_ERROR;

    forceUiRefresh();

    bool ignoreNextErrors = false;
    switch (showErrorDlg(ReturnErrorDlg::BUTTON_IGNORE | ReturnErrorDlg::BUTTON_RETRY | ReturnErrorDlg::BUTTON_ABORT,
                         message, ignoreNextErrors))
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


void CompareStatusHandler::reportFatalError(const wxString& errorMessage)
{
    forceUiRefresh();

    //show message and abort: currently there are no fatal errors during comparison that can be ignored
    bool dummy = false;
    showErrorDlg(ReturnErrorDlg::BUTTON_ABORT,
                 errorMessage, dummy);

    abortThisProcess();
}


void CompareStatusHandler::reportWarning(const wxString& warningMessage, bool& warningActive)
{
    if (!warningActive || ignoreErrors) //if errors are ignored, then warnings should also
        return;

    forceUiRefresh();

    //show popup and ask user how to handle warning
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


SyncStatusHandler::SyncStatusHandler(MainDialog* parentDlg, OnGuiError handleError, const wxString& jobName) :
    parentDlg_(parentDlg),
    syncStatusFrame(*this, parentDlg, SyncStatus::SYNCHRONIZING, false, jobName),
    handleError_(handleError)
{
}


SyncStatusHandler::~SyncStatusHandler()
{
    const int totalErrors = errorLog.typeCount(TYPE_ERROR | TYPE_FATAL_ERROR); //evaluate before finalizing log

    //finalize error log
    if (abortIsRequested())
        errorLog.logMsg(_("Synchronization aborted!") + " \n" + _("You may try to synchronize remaining items again (WITHOUT having to re-compare)!"), TYPE_ERROR);
    else if (totalErrors > 0)
        errorLog.logMsg(_("Synchronization completed with errors!") + " \n" + _("You may try to synchronize remaining items again (WITHOUT having to re-compare)!"), TYPE_WARNING);
    else
        errorLog.logMsg(_("Synchronization completed successfully!"), TYPE_INFO);

    //notify to syncStatusFrame that current process has ended
    if (abortIsRequested())
        syncStatusFrame.processHasFinished(SyncStatus::ABORTED, errorLog);  //enable okay and close events
    else if (totalErrors > 0)
        syncStatusFrame.processHasFinished(SyncStatus::FINISHED_WITH_ERROR, errorLog);
    else
        syncStatusFrame.processHasFinished(SyncStatus::FINISHED_WITH_SUCCESS, errorLog);
}


void SyncStatusHandler::initNewProcess(int objectsTotal, zen::Int64 dataTotal, Process processID)
{
    switch (processID)
    {
        case StatusHandler::PROCESS_SYNCHRONIZING:
            syncStatusFrame.resetGauge(objectsTotal, dataTotal);
            syncStatusFrame.setCurrentStatus(SyncStatus::SYNCHRONIZING);
            break;
        case StatusHandler::PROCESS_SCANNING:
        case StatusHandler::PROCESS_COMPARING_CONTENT:
        case StatusHandler::PROCESS_NONE:
            assert(false);
            break;
    }
}


void SyncStatusHandler::updateProcessedData(int objectsProcessed, zen::Int64 dataProcessed)
{
    syncStatusFrame.incProgressIndicator_NoUpdate(objectsProcessed, dataProcessed); //throw ()

    //note: this method should NOT throw in order to properly allow undoing setting of statistics!
}


void SyncStatusHandler::reportInfo(const wxString& text)
{
    //if (currentProcess == StatusHandler::PROCESS_SYNCHRONIZING)
    errorLog.logMsg(text, TYPE_INFO);

    syncStatusFrame.setStatusText_NoUpdate(text); //throw ()

    requestUiRefresh(); //throw AbortThisProccess
}


ProcessCallback::Response SyncStatusHandler::reportError(const wxString& errorMessage)
{
    switch (handleError_)
    {
        case ON_GUIERROR_POPUP:
            break;
        case ON_GUIERROR_IGNORE:
            errorLog.logMsg(errorMessage, TYPE_ERROR);
            return ProcessCallback::IGNORE_ERROR;
    }

    forceUiRefresh();

    bool ignoreNextErrors = false;
    switch (showErrorDlg(ReturnErrorDlg::BUTTON_IGNORE | ReturnErrorDlg::BUTTON_RETRY | ReturnErrorDlg::BUTTON_ABORT,
                         errorMessage,
                         ignoreNextErrors))
    {
        case ReturnErrorDlg::BUTTON_IGNORE:
            handleError_ = ignoreNextErrors ? ON_GUIERROR_IGNORE : ON_GUIERROR_POPUP;
            errorLog.logMsg(errorMessage, TYPE_ERROR);
            return ProcessCallback::IGNORE_ERROR;

        case ReturnErrorDlg::BUTTON_RETRY:
            return ProcessCallback::RETRY;

        case ReturnErrorDlg::BUTTON_ABORT:
            errorLog.logMsg(errorMessage, TYPE_ERROR);
            abortThisProcess();
    }

    assert (false);
    errorLog.logMsg(errorMessage, TYPE_ERROR);
    return ProcessCallback::IGNORE_ERROR;
}


void SyncStatusHandler::reportFatalError(const wxString& errorMessage)
{
    errorLog.logMsg(errorMessage, TYPE_FATAL_ERROR);
}


void SyncStatusHandler::reportWarning(const wxString& warningMessage, bool& warningActive)
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
