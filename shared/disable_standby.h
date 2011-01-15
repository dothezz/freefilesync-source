#ifndef PREVENTSTANDBY_H_INCLUDED
#define PREVENTSTANDBY_H_INCLUDED

#ifdef FFS_WIN
#include <wx/msw/wrapwin.h> //includes "windows.h"
#endif

namespace util
{
class DisableStandby
{
public:
    DisableStandby()
    {
#ifdef FFS_WIN
        ::SetThreadExecutionState(ES_CONTINUOUS | ES_SYSTEM_REQUIRED /* | ES_AWAYMODE_REQUIRED*/ );
#endif
    }

    ~DisableStandby()
    {
#ifdef FFS_WIN
        ::SetThreadExecutionState(ES_CONTINUOUS);
#endif
    }

};
}

#endif // PREVENTSTANDBY_H_INCLUDED
