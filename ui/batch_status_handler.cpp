// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) Zenju (zenju AT gmx DOT de) - All Rights Reserved        *
// **************************************************************************

#include "batch_status_handler.h"
#include <zen/file_handling.h>
#include <zen/file_traverser.h>
#include <zen/format_unit.h>
//#include <wx+/app_main.h>
#include <wx+/shell_execute.h>
#include <wx/app.h>
#include "msg_popup.h"
#include "exec_finished_box.h"
#include "../lib/ffs_paths.h"
#include "../lib/resolve_path.h"
#include "../lib/status_handler_impl.h"
#include "../lib/generate_logfile.h"

using namespace zen;


namespace
{
class FindLogfiles : public TraverseCallback
{
public:
    FindLogfiles(const Zstring& prefix, std::vector<Zstring>& logfiles) : prefix_(prefix), logfiles_(logfiles) {}

private:
    virtual void onFile(const Zchar* shortName, const Zstring& fullName, const FileInfo& details)
    {
        const Zstring fileName(shortName);
        if (startsWith(fileName, prefix_) && endsWith(fileName, Zstr(".log")))
            logfiles_.push_back(fullName);
    }

    virtual TraverseCallback* onDir (const Zchar* shortName, const Zstring& fullName) { return nullptr; } //DON'T traverse into subdirs
    virtual HandleLink  onSymlink(const Zchar* shortName, const Zstring& fullName, const SymlinkInfo& details) { return LINK_SKIP; }
    virtual HandleError reportDirError (const std::wstring& msg)                         { assert(false); return ON_ERROR_IGNORE; } //errors are not really critical in this context
    virtual HandleError reportItemError(const std::wstring& msg, const Zchar* shortName) { assert(false); return ON_ERROR_IGNORE; } //

    const Zstring prefix_;
    std::vector<Zstring>& logfiles_;
};


void limitLogfileCount(const Zstring& logdir, const std::wstring& jobname, size_t maxCount) //throw()
{
    std::vector<Zstring> logFiles;
    FindLogfiles traverseCallback(utfCvrtTo<Zstring>(jobname), logFiles); //throw()!

    traverseFolder(logdir,
                   traverseCallback);

    if (logFiles.size() <= maxCount)
        return;

    //delete oldest logfiles: take advantage of logfile naming convention to find them
    std::nth_element(logFiles.begin(), logFiles.end() - maxCount, logFiles.end(), LessFilename());

    std::for_each(logFiles.begin(), logFiles.end() - maxCount,
    [](const Zstring& filename) { try { removeFile(filename); } catch (FileError&) {} });
}


std::unique_ptr<FileOutput> prepareNewLogfile(const Zstring& logfileDirectory, //throw FileError
                                              const std::wstring& jobName,
                                              const TimeComp& timeStamp) //return value always bound!
{
    //create logfile directory if required
    Zstring logfileDir = logfileDirectory.empty() ?
                         getConfigDir() + Zstr("Logs") :
                         getFormattedDirectoryName(logfileDirectory);

    makeDirectory(logfileDir); //throw FileError

    //assemble logfile name
    const Zstring body = appendSeparator(logfileDir) + utfCvrtTo<Zstring>(jobName) + Zstr(" ") + formatTime<Zstring>(Zstr("%Y-%m-%d %H%M%S"), timeStamp);

    //ensure uniqueness
    for (int i = 0;; ++i)
        try
        {
            const Zstring& filename = i == 0 ?
                                      body + Zstr(".log") :
                                      body + Zstr('_') + numberTo<Zstring>(i) + Zstr(".log");

            return make_unique<FileOutput>(filename, FileOutput::ACC_CREATE_NEW); //throw FileError, ErrorTargetExisting
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
                                       const SwitchToGui& switchBatchToGui, //functionality to change from batch mode to GUI mode
                                       FfsReturnCode& returnCode,
                                       const std::wstring& execWhenFinished,
                                       std::vector<std::wstring>& execFinishedHistory) :
    switchBatchToGui_(switchBatchToGui),
    showFinalResults(showProgress), //=> exit immediately or wait when finished
    switchToGuiRequested(false),
    lastSyncsLogFileSizeMax_(lastSyncsLogFileSizeMax),
    handleError_(handleError),
    returnCode_(returnCode),
    progressDlg(createProgressDialog(*this, [this] { this->onProgressDialogTerminate(); }, *this, nullptr, showProgress, jobName, execWhenFinished, execFinishedHistory)),
            jobName_(jobName)
{
    if (logfilesCountLimit != 0) //init log file: starts internal timer!
    {
        zen::Opt<std::wstring> errMsg = tryReportingError2([&] { logFile = prepareNewLogfile(logfileDirectory, jobName, timeStamp); }, //throw FileError; return value always bound!
                                                           *this);
        if (errMsg)
        {
            raiseReturnCode(returnCode_, FFS_RC_ABORTED);
            throw BatchAbortProcess();
        }

        if (logfilesCountLimit >= 0)
            limitLogfileCount(beforeLast(logFile->getFilename(), FILE_NAME_SEPARATOR), jobName_, logfilesCountLimit); //throw()
    }

    totalTime.Start(); //measure total time

    //if (logFile)
    //	::wxSetEnv(L"logfile", utfCvrtTo<wxString>(logFile->getFilename()));
}


BatchStatusHandler::~BatchStatusHandler()
{
    const int totalErrors   = errorLog.getItemCount(TYPE_ERROR | TYPE_FATAL_ERROR); //evaluate before finalizing log
    const int totalWarnings = errorLog.getItemCount(TYPE_WARNING);

    //finalize error log
    std::wstring finalStatus;
    if (abortIsRequested())
    {
        raiseReturnCode(returnCode_, FFS_RC_ABORTED);
        finalStatus = _("Synchronization aborted");
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
        totalTime.Time() / 1000
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
        try
        {
            saveLogToFile(summary, errorLog, *logFile); //throw FileError
        }
        catch (FileError&) {}
        logFile.reset(); //close file now: user may do something with it in "on completion"
    }
    try
    {
        saveToLastSyncsLog(summary, errorLog, lastSyncsLogFileSizeMax_); //throw FileError
    }
    catch (FileError&) {}

    if (progressDlg)
    {
        //decide whether to stay on status screen or exit immediately...
        if (switchToGuiRequested) //-> avoid recursive yield() calls, thous switch not before ending batch mode
        {
            try
            {
                switchBatchToGui_.execute(); //open FreeFileSync GUI
            }
            catch (...) {}
            progressDlg->closeWindowDirectly(); //progressDlg is not main window anymore
        }
        else
        {
            if (progressDlg->getWindowIfVisible())
                showFinalResults = true;

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
        }

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



void BatchStatusHandler::initNewPhase(int objectsTotal, Int64 dataTotal, ProcessCallback::Phase phaseID)
{
    StatusHandler::initNewPhase(objectsTotal, dataTotal, phaseID);
    if (progressDlg)
        progressDlg->initNewPhase(); //call after "StatusHandler::initNewPhase"
}


void BatchStatusHandler::updateProcessedData(int objectsDelta, Int64 dataDelta)
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
            if (!progressDlg) abortThisProcess();
            PauseTimers dummy(*progressDlg);
            forceUiRefresh();

            bool dontWarnAgain = false;
            switch (showWarningDlg(progressDlg->getWindowIfVisible(),
                                   ReturnWarningDlg::BUTTON_IGNORE | ReturnWarningDlg::BUTTON_SWITCH | ReturnWarningDlg::BUTTON_CANCEL,
                                   warningMessage + L"\n\n" + _("Press \"Switch\" to resolve issues in FreeFileSync main dialog."),
                                   dontWarnAgain))
            {
                case ReturnWarningDlg::BUTTON_CANCEL:
                    abortThisProcess();
                    break;

                case ReturnWarningDlg::BUTTON_SWITCH:
                    errorLog.logMsg(_("Switching to FreeFileSync main dialog"), TYPE_INFO);
                    switchToGuiRequested = true;
                    abortThisProcess();
                    break;

                case ReturnWarningDlg::BUTTON_IGNORE: //no unhandled error situation!
                    warningActive = !dontWarnAgain;
                    break;
            }
        }
        break; //keep it! last switch might not find match

        case xmlAccess::ON_ERROR_EXIT: //abort
            abortThisProcess();
            break;

        case xmlAccess::ON_ERROR_IGNORE:
            break;
    }
}


ProcessCallback::Response BatchStatusHandler::reportError(const std::wstring& errorMessage)
{
    //always, except for "retry":
    zen::ScopeGuard guardWriteLog = zen::makeGuard([&] { errorLog.logMsg(errorMessage, TYPE_ERROR); });

    switch (handleError_)
    {
        case xmlAccess::ON_ERROR_POPUP:
        {
            if (!progressDlg) abortThisProcess();
            PauseTimers dummy(*progressDlg);
            forceUiRefresh();

            bool ignoreNextErrors = false;
            switch (showErrorDlg(progressDlg->getWindowIfVisible(),
                                 ReturnErrorDlg::BUTTON_IGNORE |  ReturnErrorDlg::BUTTON_RETRY | ReturnErrorDlg::BUTTON_CANCEL,
                                 errorMessage, &ignoreNextErrors))
            {
                case ReturnErrorDlg::BUTTON_IGNORE:
                    if (ignoreNextErrors) //falsify only
                        handleError_ = xmlAccess::ON_ERROR_IGNORE;
                    return ProcessCallback::IGNORE_ERROR;

                case ReturnErrorDlg::BUTTON_RETRY:
                    guardWriteLog.dismiss();
                    errorLog.logMsg(_("Retrying operation after error:") + L" " + errorMessage, TYPE_INFO);
                    return ProcessCallback::RETRY;

                case ReturnErrorDlg::BUTTON_CANCEL:
                    abortThisProcess();
                    break;
            }
        }
        break; //used if last switch didn't find a match

        case xmlAccess::ON_ERROR_EXIT: //abort
            abortThisProcess();
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
                        handleError_ = xmlAccess::ON_ERROR_IGNORE;
                    break;

                case ReturnFatalErrorDlg::BUTTON_CANCEL:
                    abortThisProcess();
                    break;
            }
        }
        break;

        case xmlAccess::ON_ERROR_EXIT: //abort
            abortThisProcess();
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


void BatchStatusHandler::abortThisProcess()
{
    requestAbortion(); //just make sure...
    throw BatchAbortProcess();  //abort can be triggered by progressDlg
}


void BatchStatusHandler::onProgressDialogTerminate()
{
    //it's responsibility of "progressDlg" to call requestAbortion() when closing dialog
    progressDlg = nullptr;
}
