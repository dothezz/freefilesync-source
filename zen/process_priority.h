#ifndef PREVENTSTANDBY_H_INCLUDED
#define PREVENTSTANDBY_H_INCLUDED

#ifdef FFS_WIN
#include "win.h" //includes "windows.h"

#elif defined FFS_LINUX
#endif

namespace zen
{
struct PreventStandby //signal a "busy" state to the operating system
{
#ifdef FFS_WIN
    PreventStandby () { ::SetThreadExecutionState(ES_CONTINUOUS | ES_SYSTEM_REQUIRED /* | ES_AWAYMODE_REQUIRED*/ ); }
    ~PreventStandby() { ::SetThreadExecutionState(ES_CONTINUOUS); }
#endif
};


struct ScheduleForBackgroundProcessing //lower CPU and file I/O priorities
{
#ifdef FFS_WIN
#ifndef PROCESS_MODE_BACKGROUND_BEGIN
#define PROCESS_MODE_BACKGROUND_BEGIN     0x00100000 // Windows Server 2003 and Windows XP/2000:  This value is not supported!
#define PROCESS_MODE_BACKGROUND_END       0x00200000 //
#endif

    ScheduleForBackgroundProcessing () { ::SetPriorityClass(::GetCurrentProcess(), PROCESS_MODE_BACKGROUND_BEGIN); } //this call lowers CPU priority, too!!
    ~ScheduleForBackgroundProcessing() { ::SetPriorityClass(::GetCurrentProcess(), PROCESS_MODE_BACKGROUND_END  ); }

#elif defined FFS_LINUX
    /*
    CPU prio:
    int getpriority(PRIO_PROCESS, 0); - errno caveat!
    int ::setpriority(PRIO_PROCESS, 0, int prio); //a zero value for "who" denotes the calling process

    priority can be decreased, but NOT increased :(

    file I/O prio:
        ScheduleForBackgroundProcessing() : oldIoPrio(::ioprio_get(IOPRIO_WHO_PROCESS, ::getpid()))
        {
            if (oldIoPrio != -1)
                ::ioprio_set(IOPRIO_WHO_PROCESS, ::getpid(), IOPRIO_PRIO_VALUE(IOPRIO_CLASS_IDLE, 0));
        }
        ~ScheduleForBackgroundProcessing()
        {
            if (oldIoPrio != -1)
                ::ioprio_set(IOPRIO_WHO_PROCESS, ::getpid(), oldIoPrio);
          }

    private:
            const int oldIoPrio;
    */
#endif
};
}

#endif // PREVENTSTANDBY_H_INCLUDED
