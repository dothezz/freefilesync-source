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
#include <memory>

class Application : public wxApp
{
public:
    bool OnInit();
    int  OnRun();
    int  OnExit();
    bool OnExceptionInMainLoop();
    void OnStartApplication(wxIdleEvent& event);

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
class FfsTrayIcon;

class BatchStatusHandlerSilent : public BatchStatusHandler
{
public:
    BatchStatusHandlerSilent(const xmlAccess::OnError handleError, const wxString& logfileDirectory, int& returnVal);
    ~BatchStatusHandlerSilent();


    virtual void updateStatusText(const Zstring& text);
    virtual void initNewProcess(int objectsTotal, double dataTotal, Process processID);
    virtual void updateProcessedData(int objectsProcessed, double dataProcessed) {}
    virtual void forceUiRefresh();

    virtual ErrorHandler::Response reportError(const Zstring& errorMessage);
    virtual void reportFatalError(const Zstring& errorMessage);
    virtual void reportWarning(const Zstring& warningMessage, bool& dontShowAgain);

    virtual void exitAndSetStatus(const wxString& message, ExitCode code); //abort externally

private:
    virtual void abortThisProcess();

    xmlAccess::OnError m_handleError;
    wxArrayString unhandledErrors; //list of non-resolved errors
    Process currentProcess;
    int& returnValue;
    std::auto_ptr<FfsTrayIcon> trayIcon;

    std::auto_ptr<LogFile> m_log;
};


class BatchStatusHandlerGui : public BatchStatusHandler
{
public:
    BatchStatusHandlerGui(const xmlAccess::OnError handleError, int& returnVal);
    ~BatchStatusHandlerGui();

    virtual void updateStatusText(const Zstring& text);
    virtual void initNewProcess(int objectsTotal, double dataTotal, Process processID);
    virtual void updateProcessedData(int objectsProcessed, double dataProcessed);
    virtual void forceUiRefresh();

    virtual ErrorHandler::Response reportError(const Zstring& errorMessage);
    virtual void reportFatalError(const Zstring& errorMessage);
    virtual void reportWarning(const Zstring& warningMessage, bool& dontShowAgain);

    virtual void exitAndSetStatus(const wxString& message, ExitCode code); //abort externally

private:
    virtual void abortThisProcess();

    xmlAccess::OnError m_handleError;
    wxArrayString unhandledErrors; //list of non-resolved errors
    Process currentProcess;
    int& returnValue;

    SyncStatus* syncStatusFrame;
    wxString additionalStatusInfo; //workaround to display "Nothing to synchronize..."
};

#endif // FREEFILESYNCAPP_H
