// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) Zenju (zenju AT gmx DOT de) - All Rights Reserved        *
// **************************************************************************

#include "debug_new.h"
#include <string>
#include <sstream>
#include <cstdlib>   //malloc(), free()
#include "win.h"     //includes "windows.h"
#include "DbgHelp.h" //available for MSC only
#pragma comment(lib, "Dbghelp.lib")


namespace
{
LONG WINAPI writeDumpOnException(EXCEPTION_POINTERS* pExceptionInfo)
{
    HANDLE hFile = ::CreateFile(L"exception.dmp", GENERIC_READ | GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (hFile != INVALID_HANDLE_VALUE)
    {
        MINIDUMP_EXCEPTION_INFORMATION exInfo = {};
        exInfo.ThreadId          = ::GetCurrentThreadId();
        exInfo.ExceptionPointers = pExceptionInfo;
        exInfo.ClientPointers    = FALSE;

        MINIDUMP_EXCEPTION_INFORMATION* exceptParam = pExceptionInfo ? &exInfo : nullptr;

        /*bool rv = */
        ::MiniDumpWriteDump(::GetCurrentProcess(),   //__in  HANDLE hProcess,
                            ::GetCurrentProcessId(), //__in  DWORD ProcessId,
                            hFile,                   //__in  HANDLE hFile,
                            MiniDumpWithDataSegs,    //__in  MINIDUMP_TYPE DumpType,  ->Standard: MiniDumpNormal, Medium: MiniDumpWithDataSegs, Full: MiniDumpWithFullMemory
                            exceptParam,             //__in  PMINIDUMP_EXCEPTION_INFORMATION ExceptionParam,
                            nullptr,                 //__in  PMINIDUMP_USER_STREAM_INFORMATION UserStreamParam,
                            nullptr);                //__in  PMINIDUMP_CALLBACK_INFORMATION CallbackParam

        ::CloseHandle(hFile);
    }
    return EXCEPTION_EXECUTE_HANDLER;
}

//ensure that a dump-file is written for uncaught exceptions
struct Dummy { Dummy() { ::SetUnhandledExceptionFilter(writeDumpOnException); }} dummy;
}


void debug_tools::writeMinidump()
{
    //force exception to catch the state of this thread and hopefully get a valid call stack
    __try
    {
        ::RaiseException(EXCEPTION_BREAKPOINT, 0, 0, nullptr);
    }
    __except (writeDumpOnException(GetExceptionInformation()), EXCEPTION_CONTINUE_EXECUTION) {}
}


/*
No need to include the "operator new" declarations into every compilation unit:

[basic.stc.dynamic]
"A C++ program shall provide at most one definition of a replaceable allocation or deallocation function.
Any such function definition replaces the default version provided in the library (17.6.4.6).
The following allocation and deallocation functions (18.6) are implicitly declared in global scope in each translation unit of a program.
void* operator new(std::size_t);
void* operator new[](std::size_t);
void operator delete(void*);
void operator delete[](void*);"
*/

namespace
{
class BadAllocDetailed : public std::bad_alloc
{
public:
    explicit BadAllocDetailed(size_t allocSize)
    {
        errorMsg = "Memory allocation failed: ";
        errorMsg += numberToString(allocSize);
    }

    virtual const char* what() const throw()
    {
        return errorMsg.c_str();
    }

private:
    template <class T>
    static std::string numberToString(const T& number) //convert number to string the C++ way
    {
        std::ostringstream ss;
        ss << number;
        return ss.str();
    }

    std::string errorMsg;
};
}

void* operator new(size_t size)
{
    if (void* ptr = ::malloc(size))
        return ptr;

    debug_tools::writeMinidump();
    throw debug_tools::BadAllocDetailed(size);
}

void operator delete(void* ptr) { ::free(ptr); }

void* operator new[](size_t size) { return operator new(size); }
void operator delete[](void* ptr) { operator delete(ptr); }
