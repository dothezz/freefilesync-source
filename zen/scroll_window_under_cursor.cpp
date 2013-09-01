// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) Zenju (zenju AT gmx DOT de) - All Rights Reserved        *
// **************************************************************************

//redirect mouse wheel events directly to window under cursor rather than window having input focus
//implementing new Windows Vista UI guidelines: http://msdn.microsoft.com/en-us/library/bb545459.aspx#wheel
//this is confirmed to be required for at least Windows 2000 to Windows 8
//on Ubuntu Linux, this is already the default behavior

//Usage: just include this file into a Windows project

#include <cassert>
#include "win.h"     //includes "windows.h"
#include <Windowsx.h> //WM_MOUSEWHEEL


namespace
{
LRESULT CALLBACK mouseInputHook(int nCode, WPARAM wParam, LPARAM lParam)
{
    //"if nCode is less than zero, the hook procedure must pass the message to the CallNextHookEx function
    //without further processing and should return the value returned by CallNextHookEx"
    if (nCode == HC_ACTION) //the only valid value for this hook type
    {
        MSG& msgInfo = *reinterpret_cast<MSG*>(lParam);

        if (msgInfo.message == WM_MOUSEWHEEL ||
            msgInfo.message == WM_MOUSEHWHEEL)
        {
            POINT pt = {};
            pt.x = GET_X_LPARAM(msgInfo.lParam); //yes, there's also msgInfo.pt, but let's not take chances
            pt.y = GET_Y_LPARAM(msgInfo.lParam); //

            //visible child window directly under cursor; attention: not necessarily from our process!
            if (HWND hWin = ::WindowFromPoint(pt)) //http://blogs.msdn.com/b/oldnewthing/archive/2010/12/30/10110077.aspx
                if (msgInfo.hwnd != hWin && ::GetCapture() == nullptr)
                {
                    DWORD winProcessId = 0;
                    ::GetWindowThreadProcessId( //no-fail!
                        hWin,           //_In_       HWND hWnd,
                        &winProcessId); //_Out_opt_  LPDWORD lpdwProcessId
                    if (winProcessId == ::GetCurrentProcessId()) //no-fail!
                        msgInfo.hwnd = hWin; //it would be a bug to set handle from another process here
                }
        }
    }
    return ::CallNextHookEx(nullptr, nCode, wParam, lParam);
}

struct Dummy
{
    Dummy()
    {
        hHook = ::SetWindowsHookEx(WH_GETMESSAGE,  //__in  int idHook,
                                   mouseInputHook, //__in  HOOKPROC lpfn,
                                   nullptr,        //__in  HINSTANCE hMod,
                                   ::GetCurrentThreadId()); //__in  DWORD dwThreadId
        assert(hHook);
    }

    ~Dummy()
    {
        if (hHook)
            ::UnhookWindowsHookEx(hHook);
    }

private:
    HHOOK hHook;
} dummy;
}
