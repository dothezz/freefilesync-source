// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) 2008-2011 ZenJu (zhnmju123 AT gmx.de)                    *
// **************************************************************************
//
#ifndef STATUSHANDLER_H_INCLUDED
#define STATUSHANDLER_H_INCLUDED

#include <wx/longlong.h>
#include "../shared/zstring.h"


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


class StatusHandler
{
public:
    StatusHandler() : abortRequested(false) {}
    virtual ~StatusHandler() {}

    //identifiers of different phases
    enum Process
    {
        PROCESS_NONE = 10,
        PROCESS_SCANNING,
        PROCESS_COMPARING_CONTENT,
        PROCESS_SYNCHRONIZING
    };

    //these methods have to be implemented in the derived classes to handle error and status information
    virtual void initNewProcess(int objectsTotal, wxLongLong dataTotal, Process processID) = 0; //informs about the total amount of data that will be processed from now on
    virtual void updateProcessedData(int objectsProcessed, wxLongLong dataProcessed) = 0;  //called periodically after data was processed

    virtual void reportInfo(const Zstring& text) = 0;

    //this method is triggered repeatedly by requestUiRefresh() and can be used to refresh the ui by dispatching pending events
    virtual void forceUiRefresh() = 0;

    void requestUiRefresh(bool allowAbort = true); //opportunity to abort must be implemented in a frequently executed method like requestUiRefresh()
    void requestAbortion();  //this does NOT call abortThisProcess immediately, but when appropriate (e.g. async. processes finished)
    bool abortIsRequested();

    //error handling:
    virtual ErrorHandler::Response reportError(const wxString& errorMessage) = 0; //recoverable error situation
    virtual void reportFatalError(const wxString& errorMessage) = 0;              //non-recoverable error situation, implement abort!
    virtual void reportWarning(const wxString& warningMessage, bool& warningActive) = 0;

private:
    virtual void abortThisProcess() = 0;

    bool abortRequested;
};



//##############################################################################
inline
void StatusHandler::requestUiRefresh(bool allowAbort)
{
    if (updateUiIsAllowed())  //test if specific time span between ui updates is over
        forceUiRefresh();

    if (abortRequested && allowAbort)
        abortThisProcess();  //abort can be triggered by requestAbortion()
}


inline
void StatusHandler::requestAbortion()
{
    abortRequested = true;
}


inline
bool StatusHandler::abortIsRequested()
{
    return abortRequested;
}

#endif // STATUSHANDLER_H_INCLUDED
