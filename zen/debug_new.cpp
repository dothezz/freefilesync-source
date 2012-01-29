// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) ZenJu (zhnmju123 AT gmx DOT de) - All Rights Reserved    *
// **************************************************************************

#include "debug_new.h"

#include "win.h"     //includes "windows.h"
#include "DbgHelp.h" //available for MSC only
#pragma comment(lib, "Dbghelp.lib")


namespace
{
LONG WINAPI writeDumpOnException(EXCEPTION_POINTERS* pExceptionInfo)
{
    HANDLE hFile = ::CreateFile(L"exception.dmp", GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile != INVALID_HANDLE_VALUE)
    {
        MINIDUMP_EXCEPTION_INFORMATION exInfo = {};
        exInfo.ThreadId          = ::GetCurrentThreadId();
        exInfo.ExceptionPointers = pExceptionInfo;

        MINIDUMP_EXCEPTION_INFORMATION* exceptParam = pExceptionInfo ? &exInfo : NULL;

        /*bool rv = */
        ::MiniDumpWriteDump(::GetCurrentProcess(),   //__in  HANDLE hProcess,
                            ::GetCurrentProcessId(), //__in  DWORD ProcessId,
                            hFile,                   //__in  HANDLE hFile,
                            MiniDumpWithDataSegs,    //__in  MINIDUMP_TYPE DumpType,  ->Standard: MiniDumpNormal, Medium: MiniDumpWithDataSegs, Full: MiniDumpWithFullMemory
                            exceptParam,             //__in  PMINIDUMP_EXCEPTION_INFORMATION ExceptionParam,
                            NULL,                    //__in  PMINIDUMP_USER_STREAM_INFORMATION UserStreamParam,
                            NULL);                   //__in  PMINIDUMP_CALLBACK_INFORMATION CallbackParam

        ::CloseHandle(hFile);
    }
    return EXCEPTION_EXECUTE_HANDLER;
}

//ensure that a dump-file is written for uncaught exceptions
struct Dummy { Dummy() { ::SetUnhandledExceptionFilter(writeDumpOnException); }} dummy;
}


void mem_check::writeMinidump()
{
    writeDumpOnException(NULL);
}
