// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) 2008-2011 ZenJu (zhnmju123 AT gmx.de)                    *
// **************************************************************************

#ifndef DLLLOADER_H_INCLUDED
#define DLLLOADER_H_INCLUDED

#include <memory>
#include <string>
#include "loki\ScopeGuard.h"

#ifdef __WXMSW__ //we have wxWidgets
#include <wx/msw/wrapwin.h> //includes "windows.h"
#else
#include <windows.h>
#undef max
#undef min
#endif


namespace util
{
/*
Manage DLL function and library ownership
   - thread safety: like built-in type
   - full value semantics

 Usage:
      typedef BOOL (WINAPI *IsWow64ProcessFun)(HANDLE hProcess, PBOOL Wow64Process);
      const util::DllFun<IsWow64ProcessFun> isWow64Process(L"kernel32.dll", "IsWow64Process");
      if (isWow64Process)
*/

template <class Func>
class DllFun
{
public:
    DllFun() : fun(NULL) {}

    DllFun(const wchar_t* libraryName, const char* functionName) :
        hLibRef(new HMODULE(::LoadLibrary(libraryName)), deleter),
        fun(*hLibRef ? reinterpret_cast<Func>(::GetProcAddress(*hLibRef, functionName)) : NULL) {}

    operator Func() const { return fun; }

private:
    static void deleter(HMODULE* ptr) { if (*ptr) ::FreeLibrary(*ptr); delete ptr; }

    std::shared_ptr<HMODULE> hLibRef;
    Func fun;
};

/*
extract binary resources from .exe/.dll:

-- resource.h --
#define MY_BINARY_RESOURCE 1337

-- resource.rc --
MY_BINARY_RESOURCE RCDATA "filename.dat"
*/
std::string getResourceStream(const std::wstring& libraryName, size_t resourceId);
















//---------------Inline Implementation---------------------------------------------------
inline
std::string getResourceStream(const wchar_t* libraryName, size_t resourceId)
{
    std::string output;
    HMODULE module = ::LoadLibrary(libraryName);
    if (module)
    {
        LOKI_ON_BLOCK_EXIT2(::FreeLibrary(module));

        const HRSRC res = ::FindResource(module, MAKEINTRESOURCE(resourceId), RT_RCDATA);
        if (res != NULL)
        {
            const HGLOBAL resHandle = ::LoadResource(module, res);
            if (resHandle != NULL)
            {
                const char* stream = static_cast<const char*>(::LockResource(resHandle));
                if (stream)
                {
                    const DWORD streamSize = ::SizeofResource(module, res);
                    output.assign(stream, streamSize);
                }
            }
        }
    }
    return output;
}
}

#endif // DLLLOADER_H_INCLUDED
