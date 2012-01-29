// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) ZenJu (zhnmju123 AT gmx DOT de) - All Rights Reserved    *
// **************************************************************************

#include "batch_status_handler.h"
#include "msg_popup.h"
#include <wx/ffile.h>
#include <wx/msgdlg.h>
#include "../lib/ffs_paths.h"
#include <zen/file_handling.h>
#include "../lib/resolve_path.h"
#include <wx+/string_conv.h>
#include <wx+/app_main.h>
#include <zen/file_traverser.h>
#include <zen/time.h>
#include "exec_finished_box.h"
#include <wx+/shell_execute.h>

using namespace zen;


namespace
{
class FindLogfiles : public zen::TraverseCallback
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
    onDir    (const Zchar* shortName, const Zstring& fullName) { return nullptr; } //DON'T traverse into subdirs
    virtual void        onSymlink(const Zchar* shortName, const Zstring& fullName, const SymlinkInfo& details) {}
    virtual HandleError onError  (const std::wstring& errorText) { return TRAV_ERROR_IGNORE; } //errors are not really critical in this context

private:
    const Zstring prefix_;
    std::vector<Zstring>& logfiles_;
};
}


class LogFile //throw FileError
{
public:
    LogFile(const Zstring& logfileDirectory, const wxString& jobName) :
        jobName_(jobName), //throw FileError
        logfileName(findUniqueLogname(logfileDirectory, jobName))
    {
        logFile.Open(toWx(logfileName), wxT("w"));
        if (!logFile.IsOpened())
            throw FileError(_("Unable to create log file!") + L"\"" + logfileName + L"\"");

        //write header
        wxString headerLine = wxString(L"FreeFileSync - ") + _("Batch execution") + L" - " + formatTime<wxString>(FORMAT_DATE);

        logFile.Write(headerLine + wxChar('\n'));
        logFile.Write(wxString().Pad(headerLine.Len(), wxChar('-')) + wxChar('\n') + wxChar('\n'));

        logItemStart = formatTime<wxString>(L"[%X] ") + _("Start");

        totalTime.Start(); //measure total time
    }

    void writeLog(const ErrorLogging& log, const wxString& finalStatus)
    {
        logFile.Write(finalStatus + L"\n\n"); //highlight result by placing at beginning of file

        logFile.Write(logItemStart + L"\n\n");

        //write actual logfile
        const std::vector<wxString>& messages = log.getFormattedMessages();
        for (std::vector<wxString>::const_iterator i = messages.begin(); i != messages.end(); ++i)
            logFile.Write(*i + L'\n');

        //write ending
        logFile.Write(L'\n');

        const long time = totalTime.Time(); //retrieve total time

        logFile.Write(wxString(L"[") + formatTime<wxString>(FORMAT_TIME) + L"] " + _("Stop") + L" (" + _("Total time:") + L" " + wxTimeSpan::Milliseconds(time).Format() + L")\n");
    }

    void limitLogfileCount(size_t maxCount) const
    {
        std::vector<Zstring> logFiles;
        FindLogfiles traverseCallback(toZ(jobName_), logFiles);

        traverseFolder(beforeLast(logfileName, FILE_NAME_SEPARATOR), //throw();
                       false, //don't follow symlinks
                       traverseCallback);

        if (logFiles.size() <= maxCount)
            return;

        //delete oldest logfiles
        std::sort(logFiles.begin(), logFiles.end()); //take advantage of logfile naming convention to sort by age

        std::for_each(logFiles.begin(), logFiles.end() - maxCount,
        [](const Zstring& filename) { try { zen::removeFile(filename); } catch (FileError&) {} });
    }

    //Zstring getLogfileName() const { return logfileName; }

private:
    static Zstring findUniqueLogname(const Zstring& logfileDirectory, const wxString& jobName)
    {
        //create logfile directory
        Zstring logfileDir = logfileDirectory.empty() ?
                             zen::getConfigDir() + Zstr("Logs") :
                             zen::getFormattedDirectoryName(logfileDirectory);

        if (!zen::dirExists(logfileDir))
            zen::createDirectory(logfileDir); //throw FileError; create recursively if necessary

        //assemble logfile name
        if (!endsWith(logfileDir, FILE_NAME_SEPARATOR))
            logfileDir += FILE_NAME_SEPARATOR;

        const Zstring logfileName = logfileDir + toZ(jobName) + Zstr(" ") + formatTime<Zstring>(Zstr("%Y-%m-%d %H%M%S"));

        //ensure uniqueness
        Zstring output = logfileName + Zstr(".log");

        for (int i = 1; zen::somethingExists(output); ++i)
            output = logfileName + Zstr('_') + zen::toString<Zstring>(i) + Zstr(".log");
        return output;
    }

    const wxString jobName_;
    const Zstring logfileName;
    wxFFile logFile;
    wxStopWatch totalTime;
    wxString logItemStart;
};


//##############################################################################################################################
BatchStatusHandler::BatchStatusHandler(bool showProgress,
                                       const wxString& jobName,
                                       const wxString& logfileDirectory,
                                       size_t logFileCountMax,
                                       const xmlAccess::OnError handleError,
                                       const SwitchToGui& switchBatchToGui, //functionality to change from batch mode to GUI mode
                                       int& returnVal,
                                       const std::wstring& execWhenFinished,
                                       std::vector<std::wstring>& execFinishedHistory) :
    switchBatchToGui_(switchBatchToGui),
    showFinalResults(showProgress), //=> exit immediately or wait when finished
    switchToGuiRequested(false),
    handleError_(handleError),
    currentProcess(StatusHandler::PROCESS_NONE),
    returnValue(returnVal),
    syncStatusFrame(*this, NULL, SyncStatus::SCANNING, showProgress, jobName, execWhenFinished, execFinishedHistory)
{
    if (logFileCountMax > 0)
    {
        try
        {
            logFile = std::make_shared<LogFile>(toZ(logfileDirectory), jobName); //throw FileError
            logFile->limitLogfileCount(logFileCountMax); //throw FileError
        }
        catch (zen::FileError& error)
        {
            if (handleError_ == xmlAccess::ON_ERROR_POPUP)
                wxMessageBox(error.toString(), _("Error"), wxOK | wxICON_ERROR);
            returnValue = -7;
            throw BatchAbortProcess();
        }

        //::wxSetEnv(L"logfile", logFile->getLogfileName());
    }
}


BatchStatusHandler::~BatchStatusHandler()
{
    const int totalErrors = errorLog.typeCount(TYPE_ERROR | TYPE_FATAL_ERROR); //evaluate before finalizing log

    //finalize error log
    wxString finalStatus;
    if (abortIsRequested())
    {
        returnValue = -4;
        finalStatus = _("Synchronization aborted!");
        errorLog.logMsg(finalStatus, TYPE_FATAL_ERROR);
    }
    else if (totalErrors > 0)
    {
        returnValue = -5;
        finalStatus = _("Synchronization completed with errors!");
        errorLog.logMsg(finalStatus, TYPE_WARNING);
    }
    else
    {
        finalStatus = _("Synchronization completed successfully!");
        errorLog.logMsg(finalStatus, TYPE_INFO);
    }

    //print the results list: logfile
    if (logFile.get())
    {
        logFile->writeLog(errorLog, finalStatus);
        logFile.reset(); //close file now: user may do something with it in "on completion"
    }

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
            zen::setMainWindow(syncStatusFrame.getAsWindow());

            //notify to syncStatusFrame that current process has ended
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
}


void BatchStatusHandler::initNewProcess(int objectsTotal, zen::Int64 dataTotal, StatusHandler::Process processID)
{
    currentProcess = processID;

    switch (currentProcess)
    {
        case StatusHandler::PROCESS_SCANNING:
            syncStatusFrame.initNewProcess(SyncStatus::SCANNING, 0, 0); //initialize some gui elements (remaining time, speed)
            break;
        case StatusHandler::PROCESS_COMPARING_CONTENT:
            syncStatusFrame.initNewProcess(SyncStatus::COMPARING_CONTENT, objectsTotal, dataTotal);
            break;
        case StatusHandler::PROCESS_SYNCHRONIZING:
            syncStatusFrame.initNewProcess(SyncStatus::SYNCHRONIZING, objectsTotal, dataTotal);
            break;
        case StatusHandler::PROCESS_NONE:
            assert(false);
            break;
    }
}


void BatchStatusHandler::updateProcessedData(int objectsDelta, Int64 dataDelta)
{
    switch (currentProcess)
    {
        case StatusHandler::PROCESS_SCANNING:
            syncStatusFrame.incScannedObjects_NoUpdate(objectsDelta); //throw ()
            break;
        case StatusHandler::PROCESS_COMPARING_CONTENT:
        case StatusHandler::PROCESS_SYNCHRONIZING:
            syncStatusFrame.incProcessedData_NoUpdate(objectsDelta, dataDelta);
            break;
        case StatusHandler::PROCESS_NONE:
            assert(false);
            break;
    }

    //note: this method should NOT throw in order to properly allow undoing setting of statistics!
}


void BatchStatusHandler::updateTotalData(int objectsDelta, Int64 dataDelta)
{
    assert(currentProcess != PROCESS_SCANNING);
    syncStatusFrame.incTotalData_NoUpdate(objectsDelta, dataDelta);
}


void BatchStatusHandler::reportStatus(const std::wstring& text)
{
    syncStatusFrame.setStatusText_NoUpdate(text);
    requestUiRefresh(); //throw AbortThisProcess
}


void BatchStatusHandler::reportInfo(const std::wstring& text)
{
    errorLog.logMsg(text, TYPE_INFO);

    syncStatusFrame.setStatusText_NoUpdate(text);
    requestUiRefresh(); //throw AbortThisProcess
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
            switch (showWarningDlg(ReturnWarningDlg::BUTTON_IGNORE | ReturnWarningDlg::BUTTON_SWITCH | ReturnWarningDlg::BUTTON_ABORT,
                                   warningMessage + wxT("\n\n") + _("Press \"Switch\" to open FreeFileSync GUI mode."),
                                   dontWarnAgain))
            {
                case ReturnWarningDlg::BUTTON_ABORT:
                    abortThisProcess();
                    break;

                case ReturnWarningDlg::BUTTON_SWITCH:
                    errorLog.logMsg(_("Switching to FreeFileSync GUI mode..."), TYPE_WARNING);
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

        case xmlAccess::ON_ERROR_IGNORE: //no unhandled error situation!
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
            switch (showErrorDlg(ReturnErrorDlg::BUTTON_IGNORE |  ReturnErrorDlg::BUTTON_RETRY | ReturnErrorDlg::BUTTON_ABORT,
                                 errorMessage,
                                 &ignoreNextErrors))
            {
                case ReturnErrorDlg::BUTTON_IGNORE:
                    if (ignoreNextErrors) //falsify only
                        handleError_ = xmlAccess::ON_ERROR_IGNORE;
                    errorLog.logMsg(errorMessage, TYPE_ERROR);
                    return ProcessCallback::IGNORE_ERROR;

                case ReturnErrorDlg::BUTTON_RETRY:
                    return ProcessCallback::RETRY;

                case ReturnErrorDlg::BUTTON_ABORT:
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
            switch (showErrorDlg(ReturnErrorDlg::BUTTON_IGNORE |  ReturnErrorDlg::BUTTON_ABORT,
                                 errorMessage,
                                 &ignoreNextErrors))
            {
                case ReturnErrorDlg::BUTTON_IGNORE:
                    if (ignoreNextErrors) //falsify only
                        handleError_ = xmlAccess::ON_ERROR_IGNORE;
                    break;

                case ReturnErrorDlg::BUTTON_ABORT:
                    abortThisProcess();
                    break;

                case ReturnErrorDlg::BUTTON_RETRY:
                    assert(false);
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
    syncStatusFrame.updateStatusDialogNow();
}


void BatchStatusHandler::abortThisProcess()
{
    requestAbortion();
    throw BatchAbortProcess();  //abort can be triggered by syncStatusFrame
}
