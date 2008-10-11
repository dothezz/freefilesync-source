/***************************************************************
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
#include "library/globalFunctions.h"

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

    //set program language
    programLanguage.loadLanguageFromCfg();

    //activate support for .png files
    wxImage::AddHandler(new wxPNGHandler);

    //load icon and animation resources (also needed for commandline: status dialog, error dialog
    GlobalResources::loadResourceFiles();

    //test if ffs is to be started on UI with config file passed as commandline parameter
    wxString configFileForUI = FreeFileSync::FfsLastConfigFile;
    if (argc > 1 && wxFileExists(argv[1]) && FreeFileSync::isFFS_ConfigFile(argv[1]))
        configFileForUI = argv[1];

    //should it start in commandline mode?
    else if (argc > 1)
    {
        parseCommandline();

        if (applicationRunsOnCommandLineWithoutWindows)
        {
            ExitMainLoop(); //exit programm on next main loop iteration
            return;
        }
        else
            return;         //wait for the user to close the status window
    }
    else  //no parameters passed: continue with UI
    {
        //show UI dialog
        MainDialog* frame = new MainDialog(NULL, configFileForUI, &programLanguage);
        frame->SetIcon(*GlobalResources::programIcon); //set application icon
        frame->Show();
    }
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
    GlobalResources::unloadResourceFiles();
    return 0;
}


SyncDirection convertCmdlineCfg(const wxString& cfg, const int i)
{
    assert (cfg.Len() == 5);
    assert (0 <= i <= 4);

    switch (cfg[i])
    {
    case 'L':
        return SYNC_DIR_LEFT;
    case 'R':
        return SYNC_DIR_RIGHT;
    case 'N':
        return SYNC_DIR_NONE;
    default:
        assert(false);
        return SYNC_DIR_NONE;
    }
}


void Application::logInit()
{
    wxString tmp = wxDateTime::Now().FormatISOTime();
    tmp.Replace(wxT(":"), wxEmptyString);
    wxString logfileName = wxString(wxT("FFS_")) + wxDateTime::Now().FormatISODate() + '_' + tmp + wxT(".log");

    logFile.Open(logfileName.c_str(), wxT("wb"));
    if (!logFile.IsOpened())
        throw RuntimeException(_("Unable to create logfile!"));
    logFile.Write(wxString(_("FreeFileSync   (Date: ")) + wxDateTime::Now().FormatDate() + _("  Time: ") +  wxDateTime::Now().FormatTime() + wxT(")") + wxChar('\n'));
    logFile.Write(wxString(_("-------------------------------------------------")) + '\n');
    logFile.Write(wxChar('\n'));
    logFile.Write(_("Log-messages:\n-------------"));
    logFile.Write(wxChar('\n'));
    logWrite(_("Start"));
    logFile.Write(wxChar('\n'));
}


void Application::logWrite(const wxString& logText, const wxString& problemType)
{
    logFile.Write(wxString(wxT("[")) + wxDateTime::Now().FormatTime() + wxT("] "));

    if (problemType != wxEmptyString)
        logFile.Write(problemType + wxT(": "));

    logFile.Write(logText + wxChar('\n'));
}


void Application::logClose(const wxString& finalText)
{
    logFile.Write(wxChar('\n'));
    logWrite(finalText, _("Stop"));
    //logFile.close(); <- not needed
}


void Application::parseCommandline()
{
    //commandline-descriptor must be initialized here AFTER the program language is set
    const wxCmdLineEntryDesc cmdLineDesc [] =
    {
        { wxCMD_LINE_SWITCH,
            wxT("h"),
            wxT("help"),
            _("Displays help on the command line parameters\n"),
            wxCMD_LINE_VAL_NONE,
            wxCMD_LINE_OPTION_HELP
        },

        {
            wxCMD_LINE_OPTION,
            GlobalResources::paramCompare,
            NULL,
            wxString(_("Specify algorithm to test if files are equal:\n\n\t\t")) + GlobalResources::valueSizeDate + _(": check filesize and date\n\t\t") + GlobalResources::valueContent + _(": check file content\n"),
            wxCMD_LINE_VAL_STRING,
            wxCMD_LINE_OPTION_MANDATORY
        },

        {
            wxCMD_LINE_OPTION,
            GlobalResources::paramCfg,
            NULL,
            _("Specify the sync-direction used for each type of file by a string of five chars:\n\n\t\tChar 1: Folders/files that exist on left side only\n\t\tChar 2: Folders/files that exist on right side only\n\t\tChar 3: Files that exist on both sides, left one is newer\n\t\tChar 4: Files that exist on both sides, right one is newer\n\t\tChar 5: Files that exist on both sides and are different\n\n\t\tSync-direction: L: left, R: right, N: none\n"),
            wxCMD_LINE_VAL_STRING,
            wxCMD_LINE_OPTION_MANDATORY
        },

        {
            wxCMD_LINE_PARAM,
            NULL,
            NULL,
            _("<left directory>"),
            wxCMD_LINE_VAL_STRING
        },

        {
            wxCMD_LINE_PARAM,
            NULL,
            NULL,
            _("<right directory>"),
            wxCMD_LINE_VAL_STRING
        },

        {
            wxCMD_LINE_OPTION,
            GlobalResources::paramInclude,
            NULL,
            _("Specify names to be included separated by ';'. Wildcards '*' and '?' are supported. Default: \"*\"\n"),
            wxCMD_LINE_VAL_STRING,
        },

        {
            wxCMD_LINE_OPTION,
            GlobalResources::paramExclude,
            NULL,
            _("Specify names to be excluded separated by ';'. Wildcards '*' and '?' are supported. Default: \"\"\n"),
            wxCMD_LINE_VAL_STRING,
        },

        {
            wxCMD_LINE_SWITCH,
            GlobalResources::paramContinueError,
            NULL,
            _("If errors occur during folder comparison or synchronization they are ignored and the process continues\n")
        },

        {
            wxCMD_LINE_SWITCH,
            GlobalResources::paramRecycler,
            NULL,
            _("Move files to Recycle Bin instead of deleting or overwriting them directly.\n")
        },

        {
            wxCMD_LINE_SWITCH,
            GlobalResources::paramSilent,
            NULL,
            wxString(_("Do not show graphical status and error messages but write to a logfile instead")) + _(".\n\nExamples:\n\n1.) FreeFileSync -cmp SIZEDATE -cfg RRRRR C:\\Source C:\\Target\n2.) FreeFileSync -cmp sizedate -cfg rlrln c:\\dir1 c:\\dir2 -incl *.doc\n\n1: Creates a mirror backup of the left directory\n2: Synchronizes all *.doc files from both directories simultaneously\n\n")
        },

        {
            wxCMD_LINE_NONE
        }
    };

    //now set the parser with all MANDATORY options and parameters
    wxCmdLineParser parser(cmdLineDesc, argc, argv);
    parser.SetSwitchChars(wxT("-"));
    if (parser.Parse() != 0)  //if commandline is used incorrectly display help dialog and exit program
        return;

    //commandline parameters
    wxString cmp;
    wxString cfg;
    wxString leftDir;
    wxString rightDir;
    wxString included;
    wxString excluded;
    bool continueOnError = parser.Found(GlobalResources::paramContinueError);
    bool useRecycler     = parser.Found(GlobalResources::paramRecycler);
    bool silent          = parser.Found(GlobalResources::paramSilent);

    applicationRunsOnCommandLineWithoutWindows = silent;  //this value is needed for the application to decide whether to wait for windows to close or not


//check existence of all commandline parameters
    if (!parser.Found(GlobalResources::paramCompare, &cmp) ||
            !parser.Found(GlobalResources::paramCfg, &cfg) ||
            parser.GetParamCount() != 2)
    {
        parser.Usage();
        return;
    }

    cmp.UpperCase();
    cfg.UpperCase();
    leftDir  = parser.GetParam(0);
    rightDir = parser.GetParam(1);

//evaluate filter settings
    bool filteringEnabled = false;
    if (parser.Found(GlobalResources::paramInclude, &included))
        filteringEnabled = true;
    else
        included = wxT("*");

    if (parser.Found(GlobalResources::paramExclude, &excluded))
        filteringEnabled = true;
    else
        excluded = wxEmptyString;

//until here all options and parameters have been set
//--------------------------------------------------------------------

//check consistency of all commandline parameters
    if ((cmp != GlobalResources::valueSizeDate && cmp != GlobalResources::valueContent) ||
            cfg.Len() != 5 ||
            (cfg[0] != 'L' && cfg[0] != 'R' && cfg[0] != 'N') ||
            (cfg[1] != 'L' && cfg[1] != 'R' && cfg[1] != 'N') ||
            (cfg[2] != 'L' && cfg[2] != 'R' && cfg[2] != 'N') ||
            (cfg[3] != 'L' && cfg[3] != 'R' && cfg[3] != 'N') ||
            (cfg[4] != 'L' && cfg[4] != 'R' && cfg[4] != 'N'))
    {
        parser.Usage();
        return;
    }

//init logfile
    if (silent) logInit();

    wxString logText;

    //check if directories exist
    if (!wxDirExists(leftDir))
    {
        wxString errorMessage = wxString(_("Directory ")) + wxT("\"") + leftDir + wxT("\"") + _(" does not exist.");
        wxString statusMessage = wxString(_("Synchronization aborted!"));
        if (silent)
        {
            logWrite(errorMessage, _("Warning"));
            logClose(statusMessage);
        }
        else wxMessageBox(errorMessage + wxT("\n\n") + statusMessage, _("Warning"), wxICON_WARNING);

        returnValue = -2;
        return;
    }
    else if (!wxDirExists(rightDir))
    {
        wxString errorMessage = wxString(_("Directory ")) + wxT("\"") + rightDir + wxT("\"") + _(" does not exist.");
        wxString statusMessage = wxString(_("Synchronization aborted!"));
        if (silent)
        {
            logWrite(errorMessage, _("Warning"));
            logClose(statusMessage);
        }
        else wxMessageBox(errorMessage + wxT("\n\n") + statusMessage, _("Warning"), wxICON_WARNING);

        returnValue = -2;
        return;
    }

    //test existence of Recycle Bin
    if (useRecycler)
    {
        if (!FreeFileSync::recycleBinExists())
        {
            wxString errorMessage = wxString(_("Unable to initialize Recycle Bin!"));
            wxString statusMessage = wxString(_("Synchronization aborted!"));
            if (silent)
            {
                logWrite(errorMessage, _("Error"));
                logClose(statusMessage);
            }
            else wxMessageBox(errorMessage + wxT("\n\n") + statusMessage, _("Error"), wxICON_WARNING);

            returnValue = -2;
            return;
        }
    }

//until here all options and parameters are consistent
//--------------------------------------------------------------------

    CompareVariant cmpVar = CMP_BY_CONTENT; //dummy value to suppress compiler warning
    SyncConfiguration syncConfiguration;
    FileCompareResult currentGridData;

    if (cmp == GlobalResources::valueSizeDate)
        cmpVar = CMP_BY_TIME_SIZE;
    else if (cmp == GlobalResources::valueContent)
        cmpVar = CMP_BY_CONTENT;
    else
        assert (false);

    syncConfiguration.exLeftSideOnly  = convertCmdlineCfg(cfg, 0);
    syncConfiguration.exRightSideOnly = convertCmdlineCfg(cfg, 1);
    syncConfiguration.leftNewer       = convertCmdlineCfg(cfg, 2);
    syncConfiguration.rightNewer      = convertCmdlineCfg(cfg, 3);
    syncConfiguration.different       = convertCmdlineCfg(cfg, 4);

    //begin of synchronization process (all in one try-catch block)
    try
    {
        //class handling status updates and error messages
        CommandLineStatusUpdater statusUpdater(this, continueOnError, silent);

//COMPARE DIRECTORIES
        //unsigned int startTime = GetTickCount();
        FreeFileSync::startCompareProcess(currentGridData,
                                          FreeFileSync::getFormattedDirectoryName(leftDir),
                                          FreeFileSync::getFormattedDirectoryName(rightDir),
                                          cmpVar,
                                          &statusUpdater);
        //wxMessageBox(wxString::Format(wxT("%i"), unsigned(GetTickCount()) - startTime));

//APPLY FILTERS
        if (filteringEnabled)
            FreeFileSync::filterCurrentGridData(currentGridData, included, excluded);

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
                                           syncConfiguration);
        if (objectsToCreate + objectsToOverwrite + objectsToDelete == 0)
        {
            statusUpdater.noSynchronizationNeeded(); //inform about this special case

            returnValue = -3;
            return;
        }

//START SYNCHRONIZATION
        //unsigned int startTime = GetTickCount();
        FreeFileSync::startSynchronizationProcess(currentGridData, syncConfiguration, &statusUpdater, useRecycler); //default: do not use recycle bin since it's not sure if its possible
        //wxMessageBox(wxString::Format(wxT("%i"), unsigned(GetTickCount()) - startTime));
    }
    catch (AbortThisProcess& theException)  //exit used by statusUpdater
    {
        returnValue = -4;
        return;
    }

    return; //exit program and skip UI dialogs
}
//######################################################################################################


CommandLineStatusUpdater::CommandLineStatusUpdater(Application* application, bool continueOnError, bool silent) :
        app(application),
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


CommandLineStatusUpdater::~CommandLineStatusUpdater()
{
    unsigned int failedItems = unhandledErrors.GetCount();

    //output the result
    if (silentMode)
    {
        if (abortionRequested)
            app->logClose(_("Synchronization aborted!"));
        else if (failedItems)
            app->logClose(_("Synchronization completed with errors!"));
        else
        {
            if (!synchronizationNeeded)
                app->logWrite(_("Nothing to synchronize. Both directories adhere to the sync-configuration!"), _("Info"));
            app->logClose(_("Synchronization completed successfully."));
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
void CommandLineStatusUpdater::updateStatusText(const wxString& text)
{
    if (silentMode)
    {
        if (currentProcess == FreeFileSync::synchronizeFilesProcess)
            app->logWrite(text, _("Info"));
    }
    else
        syncStatusFrame->setStatusText_NoUpdate(text);
}


void CommandLineStatusUpdater::initNewProcess(int objectsTotal, double dataTotal, int processID)
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
void CommandLineStatusUpdater::updateProcessedData(int objectsProcessed, double dataProcessed)
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


int CommandLineStatusUpdater::reportError(const wxString& text)
{
    if (silentMode) //write error message log and abort the complete session if necessary
    {
        unhandledErrors.Add(text);
        app->logWrite(text, _("Error"));

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
void CommandLineStatusUpdater::triggerUI_Refresh()
{
    if (abortionRequested)
        throw AbortThisProcess();  //may be triggered by the SyncStatusDialog

    if (!silentMode)
        if (updateUI_IsAllowed()) //test if specific time span between ui updates is over
            syncStatusFrame->updateStatusDialogNow();
}


void CommandLineStatusUpdater::noSynchronizationNeeded()
{
    synchronizationNeeded = false;;
}
