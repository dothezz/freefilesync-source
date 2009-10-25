#include "guiStatusHandler.h"
#include "smallDialogs.h"
#include "../shared/systemConstants.h"
#include "mainDialog.h"
#include <wx/wupdlock.h>
#include "../shared/globalFunctions.h"


CompareStatusHandler::CompareStatusHandler(MainDialog* dlg) :
    mainDialog(dlg),
    ignoreErrors(false),
    currentProcess(StatusHandler::PROCESS_NONE)
{
    wxWindowUpdateLocker dummy(mainDialog); //avoid display distortion

    //prevent user input during "compare", do not disable maindialog since abort-button would also be disabled
    mainDialog->disableAllElements();

    //display status panel during compare
    mainDialog->compareStatus->init(); //clear old values
    mainDialog->compareStatus->Show();

    mainDialog->bSizer1->Layout(); //both sizers need to recalculate!
    mainDialog->bSizer6->Layout(); //adapt layout for wxBitmapWithImage
    mainDialog->Refresh();

    //register abort button
    mainDialog->m_buttonAbort->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( CompareStatusHandler::OnAbortCompare ), NULL, this );
}


CompareStatusHandler::~CompareStatusHandler()
{
    updateUiNow(); //ui update before enabling buttons again: prevent strange behaviour of delayed button clicks

    //reenable complete main dialog
    mainDialog->enableAllElements();

    if (abortIsRequested())
        mainDialog->pushStatusInformation(_("Operation aborted!"));

    //hide status panel from main window
    mainDialog->compareStatus->Hide();

    mainDialog->bSizer6->Layout(); //adapt layout for wxBitmapWithImage
    mainDialog->Layout();
    mainDialog->Refresh();

    //de-register abort button
    mainDialog->m_buttonAbort->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( CompareStatusHandler::OnAbortCompare ), NULL, this );
}


inline
void CompareStatusHandler::updateStatusText(const Zstring& text)
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
        mainDialog->compareStatus->switchToCompareBytewise(objectsTotal, dataTotal);
        mainDialog->Layout();
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
    const wxString errorMessage = message + wxT("\n\n\n") + _("Ignore this error, retry or abort?");
    ErrorDlg* errorDlg = new ErrorDlg(mainDialog,
                                      ErrorDlg::BUTTON_IGNORE |  ErrorDlg::BUTTON_RETRY | ErrorDlg::BUTTON_ABORT,
                                      errorMessage, ignoreNextErrors);
    switch (static_cast<ErrorDlg::ReturnCodes>(errorDlg->ShowModal()))
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
    ErrorDlg* errorDlg = new ErrorDlg(mainDialog,
                                      ErrorDlg::BUTTON_ABORT,
                                      errorMessage, dummy);
    errorDlg->ShowModal();
    abortThisProcess();
}


void CompareStatusHandler::reportWarning(const wxString& warningMessage, bool& warningActive)
{
    if (!warningActive || ignoreErrors) //if errors are ignored, then warnings should also
        return;

    mainDialog->compareStatus->updateStatusPanelNow();

    //show popup and ask user how to handle warning
    bool dontWarnAgain = false;
    WarningDlg* warningDlg = new WarningDlg(mainDialog,
                                            WarningDlg::BUTTON_IGNORE | WarningDlg::BUTTON_ABORT,
                                            warningMessage,
                                            dontWarnAgain);
    switch (static_cast<WarningDlg::Response>(warningDlg->ShowModal()))
    {
    case WarningDlg::BUTTON_ABORT:
        abortThisProcess();
        break;

    case WarningDlg::BUTTON_IGNORE:
        warningActive = !dontWarnAgain;
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
    throw FreeFileSync::AbortThisProcess();  //abort can be triggered by syncStatusFrame
}
//########################################################################################################


SyncStatusHandler::SyncStatusHandler(wxWindow* dlg, bool ignoreAllErrors) :
    ignoreErrors(ignoreAllErrors)
{
    syncStatusFrame = new SyncStatus(this, dlg);
    syncStatusFrame->Show();
    updateUiNow();
}


SyncStatusHandler::~SyncStatusHandler()
{
    //print the results list
    wxString result;
    if (errorLog.messageCount() > 0)
    {
        if (errorLog.errorsTotal() > 0)
        {
            wxString header(_("Warning: Synchronization failed for %x item(s):"));
            header.Replace(wxT("%x"), globalFunctions::numberToWxString(errorLog.errorsTotal()), false);
            result += header + wxT("\n\n");
        }

        const std::vector<wxString>& messages = errorLog.getFormattedMessages();
        for (std::vector<wxString>::const_iterator i = messages.begin(); i != messages.end(); ++i)
        {
            result += *i;
            result += wxChar('\n');
        }

        result += wxT("\n");
    }

    //notify to syncStatusFrame that current process has ended
    if (abortIsRequested())
    {
        result += wxString(_("Synchronization aborted!")) + wxT(" ") + _("You may try to synchronize remaining items again (WITHOUT having to re-compare)!");
        syncStatusFrame->processHasFinished(SyncStatus::ABORTED, result);  //enable okay and close events
    }
    else if (errorLog.errorsTotal() > 0)
    {
        result += wxString(_("Synchronization completed with errors!")) + wxT(" ") + _("You may try to synchronize remaining items again (WITHOUT having to re-compare)!");
        syncStatusFrame->processHasFinished(SyncStatus::FINISHED_WITH_ERROR, result);
    }
    else
    {
        result += _("Synchronization completed successfully!");
        syncStatusFrame->processHasFinished(SyncStatus::FINISHED_WITH_SUCCESS, result);
    }
}


inline
void SyncStatusHandler::updateStatusText(const Zstring& text)
{
    syncStatusFrame->setStatusText_NoUpdate(text);
}


void SyncStatusHandler::initNewProcess(int objectsTotal, wxLongLong dataTotal, Process processID)
{
    switch (processID)
    {
    case StatusHandler::PROCESS_SYNCHRONIZING:
        syncStatusFrame->resetGauge(objectsTotal, dataTotal);
        syncStatusFrame->setCurrentStatus(SyncStatus::SYNCHRONIZING);
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
    syncStatusFrame->incProgressIndicator_NoUpdate(objectsProcessed, dataProcessed);
}


ErrorHandler::Response SyncStatusHandler::reportError(const wxString& errorMessage)
{
    if (ignoreErrors)
    {
        errorLog.logError(errorMessage);
        return ErrorHandler::IGNORE_ERROR;
    }

    syncStatusFrame->updateStatusDialogNow();

    bool ignoreNextErrors = false;
    ErrorDlg* errorDlg = new ErrorDlg(syncStatusFrame,
                                      ErrorDlg::BUTTON_IGNORE |  ErrorDlg::BUTTON_RETRY | ErrorDlg::BUTTON_ABORT,
                                      errorMessage + wxT("\n\n\n") + _("Ignore this error, retry or abort synchronization?"),
                                      ignoreNextErrors);
    switch (static_cast<ErrorDlg::ReturnCodes>(errorDlg->ShowModal()))
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
        syncStatusFrame->updateStatusDialogNow();

        //show popup and ask user how to handle warning
        bool dontWarnAgain = false;
        WarningDlg* warningDlg = new WarningDlg(syncStatusFrame,
                                                WarningDlg::BUTTON_IGNORE | WarningDlg::BUTTON_ABORT,
                                                warningMessage,
                                                dontWarnAgain);
        switch (static_cast<WarningDlg::Response>(warningDlg->ShowModal()))
        {
        case WarningDlg::BUTTON_IGNORE: //no unhandled error situation!
            warningActive = !dontWarnAgain;
            return;

        case WarningDlg::BUTTON_ABORT:
            abortThisProcess();
            return;
        }

        assert(false);
    }
}


void SyncStatusHandler::forceUiRefresh()
{
    syncStatusFrame->updateStatusDialogNow();
}


void SyncStatusHandler::abortThisProcess()
{
    requestAbortion();
    throw FreeFileSync::AbortThisProcess();  //abort can be triggered by syncStatusFrame
}
