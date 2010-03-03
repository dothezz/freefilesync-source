// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) 2008-2010 ZenJu (zhnmju123 AT gmx.de)                    *
// **************************************************************************
//
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
