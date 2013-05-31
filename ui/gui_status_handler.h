// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) Zenju (zenju AT gmx DOT de) - All Rights Reserved        *
// **************************************************************************

#ifndef GUISTATUSHANDLER_H_INCLUDED
#define GUISTATUSHANDLER_H_INCLUDED

#include <wx/event.h>
#include <zen/error_log.h>
#include <wx/stopwatch.h>
#include "progress_indicator.h"
#include "../lib/status_handler.h"
#include "../lib/process_xml.h"
#include "main_dlg.h"


//Exception class used to abort the "compare" and "sync" process
class GuiAbortProcess {};

//classes handling sync and compare error as well as status information

//CompareStatusHandler(CompareProgressDialog) will internally process Window messages! disable GUI controls to avoid unexpected callbacks!
class CompareStatusHandler : private wxEvtHandler, public zen::StatusHandler //throw GuiAbortProcess
{
public:
    CompareStatusHandler(MainDialog& dlg);
    ~CompareStatusHandler();

    virtual void initNewPhase(int objectsTotal, zen::Int64 dataTotal, Phase phaseID);
    virtual void forceUiRefresh();

    virtual Response reportError(const std::wstring& text);
    virtual void reportFatalError(const std::wstring& errorMessage);
    virtual void reportWarning(const std::wstring& warningMessage, bool& warningActive);

private:
    void OnKeyPressed(wxKeyEvent& event);
    void OnAbortCompare(wxCommandEvent& event); //handle abort button click
    virtual void abortThisProcess(); //throw GuiAbortProcess

    MainDialog& mainDlg;
    bool ignoreErrors;
};


//SyncStatusHandler(SyncProgressDialog) will internally process Window messages! disable GUI controls to avoid unexpected callbacks!
class SyncStatusHandler : public zen::StatusHandler
{
public:
    SyncStatusHandler(wxTopLevelWindow* parentDlg,
                      size_t lastSyncsLogFileSizeMax,
                      xmlAccess::OnGuiError handleError,
                      const std::wstring& jobName,
                      const std::wstring& execWhenFinished,
                      std::vector<std::wstring>& execFinishedHistory);
    ~SyncStatusHandler();

    virtual void initNewPhase       (int objectsTotal, zen::Int64 dataTotal, Phase phaseID);
    virtual void updateProcessedData(int objectsDelta, zen::Int64 dataDelta);
    virtual void reportInfo(const std::wstring& text);
    virtual void forceUiRefresh();

    virtual Response reportError(const std::wstring& text);
    virtual void reportFatalError(const std::wstring& errorMessage);
    virtual void reportWarning(const std::wstring& warningMessage, bool& warningActive);

private:
    virtual void abortThisProcess(); //throw GuiAbortProcess
    void onProgressDialogTerminate();

    wxTopLevelWindow* parentDlg_;
    SyncProgressDialog* progressDlg; //managed to have shorter lifetime than this handler!
    const size_t lastSyncsLogFileSizeMax_;
    xmlAccess::OnGuiError handleError_;
    zen::ErrorLog errorLog;
    const std::wstring jobName_;
    wxStopWatch totalTime;
};


#endif // GUISTATUSHANDLER_H_INCLUDED
