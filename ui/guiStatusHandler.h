#ifndef GUISTATUSHANDLER_H_INCLUDED
#define GUISTATUSHANDLER_H_INCLUDED

#include "../library/statusHandler.h"
#include <wx/arrstr.h>

class SyncStatus;
class MainDialog;
class wxWindow;

//classes handling sync and compare error as well as status information
class CompareStatusHandler : public StatusHandler
{
public:
    CompareStatusHandler(MainDialog* dlg);
    ~CompareStatusHandler();

    virtual void updateStatusText(const Zstring& text);
    virtual void initNewProcess(int objectsTotal, wxLongLong dataTotal, Process processID);
    virtual void updateProcessedData(int objectsProcessed, wxLongLong dataProcessed);
    virtual void forceUiRefresh();

    virtual ErrorHandler::Response reportError(const Zstring& text);
    virtual void reportFatalError(const Zstring& errorMessage);
    virtual void reportWarning(const Zstring& warningMessage, bool& dontShowAgain);

private:
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

    virtual ErrorHandler::Response reportError(const Zstring& text);
    virtual void reportFatalError(const Zstring& errorMessage);
    virtual void reportWarning(const Zstring& warningMessage, bool& dontShowAgain);

private:
    virtual void abortThisProcess();

    SyncStatus* syncStatusFrame;
    bool ignoreErrors;
    wxArrayString unhandledErrors;   //list of non-resolved errors
};


#endif // GUISTATUSHANDLER_H_INCLUDED
