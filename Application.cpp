// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) 2008-2010 ZenJu (zhnmju123 AT gmx.de)                    *
// **************************************************************************
//
#include "application.h"
#include "ui/mainDialog.h"
#include <wx/msgdlg.h>
#include "comparison.h"
#include "algorithm.h"
#include "synchronization.h"
#include <memory>
#include "ui/batchStatusHandler.h"
#include "ui/checkVersion.h"
#include "library/filter.h"
#include <wx/file.h>
#include "shared/xmlBase.h"
#include "library/resources.h"
#include "ui/switchToGui.h"
#include "shared/standardPaths.h"
#include "shared/localization.h"
#include "shared/appMain.h"
#include <wx/sound.h>
#include "shared/fileHandling.h"
#include "shared/stringConv.h"

#ifdef FFS_LINUX
#include <gtk/gtk.h>
#endif

using FreeFileSync::CustomLocale;
using FreeFileSync::SwitchToGui;


IMPLEMENT_APP(Application)

bool Application::OnInit()
{
    returnValue = 0;
    //do not call wxApp::OnInit() to avoid using default commandline parser

//Note: initialization is done in the FIRST idle event instead of OnInit. Reason: Commandline mode requires the wxApp eventhandler to be established
//for UI update events. This is not the case at the time of OnInit().
    Connect(wxEVT_IDLE, wxIdleEventHandler(Application::OnStartApplication), NULL, this);

    return true;
}


void Application::OnStartApplication(wxIdleEvent&)
{
    using namespace FreeFileSync;

    Disconnect(wxEVT_IDLE, wxIdleEventHandler(Application::OnStartApplication), NULL, this);

    struct HandleAppExit //wxWidgets app exit handling is a bit weird... we want the app to exit only if the logical main window is closed
    {
        HandleAppExit()
        {
            wxTheApp->SetExitOnFrameDelete(false); //avoid popup-windows from becoming temporary top windows leading to program exit after closure
        }

        ~HandleAppExit()
        {
            if (!FreeFileSync::AppMainWindow::mainWindowWasSet())
                wxTheApp->ExitMainLoop(); //quit application, if no main window was set (batch silent mode)
        }

    } dummy;


    //if appname is not set, the default is the executable's name!
    SetAppName(wxT("FreeFileSync"));

#ifdef FFS_LINUX
    ::gtk_rc_parse(FreeFileSync::wxToZ(FreeFileSync::getResourceDir()) + "styles.rc"); //remove inner border from bitmap buttons
#endif


    //test if FFS is to be started on UI with config file passed as commandline parameter

    //try to set config/batch-filename set by %1 parameter
    wxString cfgFilename;
    if (argc > 1)
    {
        const wxString filename(argv[1]);

        if (fileExists(wxToZ(filename)))  //load file specified by %1 parameter:
            cfgFilename = filename;
        else if (fileExists(wxToZ(filename + wxT(".ffs_batch"))))
            cfgFilename = filename + wxT(".ffs_batch");
        else if (fileExists(wxToZ(filename + wxT(".ffs_gui"))))
            cfgFilename = filename + wxT(".ffs_gui");
        else
        {
            wxMessageBox(wxString(_("File does not exist:")) + wxT(" \"") + filename + wxT("\""), _("Error"), wxOK | wxICON_ERROR);
            return;
        }
    }

    GlobalResources::getInstance().load(); //loads bitmap resources on program startup

    try //load global settings from XML
    {
        if (fileExists(wxToZ(xmlAccess::getGlobalConfigFile())))
            xmlAccess::readGlobalSettings(globalSettings);
        //else: globalSettings already has default values
    }
    catch (const xmlAccess::XmlError& error)
    {
        //show messagebox and continue
        if (error.getSeverity() == xmlAccess::XmlError::WARNING)
            ; //wxMessageBox(error.show(), _("Warning"), wxOK | wxICON_WARNING); -> ignore parsing errors: should be migration problems only *cross-fingers*
        else
            wxMessageBox(error.show(), _("Error"), wxOK | wxICON_ERROR);
    }

    //set program language
    CustomLocale::getInstance().setLanguage(globalSettings.programLanguage);


    if (!cfgFilename.empty())
    {
        //load file specified by %1 parameter:
        const xmlAccess::XmlType xmlConfigType = xmlAccess::getXmlType(cfgFilename);
        switch (xmlConfigType)
        {
        case xmlAccess::XML_GUI_CONFIG: //start in GUI mode (configuration file specified)
            runGuiMode(cfgFilename, globalSettings);
            break;

        case xmlAccess::XML_BATCH_CONFIG: //start in commandline mode
            runBatchMode(cfgFilename, globalSettings);
            break;

        case xmlAccess::XML_GLOBAL_SETTINGS:
        case xmlAccess::XML_REAL_CONFIG:
        case xmlAccess::XML_OTHER:
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
        wxFile safeOutput(FreeFileSync::getConfigDir() + wxT("LastError.txt"), wxFile::write);
        safeOutput.Write(wxString::FromAscii(e.what()));

        wxMessageBox(wxString::FromAscii(e.what()), _("An exception occured!"), wxOK | wxICON_ERROR);
        return -9;
    }

    return returnValue;
}


int Application::OnExit()
{
    //get program language
    globalSettings.programLanguage = CustomLocale::getInstance().getLanguage();

    try //save global settings to XML
    {
        xmlAccess::writeGlobalSettings(globalSettings);
    }
    catch (const xmlAccess::XmlError& error)
    {
        wxMessageBox(error.show(), _("Error"), wxOK | wxICON_ERROR);
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
        xmlAccess::readBatchConfig(filename, batchCfg);
    }
    catch (const xmlAccess::XmlError& error)
    {
        wxMessageBox(error.show(), _("Error"), wxOK | wxICON_ERROR);
        return;
    }
    //all settings have been read successfully...

    //regular check for program updates -> disabled for batch
    //if (!batchCfg.silent)
    //    FreeFileSync::checkForUpdatePeriodically(globSettings.lastUpdateCheck);

    try //begin of synchronization process (all in one try-catch block)
    {
        const SwitchToGui switchBatchToGui(batchCfg, globSettings); //prepare potential operational switch

        //class handling status updates and error messages
        std::auto_ptr<BatchStatusHandler> statusHandler;  //delete object automatically
        if (batchCfg.silent)
            statusHandler.reset(new BatchStatusHandler(true, filename, &batchCfg.logFileDirectory, batchCfg.handleError, switchBatchToGui, returnValue));
        else
            statusHandler.reset(new BatchStatusHandler(false, filename, NULL, batchCfg.handleError, switchBatchToGui, returnValue));

        //COMPARE DIRECTORIES
        FreeFileSync::FolderComparison folderCmp;
        FreeFileSync::CompareProcess comparison(batchCfg.mainCfg.handleSymlinks,
                                                batchCfg.mainCfg.hidden.fileTimeTolerance,
                                                globSettings.ignoreOneHourDiff,
                                                globSettings.optDialogs,
                                                statusHandler.get());

        comparison.startCompareProcess(FreeFileSync::extractCompareCfg(batchCfg.mainCfg),
                                       batchCfg.mainCfg.compareVar,
                                       folderCmp);

        //check if there are files/folders to be sync'ed at all
        if (!synchronizationNeeded(folderCmp))
        {
            statusHandler->reportInfo(_("Nothing to synchronize according to configuration!")); //inform about this special case
            //return; -> disabled: <automatic> mode requires database to be written in any case
        }

        //START SYNCHRONIZATION
        FreeFileSync::SyncProcess synchronization(
            globSettings.optDialogs,
            batchCfg.mainCfg.hidden.verifyFileCopy,
            globSettings.copyLockedFiles,
            *statusHandler);

        const std::vector<FreeFileSync::FolderPairSyncCfg> syncProcessCfg = FreeFileSync::extractSyncCfg(batchCfg.mainCfg);
        assert(syncProcessCfg.size() == folderCmp.size());

        synchronization.startSynchronizationProcess(syncProcessCfg, folderCmp);

        //play (optional) sound notification after sync has completed (GUI and batch mode)
        const wxString soundFile = FreeFileSync::getResourceDir() + wxT("Sync_Complete.wav");
        if (FreeFileSync::fileExists(FreeFileSync::wxToZ(soundFile)))
            wxSound::Play(soundFile, wxSOUND_ASYNC);
    }
    catch (FreeFileSync::AbortThisProcess&)  //exit used by statusHandler
    {
        if (returnValue >= 0)
            returnValue = -12;
        return;
    }
}
