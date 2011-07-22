// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) 2008-2011 ZenJu (zhnmju123 AT gmx.de)                    *
// **************************************************************************
//
#include "watcher.h"
#include "../shared/file_handling.h"
#include "../shared/i18n.h"
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


class MonitorExistence //detect changes to directory availability
{
public:
    MonitorExistence() : allExisting_(true) {}

    //initialization
    void addForMonitoring(const Zstring& dirName)
    {
        dirList.insert(dirName);
    }

    bool allExisting() const //polling explicitly allowed!
    {
        const int UPDATE_INTERVAL = 1000; //1 second interval

        const wxLongLong current = wxGetLocalTimeMillis();
        if (current - lastCheck >= UPDATE_INTERVAL)
        {
            lastCheck = current;
            allExisting_ = std::find_if(dirList.begin(), dirList.end(), std::not1(std::ptr_fun(&zen::dirExists))) == dirList.end();
        }

        return allExisting_;
    }

private:
    mutable wxLongLong lastCheck;
    mutable bool allExisting_;

    std::set<Zstring, LessFilename> dirList; //save avail. directories, avoid double-entries
};



rts::WaitResult rts::waitForChanges(const std::vector<Zstring>& dirNames, WaitCallback* statusHandler) //throw(FileError)
{
    if (dirNames.empty()) //pathological case, but check is needed nevertheless
        throw zen::FileError(_("A directory input field is empty."));

    //detect when volumes are removed/are not available anymore
    MonitorExistence checkExist;
    std::vector<std::shared_ptr<DirWatcher>> watches;

    for (std::vector<Zstring>::const_iterator i = dirNames.begin(); i != dirNames.end(); ++i)
    {
        const Zstring formattedDir = zen::getFormattedDirectoryName(*i);

        if (formattedDir.empty())
            throw zen::FileError(_("A directory input field is empty."));

        checkExist.addForMonitoring(formattedDir);

        try
        {
            watches.push_back(std::make_shared<DirWatcher>(formattedDir)); //throw FileError
        }
        catch (zen::FileError&)
        {
            if (!zen::dirExists(formattedDir)) //that's no good locking behavior, but better than nothing
                return CHANGE_DIR_MISSING;
            throw;
        }
    }

    while (true)
    {
        //IMPORTANT CHECK: dirwatcher has problems detecting removal of top watched directories!
        if (!checkExist.allExisting()) //check for removed devices:
            return CHANGE_DIR_MISSING;

        try
        {
            for (auto iter = watches.begin(); iter != watches.end(); ++iter)
            {
                std::vector<Zstring> changedFiles = (*iter)->getChanges(); //throw FileError

                //remove to be ignored changes
                changedFiles.erase(std::remove_if(changedFiles.begin(), changedFiles.end(),
                                                  [](const Zstring& name)
                {
                    return endsWith(name, Zstr(".ffs_lock")) || //sync.ffs_lock, sync.Del.ffs_lock
                           endsWith(name, Zstr(".ffs_db"));     //sync.ffs_db, .sync.tmp.ffs_db
                    //no need to ignore temporal recycle bin directory: this must be caused by a file deletion anyway
                }), changedFiles.end());

                if (!changedFiles.empty())
                {
/*
                std::for_each(changedFiles.begin(), changedFiles.end(),
                [](const Zstring& fn) { wxMessageBox(toWx(fn));});

				const wxString filename = toWx(changedFiles[0]);
                    ::wxSetEnv(wxT("RTS_CHANGE"), filename);
*/

                    return CHANGE_DETECTED; //directory change detected
                }
            }
        }
        catch (FileError&)
        {
            //maybe some error is caused due to some unexpected removal/unavailability of a watched directory? If so we can remedy this error:
            if (!checkExist.allExisting())
                return CHANGE_DIR_MISSING;
            throw;
        }

        wxMilliSleep(rts::UI_UPDATE_INTERVAL);
        statusHandler->requestUiRefresh();
    }
}


//support for monitoring newly connected directories volumes (e.g.: USB-sticks)
void rts::waitForMissingDirs(const std::vector<Zstring>& dirNames, WaitCallback* statusHandler) //throw(FileError)
{
    wxLongLong lastCheck;

    while (true)
    {
        const int UPDATE_INTERVAL = 1000; //1 second interval
        const wxLongLong current = wxGetLocalTimeMillis();
        if (current - lastCheck >= UPDATE_INTERVAL)
        {
            lastCheck = current;

            bool allExisting = true;
            for (std::vector<Zstring>::const_iterator i = dirNames.begin(); i != dirNames.end(); ++i)
            {
                //support specifying volume by name => call getFormattedDirectoryName() repeatedly
                const Zstring formattedDir = zen::getFormattedDirectoryName(*i);

                if (formattedDir.empty())
                    throw zen::FileError(_("A directory input field is empty."));

                if (!zen::dirExists(formattedDir))
                {
                    allExisting = false;
                    break;
                }
            }
            if (allExisting) //check for newly arrived devices:
                return;
        }

        wxMilliSleep(rts::UI_UPDATE_INTERVAL);
        statusHandler->requestUiRefresh();
    }
}
