// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) Zenju (zenju AT gmx DOT de) - All Rights Reserved        *
// **************************************************************************

#ifndef STATUSHANDLER_H_INCLUDED
#define STATUSHANDLER_H_INCLUDED

#include "../process_callback.h"
#include <vector>
#include <string>
#include <zen/int64.h>
#include <zen/i18n.h>

namespace zen
{
bool updateUiIsAllowed(); //test if a specific amount of time is over

/*
Updating GUI is fast!
    time per single call to ProcessCallback::forceUiRefresh()
    - Comparison       0.025 ms
    - Synchronization  0.74 ms (despite complex graph control!)
*/

//gui may want to abort process
struct AbortCallback
{
    virtual ~AbortCallback() {}
    virtual void requestAbortion() = 0;
};


//common statistics "everybody" needs
struct Statistics
{
    virtual ~Statistics() {}

    virtual ProcessCallback::Phase currentPhase() const = 0;

    virtual int getObjectsCurrent(ProcessCallback::Phase phaseId) const = 0;
    virtual int getObjectsTotal  (ProcessCallback::Phase phaseId) const = 0;

    virtual Int64 getDataCurrent(ProcessCallback::Phase phaseId) const = 0;
    virtual Int64 getDataTotal  (ProcessCallback::Phase phaseId) const = 0;

    virtual const std::wstring& currentStatusText() const = 0;
};


//partial callback implementation with common functionality for "batch", "GUI/Compare" and "GUI/Sync"
class StatusHandler : public ProcessCallback, public AbortCallback, public Statistics
{
public:
    StatusHandler() : currentPhase_(PHASE_NONE),
        numbersCurrent_(4), //init with phase count
        numbersTotal_  (4), //
        abortRequested(false) {}

protected:
    //implement parts of ProcessCallback
    virtual void initNewPhase(int objectsTotal, Int64 dataTotal, Phase phaseId) //may throw
    {
        currentPhase_ = phaseId;
        refNumbers(numbersTotal_, currentPhase_) = std::make_pair(objectsTotal, dataTotal);
    }

    virtual void updateProcessedData(int objectsDelta, Int64 dataDelta) { updateData(numbersCurrent_, objectsDelta, dataDelta); } //note: these methods MUST NOT throw in order
    virtual void updateTotalData    (int objectsDelta, Int64 dataDelta) { updateData(numbersTotal_  , objectsDelta, dataDelta); } //to properly allow undoing setting of statistics!

    virtual void requestUiRefresh() //throw X
    {
        if (abortRequested) //triggered by requestAbortion()
        {
            forceUiRefresh();
            abortProcessNow(); //throw X
        }
        else if (updateUiIsAllowed()) //test if specific time span between ui updates is over
            forceUiRefresh();
    }

    virtual void reportStatus(const std::wstring& text) { statusText_ = text; requestUiRefresh(); /*throw X */ }
    virtual void reportInfo  (const std::wstring& text) { statusText_ = text; requestUiRefresh(); /*throw X */ } //log text in derived class

    //implement AbortCallback
    virtual void requestAbortion()
    {
        abortRequested = true;
        statusText_ = _("Stop requested: Waiting for current operation to finish...");
    } //this does NOT call abortProcessNow() immediately, but when we're out of the C GUI call stack

    //implement Statistics
    virtual Phase currentPhase() const { return currentPhase_; }

    virtual int getObjectsCurrent(Phase phaseId) const {                                    return refNumbers(numbersCurrent_, phaseId).first; }
    virtual int getObjectsTotal  (Phase phaseId) const { assert(phaseId != PHASE_SCANNING); return refNumbers(numbersTotal_  , phaseId).first; }

    virtual Int64 getDataCurrent(Phase phaseId) const { assert(phaseId != PHASE_SCANNING); return refNumbers(numbersCurrent_, phaseId).second; }
    virtual Int64 getDataTotal  (Phase phaseId) const { assert(phaseId != PHASE_SCANNING); return refNumbers(numbersTotal_  , phaseId).second; }

    virtual const std::wstring& currentStatusText() const { return statusText_; }

    bool abortIsRequested() const { return abortRequested; }

private:
    typedef std::vector<std::pair<int, Int64>> StatNumbers;

    void updateData(StatNumbers& num, int objectsDelta, Int64 dataDelta)
    {
        auto& st = refNumbers(num, currentPhase_);
        st.first  += objectsDelta;
        st.second += dataDelta;
    }

    static const std::pair<int, Int64>& refNumbers(const StatNumbers& num, Phase phaseId)
    {
        switch (phaseId)
        {
            case PHASE_SCANNING:
                return num[0];
            case PHASE_COMPARING_CONTENT:
                return num[1];
            case PHASE_SYNCHRONIZING:
                return num[2];
            case PHASE_NONE:
                break;
        }
        assert(false);
        return num[3]; //dummy entry!
    }

    static std::pair<int, Int64>& refNumbers(StatNumbers& num, Phase phaseId) { return const_cast<std::pair<int, Int64>&>(refNumbers(static_cast<const StatNumbers&>(num), phaseId)); }

    Phase currentPhase_;
    StatNumbers numbersCurrent_;
    StatNumbers numbersTotal_;
    std::wstring statusText_;

    bool abortRequested;
};
}

#endif // STATUSHANDLER_H_INCLUDED
