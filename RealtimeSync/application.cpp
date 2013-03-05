// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) Zenju (zenju AT gmx DOT de) - All Rights Reserved        *
// **************************************************************************

#include "application.h"
#include "main_dlg.h"
#include <zen/file_handling.h>
#include <wx/event.h>
#include <wx/log.h>
#include <wx/msgdlg.h>
#include <wx+/string_conv.h>
//#include "resources.h"
#include "xml_ffs.h"
#include "../lib/localization.h"
#include "../lib/ffs_paths.h"
#include "../lib/return_codes.h"
#include "lib/error_log.h"

#ifdef FFS_WIN
#include <zen/win_ver.h>
#elif defined FFS_LINUX
#include <gtk/gtk.h>
#endif

using namespace zen;


IMPLEMENT_APP(Application);

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

    //do not call wxApp::OnInit() to avoid using default commandline parser

    //Note: initialization is done in the FIRST idle event instead of OnInit. Reason: Commandline mode requires the wxApp eventhandler to be established
    //for UI update events. This is not the case at the time of OnInit().
    Connect(wxEVT_IDLE, wxIdleEventHandler(Application::OnStartApplication), nullptr, this);

    return true;
}


void Application::OnStartApplication(wxIdleEvent& event)
{
    Disconnect(wxEVT_IDLE, wxIdleEventHandler(Application::OnStartApplication), nullptr, this);

    warn_static("fix")

    //if appname is not set, the default is the executable's name!
    SetAppName(L"FreeFileSync"); //abuse FFS's name, to have "GetUserDataDir()" return the same directory

#ifdef FFS_WIN
    //Quote: "Best practice is that all applications call the process-wide SetErrorMode function with a parameter of
    //SEM_FAILCRITICALERRORS at startup. This is to prevent error mode dialogs from hanging the application."
    ::SetErrorMode(SEM_FAILCRITICALERRORS);
#elif defined FFS_LINUX
    ::gtk_rc_parse((zen::getResourceDir() + "styles.gtk_rc").c_str()); //remove inner border from bitmap buttons
#endif

    //set program language
    try
    {
        setLanguage(rts::getProgramLanguage()); //throw FileError
    }
    catch (const FileError& e)
    {
        wxMessageBox(e.toString(), _("Error"), wxOK | wxICON_ERROR);
        //continue!
    }

    //try to set config/batch-filename set by %1 parameter
    std::vector<wxString> commandArgs;
    for (int i = 1; i < argc; ++i)
    {
        Zstring filename = toZ(argv[i]);

        if (!fileExists(filename)) //be a little tolerant
        {
            if (fileExists(filename + Zstr(".ffs_real")))
                filename += Zstr(".ffs_real");
            else if (fileExists(filename + Zstr(".ffs_batch")))
                filename += Zstr(".ffs_batch");
            else
            {
                wxMessageBox(replaceCpy(_("Cannot find file %x."), L"%x", fmtFileName(filename)), _("Error"), wxOK | wxICON_ERROR);
                return;
            }
        }
        commandArgs.push_back(toWx(filename));
    }

    wxString cfgFilename;
    if (!commandArgs.empty())
        cfgFilename = commandArgs[0];

    MainDialog* frame = new MainDialog(nullptr, cfgFilename);
    frame->Show();
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

    return FFS_RC_SUCCESS; //program's return code
}

