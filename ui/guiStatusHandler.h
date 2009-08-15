#ifndef GUISTATUSHANDLER_H_INCLUDED
#define GUISTATUSHANDLER_H_INCLUDED

#include "../library/statusHandler.h"
#include <wx/event.h>
#include "../library/errorLogging.h"

class SyncStatus;
class MainDialog;
class wxWindow;
class wxCommandEvent;


//classes handling sync and compare error as well as status information
class CompareStatusHandler : private wxEvtHandler, public StatusHandler
{
public:
    CompareStatusHandler(MainDialog* dlg);
    ~CompareStatusHandler();

    virtual void updateStatusText(const Zstring& text);
    virtual void initNewProcess(int objectsTotal, wxLongLong dataTotal, Process processID);
    virtual void updateProcessedData(int objectsProcessed, wxLongLong dataProcessed);
    virtual void forceUiRefresh();

    virtual ErrorHandler::Response reportError(const wxString& text);
    virtual void reportFatalError(const wxString& errorMessage);
    virtual void reportWarning(const wxString& warningMessage, bool& warningActive);

private:
    void OnAbortCompare(wxCommandEvent& event); //handle abort button click
    virtual void abortThisProcess();

    MainDialog* mainDialog;
    bool ignoreErrors;
    Process currentProcess;
};


class SyncStatusHandler : public StatusHandler
{
public:
    SyncStatusHandler(wxWindow* dlg, bool ignoreAllErrors);
    ~SyncStatusHandler();

    virtual void updateStatusText(const Zstring& text);
    virtual void initNewProcess(int objectsTotal, wxLongLong dataTotal, Process processID);
    virtual void updateProcessedData(int objectsProcessed, wxLongLong dataProcessed);
    virtual void forceUiRefresh();

    virtual ErrorHandler::Response reportError(const wxString& text);
    virtual void reportFatalError(const wxString& errorMessage);
    virtual void reportWarning(const wxString& warningMessage, bool& warningActive);

private:
    virtual void abortThisProcess();

    SyncStatus* syncStatusFrame;
    bool ignoreErrors;
    FreeFileSync::ErrorLogging errorLog;
};


#endif // GUISTATUSHANDLER_H_INCLUDED
