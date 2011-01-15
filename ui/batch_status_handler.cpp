// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) 2008-2011 ZenJu (zhnmju123 AT gmx.de)                    *
// **************************************************************************
//
#include "batch_status_handler.h"
//#include "small_dlgs.h"
#include "msg_popup.h"
#include <wx/ffile.h>
#include <wx/msgdlg.h>
#include "../shared/standard_paths.h"
#include "../shared/file_handling.h"
#include "../shared/string_conv.h"
#include "../shared/global_func.h"
#include "../shared/app_main.h"
#include "../shared/util.h"

using namespace ffs3;


class LogFile
{
public:
    LogFile(const wxString& logfileDirectory, const wxString& jobName) //throw (FileError&)
    {
        const wxString logfileName = findUniqueLogname(logfileDirectory, jobName);

        logFile.Open(logfileName, wxT("w"));
        if (!logFile.IsOpened())
            throw FileError(wxString(_("Unable to create logfile!")) + wxT("\"") + logfileName + wxT("\""));

        //write header
        wxString headerLine = wxString(wxT("FreeFileSync - "))  +
                              _("Batch execution") + wxT(" (") +
                              _("Date") + wxT(": ") + wxDateTime::Now().FormatDate() + wxT(" ") + //"Date" is used at other places, too
                              _("Time") + wxT(":") + wxT(" ") +  wxDateTime::Now().FormatTime() + wxT(")");
        logFile.Write(headerLine + wxChar('\n'));
        logFile.Write(wxString().Pad(headerLine.Len(), wxChar('-')) + wxChar('\n') + wxChar('\n'));

        wxString caption = _("Log-messages:");
        logFile.Write(caption + wxChar('\n'));
        logFile.Write(wxString().Pad(caption.Len(), wxChar('-')) + wxChar('\n'));

        logFile.Write(wxString(wxT("[")) + wxDateTime::Now().FormatTime() + wxT("] ") + _("Start") + wxChar('\n') + wxChar('\n'));

        totalTime.Start(); //measure total time
    }

    void writeLog(const ErrorLogging& log)
    {
        //write actual logfile
        const ErrorLogging::MessageEntry& messages = log.getFormattedMessages();
        for (ErrorLogging::MessageEntry::const_iterator i = messages.begin(); i != messages.end(); ++i)
            logFile.Write(*i + wxChar('\n'));
    }

    ~LogFile()
    {
        //write ending
        logFile.Write(wxChar('\n'));

        const long time = totalTime.Time(); //retrieve total time
        logFile.Write(wxString(wxT("[")) + wxDateTime::Now().FormatTime() + wxT("] "));
        logFile.Write(wxString(_("Stop")) + wxT(" (") + _("Total time:") + wxT(" ") + (wxTimeSpan::Milliseconds(time)).Format() + wxT(")"));
    }

private:
    static wxString findUniqueLogname(const wxString& logfileDirectory, const wxString& jobName)
    {
        using namespace common;

        //create logfile directory
        Zstring logfileDir = logfileDirectory.empty() ?
                             wxToZ(ffs3::getConfigDir() + wxT("Logs")) :
                             ffs3::getFormattedDirectoryName(wxToZ(logfileDirectory));

        if (!ffs3::dirExists(logfileDir))
            ffs3::createDirectory(logfileDir); //create recursively if necessary: may throw (FileError&)

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
        for (int i = 1; ffs3::somethingExists(wxToZ(output)); ++i)
            output = logfileName + wxChar('_') + common::numberToString(i) + wxT(".log");

        return output;
    }

    wxFFile logFile;
    wxStopWatch totalTime;
};


//##############################################################################################################################
BatchStatusHandler::BatchStatusHandler(bool runSilent,
                                       const wxString& jobName,
                                       const wxString* logfileDirectory,
                                       const xmlAccess::OnError handleError,
                                       const SwitchToGui& switchBatchToGui, //functionality to change from batch mode to GUI mode
                                       int& returnVal) :
    switchBatchToGui_(switchBatchToGui),
    exitWhenFinished(runSilent), //=> exit immediately when finished
    switchToGuiRequested(false),
    handleError_(handleError),
    currentProcess(StatusHandler::PROCESS_NONE),
    returnValue(returnVal),
    syncStatusFrame(*this, NULL, runSilent, jobName)
{
    if (logfileDirectory)
    {
        try
        {
            logFile.reset(new LogFile(*logfileDirectory, jobName));
        }
        catch (ffs3::FileError& error)
        {
            wxMessageBox(error.msg(), _("Error"), wxOK | wxICON_ERROR);
            returnValue = -7;
            throw ffs3::AbortThisProcess();
        }
    }

    assert(runSilent || handleError != xmlAccess::ON_ERROR_EXIT); //shouldn't be selectable from GUI settings
}


BatchStatusHandler::~BatchStatusHandler()
{
    const int totalErrors = errorLog.errorsTotal(); //evaluate before finalizing log

    //finalize error log
    if (abortIsRequested())
    {
        returnValue = -4;
        errorLog.logError(_("Synchronization aborted!"));
    }
    else if (totalErrors)
    {
        returnValue = -5;
        errorLog.logWarning(_("Synchronization completed with errors!"));
    }
    else
        errorLog.logInfo(_("Synchronization completed successfully!"));


    //print the results list: logfile
    if (logFile.get())
        logFile->writeLog(errorLog);

    //decide whether to stay on status screen or exit immediately...
    if (switchToGuiRequested) //-> avoid recursive yield() calls, thous switch not before ending batch mode
    {
        switchBatchToGui_.execute(); //open FreeFileSync GUI
        syncStatusFrame.closeWindowDirectly(); //syncStatusFrame is main window => program will quit directly
    }
    else if (!exitWhenFinished || syncStatusFrame.getAsWindow()->IsShown()) //warning: wxWindow::Show() is called within processHasFinished()!
    {
        //print the results list: GUI
        wxString finalMessage;
        if (totalErrors > 0)
        {
            wxString header(_("Warning: Synchronization failed for %x item(s):"));
            header.Replace(wxT("%x"), ffs3::numberToStringSep(totalErrors), false);
            finalMessage += header + wxT("\n\n");
        }

        const ErrorLogging::MessageEntry& messages = errorLog.getFormattedMessages();
        for (ErrorLogging::MessageEntry::const_iterator i = messages.begin(); i != messages.end(); ++i)
        {
            finalMessage += *i;
            finalMessage += wxT("\n\n");
        }

        //notify about (logical) application main window => program won't quit, but stay on this dialog
        ffs3::AppMainWindow::setMainWindow(syncStatusFrame.getAsWindow());

        //notify to syncStatusFrame that current process has ended
        if (abortIsRequested())
            syncStatusFrame.processHasFinished(SyncStatus::ABORTED, finalMessage);  //enable okay and close events
        else if (totalErrors)
            syncStatusFrame.processHasFinished(SyncStatus::FINISHED_WITH_ERROR, finalMessage);
        else
            syncStatusFrame.processHasFinished(SyncStatus::FINISHED_WITH_SUCCESS, finalMessage);
    }
    else
        syncStatusFrame.closeWindowDirectly(); //syncStatusFrame is main window => program will quit directly
}


inline
void BatchStatusHandler::reportInfo(const Zstring& text)
{
    if (currentProcess == StatusHandler::PROCESS_SYNCHRONIZING && logFile.get()) //write file transfer information to log
        errorLog.logInfo(zToWx(text)); //avoid spamming with file copy info: visually identifying warning messages has priority! however when saving to a log file wee need this info

    syncStatusFrame.setStatusText_NoUpdate(text);
}


void BatchStatusHandler::initNewProcess(int objectsTotal, wxLongLong dataTotal, StatusHandler::Process processID)
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
void BatchStatusHandler::updateProcessedData(int objectsProcessed, wxLongLong dataProcessed)
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


void BatchStatusHandler::logInfo(const wxString& infoMessage)
{
    errorLog.logInfo(infoMessage);
}


void BatchStatusHandler::reportWarning(const wxString& warningMessage, bool& warningActive)
{
    errorLog.logWarning(warningMessage);

    if (!warningActive)
        return;

    switch (handleError_)
    {
    case xmlAccess::ON_ERROR_POPUP:
    {
        //show popup and ask user how to handle warning
        bool dontWarnAgain = false;
        WarningDlg warningDlg(NULL,
                              WarningDlg::BUTTON_IGNORE | WarningDlg::BUTTON_SWITCH | WarningDlg::BUTTON_ABORT,
                              warningMessage + wxT("\n\n") + _("Press \"Switch\" to open FreeFileSync GUI mode."),
                              dontWarnAgain);
        warningDlg.Raise();
        const WarningDlg::Response rv = static_cast<WarningDlg::Response>(warningDlg.ShowModal());
        switch (rv)
        {
        case WarningDlg::BUTTON_ABORT:
            abortThisProcess();
            break;

        case WarningDlg::BUTTON_SWITCH:
            errorLog.logWarning(_("Switching to FreeFileSync GUI mode..."));
            switchToGuiRequested = true;
            abortThisProcess();
            break;

        case WarningDlg::BUTTON_IGNORE: //no unhandled error situation!
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
        ErrorDlg errorDlg(NULL,
                          ErrorDlg::BUTTON_IGNORE |  ErrorDlg::BUTTON_RETRY | ErrorDlg::BUTTON_ABORT,
                          errorMessage,
                          ignoreNextErrors);
        errorDlg.Raise();
        const ErrorDlg::ReturnCodes rv = static_cast<ErrorDlg::ReturnCodes>(errorDlg.ShowModal());
        switch (rv)
        {
        case ErrorDlg::BUTTON_IGNORE:
            if (ignoreNextErrors) //falsify only
                handleError_ = xmlAccess::ON_ERROR_IGNORE;
            errorLog.logError(errorMessage);
            return ErrorHandler::IGNORE_ERROR;

        case ErrorDlg::BUTTON_RETRY:
            return ErrorHandler::RETRY;

        case ErrorDlg::BUTTON_ABORT:
            errorLog.logError(errorMessage);
            abortThisProcess();
        }
    }
    break; //used if last switch didn't find a match

    case xmlAccess::ON_ERROR_EXIT: //abort
        errorLog.logError(errorMessage);
        abortThisProcess();

    case xmlAccess::ON_ERROR_IGNORE:
        errorLog.logError(errorMessage);
        return ErrorHandler::IGNORE_ERROR;
    }

    assert(false);
    return ErrorHandler::IGNORE_ERROR; //dummy value

}


void BatchStatusHandler::reportFatalError(const wxString& errorMessage)
{
    if (handleError_ == xmlAccess::ON_ERROR_POPUP)
        exitWhenFinished = false; //log fatal error and show it on status dialog

    errorLog.logFatalError(errorMessage);
    abortThisProcess();
}


inline
void BatchStatusHandler::forceUiRefresh()
{
    syncStatusFrame.updateStatusDialogNow();
}


void BatchStatusHandler::abortThisProcess()
{
    requestAbortion();
    throw ffs3::AbortThisProcess();  //abort can be triggered by syncStatusFrame
}
