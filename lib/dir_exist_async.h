// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) Zenju (zenju AT gmx DOT de) - All Rights Reserved        *
// **************************************************************************

#ifndef DIR_EXIST_HEADER_08173281673432158067342132467183267
#define DIR_EXIST_HEADER_08173281673432158067342132467183267

#include <zen/thread.h>
#include <zen/file_handling.h>
#include "process_callback.h"
#include <zen/file_error.h>
#include "resolve_path.h"

//dir existence checking may hang for non-existent network drives => run asynchronously and update UI!
namespace
{
bool dirExistsUpdating(const Zstring& dirname, bool allowUserInteraction, ProcessCallback& procCallback)
{
    using namespace zen;

    procCallback.reportStatus(replaceCpy(_("Searching for folder %x..."), L"%x", fmtFileName(dirname), false));

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
