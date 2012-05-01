// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) ZenJu (zhnmju123 AT gmx DOT de) - All Rights Reserved    *
// **************************************************************************

#ifndef BOOST_THREAD_WRAP_H
#define BOOST_THREAD_WRAP_H

//temporary solution until C++11 thread becomes fully available
#include <vector>
#include <memory>

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

//wait for all with a time limit: return true if *all* results are available!
template<class InputIterator, class Duration>
bool wait_for_all_timed(InputIterator first, InputIterator last, const Duration& wait_duration);

//wait until first job is successful or all failed
template <class T>
class RunUntilFirstHit
{
public:
    RunUntilFirstHit();

    template <class Fun>
    void addJob(Fun f); //f must return a std::unique_ptr<T> containing a value if successful

    template <class Duration>
    bool timedWait(const Duration& duration) const; //true: "get()" is ready, false: time elapsed

    //return first value or none if all jobs failed; blocks until result is ready!
    std::unique_ptr<T> get() const; //must be called only once!

private:
    class AsyncResult;
    std::vector<boost::thread> workload;
    std::shared_ptr<AsyncResult> result;
};
















//###################### implementation ######################
template <class T, class Function> inline
auto async2(Function fun) -> boost::unique_future<T> //workaround VS2010 bug: bool (*fun)();  decltype(fun()) == int!
{
    boost::packaged_task<T> pt([=] { return fun(); });
    auto fut = pt.get_future();
    boost::thread(std::move(pt));
    return std::move(fut);
}


template <class Function> inline
auto async(Function fun) -> boost::unique_future<decltype(fun())> { return async2<decltype(fun())>(fun); }


template<class InputIterator, class Duration> inline
bool wait_for_all_timed(InputIterator first, InputIterator last, const Duration& wait_duration)
{
    const boost::system_time endTime = boost::get_system_time() + wait_duration;
    while (first != last)
    {
        if (!first->timed_wait_until(endTime))
            return false; //time elapsed
        ++first;
    }
    return true;
}


template <class T>
class RunUntilFirstHit<T>::AsyncResult
{
public:
    AsyncResult() :
#ifndef NDEBUG
        returnedResult(false),
#endif
        jobsFinished(0) {}

    //context: worker threads
    void reportFinished(std::unique_ptr<T>&& result)
    {
        {
            boost::unique_lock<boost::mutex> dummy(lockResult);
            ++jobsFinished;
            if (!result_)
                result_ = std::move(result);
        }
        conditionJobDone.notify_one();
        //condition handling, see: http://www.boost.org/doc/libs/1_43_0/doc/html/thread/synchronization.html#thread.synchronization.condvar_ref
    }

    //context: main thread
    template <class Duration>
    bool waitForResult(size_t jobsTotal, const Duration& duration)
    {
        boost::unique_lock<boost::mutex> dummy(lockResult);
        return conditionJobDone.timed_wait(dummy, duration, [&] { return this->jobDone(jobsTotal); });
        //use timed_wait predicate if exitting before condition is reached: http://www.boost.org/doc/libs/1_49_0/doc/html/thread/synchronization.html#thread.synchronization.condvar_ref.condition_variable.timed_wait_rel
    }

    std::unique_ptr<T> getResult(size_t jobsTotal)
    {
        boost::unique_lock<boost::mutex> dummy(lockResult);

        while (!jobDone(jobsTotal))
            conditionJobDone.timed_wait(dummy, boost::posix_time::milliseconds(50)); //interruption point!

#ifndef NDEBUG
        assert(!returnedResult);
        returnedResult = true;
#endif
        return std::move(result_);
    }

private:
    bool jobDone(size_t jobsTotal) const { return result_ || (jobsFinished >= jobsTotal); } //call while locked!
#ifndef NDEBUG
    bool returnedResult;
#endif

    boost::mutex lockResult;
    size_t jobsFinished;        //
    std::unique_ptr<T> result_; //our condition is: "have result" or "jobsFinished == jobsTotal"
    boost::condition_variable conditionJobDone;
};



template <class T> inline
RunUntilFirstHit<T>::RunUntilFirstHit() : result(std::make_shared<AsyncResult>()) {}


template <class T>
template <class Fun> inline
void RunUntilFirstHit<T>::addJob(Fun f) //f must return a std::unique_ptr<T> containing a value on success
{
    auto result2 = result; //VC11: this is ridiculous!!!
    workload.push_back(boost::thread([result2, f]
    {
        result2->reportFinished(f());
    }));
}


template <class T>
template <class Duration> inline
bool RunUntilFirstHit<T>::timedWait(const Duration& duration) const { return result->waitForResult(workload.size(), duration); }


template <class T> inline
std::unique_ptr<T> RunUntilFirstHit<T>::get() const { return result->getResult(workload.size()); }
}

#endif //BOOST_THREAD_WRAP_H
