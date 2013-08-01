// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) Zenju (zenju AT gmx DOT de) - All Rights Reserved        *
// **************************************************************************

#include "gui_status_handler.h"
#include <wx/wupdlock.h>
#include <wx+/shell_execute.h>
#include <wx+/button.h>
#include <wx/app.h>
#include "msg_popup.h"
#include "main_dlg.h"
#include "exec_finished_box.h"
#include "../lib/generate_logfile.h"
#include "../lib/resolve_path.h"

using namespace zen;
using namespace xmlAccess;


CompareStatusHandler::CompareStatusHandler(MainDialog& dlg) :
    mainDlg(dlg),
    ignoreErrors(false)
{
    {
        wxWindowUpdateLocker dummy(&mainDlg); //avoid display distortion

        //display status panel during compare
        mainDlg.compareStatus->init(*this); //clear old values before showing panel
        mainDlg.auiMgr.GetPane(mainDlg.compareStatus->getAsWindow()).Show();
        mainDlg.auiMgr.Update();
    }

    mainDlg.Update(); //don't wait until idle event!

    //register keys
    mainDlg.Connect(wxEVT_CHAR_HOOK, wxKeyEventHandler(CompareStatusHandler::OnKeyPressed), nullptr, this);
    mainDlg.m_buttonCancel->Connect(wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler(CompareStatusHandler::OnAbortCompare), nullptr, this);
}


CompareStatusHandler::~CompareStatusHandler()
{
    //unregister keys
    mainDlg.Disconnect(wxEVT_CHAR_HOOK, wxKeyEventHandler(CompareStatusHandler::OnKeyPressed), nullptr, this);
    mainDlg.m_buttonCancel->Disconnect(wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler(CompareStatusHandler::OnAbortCompare), nullptr, this);

    mainDlg.compareStatus->finalize();
    mainDlg.auiMgr.GetPane(mainDlg.compareStatus->getAsWindow()).Hide();
    mainDlg.auiMgr.Update();
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

SyncStatusHandler::SyncStatusHandler(wxFrame* parentDlg,
                                     size_t lastSyncsLogFileSizeMax,
                                     OnGuiError handleError,
                                     const std::wstring& jobName,
                                     const std::wstring& execWhenFinished,
                                     std::vector<std::wstring>& execFinishedHistory) :
    progressDlg(createProgressDialog(*this, [this] { this->onProgressDialogTerminate(); }, *this, parentDlg, true, jobName, execWhenFinished, execFinishedHistory)),
            lastSyncsLogFileSizeMax_(lastSyncsLogFileSizeMax),
            handleError_(handleError),
            jobName_(jobName)
{
    totalTime.Start(); //measure total time
}


SyncStatusHandler::~SyncStatusHandler()
{
    const int totalErrors   = errorLog.getItemCount(TYPE_ERROR | TYPE_FATAL_ERROR); //evaluate before finalizing log
    const int totalWarnings = errorLog.getItemCount(TYPE_WARNING);

    //finalize error log
    std::wstring finalStatus;
    if (abortIsRequested())
    {
        finalStatus = _("Synchronization aborted");
        errorLog.logMsg(finalStatus, TYPE_ERROR);
    }
    else if (totalErrors > 0)
    {
        finalStatus = _("Synchronization completed with errors");
        errorLog.logMsg(finalStatus, TYPE_ERROR);
    }
    else if (totalWarnings > 0)
    {
        finalStatus = _("Synchronization completed with warnings");
        errorLog.logMsg(finalStatus, TYPE_WARNING); //give status code same warning priority as display category!
    }
    else
    {
        if (getObjectsTotal(PHASE_SYNCHRONIZING) == 0 && //we're past "initNewPhase(PHASE_SYNCHRONIZING)" at this point!
            getDataTotal   (PHASE_SYNCHRONIZING) == 0)
            finalStatus = _("Nothing to synchronize"); //even if "ignored conflicts" occurred!
        else
            finalStatus = _("Synchronization completed successfully");
        errorLog.logMsg(finalStatus, TYPE_INFO);
    }

    const SummaryInfo summary =
    {
        jobName_, finalStatus,
        getObjectsCurrent(PHASE_SYNCHRONIZING), getDataCurrent(PHASE_SYNCHRONIZING),
        getObjectsTotal  (PHASE_SYNCHRONIZING), getDataTotal  (PHASE_SYNCHRONIZING),
        totalTime.Time() / 1000
    };

    try
    {
        saveToLastSyncsLog(summary, errorLog, lastSyncsLogFileSizeMax_); //throw FileError
    }
    catch (FileError&) {}

    if (progressDlg)
    {
        bool showFinalResults = true;
        //execute "on completion" command (even in case of ignored errors)
        if (!abortIsRequested()) //if aborted (manually), we don't execute the command
        {
            const std::wstring finalCommand = progressDlg->getExecWhenFinishedCommand(); //final value (after possible user modification)
            if (isCloseProgressDlgCommand(finalCommand))
                showFinalResults = false; //take precedence over current visibility status
            else if (!finalCommand.empty())
            {
                auto cmdexp = expandMacros(utfCvrtTo<Zstring>(finalCommand));
                shellExecute(cmdexp);
            }
        }

        //notify to progressDlg that current process has ended
        if (showFinalResults)
        {
            if (abortIsRequested())
                progressDlg->processHasFinished(SyncProgressDialog::RESULT_ABORTED, errorLog);  //enable okay and close events
            else if (totalErrors > 0)
                progressDlg->processHasFinished(SyncProgressDialog::RESULT_FINISHED_WITH_ERROR, errorLog);
            else if (totalWarnings > 0)
                progressDlg->processHasFinished(SyncProgressDialog::RESULT_FINISHED_WITH_WARNINGS, errorLog);
            else
                progressDlg->processHasFinished(SyncProgressDialog::RESULT_FINISHED_WITH_SUCCESS, errorLog);
        }
        else
            progressDlg->closeWindowDirectly();

        //wait until progress dialog notified shutdown via onProgressDialogTerminate()
        //-> required since it has our "this" pointer captured in lambda "notifyWindowTerminate"!
        //-> nicely manages dialog lifetime
        while (progressDlg)
        {
            wxTheApp->Yield(); //*first* refresh GUI (removing flicker) before sleeping!
            boost::this_thread::sleep(boost::posix_time::milliseconds(UI_UPDATE_INTERVAL));
        }
    }
}


void SyncStatusHandler::initNewPhase(int objectsTotal, Int64 dataTotal, Phase phaseID)
{
    assert(phaseID == PHASE_SYNCHRONIZING);
    StatusHandler::initNewPhase(objectsTotal, dataTotal, phaseID);
    if (progressDlg)
        progressDlg->initNewPhase(); //call after "StatusHandler::initNewPhase"
}


void SyncStatusHandler::updateProcessedData(int objectsDelta, Int64 dataDelta)
{
    StatusHandler::updateProcessedData(objectsDelta, dataDelta);
    if (progressDlg)
        progressDlg->notifyProgressChange(); //noexcept
    //note: this method should NOT throw in order to properly allow undoing setting of statistics!
}


void SyncStatusHandler::reportInfo(const std::wstring& text)
{
    StatusHandler::reportInfo(text);
    errorLog.logMsg(text, TYPE_INFO);
}


ProcessCallback::Response SyncStatusHandler::reportError(const std::wstring& errorMessage)
{
    //always, except for "retry":
    zen::ScopeGuard guardWriteLog = zen::makeGuard([&] { errorLog.logMsg(errorMessage, TYPE_ERROR); });

    switch (handleError_)
    {
        case ON_GUIERROR_POPUP:
        {
            if (!progressDlg) abortThisProcess();
            PauseTimers dummy(*progressDlg);
            forceUiRefresh();

            bool ignoreNextErrors = false;
            switch (showErrorDlg(progressDlg->getWindowIfVisible(),
                                 ReturnErrorDlg::BUTTON_IGNORE | ReturnErrorDlg::BUTTON_RETRY | ReturnErrorDlg::BUTTON_CANCEL,
                                 errorMessage,
                                 &ignoreNextErrors))
            {
                case ReturnErrorDlg::BUTTON_IGNORE:
                    if (ignoreNextErrors) //falsify only
                        handleError_ = ON_GUIERROR_IGNORE;
                    return ProcessCallback::IGNORE_ERROR;

                case ReturnErrorDlg::BUTTON_RETRY:
                    guardWriteLog.dismiss();
                    errorLog.logMsg(_("Retrying operation after error:") + L" " + errorMessage, TYPE_INFO); //explain why there are duplicate "doing operation X" info messages in the log!
                    return ProcessCallback::RETRY;

                case ReturnErrorDlg::BUTTON_CANCEL:
                    abortThisProcess();
                    break;
            }
        }
        break;

        case ON_GUIERROR_IGNORE:
            return ProcessCallback::IGNORE_ERROR;
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
            if (!progressDlg) abortThisProcess();
            PauseTimers dummy(*progressDlg);
            forceUiRefresh();

            bool ignoreNextErrors = false;
            switch (showFatalErrorDlg(progressDlg->getWindowIfVisible(),
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
            if (!progressDlg) abortThisProcess();
            PauseTimers dummy(*progressDlg);
            forceUiRefresh();

            bool dontWarnAgain = false;
            switch (showWarningDlg(progressDlg->getWindowIfVisible(),
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
    if (progressDlg)
        progressDlg->updateGui();
}


void SyncStatusHandler::abortThisProcess()
{
    requestAbortion(); //just make sure...
    throw GuiAbortProcess();  //abort can be triggered by progressDlg
}


void SyncStatusHandler::onProgressDialogTerminate()
{
    //it's responsibility of "progressDlg" to call requestAbortion() when closing dialog
    progressDlg = nullptr;
}
