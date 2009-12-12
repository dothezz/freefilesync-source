#ifndef DLLLOADER_H_INCLUDED
#define DLLLOADER_H_INCLUDED

#include <string>

namespace Utility
{
    //load kernel dll functions
template <typename FunctionType>
FunctionType loadDllFunKernel(const std::string& functionName);













//---------------Inline Implementation---------------------------------------------------
void* loadSymbolKernel(const std::string& functionName);

template <typename FunctionType>
inline
FunctionType loadDllFunKernel(const std::string& functionName)
{
        return reinterpret_cast<FunctionType>(loadSymbolKernel(functionName));
}

#ifndef FFS_WIN
use in windows build only!
#endif

}

#endif // DLLLOADER_H_INCLUDED
