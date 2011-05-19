#ifndef DIR_LOCK_H_INCLUDED
#define DIR_LOCK_H_INCLUDED

#include "../shared/zstring.h"
#include "../shared/file_error.h"
#include <boost/shared_ptr.hpp>


struct DirLockCallback //while waiting for the lock
{
    virtual ~DirLockCallback() {}
    virtual void requestUiRefresh() = 0;  //allowed to throw exceptions
    virtual void reportInfo(const Zstring& text) = 0;
};

/*
RAII structure to place a directory lock against other FFS processes:
        - recursive locking supported, even with alternate lockfile names, e.g. via symlinks, network mounts etc.
        - ownership shared between all object instances refering to a specific lock location(= UUID)
        - can be copied safely and efficiently! (ref-counting)
        - detects and resolves abandoned locks (instantly if lock is associated with local pc, else after 30 seconds)
        - temporary locks created during abandoned lock resolution keep "lockfilename"'s extension
        - race-free (Windows, almost on Linux(NFS))
*/
class DirLock
{
public:
    DirLock(const Zstring& lockfilename, DirLockCallback* callback = NULL); //throw (FileError), callback only used during construction

private:
    class LockAdmin;
    class SharedDirLock;
    boost::shared_ptr<SharedDirLock> sharedLock;
};

#endif // DIR_LOCK_H_INCLUDED
