// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) 2008-2010 ZenJu (zhnmju123 AT gmx.de)                    *
// **************************************************************************
//
#include "tray_menu.h"
#include <wx/msgdlg.h>
#include <wx/taskbar.h>
#include <wx/app.h>
#include "resources.h"
#include <algorithm>
#include <iterator>
#include "../shared/string_conv.h"
#include <wx/utils.h>
#include <wx/menu.h>
#include "watcher.h"
#include <wx/utils.h>
#include <wx/log.h>
#include "../shared/assert_static.h"
#include "../shared/build_info.h"
#include <wx/icon.h> //Linux needs this
#include <wx/timer.h>

using namespace rts;


class WaitCallbackImpl : private wxEvtHandler, public rts::WaitCallback //keep this order: else VC++ generates wrong code
{
public:
    WaitCallbackImpl();
    ~WaitCallbackImpl();

    virtual void requestUiRefresh();

    void showIconActive();
    void showIconWaiting();

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

    showIconActive();

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


void WaitCallbackImpl::showIconActive()
{
    wxIcon realtimeIcon;
#ifdef FFS_WIN
    realtimeIcon.CopyFromBitmap(GlobalResources::getInstance().getImageByName(wxT("RTS_tray_win.png"))); //use a 16x16 bitmap
#elif defined FFS_LINUX
    realtimeIcon.CopyFromBitmap(GlobalResources::getInstance().getImageByName(wxT("RTS_tray_linux.png"))); //use a 22x22 bitmap for perfect fit
#endif
    trayMenu->SetIcon(realtimeIcon, wxString(wxT("RealtimeSync")) + wxT(" - ") + _("Monitoring active..."));
}


void WaitCallbackImpl::showIconWaiting()
{
    wxIcon realtimeIcon;
#ifdef FFS_WIN
    realtimeIcon.CopyFromBitmap(GlobalResources::getInstance().getImageByName(wxT("RTS_tray_waiting_win.png"))); //use a 16x16 bitmap
#elif defined FFS_LINUX
    realtimeIcon.CopyFromBitmap(GlobalResources::getInstance().getImageByName(wxT("RTS_tray_waiting_linux.png"))); //use a 22x22 bitmap for perfect fit
#endif
    trayMenu->SetIcon(realtimeIcon, wxString(wxT("RealtimeSync")) + wxT(" - ") + _("Waiting for all directories to become available..."));
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
        wxString build = __TDATE__;
#if wxUSE_UNICODE
        build += wxT(" - Unicode");
#else
        build += wxT(" - ANSI");
#endif //wxUSE_UNICODE

        //compile time info about 32/64-bit build
        if (util::is64BitBuild)
            build += wxT(" x64");
        else
            build += wxT(" x86");
        assert_static(util::is32BitBuild || util::is64BitBuild);

        wxString buildFormatted = _("(Build: %x)");
        buildFormatted.Replace(wxT("%x"), build);

        wxMessageDialog aboutDlg(NULL, wxString(wxT("RealtimeSync")) + wxT("\n\n") + buildFormatted, _("About"), wxOK);
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

namespace
{
std::vector<Zstring> convert(const std::vector<wxString>& dirList)
{
    std::set<Zstring, LessFilename> output;
    std::transform(dirList.begin(), dirList.end(),
                   std::inserter(output, output.end()), static_cast<Zstring (*)(const wxString&)>(ffs3::wxToZ));
    return std::vector<Zstring>(output.begin(), output.end());
}
}


rts::MonitorResponse rts::startDirectoryMonitor(const xmlAccess::XmlRealConfig& config)
{
    const std::vector<Zstring> dirList = convert(config.directories);

    try
    {
        WaitCallbackImpl callback;

        if (config.commandline.empty())
            throw ffs3::FileError(_("Command line is empty!"));

        while (true)
        {
            //execute commandline
            callback.showIconWaiting();
            waitForMissingDirs(dirList, &callback);
            callback.showIconActive();

            wxExecute(config.commandline, wxEXEC_SYNC); //execute command
            wxLog::FlushActive(); //show wxWidgets error messages (if any)

            //wait for changes (and for all directories to become available)
            switch (waitForChanges(dirList, &callback))
            {
            case CHANGE_DIR_MISSING: //don't execute the commandline before all directories are available!
                callback.showIconWaiting();
                waitForMissingDirs(dirList, &callback);
                callback.showIconActive();
                break;
            case CHANGE_DETECTED:
                break;
            }

            //some delay
            const long nextExec = wxGetLocalTime() + static_cast<long>(config.delay);
            while (wxGetLocalTime() < nextExec)
            {
                callback.requestUiRefresh();
                wxMilliSleep(rts::UI_UPDATE_INTERVAL);
            }
        }
    }
    catch (const ::AbortThisProcess& ab)
    {
        return ab.getCommand();
    }
    catch (const ffs3::FileError& error)
    {
        wxMessageBox(error.msg(), _("Error"), wxOK | wxICON_ERROR);
        return RESUME;
    }

    return RESUME;
}
