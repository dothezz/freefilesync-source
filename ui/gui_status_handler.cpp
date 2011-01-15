// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) 2008-2011 ZenJu (zhnmju123 AT gmx.de)                    *
// **************************************************************************
//
#include "gui_status_handler.h"
#include "small_dlgs.h"
#include "msg_popup.h"
#include "../shared/system_constants.h"
#include "main_dlg.h"
#include <wx/wupdlock.h>
#include "../shared/global_func.h"
#include "../shared/string_conv.h"
#include "../shared/util.h"

using namespace ffs3;


CompareStatusHandler::CompareStatusHandler(MainDialog* dlg) :
    mainDialog(dlg),
    ignoreErrors(false),
    currentProcess(StatusHandler::PROCESS_NONE)
{
    wxWindowUpdateLocker dummy(mainDialog); //avoid display distortion

    //prevent user input during "compare", do not disable maindialog since abort-button would also be disabled
    mainDialog->disableAllElements();
    mainDialog->compareStatus->init(); //clear old values

    //display status panel during compare
    mainDialog->auiMgr.GetPane(mainDialog->compareStatus->getAsWindow()).Show();
    mainDialog->auiMgr.Update();

    //register abort button
    mainDialog->m_buttonAbort->Connect(wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler(CompareStatusHandler::OnAbortCompare), NULL, this);
    //register key event
    mainDialog->Connect(wxEVT_CHAR_HOOK, wxKeyEventHandler(CompareStatusHandler::OnKeyPressed), NULL, this);
}


CompareStatusHandler::~CompareStatusHandler()
{
    updateUiNow(); //ui update before enabling buttons again: prevent strange behaviour of delayed button clicks

    //reenable complete main dialog
    mainDialog->enableAllElements();
    mainDialog->compareStatus->finalize();

    mainDialog->auiMgr.GetPane(mainDialog->compareStatus->getAsWindow()).Hide();
    mainDialog->auiMgr.Update();

    if (abortIsRequested())
        mainDialog->pushStatusInformation(_("Operation aborted!"));

    //de-register keys
    mainDialog->Disconnect(wxEVT_CHAR_HOOK, wxKeyEventHandler(CompareStatusHandler::OnKeyPressed), NULL, this);
    mainDialog->m_buttonAbort->Disconnect(wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler(CompareStatusHandler::OnAbortCompare), NULL, this);
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


void CompareStatusHandler::reportInfo(const Zstring& text)
{
    mainDialog->compareStatus->setStatusText_NoUpdate(text);
}


void CompareStatusHandler::initNewProcess(int objectsTotal, wxLongLong dataTotal, Process processID)
{
    currentProcess = processID;

    switch (currentProcess)
    {
    case StatusHandler::PROCESS_SCANNING:
        break;
    case StatusHandler::PROCESS_COMPARING_CONTENT:
    {
        wxWindowUpdateLocker dummy(mainDialog);
        mainDialog->compareStatus->switchToCompareBytewise(objectsTotal, dataTotal);
        mainDialog->Layout();  //show progress bar...
        mainDialog->Refresh(); //remove distortion...
    }
    break;
    case StatusHandler::PROCESS_SYNCHRONIZING:
    case StatusHandler::PROCESS_NONE:
        assert(false);
        break;
    }
}


inline
void CompareStatusHandler::updateProcessedData(int objectsProcessed, wxLongLong dataProcessed)
{
    switch (currentProcess)
    {
    case StatusHandler::PROCESS_SCANNING:
        mainDialog->compareStatus->incScannedObjects_NoUpdate(objectsProcessed);
        break;
    case StatusHandler::PROCESS_COMPARING_CONTENT:
        mainDialog->compareStatus->incProcessedCmpData_NoUpdate(objectsProcessed, dataProcessed);
        break;
    case StatusHandler::PROCESS_SYNCHRONIZING:
    case StatusHandler::PROCESS_NONE:
        assert(false);
        break;
    }
}


ErrorHandler::Response CompareStatusHandler::reportError(const wxString& message)
{
    if (ignoreErrors)
        return ErrorHandler::IGNORE_ERROR;

    mainDialog->compareStatus->updateStatusPanelNow();

    bool ignoreNextErrors = false;
    ErrorDlg errorDlg(NULL,
                      ErrorDlg::BUTTON_IGNORE |  ErrorDlg::BUTTON_RETRY | ErrorDlg::BUTTON_ABORT,
                      message, ignoreNextErrors);
    errorDlg.Raise();
    switch (static_cast<ErrorDlg::ReturnCodes>(errorDlg.ShowModal()))
    {
    case ErrorDlg::BUTTON_IGNORE:
        ignoreErrors = ignoreNextErrors;
        return ErrorHandler::IGNORE_ERROR;

    case ErrorDlg::BUTTON_RETRY:
        return ErrorHandler::RETRY;

    case ErrorDlg::BUTTON_ABORT:
        abortThisProcess();
    }

    assert(false);
    return ErrorHandler::IGNORE_ERROR; //dummy return value
}


void CompareStatusHandler::reportFatalError(const wxString& errorMessage)
{
    mainDialog->compareStatus->updateStatusPanelNow();

    bool dummy = false;
    ErrorDlg errorDlg(NULL,
                      ErrorDlg::BUTTON_ABORT,
                      errorMessage, dummy);
    errorDlg.Raise();
    errorDlg.ShowModal();
    abortThisProcess();
}


void CompareStatusHandler::reportWarning(const wxString& warningMessage, bool& warningActive)
{
    if (!warningActive || ignoreErrors) //if errors are ignored, then warnings should also
        return;

    mainDialog->compareStatus->updateStatusPanelNow();

    //show popup and ask user how to handle warning
    bool dontWarnAgain = false;
    WarningDlg warningDlg(NULL,
                          WarningDlg::BUTTON_IGNORE | WarningDlg::BUTTON_ABORT,
                          warningMessage,
                          dontWarnAgain);
    warningDlg.Raise();
    switch (static_cast<WarningDlg::Response>(warningDlg.ShowModal()))
    {
    case WarningDlg::BUTTON_IGNORE:
        warningActive = !dontWarnAgain;
        break;

    case WarningDlg::BUTTON_SWITCH:
        assert(false);
    case WarningDlg::BUTTON_ABORT:
        abortThisProcess();
        break;
    }
}


inline
void CompareStatusHandler::forceUiRefresh()
{
    mainDialog->compareStatus->updateStatusPanelNow();
}


void CompareStatusHandler::OnAbortCompare(wxCommandEvent& event)
{
    requestAbortion();
}


void CompareStatusHandler::abortThisProcess()
{
    requestAbortion();
    throw ffs3::AbortThisProcess();
}
//########################################################################################################


SyncStatusHandler::SyncStatusHandler(wxTopLevelWindow* parentDlg, bool ignoreAllErrors, const wxString& jobName) :
    syncStatusFrame(*this, parentDlg, false, jobName),
    ignoreErrors(ignoreAllErrors) {}


SyncStatusHandler::~SyncStatusHandler()
{
    const int totalErrors = errorLog.errorsTotal(); //evaluate before finalizing log

    //finalize error log
    if (abortIsRequested())
        errorLog.logError(wxString(_("Synchronization aborted!")) + wxT(" \n") + _("You may try to synchronize remaining items again (WITHOUT having to re-compare)!"));
    else if (totalErrors)
        errorLog.logWarning(wxString(_("Synchronization completed with errors!")) + wxT(" \n") + _("You may try to synchronize remaining items again (WITHOUT having to re-compare)!"));
    else
        errorLog.logInfo(_("Synchronization completed successfully!"));


    //print the results list
    wxString finalMessage;
    if (totalErrors > 0)
    {
        wxString header(_("Warning: Synchronization failed for %x item(s):"));
        header.Replace(wxT("%x"), ffs3::numberToStringSep(totalErrors), false);
        finalMessage += header + wxT("\n\n");
    }

    const ErrorLogging::MessageEntry& messages = errorLog.getFormattedMessages();
    for (ErrorLogging::MessageEntry::const_iterator i = messages.begin(); i != messages.end(); ++i)
    {
        finalMessage += *i;
        finalMessage += wxT("\n\n");
    }

    //notify to syncStatusFrame that current process has ended
    if (abortIsRequested())
        syncStatusFrame.processHasFinished(SyncStatus::ABORTED, finalMessage);  //enable okay and close events
    else if (totalErrors > 0)
        syncStatusFrame.processHasFinished(SyncStatus::FINISHED_WITH_ERROR, finalMessage);
    else
        syncStatusFrame.processHasFinished(SyncStatus::FINISHED_WITH_SUCCESS, finalMessage);
}


inline
void SyncStatusHandler::reportInfo(const Zstring& text)
{
    //if (currentProcess == StatusHandler::PROCESS_SYNCHRONIZING)
    //errorLog.logInfo(zToWx(text)); -> don't spam with file copy info: visually identifying warning messages has priority!

    syncStatusFrame.setStatusText_NoUpdate(text);
}


void SyncStatusHandler::initNewProcess(int objectsTotal, wxLongLong dataTotal, Process processID)
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


inline
void SyncStatusHandler::updateProcessedData(int objectsProcessed, wxLongLong dataProcessed)
{
    syncStatusFrame.incProgressIndicator_NoUpdate(objectsProcessed, dataProcessed);
}


ErrorHandler::Response SyncStatusHandler::reportError(const wxString& errorMessage)
{
    if (ignoreErrors)
    {
        errorLog.logError(errorMessage);
        return ErrorHandler::IGNORE_ERROR;
    }

    syncStatusFrame.updateStatusDialogNow();

    bool ignoreNextErrors = false;
    ErrorDlg errorDlg(NULL,
                      ErrorDlg::BUTTON_IGNORE |  ErrorDlg::BUTTON_RETRY | ErrorDlg::BUTTON_ABORT,
                      errorMessage,
                      ignoreNextErrors);
    errorDlg.Raise();
    const ErrorDlg::ReturnCodes rv = static_cast<ErrorDlg::ReturnCodes>(errorDlg.ShowModal());
    switch (rv)
    {
    case ErrorDlg::BUTTON_IGNORE:
        ignoreErrors = ignoreNextErrors;
        errorLog.logError(errorMessage);
        return ErrorHandler::IGNORE_ERROR;

    case ErrorDlg::BUTTON_RETRY:
        return ErrorHandler::RETRY;

    case ErrorDlg::BUTTON_ABORT:
        errorLog.logError(errorMessage);
        abortThisProcess();
    }

    assert (false);
    errorLog.logError(errorMessage);
    return ErrorHandler::IGNORE_ERROR;
}


void SyncStatusHandler::reportFatalError(const wxString& errorMessage)
{
    errorLog.logFatalError(errorMessage);
    abortThisProcess();
}


void SyncStatusHandler::reportWarning(const wxString& warningMessage, bool& warningActive)
{
    errorLog.logWarning(warningMessage);

    if (ignoreErrors || !warningActive) //if errors are ignored, then warnings should also
        return;
    else
    {
        syncStatusFrame.updateStatusDialogNow();

        //show popup and ask user how to handle warning
        bool dontWarnAgain = false;
        WarningDlg warningDlg(NULL,
                              WarningDlg::BUTTON_IGNORE | WarningDlg::BUTTON_ABORT,
                              warningMessage,
                              dontWarnAgain);
        warningDlg.Raise();
        const WarningDlg::Response rv = static_cast<WarningDlg::Response>(warningDlg.ShowModal());
        switch (rv)
        {
        case WarningDlg::BUTTON_IGNORE: //no unhandled error situation!
            warningActive = !dontWarnAgain;
            return;

        case WarningDlg::BUTTON_SWITCH:
            assert(false);
        case WarningDlg::BUTTON_ABORT:
            abortThisProcess();
            return;
        }

        assert(false);
    }
}


void SyncStatusHandler::logInfo(const wxString& infoMessage)
{
    errorLog.logInfo(infoMessage);
}


void SyncStatusHandler::forceUiRefresh()
{
    syncStatusFrame.updateStatusDialogNow();
}


void SyncStatusHandler::abortThisProcess()
{
    requestAbortion();
    throw ffs3::AbortThisProcess();  //abort can be triggered by syncStatusFrame
}
