#ifndef LOCK_HOLDER_H_489572039485723453425
#define LOCK_HOLDER_H_489572039485723453425

#include <set>
#include <zen/zstring.h>
#include <zen/stl_tools.h>
#include "dir_lock.h"
#include "status_handler.h"
//#include "dir_exist_async.h"

namespace zen
{
const Zstring LOCK_FILE_ENDING = Zstr(".ffs_lock"); //intermediate locks created by DirLock use this extension, too!

//hold locks for a number of directories without blocking during lock creation
//call after having checked directory existence!
class LockHolder
{
public:
    LockHolder(const std::set<Zstring, LessFilename>& dirnamesExisting, //resolved dirname ending with path separator
               bool& warningDirectoryLockFailed,
               ProcessCallback& procCallback)
    {
        for (const Zstring& dirnameFmt : dirnamesExisting)
        {
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
                const std::wstring msg = replaceCpy(_("Cannot set directory lock for %x."), L"%x", fmtFileName(dirnameFmt)) + L"\n\n" + e.toString();
                procCallback.reportWarning(msg, warningDirectoryLockFailed); //may throw!
            }
        }
    }

private:
    std::vector<DirLock> lockHolder;
};
}

#endif //LOCK_HOLDER_H_489572039485723453425
