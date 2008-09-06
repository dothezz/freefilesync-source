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
#include <stdexcept> //for std::runtime_error
#include <wx/wfstream.h>
#include <wx/zipstrm.h>
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

//Note: initialization is done in the FIRST idle event instead in OnInit. Reason: Commandline mode requires the wxApp eventhandler to be established
//for UI update events. This is not the case in OnInit.

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
        throw runtime_error(_("Could not set working directory to directory containing executable file!"));

    //set program language
    programLanguage = new CustomLocale;

    //activate support for .png files
    wxImage::AddHandler(new wxPNGHandler);

    //load icon and animation resources (also needed for commandline: status dialog, error dialog
    GlobalResources::loadResourceFiles();

    //test if ffs is to be started on UI with config file passed as commandline parameter
    wxString configFileUI = FreeFileSync::FFS_LastConfigFile;
    if (argc >= 2 && wxFileExists(argv[1]) && FreeFileSync::isFFS_ConfigFile(argv[1]))
        configFileUI = argv[1];

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
    else
        ;    //no parameters passed, continue with UI


    //show UI dialog
    MainDialog* frame = new MainDialog(0L, configFileUI);
    frame->SetIcon(wxICON(aaaa)); // To Set App Icon

    frame->Show();
}


int Application::OnRun()
{
    try
    {
        wxApp::OnRun();
    }
    catch (std::runtime_error& theException)    //catch runtime errors during main event loop
    {
        wxMessageBox(_(theException.what()), _("An exception occured!"), wxOK | wxICON_ERROR);
        return -1;
    }
    return returnValue;
}


int Application::OnExit()
{
    delete programLanguage;
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
        return SyncDirLeft;
        break;
    case 'R':
        return SyncDirRight;
        break;
    case 'N':
        return SyncDirNone;
        break;
    default:
        assert(false);
    }
    //dummy return value to suppress compiler warning
    return SyncDirNone;
}


void Application::initLog()
{
    wxString tmp = wxDateTime::Now().FormatISOTime();
    tmp.Replace(":", "");
    wxString logfileName = wxString("FFS_") + wxDateTime::Now().FormatISODate() + '_' + tmp + ".log";

    logFile.open(logfileName.c_str());
    if (!logFile)
        throw std::runtime_error(_("Unable to create logfile!"));
    logFile<<_("FreeFileSync   (Date: ") + wxDateTime::Now().FormatDate() + _("  Time: ") +  wxDateTime::Now().FormatTime() + ")"<<endl;
    logFile<<_("-------------------------------------------------")<<endl;
    logFile<<endl;
    logFile<<_("Log-messages:")<<endl;
    logFile<<endl;
}


void Application::writeLog(const wxString& logText, const wxString& problemType)
{
    if (problemType != wxEmptyString)
        logFile<<problemType<<": ";
    logFile<<logText<<endl;
}


void Application::closeLog()
{
    logFile.close();
}


void Application::parseCommandline()
{
    //commandline-descriptor must be initialized here AFTER the program language is set
    const wxCmdLineEntryDesc cmdLineDesc [] =
    {
        { wxCMD_LINE_SWITCH,
            wxT("h"),
            wxT("help"),
            wxT(_("Displays help on the command line parameters\n")),
            wxCMD_LINE_VAL_NONE,
            wxCMD_LINE_OPTION_HELP
        },

        {
            wxCMD_LINE_OPTION,
            "cmp",
            NULL,
            _("Specify algorithm to test if files are equal:\n\n\t\tSIZEDATE: check filesize and date\n\t\tCONTENT: check file content\n"),
            wxCMD_LINE_VAL_STRING,
            wxCMD_LINE_OPTION_MANDATORY
        },

        {
            wxCMD_LINE_OPTION,
            "cfg",
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
            "incl",
            NULL,
            _("Specify names to be included separated by ';'. Wildcards '*' and '?' are supported. Default: \"*\"\n"),
            wxCMD_LINE_VAL_STRING,
        },

        {
            wxCMD_LINE_OPTION,
            "excl",
            NULL,
            _("Specify names to be excluded separated by ';'. Wildcards '*' and '?' are supported. Default: \"\"\n"),
            wxCMD_LINE_VAL_STRING,
        },

        {
            wxCMD_LINE_SWITCH,
            "skiperrors",
            NULL,
            _("If errors occur during folder comparison or synchronization they are ignored and the process continues.\n")
        },

        {
            wxCMD_LINE_SWITCH,
            "silent",
            NULL,
            _("\tDo not show UI messages but write to a logfile instead\n\nExamples:\n\n1.) FreeFileSync -cmp SIZEDATE -cfg RRRRR C:\\Source C:\\Target\n2.) FreeFileSync -cmp sizedate -cfg rlrln c:\\dir1 c:\\dir2 -incl *.doc\n\n1: Creates a mirror backup of the left directory\n2: Synchronizes all *.doc files from both directories simultaneously\n\n")
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
    bool skipErrors = parser.Found("skiperrors");
    bool silent     = parser.Found("silent");

    applicationRunsOnCommandLineWithoutWindows = silent;  //this value is needed for the application to decide whether to wait for windows to close or not


//check existence of all commandline parameters
    if (!parser.Found("cmp", &cmp) ||
            !parser.Found("cfg", &cfg) ||
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
    if (parser.Found("incl", &included))
        filteringEnabled = true;
    else
        included = "*";

    if (parser.Found("excl", &excluded))
        filteringEnabled = true;
    else
        excluded = "";

//until here all options and parameters have been set
//--------------------------------------------------------------------

//check consistency of all commandline parameters
    if ((cmp != "SIZEDATE" && cmp != "CONTENT") ||
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
    if (silent) initLog();

    wxString logText;

    //check if directories exist
    if (!wxDirExists(leftDir))
    {
        wxString errorMessage = wxString(_("Directory ")) + leftDir + _(" does not exist. \n\nSynchronization aborted!");
        if (silent) writeLog(errorMessage, _("Warning"));
        if (silent) closeLog();
        else wxMessageBox(errorMessage, _("Warning"), wxICON_WARNING);

        returnValue = -2;
        return;
    }
    else if (!wxDirExists(rightDir))
    {
        wxString errorMessage = wxString(_("Directory ")) + rightDir + _(" does not exist. \n\nSynchronization aborted!");
        if (silent) writeLog(errorMessage, _("Warning"));
        if (silent) closeLog();
        else wxMessageBox(errorMessage, _("Warning"), wxICON_WARNING);

        returnValue = -2;
        return;
    }

//until here all options and parameters are consistent
//--------------------------------------------------------------------

    CompareVariant cmpVar = CompareByMD5;   //dummy value to suppress compiler warning
    SyncConfiguration syncConfiguration;
    FileCompareResult currentGridData;

    if (cmp == "SIZEDATE")
        cmpVar = CompareByTimeAndSize;
    else if (cmp == "CONTENT")
        cmpVar = CompareByMD5;
    else
        assert (false);

    syncConfiguration.exLeftSideOnly  = convertCmdlineCfg(cfg, 0);
    syncConfiguration.exRightSideOnly = convertCmdlineCfg(cfg, 1);
    syncConfiguration.leftNewer       = convertCmdlineCfg(cfg, 2);
    syncConfiguration.rightNewer      = convertCmdlineCfg(cfg, 3);
    syncConfiguration.different       = convertCmdlineCfg(cfg, 4);

    //class handling status updates and error messages
    CommandLineStatusUpdater statusUpdater(this, skipErrors, silent);

    //begin of synchronization process (all in one try-catch block)
    try
    {
//COMPARE DIRECTORIES
        //unsigned int startTime = GetTickCount();
        FreeFileSync::getModelDiff(currentGridData,
                                   FreeFileSync::getFormattedDirectoryName(leftDir),
                                   FreeFileSync::getFormattedDirectoryName(rightDir),
                                   cmpVar,
                                   &statusUpdater);
        //wxMessageBox(wxString::Format(wxT("%i"), unsigned(GetTickCount()) - startTime));

        //check if folders are already in sync
        bool nothingToSync = true;
        for (FileCompareResult::const_iterator i = currentGridData.begin(); i != currentGridData.end(); ++i)
            if (i->cmpResult != FilesEqual)
            {
                nothingToSync = false;
                break;
            }

        if (nothingToSync)
        {
            wxString errorMessage = _("Nothing to synchronize. Both directories seem to contain the same data!");
            if (silent) writeLog("\n");
            if (silent) writeLog(errorMessage);
            if (silent) closeLog();
            else statusUpdater.updateFinalStatus(errorMessage);

            returnValue = -3;
            return;
        }

//APPLY FILTERS
        if (filteringEnabled)
            FreeFileSync::filterCurrentGridData(currentGridData, included, excluded);

//START SYNCHRONIZATION
        //adjust progress indicator
        statusUpdater.switchToSyncProcess(FreeFileSync::calcTotalBytesToTransfer(currentGridData, syncConfiguration).get_d());

        //unsigned int startTime = GetTickCount();
        FreeFileSync::startSynchronizationProcess(currentGridData, syncConfiguration, &statusUpdater, false); //default: do not use recycle bin since it's not sure if its possible
        //wxMessageBox(wxString::Format(wxT("%i"), unsigned(GetTickCount()) - startTime));

//show success message
        wxString successMessage = _("Synchronization completed!");
        if (silent) writeLog("\n");
        if (silent) writeLog(successMessage);
        if (silent) closeLog();
        else
            statusUpdater.updateFinalStatus(successMessage);

    }
    catch (AbortThisProcess& theException)  //exit used by statusUpdater
    {
        if (silent) writeLog("\n\nSynchronization aborted!");
        if (silent) closeLog();

        returnValue = -4;
        return;
    }

    return;   //exit program and skip UI dialogs
}
//######################################################################################################


CommandLineStatusUpdater::CommandLineStatusUpdater(Application* application, bool skipErr, bool silent) :
        app(application),
        skipErrors(skipErr),
        silentMode(silent),
        switchedToSynchronisation(false)  //used for changing mode of gauge
{
    if (!silentMode)
    {
        syncStatusFrame = new SyncStatus(this, 0);
        syncStatusFrame->Show();
    }
}

CommandLineStatusUpdater::~CommandLineStatusUpdater()
{
    if (!silentMode)
    {
        if (abortionRequested)
            syncStatusFrame->processHasFinished(_("Aborted!"));  //enable okay and close events
        else
            syncStatusFrame->processHasFinished(_("Completed"));

        //print the results list
        unsigned int failedItems = unhandledErrors.GetCount();
        wxString result;
        if (failedItems)
        {
            result = wxString(_("Warning: Synchronization failed for ")) + GlobalFunctions::numberToWxString(failedItems) + _(" item(s):\n\n");
            for (unsigned int j = 0; j < failedItems; ++j)
                result+= unhandledErrors[j] + "\n";
        }
        syncStatusFrame->setStatusText_NoUpdate(result);
        syncStatusFrame->updateStatusDialogNow();
    }
}

inline
void CommandLineStatusUpdater::updateStatus(const wxString& text)
{

    if (!silentMode)
    {
        if (switchedToSynchronisation)
            syncStatusFrame->setStatusText_NoUpdate(text);
        else
            syncStatusFrame->setStatusText_NoUpdate(_("Scanning... ") + text);
    }
}


inline
void CommandLineStatusUpdater::updateProgressIndicator(double number)
{
    if (!silentMode)
    {
        if (switchedToSynchronisation)
            syncStatusFrame->incProgressIndicator_NoUpdate(number);
        else
            syncStatusFrame->m_gauge1->Pulse();
    }
}


int CommandLineStatusUpdater::reportError(const wxString& text)
{
    if (silentMode) //write error message log and abort the complete session
    {
        app->writeLog(text, _("Error"));

        if (skipErrors)                 //  <- /|\ before return, the logfile is written!!!
            return StatusUpdater::Continue;
        else
        {
            abortionRequested = true;
            throw AbortThisProcess();
        }

    }
    else    //show dialog to user who can decide how to continue
    {
        if (skipErrors)        //this option can be set from commandline or by the user in the error dialog on UI
        {
            unhandledErrors.Add(text);
            return StatusUpdater::Continue;
        }

        wxString errorMessage = text + _("\n\nContinue with next object, retry or abort synchronization?");

        ErrorDlg* errorDlg = new ErrorDlg(errorMessage, skipErrors);

        int rv = errorDlg->ShowModal();
        errorDlg->Destroy();

        switch (rv)
        {
        case ErrorDlg::ContinueButtonPressed:
            unhandledErrors.Add(text);
            return StatusUpdater::Continue;
        case ErrorDlg::RetryButtonPressed:
            return StatusUpdater::Retry;
        case ErrorDlg::AbortButtonPressed:
        {
            unhandledErrors.Add(text);
            abortionRequested = true;
            throw AbortThisProcess();
        }
        default:
            assert (false);
        }
    }

    return StatusUpdater::Continue;  //dummy return value
}


inline
void CommandLineStatusUpdater::triggerUI_Refresh()
{
    if (abortionRequested) throw AbortThisProcess();

    if (!silentMode)
        if (updateUI_IsAllowed()) //test if specific time span between ui updates is over
            syncStatusFrame->updateStatusDialogNow();
}


inline
void CommandLineStatusUpdater::updateFinalStatus(const wxString& text)  //set by parsedCommandline() to communicate e.g. the final "success message"
{
    if (!silentMode)
        syncStatusFrame->m_textCtrlInfo->SetValue(text);
}


inline
void CommandLineStatusUpdater::switchToSyncProcess(double number)
{
    if (!silentMode)
    {
        syncStatusFrame->resetGauge(number);   //usually this number is set in SyncStatus constructor, but in case of commandline
        //the total number is known only ofter "compare" so it has to be set later
        switchedToSynchronisation = true;
    }
}

//######################################################################################################


wxString exchangeEscapeChars(char* temp)
{
    wxString output(temp);
    output.Replace("\\\\", "\\");
    output.Replace("\\n", "\n");
    output.Replace("\\t", "\t");
    output.Replace("\"\"", """");
    output.Replace("\\\"", "\"");
    return output;
}


CustomLocale::CustomLocale(int language, int flags)
        :wxLocale(language, flags)
{
    char temp[100000];
//    wxString languageFile;
//    switch (language)
//    {
//    case wxLANGUAGE_GERMAN:
    wxString languageFile = "language.dat";
//        break;
//    default: ;
//    }

    //load language file into buffer
    if (languageFile.size() != 0)
    {
        ifstream langFile(languageFile.c_str());
        if (langFile)
        {
            int rowNumber = 0;
            TranslationLine currentLine;
            while (langFile.getline(temp, 100000))
            {
                if (rowNumber%2 == 0)
                    currentLine.original = exchangeEscapeChars(temp);
                else
                {
                    currentLine.translation = exchangeEscapeChars(temp);
                    translationDB.insert(currentLine);
                }
                rowNumber++;
            }
            langFile.close();
        }
    }
}


const wxChar* CustomLocale::GetString(const wxChar* szOrigString, const wxChar* szDomain) const
{
    TranslationLine currentLine;
    currentLine.original = szOrigString;

    //look for translation in buffer table
    Translation::iterator i;
    if ((i = translationDB.find(currentLine)) != translationDB.end())
        return (i->translation.c_str());
    //fallback
    return (szOrigString);
}

