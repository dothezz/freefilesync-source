#include "statusHandler.h"
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
    static wxLongLong lastExec = 0;

    wxLongLong newExec = wxGetLocalTimeMillis();

    if (newExec - lastExec >= UI_UPDATE_INTERVAL)  //perform ui updates not more often than necessary
    {
        lastExec = newExec;
        return true;
    }
    return false;
}
