#include "check_exist.h"
#include "file_handling.h"
#include <boost/thread.hpp>
#include <boost/shared_ptr.hpp>


#ifdef __MINGW32__
//oh well, nothing is for free...
//https://svn.boost.org/trac/boost/ticket/4258
#warning fix this issue at some time...
extern "C" void tss_cleanup_implemented() {}
#endif


namespace
{
template <bool (*testExist)(const Zstring&)>
class ExistenceChecker
{
public:
    ExistenceChecker(const Zstring& filename, const boost::shared_ptr<bool>& isExisting) :
        filename_(filename.c_str()), //deep copy: worker thread may run longer than main! avoid shared data
        isExisting_(isExisting) {}   //not accessed during thread run

    void operator()()
    {
        *isExisting_ = testExist(filename_); //throw()
    }

private:
    const Zstring filename_; //no reference, lifetime not known
    boost::shared_ptr<bool> isExisting_;
};


template <bool (*fun)(const Zstring&)>
util::ResultExist checkExistence(const Zstring& objName, size_t timeout) //timeout in ms
{
    using namespace util;

    boost::shared_ptr<bool> isExisting(new bool(false));

    ExistenceChecker<fun> task(objName, isExisting);
    boost::thread worker(task);

    if (worker.timed_join(boost::posix_time::milliseconds(timeout)))
        return *isExisting ? EXISTING_TRUE : EXISTING_FALSE;
    else
        return EXISTING_TIMEOUT;
    /*
    main/worker thread may access different shared_ptr instances safely (even though they have the same target!)
    http://www.boost.org/doc/libs/1_43_0/libs/smart_ptr/shared_ptr.htm?sess=8153b05b34d890e02d48730db1ff7ddc#ThreadSafety
    */

#ifndef _MSC_VER
#warning migrate this at some time...
#endif
    /*
	unfortunately packaged_task/future is not mature enough to be used...
    boost::packaged_task<bool> pt(boost::bind(fun, objName.c_str())); //attention: Zstring is not thread-safe => make deep copy
    boost::unique_future<bool> fut = pt.get_future();

    boost::thread worker(boost::move(pt)); //launch task on a thread

    if (fut.timed_wait(boost::posix_time::milliseconds(timeout)))
        return fut.get() ? EXISTING_TRUE : EXISTING_FALSE;
    else
        return EXISTING_TIMEOUT;
    */
}
}


util::ResultExist util::fileExists(const Zstring& filename, size_t timeout) //timeout in ms
{
    return ::checkExistence<ffs3::fileExists>(filename, timeout);
}


util::ResultExist util::dirExists(const Zstring& dirname, size_t timeout) //timeout in ms
{
    return ::checkExistence<ffs3::dirExists>(dirname, timeout);
}