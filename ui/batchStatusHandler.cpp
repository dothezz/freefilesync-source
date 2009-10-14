#include "batchStatusHandler.h"
#include "smallDialogs.h"
#include <wx/taskbar.h>
#include "../algorithm.h"
#include <wx/ffile.h>
#include <wx/msgdlg.h>
#include "../shared/systemConstants.h"
#include "../shared/standardPaths.h"
#include "../shared/fileHandling.h"
#include "../shared/stringConv.h"
#include "../library/resources.h"
#include "../shared/globalFunctions.h"
#include "../shared/appMain.h"

using namespace FreeFileSync;


class LogFile
{
public:
    LogFile(const wxString& logfileDirectory)
    {
        //create logfile directory
        const wxString logfileDir = logfileDirectory.empty() ? FreeFileSync::getDefaultLogDirectory() : logfileDirectory;
        if (!FreeFileSync::dirExists(wxToZ(logfileDir)))
            try
            {
                FreeFileSync::createDirectory(wxToZ(logfileDir)); //create recursively if necessary
            }
            catch (FreeFileSync::FileError&)
            {
                readyToWrite = false;
                return;
            }

        //assemble logfile name
        wxString logfileName = logfileDir;
        if (!logfileName.empty() && logfileName.Last() != globalFunctions::FILE_NAME_SEPARATOR)
            logfileName += globalFunctions::FILE_NAME_SEPARATOR;

        wxString timeNow = wxDateTime::Now().FormatISOTime();
        timeNow.Replace(wxT(":"), wxT("-"));
        logfileName += wxDateTime::Now().FormatISODate() + wxChar(' ') + timeNow + wxT(".log");


        logFile.Open(logfileName.c_str(), wxT("w"));
        readyToWrite = logFile.IsOpened();
        if (readyToWrite)
        {
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
    }

    ~LogFile()
    {
        if (readyToWrite) //could be reached when creation of logfile failed
        {
            //write actual logfile
            const std::vector<wxString>& messages = errorLog.getFormattedMessages();
            for (std::vector<wxString>::const_iterator i = messages.begin(); i != messages.end(); ++i)
                logFile.Write(*i + wxChar('\n'));

            //write ending
            logFile.Write(wxChar('\n'));

            const long time = totalTime.Time(); //retrieve total time
            logFile.Write(wxString(wxT("[")) + wxDateTime::Now().FormatTime() + wxT("] "));
            logFile.Write(wxString(_("Stop")) + wxT(" (") + _("Total time:") + wxT(" ") + (wxTimeSpan::Milliseconds(time)).Format() + wxT(")"));

            //logFile.close(); <- not needed
        }
    }


    bool isOkay() //has to be checked before LogFile can be used!
    {
        return readyToWrite;
    }


    void logError(const wxString& errorMessage)
    {
        errorLog.logError(errorMessage);
    }

    void logWarning(const wxString& warningMessage)
    {
        errorLog.logWarning(warningMessage);
    }

    void logInfo(const wxString& infoMessage)
    {
        errorLog.logInfo(infoMessage);
    }

    bool errorsOccured()
    {
        return errorLog.errorsTotal() > 0;
    }

private:
    bool readyToWrite;
    wxFFile logFile;
    wxStopWatch totalTime;

    FreeFileSync::ErrorLogging errorLog;
};


class FfsTrayIcon : public wxTaskBarIcon
{
public:
    FfsTrayIcon(StatusHandler* statusHandler) :
        m_statusHandler(statusHandler),
        processPaused(false),
        percentage(_("%x Percent")),
        currentProcess(StatusHandler::PROCESS_NONE),
        totalObjects(0),
        currentObjects(0)
    {
        running.reset(new wxIcon(*GlobalResources::getInstance().programIcon));
        paused.reset(new wxIcon);
        paused->CopyFromBitmap(*GlobalResources::getInstance().bitmapFFSPaused);

        wxTaskBarIcon::SetIcon(*running, wxT("FreeFileSync"));
    }

    ~FfsTrayIcon() {}

    enum Selection
    {
        CONTEXT_PAUSE,
        CONTEXT_ABORT,
        CONTEXT_ABOUT
    };

    virtual wxMenu* CreatePopupMenu()
    {
        wxMenu* contextMenu = new wxMenu;
        contextMenu->Append(CONTEXT_PAUSE, _("&Pause"), wxEmptyString, wxITEM_CHECK);
        contextMenu->Check(CONTEXT_PAUSE, processPaused);
        contextMenu->Append(CONTEXT_ABOUT, _("&About..."));
        contextMenu->AppendSeparator();
        contextMenu->Append(CONTEXT_ABORT, _("&Exit"));
        //event handling
        contextMenu->Connect(wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(FfsTrayIcon::onContextMenuSelection), NULL, this);

        return contextMenu; //ownership transferred to library
    }

    void onContextMenuSelection(wxCommandEvent& event)
    {
        const Selection eventId = static_cast<Selection>(event.GetId());
        switch (eventId)
        {
        case CONTEXT_PAUSE:
            processPaused = !processPaused;
            break;
        case CONTEXT_ABORT:
            processPaused = false;
            m_statusHandler->requestAbortion();
            break;
        case CONTEXT_ABOUT:
        {
            AboutDlg* aboutDlg = new AboutDlg(NULL);
            aboutDlg->ShowModal();
            aboutDlg->Destroy();
        }
        break;
        }
    }

    wxString calcPercentage(const wxLongLong& current, const wxLongLong& total)
    {
        const double ratio = current.ToDouble() * 100 / total.ToDouble();
        wxString output = percentage;
        output.Replace(wxT("%x"), wxString::Format(wxT("%3.2f"), ratio), false);
        return output;
    }

    void updateSysTray()
    {
        switch (currentProcess)
        {
        case StatusHandler::PROCESS_SCANNING:
            wxTaskBarIcon::SetIcon(*running, wxString(wxT("FreeFileSync - ")) + wxString(_("Scanning...")) + wxT(" ") +
                                   globalFunctions::numberToWxString(currentObjects));
            break;
        case StatusHandler::PROCESS_COMPARING_CONTENT:
            wxTaskBarIcon::SetIcon(*running, wxString(wxT("FreeFileSync - ")) + wxString(_("Comparing content...")) + wxT(" ") +
                                   calcPercentage(currentData, totalData));
            break;
        case StatusHandler::PROCESS_SYNCHRONIZING:
            wxTaskBarIcon::SetIcon(*running, wxString(wxT("FreeFileSync - ")) + wxString(_("Synchronizing...")) + wxT(" ") +
                                   calcPercentage(currentData, totalData));
            break;
        case StatusHandler::PROCESS_NONE:
            assert(false);
            break;
        }

        updateUiNow();


        //support for pause button
        if (processPaused)
        {
            wxTaskBarIcon::SetIcon(*paused, wxString(wxT("FreeFileSync - ")) + _("Paused"));
            while (processPaused)
            {
                wxMilliSleep(UI_UPDATE_INTERVAL);
                updateUiNow();
            }
        }
    }

    void initNewProcess(int objectsTotal, wxLongLong dataTotal, StatusHandler::Process processID)
    {
        totalObjects   = objectsTotal;
        totalData      = dataTotal;
        currentObjects = 0;
        currentData    = 0;
        currentProcess = processID;
    }

    void updateProcessedData(int objectsProcessed, wxLongLong dataProcessed)
    {
        currentObjects += objectsProcessed;
        currentData    += dataProcessed;
    }


private:
    StatusHandler* m_statusHandler;
    bool processPaused;
    std::auto_ptr<wxIcon> running;
    std::auto_ptr<wxIcon> paused;
    const wxString percentage;

    //status variables
    StatusHandler::Process currentProcess;
    int        totalObjects;
    wxLongLong totalData;
    int        currentObjects; //each object represents a file or directory processed
    wxLongLong currentData;    //each data element represents one byte for proper progress indicator scaling

};


//##############################################################################################################################

BatchStatusHandlerSilent::BatchStatusHandlerSilent(const xmlAccess::OnError handleError, const wxString& logfileDirectory, int& returnVal) :
    m_handleError(handleError),
    currentProcess(StatusHandler::PROCESS_NONE),
    returnValue(returnVal),
    trayIcon(new FfsTrayIcon(this)),
    m_log(new LogFile(logfileDirectory))
{
    //test if log was instantiated successfully
    if (!m_log->isOkay())
    {
        //handle error: file load
        wxMessageBox(_("Unable to create logfile!"), _("Error"), wxOK | wxICON_ERROR);
        returnValue = -7;
        throw FreeFileSync::AbortThisProcess();
    }
}


BatchStatusHandlerSilent::~BatchStatusHandlerSilent()
{
    //write result
    if (abortIsRequested())
    {
        returnValue = -4;
        m_log->logError(_("Synchronization aborted!"));
    }
    else if (m_log->errorsOccured())
    {
        returnValue = -5;
        m_log->logInfo(_("Synchronization completed with errors!"));
    }
    else
        m_log->logInfo(_("Synchronization completed successfully!"));
}


inline
void BatchStatusHandlerSilent::updateStatusText(const Zstring& text)
{
    switch (currentProcess)
    {
    case StatusHandler::PROCESS_SCANNING:
    case StatusHandler::PROCESS_COMPARING_CONTENT:
        break;
    case StatusHandler::PROCESS_SYNCHRONIZING:
        m_log->logInfo(zToWx(text));
        break;
    case StatusHandler::PROCESS_NONE:
        assert(false);
        break;
    }
}


inline
void BatchStatusHandlerSilent::initNewProcess(int objectsTotal, wxLongLong dataTotal, StatusHandler::Process processID)
{
    currentProcess = processID;

    trayIcon->initNewProcess(objectsTotal, dataTotal, processID);
}


void BatchStatusHandlerSilent::updateProcessedData(int objectsProcessed, wxLongLong dataProcessed)
{
    trayIcon->updateProcessedData(objectsProcessed, dataProcessed);
}


ErrorHandler::Response BatchStatusHandlerSilent::reportError(const wxString& errorMessage)
{
    switch (m_handleError)
    {
    case xmlAccess::ON_ERROR_POPUP:
    {
        bool ignoreNextErrors = false;
        ErrorDlg* errorDlg = new ErrorDlg(NULL,
                                          ErrorDlg::BUTTON_IGNORE |  ErrorDlg::BUTTON_RETRY | ErrorDlg::BUTTON_ABORT,
                                          errorMessage + wxT("\n\n\n") + _("Ignore this error, retry or abort?"),
                                          ignoreNextErrors);
        const ErrorDlg::ReturnCodes rv = static_cast<ErrorDlg::ReturnCodes>(errorDlg->ShowModal());
        errorDlg->Destroy();
        switch (rv)
        {
        case ErrorDlg::BUTTON_IGNORE:
            if (ignoreNextErrors) //falsify only
                m_handleError = xmlAccess::ON_ERROR_IGNORE;
            m_log->logError(errorMessage);
            return ErrorHandler::IGNORE_ERROR;

        case ErrorDlg::BUTTON_RETRY:
            return ErrorHandler::RETRY;

        case ErrorDlg::BUTTON_ABORT:
            m_log->logError(errorMessage);
            abortThisProcess();
        }
    }
    break; //used if last switch didn't find a match

    case xmlAccess::ON_ERROR_EXIT: //abort
        m_log->logError(errorMessage);
        abortThisProcess();

    case xmlAccess::ON_ERROR_IGNORE:
        m_log->logError(errorMessage);
        return ErrorHandler::IGNORE_ERROR;
    }

    assert(false);
    return ErrorHandler::IGNORE_ERROR; //dummy value
}


void BatchStatusHandlerSilent::reportFatalError(const wxString& errorMessage)
{
    switch (m_handleError)
    {
    case xmlAccess::ON_ERROR_POPUP:
    {
        bool dummy = false;
        ErrorDlg* errorDlg = new ErrorDlg(NULL,
                                          ErrorDlg::BUTTON_ABORT,
                                          errorMessage, dummy);
        errorDlg->ShowModal();
        errorDlg->Destroy();
    }
    break;

    case xmlAccess::ON_ERROR_EXIT:
        break;

    case xmlAccess::ON_ERROR_IGNORE:
        break;
    }

    m_log->logError(errorMessage);
    abortThisProcess();
}


void BatchStatusHandlerSilent::reportWarning(const wxString& warningMessage, bool& warningActive)
{
    m_log->logWarning(warningMessage);

    if (!warningActive)
        return;

    switch (m_handleError)
    {
    case xmlAccess::ON_ERROR_POPUP:
    {
        //show popup and ask user how to handle warning
        bool dontWarnAgain = false;
        WarningDlg* warningDlg = new WarningDlg(NULL,
                                                WarningDlg::BUTTON_IGNORE | WarningDlg::BUTTON_ABORT,
                                                warningMessage,
                                                dontWarnAgain);
        const WarningDlg::Response rv = static_cast<WarningDlg::Response>(warningDlg->ShowModal());
        warningDlg->Destroy();
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


void BatchStatusHandlerSilent::addFinalInfo(const wxString& infoMessage)
{
    m_log->logInfo(infoMessage);
}


void BatchStatusHandlerSilent::forceUiRefresh()
{
    trayIcon->updateSysTray(); //needed by sys-tray icon only
}


void BatchStatusHandlerSilent::abortThisProcess() //used by sys-tray menu
{
    requestAbortion();
    throw FreeFileSync::AbortThisProcess();
}

//######################################################################################################


BatchStatusHandlerGui::BatchStatusHandlerGui(const xmlAccess::OnError handleError, int& returnVal) :
    showPopups(true),
    currentProcess(StatusHandler::PROCESS_NONE),
    returnValue(returnVal)
{
    switch (handleError)
    {
    case xmlAccess::ON_ERROR_POPUP:
        showPopups = true;
        break;

    case xmlAccess::ON_ERROR_EXIT: //doesn't make much sense for "batch gui"-mode
        showPopups = true;
        break;

    case xmlAccess::ON_ERROR_IGNORE:
        showPopups = false;
        break;
    }


    syncStatusFrame = new SyncStatus(this, NULL);
    syncStatusFrame->Show();

    //notify about (logical) application main window
    FreeFileSync::AppMainWindow::setMainWindow(syncStatusFrame);
}


BatchStatusHandlerGui::~BatchStatusHandlerGui()
{
    //print the results list
    wxString finalMessage;
    if (errorLog.messageCount() > 0)
    {
        if (errorLog.errorsTotal() > 0)
        {
            wxString header(_("Warning: Synchronization failed for %x item(s):"));
            header.Replace(wxT("%x"), globalFunctions::numberToWxString(errorLog.errorsTotal()), false);
            finalMessage += header + wxT("\n\n");
        }

        const std::vector<wxString>& messages = errorLog.getFormattedMessages();
        for (std::vector<wxString>::const_iterator i = messages.begin(); i != messages.end(); ++i)
        {
            finalMessage += *i;
            finalMessage += wxChar('\n');
        }

        finalMessage += wxT("\n");
    }

    if (!finalInfo.IsEmpty())
        finalMessage += finalInfo + wxT("\n\n");

    //notify to syncStatusFrame that current process has ended
    if (abortIsRequested())
    {
        returnValue = -4;
        finalMessage += _("Synchronization aborted!");
        syncStatusFrame->processHasFinished(SyncStatus::ABORTED, finalMessage);  //enable okay and close events
    }
    else if (errorLog.errorsTotal())
    {
        returnValue = -5;
        finalMessage += _("Synchronization completed with errors!");
        syncStatusFrame->processHasFinished(SyncStatus::FINISHED_WITH_ERROR, finalMessage);
    }
    else
    {
        finalMessage += _("Synchronization completed successfully!");
        syncStatusFrame->processHasFinished(SyncStatus::FINISHED_WITH_SUCCESS, finalMessage);
    }
}


inline
void BatchStatusHandlerGui::updateStatusText(const Zstring& text)
{
    syncStatusFrame->setStatusText_NoUpdate(text);
}


void BatchStatusHandlerGui::initNewProcess(int objectsTotal, wxLongLong dataTotal, StatusHandler::Process processID)
{
    currentProcess = processID;

    switch (currentProcess)
    {
    case StatusHandler::PROCESS_SCANNING:
        syncStatusFrame->resetGauge(0, 0); //dummy call to initialize some gui elements (remaining time, speed)
        syncStatusFrame->setCurrentStatus(SyncStatus::SCANNING);
        break;
    case StatusHandler::PROCESS_COMPARING_CONTENT:
        syncStatusFrame->resetGauge(objectsTotal, dataTotal);
        syncStatusFrame->setCurrentStatus(SyncStatus::COMPARING_CONTENT);
        break;
    case StatusHandler::PROCESS_SYNCHRONIZING:
        syncStatusFrame->resetGauge(objectsTotal, dataTotal);
        syncStatusFrame->setCurrentStatus(SyncStatus::SYNCHRONIZING);
        break;
    case StatusHandler::PROCESS_NONE:
        assert(false);
        break;
    }
}


inline
void BatchStatusHandlerGui::updateProcessedData(int objectsProcessed, wxLongLong dataProcessed)
{
    switch (currentProcess)
    {
    case StatusHandler::PROCESS_SCANNING:
        break;
    case StatusHandler::PROCESS_COMPARING_CONTENT:
    case StatusHandler::PROCESS_SYNCHRONIZING:
        syncStatusFrame->incProgressIndicator_NoUpdate(objectsProcessed, dataProcessed);
        break;
    case StatusHandler::PROCESS_NONE:
        assert(false);
        break;
    }
}


ErrorHandler::Response BatchStatusHandlerGui::reportError(const wxString& errorMessage)
{
    if (showPopups)
    {
        syncStatusFrame->updateStatusDialogNow();

        bool ignoreNextErrors = false;
        ErrorDlg* errorDlg = new ErrorDlg(syncStatusFrame,
                                          ErrorDlg::BUTTON_IGNORE |  ErrorDlg::BUTTON_RETRY | ErrorDlg::BUTTON_ABORT,
                                          errorMessage + wxT("\n\n\n") + _("Ignore this error, retry or abort?"),
                                          ignoreNextErrors);
        switch (static_cast<ErrorDlg::ReturnCodes>(errorDlg->ShowModal()))
        {
        case ErrorDlg::BUTTON_IGNORE:
            showPopups = !ignoreNextErrors;
            errorLog.logError(errorMessage);
            return ErrorHandler::IGNORE_ERROR;
        case ErrorDlg::BUTTON_RETRY:
            return ErrorHandler::RETRY;
        case ErrorDlg::BUTTON_ABORT:
            errorLog.logError(errorMessage);
            abortThisProcess();
        }
    }
    else
    {
        errorLog.logError(errorMessage);
        return ErrorHandler::IGNORE_ERROR;
    }

    assert(false);
    errorLog.logError(errorMessage);
    return ErrorHandler::IGNORE_ERROR; //dummy
}


void BatchStatusHandlerGui::reportFatalError(const wxString& errorMessage)
{
    errorLog.logError(errorMessage);
    abortThisProcess();
}


void BatchStatusHandlerGui::reportWarning(const wxString& warningMessage, bool& warningActive)
{
    errorLog.logWarning(warningMessage);

    if (!warningActive)
        return;

    if (showPopups)
    {
        syncStatusFrame->updateStatusDialogNow();

        //show popup and ask user how to handle warning
        bool dontWarnAgain = false;
        WarningDlg* warningDlg = new WarningDlg(NULL,
                                                WarningDlg::BUTTON_IGNORE | WarningDlg::BUTTON_ABORT,
                                                warningMessage,
                                                dontWarnAgain);
        const WarningDlg::Response rv = static_cast<WarningDlg::Response>(warningDlg->ShowModal());
        warningDlg->Destroy();
        switch (rv)
        {
        case WarningDlg::BUTTON_IGNORE: //no unhandled error situation!
            warningActive = !dontWarnAgain;
            break;

        case WarningDlg::BUTTON_ABORT:
            abortThisProcess();
            break;
        }
    }
}


inline
void BatchStatusHandlerGui::forceUiRefresh()
{
    if (currentProcess == StatusHandler::PROCESS_SCANNING)
        syncStatusFrame->m_gauge1->Pulse(); //expensive! So put it here!

    syncStatusFrame->updateStatusDialogNow();
}


void BatchStatusHandlerGui::abortThisProcess()
{
    requestAbortion();
    throw FreeFileSync::AbortThisProcess();  //abort can be triggered by syncStatusFrame
}


void BatchStatusHandlerGui::addFinalInfo(const wxString& infoMessage)
{
    finalInfo = infoMessage;
}
