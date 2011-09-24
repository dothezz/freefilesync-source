// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) 2008-2011 ZenJu (zhnmju123 AT gmx.de)                    *
// **************************************************************************

#ifndef DIR_EXIST_HEADER_08173281673432158067342132467183267
#define DIR_EXIST_HEADER_08173281673432158067342132467183267

#include "../shared/check_exist.h"
#include "status_handler.h"
#include "../shared/file_error.h"
#include "../shared/i18n.h"

//dir existence checking may hang for non-existent network drives => run asynchronously and update UI!
namespace
{
using namespace zen; //restricted to unnamed namespace!

bool dirExistsUpdating(const Zstring& dirname, ProcessCallback& procCallback)
{
    using namespace util;

    std::wstring statusText = _("Searching for directory %x...");
    replace(statusText, L"%x", std::wstring(L"\"") + dirname + L"\"", false);
    procCallback.reportStatus(statusText);

    auto ft = dirExistsAsync(dirname);
    while (!ft.timed_wait(boost::posix_time::milliseconds(UI_UPDATE_INTERVAL)))
        procCallback.requestUiRefresh(); //may throw!
    return ft.get();
}
}

#endif //DIR_EXIST_HEADER_08173281673432158067342132467183267
