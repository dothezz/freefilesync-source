// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) Zenju (zenju AT gmx DOT de) - All Rights Reserved        *
// **************************************************************************

#include "application.h"
#include <memory>
#include <zen/file_handling.h>
#include <wx/msgdlg.h>
#include <wx/tooltip.h> //wxWidgets v2.9
#include <wx/log.h>
#include <wx+/app_main.h>
#include <wx+/string_conv.h>
#include "comparison.h"
#include "algorithm.h"
#include "synchronization.h"
#include "ui/batch_status_handler.h"
#include "ui/check_version.h"
#include "ui/main_dlg.h"
#include "ui/switch_to_gui.h"
#include "lib/resources.h"
#include "lib/process_xml.h"
#include "lib/error_log.h"

#ifdef FFS_WIN
#include <zen/win_ver.h>

#elif defined FFS_LINUX
#include <gtk/gtk.h>

#elif defined FFS_MAC
#include <ApplicationServices/ApplicationServices.h>
#endif

using namespace zen;
using namespace xmlAccess;


IMPLEMENT_APP(Application)

namespace
{
boost::thread::id mainThreadId = boost::this_thread::get_id();

void onTerminationRequested()
{
    std::wstring msg = boost::this_thread::get_id() == mainThreadId ?
                       L"Termination requested in main thread!\n\n" :
                       L"Termination requested in worker thread!\n\n";
    msg += L"Please take a screenshot and file a bug report at: http://sourceforge.net/projects/freefilesync";

    wxSafeShowMessage(_("An exception occurred!"), msg);
    std::abort();
}
#ifdef _MSC_VER
void crtInvalidParameterHandler(const wchar_t* expression, const wchar_t* function, const wchar_t* file, unsigned int line, uintptr_t pReserved) { assert(false); }
#endif


std::vector<Zstring> getCommandlineArgs(const wxApp& app)
{
    std::vector<Zstring> args;
#ifdef FFS_WIN
    //we do the job ourselves! both wxWidgets and ::CommandLineToArgvW() parse "C:\" "D:\" as single line C:\" D:\"
    //-> "solution": we just don't support protected quotation mark!
    Zstring cmdLine = ::GetCommandLine(); //only way to get a unicode commandline
    while (endsWith(cmdLine, L' ')) //may end with space
        cmdLine.resize(cmdLine.size() - 1);

    auto iterStart = cmdLine.end(); //end() means: no token
    for (auto it = cmdLine.begin(); it != cmdLine.end(); ++it)
        if (*it == L' ') //space commits token
        {
            if (iterStart != cmdLine.end())
            {
                args.push_back(Zstring(iterStart, it));
                iterStart = cmdLine.end(); //expect consecutive blanks!
            }
        }
        else
        {
            //start new token
            if (iterStart == cmdLine.end())
                iterStart = it;

            if (*it == L'\"')
            {
                it = std::find(it + 1, cmdLine.end(), L'\"');
                if (it == cmdLine.end())
                    break;
            }
        }
    if (iterStart != cmdLine.end())
        args.push_back(Zstring(iterStart, cmdLine.end()));

    if (!args.empty())
        args.erase(args.begin()); //remove first argument which is exe path by convention: http://blogs.msdn.com/b/oldnewthing/archive/2006/05/15/597984.aspx

    std::for_each(args.begin(), args.end(),
                  [](Zstring& str)
    {
        if (str.size() >= 2 && startsWith(str, L'\"') && endsWith(str, L'\"'))
            str = Zstring(str.c_str() + 1, str.size() - 2);
    });

#else
    for (int i = 1; i < app.argc; ++i) //wxWidgets screws up once again making "argv implicitly convertible to a wxChar**" in 2.9.3,
        args.push_back(toZ(wxString(app.argv[i])));   //so we are forced to use this pitiful excuse for a range construction!!
#endif
    return args;
}

const wxEventType EVENT_ENTER_EVENT_LOOP = wxNewEventType();
}

//##################################################################################################################

bool Application::OnInit()
{
    std::set_terminate(onTerminationRequested); //unlike wxWidgets uncaught exception handling, this works for all worker threads

#ifdef FFS_WIN
#ifdef _MSC_VER
    _set_invalid_parameter_handler(crtInvalidParameterHandler); //see comment in <zen/time.h>
#endif
    //Quote: "Best practice is that all applications call the process-wide ::SetErrorMode() function with a parameter of
    //SEM_FAILCRITICALERRORS at startup. This is to prevent error mode dialogs from hanging the application."
    ::SetErrorMode(SEM_FAILCRITICALERRORS);

#elif defined FFS_LINUX
    ::gtk_init(nullptr, nullptr);
    ::gtk_rc_parse((getResourceDir() + "styles.gtk_rc").c_str()); //remove inner border from bitmap buttons

#elif defined FFS_MAC
    ProcessSerialNumber psn = { 0, kCurrentProcess };
    ::TransformProcessType(&psn, kProcessTransformToForegroundApplication); //behave like an application bundle, even when the app is not packaged (yet)
#endif

#if wxCHECK_VERSION(2, 9, 1)
#ifdef FFS_WIN
    wxToolTip::SetMaxWidth(-1); //disable tooltip wrapping -> Windows only
#endif
    wxToolTip::SetAutoPop(7000); //tooltip visibilty in ms, 5s is default for Windows: http://msdn.microsoft.com/en-us/library/windows/desktop/aa511495.aspx
#endif

    SetAppName(L"FreeFileSync"); //if not set, the default is the executable's name!

    Connect(wxEVT_QUERY_END_SESSION, wxEventHandler(Application::onQueryEndSession), nullptr, this);
    Connect(wxEVT_END_SESSION,       wxEventHandler(Application::onQueryEndSession), nullptr, this);

    //do not call wxApp::OnInit() to avoid using wxWidgets command line parser

    //Note: app start is deferred: batch mode requires the wxApp eventhandler to be established for UI update events. This is not the case at the time of OnInit()!
    Connect(EVENT_ENTER_EVENT_LOOP, wxEventHandler(Application::onEnterEventLoop), nullptr, this);
    wxCommandEvent scrollEvent(EVENT_ENTER_EVENT_LOOP);
    AddPendingEvent(scrollEvent);

    return true; //true: continue processing; false: exit immediately.
}


int Application::OnExit()
{
    releaseWxLocale();
    return wxApp::OnExit();
}


void Application::onEnterEventLoop(wxEvent& event)
{
    Disconnect(EVENT_ENTER_EVENT_LOOP, wxEventHandler(Application::onEnterEventLoop), nullptr, this);

    //determine FFS mode of operation
    std::vector<Zstring> commandArgs = getCommandlineArgs(*this);
    launch(commandArgs);
}

warn_static("finish")

#ifdef FFS_MAC
/*
Initialization call sequences on OS X
-------------------------------------
1. double click FFS app bundle or execute from command line without arguments
	OnInit()
	OnRun()
	onEnterEventLoop()
	MacNewFile()

2. double-click .ffs_gui file
	OnInit()
	OnRun()
	onEnterEventLoop()
	MacOpenFiles()

3. start from command line with .ffs_gui file as first argument
	OnInit()
	OnRun()
	MacOpenFiles() -> WTF!?
	onEnterEventLoop()
	MacNewFile()   -> yes, wxWidgets screws up once again: http://trac.wxwidgets.org/ticket/14558
*/
void Application::MacOpenFiles(const wxArrayString& filenames)
{

    //	long wxExecute(const wxString& command, int sync = wxEXEC_ASYNC, wxProcess *callback = NULL)

    //  std::vector<wxString>(filenames.begin(), filenames.end())
    //	startApplication(commandArgs);

    //if (!fileNames.empty())
    //	wxMessageBox(fileNames[0]);
    wxApp::MacOpenFiles(filenames);
}


void Application::MacNewFile()
{
    wxApp::MacNewFile();
}

//virtual void wxApp::MacReopenApp 	( 		)
#endif


int Application::OnRun()
{
    auto processException = [](const std::wstring& msg)
    {
        //it's not always possible to display a message box, e.g. corrupted stack, however low-level file output works!
        logError(utfCvrtTo<std::string>(msg));
        wxSafeShowMessage(_("An exception occurred!") + L" - FFS", msg);
    };

    try
    {
        wxApp::OnRun();
    }
    catch (const std::exception& e) //catch all STL exceptions
    {
        processException(utfCvrtTo<std::wstring>(e.what()));
        return FFS_RC_EXCEPTION;
    }
    catch (...) //catch the rest
    {
        processException(L"Unknown error.");
        return FFS_RC_EXCEPTION;
    }

    return returnCode;
}


void Application::onQueryEndSession(wxEvent& event)
{
    //alas wxWidgets screws up once again: http://trac.wxwidgets.org/ticket/3069
    if (auto mainWin = dynamic_cast<MainDialog*>(GetTopWindow()))
        mainWin->onQueryEndSession();
    OnExit();
    //wxEntryCleanup(); -> gives popup "dll init failed" on XP
    std::exit(returnCode); //Windows will terminate anyway: destruct global objects
}


void runGuiMode(const xmlAccess::XmlGuiConfig& guiCfg);
void runGuiMode(const std::vector<Zstring>& cfgFileName);
void runBatchMode(const Zstring& filename, FfsReturnCode& returnCode);


void Application::launch(const std::vector<Zstring>& commandArgs)
{
    //wxWidgets app exit handling is weird... we want the app to exit only if the logical main window is closed
    wxTheApp->SetExitOnFrameDelete(false); //avoid popup-windows from becoming temporary top windows leading to program exit after closure
    auto app = wxTheApp; //fix lambda/wxWigets/VC fuck up
    ZEN_ON_SCOPE_EXIT(if (!mainWindowWasSet()) app->ExitMainLoop();); //quit application, if no main window was set (batch silent mode)

    try
    {
        //tentatively set program language to OS default until GlobalSettings.xml is read later
        setLanguage(xmlAccess::XmlGlobalSettings().programLanguage); //throw FileError
    }
    catch (const FileError&) {} //no messagebox: consider batch job!

    if (commandArgs.empty())
        runGuiMode(commandArgs);
    else
    {
        const bool gotDirNames = std::any_of(commandArgs.begin(), commandArgs.end(), [](const Zstring& dirname) { return dirExists(dirname); });
        if (gotDirNames) //mode 1: create temp configuration based on directory names passed
        {
            XmlGuiConfig guiCfg;
            guiCfg.mainCfg.syncCfg.directionCfg.var = DirectionConfig::MIRROR;

            for (auto it = commandArgs.begin(); it != commandArgs.end(); ++it)
            {
                size_t index = it - commandArgs.begin();

                FolderPairEnh* fp = nullptr;
                if (index < 2)
                    fp = &guiCfg.mainCfg.firstPair;
                else
                {
                    guiCfg.mainCfg.additionalPairs.resize((index - 2) / 2 + 1);
                    fp = &guiCfg.mainCfg.additionalPairs.back();
                }

                if (index % 2 == 0)
                    fp->leftDirectory = *it;
                else
                    fp->rightDirectory = *it;
            }

            runGuiMode(guiCfg);
        }
        else //mode 2: try to set config/batch-filename set by %1 parameter
        {
            std::vector<Zstring> argsTmp = commandArgs;

            for (auto it = argsTmp.begin(); it != argsTmp.end(); ++it)
            {
                const Zstring& filename = *it;

                if (!fileExists(filename)) //...be a little tolerant
                {
                    if (fileExists(filename + Zstr(".ffs_batch")))
                        *it += Zstr(".ffs_batch");
                    else if (fileExists(filename + Zstr(".ffs_gui")))
                        *it += Zstr(".ffs_gui");
                    else
                    {
                        wxMessageBox(replaceCpy(_("Cannot find file %x."), L"%x", fmtFileName(filename)), _("Error"), wxOK | wxICON_ERROR);
                        return;
                    }
                }
            }

            switch (getMergeType(argsTmp)) //throw ()
            {
                case MERGE_BATCH: //pure batch config files
                    if (argsTmp.size() == 1)
                        runBatchMode(argsTmp[0], returnCode);
                    else
                        runGuiMode(argsTmp);
                    break;

                case MERGE_GUI:       //pure gui config files
                case MERGE_GUI_BATCH: //gui and batch files
                    runGuiMode(argsTmp);
                    break;

                case MERGE_OTHER: //= none or unknown;
                    //argsTmp are not empty and contain at least one non-gui/non-batch config file: find it!
                    std::find_if(argsTmp.begin(), argsTmp.end(),
                                 [](const Zstring& filename) -> bool
                    {
                        switch (getXmlType(filename)) //throw()
                        {
                            case XML_TYPE_GLOBAL:
                            case XML_TYPE_OTHER:
                                wxMessageBox(replaceCpy(_("File %x does not contain a valid configuration."), L"%x", fmtFileName(filename)), _("Error"), wxOK | wxICON_ERROR);
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


void runGuiMode(const xmlAccess::XmlGuiConfig& guiCfg)
{
    MainDialog::create(guiCfg, true);
}


void runGuiMode(const std::vector<Zstring>& cfgFileNames)
{
    MainDialog::create(cfgFileNames);
}


void runBatchMode(const Zstring& filename, FfsReturnCode& returnCode)
{
    //load XML settings
    XmlBatchConfig batchCfg;
    try
    {
        readConfig(filename, batchCfg);
    }
    catch (const xmlAccess::FfsXmlError& e)
    {
        wxMessageBox(e.toString(), _("Error"), wxOK | wxICON_ERROR); //batch mode: break on errors AND even warnings!
        raiseReturnCode(returnCode, FFS_RC_ABORTED);
        return;
    }

    auto notifyError = [&](const std::wstring& msg)
    {
        if (batchCfg.handleError == ON_ERROR_POPUP)
            wxMessageBox(msg.c_str(), _("Error"), wxOK | wxICON_ERROR);
        else //"exit" or "ignore"
            logError(utfCvrtTo<std::string>(msg));

        raiseReturnCode(returnCode, FFS_RC_FINISHED_WITH_ERRORS);
    };

    XmlGlobalSettings globalCfg;
    try
    {
        if (fileExists(getGlobalConfigFile()))
            readConfig(globalCfg); //throw FfsXmlError
        //else: globalCfg already has default values
    }
    catch (const xmlAccess::FfsXmlError& e)
    {
        assert(false);
        if (e.getSeverity() != FfsXmlError::WARNING) //ignore parsing errors: should be migration problems only *cross-fingers*
            return notifyError(e.toString()); //abort sync!
    }

    try
    {
        setLanguage(globalCfg.programLanguage); //throw FileError
    }
    catch (const FileError& e)
    {
        notifyError(e.toString());
        //continue!
    }

    //all settings have been read successfully...

    //regular check for program updates -> disabled for batch
    //if (batchCfg.showProgress && manualProgramUpdateRequired())
    //    checkForUpdatePeriodically(globalCfg.lastUpdateCheck);

    try //begin of synchronization process (all in one try-catch block)
    {

        const TimeComp timeStamp = localTime();

        const SwitchToGui switchBatchToGui(filename, batchCfg, globalCfg); //prepare potential operational switch

        //class handling status updates and error messages
        BatchStatusHandler statusHandler(batchCfg.showProgress, //throw BatchAbortProcess
                                         extractJobName(filename),
                                         timeStamp,
                                         batchCfg.logFileDirectory,
                                         batchCfg.logfilesCountLimit,
                                         globalCfg.lastSyncsLogFileSizeMax,
                                         batchCfg.handleError,
                                         switchBatchToGui,
                                         returnCode,
                                         batchCfg.mainCfg.onCompletion,
                                         globalCfg.gui.onCompletionHistory);

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
        std::unique_ptr<LockHolder> dirLocks;

        //COMPARE DIRECTORIES
        FolderComparison folderCmp;
        compare(globalCfg.fileTimeTolerance,
                globalCfg.optDialogs,
                allowPwPrompt,
                globalCfg.runWithBackgroundPriority,
                globalCfg.createLockFile,
                dirLocks,
                cmpConfig,
                folderCmp,
                statusHandler);

        //START SYNCHRONIZATION
        const std::vector<FolderPairSyncCfg> syncProcessCfg = extractSyncCfg(batchCfg.mainCfg);
        if (syncProcessCfg.size() != folderCmp.size())
            throw std::logic_error("Programming Error: Contract violation! " + std::string(__FILE__) + ":" + numberTo<std::string>(__LINE__));

        synchronize(timeStamp,
                    globalCfg.optDialogs,
                    globalCfg.verifyFileCopy,
                    globalCfg.copyLockedFiles,
                    globalCfg.copyFilePermissions,
                    globalCfg.transactionalFileCopy,
                    globalCfg.runWithBackgroundPriority,
                    syncProcessCfg,
                    folderCmp,
                    statusHandler);
    }
    catch (BatchAbortProcess&) {} //exit used by statusHandler

    try //save global settings to XML: e.g. ignored warnings
    {
        xmlAccess::writeConfig(globalCfg); //FfsXmlError
    }
    catch (const xmlAccess::FfsXmlError& e)
    {
        notifyError(e.toString());
    }
}
