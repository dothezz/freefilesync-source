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
//- avoid checking duplicate entries by design: set<Zstring, LessFilename>
std::set<Zstring, LessFilename> getExistingDirsUpdating(const std::set<Zstring, LessFilename>& dirnames,
                                                        std::set<Zstring, LessFilename>& missing,
                                                        bool allowUserInteraction,
                                                        ProcessCallback& procCallback)
{
    using namespace zen;

    missing.clear();

    std::list<std::pair<Zstring, boost::unique_future<bool>>> futureInfo;
    for (const Zstring& dirname : dirnames)
        if (!dirname.empty())
            futureInfo.push_back(std::make_pair(dirname, async2<bool>([=]() -> bool
        {
#ifdef ZEN_WIN
            //1. login to network share, if necessary
            loginNetworkShare(dirname, allowUserInteraction);
#endif
            //2. check dir existence
            return dirExists(dirname);
        })));

    std::set<Zstring, LessFilename> output;
    //don't wait (almost) endlessly like win32 would on not existing network shares:
    const boost::system_time endTime = boost::get_system_time() + boost::posix_time::seconds(20); //consider CD-rom insert or hard disk spin up time from sleep

    for (auto& fi : futureInfo)
    {
        procCallback.reportStatus(replaceCpy(_("Searching for folder %x..."), L"%x", fmtFileName(fi.first), false)); //may throw!

        while (boost::get_system_time() < endTime &&
               !fi.second.timed_wait(boost::posix_time::milliseconds(UI_UPDATE_INTERVAL / 2)))
            procCallback.requestUiRefresh(); //may throw!

        if (fi.second.is_ready() && fi.second.get())
            output.insert(fi.first);
        else
            missing.insert(fi.first);
    }
    return output;
}
}

inline //also silences Clang "unused function" for compilation units depending from getExistingDirsUpdating() only
bool dirExistsUpdating(const Zstring& dirname, bool allowUserInteraction, ProcessCallback& procCallback)
{
    if (dirname.empty()) return false;
    std::set<Zstring, LessFilename> missing;
    std::set<Zstring, LessFilename> dirsEx = getExistingDirsUpdating({ dirname }, missing, allowUserInteraction, procCallback);
    assert(dirsEx.empty() != missing.empty());
    return dirsEx.find(dirname) != dirsEx.end();
}
}

#endif //DIR_EXIST_HEADER_08173281673432158067342132467183267
