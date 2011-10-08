#ifndef PREVENTSTANDBY_H_INCLUDED
#define PREVENTSTANDBY_H_INCLUDED

#ifdef FFS_WIN
#include "win.h" //includes "windows.h"
#endif

namespace zen
{
class DisableStandby
{
public:
#ifdef FFS_WIN
    DisableStandby()
    {
        ::SetThreadExecutionState(ES_CONTINUOUS | ES_SYSTEM_REQUIRED /* | ES_AWAYMODE_REQUIRED*/ );
    }

    ~DisableStandby()
    {
        ::SetThreadExecutionState(ES_CONTINUOUS);
    }
#endif
};
}

#endif // PREVENTSTANDBY_H_INCLUDED
