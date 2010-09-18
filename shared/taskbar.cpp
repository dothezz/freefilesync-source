// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) 2008-2010 ZenJu (zhnmju123 AT gmx.de)                    *
// **************************************************************************
//
#include "taskbar.h"
#include "Taskbar_Seven/taskbar.h"
#include "dll_loader.h"
#include "build_info.h"
#include "assert_static.h"
#include <wx/msw/wrapwin.h> //includes "windows.h"

using namespace util;
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


const std::wstring& getTaskBarDllName()
{
    assert_static(util::is32BitBuild || util::is64BitBuild);

    static const std::wstring filename(
        util::is64BitBuild ?
        L"Taskbar7_x64.dll" :
        L"Taskbar7_Win32.dll");

    return filename;
}
}
//########################################################################################################


class TaskbarProgress::Pimpl //throw (TaskbarNotAvailable)
{
public:
    Pimpl(const wxTopLevelWindow& window) :
        assocWindow(window.GetHWND()),
        setStatus_(util::getDllFun<SetStatusFct>(  getTaskBarDllName(), setStatusFctName)),
        setProgress_(util::getDllFun<SetProgressFct>(getTaskBarDllName(), setProgressFctName))
    {
        if (!assocWindow || !setProgress_ || !setStatus_)
            throw TaskbarNotAvailable();

        if (!windows7TaskbarAvailable())
            throw TaskbarNotAvailable();
    }

    ~Pimpl()
    {
        setStatus(STATUS_NOPROGRESS);
    }

    void setStatus(Status status)
    {
        TaskBarStatus tbSevenStatus = tbseven::STATUS_NORMAL;
        switch (status)
        {
        case TaskbarProgress::STATUS_NOPROGRESS:
            tbSevenStatus = tbseven::STATUS_NOPROGRESS;
            break;
        case TaskbarProgress::STATUS_INDETERMINATE:
            tbSevenStatus = tbseven::STATUS_INDETERMINATE;
            break;
        case TaskbarProgress::STATUS_NORMAL:
            tbSevenStatus = tbseven::STATUS_NORMAL;
            break;
        case TaskbarProgress::STATUS_ERROR:
            tbSevenStatus = tbseven::STATUS_ERROR;
            break;
        case TaskbarProgress::STATUS_PAUSED:
            tbSevenStatus = tbseven::STATUS_PAUSED;
            break;
        }

        setStatus_(assocWindow, tbSevenStatus);
    }

    void setProgress(size_t current, size_t total)
    {
        setProgress_(assocWindow, current, total);
    }

private:
    void* assocWindow;
    const SetStatusFct setStatus_;
    const SetProgressFct setProgress_;
};
//########################################################################################################


TaskbarProgress::TaskbarProgress(const wxTopLevelWindow& window) : pimpl_(new Pimpl(window)) {}

TaskbarProgress::~TaskbarProgress() {} //std::auto_ptr ...

void TaskbarProgress::setStatus(Status status)
{
    pimpl_->setStatus(status);
}

void TaskbarProgress::setProgress(size_t current, size_t total)
{
    pimpl_->setProgress(current, total);
}
