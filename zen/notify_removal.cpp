// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) Zenju (zenju AT gmx DOT de) - All Rights Reserved        *
// **************************************************************************

#include "notify_removal.h"
#include <set>
#include <algorithm>
#include <dbt.h>
#include "scope_guard.h"

using namespace zen;


namespace
{
bool messageProviderConstructed = false;
}


class MessageProvider //administrates a single dummy window to receive messages
{
public:
    static MessageProvider& instance() //throw (FileError)
    {
        static MessageProvider inst;
        messageProviderConstructed = true;
        return inst;
    }

    class Listener
    {
    public:
        virtual ~Listener() {}
        virtual void onMessage(UINT message, WPARAM wParam, LPARAM lParam) = 0; //throw()!
    };
    void   registerListener(Listener& l) { listener.insert(&l); }
    void unregisterListener(Listener& l) { listener.erase(&l); } //don't unregister objects with static lifetime

    HWND getWnd() const { return windowHandle; } //get handle in order to register additional notifications

private:
    MessageProvider();
    ~MessageProvider();
    MessageProvider(const MessageProvider&);
    MessageProvider& operator=(const MessageProvider&);

    static const wchar_t WINDOW_NAME[];

    friend LRESULT CALLBACK topWndProc(HWND, UINT, WPARAM, LPARAM);
    void processMessage(UINT message, WPARAM wParam, LPARAM lParam);

    const HINSTANCE hMainModule;
    HWND windowHandle;

    std::set<Listener*> listener;
};


const wchar_t MessageProvider::WINDOW_NAME[] = L"E6AD5EB1-527B-4EEF-AC75-27883B233380"; //random name


LRESULT CALLBACK topWndProc(HWND hwnd,     //handle to window
                            UINT uMsg,     //message identifier
                            WPARAM wParam, //first message parameter
                            LPARAM lParam) //second message parameter
{
    if (messageProviderConstructed) //attention: this callback is triggered in the middle of singleton construction! It is a bad idea to to call back at this time!
        try
        {
            MessageProvider::instance().processMessage(uMsg, wParam, lParam); //not supposed to throw
        }
        catch (...) { assert(false); }

    return ::DefWindowProc(hwnd, uMsg, wParam, lParam);
}


MessageProvider::MessageProvider() :
    hMainModule(::GetModuleHandle(nullptr)), //get program's module handle
    windowHandle(nullptr)
{
    if (!hMainModule)
        throw zen::FileError(std::wstring(L"Could not start monitoring window notifications:") + L"\n\n" + getLastErrorFormatted() + L" (GetModuleHandle)");

    //register the main window class
    WNDCLASS wc = {};
    wc.lpfnWndProc   = topWndProc;
    wc.hInstance     = hMainModule;
    wc.lpszClassName = WINDOW_NAME;

    if (::RegisterClass(&wc) == 0)
        throw zen::FileError(std::wstring(L"Could not start monitoring window notifications:") + L"\n\n" + getLastErrorFormatted() + L" (RegisterClass)");

    ScopeGuard guardClass = makeGuard([&] { ::UnregisterClass(WINDOW_NAME, hMainModule); });

    //create dummy-window
    windowHandle = ::CreateWindow(WINDOW_NAME, //LPCTSTR lpClassName OR ATOM in low-order word!
                                  nullptr,     //LPCTSTR lpWindowName,
                                  0, //DWORD dwStyle,
                                  0, //int x,
                                  0, //int y,
                                  0, //int nWidth,
                                  0, //int nHeight,
                                  0, //note: we need a toplevel window to receive device arrival events, not a message-window (HWND_MESSAGE)!
                                  nullptr,     //HMENU hMenu,
                                  hMainModule, //HINSTANCE hInstance,
                                  nullptr);    //LPVOID lpParam
    if (!windowHandle)
        throw zen::FileError(std::wstring(L"Could not start monitoring window notifications:") + L"\n\n" + getLastErrorFormatted() + L" (CreateWindow)");

    guardClass.dismiss();
}


MessageProvider::~MessageProvider()
{
    //clean-up in reverse order
    ::DestroyWindow(windowHandle);
    ::UnregisterClass(WINDOW_NAME, //LPCTSTR lpClassName OR ATOM in low-order word!
                      hMainModule); //HINSTANCE hInstance
}


void MessageProvider::processMessage(UINT message, WPARAM wParam, LPARAM lParam)
{
    std::for_each(listener.begin(), listener.end(),
    [&](Listener* ls) { ls->onMessage(message, wParam, lParam); });
}
//####################################################################################################


class NotifyRequestDeviceRemoval::Pimpl : private MessageProvider::Listener
{
public:
    Pimpl(NotifyRequestDeviceRemoval& parent, HANDLE hDir) : //throw FileError
        parent_(parent)
    {
        MessageProvider::instance().registerListener(*this); //throw FileError

        //register handles to receive notifications
        DEV_BROADCAST_HANDLE filter = {};
        filter.dbch_size = sizeof(filter);
        filter.dbch_devicetype = DBT_DEVTYP_HANDLE;
        filter.dbch_handle = hDir;

        hNotification = ::RegisterDeviceNotification(MessageProvider::instance().getWnd(), //__in  HANDLE hRecipient,
                                                     &filter,                              //__in  LPVOID NotificationFilter,
                                                     DEVICE_NOTIFY_WINDOW_HANDLE);         //__in  DWORD Flags
        if (!hNotification)
        {
            const DWORD lastError = ::GetLastError();
            if (lastError != ERROR_CALL_NOT_IMPLEMENTED   && //fail on SAMBA share: this shouldn't be a showstopper!
                lastError != ERROR_SERVICE_SPECIFIC_ERROR && //neither should be fail for "Pogoplug" mapped network drives
                lastError != ERROR_INVALID_DATA)             //this seems to happen for a NetDrive-mapped FTP server
                throw zen::FileError(L"Could not register device removal notifications:" L"\n\n" + getLastErrorFormatted(lastError));
        }
    }

    ~Pimpl()
    {
        ::UnregisterDeviceNotification(hNotification);
        MessageProvider::instance().unregisterListener(*this);
    }

private:
    Pimpl(Pimpl&);
    Pimpl& operator=(Pimpl&);

    virtual void onMessage(UINT message, WPARAM wParam, LPARAM lParam) //throw()!
    {
        //DBT_DEVICEQUERYREMOVE example: http://msdn.microsoft.com/en-us/library/aa363427(v=VS.85).aspx
        if (message == WM_DEVICECHANGE)
        {
            if (    wParam == DBT_DEVICEQUERYREMOVE       ||
                    wParam == DBT_DEVICEQUERYREMOVEFAILED ||
                    wParam == DBT_DEVICEREMOVECOMPLETE)
            {
                PDEV_BROADCAST_HDR header = reinterpret_cast<PDEV_BROADCAST_HDR>(lParam);
                if (header->dbch_devicetype == DBT_DEVTYP_HANDLE)
                {
                    PDEV_BROADCAST_HANDLE body = reinterpret_cast<PDEV_BROADCAST_HANDLE>(lParam);

#ifdef __MINGW32__
                    const HDEVNOTIFY requestNotification = reinterpret_cast<HDEVNOTIFY>(body->dbch_hdevnotify);
#else
                    const HDEVNOTIFY requestNotification = body->dbch_hdevnotify;
#endif
                    if (requestNotification == hNotification) //is it for the notification we registered?
                        switch (wParam)
                        {
                            case DBT_DEVICEQUERYREMOVE:
                                parent_.onRequestRemoval(body->dbch_handle);
                                break;
                            case DBT_DEVICEQUERYREMOVEFAILED:
                                parent_.onRemovalFinished(body->dbch_handle, false);
                                break;
                            case DBT_DEVICEREMOVECOMPLETE:
                                parent_.onRemovalFinished(body->dbch_handle, true);
                                break;
                        }
                }
            }
        }
    }

    NotifyRequestDeviceRemoval& parent_;
    HDEVNOTIFY hNotification;
};
//####################################################################################################


NotifyRequestDeviceRemoval::NotifyRequestDeviceRemoval(HANDLE hDir)
{
    pimpl.reset(new Pimpl(*this, hDir));
}


NotifyRequestDeviceRemoval::~NotifyRequestDeviceRemoval() {} //make sure ~auto_ptr() works with complete type
