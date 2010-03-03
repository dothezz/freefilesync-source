// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) 2008-2010 ZenJu (zhnmju123 AT gmx.de)                    *
// **************************************************************************
//
#include "application.h"
#include "mainDialog.h"
#include <wx/event.h>
#include "resources.h"
#include <wx/msgdlg.h>
#include "../shared/localization.h"
#include "xmlFreeFileSync.h"
#include "../shared/standardPaths.h"
#include <wx/file.h>
#include "../shared/stringConv.h"

#ifdef FFS_LINUX
#include <gtk/gtk.h>
#endif

IMPLEMENT_APP(Application);

bool Application::OnInit()
{
//do not call wxApp::OnInit() to avoid using default commandline parser

//Note: initialization is done in the FIRST idle event instead of OnInit. Reason: Commandline mode requires the wxApp eventhandler to be established
//for UI update events. This is not the case at the time of OnInit().
    Connect(wxEVT_IDLE, wxIdleEventHandler(Application::OnStartApplication), NULL, this);

    return true;
}


void Application::OnStartApplication(wxIdleEvent& event)
{
    Disconnect(wxEVT_IDLE, wxIdleEventHandler(Application::OnStartApplication), NULL, this);

    //if appname is not set, the default is the executable's name!
    SetAppName(wxT("FreeFileSync")); //use a different app name, to have "GetUserDataDir()" return the same directory as for FreeFileSync

#ifdef FFS_LINUX
    ::gtk_rc_parse(FreeFileSync::wxToZ(FreeFileSync::getResourceDir()) + "styles.rc"); //remove inner border from bitmap buttons
#endif

    //set program language
    FreeFileSync::CustomLocale::getInstance().setLanguage(RealtimeSync::getProgramLanguage());

    //try to set config/batch-filename set by %1 parameter
    wxString cfgFilename;
    if (argc > 1)
    {
        const wxString filename(argv[1]);

        if (wxFileExists(filename))  //load file specified by %1 parameter:
            cfgFilename = filename;
        else if (wxFileExists(filename + wxT(".ffs_real")))
            cfgFilename = filename + wxT(".ffs_real");
        else if (wxFileExists(filename + wxT(".ffs_batch")))
            cfgFilename = filename + wxT(".ffs_batch");
        else
        {
            wxMessageBox(wxString(_("File does not exist:")) + wxT(" \"") + filename + wxT("\""), _("Error"), wxOK | wxICON_ERROR);
            return;
        }
    }

    GlobalResources::getInstance().load(); //loads bitmap resources on program startup

    MainDialog* frame = new MainDialog(NULL, cfgFilename);
    frame->SetIcon(*GlobalResources::getInstance().programIcon); //set application icon
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
        //unfortunately it's not always possible to display a message box in this erroneous situation, however (non-stream) file output always works!
        wxFile safeOutput(FreeFileSync::getConfigDir() + wxT("LastError.txt"), wxFile::write);
        safeOutput.Write(wxString::FromAscii(e.what()));

        wxMessageBox(wxString::FromAscii(e.what()), _("An exception occured!"), wxOK | wxICON_ERROR);
        return -9;
    }

    return 0; //program's return value
}

