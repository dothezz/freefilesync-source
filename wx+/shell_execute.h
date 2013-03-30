// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) Zenju (zenju AT gmx DOT de) - All Rights Reserved        *
// **************************************************************************

#ifndef EXECUTE_HEADER_23482134578134134
#define EXECUTE_HEADER_23482134578134134

#include <zen/zstring.h>
#include <zen/scope_guard.h>
#include <zen/i18n.h>
#include <zen/utf.h>
#include <wx/msgdlg.h>

#ifdef FFS_WIN
#include <zen/last_error.h>
//#include <zen/string_tools.h>
#include <zen/win.h> //includes "windows.h"

#elif defined FFS_LINUX || defined FFS_MAC
#include <zen/thread.h>
#include <stdlib.h> //::system()
//#include <wx/utils.h>
//#include <wx/log.h>
#endif


namespace zen
{
//launch commandline and report errors via popup dialog
//windows: COM needs to be initialized before calling this function!
enum ExecutionType
{
    EXEC_TYPE_SYNC,
    EXEC_TYPE_ASYNC
};

namespace
{
void shellExecute(const Zstring& command, ExecutionType type = EXEC_TYPE_ASYNC)
{
#ifdef FFS_WIN
    //parse commandline
    std::vector<std::wstring> argv;
    int argc = 0;
    if (LPWSTR* tmp = ::CommandLineToArgvW(command.c_str(), &argc))
    {
        std::copy(tmp, tmp + argc, std::back_inserter(argv));
        ::LocalFree(tmp);
    }

    std::wstring filename;
    std::wstring arguments;
    if (!argv.empty())
    {
        filename = argv[0];
        for (auto iter = argv.begin() + 1; iter != argv.end(); ++iter)
            arguments += (iter != argv.begin() ? L" " : L"") +
                         (iter->empty() || std::any_of(iter->begin(), iter->end(), &isWhiteSpace<wchar_t>) ? L"\"" + *iter + L"\"" : *iter);
    }

    SHELLEXECUTEINFO execInfo = {};
    execInfo.cbSize = sizeof(execInfo);

    //SEE_MASK_NOASYNC is equal to SEE_MASK_FLAG_DDEWAIT, but former is defined not before Win SDK 6.0
    execInfo.fMask        = type == EXEC_TYPE_SYNC ? (SEE_MASK_NOCLOSEPROCESS | SEE_MASK_FLAG_DDEWAIT) : 0; //don't use SEE_MASK_ASYNCOK -> returns successful despite errors!
    execInfo.fMask       |= SEE_MASK_UNICODE | SEE_MASK_FLAG_NO_UI; //::ShellExecuteEx() shows a non-blocking pop-up dialog on errors -> we want a blocking one
    execInfo.lpVerb       = nullptr;
    execInfo.lpFile       = filename.c_str();
    execInfo.lpParameters = arguments.c_str();
    execInfo.nShow        = SW_SHOWNORMAL;

    if (!::ShellExecuteEx(&execInfo)) //__inout  LPSHELLEXECUTEINFO lpExecInfo
    {
        wxString cmdFmt = L"File: " + filename + L"\nArg: " + arguments;
        wxMessageBox(_("Invalid command line:") + L"\n" + cmdFmt + L"\n\n" + getLastErrorFormatted());
        return;
    }

    if (execInfo.hProcess)
    {
        ZEN_ON_SCOPE_EXIT(::CloseHandle(execInfo.hProcess));

        if (type == EXEC_TYPE_SYNC)
            ::WaitForSingleObject(execInfo.hProcess, INFINITE);
    }

#elif defined FFS_LINUX || defined FFS_MAC
    /*
    we cannot use wxExecute due to various issues:
    - screws up encoding on OS X for non-ASCII characters
    - does not provide any reasonable error information
    - uses a zero-sized dummy window as a hack to keep focus which leaves a useless empty icon in ALT-TAB list
    */

    if (type == EXEC_TYPE_SYNC)
    {
        //Posix::system - execute a shell command
        int rv = ::system(command.c_str()); //do NOT use std::system as its documentation says nothing about "WEXITSTATUS(rv)", ect...
        if (rv == -1 || WEXITSTATUS(rv) == 127) //http://linux.die.net/man/3/system    "In case /bin/sh could not be executed, the exit status will be that of a command that does exit(127)"
            wxMessageBox(_("Invalid command line:") + L"\n" + utfCvrtTo<wxString>(command));
    }
    else
        async([=] { /*int rv = */ ::system(command.c_str()); });
    //unfortunately we are not allowed to show a wxMessageBox from a worker thread
#endif
}
}
}

#endif //EXECUTE_HEADER_23482134578134134
