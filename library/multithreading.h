#ifndef MULTITHREADING_H_INCLUDED
#define MULTITHREADING_H_INCLUDED

#include "statusHandler.h"

class WorkerThread;


//class handling execution of a method while updating the UI
class UpdateWhileExecuting
{
    friend class WorkerThread;

public:
    UpdateWhileExecuting();

    virtual ~UpdateWhileExecuting();

    void waitUntilReady();
    void execute(StatusHandler* statusUpdater);


private:
    //implement a longrunning method without dependencies (e.g. copy file function); share input/output parameters as instance variables
    virtual void longRunner() = 0;

    WorkerThread* theWorkerThread;

    wxMutex     readyToReceiveResult;
    wxCondition receivingResult;
    bool workDone; //workaround for a bug in wxWidgets v2.8.9 class wxCondition: signals might get lost
};

#endif // MULTITHREADING_H_INCLUDED
