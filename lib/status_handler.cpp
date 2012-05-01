// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) ZenJu (zhnmju123 AT gmx DOT de) - All Rights Reserved    *
// **************************************************************************

#include "status_handler.h"
#include <wx/app.h>
#include <zen/tick_count.h>

using namespace zen;


void zen::updateUiNow()
{
    //process UI events and prevent application from "not responding"   -> NO performance issue!
    wxTheApp->Yield();

    //    while (wxTheApp->Pending())
    //        wxTheApp->Dispatch();
}

namespace
{
const std::int64_t TICKS_UPDATE_INTERVAL = UI_UPDATE_INTERVAL* ticksPerSec() / 1000;
TickVal lastExec = getTicks();
};

bool zen::updateUiIsAllowed()
{
    const TickVal now = getTicks(); //0 on error
    if (now - lastExec >= TICKS_UPDATE_INTERVAL)  //perform ui updates not more often than necessary
    {
        lastExec = now;
        return true;
    }
    return false;
}