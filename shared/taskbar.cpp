// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) 2008-2011 ZenJu (zhnmju123 AT gmx.de)                    *
// **************************************************************************
//
#include "taskbar.h"

#ifdef FFS_WIN
#include "Taskbar_Seven/taskbar.h"
#include "dll_loader.h"
#include "build_info.h"
#include "assert_static.h"
#include <wx/msw/wrapwin.h> //includes "windows.h"

#elif defined HAVE_UBUNTU_UNITY
#include <unity/unity/unity.h>
#endif

using namespace util;


#ifdef FFS_WIN
using namespace tbseven;

namespace
{
bool windows7TaskbarAvailable()
{
    OSVERSIONINFO osvi = {};
    osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);

    if (::GetVersionEx(&osvi))
        return osvi.dwMajorVersion > 6 ||
               (osvi.dwMajorVersion == 6 && osvi.dwMinorVersion >= 1); //task bar progress available with Windows 7
    //XP          has majorVersion == 5, minorVersion == 1
    //Server 2003 has majorVersion == 5, minorVersion == 2
    //Seven       has majorVersion == 6, minorVersion == 1
    //version overview: http://msdn.microsoft.com/en-us/library/ms724834(VS.85).aspx
    return false;
}
}
//########################################################################################################


class Taskbar::Pimpl //throw (TaskbarNotAvailable)
{
public:
    Pimpl(const wxTopLevelWindow& window) :
        assocWindow(window.GetHWND()),
        setStatus_  (getDllName(), setStatusFctName),
        setProgress_(getDllName(), setProgressFctName)
    {
        if (!assocWindow || !setProgress_ || !setStatus_)
            throw TaskbarNotAvailable();

        if (!windows7TaskbarAvailable())
            throw TaskbarNotAvailable();
    }

    ~Pimpl() { setStatus(STATUS_NOPROGRESS); }

    void setStatus(Status status)
    {
        TaskBarStatus tbSevenStatus = tbseven::STATUS_NORMAL;
        switch (status)
        {
            case Taskbar::STATUS_NOPROGRESS:
                tbSevenStatus = tbseven::STATUS_NOPROGRESS;
                break;
            case Taskbar::STATUS_INDETERMINATE:
                tbSevenStatus = tbseven::STATUS_INDETERMINATE;
                break;
            case Taskbar::STATUS_NORMAL:
                tbSevenStatus = tbseven::STATUS_NORMAL;
                break;
            case Taskbar::STATUS_ERROR:
                tbSevenStatus = tbseven::STATUS_ERROR;
                break;
            case Taskbar::STATUS_PAUSED:
                tbSevenStatus = tbseven::STATUS_PAUSED;
                break;
        }

        setStatus_(assocWindow, tbSevenStatus);
    }

    void setProgress(double fraction)
    {
        setProgress_(assocWindow, fraction * 100000, 100000);
    }

private:
    void*  assocWindow; //HWND
    const util::DllFun<SetStatusFct>   setStatus_;
    const util::DllFun<SetProgressFct> setProgress_;
};

#elif defined HAVE_UBUNTU_UNITY //Ubuntu unity
namespace
{
const char FFS_DESKTOP_FILE[] = "freefilesync.desktop";
}

class Taskbar::Pimpl //throw (TaskbarNotAvailable)
{
public:
    Pimpl(const wxTopLevelWindow& window) :
        tbEntry(unity_launcher_entry_get_for_desktop_id(FFS_DESKTOP_FILE))
        //tbEntry(unity_launcher_entry_get_for_app_uri("application://freefilesync.desktop"))
    {
        if (!tbEntry)
            throw TaskbarNotAvailable();
    }

    ~Pimpl() { setStatus(STATUS_NOPROGRESS); } //it seems UnityLauncherEntry* does not need destruction

    void setStatus(Status status)
    {
        switch (status)
        {
            case Taskbar::STATUS_ERROR:
                unity_launcher_entry_set_urgent(tbEntry, true);
                break;

            case Taskbar::STATUS_NOPROGRESS:
            case Taskbar::STATUS_INDETERMINATE:
                unity_launcher_entry_set_urgent(tbEntry, false);
                unity_launcher_entry_set_progress_visible(tbEntry, false);
                break;

            case Taskbar::STATUS_NORMAL:
                unity_launcher_entry_set_urgent(tbEntry, false);
                unity_launcher_entry_set_progress_visible(tbEntry, true);
                break;

            case Taskbar::STATUS_PAUSED:
                unity_launcher_entry_set_urgent (tbEntry, false);
                break;
        }
    }

    void setProgress(double fraction)
    {
        unity_launcher_entry_set_progress(tbEntry, fraction);
    }

private:
    UnityLauncherEntry* tbEntry;
};


#else //no taskbar support yet
class Taskbar::Pimpl
{
public:
    Pimpl(const wxTopLevelWindow& window) { throw TaskbarNotAvailable(); }
    void setStatus(Status status) {}
    void setProgress(size_t current, size_t total) {}

};
#endif


//########################################################################################################
Taskbar::Taskbar(const wxTopLevelWindow& window) : pimpl_(new Pimpl(window)) {} //throw TaskbarNotAvailable
Taskbar::~Taskbar() {} //std::unique_ptr ...

void Taskbar::setStatus(Status status) { pimpl_->setStatus(status); }
void Taskbar::setProgress(double fraction) { pimpl_->setProgress(fraction); }
