// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) 2008-2010 ZenJu (zhnmju123 AT gmx.de)                    *
// **************************************************************************
//
#include "appMain.h"
#include <wx/window.h>
#include <wx/app.h>

using namespace FreeFileSync;


bool AppMainWindow::mainWndAct = false;


void FreeFileSync::AppMainWindow::setMainWindow(wxWindow* window)
{
    wxTheApp->SetTopWindow(window);
    wxTheApp->SetExitOnFrameDelete(true);

    mainWndAct = true;
}


bool AppMainWindow::mainWindowWasSet()
{
    return mainWndAct;
}
