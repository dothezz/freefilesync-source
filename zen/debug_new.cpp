// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) Zenju (zenju AT gmx DOT de) - All Rights Reserved        *
// **************************************************************************

#include "debug_new.h"

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


void mem_check::writeMinidump()
{
    //force exception to catch the state of this thread and hopefully get a valid call stack
    __try
    {
        ::RaiseException(EXCEPTION_BREAKPOINT, 0, 0, nullptr);
    }
    __except (writeDumpOnException(GetExceptionInformation()), EXCEPTION_CONTINUE_EXECUTION) {}
}
