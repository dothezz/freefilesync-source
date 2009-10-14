#include "watcher.h"
#include "../shared/systemFunctions.h"
#include "functions.h"
#include <wx/intl.h>
#include <wx/filefn.h>
#include "../shared/fileHandling.h"
#include "../shared/stringConv.h"

#ifdef FFS_WIN
#include <wx/msw/wrapwin.h> //includes "windows.h"

#elif defined FFS_LINUX
#include <wx/timer.h>
#include <exception>
#include "../shared/inotify/inotify-cxx.h"
#include "../shared/fileTraverser.h"
#endif

using namespace FreeFileSync;


#ifdef FFS_WIN
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

    size_t getSize()
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
        return ReturnValDir(ReturnValDir::Continue(), this);
    }
    virtual ReturnValue onError(const wxString& errorText)
    {
        throw FreeFileSync::FileError(errorText);
    }

private:
    std::vector<std::string>& m_dirs;
};
#endif


void RealtimeSync::waitForChanges(const std::vector<wxString>& dirNames, WaitCallback* statusHandler)
{
    if (dirNames.empty()) //pathological case, but check is needed later
        return;

#ifdef FFS_WIN
    ChangeNotifications notifications;

    for (std::vector<wxString>::const_iterator i = dirNames.begin(); i != dirNames.end(); ++i)
    {
        const Zstring formattedDir = FreeFileSync::getFormattedDirectoryName(i->c_str());

        if (formattedDir.empty())
            throw FreeFileSync::FileError(_("Please fill all empty directory fields."));
        else if (!FreeFileSync::dirExists(formattedDir))
            throw FreeFileSync::FileError(wxString(_("Directory does not exist:")) + wxT("\n") +
                                          wxT("\"") + formattedDir + wxT("\"") + wxT("\n\n") +
                                          FreeFileSync::getLastErrorFormatted());

        const HANDLE rv = ::FindFirstChangeNotification(
                              formattedDir.c_str(),           //__in  LPCTSTR lpPathName,
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


    while (true)
    {
        const DWORD rv = ::WaitForMultipleObjects(     //NOTE: notifications.getArray() returns valid pointer, because it cannot be empty in this context
                             notifications.getSize(),  //__in  DWORD nCount,
                             notifications.getArray(), //__in  const HANDLE *lpHandles,
                             false,                    //__in  BOOL bWaitAll,
                             UI_UPDATE_INTERVAL);      //__in  DWORD dwMilliseconds
        if (WAIT_OBJECT_0 <= rv && rv < WAIT_OBJECT_0 + notifications.getSize())
            return; //directory change detected
        else if (rv == WAIT_FAILED)
            throw FreeFileSync::FileError(wxString(_("Error when monitoring directories.")) + wxT("\n\n") + FreeFileSync::getLastErrorFormatted());
        else if (rv == WAIT_TIMEOUT)
            statusHandler->requestUiRefresh();
    }

#elif defined FFS_LINUX
    std::vector<std::string> fullDirList;

    //add all subdirectories
    for (std::vector<wxString>::const_iterator i = dirNames.begin(); i != dirNames.end(); ++i)
    {
        const Zstring formattedDir = FreeFileSync::getFormattedDirectoryName(wxToZ(*i));

        if (formattedDir.empty())
            throw FreeFileSync::FileError(_("Please fill all empty directory fields."));

        else if (!FreeFileSync::dirExists(formattedDir))
            throw FreeFileSync::FileError(wxString(_("Directory does not exist:")) + wxT("\n") +
                                          wxT("\"") + zToWx(formattedDir) + wxT("\"") + wxT("\n\n") +
                                          FreeFileSync::getLastErrorFormatted());

        fullDirList.push_back(formattedDir.c_str());
        //get all subdirectories
        DirsOnlyTraverser traverser(fullDirList);
        FreeFileSync::traverseFolder(formattedDir, false, &traverser); //don't traverse into symlinks (analog to windows build)
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
