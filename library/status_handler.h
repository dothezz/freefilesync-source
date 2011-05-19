// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) 2008-2011 ZenJu (zhnmju123 AT gmx.de)                    *
// **************************************************************************
//
#ifndef STATUSHANDLER_H_INCLUDED
#define STATUSHANDLER_H_INCLUDED

#include <wx/string.h>
#include "../shared/zstring.h"
#include "../shared/int64.h"

const int UI_UPDATE_INTERVAL = 100; //perform ui updates not more often than necessary, 100 seems to be a good value with only a minimal performance loss

bool updateUiIsAllowed(); //test if a specific amount of time is over
void updateUiNow();       //do the updating


//interfaces for status updates (can be implemented by GUI or Batch mode)
//overwrite virtual methods for respective functionality

class ErrorHandler
{
public:
    ErrorHandler() {}
    virtual ~ErrorHandler() {}

    enum Response
    {
        IGNORE_ERROR = 10,
        RETRY
    };
    virtual Response reportError(const Zstring& errorMessage) = 0;
};


//report status during comparison and synchronization
struct ProcessCallback
{
    virtual ~ProcessCallback() {}

    //identifiers of different phases
    enum Process
    {
        PROCESS_NONE = 10,
        PROCESS_SCANNING,
        PROCESS_COMPARING_CONTENT,
        PROCESS_SYNCHRONIZING
    };

    //these methods have to be implemented in the derived classes to handle error and status information
    virtual void initNewProcess(int objectsTotal, zen::Int64 dataTotal, Process processID) = 0; //informs about the total amount of data that will be processed from now on
    virtual void updateProcessedData(int objectsProcessed, zen::Int64 dataProcessed) = 0;  //called periodically after data was processed

    virtual void reportInfo(const Zstring& text) = 0;

    //this method is triggered repeatedly by requestUiRefresh() and can be used to refresh the ui by dispatching pending events
    virtual void forceUiRefresh() = 0;

    virtual void requestUiRefresh(bool allowExceptions = true) = 0; //opportunity to abort must be implemented in a frequently executed method like requestUiRefresh()

    virtual bool abortIsRequested() = 0; //thanks to Windows C-Api not supporting exceptions we need this one...

    //error handling:
    virtual ErrorHandler::Response reportError(const wxString& errorMessage) = 0; //recoverable error situation
    virtual void reportFatalError(const wxString& errorMessage) = 0;              //non-recoverable error situation, implement abort!
    virtual void reportWarning   (const wxString& warningMessage, bool& warningActive) = 0;
};


//gui may want to abort process
struct AbortCallback
{
    virtual ~AbortCallback() {}
    virtual void requestAbortion() = 0;
};


//actual callback implementation will have to satisfy "process" and "gui"
class StatusHandler : public ProcessCallback, public AbortCallback
{
public:
    StatusHandler() : abortRequested(false) {}

    virtual void requestUiRefresh(bool allowExceptions)
    {
        if (updateUiIsAllowed())  //test if specific time span between ui updates is over
            forceUiRefresh();

        if (abortRequested && allowExceptions)
            abortThisProcess();  //abort can be triggered by requestAbortion()
    }

    virtual void requestAbortion() { abortRequested = true; } //this does NOT call abortThisProcess immediately, but when appropriate (e.g. async. processes finished)
    virtual bool abortIsRequested() { return abortRequested; }
    virtual void abortThisProcess() = 0;

private:
    bool abortRequested;
};



#endif // STATUSHANDLER_H_INCLUDED
