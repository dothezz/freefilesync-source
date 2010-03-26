// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) 2008-2010 ZenJu (zhnmju123 AT gmx.de)                    *
// **************************************************************************
//
#ifndef GUISTATUSHANDLER_H_INCLUDED
#define GUISTATUSHANDLER_H_INCLUDED

#include "../library/statusHandler.h"
#include <wx/event.h>
#include "../library/errorLogging.h"
#include "progressIndicator.h"

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
void OnKeyPressed(wxKeyEvent& event);
    void OnAbortCompare(wxCommandEvent& event); //handle abort button click
    virtual void abortThisProcess();

    MainDialog* mainDialog;
    bool ignoreErrors;
    Process currentProcess;
};


class SyncStatusHandler : public StatusHandler
{
public:
    SyncStatusHandler(wxTopLevelWindow* parentDlg, bool ignoreAllErrors);
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

    SyncStatus syncStatusFrame; //the window managed by SyncStatus has longer lifetime than this handler!
    bool ignoreErrors;
    FreeFileSync::ErrorLogging errorLog;
};


#endif // GUISTATUSHANDLER_H_INCLUDED
