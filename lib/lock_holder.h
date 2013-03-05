#ifndef LOCK_HOLDER_H_INCLUDED
#define LOCK_HOLDER_H_INCLUDED

#include <map>
#include <zen/zstring.h>
#include <zen/stl_tools.h>
#include "dir_lock.h"
#include "status_handler.h"
#include "dir_exist_async.h"

namespace zen
{
const Zstring LOCK_FILE_ENDING = Zstr(".ffs_lock"); //intermediate locks created by DirLock use this extension, too!

//hold locks for a number of directories without blocking during lock creation
class LockHolder
{
public:
    LockHolder(const std::vector<Zstring>& dirnamesFmt, //resolved dirname ending with path separator
               ProcessCallback& procCallback,
               bool allowUserInteraction)
    {
        std::set<Zstring, LessFilename> existingDirs = getExistingDirsUpdating(dirnamesFmt, allowUserInteraction, procCallback);

        for (auto it = existingDirs.begin(); it != existingDirs.end(); ++it)
        {
            const Zstring& dirnameFmt = *it;

            assert(endsWith(dirnameFmt, FILE_NAME_SEPARATOR)); //this is really the contract, formatting does other things as well, e.g. macro substitution

            class WaitOnLockHandler : public DirLockCallback
            {
            public:
                WaitOnLockHandler(ProcessCallback& pc) : pc_(pc) {}
                virtual void requestUiRefresh() { pc_.requestUiRefresh(); }  //allowed to throw exceptions
                virtual void reportStatus(const std::wstring& text) { pc_.reportStatus(text); }
            private:
                ProcessCallback& pc_;
            } callback(procCallback);

            try
            {
                //lock file creation is synchronous and may block noticeably for very slow devices (usb sticks, mapped cloud storages)
                lockHolder.push_back(DirLock(dirnameFmt + Zstr("sync") + LOCK_FILE_ENDING, &callback)); //throw FileError
            }
            catch (const FileError& e)
            {
                bool dummy = false; //this warning shall not be shown but logged only
                procCallback.reportWarning(e.toString(), dummy); //may throw!
            }
        }
    }

private:
    std::vector<DirLock> lockHolder;
};

}


#endif // LOCK_HOLDER_H_INCLUDED
