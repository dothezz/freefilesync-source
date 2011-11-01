// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) 2008-2011 ZenJu (zhnmju123 AT gmx.de)                    *
// **************************************************************************

#include "tray_menu.h"
#include <algorithm>
#include <iterator>
#include <limits>
#include <set>
#include <wx/msgdlg.h>
#include <wx/taskbar.h>
#include <wx/app.h>
#include <wx/utils.h>
#include <wx/menu.h>
#include "watcher.h"
#include <wx/utils.h>
#include <wx/log.h>
#include <wx/icon.h> //Linux needs this
#include <wx/timer.h>
#include "resources.h"
#include <wx+/string_conv.h>
#include <zen/assert_static.h>
#include <zen/build_info.h>
#include <wx+/shell_execute.h>

using namespace rts;
using namespace zen;

class TrayIconHolder : private wxEvtHandler
{
public:
    TrayIconHolder(const wxString& jobname);
    ~TrayIconHolder();

    void doUiRefreshNow();
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

    const wxString jobName_; //RTS job name, may be empty
};


//RtsTrayIcon shall be a dumb class whose sole purpose is to enable wxWidgets deferred deletion
class TrayIconHolder::RtsTrayIcon : public wxTaskBarIcon
{
public:
    RtsTrayIcon(TrayIconHolder* parent) : parent_(parent) {}

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
        contextMenu->Connect(wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(TrayIconHolder::OnContextMenuSelection), NULL, parent_);

        return contextMenu; //ownership transferred to caller
    }

    TrayIconHolder* parent_;
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


TrayIconHolder::TrayIconHolder(const wxString& jobname) :
    m_abortRequested(false),
    m_resumeRequested(false),
    jobName_(jobname)
{
    trayMenu = new RtsTrayIcon(this); //not in initialization list: give it a valid parent object!

    showIconActive();

    //register double-click
    trayMenu->Connect(wxEVT_TASKBAR_LEFT_DCLICK, wxCommandEventHandler(TrayIconHolder::OnRequestResume), NULL, this);
}


TrayIconHolder::~TrayIconHolder()
{
    trayMenu->Disconnect(wxEVT_TASKBAR_LEFT_DCLICK, wxCommandEventHandler(TrayIconHolder::OnRequestResume), NULL, this);
    trayMenu->RemoveIcon(); //(try to) hide icon until final deletion takes place
    trayMenu->parentHasDied();

    //use wxWidgets delayed destruction: delete during next idle loop iteration (handle late window messages, e.g. when double-clicking)
    if (!wxPendingDelete.Member(trayMenu))
        wxPendingDelete.Append(trayMenu);
}


void TrayIconHolder::showIconActive()
{
    wxIcon realtimeIcon;
#ifdef FFS_WIN
    realtimeIcon.CopyFromBitmap(GlobalResources::getImage(wxT("RTS_tray_win.png"))); //use a 16x16 bitmap
#elif defined FFS_LINUX
    realtimeIcon.CopyFromBitmap(GlobalResources::getImage(wxT("RTS_tray_linux.png"))); //use a 22x22 bitmap for perfect fit
#endif
    const wxString postFix = jobName_.empty() ? wxString() : (wxT("\n\"") + jobName_ + wxT("\""));
    trayMenu->SetIcon(realtimeIcon, _("Monitoring active...") + postFix);
}


void TrayIconHolder::showIconWaiting()
{
    wxIcon realtimeIcon;
#ifdef FFS_WIN
    realtimeIcon.CopyFromBitmap(GlobalResources::getImage(wxT("RTS_tray_waiting_win.png"))); //use a 16x16 bitmap
#elif defined FFS_LINUX
    realtimeIcon.CopyFromBitmap(GlobalResources::getImage(wxT("RTS_tray_waiting_linux.png"))); //use a 22x22 bitmap for perfect fit
#endif
    const wxString postFix = jobName_.empty() ? wxString() : (wxT("\n\"") + jobName_ + wxT("\""));
    trayMenu->SetIcon(realtimeIcon, _("Waiting for missing directories...") + postFix);
}


void TrayIconHolder::OnContextMenuSelection(wxCommandEvent& event)
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
            if (zen::is64BitBuild)
                build += wxT(" x64");
            else
                build += wxT(" x86");
            assert_static(zen::is32BitBuild || zen::is64BitBuild);

            wxString buildFormatted = _("(Build: %x)");
            buildFormatted.Replace(wxT("%x"), build);

            wxMessageDialog aboutDlg(NULL, wxString(wxT("RealtimeSync")) + wxT("\n\n") + buildFormatted, _("About"), wxOK);
            aboutDlg.ShowModal();
        }
        break;
    }
}


void TrayIconHolder::doUiRefreshNow()
{
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
    std::inserter(output, output.end()), [](const wxString & str) { return zen::toZ(str); });
    return std::vector<Zstring>(output.begin(), output.end());
}
}


class StartSyncNowException {};


class WaitCallbackImpl : public rts::WaitCallback
{
public:
    WaitCallbackImpl(const wxString& jobname) :
        trayIcon(jobname),
        nextSyncStart_(std::numeric_limits<long>::max()) {}

    void notifyAllDirectoriesExist()
    {
        trayIcon.showIconActive();
    }

    void notifyDirectoryMissing()
    {
        trayIcon.showIconWaiting();
    }

    virtual void requestUiRefresh() //throw StartSyncNowException()
    {
        if (nextSyncStart_ <= wxGetLocalTime())
            throw StartSyncNowException(); //abort wait and start sync

        if (updateUiIsAllowed())
            trayIcon.doUiRefreshNow();
    }

    void scheduleNextSync(long nextSyncStart)
    {
        nextSyncStart_ = nextSyncStart;
    }

private:
    TrayIconHolder trayIcon;
    long nextSyncStart_;
};

/*
Data Flow:
----------

TrayIconHolder (GUI output)
  /|\
   |
WaitCallbackImpl (higher level "interface")
  /|\
   |
startDirectoryMonitor() (wire dir-changes and execution of commandline)
  /|\
   |
watcher.h (low level wait for directory changes)
*/


rts::MonitorResponse rts::startDirectoryMonitor(const xmlAccess::XmlRealConfig& config, const wxString& jobname)
{
    Zstring lastFileChanged;

    const std::vector<Zstring> dirList = convert(config.directories);
    try
    {
        WaitCallbackImpl callback(jobname);

        if (config.commandline.empty())
        {
            std::wstring errorMsg = _("Invalid command line: %x");
            replace(errorMsg, L"%x", L"\"" + config.commandline + L"\"");
            throw FileError(errorMsg);
        }

        callback.notifyDirectoryMissing();
        waitForMissingDirs(dirList, &callback);
        callback.notifyAllDirectoriesExist();

        while (true)
        {
            ::wxSetEnv(L"changed_file", utf8CvrtTo<wxString>(lastFileChanged)); //some way to output what file changed to the user
            lastFileChanged.clear(); //make sure old name is not shown again after a directory reappears

            //execute command
            zen::shellExecute(config.commandline, zen::EXEC_TYPE_SYNC);

            wxLog::FlushActive(); //show wxWidgets error messages (if any)

            callback.scheduleNextSync(std::numeric_limits<long>::max()); //next sync not scheduled (yet)

            try
            {
                while (true)
                {
                    //wait for changes (and for all directories to become available)
                    WaitResult res = waitForChanges(dirList, &callback);
                    switch (res.type)
                    {
                        case CHANGE_DIR_MISSING: //don't execute the commandline before all directories are available!
                            callback.scheduleNextSync(std::numeric_limits<long>::max()); //next sync not scheduled (yet)
                            callback.notifyDirectoryMissing();
                            waitForMissingDirs(dirList, &callback);
                            callback.notifyAllDirectoriesExist();
                            break;
                        case CHANGE_DETECTED:
                            lastFileChanged = res.filename;
                            break;
                    }

                    callback.scheduleNextSync(wxGetLocalTime() + static_cast<long>(config.delay));
                }
            }
            catch (StartSyncNowException&) {}
        }
    }
    catch (const ::AbortThisProcess& ab)
    {
        return ab.getCommand();
    }
    catch (const zen::FileError& error)
    {
        wxMessageBox(error.toString(), _("Error"), wxOK | wxICON_ERROR);
        return RESUME;
    }

    return RESUME;
}
