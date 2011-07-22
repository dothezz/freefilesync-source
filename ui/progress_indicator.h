// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) 2008-2011 ZenJu (zhnmju123 AT gmx.de)                    *
// **************************************************************************
//
#ifndef PROGRESSINDICATOR_H_INCLUDED
#define PROGRESSINDICATOR_H_INCLUDED

#include "../shared/zstring.h"
#include <wx/toplevel.h>
#include "../library/status_handler.h"
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
    void incProcessedCmpData_NoUpdate(int objectsProcessed, zen::Int64 dataProcessed);
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
               bool startSilent,
               const wxString& jobName);
    ~SyncStatus();

    wxWindow* getAsWindow(); //convenience! don't abuse!

    void resetGauge(int totalObjectsToProcess, zen::Int64 totalDataToProcess);
    void incScannedObjects_NoUpdate(int number);
    void incProgressIndicator_NoUpdate(int objectsProcessed, zen::Int64 dataProcessed);
    void setStatusText_NoUpdate(const wxString& text);
    void updateStatusDialogNow();

    void setCurrentStatus(SyncStatusID id);

    //essential to call one of these two methods in StatusUpdater derived class destructor at the LATEST(!)
    //to prevent access to callback to updater (e.g. request abort)
    void processHasFinished(SyncStatusID id, const zen::ErrorLogging& log);
    void closeWindowDirectly(); //don't wait for user

private:
    class SyncStatusImpl;
    SyncStatusImpl* const pimpl;
};

#endif // PROGRESSINDICATOR_H_INCLUDED
