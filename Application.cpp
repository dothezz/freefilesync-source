// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) 2008-2011 ZenJu (zhnmju123 AT gmx.de)                    *
// **************************************************************************

#include "application.h"
#include "ui/main_dlg.h"
#include <wx/msgdlg.h>
#include "comparison.h"
#include "algorithm.h"
#include "synchronization.h"
#include <memory>
#include "ui/batch_status_handler.h"
#include "ui/check_version.h"
#include <wx/file.h>
#include "lib/resources.h"
#include "ui/switch_to_gui.h"
#include "lib/ffs_paths.h"
#include <wx+/app_main.h>
#include <wx/sound.h>
#include <zen/file_handling.h>
#include <wx+/string_conv.h>
#include <wx/log.h>
#include <wx/tooltip.h> //wxWidgets v2.9
#include "lib/lock_holder.h"

#ifdef FFS_LINUX
#include <gtkmm/main.h>
#include <gtk/gtk.h>
#endif

using namespace zen;
using namespace xmlAccess;


IMPLEMENT_APP(Application)


#ifdef FFS_WIN
namespace
{
const DWORD mainThreadId = ::GetCurrentThreadId();

void onTerminationRequested()
{
    std::wstring msg = ::GetCurrentThreadId() == mainThreadId ?
                       L"Termination requested in main thread!\n\n" :
                       L"Termination requested in worker thread!\n\n";
    msg += L"Please take a screenshot and file a bug report on: http://sourceforge.net/projects/freefilesync";

    ::MessageBox(0, msg.c_str(), _("An exception occurred!").c_str(), 0);
    std::abort();
}
}
#endif


bool Application::OnInit()
{
#ifdef FFS_WIN
    std::set_terminate(onTerminationRequested); //unlike wxWidgets uncaught exception handling, this works for all worker threads
#endif

    returnValue = 0;
    //do not call wxApp::OnInit() to avoid using default commandline parser

    //Note: initialization is done in the FIRST idle event instead of OnInit. Reason: batch mode requires the wxApp eventhandler to be established
    //for UI update events. This is not the case at the time of OnInit().
    Connect(wxEVT_IDLE,              wxIdleEventHandler(Application::OnStartApplication), NULL, this);
    Connect(wxEVT_QUERY_END_SESSION, wxEventHandler    (Application::OnQueryEndSession ), NULL, this);
    Connect(wxEVT_END_SESSION,       wxEventHandler    (Application::OnQueryEndSession ), NULL, this);

    return true;
}


void Application::OnStartApplication(wxIdleEvent&)
{
    Disconnect(wxEVT_IDLE, wxIdleEventHandler(Application::OnStartApplication), NULL, this);

    //wxWidgets app exit handling is quite weird... we want the app to exit only if the logical main window is closed
    wxTheApp->SetExitOnFrameDelete(false); //avoid popup-windows from becoming temporary top windows leading to program exit after closure
    wxApp* app = wxTheApp;
    ZEN_ON_BLOCK_EXIT(if (!mainWindowWasSet()) app->ExitMainLoop();); //quit application, if no main window was set (batch silent mode)

    //if appname is not set, the default is the executable's name!
    SetAppName(wxT("FreeFileSync"));

#ifdef FFS_WIN
    //Quote: "Best practice is that all applications call the process-wide SetErrorMode function with a parameter of
    //SEM_FAILCRITICALERRORS at startup. This is to prevent error mode dialogs from hanging the application."
    ::SetErrorMode(SEM_FAILCRITICALERRORS);

#elif defined FFS_LINUX
    Gtk::Main::init_gtkmm_internals();

    ::gtk_rc_parse((utf8CvrtTo<Zstring>(getResourceDir()) + "styles.rc").c_str()); //remove inner border from bitmap buttons
#endif


#if wxCHECK_VERSION(2, 9, 1)
#ifdef FFS_WIN
    wxToolTip::SetMaxWidth(-1); //disable tooltip wrapping -> Windows only (as of 2.9.2)
#endif
    wxToolTip::SetAutoPop(7000); //tooltip visibilty in ms, 5s seems to be default for Windows
#endif


    try //load global settings from XML: they are written on exit, so read them FIRST
    {
        if (fileExists(toZ(getGlobalConfigFile())))
            readConfig(globalSettings);
        //else: globalSettings already has default values
    }
    catch (const xmlAccess::FfsXmlError& error)
    {
        //show messagebox and continue
        if (error.getSeverity() == FfsXmlError::WARNING)
            ; //wxMessageBox(error.toString(), _("Warning"), wxOK | wxICON_WARNING); -> ignore parsing errors: should be migration problems only *cross-fingers*
        else
            wxMessageBox(error.toString(), _("Error"), wxOK | wxICON_ERROR);
    }

    //set program language
    setLanguage(globalSettings.programLanguage);


    //test if FFS is to be started on UI with config file passed as commandline parameter
    std::vector<wxString> commandArgs;
    for (int i = 1; i < argc; ++i)
        commandArgs.push_back(argv[i]);

    bool gotDirNames = false;
    for (auto iter = commandArgs.begin(); iter != commandArgs.end(); ++iter)
    {
        Zstring filename = toZ(*iter);

        if (iter == commandArgs.begin() && dirExists(filename)) //detect which "mode" by testing first command line argument
        {
            gotDirNames = true;
            break;
        }

        if (!fileExists(filename)) //be a little tolerant
        {
            if (fileExists(filename + Zstr(".ffs_batch")))
                filename = filename + Zstr(".ffs_batch");
            else if (fileExists(filename + Zstr(".ffs_gui")))
                filename = filename + Zstr(".ffs_gui");
            else
            {
                wxMessageBox(_("File does not exist:") + L" \"" + filename + L"\"", _("Error"), wxOK | wxICON_ERROR);
                return;
            }
        }
    }

    if (commandArgs.empty())
        runGuiMode(commandArgs, globalSettings);
    else if (gotDirNames) //mode 1: create temp configuration based on directory names passed
    {
        XmlGuiConfig guiCfg;
        guiCfg.mainCfg.syncCfg.directionCfg.var = DirectionConfig::MIRROR;

        for (auto iter = commandArgs.begin(); iter != commandArgs.end(); ++iter)
        {
            size_t index = iter - commandArgs.begin();
            Zstring dirname = toZ(*iter);

            FolderPairEnh& fp = [&]() -> FolderPairEnh&
            {
                if (index < 2)
                    return guiCfg.mainCfg.firstPair;

                guiCfg.mainCfg.additionalPairs.resize((index - 2) / 2 + 1);
                return guiCfg.mainCfg.additionalPairs.back();
            }();

            if (index % 2 == 0)
                fp.leftDirectory = dirname;
            else if (index % 2 == 1)
                fp.rightDirectory = dirname;
        }

        runGuiMode(guiCfg, globalSettings);
    }
    else //mode 2: try to set config/batch-filename set by %1 parameter
        switch (getMergeType(commandArgs)) //throw ()
        {
            case MERGE_BATCH: //pure batch config files
                if (commandArgs.size() == 1)
                    runBatchMode(commandArgs[0], globalSettings);
                else
                    runGuiMode(commandArgs, globalSettings);
                break;

            case MERGE_GUI:       //pure gui config files
            case MERGE_GUI_BATCH: //gui and batch files
                runGuiMode(commandArgs, globalSettings);
                break;

            case MERGE_OTHER: //= none or unknown;
                //commandArgs are not empty and contain at least one non-gui/non-batch config file: find it!
                std::find_if(commandArgs.begin(), commandArgs.end(),
                             [](const wxString& filename) -> bool
                {
                    switch (getXmlType(filename)) //throw()
                    {
                        case XML_TYPE_GLOBAL:
                        case XML_TYPE_OTHER:
                            wxMessageBox(wxString(_("The file does not contain a valid configuration:")) + wxT(" \"") + filename + wxT("\""), _("Error"), wxOK | wxICON_ERROR);
                            return true;

                        case XML_TYPE_GUI:
                        case XML_TYPE_BATCH:
                            break;
                    }
                    return false;
                });
                break;
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
    catch (const std::exception& e) //catch all STL exceptions
    {
        //unfortunately it's not always possible to display a message box in this erroneous situation, however (non-stream) file output always works!
        wxFile safeOutput(toWx(getConfigDir()) + wxT("LastError.txt"), wxFile::write);
        safeOutput.Write(wxString::FromAscii(e.what()));

        wxSafeShowMessage(_("An exception occurred!") + L" - FFS", wxString::FromAscii(e.what()));
        return -9;
    }
    catch (...) //catch the rest
    {
        wxFile safeOutput(toWx(getConfigDir()) + wxT("LastError.txt"), wxFile::write);
        safeOutput.Write(wxT("Unknown exception!"));

        wxSafeShowMessage(_("An exception occurred!"), wxT("Unknown exception!"));
        return -9;
    }

    return returnValue;
}


int Application::OnExit()
{
    //get program language
    globalSettings.programLanguage = getLanguage();

    try //save global settings to XML
    {
        xmlAccess::writeConfig(globalSettings);
    }
    catch (const xmlAccess::FfsXmlError&)
    {
        //wxMessageBox(error.msg(), _("Error"), wxOK | wxICON_ERROR); -> not that important/might be tedious in silent batch?
        assert(false); //get info in debug build
    }

    return 0;
}


void Application::OnQueryEndSession(wxEvent& event)
{
    //alas wxWidgets screws up once again: http://trac.wxwidgets.org/ticket/3069

    MainDialog* mainWin = dynamic_cast<MainDialog*>(GetTopWindow());
    if (mainWin)
        mainWin->onQueryEndSession();
    OnExit();
    //wxEntryCleanup(); -> gives popup "dll init failed" on XP
    std::exit(returnValue); //Windows will terminate anyway: destruct global objects
}


void Application::runGuiMode(const xmlAccess::XmlGuiConfig& guiCfg, xmlAccess::XmlGlobalSettings& settings)
{
    MainDialog* frame = new MainDialog(std::vector<wxString>(), guiCfg, settings, true);
    frame->Show();
}


void Application::runGuiMode(const std::vector<wxString>& cfgFileNames, xmlAccess::XmlGlobalSettings& settings)
{
    MainDialog* frame = new MainDialog(cfgFileNames, settings);
    frame->Show();
}


void Application::runBatchMode(const wxString& filename, xmlAccess::XmlGlobalSettings& globSettings)
{
    using namespace xmlAccess;
    using namespace zen;
    //load XML settings
    XmlBatchConfig batchCfg;  //structure to receive gui settings
    try
    {
        readConfig(filename, batchCfg);
    }
    catch (const xmlAccess::FfsXmlError& error)
    {
        wxMessageBox(error.toString(), _("Error"), wxOK | wxICON_ERROR); //batch mode: break on errors AND even warnings!
        return;
    }
    //all settings have been read successfully...

    //regular check for program updates -> disabled for batch
    //if (batchCfg.showProgress)
    //    checkForUpdatePeriodically(globSettings.lastUpdateCheck);

    try //begin of synchronization process (all in one try-catch block)
    {
        const SwitchToGui switchBatchToGui(filename, batchCfg, globSettings); //prepare potential operational switch

        //class handling status updates and error messages
        BatchStatusHandler statusHandler(batchCfg.showProgress,
                                         extractJobName(filename),
                                         batchCfg.logFileDirectory,
                                         batchCfg.logFileCountMax,
                                         batchCfg.handleError,
                                         switchBatchToGui,
                                         returnValue,
                                         batchCfg.mainCfg.onCompletion,
                                         globSettings.gui.onCompletionHistory);

        const std::vector<FolderPairCfg> cmpConfig = extractCompareCfg(batchCfg.mainCfg);

        bool allowPwPrompt = false;
        switch (batchCfg.handleError)
        {
            case ON_ERROR_POPUP:
                allowPwPrompt = true;
                break;
            case ON_ERROR_IGNORE:
            case ON_ERROR_EXIT:
                break;
        }

        //batch mode: place directory locks on directories during both comparison AND synchronization
        LockHolder dummy(allowPwPrompt);
        std::for_each(cmpConfig.begin(), cmpConfig.end(),
                      [&](const FolderPairCfg& fpCfg)
        {
            dummy.addDir(fpCfg.leftDirectoryFmt,  statusHandler);
            dummy.addDir(fpCfg.rightDirectoryFmt, statusHandler);
        });

        //COMPARE DIRECTORIES
        CompareProcess cmpProc(globSettings.fileTimeTolerance,
                               globSettings.optDialogs,
                               allowPwPrompt,
                               globSettings.runWithBackgroundPriority,
                               statusHandler);

        FolderComparison folderCmp;
        cmpProc.startCompareProcess(cmpConfig, folderCmp);

        //START SYNCHRONIZATION
        SyncProcess syncProc(
            globSettings.optDialogs,
            globSettings.verifyFileCopy,
            globSettings.copyLockedFiles,
            globSettings.copyFilePermissions,
            globSettings.transactionalFileCopy,
            globSettings.runWithBackgroundPriority,
            statusHandler);

        const std::vector<FolderPairSyncCfg> syncProcessCfg = extractSyncCfg(batchCfg.mainCfg);
        assert(syncProcessCfg.size() == folderCmp.size());

        syncProc.startSynchronizationProcess(syncProcessCfg, folderCmp);

        //play (optional) sound notification after sync has completed -> don't play in silent mode, consider RealtimeSync!
        if (batchCfg.showProgress)
        {
            const wxString soundFile = toWx(getResourceDir()) + wxT("Sync_Complete.wav");
            if (fileExists(toZ(soundFile)))
                wxSound::Play(soundFile, wxSOUND_ASYNC); //warning: this may fail and show a wxWidgets error message! => must not play when running FFS as a service!
        }
    }
    catch (BatchAbortProcess&)  //exit used by statusHandler
    {
        if (returnValue >= 0)
            returnValue = -12;
        return;
    }
}
