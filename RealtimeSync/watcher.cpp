// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) ZenJu (zhnmju123 AT gmx DOT de) - All Rights Reserved    *
// **************************************************************************

#include "watcher.h"
#include <zen/file_handling.h>
#include <zen/stl_tools.h>
#include <set>
#include <ctime>
#include <wx/timer.h>
#include "../lib/resolve_path.h"
#include <zen/dir_watcher.h>
#include <wx+/string_conv.h>
#include <zen/thread.h>
//#include "../library/db_file.h"     //SYNC_DB_FILE_ENDING -> complete file too much of a dependency; file ending too little to decouple into single header
//#include "../library/lock_holder.h" //LOCK_FILE_ENDING
#include <wx/msgdlg.h>

using namespace zen;


bool rts::updateUiIsAllowed()
{
    const std::clock_t CLOCK_UPDATE_INTERVAL = UI_UPDATE_INTERVAL * CLOCKS_PER_SEC / 1000;

    static std::clock_t lastExec = 0;
    const std::clock_t now = std::clock(); //this is quite fast: 2 * 10^-5

    if (now - lastExec >= CLOCK_UPDATE_INTERVAL)  //perform ui updates not more often than necessary
    {
        lastExec = now;
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
            watches.push_back(std::make_pair(dirnameFmt, std::make_shared<DirWatcher>(dirnameFmt))); //throw FileError, ErrorNotExisting
        }
        catch (ErrorNotExisting&) //nice atomic behavior: *no* second directory existence check!!!
        {
            return CHANGE_DIR_MISSING;
        }
        catch (FileError&) //play safe: remedy potential FileErrors that should have been ErrorNotExisting (e.g. Linux: errors during directory traversing)
        {
            if (!dirExists(dirnameFmt)) //not an atomic behavior!!!
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
                std::vector<Zstring> changedFiles = watcher.getChanges([&] { statusHandler->requestUiRefresh(); }); //throw FileError, ErrorNotExisting

                //remove to be ignored changes
                vector_remove_if(changedFiles, [](const Zstring& name)
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
            catch (ErrorNotExisting&) //nice atomic behavior: *no* second directory existence check!!!
            {
                return CHANGE_DIR_MISSING;
            }
            catch (FileError&) //play safe: remedy potential FileErrors that should have been ErrorNotExisting (e.g. Linux: errors during directory traversing)
            {
                if (!dirExists(dirname)) //not an atomic behavior!!!
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

            auto ftDirMissing = async([=]() -> bool
            {
                return std::find_if(dirNamesNonFmt.begin(), dirNamesNonFmt.end(),
                [](const Zstring& dirnameNonFmt) -> bool
                {
                    //support specifying volume by name => call getFormattedDirectoryName() repeatedly
                    const Zstring dirnameFmt = zen::getFormattedDirectoryName(dirnameNonFmt);

                    if (dirnameFmt.empty())
                        throw zen::FileError(_("A directory input field is empty."));
#ifdef FFS_WIN
                    //1. login to network share, if necessary
                    loginNetworkShare(dirnameFmt, false); //login networks shares, no PW prompt -> is this really RTS's task?
#endif
                    //2. check dir existence
                    return !zen::dirExists(dirnameFmt);
                }) != dirNamesNonFmt.end();
            });
            while (!ftDirMissing.timed_wait(boost::posix_time::milliseconds(rts::UI_UPDATE_INTERVAL)))
                statusHandler->requestUiRefresh(); //may throw!

            try
            {
                if (!ftDirMissing.get()) //throw X
                    return;
            }
            catch (...) //boost::future seems to map async exceptions to "some" boost exception type -> migrate this for C++11
            {
                throw zen::FileError(_("A directory input field is empty."));
            }
        }

        wxMilliSleep(rts::UI_UPDATE_INTERVAL);
        statusHandler->requestUiRefresh();
    }
}
