/***************************************************************
 * Purpose:   Code for Application Class
 * Author:    ZenJu (zhnmju123@gmx.de)
 * Created:   2009-07-06
 * Copyright: ZenJu (http://sourceforge.net/projects/freefilesync/)
 **************************************************************/

#include "application.h"
#include "mainDialog.h"
#include <wx/event.h>
#include "resources.h"
#include <wx/msgdlg.h>
#include "../shared/localization.h"
#include "xmlFreeFileSync.h"
#include "../shared/standardPaths.h"

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
    ::gtk_rc_parse("styles.rc"); //remove inner border from bitmap buttons
#endif

    //set program language
    try
    {
        FreeFileSync::CustomLocale::getInstance().setLanguage(RealtimeSync::getProgramLanguage());
    }
    catch (const xmlAccess::XmlError& error)
    {
        if (wxFileExists(FreeFileSync::getGlobalConfigFile()))
        {
            SetExitOnFrameDelete(false); //prevent error messagebox from becoming top-level window
            if (error.getSeverity() == xmlAccess::XmlError::WARNING)
                wxMessageBox(error.show(), _("Warning"), wxOK | wxICON_WARNING);
            else
                wxMessageBox(error.show(), _("Error"), wxOK | wxICON_ERROR);
            SetExitOnFrameDelete(true);

        }
    }

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
