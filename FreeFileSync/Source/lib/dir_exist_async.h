// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl-3.0        *
// * Copyright (C) Zenju (zenju AT gmx DOT de) - All Rights Reserved        *
// **************************************************************************

#ifndef DIR_EXIST_HEADER_08173281673432158067342132467183267
#define DIR_EXIST_HEADER_08173281673432158067342132467183267

#include <zen/thread.h>
#include <zen/file_error.h>
#include "../fs/abstract.h"
#include "../process_callback.h"

namespace zen
{
namespace
{
//directory existence checking may hang for non-existent network drives => run asynchronously and update UI!
//- check existence of all directories in parallel! (avoid adding up search times if multiple network drives are not reachable)
//- add reasonable time-out time!
//- avoid checking duplicate entries by design: std::set
struct DirectoryStatus
{
    std::set<const ABF*, ABF::LessItemPath> existingBaseFolder;
    std::set<const ABF*, ABF::LessItemPath> missingBaseFolder;
    std::map<const ABF*, FileError, ABF::LessItemPath> failedChecks;
};


struct DirCheckResult
{
    bool exists;
    std::unique_ptr<FileError> error;

    DirCheckResult(bool existsIn, std::unique_ptr<FileError>&& errorIn) : exists(existsIn), error(std::move(errorIn)) {}

    DirCheckResult           (DirCheckResult&& tmp) : exists(tmp.exists), error(std::move(tmp.error)) {} //= default with C++14
    DirCheckResult& operator=(DirCheckResult&& tmp)  //= default with C++14
    {
        exists = tmp.exists;
        error = std::move(tmp.error);
        return *this;
    }
};


DirectoryStatus checkFolderExistenceUpdating(const std::set<const ABF*, ABF::LessItemPath>& baseFolders, bool allowUserInteraction, ProcessCallback& procCallback)
{
    using namespace zen;

    DirectoryStatus output;

    std::list<std::pair<const ABF*, boost::unique_future<DirCheckResult>>> futureInfo;

    for (const ABF* baseFolder : baseFolders)
        if (!baseFolder->emptyBaseFolderPath()) //skip empty dirs
        {
            AbstractPathRef folderPath = baseFolder->getAbstractPath(Zstring());

            std::function<void()> connectFolder /*throw FileError*/ = baseFolder->getAsyncConnectFolder(allowUserInteraction); //noexcept
            std::function<bool()> dirExists     /*throw FileError*/ = ABF::getAsyncCheckDirExists(folderPath); //noexcept

            futureInfo.emplace_back(baseFolder, runAsync([connectFolder, dirExists]
            {
                try
                {
                    //1. login to network share, open FTP connection, ect.
                    if (connectFolder)
                        connectFolder(); //throw FileError

                    //2. check dir existence
                    return DirCheckResult(dirExists(), nullptr); //throw FileError
                }
                catch (const FileError& e)
                {
                    return DirCheckResult(false, make_unique<FileError>(e));
                }
            }));
        }

    //don't wait (almost) endlessly like win32 would on non-existing network shares:
    boost::chrono::steady_clock::time_point endTime = boost::chrono::steady_clock::now() + boost::chrono::seconds(20); //consider CD-rom insert or hard disk spin up time from sleep

    for (auto& fi : futureInfo)
    {
        const std::wstring& displayPathFmt = fmtPath(ABF::getDisplayPath(fi.first->getAbstractPath(Zstring())));

        procCallback.reportStatus(replaceCpy(_("Searching for folder %x..."), L"%x", displayPathFmt)); //may throw!

        while (boost::chrono::steady_clock::now() < endTime &&
               fi.second.wait_for(boost::chrono::milliseconds(UI_UPDATE_INTERVAL / 2)) != boost::future_status::ready)
            procCallback.requestUiRefresh(); //may throw!

        if (fi.second.is_ready())
        {
            const DirCheckResult result = fi.second.get(); //call future::get() only *once*! otherwise: undefined behavior!
            if (result.error)
                output.failedChecks.emplace(fi.first, *result.error);
            else if (result.exists)
                output.existingBaseFolder.insert(fi.first);
            else
                output.missingBaseFolder.insert(fi.first);
        }
        else
            output.failedChecks.emplace(fi.first, FileError(replaceCpy(_("Time out while searching for folder %x."), L"%x", displayPathFmt)));
    }

    return output;
}
}

inline //also silences Clang "unused function" for compilation units depending from getExistingDirsUpdating() only
bool folderExistsUpdating(const ABF& baseFolder, bool allowUserInteraction, ProcessCallback& procCallback)
{
    std::set<const ABF*, ABF::LessItemPath> baseFolders{ &baseFolder };
    const DirectoryStatus status = checkFolderExistenceUpdating(baseFolders, allowUserInteraction, procCallback);
    return status.existingBaseFolder.find(&baseFolder) != status.existingBaseFolder.end();
}
}

#endif //DIR_EXIST_HEADER_08173281673432158067342132467183267
