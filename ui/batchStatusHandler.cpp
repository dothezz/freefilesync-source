#include "batchStatusHandler.h"
#include "smallDialogs.h"
#include <wx/stopwatch.h>
#include <wx/taskbar.h>
#include "../algorithm.h"
#include <wx/ffile.h>
#include <wx/msgdlg.h>
#include "../library/globalFunctions.h"


class LogFile
{
public:
    LogFile(const wxString& logfileDirectory)
    {
        //create logfile directory
        const Zstring logfileDir = logfileDirectory.empty() ? wxT("Logs") : logfileDirectory.c_str();
        if (!wxDirExists(logfileDir))
            try
            {
                FreeFileSync::createDirectory(logfileDir, Zstring(), false);
            }
            catch (FileError&)
            {
                readyToWrite = false;
                return;
            }

        //assemble logfile name
        wxString logfileName = logfileDir.c_str();
        if (!FreeFileSync::endsWithPathSeparator(logfileName.c_str()))
            logfileName += GlobalResources::FILE_NAME_SEPARATOR;
        wxString timeNow = wxDateTime::Now().FormatISOTime();
        timeNow.Replace(wxT(":"), wxEmptyString);
        logfileName += wxT("FFS_") + wxDateTime::Now().FormatISODate() + wxChar('_') + timeNow + wxT(".log");


        logFile.Open(logfileName.c_str(), wxT("w"));
        readyToWrite = logFile.IsOpened();
        if (readyToWrite)
        {
            wxString headerLine = wxString(wxT("FreeFileSync - "))  +
                                  _("Batch execution") + wxT(" (") +
                                  _("Date") + wxT(": ") + wxDateTime::Now().FormatDate() + wxT(" ") + //"Date" is used at other places too
                                  _("Time") + wxT(":") + wxT(" ") +  wxDateTime::Now().FormatTime() + wxT(")");
            logFile.Write(headerLine + wxChar('\n'));
            logFile.Write(wxString().Pad(headerLine.Len(), wxChar('-')) + wxChar('\n') + wxChar('\n'));

            wxString caption = _("Log-messages:");
            logFile.Write(caption + wxChar('\n'));
            logFile.Write(wxString().Pad(caption.Len(), wxChar('-')) + wxChar('\n'));

            write(_("Start"));           //attention: write() replaces '\n'-characters
            logFile.Write(wxChar('\n')); //

            totalTime.Start(); //measure total time
        }
    }

    ~LogFile()
    {
        if (readyToWrite)
            close();
    }

    bool isOkay()
    {
        return readyToWrite;
    }

    void write(const wxString& logText, const wxString& problemType = wxEmptyString)
    {
        logFile.Write(wxString(wxT("[")) + wxDateTime::Now().FormatTime() + wxT("] "));

        if (problemType != wxEmptyString)
            logFile.Write(problemType + wxT(": "));

        //remove linebreaks
        wxString formattedText = logText;
        for (wxString::iterator i = formattedText.begin(); i != formattedText.end(); ++i)
            if (*i == wxChar('\n'))
                *i = wxChar(' ');

        logFile.Write(formattedText + wxChar('\n'));
    }

private:

    void close()
    {
        logFile.Write(wxChar('\n'));

        long time = totalTime.Time(); //retrieve total time

        logFile.Write(wxString(wxT("[")) + wxDateTime::Now().FormatTime() + wxT("] "));
        logFile.Write(wxString(_("Stop")) + wxT(" (") + _("Total time:") + wxT(" ") + (wxTimeSpan::Milliseconds(time)).Format() + wxT(")"));

        //logFile.close(); <- not needed
    }

    bool readyToWrite;
    wxFFile logFile;
    wxStopWatch totalTime;
};


class FfsTrayIcon : public wxTaskBarIcon
{
public:
    FfsTrayIcon(StatusHandler* statusHandler) :
            m_statusHandler(statusHandler),
            processPaused(false)
    {
        running.reset(new wxIcon(*globalResource.programIcon));
        paused.reset(new wxIcon);
        paused->CopyFromBitmap(*globalResource.bitmapFFSPaused);

        wxTaskBarIcon::SetIcon(*running);
    }

    ~FfsTrayIcon() {}

    enum
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
        contextMenu->Append(CONTEXT_ABORT, _("&Abort"));
        contextMenu->AppendSeparator();
        contextMenu->Append(CONTEXT_ABOUT, _("&About..."));
        //event handling
        contextMenu->Connect(wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(FfsTrayIcon::onContextMenuSelection), NULL, this);

        return contextMenu; //ownership transferred to library
    }

    void onContextMenuSelection(wxCommandEvent& event)
    {
        int eventId = event.GetId();
        if (eventId == CONTEXT_PAUSE)
        {
            processPaused = !processPaused;
            if (processPaused)
                wxTaskBarIcon::SetIcon(*paused);
            else
                wxTaskBarIcon::SetIcon(*running);
        }
        else if (eventId == CONTEXT_ABORT)
        {
            processPaused = false;
            wxTaskBarIcon::SetIcon(*running);
            m_statusHandler->requestAbortion();
        }
        else if (eventId == CONTEXT_ABOUT)
        {
            AboutDlg* aboutDlg = new AboutDlg(NULL);
            aboutDlg->ShowModal();
            aboutDlg->Destroy();
        }
    }

    void updateSysTray()
    {
        updateUiNow();

        //support for pause button
        while (processPaused)
        {
            wxMilliSleep(UI_UPDATE_INTERVAL);
            updateUiNow();
        }
    }

private:
    StatusHandler* m_statusHandler;
    bool processPaused;
    std::auto_ptr<wxIcon> running;
    std::auto_ptr<wxIcon> paused;
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
    {  //handle error: file load
        wxMessageBox(_("Unable to create logfile!"), _("Error"), wxOK | wxICON_ERROR);
        throw FreeFileSync::AbortThisProcess();
    }
}


BatchStatusHandlerSilent::~BatchStatusHandlerSilent()
{
    unsigned int failedItems = unhandledErrors.GetCount();

    //write result
    if (abortRequested)
    {
        returnValue = -4;
        m_log->write(_("Synchronization aborted!"), _("Error"));
    }
    else if (failedItems)
    {
        returnValue = -5;
        m_log->write(_("Synchronization completed with errors!"), _("Info"));
    }
    else
        m_log->write(_("Synchronization completed successfully!"), _("Info"));
}


inline
void BatchStatusHandlerSilent::updateStatusText(const Zstring& text)
{
    if (currentProcess == StatusHandler::PROCESS_SYNCHRONIZING)
        m_log->write(text.c_str(), _("Info"));
}


inline
void BatchStatusHandlerSilent::initNewProcess(int objectsTotal, wxLongLong dataTotal, StatusHandler::Process processID)
{
    currentProcess = processID;
}


ErrorHandler::Response BatchStatusHandlerSilent::reportError(const Zstring& errorMessage)
{
    switch (m_handleError)
    {
    case xmlAccess::ON_ERROR_POPUP:
    {
        bool ignoreNextErrors = false;
        ErrorDlg* errorDlg = new ErrorDlg(NULL,
                                          ErrorDlg::BUTTON_IGNORE |  ErrorDlg::BUTTON_RETRY | ErrorDlg::BUTTON_ABORT,
                                          wxString(errorMessage) + wxT("\n\n") + _("Ignore this error, retry or abort?"),
                                          ignoreNextErrors);
        const int rv = errorDlg->ShowModal();
        errorDlg->Destroy();
        switch (rv)
        {
        case ErrorDlg::BUTTON_IGNORE:
            if (ignoreNextErrors) //falsify only
                m_handleError = xmlAccess::ON_ERROR_IGNORE;
            unhandledErrors.Add(errorMessage.c_str());
            m_log->write(errorMessage.c_str(), _("Error"));
            return ErrorHandler::IGNORE_ERROR;

        case ErrorDlg::BUTTON_RETRY:
            return ErrorHandler::RETRY;

        case ErrorDlg::BUTTON_ABORT:
            unhandledErrors.Add(errorMessage.c_str());
            m_log->write(errorMessage.c_str(), _("Error"));
            abortThisProcess();
        }
    }
    break; //used if last switch didn't find a match

    case xmlAccess::ON_ERROR_EXIT: //abort
        unhandledErrors.Add(errorMessage.c_str());
        m_log->write(errorMessage.c_str(), _("Error"));
        abortThisProcess();

    case xmlAccess::ON_ERROR_IGNORE:
        unhandledErrors.Add(errorMessage.c_str());
        m_log->write(errorMessage.c_str(), _("Error"));
        return ErrorHandler::IGNORE_ERROR;
    }

    assert(false);
    return ErrorHandler::IGNORE_ERROR; //dummy value
}


void BatchStatusHandlerSilent::reportFatalError(const Zstring& errorMessage)
{
    switch (m_handleError)
    {
    case xmlAccess::ON_ERROR_POPUP:
    {
        bool dummy = false;
        ErrorDlg* errorDlg = new ErrorDlg(NULL,
                                          ErrorDlg::BUTTON_ABORT,
                                          errorMessage.c_str(), dummy);
        errorDlg->ShowModal();
        errorDlg->Destroy();
    }
    break;

    case xmlAccess::ON_ERROR_EXIT:
        break;

    case xmlAccess::ON_ERROR_IGNORE:
        break;
    }

    unhandledErrors.Add(errorMessage.c_str());
    m_log->write(errorMessage.c_str(), _("Error"));
    abortThisProcess();
}


void BatchStatusHandlerSilent::reportWarning(const Zstring& warningMessage, bool& dontShowAgain)
{
    switch (m_handleError)
    {
    case xmlAccess::ON_ERROR_POPUP:
    {
        //show popup and ask user how to handle warning
        bool dontWarnAgain = false;
        WarningDlg* warningDlg = new WarningDlg(NULL,
                                                WarningDlg::BUTTON_IGNORE | WarningDlg::BUTTON_ABORT,
                                                warningMessage.c_str(),
                                                dontWarnAgain);
        const int rv = warningDlg->ShowModal();
        warningDlg->Destroy();
        switch (rv)
        {
        case WarningDlg::BUTTON_ABORT:
            unhandledErrors.Add(warningMessage.c_str());
            m_log->write(warningMessage.c_str(), _("Warning"));
            abortThisProcess();
        case WarningDlg::BUTTON_IGNORE: //no unhandled error situation!
            dontShowAgain = dontWarnAgain;
            m_log->write(warningMessage.c_str(), _("Warning"));
            return;
        }
    }
    break; //keep it! last switch might not find match

    case xmlAccess::ON_ERROR_EXIT: //abort
        unhandledErrors.Add(warningMessage.c_str());
        m_log->write(warningMessage.c_str(), _("Warning"));
        abortThisProcess();

    case xmlAccess::ON_ERROR_IGNORE: //no unhandled error situation!
        m_log->write(warningMessage.c_str(), _("Warning"));
        return;
    }

    assert(false);
}


void BatchStatusHandlerSilent::addFinalInfo(const Zstring& infoMessage)
{
        m_log->write(infoMessage.c_str(), _("Info"));
}


void BatchStatusHandlerSilent::forceUiRefresh()
{
    trayIcon->updateSysTray(); //needed by sys-tray icon only
}


void BatchStatusHandlerSilent::abortThisProcess() //used by sys-tray menu
{
    abortRequested = true;
    throw FreeFileSync::AbortThisProcess();
}

//######################################################################################################


BatchStatusHandlerGui::BatchStatusHandlerGui(const xmlAccess::OnError handleError, int& returnVal) :
        m_handleError(handleError),
        currentProcess(StatusHandler::PROCESS_NONE),
        returnValue(returnVal)
{
    syncStatusFrame = new SyncStatus(this, NULL);
    syncStatusFrame->Show();
}


BatchStatusHandlerGui::~BatchStatusHandlerGui()
{
    //display result
    wxString finalMessage;

    unsigned int failedItems = unhandledErrors.GetCount();
    if (failedItems)
    {
        finalMessage = wxString(_("Warning: Synchronization failed for %x item(s):")) + wxT("\n\n");
        finalMessage.Replace(wxT("%x"), globalFunctions::numberToWxString(failedItems), false);

        for (unsigned int j = 0; j < failedItems; ++j)
        {   //remove linebreaks
            wxString errorMessage = unhandledErrors[j];
            for (wxString::iterator i = errorMessage.begin(); i != errorMessage.end(); ++i)
                if (*i == wxChar('\n'))
                    *i = wxChar(' ');

            finalMessage += errorMessage + wxT("\n");
        }
        finalMessage += wxT("\n");
    }

    if (!finalInfo.IsEmpty())
        finalMessage += finalInfo + wxT("\n\n");

    //notify to syncStatusFrame that current process has ended
    if (abortRequested)
    {
        returnValue = -4;
        finalMessage += _("Synchronization aborted!");
        syncStatusFrame->setStatusText_NoUpdate(finalMessage.c_str());
        syncStatusFrame->processHasFinished(SyncStatus::ABORTED);  //enable okay and close events
    }
    else if (failedItems)
    {
        returnValue = -5;
        finalMessage += _("Synchronization completed with errors!");
        syncStatusFrame->setStatusText_NoUpdate(finalMessage.c_str());
        syncStatusFrame->processHasFinished(SyncStatus::FINISHED_WITH_ERROR);
    }
    else
    {
        finalMessage += _("Synchronization completed successfully!");
        syncStatusFrame->setStatusText_NoUpdate(finalMessage.c_str());
        syncStatusFrame->processHasFinished(SyncStatus::FINISHED_WITH_SUCCESS);
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

    if (currentProcess == StatusHandler::PROCESS_SCANNING)
        syncStatusFrame->setCurrentStatus(SyncStatus::SCANNING);

    else if (currentProcess == StatusHandler::PROCESS_COMPARING_CONTENT)
    {
        syncStatusFrame->resetGauge(objectsTotal, dataTotal);
        syncStatusFrame->setCurrentStatus(SyncStatus::COMPARING);
    }

    else if (currentProcess == StatusHandler::PROCESS_SYNCHRONIZING)
    {
        syncStatusFrame->resetGauge(objectsTotal, dataTotal);
        syncStatusFrame->setCurrentStatus(SyncStatus::SYNCHRONIZING);
    }
    else assert(false);
}


inline
void BatchStatusHandlerGui::updateProcessedData(int objectsProcessed, wxLongLong dataProcessed)
{
    if (currentProcess == StatusHandler::PROCESS_SCANNING)
        ;
    else if (currentProcess == StatusHandler::PROCESS_COMPARING_CONTENT)
        syncStatusFrame->incProgressIndicator_NoUpdate(objectsProcessed, dataProcessed);
    else if (currentProcess == StatusHandler::PROCESS_SYNCHRONIZING)
        syncStatusFrame->incProgressIndicator_NoUpdate(objectsProcessed, dataProcessed);
    else assert(false);
}


ErrorHandler::Response BatchStatusHandlerGui::reportError(const Zstring& errorMessage)
{
    //add current time before error message
    wxString errorWithTime = wxString(wxT("[")) + wxDateTime::Now().FormatTime() + wxT("] ") + errorMessage.c_str();

    switch (m_handleError)
    {
    case xmlAccess::ON_ERROR_POPUP:
    {
        syncStatusFrame->updateStatusDialogNow();

        bool ignoreNextErrors = false;
        ErrorDlg* errorDlg = new ErrorDlg(syncStatusFrame,
                                          ErrorDlg::BUTTON_IGNORE |  ErrorDlg::BUTTON_RETRY | ErrorDlg::BUTTON_ABORT,
                                          wxString(errorMessage) + wxT("\n\n") + _("Ignore this error, retry or abort?"),
                                          ignoreNextErrors);
        switch (errorDlg->ShowModal())
        {
        case ErrorDlg::BUTTON_IGNORE:
            if (ignoreNextErrors) //falsify only
                m_handleError = xmlAccess::ON_ERROR_IGNORE;
            unhandledErrors.Add(errorWithTime);
            return ErrorHandler::IGNORE_ERROR;
        case ErrorDlg::BUTTON_RETRY:
            return ErrorHandler::RETRY;
        case ErrorDlg::BUTTON_ABORT:
            unhandledErrors.Add(errorWithTime);
            abortThisProcess();
        }
    }
    break; //used IF last switch didn't find a match

    case xmlAccess::ON_ERROR_EXIT: //abort
        unhandledErrors.Add(errorWithTime);
        abortThisProcess();

    case xmlAccess::ON_ERROR_IGNORE:
        unhandledErrors.Add(errorWithTime);
        return ErrorHandler::IGNORE_ERROR;
    }

    assert(false);
    return ErrorHandler::IGNORE_ERROR; //dummy value
}


void BatchStatusHandlerGui::reportFatalError(const Zstring& errorMessage)
{   //add current time before error message
    wxString errorWithTime = wxString(wxT("[")) + wxDateTime::Now().FormatTime() + wxT("] ") + errorMessage.c_str();

    unhandledErrors.Add(errorWithTime);
    abortThisProcess();
}


void BatchStatusHandlerGui::reportWarning(const Zstring& warningMessage, bool& dontShowAgain)
{   //add current time before warning message
    wxString warningWithTime = wxString(wxT("[")) + wxDateTime::Now().FormatTime() + wxT("] ") + warningMessage.c_str();

    switch (m_handleError)
    {
    case xmlAccess::ON_ERROR_POPUP:
    case xmlAccess::ON_ERROR_EXIT: //show popup in this case also
    {
        //show popup and ask user how to handle warning
        bool dontWarnAgain = false;
        WarningDlg* warningDlg = new WarningDlg(NULL,
                                                WarningDlg::BUTTON_IGNORE | WarningDlg::BUTTON_ABORT,
                                                warningMessage.c_str(),
                                                dontWarnAgain);
        const int rv = warningDlg->ShowModal();
        warningDlg->Destroy();
        switch (rv)
        {
        case WarningDlg::BUTTON_IGNORE: //no unhandled error situation!
            dontShowAgain = dontWarnAgain;
            return;
        case WarningDlg::BUTTON_ABORT:
            unhandledErrors.Add(warningWithTime);
            abortThisProcess();
        }
    }
    break; //keep it! last switch might not find match

    case xmlAccess::ON_ERROR_IGNORE: //no unhandled error situation!
        return;
    }

    assert(false);
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
    abortRequested = true;
    throw FreeFileSync::AbortThisProcess();  //abort can be triggered by syncStatusFrame
}


void BatchStatusHandlerGui::addFinalInfo(const Zstring& infoMessage)
{
        finalInfo = infoMessage.c_str();
}
