// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) ZenJu (zhnmju123 AT gmx DOT de) - All Rights Reserved    *
// **************************************************************************

#include "application.h"
#include "main_dlg.h"
#include <wx/event.h>
#include "resources.h"
#include <wx/msgdlg.h>
#include "../lib/localization.h"
#include "xml_ffs.h"
#include "../lib/ffs_paths.h"
#include <wx/file.h>
#include <wx+/string_conv.h>
#include <wx/log.h>
#include <zen/file_handling.h>

#ifdef FFS_LINUX
#include <gtk/gtk.h>
#endif

using namespace zen;


IMPLEMENT_APP(Application);


bool Application::OnInit()
{
    //do not call wxApp::OnInit() to avoid using default commandline parser

    //Note: initialization is done in the FIRST idle event instead of OnInit. Reason: Commandline mode requires the wxApp eventhandler to be established
    //for UI update events. This is not the case at the time of OnInit().
    Connect(wxEVT_IDLE, wxIdleEventHandler(Application::OnStartApplication), nullptr, this);

    return true;
}


void Application::OnStartApplication(wxIdleEvent& event)
{
    Disconnect(wxEVT_IDLE, wxIdleEventHandler(Application::OnStartApplication), nullptr, this);

    //if appname is not set, the default is the executable's name!
    SetAppName(wxT("FreeFileSync")); //use a different app name, to have "GetUserDataDir()" return the same directory as for FreeFileSync

#ifdef FFS_WIN
    //Quote: "Best practice is that all applications call the process-wide SetErrorMode function with a parameter of
    //SEM_FAILCRITICALERRORS at startup. This is to prevent error mode dialogs from hanging the application."
    ::SetErrorMode(SEM_FAILCRITICALERRORS);

#elif defined FFS_LINUX
    ::gtk_rc_parse((zen::getResourceDir() + "styles.rc").c_str()); //remove inner border from bitmap buttons
#endif

    //set program language
    zen::setLanguage(rts::getProgramLanguage());

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
    frame->SetIcon(GlobalResources::instance().programIcon); //set application icon
    frame->Show();
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

        wxSafeShowMessage(_("An exception occurred!") + L" - RTS", utfCvrtTo<wxString>(e.what()));
        return -9;
    }
    catch (...) //catch the rest
    {
        const wxString& msg = L"Unknown error.";

        wxFile safeOutput(toWx(getConfigDir()) + L"LastError.txt", wxFile::write);
        safeOutput.Write(msg);

        wxSafeShowMessage(_("An exception occurred!"), msg);
        return -9;
    }

    return 0; //program's return code
}

