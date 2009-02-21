/*******#include <wx/msgdlg.h>********************************************************
 * Name:      FreeFileSyncApp.cpp
 * Purpose:   Code for Application Class
 * Author:    ZenJu (zhnmju123@gmx.de)
 * Created:   2008-07-16
 * Copyright: ZenJu ()
 * License:
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

using namespace xmlAccess;


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
    programLanguage.setLanguage(globalSettings.global.programLanguage);
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
        XmlType xmlConfigType = xmlAccess::getXmlType(argv[1]);
        if (xmlConfigType == XML_GUI_CONFIG) //start in GUI mode (configuration file specified)
        {
            MainDialog* frame = new MainDialog(NULL, argv[1], &programLanguage, globalSettings);
            frame->SetIcon(*globalResource.programIcon); //set application icon
            frame->Show();
        }
        else if (xmlConfigType == XML_BATCH_CONFIG) //start in commandline mode
        {
            runBatchMode(argv[1], globalSettings);

            if (wxApp::GetTopWindow() == NULL) //if no windows are shown program won't exit automatically
                ExitMainLoop();
            return;
        }
        else
        {
            wxMessageBox(wxString(_("The file does not contain a valid configuration:")) + wxT(" \"") + argv[1] + wxT("\""), _("Error"), wxOK | wxICON_ERROR);
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
    globalSettings.global.programLanguage = programLanguage.getLanguage();

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
        {                                                           //"Date" is used at other places too
            wxString headerLine = wxString(wxT("FreeFileSync (")) + _("Date") + wxT(": ") + wxDateTime::Now().FormatDate() + wxT(" ") + _("Time:") + wxT(" ") +  wxDateTime::Now().FormatTime() + wxT(")");
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

        logFile.Write(logText + wxChar('\n'));
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
    XmlBatchConfig batchCfg;  //structure to receive gui settings
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

    //format directory names
    for (std::vector<FolderPair>::iterator i = batchCfg.directoryPairs.begin(); i != batchCfg.directoryPairs.end(); ++i)
    {
        i->leftDirectory  = FreeFileSync::getFormattedDirectoryName(i->leftDirectory);
        i->rightDirectory = FreeFileSync::getFormattedDirectoryName(i->rightDirectory);
    }

    //init logfile
    std::auto_ptr<LogFile> log; //delete log object on exit (if not NULL)
    if (batchCfg.silent)
    {
        log = std::auto_ptr<LogFile>(new LogFile);
        if (!log->isOkay())
        { //handle error: file load
            wxMessageBox(_("Unable to create logfile!"), _("Error"), wxOK | wxICON_ERROR);
            return;
        }
    }

    //begin of synchronization process (all in one try-catch block)
    try
    {
        //class handling status updates and error messages
        std::auto_ptr<BatchStatusHandler> statusHandler;  //delete object automatically
        if (batchCfg.silent)
            statusHandler = std::auto_ptr<BatchStatusHandler>(new BatchStatusHandlerSilent(batchCfg.mainCfg.ignoreErrors, log.get(), returnValue));
        else
            statusHandler = std::auto_ptr<BatchStatusHandler>(new BatchStatusHandlerGui(batchCfg.mainCfg.ignoreErrors, returnValue));

        //test existence of Recycle Bin
        if (batchCfg.mainCfg.useRecycleBin)
        {
            if (!FreeFileSync::recycleBinExists())
                statusHandler->exitAndSetStatus(_("Unable to initialize Recycle Bin!"), BatchStatusHandler::ABORTED);
        }

        //check if directories are valid
        wxString errorMessage;
        if (!FreeFileSync::foldersAreValidForComparison(batchCfg.directoryPairs, errorMessage))
            statusHandler->exitAndSetStatus(errorMessage, BatchStatusHandler::ABORTED);


        //check if folders have dependencies
        if (globalSettings.global.folderDependCheckActive)
        {
            wxString warningMessage;
            if (FreeFileSync::foldersHaveDependencies(batchCfg.directoryPairs, warningMessage))
            {
                //abort if in silent mode
                if (batchCfg.silent)
                    statusHandler->exitAndSetStatus(warningMessage, BatchStatusHandler::ABORTED);

                //non silent mode: offer possibility to ignore issue
                bool hideThisDialog = false;
                wxString messageText = warningMessage + wxT("\n\n") +
                                       _("Consider this when setting up synchronization rules: You might want to avoid write access to these directories so that synchronization of both does not interfere.");

                //show popup and ask user how to handle warning
                WarningDlg* warningDlg = new WarningDlg(NULL, WarningDlg::BUTTON_IGNORE | WarningDlg::BUTTON_ABORT, messageText, hideThisDialog);
                const int rv = warningDlg->ShowModal();
                warningDlg->Destroy();
                if (rv == WarningDlg::BUTTON_ABORT)
                    statusHandler->exitAndSetStatus(warningMessage, BatchStatusHandler::ABORTED);
                else
                    globalSettings.global.folderDependCheckActive = !hideThisDialog;
            }
        }


        //COMPARE DIRECTORIES
        FileCompareResult currentGridData;
        bool lineBreakOnMessages = !batchCfg.silent;

        #ifdef FFS_WIN
        FreeFileSync::CompareProcess comparison(lineBreakOnMessages, globalSettings.global.handleDstOnFat32, statusHandler.get());
        #elif defined FFS_LINUX
        FreeFileSync::CompareProcess comparison(lineBreakOnMessages, false, statusHandler.get());
        #endif
        comparison.startCompareProcess(batchCfg.directoryPairs,
                                       batchCfg.mainCfg.compareVar,
                                       currentGridData);

        //APPLY FILTERS
        if (batchCfg.mainCfg.filterIsActive)
            FreeFileSync::filterCurrentGridData(currentGridData, batchCfg.mainCfg.includeFilter, batchCfg.mainCfg.excludeFilter);

        //check if there are files/folders to be sync'ed at all
        if (!synchronizationNeeded(currentGridData, batchCfg.mainCfg.syncConfiguration))
            statusHandler->exitAndSetStatus(_("Nothing to synchronize according to configuration!"), BatchStatusHandler::FINISHED); //inform about this special case

        //START SYNCHRONIZATION
        FreeFileSync::SyncProcess synchronization(batchCfg.mainCfg.useRecycleBin, lineBreakOnMessages, statusHandler.get());
        synchronization.startSynchronizationProcess(currentGridData, batchCfg.mainCfg.syncConfiguration);
    }
    catch (AbortThisProcess&)  //exit used by statusHandler
    {   //don't set returnValue here! Program flow may arrive here in non-error situations also! E.g. "nothing to synchronize"
        return;
    }

    return; //exit program
}


//######################################################################################################
BatchStatusHandlerSilent::BatchStatusHandlerSilent(bool ignoreAllErrors, LogFile* log, int& returnVal) :
        ignoreErrors(ignoreAllErrors),
        currentProcess(StatusHandler::PROCESS_NONE),
        returnValue(returnVal),
        m_log(log) {}


BatchStatusHandlerSilent::~BatchStatusHandlerSilent()
{
    unsigned int failedItems = unhandledErrors.GetCount();
    if (failedItems)
        returnValue = -5;

    //write result
    if (abortRequested)
        m_log->write(_("Synchronization aborted!"), _("Error"));
    else if (failedItems)
        m_log->write(_("Synchronization completed with errors!"), _("Error"));
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


ErrorHandler::Response BatchStatusHandlerSilent::reportError(const Zstring& text)
{
    if (ignoreErrors)
    {
        unhandledErrors.Add(text.c_str());
        m_log->write(text.c_str(), _("Error"));
        return ErrorHandler::IGNORE_ERROR;
    }
    else
    {
        unhandledErrors.Add(text.c_str());
        m_log->write(text.c_str(), _("Error"));
        abortRequested = true;
        throw AbortThisProcess();
    }
}


void BatchStatusHandlerSilent::abortThisProcess() //not used in this context!
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
        break;

    case BatchStatusHandler::FINISHED:
        m_log->write(message, _("Info"));
        throw AbortThisProcess();
        break;
    default:
        assert(false);
    }
}


//######################################################################################################
BatchStatusHandlerGui::BatchStatusHandlerGui(bool ignoreAllErrors, int& returnVal) :
        ignoreErrors(ignoreAllErrors),
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
        returnValue = -5;

        finalMessage = wxString(_("Warning: Synchronization failed for %x item(s):")) + wxT("\n\n");
        finalMessage.Replace(wxT("%x"), globalFunctions::numberToWxString(failedItems), false);

        for (unsigned int j = 0; j < failedItems; ++j)
            finalMessage+= unhandledErrors[j] + wxT("\n");
        finalMessage+= wxT("\n");
    }

    if (!additionalStatusInfo.IsEmpty())
        finalMessage += additionalStatusInfo + wxT("\n\n");

    //notify to syncStatusFrame that current process has ended
    if (abortRequested)
    {
        finalMessage+= _("Synchronization aborted!");
        syncStatusFrame->setStatusText_NoUpdate(finalMessage.c_str());
        syncStatusFrame->processHasFinished(SyncStatus::ABORTED);  //enable okay and close events
    }
    else if (failedItems)
    {
        finalMessage+= _("Synchronization completed with errors!");
        syncStatusFrame->setStatusText_NoUpdate(finalMessage.c_str());
        syncStatusFrame->processHasFinished(SyncStatus::FINISHED_WITH_ERROR);
    }
    else
    {
        finalMessage+= _("Synchronization completed successfully!");
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


ErrorHandler::Response BatchStatusHandlerGui::reportError(const Zstring& text)
{
    if (ignoreErrors) //this option can be set from commandline or by the user in the error dialog on UI
    {
        unhandledErrors.Add(text.c_str());
        return ErrorHandler::IGNORE_ERROR;
    }
    else
    {
        syncStatusFrame->updateStatusDialogNow();

        bool ignoreNextErrors = false;
        wxString errorMessage = wxString(text) + wxT("\n\n") + _("Ignore this error, retry or abort?");
        ErrorDlg* errorDlg = new ErrorDlg(syncStatusFrame, errorMessage, ignoreNextErrors);

        switch (errorDlg->ShowModal())
        {
        case ErrorDlg::BUTTON_IGNORE:
            ignoreErrors = ignoreNextErrors;
            unhandledErrors.Add(text.c_str());
            return ErrorHandler::IGNORE_ERROR;
        case ErrorDlg::BUTTON_RETRY:
            return ErrorHandler::RETRY;
        case ErrorDlg::BUTTON_ABORT:
        {
            unhandledErrors.Add(text.c_str());
            abortRequested = true;
            throw AbortThisProcess();
        }
        default:
            assert (false);
            return ErrorHandler::IGNORE_ERROR;
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
