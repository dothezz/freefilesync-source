// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) 2008-2011 ZenJu (zhnmju123 AT gmx.de)                    *
// **************************************************************************

#include "app_main.h"
#include <wx/window.h>
#include <wx/app.h>

using namespace zen;


bool AppMainWindow::mainWndActive = false;


void zen::AppMainWindow::setMainWindow(wxWindow* window)
{
    wxTheApp->SetTopWindow(window);
    wxTheApp->SetExitOnFrameDelete(true);

    mainWndActive = true;
}


bool AppMainWindow::mainWindowWasSet()
{
    return mainWndActive;
}
