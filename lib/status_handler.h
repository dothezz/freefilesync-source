// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) ZenJu (zhnmju123 AT gmx DOT de) - All Rights Reserved    *
// **************************************************************************

#ifndef STATUSHANDLER_H_INCLUDED
#define STATUSHANDLER_H_INCLUDED

#include "../process_callback.h"
#include <string>
#include <zen/int64.h>

const int UI_UPDATE_INTERVAL = 100; //unit: [ms]; perform ui updates not more often than necessary, 100 seems to be a good value with only a minimal performance loss

bool updateUiIsAllowed(); //test if a specific amount of time is over
void updateUiNow();       //do the updating

/*
Updating GUI is fast!
    time per single call to ProcessCallback::forceUiRefresh()
    - Comparison        25 µs
    - Synchronization  0.6 ms   (despite complex graph control!)
*/

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
