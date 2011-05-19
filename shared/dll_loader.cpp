// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) 2008-2011 ZenJu (zhnmju123 AT gmx.de)                    *
// **************************************************************************
//
#include "dll_loader.h"
#include <map>

namespace
{
class DllHandler
{
public:
    static DllHandler& getInstance()
    {
        static DllHandler instance;
        return instance;
    }

    HMODULE getHandle(const std::wstring& libraryName)
    {
        HandleMap::const_iterator foundEntry = handles.find(libraryName);
        if (foundEntry == handles.end())
        {
            if (libraryName.empty())
                return ::GetModuleHandle(NULL); //return handle to calling executable

            HMODULE newHandle = ::LoadLibrary(libraryName.c_str());
            if (newHandle != NULL)
                handles.insert(std::make_pair(libraryName, newHandle));

            return newHandle;
        }
        else
            return foundEntry->second;
    }

private:
    DllHandler() {}
    DllHandler(const DllHandler&);
    DllHandler& operator=(const DllHandler&);

    ~DllHandler()
    {
        for (HandleMap::const_iterator i = handles.begin(); i != handles.end(); ++i)
            ::FreeLibrary(i->second);
    }

    typedef std::map<std::wstring, HMODULE> HandleMap;
    HandleMap handles; //only valid handles here!
};
}


FARPROC util::loadSymbol(const std::wstring& libraryName, const std::string& functionName)
{
    const HMODULE libHandle = DllHandler::getInstance().getHandle(libraryName);

    if (libHandle != NULL)
        return ::GetProcAddress(libHandle, functionName.c_str());
    else
        return NULL;
}


std::string util::getResourceStream(const std::wstring& libraryName, size_t resourceId)
{
    std::string output;
    const HMODULE module = DllHandler::getInstance().getHandle(libraryName);
    if (module)
    {
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
