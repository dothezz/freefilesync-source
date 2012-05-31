// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) ZenJu (zhnmju123 AT gmx DOT de) - All Rights Reserved    *
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
    msg += L"Please take a screenshot and file a bug report at: http://sourceforge.net/projects/freefilesync";

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

    returnCode = FFS_RC_SUCCESS;
    //do not call wxApp::OnInit() to avoid using default commandline parser

    //Note: initialization is done in the FIRST idle event instead of OnInit. Reason: batch mode requires the wxApp eventhandler to be established
    //for UI update events. This is not the case at the time of OnInit().
    Connect(wxEVT_IDLE,              wxIdleEventHandler(Application::OnStartApplication), nullptr, this);
    Connect(wxEVT_QUERY_END_SESSION, wxEventHandler    (Application::OnQueryEndSession ), nullptr, this);
    Connect(wxEVT_END_SESSION,       wxEventHandler    (Application::OnQueryEndSession ), nullptr, this);

    return true;
}


std::vector<wxString> getCommandlineArgs(const wxApp& app)
{
    std::vector<wxString> args;
#ifdef FFS_WIN
    //we do the job ourselves! both wxWidgets and ::CommandLineToArgvW() parse "C:\" "D:\" as single line C:\" D:\"
    //-> "solution": we just don't support protected quotation mark!
    std::wstring cmdLine = ::GetCommandLine(); //only way to get a unicode commandline
    while (endsWith(cmdLine, L' ')) //may end with space
        cmdLine.resize(cmdLine.size() - 1);

    auto iterStart = cmdLine.end(); //end() means: no token
    for (auto iter = cmdLine.begin(); iter != cmdLine.end(); ++iter)
        if (*iter == L' ') //space commits token
        {
            if (iterStart != cmdLine.end())
            {
                args.push_back(std::wstring(iterStart, iter));
                iterStart = cmdLine.end(); //expect consecutive blanks!
            }
        }
        else
        {
            //start new token
            if (iterStart == cmdLine.end())
                iterStart = iter;

            if (*iter == L'\"')
            {
                iter = std::find(iter + 1, cmdLine.end(), L'\"');
                if (iter == cmdLine.end())
                    break;
            }
        }
    if (iterStart != cmdLine.end())
        args.push_back(std::wstring(iterStart, cmdLine.end()));

    if (!args.empty())
        args.erase(args.begin()); //remove first argument which is exe path by convention: http://blogs.msdn.com/b/oldnewthing/archive/2006/05/15/597984.aspx

    std::for_each(args.begin(), args.end(),
                  [](wxString& str)
    {
        if (str.size() >= 2 && startsWith(str, L'\"') && endsWith(str, L'\"'))
            str = wxString(str.c_str() + 1, str.size() - 2);
    });

#else
    for (int i = 1; i < app.argc; ++i) //wxWidgets screws up once again making "argv implicitly convertible to a wxChar**" in 2.9.3,
        args.push_back(app.argv[i]);   //so we are forced to use this pitiful excuse for a range construction!!
#endif
    return args;
}


void Application::OnStartApplication(wxIdleEvent&)
{
    Disconnect(wxEVT_IDLE, wxIdleEventHandler(Application::OnStartApplication), nullptr, this);

    //wxWidgets app exit handling is weird... we want the app to exit only if the logical main window is closed
    wxTheApp->SetExitOnFrameDelete(false); //avoid popup-windows from becoming temporary top windows leading to program exit after closure
    auto app = wxTheApp; //fix lambda/wxWigets/VC fuck up
    ZEN_ON_SCOPE_EXIT(if (!mainWindowWasSet()) app->ExitMainLoop();); //quit application, if no main window was set (batch silent mode)

    //if appname is not set, the default is the executable's name!
    SetAppName(L"FreeFileSync");

#ifdef FFS_WIN
    //Quote: "Best practice is that all applications call the process-wide SetErrorMode function with a parameter of
    //SEM_FAILCRITICALERRORS at startup. This is to prevent error mode dialogs from hanging the application."
    ::SetErrorMode(SEM_FAILCRITICALERRORS);

#elif defined FFS_LINUX
    ::gtk_init(nullptr, nullptr);
    ::gtk_rc_parse((getResourceDir() + "styles.rc").c_str()); //remove inner border from bitmap buttons
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

    //determine FFS mode of operation
    std::vector<wxString> commandArgs = getCommandlineArgs(*this);

    if (commandArgs.empty())
        runGuiMode(commandArgs, globalSettings);
    else
    {
        const bool gotDirNames = std::any_of(commandArgs.begin(), commandArgs.end(), [](const wxString& dirname) { return dirExists(toZ(dirname)); });
        if (gotDirNames) //mode 1: create temp configuration based on directory names passed
        {
            XmlGuiConfig guiCfg;
            guiCfg.mainCfg.syncCfg.directionCfg.var = DirectionConfig::MIRROR;

            for (auto iter = commandArgs.begin(); iter != commandArgs.end(); ++iter)
            {
                size_t index = iter - commandArgs.begin();

                FolderPairEnh& fp = [&]() -> FolderPairEnh&
                {
                    if (index < 2)
                        return guiCfg.mainCfg.firstPair;

                    guiCfg.mainCfg.additionalPairs.resize((index - 2) / 2 + 1);
                    return guiCfg.mainCfg.additionalPairs.back();
                }();

                if (index % 2 == 0)
                    fp.leftDirectory = toZ(*iter);
                else
                    fp.rightDirectory = toZ(*iter);
            }

            runGuiMode(guiCfg, globalSettings);
        }
        else //mode 2: try to set config/batch-filename set by %1 parameter
        {
            for (auto iter = commandArgs.begin(); iter != commandArgs.end(); ++iter)
            {
                const Zstring& filename = toZ(*iter);

                if (!fileExists(filename)) //be a little tolerant
                {
                    if (fileExists(filename + Zstr(".ffs_batch")))
                        *iter += L".ffs_batch";
                    else if (fileExists(filename + Zstr(".ffs_gui")))
                        *iter += L".ffs_gui";
                    else
                    {
                        wxMessageBox(replaceCpy(_("Cannot find file %x."), L"%x", fmtFileName(filename)), _("Error"), wxOK | wxICON_ERROR);
                        return;
                    }
                }
            }

            switch (getMergeType(toZ(commandArgs))) //throw ()
            {
                case MERGE_BATCH: //pure batch config files
                    if (commandArgs.size() == 1)
                        runBatchMode(utfCvrtTo<Zstring>(commandArgs[0]), globalSettings);
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
                        switch (getXmlType(toZ(filename))) //throw()
                        {
                            case XML_TYPE_GLOBAL:
                            case XML_TYPE_OTHER:
                                wxMessageBox(replaceCpy(_("File %x does not contain a valid configuration."), L"%x", fmtFileName(toZ(filename))), _("Error"), wxOK | wxICON_ERROR);
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
        //it's not always possible to display a message box, e.g. corrupted stack, however (non-stream) file output works!
        wxFile safeOutput(toWx(getConfigDir()) + L"LastError.txt", wxFile::write);
        safeOutput.Write(utfCvrtTo<wxString>(e.what()));

        wxSafeShowMessage(_("An exception occurred!") + L" - FFS", utfCvrtTo<wxString>(e.what()));
        return FFS_RC_EXCEPTION;
    }
    catch (...) //catch the rest
    {
        const wxString& msg = L"Unknown error.";

        wxFile safeOutput(toWx(getConfigDir()) + L"LastError.txt", wxFile::write);
        safeOutput.Write(msg);

        wxSafeShowMessage(_("An exception occurred!"), msg);
        return FFS_RC_EXCEPTION;
    }

    return returnCode;
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
    if (auto mainWin = dynamic_cast<MainDialog*>(GetTopWindow()))
        mainWin->onQueryEndSession();
    OnExit();
    //wxEntryCleanup(); -> gives popup "dll init failed" on XP
    std::exit(returnCode); //Windows will terminate anyway: destruct global objects
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


void Application::runBatchMode(const Zstring& filename, xmlAccess::XmlGlobalSettings& globSettings)
{
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
        const std::wstring timestamp = formatTime<std::wstring>(L"%Y-%m-%d %H%M%S");

        const SwitchToGui switchBatchToGui(utfCvrtTo<wxString>(filename), batchCfg, globSettings); //prepare potential operational switch

        //class handling status updates and error messages
        BatchStatusHandler statusHandler(batchCfg.showProgress,
                                         extractJobName(filename),
                                         timestamp,
                                         batchCfg.logFileDirectory,
                                         batchCfg.logFileCountMax,
                                         batchCfg.handleError,
                                         switchBatchToGui,
                                         returnCode,
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

        std::unique_ptr<LockHolder> dummy;
        if (globSettings.createLockFile)
        {
            dummy.reset(new LockHolder(allowPwPrompt));
            std::for_each(cmpConfig.begin(), cmpConfig.end(),
                          [&](const FolderPairCfg& fpCfg)
            {
                dummy->addDir(fpCfg.leftDirectoryFmt,  statusHandler);
                dummy->addDir(fpCfg.rightDirectoryFmt, statusHandler);
            });
        }

        //COMPARE DIRECTORIES
        CompareProcess cmpProc(globSettings.fileTimeTolerance,
                               globSettings.optDialogs,
                               allowPwPrompt,
                               globSettings.runWithBackgroundPriority,
                               statusHandler);

        FolderComparison folderCmp;
        cmpProc.startCompareProcess(cmpConfig, folderCmp);

        //START SYNCHRONIZATION
        SyncProcess syncProc(extractJobName(filename),
                             timestamp,
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
            const wxString soundFile = toWx(getResourceDir()) + L"Sync_Complete.wav";
            if (fileExists(toZ(soundFile)))
                wxSound::Play(soundFile, wxSOUND_ASYNC); //warning: this may fail and show a wxWidgets error message! => must not play when running FFS as a service!
        }
    }
    catch (BatchAbortProcess&) {} //exit used by statusHandler
}
