// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) 2008-2010 ZenJu (zhnmju123 AT gmx.de)                    *
// **************************************************************************
//
#include "app_main.h"
#include <wx/window.h>
#include <wx/app.h>

using namespace ffs3;


bool AppMainWindow::mainWndAct = false;


void ffs3::AppMainWindow::setMainWindow(wxWindow* window)
{
    wxTheApp->SetTopWindow(window);
    wxTheApp->SetExitOnFrameDelete(true);

    mainWndAct = true;
}


bool AppMainWindow::mainWindowWasSet()
{
    return mainWndAct;
}
