#ifndef BATCHSTATUSHANDLER_H_INCLUDED
#define BATCHSTATUSHANDLER_H_INCLUDED

#include "../library/statusHandler.h"
#include <memory>
#include "../library/processXml.h"
#include <wx/arrstr.h>

class LogFile;
class FfsTrayIcon;
class SyncStatus;


class BatchStatusHandler : public StatusHandler
{
public:
    BatchStatusHandler() {}
    virtual ~BatchStatusHandler() {}

    virtual void addFinalInfo(const Zstring& infoMessage) = 0;
};


class BatchStatusHandlerSilent : public BatchStatusHandler
{
public:
    BatchStatusHandlerSilent(const xmlAccess::OnError handleError, const wxString& logfileDirectory, int& returnVal);
    ~BatchStatusHandlerSilent();


    virtual void updateStatusText(const Zstring& text);
    virtual void initNewProcess(int objectsTotal, wxLongLong dataTotal, Process processID);
    virtual void updateProcessedData(int objectsProcessed, wxLongLong dataProcessed) {}
    virtual void forceUiRefresh();

    virtual ErrorHandler::Response reportError(const Zstring& errorMessage);
    virtual void reportFatalError(const Zstring& errorMessage);
    virtual void reportWarning(const Zstring& warningMessage, bool& dontShowAgain);
    virtual void addFinalInfo(const Zstring& infoMessage);

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
    virtual void initNewProcess(int objectsTotal, wxLongLong dataTotal, Process processID);
    virtual void updateProcessedData(int objectsProcessed, wxLongLong dataProcessed);
    virtual void forceUiRefresh();

    virtual ErrorHandler::Response reportError(const Zstring& errorMessage);
    virtual void reportFatalError(const Zstring& errorMessage);
    virtual void reportWarning(const Zstring& warningMessage, bool& dontShowAgain);
    virtual void addFinalInfo(const Zstring& infoMessage);

private:
    virtual void abortThisProcess();

    xmlAccess::OnError m_handleError;
    wxArrayString unhandledErrors; //list of non-resolved errors
    Process currentProcess;
    int& returnValue;

    SyncStatus* syncStatusFrame;
    wxString finalInfo; //workaround to display "Nothing to synchronize..."
};


#endif // BATCHSTATUSHANDLER_H_INCLUDED
