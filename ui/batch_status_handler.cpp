// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) ZenJu (zhnmju123 AT gmx DOT de) - All Rights Reserved    *
// **************************************************************************

#include "batch_status_handler.h"
#include <zen/file_handling.h>
#include <zen/file_traverser.h>
#include <wx+/app_main.h>
#include <wx+/format_unit.h>
#include <wx+/shell_execute.h>
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

    virtual void onFile(const Zchar* shortName, const Zstring& fullName, const FileInfo& details)
    {
        const Zstring fileName(shortName);
        if (startsWith(fileName, prefix_) && endsWith(fileName, Zstr(".log")))
            logfiles_.push_back(fullName);
    }

    virtual std::shared_ptr<TraverseCallback>
    onDir (const Zchar* shortName, const Zstring& fullName) { return nullptr; } //DON'T traverse into subdirs
    virtual HandleLink  onSymlink(const Zchar* shortName, const Zstring& fullName, const SymlinkInfo& details) { return LINK_SKIP; }
    virtual HandleError onError  (const std::wstring& errorText) { return ON_ERROR_IGNORE; } //errors are not really critical in this context

private:
    const Zstring prefix_;
    std::vector<Zstring>& logfiles_;
};


void limitLogfileCount(const Zstring& logdir, const std::wstring& jobname, size_t maxCount) //throw()
{
    std::vector<Zstring> logFiles;
    FindLogfiles traverseCallback(toZ(jobname), logFiles);

    traverseFolder(logdir, //throw();
                   traverseCallback);

    if (logFiles.size() <= maxCount)
        return;

    //delete oldest logfiles
    std::nth_element(logFiles.begin(), logFiles.end() - maxCount, logFiles.end()); //take advantage of logfile naming convention to find oldest files

    std::for_each(logFiles.begin(), logFiles.end() - maxCount,
    [](const Zstring& filename) { try { removeFile(filename); } catch (FileError&) {} });
}


std::unique_ptr<FileOutput> prepareNewLogfile(const Zstring& logfileDirectory, //throw FileError
                                              const std::wstring& jobName,
                                              const std::wstring& timestamp) //return value always bound!
{
    //create logfile directory if required
    Zstring logfileDir = logfileDirectory.empty() ?
                         getConfigDir() + Zstr("Logs") :
                         getFormattedDirectoryName(logfileDirectory);

    makeDirectory(logfileDir); //throw FileError

    //assemble logfile name
    const Zstring body = appendSeparator(logfileDir) + toZ(jobName) + Zstr(" ") + utfCvrtTo<Zstring>(timestamp);

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
                                       const std::wstring& timestamp,
                                       const wxString& logfileDirectory, //may be empty
                                       size_t logFileCountMax,
                                       const xmlAccess::OnError handleError,
                                       const SwitchToGui& switchBatchToGui, //functionality to change from batch mode to GUI mode
                                       FfsReturnCode& returnCode,
                                       const std::wstring& execWhenFinished,
                                       std::vector<std::wstring>& execFinishedHistory) :
    switchBatchToGui_(switchBatchToGui),
    showFinalResults(showProgress), //=> exit immediately or wait when finished
    switchToGuiRequested(false),
    handleError_(handleError),
    returnCode_(returnCode),
    syncStatusFrame(*this, *this, nullptr, showProgress, jobName, execWhenFinished, execFinishedHistory),
    jobName_(jobName)
{
    if (logFileCountMax > 0) //init log file: starts internal timer!
        if (!tryReportingError([&]
    {
        logFile = prepareNewLogfile(toZ(logfileDirectory), jobName, timestamp); //throw FileError; return value always bound!

            limitLogfileCount(beforeLast(logFile->getFilename(), FILE_NAME_SEPARATOR), jobName_, logFileCountMax); //throw()
        }, *this))
    {
        returnCode_ = FFS_RC_ABORTED;
        throw BatchAbortProcess();
    }

    totalTime.Start(); //measure total time

    //::wxSetEnv(L"logfile", logFile->getLogfileName());
}


BatchStatusHandler::~BatchStatusHandler()
{
    const int totalErrors = errorLog.getItemCount(TYPE_ERROR | TYPE_FATAL_ERROR); //evaluate before finalizing log

    //finalize error log
    std::wstring finalStatus;
    if (abortIsRequested())
    {
        returnCode_ = FFS_RC_ABORTED;
        finalStatus = _("Synchronization aborted!");
        errorLog.logMsg(finalStatus, TYPE_ERROR);
    }
    else if (totalErrors > 0)
    {
        returnCode_ = FFS_RC_FINISHED_WITH_ERRORS;
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
    //print the results list: logfile
    if (logFile.get())
    {
        try
        {
            if (!logStream.empty())
                logFile->write(&*logStream.begin(), logStream.size()); //throw FileError
        }
        catch (FileError&) {}

        logFile.reset(); //close file now: user may do something with it in "on completion"
    }
    try
    {
        saveToLastSyncsLog(logStream); //throw FileError
    }
    catch (FileError&) {}

    //decide whether to stay on status screen or exit immediately...
    if (switchToGuiRequested) //-> avoid recursive yield() calls, thous switch not before ending batch mode
    {
        try
        {
            switchBatchToGui_.execute(); //open FreeFileSync GUI
        }
        catch (...) {}
        syncStatusFrame.closeWindowDirectly(); //syncStatusFrame is main window => program will quit directly
    }
    else
    {
        if (syncStatusFrame.getAsWindow()->IsShown())
            showFinalResults = true;

        //execute "on completion" command (even in case of ignored errors)
        if (!abortIsRequested()) //if aborted (manually), we don't execute the command
        {
            const std::wstring finalCommand = syncStatusFrame.getExecWhenFinishedCommand(); //final value (after possible user modification)
            if (isCloseProgressDlgCommand(finalCommand))
                showFinalResults = false; //take precedence over current visibility status
            else if (!finalCommand.empty())
                shellExecute(finalCommand);
        }

        if (showFinalResults) //warning: wxWindow::Show() is called within processHasFinished()!
        {
            //notify about (logical) application main window => program won't quit, but stay on this dialog
            setMainWindow(syncStatusFrame.getAsWindow());

            //notify to syncStatusFrame that current process has ended
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
}


void BatchStatusHandler::initNewPhase(int objectsTotal, Int64 dataTotal, ProcessCallback::Phase phaseID)
{
    StatusHandler::initNewPhase(objectsTotal, dataTotal, phaseID);
    syncStatusFrame.initNewPhase(); //call after "StatusHandler::initNewPhase"
}


void BatchStatusHandler::updateProcessedData(int objectsDelta, Int64 dataDelta)
{
    StatusHandler::updateProcessedData(objectsDelta, dataDelta);

    switch (currentPhase())
    {
        case ProcessCallback::PHASE_NONE:
            assert(false);
        case ProcessCallback::PHASE_SCANNING:
            break;
        case ProcessCallback::PHASE_COMPARING_CONTENT:
        case ProcessCallback::PHASE_SYNCHRONIZING:
            syncStatusFrame.reportCurrentBytes(getDataCurrent(currentPhase()));
            break;
    }
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
            PauseTimers dummy(syncStatusFrame);
            forceUiRefresh();

            bool dontWarnAgain = false;
            switch (showWarningDlg(syncStatusFrame.getAsWindow(),
                                   ReturnWarningDlg::BUTTON_IGNORE | ReturnWarningDlg::BUTTON_SWITCH | ReturnWarningDlg::BUTTON_CANCEL,
                                   warningMessage + L"\n\n" + _("Press \"Switch\" to resolve issues in FreeFileSync main dialog."),
                                   dontWarnAgain))
            {
                case ReturnWarningDlg::BUTTON_CANCEL:
                    abortThisProcess();
                    break;

                case ReturnWarningDlg::BUTTON_SWITCH:
                    errorLog.logMsg(_("Switching to FreeFileSync main dialog..."), TYPE_INFO);
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
    switch (handleError_)
    {
        case xmlAccess::ON_ERROR_POPUP:
        {
            PauseTimers dummy(syncStatusFrame);
            forceUiRefresh();

            bool ignoreNextErrors = false;
            switch (showErrorDlg(syncStatusFrame.getAsWindow(),
                                 ReturnErrorDlg::BUTTON_IGNORE |  ReturnErrorDlg::BUTTON_RETRY | ReturnErrorDlg::BUTTON_CANCEL,
                                 errorMessage, &ignoreNextErrors))
            {
                case ReturnErrorDlg::BUTTON_IGNORE:
                    if (ignoreNextErrors) //falsify only
                        handleError_ = xmlAccess::ON_ERROR_IGNORE;
                    errorLog.logMsg(errorMessage, TYPE_ERROR);
                    return ProcessCallback::IGNORE_ERROR;

                case ReturnErrorDlg::BUTTON_RETRY:
                    return ProcessCallback::RETRY;

                case ReturnErrorDlg::BUTTON_CANCEL:
                    errorLog.logMsg(errorMessage, TYPE_ERROR);
                    abortThisProcess();
            }
        }
        break; //used if last switch didn't find a match

        case xmlAccess::ON_ERROR_EXIT: //abort
            errorLog.logMsg(errorMessage, TYPE_ERROR);
            abortThisProcess();
            break;

        case xmlAccess::ON_ERROR_IGNORE:
            errorLog.logMsg(errorMessage, TYPE_ERROR);
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
            PauseTimers dummy(syncStatusFrame);
            forceUiRefresh();

            bool ignoreNextErrors = false;
            switch (showFatalErrorDlg(syncStatusFrame.getAsWindow(),
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
    syncStatusFrame.updateProgress();
}


void BatchStatusHandler::abortThisProcess()
{
    requestAbortion(); //just make sure...
    throw BatchAbortProcess();  //abort can be triggered by syncStatusFrame
}
