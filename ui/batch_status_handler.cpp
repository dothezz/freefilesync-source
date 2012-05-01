// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) ZenJu (zhnmju123 AT gmx DOT de) - All Rights Reserved    *
// **************************************************************************

#include "batch_status_handler.h"
#include <wx/ffile.h>
#include <zen/file_handling.h>
#include <zen/file_traverser.h>
#include <wx+/string_conv.h>
#include <wx+/app_main.h>
#include <wx+/format_unit.h>
#include <wx+/shell_execute.h>
#include "msg_popup.h"
#include "exec_finished_box.h"
#include "../lib/ffs_paths.h"
#include "../lib/resolve_path.h"
#include "../lib/status_handler_impl.h"

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
        logFile.Open(toWx(logfileName), L"w");
        if (!logFile.IsOpened())
            throw FileError(replaceCpy(_("Cannot write file %x."), L"%x", fmtFileName(logfileName)));

        //write header
        const wxString& headerLine = wxString(L"FreeFileSync - ") + _("Batch execution") + L" - " + formatTime<wxString>(FORMAT_DATE);
        logFile.Write(headerLine + L'\n');
        logFile.Write(wxString().Pad(headerLine.Len(), L'=') + L'\n');

        //logItemStart = formatTime<wxString>(L"[%X] ") + _("Start");

        totalTime.Start(); //measure total time
    }

    void writeLog(const ErrorLog& log, const std::wstring& finalStatus,
                  int itemsSynced, Int64 dataSynced,
                  int itemsTotal,  Int64 dataTotal)
    {
        //assemble results box
        std::vector<wxString> results;
        results.push_back(finalStatus);
        results.push_back(L"");
        if (itemsTotal != 0 || dataTotal != 0) //=: sync phase was reached and there were actual items to sync
        {
            results.push_back(L"    " + _("Items processed:") + L" " + toStringSep(itemsSynced) + L" (" + filesizeToShortString(dataSynced) + L")");

            if (itemsSynced != itemsTotal ||
                dataSynced  != dataTotal)
                results.push_back(L"    " + _("Items remaining:") + L" " + toStringSep(itemsTotal - itemsSynced) + L" (" + filesizeToShortString(dataTotal - dataSynced) + L")");
        }
        results.push_back(L"    " + _("Total time:") + L" " + wxTimeSpan::Milliseconds(totalTime.Time()).Format());

        //write results box
        size_t sepLineLen = 0;
        std::for_each(results.begin(), results.end(), [&](const wxString& str) { sepLineLen = std::max(sepLineLen, str.size()); });

        logFile.Write(wxString().Pad(sepLineLen, L'_') + L"\n\n");
        std::for_each(results.begin(), results.end(), [&](const wxString& str) { logFile.Write(str + L'\n'); });
        logFile.Write(wxString().Pad(sepLineLen, L'_') + L"\n\n");

        //logFile.Write(logItemStart + L"\n\n");

        //write log items
        const auto& entries = log.getEntries();
        for (auto iter = entries.begin(); iter != entries.end(); ++iter)
        {
            const std::string& msg = utf8CvrtTo<std::string>(formatMessage(*iter));
            logFile.Write(msg.c_str(), msg.size()); //better do UTF8 conversion ourselves rather than to rely on wxWidgets
            logFile.Write(L'\n');
        }

        ////write footer
        //logFile.Write(L'\n');
        //logFile.Write(formatTime<wxString>(L"[%X] ") + _("Stop") + L" (" + _("Total time:") + L" " + wxTimeSpan::Milliseconds(totalTime.Time()).Format() + L")\n");
    }

    void limitLogfileCount(size_t maxCount) const //throw()
    {
        std::vector<Zstring> logFiles;
        FindLogfiles traverseCallback(toZ(jobName_), logFiles);

        traverseFolder(beforeLast(logfileName, FILE_NAME_SEPARATOR), //throw();
                       false, //don't follow symlinks
                       traverseCallback);

        if (logFiles.size() <= maxCount)
            return;

        //delete oldest logfiles
        std::nth_element(logFiles.begin(), logFiles.end() - maxCount, logFiles.end()); //take advantage of logfile naming convention to find oldest files

        std::for_each(logFiles.begin(), logFiles.end() - maxCount,
        [](const Zstring& filename) { try { removeFile(filename); } catch (FileError&) {} });
    }

    //Zstring getLogfileName() const { return logfileName; }

private:
    static Zstring findUniqueLogname(const Zstring& logfileDirectory, const wxString& jobName)
    {
        //create logfile directory
        Zstring logfileDir = logfileDirectory.empty() ?
                             getConfigDir() + Zstr("Logs") :
                             getFormattedDirectoryName(logfileDirectory);

        if (!dirExists(logfileDir))
            createDirectory(logfileDir); //throw FileError; create recursively if necessary

        //assemble logfile name
        const Zstring logfileName = appendSeparator(logfileDir) + toZ(jobName) + Zstr(" ") + formatTime<Zstring>(Zstr("%Y-%m-%d %H%M%S"));

        //ensure uniqueness
        Zstring output = logfileName + Zstr(".log");

        for (int i = 1; somethingExists(output); ++i)
            output = logfileName + Zstr('_') + numberTo<Zstring>(i) + Zstr(".log");
        return output;
    }

    const wxString jobName_;
    const Zstring logfileName;
    wxFFile logFile;
    wxStopWatch totalTime;
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
    returnValue(returnVal),
    syncStatusFrame(*this, *this, nullptr, showProgress, jobName, execWhenFinished, execFinishedHistory)
{
    if (logFileCountMax > 0) //init log file: starts internal timer!
        if (!tryReportingError([&]
    {
        logFile.reset(new LogFile(toZ(logfileDirectory), jobName)); //throw FileError
            logFile->limitLogfileCount(logFileCountMax); //throw()
        }, *this))
    {
        returnValue = -7;
        throw BatchAbortProcess();
    }

    //::wxSetEnv(L"logfile", logFile->getLogfileName());
}


BatchStatusHandler::~BatchStatusHandler()
{
    const int totalErrors = errorLog.getItemCount(TYPE_ERROR | TYPE_FATAL_ERROR); //evaluate before finalizing log

    //finalize error log
    std::wstring finalStatus;
    if (abortIsRequested())
    {
        returnValue = -4;
        finalStatus = _("Synchronization aborted!");
        errorLog.logMsg(finalStatus, TYPE_ERROR);
    }
    else if (totalErrors > 0)
    {
        returnValue = -5;
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

    //print the results list: logfile
    if (logFile.get())
    {
        logFile->writeLog(errorLog, finalStatus,
                          getObjectsCurrent(PHASE_SYNCHRONIZING), getDataCurrent(PHASE_SYNCHRONIZING),
                          getObjectsTotal  (PHASE_SYNCHRONIZING), getDataTotal  (PHASE_SYNCHRONIZING));
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
                                   ReturnWarningDlg::BUTTON_IGNORE | ReturnWarningDlg::BUTTON_SWITCH | ReturnWarningDlg::BUTTON_ABORT,
                                   warningMessage + L"\n\n" + _("Press \"Switch\" to resolve issues in FreeFileSync main dialog."),
                                   dontWarnAgain))
            {
                case ReturnWarningDlg::BUTTON_ABORT:
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
                                 ReturnErrorDlg::BUTTON_IGNORE |  ReturnErrorDlg::BUTTON_RETRY | ReturnErrorDlg::BUTTON_ABORT,
                                 errorMessage, &ignoreNextErrors))
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
            switch (showErrorDlg(syncStatusFrame.getAsWindow(),
                                 ReturnErrorDlg::BUTTON_IGNORE |  ReturnErrorDlg::BUTTON_ABORT,
                                 errorMessage, &ignoreNextErrors))
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
    syncStatusFrame.updateProgress();
}


void BatchStatusHandler::abortThisProcess()
{
    requestAbortion(); //just make sure...
    throw BatchAbortProcess();  //abort can be triggered by syncStatusFrame
}
