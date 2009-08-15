#ifndef TRAYMENU_H_INCLUDED
#define TRAYMENU_H_INCLUDED

#include "watcher.h"
#include "xmlProcessing.h"


namespace RealtimeSync
{
    enum MonitorResponse
    {
        RESUME,
        QUIT
    };

    MonitorResponse startDirectoryMonitor(const xmlAccess::XmlRealConfig& config);
}


#endif // TRAYMENU_H_INCLUDED
