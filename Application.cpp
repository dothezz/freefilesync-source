/***************************************************************
 * Purpose:   Code for Application Class
 * Author:    ZenJu (zhnmju123@gmx.de)
 * Created:   2008-07-16
 **************************************************************/

#include "application.h"
#include "ui/mainDialog.h"
#include "shared/globalFunctions.h"
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
#include "shared/standardPaths.h"
#include "shared/localization.h"

#ifdef FFS_LINUX
#include <gtk/gtk.h>
#endif

using FreeFileSync::CustomLocale;


IMPLEMENT_APP(Application);

bool Application::OnInit()
{
    returnValue = 0;
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
    SetAppName(wxT("FreeFileSync"));

#ifdef FFS_LINUX
    ::gtk_rc_parse("styles.rc"); //remove inner border from bitmap buttons
#endif

    //test if FFS is to be started on UI with config file passed as commandline parameter

    //try to set config/batch-filename set by %1 parameter
    wxString cfgFilename;
    if (argc > 1)
    {
        const wxString filename(argv[1]);

        if (wxFileExists(filename))  //load file specified by %1 parameter:
            cfgFilename = filename;
        else if (wxFileExists(filename + wxT(".ffs_batch")))
            cfgFilename = filename + wxT(".ffs_batch");
        else if (wxFileExists(filename + wxT(".ffs_gui")))
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
        xmlAccess::readGlobalSettings(globalSettings);
    }
    catch (const xmlAccess::XmlError& error)
    {
        if (wxFileExists(FreeFileSync::getGlobalConfigFile()))
        {   //show messagebox and continue
            SetExitOnFrameDelete(false); //prevent error messagebox from becoming top-level window
            if (error.getSeverity() == xmlAccess::XmlError::WARNING)
                wxMessageBox(error.show(), _("Warning"), wxOK | wxICON_WARNING);
            else
                wxMessageBox(error.show(), _("Error"), wxOK | wxICON_ERROR);
            SetExitOnFrameDelete(true);
        }
        //else: globalSettings already has default values
    }

    //set program language
    SetExitOnFrameDelete(false); //prevent error messagebox from becoming top-level window
    CustomLocale::getInstance().setLanguage(globalSettings.programLanguage);
    SetExitOnFrameDelete(true);


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

            if (wxApp::GetTopWindow() == NULL) //if no windows are shown, program won't exit automatically
                ExitMainLoop();
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
    catch (const RuntimeException& e) //catch runtime errors during main event loop; wxApp::OnUnhandledException could be used alternatively
    {
        //unfortunately it's not always possible to display a message box in this erroneous situation, however (non-stream) file output always works!
        wxFile safeOutput(FreeFileSync::getLastErrorTxtFile(), wxFile::write);
        safeOutput.Write(e.show());

        wxMessageBox(e.show(), _("An exception occured!"), wxOK | wxICON_ERROR);
        return -8;
    }
    catch (const std::exception& e) //catch all STL exceptions
    {
        //unfortunately it's not always possible to display a message box in this erroneous situation, however (non-stream) file output always works!
        wxFile safeOutput(FreeFileSync::getLastErrorTxtFile(), wxFile::write);
        safeOutput.Write(wxString::From8BitData(e.what()));

        wxMessageBox(wxString::From8BitData(e.what()), _("An exception occured!"), wxOK | wxICON_ERROR);
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
    MainDialog* frame = new MainDialog(NULL, cfgFileName, settings);
    frame->SetIcon(*GlobalResources::getInstance().programIcon); //set application icon
    frame->Show();
}


void Application::runBatchMode(const wxString& filename, xmlAccess::XmlGlobalSettings& globSettings)
{
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

    //regular check for program updates
    if (!batchCfg.silent)
        FreeFileSync::checkForUpdatePeriodically(globSettings.lastUpdateCheck);


    try //begin of synchronization process (all in one try-catch block)
    {
        //class handling status updates and error messages
        std::auto_ptr<BatchStatusHandler> statusHandler;  //delete object automatically
        if (batchCfg.silent)
            statusHandler = std::auto_ptr<BatchStatusHandler>(new BatchStatusHandlerSilent(batchCfg.handleError, batchCfg.logFileDirectory, returnValue));
        else
            statusHandler = std::auto_ptr<BatchStatusHandler>(new BatchStatusHandlerGui(batchCfg.handleError, returnValue));

        //PREPARE FILTER
        std::auto_ptr<FreeFileSync::FilterProcess> filterInstance(NULL);
        if (batchCfg.mainCfg.filterIsActive)
            filterInstance.reset(new FreeFileSync::FilterProcess(batchCfg.mainCfg.includeFilter, batchCfg.mainCfg.excludeFilter));

        //COMPARE DIRECTORIES
        FreeFileSync::FolderComparison folderCmp;
        FreeFileSync::CompareProcess comparison(globSettings.traverseDirectorySymlinks,
                                                globSettings.fileTimeTolerance,
                                                globSettings.ignoreOneHourDiff,
                                                globSettings.warnings,
                                                filterInstance.get(),
                                                statusHandler.get());

        comparison.startCompareProcess(batchCfg.directoryPairs,
                                       batchCfg.mainCfg.compareVar,
                                       batchCfg.mainCfg.syncConfiguration,
                                       folderCmp);

        //check if there are files/folders to be sync'ed at all
        if (!synchronizationNeeded(folderCmp))
        {
            statusHandler->addFinalInfo(_("Nothing to synchronize according to configuration!")); //inform about this special case
            return;
        }

        //START SYNCHRONIZATION
        FreeFileSync::SyncProcess synchronization(
            batchCfg.mainCfg.handleDeletion,
            batchCfg.mainCfg.customDeletionDirectory,
            globSettings.copyFileSymlinks,
            globSettings.traverseDirectorySymlinks,
            globSettings.warnings,
            statusHandler.get());

        synchronization.startSynchronizationProcess(folderCmp);
    }
    catch (FreeFileSync::AbortThisProcess&)  //exit used by statusHandler
    {
        if (returnValue >= 0)
            returnValue = -12;
        return;
    }
}
