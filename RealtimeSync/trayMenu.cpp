// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) 2008-2010 ZenJu (zhnmju123 AT gmx.de)                    *
// **************************************************************************
//
#include "trayMenu.h"
#include <wx/msgdlg.h>
#include <wx/taskbar.h>
#include <wx/app.h>
#include "resources.h"
//#include <memory>
#include <wx/utils.h>
#include <wx/menu.h>
#include "watcher.h"
#include <wx/utils.h>
#include <wx/log.h>
#include "../shared/staticAssert.h"
#include "../shared/buildInfo.h"
#include <wx/icon.h> //Linux needs this
#include <wx/timer.h>

using namespace RealtimeSync;


class WaitCallbackImpl : private wxEvtHandler, public RealtimeSync::WaitCallback //keep this order: else VC++ generated wrong code
{
public:
    WaitCallbackImpl();
    ~WaitCallbackImpl();

    virtual void requestUiRefresh();

    void requestAbort()
    {
        m_abortRequested = true;
    }

    void OnRequestResume(wxCommandEvent& event)
    {
        m_resumeRequested = true;
    }

    enum Selection
    {
        CONTEXT_ABORT,
        CONTEXT_RESTORE,
        CONTEXT_ABOUT
    };

    void OnContextMenuSelection(wxCommandEvent& event);

private:
    class RtsTrayIcon;
    RtsTrayIcon* trayMenu;

    bool m_abortRequested;
    bool m_resumeRequested;
};


//RtsTrayIcon shall be a dumb class whose sole purpose is to enable wxWidgets deferred deletion
class WaitCallbackImpl::RtsTrayIcon : public wxTaskBarIcon
{
public:
    RtsTrayIcon(WaitCallbackImpl* parent) : parent_(parent) {}

    void parentHasDied() //call before tray icon is marked for deferred deletion
    {
        parent_ = NULL;
    }

private:
    virtual wxMenu* CreatePopupMenu()
    {
        if (!parent_)
            return NULL;

        wxMenu* contextMenu = new wxMenu;
        contextMenu->Append(CONTEXT_RESTORE, _("&Restore"));
        contextMenu->Append(CONTEXT_ABOUT, _("&About..."));
        contextMenu->AppendSeparator();
        contextMenu->Append(CONTEXT_ABORT, _("&Exit"));
        //event handling
        contextMenu->Connect(wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(WaitCallbackImpl::OnContextMenuSelection), NULL, parent_);

        return contextMenu; //ownership transferred to caller
    }

    WaitCallbackImpl* parent_;
};
//##############################################################################################################


class AbortThisProcess //exception class
{
public:
    AbortThisProcess(MonitorResponse command) : command_(command) {}

    MonitorResponse getCommand() const
    {
        return command_;
    }

private:
    MonitorResponse command_;
};
//##############################################################################################################


WaitCallbackImpl::WaitCallbackImpl() :
    m_abortRequested(false),
    m_resumeRequested(false)
{
    trayMenu = new RtsTrayIcon(this); //not in initialization list: give it a valid parent object!

#ifdef FFS_WIN
    const wxIcon& realtimeIcon = *GlobalResources::getInstance().programIcon;
#elif defined FFS_LINUX
    wxIcon realtimeIcon;
    realtimeIcon.CopyFromBitmap(GlobalResources::getInstance().getImageByName(wxT("RTS_tray_linux.png"))); //use a 22x22 bitmap for perfect fit
#endif
    trayMenu->SetIcon(realtimeIcon, wxString(wxT("RealtimeSync")) + wxT(" - ") + _("Monitoring active..."));

    //register double-click
    trayMenu->Connect(wxEVT_TASKBAR_LEFT_DCLICK, wxCommandEventHandler(WaitCallbackImpl::OnRequestResume), NULL, this);
}


WaitCallbackImpl::~WaitCallbackImpl()
{
    trayMenu->Disconnect(wxEVT_TASKBAR_LEFT_DCLICK, wxCommandEventHandler(WaitCallbackImpl::OnRequestResume), NULL, this);
    trayMenu->RemoveIcon(); //(try to) hide icon until final deletion takes place
    trayMenu->parentHasDied();

    //use wxWidgets delayed destruction: delete during next idle loop iteration (handle late window messages, e.g. when double-clicking)
    if (!wxPendingDelete.Member(trayMenu))
        wxPendingDelete.Append(trayMenu);
}


void WaitCallbackImpl::OnContextMenuSelection(wxCommandEvent& event)
{
    const int eventId = event.GetId();
    switch (static_cast<Selection>(eventId))
    {
    case CONTEXT_ABORT:
        requestAbort();
        break;
    case CONTEXT_RESTORE:
        OnRequestResume(event); //just remember: never throw exceptions through a C-Layer (GUI) ;)
        break;
    case CONTEXT_ABOUT:
    {
        //build information
        wxString build = wxString(wxT("(")) + _("Build:") + wxT(" ") + __TDATE__;
#if wxUSE_UNICODE
        build += wxT(" - Unicode");
#else
        build += wxT(" - ANSI");
#endif //wxUSE_UNICODE

        //compile time info about 32/64-bit build
        if (Utility::is64BitBuild)
            build += wxT(" x64)");
        else
            build += wxT(" x86)");
        assert_static(Utility::is32BitBuild || Utility::is64BitBuild);

        wxMessageDialog aboutDlg(NULL, wxString(wxT("RealtimeSync")) + wxT("\n\n") + build, _("About"), wxOK);
        aboutDlg.ShowModal();
    }
    break;
    }
}


void WaitCallbackImpl::requestUiRefresh()
{
    if (updateUiIsAllowed())
        wxTheApp->Yield();

    if (m_abortRequested)
        throw ::AbortThisProcess(QUIT);

    if (m_resumeRequested)
        throw ::AbortThisProcess(RESUME);
}
//##############################################################################################################


RealtimeSync::MonitorResponse RealtimeSync::startDirectoryMonitor(const xmlAccess::XmlRealConfig& config)
{
    try
    {
        WaitCallbackImpl callback;

        if (config.commandline.empty())
            throw FreeFileSync::FileError(_("Command line is empty!"));

        long lastExec = 0;
        while (true)
        {
            wxExecute(config.commandline, wxEXEC_SYNC); //execute command
            wxLog::FlushActive(); //show wxWidgets error messages (if any)

            //wait
            waitForChanges(config.directories, &callback);
            lastExec = wxGetLocalTime();

            //some delay
            while (wxGetLocalTime() - lastExec < static_cast<long>(config.delay))
            {
                callback.requestUiRefresh();
                wxMilliSleep(RealtimeSync::UI_UPDATE_INTERVAL);
            }
        }
    }
    catch (const ::AbortThisProcess& ab)
    {
        return ab.getCommand();
    }
    catch (const FreeFileSync::FileError& error)
    {
        wxMessageBox(error.show().c_str(), _("Error"), wxOK | wxICON_ERROR);
        return RESUME;
    }

    return RESUME;
}
