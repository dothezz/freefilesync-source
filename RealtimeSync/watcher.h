// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) 2008-2010 ZenJu (zhnmju123 AT gmx.de)                    *
// **************************************************************************
//
#ifndef WATCHER_H_INCLUDED
#define WATCHER_H_INCLUDED

#include "functions.h"
#include <vector>
#include "../shared/fileError.h"


namespace RealtimeSync
{
const int UI_UPDATE_INTERVAL = 100; //perform ui updates not more often than necessary, 100 seems to be a good value with only a minimal performance loss

class WaitCallback
{
public:
    virtual ~WaitCallback() {}
    virtual void requestUiRefresh() = 0; //opportunity to abort must be implemented in a frequently executed method like requestUiRefresh()
};

void waitForChanges(const std::vector<wxString>& dirNames, WaitCallback* statusHandler); //throw(FreeFileSync::FileError);
}

#endif // WATCHER_H_INCLUDED
