// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) 2008-2011 ZenJu (zhnmju123 AT gmx.de)                    *
// **************************************************************************

#include "progress_indicator.h"
#include <memory>
#include "gui_generated.h"
#include <wx/stopwatch.h>
#include "../lib/resources.h"
#include <wx+/string_conv.h>
#include <wx+/format_unit.h>
#include "../lib/statistics.h"
#include <wx/wupdlock.h>
#include <zen/basic_math.h>
#include "tray_icon.h"
#include <memory>
#include <wx+/mouse_move_dlg.h>
#include "../lib/error_log.h"
#include <wx+/toggle_button.h>
#include "taskbar.h"
#include <wx+/image_tools.h>
#include <wx+/graph.h>

using namespace zen;


namespace
{
const int GAUGE_FULL_RANGE = 50000;

//window size used for statistics in milliseconds
const int windowSizeRemainingTime = 60000; //some usecases have dropouts of 40 seconds -> 60 sec. window size handles them well
const int windowSizeBytesPerSec   =  5000; //


void setNewText(const wxString& newText, wxTextCtrl& control, bool& updateLayout)
{
    if (control.GetValue().length() != newText.length())
        updateLayout = true; //avoid screen flicker: apply only when necessary

    if (control.GetValue() != newText)
        control.ChangeValue(newText);
}

void setNewText(const wxString& newText, wxStaticText& control, bool& updateLayout)
{
    if (control.GetLabel().length() != newText.length())
        updateLayout = true; //avoid screen flicker

    if (control.GetLabel() != newText)
        control.SetLabel(newText);
}
}


class CompareStatus::CompareStatusImpl : public CompareStatusGenerated
{
public:
    CompareStatusImpl(wxTopLevelWindow& parentWindow);

    void init(); //make visible, initialize all status values
    void finalize(); //hide again

    void switchToCompareBytewise(int totalObjectsToProcess, zen::Int64 totalDataToProcess);
    void incScannedObjects_NoUpdate(int number);
    void incProcessedCmpData_NoUpdate(int objectsProcessed, zen::Int64 dataProcessed);
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
    int        totalObjects;
    zen::Int64 totalData;      //each data element represents one byte for proper progress indicator scaling
    int        currentObjects; //each object represents a file or directory processed
    zen::Int64 currentData;

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

void CompareStatus::incProcessedCmpData_NoUpdate(int objectsProcessed, zen::Int64 dataProcessed)
{
    pimpl->incProcessedCmpData_NoUpdate(objectsProcessed, dataProcessed);
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
    bSizerFilesFound->Show(true);
    bSizerFilesRemaining->Show(false);
    sSizerSpeed->Show(false);
    sSizerTimeRemaining->Show(false);

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


void CompareStatus::CompareStatusImpl::finalize() //hide again
{
    taskbar_.reset();
    parentWindow_.SetTitle(titleTextBackup);
}


void CompareStatus::CompareStatusImpl::switchToCompareBytewise(int totalObjectsToProcess, zen::Int64 totalDataToProcess)
{
    status = COMPARING_CONTENT;

    currentData = 0;
    totalData = totalDataToProcess;

    currentObjects = 0;
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


void CompareStatus::CompareStatusImpl::incProcessedCmpData_NoUpdate(int objectsProcessed, zen::Int64 dataProcessed)
{
    currentData    +=    dataProcessed;
    currentObjects += objectsProcessed;
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
        const double fraction = totalData + totalObjects == 0 ? 0 : to<double>(currentData + currentObjects) / to<double>(totalData + totalObjects);

        //write status information to taskbar, parent title ect.
        switch (status)
        {
            case SCANNING:
                showProgressExternally(toStringSep(scannedObjects) + wxT(" - ") + _("Scanning..."));
                break;
            case COMPARING_CONTENT:
                showProgressExternally(percentageToShortString(fraction) + wxT(" - ") + _("Comparing content..."), fraction);
                break;
        }

        bool updateLayout = false; //avoid screen flicker by calling layout() only if necessary

        //remove linebreaks from currentStatusText
        wxString statusTextFmt = currentStatusText;
        replace(statusTextFmt, L'\n', L' ');

        //status texts
        if (m_textCtrlStatus->GetValue() != statusTextFmt) //no layout update for status texts!
            m_textCtrlStatus->ChangeValue(statusTextFmt);

        //nr of scanned objects
        setNewText(toStringSep(scannedObjects), *m_staticTextScanned, updateLayout);

        //progress indicator for "compare file content"
        m_gauge2->SetValue(numeric::round(fraction * GAUGE_FULL_RANGE));

        //remaining files left for file comparison
        const wxString filesToCompareTmp = toStringSep(totalObjects - currentObjects);
        setNewText(filesToCompareTmp, *m_staticTextFilesRemaining, updateLayout);

        //remaining bytes left for file comparison
        const wxString remainingBytesTmp = zen::filesizeToShortString(to<zen::UInt64>(totalData - currentData));
        setNewText(remainingBytesTmp, *m_staticTextDataRemaining, updateLayout);

        if (statistics.get())
        {
            if (timeElapsed.Time() - lastStatCallSpeed >= 500) //call method every 500 ms
            {
                lastStatCallSpeed = timeElapsed.Time();

                statistics->addMeasurement(currentObjects, to<double>(currentData));

                //current speed
                setNewText(statistics->getBytesPerSecond(), *m_staticTextSpeed, updateLayout);

                if (timeElapsed.Time() - lastStatCallRemTime >= 2000) //call method every two seconds only
                {
                    lastStatCallRemTime = timeElapsed.Time();

                    //remaining time
                    setNewText(statistics->getRemainingTime(), *m_staticTextRemTime, updateLayout);
                }
            }
        }

        //time elapsed
        setNewText(wxTimeSpan::Milliseconds(timeElapsed.Time()).Format(), *m_staticTextTimeElapsed, updateLayout);

        //do the ui update
        if (updateLayout)
            bSizer42->Layout();
    }
    updateUiNow();
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

        m_bpButtonErrors->init(buttonPressed ("error"), _("Error") + wxString::Format(wxT(" (%d)"), errorCount),
                               buttonReleased("error"), _("Error") + wxString::Format(wxT(" (%d)"), errorCount));

        m_bpButtonWarnings->init(buttonPressed ("warning"), _("Warning") + wxString::Format(wxT(" (%d)"), warningCount),
                                 buttonReleased("warning"), _("Warning") + wxString::Format(wxT(" (%d)"), warningCount));

        m_bpButtonInfo->init(buttonPressed ("info"), _("Info") + wxString::Format(wxT(" (%d)"), infoCount),
                             buttonReleased("info"), _("Info") + wxString::Format(wxT(" (%d)"), infoCount));

        m_bpButtonErrors  ->setActive(true);
        m_bpButtonWarnings->setActive(true);
        m_bpButtonInfo    ->setActive(false);

        m_bpButtonErrors  ->Show(errorCount   != 0);
        m_bpButtonWarnings->Show(warningCount != 0);
        m_bpButtonInfo    ->Show(infoCount    != 0);

        updateLogText();
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
                newLogText += cvrtString<zxString>(*i);
                newLogText += wxT("\n\n");
            }
        else //if no messages match selected view filter, show final status message at least
        {
            const std::vector<wxString>& allMessages = log_.getFormattedMessages();
            if (!allMessages.empty())
                newLogText = cvrtString<zxString>(allMessages.back());
        }

        wxWindowUpdateLocker dummy(m_textCtrlInfo);
        m_textCtrlInfo->ChangeValue(cvrtString<wxString>(newLogText));
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

private:
    static const size_t MAX_BUFFER_SIZE = 2500000; //sizeof(single node) worst case ~ 3 * 8 byte ptr + 16 byte key/value = 40 byte

    virtual double getValue(double x) const //x: seconds since begin
    {
        auto iter = data.lower_bound(x * 1000);
        if (iter == data.end())
            return data.empty() ? 0 : (--data.end())->second;
        return iter->second;
    }
    virtual double getXBegin() const { return 0; } //{ return data.empty() ? 0 : data.begin()->first / 1000.0; }
    virtual double getXEnd  () const { return data.empty() ? 0 : (--data.end())->first / 1000.0; }
    //example: two-element range is accessible within [0, 2)

    wxStopWatch timer;
    std::map<long, double> data;
};


struct LabelFormatterBytes : public LabelFormatter
{
    virtual double getOptimalBlockSize(double bytesProposed) const
    {
        //round to next number which is a convenient to read block size
        if (bytesProposed <= 0)
            return 0;

        const double k = std::floor(std::log(bytesProposed) / std::log(2.0));
        const double e = std::pow(2.0, k);
        const double a = bytesProposed / e; //bytesProposed = a * 2^k with a in (1, 2)
        return (a < 1.5 ? 1 : 2) * e;
    }

    virtual wxString formatText(double value, double optimalBlockSize) const { return filesizeToShortString(UInt64(value)); };
};


inline
double bestFit(double val, double low, double high) { return val < (high + low) / 2 ? low : high; }


struct LabelFormatterTimeElapsed : public LabelFormatter
{
    virtual double getOptimalBlockSize(double secProposed) const
    {
        if (secProposed <= 5)
            return 5; //minimum block size

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
}


class SyncStatus::SyncStatusImpl : public SyncStatusDlgGenerated
{
public:
    SyncStatusImpl(AbortCallback& abortCb, MainDialog* parentWindow, SyncStatusID startStatus, const wxString& jobName);
    ~SyncStatusImpl();

    void resetGauge(int totalObjectsToProcess, zen::Int64 totalDataToProcess);
    void incProgressIndicator_NoUpdate(int objectsProcessed, zen::Int64 dataProcessed);
    void incScannedObjects_NoUpdate(int number);
    void setStatusText_NoUpdate(const wxString& text);
    void updateStatusDialogNow(bool allowYield = true);

    void setCurrentStatus(SyncStatus::SyncStatusID id);
    void processHasFinished(SyncStatus::SyncStatusID id, const ErrorLogging& log);  //essential to call this in StatusUpdater derived class destructor at the LATEST(!) to prevent access to currentStatusUpdater

    void minimizeToTray();

private:
    void OnKeyPressed(wxKeyEvent& event);
    virtual void OnOkay(wxCommandEvent& event);
    virtual void OnPause(wxCommandEvent& event);
    virtual void OnAbort(wxCommandEvent& event);
    virtual void OnClose(wxCloseEvent& event);
    virtual void OnIconize(wxIconizeEvent& event);

    void resumeFromSystray();
    void OnResumeFromTray(wxCommandEvent& event);

    bool currentProcessIsRunning();
    void showProgressExternally(const wxString& progressText, double fraction = 0); //between [0, 1]

    const wxString jobName_;
    wxStopWatch timeElapsed;

    AbortCallback* abortCb_; //temporarily bound
    MainDialog* mainDialog; //optional

    //gauge variables
    int        totalObjects;
    zen::Int64 totalData;
    int        currentObjects; //each object represents a file or directory processed
    zen::Int64 currentData;    //each data element represents one byte for proper progress indicator scaling

    //status variables
    size_t scannedObjects;
    wxString currentStatusText;

    bool processPaused;
    SyncStatus::SyncStatusID currentStatus;

    std::unique_ptr<Taskbar> taskbar_;

    //remaining time
    std::unique_ptr<Statistics> statistics;
    long lastStatCallSpeed;   //used for calculating intervals between statistics update
    long lastStatCallRemTime; //

    std::shared_ptr<GraphDataBytes> graphDataBytes;

    wxString titelTextBackup;

    std::unique_ptr<FfsTrayIcon> trayIcon; //optional: if filled all other windows should be hidden and conversely
};


//redirect to implementation
SyncStatus::SyncStatus(AbortCallback& abortCb,
                       MainDialog* parentWindow,
                       SyncStatusID startStatus,
                       bool startSilent,
                       const wxString& jobName) :
    pimpl(new SyncStatusImpl(abortCb, parentWindow, startStatus, jobName))
{
    if (startSilent)
        pimpl->minimizeToTray();
    else
    {
        pimpl->Show();
        pimpl->updateStatusDialogNow(false); //update visual statistics to get rid of "dummy" texts
    }
}

SyncStatus::~SyncStatus()
{
    //DON'T delete pimpl! it will be deleted by the user clicking "OK/Cancel" -> (wxWindow::Destroy())
}

wxWindow* SyncStatus::getAsWindow()
{
    return pimpl;
}

void SyncStatus::closeWindowDirectly() //don't wait for user (silent mode)
{
    pimpl->Destroy();
}

void SyncStatus::resetGauge(int totalObjectsToProcess, zen::Int64 totalDataToProcess)
{
    pimpl->resetGauge(totalObjectsToProcess, totalDataToProcess);
}

void SyncStatus::incScannedObjects_NoUpdate(int number)
{
    pimpl->incScannedObjects_NoUpdate(number);
}

void SyncStatus::incProgressIndicator_NoUpdate(int objectsProcessed, zen::Int64 dataProcessed)
{
    pimpl->incProgressIndicator_NoUpdate(objectsProcessed, dataProcessed);
}

void SyncStatus::setStatusText_NoUpdate(const wxString& text)
{
    pimpl->setStatusText_NoUpdate(text);
}

void SyncStatus::updateStatusDialogNow()
{
    pimpl->updateStatusDialogNow();
}

void SyncStatus::setCurrentStatus(SyncStatusID id)
{
    pimpl->setCurrentStatus(id);
}

void SyncStatus::processHasFinished(SyncStatusID id, const ErrorLogging& log)
{
    pimpl->processHasFinished(id, log);
}
//########################################################################################

namespace
{
void fitHeight(wxTopLevelWindow& wnd)
{
    if (wnd.IsMaximized())
        return;
    //Fit() height only:
    int width = wnd.GetSize().GetWidth();
    wnd.Fit();
    int height = wnd.GetSize().GetHeight();
    wnd.SetSize(wxSize(width, height));
}
}

SyncStatus::SyncStatusImpl::SyncStatusImpl(AbortCallback& abortCb,
                                           MainDialog* parentWindow,
                                           SyncStatusID startStatus,
                                           const wxString& jobName) :
    SyncStatusDlgGenerated(parentWindow,
                           wxID_ANY,
                           parentWindow ? wxString(wxEmptyString) : (wxString(wxT("FreeFileSync - ")) + _("Folder Comparison and Synchronization")),
                           wxDefaultPosition, wxSize(620, 330),
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
    processPaused(false),
    currentStatus(SyncStatus::ABORTED),
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

    m_staticTextSpeed->SetLabel(wxT("-"));
    m_staticTextRemTime->SetLabel(wxT("-"));

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
    bSizerFinalStat->Show(false);

    SetIcon(GlobalResources::instance().programIcon); //set application icon

    //register key event
    Connect(wxEVT_CHAR_HOOK, wxKeyEventHandler(SyncStatusImpl::OnKeyPressed), NULL, this);

    setCurrentStatus(startStatus); //first state: will be shown while waiting for dir locks (if at all)

    //init graph
    graphDataBytes = std::make_shared<GraphDataBytes>();
    m_panelGraph->setAttributes(Graph2D::GraphAttributes().
                                setLabelX(Graph2D::X_LABEL_BOTTOM, 20, std::make_shared<LabelFormatterTimeElapsed>()).
                                setLabelY(Graph2D::Y_LABEL_RIGHT,  60, std::make_shared<LabelFormatterBytes>()));
    m_panelGraph->setData(graphDataBytes, Graph2D::LineAttributes().setLineWidth(2).setColor(wxColor(0, 192, 0))); //medium green

    //Fit() height only:
    //fitHeight(*this);
}


SyncStatus::SyncStatusImpl::~SyncStatusImpl()
{
    if (mainDialog)
    {
        mainDialog->enableAllElements();

        //restore title text
        mainDialog->SetTitle(titelTextBackup);

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


void SyncStatus::SyncStatusImpl::resetGauge(int totalObjectsToProcess, zen::Int64 totalDataToProcess)
{
    currentData = 0;
    totalData = totalDataToProcess;

    currentObjects = 0;
    totalObjects   = totalObjectsToProcess;

    //set new statistics handler: 10 seconds "window" for remaining time, 5 seconds for speed
    statistics.reset(new Statistics(totalObjectsToProcess, to<double>(totalDataToProcess), windowSizeRemainingTime, windowSizeBytesPerSec));

    lastStatCallSpeed   = -1000000; //some big number
    lastStatCallRemTime = -1000000;

    //set to 0 even if totalDataToProcess is 0: due to a bug in wxGauge::SetValue, it doesn't change to determinate mode when setting the old value again
    //so give updateStatusDialogNow() a chance to set a different value
    m_gauge1->SetValue(0);
}


void SyncStatus::SyncStatusImpl::incProgressIndicator_NoUpdate(int objectsProcessed, zen::Int64 dataProcessed)
{
    //assert(dataProcessed >= 0);

    currentData    +=    dataProcessed;
    currentObjects += objectsProcessed;

    //update graph data
    graphDataBytes->addCurrentValue(to<double>(currentData));
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

Zorder validateZorder(const wxWindow& top, const wxWindow& bottom)
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
    const double fraction = totalData + totalObjects == 0 ? 1 : to<double>(currentData + currentObjects) / to<double>(totalData + totalObjects);

    //write status information to systray, taskbar, parent title ect.

    const wxString postFix = jobName_.empty() ? wxString() : (wxT("\n\"") + jobName_ + wxT("\""));
    switch (currentStatus)
    {
        case SyncStatus::SCANNING:
            showProgressExternally(toStringSep(scannedObjects) + wxT(" - ") + _("Scanning...") + postFix);
            break;
        case SyncStatus::COMPARING_CONTENT:
            showProgressExternally(percentageToShortString(fraction) + wxT(" - ") + _("Comparing content...") + postFix, fraction);
            break;
        case SyncStatus::SYNCHRONIZING:
            showProgressExternally(percentageToShortString(fraction) + wxT(" - ") + _("Synchronizing...") + postFix, fraction);
            break;
        case SyncStatus::PAUSE:
            showProgressExternally(percentageToShortString(fraction) + wxT(" - ") + _("Paused") + postFix, fraction);
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

        bool updateLayout = false; //avoid screen flicker by calling layout() only if necessary

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
        setNewText(remainingObjTmp, *m_staticTextRemainingObj, updateLayout);

        //remaining bytes left for copy
        const wxString remainingBytesTmp = zen::filesizeToShortString(to<zen::UInt64>(totalData - currentData));
        setNewText(remainingBytesTmp, *m_staticTextDataRemaining, updateLayout);

        //statistics
        if (statistics.get())
        {
            if (timeElapsed.Time() - lastStatCallSpeed >= 500) //call method every 500 ms
            {
                lastStatCallSpeed = timeElapsed.Time();

                statistics->addMeasurement(currentObjects, to<double>(currentData));

                //current speed
                setNewText(statistics->getBytesPerSecond(), *m_staticTextSpeed, updateLayout);

                if (timeElapsed.Time() - lastStatCallRemTime >= 2000) //call method every two seconds only
                {
                    lastStatCallRemTime = timeElapsed.Time();

                    //remaining time
                    setNewText(statistics->getRemainingTime(), *m_staticTextRemTime, updateLayout);
                }
            }
        }

        m_panelGraph->Refresh();

        //time elapsed
        const int timeElapSec = timeElapsed.Time() / 1000;
        setNewText(timeElapSec < 3600 ?
                   wxTimeSpan::Seconds(timeElapSec).Format(   L"%M:%S") :
                   wxTimeSpan::Seconds(timeElapSec).Format(L"%H:%M:%S"), *m_staticTextTimeElapsed, updateLayout);

        //do the ui update
        if (updateLayout)
        {
            bSizerProgressStat->Layout();
            bSizerProgressMain->Layout();
        }
    }

#ifdef FFS_WIN
    //workaround Windows 7 bug messing up z-order after temporary application hangs: https://sourceforge.net/tracker/index.php?func=detail&aid=3376523&group_id=234430&atid=1093080
    if (mainDialog)
        if (validateZorder(*this, *mainDialog) == ZORDER_WRONG)
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
        if (processPaused)
        {
            if (statistics.get()) statistics->pauseTimer();
            graphDataBytes->pauseTimer();

            while (processPaused && currentProcessIsRunning())
            {
                wxMilliSleep(UI_UPDATE_INTERVAL);
                updateUiNow();
            }

            if (statistics.get()) statistics->resumeTimer();
            graphDataBytes->resumeTimer();
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
    Layout();
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

    //hide progress statistics
    bSizerProgressMain->Show(false);

    //show and prepare final statistics
    bSizerFinalStat->Show(true);
    m_staticTextProcessedObj->SetLabel(toStringSep(currentObjects));
    m_staticTextDataProcessed->SetLabel(zen::filesizeToShortString(to<zen::UInt64>(currentData)));

    m_staticTextTimeTotal->SetLabel(m_staticTextTimeElapsed->GetLabel());

    //    if (totalObjects == currentObjects &&  ->if everything was processed successfully
    //        totalData    == currentData)
    //        ;

    updateStatusDialogNow(false); //keep this sequence to avoid display distortion, if e.g. only 1 item is sync'ed

    //fill result listbox:
    //1. log file
    LogControl* logControl = new LogControl(m_listbookResult, log);
    m_listbookResult->AddPage(logControl, _("Logging"), true);
    //bSizerHoldStretch->Insert(0, logControl, 1, wxEXPAND);

    //2. re-arrange graph into results listbook
    bSizerProgressMain->Detach(m_panelGraph);
    m_panelGraph->Reparent(m_listbookResult);
    m_listbookResult->AddPage(m_panelGraph, _("Statistics"), false);

    //fitHeight(*this);
    Layout();

    //Raise(); -> don't! user may be watching a movie in the meantime ;)
}


void SyncStatus::SyncStatusImpl::OnOkay(wxCommandEvent& event)
{
    Close(); //generate close event: do NOT destroy window unconditionally!
}


void SyncStatus::SyncStatusImpl::OnAbort(wxCommandEvent& event)
{
    processPaused = false;
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


void SyncStatus::SyncStatusImpl::OnPause(wxCommandEvent& event)
{
    static SyncStatus::SyncStatusID previousStatus = SyncStatus::ABORTED;

    processPaused = !processPaused;

    if (processPaused)
    {
        previousStatus = currentStatus; //save current status
        setCurrentStatus(SyncStatus::PAUSE);

        m_buttonPause->SetLabel(_("Continue"));
        m_animationControl1->Stop();

        //pause timers
        timeElapsed.Pause();
    }
    else
    {
        setCurrentStatus(previousStatus);

        m_buttonPause->SetLabel(_("Pause"));
        m_animationControl1->Play();

        //resume timers
        timeElapsed.Resume();
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
