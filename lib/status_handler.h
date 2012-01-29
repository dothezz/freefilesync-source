// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) ZenJu (zhnmju123 AT gmx DOT de) - All Rights Reserved    *
// **************************************************************************

#ifndef STATUSHANDLER_H_INCLUDED
#define STATUSHANDLER_H_INCLUDED

#include <string>
#include <zen/int64.h>

const int UI_UPDATE_INTERVAL = 100; //unit: [ms]; perform ui updates not more often than necessary, 100 seems to be a good value with only a minimal performance loss

bool updateUiIsAllowed(); //test if a specific amount of time is over
void updateUiNow();       //do the updating

/*
Updating GUI is fast!
    time per single call to ProcessCallback::forceUiRefresh()
    - Comparison        25 µs
    - Synchronization  0.6 ms   (despite complex graph!)
*/

//interfaces for status updates (can be implemented by GUI or Batch mode)


//report status during comparison and synchronization
struct ProcessCallback
{
    virtual ~ProcessCallback() {}

    //identifiers of different phases
    enum Process
    {
        PROCESS_NONE = 10,
        PROCESS_SCANNING,
        PROCESS_COMPARING_CONTENT,
        PROCESS_SYNCHRONIZING
    };

    //these methods have to be implemented in the derived classes to handle error and status information
    virtual void initNewProcess(int objectsTotal, zen::Int64 dataTotal, Process processID) = 0; //informs about the total amount of data that will be processed from now on

    //note: this one must NOT throw in order to properly allow undoing setting of statistics!
    //it is in general paired with a call to requestUiRefresh() to compensate!
    virtual void updateProcessedData(int objectsDelta, zen::Int64 dataDelta) = 0; //throw()!!
    virtual void updateTotalData    (int objectsDelta, zen::Int64 dataDelta) = 0; //
    /*the estimated total may change *during* sync:
    		1. move file -> fallback to copy + delete
    		2. file copy, actual size changed after comparison
            3. auto-resolution for failed create operations due to missing source
    		4. directory deletion: may contain more items than scanned by FFS: excluded by filter
    		5. delete directory to recycler or move to user-defined dir on same volume: no matter how many sub-elements exist, this is only 1 object to process!
    		6. user-defined deletion directory on different volume: full file copy required (instead of move) */

    //opportunity to abort must be implemented in a frequently executed method like requestUiRefresh()
    virtual void requestUiRefresh() = 0; //throw ?
    virtual void forceUiRefresh  () = 0; //throw ? - call before starting long running task which doesn't update regularly

    //called periodically after data was processed: expected(!) to request GUI update
    virtual void reportStatus(const std::wstring& text) = 0; //status info only, should not be logged!

    //called periodically after data was processed: expected(!) to request GUI update
    virtual void reportInfo(const std::wstring& text) = 0;

    virtual void reportWarning(const std::wstring& warningMessage, bool& warningActive) = 0;

    //error handling:
    enum Response
    {
        IGNORE_ERROR = 10,
        RETRY
    };
    virtual Response reportError     (const std::wstring& errorMessage) = 0; //recoverable error situation
    virtual void     reportFatalError(const std::wstring& errorMessage) = 0; //non-recoverable error situation
};


//gui may want to abort process
struct AbortCallback
{
    virtual ~AbortCallback() {}
    virtual void requestAbortion() = 0;
};


//actual callback implementation will have to satisfy "process" and "gui"
class StatusHandler : public ProcessCallback, public AbortCallback
{
public:
    StatusHandler() : abortRequested(false) {}

    virtual void requestUiRefresh()
    {
        if (updateUiIsAllowed())  //test if specific time span between ui updates is over
            forceUiRefresh();

        if (abortRequested)
            abortThisProcess();  //abort can be triggered by requestAbortion()
    }

    virtual void abortThisProcess() = 0;
    virtual void requestAbortion() { abortRequested = true; } //this does NOT call abortThisProcess immediately, but when appropriate (e.g. async. processes finished)
    bool abortIsRequested() { return abortRequested; }

private:
    bool abortRequested;
};



#endif // STATUSHANDLER_H_INCLUDED
