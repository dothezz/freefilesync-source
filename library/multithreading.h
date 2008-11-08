#ifndef MULTITHREADING_H_INCLUDED
#define MULTITHREADING_H_INCLUDED

#include <wx/string.h>
#include <wx/thread.h>

const int UI_UPDATE_INTERVAL = 100; //perform ui updates not more often than necessary, 100 seems to be a good value with only a minimal performance loss

bool updateUiIsAllowed(); //test if a specific amount of time is over
void updateUiNow();       //do the updating


//interface for status updates (can be implemented by UI or commandline)
//overwrite virtual methods for respective functionality
class StatusUpdater
{
public:
    StatusUpdater() :
            abortionRequested(false) {}
    virtual ~StatusUpdater() {}

    //these methods have to be implemented in the derived classes to handle error and status information
    virtual void updateStatusText(const wxString& text) = 0;
    virtual void initNewProcess(int objectsTotal, double dataTotal, int processID) = 0; //informs about the total amount of data that will be processed from now on
    virtual void updateProcessedData(int objectsProcessed, double dataProcessed) = 0;   //called periodically after data was processed
    virtual int  reportError(const wxString& text) = 0;

    //this method is triggered repeatedly by requestUiRefresh() and can be used to refresh the ui by dispatching pending events
    virtual void forceUiRefresh() = 0;
    void requestUiRefresh(bool asyncProcessActive = false)
    {
        if (updateUiIsAllowed())  //test if specific time span between ui updates is over
            forceUiRefresh();

        if (abortionRequested && !asyncProcessActive)
            abortThisProcess();  //abort can be triggered by requestAbortion()
    }

    void requestAbortion() //opportunity to abort must be implemented in a frequently executed method like requestUiRefresh()
    {                      //currently used by the UI status information screen, when button "Abort is pressed"
        abortionRequested = true;
    }

    static const int continueNext = -1;
    static const int retry        = -2;

protected:
    virtual void abortThisProcess() = 0;

    bool abortionRequested;
};


class WorkerThread;


//class handling execution of a method while updating the UI
class UpdateWhileExecuting
{
    friend class WorkerThread;

public:
    UpdateWhileExecuting();

    virtual ~UpdateWhileExecuting();

    void waitUntilReady();
    void execute(StatusUpdater* statusUpdater);


private:
    //implement a longrunning method without dependencies (e.g. copy file function); share input/output parameters as instance variables
    virtual void longRunner() = 0;

    WorkerThread* theWorkerThread;

    wxMutex     readyToReceiveResult;
    wxCondition receivingResult;
    bool workDone; //workaround for a bug in wxWidgets v2.8.9 class wxCondition: signals might get lost
};

#endif // MULTITHREADING_H_INCLUDED
