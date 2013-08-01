// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) Zenju (zenju AT gmx DOT de) - All Rights Reserved        *
// **************************************************************************

#include "tray_menu.h"
#include <zen/build_info.h>
#include <zen/tick_count.h>
#include <zen/thread.h>
#include <wx+/mouse_move_dlg.h>
#include <wx+/image_tools.h>
//#include <wx+/string_conv.h>
#include <wx+/shell_execute.h>
#include <wx+/std_button_order.h>
#include <wx/taskbar.h>
#include <wx/icon.h> //Linux needs this
#include <wx/app.h>
#include "resources.h"
#include "gui_generated.h"
#include "monitor.h"
#include "../lib/resolve_path.h"

using namespace rts;
using namespace zen;


namespace
{
const std::int64_t TICKS_UPDATE_INTERVAL = rts::UI_UPDATE_INTERVAL* ticksPerSec() / 1000;
TickVal lastExec = getTicks();

bool updateUiIsAllowed()
{
    const TickVal now = getTicks(); //0 on error
    if (dist(lastExec, now) >= TICKS_UPDATE_INTERVAL)  //perform ui updates not more often than necessary
    {
        lastExec = now;
        return true;
    }
    return false;
}


enum TrayMode
{
    TRAY_MODE_ACTIVE,
    TRAY_MODE_WAITING,
    TRAY_MODE_ERROR,
};


class TrayIconObject : public wxTaskBarIcon
{
public:
    TrayIconObject(const wxString& jobname) :
        resumeRequested(false),
        abortRequested(false),
        showErrorMsgRequested(false),
        mode(TRAY_MODE_ACTIVE),
        iconFlashStatusLast(false),
        jobName_(jobname),
#if defined ZEN_WIN || defined ZEN_MAC //16x16 seems to be the only size that is shown correctly on OS X
        trayBmp(getResourceImage(L"RTS_tray_16x16")) //use a 16x16 bitmap
#elif defined ZEN_LINUX
        trayBmp(getResourceImage(L"RTS_tray_24x24")) //use a 24x24 bitmap for perfect fit
#endif
    {
        Connect(wxEVT_TASKBAR_LEFT_DCLICK, wxCommandEventHandler(TrayIconObject::OnDoubleClick), nullptr, this);
        setMode(mode);
    }

    //require polling:
    bool resumeIsRequested() const { return resumeRequested; }
    bool abortIsRequested () const { return abortRequested;  }

    //during TRAY_MODE_ERROR those two functions are available:
    void clearShowErrorRequested() { assert(mode == TRAY_MODE_ERROR); showErrorMsgRequested = false; }
    bool getShowErrorRequested() const { assert(mode == TRAY_MODE_ERROR); return showErrorMsgRequested; }

    void setMode(TrayMode m)
    {
        mode = m;
        timer.Stop();
        timer.Disconnect(wxEVT_TIMER, wxEventHandler(TrayIconObject::OnErrorFlashIcon), nullptr, this);
        switch (m)
        {
            case TRAY_MODE_ACTIVE:
                setTrayIcon(trayBmp, _("Directory monitoring active"));
                break;

            case TRAY_MODE_WAITING:
                setTrayIcon(greyScale(trayBmp), _("Waiting until all directories are available..."));
                break;

            case TRAY_MODE_ERROR:
                timer.Connect(wxEVT_TIMER, wxEventHandler(TrayIconObject::OnErrorFlashIcon), nullptr, this);
                timer.Start(500); //timer interval in [ms]
                break;
        }
    }

private:
    void OnErrorFlashIcon(wxEvent& event)
    {
        iconFlashStatusLast = !iconFlashStatusLast;
        setTrayIcon(iconFlashStatusLast ? trayBmp : greyScale(trayBmp), _("Error"));
    }

    void setTrayIcon(const wxBitmap& bmp, const wxString& statusTxt)
    {
        wxIcon realtimeIcon;
        realtimeIcon.CopyFromBitmap(bmp);
        wxString tooltip = L"RealtimeSync\n" + statusTxt;
        if (!jobName_.empty())
            tooltip += L"\n\"" + jobName_ + L"\"";
        SetIcon(realtimeIcon, tooltip);
    }

    enum Selection
    {
        CONTEXT_RESTORE = 1, //wxWidgets: "A MenuItem ID of zero does not work under Mac"
        CONTEXT_SHOW_ERROR,
        CONTEXT_ABORT = wxID_EXIT,
        CONTEXT_ABOUT = wxID_ABOUT
    };

    virtual wxMenu* CreatePopupMenu()
    {
        wxMenu* contextMenu = new wxMenu;
        switch (mode)
        {
            case TRAY_MODE_ACTIVE:
            case TRAY_MODE_WAITING:
                contextMenu->Append(CONTEXT_RESTORE, _("&Restore"));
                break;
            case TRAY_MODE_ERROR:
                contextMenu->Append(CONTEXT_SHOW_ERROR, _("&Show error"));
                break;
        }
        contextMenu->Append(CONTEXT_ABOUT, _("&About"));
        contextMenu->AppendSeparator();
        contextMenu->Append(CONTEXT_ABORT, _("&Exit"));
        //event handling
        contextMenu->Connect(wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(TrayIconObject::OnContextMenuSelection), nullptr, this);

        return contextMenu; //ownership transferred to caller
    }

    void OnContextMenuSelection(wxCommandEvent& event)
    {
        switch (static_cast<Selection>(event.GetId()))
        {
            case CONTEXT_ABORT:
                abortRequested  = true;
                break;

            case CONTEXT_RESTORE:
                resumeRequested = true;
                break;

            case CONTEXT_SHOW_ERROR:
                showErrorMsgRequested = true;
                break;

            case CONTEXT_ABOUT:
            {
                //ATTENTION: the modal dialog below does NOT disable all GUI input, e.g. user may still double-click on tray icon
                //no crash in this context, but the double-click is remembered and executed after the modal dialog quits
                SetEvtHandlerEnabled(false);
                ZEN_ON_SCOPE_EXIT(SetEvtHandlerEnabled(true));

                wxString build = __TDATE__;
#if wxUSE_UNICODE
                build += L" - Unicode";
#else
                build += L" - ANSI";
#endif //wxUSE_UNICODE

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
        switch (mode)
        {
            case TRAY_MODE_ACTIVE:
            case TRAY_MODE_WAITING:
                resumeRequested = true; //never throw exceptions through a C-Layer call stack (GUI)!
                break;
            case TRAY_MODE_ERROR:
                showErrorMsgRequested = true;
                break;
        }
    }

    bool resumeRequested;
    bool abortRequested;
    bool showErrorMsgRequested;

    TrayMode mode;

    bool iconFlashStatusLast; //flash try icon for TRAY_MODE_ERROR
    wxTimer timer;            //

    const wxString jobName_; //RTS job name, may be empty
    const wxBitmap trayBmp;
};


struct AbortMonitoring //exception class
{
    AbortMonitoring(AbortReason reasonCode) : reasonCode_(reasonCode) {}
    AbortReason reasonCode_;
};


//=> don't derive from wxEvtHandler or any other wxWidgets object unless instance is safely deleted (deferred) during idle event!!tray_icon.h
class TrayIconHolder
{
public:
    TrayIconHolder(const wxString& jobname) :
        trayObj(new TrayIconObject(jobname)) {}

    ~TrayIconHolder()
    {
        //harmonize with tray_icon.cpp!!!
        trayObj->RemoveIcon();
        //use wxWidgets delayed destruction: delete during next idle loop iteration (handle late window messages, e.g. when double-clicking)
        wxPendingDelete.Append(trayObj);
    }

    void doUiRefreshNow() //throw AbortMonitoring
    {
        wxTheApp->Yield(); //yield is UI-layer which is represented by this tray icon

        //advantage of polling vs callbacks: we can throw exceptions!
        if (trayObj->resumeIsRequested())
            throw AbortMonitoring(SHOW_GUI);

        if (trayObj->abortIsRequested())
            throw AbortMonitoring(EXIT_APP);
    }

    void setMode(TrayMode m) { trayObj->setMode(m); }

    bool getShowErrorRequested() const { return trayObj->getShowErrorRequested(); }
    void clearShowErrorRequested() { trayObj->clearShowErrorRequested(); }

private:
    TrayIconObject* trayObj;
};

//##############################################################################################################

//#define ERROR_DLG_ENABLE_TIMEOUT
class ErrorDlgWithTimeout : public ErrorDlgGenerated
{
public:
    ErrorDlgWithTimeout(wxWindow* parent, const wxString& messageText) :
        ErrorDlgGenerated(parent)
#ifdef ERROR_DLG_ENABLE_TIMEOUT
        , secondsLeft(15) //give user some time to read msg!?
#endif
    {
#ifdef ZEN_WIN
        new zen::MouseMoveWindow(*this); //allow moving main dialog by clicking (nearly) anywhere...; ownership passed to "this"
#endif
        setStandardButtonOrder(*bSizerStdButtons, StdButtons().setAffirmative(m_buttonRetry).setCancel(m_buttonCancel));

        m_bitmap10->SetBitmap(getResourceImage(L"msg_error"));
        m_textCtrl8->SetValue(messageText);

#ifdef ERROR_DLG_ENABLE_TIMEOUT
        //count down X seconds then automatically press "retry"
        timer.Connect(wxEVT_TIMER, wxEventHandler(ErrorDlgWithTimeout::OnTimerEvent), nullptr, this);
        timer.Start(1000); //timer interval in ms
        updateButtonLabel();
#endif
        Fit(); //child-element widths have changed: image was set
        m_buttonRetry->SetFocus();
    }

    enum ButtonPressed
    {
        BUTTON_RETRY,
        BUTTON_ABORT
    };

private:
#ifdef ERROR_DLG_ENABLE_TIMEOUT
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
#endif

    void OnClose(wxCloseEvent&   event) { EndModal(BUTTON_ABORT); }
    void OnRetry(wxCommandEvent& event) { EndModal(BUTTON_RETRY); }
    void OnAbort(wxCommandEvent& event) { EndModal(BUTTON_ABORT); }

#ifdef ERROR_DLG_ENABLE_TIMEOUT
    int secondsLeft;
    wxTimer timer;
#endif
};


bool reportErrorTimeout(const std::wstring& msg) //return true: "retry"; false: "abort"
{
    ErrorDlgWithTimeout errorDlg(nullptr, msg);
    errorDlg.Raise();
    switch (static_cast<ErrorDlgWithTimeout::ButtonPressed>(errorDlg.ShowModal()))
    {
        case ErrorDlgWithTimeout::BUTTON_RETRY:
            return true;
        case ErrorDlgWithTimeout::BUTTON_ABORT:
            return false;
    }
    return false;
}
}


rts::AbortReason rts::startDirectoryMonitor(const xmlAccess::XmlRealConfig& config, const wxString& jobname)
{
    std::vector<Zstring> dirNamesNonFmt = config.directories;
    vector_remove_if(dirNamesNonFmt, [](Zstring str) -> bool { trim(str); return str.empty(); }); //remove empty entries WITHOUT formatting paths yet!

    if (dirNamesNonFmt.empty())
    {
        wxMessageBox(_("A folder input field is empty."), L"RealtimeSync" + _("Error"), wxOK | wxICON_ERROR);
        return SHOW_GUI;
    }

    Zstring cmdLine = config.commandline;
    trim(cmdLine);

    if (cmdLine.empty())
    {
        wxMessageBox(_("Invalid command line:") + L" \"\"", L"RealtimeSync" + _("Error"), wxOK | wxICON_ERROR);
        return SHOW_GUI;
    }

    struct MonitorCallbackImpl : public MonitorCallback
    {
        MonitorCallbackImpl(const wxString& jobname,
                            const Zstring& cmdLine) : trayIcon(jobname), cmdLine_(cmdLine) {}

        virtual void setPhase(WatchPhase mode)
        {
            switch (mode)
            {
                case MONITOR_PHASE_ACTIVE:
                    trayIcon.setMode(TRAY_MODE_ACTIVE);
                    break;
                case MONITOR_PHASE_WAITING:
                    trayIcon.setMode(TRAY_MODE_WAITING);
                    break;
            }
        }

        virtual void executeExternalCommand()
        {
            auto cmdLineExp = expandMacros(cmdLine_);
            zen::shellExecute(cmdLineExp, zen::EXEC_TYPE_SYNC);
        }

        virtual void requestUiRefresh()
        {
            if (updateUiIsAllowed())
                trayIcon.doUiRefreshNow(); //throw AbortMonitoring
        }

        virtual void reportError(const std::wstring& msg)
        {
            trayIcon.setMode(TRAY_MODE_ERROR);
            trayIcon.clearShowErrorRequested();

            //wait for some time, then return to retry
            assert_static(15 * 1000 % UI_UPDATE_INTERVAL == 0);
            for (int i = 0; i < 15 * 1000 / UI_UPDATE_INTERVAL; ++i)
            {
                trayIcon.doUiRefreshNow(); //throw AbortMonitoring

                if (trayIcon.getShowErrorRequested())
                {
                    if (reportErrorTimeout(msg)) //return true: "retry"; false: "abort"
                        return;
                    else
                        throw AbortMonitoring(SHOW_GUI);
                }
                boost::this_thread::sleep(boost::posix_time::milliseconds(UI_UPDATE_INTERVAL));
            }
        }

        TrayIconHolder trayIcon;
        const Zstring cmdLine_;
    } cb(jobname, cmdLine);

    try
    {
        monitorDirectories(dirNamesNonFmt, config.delay, cb); //cb: throw AbortMonitoring
        assert(false);
        return SHOW_GUI;
    }
    catch (const AbortMonitoring& ab)
    {
        return ab.reasonCode_;
    }
}
