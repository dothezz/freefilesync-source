// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) 2008-2011 ZenJu (zhnmju123 AT gmx.de)                    *
// **************************************************************************

#ifndef CHECKEXIST_H_INCLUDED
#define CHECKEXIST_H_INCLUDED

#include "zstring.h"
#include "boost_thread_wrap.h" //include <boost/thread.hpp>
#include "file_handling.h"


namespace util
{
//check for file or folder existence asynchronously
boost::unique_future<bool> somethingExistsAsync(const Zstring& somename);
boost::unique_future<bool>      fileExistsAsync(const Zstring& filename);
boost::unique_future<bool>       dirExistsAsync(const Zstring&  dirname);

//some syntactic sugar:
enum ResultExist
{
    EXISTING_TRUE,
    EXISTING_FALSE,
    EXISTING_NOT_READY
};

ResultExist somethingExists(const Zstring& somename, size_t timeout);
ResultExist      fileExists(const Zstring& filename, size_t timeout);
ResultExist       dirExists(const Zstring&  dirname, size_t timeout);












//################## implementation ##########################
template <bool (*fun)(const Zstring&)>
boost::unique_future<bool> objExistsAsync(const Zstring& objname)
{
    //thread safety: make it a pure value type for use in the thread!
    const Zstring objnameVal = objname; //atomic ref-count => binary value-type semantics!
    boost::packaged_task<bool> pt([ = ] { return (*fun)(objnameVal); });
    auto fut = pt.get_future();
    boost::thread(std::move(pt));
    return std::move(fut);
}


inline
boost::unique_future<bool> somethingExistsAsync(const Zstring& somename) { return objExistsAsync<&zen::somethingExists>(somename); }

inline
boost::unique_future<bool> fileExistsAsync(const Zstring& filename) { return objExistsAsync<&zen::fileExists>(filename); }

inline
boost::unique_future<bool>  dirExistsAsync(const Zstring& dirname) { return objExistsAsync<&zen::dirExists>(dirname); }


template <bool (*fun)(const Zstring&)> inline
ResultExist objExists(const Zstring& objname, size_t timeout)
{
    auto ft = objExistsAsync<fun>(objname);
    if (!ft.timed_wait(boost::posix_time::milliseconds(timeout)))
        return EXISTING_NOT_READY;
    return ft.get() ? EXISTING_TRUE : EXISTING_FALSE;
}


inline
ResultExist somethingExists(const Zstring& somename, size_t timeout) { return objExists<&zen::somethingExists>(somename, timeout); }

inline
ResultExist fileExists(const Zstring& filename, size_t timeout) { return objExists<&zen::fileExists>(filename, timeout); }

inline
ResultExist dirExists(const Zstring& dirname, size_t timeout) { return objExists<&zen::dirExists>(dirname, timeout); }
}

#endif // CHECKEXIST_H_INCLUDED
