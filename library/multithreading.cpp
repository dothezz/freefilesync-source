#include "multithreading.h"
#include <wx/utils.h>
//#include "windows.h"
//MessageBox(0, "hi", "", 0);

/*choreography:

        -------------                           ---------------
        |main thread|                           |worker thread|
        -------------                           ---------------

1.  Instantiation (Constructor)
-------------------------------
        create thread
                                                enter waiting state
2. waitUntilReady
-------------------------------
        wait until thread is ready

3. Call execute
-------------------------------
        send signal to start
                                                start processing
        update UI while thread works
                                                finish processing
                                                wait until main thread is ready to receive signal
        receive signal
                                                enter waiting state
4. Termination (Destructor)
-------------------------------
        wait until thread is in wait state
        set exit flag
        signal thread to continue (and exit)
*/
class WorkerThread : public wxThread
{
    friend class UpdateWhileExecuting;

public:
    WorkerThread(UpdateWhileExecuting* handler) :
            readyToBeginProcessing(),
            beginProcessing(readyToBeginProcessing),
            threadIsInitialized(false),
            threadExitIsRequested(false),
            threadHandler(handler)
    { }


    ~WorkerThread() {}


    ExitCode Entry()
    {
        readyToBeginProcessing.Lock(); //this lock needs to be called IN the thread => calling it from constructor(Main thread) would be useless
        sharedData.Enter();
        threadIsInitialized = true;
        sharedData.Leave();

        while (true)
        {
            beginProcessing.Wait();

            //no mutex needed in this context
            if (threadExitIsRequested)
                return 0;

            //actual (long running) work is done in this method
            threadHandler->longRunner();

            threadHandler->readyToReceiveResult.Lock();
            threadHandler->receivingResult.Signal();
            threadHandler->readyToReceiveResult.Unlock();
        }

        return 0;
    }

private:
    wxMutex     readyToBeginProcessing;
    wxCondition beginProcessing;

    //shared data
    wxCriticalSection sharedData;
    bool threadIsInitialized;
    bool threadExitIsRequested;

    UpdateWhileExecuting* threadHandler;
};


UpdateWhileExecuting::UpdateWhileExecuting() :
        readyToReceiveResult(),
        receivingResult(readyToReceiveResult)
{
    //mutex needs to be initially locked for condition receivingResult to work properly
    readyToReceiveResult.Lock();


    theWorkerThread = new WorkerThread(this);

    theWorkerThread->Create();
    theWorkerThread->Run();

    //wait until the thread has locked readyToBeginProcessing
    bool threadInitialized = false;
    while (!threadInitialized)
    {
        theWorkerThread->sharedData.Enter();
        threadInitialized = theWorkerThread->threadIsInitialized;
        theWorkerThread->sharedData.Leave();
        wxMilliSleep(5);
    }   //-> it's not nice, but works and is no issue
}


UpdateWhileExecuting::~UpdateWhileExecuting()
{
    //wait until thread is ready, then start and exit immediately
    readyToReceiveResult.Unlock(); //avoid possible deadlock, when thread might be waiting to send the signal

    theWorkerThread->readyToBeginProcessing.Lock();
    //set flag to exit thread
    theWorkerThread->threadExitIsRequested = true;
    theWorkerThread->beginProcessing.Signal();

    theWorkerThread->readyToBeginProcessing.Unlock();

    //theWorkerThread deletes itself!
}


void UpdateWhileExecuting::waitUntilReady()
{
    readyToReceiveResult.Unlock(); //avoid possible deadlock, when thread might be waiting to send the signal (if abort was pressed)

    theWorkerThread->readyToBeginProcessing.Lock();
}
//          /|\  \|/   must be called directly after each other

void UpdateWhileExecuting::execute(StatusUpdater* statusUpdater)
{
    readyToReceiveResult.Lock();

    theWorkerThread->beginProcessing.Signal();
    theWorkerThread->readyToBeginProcessing.Unlock();

    while (receivingResult.WaitTimeout(UI_UPDATE_INTERVAL) == wxCOND_TIMEOUT)
        statusUpdater->triggerUI_Refresh(true); //ATTENTION: Exception "AbortThisProcess" may be thrown here!!!
}

