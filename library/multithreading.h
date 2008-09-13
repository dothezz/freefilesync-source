#ifndef MULTITHREADING_H_INCLUDED
#define MULTITHREADING_H_INCLUDED

#include <wx/string.h>
#include <wx/thread.h>

//interface for status updates (can be implemented by UI or commandline)
//overwrite virtual methods for respective functionality
class StatusUpdater
{
public:
    StatusUpdater() : abortionRequested(false) {}
    virtual ~StatusUpdater() {}

    //these four methods have to be implemented in the derived classes to handle error and status information
    virtual void updateStatusText(const wxString& text) = 0;
    virtual void initNewProcess(int objectsTotal, double dataTotal, int processID) = 0; //informs about the total amount of data that will be processed from now on
    virtual void updateProcessedData(int objectsProcessed, double dataProcessed) = 0; //called periodically after data was processed
    virtual int  reportError(const wxString& text) = 0;

    //this method is triggered repeatedly and can be used to refresh the ui by dispatching pending events
    virtual void triggerUI_Refresh() {}

    void requestAbortion() //opportunity to abort must be implemented in the three virtual status and error methods (for example in triggerUI_Refresh())
    {                      //currently used by the UI status information screen, when button "Abort is pressed"
        abortionRequested = true;
    }
    static const int continueNext = -1;
    static const int retry        = -2;

protected:
    bool abortionRequested;
};


const int uiUpdateInterval = 100; //perform ui updates not more often than necessary, 100 seems to be a good value with only a minimal performance loss

class WorkerThread;


//class handling execution of a method while updating the UI
class UpdateWhileExecuting
{
    friend class WorkerThread;

public:
    UpdateWhileExecuting();
    virtual ~UpdateWhileExecuting();

    void waitUntilReady();
    void execAndUpdate(StatusUpdater* statusUpdater);


private:
    //implement a longrunning method without dependencies (e.g. copy file function) returning "true" on success
    virtual void longRunner() = 0;

    WorkerThread* theWorkerThread;

    wxMutex     readyToReceiveResult;
    wxCondition receivingResult;
};

#endif // MULTITHREADING_H_INCLUDED
