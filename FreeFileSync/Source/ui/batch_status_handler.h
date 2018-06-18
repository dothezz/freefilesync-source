// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) Zenju (zenju AT gmx DOT de) - All Rights Reserved        *
// **************************************************************************

#ifndef BATCHSTATUSHANDLER_H_INCLUDED
#define BATCHSTATUSHANDLER_H_INCLUDED

#include <zen/error_log.h>
#include <zen/file_io.h>
#include <zen/time.h>
#include "../lib/status_handler.h"
#include "../lib/process_xml.h"
#include "progress_indicator.h"
#include "switch_to_gui.h"
#include "../lib/return_codes.h"


//Exception class used to abort the "compare" and "sync" process
class BatchAbortProcess {};


//BatchStatusHandler(SyncProgressDialog) will internally process Window messages! disable GUI controls to avoid unexpected callbacks!
class BatchStatusHandler : public zen::StatusHandler //throw BatchAbortProcess
{
public:
    BatchStatusHandler(bool showProgress, //defines: -start minimized and -quit immediately when finished
                       const std::wstring& jobName, //should not be empty for a batch job!
                       const zen::TimeComp& timeStamp,
                       const Zstring& logfileDirectory,
                       int logfilesCountLimit, //0: logging inactive; < 0: no limit
                       size_t lastSyncsLogFileSizeMax,
                       const xmlAccess::OnError handleError,
                       size_t automaticRetryCount,
                       size_t automaticRetryDelay,
                       const zen::SwitchToGui& switchBatchToGui, //functionality to change from batch mode to GUI mode
                       zen::FfsReturnCode& returnCode,
                       const Zstring& onCompletion,
                       std::vector<Zstring>& onCompletionHistory);
    ~BatchStatusHandler();

    virtual void initNewPhase       (int objectsTotal, std::int64_t dataTotal, Phase phaseID);
    virtual void updateProcessedData(int objectsDelta, std::int64_t dataDelta);
    virtual void reportInfo(const std::wstring& text);
    virtual void forceUiRefresh();

    virtual void reportWarning(const std::wstring& warningMessage, bool& warningActive);
    virtual Response reportError(const std::wstring& errorMessage, size_t retryNumber);
    virtual void reportFatalError(const std::wstring& errorMessage);

    virtual void abortProcessNow(); //throw BatchAbortProcess

private:
    void onProgressDialogTerminate();

    const zen::SwitchToGui& switchBatchToGui_; //functionality to change from batch mode to GUI mode
    bool showFinalResults;
    bool switchToGuiRequested;
    const int logfilesCountLimit_;
    const size_t lastSyncsLogFileSizeMax_;
    xmlAccess::OnError handleError_;
    zen::ErrorLog errorLog; //list of non-resolved errors and warnings
    zen::FfsReturnCode& returnCode_;

    const size_t automaticRetryCount_;
    const size_t automaticRetryDelay_;

    SyncProgressDialog* progressDlg; //managed to have shorter lifetime than this handler!
    std::unique_ptr<zen::FileOutput> logFile; //optional!

    const std::wstring jobName_;
    const int64_t startTime_; //don't use wxStopWatch: may overflow after a few days due to ::QueryPerformanceCounter()
};


#endif // BATCHSTATUSHANDLER_H_INCLUDED
