#ifndef BATCHSTATUSHANDLER_H_INCLUDED
#define BATCHSTATUSHANDLER_H_INCLUDED

#include "../library/statusHandler.h"
#include <memory>
#include "../library/processXml.h"
#include "../library/errorLogging.h"

class LogFile;
class FfsTrayIcon;
class SyncStatus;


class BatchStatusHandler : public StatusHandler
{
public:
    virtual void addFinalInfo(const wxString& infoMessage) = 0;
virtual void setErrorStrategy(xmlAccess::OnError handleError) = 0; //change error handling during process
};


class BatchStatusHandlerSilent : public BatchStatusHandler
{
public:
    BatchStatusHandlerSilent(const xmlAccess::OnError handleError, const wxString& logfileDirectory, int& returnVal);
    ~BatchStatusHandlerSilent();


    virtual void updateStatusText(const Zstring& text);
    virtual void initNewProcess(int objectsTotal, wxLongLong dataTotal, Process processID);
    virtual void updateProcessedData(int objectsProcessed, wxLongLong dataProcessed);
    virtual void forceUiRefresh();

    virtual ErrorHandler::Response reportError(const wxString& errorMessage);
    virtual void reportFatalError(const wxString& errorMessage);
    virtual void reportWarning(const wxString& warningMessage, bool& warningActive);
    virtual void addFinalInfo(const wxString& infoMessage);

virtual void setErrorStrategy(xmlAccess::OnError handleError); //change error handling during process

private:
    virtual void abortThisProcess();

    xmlAccess::OnError m_handleError;

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

    virtual ErrorHandler::Response reportError(const wxString& errorMessage);
    virtual void reportFatalError(const wxString& errorMessage);
    virtual void reportWarning(const wxString& warningMessage, bool& warningActive);
    virtual void addFinalInfo(const wxString& infoMessage);

virtual void setErrorStrategy(xmlAccess::OnError handleError); //change error handling during process

private:
    virtual void abortThisProcess();

    bool showPopups;
    FreeFileSync::ErrorLogging errorLog; //list of non-resolved errors and warnings
    Process currentProcess;
    int& returnValue;

    SyncStatus* syncStatusFrame;
    wxString finalInfo; //workaround to display "Nothing to synchronize..."
};


#endif // BATCHSTATUSHANDLER_H_INCLUDED
