#ifndef DLLLOADER_H_INCLUDED
#define DLLLOADER_H_INCLUDED

#include <string>

namespace Utility
{

//load function from a DLL library, e.g. from kernel32.dll
//NOTE: you're allowed to take a static reference to the return value to optimize performance! :)
template <typename FunctionType>
FunctionType loadDllFunction(const std::wstring& libraryName, const std::string& functionName);












//---------------Inline Implementation---------------------------------------------------
void* loadSymbol(const std::wstring& libraryName, const std::string& functionName);

template <typename FunctionType>
inline
FunctionType loadDllFunction(const std::wstring& libraryName, const std::string& functionName)
{
    return reinterpret_cast<FunctionType>(loadSymbol(libraryName, functionName));
}

#ifndef FFS_WIN
use in windows build only!
#endif

}

#endif // DLLLOADER_H_INCLUDED
