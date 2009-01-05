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
    if (!wxSetWorkingDirectory(wxFileName(wxStandardPaths::Get().GetExecutablePath()).GetPath()))
    {   //show messagebox and quit program immediately
        wxMessageBox(_("Could not set working directory to directory containing executable file!"), _("An exception occured!"), wxOK | wxICON_ERROR);
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
            wxMessageBox(error.show(), _("Error"), wxOK | wxICON_ERROR);
            return false;
        }
        //else: globalSettings already has default values
    }

    //set program language: needs to happen aber working directory has been set!
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
    else //start in GUI mode (standard)
    {
        MainDialog* frame = new MainDialog(NULL, FreeFileSync::LAST_CONFIG_FILE, &programLanguage, globalSettings);
        frame->SetIcon(*globalResource.programIcon); //set application icon
        frame->Show();
    }
}


bool Application::OnExceptionInMainLoop()
{
    throw; //just pass exceptions and avoid display of additional exception messagebox: they will be caught in OnRun()
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
        wxMessageBox(error.show(), _("Error"), wxOK | wxICON_ERROR);
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
        wxString logfileName = wxString(wxT("FFS_")) + wxDateTime::Now().FormatISODate() + wxChar('_') + tmp + wxT(".log");

        logFile.Open(logfileName.c_str(), wxT("w"));
        readyToWrite = logFile.IsOpened();
        if (readyToWrite)
        {
            wxString headerLine = wxString(wxT("FreeFileSync (")) + _("Date: ") + wxDateTime::Now().FormatDate() + wxT(" ") + _("Time: ") +  wxDateTime::Now().FormatTime() + wxT(")");
            logFile.Write(headerLine + wxChar('\n'));
            logFile.Write(wxString().Pad(headerLine.Len(), wxChar('-')) + wxChar('\n') + wxChar('\n'));

            wxString caption = _("Log-messages:");
            logFile.Write(caption + wxChar('\n'));
            logFile.Write(wxString().Pad(caption.Len(), wxChar('-')) + wxChar('\n'));

            write(wxString(_("Start")) + wxChar('\n'));

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


void Application::runBatchMode(const wxString& filename, xmlAccess::XmlGlobalSettings& globalSettings)
{
    applicationRunsInBatchWithoutWindows = false; //default value

    //load XML settings
    XmlBatchConfig batchCfg;  //structure to receive gui settings
    try
    {
        batchCfg = xmlAccess::readBatchConfig(filename);
    }
    catch (const FileError& error)
    {
        wxMessageBox(error.show(), _("Error"), wxOK | wxICON_ERROR);
        return;
    }

    //all settings have been read successfully...
    applicationRunsInBatchWithoutWindows = batchCfg.silent; //this value is needed for the application to decide whether to wait for all windows to close or not

    //format directory names
    for (vector<FolderPair>::iterator i = batchCfg.directoryPairs.begin(); i != batchCfg.directoryPairs.end(); ++i)
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


    //begin of synchronization process (all in one try-catch block)
    try
    {
        FileCompareResult currentGridData;
        //class handling status updates and error messages
        BatchStatusUpdater statusUpdater(batchCfg.mainCfg.ignoreErrors, batchCfg.silent, log);

        //test existence of Recycle Bin
        if (batchCfg.mainCfg.useRecycleBin)
        {
            if (!FreeFileSync::recycleBinExists())
            {
                statusUpdater.setFinalStatus(_("Unable to initialize Recycle Bin!"), SyncStatus::ABORTED);
                returnValue = -2;
                return;
            }
        }

        //check if directories are valid
        wxString errorMessage;
        if (!FreeFileSync::foldersAreValidForComparison(batchCfg.directoryPairs, errorMessage))
        {
            statusUpdater.setFinalStatus(errorMessage, SyncStatus::ABORTED);
            returnValue = -2;
            return;
        }


        //check if folders have dependencies
        if (globalSettings.global.folderDependCheckActive)
        {
            wxString warningMessage;
            if (FreeFileSync::foldersHaveDependencies(batchCfg.directoryPairs, warningMessage))
            {
                //abort if in silent mode
                if (batchCfg.silent)
                {
                    statusUpdater.setFinalStatus(warningMessage, SyncStatus::ABORTED);
                    returnValue = -2;
                    return;
                }

                //non silent mode: offer possibility to ignore issue
                bool hideThisDialog = false;
                wxString messageText = warningMessage + wxT("\n\n") +
                                       _("Consider this when setting up synchronization rules: You might want to avoid write access to these directories so that synchronization of both does not interfere.");

                //show popup and ask user how to handle warning
                WarningDlg* warningDlg = new WarningDlg(statusUpdater.getWindow(), WarningDlg::BUTTON_IGNORE | WarningDlg::BUTTON_ABORT, messageText, hideThisDialog);
                if (warningDlg->ShowModal() == WarningDlg::BUTTON_ABORT)
                {
                    statusUpdater.setFinalStatus(warningMessage, SyncStatus::ABORTED);
                    returnValue = -2;
                    return;
                }
                else
                    globalSettings.global.folderDependCheckActive = !hideThisDialog;
            }
        }


        //COMPARE DIRECTORIES
        bool lineBreakOnMessages = !batchCfg.silent;
        FreeFileSync::CompareProcess comparison(lineBreakOnMessages, &statusUpdater);
        comparison.startCompareProcess(batchCfg.directoryPairs,
                                       batchCfg.mainCfg.compareVar,
                                       currentGridData);

#ifdef FFS_WIN
        //check if DST time correction needs to be applied
        if (globalSettings.global.dstCheckActive)
        {
            int timeShift = 0;
            wxString driveName;
            FreeFileSync::checkForDSTChange(currentGridData, batchCfg.directoryPairs, timeShift, driveName);
            if (timeShift)
            {
                //abort if in silent mode
                if (batchCfg.silent)
                {
                    statusUpdater.setFinalStatus(_("Daylight saving time change detected for FAT/FAT32 drive."), SyncStatus::ABORTED);
                    returnValue = -2;
                    return;
                }

                //non silent mode: offer possibility to resolve issue
                bool hideThisDialog = false;
                wxString errorMessage = wxString(_("A file time shift due to a daylight saving time change was detected for a FAT/FAT32 drive.")) + wxT("\n")
                                        + _("You can adjust the file times accordingly to resolve the issue:");
                errorMessage+= wxString(wxT("\n\n")) + _("Drive:") + wxT(" ") + driveName + wxT("\n")
                               + _("Time shift:") + wxT(" ") + globalFunctions::numberToWxString(timeShift);

                //show popup and ask user how to handle the DST change
                WarningDlg* warningDlg = new WarningDlg(statusUpdater.getWindow(), WarningDlg::BUTTON_RESOLVE | WarningDlg::BUTTON_IGNORE, errorMessage, hideThisDialog);
                warningDlg->m_bitmap10->SetBitmap(*globalResource.bitmapClock);

                if (warningDlg->ShowModal() == WarningDlg::BUTTON_RESOLVE)
                {
                    ModifyFilesDlg* modifyDlg = new ModifyFilesDlg(NULL, driveName, timeShift);
                    modifyDlg->ShowModal();
                    delete modifyDlg;

                    //exit batch processing
                    statusUpdater.setFinalStatus(wxString(_("Daylight saving time change detected for FAT/FAT32 drive.")) + wxT("\n\n")
                                                 + _("Please restart synchronization!"), SyncStatus::ABORTED);
                    returnValue = -2;
                    return;
                }
                else
                    globalSettings.global.dstCheckActive = !hideThisDialog;
            }
        }
#endif  //FFS_WIN


        //APPLY FILTERS
        if (batchCfg.mainCfg.filterIsActive)
            FreeFileSync::filterCurrentGridData(currentGridData, batchCfg.mainCfg.includeFilter, batchCfg.mainCfg.excludeFilter);

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
                                           batchCfg.mainCfg.syncConfiguration);
        if (objectsToCreate + objectsToOverwrite + objectsToDelete == 0)
        {
            statusUpdater.setFinalStatus(_("Nothing to synchronize. Both directories adhere to the sync-configuration!"), SyncStatus::FINISHED_WITH_SUCCESS); //inform about this special case
            returnValue = 0;
            return;
        }

        //START SYNCHRONIZATION
        FreeFileSync::SyncProcess synchronization(batchCfg.mainCfg.useRecycleBin, lineBreakOnMessages, &statusUpdater);
        synchronization.startSynchronizationProcess(currentGridData, batchCfg.mainCfg.syncConfiguration);
    }
    catch (AbortThisProcess& theException)  //exit used by statusUpdater
    {
        returnValue = -4;
        return;
    }

    return; //exit program
}
//######################################################################################################


BatchStatusUpdater::BatchStatusUpdater(bool ignoreAllErrors, bool silent, LogFile* log) :
        m_log(log),
        ignoreErrors(ignoreAllErrors),
        silentMode(silent),
        currentProcess(StatusHandler::PROCESS_NONE)
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
        if (!customStatusMessage.IsEmpty())
            m_log->close(customStatusMessage);
        else if (abortRequested)
            m_log->close(_("Synchronization aborted!"));
        else if (failedItems)
            m_log->close(_("Synchronization completed with errors!"));
        else
            m_log->close(_("Synchronization completed successfully."));
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
        if (!customStatusMessage.IsEmpty()) //custom status message written by service consumer
        {
            finalMessage+= customStatusMessage;
            syncStatusFrame->setStatusText_NoUpdate(finalMessage);
            syncStatusFrame->processHasFinished(customStatusId);
        }
        else if (abortRequested)
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
            finalMessage+= _("Synchronization completed successfully.");
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
        if (currentProcess == StatusHandler::PROCESS_SYNCHRONIZING)
            m_log->write(text, _("Info"));
    }
    else
        syncStatusFrame->setStatusText_NoUpdate(text);
}


void BatchStatusUpdater::initNewProcess(int objectsTotal, double dataTotal, StatusHandler::Process processID)
{
    currentProcess = processID;

    if (!silentMode)
    {
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
}


inline
void BatchStatusUpdater::updateProcessedData(int objectsProcessed, double dataProcessed)
{
    if (!silentMode)
    {
        if (currentProcess == StatusHandler::PROCESS_SCANNING)
            syncStatusFrame->m_gauge1->Pulse();
        else if (currentProcess == StatusHandler::PROCESS_COMPARING_CONTENT)
            syncStatusFrame->incProgressIndicator_NoUpdate(objectsProcessed, dataProcessed);
        else if (currentProcess == StatusHandler::PROCESS_SYNCHRONIZING)
            syncStatusFrame->incProgressIndicator_NoUpdate(objectsProcessed, dataProcessed);
        else assert(false);
    }
}


ErrorHandler::Response BatchStatusUpdater::reportError(const wxString& text)
{
    if (silentMode) //write error message log and abort the complete session if necessary
    {
        unhandledErrors.Add(text);
        m_log->write(text, _("Error"));

        if (ignoreErrors) // /|\ before return, the logfile is written!!!
            return ErrorHandler::CONTINUE_NEXT;
        else
        {
            abortRequested = true;
            throw AbortThisProcess();
        }
    }
    else //show dialog to user who can decide how to continue
    {
        if (ignoreErrors) //this option can be set from commandline or by the user in the error dialog on UI
        {
            unhandledErrors.Add(text);
            return ErrorHandler::CONTINUE_NEXT;
        }

        syncStatusFrame->updateStatusDialogNow();

        bool ignoreNextErrors = false;
        wxString errorMessage = text + wxT("\n\n") + _("Ignore this error, retry or abort synchronization?");
        ErrorDlg* errorDlg = new ErrorDlg(syncStatusFrame, errorMessage, ignoreNextErrors, 90);

        int rv = errorDlg->ShowModal();
        switch (rv)
        {
        case ErrorDlg::BUTTON_IGNORE:
            ignoreErrors = ignoreNextErrors;
            unhandledErrors.Add(text);
            return ErrorHandler::CONTINUE_NEXT;
        case ErrorDlg::BUTTON_RETRY:
            return ErrorHandler::RETRY;
        case ErrorDlg::BUTTON_ABORT:
        {
            unhandledErrors.Add(text);
            abortRequested = true;
            throw AbortThisProcess();
        }
        default:
            assert (false);
            return ErrorHandler::CONTINUE_NEXT;
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


void BatchStatusUpdater::setFinalStatus(const wxString& message, SyncStatus::SyncStatusID id)
{
    customStatusMessage = message;
    customStatusId = id;
}


wxWindow* BatchStatusUpdater::getWindow()
{
    assert(!silentMode);
    if (!silentMode)
        return syncStatusFrame;
    else
        return NULL;
}
