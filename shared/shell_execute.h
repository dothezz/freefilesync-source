// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) 2008-2011 ZenJu (zhnmju123 AT gmx.de)                    *
// **************************************************************************

#ifndef EXECUTE_HEADER_23482134578134134
#define EXECUTE_HEADER_23482134578134134

#include <wx/msgdlg.h>

#ifdef FFS_WIN
#include "last_error.h"
#include "string_tools.h"
#include "i18n.h"
#include <wx/msw/wrapwin.h> //includes "windows.h"

#elif defined FFS_LINUX
#include <stdlib.h>
#include <wx/utils.h>
#endif


namespace zen
{
//launch commandline and report errors via popup dialog
//windows: COM needs to be initialized before calling this function!
namespace
{
enum ExecutionType
{
    EXEC_TYPE_SYNC,
    EXEC_TYPE_ASYNC
};

void shellExecute(const wxString& command, ExecutionType type = EXEC_TYPE_ASYNC)
{
#ifdef FFS_WIN
    //parse commandline
    std::vector<std::wstring> argv;
    {
        int argc = 0;
        LPWSTR* tmp = ::CommandLineToArgvW(command.c_str(), &argc);
        for (int i = 0; i < argc; ++i)
            argv.push_back(tmp[i]);
        ::LocalFree(tmp);
    }

    wxString filename;
    wxString arguments;
    if (!argv.empty())
    {
        filename = argv[0];
        for (std::vector<std::wstring>::const_iterator i = argv.begin() + 1; i != argv.end(); ++i)
            arguments += (i != argv.begin() ? L" " : L"") +
                         (i->empty() || std::find_if(i->begin(), i->end(), &cStringIsWhiteSpace<wchar_t>) != i->end() ? L"\"" + *i + L"\"" : *i);
    }

    SHELLEXECUTEINFO execInfo = {};
    execInfo.cbSize       = sizeof(execInfo);

    //SEE_MASK_NOASYNC is equal to SEE_MASK_FLAG_DDEWAIT, but former is defined not before Win SDK 6.0
    execInfo.fMask        = type == EXEC_TYPE_SYNC ? (SEE_MASK_NOCLOSEPROCESS | SEE_MASK_FLAG_DDEWAIT) : 0; //don't use SEE_MASK_ASYNCOK -> returns successful despite errors!
    execInfo.fMask       |= SEE_MASK_UNICODE | SEE_MASK_FLAG_NO_UI; //::ShellExecuteEx() shows a non-blocking pop-up dialog on errors -> we want a blocking one
    execInfo.lpVerb       = L"open";
    execInfo.lpFile       = filename.c_str();
    execInfo.lpParameters = arguments.c_str();
    execInfo.nShow        = SW_SHOWNORMAL;

    if (!::ShellExecuteEx(&execInfo)) //__inout  LPSHELLEXECUTEINFO lpExecInfo
    {
        wxString errorMsg = _("Invalid commandline: %x");
        wxString cmdFmt = wxString(L"\nFile: ") + filename + L"\nArg: " + arguments;

        errorMsg.Replace(L"%x", cmdFmt);
        wxMessageBox(errorMsg + L"\n\n" + getLastErrorFormatted());
        return;
    }

    if (type == EXEC_TYPE_SYNC)
    {
        if (execInfo.hProcess != 0)
        {
            ::WaitForSingleObject(execInfo.hProcess, INFINITE);
            ::CloseHandle(execInfo.hProcess);
        }
    }

#elif defined FFS_LINUX
    if (type == EXEC_TYPE_SYNC)
    {
        int rv = ::system(utf8CvrtTo<std::string>(command).c_str()); //do NOT use std::system as its documentation says nothing about "WEXITSTATUS(rv)", ect...
        if (rv == -1 || WEXITSTATUS(rv) == 127) //http://linux.die.net/man/3/system    "In case /bin/sh could not be executed, the exit status will be that of a command that does exit(127)"
        {
            wxString errorMsg = _("Invalid commandline: %x");
            replace(errorMsg, L"%x", L"\n" + command);
            wxMessageBox(errorMsg);
            return;
        }
    }
    else
    {
        // ! unfortunately it seems there is no way on Linux to get a failure notification for calling an invalid commandline asynchronously !

        //by default wxExecute uses a zero sized dummy window as a hack to keep focus which leaves a useless empty icon in ALT-TAB list
        //=> use wxEXEC_NODISABLE and roll our own window disabler!                   (see comment in  app.cpp: void *wxGUIAppTraits::BeforeChildWaitLoop())
        wxWindowDisabler dummy; //disables all top level windows
        wxExecute(command, wxEXEC_ASYNC | wxEXEC_NODISABLE);
    }
#endif
}
}
}

#endif //EXECUTE_HEADER_23482134578134134
