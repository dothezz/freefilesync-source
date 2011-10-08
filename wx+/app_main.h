// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) 2008-2011 ZenJu (zhnmju123 AT gmx.de)                    *
// **************************************************************************

#ifndef APPMAIN_H_INCLUDED
#define APPMAIN_H_INCLUDED

#include <wx/window.h>
#include <wx/app.h>

namespace zen
{
//just some wrapper around a global variable representing the (logical) main application window
void setMainWindow(wxWindow* window); //set main window and enable "exit on frame delete"
bool mainWindowWasSet();








//######################## implementation ########################
inline
bool& getMainWndStatus()
{
    static bool status = false; //external linkage!
    return status;
}


inline
void setMainWindow(wxWindow* window)
{
    wxTheApp->SetTopWindow(window);
    wxTheApp->SetExitOnFrameDelete(true);

    getMainWndStatus() = true;
}


inline
bool mainWindowWasSet() { return getMainWndStatus(); }
}

#endif // APPMAIN_H_INCLUDED
