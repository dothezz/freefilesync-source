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
#include "FreeFileSync.h"
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
    BatchStatusUpdater(bool continueOnError, bool silent, LogFile* log);
    ~BatchStatusUpdater();

    void updateStatusText(const wxString& text);
    void initNewProcess(int objectsTotal, double dataTotal, Process processID);
    void updateProcessedData(int objectsProcessed, double dataProcessed);
    ErrorHandler::Response reportError(const wxString& text);
    void forceUiRefresh();

    void noSynchronizationNeeded();

private:
    void abortThisProcess();

    LogFile* m_log;
    SyncStatus* syncStatusFrame;
    bool continueErrors;
    bool silentMode;

    wxArrayString unhandledErrors; //list of non-resolved errors
    Process currentProcess;
    bool synchronizationNeeded;
};


#endif // FREEFILESYNCAPP_H
