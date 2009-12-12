#ifndef TRAYICON_H_INCLUDED
#define TRAYICON_H_INCLUDED

#include <wx/event.h>
#include <wx/toplevel.h>


class MinimizeToTray : private wxEvtHandler
{
public:
    MinimizeToTray(wxTopLevelWindow* callerWnd, wxWindow* secondWnd = NULL); //ensure callerWind has longer lifetime!
    ~MinimizeToTray(); //show windows again

    void setToolTip(const wxString& toolTipText);
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
