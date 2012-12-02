// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) Zenju (zenju AT gmx DOT de) - All Rights Reserved        *
// **************************************************************************

#include "application.h"
#include <memory>
#include "ui/main_dlg.h"
#include <wx/msgdlg.h>
#include <wx/tooltip.h> //wxWidgets v2.9
#include <wx/log.h>
#include <wx+/app_main.h>
#include "comparison.h"
#include "algorithm.h"
#include "synchronization.h"
#include "ui/batch_status_handler.h"
#include "ui/check_version.h"
#include "ui/switch_to_gui.h"
#include "lib/resources.h"
#include "lib/lock_holder.h"
#include "lib/process_xml.h"
#include "lib/error_log.h"

#ifdef FFS_WIN
#include <zen/win_ver.h>
#elif defined FFS_LINUX
#include <gtk/gtk.h>
#endif

using namespace zen;
using namespace xmlAccess;


IMPLEMENT_APP(Application)

void runGuiMode(const xmlAccess::XmlGuiConfig& guiCfg);
void runGuiMode(const std::vector<wxString>& cfgFileName);
void runBatchMode(const Zstring& filename, FfsReturnCode& returnCode);


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

#ifdef _MSC_VER
void crtInvalidParameterHandler(const wchar_t* expression, const wchar_t* function, const wchar_t* file, unsigned int line, uintptr_t pReserved) { assert(false); }
#endif
}
#endif 


bool Application::OnInit()
{
#ifdef FFS_WIN
    std::set_terminate(onTerminationRequested); //unlike wxWidgets uncaught exception handling, this works for all worker threads
    assert(!win8OrLater()); //another breadcrumb: test and add new OS entry to "compatibility" in application manifest
#ifdef _MSC_VER
    _set_invalid_parameter_handler(crtInvalidParameterHandler); //see comment in <zen/time.h>
#endif
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
    //Quote: "Best practice is that all applications call the process-wide ::SetErrorMode() function with a parameter of
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

    try
    {
        //tentatively set program language to OS default until GlobalSettings.xml is read later
        setLanguage(xmlAccess::XmlGlobalSettings().programLanguage); //throw FileError
    }
    catch (const FileError&) {} //no messagebox: consider batch job!

    //determine FFS mode of operation
    std::vector<wxString> commandArgs = getCommandlineArgs(*this);

    if (commandArgs.empty())
        runGuiMode(commandArgs);
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

            runGuiMode(guiCfg);
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
                        runBatchMode(utfCvrtTo<Zstring>(commandArgs[0]), returnCode);
                    else
                        runGuiMode(commandArgs);
                    break;

                case MERGE_GUI:       //pure gui config files
                case MERGE_GUI_BATCH: //gui and batch files
                    runGuiMode(commandArgs);
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


void Application::OnQueryEndSession(wxEvent& event)
{
    //alas wxWidgets screws up once again: http://trac.wxwidgets.org/ticket/3069
    if (auto mainWin = dynamic_cast<MainDialog*>(GetTopWindow()))
        mainWin->onQueryEndSession();
    OnExit();
    //wxEntryCleanup(); -> gives popup "dll init failed" on XP
    std::exit(returnCode); //Windows will terminate anyway: destruct global objects
}


void runGuiMode(const xmlAccess::XmlGuiConfig& guiCfg)
{
    MainDialog::create(guiCfg, true);
}


void runGuiMode(const std::vector<wxString>& cfgFileNames)
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
        if (fileExists(toZ(getGlobalConfigFile())))
            readConfig(globalCfg); //throw FfsXmlError
        //else: globalCfg already has default values
    }
    catch (const xmlAccess::FfsXmlError& e)
    {
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
    //if (batchCfg.showProgress)
    //    checkForUpdatePeriodically(globalCfg.lastUpdateCheck);

    try //begin of synchronization process (all in one try-catch block)
    {

        const TimeComp timeStamp = localTime();

        const SwitchToGui switchBatchToGui(utfCvrtTo<wxString>(filename), batchCfg, globalCfg); //prepare potential operational switch

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

        std::unique_ptr<LockHolder> dummy;
        if (globalCfg.createLockFile)
        {
            std::vector<Zstring> dirnames;
            std::for_each(cmpConfig.begin(), cmpConfig.end(),
                          [&](const FolderPairCfg& fpCfg)
            {
                dirnames.push_back(fpCfg.leftDirectoryFmt);
                dirnames.push_back(fpCfg.rightDirectoryFmt);
            });
            dummy = make_unique<LockHolder>(dirnames, statusHandler, allowPwPrompt);
        }

        //COMPARE DIRECTORIES
        FolderComparison folderCmp;
        compare(globalCfg.fileTimeTolerance,
                globalCfg.optDialogs,
                allowPwPrompt,
                globalCfg.runWithBackgroundPriority,
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
