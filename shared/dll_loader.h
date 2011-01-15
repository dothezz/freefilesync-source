// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) 2008-2011 ZenJu (zhnmju123 AT gmx.de)                    *
// **************************************************************************
//
#ifndef DLLLOADER_H_INCLUDED
#define DLLLOADER_H_INCLUDED

#include <string>
#include <wx/msw/wrapwin.h> //includes "windows.h"


namespace util
{

/*
load function from a DLL library, e.g. from kernel32.dll
NOTE: you're allowed to take a static reference to the return value to optimize performance! :)
*/
template <typename FunctionType>
FunctionType getDllFun(const std::wstring& libraryName, const std::string& functionName);

/*
extract binary resources from .exe/.dll:

-- resource.h --
#define MY_BINARY_RESOURCE 1337

-- resource.rc --
MY_BINARY_RESOURCE RCDATA "filename.dat"
*/
std::string getResourceStream(const std::wstring& libraryName, size_t resourceId);










//---------------Inline Implementation---------------------------------------------------
FARPROC loadSymbol(const std::wstring& libraryName, const std::string& functionName);

template <typename FunctionType>
inline
FunctionType getDllFun(const std::wstring& libraryName, const std::string& functionName)
{
    return reinterpret_cast<FunctionType>(loadSymbol(libraryName, functionName));
}
}

#endif // DLLLOADER_H_INCLUDED
