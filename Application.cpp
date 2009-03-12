/*******#include <wx/msgdlg.h>********************************************************
 * Purpose:   Code for Application Class
 * Author:    ZenJu (zhnmju123@gmx.de)
 * Created:   2008-07-16
 * Copyright: ZenJu ()
 **************************************************************/

#include "application.h"
#include "ui/mainDialog.h"
#include <wx/stdpaths.h>
#include <wx/filename.h>
#include <wx/ffile.h>
#include "library/globalFunctions.h"
#include <wx/msgdlg.h>
#include "library/processXml.h"
#include <wx/stopwatch.h>
#include "comparison.h"
#include "synchronization.h"
#include "algorithm.h"
#include <wx/taskbar.h>
#include "ui/smallDialogs.h"


IMPLEMENT_APP(Application);

bool Application::ProcessIdle()
{
    static bool initialized = false;
    if (!initialized)
    {
        initialized = true;
        initialize();   //here the program initialization takes place
    }
    return wxApp::ProcessIdle();
}

//Note: initialization is done in the FIRST idle event instead of OnInit. Reason: Commandline mode requires the wxApp eventhandler to be established
//for UI update events. This is not the case at the time of OnInit().

bool Application::OnInit()
{
    returnValue = 0;
    //do not call wxApp::OnInit() to avoid using default commandline parser

    //set working directory to current executable directory
    const wxString workingDir = wxFileName(wxStandardPaths::Get().GetExecutablePath()).GetPath();
    if (!wxSetWorkingDirectory(workingDir))
    {   //show messagebox and quit program immediately
        wxMessageBox(wxString(_("Could not set working directory:")) + wxT(" ") + workingDir, _("An exception occured!"), wxOK | wxICON_ERROR);
        return false;
    }

    try //load global settings from XML: must be called AFTER working dir was set
    {
        globalSettings = xmlAccess::readGlobalSettings();
    }
    catch (const FileError& error)
    {
        if (wxFileExists(FreeFileSync::GLOBAL_CONFIG_FILE))
        {   //show messagebox and quit program immediately
            wxMessageBox(error.show().c_str(), _("Error"), wxOK | wxICON_ERROR);
            return false;
        }
        //else: globalSettings already has default values
    }

    //set program language: needs to happen after working directory has been set!
    SetExitOnFrameDelete(false); //prevent error messagebox from becoming top-level window
    programLanguage.setLanguage(globalSettings.shared.programLanguage);
    SetExitOnFrameDelete(true);

    //load image resources from file: must be called after working directory has been set
    globalResource.load();

    return true;
}


void Application::initialize()
{
    //test if FFS is to be started on UI with config file passed as commandline parameter
    if (argc > 1)
    {
        //load file specified by %1 parameter:
        wxString filename;
        if (wxFileExists(argv[1]))
            filename = argv[1];
        else if (wxFileExists(wxString(argv[1]) + wxT(".ffs_batch")))
            filename = wxString(argv[1]) + wxT(".ffs_batch");
        else if (wxFileExists(wxString(argv[1]) + wxT(".ffs_gui")))
            filename = wxString(argv[1]) + wxT(".ffs_gui");
        else
        {
            wxMessageBox(wxString(_("The file does not exist:")) + wxT(" \"") + argv[1] + wxT("\""), _("Error"), wxOK | wxICON_ERROR);
            return;
        }

        xmlAccess::XmlType xmlConfigType = xmlAccess::getXmlType(filename);
        if (xmlConfigType == xmlAccess::XML_GUI_CONFIG) //start in GUI mode (configuration file specified)
        {
            MainDialog* frame = new MainDialog(NULL, filename, &programLanguage, globalSettings);
            frame->SetIcon(*globalResource.programIcon); //set application icon
            frame->Show();
        }
        else if (xmlConfigType == xmlAccess::XML_BATCH_CONFIG) //start in commandline mode
        {
            runBatchMode(filename, globalSettings);

            if (wxApp::GetTopWindow() == NULL) //if no windows are shown program won't exit automatically
                ExitMainLoop();
            return;
        }
        else
        {
            wxMessageBox(wxString(_("The file does not contain a valid configuration:")) + wxT(" \"") + filename + wxT("\""), _("Error"), wxOK | wxICON_ERROR);
            return;
        }
    }
    else //start in GUI mode (standard)
    {
        MainDialog* frame = new MainDialog(NULL, FreeFileSync::LAST_CONFIG_FILE, &programLanguage, globalSettings);
        frame->SetIcon(*globalResource.programIcon); //set application icon
        frame->Show();
    }
}


bool Application::OnExceptionInMainLoop()
{
    throw; //just re-throw exception and avoid display of additional exception messagebox: it will be caught in OnRun()
}


int Application::OnRun()
{
    try
    {
        wxApp::OnRun();
    }
    catch (const RuntimeException& theException) //catch runtime errors during main event loop; wxApp::OnUnhandledException could be used alternatively
    {
        wxMessageBox(theException.show(), _("An exception occured!"), wxOK | wxICON_ERROR);
        return -1;
    }
    catch (std::exception& e) //catch all STL exceptions
    {
        wxMessageBox(wxString::From8BitData(e.what()), _("An exception occured!"), wxOK | wxICON_ERROR);
        return -1;
    }

    return returnValue;
}


int Application::OnExit()
{
    //get program language
    globalSettings.shared.programLanguage = programLanguage.getLanguage();

    try //save global settings to XML
    {
        xmlAccess::writeGlobalSettings(globalSettings);
    }
    catch (const FileError& error)
    {
        wxMessageBox(error.show().c_str(), _("Error"), wxOK | wxICON_ERROR);
    }

    return 0;
}


class LogFile
{
public:
    LogFile()
    {
        wxString tmp = wxDateTime::Now().FormatISOTime();
        tmp.Replace(wxT(":"), wxEmptyString);

        //create subfolder "log" to hold logfiles
        if (!wxDirExists(wxT("Logs")))
            wxMkdir(wxT("Logs"));
        wxString logfileName = wxString(wxT("Logs")) + GlobalResources::FILE_NAME_SEPARATOR + wxT("FFS_") + wxDateTime::Now().FormatISODate() + wxChar('_') + tmp + wxT(".log");

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

            write(wxString(_("Start")) + wxChar('\n'));

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


void Application::runBatchMode(const wxString& filename, xmlAccess::XmlGlobalSettings& globalSettings)
{
    //load XML settings
    xmlAccess::XmlBatchConfig batchCfg;  //structure to receive gui settings
    try
    {
        batchCfg = xmlAccess::readBatchConfig(filename);
    }
    catch (const FileError& error)
    {
        wxMessageBox(error.show().c_str(), _("Error"), wxOK | wxICON_ERROR);
        return;
    }
    //all settings have been read successfully...

    try //begin of synchronization process (all in one try-catch block)
    {
        //class handling status updates and error messages
        std::auto_ptr<BatchStatusHandler> statusHandler;  //delete object automatically
        if (batchCfg.silent)
            statusHandler = std::auto_ptr<BatchStatusHandler>(new BatchStatusHandlerSilent(batchCfg.handleError, returnValue));
        else
            statusHandler = std::auto_ptr<BatchStatusHandler>(new BatchStatusHandlerGui(batchCfg.handleError, returnValue));

        //COMPARE DIRECTORIES
        FileCompareResult currentGridData;
#ifdef FFS_WIN
        FreeFileSync::CompareProcess comparison(globalSettings.shared.traverseSymbolicLinks,
                                                globalSettings.shared.handleDstOnFat32,
                                                globalSettings.shared.warningDependentFolders,
                                                statusHandler.get());
#elif defined FFS_LINUX
        FreeFileSync::CompareProcess comparison(globalSettings.shared.traverseSymbolicLinks,
                                                false,
                                                globalSettings.shared.warningDependentFolders,
                                                statusHandler.get());
#endif
        comparison.startCompareProcess(batchCfg.directoryPairs,
                                       batchCfg.mainCfg.compareVar,
                                       currentGridData);

        //APPLY FILTERS
        if (batchCfg.mainCfg.filterIsActive)
            FreeFileSync::filterGridData(currentGridData, batchCfg.mainCfg.includeFilter, batchCfg.mainCfg.excludeFilter);

        //check if there are files/folders to be sync'ed at all
        if (!synchronizationNeeded(currentGridData, batchCfg.mainCfg.syncConfiguration))
            statusHandler->exitAndSetStatus(_("Nothing to synchronize according to configuration!"), BatchStatusHandler::FINISHED); //inform about this special case

        //START SYNCHRONIZATION
        FreeFileSync::SyncProcess synchronization(
            batchCfg.mainCfg.useRecycleBin,
            globalSettings.shared.warningSignificantDifference,
            statusHandler.get());

        synchronization.startSynchronizationProcess(currentGridData, batchCfg.mainCfg.syncConfiguration);
    }
    catch (AbortThisProcess&)  //exit used by statusHandler
    {   //don't set returnValue here! Program flow may arrive here in non-error situations also! E.g. "nothing to synchronize"
        return;
    }

    return; //exit program
}


//######################################################################################################

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


BatchStatusHandlerSilent::BatchStatusHandlerSilent(const xmlAccess::OnError handleError, int& returnVal) :
        m_handleError(handleError),
        currentProcess(StatusHandler::PROCESS_NONE),
        returnValue(returnVal),
        trayIcon(new FfsTrayIcon(this)),
        m_log(new LogFile)
{
    //test if log was instantiated successfully
    if (!m_log->isOkay())
    {  //handle error: file load
        wxMessageBox(_("Unable to create logfile!"), _("Error"), wxOK | wxICON_ERROR);
        throw AbortThisProcess();
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
void BatchStatusHandlerSilent::initNewProcess(int objectsTotal, double dataTotal, StatusHandler::Process processID)
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


void BatchStatusHandlerSilent::forceUiRefresh()
{
    trayIcon->updateSysTray(); //needed by sys-tray icon only
}


void BatchStatusHandlerSilent::abortThisProcess() //used by sys-tray menu
{
    abortRequested = true;
    throw AbortThisProcess();
}


void BatchStatusHandlerSilent::exitAndSetStatus(const wxString& message, ExitCode code) //abort externally
{
    switch (code)
    {
    case BatchStatusHandler::ABORTED:
        unhandledErrors.Add(message);
        m_log->write(message, _("Error"));
        abortRequested = true;
        throw AbortThisProcess();

    case BatchStatusHandler::FINISHED:
        m_log->write(message, _("Info"));
        throw AbortThisProcess();
    default:
        assert(false);
    }
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

    if (!additionalStatusInfo.IsEmpty())
        finalMessage += additionalStatusInfo + wxT("\n\n");

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


void BatchStatusHandlerGui::initNewProcess(int objectsTotal, double dataTotal, StatusHandler::Process processID)
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
void BatchStatusHandlerGui::updateProcessedData(int objectsProcessed, double dataProcessed)
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
    throw AbortThisProcess();  //abort can be triggered by syncStatusFrame
}


void BatchStatusHandlerGui::exitAndSetStatus(const wxString& message, ExitCode code) //abort externally
{
    switch (code)
    {
    case BatchStatusHandler::ABORTED:
        unhandledErrors.Add(message);
        abortRequested = true;
        throw AbortThisProcess();
        break;

    case BatchStatusHandler::FINISHED:
        additionalStatusInfo = message;
        throw AbortThisProcess();
        break;
    default:
        assert(false);
    }
}
