#include "appMain.h"
#include <wx/window.h>
#include <wx/app.h>

using namespace FreeFileSync;


bool AppMainWindow::mainWndAct = false;


void FreeFileSync::AppMainWindow::setMainWindow(wxWindow* window)
{
    wxTheApp->SetTopWindow(window);
    wxTheApp->SetExitOnFrameDelete(true);

    assert (!mainWndAct);
    mainWndAct = true;
}


bool AppMainWindow::mainWindowWasSet()
{
    return mainWndAct;
}
