// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) Zenju (zenju AT gmx DOT de) - All Rights Reserved        *
// **************************************************************************

#include "watcher.h"
#include <set>
#include <zen/tick_count.h>
#include <zen/file_handling.h>
#include <zen/stl_tools.h>
#include <zen/dir_watcher.h>
#include <zen/thread.h>
#include <zen/assert_static.h>
#include <zen/tick_count.h>
#include <wx+/string_conv.h>
#include "../lib/resolve_path.h"
//#include "../library/db_file.h"     //SYNC_DB_FILE_ENDING -> complete file too much of a dependency; file ending too little to decouple into single header
//#include "../library/lock_holder.h" //LOCK_FILE_ENDING

using namespace zen;


namespace
{
const std::int64_t TICKS_UPDATE_INTERVAL = rts::UI_UPDATE_INTERVAL* ticksPerSec() / 1000;
TickVal lastExec = getTicks();
};

bool rts::updateUiIsAllowed()
{
    const TickVal now = getTicks(); //0 on error
    if (dist(lastExec, now) >= TICKS_UPDATE_INTERVAL)  //perform ui updates not more often than necessary
    {
        lastExec = now;
        return true;
    }
    return false;
}


namespace
{
const int CHECK_DIR_INTERVAL = 1; //unit: [s]


std::vector<Zstring> getFormattedDirs(const std::vector<Zstring>& dirs) //throw FileError
{
    std::set<Zstring, LessFilename> tmp; //make unique

    std::transform(dirs.begin(), dirs.end(), std::inserter(tmp, tmp.end()),
    [](const Zstring& dirnameNonFmt) { return getFormattedDirectoryName(dirnameNonFmt); });

    return std::vector<Zstring>(tmp.begin(), tmp.end());
}
}


rts::WaitResult rts::waitForChanges(const std::vector<Zstring>& dirNamesNonFmt, WaitCallback& statusHandler) //throw FileError
{
    const std::vector<Zstring> dirNamesFmt = getFormattedDirs(dirNamesNonFmt); //throw FileError
    if (dirNamesFmt.empty()) //pathological case, but we have to check else this function will wait endlessly
        throw zen::FileError(_("A folder input field is empty.")); //should have been checked by caller!

    //detect when volumes are removed/are not available anymore
    std::vector<std::pair<Zstring, std::shared_ptr<DirWatcher>>> watches;

    for (auto it = dirNamesFmt.begin(); it != dirNamesFmt.end(); ++it)
    {
        const Zstring& dirnameFmt = *it;
        try
        {
            //a non-existent network path may block, so check existence asynchronously!
            auto ftDirExists = async([=] { return zen::dirExists(dirnameFmt); });
            while (!ftDirExists.timed_wait(boost::posix_time::milliseconds(UI_UPDATE_INTERVAL / 2)))
                statusHandler.requestUiRefresh(); //may throw!
            if (!ftDirExists.get())
                return WaitResult(dirnameFmt);

            watches.push_back(std::make_pair(dirnameFmt, std::make_shared<DirWatcher>(dirnameFmt))); //throw FileError, ErrorNotExisting
        }
        catch (ErrorNotExisting&) //nice atomic behavior: *no* second directory existence check!!!
        {
            return WaitResult(dirnameFmt);
        }
        catch (FileError&) //play safe: remedy potential FileErrors that should have been ErrorNotExisting (e.g. Linux: errors during directory traversing)
        {
            if (!dirExists(dirnameFmt)) //file system race condition!!
                return WaitResult(dirnameFmt);
            throw;
        }
    }

    const std::int64_t TICKS_DIR_CHECK_INTERVAL = CHECK_DIR_INTERVAL * ticksPerSec(); //0 on error
    TickVal lastCheck = getTicks(); //0 on error
    while (true)
    {
        const bool checkDirExistNow = [&]() -> bool //checking once per sec should suffice
        {
            const TickVal now = getTicks(); //0 on error
            if (dist(lastCheck, now) >= TICKS_DIR_CHECK_INTERVAL)
            {
                lastCheck = now;
                return true;
            }
            return false;
        }();


        for (auto it = watches.begin(); it != watches.end(); ++it)
        {
            const Zstring& dirname = it->first;
            DirWatcher& watcher = *(it->second);

            //IMPORTANT CHECK: dirwatcher has problems detecting removal of top watched directories!
            if (checkDirExistNow)
                if (!dirExists(dirname)) //catch errors related to directory removal, e.g. ERROR_NETNAME_DELETED
                    return WaitResult(dirname);

            try
            {
                std::vector<DirWatcher::Entry> changedItems = watcher.getChanges([&] { statusHandler.requestUiRefresh(); }); //throw FileError, ErrorNotExisting

                //remove to be ignored changes
                vector_remove_if(changedItems, [](const DirWatcher::Entry& e)
                {
                    return endsWith(e.filename_, Zstr(".ffs_lock")) || //sync.ffs_lock, sync.Del.ffs_lock
                           endsWith(e.filename_, Zstr(".ffs_db"));     //sync.ffs_db, .sync.tmp.ffs_db
                    //no need to ignore temporal recycle bin directory: this must be caused by a file deletion anyway
                });

                if (!changedItems.empty())
                {
                    /*
                                    std::for_each(changedItems.begin(), changedItems.end(),
                                    [](const Zstring& fn) { wxMessageBox(toWx(fn));});
                    */
                    return WaitResult(changedItems[0]); //directory change detected
                }

            }
            catch (ErrorNotExisting&) //nice atomic behavior: *no* second directory existence check!!!
            {
                return WaitResult(dirname);
            }
            catch (FileError&) //play safe: remedy potential FileErrors that should have been ErrorNotExisting (e.g. Linux: errors during directory traversing)
            {
                if (!dirExists(dirname)) //file system race condition!!
                    return WaitResult(dirname);
                throw;
            }
        }

        boost::this_thread::sleep(boost::posix_time::milliseconds(rts::UI_UPDATE_INTERVAL / 2));
        statusHandler.requestUiRefresh(true); //throw ?: may start sync at this presumably idle time
    }
}


//support for monitoring newly connected directories volumes (e.g.: USB-sticks)
void rts::waitForMissingDirs(const std::vector<Zstring>& dirNamesNonFmt, WaitCallback& statusHandler) //throw FileError
{
    while (true)
    {
        //support specifying volume by name => call getFormattedDirectoryName() repeatedly
        const std::vector<Zstring>& dirNamesFmt = getFormattedDirs(dirNamesNonFmt); //throw FileError

        bool allExisting = true;
        for (auto it = dirNamesFmt.begin(); it != dirNamesFmt.end(); ++it)
        {
            const Zstring dirnameFmt = *it;
            auto ftDirExisting = async([=]() -> bool
            {
#ifdef ZEN_WIN
                //1. login to network share, if necessary -> we probably do NOT want multiple concurrent runs: GUI!?
                loginNetworkShare(dirnameFmt, false); //login networks shares, no PW prompt -> is this really RTS's job?
#endif
                //2. check dir existence
                return zen::dirExists(dirnameFmt);
            });
            while (!ftDirExisting.timed_wait(boost::posix_time::milliseconds(rts::UI_UPDATE_INTERVAL / 2)))
                statusHandler.requestUiRefresh(); //may throw!

            if (!ftDirExisting.get())
            {
                allExisting = false;
                break;
            }
        }
        if (allExisting)
            return;

        //wait some time...
        const int refreshInterval = UI_UPDATE_INTERVAL / 2;
        assert_static(1000 * CHECK_DIR_INTERVAL % refreshInterval == 0);
        for (int i = 0; i < 1000 * CHECK_DIR_INTERVAL / refreshInterval; ++i)
        {
            boost::this_thread::sleep(boost::posix_time::milliseconds(refreshInterval));
            statusHandler.requestUiRefresh();
        }
    }
}
