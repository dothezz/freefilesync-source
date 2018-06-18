// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) Zenju (zenju AT gmx DOT de) - All Rights Reserved        *
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
    virtual void updateProcessedData(int objectsDelta, zen::Int64 dataDelta) = 0; //noexcept!!
    virtual void updateTotalData    (int objectsDelta, zen::Int64 dataDelta) = 0; //
    /*the estimated and actual total workload may change *during* sync:
    		1. detected file can be moved -> fallback to copy + delete
    		2. file copy, actual size changed after comparison
    		3. file contains significant ADS data, is sparse or compressed
            4. auto-resolution for failed create operations due to missing source
    		5. directory deletion: may contain more items than scanned by FFS (excluded by filter) or less (contains followed symlinks)
    		6. delete directory to recycler: no matter how many child-elements exist, this is only 1 item to process!
    		7. file/directory already deleted externally: nothing to do, 0 logical operations and data
    		8. user-defined deletion directory on different volume: full file copy required (instead of move)
    		9. Binary file comparison: if files differ at the first few bytes, the result is already known
    		10. Error during file copy, retry: bytes were copied => increases total workload!
    */

    //opportunity to abort must be implemented in a frequently executed method like requestUiRefresh()
    virtual void requestUiRefresh() = 0; //throw ?
    virtual void forceUiRefresh  () = 0; //throw ? - called before starting long running tasks which don't update regularly

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
