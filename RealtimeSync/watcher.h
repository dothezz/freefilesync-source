// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) ZenJu (zhnmju123 AT gmx DOT de) - All Rights Reserved    *
// **************************************************************************

#ifndef WATCHER_H_INCLUDED
#define WATCHER_H_INCLUDED

#include <zen/zstring.h>
#include <vector>
#include <zen/file_error.h>


namespace rts
{
const int UI_UPDATE_INTERVAL = 100; //perform ui updates not more often than necessary, 100 seems to be a good value with only a minimal performance loss

bool updateUiIsAllowed();


class WaitCallback
{
public:
    virtual ~WaitCallback() {}
    virtual void requestUiRefresh() = 0; //opportunity to abort must be implemented in a frequently executed method like requestUiRefresh()
};


//wait until changes are detected or if a directory is not available (anymore)
enum ChangeType
{
    CHANGE_DETECTED,
    CHANGE_DIR_MISSING
};
struct WaitResult
{
    WaitResult(ChangeType tp, const Zstring& chgFile = Zstring()) : type(tp), filename(chgFile) {}

    ChangeType type;
    Zstring filename; //filled if type == CHANGE_DETECTED
};
WaitResult waitForChanges(const std::vector<Zstring>& dirNamesNonFmt, //non-formatted dir names that yet require call to getFormattedDirectoryName()
                          WaitCallback* statusHandler); //throw(FileError)

//wait until all directories become available (again)
void waitForMissingDirs(const std::vector<Zstring>& dirNamesNonFmt,
                        WaitCallback* statusHandler); //throw(FileError)
}

#endif // WATCHER_H_INCLUDED
