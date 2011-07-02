// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) 2008-2011 ZenJu (zhnmju123 AT gmx.de)                    *
// **************************************************************************
//
#include "batch_status_handler.h"
#include "msg_popup.h"
#include <wx/ffile.h>
#include <wx/msgdlg.h>
#include "../shared/standard_paths.h"
#include "../shared/file_handling.h"
#include "../shared/resolve_path.h"
#include "../shared/string_conv.h"
#include "../shared/global_func.h"
#include "../shared/app_main.h"
#include "../shared/util.h"
#include "../shared/file_traverser.h"

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
        if (fileName.StartsWith(prefix_) && fileName.EndsWith(Zstr(".log")))
            logfiles_.push_back(fullName);
    }

    virtual void onSymlink(const Zchar* shortName, const Zstring& fullName, const SymlinkInfo& details) {}

    virtual ReturnValDir onDir(const Zchar* shortName, const Zstring& fullName)
    {
        return Loki::Int2Type<ReturnValDir::TRAVERSING_DIR_IGNORE>(); //DON'T traverse into subdirs
    }

    virtual void onError(const wxString& errorText) {} //errors are not really critical in this context

private:
    const Zstring prefix_;
    std::vector<Zstring>& logfiles_;
};

void removeFileNoThrow(const Zstring& filename)
{
    try
    {
        zen::removeFile(filename);
    }
    catch(...) {}
}
}


class LogFile
{
public:
    LogFile(const wxString& logfileDirectory, const wxString& jobName) : jobName_(jobName) //throw (FileError&)
    {
        logfileName = findUniqueLogname(logfileDirectory, jobName);

        logFile.Open(logfileName, wxT("w"));
        if (!logFile.IsOpened())
            throw FileError(wxString(_("Unable to create logfile!")) + wxT("\"") + logfileName + wxT("\""));

        //write header
        wxString headerLine = wxString(wxT("FreeFileSync - ")) + _("Batch execution") +
                              wxT(" (") + _("Date") + wxT(": ") + wxDateTime::Now().FormatDate() +  wxT(")"); //"Date" is used at other places, too

        logFile.Write(headerLine + wxChar('\n'));
        logFile.Write(wxString().Pad(headerLine.Len(), wxChar('-')) + wxChar('\n') + wxChar('\n'));

        /*
        wxString caption = _("Log-messages:");
        logFile.Write(caption + wxChar('\n'));
        logFile.Write(wxString().Pad(caption.Len(), wxChar('-')) + wxChar('\n'));
        */

        logItemStart = wxString(wxT("[")) + wxDateTime::Now().FormatTime() + wxT("] ") + _("Start");

        totalTime.Start(); //measure total time
    }

    void writeLog(const ErrorLogging& log, const wxString& finalStatus)
    {
        logFile.Write(finalStatus + wxChar('\n') + wxChar('\n')); //highlight result by placing at beginning of file

        logFile.Write(logItemStart + wxChar('\n') + wxChar('\n'));

        //write actual logfile
        const std::vector<wxString>& messages = log.getFormattedMessages();
        for (std::vector<wxString>::const_iterator i = messages.begin(); i != messages.end(); ++i)
            logFile.Write(*i + wxChar('\n'));

        //write ending
        logFile.Write(wxChar('\n'));

        const long time = totalTime.Time(); //retrieve total time
        logFile.Write(wxString(wxT("[")) + wxDateTime::Now().FormatTime() + wxT("] "));
        logFile.Write(wxString(_("Stop")) + wxT(" (") + _("Total time:") + wxT(" ") + (wxTimeSpan::Milliseconds(time)).Format() + wxT(")"));
    }

    void limitLogfileCount(size_t maxCount) const
    {
        std::vector<Zstring> logFiles;
        FindLogfiles traverseCallback(wxToZ(jobName_), logFiles);

        traverseFolder(wxToZ(logfileName).BeforeLast(common::FILE_NAME_SEPARATOR), //throw();
                       false, //don't follow symlinks
                       traverseCallback);

        if (logFiles.size() <= maxCount)
            return;

        //delete oldest logfiles
        std::sort(logFiles.begin(), logFiles.end()); //take advantage of logfile naming convention to sort by age
        std::for_each(logFiles.begin(), logFiles.end() - maxCount, ::removeFileNoThrow);
    }

private:
    static wxString findUniqueLogname(const wxString& logfileDirectory, const wxString& jobName)
    {
        using namespace common;

        //create logfile directory
        Zstring logfileDir = logfileDirectory.empty() ?
                             wxToZ(zen::getConfigDir() + wxT("Logs")) :
                             zen::getFormattedDirectoryName(wxToZ(logfileDirectory));

        if (!zen::dirExists(logfileDir))
            zen::createDirectory(logfileDir); //create recursively if necessary: may throw (FileError&)

        //assemble logfile name
        if (!logfileDir.EndsWith(FILE_NAME_SEPARATOR))
            logfileDir += FILE_NAME_SEPARATOR;

        wxString logfileName = zToWx(logfileDir);

        //add prefix
        logfileName += jobName + wxT(" ");

        //add timestamp
        wxString timeNow = wxDateTime::Now().FormatISOTime();
        timeNow.Replace(wxT(":"), wxT(""));
        logfileName += wxDateTime::Now().FormatISODate() + wxChar(' ') + timeNow;

        wxString output = logfileName + wxT(".log");

        //ensure uniqueness
        for (int i = 1; zen::somethingExists(wxToZ(output)); ++i)
            output = logfileName + wxChar('_') + zen::toString<wxString>(i) + wxT(".log");

        return output;
    }

    const wxString jobName_;
    wxString logfileName;
    wxFFile logFile;
    wxStopWatch totalTime;
    wxString logItemStart;
};


//##############################################################################################################################
BatchStatusHandler::BatchStatusHandler(bool runSilent,
                                       const wxString& jobName,
                                       const wxString* logfileDirectory,
                                       size_t logFileMaxCount,
                                       const xmlAccess::OnError handleError,
                                       const SwitchToGui& switchBatchToGui, //functionality to change from batch mode to GUI mode
                                       int& returnVal) :
    switchBatchToGui_(switchBatchToGui),
    exitWhenFinished(runSilent), //=> exit immediately when finished
    switchToGuiRequested(false),
    handleError_(handleError),
    currentProcess(StatusHandler::PROCESS_NONE),
    returnValue(returnVal),
    syncStatusFrame(*this, NULL, SyncStatus::SCANNING, runSilent, jobName)
{
    if (logfileDirectory && logFileMaxCount > 0)
    {
        try
        {
            logFile.reset(new LogFile(*logfileDirectory, jobName));
            logFile->limitLogfileCount(logFileMaxCount);
        }
        catch (zen::FileError& error)
        {
            wxMessageBox(error.msg(), _("Error"), wxOK | wxICON_ERROR);
            returnValue = -7;
            throw BatchAbortProcess();
        }
    }

    assert(runSilent || handleError != xmlAccess::ON_ERROR_EXIT); //shouldn't be selectable from GUI settings
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
        finalStatus = _("Synchronization completed successfully!");
        errorLog.logMsg(finalStatus, TYPE_INFO);
    }

    //print the results list: logfile
    if (logFile.get())
        logFile->writeLog(errorLog, finalStatus);

    //decide whether to stay on status screen or exit immediately...
    if (switchToGuiRequested) //-> avoid recursive yield() calls, thous switch not before ending batch mode
    {
        switchBatchToGui_.execute(); //open FreeFileSync GUI
        syncStatusFrame.closeWindowDirectly(); //syncStatusFrame is main window => program will quit directly
    }
    else if (!exitWhenFinished || syncStatusFrame.getAsWindow()->IsShown()) //warning: wxWindow::Show() is called within processHasFinished()!
    {
        //notify about (logical) application main window => program won't quit, but stay on this dialog
        zen::AppMainWindow::setMainWindow(syncStatusFrame.getAsWindow());

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


inline
void BatchStatusHandler::reportInfo(const Zstring& text)
{
    if (currentProcess == StatusHandler::PROCESS_SYNCHRONIZING) //write file transfer information to log
        errorLog.logMsg(zToWx(text), TYPE_INFO); //avoid spamming with file copy info: visually identifying warning messages has priority! however when saving to a log file wee need this info

    syncStatusFrame.setStatusText_NoUpdate(text);
}


void BatchStatusHandler::initNewProcess(int objectsTotal, zen::Int64 dataTotal, StatusHandler::Process processID)
{
    currentProcess = processID;

    switch (currentProcess)
    {
        case StatusHandler::PROCESS_SCANNING:
            syncStatusFrame.resetGauge(0, 0); //initialize some gui elements (remaining time, speed)
            syncStatusFrame.setCurrentStatus(SyncStatus::SCANNING);
            break;
        case StatusHandler::PROCESS_COMPARING_CONTENT:
            syncStatusFrame.resetGauge(objectsTotal, dataTotal);
            syncStatusFrame.setCurrentStatus(SyncStatus::COMPARING_CONTENT);
            break;
        case StatusHandler::PROCESS_SYNCHRONIZING:
            syncStatusFrame.resetGauge(objectsTotal, dataTotal);
            syncStatusFrame.setCurrentStatus(SyncStatus::SYNCHRONIZING);
            break;
        case StatusHandler::PROCESS_NONE:
            assert(false);
            break;
    }
}


inline
void BatchStatusHandler::updateProcessedData(int objectsProcessed, zen::Int64 dataProcessed)
{
    switch (currentProcess)
    {
        case StatusHandler::PROCESS_SCANNING:
            syncStatusFrame.incScannedObjects_NoUpdate(objectsProcessed);
            break;
        case StatusHandler::PROCESS_COMPARING_CONTENT:
        case StatusHandler::PROCESS_SYNCHRONIZING:
            syncStatusFrame.incProgressIndicator_NoUpdate(objectsProcessed, dataProcessed);
            break;
        case StatusHandler::PROCESS_NONE:
            assert(false);
            break;
    }
}


void BatchStatusHandler::reportWarning(const wxString& warningMessage, bool& warningActive)
{
    errorLog.logMsg(warningMessage, TYPE_WARNING);

    if (!warningActive)
        return;

    switch (handleError_)
    {
        case xmlAccess::ON_ERROR_POPUP:
        {
            //show popup and ask user how to handle warning
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


ErrorHandler::Response BatchStatusHandler::reportError(const wxString& errorMessage)
{
    switch (handleError_)
    {
        case xmlAccess::ON_ERROR_POPUP:
        {
            bool ignoreNextErrors = false;

            switch (showErrorDlg(ReturnErrorDlg::BUTTON_IGNORE |  ReturnErrorDlg::BUTTON_RETRY | ReturnErrorDlg::BUTTON_ABORT,
                                 errorMessage,
                                 ignoreNextErrors))
            {
                case ReturnErrorDlg::BUTTON_IGNORE:
                    if (ignoreNextErrors) //falsify only
                        handleError_ = xmlAccess::ON_ERROR_IGNORE;
                    errorLog.logMsg(errorMessage, TYPE_ERROR);
                    return ErrorHandler::IGNORE_ERROR;

                case ReturnErrorDlg::BUTTON_RETRY:
                    return ErrorHandler::RETRY;

                case ReturnErrorDlg::BUTTON_ABORT:
                    errorLog.logMsg(errorMessage, TYPE_ERROR);
                    abortThisProcess();
            }
        }
        break; //used if last switch didn't find a match

        case xmlAccess::ON_ERROR_EXIT: //abort
            errorLog.logMsg(errorMessage, TYPE_ERROR);
            abortThisProcess();

        case xmlAccess::ON_ERROR_IGNORE:
            errorLog.logMsg(errorMessage, TYPE_ERROR);
            return ErrorHandler::IGNORE_ERROR;
    }

    assert(false);
    return ErrorHandler::IGNORE_ERROR; //dummy value

}


void BatchStatusHandler::reportFatalError(const wxString& errorMessage)
{
    if (handleError_ == xmlAccess::ON_ERROR_POPUP)
        exitWhenFinished = false; //log fatal error and show it on status dialog

    errorLog.logMsg(errorMessage, TYPE_FATAL_ERROR);
}


inline
void BatchStatusHandler::forceUiRefresh()
{
    syncStatusFrame.updateStatusDialogNow();
}


void BatchStatusHandler::abortThisProcess()
{
    requestAbortion();
    throw BatchAbortProcess();  //abort can be triggered by syncStatusFrame
}
