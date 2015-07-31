// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl-3.0        *
// * Copyright (C) Zenju (zenju AT gmx DOT de) - All Rights Reserved        *
// **************************************************************************

#ifndef ASYNC_JOB_839147839170432143214321
#define ASYNC_JOB_839147839170432143214321

#include <list>
#include <functional>
#include "thread.h"
#include "scope_guard.h"

namespace zen
{
//run a job in an async thread, but process result on GUI event loop
class AsyncTasks
{
public:
    AsyncTasks() {}

    template <class Fun, class Fun2>
    void add(Fun runAsync, Fun2 evalOnGui)
    //equivalent to "evalOnGui(runAsync())"
    //	-> runAsync: the usual thread-safety requirements apply!
    //	-> evalOnGui: no thread-safety concerns, but must only reference variables with greater-equal lifetime than the AsyncTask instance!
    {
        tasks.push_back(zen::runAsync([=]() -> std::function<void()>
        {
            auto result = runAsync();
            return [=]{ evalOnGui(result); };
        }));
    }

    template <class Fun, class Fun2>
    void add2(Fun runAsync, Fun2 evalOnGui) //for evalOnGui taking no parameters
    {
        tasks.push_back(zen::runAsync([runAsync, evalOnGui]() -> std::function<void()> { runAsync(); return [evalOnGui]{ evalOnGui(); }; }));
    }

    void evalResults() //call from gui thread repreatedly
    {
        if (!inRecursion) //prevent implicit recursion, e.g. if we're called from an idle event and spawn another one via the callback below
        {
            inRecursion = true;
            ZEN_ON_SCOPE_EXIT(inRecursion = false);

            tasks.remove_if([](std::future<std::function<void()>>& ft) -> bool
            {
                if (isReady(ft))
                {
                    (ft.get())();
                    return true;
                }
                return false;
            });
        }
    }

    bool empty() const { return tasks.empty(); }

private:
    AsyncTasks           (const AsyncTasks&) = delete;
    AsyncTasks& operator=(const AsyncTasks&) = delete;

    bool inRecursion = false;
    std::list<std::future<std::function<void()>>> tasks;
};
}

#endif //ASYNC_JOB_839147839170432143214321
