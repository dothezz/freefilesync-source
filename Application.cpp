// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) 2008-2011 ZenJu (zhnmju123 AT gmx.de)                    *
// **************************************************************************
//
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
#include "library/resources.h"
#include "ui/switch_to_gui.h"
#include "shared/standard_paths.h"
#include "shared/i18n.h"
#include "shared/app_main.h"
#include <wx/sound.h>
#include "shared/file_handling.h"
#include "shared/string_conv.h"
#include "shared/util.h"
#include <wx/log.h>
#include "library/lock_holder.h"

#ifdef FFS_LINUX
#include <gtkmm/main.h>
#include <gtk/gtk.h>
#endif


using namespace zen;


IMPLEMENT_APP(Application)


bool Application::OnInit()
{
    returnValue = 0;
    //do not call wxApp::OnInit() to avoid using default commandline parser

    //Note: initialization is done in the FIRST idle event instead of OnInit. Reason: batch mode requires the wxApp eventhandler to be established
    //for UI update events. This is not the case at the time of OnInit().
    Connect(wxEVT_IDLE, wxIdleEventHandler(Application::OnStartApplication), NULL, this);

    return true;
}


void Application::OnStartApplication(wxIdleEvent&)
{
    using namespace zen;

    Disconnect(wxEVT_IDLE, wxIdleEventHandler(Application::OnStartApplication), NULL, this);

    struct HandleAppExit //wxWidgets app exit handling is a bit weird... we want the app to exit only if the logical main window is closed
    {
        HandleAppExit()
        {
            wxTheApp->SetExitOnFrameDelete(false); //avoid popup-windows from becoming temporary top windows leading to program exit after closure
        }

        ~HandleAppExit()
        {
            if (!zen::AppMainWindow::mainWindowWasSet())
                wxTheApp->ExitMainLoop(); //quit application, if no main window was set (batch silent mode)
        }

    } dummy;


    //if appname is not set, the default is the executable's name!
    SetAppName(wxT("FreeFileSync"));

#ifdef FFS_LINUX
    Gtk::Main::init_gtkmm_internals();

    ::gtk_rc_parse((Zstring(zen::getResourceDir()) + "styles.rc").c_str()); //remove inner border from bitmap buttons
#endif


#if wxCHECK_VERSION(2, 9, 1)
    wxToolTip::SetMaxWidth(-1); //disable tooltip wrapping
    wxToolTip::SetAutoPop(7000); //tooltip visibilty in ms, 5s seems to be default for Windows
#endif


    try //load global settings from XML: they are written on exit, so read them FIRST
    {
        if (fileExists(wxToZ(xmlAccess::getGlobalConfigFile())))
            xmlAccess::readConfig(globalSettings);
        //else: globalSettings already has default values
    }
    catch (const xmlAccess::FfsXmlError& error)
    {
        //show messagebox and continue
        if (error.getSeverity() == xmlAccess::FfsXmlError::WARNING)
            ; //wxMessageBox(error.msg(), _("Warning"), wxOK | wxICON_WARNING); -> ignore parsing errors: should be migration problems only *cross-fingers*
        else
            wxMessageBox(error.msg(), _("Error"), wxOK | wxICON_ERROR);
    }

    //set program language
    zen::setLanguage(globalSettings.programLanguage);


    //test if FFS is to be started on UI with config file passed as commandline parameter

    //try to set config/batch-filename set by %1 parameter
    wxString cfgFilename;
    if (argc > 1)
    {
        const wxString arg1(argv[1]);

        if (fileExists(wxToZ(arg1)))  //load file specified by %1 parameter:
            cfgFilename = arg1;
        else if (fileExists(wxToZ(arg1 + wxT(".ffs_batch"))))
            cfgFilename = arg1 + wxT(".ffs_batch");
        else if (fileExists(wxToZ(arg1 + wxT(".ffs_gui"))))
            cfgFilename = arg1 + wxT(".ffs_gui");
        else
        {
            wxMessageBox(wxString(_("File does not exist:")) + wxT(" \"") + arg1 + wxT("\""), _("Error"), wxOK | wxICON_ERROR);
            returnValue = -13;
            return;
        }
    }

    if (!cfgFilename.empty())
    {
        //load file specified by %1 parameter:
        const xmlAccess::XmlType xmlConfigType = xmlAccess::getXmlType(cfgFilename);
        switch (xmlConfigType)
        {
            case xmlAccess::XML_TYPE_GUI: //start in GUI mode (configuration file specified)
                runGuiMode(cfgFilename, globalSettings);
                break;

            case xmlAccess::XML_TYPE_BATCH: //start in commandline mode
                runBatchMode(cfgFilename, globalSettings);
                break;

            case xmlAccess::XML_TYPE_GLOBAL:
            case xmlAccess::XML_TYPE_OTHER:
                wxMessageBox(wxString(_("The file does not contain a valid configuration:")) + wxT(" \"") + cfgFilename + wxT("\""), _("Error"), wxOK | wxICON_ERROR);
                break;
        }
    }
    else //start in GUI mode (standard)
        runGuiMode(wxEmptyString, globalSettings);
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
        wxFile safeOutput(zen::getConfigDir() + wxT("LastError.txt"), wxFile::write);
        safeOutput.Write(wxString::FromAscii(e.what()));

        wxSafeShowMessage(_("An exception occurred!"), wxString::FromAscii(e.what()));
        return -9;
    }
    catch (...) //catch the rest
    {
        wxFile safeOutput(zen::getConfigDir() + wxT("LastError.txt"), wxFile::write);
        safeOutput.Write(wxT("Unknown exception!"));

        wxSafeShowMessage(_("An exception occurred!"), wxT("Unknown exception!"));
        return -9;
    }

    return returnValue;
}


int Application::OnExit()
{
    //get program language
    globalSettings.programLanguage = zen::getLanguage();

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


void Application::runGuiMode(const wxString& cfgFileName, xmlAccess::XmlGlobalSettings& settings)
{
    MainDialog* frame = new MainDialog(cfgFileName, settings);
    frame->Show();
}

void Application::runBatchMode(const wxString& filename, xmlAccess::XmlGlobalSettings& globSettings)
{
    using namespace xmlAccess;

    //load XML settings
    xmlAccess::XmlBatchConfig batchCfg;  //structure to receive gui settings
    try
    {
        xmlAccess::readConfig(filename, batchCfg);
    }
    catch (const xmlAccess::FfsXmlError& error)
    {
        wxMessageBox(error.msg(), _("Error"), wxOK | wxICON_ERROR); //batch mode: break on errors AND even warnings!
        return;
    }
    //all settings have been read successfully...

    //regular check for program updates -> disabled for batch
    //if (!batchCfg.silent)
    //    zen::checkForUpdatePeriodically(globSettings.lastUpdateCheck);

    try //begin of synchronization process (all in one try-catch block)
    {
        const SwitchToGui switchBatchToGui(batchCfg, globSettings); //prepare potential operational switch

        //class handling status updates and error messages
        BatchStatusHandler statusHandler(batchCfg.silent,
                                         zen::extractJobName(filename),
                                         batchCfg.silent ? &batchCfg.logFileDirectory : NULL,
                                         batchCfg.logFileCountMax,
                                         batchCfg.handleError,
                                         switchBatchToGui,
                                         returnValue);

        const std::vector<zen::FolderPairCfg> cmpConfig = zen::extractCompareCfg(batchCfg.mainCfg);

        //batch mode: place directory locks on directories during both comparison AND synchronization
        LockHolder dummy;
        for (std::vector<FolderPairCfg>::const_iterator i = cmpConfig.begin(); i != cmpConfig.end(); ++i)
        {
            dummy.addDir(i->leftDirectoryFmt,  statusHandler);
            dummy.addDir(i->rightDirectoryFmt, statusHandler);
        }

        //COMPARE DIRECTORIES
        zen::CompareProcess comparison(batchCfg.mainCfg.handleSymlinks,
                                       globSettings.fileTimeTolerance,
                                       globSettings.optDialogs,
                                       statusHandler);

        zen::FolderComparison folderCmp;
        comparison.startCompareProcess(cmpConfig,
                                       batchCfg.mainCfg.compareVar,
                                       folderCmp);

        //START SYNCHRONIZATION
        zen::SyncProcess synchronization(
            globSettings.optDialogs,
            globSettings.verifyFileCopy,
            globSettings.copyLockedFiles,
            globSettings.copyFilePermissions,
            statusHandler);

        const std::vector<zen::FolderPairSyncCfg> syncProcessCfg = zen::extractSyncCfg(batchCfg.mainCfg);
        assert(syncProcessCfg.size() == folderCmp.size());

        synchronization.startSynchronizationProcess(syncProcessCfg, folderCmp);

        //play (optional) sound notification after sync has completed
        if (!batchCfg.silent)
        {
            const wxString soundFile = zen::getResourceDir() + wxT("Sync_Complete.wav");
            if (zen::fileExists(zen::wxToZ(soundFile)))
                wxSound::Play(soundFile, wxSOUND_ASYNC); //warning: this may fail and show a wxWidgets error message!
        }
    }
    catch (BatchAbortProcess&)  //exit used by statusHandler
    {
        if (returnValue >= 0)
            returnValue = -12;
        return;
    }
}
