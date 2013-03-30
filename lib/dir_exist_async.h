// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) Zenju (zenju AT gmx DOT de) - All Rights Reserved        *
// **************************************************************************

#ifndef DIR_EXIST_HEADER_08173281673432158067342132467183267
#define DIR_EXIST_HEADER_08173281673432158067342132467183267

#include <zen/thread.h>
#include <zen/file_handling.h>
#include <zen/file_error.h>
#include "process_callback.h"
#include "resolve_path.h"

namespace zen
{
namespace
{
//directory existence checking may hang for non-existent network drives => run asynchronously and update UI!
//- check existence of all directories in parallel! (avoid adding up search times if multiple network drives are not reachable)
//- add reasonable time-out time!
std::set<Zstring, LessFilename> getExistingDirsUpdating(const std::vector<Zstring>& dirnames, bool allowUserInteraction, ProcessCallback& procCallback)
{
    using namespace zen;

    std::list<boost::unique_future<bool>> dirEx;

    std::for_each(dirnames.begin(), dirnames.end(),
                  [&](const Zstring& dirname)
    {
        dirEx.push_back(zen::async2<bool>([=]() -> bool
        {
            if (dirname.empty())
                return false;
#ifdef FFS_WIN
            //1. login to network share, if necessary
            loginNetworkShare(dirname, allowUserInteraction);
#endif
            //2. check dir existence
            return dirExists(dirname);
        }));
    });

    std::set<Zstring, LessFilename> output;
    const boost::system_time endTime = boost::get_system_time() + boost::posix_time::seconds(10); //10 sec should be enough even if Win32 waits much longer

    auto itDirname = dirnames.begin();
    for (auto it = dirEx.begin(); it != dirEx.end(); ++it, ++itDirname)
    {
        procCallback.reportStatus(replaceCpy(_("Searching for folder %x..."), L"%x", fmtFileName(*itDirname), false)); //may throw!

        while (boost::get_system_time() < endTime &&
               !it->timed_wait(boost::posix_time::milliseconds(UI_UPDATE_INTERVAL / 2)))
            procCallback.requestUiRefresh(); //may throw!

        if (it->is_ready() && it->get())
            output.insert(*itDirname);
    }
    return output;
}
}

inline //also silences Clang "unused function" for compilation units depending from getExistingDirsUpdating() only
bool dirExistsUpdating(const Zstring& dirname, bool allowUserInteraction, ProcessCallback& procCallback)
{
    std::vector<Zstring> dirnames;
    dirnames.push_back(dirname);
    std::set<Zstring, LessFilename> dirsEx = getExistingDirsUpdating(dirnames, allowUserInteraction, procCallback);
    return dirsEx.find(dirname) != dirsEx.end();
}
}

#endif //DIR_EXIST_HEADER_08173281673432158067342132467183267
