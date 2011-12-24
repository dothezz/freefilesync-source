// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) 2008-2011 ZenJu (zhnmju123 AT gmx.de)                    *
// **************************************************************************

#ifndef BATCHSTATUSHANDLER_H_INCLUDED
#define BATCHSTATUSHANDLER_H_INCLUDED

#include "../lib/status_handler.h"
#include "../lib/process_xml.h"
#include "../lib/error_log.h"
#include "progress_indicator.h"
#include "switch_to_gui.h"

class LogFile;
class SyncStatus;

//Exception class used to abort the "compare" and "sync" process
class BatchAbortProcess {};


class BatchStatusHandler : public StatusHandler
{
public:
    BatchStatusHandler(bool showProgress, //defines: -start minimized and -quit immediately when finished
                       const wxString& jobName,
                       const wxString& logfileDirectory,
                       size_t logFileCountMax, //0 if logging shall be inactive
                       const xmlAccess::OnError handleError,
                       const zen::SwitchToGui& switchBatchToGui, //functionality to change from batch mode to GUI mode
                       int& returnVal,
                       const std::wstring& execWhenFinished,
                       std::vector<std::wstring>& execFinishedHistory);
    ~BatchStatusHandler();

    virtual void initNewProcess     (int objectsTotal, zen::Int64 dataTotal, Process processID);
    virtual void updateProcessedData(int objectsDelta, zen::Int64 dataDelta);
    virtual void updateTotalData    (int objectsDelta, zen::Int64 dataDelta);
    virtual void reportStatus(const std::wstring& text);
    virtual void reportInfo(const std::wstring& text);
    virtual void forceUiRefresh();

    virtual void reportWarning(const std::wstring& warningMessage, bool& warningActive);
    virtual Response reportError(const std::wstring& errorMessage);
    virtual void reportFatalError(const std::wstring& errorMessage);

private:
    virtual void abortThisProcess();

    const zen::SwitchToGui& switchBatchToGui_; //functionality to change from batch mode to GUI mode
    bool showFinalResults;
    bool switchToGuiRequested;
    xmlAccess::OnError handleError_;
    zen::ErrorLogging errorLog; //list of non-resolved errors and warnings
    Process currentProcess;
    int& returnValue;

    SyncStatus syncStatusFrame; //the window managed by SyncStatus has longer lifetime than this handler!
    std::shared_ptr<LogFile> logFile; //optional!
};


#endif // BATCHSTATUSHANDLER_H_INCLUDED
