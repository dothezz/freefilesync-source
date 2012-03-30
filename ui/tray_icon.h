// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) ZenJu (zhnmju123 AT gmx DOT de) - All Rights Reserved    *
// **************************************************************************

#ifndef TRAYICON_H_INCLUDED
#define TRAYICON_H_INCLUDED

#include <wx/event.h>

//show tray icon with progress during lifetime of this instance
//emits the following wxCommandEvent in case user double-clicks on tray icon or selects corresponding context menu item:
extern const wxEventType FFS_REQUEST_RESUME_TRAY_EVENT;

class FfsTrayIcon : public wxEvtHandler
{
public:
    FfsTrayIcon();
    ~FfsTrayIcon();

    void setToolTip(const wxString& toolTipText, double fraction = 0); //number between [0, 1], for small progress indicator

private:
    FfsTrayIcon(const FfsTrayIcon&);
    FfsTrayIcon& operator=(const FfsTrayIcon&);

    void OnContextMenuSelection(wxCommandEvent& event);
    void OnDoubleClick(wxCommandEvent& event);

    class TaskBarImpl;
    TaskBarImpl* trayIcon; //actual tray icon (don't use inheritance to enable delayed deletion)
};

#endif // TRAYICON_H_INCLUDED
