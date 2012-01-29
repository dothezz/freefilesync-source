// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) ZenJu (zhnmju123 AT gmx DOT de) - All Rights Reserved    *
// **************************************************************************

#include "status_handler.h"
#include <wx/app.h>
#include <ctime>

void updateUiNow()
{
    //process UI events and prevent application from "not responding"   -> NO performance issue!
    wxTheApp->Yield();

    //    while (wxTheApp->Pending())
    //        wxTheApp->Dispatch();
}


bool updateUiIsAllowed()
{
    const std::clock_t CLOCK_UPDATE_INTERVAL = UI_UPDATE_INTERVAL * CLOCKS_PER_SEC / 1000;

    static std::clock_t lastExec = 0;
    const std::clock_t now = std::clock(); //this is quite fast: 2 * 10^-5

    if (now - lastExec >= CLOCK_UPDATE_INTERVAL)  //perform ui updates not more often than necessary
    {
        lastExec = now;
        return true;
    }
    return false;
}
