// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) ZenJu (zenju AT gmx DOT de) - All Rights Reserved        *
// **************************************************************************

#ifndef TRAYMENU_H_INCLUDED
#define TRAYMENU_H_INCLUDED

#include "watcher.h"
#include "xml_proc.h"


namespace rts
{
enum AbortReason
{
    SHOW_GUI,
    EXIT_APP
};
AbortReason startDirectoryMonitor(const xmlAccess::XmlRealConfig& config, const wxString& jobname); //jobname may be empty
}


#endif // TRAYMENU_H_INCLUDED
