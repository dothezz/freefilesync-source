// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) 2008-2011 ZenJu (zhnmju123 AT gmx.de)                    *
// **************************************************************************
//
#include "watcher.h"
#include "../shared/system_func.h"
#include "../shared/string_conv.h"
#include "../shared/file_handling.h"
#include "../shared/i18n.h"
#include <stdexcept>
#include <set>
#include <wx/timer.h>
#include <algorithm>
#include "../shared/resolve_path.h"

#ifdef FFS_WIN
#include "notify.h"
#include <wx/msw/wrapwin.h> //includes "windows.h"
#include "../shared/long_path_prefix.h"
#include <boost/shared_ptr.hpp>
#include "../shared/loki/ScopeGuard.h"
#include <boost/scoped_array.hpp>

#elif defined FFS_LINUX
#include "../shared/inotify/inotify-cxx.h"
#include "../shared/file_traverser.h"
#endif

using namespace ffs3;


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


#ifdef FFS_WIN
//shared_ptr custom deleter
void cleanUpChangeNotifications(const std::vector<HANDLE>* handles)
{
    for (std::vector<HANDLE>::const_iterator i = handles->begin(); i != handles->end(); ++i)
        if (*i != INVALID_HANDLE_VALUE)
            ::FindCloseChangeNotification(*i);

    delete handles; //don't forget!!! custom deleter needs to care for everything!
}

#elif defined FFS_LINUX
class DirsOnlyTraverser : public ffs3::TraverseCallback
{
public:
    DirsOnlyTraverser(std::vector<std::string>& dirs) : m_dirs(dirs) {}

    virtual void onFile(const Zchar* shortName, const Zstring& fullName, const FileInfo& details) {}
    virtual void onSymlink(const Zchar* shortName, const Zstring& fullName, const SymlinkInfo& details) {}
    virtual ReturnValDir onDir(const Zchar* shortName, const Zstring& fullName)
    {
        m_dirs.push_back(fullName.c_str());
        return ReturnValDir(Loki::Int2Type<ReturnValDir::TRAVERSING_DIR_CONTINUE>(), *this);
    }
    virtual void onError(const wxString& errorText)
    {
        throw ffs3::FileError(errorText);
    }

private:
    std::vector<std::string>& m_dirs;
};
#endif


class WatchDirectories //detect changes to directory availability
{
public:
    WatchDirectories() : allExisting_(true) {}

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
            allExisting_ = std::find_if(dirList.begin(), dirList.end(), std::not1(std::ptr_fun(&ffs3::dirExists))) == dirList.end();
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
    /*
    #warning cleanup
    {
    const Zstring formattedDir = ffs3::getFormattedDirectoryName(dirNames.front());

        //SE_BACKUP_NAME and SE_RESTORE_NAME <- required by FILE_FLAG_BACKUP_SEMANTICS???

        //open the directory to watch....
        HANDLE hDir = ::CreateFile(ffs3::applyLongPathPrefix(formattedDir).c_str(),
                                   FILE_LIST_DIRECTORY,
                                   FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, //leaving out last flag may prevent files to be deleted WITHIN monitored dir (http://qualapps.blogspot.com/2010/05/understanding-readdirectorychangesw_19.html)
                                   NULL,
                                   OPEN_EXISTING,
                                   FILE_FLAG_BACKUP_SEMANTICS,
                                   NULL);
        if (hDir == INVALID_HANDLE_VALUE)
        {
            const DWORD lastError = ::GetLastError();
            if (    lastError == ERROR_FILE_NOT_FOUND || //no need to check this condition any earlier!
                    lastError == ERROR_BAD_NETPATH)      //
                return CHANGE_DIR_MISSING;

            const wxString errorMessage = wxString(_("Could not initialize directory monitoring:")) + wxT("\n\"") + zToWx(formattedDir) + wxT("\"");
            throw ffs3::FileError(errorMessage + wxT("\n\n") + ffs3::getLastErrorFormatted());
        }

        Loki::ScopeGuard dummy = Loki::MakeGuard(::CloseHandle, hDir);
        (void)dummy; //silence warning "unused variable"


        const size_t bufferSize = sizeof(FILE_NOTIFY_INFORMATION);
        boost::scoped_array<char> tmp(new char[bufferSize]);
        FILE_NOTIFY_INFORMATION& notifyInfo = reinterpret_cast<FILE_NOTIFY_INFORMATION&>(*tmp.get());

        DWORD bytesWritten = 0;

        if (!::ReadDirectoryChangesW(
                hDir,        //__in         HANDLE hDirectory,
                &notifyInfo, //__out        LPVOID lpBuffer,
                bufferSize,  //__in         DWORD nBufferLength,
                true,        //__in         BOOL bWatchSubtree,
                FILE_NOTIFY_CHANGE_FILE_NAME  | //__in         DWORD dwNotifyFilter,
                FILE_NOTIFY_CHANGE_DIR_NAME   |
                FILE_NOTIFY_CHANGE_SIZE       |
                FILE_NOTIFY_CHANGE_LAST_WRITE,
                &bytesWritten, //__out_opt    LPDWORD lpBytesReturned,
                NULL,          //__inout_opt  LPOVERLAPPED lpOverlapped,
                NULL))         //__in_opt     LPOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine
        {
            const wxString errorMessage = wxString(_("Could not initialize directory monitoring:")) + wxT("\n\"") + zToWx(formattedDir) + wxT("\"");
            throw ffs3::FileError(errorMessage + wxT("\n\n") + ffs3::getLastErrorFormatted());
        }
        return CHANGE_DETECTED;
    }
    */









    if (dirNames.empty()) //pathological case, but check is needed nevertheless
        throw ffs3::FileError(_("At least one directory input field is empty."));

    //detect when volumes are removed/are not available anymore
    WatchDirectories dirWatcher;

#ifdef FFS_WIN
    typedef boost::shared_ptr<std::vector<HANDLE> > ChangeNotifList;
    ChangeNotifList changeNotifications(new std::vector<HANDLE>, ::cleanUpChangeNotifications);

    for (std::vector<Zstring>::const_iterator i = dirNames.begin(); i != dirNames.end(); ++i)
    {
        const Zstring formattedDir = ffs3::getFormattedDirectoryName(*i);

        if (formattedDir.empty())
            throw ffs3::FileError(_("At least one directory input field is empty."));

        dirWatcher.addForMonitoring(formattedDir);

        const HANDLE rv = ::FindFirstChangeNotification(
                              ffs3::applyLongPathPrefix(formattedDir).c_str(), //__in  LPCTSTR lpPathName,
                              true,                           //__in  BOOL bWatchSubtree,
                              FILE_NOTIFY_CHANGE_FILE_NAME |
                              FILE_NOTIFY_CHANGE_DIR_NAME  |
                              FILE_NOTIFY_CHANGE_SIZE      |
                              FILE_NOTIFY_CHANGE_LAST_WRITE); //__in  DWORD dwNotifyFilter

        if (rv == INVALID_HANDLE_VALUE)
        {
            const DWORD lastError = ::GetLastError();
            if (    lastError == ERROR_FILE_NOT_FOUND || //no need to check this condition any earlier!
                    lastError == ERROR_BAD_NETPATH)      //
                return CHANGE_DIR_MISSING;

            const wxString errorMessage = wxString(_("Could not initialize directory monitoring:")) + wxT("\n\"") + zToWx(formattedDir) + wxT("\"");
            throw ffs3::FileError(errorMessage + wxT("\n\n") + ffs3::getLastErrorFormatted());
        }

        changeNotifications->push_back(rv);
    }

    if (changeNotifications->size() == 0)
        throw ffs3::FileError(_("At least one directory input field is empty."));


    //detect user request for device removal (e.g. usb stick)
    class HandleVolumeRemoval : public NotifyRequestDeviceRemoval
    {
    public:
        HandleVolumeRemoval(ChangeNotifList& openHandles) :
            NotifyRequestDeviceRemoval(*openHandles), //throw (FileError)
            removalRequested(false),
            operationComplete(false),
            openHandles_(openHandles) {}

        bool requestReceived() const
        {
            return removalRequested;
        }
        bool finished() const
        {
            return operationComplete;
        }

    private:
        virtual void onRequestRemoval(HANDLE hnd) //don't throw!
        {
            openHandles_.reset();    //free all handles
            removalRequested = true; //and make sure they are not used anymore
        }
        virtual void onRemovalFinished(HANDLE hnd, bool successful) //throw()!
        {
            operationComplete = true;
        }

        bool removalRequested;
        bool operationComplete;
        ChangeNotifList& openHandles_;
    } removalRequest(changeNotifications);


    while (true)
    {
        //check for changes within directories:
        const DWORD rv = ::WaitForMultipleObjects( //NOTE: changeNotifications returns valid pointer, because it cannot be empty in this context
                             static_cast<DWORD>(changeNotifications->size()),  //__in  DWORD nCount,
                             &(*changeNotifications)[0],  //__in  const HANDLE *lpHandles,
                             false,                       //__in  BOOL bWaitAll,
                             UI_UPDATE_INTERVAL);         //__in  DWORD dwMilliseconds
        if (WAIT_OBJECT_0 <= rv && rv < WAIT_OBJECT_0 + changeNotifications->size())
            return CHANGE_DETECTED; //directory change detected
        else if (rv == WAIT_FAILED)
            throw ffs3::FileError(wxString(_("Error when monitoring directories.")) + wxT("\n\n") + ffs3::getLastErrorFormatted());
        //else if (rv == WAIT_TIMEOUT)

        if (!dirWatcher.allExisting()) //check for removed devices:
            return CHANGE_DIR_MISSING;

        statusHandler->requestUiRefresh();

        //handle device removal
        if (removalRequest.requestReceived())
        {
            const wxMilliClock_t maxwait = wxGetLocalTimeMillis() + 5000; //HandleVolumeRemoval::finished() not guaranteed!
            while (!removalRequest.finished() && wxGetLocalTimeMillis() < maxwait)
            {
                wxMilliSleep(rts::UI_UPDATE_INTERVAL);
                statusHandler->requestUiRefresh();
            }
            return CHANGE_DIR_MISSING;
        }
    }

#elif defined FFS_LINUX
    std::vector<std::string> fullDirList; //including subdirectories!

    //add all subdirectories
    for (std::vector<Zstring>::const_iterator i = dirNames.begin(); i != dirNames.end(); ++i)
    {
        const Zstring formattedDir = ffs3::getFormattedDirectoryName(*i);

        if (formattedDir.empty())
            throw ffs3::FileError(_("At least one directory input field is empty."));

        dirWatcher.addForMonitoring(formattedDir);


        fullDirList.push_back(formattedDir.c_str());

        try //get all subdirectories
        {
            DirsOnlyTraverser traverser(fullDirList);
            ffs3::traverseFolder(formattedDir, false, traverser); //don't traverse into symlinks (analog to windows build)
        }
        catch (const ffs3::FileError&)
        {
            if (!ffs3::dirExists(formattedDir)) //that's no good locking behavior, but better than nothing
                return CHANGE_DIR_MISSING;

            throw;
        }
    }

    try
    {
        Inotify notifications;
        notifications.SetNonBlock(true);

        for (std::vector<std::string>::const_iterator i = fullDirList.begin(); i != fullDirList.end(); ++i)
        {
            try
            {
                InotifyWatch newWatch(*i,              //dummy object: InotifyWatch may be destructed safely after Inotify::Add()
                                      IN_DONT_FOLLOW | //don't follow symbolic links
                                      IN_ONLYDIR     | //watch directories only
                                      IN_CLOSE_WRITE |
                                      IN_CREATE 	 |
                                      IN_DELETE 	 |
                                      IN_DELETE_SELF |
                                      IN_MODIFY 	 |
                                      IN_MOVE_SELF   |
                                      IN_MOVED_FROM  |
                                      IN_MOVED_TO );
                notifications.Add(newWatch);
            }
            catch (const InotifyException& e)
            {
                if (!ffs3::dirExists(i->c_str())) //that's no good locking behavior, but better than nothing
                    return CHANGE_DIR_MISSING;

                const wxString errorMessage = wxString(_("Could not initialize directory monitoring:")) + wxT("\n\"") + zToWx(i->c_str()) + wxT("\"");
                throw ffs3::FileError(errorMessage + wxT("\n\n") + zToWx(e.GetMessage().c_str()));
            }
        }


        if (notifications.GetWatchCount() == 0)
            throw ffs3::FileError(_("At least one directory input field is empty."));

        while (true)
        {
            notifications.WaitForEvents(); //called in non-blocking mode

            if (notifications.GetEventCount() > 0)
                return CHANGE_DETECTED; //directory change detected

            if (!dirWatcher.allExisting()) //check for removed devices:
                return CHANGE_DIR_MISSING;

            wxMilliSleep(rts::UI_UPDATE_INTERVAL);
            statusHandler->requestUiRefresh();
        }
    }
    catch (const InotifyException& e)
    {
        throw ffs3::FileError(wxString(_("Error when monitoring directories.")) + wxT("\n\n") + zToWx(e.GetMessage().c_str()));
    }
    catch (const std::exception& e)
    {
        throw ffs3::FileError(wxString(_("Error when monitoring directories.")) + wxT("\n\n") + zToWx(e.what()));
    }
#endif
}


void rts::waitForMissingDirs(const std::vector<Zstring>& dirNames, WaitCallback* statusHandler) //throw(FileError)
{
    //new: support for monitoring newly connected directories volumes (e.g.: USB-sticks)

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
                const Zstring formattedDir = ffs3::getFormattedDirectoryName(*i);

                if (formattedDir.empty())
                    throw ffs3::FileError(_("At least one directory input field is empty."));

                if (!ffs3::dirExists(formattedDir))
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
