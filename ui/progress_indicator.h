// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) 2008-2010 ZenJu (zhnmju123 AT gmx.de)                    *
// **************************************************************************
//
#ifndef PROGRESSINDICATOR_H_INCLUDED
#define PROGRESSINDICATOR_H_INCLUDED

#include "../shared/zstring.h"
#include <wx/toplevel.h>
#include "../library/status_handler.h"


class CompareStatus
{
public:
    CompareStatus(wxTopLevelWindow& parentWindow); //CompareStatus will be owned by parentWindow!
    ~CompareStatus();

    wxWindow* getAsWindow(); //convenience! don't abuse!

    void init();     //make visible, initialize all status values
    void finalize(); //hide again

    void switchToCompareBytewise(int totalObjectsToProcess, wxLongLong totalDataToProcess);
    void incScannedObjects_NoUpdate(int number);
    void incProcessedCmpData_NoUpdate(int objectsProcessed, wxLongLong dataProcessed);
    void setStatusText_NoUpdate(const Zstring& text);
    void updateStatusPanelNow();

private:
    class CompareStatusImpl;
    CompareStatusImpl* const pimpl;
};


class SyncStatus
{
public:
    SyncStatus(StatusHandler& updater,
               wxTopLevelWindow* parentWindow, //may be NULL
               bool startSilent);
    ~SyncStatus();

    wxWindow* getAsWindow(); //convenience! don't abuse!

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

    void resetGauge(int totalObjectsToProcess, wxLongLong totalDataToProcess);
    void incScannedObjects_NoUpdate(int number);
    void incProgressIndicator_NoUpdate(int objectsProcessed, wxLongLong dataProcessed);
    void setStatusText_NoUpdate(const Zstring& text);
    void updateStatusDialogNow();

    void setCurrentStatus(SyncStatusID id);

    //essential to call one of these two methods in StatusUpdater derived class destructor at the LATEST(!)
    //to prevent access to callback to updater (e.g. request abort)
    void processHasFinished(SyncStatusID id, const wxString& finalMessage);
    void closeWindowDirectly(); //don't wait for user

private:
    class SyncStatusImpl;
    SyncStatusImpl* const pimpl;
};

#endif // PROGRESSINDICATOR_H_INCLUDED
