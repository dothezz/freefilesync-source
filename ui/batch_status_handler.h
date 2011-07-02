// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) 2008-2011 ZenJu (zhnmju123 AT gmx.de)                    *
// **************************************************************************
//
#ifndef BATCHSTATUSHANDLER_H_INCLUDED
#define BATCHSTATUSHANDLER_H_INCLUDED

#include "../library/status_handler.h"
#include "../library/process_xml.h"
#include "../library/error_log.h"
#include "progress_indicator.h"
#include "switch_to_gui.h"

class LogFile;
class SyncStatus;

//Exception class used to abort the "compare" and "sync" process
class BatchAbortProcess {};


class BatchStatusHandler : public StatusHandler
{
public:
    BatchStatusHandler(bool runSilent, //defines: -start minimized and -quit immediately when finished
                       const wxString& jobName,
                       const wxString* logfileDirectory, //non-empty if logging shall be active
                       size_t logFileMaxCount,
                       const xmlAccess::OnError handleError,
                       const zen::SwitchToGui& switchBatchToGui, //functionality to change from batch mode to GUI mode
                       int& returnVal);
    ~BatchStatusHandler();

    virtual void initNewProcess(int objectsTotal, zen::Int64 dataTotal, Process processID);
    virtual void updateProcessedData(int objectsProcessed, zen::Int64 dataProcessed);
    virtual void reportInfo(const Zstring& text);
    virtual void forceUiRefresh();

    virtual void reportWarning(const wxString& warningMessage, bool& warningActive);
    virtual ErrorHandler::Response reportError(const wxString& errorMessage);
    virtual void reportFatalError(const wxString& errorMessage);

private:
    virtual void abortThisProcess();

    const zen::SwitchToGui& switchBatchToGui_; //functionality to change from batch mode to GUI mode
    bool exitWhenFinished;
    bool switchToGuiRequested;
    xmlAccess::OnError handleError_;
    zen::ErrorLogging errorLog; //list of non-resolved errors and warnings
    Process currentProcess;
    int& returnValue;

    SyncStatus syncStatusFrame; //the window managed by SyncStatus has longer lifetime than this handler!
    std::shared_ptr<LogFile> logFile; //optional!
};


#endif // BATCHSTATUSHANDLER_H_INCLUDED
