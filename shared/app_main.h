// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) 2008-2011 ZenJu (zhnmju123 AT gmx.de)                    *
// **************************************************************************

#ifndef APPMAIN_H_INCLUDED
#define APPMAIN_H_INCLUDED

class wxWindow;

namespace zen
{
//just some wrapper around a global variable representing the (logical) main application window
class AppMainWindow
{
public:
    static void setMainWindow(wxWindow* window); //set main window and enable "exit on frame delete"
    static bool mainWindowWasSet();

private:
    static bool mainWndActive;
};
}


#endif // APPMAIN_H_INCLUDED
