#include "dllLoader.h"
#include <wx/msw/wrapwin.h> //includes "windows.h"

namespace
{
class KernelDllHandler //dynamically load "kernel32.dll"
{
public:
    static const KernelDllHandler& getInstance()
    {
        static KernelDllHandler instance;
        return instance;
    }

    HINSTANCE getHandle() const
    {
        return hKernel;
    }

private:
    KernelDllHandler() :
        hKernel(NULL)
    {
        //get a handle to the DLL module containing required functionality
        hKernel = ::LoadLibrary(L"kernel32.dll");
    }

    ~KernelDllHandler()
    {
        if (hKernel) ::FreeLibrary(hKernel);
    }

    HINSTANCE hKernel;
};
}


void* Utility::loadSymbolKernel(const std::string& functionName)
{
    if (KernelDllHandler::getInstance().getHandle() != NULL)
        return reinterpret_cast<void*>(::GetProcAddress(KernelDllHandler::getInstance().getHandle(), functionName.c_str()));
    else
        return NULL;
}
