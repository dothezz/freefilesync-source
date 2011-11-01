#ifndef PREVENTSTANDBY_H_INCLUDED
#define PREVENTSTANDBY_H_INCLUDED

#ifdef FFS_WIN
#include "win.h" //includes "windows.h"
#endif

namespace zen
{
struct DisableStandby //signal a "busy" state to the operating system
{
#ifdef FFS_WIN
    DisableStandby() { ::SetThreadExecutionState(ES_CONTINUOUS | ES_SYSTEM_REQUIRED /* | ES_AWAYMODE_REQUIRED*/ ); }
    ~DisableStandby() { ::SetThreadExecutionState(ES_CONTINUOUS); }
#endif
};


#ifndef PROCESS_MODE_BACKGROUND_BEGIN
#define PROCESS_MODE_BACKGROUND_BEGIN     0x00100000 // Windows Server 2003 and Windows XP/2000:  This value is not supported!
#define PROCESS_MODE_BACKGROUND_END       0x00200000 //
#endif

struct ScheduleForBackgroundProcessing //lower CPU and file I/O priorities
{
#ifdef FFS_WIN
    ScheduleForBackgroundProcessing() { ::SetPriorityClass(::GetCurrentProcess(), PROCESS_MODE_BACKGROUND_BEGIN); }
    ~ScheduleForBackgroundProcessing() { ::SetPriorityClass(::GetCurrentProcess(), PROCESS_MODE_BACKGROUND_END); }
#endif
};
}

#endif // PREVENTSTANDBY_H_INCLUDED
