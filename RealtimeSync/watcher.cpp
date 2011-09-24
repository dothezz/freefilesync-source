// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) 2008-2011 ZenJu (zhnmju123 AT gmx.de)                    *
// **************************************************************************

#include "watcher.h"
#include "../shared/file_handling.h"
#include "../shared/i18n.h"
#include "../shared/stl_tools.h"
#include <set>
#include <wx/timer.h>
#include "../shared/resolve_path.h"
#include "../shared/dir_watcher.h"
#include "../shared/string_conv.h"
//#include "../library/db_file.h"     //SYNC_DB_FILE_ENDING -> complete file too much of a dependency; file ending too little to decouple into single header
//#include "../library/lock_holder.h" //LOCK_FILE_ENDING
#include <wx/msgdlg.h>

using namespace zen;


bool rts::updateUiIsAllowed()
{
    static wxLongLong lastExec;
    const wxLongLong  newExec = wxGetLocalTimeMillis();

    if (newExec - lastExec >= rts::UI_UPDATE_INTERVAL)  //perform ui updates not more often than necessary
    {
        lastExec = newExec;
        return true;
    }
    return false;
}

namespace
{
const int CHECK_DIR_INTERVAL = 1000; //1 second interval
}


rts::WaitResult rts::waitForChanges(const std::vector<Zstring>& dirNamesNonFmt, WaitCallback* statusHandler) //throw FileError
{
    std::set<Zstring, LessFilename> dirNamesFmt;

    std::for_each(dirNamesNonFmt.begin(), dirNamesNonFmt.end(),
                  [&](const Zstring& dirnameNonFmt)
    {
        const Zstring& dirnameFmt = zen::getFormattedDirectoryName(dirnameNonFmt);

        if (dirnameFmt.empty())
            throw zen::FileError(_("A directory input field is empty."));
        dirNamesFmt.insert(dirnameFmt);
    });
    if (dirNamesFmt.empty()) //pathological case, but check is needed nevertheless
        throw zen::FileError(_("A directory input field is empty."));


    //detect when volumes are removed/are not available anymore
    std::vector<std::pair<Zstring, std::shared_ptr<DirWatcher>>> watches;

    for (auto iter = dirNamesFmt.begin(); iter != dirNamesFmt.end(); ++iter)
    {
        const Zstring& dirnameFmt = *iter;
        try
        {
            watches.push_back(std::make_pair(dirnameFmt, std::make_shared<DirWatcher>(dirnameFmt))); //throw FileError
        }
        catch (FileError&)
        {
            //Note: checking for directory existence is NOT transactional!!!
            if (!dirExists(dirnameFmt)) //that's no good locking behavior, but better than nothing
                return CHANGE_DIR_MISSING;
            throw;
        }
    }

    wxLongLong lastCheck;
    while (true)
    {
        const bool checkDirExistNow = [&lastCheck]() -> bool //checking once per sec should suffice
        {
            const wxLongLong current = wxGetLocalTimeMillis();
            if (current - lastCheck >= CHECK_DIR_INTERVAL)
            {
                lastCheck = current;
                return true;
            }
            return false;
        }();


        for (auto iter = watches.begin(); iter != watches.end(); ++iter)
        {
            const Zstring& dirname = iter->first;
            DirWatcher& watcher = *(iter->second);

            //IMPORTANT CHECK: dirwatcher has problems detecting removal of top watched directories!
            if (checkDirExistNow)
                if (!dirExists(dirname)) //catch errors related to directory removal, e.g. ERROR_NETNAME_DELETED
                    return CHANGE_DIR_MISSING;

            try
            {
                std::vector<Zstring> changedFiles = watcher.getChanges(); //throw FileError

                //remove to be ignored changes
                vector_remove_if(changedFiles, [](const Zstring & name)
                {
                    return endsWith(name, Zstr(".ffs_lock")) || //sync.ffs_lock, sync.Del.ffs_lock
                           endsWith(name, Zstr(".ffs_db"));     //sync.ffs_db, .sync.tmp.ffs_db
                    //no need to ignore temporal recycle bin directory: this must be caused by a file deletion anyway
                });

                if (!changedFiles.empty())
                {
                    /*
                                    std::for_each(changedFiles.begin(), changedFiles.end(),
                                    [](const Zstring& fn) { wxMessageBox(toWx(fn));});
                    */
                    return WaitResult(CHANGE_DETECTED, changedFiles[0]); //directory change detected
                }

            }
            catch (FileError&)
            {
                //Note: checking for directory existence is NOT transactional!!!
                if (!dirExists(dirname)) //catch errors related to directory removal, e.g. ERROR_NETNAME_DELETED
                    return CHANGE_DIR_MISSING;
                throw;
            }
        }

        wxMilliSleep(rts::UI_UPDATE_INTERVAL);
        statusHandler->requestUiRefresh();
    }
}


//support for monitoring newly connected directories volumes (e.g.: USB-sticks)
void rts::waitForMissingDirs(const std::vector<Zstring>& dirNamesNonFmt, WaitCallback* statusHandler) //throw FileError
{
    wxLongLong lastCheck;

    while (true)
    {
        const wxLongLong current = wxGetLocalTimeMillis();
        if (current - lastCheck >= CHECK_DIR_INTERVAL)
        {
            lastCheck = current;

            if (std::find_if(dirNamesNonFmt.begin(), dirNamesNonFmt.end(),
                             [&](const Zstring& dirnameNonFmt) -> bool
        {
            //support specifying volume by name => call getFormattedDirectoryName() repeatedly
            const Zstring formattedDir = zen::getFormattedDirectoryName(dirnameNonFmt);

                if (formattedDir.empty())
                    throw zen::FileError(_("A directory input field is empty."));

                return !dirExists(formattedDir);
            }) == dirNamesNonFmt.end())
            return;
        }

        wxMilliSleep(rts::UI_UPDATE_INTERVAL);
        statusHandler->requestUiRefresh();
    }
}
