// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) 2008-2011 ZenJu (zhnmju123 AT gmx.de)                    *
// **************************************************************************

#include "progress_indicator.h"
#include <memory>
#include <zen/basic_math.h>
#include <wx/imaglist.h>
#include <wx/stopwatch.h>
#include <wx/wupdlock.h>
#include <wx+/mouse_move_dlg.h>
#include <wx+/toggle_button.h>
#include <wx+/string_conv.h>
#include <wx+/format_unit.h>
#include <wx+/image_tools.h>
#include <wx+/graph.h>
#include <wx+/no_flicker.h>
#include "gui_generated.h"
#include "../lib/resources.h"
#include "../lib/error_log.h"
#include "../lib/statistics.h"
#include "tray_icon.h"
#include "taskbar.h"
#include "exec_finished_box.h"

using namespace zen;


namespace
{
const int GAUGE_FULL_RANGE = 50000;

//window size used for statistics in milliseconds
const int windowSizeRemainingTime = 60000; //some usecases have dropouts of 40 seconds -> 60 sec. window size handles them well
const int windowSizeBytesPerSec   =  5000; //
}


class CompareStatus::CompareStatusImpl : public CompareStatusGenerated
{
public:
    CompareStatusImpl(wxTopLevelWindow& parentWindow);

    void init();     //constructor/destructor semantics, but underlying Window is reused
    void finalize(); //

    void switchToCompareBytewise(int totalObjectsToProcess, Int64 totalDataToProcess);
    void incScannedObjects_NoUpdate(int number);
    void incProcessedCmpData_NoUpdate(int objectsProcessed, Int64 dataProcessed);
    void incTotalCmpData_NoUpdate    (int objectsProcessed, Int64 dataProcessed);

    void setStatusText_NoUpdate(const wxString& text);
    void updateStatusPanelNow();

private:
    wxTopLevelWindow& parentWindow_;
    wxString titleTextBackup;

    //status variables
    size_t scannedObjects;
    wxString currentStatusText;

    wxStopWatch timeElapsed;

    //gauge variables
    int   totalObjects;   //file/dir/symlink/operation count
    Int64 totalData;      //unit: [bytes]
    int   currentObjects;
    Int64 currentData;

    void showProgressExternally(const wxString& progressText, double fraction = 0); //between [0, 1]

    enum CurrentStatus
    {
        SCANNING,
        COMPARING_CONTENT,
    };

    CurrentStatus status;

    std::unique_ptr<Taskbar> taskbar_;

    //remaining time
    std::unique_ptr<Statistics> statistics;

    long lastStatCallSpeed;   //used for calculating intervals between statistics update
    long lastStatCallRemTime; //
};


CompareStatus::CompareStatusImpl::CompareStatusImpl(wxTopLevelWindow& parentWindow) :
    CompareStatusGenerated(&parentWindow),
    parentWindow_(parentWindow),
    scannedObjects(0),
    totalObjects(0),
    totalData(0),
    currentObjects(0),
    currentData(0),
    status(SCANNING),
    lastStatCallSpeed(-1000000), //some big number
    lastStatCallRemTime(-1000000)
{
    init();
}


void CompareStatus::CompareStatusImpl::init()
{
    titleTextBackup = parentWindow_.GetTitle();

    try //try to get access to Windows 7/Ubuntu taskbar
    {
        taskbar_.reset(new Taskbar(parentWindow_));
    }
    catch (const TaskbarNotAvailable&) {}

    status = SCANNING;

    //initialize gauge
    m_gauge2->SetRange(GAUGE_FULL_RANGE);
    m_gauge2->SetValue(0);

    //initially hide status that's relevant for comparing bytewise only
    bSizerFilesFound    ->Show(true);
    bSizerFilesRemaining->Show(false);
    sSizerSpeed         ->Show(false);
    sSizerTimeRemaining ->Show(false);

    m_gauge2->Hide();
    bSizer42->Layout();

    scannedObjects = 0;
    currentStatusText.clear();

    totalObjects   = 0;
    totalData      = 0;
    currentObjects = 0;
    currentData    = 0;

    statistics.reset();

    timeElapsed.Start(); //measure total time

    updateStatusPanelNow();

    Layout();
}


void CompareStatus::CompareStatusImpl::finalize()
{
    taskbar_.reset();
    parentWindow_.SetTitle(titleTextBackup);
}


void CompareStatus::CompareStatusImpl::switchToCompareBytewise(int totalObjectsToProcess, Int64 totalDataToProcess)
{
    status = COMPARING_CONTENT;

    currentData    = 0;
    currentObjects = 0;
    totalData      = totalDataToProcess;
    totalObjects   = totalObjectsToProcess;

    //set new statistics handler: 10 seconds "window" for remaining time, 5 seconds for speed
    statistics.reset(new Statistics(totalObjectsToProcess, to<double>(totalDataToProcess), windowSizeRemainingTime, windowSizeBytesPerSec));
    lastStatCallSpeed   = -1000000; //some big number
    lastStatCallRemTime = -1000000;

    //show status for comparing bytewise
    bSizerFilesFound->Show(false);
    bSizerFilesRemaining->Show(true);
    sSizerSpeed->Show(true);
    sSizerTimeRemaining->Show(true);

    m_gauge2->Show();
    bSizer42->Layout();
    Layout();
}


void CompareStatus::CompareStatusImpl::incScannedObjects_NoUpdate(int number)
{
    scannedObjects += number;
}


void CompareStatus::CompareStatusImpl::incProcessedCmpData_NoUpdate(int objectsDelta, Int64 dataDelta)
{
    currentData    +=    dataDelta;
    currentObjects += objectsDelta;
}


void CompareStatus::CompareStatusImpl::incTotalCmpData_NoUpdate(int objectsDelta, Int64 dataDelta)
{
    totalData    +=    dataDelta;
    totalObjects += objectsDelta;

    if (statistics)
        statistics->setNewTotal(totalObjects, to<double>(totalData));
}


void CompareStatus::CompareStatusImpl::setStatusText_NoUpdate(const wxString& text)
{
    currentStatusText = text;
}


void CompareStatus::CompareStatusImpl::showProgressExternally(const wxString& progressText, double fraction)
{
    if (parentWindow_.GetTitle() != progressText)
        parentWindow_.SetTitle(progressText);

    //show progress on Windows 7 taskbar

    if (taskbar_.get())
        switch (status)
        {
            case SCANNING:
                taskbar_->setStatus(Taskbar::STATUS_INDETERMINATE);
                break;
            case COMPARING_CONTENT:
                taskbar_->setProgress(fraction);
                taskbar_->setStatus(Taskbar::STATUS_NORMAL);
                break;
        }
}


void CompareStatus::CompareStatusImpl::updateStatusPanelNow()
{
    //static RetrieveStatistics statistic;
    //statistic.writeEntry(currentData.ToDouble(), currentObjects);
    {
        //wxWindowUpdateLocker dummy(this) -> not needed

        //add both data + obj-count, to handle "deletion-only" cases
        const double fraction = totalData + totalObjects == 0 ? 0 : std::max(0.0, to<double>(currentData + currentObjects) / to<double>(totalData + totalObjects));

        //write status information to taskbar, parent title ect.
        switch (status)
        {
            case SCANNING:
                showProgressExternally(toStringSep(scannedObjects) + wxT(" - ") + _("Scanning..."));
                break;
            case COMPARING_CONTENT:
                showProgressExternally(fractionToShortString(fraction) + wxT(" - ") + _("Comparing content..."), fraction);
                break;
        }

        bool layoutChanged = false; //avoid screen flicker by calling layout() only if necessary

        //remove linebreaks from currentStatusText
        wxString statusTextFmt = currentStatusText;
        replace(statusTextFmt, L'\n', L' ');

        //status texts
        if (m_textCtrlStatus->GetValue() != statusTextFmt) //no layout update for status texts!
            m_textCtrlStatus->ChangeValue(statusTextFmt);

        //nr of scanned objects
        setText(*m_staticTextScanned, toStringSep(scannedObjects), &layoutChanged);

        //progress indicator for "compare file content"
        m_gauge2->SetValue(numeric::round(fraction * GAUGE_FULL_RANGE));

        //remaining files left for file comparison
        const wxString filesToCompareTmp = toStringSep(totalObjects - currentObjects);
        setText(*m_staticTextFilesRemaining, filesToCompareTmp, &layoutChanged);

        //remaining bytes left for file comparison
        const wxString remainingBytesTmp = zen::filesizeToShortString(totalData - currentData);
        setText(*m_staticTextDataRemaining, remainingBytesTmp, &layoutChanged);

        if (statistics.get())
        {
            if (timeElapsed.Time() - lastStatCallSpeed >= 500) //call method every 500 ms
            {
                lastStatCallSpeed = timeElapsed.Time();

                statistics->addMeasurement(currentObjects, to<double>(currentData));

                //current speed
                setText(*m_staticTextSpeed, statistics->getBytesPerSecond(), &layoutChanged);

                if (timeElapsed.Time() - lastStatCallRemTime >= 2000) //call method every two seconds only
                {
                    lastStatCallRemTime = timeElapsed.Time();

                    //remaining time
                    setText(*m_staticTextRemTime, statistics->getRemainingTime(), &layoutChanged);
                }
            }
        }

        //time elapsed
        const long timeElapSec = timeElapsed.Time() / 1000;
        setText(*m_staticTextTimeElapsed,
                timeElapSec < 3600 ?
                wxTimeSpan::Seconds(timeElapSec).Format(   L"%M:%S") :
                wxTimeSpan::Seconds(timeElapSec).Format(L"%H:%M:%S"), &layoutChanged);

        //do the ui update
        if (layoutChanged)
            bSizer42->Layout();
    }
    updateUiNow();
}
//########################################################################################

//redirect to implementation
CompareStatus::CompareStatus(wxTopLevelWindow& parentWindow) :
    pimpl(new CompareStatusImpl(parentWindow)) {}

CompareStatus::~CompareStatus()
{
    //DON'T delete pimpl! it relies on wxWidgets destruction (parent window destroys child windows!)
}

wxWindow* CompareStatus::getAsWindow()
{
    return pimpl;
}

void CompareStatus::init()
{
    pimpl->init();
}

void CompareStatus::finalize()
{
    pimpl->finalize();
}

void CompareStatus::switchToCompareBytewise(int totalObjectsToProcess, zen::Int64 totalDataToProcess)
{
    pimpl->switchToCompareBytewise(totalObjectsToProcess, totalDataToProcess);
}

void CompareStatus::incScannedObjects_NoUpdate(int number)
{
    pimpl->incScannedObjects_NoUpdate(number);
}

void CompareStatus::incProcessedCmpData_NoUpdate(int objectsDelta, Int64 dataDelta)
{
    pimpl->incProcessedCmpData_NoUpdate(objectsDelta, dataDelta);
}

void CompareStatus::incTotalCmpData_NoUpdate(int objectsDelta, Int64 dataDelta)
{
    pimpl->incTotalCmpData_NoUpdate(objectsDelta, dataDelta);
}

void CompareStatus::setStatusText_NoUpdate(const wxString& text)
{
    pimpl->setStatusText_NoUpdate(text);
}

void CompareStatus::updateStatusPanelNow()
{
    pimpl->updateStatusPanelNow();
}
//########################################################################################

namespace
{
inline
wxBitmap buttonPressed(const std::string& name)
{
    wxBitmap background = GlobalResources::getImage(wxT("log button pressed"));
    return layOver(GlobalResources::getImage(utf8CvrtTo<wxString>(name)), background);
}


inline
wxBitmap buttonReleased(const std::string& name)
{
    wxImage output = greyScale(GlobalResources::getImage(utf8CvrtTo<wxString>(name))).ConvertToImage();
    //GlobalResources::getImage(utf8CvrtTo<wxString>(name)).ConvertToImage().ConvertToGreyscale(1.0/3, 1.0/3, 1.0/3); //treat all channels equally!
    //brighten(output, 30);

    zen::move(output, 0, -1); //move image right one pixel
    return output;
}
}


class LogControl : public LogControlGenerated
{
public:
    LogControl(wxWindow* parent, const ErrorLogging& log) : LogControlGenerated(parent), log_(log)
    {
        const int errorCount   = log_.typeCount(TYPE_ERROR) + log_.typeCount(TYPE_FATAL_ERROR);
        const int warningCount = log_.typeCount(TYPE_WARNING);
        const int infoCount    = log_.typeCount(TYPE_INFO);

        m_bpButtonErrors->init(buttonPressed ("error"), wxString(_("Error")) + wxString::Format(wxT(" (%d)"), errorCount),
                               buttonReleased("error"), wxString(_("Error")) + wxString::Format(wxT(" (%d)"), errorCount));

        m_bpButtonWarnings->init(buttonPressed ("warning"), wxString(_("Warning")) + wxString::Format(wxT(" (%d)"), warningCount),
                                 buttonReleased("warning"), wxString(_("Warning")) + wxString::Format(wxT(" (%d)"), warningCount));

        m_bpButtonInfo->init(buttonPressed ("info"), wxString(_("Info")) + wxString::Format(wxT(" (%d)"), infoCount),
                             buttonReleased("info"), wxString(_("Info")) + wxString::Format(wxT(" (%d)"), infoCount));

        m_bpButtonErrors  ->setActive(true);
        m_bpButtonWarnings->setActive(true);
        m_bpButtonInfo    ->setActive(errorCount + warningCount == 0);

        m_bpButtonErrors  ->Show(errorCount   != 0);
        m_bpButtonWarnings->Show(warningCount != 0);
        m_bpButtonInfo    ->Show(infoCount    != 0);

        updateLogText();

        m_textCtrlInfo->Connect(wxEVT_KEY_DOWN, wxKeyEventHandler(LogControl::onKeyEvent), NULL, this);
    }

    virtual void OnErrors(wxCommandEvent& event)
    {
        m_bpButtonErrors->toggle();
        updateLogText();
    }

    virtual void OnWarnings(wxCommandEvent& event)
    {
        m_bpButtonWarnings->toggle();
        updateLogText();
    }

    virtual void OnInfo(wxCommandEvent& event)
    {
        m_bpButtonInfo->toggle();
        updateLogText();
    }

private:
    void onKeyEvent(wxKeyEvent& event)
    {
        const int keyCode = event.GetKeyCode();

        if (event.ControlDown())
            switch (keyCode)
            {
                case 'A': //CTRL + A
                    m_textCtrlInfo->SetSelection(-1, -1); //select all
                    return;
            }
        event.Skip();
    }

    void updateLogText()
    {
        int includedTypes = 0;
        if (m_bpButtonErrors->isActive())
            includedTypes |= TYPE_ERROR | TYPE_FATAL_ERROR;

        if (m_bpButtonWarnings->isActive())
            includedTypes |= TYPE_WARNING;

        if (m_bpButtonInfo->isActive())
            includedTypes |= TYPE_INFO;

        const std::vector<wxString>& messages = log_.getFormattedMessages(includedTypes);

        //fast replacement for wxString modelling exponential growth
        typedef Zbase<wchar_t> zxString;

        zxString newLogText; //perf: wxString doesn't model exponential growth and so is out

        if (!messages.empty())
            for (std::vector<wxString>::const_iterator i = messages.begin(); i != messages.end(); ++i)
            {
                newLogText += copyStringTo<zxString>(*i);
                newLogText += wxT("\n");
            }
        else //if no messages match selected view filter, show final status message at least
        {
            const std::vector<wxString>& allMessages = log_.getFormattedMessages();
            if (!allMessages.empty())
                newLogText = copyStringTo<zxString>(allMessages.back());
        }

        wxWindowUpdateLocker dummy(m_textCtrlInfo);
        m_textCtrlInfo->ChangeValue(copyStringTo<wxString>(newLogText));
        m_textCtrlInfo->ShowPosition(m_textCtrlInfo->GetLastPosition());
    }

    const ErrorLogging log_;
};


//########################################################################################

namespace
{
class GraphDataBytes : public GraphData
{
public:
    void addCurrentValue(double dataCurrent)
    {
        data.insert(data.end(), std::make_pair(timer.Time(), dataCurrent));
        //documentation differs about whether "hint" should be before or after the to be inserted element!
        //however "std::map<>::end()" is interpreted correctly by GCC and VS2010

        if (data.size() > MAX_BUFFER_SIZE) //guard against too large a buffer
            data.erase(data.begin());
    }

    void pauseTimer () { timer.Pause();  }
    void resumeTimer() { timer.Resume(); }

    virtual double getXBegin() const { return 0; } //{ return data.empty() ? 0 : data.begin()->first / 1000.0; }
    virtual double getXEnd  () const { return data.empty() ? 0 : (--data.end())->first / 1000.0; }

private:
    static const size_t MAX_BUFFER_SIZE = 2500000; //sizeof(single node) worst case ~ 3 * 8 byte ptr + 16 byte key/value = 40 byte

    virtual double getValue(double x) const //x: seconds since begin
    {
        auto iter = data.lower_bound(x * 1000);
        if (iter == data.end())
            return data.empty() ? 0 : (--data.end())->second;
        return iter->second;
    }
    //example: two-element range is accessible within [0, 2)

    wxStopWatch timer;
    std::map<long, double> data;
};


class GraphDataConstLine : public GraphData //a constant line
{
public:
    GraphDataConstLine() : value_(0) {}

    void setValue(double val) { value_ = val; }

private:
    virtual double getValue(double x) const { return value_; }
    virtual double getXBegin() const { return -std::numeric_limits<double>::infinity(); }
    virtual double getXEnd  () const { return  std::numeric_limits<double>::infinity(); }

    double value_;
};


inline
double bestFit(double val, double low, double high) { return val < (high + low) / 2 ? low : high; }


struct LabelFormatterBytes : public LabelFormatter
{
    virtual double getOptimalBlockSize(double bytesProposed) const
    {
        if (bytesProposed <= 0)
            return 0;

        bytesProposed *= 1.5; //enlarge block default size

        //if (bytesProposed <= 1024 * 1024) //set 1 MB min size: reduce initial rapid changes in y-label
        //  return 1024 * 1024;

        //round to next number which is a convenient to read block size

        const double k = std::floor(std::log(bytesProposed) / std::log(2.0));
        const double e = std::pow(2.0, k);
        if (numeric::isNull(e))
            return 0;
        const double a = bytesProposed / e; //bytesProposed = a * 2^k with a in (1, 2)

        return bestFit(a, 1, 2) * e;
    }

    virtual wxString formatText(double value, double optimalBlockSize) const { return filesizeToShortString(Int64(value)); };
};


struct LabelFormatterTimeElapsed : public LabelFormatter
{
    virtual double getOptimalBlockSize(double secProposed) const
    {
        if (secProposed <= 10)
            return 10; //minimum block size

        if (secProposed <= 20) //avoid flicker between 10<->15<->20 sec blocks
            return bestFit(secProposed, 10, 20);

        //for seconds and minutes: nice numbers are 1, 5, 10, 15, 20, 30
        auto calcBlock = [](double val) -> double
        {
            if (val <= 5)
                return bestFit(val, 1, 5); //
            if (val <= 10)
                return bestFit(val, 5, 10); // a good candidate for a variadic template!
            if (val <= 15)
                return bestFit(val, 10, 15); //
            if (val <= 20)
                return bestFit(val, 15, 20);
            if (val <= 30)
                return bestFit(val, 20, 30);
            return bestFit(val, 30, 60);
        };

        if (secProposed <= 60)
            return calcBlock(secProposed);
        else if (secProposed <= 3600)
            return calcBlock(secProposed / 60) * 60;
        else if (secProposed <= 3600 * 24)
            return nextNiceNumber(secProposed / 3600) * 3600;
        else
            return nextNiceNumber(secProposed / (24 * 3600)) * 24 * 3600; //round up to full days
    }

    virtual wxString formatText(double timeElapsed, double optimalBlockSize) const
    {
        return timeElapsed < 60 ?
               remainingTimeToShortString(timeElapsed) :
               timeElapsed < 3600 ?
               wxTimeSpan::Seconds(timeElapsed).Format(   L"%M:%S") :
               wxTimeSpan::Seconds(timeElapsed).Format(L"%H:%M:%S");
    }
};

//void fitHeight(wxTopLevelWindow& wnd)
//{
//    if (wnd.IsMaximized())
//        return;
//    //Fit() height only:
//    int width = wnd.GetSize().GetWidth();
//    wnd.Fit();
//    int height = wnd.GetSize().GetHeight();
//    wnd.SetSize(wxSize(width, height));
//}
}


class SyncStatus::SyncStatusImpl : public SyncStatusDlgGenerated
{
public:
    SyncStatusImpl(AbortCallback& abortCb,
                   MainDialog* parentWindow,
                   SyncStatusID startStatus,
                   const wxString& jobName,
                   const std::wstring& execWhenFinished,
                   std::vector<std::wstring>& execFinishedHistory);
    ~SyncStatusImpl();

    void initNewProcess(SyncStatusID id, int totalObjectsToProcess, Int64 totalDataToProcess);

    void incScannedObjects_NoUpdate(int number);
    void incProcessedData_NoUpdate(int objectsDelta, Int64 dataDelta);
    void incTotalData_NoUpdate    (int objectsDelta, Int64 dataDelta);

    void setStatusText_NoUpdate(const wxString& text);
    void updateStatusDialogNow(bool allowYield = true);

    void processHasFinished(SyncStatus::SyncStatusID id, const ErrorLogging& log);  //essential to call this in StatusUpdater derived class destructor at the LATEST(!) to prevent access to currentStatusUpdater

    std::wstring getExecWhenFinishedCommand() const;

    void stopTimer();   //halt all internal counters!
    void resumeTimer(); //

    void minimizeToTray();

private:
    void OnKeyPressed(wxKeyEvent& event);
    virtual void OnOkay(wxCommandEvent& event);
    virtual void OnPause(wxCommandEvent& event);
    virtual void OnAbort(wxCommandEvent& event);
    virtual void OnClose(wxCloseEvent& event);
    virtual void OnIconize(wxIconizeEvent& event);

    void setCurrentStatus(SyncStatus::SyncStatusID id);

    void resumeFromSystray();
    void OnResumeFromTray(wxCommandEvent& event);

    bool currentProcessIsRunning();
    void showProgressExternally(const wxString& progressText, double fraction = 0); //between [0, 1]

    const wxString jobName_;
    wxStopWatch timeElapsed;

    AbortCallback* abortCb_; //temporarily bound
    MainDialog* mainDialog; //optional

    //gauge variables
    int   totalObjects;   //file/dir/symlink/operation count
    Int64 totalData;      //unit: [bytes]
    int   currentObjects;
    Int64 currentData;

    //status variables
    size_t scannedObjects;
    wxString currentStatusText;

    SyncStatus::SyncStatusID currentStatus;
    SyncStatus::SyncStatusID previousStatus; //save old status if "currentStatus == SyncStatus::PAUSED"

    std::unique_ptr<Taskbar> taskbar_;

    //remaining time
    std::unique_ptr<Statistics> statistics;
    long lastStatCallSpeed;   //used for calculating intervals between statistics update
    long lastStatCallRemTime; //

    std::shared_ptr<GraphDataBytes> graphDataBytes;
    //std::shared_ptr<GraphDataConstLine> graphDataBytesCurrent;
    std::shared_ptr<GraphDataConstLine> graphDataBytesTotal;

    wxString titelTextBackup;

    std::unique_ptr<FfsTrayIcon> trayIcon; //optional: if filled all other windows should be hidden and conversely
};


SyncStatus::SyncStatusImpl::SyncStatusImpl(AbortCallback& abortCb,
                                           MainDialog* parentWindow,
                                           SyncStatusID startStatus,
                                           const wxString& jobName,
                                           const std::wstring& execWhenFinished,
                                           std::vector<std::wstring>& execFinishedHistory) :
    SyncStatusDlgGenerated(parentWindow,
                           wxID_ANY,
                           parentWindow ? wxString(wxEmptyString) : (wxString(wxT("FreeFileSync - ")) + _("Folder Comparison and Synchronization")),
                           wxDefaultPosition, wxSize(640, 350),
                           parentWindow ?
                           wxDEFAULT_FRAME_STYLE | wxTAB_TRAVERSAL | wxFRAME_NO_TASKBAR | wxFRAME_FLOAT_ON_PARENT : //wxTAB_TRAVERSAL is needed for standard button handling: wxID_OK/wxID_CANCEL
                           wxDEFAULT_FRAME_STYLE | wxTAB_TRAVERSAL),
    jobName_(jobName),
    abortCb_(&abortCb),
    mainDialog(parentWindow),
    totalObjects(0),
    totalData(0),
    currentObjects(0),
    currentData(0),
    scannedObjects(0),
    currentStatus (SyncStatus::ABORTED),
    previousStatus(SyncStatus::ABORTED),
    lastStatCallSpeed(-1000000), //some big number
    lastStatCallRemTime(-1000000)
{
#ifdef FFS_WIN
    new zen::MouseMoveWindow(*this); //allow moving main dialog by clicking (nearly) anywhere...; ownership passed to "this"
#endif

    if (mainDialog) //save old title (will be used as progress indicator)
        titelTextBackup = mainDialog->GetTitle();

    m_animationControl1->SetAnimation(GlobalResources::instance().animationSync);
    m_animationControl1->Play();

    //initialize gauge
    m_gauge1->SetRange(GAUGE_FULL_RANGE);
    m_gauge1->SetValue(0);


    EnableCloseButton(false);

    if (IsShown()) //don't steal focus when starting in sys-tray!
        m_buttonAbort->SetFocus();

    if (mainDialog)
        mainDialog->disableAllElements(false); //disable all child elements

    timeElapsed.Start(); //measure total time

    try //try to get access to Windows 7/Ubuntu taskbar
    {
        taskbar_.reset(new Taskbar(mainDialog != NULL ? *static_cast<wxTopLevelWindow*>(mainDialog) : *this));
    }
    catch (const TaskbarNotAvailable&) {}

    //hide "processed" statistics until end of process
    bSizerFinalStat      ->Show(false);
    m_buttonOK           ->Show(false);
    m_staticTextItemsProc->Show(false);
    bSizerItemsProc      ->Show(false);

    SetIcon(GlobalResources::instance().programIcon); //set application icon

    //register key event
    Connect(wxEVT_CHAR_HOOK, wxKeyEventHandler(SyncStatusImpl::OnKeyPressed), NULL, this);

    setCurrentStatus(startStatus); //first state: will be shown while waiting for dir locks (if at all)

    //init graph
    graphDataBytes        = std::make_shared<GraphDataBytes>();
    //graphDataBytesCurrent = std::make_shared<GraphDataConstLine>();
    graphDataBytesTotal   = std::make_shared<GraphDataConstLine>();

    m_panelGraph->setAttributes(Graph2D::GraphAttributes().
                                setLabelX(Graph2D::X_LABEL_BOTTOM, 20, std::make_shared<LabelFormatterTimeElapsed>()).
                                setLabelY(Graph2D::Y_LABEL_RIGHT,  60, std::make_shared<LabelFormatterBytes>()));

    m_panelGraph->setData(graphDataBytesTotal,   Graph2D::LineAttributes().setLineWidth(2).setColor(wxColor(0, 64, 0))); //green
    //m_panelGraph->addData(graphDataBytesCurrent, Graph2D::LineAttributes().setLineWidth(2).setColor(wxColor(0, 128, 0))); //green
    m_panelGraph->addData(graphDataBytes, Graph2D::LineAttributes().setLineWidth(2).setColor(wxColor(0, 192, 0))); //medium green

    //allow changing on completion command
    m_comboBoxExecFinished->setValue(execWhenFinished);
    m_comboBoxExecFinished->setHistoryRef(execFinishedHistory);

    //Fit() height only:
    //fitHeight(*this);
}


SyncStatus::SyncStatusImpl::~SyncStatusImpl()
{
    if (mainDialog)
    {
        mainDialog->enableAllElements();
        mainDialog->SetTitle(titelTextBackup); //restore title text
        mainDialog->Raise();
        mainDialog->SetFocus();
    }
}


void SyncStatus::SyncStatusImpl::OnKeyPressed(wxKeyEvent& event)
{
    const int keyCode = event.GetKeyCode();
    if (keyCode == WXK_ESCAPE)
    {
        wxCommandEvent dummy(wxEVT_COMMAND_BUTTON_CLICKED);

        //simulate click on abort button
        if (m_buttonAbort->IsShown()) //delegate to "abort" button if available
        {
            m_buttonAbort->GetEventHandler()->ProcessEvent(dummy);
            return;
        }
        else if (m_buttonOK->IsShown()) //delegate to "abort" button if available
        {
            m_buttonOK->GetEventHandler()->ProcessEvent(dummy);
            return;
        }
    }

    event.Skip();
}


void SyncStatus::SyncStatusImpl::initNewProcess(SyncStatusID id, int totalObjectsToProcess, Int64 totalDataToProcess)
{
    setCurrentStatus(id);

    currentData    = 0;
    currentObjects = 0;
    totalData      = totalDataToProcess;
    totalObjects   = totalObjectsToProcess;

    incProcessedData_NoUpdate(0, 0); //update graph
    incTotalData_NoUpdate    (0, 0); //

    //set new statistics handler: 10 seconds "window" for remaining time, 5 seconds for speed
    statistics.reset(new Statistics(totalObjectsToProcess, to<double>(totalDataToProcess), windowSizeRemainingTime, windowSizeBytesPerSec));

    lastStatCallSpeed   = -1000000; //some big number
    lastStatCallRemTime = -1000000;

    //set to 0 even if totalDataToProcess is 0: due to a bug in wxGauge::SetValue, it doesn't change to determinate mode when setting the old value again
    //so give updateStatusDialogNow() a chance to set a different value
    m_gauge1->SetValue(0);

    updateStatusDialogNow(false); //get rid of "dummy" texts!
}


void SyncStatus::SyncStatusImpl::incProcessedData_NoUpdate(int objectsDelta, Int64 dataDelta)
{
    currentData    +=    dataDelta;
    currentObjects += objectsDelta;

    //update graph data
    graphDataBytes->addCurrentValue(to<double>(currentData));
}


void SyncStatus::SyncStatusImpl::incTotalData_NoUpdate(int objectsDelta, Int64 dataDelta)
{
    totalData    +=    dataDelta;
    totalObjects += objectsDelta;

    if (statistics)
        statistics->setNewTotal(totalObjects, to<double>(totalData));

    graphDataBytesTotal->setValue(to<double>(totalData));
    //m_panelGraph->setAttributes(m_panelGraph->getAttributes().setMaxY(to<double>(totalDataToProcess)));
}


void SyncStatus::SyncStatusImpl::incScannedObjects_NoUpdate(int number)
{
    scannedObjects += number;
}


void SyncStatus::SyncStatusImpl::setStatusText_NoUpdate(const wxString& text)
{
    currentStatusText = text;
}


void SyncStatus::SyncStatusImpl::showProgressExternally(const wxString& progressText, double fraction)
{
    //write status information to systray, if window is minimized
    if (trayIcon.get())
        trayIcon->setToolTip2(progressText, fraction);

    wxString progressTextFmt = progressText;
    progressTextFmt.Replace(wxT("\n"), wxT(" - "));

    if (mainDialog) //show percentage in maindialog title (and thereby in taskbar)
    {
        if (mainDialog->GetTitle() != progressTextFmt)
            mainDialog->SetTitle(progressTextFmt);
    }
    else //show percentage in this dialog's title (and thereby in taskbar)
    {
        if (this->GetTitle() != progressTextFmt)
            this->SetTitle(progressTextFmt);
    }


    //show progress on Windows 7 taskbar
    if (taskbar_.get())
    {
        switch (currentStatus)
        {
            case SyncStatus::SCANNING:
                taskbar_->setStatus(Taskbar::STATUS_INDETERMINATE);
                break;
            case SyncStatus::FINISHED_WITH_SUCCESS:
            case SyncStatus::COMPARING_CONTENT:
            case SyncStatus::SYNCHRONIZING:
                taskbar_->setProgress(fraction);
                taskbar_->setStatus(Taskbar::STATUS_NORMAL);
                break;
            case SyncStatus::PAUSE:
                taskbar_->setProgress(fraction);
                taskbar_->setStatus(Taskbar::STATUS_PAUSED);
                break;
            case SyncStatus::ABORTED:
            case SyncStatus::FINISHED_WITH_ERROR:
                taskbar_->setProgress(fraction);
                taskbar_->setStatus(Taskbar::STATUS_ERROR);
                break;
        }
    }
}


#ifdef FFS_WIN
namespace
{
enum Zorder
{
    ZORDER_CORRECT,
    ZORDER_WRONG,
    ZORDER_INDEFINITE,
};

Zorder evaluateZorder(const wxWindow& top, const wxWindow& bottom)
{
    HWND hTop    = static_cast<HWND>(top.GetHWND());
    HWND hBottom = static_cast<HWND>(bottom.GetHWND());
    assert(hTop && hBottom);

    for (HWND hAbove = hBottom; hAbove; hAbove = ::GetNextWindow(hAbove, GW_HWNDPREV)) //GW_HWNDPREV means "to foreground"
        if (hAbove == hTop)
            return ZORDER_CORRECT;

    for (HWND hAbove = ::GetNextWindow(hTop, GW_HWNDPREV); hAbove; hAbove = ::GetNextWindow(hAbove, GW_HWNDPREV))
        if (hAbove == hBottom)
            return ZORDER_WRONG;

    return ZORDER_INDEFINITE;
}
}
#endif


void SyncStatus::SyncStatusImpl::updateStatusDialogNow(bool allowYield)
{
    //static RetrieveStatistics statistic;
    //statistic.writeEntry(currentData.ToDouble(), currentObjects);

    //add both data + obj-count, to handle "deletion-only" cases
    const double fraction = totalData + totalObjects == 0 ? 1 : std::max(0.0, to<double>(currentData + currentObjects) / to<double>(totalData + totalObjects));
    //yes, this may legitimately become < 0: failed rename operation falls-back to copy + delete, reducing "currentData" to potentially < 0!

    //write status information to systray, taskbar, parent title ect.

    const wxString postFix = jobName_.empty() ? wxString() : (wxT("\n\"") + jobName_ + wxT("\""));
    switch (currentStatus)
    {
        case SyncStatus::SCANNING:
            showProgressExternally(wxString() + toStringSep(scannedObjects) + wxT(" - ") + _("Scanning...") + postFix);
            break;
        case SyncStatus::COMPARING_CONTENT:
            showProgressExternally(wxString() + fractionToShortString(fraction) + wxT(" - ") + _("Comparing content...") + postFix, fraction);
            break;
        case SyncStatus::SYNCHRONIZING:
            showProgressExternally(wxString() + fractionToShortString(fraction) + wxT(" - ") + _("Synchronizing...") + postFix, fraction);
            break;
        case SyncStatus::PAUSE:
            showProgressExternally(wxString() + fractionToShortString(fraction) + wxT(" - ") + _("Paused") + postFix, fraction);
            break;
        case SyncStatus::ABORTED:
            showProgressExternally(_("Aborted") + postFix, fraction);
            break;
        case SyncStatus::FINISHED_WITH_SUCCESS:
        case SyncStatus::FINISHED_WITH_ERROR:
            showProgressExternally(_("Completed") + postFix, fraction);
            break;
    }

    //write regular status information (whether dialog is visible or not)
    {
        //wxWindowUpdateLocker dummy(this); -> not needed

        bool layoutChanged = false; //avoid screen flicker by calling layout() only if necessary

        //progress indicator
        switch (currentStatus)
        {
            case SyncStatus::SCANNING:
                m_gauge1->Pulse();
                break;
            case SyncStatus::COMPARING_CONTENT:
            case SyncStatus::SYNCHRONIZING:
            case SyncStatus::FINISHED_WITH_SUCCESS:
            case SyncStatus::FINISHED_WITH_ERROR:
            case SyncStatus::ABORTED:
                m_gauge1->SetValue(numeric::round(fraction * GAUGE_FULL_RANGE));
                break;
            case SyncStatus::PAUSE: //no change to gauge: don't switch between indeterminate/determinate modus
                break;
        }


        wxString statusTextFmt = currentStatusText;  //remove linebreaks
        replace(statusTextFmt, L'\n', L' ');         //

        //status text
        if (m_textCtrlInfo->GetValue() != statusTextFmt) //no layout update for status texts!
            m_textCtrlInfo->ChangeValue(statusTextFmt);

        //remaining objects
        const wxString remainingObjTmp = toStringSep(totalObjects - currentObjects);
        setText(*m_staticTextRemainingObj, remainingObjTmp, &layoutChanged);

        //remaining bytes left for copy
        const wxString remainingBytesTmp = zen::filesizeToShortString(totalData - currentData);
        setText(*m_staticTextDataRemaining, remainingBytesTmp, &layoutChanged);

        //statistics
        if (statistics.get())
        {
            if (timeElapsed.Time() - lastStatCallSpeed >= 500) //call method every 500 ms
            {
                lastStatCallSpeed = timeElapsed.Time();

                statistics->addMeasurement(currentObjects, to<double>(currentData));

                //current speed
                setText(*m_staticTextSpeed, statistics->getBytesPerSecond(), &layoutChanged);

                if (timeElapsed.Time() - lastStatCallRemTime >= 2000) //call method every two seconds only
                {
                    lastStatCallRemTime = timeElapsed.Time();

                    //remaining time
                    setText(*m_staticTextRemTime, statistics->getRemainingTime(), &layoutChanged);
                }
            }
        }

        {
            m_panelGraph->setAttributes(m_panelGraph->getAttributes().setMinX(graphDataBytes->getXBegin()));
            m_panelGraph->setAttributes(m_panelGraph->getAttributes().setMaxX(graphDataBytes->getXEnd()));

            m_panelGraph->Refresh();
        }

        //time elapsed
        const long timeElapSec = timeElapsed.Time() / 1000;
        setText(*m_staticTextTimeElapsed,
                timeElapSec < 3600 ?
                wxTimeSpan::Seconds(timeElapSec).Format(   L"%M:%S") :
                wxTimeSpan::Seconds(timeElapSec).Format(L"%H:%M:%S"), &layoutChanged);

        //do the ui update
        if (layoutChanged)
        {
            //            Layout();
            //            bSizerItemsRem->Layout();
            //            bSizer171->Layout();
            bSizerProgressStat->Layout(); //
            m_panelProgress->Layout();    //both needed
            //m_panelBackground->Layout(); //we use a dummy panel as actual background: replaces simple "Layout()" call
            //-> it seems this layout is not required, and even harmful: resets m_comboBoxExecFinished dropdown while user is selecting!
        }
    }

#ifdef FFS_WIN
    //workaround Windows 7 bug messing up z-order after temporary application hangs: https://sourceforge.net/tracker/index.php?func=detail&aid=3376523&group_id=234430&atid=1093080
    if (mainDialog)
        if (evaluateZorder(*this, *mainDialog) == ZORDER_WRONG)
        {
            HWND hProgress = static_cast<HWND>(GetHWND());

            if (::IsWindowVisible(hProgress))
            {
                ::ShowWindow(hProgress, SW_HIDE); //make Windows recalculate z-order
                ::ShowWindow(hProgress, SW_SHOW); //
            }
        }
#endif

    if (allowYield)
    {
        //support for pause button
        while (currentStatus == SyncStatus::PAUSE && currentProcessIsRunning())
        {
            wxMilliSleep(UI_UPDATE_INTERVAL);
            updateUiNow();
        }

        /*
            /|\
             |   keep this order to ensure one full statistics update before entering pause mode
            \|/
        */
        updateUiNow();
    }
}


bool SyncStatus::SyncStatusImpl::currentProcessIsRunning()
{
    return abortCb_ != NULL;
}


std::wstring SyncStatus::SyncStatusImpl::getExecWhenFinishedCommand() const
{
    return m_comboBoxExecFinished->getValue();
}


void SyncStatus::SyncStatusImpl::setCurrentStatus(SyncStatus::SyncStatusID id)
{
    switch (id)
    {
        case SyncStatus::ABORTED:
            m_bitmapStatus->SetBitmap(GlobalResources::getImage(wxT("statusError")));
            m_staticTextStatus->SetLabel(_("Aborted"));
            break;

        case SyncStatus::FINISHED_WITH_SUCCESS:
            m_bitmapStatus->SetBitmap(GlobalResources::getImage(wxT("statusSuccess")));
            m_staticTextStatus->SetLabel(_("Completed"));
            break;

        case SyncStatus::FINISHED_WITH_ERROR:
            m_bitmapStatus->SetBitmap(GlobalResources::getImage(wxT("statusWarning")));
            m_staticTextStatus->SetLabel(_("Completed"));
            break;

        case SyncStatus::PAUSE:
            m_bitmapStatus->SetBitmap(GlobalResources::getImage(wxT("statusPause")));
            m_staticTextStatus->SetLabel(_("Paused"));
            break;

        case SyncStatus::SCANNING:
            m_bitmapStatus->SetBitmap(GlobalResources::getImage(wxT("statusScanning")));
            m_staticTextStatus->SetLabel(_("Scanning..."));
            break;

        case SyncStatus::COMPARING_CONTENT:
            m_bitmapStatus->SetBitmap(GlobalResources::getImage(wxT("statusBinaryCompare")));
            m_staticTextStatus->SetLabel(_("Comparing content..."));
            break;

        case SyncStatus::SYNCHRONIZING:
            m_bitmapStatus->SetBitmap(GlobalResources::getImage(wxT("statusSyncing")));
            m_staticTextStatus->SetLabel(_("Synchronizing..."));
            break;
    }

    currentStatus = id;

    m_panelBackground->Layout(); //we use a dummy panel as actual background: replaces simple "Layout()" call
}


void SyncStatus::SyncStatusImpl::processHasFinished(SyncStatus::SyncStatusID id, const ErrorLogging& log) //essential to call this in StatusHandler derived class destructor
{
    //at the LATEST(!) to prevent access to currentStatusHandler
    //enable okay and close events; may be set in this method ONLY

    abortCb_ = NULL; //avoid callback to (maybe) deleted parent process

    setCurrentStatus(id);

    resumeFromSystray(); //if in tray mode...

    EnableCloseButton(true);

    m_buttonAbort->Disable();
    m_buttonAbort->Hide();
    m_buttonPause->Disable();
    m_buttonPause->Hide();
    m_buttonOK->Show();
    m_buttonOK->Enable();

    if (IsShown()) //don't steal focus when residing in sys-tray!
        m_buttonOK->SetFocus();

    m_animationControl1->Stop();
    m_animationControl1->Hide();

    //hide current operation status
    bSizerCurrentOperation->Show(false);

    bSizerExecFinished->Show(false);

    //show and prepare final statistics
    bSizerFinalStat->Show(true);

    if (totalObjects == currentObjects &&  //if everything was processed successfully
        totalData    == currentData)
    {
        m_staticTextItemsRem->Show(false);
        bSizerItemsRem      ->Show(false);
    }

    m_staticTextItemsProc->Show(true);
    bSizerItemsProc      ->Show(true);
    m_staticTextProcessedObj ->SetLabel(toStringSep(currentObjects));
    m_staticTextDataProcessed->SetLabel(zen::filesizeToShortString(currentData));

    m_staticTextRemTimeDescr->Show(false);
    m_staticTextRemTime     ->Show(false);

    updateStatusDialogNow(false); //keep this sequence to avoid display distortion, if e.g. only 1 item is sync'ed

    //changed meaning: overall speed: -> make sure to call after "updateStatusDialogNow"
    const long timeElapMs = timeElapsed.Time();
    m_staticTextSpeed->SetLabel(timeElapMs <= 0 ? L"-" : zen::filesizeToShortString(currentData * 1000 / timeElapMs) + _("/sec"));

    //fill result listbox:

    //workaround wxListBox bug on Windows XP: labels are drawn on top of each other
    assert(m_listbookResult->GetImageList()); //make sure listbook keeps *any* image list
    //due to some crazy reasons that aren't worth debugging, this needs to be done directly in wxFormBuilder,
    //the following call is *not* sufficient: m_listbookResult->AssignImageList(new wxImageList(0, 0));
    //note: alternative solutions involving wxLC_LIST, wxLC_REPORT and SetWindowStyleFlag() do not work portably! wxListBook using wxLC_ICON is obviously a class invariant!

    //1. re-arrange graph into results listbook
    bSizerTop->Detach(m_panelProgress);
    m_panelProgress->Reparent(m_listbookResult);
    m_listbookResult->AddPage(m_panelProgress, _("Statistics"), true); //AddPage() takes ownership!

    //2. log file
    LogControl* logControl = new LogControl(m_listbookResult, log);
    m_listbookResult->AddPage(logControl, _("Logging"), false);
    //bSizerHoldStretch->Insert(0, logControl, 1, wxEXPAND);

    m_panelBackground->Layout(); //we use a dummy panel as actual background: replaces simple "Layout()" call

    //Raise(); -> don't! user may be watching a movie in the meantime ;)
}


void SyncStatus::SyncStatusImpl::OnOkay(wxCommandEvent& event)
{
    Close(); //generate close event: do NOT destroy window unconditionally!
}


void SyncStatus::SyncStatusImpl::OnAbort(wxCommandEvent& event)
{
    if (currentStatus == SyncStatus::PAUSE)
    {
        wxCommandEvent dummy;
        OnPause(dummy);
    }

    if (currentProcessIsRunning())
    {
        m_buttonAbort->Disable();
        m_buttonAbort->Hide();
        m_buttonPause->Disable();
        m_buttonPause->Hide();

        setStatusText_NoUpdate(_("Abort requested: Waiting for current operation to finish..."));
        //no Layout() or UI-update here to avoid cascaded Yield()-call

        abortCb_->requestAbortion();
    }
}


void SyncStatus::SyncStatusImpl::stopTimer()
{
    timeElapsed.Pause();
    if (statistics.get()) statistics->pauseTimer();
    graphDataBytes->pauseTimer();
}


void SyncStatus::SyncStatusImpl::resumeTimer()
{
    timeElapsed.Resume();
    if (statistics.get()) statistics->resumeTimer();
    graphDataBytes->resumeTimer();
}


void SyncStatus::SyncStatusImpl::OnPause(wxCommandEvent& event)
{
    if (currentStatus == SyncStatus::PAUSE)
    {
        resumeTimer();
        setCurrentStatus(previousStatus);

        m_buttonPause->SetLabel(_("Pause"));
        m_animationControl1->Play();
    }
    else
    {
        stopTimer();
        previousStatus = currentStatus; //save current status
        setCurrentStatus(SyncStatus::PAUSE);

        m_buttonPause->SetLabel(_("Continue"));
        m_animationControl1->Stop();
    }
}


void SyncStatus::SyncStatusImpl::OnClose(wxCloseEvent& event)
{
    //this event handler may be called due to a system shutdown DURING synchronization!
    //try to stop sync gracefully and cross fingers:
    if (currentProcessIsRunning())
        abortCb_->requestAbortion();
    //Note: we must NOT veto dialog destruction, else we will cancel system shutdown if this dialog is application main window (like in batch mode)

    Destroy();
}


void SyncStatus::SyncStatusImpl::OnIconize(wxIconizeEvent& event)
{
    if (event.IsIconized()) //ATTENTION: iconize event is also triggered on "Restore"! (at least under Linux)
        minimizeToTray();
    else
        resumeFromSystray(); //may be initiated by "show desktop" although all windows are hidden!
}


void SyncStatus::SyncStatusImpl::OnResumeFromTray(wxCommandEvent& event)
{
    resumeFromSystray();
}


void SyncStatus::SyncStatusImpl::minimizeToTray()
{
    if (!trayIcon.get())
    {
        trayIcon.reset(new FfsTrayIcon);
        trayIcon->Connect(FFS_REQUEST_RESUME_TRAY_EVENT, wxCommandEventHandler(SyncStatus::SyncStatusImpl::OnResumeFromTray), NULL, this);
        //tray icon has shorter lifetime than this => no need to disconnect event later
    }

    updateStatusDialogNow(false); //set tooltip: in pause mode there is no statistics update, so this is the only chance

    Hide();
    if (mainDialog)
        mainDialog->Hide();
}


void SyncStatus::SyncStatusImpl::resumeFromSystray()
{
    trayIcon.reset();

    if (mainDialog)
    {
        if (mainDialog->IsIconized()) //caveat: if window is maximized calling Iconize(false) will erroneously un-maximize!
            mainDialog->Iconize(false);
        mainDialog->Show();
        mainDialog->Raise();
    }

    if (IsIconized()) //caveat: if window is maximized calling Iconize(false) will erroneously un-maximize!
        Iconize(false);
    Show();
    Raise();
    SetFocus();

    updateStatusDialogNow(false); //restore Windows 7 task bar status (e.g. required in pause mode)
}


//########################################################################################


//redirect to implementation
SyncStatus::SyncStatus(AbortCallback& abortCb,
                       MainDialog* parentWindow,
                       SyncStatusID startStatus,
                       bool showProgress,
                       const wxString& jobName,
                       const std::wstring& execWhenFinished,
                       std::vector<std::wstring>& execFinishedHistory) :
    pimpl(new SyncStatusImpl(abortCb, parentWindow, startStatus, jobName, execWhenFinished, execFinishedHistory))
{
    if (showProgress)
    {
        pimpl->Show();
        pimpl->updateStatusDialogNow(false); //update visual statistics to get rid of "dummy" texts
    }
    else
        pimpl->minimizeToTray();
}

SyncStatus::~SyncStatus()
{
    //DON'T delete pimpl! it will be deleted by the user clicking "OK/Cancel" -> (wxWindow::Destroy())
}

wxWindow* SyncStatus::getAsWindow()
{
    return pimpl;
}

void SyncStatus::initNewProcess(SyncStatusID id, int totalObjectsToProcess, Int64 totalDataToProcess)
{
    pimpl->initNewProcess(id, totalObjectsToProcess, totalDataToProcess);
}

void SyncStatus::incScannedObjects_NoUpdate(int number)
{
    pimpl->incScannedObjects_NoUpdate(number);
}

void SyncStatus::incProcessedData_NoUpdate(int objectsDelta, Int64 dataDelta)
{
    pimpl->incProcessedData_NoUpdate(objectsDelta, dataDelta);
}

void SyncStatus::incTotalData_NoUpdate(int objectsDelta, Int64 dataDelta)
{
    pimpl->incTotalData_NoUpdate(objectsDelta, dataDelta);
}

void SyncStatus::setStatusText_NoUpdate(const wxString& text)
{
    pimpl->setStatusText_NoUpdate(text);
}

void SyncStatus::updateStatusDialogNow()
{
    pimpl->updateStatusDialogNow();
}

std::wstring SyncStatus::getExecWhenFinishedCommand() const
{
    return pimpl->getExecWhenFinishedCommand();
}

void SyncStatus::stopTimer()
{
    return pimpl->stopTimer();
}

void SyncStatus::resumeTimer()
{
    return pimpl->resumeTimer();
}

void SyncStatus::processHasFinished(SyncStatusID id, const ErrorLogging& log)
{
    pimpl->processHasFinished(id, log);
}

void SyncStatus::closeWindowDirectly() //don't wait for user (silent mode)
{
    pimpl->Destroy();
}
