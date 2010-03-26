// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) 2008-2010 ZenJu (zhnmju123 AT gmx.de)                    *
// **************************************************************************
//
#include "watcher.h"
#include "../shared/systemFunctions.h"
#include "functions.h"
#include <wx/intl.h>
#include <wx/filefn.h>
#include "../shared/fileHandling.h"
#include "../shared/stringConv.h"
#include <stdexcept>
#include <map>
#include <wx/timer.h>

#ifdef FFS_WIN
//#include "../shared/fileId.h"
//#include "Dbt.h"
#include <wx/msw/wrapwin.h> //includes "windows.h"
#include "../shared/longPathPrefix.h"

#elif defined FFS_LINUX
#include <wx/timer.h>
#include <exception>
#include "../shared/inotify/inotify-cxx.h"
#include "../shared/fileTraverser.h"
#endif

using namespace FreeFileSync;


bool RealtimeSync::updateUiIsAllowed()
{
    static wxLongLong lastExec = 0;
    const wxLongLong  newExec  = wxGetLocalTimeMillis();

    if (newExec - lastExec >= RealtimeSync::UI_UPDATE_INTERVAL)  //perform ui updates not more often than necessary
    {
        lastExec = newExec;
        return true;
    }
    return false;
}

#ifdef FFS_WIN
/*
template <class T>    //have a disctinct static variable per class!
class InstanceCounter //exception safety!!!! use RAII for counter inc/dec!
{
public:
    InstanceCounter()
    {
        ++instanceCount;
        //we're programming on global variables: only one instance of NotifyDeviceArrival allowed at a time!
        if (instanceCount > 1)
            throw std::logic_error("Only one instance of NotifyDeviceArrival allowed!");
    }
    ~InstanceCounter()
    {
        --instanceCount;
    }
private:
    static size_t instanceCount; //this class needs to be a singleton but with variable lifetime! => count instances to check consistency
};
template <class T> //we need a disctinct static variable per class!
size_t InstanceCounter<T>::instanceCount = 0;


std::set<Zstring> driveNamesArrived; //letters of newly arrived drive names


//convert bitmask into "real" drive-letter
void notifyDriveFromMask (ULONG unitmask)
{
    for (wchar_t i = 0; i < 26; ++i)
    {
        if (unitmask & 0x1)
        {
            Zstring newDrivePath;
            newDrivePath += DefaultChar('A') + i;
            newDrivePath += DefaultStr(":\\");
            driveNamesArrived.insert(newDrivePath);
            return;
        }
        unitmask = unitmask >> 1;
    }
}


LRESULT CALLBACK MainWndProc(
    HWND hwnd,        // handle to window
    UINT uMsg,        // message identifier
    WPARAM wParam,    // first message parameter
    LPARAM lParam)    // second message parameter
{

    //detect device arrival: http://msdn.microsoft.com/en-us/library/aa363215(VS.85).aspx
    if (uMsg == WM_DEVICECHANGE)
    {
        if (wParam == DBT_DEVICEARRIVAL)
        {
            PDEV_BROADCAST_HDR lpdb = reinterpret_cast<PDEV_BROADCAST_HDR>(lParam);
            if (lpdb->dbch_devicetype == DBT_DEVTYP_VOLUME)
            {
                PDEV_BROADCAST_VOLUME lpdbv = reinterpret_cast<PDEV_BROADCAST_VOLUME>(lpdb);
                //warning: lpdbv->dbcv_flags is 0 for USB-sticks!

                //insert drive name notification into global variable:
                notifyDriveFromMask(lpdbv->dbcv_unitmask);
            }
        }
    }
    //default
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}


class NotifyDeviceArrival //e.g. insertion of USB-Stick
{
public:
    NotifyDeviceArrival() :
        parentInstance(NULL),
        registeredClass(NULL),
        windowHandle(NULL)
    {
        //get program's module handle
        parentInstance = GetModuleHandle(NULL);
        if (parentInstance == NULL)
            throw FreeFileSync::FileError(wxString(("Could not start monitoring for volume arrival:")) + wxT("\n\n") +
                                          FreeFileSync::getLastErrorFormatted()+ wxT(" (GetModuleHandle)"));

        //register the main window class
        WNDCLASS wc;
        wc.style         = 0;
        wc.lpfnWndProc   = MainWndProc;
        wc.cbClsExtra    = 0;
        wc.cbWndExtra    = 0;
        wc.hInstance     = parentInstance;
        wc.hIcon         = NULL;
        wc.hCursor       = NULL;
        wc.hbrBackground = NULL;
        wc.lpszMenuName  = NULL;
        wc.lpszClassName = wxT("DeviceArrivalWatcher");

        registeredClass =:: RegisterClass(&wc);
        if (registeredClass == 0)
            throw FreeFileSync::FileError(wxString(("Could not start monitoring for volume arrival:")) + wxT("\n\n") +
                                          FreeFileSync::getLastErrorFormatted()+ wxT(" (RegisterClass)"));

        //create dummy-window
        windowHandle = ::CreateWindow(
                           reinterpret_cast<LPCTSTR>(registeredClass), //LPCTSTR lpClassName OR ATOM in low-order word!
                           0, //LPCTSTR lpWindowName,
                           0, //DWORD dwStyle,
                           0, //int x,
                           0, //int y,
                           0, //int nWidth,
                           0, //int nHeight,
                           0, //note: we need a toplevel window to receive device arrival events, not a message-window (HWND_MESSAGE)!
                           NULL,           //HMENU hMenu,
                           parentInstance, //HINSTANCE hInstance,
                           NULL);          //LPVOID lpParam
        if (windowHandle == NULL)
            throw FreeFileSync::FileError(wxString( ("Could not start monitoring for volume arrival:")) + wxT("\n\n") +
                                          FreeFileSync::getLastErrorFormatted() + wxT(" (CreateWindow)"));

        //clear global variable
        driveNamesArrived.clear();
    }

    ~NotifyDeviceArrival()
    {
        //clean-up in reverse order
        if (windowHandle != NULL)
            ::DestroyWindow(windowHandle);

        if (registeredClass != 0)
            ::UnregisterClass(reinterpret_cast<LPCTSTR>(registeredClass), //LPCTSTR lpClassName OR ATOM in low-order word!
                              parentInstance); //HINSTANCE hInstance
    }


    //test if one of the notifications matches one of the directory paths specified
    bool notificationsFound(const std::vector<Zstring>& dirList) const
    {
        //do NOT rely on string parsing! use (volume directory) file ids!
        std::set<Utility::FileID> notifiedIds;
        for (std::set<Zstring>::const_iterator j = driveNamesArrived.begin(); j != driveNamesArrived.end(); ++j)
        {
            const Utility::FileID notifiedVolId = Utility::retrieveFileID(*j);
            if (notifiedVolId != Utility::FileID())
                notifiedIds.insert(notifiedVolId);
        }
        //clear global variable
        driveNamesArrived.clear();

        if (!notifiedIds.empty()) //minor optimization
        {
            for (std::vector<Zstring>::const_iterator i = dirList.begin(); i != dirList.end(); ++i)
            {
                //retrieve volume name
                wchar_t volumeNameRaw[1000];
                if (::GetVolumePathName(i->c_str(),    //__in   LPCTSTR lpszFileName,
                                        volumeNameRaw, //__out  LPTSTR lpszVolumePathName,
                                        1000))         //__in   DWORD cchBufferLength
                {
                    const Utility::FileID monitoredId = Utility::retrieveFileID(volumeNameRaw);
                    if (monitoredId != Utility::FileID())
                    {
                        if (notifiedIds.find(monitoredId) != notifiedIds.end())
                            return true;
                    }
                }
            }
        }

        return false;
    }

private:
    HINSTANCE parentInstance;
    ATOM registeredClass;
    HWND windowHandle;

    //we're programming on global variables: only one instance of NotifyDeviceArrival allowed at a time!
    InstanceCounter<NotifyDeviceArrival> dummy; //exception safety!!!! use RAII for counter inc/dec!
};
*/


//--------------------------------------------------------------------------------------------------------------
class ChangeNotifications
{
public:
    ~ChangeNotifications()
    {
        for (std::vector<HANDLE>::const_iterator i = arrayHandle.begin(); i != arrayHandle.end(); ++i)
            if (*i != INVALID_HANDLE_VALUE)
                ::FindCloseChangeNotification(*i);
    }

    void addHandle(const HANDLE hndl)
    {
        arrayHandle.push_back(hndl);
    }

    size_t getSize() const
    {
        return arrayHandle.size();
    }

    const HANDLE* getArray()
    {
        return &arrayHandle[0]; //client needs to check getSize() before calling this method!
    }

private:
    std::vector<HANDLE> arrayHandle;
};

#elif defined FFS_LINUX
class DirsOnlyTraverser : public FreeFileSync::TraverseCallback
{
public:
    DirsOnlyTraverser(std::vector<std::string>& dirs) : m_dirs(dirs) {}

    virtual ReturnValue onFile(const DefaultChar* shortName, const Zstring& fullName, const FileInfo& details)
    {
        return TRAVERSING_CONTINUE;
    }
    virtual ReturnValDir onDir(const DefaultChar* shortName, const Zstring& fullName)
    {
        m_dirs.push_back(fullName.c_str());
        return ReturnValDir(Loki::Int2Type<ReturnValDir::TRAVERSING_DIR_CONTINUE>(), this);
    }
    virtual ReturnValue onError(const wxString& errorText)
    {
        throw FreeFileSync::FileError(errorText);
    }

private:
    std::vector<std::string>& m_dirs;
};
#endif


class NotifyDirectoryArrival //detect changes to directory availability
{
public:
    //initialization
    void addForMonitoring(const Zstring& dirName, bool isExisting) //dir-existence already checked by calling method, avoid double-checking -> consistency!
    {
        availablility[dirName] = isExisting;
    }

    //detection
    bool changeDetected() //polling explicitly allowed!
    {
        const int UPDATE_INTERVAL = 1000; //1 second interval

        static wxLongLong lastExec = 0;
        const wxLongLong  newExec  = wxGetLocalTimeMillis();

        if (newExec - lastExec >= UPDATE_INTERVAL)
        {
            lastExec = newExec;

            for (std::map<Zstring, bool>::iterator i = availablility.begin(); i != availablility.end(); ++i)
                if (FreeFileSync::dirExists(i->first) != i->second) //change in availability
                {
                    if (i->second) //directory doesn't exist anymore: no reason to trigger the commandline! (sometimes triggered by ChangeNotifications anyway...)
                        i->second = false; //update value, so that dir-arrival will be detected next time
                    else //directory arrival: trigger commandline!
                        return true;
                }
        }

        return false;
    }

private:
    std::map<Zstring, bool> availablility; //save avail. status for each directory, avoid double-entries
};


void RealtimeSync::waitForChanges(const std::vector<wxString>& dirNames, WaitCallback* statusHandler)
{
    if (dirNames.empty()) //pathological case, but check is needed later
        return;

    //new: support for monitoring newly connected directories volumes (e.g.: USB-sticks)
    NotifyDirectoryArrival monitorAvailability;

#ifdef FFS_WIN
    ChangeNotifications notifications;

    for (std::vector<wxString>::const_iterator i = dirNames.begin(); i != dirNames.end(); ++i)
    {
        const Zstring formattedDir = FreeFileSync::getFormattedDirectoryName(i->c_str());

        if (formattedDir.empty())
            throw FreeFileSync::FileError(_("At least one directory input field is empty."));

        const bool isExisting = FreeFileSync::dirExists(formattedDir);
        if (isExisting)
        {
            const HANDLE rv = ::FindFirstChangeNotification(
                                  FreeFileSync::applyLongPathPrefix(formattedDir).c_str(),           //__in  LPCTSTR lpPathName,
                                  true,                           //__in  BOOL bWatchSubtree,
                                  FILE_NOTIFY_CHANGE_FILE_NAME |
                                  FILE_NOTIFY_CHANGE_DIR_NAME  |
                                  FILE_NOTIFY_CHANGE_SIZE      |
                                  FILE_NOTIFY_CHANGE_LAST_WRITE); //__in  DWORD dwNotifyFilter

            if (rv == INVALID_HANDLE_VALUE)
            {
                const wxString errorMessage = wxString(_("Could not initialize directory monitoring:")) + wxT("\n\"") + *i + wxT("\"");
                throw FreeFileSync::FileError(errorMessage + wxT("\n\n") + FreeFileSync::getLastErrorFormatted());
            }

            notifications.addHandle(rv);
        }
        //else: we silently ignore this error: it may be that the directory becomes available later, e.g. if it is a USB-stick

        monitorAvailability.addForMonitoring(formattedDir, isExisting); //all directories (including not yet existing) are relevant
    }


    while (true)
    {
        //check for changes within directories:
        if (notifications.getSize() > 0)
        {
            const DWORD rv = ::WaitForMultipleObjects(     //NOTE: notifications.getArray() returns valid pointer, because it cannot be empty in this context
                                 static_cast<DWORD>(notifications.getSize()),  //__in  DWORD nCount,
                                 notifications.getArray(), //__in  const HANDLE *lpHandles,
                                 false,                    //__in  BOOL bWaitAll,
                                 UI_UPDATE_INTERVAL);      //__in  DWORD dwMilliseconds
            if (WAIT_OBJECT_0 <= rv && rv < WAIT_OBJECT_0 + notifications.getSize())
                return; //directory change detected
            else if (rv == WAIT_FAILED)
                throw FreeFileSync::FileError(wxString(_("Error when monitoring directories.")) + wxT("\n\n") + FreeFileSync::getLastErrorFormatted());
            //else if (rv == WAIT_TIMEOUT)
        }

        if (monitorAvailability.changeDetected()) //check for newly arrived devices:
            return;

        statusHandler->requestUiRefresh();
    }

#elif defined FFS_LINUX
    std::vector<std::string> fullDirList; //including subdirectories!

    //add all subdirectories
    for (std::vector<wxString>::const_iterator i = dirNames.begin(); i != dirNames.end(); ++i)
    {
        const Zstring formattedDir = FreeFileSync::getFormattedDirectoryName(wxToZ(*i));

        if (formattedDir.empty())
            throw FreeFileSync::FileError(_("At least one directory input field is empty."));

        const bool isExisting = FreeFileSync::dirExists(formattedDir);
        if (isExisting)
        {
            fullDirList.push_back(formattedDir.c_str());
            //get all subdirectories
            DirsOnlyTraverser traverser(fullDirList);
            FreeFileSync::traverseFolder(formattedDir, false, &traverser); //don't traverse into symlinks (analog to windows build)
        }
        //else: we silently ignore this error: it may be that the directory becomes available later, e.g. if it is a USB-stick

        monitorAvailability.addForMonitoring(formattedDir, isExisting); //all directories (including not yet existing) are relevant
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
                const wxString errorMessage = wxString(_("Could not initialize directory monitoring:")) + wxT("\n\"") + zToWx(i->c_str()) + wxT("\"");
                throw FreeFileSync::FileError(errorMessage + wxT("\n\n") + zToWx(e.GetMessage().c_str()));
            }
        }


        while (true)
        {
            notifications.WaitForEvents(); //called in non-blocking mode

            if (notifications.GetEventCount() > 0)
                return; //directory change detected

            if (monitorAvailability.changeDetected()) //check for newly arrived devices:
                return;

            wxMilliSleep(RealtimeSync::UI_UPDATE_INTERVAL);
            statusHandler->requestUiRefresh();
        }
    }
    catch (const InotifyException& e)
    {
        throw FreeFileSync::FileError(wxString(_("Error when monitoring directories.")) + wxT("\n\n") + zToWx(e.GetMessage().c_str()));
    }
    catch (const std::exception& e)
    {
        throw FreeFileSync::FileError(wxString(_("Error when monitoring directories.")) + wxT("\n\n") + zToWx(e.what()));
    }
#endif
}

