#include "trayIcon.h"
#include "../library/resources.h"
#include "smallDialogs.h"
#include "../library/statusHandler.h"
#include <wx/taskbar.h>


enum Selection
{
    CONTEXT_RESTORE,
    CONTEXT_ABOUT
};


class MinimizeToTray::TaskBarImpl : public wxTaskBarIcon
{
public:
    TaskBarImpl(MinimizeToTray* parent) : parent_(parent) {}

    void parentHasDied()
    {
        parent_ = NULL;
    }
private:
    virtual wxMenu* CreatePopupMenu()
    {
        if (!parent_)
            return NULL;

        wxMenu* contextMenu = new wxMenu;
        contextMenu->Append(CONTEXT_ABOUT, _("&About..."));
        contextMenu->AppendSeparator();
        contextMenu->Append(CONTEXT_RESTORE, _("&Restore"));
        //event handling
        contextMenu->Connect(wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(MinimizeToTray::OnContextMenuSelection), NULL, parent_);

        return contextMenu; //ownership transferred to library
    }

    MinimizeToTray* parent_;
};


MinimizeToTray::MinimizeToTray(wxTopLevelWindow* callerWnd, wxWindow* secondWnd) :
    callerWnd_(callerWnd),
    secondWnd_(secondWnd),
    trayIcon(new TaskBarImpl(this))
{
    trayIcon->SetIcon(*GlobalResources::getInstance().programIcon, wxT("FreeFileSync"));
    trayIcon->Connect(wxEVT_TASKBAR_LEFT_DCLICK, wxCommandEventHandler(MinimizeToTray::OnDoubleClick), NULL, this); //register double-click

    if (callerWnd_)
        callerWnd_->Hide();
    if (secondWnd_)
        secondWnd_->Hide();
}


MinimizeToTray::~MinimizeToTray()
{
    resumeFromTray();
}


void MinimizeToTray::resumeFromTray() //remove trayIcon and restore windows:  MinimizeToTray is now a zombie object...
{
    if (trayIcon)
    {
        if (secondWnd_)
            secondWnd_->Show();

        if (callerWnd_) //usecase: avoid dialog flashing in batch silent mode
        {
            callerWnd_->Iconize(false);
            callerWnd_->Show();
            callerWnd_->Raise();
            callerWnd_->SetFocus();
        }
        trayIcon->RemoveIcon(); //hide icon until final deletion takes place
        trayIcon->Disconnect(wxEVT_TASKBAR_LEFT_DCLICK, wxCommandEventHandler(MinimizeToTray::OnDoubleClick), NULL, this);

        //delayed destruction: delete during next idle loop iteration (handle late window messages, e.g. when double-clicking)
        if (!wxPendingDelete.Member(trayIcon))
            wxPendingDelete.Append(trayIcon);

        trayIcon->parentHasDied(); //TaskBarImpl (potentially) has longer lifetime than MinimizeToTray: avoid callback!
        trayIcon = NULL; //avoid reentrance
    }
}


void MinimizeToTray::setToolTip(const wxString& toolTipText)
{
    if (trayIcon)
        trayIcon->SetIcon(*GlobalResources::getInstance().programIcon, toolTipText);
}


void MinimizeToTray::keepHidden()
{
    callerWnd_ = NULL;
    secondWnd_ = NULL;
}


void MinimizeToTray::OnContextMenuSelection(wxCommandEvent& event)
{
    const Selection eventId = static_cast<Selection>(event.GetId());
    switch (eventId)
    {
    case CONTEXT_ABOUT:
    {
        AboutDlg* aboutDlg = new AboutDlg(NULL);
        aboutDlg->ShowModal();
        aboutDlg->Destroy();
    }
    break;
    case CONTEXT_RESTORE:
        resumeFromTray();
    }
}


void MinimizeToTray::OnDoubleClick(wxCommandEvent& event)
{
    resumeFromTray();
}
