// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) 2008-2011 ZenJu (zhnmju123 AT gmx.de)                    *
// **************************************************************************
//
#include "check_exist.h"
#include "file_handling.h"
#include <memory>
#include "boost_thread_wrap.h" //include <boost/thread.hpp>


/*
#ifdef __MINGW32__
//oh well, nothing is for free...
//https://svn.boost.org/trac/boost/ticket/4258
extern "C" void tss_cleanup_implemented() {};
#endif
*/

namespace
{
typedef Zbase<Zchar, StorageDeepCopy> BasicString; //thread safe string class

template <bool (*fun)(const Zstring&)>
util::ResultExist checkExistence(const Zstring& objName, size_t timeout) //timeout in ms
{
    using namespace util;

    std::shared_ptr<bool> isExisting = std::make_shared<bool>(false); //no mutex required: accessed after thread has finished only!
    BasicString filename = objName.c_str(); //using thread safe string without ref-count!

    boost::thread worker([=]() { *isExisting = fun(filename.c_str()); }); //throw()

    if (worker.timed_join(boost::posix_time::milliseconds(timeout)))
        return *isExisting ? EXISTING_TRUE : EXISTING_FALSE;
    else
        return EXISTING_TIMEOUT;
    /*
    main/worker thread may access different shared_ptr instances safely (even though they have the same target!)
    http://www.boost.org/doc/libs/1_43_0/libs/smart_ptr/shared_ptr.htm?sess=8153b05b34d890e02d48730db1ff7ddc#ThreadSafety
    */
}
}


util::ResultExist util::fileExists(const Zstring& filename, size_t timeout) //timeout in ms
{
    assert(!filename.empty());
    return ::checkExistence<zen::fileExists>(filename, timeout);
}


util::ResultExist util::dirExists(const Zstring& dirname, size_t timeout) //timeout in ms
{
    assert(!dirname.empty());
    return ::checkExistence<zen::dirExists>(dirname, timeout);
}
