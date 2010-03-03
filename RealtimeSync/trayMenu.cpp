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
#include <memory>
#include <wx/utils.h>
#include <wx/menu.h>
#include "watcher.h"
#include <wx/timer.h>
#include <wx/utils.h>
#include <wx/log.h>
#include "../shared/staticAssert.h"
#include "../shared/buildInfo.h"
#include <wx/icon.h> //Linux needs this

class RtsTrayIcon;


class WaitCallbackImpl : public RealtimeSync::WaitCallback
{
public:
    WaitCallbackImpl();

    virtual void requestUiRefresh();

    void requestAbort()
    {
        m_abortRequested = true;
    }

    void requestResume()
    {
        m_resumeRequested = true;
    }

private:
    std::auto_ptr<RtsTrayIcon> trayMenu;
    bool m_abortRequested;
    bool m_resumeRequested;
};


class RtsTrayIcon : public wxTaskBarIcon
{
public:
    RtsTrayIcon(WaitCallbackImpl* callback) :
        m_callback(callback)
    {
#ifdef FFS_WIN
        const wxIcon& realtimeIcon = *GlobalResources::getInstance().programIcon;
#elif defined FFS_LINUX
        wxIcon realtimeIcon;
        realtimeIcon.CopyFromBitmap(GlobalResources::getInstance().getImageByName(wxT("RTS_tray_linux.png"))); //use a 22x22 bitmap for perfect fit
#endif
        wxTaskBarIcon::SetIcon(realtimeIcon, wxString(wxT("RealtimeSync")) + wxT(" - ") + _("Monitoring active..."));

        //register double-click
        Connect(wxEVT_TASKBAR_LEFT_DCLICK, wxCommandEventHandler(RtsTrayIcon::resumeToMain), NULL, this);
    }

    void updateSysTray()
    {
        wxTheApp->Yield();
    }

private:
    enum Selection
    {
        CONTEXT_ABORT,
        CONTEXT_RESTORE,
        CONTEXT_ABOUT
    };

    virtual wxMenu* CreatePopupMenu()
    {
        wxMenu* contextMenu = new wxMenu;
        contextMenu->Append(CONTEXT_RESTORE, _("&Restore"));
        contextMenu->Append(CONTEXT_ABOUT, _("&About..."));
        contextMenu->AppendSeparator();
        contextMenu->Append(CONTEXT_ABORT, _("&Exit"));
        //event handling
        contextMenu->Connect(wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(RtsTrayIcon::OnContextMenuSelection), NULL, this);

        return contextMenu; //ownership transferred to library
    }

    void OnContextMenuSelection(wxCommandEvent& event)
    {
        const int eventId = event.GetId();
        switch (static_cast<Selection>(eventId))
        {
        case CONTEXT_ABORT:
            m_callback->requestAbort();
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

            wxMessageDialog* aboutDlg = new wxMessageDialog(NULL, wxString(wxT("RealtimeSync")) + wxT("\n\n") + build, _("About"), wxOK);
            aboutDlg->ShowModal();
            aboutDlg->Destroy();
        }
        break;
        case CONTEXT_RESTORE:
            m_callback->requestResume();
            break;
        }
    }

    void resumeToMain(wxCommandEvent& event)
    {
        m_callback->requestResume();
    }

    WaitCallbackImpl* m_callback;
};


bool updateUiIsAllowed()
{
    static wxLongLong lastExec = 0;
    const wxLongLong  newExec  = wxGetLocalTimeMillis();

    if (newExec - lastExec >= RealtimeSync::UI_UPDATE_INTERVAL)  //perform ui updates not more often than necessary
    {
        lastExec = newExec;
        return true;
    }
    return false;
}


class AbortThisProcess //exception class
{
public:
    AbortThisProcess(bool backToMain) : m_backToMain(backToMain) {}

    bool backToMainMenu() const
    {
        return m_backToMain;
    }

private:
    bool m_backToMain;
};



WaitCallbackImpl::WaitCallbackImpl() :
    m_abortRequested(false),
    m_resumeRequested(false)
{
    trayMenu.reset(new RtsTrayIcon(this));
}


void WaitCallbackImpl::requestUiRefresh()
{
    if (updateUiIsAllowed())
        trayMenu->updateSysTray();

    if (m_abortRequested)
        throw ::AbortThisProcess(false);

    if (m_resumeRequested)
        throw ::AbortThisProcess(true);
}


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
        if (ab.backToMainMenu())
            return RESUME;
        else
            return QUIT;
    }
    catch (const FreeFileSync::FileError& error)
    {
        wxMessageBox(error.show().c_str(), _("Error"), wxOK | wxICON_ERROR);
        return RESUME;
    }

    return RESUME;
}

