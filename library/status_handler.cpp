// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) 2008-2011 ZenJu (zhnmju123 AT gmx.de)                    *
// **************************************************************************
//
#include "status_handler.h"
#include <wx/app.h>
#include <wx/timer.h>


void updateUiNow()
{
    //process UI events and prevent application from "not responding"   -> NO performance issue!
    wxTheApp->Yield();

    //    while (wxTheApp->Pending())
    //        wxTheApp->Dispatch();
}


bool updateUiIsAllowed()
{
    static wxMilliClock_t lastExec = 0;
    const wxMilliClock_t  newExec  = wxGetLocalTimeMillis();

    if (newExec - lastExec >= UI_UPDATE_INTERVAL)  //perform ui updates not more often than necessary
    {
        lastExec = newExec;
        return true;
    }
    return false;
}
