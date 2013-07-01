// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) Zenju (zenju AT gmx DOT de) - All Rights Reserved        *
// **************************************************************************

#include "application.h"
#include "main_dlg.h"
#include <zen/file_handling.h>
#include <zen/thread.h>
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

#ifdef ZEN_WIN
#include <zen/win_ver.h>

#elif defined ZEN_LINUX
#include <gtk/gtk.h>

#elif defined ZEN_MAC
#include <ApplicationServices/ApplicationServices.h>
#endif

using namespace zen;


IMPLEMENT_APP(Application);

namespace
{
boost::thread::id mainThreadId = boost::this_thread::get_id();

void onTerminationRequested()
{
    std::wstring msg = boost::this_thread::get_id() == mainThreadId ?
                       L"Termination requested in main thread!\n\n" :
                       L"Termination requested in worker thread!\n\n";
    msg += L"Please file a bug report at: http://sourceforge.net/projects/freefilesync";

    wxSafeShowMessage(_("An exception occurred"), msg);
    std::abort();
}

#ifdef _MSC_VER
void crtInvalidParameterHandler(const wchar_t* expression, const wchar_t* function, const wchar_t* file, unsigned int line, uintptr_t pReserved) { assert(false); }
#endif

const wxEventType EVENT_ENTER_EVENT_LOOP = wxNewEventType();
}


bool Application::OnInit()
{
    std::set_terminate(onTerminationRequested); //unlike wxWidgets uncaught exception handling, this works for all worker threads

#ifdef ZEN_WIN
#ifdef _MSC_VER
    _set_invalid_parameter_handler(crtInvalidParameterHandler); //see comment in <zen/time.h>
#endif
    //Quote: "Best practice is that all applications call the process-wide SetErrorMode function with a parameter of
    //SEM_FAILCRITICALERRORS at startup. This is to prevent error mode dialogs from hanging the application."
    ::SetErrorMode(SEM_FAILCRITICALERRORS);

#elif defined ZEN_LINUX
    ::gtk_rc_parse((zen::getResourceDir() + "styles.gtk_rc").c_str()); //remove inner border from bitmap buttons

#elif defined ZEN_MAC
    ProcessSerialNumber psn = { 0, kCurrentProcess };
    ::TransformProcessType(&psn, kProcessTransformToForegroundApplication); //behave like an application bundle, even when the app is not packaged (yet)
#endif

    SetAppName(L"FreeFileSync"); //reuse FFS's name, to have "GetUserDataDir()/GetResourcesDir()" return the same directory in ffs_paths.cpp

    //do not call wxApp::OnInit() to avoid using default commandline parser

    //Note: app start is deferred:  -> see FreeFileSync
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
        wxSafeShowMessage(_("An exception occurred"), msg);
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
