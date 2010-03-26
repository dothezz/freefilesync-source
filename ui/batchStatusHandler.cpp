// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) 2008-2010 ZenJu (zhnmju123 AT gmx.de)                    *
// **************************************************************************
//
#include "batchStatusHandler.h"
//#include "smallDialogs.h"
#include "messagePopup.h"
#include <wx/ffile.h>
#include <wx/msgdlg.h>
#include "../shared/standardPaths.h"
#include "../shared/fileHandling.h"
#include "../shared/stringConv.h"
#include "../shared/globalFunctions.h"
#include "../shared/appMain.h"
#include "../shared/util.h"

using namespace FreeFileSync;


class LogFile
{
public:
    LogFile(const wxString& logfileDirectory, const wxString& batchFilename) //throw (FileError&)
    {
        const wxString logfileName = findUniqueLogname(logfileDirectory, batchFilename);

        logFile.Open(logfileName, wxT("w"));
        if (!logFile.IsOpened())
            throw FileError(wxString(_("Unable to create logfile!")) + wxT("\"") + logfileName + wxT("\""));

        //write header
        wxString headerLine = wxString(wxT("FreeFileSync - "))  +
                              _("Batch execution") + wxT(" (") +
                              _("Date") + wxT(": ") + wxDateTime::Now().FormatDate() + wxT(" ") + //"Date" is used at other places too
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
    static wxString extractJobName(const wxString& batchFilename)
    {
        using namespace globalFunctions;

        const wxString shortName = batchFilename.AfterLast(FILE_NAME_SEPARATOR); //returns the whole string if seperator not found
        const wxString jobName = shortName.BeforeLast(wxChar('.')); //returns empty string if seperator not found
        return jobName.IsEmpty() ? shortName : jobName;
    }


    static wxString findUniqueLogname(const wxString& logfileDirectory, const wxString& batchFilename)
    {
        using namespace globalFunctions;

        //create logfile directory
        Zstring logfileDir = logfileDirectory.empty() ?
                             wxToZ(FreeFileSync::getConfigDir() + wxT("Logs")) :
                             FreeFileSync::getFormattedDirectoryName(wxToZ(logfileDirectory));

        if (!FreeFileSync::dirExists(logfileDir))
            FreeFileSync::createDirectory(logfileDir); //create recursively if necessary: may throw (FileError&)

        //assemble logfile name
        if (!logfileDir.EndsWith(FILE_NAME_SEPARATOR))
            logfileDir += FILE_NAME_SEPARATOR;

        wxString logfileName = zToWx(logfileDir);

        //add prefix
        logfileName += extractJobName(batchFilename) + wxT(" ");

        //add timestamp
        wxString timeNow = wxDateTime::Now().FormatISOTime();
        timeNow.Replace(wxT(":"), wxT("-"));
        logfileName += wxDateTime::Now().FormatISODate() + wxChar(' ') + timeNow;

        wxString output = logfileName + wxT(".log");

        //ensure uniqueness
        if (FreeFileSync::fileExists(wxToZ(output)))
        {
            //if it's not unique, add a postfix number
            int postfix = 1;
            while (FreeFileSync::fileExists(wxToZ(logfileName + wxT('_') + FreeFileSync::numberToWxString(postfix, false) + wxT(".log"))))
                ++postfix;

            output = logfileName + wxT('_') + numberToWxString(postfix, false) + wxT(".log");
        }

        return output;
    }

    wxFFile logFile;
    wxStopWatch totalTime;
};


//##############################################################################################################################
BatchStatusHandler::BatchStatusHandler(bool runSilent,
                                       const wxString& batchFilename,
                                       const wxString* logfileDirectory,
                                       const xmlAccess::OnError handleError,
                                       int& returnVal) :
    exitWhenFinished(runSilent), //=> exit immediately when finished
    handleError_(handleError),
    currentProcess(StatusHandler::PROCESS_NONE),
    returnValue(returnVal),
    syncStatusFrame(*this, NULL, runSilent)
{
    if (logfileDirectory)
    {
        try
        {
            logFile.reset(new LogFile(*logfileDirectory, batchFilename));
        }
        catch (FreeFileSync::FileError& error)
        {
            wxMessageBox(error.show(), _("Error"), wxOK | wxICON_ERROR);
            returnValue = -7;
            throw FreeFileSync::AbortThisProcess();
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
    if (!exitWhenFinished || syncStatusFrame.getAsWindow()->IsShown()) //warning: wxWindow::Show() is called within processHasFinished()!
    {
        //print the results list: GUI
        wxString finalMessage;
        if (totalErrors > 0)
        {
            wxString header(_("Warning: Synchronization failed for %x item(s):"));
            header.Replace(wxT("%x"), FreeFileSync::numberToWxString(totalErrors, true), false);
            finalMessage += header + wxT("\n\n");
        }

        const ErrorLogging::MessageEntry& messages = errorLog.getFormattedMessages();
        for (ErrorLogging::MessageEntry::const_iterator i = messages.begin(); i != messages.end(); ++i)
        {
            finalMessage += *i;
            finalMessage += wxT("\n\n");
        }

        //notify about (logical) application main window => program won't quit, but stay on this dialog
        FreeFileSync::AppMainWindow::setMainWindow(syncStatusFrame.getAsWindow());

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
void BatchStatusHandler::updateStatusText(const Zstring& text)
{
    if (currentProcess == StatusHandler::PROCESS_SYNCHRONIZING && logFile.get()) //write file transfer information to log
        errorLog.logInfo(zToWx(text));

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


void BatchStatusHandler::reportInfo(const wxString& infoMessage)
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
                              WarningDlg::BUTTON_IGNORE | WarningDlg::BUTTON_ABORT,
                              warningMessage,
                              dontWarnAgain);
        warningDlg.Raise();
        const WarningDlg::Response rv = static_cast<WarningDlg::Response>(warningDlg.ShowModal());
        switch (rv)
        {
        case WarningDlg::BUTTON_ABORT:
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
                          errorMessage + wxT("\n\n\n") + _("Ignore this error, retry or abort?"),
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
    throw FreeFileSync::AbortThisProcess();  //abort can be triggered by syncStatusFrame
}
