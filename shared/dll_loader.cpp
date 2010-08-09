// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) 2008-2010 ZenJu (zhnmju123 AT gmx.de)                    *
// **************************************************************************
//
#include "dll_loader.h"
#include <wx/msw/wrapwin.h> //includes "windows.h"
#include <map>
#include <cassert>

namespace
{
class DllHandler //dynamically load "kernel32.dll"
{
public:
    static DllHandler& getInstance()
    {
        static DllHandler instance;
        return instance;
    }

    HINSTANCE getHandle(const std::wstring& libraryName)
    {
        HandleMap::const_iterator foundEntry = handles.find(libraryName);
        if (foundEntry == handles.end())
        {
            HINSTANCE newHandle = ::LoadLibrary(libraryName.c_str());
            if (newHandle != NULL)
                handles.insert(std::make_pair(libraryName, newHandle));

            return newHandle;
        }
        else
            return foundEntry->second;
    }

private:
    DllHandler() {}

    ~DllHandler()
    {
        for (HandleMap::const_iterator i = handles.begin(); i != handles.end(); ++i)
            ::FreeLibrary(i->second);
    }

    typedef std::map<std::wstring, HINSTANCE> HandleMap;
    HandleMap handles; //only valid handles here!
};
}


void* util::loadSymbol(const std::wstring& libraryName, const std::string& functionName)
{
    const HINSTANCE libHandle = DllHandler::getInstance().getHandle(libraryName);

    if (libHandle != NULL)
        return reinterpret_cast<void*>(::GetProcAddress(libHandle, functionName.c_str()));
    else
        return NULL;
}
