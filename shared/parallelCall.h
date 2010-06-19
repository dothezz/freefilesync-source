// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) 2008-2010 ZenJu (zhnmju123 AT gmx.de)                    *
// **************************************************************************
//
#ifndef MULTITHREADING_H_INCLUDED
#define MULTITHREADING_H_INCLUDED

#include <boost/shared_ptr.hpp>


namespace Async
{
class Procedure
{
public:
    virtual ~Procedure() {}
    virtual void doWork() = 0;
};

enum Result
{
    TIMEOUT,
    WORK_DONE
};

typedef boost::shared_ptr<Procedure> AsyncProcess;

//wait for Procedure to finish in separate thread; shared data (Procedure*) may be accessed after WORK_DONE only! => Beware shared data if TIMEOUT (e.g. ref-counting!)
Result execute(AsyncProcess proc, size_t maxWait); //maxWait = max. wait time in milliseconds
}

#endif // MULTITHREADING_H_INCLUDED
