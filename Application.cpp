/***************************************************************
 * Purpose:   Code for Application Class
 * Author:    ZenJu (zhnmju123@gmx.de)
 * Created:   2008-07-16
 **************************************************************/

#include "application.h"
#include "ui/mainDialog.h"
#include <wx/stdpaths.h>
#include "library/globalFunctions.h"
#include <wx/msgdlg.h>
#include "comparison.h"
#include "algorithm.h"
#include "synchronization.h"
#include <memory>
#include "ui/batchStatusHandler.h"
#include "ui/checkVersion.h"
#include "library/filter.h"
#include <wx/file.h>
#include <wx/filename.h>

using FreeFileSync::FileError;


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

    //test if FFS is to be started on UI with config file passed as commandline parameter

//#warning
    //this needs to happen BEFORE the working directory is set!
    wxString cfgFilename;
    if (argc > 1)
    {
        //resolve relative names to avoid problems after working directory is changed
        wxFileName filename(argv[1]);
        if (!filename.Normalize())
        {
            wxMessageBox(wxString(_("Error retrieving full path:")) + wxT("\n\"") + argv[1] + wxT("\""));
            return;
        }
        const wxString fullFilename = filename.GetFullPath();

        if (wxFileExists(fullFilename))  //load file specified by %1 parameter:
            cfgFilename = fullFilename;
        else if (wxFileExists(fullFilename + wxT(".ffs_batch")))
            cfgFilename = fullFilename + wxT(".ffs_batch");
        else if (wxFileExists(fullFilename + wxT(".ffs_gui")))
            cfgFilename = fullFilename + wxT(".ffs_gui");
        else
        {
            wxMessageBox(wxString(_("File does not exist:")) + wxT(" \"") + fullFilename + wxT("\""), _("Error"), wxOK | wxICON_ERROR);
            return;
        }
    }


    //set working directory to current executable directory
    const wxString workingDir = FreeFileSync::getInstallationDir();
    if (!wxSetWorkingDirectory(workingDir))
    {   //show messagebox and quit program immediately
        wxMessageBox(wxString(_("Could not set working directory:")) + wxT(" ") + workingDir, _("An exception occured!"), wxOK | wxICON_ERROR);
        return;
    }

    GlobalResources::getInstance().load(); //loads bitmap resources on program startup

    try //load global settings from XML: must be called AFTER working dir was set
    {
        globalSettings = xmlAccess::readGlobalSettings();
    }
    catch (const FileError& error)
    {
        if (wxFileExists(FreeFileSync::getGlobalConfigFile()))
        {   //show messagebox and quit program immediately
            wxMessageBox(error.show().c_str(), _("Error"), wxOK | wxICON_ERROR);
            return;
        }
        //else: globalSettings already has default values
    }

//set program language: needs to happen after working directory has been set!
    SetExitOnFrameDelete(false); //prevent error messagebox from becoming top-level window
    programLanguage.setLanguage(globalSettings.programLanguage);
    SetExitOnFrameDelete(true);


    if (!cfgFilename.empty())
    {
        //load file specified by %1 parameter:
        xmlAccess::XmlType xmlConfigType = xmlAccess::getXmlType(cfgFilename);
        if (xmlConfigType == xmlAccess::XML_GUI_CONFIG) //start in GUI mode (configuration file specified)
        {
            MainDialog* frame = new MainDialog(NULL, cfgFilename, &programLanguage, globalSettings);
            frame->SetIcon(*GlobalResources::getInstance().programIcon); //set application icon
            frame->Show();
        }
        else if (xmlConfigType == xmlAccess::XML_BATCH_CONFIG) //start in commandline mode
        {
            runBatchMode(cfgFilename, globalSettings);

            if (wxApp::GetTopWindow() == NULL) //if no windows are shown program won't exit automatically
                ExitMainLoop();
            return;
        }
        else
        {
            wxMessageBox(wxString(_("The file does not contain a valid configuration:")) + wxT(" \"") + cfgFilename + wxT("\""), _("Error"), wxOK | wxICON_ERROR);
            return;
        }
    }
    else //start in GUI mode (standard)
    {
        MainDialog* frame = new MainDialog(NULL, FreeFileSync::getLastConfigFile(), &programLanguage, globalSettings);
        frame->SetIcon(*GlobalResources::getInstance().programIcon); //set application icon
        frame->Show();
    }
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
    catch (std::exception& e) //catch all STL exceptions
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
    globalSettings.programLanguage = programLanguage.getLanguage();

    try //save global settings to XML
    {
        xmlAccess::writeGlobalSettings(globalSettings);
    }
    catch (const FileError& error)
    {
        wxMessageBox(error.show().c_str(), _("Error"), wxOK | wxICON_ERROR);
    }

    return 0;
}


void Application::runBatchMode(const wxString& filename, xmlAccess::XmlGlobalSettings& globalSettings)
{
    //load XML settings
    xmlAccess::XmlBatchConfig batchCfg;  //structure to receive gui settings
    try
    {
        batchCfg = xmlAccess::readBatchConfig(filename);
    }
    catch (const FileError& error)
    {
        wxMessageBox(error.show().c_str(), _("Error"), wxOK | wxICON_ERROR);
        return;
    }
    //all settings have been read successfully...

    //regular check for program updates
    if (!batchCfg.silent)
        FreeFileSync::checkForUpdatePeriodically(globalSettings.lastUpdateCheck);


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
        FreeFileSync::CompareProcess comparison(globalSettings.traverseDirectorySymlinks,
                                                globalSettings.fileTimeTolerance,
                                                globalSettings.ignoreOneHourDiff,
                                                globalSettings.warnings,
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
            batchCfg.mainCfg.useRecycleBin,
            globalSettings.copyFileSymlinks,
            globalSettings.traverseDirectorySymlinks,
            globalSettings.warnings,
            statusHandler.get());

        synchronization.startSynchronizationProcess(folderCmp);
    }
    catch (FreeFileSync::AbortThisProcess&)  //exit used by statusHandler
    {
        return;
    }
}
