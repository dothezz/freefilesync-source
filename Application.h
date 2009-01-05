/***************************************************************
 * Name:      FreeFileSyncApp.h
 * Purpose:   Defines Application Class
 * Author:    ZenJu (zhnmju123@gmx.de)
 * Created:   2008-07-16
 * Copyright: ZenJu ()
 * License:
 **************************************************************/

#ifndef FREEFILESYNCAPP_H
#define FREEFILESYNCAPP_H

#include <wx/app.h>
#include <wx/cmdline.h>
#include "ui/smallDialogs.h"
#include "library/misc.h"
#include "library/processXml.h"

class Application : public wxApp
{
public:
    bool OnInit();
    int  OnRun();
    int  OnExit();
    bool OnExceptionInMainLoop();

    void initialize();
    bool ProcessIdle(); //virtual method!

private:
    void runBatchMode(const wxString& filename, xmlAccess::XmlGlobalSettings& globalSettings);

    bool applicationRunsInBatchWithoutWindows;
    CustomLocale programLanguage;

    int returnValue;
    xmlAccess::XmlGlobalSettings globalSettings; //settings used by GUI, batch mode or both
};

class LogFile;

class BatchStatusUpdater : public StatusHandler
{
public:
    BatchStatusUpdater(bool ignoreAllErrors, bool silent, LogFile* log);
    ~BatchStatusUpdater();

    virtual void updateStatusText(const wxString& text);
    virtual void initNewProcess(int objectsTotal, double dataTotal, Process processID);
    virtual void updateProcessedData(int objectsProcessed, double dataProcessed);
    virtual ErrorHandler::Response reportError(const wxString& text);
    virtual void forceUiRefresh();

    wxWindow* getWindow();
    void setFinalStatus(const wxString& message, SyncStatus::SyncStatusID id); //overwrite final status message text

private:
    virtual void abortThisProcess();

    LogFile* m_log;
    SyncStatus* syncStatusFrame;
    bool ignoreErrors;
    bool silentMode;

    wxArrayString unhandledErrors; //list of non-resolved errors
    Process currentProcess;

    wxString customStatusMessage; //final status message written by service consumer
    SyncStatus::SyncStatusID customStatusId;
};


#endif // FREEFILESYNCAPP_H
