// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) Zenju (zenju AT gmx DOT de) - All Rights Reserved        *
// **************************************************************************

#ifndef PROGRESSINDICATOR_H_INCLUDED
#define PROGRESSINDICATOR_H_INCLUDED

#include <zen/error_log.h>
#include <zen/zstring.h>
#include <wx/toplevel.h>
#include "../lib/status_handler.h"
#include "main_dlg.h"


class CompareProgressDialog
{
public:
    CompareProgressDialog(wxTopLevelWindow& parentWindow); //CompareProgressDialog will be owned by parentWindow!

    wxWindow* getAsWindow(); //convenience! don't abuse!

    void init(const zen::Statistics& syncStat); //begin of sync: make visible, set pointer to "syncStat", initialize all status values
    void finalize(); //end of sync: hide again, clear pointer to "syncStat"

    void switchToCompareBytewise();
    void updateStatusPanelNow();

private:
    class Pimpl;
    Pimpl* const pimpl;
};


class SyncProgressDialog
{
public:
    SyncProgressDialog(zen::AbortCallback& abortCb,
                       const zen::Statistics& syncStat,
                       MainDialog* parentWindow, //may be nullptr
                       bool showProgress,
                       const wxString& jobName,
                       const std::wstring& execWhenFinished,
                       std::vector<std::wstring>& execFinishedHistory); //changing parameter!
    ~SyncProgressDialog();

    wxWindow* getAsWindow(); //convenience! don't abuse!

    void initNewPhase(); //call after "StatusHandler::initNewPhase"

    void notifyProgressChange(); //throw (), required by graph!
    void updateGui();

    std::wstring getExecWhenFinishedCommand() const; //final value (after possible user modification)

    void stopTimer();   //halt all internal counters!
    void resumeTimer(); //

    enum SyncResult
    {
        RESULT_ABORTED,
        RESULT_FINISHED_WITH_ERROR,
        RESULT_FINISHED_WITH_WARNINGS,
        RESULT_FINISHED_WITH_SUCCESS
    };
    //essential to call one of these two methods in StatusUpdater derived class destructor at the LATEST(!)
    //to prevent access to callback to updater (e.g. request abort)
    void processHasFinished(SyncResult resultId, const zen::ErrorLog& log); //sync finished, still dialog may live on
    void closeWindowDirectly(); //don't wait for user

private:
    class Pimpl;
    Pimpl* const pimpl;
};


class PauseTimers
{
public:
    PauseTimers(SyncProgressDialog& ss) : ss_(ss) { ss_.stopTimer(); }
    ~PauseTimers() { ss_.resumeTimer(); }
private:
    SyncProgressDialog& ss_;
};


#endif // PROGRESSINDICATOR_H_INCLUDED
