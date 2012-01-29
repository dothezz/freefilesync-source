// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) ZenJu (zhnmju123 AT gmx DOT de) - All Rights Reserved    *
// **************************************************************************

#ifndef PROGRESSINDICATOR_H_INCLUDED
#define PROGRESSINDICATOR_H_INCLUDED

#include <zen/zstring.h>
#include <wx/toplevel.h>
#include "../lib/status_handler.h"
#include "main_dlg.h"

namespace zen
{
class ErrorLogging;
}


class CompareStatus
{
public:
    CompareStatus(wxTopLevelWindow& parentWindow); //CompareStatus will be owned by parentWindow!
    ~CompareStatus();

    wxWindow* getAsWindow(); //convenience! don't abuse!

    void init();     //make visible, initialize all status values
    void finalize(); //hide again

    void switchToCompareBytewise(int totalObjectsToProcess, zen::Int64 totalDataToProcess);
    void incScannedObjects_NoUpdate(int number);
    void incProcessedCmpData_NoUpdate(int objectsDelta, zen::Int64 dataDelta);
    void incTotalCmpData_NoUpdate    (int objectsDelta, zen::Int64 dataDelta);
    void setStatusText_NoUpdate(const wxString& text);
    void updateStatusPanelNow();

private:
    class CompareStatusImpl;
    CompareStatusImpl* const pimpl;
};


class SyncStatus
{
public:
    enum SyncStatusID
    {
        ABORTED,
        FINISHED_WITH_SUCCESS,
        FINISHED_WITH_ERROR,
        PAUSE,
        SCANNING,
        COMPARING_CONTENT,
        SYNCHRONIZING
    };

    SyncStatus(AbortCallback& abortCb,
               MainDialog* parentWindow, //may be NULL
               SyncStatusID startStatus,
               bool showProgress,
               const wxString& jobName,
               const std::wstring& execWhenFinished,
               std::vector<std::wstring>& execFinishedHistory); //changing parameter!
    ~SyncStatus();

    wxWindow* getAsWindow(); //convenience! don't abuse!

    void initNewProcess(SyncStatusID id, int totalObjectsToProcess, zen::Int64 totalDataToProcess);

    void incScannedObjects_NoUpdate(int number);
    void incProcessedData_NoUpdate(int objectsDelta, zen::Int64 dataDelta);
    void incTotalData_NoUpdate    (int objectsDelta, zen::Int64 dataDelta);
    void setStatusText_NoUpdate(const wxString& text);
    void updateStatusDialogNow();

    std::wstring getExecWhenFinishedCommand() const; //final value (after possible user modification)

    void stopTimer();   //halt all internal counters!
    void resumeTimer(); //

    //essential to call one of these two methods in StatusUpdater derived class destructor at the LATEST(!)
    //to prevent access to callback to updater (e.g. request abort)
    void processHasFinished(SyncStatusID id, const zen::ErrorLogging& log);
    void closeWindowDirectly(); //don't wait for user

private:
    class SyncStatusImpl;
    SyncStatusImpl* const pimpl;
};


class PauseTimers
{
public:
    PauseTimers(SyncStatus& ss) : ss_(ss) { ss_.stopTimer(); }
    ~PauseTimers() { ss_.resumeTimer(); }
private:
    SyncStatus& ss_;
};


#endif // PROGRESSINDICATOR_H_INCLUDED
