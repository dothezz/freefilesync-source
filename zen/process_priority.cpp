// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) Zenju (zenju AT gmx DOT de) - All Rights Reserved        *
// **************************************************************************

#include "process_priority.h"
#include <zen/sys_error.h>
#include <zen/i18n.h>

#ifdef ZEN_WIN
#include "win.h" //includes "windows.h"

#elif defined ZEN_LINUX
//#include <sys/syscall.h>

#elif defined ZEN_MAC
#include <sys/resource.h> //getiopolicy_np
#include <IOKit/pwr_mgt/IOPMLib.h> //keep in .cpp file to not pollute global namespace! e.g. with UInt64
#endif

using namespace zen;


#ifdef ZEN_WIN
struct PreventStandby::Pimpl {};

PreventStandby::PreventStandby()
{
    if (::SetThreadExecutionState(ES_CONTINUOUS | ES_SYSTEM_REQUIRED) == 0)
        throw FileError(_("Unable to suspend system sleep mode.")); //no GetLastError() support?
}


PreventStandby::~PreventStandby()
{
    ::SetThreadExecutionState(ES_CONTINUOUS);
}


#ifndef PROCESS_MODE_BACKGROUND_BEGIN
#define PROCESS_MODE_BACKGROUND_BEGIN     0x00100000 // Windows Server 2003 and Windows XP/2000:  This value is not supported!
#define PROCESS_MODE_BACKGROUND_END       0x00200000 //
#endif

struct ScheduleForBackgroundProcessing::Pimpl {};


ScheduleForBackgroundProcessing::ScheduleForBackgroundProcessing()
{
    if (!::SetPriorityClass(::GetCurrentProcess(), PROCESS_MODE_BACKGROUND_BEGIN)) //this call lowers CPU priority, too!!
        throwFileError(_("Cannot change process I/O priorities."), L"SetPriorityClass", getLastError());
}


ScheduleForBackgroundProcessing::~ScheduleForBackgroundProcessing()
{
    ::SetPriorityClass(::GetCurrentProcess(), PROCESS_MODE_BACKGROUND_END);
}

#elif defined ZEN_LINUX
struct PreventStandby::Pimpl {};
PreventStandby::PreventStandby() {}
PreventStandby::~PreventStandby() {}

//solution for GNOME?: http://people.gnome.org/~mccann/gnome-session/docs/gnome-session.html#org.gnome.SessionManager.Inhibit

struct ScheduleForBackgroundProcessing::Pimpl {};
ScheduleForBackgroundProcessing::ScheduleForBackgroundProcessing() {};
ScheduleForBackgroundProcessing::~ScheduleForBackgroundProcessing() {};

/*
struct ScheduleForBackgroundProcessing
{
	- required functions ioprio_get/ioprio_set are not part of glibc: http://linux.die.net/man/2/ioprio_set
	- and probably never will: http://sourceware.org/bugzilla/show_bug.cgi?id=4464
	- /usr/include/linux/ioprio.h not available on Ubuntu, so we can't use it instead

	ScheduleForBackgroundProcessing() : oldIoPrio(getIoPriority(IOPRIO_WHO_PROCESS, ::getpid()))
	{
		if (oldIoPrio != -1)
			setIoPriority(::getpid(), IOPRIO_PRIO_VALUE(IOPRIO_CLASS_IDLE, 0));
	}
	~ScheduleForBackgroundProcessing()
	{
		if (oldIoPrio != -1)
			setIoPriority(::getpid(), oldIoPrio);
	}

private:
	static int getIoPriority(pid_t pid)
	{
		return ::syscall(SYS_ioprio_get, IOPRIO_WHO_PROCESS, pid);
	}
	static int setIoPriority(pid_t pid, int ioprio)
	{
		return ::syscall(SYS_ioprio_set, IOPRIO_WHO_PROCESS, pid, ioprio);
	}

	const int oldIoPrio;
};
*/

#elif defined ZEN_MAC
//https://developer.apple.com/library/mac/#qa/qa1340
struct PreventStandby::Pimpl
{
    Pimpl() : assertionID() {}
    IOPMAssertionID assertionID;
};

PreventStandby::PreventStandby() : pimpl(make_unique<Pimpl>())
{
    IOReturn rv = ::IOPMAssertionCreateWithName(kIOPMAssertionTypeNoIdleSleep,
                                                kIOPMAssertionLevelOn,
                                                CFSTR("FreeFileSync"),
                                                &pimpl->assertionID);
    if (rv != kIOReturnSuccess)
        throw FileError(_("Unable to suspend system sleep mode."), replaceCpy<std::wstring>(L"IOReturn Code %x", L"%x", numberTo<std::wstring>(rv))); //could not find a better way to convert IOReturn to string
}

PreventStandby::~PreventStandby()
{
    ::IOPMAssertionRelease(pimpl->assertionID);
}


struct ScheduleForBackgroundProcessing::Pimpl
{
    Pimpl() : oldIoPrio() {}
    int oldIoPrio;
};


ScheduleForBackgroundProcessing::ScheduleForBackgroundProcessing() : pimpl(make_unique<Pimpl>())
{
    pimpl->oldIoPrio = ::getiopolicy_np(IOPOL_TYPE_DISK, IOPOL_SCOPE_PROCESS);
    if (pimpl->oldIoPrio == -1)
        throwFileError(_("Cannot change process I/O priorities."), L"getiopolicy_np", getLastError());

    if (::setiopolicy_np(IOPOL_TYPE_DISK, IOPOL_SCOPE_PROCESS, IOPOL_THROTTLE) != 0)
        throwFileError(_("Cannot change process I/O priorities."), L"setiopolicy_np", getLastError());
}


ScheduleForBackgroundProcessing::~ScheduleForBackgroundProcessing()
{
    ::setiopolicy_np(IOPOL_TYPE_DISK, IOPOL_SCOPE_PROCESS, pimpl->oldIoPrio);
}
#endif
