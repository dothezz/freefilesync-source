// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) Zenju (zenju AT gmx DOT de) - All Rights Reserved        *
// **************************************************************************

#include "batch_status_handler.h"
#include <zen/file_handling.h>
#include <zen/file_traverser.h>
#include <zen/shell_execute.h>
#include <wx+/popup_dlg.h>
#include <wx/app.h>
#include "on_completion_box.h"
#include "../lib/ffs_paths.h"
#include "../lib/resolve_path.h"
#include "../lib/status_handler_impl.h"
#include "../lib/generate_logfile.h"

using namespace zen;


namespace
{
//"Backup FreeFileSync 2013-09-15 015052.log" ->
//"Backup FreeFileSync 2013-09-15 015052 (Error).log"
Zstring addStatusToLogfilename(const Zstring& logfilepath, const std::wstring& status)
{
    //attention: do not interfere with naming convention required by limitLogfileCount()!
    size_t pos = logfilepath.rfind(Zstr('.'));
    if (pos != Zstring::npos)
        return Zstring(logfilepath.begin(), logfilepath.begin() + pos) +
               utfCvrtTo<Zstring>(L" (" + status + L")") +
               Zstring(logfilepath.begin() + pos, logfilepath.end());
    assert(false);
    return logfilepath;
}


class FindLogfiles : public TraverseCallback
{
public:
    FindLogfiles(const Zstring& prefix, std::vector<Zstring>& logfiles) : prefix_(prefix), logfiles_(logfiles) {}

private:
    virtual void onFile(const Zchar* shortName, const Zstring& filepath, const FileInfo& details)
    {
        const Zstring fileName(shortName);
        if (startsWith(fileName, prefix_) && endsWith(fileName, Zstr(".log")))
            logfiles_.push_back(filepath);
    }

    virtual TraverseCallback* onDir(const Zchar* shortName, const Zstring& dirpath) { return nullptr; } //DON'T traverse into subdirs
    virtual HandleLink  onSymlink(const Zchar* shortName, const Zstring& linkpath, const SymlinkInfo& details) { return LINK_SKIP; }
    virtual HandleError reportDirError (const std::wstring& msg, size_t retryNumber)                         { assert(false); return ON_ERROR_IGNORE; } //errors are not really critical in this context
    virtual HandleError reportItemError(const std::wstring& msg, size_t retryNumber, const Zchar* shortName) { assert(false); return ON_ERROR_IGNORE; } //

    const Zstring prefix_;
    std::vector<Zstring>& logfiles_;
};


void limitLogfileCount(const Zstring& logdir, const std::wstring& jobname, size_t maxCount) //throw()
{
    std::vector<Zstring> logFiles;
    FindLogfiles traverseCallback(utfCvrtTo<Zstring>(jobname), logFiles); //throw()!
    traverseFolder(logdir, traverseCallback);

    if (logFiles.size() <= maxCount)
        return;

    //delete oldest logfiles: take advantage of logfile naming convention to find them
    std::nth_element(logFiles.begin(), logFiles.end() - maxCount, logFiles.end(), LessFilename());

    std::for_each(logFiles.begin(), logFiles.end() - maxCount,
    [](const Zstring& filepath) { try { removeFile(filepath); } catch (FileError&) {} });
}


std::unique_ptr<FileOutput> prepareNewLogfile(const Zstring& logfileDirectory, //throw FileError
                                              const std::wstring& jobName,
                                              const TimeComp& timeStamp) //return value always bound!
{
    Zstring logfileDir = logfileDirectory.empty() ?
                         getConfigDir() + Zstr("Logs") :
                         getFormattedDirectoryPath(logfileDirectory);

    //create logfile directory if required
    makeDirectory(logfileDir); //throw FileError

    //assemble logfile name
    const Zstring body = appendSeparator(logfileDir) + utfCvrtTo<Zstring>(jobName) + Zstr(" ") + formatTime<Zstring>(Zstr("%Y-%m-%d %H%M%S"), timeStamp);

    //ensure uniqueness
    for (int i = 0;; ++i)
        try
        {
            const Zstring& filepath = i == 0 ?
                                      body + Zstr(".log") :
                                      body + Zstr('_') + numberTo<Zstring>(i) + Zstr(".log");

            return make_unique<FileOutput>(filepath, FileOutput::ACC_CREATE_NEW); //throw FileError, ErrorTargetExisting
            //*no* file system race-condition!
        }
        catch (const ErrorTargetExisting&) {}
}
}

//##############################################################################################################################

BatchStatusHandler::BatchStatusHandler(bool showProgress,
                                       const std::wstring& jobName,
                                       const TimeComp& timeStamp,
                                       const Zstring& logfileDirectory, //may be empty
                                       int logfilesCountLimit,
                                       size_t lastSyncsLogFileSizeMax,
                                       const xmlAccess::OnError handleError,
                                       size_t automaticRetryCount,
                                       size_t automaticRetryDelay,
                                       const SwitchToGui& switchBatchToGui, //functionality to change from batch mode to GUI mode
                                       FfsReturnCode& returnCode,
                                       const Zstring& onCompletion,
                                       std::vector<Zstring>& onCompletionHistory) :
    switchBatchToGui_(switchBatchToGui),
    showFinalResults(showProgress), //=> exit immediately or wait when finished
    switchToGuiRequested(false),
    logfilesCountLimit_(logfilesCountLimit),
    lastSyncsLogFileSizeMax_(lastSyncsLogFileSizeMax),
    handleError_(handleError),
    returnCode_(returnCode),
    automaticRetryCount_(automaticRetryCount),
    automaticRetryDelay_(automaticRetryDelay),
    progressDlg(createProgressDialog(*this, [this] { this->onProgressDialogTerminate(); }, *this, nullptr, showProgress, jobName, onCompletion, onCompletionHistory)),
            jobName_(jobName),
            startTime_(wxGetUTCTimeMillis().GetValue())
{
    if (logfilesCountLimit != 0)
    {
        zen::Opt<std::wstring> errMsg = tryReportingError([&] { logFile = prepareNewLogfile(logfileDirectory, jobName, timeStamp); }, //throw FileError; return value always bound!
                                                          *this);
        if (errMsg)
        {
            raiseReturnCode(returnCode_, FFS_RC_ABORTED);
            throw BatchAbortProcess();
        }
    }

    //if (logFile)
    //	::wxSetEnv(L"logfile", utfCvrtTo<wxString>(logFile->getFilename()));
}


BatchStatusHandler::~BatchStatusHandler()
{
    //------------ "on completion" command conceptually is part of the sync, not cleanup --------------------------------------

    //decide whether to stay on status screen or exit immediately...
    if (switchToGuiRequested) //-> avoid recursive yield() calls, thous switch not before ending batch mode
    {
        try
        {
            switchBatchToGui_.execute(); //open FreeFileSync GUI
        }
        catch (...) {}
        showFinalResults = false;
    }
    else if (progressDlg)
    {
        if (progressDlg->getWindowIfVisible())
            showFinalResults = true;

        //execute "on completion" command (even in case of ignored errors)
        if (!abortIsRequested()) //if aborted (manually), we don't execute the command
        {
            const Zstring finalCommand = progressDlg->getExecWhenFinishedCommand(); //final value (after possible user modification)
            if (!finalCommand.empty())
            {
                if (isCloseProgressDlgCommand(finalCommand))
                    showFinalResults = false; //take precedence over current visibility status
                else
                    try
                    {
                        //use EXEC_TYPE_ASYNC until there is reason no to: https://sourceforge.net/p/freefilesync/discussion/help/thread/828dca52
                        tryReportingError([&] { shellExecute2(expandMacros(finalCommand), EXEC_TYPE_ASYNC); }, //throw FileError, throw X?
                                          *this);
                    }
                    catch (...) {}
            }
        }
    }
    //------------ end of sync: begin of cleanup --------------------------------------

    const int totalErrors   = errorLog.getItemCount(TYPE_ERROR | TYPE_FATAL_ERROR); //evaluate before finalizing log
    const int totalWarnings = errorLog.getItemCount(TYPE_WARNING);

    //finalize error log
    std::wstring finalStatus;
    if (abortIsRequested())
    {
        raiseReturnCode(returnCode_, FFS_RC_ABORTED);
        finalStatus = _("Synchronization stopped");
        errorLog.logMsg(finalStatus, TYPE_ERROR);
    }
    else if (totalErrors > 0)
    {
        raiseReturnCode(returnCode_, FFS_RC_FINISHED_WITH_ERRORS);
        finalStatus = _("Synchronization completed with errors");
        errorLog.logMsg(finalStatus, TYPE_ERROR);
    }
    else if (totalWarnings > 0)
    {
        raiseReturnCode(returnCode_, FFS_RC_FINISHED_WITH_WARNINGS);
        finalStatus = _("Synchronization completed with warnings");
        errorLog.logMsg(finalStatus, TYPE_WARNING);
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
        jobName_,
        finalStatus,
        getObjectsCurrent(PHASE_SYNCHRONIZING), getDataCurrent(PHASE_SYNCHRONIZING),
        getObjectsTotal  (PHASE_SYNCHRONIZING), getDataTotal  (PHASE_SYNCHRONIZING),
        (wxGetUTCTimeMillis().GetValue() - startTime_) / 1000
    };

    //print the results list: logfile
    if (logFile.get())
    {
        try
        {
            //saving log file below may take a *long* time, so report (without logging)
            reportStatus(replaceCpy(_("Saving log file %x..."), L"%x", fmtFileName(logFile->getFilename()))); //throw?
            forceUiRefresh(); //
        }
        catch (...) {}

        if (logfilesCountLimit_ > 0)
            limitLogfileCount(beforeLast(logFile->getFilename(), FILE_NAME_SEPARATOR), jobName_, logfilesCountLimit_); //throw()

        try
        {
            saveLogToFile(summary, errorLog, *logFile); //throw FileError

            //additionally notify errors by showing in log file name
            const Zstring oldLogfilepath = logFile->getFilename();
            logFile.reset();

            if (abortIsRequested())
                renameFile(oldLogfilepath, addStatusToLogfilename(oldLogfilepath, _("Stopped"))); //throw FileError
            else if (totalErrors > 0)
                renameFile(oldLogfilepath, addStatusToLogfilename(oldLogfilepath, _("Error"))); //throw FileError
            //status "warning" is not important enough to show up in log file name
        }
        catch (FileError&) {}
    }
    try
    {
        saveToLastSyncsLog(summary, errorLog, lastSyncsLogFileSizeMax_); //throw FileError
    }
    catch (FileError&) {}

    if (progressDlg)
    {
        if (showFinalResults) //warning: wxWindow::Show() is called within processHasFinished()!
        {
            //notify about (logical) application main window => program won't quit, but stay on this dialog
            //setMainWindow(progressDlg->getAsWindow()); -> not required anymore since we block waiting until dialog is closed below

            //notify to progressDlg that current process has ended
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
            progressDlg->closeWindowDirectly(); //progressDlg is main window => program will quit directly

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


void BatchStatusHandler::initNewPhase(int objectsTotal, std::int64_t dataTotal, ProcessCallback::Phase phaseID)
{
    StatusHandler::initNewPhase(objectsTotal, dataTotal, phaseID);
    if (progressDlg)
        progressDlg->initNewPhase(); //call after "StatusHandler::initNewPhase"

    forceUiRefresh(); //throw ?; OS X needs a full yield to update GUI and get rid of "dummy" texts
}


void BatchStatusHandler::updateProcessedData(int objectsDelta, std::int64_t dataDelta)
{
    StatusHandler::updateProcessedData(objectsDelta, dataDelta);

    if (progressDlg)
        progressDlg->notifyProgressChange(); //noexcept
    //note: this method should NOT throw in order to properly allow undoing setting of statistics!
}


void BatchStatusHandler::reportInfo(const std::wstring& text)
{
    StatusHandler::reportInfo(text);
    errorLog.logMsg(text, TYPE_INFO);
}


void BatchStatusHandler::reportWarning(const std::wstring& warningMessage, bool& warningActive)
{
    errorLog.logMsg(warningMessage, TYPE_WARNING);

    if (!warningActive)
        return;

    switch (handleError_)
    {
        case xmlAccess::ON_ERROR_POPUP:
        {
            if (!progressDlg) abortProcessNow();
            PauseTimers dummy(*progressDlg);
            forceUiRefresh();

            bool dontWarnAgain = false;
            switch (showConfirmationDialog3(progressDlg->getWindowIfVisible(), DialogInfoType::WARNING, PopupDialogCfg3().
                                            setDetailInstructions(warningMessage + L"\n\n" + _("You can switch to FreeFileSync's main window to resolve this issue.")).
                                            setCheckBox(dontWarnAgain, _("&Don't show this warning again"), ConfirmationButton3::DONT_DO_IT),
                                            _("&Ignore"), _("&Switch")))
            {
                case ConfirmationButton3::DO_IT: //ignore
                    warningActive = !dontWarnAgain;
                    break;

                case ConfirmationButton3::DONT_DO_IT: //switch
                    errorLog.logMsg(_("Switching to FreeFileSync's main window"), TYPE_INFO);
                    switchToGuiRequested = true;
                    abortProcessNow();
                    break;

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
        errorLog.logMsg(errorMessage + L"\n=> " +
                        _P("Automatic retry in 1 second...", "Automatic retry in %x seconds...", automaticRetryDelay_), TYPE_INFO);
        //delay
        const int iterations = static_cast<int>(1000 * automaticRetryDelay_ / UI_UPDATE_INTERVAL); //always round down: don't allow for negative remaining time below
        for (int i = 0; i < iterations; ++i)
        {
            reportStatus(_("Error") + L": " + _P("Automatic retry in 1 second...", "Automatic retry in %x seconds...",
                                                 (1000 * automaticRetryDelay_ - i * UI_UPDATE_INTERVAL + 999) / 1000)); //integer round up
            boost::this_thread::sleep(boost::posix_time::milliseconds(UI_UPDATE_INTERVAL));
        }
        return ProcessCallback::RETRY;
    }


    //always, except for "retry":
    zen::ScopeGuard guardWriteLog = zen::makeGuard([&] { errorLog.logMsg(errorMessage, TYPE_ERROR); });

    switch (handleError_)
    {
        case xmlAccess::ON_ERROR_POPUP:
        {
            if (!progressDlg) abortProcessNow();
            PauseTimers dummy(*progressDlg);
            forceUiRefresh();

            bool ignoreNextErrors = false;
            switch (showConfirmationDialog3(progressDlg->getWindowIfVisible(), DialogInfoType::ERROR2, PopupDialogCfg3().
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
                    errorLog.logMsg(errorMessage + L"\n=> " + _("Retrying operation..."), TYPE_INFO);
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
    errorLog.logMsg(errorMessage, TYPE_FATAL_ERROR);

    switch (handleError_)
    {
        case xmlAccess::ON_ERROR_POPUP:
        {
            if (!progressDlg) abortProcessNow();
            PauseTimers dummy(*progressDlg);
            forceUiRefresh();

            bool ignoreNextErrors = false;
            switch (showConfirmationDialog(progressDlg->getWindowIfVisible(), DialogInfoType::ERROR2,
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
    if (progressDlg)
        progressDlg->updateGui();
}


void BatchStatusHandler::abortProcessNow()
{
    requestAbortion(); //just make sure...
    throw BatchAbortProcess();  //abort can be triggered by progressDlg
}


void BatchStatusHandler::onProgressDialogTerminate()
{
    //it's responsibility of "progressDlg" to call requestAbortion() when closing dialog
    progressDlg = nullptr;
}
