// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) 2008-2011 ZenJu (zhnmju123 AT gmx.de)                    *
// **************************************************************************

#ifndef GUISTATUSHANDLER_H_INCLUDED
#define GUISTATUSHANDLER_H_INCLUDED

#include "../lib/status_handler.h"
#include <wx/event.h>
#include "../lib/error_log.h"
#include "progress_indicator.h"
#include "../lib/process_xml.h"
#include "main_dlg.h"

class SyncStatus;
class wxCommandEvent;

//Exception class used to abort the "compare" and "sync" process
class GuiAbortProcess {};

//classes handling sync and compare error as well as status information
class CompareStatusHandler : private wxEvtHandler, public StatusHandler
{
public:
    CompareStatusHandler(MainDialog& dlg);
    ~CompareStatusHandler();

    virtual void initNewProcess(int objectsTotal, zen::Int64 dataTotal, Process processID);
    virtual void updateProcessedData(int objectsProcessed, zen::Int64 dataProcessed);
    virtual void reportStatus(const wxString& text);
    virtual void reportInfo(const wxString& text);
    virtual void forceUiRefresh();

    virtual Response reportError(const wxString& text);
    virtual void reportFatalError(const wxString& errorMessage);
    virtual void reportWarning(const wxString& warningMessage, bool& warningActive);

private:
    void OnKeyPressed(wxKeyEvent& event);
    void OnAbortCompare(wxCommandEvent& event); //handle abort button click
    virtual void abortThisProcess();

    MainDialog& mainDlg;
    bool ignoreErrors;
    Process currentProcess;
};


class SyncStatusHandler : public StatusHandler
{
public:
    SyncStatusHandler(MainDialog* parentDlg, xmlAccess::OnGuiError handleError, const wxString& jobName);
    ~SyncStatusHandler();

    virtual void initNewProcess(int objectsTotal, zen::Int64 dataTotal, Process processID);
    virtual void updateProcessedData(int objectsProcessed, zen::Int64 dataProcessed);
    virtual void reportStatus(const wxString& text);
    virtual void reportInfo(const wxString& text);
    virtual void forceUiRefresh();

    virtual Response reportError(const wxString& text);
    virtual void reportFatalError(const wxString& errorMessage);
    virtual void reportWarning(const wxString& warningMessage, bool& warningActive);

private:
    virtual void abortThisProcess();

    MainDialog* parentDlg_;
    SyncStatus syncStatusFrame; //the window managed by SyncStatus has longer lifetime than this handler!
    xmlAccess::OnGuiError handleError_;
    zen::ErrorLogging errorLog;
};


#endif // GUISTATUSHANDLER_H_INCLUDED
