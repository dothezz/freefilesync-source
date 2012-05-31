// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) ZenJu (zhnmju123 AT gmx DOT de) - All Rights Reserved    *
// **************************************************************************

#include "progress_indicator.h"
#include <memory>
#include <wx/imaglist.h>
#include <wx/stopwatch.h>
#include <wx/wupdlock.h>
#include <zen/basic_math.h>
#include <wx+/mouse_move_dlg.h>
#include <wx+/toggle_button.h>
#include <wx+/format_unit.h>
#include <wx+/image_tools.h>
#include <wx+/graph.h>
#include <wx+/no_flicker.h>
#include "gui_generated.h"
#include "../lib/resources.h"
#include "../lib/perf_check.h"
#include "tray_icon.h"
#include "taskbar.h"
#include "exec_finished_box.h"

using namespace zen;


namespace
{
const int GAUGE_FULL_RANGE = 50000;

//window size used for statistics in milliseconds
const int WINDOW_REMAINING_TIME = 60000; //some usecases have dropouts of 40 seconds -> 60 sec. window size handles them well
const int WINDOW_BYTES_PER_SEC  =  5000; //
}


class CompareStatus::CompareStatusImpl : public CompareStatusGenerated
{
public:
    CompareStatusImpl(wxTopLevelWindow& parentWindow);

    void init(const Statistics& syncStat); //constructor/destructor semantics, but underlying Window is reused
    void finalize(); //

    void switchToCompareBytewise();
    void updateStatusPanelNow();

private:
    wxTopLevelWindow& parentWindow_;
    wxString titleTextBackup;

    wxStopWatch timeElapsed;

    const Statistics* syncStat_; //only bound while sync is running

    std::unique_ptr<Taskbar> taskbar_;
    std::unique_ptr<PerfCheck> perf; //estimate remaining time

    long lastStatCallSpeed;   //used for calculating intervals between showing and collecting perf samples
    long lastStatCallRemTime; //
};


CompareStatus::CompareStatusImpl::CompareStatusImpl(wxTopLevelWindow& parentWindow) :
    CompareStatusGenerated(&parentWindow),
    parentWindow_(parentWindow),
    syncStat_(nullptr),
    lastStatCallSpeed  (-1000000), //some big number
    lastStatCallRemTime(-1000000)
{
    //init(); -> needed?
}


void CompareStatus::CompareStatusImpl::init(const Statistics& syncStat)
{
    syncStat_ = &syncStat;
    titleTextBackup = parentWindow_.GetTitle();

    try //try to get access to Windows 7/Ubuntu taskbar
    {
        taskbar_.reset(new Taskbar(parentWindow_));
    }
    catch (const TaskbarNotAvailable&) {}

    //initialize gauge
    m_gauge2->SetRange(GAUGE_FULL_RANGE);
    m_gauge2->SetValue(0);

    perf.reset();
    timeElapsed.Start(); //measure total time

    //initially hide status that's relevant for comparing bytewise only
    bSizerFilesFound    ->Show(true);
    bSizerFilesRemaining->Show(false);
    sSizerSpeed         ->Show(false);
    sSizerTimeRemaining ->Show(false);

    updateStatusPanelNow();

    m_gauge2->Hide();
    bSizer42->Layout();
    Layout();
}


void CompareStatus::CompareStatusImpl::finalize()
{
    syncStat_ = nullptr;
    parentWindow_.SetTitle(titleTextBackup);
    taskbar_.reset();
}


void CompareStatus::CompareStatusImpl::switchToCompareBytewise()
{
    //start to measure perf
    perf.reset(new PerfCheck(WINDOW_REMAINING_TIME, WINDOW_BYTES_PER_SEC));
    lastStatCallSpeed   = -1000000; //some big number
    lastStatCallRemTime = -1000000;

    //show status for comparing bytewise
    bSizerFilesFound    ->Show(false);
    bSizerFilesRemaining->Show(true);
    sSizerSpeed         ->Show(true);
    sSizerTimeRemaining ->Show(true);

    m_gauge2->Show();
    bSizer42->Layout();
    Layout();
}


void CompareStatus::CompareStatusImpl::updateStatusPanelNow()
{
    if (!syncStat_) //no comparison running!!
        return;

    //wxWindowUpdateLocker dummy(this) -> not needed

    const wxString& scannedObjects = toGuiString(syncStat_->getObjectsCurrent(ProcessCallback::PHASE_SCANNING));

    auto setTitle = [&](const wxString& title)
    {
        if (parentWindow_.GetTitle() != title)
            parentWindow_.SetTitle(title);
    };

    bool layoutChanged = false; //avoid screen flicker by calling layout() only if necessary

    //status texts
    setText(*m_textCtrlStatus, replaceCpy(syncStat_->currentStatusText(), L'\n', L' ')); //no layout update for status texts!

    //write status information to taskbar, parent title ect.
    switch (syncStat_->currentPhase())
    {
        case ProcessCallback::PHASE_NONE:
        case ProcessCallback::PHASE_SCANNING:
            //dialog caption, taskbar
            setTitle(scannedObjects + L" - " + _("Scanning..."));
            if (taskbar_.get()) //support Windows 7 taskbar
                taskbar_->setStatus(Taskbar::STATUS_INDETERMINATE);
            break;

        case ProcessCallback::PHASE_COMPARING_CONTENT:
        {
            auto objectsCurrent = syncStat_->getObjectsCurrent(ProcessCallback::PHASE_COMPARING_CONTENT);
            auto objectsTotal   = syncStat_->getObjectsTotal  (ProcessCallback::PHASE_COMPARING_CONTENT);
            auto dataCurrent    = syncStat_->getDataCurrent   (ProcessCallback::PHASE_COMPARING_CONTENT);
            auto dataTotal      = syncStat_->getDataTotal     (ProcessCallback::PHASE_COMPARING_CONTENT);

            //add both data + obj-count, to handle "deletion-only" cases
            const double fraction = dataTotal + objectsTotal == 0 ? 0 : std::max(0.0, to<double>(dataCurrent + objectsCurrent) / to<double>(dataTotal + objectsTotal));

            //dialog caption, taskbar
            setTitle(fractionToShortString(fraction) + wxT(" - ") + _("Comparing content..."));
            if (taskbar_.get())
            {
                taskbar_->setProgress(fraction);
                taskbar_->setStatus(Taskbar::STATUS_NORMAL);
            }

            //progress indicator, shown for binary comparison only
            m_gauge2->SetValue(numeric::round(fraction * GAUGE_FULL_RANGE));

            //remaining objects and bytes for file comparison
            setText(*m_staticTextFilesRemaining, toGuiString(objectsTotal - objectsCurrent), &layoutChanged);
            setText(*m_staticTextDataRemaining, L"(" + filesizeToShortString(dataTotal - dataCurrent) + L")", &layoutChanged);

            //remaining time and speed: only visible during binary comparison
            if (perf)
                if (timeElapsed.Time() - lastStatCallSpeed >= 500) //-> Win 7 copy uses 1 sec update interval
                {
                    lastStatCallSpeed = timeElapsed.Time();

                    perf->addSample(objectsCurrent, to<double>(dataCurrent), timeElapsed.Time());

                    //current speed
                    setText(*m_staticTextSpeed, perf->getBytesPerSecond(), &layoutChanged);

                    if (timeElapsed.Time() - lastStatCallRemTime >= 2000) //update GUI every 2 sec
                    {
                        lastStatCallRemTime = timeElapsed.Time();

                        //remaining time
                        setText(*m_staticTextRemTime, perf->getRemainingTime(to<double>(dataTotal - dataCurrent)), &layoutChanged);
                    }
                }
        }
        break;

        case ProcessCallback::PHASE_SYNCHRONIZING:
            assert(false);
            break;
    }

    //nr of scanned objects
    setText(*m_staticTextScanned, scannedObjects, &layoutChanged);

    //time elapsed
    const long timeElapSec = timeElapsed.Time() / 1000;
    setText(*m_staticTextTimeElapsed,
            timeElapSec < 3600 ?
            wxTimeSpan::Seconds(timeElapSec).Format(   L"%M:%S") :
            wxTimeSpan::Seconds(timeElapSec).Format(L"%H:%M:%S"), &layoutChanged);

    //do the ui update
    if (layoutChanged)
        bSizer42->Layout();

    updateUiNow();
}
//########################################################################################

//redirect to implementation
CompareStatus::CompareStatus(wxTopLevelWindow& parentWindow) :
    pimpl(new CompareStatusImpl(parentWindow)) {} //owned by parentWindow

wxWindow* CompareStatus::getAsWindow()
{
    return pimpl;
}

void CompareStatus::init(const Statistics& syncStat)
{
    pimpl->init(syncStat);
}

void CompareStatus::finalize()
{
    pimpl->finalize();
}

void CompareStatus::switchToCompareBytewise()
{
    pimpl->switchToCompareBytewise();
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
    return layOver(GlobalResources::getImage(utfCvrtTo<wxString>(name)), background);
}


inline
wxBitmap buttonReleased(const std::string& name)
{
    wxImage output = greyScale(GlobalResources::getImage(utfCvrtTo<wxString>(name))).ConvertToImage();
    //GlobalResources::getImage(utfCvrtTo<wxString>(name)).ConvertToImage().ConvertToGreyscale(1.0/3, 1.0/3, 1.0/3); //treat all channels equally!
    //brighten(output, 30);

    zen::move(output, 0, -1); //move image right one pixel
    return output;
}
}


class LogControl : public LogControlGenerated
{
public:
    LogControl(wxWindow* parent, const ErrorLog& log) : LogControlGenerated(parent), log_(log)
    {
        const int errorCount   = log_.getItemCount(TYPE_ERROR | TYPE_FATAL_ERROR);
        const int warningCount = log_.getItemCount(TYPE_WARNING);
        const int infoCount    = log_.getItemCount(TYPE_INFO);

        m_bpButtonErrors  ->init(buttonPressed ("error"  ), buttonReleased("error"  ), _("Error"  ) + wxString::Format(L" (%d)", errorCount  ));
        m_bpButtonWarnings->init(buttonPressed ("warning"), buttonReleased("warning"), _("Warning") + wxString::Format(L" (%d)", warningCount));
        m_bpButtonInfo    ->init(buttonPressed ("info"   ), buttonReleased("info"   ), _("Info"   ) + wxString::Format(L" (%d)", infoCount   ));

        m_bpButtonErrors  ->setActive(true);
        m_bpButtonWarnings->setActive(true);
        m_bpButtonInfo    ->setActive(errorCount + warningCount == 0);

        m_bpButtonErrors  ->Show(errorCount   != 0);
        m_bpButtonWarnings->Show(warningCount != 0);
        m_bpButtonInfo    ->Show(infoCount    != 0);

        m_textCtrlInfo->SetMaxLength(0); //allow large entries!

        updateLogText();

        m_textCtrlInfo->Connect(wxEVT_KEY_DOWN, wxKeyEventHandler(LogControl::onKeyEvent), nullptr, this);
    }

private:
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

        //fast replacement for wxString modelling exponential growth
        typedef Zbase<wchar_t> zxString;
        zxString logText;

        const auto& entries = log_.getEntries();
        for (auto iter = entries.begin(); iter != entries.end(); ++iter)
            if (iter->type & includedTypes)
            {
                logText += copyStringTo<zxString>(formatMessage(*iter));
                logText += L'\n';
            }

        if (logText.empty()) //if no messages match selected view filter, at least show final status message
            if (!entries.empty())
                logText = copyStringTo<zxString>(formatMessage(entries.back()));

        wxWindowUpdateLocker dummy(m_textCtrlInfo);
        m_textCtrlInfo->ChangeValue(copyStringTo<wxString>(logText));
        m_textCtrlInfo->ShowPosition(m_textCtrlInfo->GetLastPosition());
    }

    const ErrorLog log_;
};

//########################################################################################

namespace
{
class GraphDataBytes : public GraphData
{
public:
    void addRecord(double dataCurrent, long timeMs)
    {
        data.insert(data.end(), std::make_pair(timeMs, dataCurrent));
        //documentation differs about whether "hint" should be before or after the to be inserted element!
        //however "std::map<>::end()" is interpreted correctly by GCC and VS2010

        if (data.size() > MAX_BUFFER_SIZE) //limit buffer size
            data.erase(data.begin());
    }

    void clear() { data.clear(); }

    virtual double getXBegin() const { return data.empty() ? 0 : data.begin()->first / 1000.0; } //need not start with 0, e.g. "binary comparison, graph reset, followed by sync"
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
        if (secProposed <= 30)
            return bestFit(secProposed, 20, 30);
        if (secProposed <= 60)
            return bestFit(secProposed, 30, 60);

        //for minutes: nice numbers are 1, 2, 5, 10, 15, 20, 30
        auto calcBlock = [](double val) -> double
        {
            if (val <= 2)
                return bestFit(val, 1, 2); //
            if (val <= 5)
                return bestFit(val, 2, 5); //
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
        if (secProposed <= 3600)
            return calcBlock(secProposed / 60) * 60;

        if (secProposed <= 3600 * 24)
            return nextNiceNumber(secProposed / 3600) * 3600;

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
                   const Statistics& syncStat,
                   MainDialog* parentWindow,
                   const wxString& jobName,
                   const std::wstring& execWhenFinished,
                   std::vector<std::wstring>& execFinishedHistory);
    ~SyncStatusImpl();

    void initNewPhase();
    void reportCurrentBytes(Int64 currentData);
    void updateProgress(bool allowYield = true);

    //call this in StatusUpdater derived class destructor at the LATEST(!) to prevent access to currentStatusUpdater
    void processHasFinished(SyncResult resultId, const ErrorLog& log);
    void closeWindowDirectly();

    std::wstring getExecWhenFinishedCommand() const;

    void stopTimer() //halt all internal counters!
    {
        m_animationControl1->Stop();
        timeElapsed.Pause ();
    }
    void resumeTimer()
    {
        m_animationControl1->Play();
        timeElapsed.Resume();
    }

    void minimizeToTray();

private:
    void OnKeyPressed(wxKeyEvent& event);
    virtual void OnOkay   (wxCommandEvent& event);
    virtual void OnPause  (wxCommandEvent& event);
    virtual void OnAbort  (wxCommandEvent& event);
    virtual void OnClose  (wxCloseEvent& event);
    virtual void OnIconize(wxIconizeEvent& event);

    void updateDialogStatus();

    void resumeFromSystray();
    void OnResumeFromTray(wxCommandEvent& event);

    void setExternalStatus(const wxString& status, const wxString& progress); //progress may be empty!

    const wxString jobName_;
    wxStopWatch timeElapsed;

    MainDialog* mainDialog; //optional

    //status variables
    AbortCallback*    abortCb_;  //temporarily bound while sync is running
    const Statistics* syncStat_; //
    bool paused_; //valid only while sync is running
    SyncResult finalResult; //set after sync

    bool isZombie; //wxGTK sends iconize event *after* wxWindow::Destroy, sigh...

    //remaining time
    std::unique_ptr<PerfCheck> perf;
    long lastStatCallSpeed;   //used for calculating intervals between collecting perf samples
    long lastStatCallRemTime; //

    std::shared_ptr<GraphDataBytes> graphDataBytes;
    std::shared_ptr<GraphDataConstLine> graphDataBytesTotal;

    wxString titelTextBackup;
    std::unique_ptr<FfsTrayIcon> trayIcon; //optional: if filled all other windows should be hidden and conversely
    std::unique_ptr<Taskbar> taskbar_;
};


SyncStatus::SyncStatusImpl::SyncStatusImpl(AbortCallback& abortCb,
                                           const Statistics& syncStat,
                                           MainDialog* parentWindow,
                                           const wxString& jobName,
                                           const std::wstring& execWhenFinished,
                                           std::vector<std::wstring>& execFinishedHistory) :
    SyncStatusDlgGenerated(parentWindow,
                           wxID_ANY,
                           parentWindow ? wxString() : (wxString(L"FreeFileSync - ") + _("Folder Comparison and Synchronization")),
                           wxDefaultPosition, wxSize(640, 350),
                           parentWindow ?
                           wxDEFAULT_FRAME_STYLE | wxTAB_TRAVERSAL | wxFRAME_NO_TASKBAR | wxFRAME_FLOAT_ON_PARENT : //wxTAB_TRAVERSAL is needed for standard button handling: wxID_OK/wxID_CANCEL
                           wxDEFAULT_FRAME_STYLE | wxTAB_TRAVERSAL),
    jobName_  (jobName),
    mainDialog(parentWindow),
    abortCb_  (&abortCb),
    syncStat_ (&syncStat),
    paused_   (false),
    finalResult(RESULT_ABORTED), //dummy value
    isZombie(false),
    lastStatCallSpeed  (-1000000), //some big number
    lastStatCallRemTime(-1000000)
{
#ifdef FFS_WIN
    new MouseMoveWindow(*this); //allow moving main dialog by clicking (nearly) anywhere...; ownership passed to "this"
#endif

    if (mainDialog)
    {
        titelTextBackup = mainDialog->GetTitle(); //save old title (will be used as progress indicator)
        mainDialog->disableAllElements(false); //disable all child elements
    }

    m_animationControl1->SetAnimation(GlobalResources::instance().animationSync);
    m_animationControl1->Play();

    SetIcon(GlobalResources::instance().programIcon);

    //initialize gauge
    m_gauge1->SetRange(GAUGE_FULL_RANGE);
    m_gauge1->SetValue(0);

    EnableCloseButton(false);

    if (IsShown()) //don't steal focus when starting in sys-tray!
        m_buttonAbort->SetFocus();

    timeElapsed.Start(); //measure total time

    try //try to get access to Windows 7/Ubuntu taskbar
    {
        taskbar_.reset(new Taskbar(mainDialog ? *static_cast<wxTopLevelWindow*>(mainDialog) : *this));
    }
    catch (const TaskbarNotAvailable&) {}

    //hide "processed" statistics until end of process
    bSizerFinalStat           ->Show(false);
    m_staticTextLabelItemsProc->Show(false);
    bSizerItemsProc           ->Show(false);
    m_buttonOK                ->Show(false);
    m_panelFooter->Layout();

    //register key event
    Connect(wxEVT_CHAR_HOOK, wxKeyEventHandler(SyncStatusImpl::OnKeyPressed), nullptr, this);

    //init graph
    graphDataBytes      = std::make_shared<GraphDataBytes>();
    graphDataBytesTotal = std::make_shared<GraphDataConstLine>();

    m_panelGraph->setAttributes(Graph2D::GraphAttributes().
                                setLabelX(Graph2D::X_LABEL_BOTTOM, 20, std::make_shared<LabelFormatterTimeElapsed>()).
                                setLabelY(Graph2D::Y_LABEL_RIGHT,  60, std::make_shared<LabelFormatterBytes>()));

    m_panelGraph->setData(graphDataBytesTotal, Graph2D::LineAttributes().setLineWidth(2).setColor(wxColor(0,  64, 0))); //green
    m_panelGraph->addData(graphDataBytes,      Graph2D::LineAttributes().setLineWidth(2).setColor(wxColor(0, 192, 0))); //medium green

    //allow changing on completion command
    m_comboBoxExecFinished->setValue     (execWhenFinished);
    m_comboBoxExecFinished->setHistoryRef(execFinishedHistory);

    updateDialogStatus(); //null-status will be shown while waiting for dir locks (if at all)

    Fit();
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
            if (wxEvtHandler* handler = m_buttonAbort->GetEventHandler())
                handler->ProcessEvent(dummy);
            return;
        }
        else if (m_buttonOK->IsShown())
        {
            if (wxEvtHandler* handler = m_buttonOK->GetEventHandler())
                handler->ProcessEvent(dummy);
            return;
        }
    }

    event.Skip();
}


void SyncStatus::SyncStatusImpl::initNewPhase()
{
    updateDialogStatus(); //evaluates "syncStat_->currentPhase()"

    //reset graph (e.g. after binary comparison)
    graphDataBytes->clear();
    reportCurrentBytes(0);

    //start new measurement
    perf.reset(new PerfCheck(WINDOW_REMAINING_TIME, WINDOW_BYTES_PER_SEC));
    lastStatCallSpeed   = -1000000; //some big number
    lastStatCallRemTime = -1000000;

    //set to 0 even if totalDataToProcess is 0: due to a bug in wxGauge::SetValue, it doesn't change to determinate mode when setting the old value again
    //so give updateProgress() a chance to set a different value
    m_gauge1->SetValue(0);

    updateProgress(false);
}


void SyncStatus::SyncStatusImpl::reportCurrentBytes(Int64 currentData)
{
    //add sample for perf measurements + calc. of remaining time
    graphDataBytes->addRecord(to<double>(currentData), timeElapsed.Time());
}


namespace
{
#ifdef FFS_WIN
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
#endif


std::wstring getDialogStatusText(const Statistics* syncStat, bool paused, SyncStatus::SyncResult finalResult)
{
    if (syncStat) //sync running
    {
        if (paused)
            return _("Paused");
        else
            switch (syncStat->currentPhase())
            {
                case ProcessCallback::PHASE_NONE:
                    return _("Initializing..."); //dialog is shown *before* sync starts, so this text may be visible!
                case ProcessCallback::PHASE_SCANNING:
                    return _("Scanning...");
                case ProcessCallback::PHASE_COMPARING_CONTENT:
                    return _("Comparing content...");
                case ProcessCallback::PHASE_SYNCHRONIZING:
                    return _("Synchronizing...");
            }
    }
    else //sync finished
        switch (finalResult)
        {
            case SyncStatus::RESULT_ABORTED:
                return _("Aborted");
            case SyncStatus::RESULT_FINISHED_WITH_ERROR:
            case SyncStatus::RESULT_FINISHED_WITH_SUCCESS:
                return _("Completed");
        }
    return std::wstring();
}
}


void SyncStatus::SyncStatusImpl::setExternalStatus(const wxString& status, const wxString& progress) //progress may be empty!
{
    //sys tray: order "top-down": jobname, status, progress
    wxString newTrayInfo = jobName_.empty() ? status : L"\"" + jobName_ + L"\"\n" + status;
    if (!progress.empty())
        newTrayInfo += L" " + progress;

    //window caption/taskbar; inverse order: progress, status, jobname
    wxString newCaption = progress.empty() ? status : progress + L" - " + status;
    if (!jobName_.empty())
        newCaption += L" - \"" + jobName_ + L"\"";

    //systray tooltip, if window is minimized
    if (trayIcon.get())
        trayIcon->setToolTip(newTrayInfo);

    //show text in dialog title (and at the same time in taskbar)
    if (mainDialog)
    {
        if (mainDialog->GetTitle() != newCaption)
            mainDialog->SetTitle(newCaption);
    }

    //always set a title: we don't wxGTK to show "nameless window" instead
    if (this->GetTitle() != newCaption)
        this->SetTitle(newCaption);
}


void SyncStatus::SyncStatusImpl::updateProgress(bool allowYield)
{
    assert(syncStat_);
    if (!syncStat_) //no sync running!!
        return;

    //wxWindowUpdateLocker dummy(this); -> not needed

    bool layoutChanged = false; //avoid screen flicker by calling layout() only if necessary

    //sync status text
    setText(*m_textCtrlStatus, replaceCpy(syncStat_->currentStatusText(), L'\n', L' ')); //no layout update for status texts!

    switch (syncStat_->currentPhase()) //no matter if paused or not
    {
        case ProcessCallback::PHASE_NONE:
        case ProcessCallback::PHASE_SCANNING:
            //dialog caption, taskbar, systray tooltip
            setExternalStatus(getDialogStatusText(syncStat_, paused_, finalResult), toGuiString(syncStat_->getObjectsCurrent(ProcessCallback::PHASE_SCANNING))); //status text may be "paused"!

            //progress indicators
            m_gauge1->Pulse();
            if (trayIcon.get()) trayIcon->setProgress(1); //1 = regular FFS logo

            //taskbar_ status is Taskbar::STATUS_INDETERMINATE

            //constant line graph
            graphDataBytesTotal->setValue(0);

            //remaining objects and data
            setText(*m_staticTextRemainingObj , L"-", &layoutChanged);
            setText(*m_staticTextDataRemaining, L"", &layoutChanged);

            //remaining time and speed
            setText(*m_staticTextSpeed,   L"-", &layoutChanged);
            setText(*m_staticTextRemTime, L"-", &layoutChanged);
            break;

        case ProcessCallback::PHASE_COMPARING_CONTENT:
        case ProcessCallback::PHASE_SYNCHRONIZING:
        {
            auto objectsCurrent = syncStat_->getObjectsCurrent(syncStat_->currentPhase());
            auto objectsTotal   = syncStat_->getObjectsTotal  (syncStat_->currentPhase());
            auto dataCurrent    = syncStat_->getDataCurrent   (syncStat_->currentPhase());
            auto dataTotal      = syncStat_->getDataTotal     (syncStat_->currentPhase());

            //add both data + obj-count, to handle "deletion-only" cases
            const double fraction = dataTotal + objectsTotal == 0 ? 1 : std::max(0.0, to<double>(dataCurrent + objectsCurrent) / to<double>(dataTotal + objectsTotal));
            //yes, this may legitimately become < 0: failed rename operation falls-back to copy + delete, reducing "dataCurrent" to potentially < 0!
            //----------------------------------------------------------------------------------------------------

            //dialog caption, taskbar, systray tooltip
            setExternalStatus(getDialogStatusText(syncStat_, paused_, finalResult), fractionToShortString(fraction)); //status text may be "paused"!

            //progress indicators
            m_gauge1->SetValue(numeric::round(fraction * GAUGE_FULL_RANGE));
            if (trayIcon.get()) trayIcon->setProgress(fraction);
            if (taskbar_.get()) taskbar_->setProgress(fraction);

            //constant line graph
            graphDataBytesTotal->setValue(to<double>(dataTotal));

            //remaining objects and data
            setText(*m_staticTextRemainingObj, toGuiString(objectsTotal - objectsCurrent), &layoutChanged);
            setText(*m_staticTextDataRemaining, L"(" + filesizeToShortString(dataTotal - dataCurrent) + L")", &layoutChanged);

            //remaining time and speed
            assert(perf);
            if (perf)
                if (timeElapsed.Time() - lastStatCallSpeed >= 500) //-> Win 7 copy uses 1 sec update interval
                {
                    lastStatCallSpeed = timeElapsed.Time();

                    perf->addSample(objectsCurrent, to<double>(dataCurrent), timeElapsed.Time());

                    //current speed
                    setText(*m_staticTextSpeed, perf->getBytesPerSecond(), &layoutChanged);

                    if (timeElapsed.Time() - lastStatCallRemTime >= 2000) //update GUI every 2 sec
                    {
                        lastStatCallRemTime = timeElapsed.Time();

                        //remaining time
                        setText(*m_staticTextRemTime, perf->getRemainingTime(to<double>(dataTotal - dataCurrent)), &layoutChanged);
                    }
                }
        }
        break;
    }

    m_panelGraph->setAttributes(m_panelGraph->getAttributes().setMinX(graphDataBytes->getXBegin()));
    m_panelGraph->setAttributes(m_panelGraph->getAttributes().setMaxX(graphDataBytes->getXEnd()));
    m_panelGraph->Refresh();

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
        //bSizerProgressStat->Layout(); //
        m_panelProgress->Layout();    //both needed
        //m_panelBackground->Layout(); //we use a dummy panel as actual background: replaces simple "Layout()" call
        //-> it seems this layout is not required, and even harmful: resets m_comboBoxExecFinished dropdown while user is selecting!
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
        if (paused_)
        {
            stopTimer();
            while (paused_)
            {
                wxMilliSleep(UI_UPDATE_INTERVAL);
                updateUiNow(); //receive UI message that ends pause
            }
            resumeTimer();
        }
        /*
            /|\
             |   keep this sequence to ensure one full progress update before entering pause mode!
            \|/
        */
        updateUiNow(); //receive UI message that sets pause status
    }
    else
        Update(); //don't wait until next idle event (who knows what blocking process comes next?)
}


std::wstring SyncStatus::SyncStatusImpl::getExecWhenFinishedCommand() const
{
    return m_comboBoxExecFinished->getValue();
}


void SyncStatus::SyncStatusImpl::updateDialogStatus() //depends on "syncStat_, paused_, finalResult"
{
    m_staticTextStatus->SetLabel(getDialogStatusText(syncStat_, paused_, finalResult));

    if (syncStat_) //sync running
    {
        if (paused_)
            m_buttonPause->SetLabel(_("Continue"));
        else
            m_buttonPause->SetLabel(_("Pause"));

        if (paused_)
            m_bitmapStatus->SetBitmap(GlobalResources::getImage(L"statusPause"));
        else
            switch (syncStat_->currentPhase())
            {
                case ProcessCallback::PHASE_NONE:
                    break;

                case ProcessCallback::PHASE_SCANNING:
                    m_bitmapStatus->SetBitmap(GlobalResources::getImage(L"statusScanning"));
                    break;

                case ProcessCallback::PHASE_COMPARING_CONTENT:
                    m_bitmapStatus->SetBitmap(GlobalResources::getImage(L"statusBinaryCompare"));
                    break;

                case ProcessCallback::PHASE_SYNCHRONIZING:
                    m_bitmapStatus->SetBitmap(GlobalResources::getImage(L"statusSyncing"));
                    break;
            }
    }
    else //sync finished
        switch (finalResult)
        {
            case RESULT_ABORTED:
                m_bitmapStatus->SetBitmap(GlobalResources::getImage(L"statusError"));
                break;

            case RESULT_FINISHED_WITH_ERROR:
                m_bitmapStatus->SetBitmap(GlobalResources::getImage(L"statusWarning"));
                break;

            case RESULT_FINISHED_WITH_SUCCESS:
                m_bitmapStatus->SetBitmap(GlobalResources::getImage(L"statusSuccess"));
                break;
        }

    //show status on Windows 7 taskbar
    if (taskbar_.get())
    {
        if (syncStat_) //sync running
        {
            if (paused_)
                taskbar_->setStatus(Taskbar::STATUS_PAUSED);
            else
                switch (syncStat_->currentPhase())
                {
                    case ProcessCallback::PHASE_NONE:
                    case ProcessCallback::PHASE_SCANNING:
                        taskbar_->setStatus(Taskbar::STATUS_INDETERMINATE);
                        break;

                    case ProcessCallback::PHASE_COMPARING_CONTENT:
                    case ProcessCallback::PHASE_SYNCHRONIZING:
                        taskbar_->setStatus(Taskbar::STATUS_NORMAL);
                        break;
                }
        }
        else //sync finished
            switch (finalResult)
            {
                case RESULT_ABORTED:
                case RESULT_FINISHED_WITH_ERROR:
                    taskbar_->setStatus(Taskbar::STATUS_ERROR);
                    break;

                case RESULT_FINISHED_WITH_SUCCESS:
                    taskbar_->setStatus(Taskbar::STATUS_NORMAL);
                    break;
            }
    }

    m_panelHeader->Layout();
    Layout();
}


void SyncStatus::SyncStatusImpl::closeWindowDirectly() //this should really be called: do not call back + schedule deletion
{
    paused_ = false; //you never know?

    //ATTENTION: dialog may live a little longer, so cut callbacks!
    //e.g. wxGTK calls OnIconize after wxWindow::Close() (better not ask why) and before physical destruction! => indirectly calls updateDialogStatus(), which reads syncStat_!!!

    //------- change class state -------
    abortCb_  = nullptr; //avoid callback to (maybe) deleted parent process
    syncStat_ = nullptr; //set *after* last call to "updateProgress"
    //----------------------------------

    Close();
}


void SyncStatus::SyncStatusImpl::processHasFinished(SyncResult resultId, const ErrorLog& log) //essential to call this in StatusHandler derived class destructor
{
    //at the LATEST(!) to prevent access to currentStatusHandler
    //enable okay and close events; may be set in this method ONLY

    wxWindowUpdateLocker dummy(this); //badly needed

    paused_ = false; //you never know?

    //update numbers one last time (as if sync were still running)
    updateProgress(false);

    switch (syncStat_->currentPhase()) //no matter if paused or not
    {
        case ProcessCallback::PHASE_NONE:
        case ProcessCallback::PHASE_SCANNING:
            //set overall speed -> not needed
            //items processed -> not needed
            break;

        case ProcessCallback::PHASE_COMPARING_CONTENT:
        case ProcessCallback::PHASE_SYNCHRONIZING:
        {
            auto objectsCurrent = syncStat_->getObjectsCurrent(syncStat_->currentPhase());
            auto objectsTotal   = syncStat_->getObjectsTotal  (syncStat_->currentPhase());
            auto dataCurrent    = syncStat_->getDataCurrent   (syncStat_->currentPhase());
            auto dataTotal      = syncStat_->getDataTotal     (syncStat_->currentPhase());

            //set overall speed (instead of current speed)
            assert(perf);
            if (perf)
                m_staticTextSpeed->SetLabel(perf->getOverallBytesPerSecond()); //note: we can't simply divide "sync total bytes" by "timeElapsed"

            //show new element "items processed"
            m_staticTextLabelItemsProc->Show(true);
            bSizerItemsProc           ->Show(true);
            m_staticTextProcessedObj ->SetLabel(toGuiString(objectsCurrent));
            m_staticTextDataProcessed->SetLabel(L"(" + filesizeToShortString(dataCurrent) + L")");

            //hide remaining elements...
            if (objectsCurrent == objectsTotal && //...if everything was processed successfully
                dataCurrent    == dataTotal)
            {
                m_staticTextLabelItemsRem->Show(false);
                bSizerItemsRem           ->Show(false);
            }
        }
        break;
    }

    //------- change class state -------
    abortCb_    = nullptr; //avoid callback to (maybe) deleted parent process
    syncStat_   = nullptr; //set *after* last call to "updateProgress"
    finalResult = resultId;
    //----------------------------------

    updateDialogStatus();
    setExternalStatus(getDialogStatusText(syncStat_, paused_, finalResult), wxString());

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
    m_staticlineHeader ->Hide();
    m_textCtrlStatus   ->Hide();

    bSizerExecFinished->Show(false);

    //show and prepare final statistics
    bSizerFinalStat->Show(true);

    //show total time
    m_staticTextLabelElapsedTime->SetLabel(_("Total time:")); //it's not "elapsed time" anymore

    //hide remaining time
    m_staticTextLabelRemTime->Show(false);
    m_staticTextRemTime     ->Show(false);

    //workaround wxListBox bug on Windows XP: labels are drawn on top of each other
    assert(m_listbookResult->GetImageList()); //make sure listbook keeps *any* image list
    //due to some crazy reasons that aren't worth debugging, this needs to be done directly in wxFormBuilder,
    //the following call is *not* sufficient: m_listbookResult->AssignImageList(new wxImageList(180, 1));
    //note: alternative solutions involving wxLC_LIST, wxLC_REPORT and SetWindowStyleFlag() do not work portably! wxListBook using wxLC_ICON is obviously a class invariant!

    //1. re-arrange graph into results listbook
    bSizerTop->Detach(m_panelProgress);
    m_panelProgress->Reparent(m_listbookResult);
#ifdef FFS_LINUX
    wxYield(); //wxGTK 2.9.3 fails miserably at "reparent" whithout this
#endif
    m_listbookResult->AddPage(m_panelProgress, _("Statistics"), true); //AddPage() takes ownership!

    //2. log file
    const size_t posLog = 1;
    LogControl* logControl = new LogControl(m_listbookResult, log); //owned by m_listbookResult
    m_listbookResult->AddPage(logControl, _("Logging"), false);
    //bSizerHoldStretch->Insert(0, logControl, 1, wxEXPAND);

    //show log instead of graph if errors occured! (not required for ignored warnings)
    if (log.getItemCount(TYPE_ERROR | TYPE_FATAL_ERROR) > 0)
        m_listbookResult->ChangeSelection(posLog);

    m_panelFooter->Layout();
    Layout();

    //Raise(); -> don't! user may be watching a movie in the meantime ;)
}


void SyncStatus::SyncStatusImpl::OnOkay(wxCommandEvent& event)
{
    Close(); //generate close event: do NOT destroy window unconditionally!
}


void SyncStatus::SyncStatusImpl::OnAbort(wxCommandEvent& event)
{
    paused_ = false;
    updateDialogStatus(); //update status + pause button

    //no Layout() or UI-update here to avoid cascaded Yield()-call
    if (abortCb_)
        abortCb_->requestAbortion();
}


void SyncStatus::SyncStatusImpl::OnPause(wxCommandEvent& event)
{
    paused_ = !paused_;
    updateDialogStatus(); //update status + pause button
}


void SyncStatus::SyncStatusImpl::OnClose(wxCloseEvent& event)
{
    //this event handler may be called due to a system shutdown DURING synchronization!
    //try to stop sync gracefully and cross fingers:
    if (abortCb_)
        abortCb_->requestAbortion();
    //Note: we must NOT veto dialog destruction, else we will cancel system shutdown if this dialog is application main window (as in batch mode)

    isZombie = true; //it "lives" until cleanup in next idle event
    Destroy();
}


void SyncStatus::SyncStatusImpl::OnIconize(wxIconizeEvent& event)
{
    if (isZombie) return; //wxGTK sends iconize event *after* wxWindow::Destroy, sigh...

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
        trayIcon->Connect(FFS_REQUEST_RESUME_TRAY_EVENT, wxCommandEventHandler(SyncStatus::SyncStatusImpl::OnResumeFromTray), nullptr, this);
        //tray icon has shorter lifetime than this => no need to disconnect event later
    }

    if (syncStat_)
        updateProgress(false); //set tray tooltip + progress: e.g. no updates while paused

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

    updateDialogStatus(); //restore Windows 7 task bar status (e.g. required in pause mode)
    if (syncStat_)
        updateProgress(false); //restore Windows 7 task bar progress (e.g. required in pause mode)
}


//########################################################################################


//redirect to implementation
SyncStatus::SyncStatus(AbortCallback& abortCb,
                       const Statistics& syncStat,
                       MainDialog* parentWindow,
                       bool showProgress,
                       const wxString& jobName,
                       const std::wstring& execWhenFinished,
                       std::vector<std::wstring>& execFinishedHistory) :
    pimpl(new SyncStatusImpl(abortCb, syncStat, parentWindow, jobName, execWhenFinished, execFinishedHistory))
{
    if (showProgress)
    {
        pimpl->Show();
        pimpl->updateProgress(false); //clear gui flicker, remove dummy texts: window must be visible to make this work!
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

void SyncStatus::initNewPhase()
{
    pimpl->initNewPhase();
}

void SyncStatus::reportCurrentBytes(Int64 currentData)
{
    pimpl->reportCurrentBytes(currentData);
}

void SyncStatus::updateProgress()
{
    pimpl->updateProgress();
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

void SyncStatus::processHasFinished(SyncResult resultId, const ErrorLog& log)
{
    pimpl->processHasFinished(resultId, log);
}

void SyncStatus::closeWindowDirectly() //don't wait for user (silent mode)
{
    pimpl->closeWindowDirectly();
}
