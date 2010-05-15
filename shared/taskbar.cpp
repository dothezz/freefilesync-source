// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) 2008-2010 ZenJu (zhnmju123 AT gmx.de)                    *
// **************************************************************************
//
#include "taskbar.h"
#include "Taskbar_Seven/taskbar.h"
#include "dllLoader.h"
#include "buildInfo.h"
#include "staticAssert.h"
#include <wx/msw/wrapwin.h> //includes "windows.h"

using namespace Utility;


namespace
{
bool windows7TaskbarAvailable()
{
    OSVERSIONINFO osvi;
    ZeroMemory(&osvi, sizeof(OSVERSIONINFO));
    osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);

    if (GetVersionEx(&osvi))
        return osvi.dwMajorVersion > 6 ||
               (osvi.dwMajorVersion == 6 && osvi.dwMinorVersion >= 1); //task bar progress available with Windows 7
    //XP          has majorVersion == 5, minorVersion == 1
    //Server 2003 has majorVersion == 5, minorVersion == 2
    //Seven       has majorVersion == 6, minorVersion == 1
    //version overview: http://msdn.microsoft.com/en-us/library/ms724834(VS.85).aspx
    return false;
}


const wxString& getTaskBarDllName()
{
    static const wxString filename(
        Utility::is64BitBuild ?
        wxT("Taskbar7_x64.dll") :
        wxT("Taskbar7_Win32.dll"));

    assert_static(Utility::is32BitBuild || Utility::is64BitBuild);

    return filename;
}
}


struct TaskbarProgress::Pimpl
{
    Pimpl() : tbHandle(0),
        assocWindow(NULL),
        init_(NULL),
        release_(NULL),
        setStatus_(NULL),
        setProgress_(NULL) {}

    TaskbarSeven::TBHandle tbHandle;
    void* assocWindow;

    TaskbarSeven::initFct init_;
    TaskbarSeven::releaseFct release_;
    TaskbarSeven::setStatusFct setStatus_;
    TaskbarSeven::setProgressFct setProgress_;
};


TaskbarProgress::TaskbarProgress(const wxTopLevelWindow& window) : pimpl_(new Pimpl)
{
    if (!windows7TaskbarAvailable())
        throw TaskbarNotAvailable();

    pimpl_->init_         = Utility::loadDllFunction<TaskbarSeven::initFct>(       getTaskBarDllName().c_str(), TaskbarSeven::initFctName);
    pimpl_->release_      = Utility::loadDllFunction<TaskbarSeven::releaseFct>(    getTaskBarDllName().c_str(), TaskbarSeven::releaseFctName);
    pimpl_->setProgress_  = Utility::loadDllFunction<TaskbarSeven::setProgressFct>(getTaskBarDllName().c_str(), TaskbarSeven::setProgressFctName);
    pimpl_->setStatus_    = Utility::loadDllFunction<TaskbarSeven::setStatusFct>(  getTaskBarDllName().c_str(), TaskbarSeven::setStatusFctName);

    if (    !pimpl_->init_        ||
            !pimpl_->release_     ||
            !pimpl_->setProgress_ ||
            !pimpl_->setStatus_)
        throw TaskbarNotAvailable();

    pimpl_->tbHandle = pimpl_->init_();
    if (pimpl_->tbHandle == 0)
        throw TaskbarNotAvailable();

    pimpl_->assocWindow = window.GetHWND();
}


TaskbarProgress::~TaskbarProgress()
{
    setStatus(STATUS_NOPROGRESS);

    pimpl_->release_(pimpl_->tbHandle);
}


void TaskbarProgress::setStatus(Status status)
{
    TaskbarSeven::TaskBarStatus tbSevenStatus = TaskbarSeven::STATUS_NORMAL;
    switch (status)
    {
    case TaskbarProgress::STATUS_NOPROGRESS:
        tbSevenStatus = TaskbarSeven::STATUS_NOPROGRESS;
        break;
    case TaskbarProgress::STATUS_INDETERMINATE:
        tbSevenStatus = TaskbarSeven::STATUS_INDETERMINATE;
        break;
    case TaskbarProgress::STATUS_NORMAL:
        tbSevenStatus = TaskbarSeven::STATUS_NORMAL;
        break;
    case TaskbarProgress::STATUS_ERROR:
        tbSevenStatus = TaskbarSeven::STATUS_ERROR;
        break;
    case TaskbarProgress::STATUS_PAUSED:
        tbSevenStatus = TaskbarSeven::STATUS_PAUSED;
        break;
    }

    pimpl_->setStatus_(pimpl_->tbHandle, pimpl_->assocWindow, tbSevenStatus);
}


void TaskbarProgress::setProgress(size_t current, size_t total)
{
    pimpl_->setProgress_(pimpl_->tbHandle, pimpl_->assocWindow, current, total);
}
