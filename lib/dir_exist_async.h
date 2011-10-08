// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) 2008-2011 ZenJu (zhnmju123 AT gmx.de)                    *
// **************************************************************************

#ifndef DIR_EXIST_HEADER_08173281673432158067342132467183267
#define DIR_EXIST_HEADER_08173281673432158067342132467183267

#include <zen/thread.h>
#include <zen/file_handling.h>
#include "status_handler.h"
#include <zen/file_error.h>
#include "resolve_path.h"

//dir existence checking may hang for non-existent network drives => run asynchronously and update UI!
namespace
{
bool dirExistsUpdating(const Zstring& dirname, bool allowUserInteraction, ProcessCallback& procCallback)
{
    using namespace zen;

    std::wstring statusText = _("Searching for directory %x...");
    replace(statusText, L"%x", std::wstring(L"\"") + dirname + L"\"", false);
    procCallback.reportStatus(statusText);

    auto ft = async([=]() -> bool
    {
#ifdef FFS_WIN
        //1. login to network share, if necessary
        loginNetworkShare(dirname, allowUserInteraction);
#endif
        //2. check dir existence
        return zen::dirExists(dirname);
    });
    while (!ft.timed_wait(boost::posix_time::milliseconds(UI_UPDATE_INTERVAL)))
        procCallback.requestUiRefresh(); //may throw!
    return ft.get();
}
}

#endif //DIR_EXIST_HEADER_08173281673432158067342132467183267
