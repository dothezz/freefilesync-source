// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) Zenju (zenju AT gmx DOT de) - All Rights Reserved        *
// **************************************************************************

#include "tray_menu.h"
#include <algorithm>
#include <iterator>
#include <limits>
#include <set>
#include <zen/assert_static.h>
#include <zen/build_info.h>
#include <wx+/mouse_move_dlg.h>
#include <wx+/image_tools.h>
#include <wx+/string_conv.h>
#include <wx+/shell_execute.h>
#include <wx/msgdlg.h>
#include <wx/taskbar.h>
#include <wx/app.h>
#include <wx/utils.h>
#include <wx/menu.h>
#include <wx/utils.h>
#include <wx/icon.h> //Linux needs this
#include <wx/timer.h>
#include "resources.h"
#include "gui_generated.h"
#include "watcher.h"
#include "../lib/resolve_path.h"

using namespace rts;
using namespace zen;


namespace
{
struct AbortCallback  //never throw exceptions through a C-Layer (GUI)!
{
    virtual ~AbortCallback() {}
    virtual void requestResume() = 0;
    virtual void requestAbort() = 0;
};


//RtsTrayIcon is a dumb class whose sole purpose is to enable wxWidgets deferred deletion
class RtsTrayIconRaw : public wxTaskBarIcon
{
public:
    RtsTrayIconRaw(AbortCallback& abortCb) : abortCb_(&abortCb)
    {
        Connect(wxEVT_TASKBAR_LEFT_DCLICK, wxCommandEventHandler(RtsTrayIconRaw::OnDoubleClick), nullptr, this);
    }

    void dontCallBackAnymore() { abortCb_ = nullptr; } //call before tray icon is marked for deferred deletion

private:
    enum Selection
    {
        CONTEXT_RESTORE = 1, //wxWidgets: "A MenuItem ID of Zero does not work under Mac"
        CONTEXT_ABORT = wxID_EXIT,
        CONTEXT_ABOUT = wxID_ABOUT
    };

    virtual wxMenu* CreatePopupMenu()
    {
        if (!abortCb_)
            return nullptr;

        wxMenu* contextMenu = new wxMenu;
        contextMenu->Append(CONTEXT_RESTORE, _("&Restore"));
        contextMenu->Append(CONTEXT_ABOUT, _("&About"));
        contextMenu->AppendSeparator();
        contextMenu->Append(CONTEXT_ABORT, _("&Exit"));
        //event handling
        contextMenu->Connect(wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(RtsTrayIconRaw::OnContextMenuSelection), nullptr, this);

        return contextMenu; //ownership transferred to caller
    }

    void OnContextMenuSelection(wxCommandEvent& event)
    {
        if (!abortCb_)
            return;

        switch (static_cast<Selection>(event.GetId()))
        {
            case CONTEXT_ABORT:
                abortCb_->requestAbort();
                break;

            case CONTEXT_RESTORE:
                abortCb_->requestResume();
                break;

            case CONTEXT_ABOUT:
            {
                //build information
                wxString build = __TDATE__;
#if wxUSE_UNICODE
                build += L" - Unicode";
#else
                build += L" - ANSI";
#endif //wxUSE_UNICODE

                //compile time info about 32/64-bit build
                if (zen::is64BitBuild)
                    build += L" x64";
                else
                    build += L" x86";
                assert_static(zen::is32BitBuild || zen::is64BitBuild);

                wxMessageBox(L"RealtimeSync" L"\n\n" + replaceCpy(_("Build: %x"), L"%x", build), _("About"), wxOK);
            }
            break;
        }
    }

    void OnDoubleClick(wxCommandEvent& event)
    {
        if (abortCb_)
            abortCb_->requestResume();
    }

    AbortCallback* abortCb_;
};


class TrayIconHolder
{
public:
    TrayIconHolder(const wxString& jobname, AbortCallback& abortCb) :
        jobName_(jobname),
        trayMenu(new RtsTrayIconRaw(abortCb))
    {
        showIconActive();
    }

    ~TrayIconHolder()
    {
        trayMenu->RemoveIcon(); //(try to) hide icon until final deletion takes place
        trayMenu->dontCallBackAnymore();

        //use wxWidgets delayed destruction: delete during next idle loop iteration (handle late window messages, e.g. when double-clicking)
        if (!wxPendingDelete.Member(trayMenu))
            wxPendingDelete.Append(trayMenu);
    }

    void doUiRefreshNow()
    {
        wxTheApp->Yield();
    } //yield is UI-layer which is represented by this tray icon

    void showIconActive()
    {
        wxIcon realtimeIcon;
#if defined FFS_WIN || defined FFS_MAC //16x16 seems to be the only size that is shown correctly on OS X
        realtimeIcon.CopyFromBitmap(getResourceImage(L"RTS_tray_16x16")); //use a 16x16 bitmap
#elif defined FFS_LINUX
        realtimeIcon.CopyFromBitmap(getResourceImage(L"RTS_tray_24x24")); //use a 24x24 bitmap for perfect fit
#endif
        const wxString postFix = jobName_.empty() ? wxString() : (L"\n\"" + jobName_ + L"\"");
        trayMenu->SetIcon(realtimeIcon, _("Monitoring active...") + postFix);
    }

    void showIconWaiting()
    {
        wxIcon realtimeIcon;
#if defined FFS_WIN || defined FFS_MAC
        realtimeIcon.CopyFromBitmap(greyScale(getResourceImage(L"RTS_tray_16x16")));
#elif defined FFS_LINUX
        realtimeIcon.CopyFromBitmap(greyScale(getResourceImage(L"RTS_tray_24x24")));
#endif
        const wxString postFix = jobName_.empty() ? wxString() : (L"\n\"" + jobName_ + L"\"");
        trayMenu->SetIcon(realtimeIcon, _("Waiting for missing directories...") + postFix);
    }

private:
    const wxString jobName_; //RTS job name, may be empty
    RtsTrayIconRaw* trayMenu;
};


//##############################################################################################################

struct AbortMonitoring//exception class
{
    AbortMonitoring(AbortReason reasonCode) : reasonCode_(reasonCode) {}
    AbortReason reasonCode_;
};

class StartSyncNowException {};

//##############################################################################################################

class WaitCallbackImpl : public rts::WaitCallback, private AbortCallback
{
public:
    WaitCallbackImpl(const wxString& jobname) :
        trayIcon(jobname, *this),
        nextSyncStart_(std::numeric_limits<long>::max()),
        resumeRequested(false),
        abortRequested(false) {}

    void notifyAllDirectoriesExist() { trayIcon.showIconActive(); }
    void notifyDirectoryMissing   () { trayIcon.showIconWaiting(); }

    void scheduleNextSync(long nextSyncStart) { nextSyncStart_ = nextSyncStart; }
    void clearSchedule() { nextSyncStart_ = std::numeric_limits<long>::max(); }

    //implement WaitCallback
    virtual void requestUiRefresh(bool readyForSync) //throw StartSyncNowException, AbortMonitoring
    {
        if (resumeRequested)
            throw AbortMonitoring(SHOW_GUI);

        if (abortRequested)
            throw AbortMonitoring(EXIT_APP);

        if (readyForSync)
            if (nextSyncStart_ <= wxGetLocalTime())
                throw StartSyncNowException(); //abort wait and start sync

        if (updateUiIsAllowed())
            trayIcon.doUiRefreshNow();
    }

private:
    //implement AbortCallback: used from C-GUI call stack
    virtual void requestResume() { resumeRequested = true; }
    virtual void requestAbort () { abortRequested  = true; }

    TrayIconHolder trayIcon;
    long nextSyncStart_;
    bool resumeRequested;
    bool abortRequested;
};



class ErrorDlgWithTimeout : public ErrorDlgGenerated
{
public:
    ErrorDlgWithTimeout(wxWindow* parent, const wxString& messageText) :
        ErrorDlgGenerated(parent),
        secondsLeft(15) //give user some time to read msg!?
    {
#ifdef FFS_WIN
        new zen::MouseMoveWindow(*this); //allow moving main dialog by clicking (nearly) anywhere...; ownership passed to "this"
#endif
        m_bitmap10->SetBitmap(getResourceImage(L"msg_error"));
        m_textCtrl8->SetValue(messageText);

        //count down X seconds then automatically press "retry"
        timer.Connect(wxEVT_TIMER, wxEventHandler(ErrorDlgWithTimeout::OnTimerEvent), nullptr, this);
        timer.Start(1000); //timer interval in ms
        updateButtonLabel();

        Fit(); //child-element widths have changed: image was set
        m_buttonRetry->SetFocus();
    }

    enum ButtonPressed
    {
        BUTTON_RETRY,
        BUTTON_ABORT
    };

private:
    void OnTimerEvent(wxEvent& event)
    {
        if (secondsLeft <= 0)
        {
            EndModal(BUTTON_RETRY);
            return;
        }
        --secondsLeft;
        updateButtonLabel();
    }

    void updateButtonLabel()
    {
        m_buttonRetry->SetLabel(_("&Retry") + L" (" + replaceCpy(_P("1 sec", "%x sec", secondsLeft), L"%x", numberTo<std::wstring>(secondsLeft)) + L")");
        Layout();
    }

    void OnClose(wxCloseEvent&   event) { EndModal(BUTTON_ABORT); }
    void OnRetry(wxCommandEvent& event) { EndModal(BUTTON_RETRY); }
    void OnAbort(wxCommandEvent& event) { EndModal(BUTTON_ABORT); }

    int secondsLeft;
    wxTimer timer;
};


bool reportErrorTimeout(const std::wstring& msg) //return true if timeout or user selected "retry", else abort
{
    ErrorDlgWithTimeout errorDlg(nullptr, msg);
    //errorDlg.Raise(); -> don't steal focus every X seconds
    switch (static_cast<ErrorDlgWithTimeout::ButtonPressed>(errorDlg.ShowModal()))
    {
        case ErrorDlgWithTimeout::BUTTON_RETRY:
            return true;
        case ErrorDlgWithTimeout::BUTTON_ABORT:
            return false;
    }
    return false;
}


inline
wxString toString(DirWatcher::ActionType type)
{
    switch (type)
    {
        case DirWatcher::ACTION_CREATE:
            return L"CREATE";
        case DirWatcher::ACTION_UPDATE:
            return L"UPDATE";
        case DirWatcher::ACTION_DELETE:
            return L"DELETE";
    }
    return L"ERROR";
}
}

/*
Data Flow:
----------

TrayIconHolder (GUI output)
  /|\
   |
WaitCallbackImpl
  /|\
   |
startDirectoryMonitor() (wire dir-changes and execution of commandline)
*/


rts::AbortReason rts::startDirectoryMonitor(const xmlAccess::XmlRealConfig& config, const wxString& jobname)
{
    std::vector<Zstring> dirList = toZ(config.directories);
    vector_remove_if(dirList, [](Zstring str) -> bool { trim(str); return str.empty(); }); //remove empty entries WITHOUT formatting dirList yet!

    if (dirList.empty())
    {
        wxMessageBox(_("A folder input field is empty."), _("Error"), wxOK | wxICON_ERROR);
        return SHOW_GUI;
    }

    wxString cmdLine = config.commandline;
    trim(cmdLine);

    if (cmdLine.empty())
    {
        wxMessageBox(_("Invalid command line:") + L" \"\"", _("Error"), wxOK | wxICON_ERROR);
        return SHOW_GUI;
    }

    try
    {
        DirWatcher::Entry lastChangeDetected;
        WaitCallbackImpl callback(jobname);

        auto execMonitoring = [&] //throw FileError, AbortMonitoring
        {
            callback.notifyDirectoryMissing();
            callback.clearSchedule();
            waitForMissingDirs(dirList, callback); //throw FileError, StartSyncNowException(not scheduled yet), AbortMonitoring
            callback.notifyAllDirectoriesExist();

            //schedule initial execution (*after* all directories have arrived, which could take some time which we don't want to include)
            callback.scheduleNextSync(wxGetLocalTime() + static_cast<long>(config.delay));

            while (true)
            {
                try
                {
                    while (true)
                    {
                        //wait for changes (and for all directories to become available)
                        WaitResult res = waitForChanges(dirList, callback); //throw FileError, StartSyncNowException, AbortMonitoring
                        switch (res.type)
                        {
                            case CHANGE_DIR_MISSING: //don't execute the commandline before all directories are available!
                                callback.notifyDirectoryMissing();
                                callback.clearSchedule();
                                waitForMissingDirs(dirList, callback); //throw FileError, StartSyncNowException(not scheduled yet), AbortMonitoring
                                callback.notifyAllDirectoriesExist();
                                break;

                            case CHANGE_DETECTED:
                                lastChangeDetected = res.changedItem_;
                                break;
                        }
                        callback.scheduleNextSync(wxGetLocalTime() + static_cast<long>(config.delay));
                    }
                }
                catch (StartSyncNowException&) {}

                ::wxSetEnv(L"change_path", utfCvrtTo<wxString>(lastChangeDetected.filename_)); //some way to output what file changed to the user
                ::wxSetEnv(L"change_action", toString(lastChangeDetected.action_)); //
                lastChangeDetected = DirWatcher::Entry(); //make sure old name is not shown again after a directory reappears

                //execute command
                auto cmdLineExp = expandMacros(utfCvrtTo<Zstring>(cmdLine));
                zen::shellExecute(cmdLineExp, zen::EXEC_TYPE_SYNC);
                callback.clearSchedule();
            }
        };

        while (true)
            try
            {
                execMonitoring(); //throw FileError, AbortMonitoring
            }
            catch (const zen::FileError& e)
            {
                if (!reportErrorTimeout(e.toString())) //return true if timeout or user selected "retry", else abort
                    return SHOW_GUI;
            }
    }
    catch (const AbortMonitoring& ab)
    {
        return ab.reasonCode_;
    }
}
