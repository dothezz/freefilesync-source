// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) ZenJu (zhnmju123 AT gmx DOT de) - All Rights Reserved    *
// **************************************************************************

#ifndef PROC_HEADER_48257827842345454545
#define PROC_HEADER_48257827842345454545

#include <string>
#include <zen/int64.h>

//interface for comparison and synchronization process status updates (used by GUI or Batch mode)
const int UI_UPDATE_INTERVAL = 100; //unit: [ms]; perform ui updates not more often than necessary,
//100 seems to be a good value with only a minimal performance loss; also used by Win 7 copy progress bar
//this one is required by async directory existence check!

//report status during comparison and synchronization
struct ProcessCallback
{
    virtual ~ProcessCallback() {}

    //these methods have to be implemented in the derived classes to handle error and status information

    //notify synchronization phases
    enum Phase
    {
        PHASE_NONE, //initial status
        PHASE_SCANNING,
        PHASE_COMPARING_CONTENT,
        PHASE_SYNCHRONIZING
    };
    virtual void initNewPhase(int objectsTotal, zen::Int64 dataTotal, Phase phaseId) = 0; //informs about the estimated amount of data that will be processed in this phase

    //note: this one must NOT throw in order to properly allow undoing setting of statistics!
    //it is in general paired with a call to requestUiRefresh() to compensate!
    virtual void updateProcessedData(int objectsDelta, zen::Int64 dataDelta) = 0; //throw()!!
    virtual void updateTotalData    (int objectsDelta, zen::Int64 dataDelta) = 0; //
    /*the estimated total may change *during* sync:
    		1. move file -> fallback to copy + delete
    		2. file copy, actual size changed after comparison or file contains significant ADS data
            3. auto-resolution for failed create operations due to missing source
    		4. directory deletion: may contain more items than scanned by FFS: excluded by filter
    		5. delete directory to recycler or move to user-defined dir on same volume: no matter how many sub-elements exist, this is only 1 object to process!
    		6. user-defined deletion directory on different volume: full file copy required (instead of move)
            7. Copy sparse files */

    //opportunity to abort must be implemented in a frequently executed method like requestUiRefresh()
    virtual void requestUiRefresh() = 0; //throw ?
    virtual void forceUiRefresh  () = 0; //throw ? - called before starting long running task which doesn't update regularly

    //called periodically after data was processed: expected(!) to request GUI update
    virtual void reportStatus(const std::wstring& text) = 0; //UI info only, should not be logged!

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

#endif //PROC_HEADER_48257827842345454545