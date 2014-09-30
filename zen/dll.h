// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) Zenju (zenju AT gmx DOT de) - All Rights Reserved        *
// **************************************************************************

#ifndef DLLLOADER_H_4239582598670968
#define DLLLOADER_H_4239582598670968

#include <memory>
#ifdef ZEN_WIN
#include <string>
#include "scope_guard.h"
#include "win.h" //includes "windows.h"

#elif defined ZEN_LINUX || defined ZEN_MAC
#include <dlfcn.h>
#endif

namespace zen
{
/*
Manage DLL function and library ownership
   - thread safety: like built-in type
   - full value semantics

 Usage:
    typedef BOOL (WINAPI* FunType_IsWow64Process)(HANDLE hProcess, PBOOL Wow64Process);
    const zen::SysDllFun<FunType_IsWow64Process> isWow64Process(L"kernel32.dll", "IsWow64Process");
    if (isWow64Process) ... use function ptr ...

 Usage 2:
	#define DEF_DLL_FUN(name) DllFun<dll_ns::FunType_##name> name(dll_ns::getDllName(), dll_ns::funName_##name);
	DEF_DLL_FUN(funname1); DEF_DLL_FUN(funname2); DEF_DLL_FUN(funname3);
*/

template <class Func>
class DllFun
{
public:
    DllFun() : fun(nullptr) {}

#ifdef ZEN_WIN
    DllFun(const wchar_t* libraryName, const char* functionName) :
        hLibRef(::LoadLibrary(libraryName), ::FreeLibrary),
        fun(hLibRef ? reinterpret_cast<Func>(::GetProcAddress(static_cast<HMODULE>(hLibRef.get()), functionName)) : nullptr) {}
#elif defined ZEN_LINUX || defined ZEN_MAC
    DllFun(const char* libraryName, const char* functionName) :
        hLibRef(::dlopen(libraryName, RTLD_LAZY), ::dlclose),
        fun(hLibRef ? reinterpret_cast<Func>(::dlsym(hLibRef.get(), functionName)) : nullptr) {}
#endif
    operator Func() const { return fun; }

private:
    std::shared_ptr<void> hLibRef; //we would prefer decltype(*HMODULE()) if only it would work...
    Func fun;
};


#ifdef ZEN_WIN
//if the dll is already part of the process space, e.g. "kernel32.dll" or "shell32.dll", we can use a faster variant:
//NOTE: since the lifetime of the referenced library is *not* controlled, this is safe to use only for permanently loaded libraries like these!
template <class Func>
class SysDllFun
{
public:
    SysDllFun() : fun(nullptr) {}

    SysDllFun(const wchar_t* systemLibrary, const char* functionName)
    {
        HMODULE mod = ::GetModuleHandle(systemLibrary);
        fun = mod ? reinterpret_cast<Func>(::GetProcAddress(mod, functionName)) : nullptr;
    }

    operator Func() const { return fun; }

private:
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
#endif










//--------------- implementation---------------------------------------------------
#ifdef ZEN_WIN
inline
std::string getResourceStream(const wchar_t* libraryName, size_t resourceId)
{
    if (HMODULE module = ::LoadLibrary(libraryName))
    {
        ZEN_ON_SCOPE_EXIT(::FreeLibrary(module));

        if (HRSRC res = ::FindResource(module, MAKEINTRESOURCE(resourceId), RT_RCDATA))
            if (HGLOBAL resHandle = ::LoadResource(module, res))
                if (const char* stream = static_cast<const char*>(::LockResource(resHandle)))
                    return std::string(stream, static_cast<size_t>(::SizeofResource(module, res))); //size is 0 on error
    }
    return std::string();
}
#endif
}

#endif //DLLLOADER_H_4239582598670968
