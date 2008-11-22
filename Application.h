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

class Application : public wxApp
{
public:
    bool OnInit();
    int  OnRun();
    int  OnExit();

    void initialize();
    bool ProcessIdle(); //virtual method!

private:
    void runBatchMode(const wxString& filename);

    bool applicationRunsInBatchWithoutWindows;
    CustomLocale programLanguage;

    int returnValue;
};

class LogFile;

class BatchStatusUpdater : public StatusUpdater
{
public:
    BatchStatusUpdater(bool continueOnError, bool silent, LogFile* log);
    ~BatchStatusUpdater();

    void updateStatusText(const wxString& text);
    void initNewProcess(int objectsTotal, double dataTotal, int processID);
    void updateProcessedData(int objectsProcessed, double dataProcessed);
    int reportError(const wxString& text);
    void forceUiRefresh();

    void noSynchronizationNeeded();

private:
    void abortThisProcess();

    LogFile* m_log;
    SyncStatus* syncStatusFrame;
    bool continueErrors;
    bool silentMode;

    wxArrayString unhandledErrors; //list of non-resolved errors
    int currentProcess;
    bool synchronizationNeeded;
};


#endif // FREEFILESYNCAPP_H
