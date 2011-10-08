// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) 2008-2011 ZenJu (zhnmju123 AT gmx.de)                    *
// **************************************************************************

#ifndef BOOST_THREAD_WRAP_H
#define BOOST_THREAD_WRAP_H

//temporary solution until C++11 thread becomes fully available

#ifdef __MINGW32__
#pragma GCC diagnostic push

#pragma GCC diagnostic ignored "-Wswitch-enum"
#pragma GCC diagnostic ignored "-Wstrict-aliasing"
//#pragma GCC diagnostic ignored "-Wno-attributes"
//#pragma GCC diagnostic ignored "-Wredundant-decls"
//#pragma GCC diagnostic ignored "-Wcast-align"
//#pragma GCC diagnostic ignored "-Wunused-value"
#endif

#include <boost/thread.hpp>

#ifdef __MINGW32__
#pragma GCC diagnostic pop
#endif

namespace zen
{
//until std::async is available:
/*
Example:
        Zstring dirname = ...
        auto ft = zen::async([=](){ return zen::dirExists(dirname); });
        if (ft.timed_wait(boost::posix_time::milliseconds(200)) && ft.get())
            //dir exising
*/
template <class Function>
auto async(Function fun) -> boost::unique_future<decltype(fun())>;

template<class InputIterator, class Duration>
void wait_for_all_timed(InputIterator first, InputIterator last, const Duration& wait_duration);




















//###################### implementation ######################
template <class T, class Function> inline
auto async2(Function fun) -> boost::unique_future<T> //workaround VS2010 bug: bool (*fun)();  decltype(fun()) == int!
{
    boost::packaged_task<T> pt([=] { return fun(); });
    auto fut = pt.get_future();
    boost::thread(std::move(pt));
    return std::move(fut);
}


template <class Function> inline auto async(Function fun) -> boost::unique_future<decltype(fun())> { return async2<decltype(fun())>(fun); }


template<class InputIterator, class Duration> inline
void wait_for_all_timed(InputIterator first, InputIterator last, const Duration& wait_duration)
{
    const boost::system_time endTime = boost::get_system_time() + wait_duration;
    while (first != last)
    {
        first->timed_wait_until(endTime);
        if (boost::get_system_time() >= endTime)
            return;
        ++first;
    }
}
}

#endif //BOOST_THREAD_WRAP_H
