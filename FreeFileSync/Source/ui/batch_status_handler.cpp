// *****************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under    *
// * GNU General Public License: http://www.gnu.org/licenses/gpl-3.0           *
// * Copyright (C) Zenju (zenju AT freefilesync DOT org) - All Rights Reserved *
// *****************************************************************************

#include "batch_status_handler.h"
#include <zen/shell_execute.h>
#include <zen/thread.h>
#include <wx+/popup_dlg.h>
#include <wx/app.h>
#include "on_completion_box.h"
#include "../lib/ffs_paths.h"
#include "../lib/resolve_path.h"
#include "../lib/status_handler_impl.h"
#include "../lib/generate_logfile.h"
#include "../fs/concrete.h"

using namespace zen;


namespace
{
//"Backup FreeFileSync 2013-09-15 015052.log" ->
//"Backup FreeFileSync 2013-09-15 015052 [Error].log"

//return value always bound!
std::unique_ptr<AFS::OutputStream> prepareNewLogfile(const AbstractPath& logFolderPath, //throw FileError
                                                     const std::wstring& jobName,
                                                     const TimeComp& timeStamp,
                                                     const std::wstring& status,
                                                     ProcessCallback& pc)
{
    assert(!jobName.empty());

    //create logfile folder if required
    AFS::createFolderIfMissingRecursion(logFolderPath); //throw FileError

    //const std::string colon = "\xcb\xb8"; //="modifier letter raised colon" => regular colon is forbidden in file names on Windows and OS X
    //=> too many issues, most notably cmd.exe is not Unicode-awere: http://www.freefilesync.org/forum/viewtopic.php?t=1679

    //assemble logfile name
    Zstring body = utfTo<Zstring>(jobName) + Zstr(" ") + formatTime<Zstring>(Zstr("%Y-%m-%d %H%M%S"), timeStamp);
    if (!status.empty())
        body += utfTo<Zstring>(L" [" + status + L"]");

    //ensure uniqueness; avoid file system race-condition!
    Zstring logFileName = body + Zstr(".log");
    for (int i = 0;; ++i)
        try
        {
            const AbstractPath logFilePath = AFS::appendRelPath(logFolderPath, logFileName);

            return AFS::getOutputStream(logFilePath, //throw FileError, ErrorTargetExisting
                                        nullptr, /*streamSize*/
                                        nullptr /*modificationTime*/,
                                        OnUpdateLogfileStatusNoThrow(pc, AFS::getDisplayPath(logFilePath)));
        }
        catch (ErrorTargetExisting&)
        {
            if (i == 10) throw; //avoid endless recursion in pathological cases
            logFileName = body + Zstr('_') + numberTo<Zstring>(i) + Zstr(".log");
        }
}


struct LogTraverserCallback: public AFS::TraverserCallback
{
    LogTraverserCallback(const Zstring& prefix, const std::function<void()>& onUpdateStatus) :
        prefix_(prefix),
        onUpdateStatus_(onUpdateStatus) {}

    void onFile(const FileInfo& fi) override
    {
        if (startsWith(fi.itemName, prefix_, CmpFilePath() /*even on Linux!*/) && endsWith(fi.itemName, Zstr(".log"), CmpFilePath()))
            logFileNames_.push_back(fi.itemName);

        if (onUpdateStatus_) onUpdateStatus_();
    }
    std::unique_ptr<TraverserCallback> onFolder (const FolderInfo&  fi) override { return nullptr; }
    HandleLink                         onSymlink(const SymlinkInfo& si) override { return TraverserCallback::LINK_SKIP; }

    HandleError reportDirError (const std::wstring& msg, size_t retryNumber                         ) override { setError(msg); return ON_ERROR_CONTINUE; }
    HandleError reportItemError(const std::wstring& msg, size_t retryNumber, const Zstring& itemName) override { setError(msg); return ON_ERROR_CONTINUE; }

    const std::vector<Zstring>& refFileNames() const { return logFileNames_; }
    const Opt<FileError>& getLastError() const { return lastError_; }

private:
    void setError(const std::wstring& msg) //implement late failure
    {
        if (!lastError_)
            lastError_ = FileError(msg);
    }

    const Zstring prefix_;
    const std::function<void()> onUpdateStatus_;
    std::vector<Zstring> logFileNames_; //out
    Opt<FileError> lastError_;
};


void limitLogfileCount(const AbstractPath& logFolderPath, const std::wstring& jobname, size_t maxCount, const std::function<void()>& onUpdateStatus) //throw FileError
{
    const Zstring prefix = utfTo<Zstring>(jobname);

    LogTraverserCallback lt(prefix, onUpdateStatus); //traverse source directory one level deep
    AFS::traverseFolder(logFolderPath, lt);

    std::vector<Zstring> logFileNames = lt.refFileNames();
    Opt<FileError> lastError = lt.getLastError();

    if (logFileNames.size() > maxCount)
    {
        //delete oldest logfiles: take advantage of logfile naming convention to find them
        std::nth_element(logFileNames.begin(), logFileNames.end() - maxCount, logFileNames.end(), LessFilePath());

        std::for_each(logFileNames.begin(), logFileNames.end() - maxCount, [&](const Zstring& logFileName)
        {
            try
            {
                AFS::removeFilePlain(AFS::appendRelPath(logFolderPath, logFileName)); //throw FileError
            }
            catch (const FileError& e) { if (!lastError) lastError = e; };

            if (onUpdateStatus) onUpdateStatus();
        });
    }

    if (lastError) //late failure!
        throw* lastError;
}
}

//##############################################################################################################################

BatchStatusHandler::BatchStatusHandler(bool showProgress,
                                       const std::wstring& jobName,
                                       const Zstring& soundFileSyncComplete,
                                       const TimeComp& timeStamp,
                                       const Zstring& logFolderPathPhrase, //may be empty
                                       int logfilesCountLimit,
                                       size_t lastSyncsLogFileSizeMax,
                                       const xmlAccess::OnError handleError,
                                       size_t automaticRetryCount,
                                       size_t automaticRetryDelay,
                                       FfsReturnCode& returnCode,
                                       const Zstring& onCompletion,
                                       std::vector<Zstring>& onCompletionHistory) :
    showFinalResults_(showProgress), //=> exit immediately or wait when finished
    logfilesCountLimit_(logfilesCountLimit),
    lastSyncsLogFileSizeMax_(lastSyncsLogFileSizeMax),
    handleError_(handleError),
    returnCode_(returnCode),
    automaticRetryCount_(automaticRetryCount),
    automaticRetryDelay_(automaticRetryDelay),
    progressDlg_(createProgressDialog(*this, [this] { this->onProgressDialogTerminate(); }, *this, nullptr, showProgress, jobName, soundFileSyncComplete, onCompletion, onCompletionHistory)),
             jobName_(jobName),
             timeStamp_(timeStamp),
             logFolderPathPhrase_(logFolderPathPhrase)
{
    //ATTENTION: "progressDlg_" is an unmanaged resource!!! However, at this point we already consider construction complete! =>
    //ZEN_ON_SCOPE_FAIL( cleanup(); ); //destructor call would lead to member double clean-up!!!

    //...

    //if (logFile)
    //  ::wxSetEnv(L"logfile", utfTo<wxString>(logFile->getFilename()));
}


BatchStatusHandler::~BatchStatusHandler()
{
    //------------ "on completion" command conceptually is part of the sync, not cleanup --------------------------------------

    //decide whether to stay on status screen or exit immediately...
    if (progressDlg_)
    {
        if (switchToGuiRequested_) //-> avoid recursive yield() calls, thous switch not before ending batch mode
            showFinalResults_ = false;
        else if (progressDlg_->getWindowIfVisible())
            showFinalResults_ = true;

        //execute "on completion" command (even in case of ignored errors)
        if (!abortIsRequested()) //if aborted (manually), we don't execute the command
        {
            const Zstring finalCommand = progressDlg_->getExecWhenFinishedCommand(); //final value (after possible user modification)
            if (!finalCommand.empty())
            {
                if (isCloseProgressDlgCommand(finalCommand))
                    showFinalResults_ = false; //take precedence over current visibility status
                else
                    try
                    {
                        //use EXEC_TYPE_ASYNC until there is reason not to: http://www.freefilesync.org/forum/viewtopic.php?t=31
                        tryReportingError([&] { shellExecute(expandMacros(finalCommand), EXEC_TYPE_ASYNC); }, //throw FileError
                                          *this); //throw X?
                    }
                    catch (...) {}
            }
        }
    }
    //------------ end of sync: begin of cleanup --------------------------------------

    const int totalErrors   = errorLog_.getItemCount(TYPE_ERROR | TYPE_FATAL_ERROR); //evaluate before finalizing log
    const int totalWarnings = errorLog_.getItemCount(TYPE_WARNING);

    //finalize error log
    std::wstring status; //additionally indicate errors in log file name
    std::wstring finalStatusMsg;
    if (abortIsRequested())
    {
        raiseReturnCode(returnCode_, FFS_RC_ABORTED);
        finalStatusMsg = _("Synchronization stopped");
        errorLog_.logMsg(finalStatusMsg, TYPE_ERROR);
        status = _("Stopped");
    }
    else if (totalErrors > 0)
    {
        raiseReturnCode(returnCode_, FFS_RC_FINISHED_WITH_ERRORS);
        finalStatusMsg = _("Synchronization completed with errors");
        errorLog_.logMsg(finalStatusMsg, TYPE_ERROR);
        status = _("Error");
    }
    else if (totalWarnings > 0)
    {
        raiseReturnCode(returnCode_, FFS_RC_FINISHED_WITH_WARNINGS);
        finalStatusMsg = _("Synchronization completed with warnings");
        errorLog_.logMsg(finalStatusMsg, TYPE_WARNING);
        status = _("Warning");
    }
    else
    {
        if (getItemsTotal(PHASE_SYNCHRONIZING) == 0 && //we're past "initNewPhase(PHASE_SYNCHRONIZING)" at this point!
            getBytesTotal(PHASE_SYNCHRONIZING) == 0)
            finalStatusMsg = _("Nothing to synchronize"); //even if "ignored conflicts" occurred!
        else
            finalStatusMsg = _("Synchronization completed successfully");
        errorLog_.logMsg(finalStatusMsg, TYPE_INFO);
    }

    const SummaryInfo summary =
    {
        jobName_,
        finalStatusMsg,
        getItemsCurrent(PHASE_SYNCHRONIZING), getBytesCurrent(PHASE_SYNCHRONIZING),
        getItemsTotal  (PHASE_SYNCHRONIZING), getBytesTotal  (PHASE_SYNCHRONIZING),
        std::time(nullptr) - startTime_
    };

    //----------------- write results into user-specified logfile ------------------------
    //create not before destruction: 1. avoid issues with FFS trying to sync open log file 2. simplify transactional retry on failure 3. no need to rename log file to include status
    // 4. failure to write to particular stream must not be retried!
    if (logfilesCountLimit_ != 0)
    {
        auto requestUiRefreshNoThrow = [&] { try { requestUiRefresh();  /*throw X*/ } catch (...) {} };

        const AbstractPath logFolderPath = createAbstractPath(trimCpy(logFolderPathPhrase_).empty() ? getConfigDirPathPf() + Zstr("Logs") : logFolderPathPhrase_); //noexcept

        try
        {
            tryReportingError([&] //errors logged here do not impact final status calculation above! => not a problem!
            {
                std::unique_ptr<AFS::OutputStream> logFileStream = prepareNewLogfile(logFolderPath, jobName_, timeStamp_, status, *this); //throw FileError; return value always bound!

                streamToLogFile(summary, errorLog_, *logFileStream); //throw FileError, (X)
                logFileStream->finalize();                           //throw FileError, (X)
            }, *this); //throw X! by ProcessCallback!
        }
        catch (...) {}

        if (logfilesCountLimit_ > 0)
        {
            try { reportStatus(_("Cleaning up old log files...")); }
            catch (...) {}

            try
            {
                tryReportingError([&]
                {
                    limitLogfileCount(logFolderPath, jobName_, logfilesCountLimit_, requestUiRefreshNoThrow); //throw FileError
                }, *this); //throw X
            }
            catch (...) {}
        }
    }
    //----------------- write results into LastSyncs.log------------------------
    try
    {
        saveToLastSyncsLog(summary, errorLog_, lastSyncsLogFileSizeMax_, OnUpdateLogfileStatusNoThrow(*this, utfTo<std::wstring>(getLastSyncsLogfilePath()))); //throw FileError
    }
    catch (FileError&) { assert(false); }

    if (progressDlg_)
    {
        if (showFinalResults_) //warning: wxWindow::Show() is called within processHasFinished()!
        {
            //notify about (logical) application main window => program won't quit, but stay on this dialog
            //setMainWindow(progressDlg_->getAsWindow()); -> not required anymore since we block waiting until dialog is closed below

            //notify to progressDlg_ that current process has ended
            if (abortIsRequested())
                progressDlg_->processHasFinished(SyncProgressDialog::RESULT_ABORTED, errorLog_); //enable okay and close events
            else if (totalErrors > 0)
                progressDlg_->processHasFinished(SyncProgressDialog::RESULT_FINISHED_WITH_ERROR, errorLog_);
            else if (totalWarnings > 0)
                progressDlg_->processHasFinished(SyncProgressDialog::RESULT_FINISHED_WITH_WARNINGS, errorLog_);
            else
                progressDlg_->processHasFinished(SyncProgressDialog::RESULT_FINISHED_WITH_SUCCESS, errorLog_);
        }
        else
            progressDlg_->closeWindowDirectly(); //progressDlg_ is main window => program will quit directly

        //wait until progress dialog notified shutdown via onProgressDialogTerminate()
        //-> required since it has our "this" pointer captured in lambda "notifyWindowTerminate"!
        //-> nicely manages dialog lifetime
        while (progressDlg_)
        {
            wxTheApp->Yield(); //*first* refresh GUI (removing flicker) before sleeping!
            std::this_thread::sleep_for(std::chrono::milliseconds(UI_UPDATE_INTERVAL_MS));
        }
    }
}


void BatchStatusHandler::initNewPhase(int itemsTotal, int64_t bytesTotal, ProcessCallback::Phase phaseID)
{
    StatusHandler::initNewPhase(itemsTotal, bytesTotal, phaseID);
    if (progressDlg_)
        progressDlg_->initNewPhase(); //call after "StatusHandler::initNewPhase"

    forceUiRefresh(); //throw ?; OS X needs a full yield to update GUI and get rid of "dummy" texts
}


void BatchStatusHandler::updateProcessedData(int itemsDelta, int64_t bytesDelta)
{
    StatusHandler::updateProcessedData(itemsDelta, bytesDelta);

    if (progressDlg_)
        progressDlg_->notifyProgressChange(); //noexcept
    //note: this method should NOT throw in order to properly allow undoing setting of statistics!
}


void BatchStatusHandler::reportInfo(const std::wstring& text)
{
    StatusHandler::reportInfo(text);
    errorLog_.logMsg(text, TYPE_INFO);
}


void BatchStatusHandler::reportWarning(const std::wstring& warningMessage, bool& warningActive)
{
    errorLog_.logMsg(warningMessage, TYPE_WARNING);

    if (!warningActive)
        return;

    switch (handleError_)
    {
        case xmlAccess::ON_ERROR_POPUP:
        {
            if (!progressDlg_) abortProcessNow();
            PauseTimers dummy(*progressDlg_);
            forceUiRefresh();

            bool dontWarnAgain = false;
            switch (showConfirmationDialog3(progressDlg_->getWindowIfVisible(), DialogInfoType::WARNING, PopupDialogCfg3().
                                            setDetailInstructions(warningMessage + L"\n\n" + _("You can switch to FreeFileSync's main window to resolve this issue.")).
                                            setCheckBox(dontWarnAgain, _("&Don't show this warning again"), ConfirmationButton3::DONT_DO_IT),
                                            _("&Ignore"), _("&Switch")))
            {
                case ConfirmationButton3::DO_IT: //ignore
                    warningActive = !dontWarnAgain;
                    break;

                case ConfirmationButton3::DONT_DO_IT: //switch
                    errorLog_.logMsg(_("Switching to FreeFileSync's main window"), TYPE_INFO);
                    switchToGuiRequested_ = true; //treat as a special kind of cancel
                    requestAbortion();            //
                    throw BatchRequestSwitchToMainDialog();

                case ConfirmationButton3::CANCEL:
                    abortProcessNow();
                    break;
            }
        }
        break; //keep it! last switch might not find match

        case xmlAccess::ON_ERROR_STOP:
            abortProcessNow();
            break;

        case xmlAccess::ON_ERROR_IGNORE:
            break;
    }
}


ProcessCallback::Response BatchStatusHandler::reportError(const std::wstring& errorMessage, size_t retryNumber)
{
    //auto-retry
    if (retryNumber < automaticRetryCount_)
    {
        errorLog_.logMsg(errorMessage + L"\n-> " +
                         _P("Automatic retry in 1 second...", "Automatic retry in %x seconds...", automaticRetryDelay_), TYPE_INFO);
        //delay
        const int iterations = static_cast<int>(1000 * automaticRetryDelay_ / UI_UPDATE_INTERVAL_MS); //always round down: don't allow for negative remaining time below
        for (int i = 0; i < iterations; ++i)
        {
            reportStatus(_("Error") + L": " + _P("Automatic retry in 1 second...", "Automatic retry in %x seconds...",
                                                 (1000 * automaticRetryDelay_ - i * UI_UPDATE_INTERVAL_MS + 999) / 1000)); //integer round up
            std::this_thread::sleep_for(std::chrono::milliseconds(UI_UPDATE_INTERVAL_MS));
        }
        return ProcessCallback::RETRY;
    }


    //always, except for "retry":
    auto guardWriteLog = zen::makeGuard<ScopeGuardRunMode::ON_EXIT>([&] { errorLog_.logMsg(errorMessage, TYPE_ERROR); });

    switch (handleError_)
    {
        case xmlAccess::ON_ERROR_POPUP:
        {
            if (!progressDlg_) abortProcessNow();
            PauseTimers dummy(*progressDlg_);
            forceUiRefresh();

            bool ignoreNextErrors = false;
            switch (showConfirmationDialog3(progressDlg_->getWindowIfVisible(), DialogInfoType::ERROR2, PopupDialogCfg3().
                                            setDetailInstructions(errorMessage).
                                            setCheckBox(ignoreNextErrors, _("&Ignore subsequent errors"), ConfirmationButton3::DONT_DO_IT),
                                            _("&Ignore"), _("&Retry")))
            {
                case ConfirmationButton3::DO_IT: //ignore
                    if (ignoreNextErrors) //falsify only
                        handleError_ = xmlAccess::ON_ERROR_IGNORE;
                    return ProcessCallback::IGNORE_ERROR;

                case ConfirmationButton3::DONT_DO_IT: //retry
                    guardWriteLog.dismiss();
                    errorLog_.logMsg(errorMessage + L"\n-> " + _("Retrying operation..."), TYPE_INFO);
                    return ProcessCallback::RETRY;

                case ConfirmationButton3::CANCEL:
                    abortProcessNow();
                    break;
            }
        }
        break; //used if last switch didn't find a match

        case xmlAccess::ON_ERROR_STOP:
            abortProcessNow();
            break;

        case xmlAccess::ON_ERROR_IGNORE:
            return ProcessCallback::IGNORE_ERROR;
    }

    assert(false);
    return ProcessCallback::IGNORE_ERROR; //dummy value
}


void BatchStatusHandler::reportFatalError(const std::wstring& errorMessage)
{
    errorLog_.logMsg(errorMessage, TYPE_FATAL_ERROR);

    switch (handleError_)
    {
        case xmlAccess::ON_ERROR_POPUP:
        {
            if (!progressDlg_) abortProcessNow();
            PauseTimers dummy(*progressDlg_);
            forceUiRefresh();

            bool ignoreNextErrors = false;
            switch (showConfirmationDialog(progressDlg_->getWindowIfVisible(), DialogInfoType::ERROR2,
                                           PopupDialogCfg().setTitle(_("Serious Error")).
                                           setDetailInstructions(errorMessage).
                                           setCheckBox(ignoreNextErrors, _("&Ignore subsequent errors")),
                                           _("&Ignore")))
            {
                case ConfirmationButton::DO_IT:
                    if (ignoreNextErrors) //falsify only
                        handleError_ = xmlAccess::ON_ERROR_IGNORE;
                    break;
                case ConfirmationButton::CANCEL:
                    abortProcessNow();
                    break;
            }
        }
        break;

        case xmlAccess::ON_ERROR_STOP:
            abortProcessNow();
            break;

        case xmlAccess::ON_ERROR_IGNORE:
            break;
    }
}


void BatchStatusHandler::forceUiRefresh()
{
    if (progressDlg_)
        progressDlg_->updateGui();
}


void BatchStatusHandler::abortProcessNow()
{
    requestAbortion(); //just make sure...
    throw BatchAbortProcess(); //abort can be triggered by progressDlg_
}


void BatchStatusHandler::onProgressDialogTerminate()
{
    //it's responsibility of "progressDlg_" to call requestAbortion() when closing dialog
    progressDlg_ = nullptr;
}
