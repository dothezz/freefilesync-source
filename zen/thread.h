// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl-3.0        *
// * Copyright (C) Zenju (zenju AT gmx DOT de) - All Rights Reserved        *
// **************************************************************************

#ifndef BOOST_THREAD_WRAP_H_78963234
#define BOOST_THREAD_WRAP_H_78963234

//temporary solution until C++11 thread becomes fully available (considering std::thread's non-interruptibility and std::async craziness, this may be NEVER)
#include <memory>

//workaround this pathetic boost thread warning mess
#ifdef __GNUC__
    #pragma GCC diagnostic push
    #pragma GCC diagnostic ignored "-Wswitch-enum"
    #pragma GCC diagnostic ignored "-Wstrict-aliasing"
    #pragma GCC diagnostic ignored "-Wredundant-decls"
    #pragma GCC diagnostic ignored "-Wshadow"
    #ifndef __clang__ //clang defines __GNUC__, but doesn't support this warning
        #pragma GCC diagnostic ignored "-Wunused-local-typedefs"
    #endif
#endif
#ifdef _MSC_VER
    #pragma warning(push)
    #pragma warning(disable: 4702 4913) //unreachable code; user defined binary operator ',' exists but no overload could convert all operands, default built-in binary operator ',' used
#endif

#include <boost/thread.hpp>

#ifdef __GNUC__
    #pragma GCC diagnostic pop
#endif
#ifdef _MSC_VER
    #pragma warning(pop)
#endif

namespace zen
{
/*
std::async replacement without crappy semantics:
	1. guaranteed to run asynchronously
	2. does not follow C++11 [futures.async], Paragraph 5, where std::future waits for thread in destructor

Example:
        Zstring dirpath = ...
        auto ft = zen::runAsync([=](){ return zen::dirExists(dirpath); });
        if (ft.wait_for(boost::chrono::milliseconds(200)) == boost::future_status::ready && ft.get())
            //dir exising
*/
template <class Function>
auto runAsync(Function fun) -> boost::unique_future<decltype(fun())>;

//wait for all with a time limit: return true if *all* results are available!
template<class InputIterator, class Duration>
bool wait_for_all_timed(InputIterator first, InputIterator last, const Duration& wait_duration);

//wait until first job is successful or all failed: substitute until std::when_any is available
template <class T>
class GetFirstResult
{
public:
    GetFirstResult();

    template <class Fun>
    void addJob(Fun f); //f must return a std::unique_ptr<T> containing a value if successful

    template <class Duration>
    bool timedWait(const Duration& duration) const; //true: "get()" is ready, false: time elapsed

    //return first value or none if all jobs failed; blocks until result is ready!
    std::unique_ptr<T> get() const; //may be called only once!

private:
    class AsyncResult;
    std::shared_ptr<AsyncResult> asyncResult_;
    size_t jobsTotal_;
};


//value associated with mutex and guaranteed protected access:
template <class T>
class Protected
{
public:
    Protected()               : value_() {}
    Protected(const T& value) : value_(value) {}

    template <class Function>
    void access(Function fun)
    {
        boost::lock_guard<boost::mutex> dummy(lockValue);
        fun(value_);
    }

private:
    Protected           (const Protected&) = delete;
    Protected& operator=(const Protected&) = delete;

    boost::mutex lockValue;
    T value_;
};









//###################### implementation ######################
#ifndef BOOST_HAS_THREADS
    #error just some paranoia check...
#endif

template <class Function> inline
auto runAsync(Function fun) -> boost::unique_future<decltype(fun())>
{
    typedef decltype(fun()) ResultType;

#if defined BOOST_THREAD_PROVIDES_SIGNATURE_PACKAGED_TASK //mirror "boost/thread/future.hpp", hopefully they know what they're doing
    boost::packaged_task<ResultType()> pt(std::move(fun));
#else
    boost::packaged_task<ResultType> pt(std::move(fun));
#endif
    auto fut = pt.get_future();
    boost::thread(std::move(pt)).detach(); //we have to explicitly detach since C++11: [thread.thread.destr] ~thread() calls std::terminate() if joinable()!!!
    return fut;
}


template<class InputIterator, class Duration> inline
bool wait_for_all_timed(InputIterator first, InputIterator last, const Duration& duration)
{
    const boost::chrono::steady_clock::time_point endTime = boost::chrono::steady_clock::now() + duration;
    for (; first != last; ++first)
        if (first->wait_until(endTime) != boost::future_status::ready)
            return false; //time elapsed
    return true;
}


template <class T>
class GetFirstResult<T>::AsyncResult
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
            boost::lock_guard<boost::mutex> dummy(lockResult);
            ++jobsFinished;
            if (!result_)
                result_ = std::move(result);
        }
        conditionJobDone.notify_all(); //instead of notify_one(); workaround bug: https://svn.boost.org/trac/boost/ticket/7796
        //condition handling, see: http://www.boost.org/doc/libs/1_43_0/doc/html/thread/synchronization.html#thread.synchronization.condvar_ref
    }

    //context: main thread
    template <class Duration>
    bool waitForResult(size_t jobsTotal, const Duration& duration)
    {
        boost::unique_lock<boost::mutex> dummy(lockResult);
        return conditionJobDone.wait_for(dummy, duration, [&] { return this->jobDone(jobsTotal); }); //throw boost::thread_interrupted
    }

    std::unique_ptr<T> getResult(size_t jobsTotal)
    {
        boost::unique_lock<boost::mutex> dummy(lockResult);
        conditionJobDone.wait(dummy, [&] { return this->jobDone(jobsTotal); }); //throw boost::thread_interrupted

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
GetFirstResult<T>::GetFirstResult() : asyncResult_(std::make_shared<AsyncResult>()), jobsTotal_(0) {}


template <class T>
template <class Fun> inline
void GetFirstResult<T>::addJob(Fun f) //f must return a std::unique_ptr<T> containing a value on success
{
    auto asyncResult = this->asyncResult_; //capture member variable, not "this"!
    boost::thread t([asyncResult, f] { asyncResult->reportFinished(f()); });
    ++jobsTotal_;
    t.detach(); //we have to be explicit since C++11: [thread.thread.destr] ~thread() calls std::terminate() if joinable()!!!
}


template <class T>
template <class Duration> inline
bool GetFirstResult<T>::timedWait(const Duration& duration) const { return asyncResult_->waitForResult(jobsTotal_, duration); }


template <class T> inline
std::unique_ptr<T> GetFirstResult<T>::get() const { return asyncResult_->getResult(jobsTotal_); }
}

#endif //BOOST_THREAD_WRAP_H_78963234
