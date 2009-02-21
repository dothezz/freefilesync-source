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

    bool ProcessIdle();  //virtual impl.

private:
    void runBatchMode(const wxString& filename, xmlAccess::XmlGlobalSettings& globalSettings);

    CustomLocale programLanguage;

    int returnValue;
    xmlAccess::XmlGlobalSettings globalSettings; //settings used by GUI, batch mode or both
};


class BatchStatusHandler : public StatusHandler
{
public:
    BatchStatusHandler() {}
    virtual ~BatchStatusHandler() {}

    enum ExitCode
    {
        NONE,
        ABORTED,
        FINISHED
    };
    virtual void exitAndSetStatus(const wxString& message, ExitCode code) = 0; //overwrite final status message text
};


class LogFile;
class BatchStatusHandlerSilent : public BatchStatusHandler
{
public:
    BatchStatusHandlerSilent(bool ignoreAllErrors, LogFile* log, int& returnVal);
    ~BatchStatusHandlerSilent();


    void updateStatusText(const Zstring& text); //virtual impl.
    void initNewProcess(int objectsTotal, double dataTotal, Process processID); //virtual impl.
    void updateProcessedData(int objectsProcessed, double dataProcessed) {} //virtual impl.
    ErrorHandler::Response reportError(const Zstring& text); //virtual impl.
    void forceUiRefresh() {} //virtual impl.

    void exitAndSetStatus(const wxString& message, ExitCode code); //abort externally //virtual impl.

private:
    void abortThisProcess(); //virtual impl.

    bool ignoreErrors;
    wxArrayString unhandledErrors; //list of non-resolved errors
    Process currentProcess;
    int& returnValue;

    LogFile* m_log;
};


class BatchStatusHandlerGui : public BatchStatusHandler
{
public:
    BatchStatusHandlerGui(bool ignoreAllErrors, int& returnVal);
    ~BatchStatusHandlerGui();

    void updateStatusText(const Zstring& text); //virtual impl.
    void initNewProcess(int objectsTotal, double dataTotal, Process processID); //virtual impl.
    void updateProcessedData(int objectsProcessed, double dataProcessed); //virtual impl.
    ErrorHandler::Response reportError(const Zstring& text); //virtual impl.
    void forceUiRefresh(); //virtual impl.

    void exitAndSetStatus(const wxString& message, ExitCode code); //abort externally //virtual impl.

private:
    void abortThisProcess(); //virtual impl.

    bool ignoreErrors;
    wxArrayString unhandledErrors; //list of non-resolved errors
    Process currentProcess;
    int& returnValue;

    SyncStatus* syncStatusFrame;
    wxString additionalStatusInfo; //workaround to display "Nothing to synchronize..."
};

#endif // FREEFILESYNCAPP_H
