#ifndef BATCHSTATUSHANDLER_H_INCLUDED
#define BATCHSTATUSHANDLER_H_INCLUDED

#include "../library/statusHandler.h"
#include "../library/processXml.h"
#include "../library/errorLogging.h"

class LogFile;
class SyncStatus;


class BatchStatusHandler : public StatusHandler
{
public:
    BatchStatusHandler(bool runSilent, //defines: -start minimized and -quit immediately when finished
                       const wxString* logfileDirectory, //optional: enable logging if available
                       const xmlAccess::OnError handleError,
                       int& returnVal);
    ~BatchStatusHandler();

    virtual void updateStatusText(const Zstring& text);
    virtual void initNewProcess(int objectsTotal, wxLongLong dataTotal, Process processID);
    virtual void updateProcessedData(int objectsProcessed, wxLongLong dataProcessed);
    virtual void forceUiRefresh();

    void reportInfo(const wxString& infoMessage);
    virtual void reportWarning(const wxString& warningMessage, bool& warningActive);
    virtual ErrorHandler::Response reportError(const wxString& errorMessage);
    virtual void reportFatalError(const wxString& errorMessage);

private:
    virtual void abortThisProcess();

    bool exitWhenFinished;
    xmlAccess::OnError handleError_;
    FreeFileSync::ErrorLogging errorLog; //list of non-resolved errors and warnings
    Process currentProcess;
    int& returnValue;

    SyncStatus* syncStatusFrame;
    boost::shared_ptr<LogFile> logFile; //optional!
};


#endif // BATCHSTATUSHANDLER_H_INCLUDED
