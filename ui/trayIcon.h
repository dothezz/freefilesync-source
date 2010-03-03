// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) 2008-2010 ZenJu (zhnmju123 AT gmx.de)                    *
// **************************************************************************
//
#ifndef TRAYICON_H_INCLUDED
#define TRAYICON_H_INCLUDED

#include <wx/event.h>
#include <wx/toplevel.h>


class MinimizeToTray : private wxEvtHandler
{
public:
    MinimizeToTray(wxTopLevelWindow* callerWnd, wxWindow* secondWnd = NULL); //ensure callerWind has longer lifetime!
    ~MinimizeToTray(); //show windows again

    void setToolTip(const wxString& toolTipText, size_t percent = 0); //percent (optional), number between [0, 100], for small progress indicator
    void keepHidden(); //do not show windows again: avoid window flashing shortly before it is destroyed

private:
    void OnContextMenuSelection(wxCommandEvent& event);
    void OnDoubleClick(wxCommandEvent& event);
    void resumeFromTray();

    wxTopLevelWindow* callerWnd_;
    wxWindow* secondWnd_;
    class TaskBarImpl;
    TaskBarImpl* trayIcon; //actual tray icon (don't use inheritance to enable delayed deletion)
};

#endif // TRAYICON_H_INCLUDED
