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

using namespace xmlAccess;


IMPLEMENT_APP(Application);

bool Application::ProcessIdle()
{
    static bool firstExecution = true;
    if (firstExecution)
    {
        firstExecution = false;
        initialize();   //here the program initialization takes place
    }
    return wxApp::ProcessIdle();
}

//Note: initialization is done in the FIRST idle event instead of OnInit. Reason: Commandline mode requires the wxApp eventhandler to be established
//for UI update events. This is not the case at the time of OnInit.

bool Application::OnInit()
{
    returnValue = 0;
    //do not call wxApp::OnInit() to avoid parsing commandline now, instead use own parser later
    return true;
}


void Application::initialize()
{
    //set working directory to current executable directory
    if (!wxSetWorkingDirectory(wxFileName(wxStandardPaths::Get().GetExecutablePath()).GetPath()))
        throw RuntimeException(_("Could not set working directory to directory containing executable file!"));

    globalResource.load(); //load image resources from file: must be called after working directory has been set

    //set program language
    SetExitOnFrameDelete(false);    //prevent messagebox from becoming top-level window
    programLanguage.loadLanguageFromCfg();
    SetExitOnFrameDelete(true);

    //test if ffs is to be started on UI with config file passed as commandline parameter
    wxString configFileForUI = FreeFileSync::FfsLastConfigFile;
    if (argc > 1)
    {
        XmlType xmlConfigType = xmlAccess::getXmlType(argv[1]);
        if (xmlConfigType == XML_GUI_CONFIG)
            configFileForUI = argv[1];
        else if (xmlConfigType == XML_BATCH_CONFIG) //start in commandline mode
        {
            runBatchMode(argv[1]);

            if (applicationRunsInBatchWithoutWindows)
                ExitMainLoop(); //exit programm on next main loop iteration
            return;             //program will exit automatically if a main window is present and closed
        }
        else
        {
            wxMessageBox(wxString(_("No valid configuration file specified: ")) + argv[1], _("Error"), wxOK | wxICON_ERROR);
            return;
        }
    }

    //start in GUI mode
    MainDialog* frame = new MainDialog(NULL, configFileForUI, &programLanguage);
    frame->SetIcon(*globalResource.programIcon); //set application icon
    frame->Show();
}


int Application::OnRun()
{
    try
    {
        wxApp::OnRun();
    }
    catch (const RuntimeException& theException)    //catch runtime errors during main event loop
    {
        wxMessageBox(theException.show(), _("An exception occured!"), wxOK | wxICON_ERROR);
        return -1;
    }
    return returnValue;
}


int Application::OnExit()
{
    return 0;
}


class LogFile
{
public:
    LogFile()
    {
        wxString tmp = wxDateTime::Now().FormatISOTime();
        tmp.Replace(wxT(":"), wxEmptyString);
        wxString logfileName = wxString(wxT("FFS_")) + wxDateTime::Now().FormatISODate() + wxChar('_') + tmp + wxT(".log");

        logFile.Open(logfileName.c_str(), wxT("w"));
        readyToWrite = logFile.IsOpened();
        if (readyToWrite)
        {
            logFile.Write(wxString(_("FreeFileSync   (Date: ")) + wxDateTime::Now().FormatDate() + _("  Time: ") +  wxDateTime::Now().FormatTime() + wxT(")") + wxChar('\n'));
            logFile.Write(wxString(_("-------------------------------------------------")) + wxChar('\n'));
            logFile.Write(wxChar('\n'));
            logFile.Write(_("Log-messages:\n-------------"));
            logFile.Write(wxChar('\n'));
            write(_("Start"));
            logFile.Write(wxChar('\n'));

            totalTime.Start(); //measure total time
        }
    }

    ~LogFile() {}

    bool isOkay()
    {
        return readyToWrite;
    }

    void write(const wxString& logText, const wxString& problemType = wxEmptyString)
    {
        if (readyToWrite)
        {
            logFile.Write(wxString(wxT("[")) + wxDateTime::Now().FormatTime() + wxT("] "));

            if (problemType != wxEmptyString)
                logFile.Write(problemType + wxT(": "));

            logFile.Write(logText + wxChar('\n'));
        }
    }

    void close(const wxString& finalText)
    {
        if (readyToWrite)
        {
            logFile.Write(wxChar('\n'));

            long time = totalTime.Time(); //retrieve total time
            write(finalText + wxT(" (") + _("Total time: ") + (wxTimeSpan::Milliseconds(time)).Format() + wxT(")"), _("Stop"));

            //logFile.close(); <- not needed
        }
    }

private:
    bool readyToWrite;
    wxFFile logFile;
    wxStopWatch totalTime;
};


class DeleteOnExit
{
public:
    DeleteOnExit(LogFile* log) : m_log(log) {}
    ~DeleteOnExit()
    {
        if (m_log) delete m_log;
    }

private:
    LogFile* m_log;
};


void Application::runBatchMode(const wxString& filename)
{
    applicationRunsInBatchWithoutWindows = false; //default value

    //load XML settings
    XmlInput inputFile(filename, XML_BATCH_CONFIG);

    if (!inputFile.loadedSuccessfully())
    { //handle error: file load
        wxMessageBox(wxString(_("Could not open configuration file ")) + wxT("\"") + filename + wxT("\""), _("Error"), wxOK | wxICON_ERROR);
        return;
    }

    XmlMainConfig  mainCfg;  //structure to receive main settings
    XmlBatchConfig batchCfg; //structure to receive batch settings
    if (    inputFile.readXmlMainConfig(mainCfg) && //read main configuration settings
            inputFile.readXmlBatchConfig(batchCfg)) //read GUI layout configuration
    {
        //all settings have been read successfully...

        applicationRunsInBatchWithoutWindows = batchCfg.silent;  //this value is needed for the application to decide whether to wait for windows to close or not

        //format directory names
        for (vector<FolderPair>::iterator i = mainCfg.directoryPairs.begin(); i != mainCfg.directoryPairs.end(); ++i)
        {
            i->leftDirectory  = FreeFileSync::getFormattedDirectoryName(i->leftDirectory);
            i->rightDirectory = FreeFileSync::getFormattedDirectoryName(i->rightDirectory);
        }

        //init logfile
        LogFile* log = NULL;
        if (batchCfg.silent)
            log = new LogFile;
        DeleteOnExit dummy(log); //delete log object on exit (if not NULL)

        if (log && !log->isOkay())
        { //handle error: file load
            wxMessageBox(_("Unable to create logfile!"), _("Error"), wxOK | wxICON_ERROR);
            return;
        }


        //check if directories exist
        wxString errorMessage;
        if (!FreeFileSync::foldersAreValidForComparison(mainCfg.directoryPairs, errorMessage))
        {
            if (batchCfg.silent)
            {
                log->write(errorMessage, _("Warning"));
                log->close(_("Synchronization aborted!"));
            }
            else wxMessageBox(errorMessage + wxT("\n\n") + _("Synchronization aborted!"), _("Warning"), wxICON_WARNING);

            returnValue = -2;
            return;
        }


        //test existence of Recycle Bin
        if (mainCfg.cfg.useRecycleBin)
        {
            if (!FreeFileSync::recycleBinExists())
            {
                wxString errorMessage  = wxString(_("Unable to initialize Recycle Bin!"));
                wxString statusMessage = wxString(_("Synchronization aborted!"));
                if (batchCfg.silent)
                {
                    log->write(errorMessage, _("Error"));
                    log->close(statusMessage);
                }
                else wxMessageBox(errorMessage + wxT("\n\n") + statusMessage, _("Error"), wxICON_WARNING);

                returnValue = -2;
                return;
            }
        }

        //begin of synchronization process (all in one try-catch block)
        try
        {
            FileCompareResult currentGridData;
            //class handling status updates and error messages
            BatchStatusUpdater statusUpdater(mainCfg.cfg.continueOnError, batchCfg.silent, log);

//COMPARE DIRECTORIES
            //unsigned int startTime = GetTickCount();
            FreeFileSync::startCompareProcess(currentGridData,
                                              mainCfg.directoryPairs,
                                              mainCfg.cfg.compareVar,
                                              &statusUpdater);
            //wxMessageBox(wxString::Format(wxT("%i"), unsigned(GetTickCount()) - startTime));

//APPLY FILTERS
            if (mainCfg.cfg.filterIsActive)
                FreeFileSync::filterCurrentGridData(currentGridData, mainCfg.cfg.includeFilter, mainCfg.cfg.excludeFilter);

//check if there are files/folders to be sync'ed at all
            int objectsToCreate    = 0;
            int objectsToOverwrite = 0;
            int objectsToDelete    = 0;
            double dataToProcess   = 0;
            FreeFileSync::calcTotalBytesToSync(objectsToCreate,
                                               objectsToOverwrite,
                                               objectsToDelete,
                                               dataToProcess,
                                               currentGridData,
                                               mainCfg.cfg.syncConfiguration);
            if (objectsToCreate + objectsToOverwrite + objectsToDelete == 0)
            {
                statusUpdater.noSynchronizationNeeded(); //inform about this special case

                returnValue = -3;
                return;
            }

//START SYNCHRONIZATION
            //unsigned int startTime = GetTickCount();
            FreeFileSync::startSynchronizationProcess(currentGridData, mainCfg.cfg.syncConfiguration, &statusUpdater, mainCfg.cfg.useRecycleBin);
            //wxMessageBox(wxString::Format(wxT("%i"), unsigned(GetTickCount()) - startTime));
        }
        catch (AbortThisProcess& theException)  //exit used by statusUpdater
        {
            returnValue = -4;
            return;
        }

    }
    else
    {   //handle error: parsing
        wxMessageBox(wxString(_("Error parsing configuration file ")) + wxT("\"") + filename + wxT("\""), _("Error"), wxOK | wxICON_ERROR);
        return;
    }

    return; //exit program
}
//######################################################################################################


BatchStatusUpdater::BatchStatusUpdater(bool continueOnError, bool silent, LogFile* log) :
        m_log(log),
        continueErrors(continueOnError),
        silentMode(silent),
        currentProcess(-1),
        synchronizationNeeded(true)
{
    if (!silentMode)
    {
        syncStatusFrame = new SyncStatus(this, NULL);
        syncStatusFrame->Show();
    }
}


BatchStatusUpdater::~BatchStatusUpdater()
{
    unsigned int failedItems = unhandledErrors.GetCount();

    //output the result
    if (silentMode)
    {
        if (abortionRequested)
            m_log->close(_("Synchronization aborted!"));
        else if (failedItems)
            m_log->close(_("Synchronization completed with errors!"));
        else
        {
            if (!synchronizationNeeded)
                m_log->write(_("Nothing to synchronize. Both directories adhere to the sync-configuration!"), _("Info"));
            m_log->close(_("Synchronization completed successfully."));
        }
    }
    else
    {
        wxString finalMessage;
        if (failedItems)
        {
            finalMessage = wxString(_("Warning: Synchronization failed for ")) + globalFunctions::numberToWxString(failedItems) + _(" item(s):\n\n");
            for (unsigned int j = 0; j < failedItems; ++j)
                finalMessage+= unhandledErrors[j] + wxT("\n");
            finalMessage+= wxT("\n");
        }

        //notify to syncStatusFrame that current process has ended
        if (abortionRequested)
        {
            finalMessage+= _("Synchronization aborted!");
            syncStatusFrame->setStatusText_NoUpdate(finalMessage);
            syncStatusFrame->processHasFinished(SyncStatus::ABORTED);  //enable okay and close events
        }
        else if (failedItems)
        {
            finalMessage+= _("Synchronization completed with errors!");
            syncStatusFrame->setStatusText_NoUpdate(finalMessage);
            syncStatusFrame->processHasFinished(SyncStatus::FINISHED_WITH_ERROR);
        }
        else
        {
            if (synchronizationNeeded)
                finalMessage+= _("Synchronization completed successfully.");
            else
                finalMessage+= _("Nothing to synchronize. Both directories adhere to the sync-configuration!");

            syncStatusFrame->setStatusText_NoUpdate(finalMessage);
            syncStatusFrame->processHasFinished(SyncStatus::FINISHED_WITH_SUCCESS);
        }
    }
}


inline
void BatchStatusUpdater::updateStatusText(const wxString& text)
{
    if (silentMode)
    {
        if (currentProcess == FreeFileSync::synchronizeFilesProcess)
            m_log->write(text, _("Info"));
    }
    else
        syncStatusFrame->setStatusText_NoUpdate(text);
}


void BatchStatusUpdater::initNewProcess(int objectsTotal, double dataTotal, int processID)
{
    currentProcess = processID;

    if (!silentMode)
    {
        if (currentProcess == FreeFileSync::scanningFilesProcess)
            syncStatusFrame->setCurrentStatus(SyncStatus::SCANNING);

        else if (currentProcess == FreeFileSync::compareFileContentProcess)
        {
            syncStatusFrame->resetGauge(objectsTotal, dataTotal);
            syncStatusFrame->setCurrentStatus(SyncStatus::COMPARING);
        }

        else if (currentProcess == FreeFileSync::synchronizeFilesProcess)
        {
            syncStatusFrame->resetGauge(objectsTotal, dataTotal);
            syncStatusFrame->setCurrentStatus(SyncStatus::SYNCHRONIZING);
        }
        else assert(false);
    }
}


inline
void BatchStatusUpdater::updateProcessedData(int objectsProcessed, double dataProcessed)
{
    if (!silentMode)
    {
        if (currentProcess == FreeFileSync::scanningFilesProcess)
            syncStatusFrame->m_gauge1->Pulse();
        else if (currentProcess == FreeFileSync::compareFileContentProcess)
            syncStatusFrame->incProgressIndicator_NoUpdate(objectsProcessed, dataProcessed);
        else if (currentProcess == FreeFileSync::synchronizeFilesProcess)
            syncStatusFrame->incProgressIndicator_NoUpdate(objectsProcessed, dataProcessed);
        else assert(false);
    }
}


int BatchStatusUpdater::reportError(const wxString& text)
{
    if (silentMode) //write error message log and abort the complete session if necessary
    {
        unhandledErrors.Add(text);
        m_log->write(text, _("Error"));

        if (continueErrors)                 //  <- /|\ before return, the logfile is written!!!
            return StatusUpdater::continueNext;
        else
        {
            abortionRequested = true;
            throw AbortThisProcess();
        }
    }
    else    //show dialog to user who can decide how to continue
    {
        if (continueErrors) //this option can be set from commandline or by the user in the error dialog on UI
        {
            unhandledErrors.Add(text);
            return StatusUpdater::continueNext;
        }

        wxString errorMessage = text + _("\n\nContinue with next object, retry or abort synchronization?");
        ErrorDlg* errorDlg = new ErrorDlg(errorMessage, continueErrors);

        syncStatusFrame->updateStatusDialogNow();
        int rv = errorDlg->ShowModal();
        errorDlg->Destroy();

        switch (rv)
        {
        case ErrorDlg::continueButtonPressed:
            unhandledErrors.Add(text);
            return StatusUpdater::continueNext;
        case ErrorDlg::retryButtonPressed:
            return StatusUpdater::retry;
        case ErrorDlg::abortButtonPressed:
        {
            unhandledErrors.Add(text);
            abortionRequested = true;
            throw AbortThisProcess();
        }
        default:
            assert (false);
            return StatusUpdater::continueNext;
        }
    }
}


inline
void BatchStatusUpdater::forceUiRefresh()
{
    if (!silentMode)
        syncStatusFrame->updateStatusDialogNow();
}


void BatchStatusUpdater::abortThisProcess()
{
    throw AbortThisProcess();  //abort can be triggered by syncStatusFrame
}


void BatchStatusUpdater::noSynchronizationNeeded()
{
    synchronizationNeeded = false;;
}
