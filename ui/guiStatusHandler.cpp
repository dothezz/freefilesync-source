#include "guiStatusHandler.h"
#include "../library/customButton.h"
#include "smallDialogs.h"
#include "../library/globalFunctions.h"
#include "mainDialog.h"


CompareStatusHandler::CompareStatusHandler(MainDialog* dlg) :
        mainDialog(dlg),
        ignoreErrors(false),
        currentProcess(StatusHandler::PROCESS_NONE)
{
    //prevent user input during "compare", do not disable maindialog since abort-button would also be disabled
    //it's not nice, but works
    mainDialog->m_notebookBottomLeft->Disable();
    mainDialog->m_radioBtnSizeDate->Disable();
    mainDialog->m_radioBtnContent->Disable();
    mainDialog->m_bpButtonFilter->Disable();
    mainDialog->m_hyperlinkCfgFilter->Disable();
    mainDialog->m_checkBoxHideFilt->Disable();
    mainDialog->m_bpButtonSyncConfig->Disable();
    mainDialog->m_buttonStartSync->Disable();
    mainDialog->m_dirPickerLeft->Disable();
    mainDialog->m_dirPickerRight->Disable();
    mainDialog->m_bpButtonSwapSides->Disable();
    mainDialog->m_bpButtonLeftOnly->Disable();
    mainDialog->m_bpButtonLeftNewer->Disable();
    mainDialog->m_bpButtonEqual->Disable();
    mainDialog->m_bpButtonDifferent->Disable();
    mainDialog->m_bpButtonRightNewer->Disable();
    mainDialog->m_bpButtonRightOnly->Disable();
    mainDialog->m_panelLeft->Disable();
    mainDialog->m_panelMiddle->Disable();
    mainDialog->m_panelRight->Disable();
    mainDialog->m_panelTopLeft->Disable();
    mainDialog->m_panelTopMiddle->Disable();
    mainDialog->m_panelTopRight->Disable();
    mainDialog->m_bpButtonSave->Disable();
    mainDialog->m_bpButtonLoad->Disable();
    mainDialog->m_choiceHistory->Disable();
    mainDialog->m_bpButton10->Disable();
    mainDialog->m_bpButton14->Disable();
    mainDialog->m_scrolledWindowFolderPairs->Disable();
    mainDialog->m_menubar1->EnableTop(0, false);
    mainDialog->m_menubar1->EnableTop(1, false);
    mainDialog->m_menubar1->EnableTop(2, false);

    //show abort button
    mainDialog->m_buttonAbort->Enable();
    mainDialog->m_buttonAbort->Show();
    mainDialog->m_buttonCompare->Disable();
    mainDialog->m_buttonCompare->Hide();
    mainDialog->m_buttonAbort->SetFocus();

    //display status panel during compare
    mainDialog->compareStatus->init(); //clear old values
    mainDialog->compareStatus->Show();

    mainDialog->bSizer1->Layout(); //both sizers need to recalculate!
    mainDialog->bSizer6->Layout(); //adapt layout for wxBitmapWithImage
    mainDialog->Refresh();
}


CompareStatusHandler::~CompareStatusHandler()
{
    updateUiNow(); //ui update before enabling buttons again: prevent strange behaviour of delayed button clicks

    //reenable complete main dialog
    mainDialog->m_notebookBottomLeft->Enable();
    mainDialog->m_radioBtnSizeDate->Enable();
    mainDialog->m_radioBtnContent->Enable();
    mainDialog->m_bpButtonFilter->Enable();
    mainDialog->m_hyperlinkCfgFilter->Enable();
    mainDialog->m_checkBoxHideFilt->Enable();
    mainDialog->m_bpButtonSyncConfig->Enable();
    mainDialog->m_buttonStartSync->Enable();
    mainDialog->m_dirPickerLeft->Enable();
    mainDialog->m_dirPickerRight->Enable();
    mainDialog->m_bpButtonSwapSides->Enable();
    mainDialog->m_bpButtonLeftOnly->Enable();
    mainDialog->m_bpButtonLeftNewer->Enable();
    mainDialog->m_bpButtonEqual->Enable();
    mainDialog->m_bpButtonDifferent->Enable();
    mainDialog->m_bpButtonRightNewer->Enable();
    mainDialog->m_bpButtonRightOnly->Enable();
    mainDialog->m_panelLeft->Enable();
    mainDialog->m_panelMiddle->Enable();
    mainDialog->m_panelRight->Enable();
    mainDialog->m_panelTopLeft->Enable();
    mainDialog->m_panelTopMiddle->Enable();
    mainDialog->m_panelTopRight->Enable();
    mainDialog->m_bpButtonSave->Enable();
    mainDialog->m_bpButtonLoad->Enable();
    mainDialog->m_choiceHistory->Enable();
    mainDialog->m_bpButton10->Enable();
    mainDialog->m_bpButton14->Enable();
    mainDialog->m_scrolledWindowFolderPairs->Enable();
    mainDialog->m_menubar1->EnableTop(0, true);
    mainDialog->m_menubar1->EnableTop(1, true);
    mainDialog->m_menubar1->EnableTop(2, true);

    if (abortIsRequested())
        mainDialog->pushStatusInformation(_("Operation aborted!"));

    mainDialog->m_buttonAbort->Disable();
    mainDialog->m_buttonAbort->Hide();
    mainDialog->m_buttonCompare->Enable(); //enable compare button
    mainDialog->m_buttonCompare->Show();

    //hide status panel from main window
    mainDialog->compareStatus->Hide();

    mainDialog->bSizer6->Layout(); //adapt layout for wxBitmapWithImage

    mainDialog->Layout();
    mainDialog->Refresh();
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


ErrorHandler::Response CompareStatusHandler::reportError(const Zstring& text)
{
    if (ignoreErrors)
        return ErrorHandler::IGNORE_ERROR;

    mainDialog->compareStatus->updateStatusPanelNow();

    bool ignoreNextErrors = false;
    wxString errorMessage = wxString(text.c_str()) + wxT("\n\n") + _("Ignore this error, retry or abort?");
    ErrorDlg* errorDlg = new ErrorDlg(mainDialog,
                                      ErrorDlg::BUTTON_IGNORE |  ErrorDlg::BUTTON_RETRY | ErrorDlg::BUTTON_ABORT,
                                      errorMessage, ignoreNextErrors);
    int rv = errorDlg->ShowModal();
    switch (rv)
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


void CompareStatusHandler::reportFatalError(const Zstring& errorMessage)
{
    mainDialog->compareStatus->updateStatusPanelNow();

    bool dummy = false;
    ErrorDlg* errorDlg = new ErrorDlg(mainDialog,
                                      ErrorDlg::BUTTON_ABORT,
                                      errorMessage.c_str(), dummy);
    errorDlg->ShowModal();
    abortThisProcess();
}


void CompareStatusHandler::reportWarning(const Zstring& warningMessage, bool& dontShowAgain)
{
    if (ignoreErrors) //if errors are ignored, then warnings should also
        return;

    mainDialog->compareStatus->updateStatusPanelNow();

    //show popup and ask user how to handle warning
    bool dontWarnAgain = false;
    WarningDlg* warningDlg = new WarningDlg(mainDialog,
                                            WarningDlg::BUTTON_IGNORE | WarningDlg::BUTTON_ABORT,
                                            warningMessage.c_str(),
                                            dontWarnAgain);
    switch (warningDlg->ShowModal())
    {
    case WarningDlg::BUTTON_ABORT:
        abortThisProcess();

    case WarningDlg::BUTTON_IGNORE:
        dontShowAgain = dontWarnAgain;
        return;
    }

    assert(false);
}


inline
void CompareStatusHandler::forceUiRefresh()
{
    mainDialog->compareStatus->updateStatusPanelNow();
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
    unsigned int failedItems = unhandledErrors.GetCount();
    wxString result;
    if (failedItems)
    {
        result = wxString(_("Warning: Synchronization failed for %x item(s):")) + wxT("\n\n");
        result.Replace(wxT("%x"), globalFunctions::numberToWxString(failedItems), false);

        for (unsigned int j = 0; j < failedItems; ++j)
        {   //remove linebreaks
            wxString errorMessage = unhandledErrors[j];
            for (wxString::iterator i = errorMessage.begin(); i != errorMessage.end(); ++i)
                if (*i == wxChar('\n'))
                    *i = wxChar(' ');

            result += errorMessage + wxT("\n");
        }
        result+= wxT("\n");
    }

    //notify to syncStatusFrame that current process has ended
    if (abortIsRequested())
    {
        result+= wxString(_("Synchronization aborted!")) + wxT(" ") + _("You may try to synchronize remaining items again (WITHOUT having to re-compare)!");
        syncStatusFrame->setStatusText_NoUpdate(result.c_str());
        syncStatusFrame->processHasFinished(SyncStatus::ABORTED);  //enable okay and close events
    }
    else  if (failedItems)
    {
        result+= wxString(_("Synchronization completed with errors!")) + wxT(" ") + _("You may try to synchronize remaining items again (WITHOUT having to re-compare)!");
        syncStatusFrame->setStatusText_NoUpdate(result.c_str());
        syncStatusFrame->processHasFinished(SyncStatus::FINISHED_WITH_ERROR);
    }
    else
    {
        result+= _("Synchronization completed successfully!");
        syncStatusFrame->setStatusText_NoUpdate(result.c_str());
        syncStatusFrame->processHasFinished(SyncStatus::FINISHED_WITH_SUCCESS);
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


ErrorHandler::Response SyncStatusHandler::reportError(const Zstring& text)
{
    //add current time before error message
    wxString errorWithTime = wxString(wxT("[")) + wxDateTime::Now().FormatTime() + wxT("] ") + text.c_str();

    if (ignoreErrors)
    {
        unhandledErrors.Add(errorWithTime);
        return ErrorHandler::IGNORE_ERROR;
    }

    syncStatusFrame->updateStatusDialogNow();

    bool ignoreNextErrors = false;
    ErrorDlg* errorDlg = new ErrorDlg(syncStatusFrame,
                                      ErrorDlg::BUTTON_IGNORE |  ErrorDlg::BUTTON_RETRY | ErrorDlg::BUTTON_ABORT,
                                      wxString(text) + wxT("\n\n") + _("Ignore this error, retry or abort synchronization?"),
                                      ignoreNextErrors);
    int rv = errorDlg->ShowModal();
    switch (rv)
    {
    case ErrorDlg::BUTTON_IGNORE:
        ignoreErrors = ignoreNextErrors;
        unhandledErrors.Add(errorWithTime);
        return ErrorHandler::IGNORE_ERROR;

    case ErrorDlg::BUTTON_RETRY:
        return ErrorHandler::RETRY;

    case ErrorDlg::BUTTON_ABORT:
        unhandledErrors.Add(errorWithTime);
        abortThisProcess();
    }

    assert (false);
    unhandledErrors.Add(errorWithTime);
    return ErrorHandler::IGNORE_ERROR;
}


void SyncStatusHandler::reportFatalError(const Zstring& errorMessage)
{   //add current time before error message
    wxString errorWithTime = wxString(wxT("[")) + wxDateTime::Now().FormatTime() + wxT("] ") + errorMessage.c_str();

    unhandledErrors.Add(errorWithTime);
    abortThisProcess();
}


void SyncStatusHandler::reportWarning(const Zstring& warningMessage, bool& dontShowAgain)
{   //add current time before warning message
    wxString warningWithTime = wxString(wxT("[")) + wxDateTime::Now().FormatTime() + wxT("] ") + warningMessage.c_str();

    if (ignoreErrors) //if errors are ignored, then warnings should also
        return; //no unhandled error situation!

    syncStatusFrame->updateStatusDialogNow();

    //show popup and ask user how to handle warning
    bool dontWarnAgain = false;
    WarningDlg* warningDlg = new WarningDlg(syncStatusFrame,
                                            WarningDlg::BUTTON_IGNORE | WarningDlg::BUTTON_ABORT,
                                            warningMessage.c_str(),
                                            dontWarnAgain);
    switch (warningDlg->ShowModal())
    {
    case WarningDlg::BUTTON_IGNORE: //no unhandled error situation!
        dontShowAgain = dontWarnAgain;
        return;

    case WarningDlg::BUTTON_ABORT:
        unhandledErrors.Add(warningWithTime);
        abortThisProcess();
    }

    assert(false);
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
