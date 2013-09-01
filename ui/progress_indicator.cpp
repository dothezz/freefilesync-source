// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) Zenju (zenju AT gmx DOT de) - All Rights Reserved        *
// **************************************************************************

#include "progress_indicator.h"
#include <memory>
#include <wx/imaglist.h>
#include <wx/stopwatch.h>
#include <wx/wupdlock.h>
#include <wx/sound.h>
#include <wx/clipbrd.h>
#include <wx/msgdlg.h>
#include <wx/dcclient.h>
#include <wx/dataobj.h> //wxTextDataObject
#include <zen/basic_math.h>
#include <zen/format_unit.h>
#include <zen/scope_guard.h>
#include <wx+/grid.h>
#include <wx+/mouse_move_dlg.h>
#include <wx+/toggle_button.h>
#include <wx+/image_tools.h>
#include <wx+/graph.h>
#include <wx+/context_menu.h>
#include <wx+/no_flicker.h>
#include <wx+/font_size.h>
#include <wx+/std_button_order.h>
#include <zen/file_handling.h>
#include <zen/thread.h>
#include "gui_generated.h"
#include "../lib/ffs_paths.h"
#include "../lib/resources.h"
#include "../lib/perf_check.h"
#include "tray_icon.h"
#include "taskbar.h"
#include "exec_finished_box.h"
//#include <wx/msgdlg.h>
#ifdef ZEN_MAC
#include <ApplicationServices/ApplicationServices.h>
#endif

using namespace zen;


namespace
{
const int GAUGE_FULL_RANGE = 50000;

//window size used for statistics in milliseconds
const int WINDOW_REMAINING_TIME = 60000; //some scenarios have dropouts of 40 seconds -> 60 sec. window size handles them well
const int WINDOW_BYTES_PER_SEC  =  5000; //
}


class CompareProgressDialog::Pimpl : public CompareProgressDlgGenerated
{
public:
    Pimpl(wxFrame& parentWindow);

    void init(const Statistics& syncStat); //constructor/destructor semantics, but underlying Window is reused
    void finalize(); //

    void switchToCompareBytewise();
    void updateStatusPanelNow();

private:
    wxFrame& parentWindow_;
    wxString titleTextBackup;

    wxStopWatch timeElapsed;
    long binCompStartMs; //begin of binary comparison phase in [ms]

    const Statistics* syncStat_; //only bound while sync is running

    std::unique_ptr<Taskbar> taskbar_;
    std::unique_ptr<PerfCheck> perf; //estimate remaining time

    long lastStatCallSpeed;   //used for calculating intervals between showing and collecting perf samples
};


CompareProgressDialog::Pimpl::Pimpl(wxFrame& parentWindow) :
    CompareProgressDlgGenerated(&parentWindow),
    parentWindow_(parentWindow),
    binCompStartMs(0),
    syncStat_(nullptr),
    lastStatCallSpeed(-1000000) //some big number
{
    //init(); -> needed?
}


void CompareProgressDialog::Pimpl::init(const Statistics& syncStat)
{
    syncStat_ = &syncStat;
    titleTextBackup = parentWindow_.GetTitle();

    try //try to get access to Windows 7/Ubuntu taskbar
    {
        taskbar_ = make_unique<Taskbar>(parentWindow_);
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


void CompareProgressDialog::Pimpl::finalize()
{
    syncStat_ = nullptr;
    parentWindow_.SetTitle(titleTextBackup);
    taskbar_.reset();
}


void CompareProgressDialog::Pimpl::switchToCompareBytewise()
{
    //start to measure perf
    perf = make_unique<PerfCheck>(WINDOW_REMAINING_TIME, WINDOW_BYTES_PER_SEC);
    lastStatCallSpeed   = -1000000; //some big number

    binCompStartMs = timeElapsed.Time();

    //show status for comparing bytewise
    bSizerFilesFound    ->Show(false);
    bSizerFilesRemaining->Show(true);
    sSizerSpeed         ->Show(true);
    sSizerTimeRemaining ->Show(true);

    m_gauge2->Show();
    bSizer42->Layout();
    Layout();
}


void CompareProgressDialog::Pimpl::updateStatusPanelNow()
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
    const long timeNow = timeElapsed.Time();

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
            setTitle(fractionToString(fraction) + wxT(" - ") + _("Comparing content..."));
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
            assert(perf);
            if (perf)
                if (numeric::dist(lastStatCallSpeed, timeNow) >= 500)
                {
                    lastStatCallSpeed = timeNow;

                    if (numeric::dist(binCompStartMs, timeNow) >= 1000) //discard stats for first second: probably messy
                        perf->addSample(objectsCurrent, to<double>(dataCurrent), timeNow);

                    //current speed -> Win 7 copy uses 1 sec update interval instead
                    Opt<std::wstring> bps = perf->getBytesPerSecond();
                    setText(*m_staticTextSpeed, bps ? *bps : L"-", &layoutChanged);

                    //remaining time: display with relative error of 10% - based on samples taken every 0.5 sec only
                    //-> call more often than once per second to correctly show last few seconds countdown, but don't call too often to avoid occasional jitter
                    Opt<std::wstring> rt = perf->getRemainingTime(to<double>(dataTotal - dataCurrent));
                    setText(*m_staticTextRemTime, rt ? *rt : L"-", &layoutChanged);
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
    const long timeElapSec = timeNow / 1000;
    setText(*m_staticTextTimeElapsed,
            timeElapSec < 3600 ?
            wxTimeSpan::Seconds(timeElapSec).Format(   L"%M:%S") :
            wxTimeSpan::Seconds(timeElapSec).Format(L"%H:%M:%S"), &layoutChanged);

    if (layoutChanged)
        bSizer42->Layout();

    //do the ui update
    wxTheApp->Yield();
}

//########################################################################################

//redirect to implementation
CompareProgressDialog::CompareProgressDialog(wxFrame& parentWindow) :
    pimpl(new Pimpl(parentWindow)) {} //owned by parentWindow

wxWindow* CompareProgressDialog::getAsWindow()
{
    return pimpl;
}

void CompareProgressDialog::init(const Statistics& syncStat)
{
    pimpl->init(syncStat);
}

void CompareProgressDialog::finalize()
{
    pimpl->finalize();
}

void CompareProgressDialog::switchToCompareBytewise()
{
    pimpl->switchToCompareBytewise();
}

void CompareProgressDialog::updateStatusPanelNow()
{
    pimpl->updateStatusPanelNow();
}
//########################################################################################

namespace
{
inline
wxBitmap buttonPressed(const std::string& name)
{
    wxBitmap background = getResourceImage(L"log button pressed");
    return layOver(getResourceImage(utfCvrtTo<wxString>(name)), background);
}


inline
wxBitmap buttonReleased(const std::string& name)
{
    wxImage output = greyScale(getResourceImage(utfCvrtTo<wxString>(name))).ConvertToImage();
    //getResourceImage(utfCvrtTo<wxString>(name)).ConvertToImage().ConvertToGreyscale(1.0/3, 1.0/3, 1.0/3); //treat all channels equally!
    //brighten(output, 30);

    //zen::moveImage(output, 1, 0); //move image right one pixel
    return output;
}


//a vector-view on ErrorLog considering multi-line messages: prepare consumption by Grid
class MessageView
{
public:
    MessageView(const ErrorLog& log) : log_(log) {}

    size_t rowsOnView() const { return viewRef.size(); }

    struct LogEntryView
    {
        time_t      time;
        MessageType type;
        MsgString   messageLine;
        bool firstLine; //if LogEntry::message spans multiple rows
    };
    bool getEntry(size_t row, LogEntryView& out) const
    {
        if (row < viewRef.size())
        {
            const Line& line = viewRef[row];
            out.time = line.entry_->time;
            out.type = line.entry_->type;
            out.messageLine = extractLine(line.entry_->message, line.rowNumber_);
            out.firstLine = line.rowNumber_ == 0; //this is virtually always correct, unless first line of the original message is empty!
            return true;
        }
        return false;
    }

    void updateView(int includedTypes) //TYPE_INFO | TYPE_WARNING, ect. see error_log.h
    {
        viewRef.clear();

        for (auto it = log_.begin(); it != log_.end(); ++it)
            if (it->type & includedTypes)
            {
                assert_static((IsSameType<GetCharType<MsgString>::Type, wchar_t>::value));
                assert(!startsWith(it->message, L'\n'));

                size_t rowNumber = 0;
                bool lastCharNewline = true;
                for (const wchar_t c : it->message)
                    if (c == L'\n')
                    {
                        if (!lastCharNewline) //do not reference empty lines!
                            viewRef.push_back(Line(&*it, rowNumber));
                        ++rowNumber;
                        lastCharNewline = true;
                    }
                    else
                        lastCharNewline = false;

                if (!lastCharNewline)
                    viewRef.push_back(Line(&*it, rowNumber));
            }
    }

private:
    static MsgString extractLine(const MsgString& message, size_t textRow)
    {
        auto iter1 = message.begin();
        for (;;)
        {
            auto iter2 = std::find_if(iter1, message.end(), [](wchar_t c) { return c == L'\n'; });
            if (textRow == 0)
                return iter1 == message.end() ? MsgString() : MsgString(&*iter1, iter2 - iter1); //must not dereference iterator pointing to "end"!

            if (iter2 == message.end())
            {
                assert(false);
                return MsgString();
            }

            iter1 = iter2 + 1; //skip newline
            --textRow;
        }
    }

    struct Line
    {
        Line(const LogEntry* entry, size_t rowNumber) : entry_(entry), rowNumber_(rowNumber) {}
        const LogEntry* entry_; //always bound!
        size_t rowNumber_; //LogEntry::message may span multiple rows
    };

    std::vector<Line> viewRef; //partial view on log_
    /*          /|\
                 | updateView()
                 |                      */
    const ErrorLog log_;
};

//-----------------------------------------------------------------------------

enum ColumnTypeMsg
{
    COL_TYPE_MSG_TIME,
    COL_TYPE_MSG_CATEGORY,
    COL_TYPE_MSG_TEXT,
};

//Grid data implementation referencing MessageView
class GridDataMessages : public GridData
{
    static const int COLUMN_BORDER_LEFT = 4; //for left-aligned text

public:
    GridDataMessages(const std::shared_ptr<MessageView>& msgView) : msgView_(msgView) {}

    virtual size_t getRowCount() const { return msgView_ ? msgView_->rowsOnView() : 0; }

    virtual wxString getValue(size_t row, ColumnType colType) const
    {
        MessageView::LogEntryView entry = {};
        if (msgView_ && msgView_->getEntry(row, entry))
            switch (static_cast<ColumnTypeMsg>(colType))
            {
                case COL_TYPE_MSG_TIME:
                    if (entry.firstLine)
                        return formatTime<wxString>(FORMAT_TIME, localTime(entry.time));
                    break;

                case COL_TYPE_MSG_CATEGORY:
                    if (entry.firstLine)
                        switch (entry.type)
                        {
                            case TYPE_INFO:
                                return _("Info");
                            case TYPE_WARNING:
                                return _("Warning");
                            case TYPE_ERROR:
                                return _("Error");
                            case TYPE_FATAL_ERROR:
                                return _("Fatal Error");
                        }
                    break;

                case COL_TYPE_MSG_TEXT:
                    return copyStringTo<wxString>(entry.messageLine);
            }
        return wxEmptyString;
    }

    virtual void renderCell(Grid& grid, wxDC& dc, const wxRect& rect, size_t row, ColumnType colType)
    {
        wxRect rectTmp = rect;

        const wxColor colorGridLine = wxColour(192, 192, 192); //light grey

        wxDCPenChanger dummy2(dc, wxPen(colorGridLine, 1, wxSOLID));
        const bool drawBottomLine = [&]() -> bool //don't separate multi-line messages
        {
            MessageView::LogEntryView nextEntry = {};
            if (msgView_ && msgView_->getEntry(row + 1, nextEntry))
                return nextEntry.firstLine;
            return true;
        }();

        if (drawBottomLine)
        {
            dc.DrawLine(rect.GetBottomLeft(),  rect.GetBottomRight() + wxPoint(1, 0));
            --rectTmp.height;
        }

        //--------------------------------------------------------

        MessageView::LogEntryView entry = {};
        if (msgView_ && msgView_->getEntry(row, entry))
            switch (static_cast<ColumnTypeMsg>(colType))
            {
                case COL_TYPE_MSG_TIME:
                    drawCellText(dc, rectTmp, getValue(row, colType), true, wxALIGN_CENTER);
                    break;

                case COL_TYPE_MSG_CATEGORY:
                    if (entry.firstLine)
                        switch (entry.type)
                        {
                            case TYPE_INFO:
                                dc.DrawLabel(wxString(), getResourceImage(L"msg_info_small"), rectTmp, wxALIGN_CENTER);
                                break;
                            case TYPE_WARNING:
                                dc.DrawLabel(wxString(), getResourceImage(L"msg_warning_small"), rectTmp, wxALIGN_CENTER);
                                break;
                            case TYPE_ERROR:
                            case TYPE_FATAL_ERROR:
                                dc.DrawLabel(wxString(), getResourceImage(L"msg_error_small"), rectTmp, wxALIGN_CENTER);
                                break;
                        }
                    break;

                case COL_TYPE_MSG_TEXT:
                {
                    rectTmp.x     += COLUMN_BORDER_LEFT;
                    rectTmp.width -= COLUMN_BORDER_LEFT;
                    drawCellText(dc, rectTmp, getValue(row, colType), true);
                }
                break;
            }
    }

    virtual int getBestSize(wxDC& dc, size_t row, ColumnType colType)
    {
        // -> synchronize renderCell() <-> getBestSize()

        MessageView::LogEntryView entry = {};
        if (msgView_ && msgView_->getEntry(row, entry))
            switch (static_cast<ColumnTypeMsg>(colType))
            {
                case COL_TYPE_MSG_TIME:
                    return 2 * COLUMN_BORDER_LEFT + dc.GetTextExtent(getValue(row, colType)).GetWidth();

                case COL_TYPE_MSG_CATEGORY:
                    return getResourceImage(L"msg_info_small").GetWidth();

                case COL_TYPE_MSG_TEXT:
                    return COLUMN_BORDER_LEFT + dc.GetTextExtent(getValue(row, colType)).GetWidth();
            }
        return 0;
    }

    static int getColumnTimeDefaultWidth(Grid& grid)
    {
        wxClientDC dc(&grid.getMainWin());
        dc.SetFont(grid.getMainWin().GetFont());
        return 2 * COLUMN_BORDER_LEFT + dc.GetTextExtent(formatTime<wxString>(FORMAT_TIME)).GetWidth();
    }

    static int getColumnCategoryDefaultWidth()
    {
        return getResourceImage(L"msg_info_small").GetWidth();
    }

    static int getRowDefaultHeight(const Grid& grid)
    {
        return std::max(getResourceImage(L"msg_info_small").GetHeight(), grid.getMainWin().GetCharHeight() + 2) + 1; //+ some space + bottom border
    }

    virtual wxString getToolTip(size_t row, ColumnType colType) const
    {
        MessageView::LogEntryView entry = {};
        if (msgView_ && msgView_->getEntry(row, entry))
            switch (static_cast<ColumnTypeMsg>(colType))
            {
                case COL_TYPE_MSG_TIME:
                case COL_TYPE_MSG_TEXT:
                    break;

                case COL_TYPE_MSG_CATEGORY:
                    return getValue(row, colType);
            }
        return wxEmptyString;
    }

    virtual wxString getColumnLabel(ColumnType colType) const { return wxEmptyString; }

private:
    const std::shared_ptr<MessageView> msgView_;
};
}


class LogPanel : public LogPanelGenerated
{
public:
    LogPanel(wxWindow* parent, const ErrorLog& log) : LogPanelGenerated(parent),
        msgView(std::make_shared<MessageView>(log))
    {
        const int errorCount   = log.getItemCount(TYPE_ERROR | TYPE_FATAL_ERROR);
        const int warningCount = log.getItemCount(TYPE_WARNING);
        const int infoCount    = log.getItemCount(TYPE_INFO);

        auto initButton = [](ToggleButton& btn, const char* imgName, const wxString& tooltip) { btn.init(buttonPressed(imgName), buttonReleased(imgName)); btn.SetToolTip(tooltip); };

        initButton(*m_bpButtonErrors,   "msg_error",   _("Error"  ) + wxString::Format(L" (%d)", errorCount  ));
        initButton(*m_bpButtonWarnings, "msg_warning", _("Warning") + wxString::Format(L" (%d)", warningCount));
        initButton(*m_bpButtonInfo,     "msg_info",    _("Info"   ) + wxString::Format(L" (%d)", infoCount   ));

        m_bpButtonErrors  ->setActive(true);
        m_bpButtonWarnings->setActive(true);
        m_bpButtonInfo    ->setActive(errorCount + warningCount == 0);

        m_bpButtonErrors  ->Show(errorCount   != 0);
        m_bpButtonWarnings->Show(warningCount != 0);
        m_bpButtonInfo    ->Show(infoCount    != 0);

        //init grid, determine default sizes
        const int rowHeight           = GridDataMessages::getRowDefaultHeight(*m_gridMessages);
        const int colMsgTimeWidth     = GridDataMessages::getColumnTimeDefaultWidth(*m_gridMessages);
        const int colMsgCategoryWidth = GridDataMessages::getColumnCategoryDefaultWidth();

        m_gridMessages->setDataProvider(std::make_shared<GridDataMessages>(msgView));
        m_gridMessages->setColumnLabelHeight(0);
        m_gridMessages->showRowLabel(false);
        m_gridMessages->setRowHeight(rowHeight);
        std::vector<Grid::ColumnAttribute> attr;
        attr.push_back(Grid::ColumnAttribute(static_cast<ColumnType>(COL_TYPE_MSG_TIME    ), colMsgTimeWidth, 0));
        attr.push_back(Grid::ColumnAttribute(static_cast<ColumnType>(COL_TYPE_MSG_CATEGORY), colMsgCategoryWidth, 0));
        attr.push_back(Grid::ColumnAttribute(static_cast<ColumnType>(COL_TYPE_MSG_TEXT    ), -colMsgTimeWidth - colMsgCategoryWidth, 1));
        m_gridMessages->setColumnConfig(attr);

        //support for CTRL + C
        m_gridMessages->getMainWin().Connect(wxEVT_KEY_DOWN, wxKeyEventHandler(LogPanel::onGridButtonEvent), nullptr, this);

        m_gridMessages->Connect(EVENT_GRID_MOUSE_RIGHT_UP, GridClickEventHandler(LogPanel::onMsgGridContext), nullptr, this);

        updateGrid();
    }

private:
    virtual void OnErrors(wxCommandEvent& event)
    {
        m_bpButtonErrors->toggle();
        updateGrid();
    }

    virtual void OnWarnings(wxCommandEvent& event)
    {
        m_bpButtonWarnings->toggle();
        updateGrid();
    }

    virtual void OnInfo(wxCommandEvent& event)
    {
        m_bpButtonInfo->toggle();
        updateGrid();
    }

    void updateGrid()
    {
        int includedTypes = 0;
        if (m_bpButtonErrors->isActive())
            includedTypes |= TYPE_ERROR | TYPE_FATAL_ERROR;

        if (m_bpButtonWarnings->isActive())
            includedTypes |= TYPE_WARNING;

        if (m_bpButtonInfo->isActive())
            includedTypes |= TYPE_INFO;

        msgView->updateView(includedTypes); //update MVC "model"
        m_gridMessages->Refresh();   //update MVC "view"
    }

    void onGridButtonEvent(wxKeyEvent& event)
    {
        int keyCode = event.GetKeyCode();

        if (event.ControlDown())
            switch (keyCode)
            {
                case 'C':
                case WXK_INSERT: //CTRL + C || CTRL + INS
                    copySelectionToClipboard();
                    return; // -> swallow event! don't allow default grid commands!
            }

        event.Skip(); //unknown keypress: propagate
    }

    void onMsgGridContext(GridClickEvent& event)
    {
        const std::vector<size_t> selection = m_gridMessages->getSelectedRows();

        ContextMenu menu;
        menu.addItem(_("Copy") + L"\tCtrl+C", [this] { copySelectionToClipboard(); }, nullptr, !selection.empty());
        menu.popup(*this);
    }

    void copySelectionToClipboard()
    {
        try
        {
            typedef Zbase<wchar_t> zxString; //guaranteed exponential growth, unlike wxString
            zxString clipboardString;

            if (auto prov = m_gridMessages->getDataProvider())
            {
                std::vector<Grid::ColumnAttribute> colAttr = m_gridMessages->getColumnConfig();
                vector_remove_if(colAttr, [](const Grid::ColumnAttribute& ca) { return !ca.visible_; });
                if (!colAttr.empty())
                {
                    const std::vector<size_t> selection = m_gridMessages->getSelectedRows();
                    std::for_each(selection.begin(), selection.end(),
                                  [&](size_t row)
                    {
#ifdef _MSC_VER
                        typedef zxString zxString; //workaround MSVC compiler bug!
#endif
                        std::for_each(colAttr.begin(), --colAttr.end(),
                                      [&](const Grid::ColumnAttribute& ca)
                        {
                            clipboardString += copyStringTo<zxString>(prov->getValue(row, ca.type_));
                            clipboardString += L'\t';
                        });
                        clipboardString += copyStringTo<zxString>(prov->getValue(row, colAttr.back().type_));
                        clipboardString += L'\n';
                    });
                }
            }

            //finally write to clipboard
            if (!clipboardString.empty())
                if (wxClipboard::Get()->Open())
                {
                    ZEN_ON_SCOPE_EXIT(wxClipboard::Get()->Close());
                    wxClipboard::Get()->SetData(new wxTextDataObject(copyStringTo<wxString>(clipboardString))); //ownership passed
                }
        }
        catch (const std::bad_alloc& e)
        {
            wxMessageBox(_("Out of memory.") + L" " + utfCvrtTo<std::wstring>(e.what()), L"FreeFileSync - " + _("Error"), wxOK | wxICON_ERROR);
        }
    }

    std::shared_ptr<MessageView> msgView; //bound!
};

//########################################################################################

namespace
{
class CurveDataStatistics : public SparseCurveData
{
public:
    CurveDataStatistics() : SparseCurveData(true), /*true: add steps*/ timeNow(0) {}

    void clear() { samples.clear(); timeNow = 0; }

    void addRecord(long timeNowMs, double value)
    {
        //samples.clear();
        //samples.insert(std::make_pair(-1000, 0));
        //samples.insert(std::make_pair(0, 0));
        //samples.insert(std::make_pair(1, 1));
        //samples.insert(std::make_pair(1000, 0));
        //return;

        timeNow = timeNowMs;
        if (!samples.empty() && samples.rbegin()->second == value)
            return; //don't insert duplicate values

        samples.insert(samples.end(), std::make_pair(timeNowMs, value)); //time is "expected" to be monotonously ascending
        //documentation differs about whether "hint" should be before or after the to be inserted element!
        //however "std::map<>::end()" is interpreted correctly by GCC and VS2010

        if (samples.size() > MAX_BUFFER_SIZE) //limit buffer size
            samples.erase(samples.begin());
    }

private:
    virtual std::pair<double, double> getRangeX() const final
    {
        if (samples.empty()) return std::make_pair(0.0, 0.0);

        double upperEndMs = std::max(timeNow, samples.rbegin()->first);

        //report some additional width by 5% elapsed time to make graph recalibrate before hitting the right border
        //caveat: graph for batch mode binary comparison does NOT start at elapsed time 0!! PHASE_COMPARING_CONTENT and PHASE_SYNCHRONIZING!
        //=> consider width of current sample set!
        upperEndMs += 0.05 *(upperEndMs - samples.begin()->first);

        return std::make_pair(samples.begin()->first / 1000.0, //need not start with 0, e.g. "binary comparison, graph reset, followed by sync"
                              upperEndMs / 1000.0);
    }

    virtual Opt<CurvePoint> getLessEq(double x) const final //x: seconds since begin
    {
        const long timex = std::floor(x * 1000);
        //------ add artifical last sample value -------
        if (!samples.empty() && samples.rbegin()->first < timeNow)
            if (timeNow <= timex)
                return CurvePoint(timeNow / 1000.0, samples.rbegin()->second);
        //--------------------------------------------------

        //find first key > x, then go one step back: => samples must be a std::map, NOT std::multimap!!!
        auto it = samples.upper_bound(timex);
        if (it == samples.begin())
            return NoValue();
        //=> samples not empty in this context
        --it;
        return CurvePoint(it->first / 1000.0, it->second);
    }

    virtual Opt<CurvePoint> getGreaterEq(double x) const final
    {
        const long timex = std::ceil(x * 1000);
        //------ add artifical last sample value -------
        if (!samples.empty() && samples.rbegin()->first < timeNow)
            if (samples.rbegin()->first < timex && timex <= timeNow)
                return CurvePoint(timeNow / 1000.0, samples.rbegin()->second);
        //--------------------------------------------------

        auto it = samples.lower_bound(timex);
        if (it == samples.end())
            return NoValue();
        return CurvePoint(it->first / 1000.0, it->second);
    }

    static const size_t MAX_BUFFER_SIZE = 2500000; //sizeof(single node) worst case ~ 3 * 8 byte ptr + 16 byte key/value = 40 byte

    std::map<long, double> samples; //time, unit: [ms]  !don't use std::multimap, see getLessEq()
    long timeNow; //help create an artificial record at the end of samples to visualize current time!
};


class CurveDataCurrentValue : public CurveData
{
public:
    CurveDataCurrentValue() : x(0), yCurrent_(0), yTotal_(0) {}

    void setValue(long xTimeNowMs, double yCurrent, double yTotal) { x = xTimeNowMs / 1000.0; yCurrent_ = yCurrent; yTotal_ = yTotal; }

private:
    virtual std::pair<double, double> getRangeX() const  final { return std::make_pair(x, x); } //conceptually just a vertical line!

    virtual void getPoints(double minX, double maxX, int pixelWidth, std::vector<CurvePoint>& points) const final
    {
        //points.push_back(CurvePoint(-1, 0));
        //points.push_back(CurvePoint(0, 0));
        //points.push_back(CurvePoint(0.0001, 1));
        //points.push_back(CurvePoint(1, 0));
        //return;

        if (x <= maxX)
        {
            points.push_back(CurvePoint(x,    0));
            points.push_back(CurvePoint(x,    yCurrent_));
            points.push_back(CurvePoint(maxX, yCurrent_));
            points.push_back(CurvePoint(x,    yCurrent_));
            points.push_back(CurvePoint(x,    yTotal_));
        }
    }

    double x; //time elapsed in seconds
    double yCurrent_; //current bytes/items
    double yTotal_;
};


class CurveDataTotalValue : public CurveData
{
public:
    CurveDataTotalValue () : x(0), yTotal_(0) {}

    void setValue(long xTimeNowMs, double yTotal) { x = xTimeNowMs / 1000.0; yTotal_ = yTotal; }

private:
    virtual std::pair<double, double> getRangeX() const  final { return std::make_pair(x, x); } //conceptually just a vertical line!

    virtual void getPoints(double minX, double maxX, int pixelWidth, std::vector<CurvePoint>& points) const final
    {
        if (x <= maxX)
        {
            points.push_back(CurvePoint(x,    yTotal_));
            points.push_back(CurvePoint(maxX, yTotal_));
        }
    }

    double x; //time elapsed in seconds
    double yTotal_;
};


const double stretchDefaultBlockSize = 1.4; //enlarge block default size


struct LabelFormatterBytes : public LabelFormatter
{
    virtual double getOptimalBlockSize(double bytesProposed) const
    {
        bytesProposed *= stretchDefaultBlockSize; //enlarge block default size

        if (bytesProposed <= 1) //never smaller than 1 byte
            return 1;

        //round to next number which is a convenient to read block size
        const double k = std::floor(std::log(bytesProposed) / std::log(2.0));
        const double e = std::pow(2.0, k);
        if (numeric::isNull(e))
            return 0;
        const double a = bytesProposed / e; //bytesProposed = a * 2^k with a in [1, 2)
        assert(1 <= a && a < 2);
        const double steps[] = { 1, 2 };
        return e * numeric::nearMatch(a, std::begin(steps), std::end(steps));
    }

    virtual wxString formatText(double value, double optimalBlockSize) const { return filesizeToShortString(Int64(value)); };
};


struct LabelFormatterItemCount : public LabelFormatter
{
    virtual double getOptimalBlockSize(double itemsProposed) const
    {
        itemsProposed *= stretchDefaultBlockSize; //enlarge block default size

        const double steps[] = { 1, 2, 5, 10 };
        if (itemsProposed <= 10)
            return numeric::nearMatch(itemsProposed, std::begin(steps), std::end(steps)); //similar to nextNiceNumber(), but without the 2.5 step!
        return nextNiceNumber(itemsProposed);
    }

    virtual wxString formatText(double value, double optimalBlockSize) const
    {
        return toGuiString(numeric::round(value)); //not enough room for a "%x items" representation
    };
};


struct LabelFormatterTimeElapsed : public LabelFormatter
{
    LabelFormatterTimeElapsed(bool drawLabel) : drawLabel_(drawLabel) {}

    virtual double getOptimalBlockSize(double secProposed) const
    {
        const double stepsSec[] = { 20, 30, 60 }; //20 sec: minimum block size; (no 15: avoid flicker between 10<->15<->20 sec blocks)
        if (secProposed <= 60)
            return numeric::nearMatch(secProposed, std::begin(stepsSec), std::end(stepsSec));

        const double stepsMin[] = { 1, 2, 5, 10, 15, 20, 30, 60 }; //nice numbers for minutes
        if (secProposed <= 3600)
            return 60.0 * numeric::nearMatch(secProposed / 60, std::begin(stepsMin), std::end(stepsMin));

        if (secProposed <= 3600 * 24)
            return nextNiceNumber(secProposed / 3600) * 3600; //round up to full hours

        return nextNiceNumber(secProposed / (24 * 3600)) * 24 * 3600; //round to full days
    }

    virtual wxString formatText(double timeElapsed, double optimalBlockSize) const
    {
        if (!drawLabel_) return wxString();
        return timeElapsed < 60 ?
               replaceCpy(_P("1 sec", "%x sec", numeric::round(timeElapsed)), L"%x", zen::numberTo<std::wstring>(numeric::round(timeElapsed))) :
               timeElapsed < 3600 ?
               wxTimeSpan::Seconds(timeElapsed).Format(   L"%M:%S") :
               wxTimeSpan::Seconds(timeElapsed).Format(L"%H:%M:%S");
    }

private:
    bool drawLabel_;
};
}


template <class TopLevelDialog> //can be a wxFrame or wxDialog
class SyncProgressDialogImpl : private TopLevelDialog, public SyncProgressDialog
/*we need derivation, not composition!
      1. SyncProgressDialogImpl IS a wxFrame/wxDialog
	  2. implement virtual ~wxFrame()
	  3. event handling below assumes lifetime is larger-equal than wxFrame's
*/
{
public:
    SyncProgressDialogImpl(long style, //wxFrame/wxDialog style
                           const std::function<wxFrame*(TopLevelDialog& progDlg)>& getTaskbarFrame,
                           AbortCallback& abortCb,
                           const std::function<void()>& notifyWindowTerminate,
                           const Statistics& syncStat,
                           wxFrame* parentFrame,
                           bool showProgress,
                           const wxString& jobName,
                           const std::wstring& execWhenFinished,
                           std::vector<std::wstring>& execFinishedHistory);
    virtual ~SyncProgressDialogImpl();

    //call this in StatusUpdater derived class destructor at the LATEST(!) to prevent access to currentStatusUpdater
    virtual void processHasFinished(SyncResult resultId, const ErrorLog& log);
    virtual void closeWindowDirectly();

    virtual wxWindow* getWindowIfVisible() { return this->IsShown() ? this : nullptr; }
    //workaround OS X bug: if "this" is used as parent window for a modal dialog then this dialog will erroneously un-hide its parent!

    virtual void initNewPhase();
    virtual void notifyProgressChange();
    virtual void updateGui() { updateGuiInt(true); }

    virtual std::wstring getExecWhenFinishedCommand() const { return pnl.m_comboBoxExecFinished->getValue(); }

    virtual void stopTimer() //halt all internal counters!
    {
        pnl.m_animCtrlSyncing->Stop();
        timeElapsed.Pause ();
    }
    virtual void resumeTimer()
    {
        pnl.m_animCtrlSyncing->Play();
        timeElapsed.Resume();
    }

private:
    void updateGuiInt(bool allowYield);

    void OnKeyPressed(wxKeyEvent& event);
    void OnOkay   (wxCommandEvent& event);
    void OnPause  (wxCommandEvent& event);
    void OnCancel (wxCommandEvent& event);
    void OnClose  (wxCloseEvent& event);
    void OnIconize(wxIconizeEvent& event);
    void OnMinimizeToTray(wxCommandEvent& event) { minimizeToTray(); }

    void minimizeToTray();
    void resumeFromSystray();

    void updateDialogStatus();
    void setExternalStatus(const wxString& status, const wxString& progress); //progress may be empty!

    SyncProgressPanelGenerated& pnl; //wxPanel containing the GUI controls of *this

    const wxString jobName_;
    wxStopWatch timeElapsed;

    wxFrame* parentFrame_; //optional

    std::function<void()> notifyWindowTerminate_; //call once in OnClose(), NOT in destructor which is called far too late somewhere in wxWidgets main loop!

    bool wereDead; //after wxWindow::Delete, which is equal to "delete this" on OS X!

    //status variables
    const Statistics* syncStat_;                  //
    AbortCallback*    abortCb_;                   //valid only while sync is running
    bool paused_; //valid only while sync is running
    SyncResult finalResult; //set after sync

    //remaining time
    std::unique_ptr<PerfCheck> perf;
    long lastStatCallSpeed;   //used for calculating intervals between collecting perf samples
    //help calculate total speed
    long phaseStartMs; //begin of current phase in [ms]

    std::shared_ptr<CurveDataStatistics>  curveDataBytes;
    std::shared_ptr<CurveDataStatistics>  curveDataItems;

    std::shared_ptr<CurveDataTotalValue  > curveDataBytesTotal;
    std::shared_ptr<CurveDataCurrentValue> curveDataBytesCurrent;
    std::shared_ptr<CurveDataTotalValue  > curveDataItemsTotal;
    std::shared_ptr<CurveDataCurrentValue> curveDataItemsCurrent;

    wxString parentFrameTitleBackup;
    std::unique_ptr<FfsTrayIcon> trayIcon; //optional: if filled all other windows should be hidden and conversely
    std::unique_ptr<Taskbar> taskbar_;
};


template <class TopLevelDialog>
SyncProgressDialogImpl<TopLevelDialog>::SyncProgressDialogImpl(long style, //wxFrame/wxDialog style
                                                               const std::function<wxFrame*(TopLevelDialog& progDlg)>& getTaskbarFrame,
                                                               AbortCallback& abortCb,
                                                               const std::function<void()>& notifyWindowTerminate,
                                                               const Statistics& syncStat,
                                                               wxFrame* parentFrame,
                                                               bool showProgress,
                                                               const wxString& jobName,
                                                               const std::wstring& execWhenFinished,
                                                               std::vector<std::wstring>& execFinishedHistory) :
    TopLevelDialog(parentFrame, wxID_ANY, wxString(), wxDefaultPosition, wxDefaultSize, style), //title is overwritten anyway in setExternalStatus()
    pnl(*new SyncProgressPanelGenerated(this)), //ownership passed to "this"
    jobName_  (jobName),
    parentFrame_(parentFrame),
    notifyWindowTerminate_(notifyWindowTerminate),
    wereDead(false),
    syncStat_ (&syncStat),
    abortCb_  (&abortCb),
    paused_   (false),
    finalResult(RESULT_ABORTED), //dummy value
    lastStatCallSpeed(-1000000), //some big number
    phaseStartMs(0)
{
    assert_static((IsSameType<TopLevelDialog, wxFrame >::value ||
                   IsSameType<TopLevelDialog, wxDialog>::value));
    assert((IsSameType<TopLevelDialog, wxFrame>::value == !parentFrame));

    //finish construction of this dialog:
    this->SetMinSize(wxSize(470, 280)); //== minimum size! no idea why SetMinSize() is not used...
    wxBoxSizer* bSizer170 = new wxBoxSizer(wxVERTICAL);
    bSizer170->Add(&pnl, 1, wxEXPAND);
    this->SetSizer(bSizer170); //pass ownership
    //this->Layout();
    //bSizer170->Fit(this);
    this->Centre(wxBOTH);

    //lifetime of event sources is subset of this instance's lifetime => no wxEvtHandler::Disconnect() needed
    this->Connect(wxEVT_CLOSE_WINDOW, wxCloseEventHandler  (SyncProgressDialogImpl<TopLevelDialog>::OnClose));
    this->Connect(wxEVT_ICONIZE,      wxIconizeEventHandler(SyncProgressDialogImpl<TopLevelDialog>::OnIconize));
    this->Connect(wxEVT_CHAR_HOOK, wxKeyEventHandler(SyncProgressDialogImpl::OnKeyPressed), nullptr, this);
    pnl.m_buttonClose ->Connect(wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler(SyncProgressDialogImpl::OnOkay  ), NULL, this);
    pnl.m_buttonPause ->Connect(wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler(SyncProgressDialogImpl::OnPause ), NULL, this);
    pnl.m_buttonCancel->Connect(wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler(SyncProgressDialogImpl::OnCancel), NULL, this);
    pnl.m_bpButtonMinimizeToTray->Connect(wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler(SyncProgressDialogImpl::OnMinimizeToTray), NULL, this);

#ifdef ZEN_WIN
    new MouseMoveWindow(*this); //allow moving main dialog by clicking (nearly) anywhere...; ownership passed to "this"
#endif

    assert(pnl.m_buttonClose->GetId() == wxID_OK); //we cannot use wxID_CLOSE else Esc key won't work: yet another wxWidgets bug??

    setRelativeFontSize(*pnl.m_staticTextPhase, 1.5);

    if (parentFrame_)
        parentFrameTitleBackup = parentFrame_->GetTitle(); //save old title (will be used as progress indicator)

    pnl.m_animCtrlSyncing->SetAnimation(GlobalResources::instance().aniWorking);
    pnl.m_animCtrlSyncing->Play();

    this->SetIcon(GlobalResources::instance().programIconFFS);

    this->EnableCloseButton(false); //this is NOT honored on OS X or during system shutdown on Windows!

    timeElapsed.Start(); //measure total time

    if (wxFrame* frame = getTaskbarFrame(*this))
        try //try to get access to Windows 7/Ubuntu taskbar
        {
            taskbar_ = make_unique<Taskbar>(*frame); //throw TaskbarNotAvailable
        }
        catch (const TaskbarNotAvailable&) {}

    //hide "processed" statistics until end of process
    pnl.m_notebookResult ->Hide();
    pnl.m_panelItemsProcessed->Hide();
    pnl.m_buttonClose    ->Show(false);
    //set std order after button visibility was set
    setStandardButtonOrder(*pnl.bSizerStdButtons, StdButtons().setAffirmative(pnl.m_buttonPause).setCancel(pnl.m_buttonCancel));

    pnl.m_bpButtonMinimizeToTray->SetBitmapLabel(getResourceImage(L"minimize_to_tray"));

    //init graph
    curveDataBytes        = std::make_shared<CurveDataStatistics>();
    curveDataItems        = std::make_shared<CurveDataStatistics>();
    curveDataBytesTotal   = std::make_shared<CurveDataTotalValue>();
    curveDataBytesCurrent = std::make_shared<CurveDataCurrentValue>();
    curveDataItemsTotal   = std::make_shared<CurveDataTotalValue>();
    curveDataItemsCurrent = std::make_shared<CurveDataCurrentValue>();

    const int xLabelHeight = 18; //we need to use the same height for both graphs to make sure they stretch evenly
    const int yLabelWidth  = 70;
    pnl.m_panelGraphBytes->setAttributes(Graph2D::MainAttributes().
                                         setLabelX(Graph2D::X_LABEL_BOTTOM, xLabelHeight, std::make_shared<LabelFormatterTimeElapsed>(true)).
                                         setLabelY(Graph2D::Y_LABEL_RIGHT,  yLabelWidth,  std::make_shared<LabelFormatterBytes>()).
                                         setSelectionMode(Graph2D::SELECT_NONE));

    pnl.m_panelGraphItems->setAttributes(Graph2D::MainAttributes().
                                         setLabelX(Graph2D::X_LABEL_BOTTOM, xLabelHeight, std::make_shared<LabelFormatterTimeElapsed>(false)).
                                         setLabelY(Graph2D::Y_LABEL_RIGHT,  yLabelWidth,  std::make_shared<LabelFormatterItemCount>()).
                                         setSelectionMode(Graph2D::SELECT_NONE));

    //pnl.m_panelGraphBytes->setAttributes(Graph2D::MainAttributes().setMinX(-1).setMaxX(1).setMinY(-1).setMaxY(1));

    pnl.m_panelGraphBytes->setCurve(curveDataBytes, Graph2D::CurveAttributes().setLineWidth(2).
                                    fillCurveArea(wxColor(205, 255, 202)).  //faint green
                                    setColor     (wxColor( 20, 200,   0))); //medium green

    pnl.m_panelGraphItems->setCurve(curveDataItems, Graph2D::CurveAttributes().setLineWidth(2).
                                    fillCurveArea(wxColor(198, 206, 255)).  //faint blue
                                    setColor     (wxColor( 90, 120, 255))); //medium blue

    pnl.m_panelGraphBytes->addCurve(curveDataBytesTotal,   Graph2D::CurveAttributes().setLineWidth(2).setColor(wxColor(12, 128, 0))); //dark green
    pnl.m_panelGraphBytes->addCurve(curveDataBytesCurrent, Graph2D::CurveAttributes().setLineWidth(1).setColor(wxColor(12, 128, 0))); //

    pnl.m_panelGraphItems->addCurve(curveDataItemsTotal,   Graph2D::CurveAttributes().setLineWidth(2).setColor(wxColor(53, 25, 255))); //dark blue
    pnl.m_panelGraphItems->addCurve(curveDataItemsCurrent, Graph2D::CurveAttributes().setLineWidth(1).setColor(wxColor(53, 25, 255))); //

    //allow changing on completion command
    pnl.m_comboBoxExecFinished->initHistory(execFinishedHistory, execFinishedHistory.size()); //-> we won't use addItemHistory() later
    pnl.m_comboBoxExecFinished->setValue(execWhenFinished);

    updateDialogStatus(); //null-status will be shown while waiting for dir locks

    this->Fit();
    pnl.Layout();

    if (showProgress)
    {
        this->Show();
#ifdef ZEN_MAC
        ProcessSerialNumber psn = { 0, kCurrentProcess };
        ::TransformProcessType(&psn, kProcessTransformToForegroundApplication); //show dock icon (consider non-silent batch mode)
        ::SetFrontProcess(&psn); //why isn't this covered by wxWindows::Raise()??
#endif

        pnl.m_buttonCancel->SetFocus(); //don't steal focus when starting in sys-tray!

        //clear gui flicker, remove dummy texts: window must be visible to make this work!
        updateGuiInt(true); //at least on OS X a real Yield() is required to flush pending GUI updates; Update() is not enough
    }
    else
        minimizeToTray();
}


template <class TopLevelDialog>
SyncProgressDialogImpl<TopLevelDialog>::~SyncProgressDialogImpl()
{
    if (parentFrame_)
    {
        parentFrame_->SetTitle(parentFrameTitleBackup); //restore title text

        //make sure main dialog is shown again if still "minimized to systray"! see SyncProgressDialog::closeWindowDirectly()
        parentFrame_->Show();
#ifdef ZEN_MAC
        ProcessSerialNumber psn = { 0, kCurrentProcess };
        ::TransformProcessType(&psn, kProcessTransformToForegroundApplication); //show dock icon (consider GUI mode with "close progress dialog")
        ::SetFrontProcess(&psn); //why isn't this covered by wxWindows::Raise()??
#endif
        //if (parentFrame_->IsIconized()) //caveat: if window is maximized calling Iconize(false) will erroneously un-maximize!
        //    parentFrame_->Iconize(false);
    }

    //our client is NOT expecting a second call via notifyWindowTerminate_()!
}


template <class TopLevelDialog>
void SyncProgressDialogImpl<TopLevelDialog>::OnKeyPressed(wxKeyEvent& event)
{
    const int keyCode = event.GetKeyCode();
    if (keyCode == WXK_ESCAPE)
    {
        wxCommandEvent dummy(wxEVT_COMMAND_BUTTON_CLICKED);

        //simulate click on abort button
        if (pnl.m_buttonCancel->IsShown()) //delegate to "cancel" button if available
        {
            if (wxEvtHandler* handler = pnl.m_buttonCancel->GetEventHandler())
                handler->ProcessEvent(dummy);
            return;
        }
        else if (pnl.m_buttonClose->IsShown())
        {
            if (wxEvtHandler* handler = pnl.m_buttonClose->GetEventHandler())
                handler->ProcessEvent(dummy);
            return;
        }
    }

    event.Skip();
}


template <class TopLevelDialog>
void SyncProgressDialogImpl<TopLevelDialog>::initNewPhase()
{
    updateDialogStatus(); //evaluates "syncStat_->currentPhase()"

    //reset graphs (e.g. after binary comparison)
    curveDataBytesTotal  ->setValue(0, 0);
    curveDataBytesCurrent->setValue(0, 0, 0);
    curveDataItemsTotal  ->setValue(0, 0);
    curveDataItemsCurrent->setValue(0, 0, 0);
    curveDataBytes->clear();
    curveDataItems->clear();

    notifyProgressChange();

    //start new measurement
    perf = make_unique<PerfCheck>(WINDOW_REMAINING_TIME, WINDOW_BYTES_PER_SEC);
    lastStatCallSpeed = -1000000; //some big number

    phaseStartMs = timeElapsed.Time();

    updateGuiInt(false);
}


template <class TopLevelDialog>
void SyncProgressDialogImpl<TopLevelDialog>::notifyProgressChange() //noexcept!
{
    if (syncStat_) //sync running
        switch (syncStat_->currentPhase())
        {
            case ProcessCallback::PHASE_NONE:
                assert(false);
            case ProcessCallback::PHASE_SCANNING:
                break;
            case ProcessCallback::PHASE_COMPARING_CONTENT:
            case ProcessCallback::PHASE_SYNCHRONIZING:
                curveDataBytes->addRecord(timeElapsed.Time(), to<double>(syncStat_->getDataCurrent   (syncStat_->currentPhase())));
                curveDataItems->addRecord(timeElapsed.Time(),            syncStat_->getObjectsCurrent(syncStat_->currentPhase()));
                break;
        }
}


namespace
{
#ifdef ZEN_WIN
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

    for (HWND hAbove = hTop; hAbove; hAbove = ::GetNextWindow(hAbove, GW_HWNDPREV))
        if (hAbove == hBottom)
            return ZORDER_WRONG;

    return ZORDER_INDEFINITE;
}
#endif


std::wstring getDialogPhaseText(const Statistics* syncStat, bool paused, SyncProgressDialog::SyncResult finalResult)
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
            case SyncProgressDialog::RESULT_ABORTED:
                return _("Aborted");
            case SyncProgressDialog::RESULT_FINISHED_WITH_ERROR:
            case SyncProgressDialog::RESULT_FINISHED_WITH_WARNINGS:
            case SyncProgressDialog::RESULT_FINISHED_WITH_SUCCESS:
                return _("Completed");
        }
    return std::wstring();
}
}


template <class TopLevelDialog>
void SyncProgressDialogImpl<TopLevelDialog>::setExternalStatus(const wxString& status, const wxString& progress) //progress may be empty!
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
    if (parentFrame_)
    {
        if (parentFrame_->GetTitle() != newCaption)
            parentFrame_->SetTitle(newCaption);
    }

    //always set a title: we don't wxGTK to show "nameless window" instead
    if (this->GetTitle() != newCaption)
        this->SetTitle(newCaption);
}


template <class TopLevelDialog>
void SyncProgressDialogImpl<TopLevelDialog>::updateGuiInt(bool allowYield)
{
    if (!syncStat_) //sync not running
        return;

    //wxWindowUpdateLocker dummy(this); -> not needed

    bool layoutChanged = false; //avoid screen flicker by calling layout() only if necessary
    const long timeNow = timeElapsed.Time();

    //sync status text
    setText(*pnl.m_staticTextStatus, replaceCpy(syncStat_->currentStatusText(), L'\n', L' ')); //no layout update for status texts!

    switch (syncStat_->currentPhase()) //no matter if paused or not
    {
        case ProcessCallback::PHASE_NONE:
        case ProcessCallback::PHASE_SCANNING:
            //dialog caption, taskbar, systray tooltip
            setExternalStatus(getDialogPhaseText(syncStat_, paused_, finalResult), toGuiString(syncStat_->getObjectsCurrent(ProcessCallback::PHASE_SCANNING))); //status text may be "paused"!

            //progress indicators
            if (trayIcon.get()) trayIcon->setProgress(1); //100% = regular FFS logo

            //ignore graphs: should already have been cleared in initNewPhase()

            //remaining objects and data
            setText(*pnl.m_staticTextRemainingObj , L"-", &layoutChanged);
            setText(*pnl.m_staticTextDataRemaining, L"", &layoutChanged);

            //remaining time and speed
            setText(*pnl.m_staticTextRemTime, L"-", &layoutChanged);
            pnl.m_panelGraphBytes->setAttributes(pnl.m_panelGraphBytes->getAttributes().setCornerText(wxString(), Graph2D::CORNER_TOP_LEFT));
            pnl.m_panelGraphItems->setAttributes(pnl.m_panelGraphItems->getAttributes().setCornerText(wxString(), Graph2D::CORNER_TOP_LEFT));
            break;

        case ProcessCallback::PHASE_COMPARING_CONTENT:
        case ProcessCallback::PHASE_SYNCHRONIZING:
        {
            const int   itemsCurrent = syncStat_->getObjectsCurrent(syncStat_->currentPhase());
            const int   itemsTotal   = syncStat_->getObjectsTotal  (syncStat_->currentPhase());
            const Int64 dataCurrent  = syncStat_->getDataCurrent   (syncStat_->currentPhase());
            const Int64 dataTotal    = syncStat_->getDataTotal     (syncStat_->currentPhase());

            //add both data + obj-count, to handle "deletion-only" cases
            const double fraction = dataTotal + itemsTotal == 0 ? 1 : std::max(0.0, to<double>(dataCurrent + itemsCurrent) / to<double>(dataTotal + itemsTotal));
            //yes, this may legitimately become < 0: failed rename operation falls-back to copy + delete, reducing "dataCurrent" to potentially < 0!
            //----------------------------------------------------------------------------------------------------

            //dialog caption, taskbar, systray tooltip
            setExternalStatus(getDialogPhaseText(syncStat_, paused_, finalResult), fractionToString(fraction)); //status text may be "paused"!

            //progress indicators
            if (trayIcon.get()) trayIcon->setProgress(fraction);
            if (taskbar_.get()) taskbar_->setProgress(fraction);

            //constant line graph
            curveDataBytesTotal  ->setValue(timeNow, to<double>(dataTotal));
            curveDataBytesCurrent->setValue(timeNow, to<double>(dataCurrent), to<double>(dataTotal));
            curveDataItemsTotal  ->setValue(timeNow, itemsTotal);
            curveDataItemsCurrent->setValue(timeNow, itemsCurrent, itemsTotal);
            //even though notifyProgressChange() already set the latest data, let's add another sample to have all curves consider "timeNow"
            //no problem with adding too many records: CurveDataStatistics will remove duplicate entries!
            curveDataBytes->addRecord(timeNow, to<double>(dataCurrent));
            curveDataItems->addRecord(timeNow, itemsCurrent);

            //remaining objects and data
            setText(*pnl.m_staticTextRemainingObj, toGuiString(itemsTotal - itemsCurrent), &layoutChanged);
            setText(*pnl.m_staticTextDataRemaining, L"(" + filesizeToShortString(dataTotal - dataCurrent) + L")", &layoutChanged);
            //it's possible data remaining becomes shortly negative if last file synced has ADS data and the dataTotal was not yet corrected!

            //remaining time and speed
            assert(perf);
            if (perf)
                if (numeric::dist(lastStatCallSpeed, timeNow) >= 500)
                {
                    lastStatCallSpeed = timeNow;

                    if (numeric::dist(phaseStartMs, timeNow) >= 1000) //discard stats for first second: probably messy
                        perf->addSample(itemsCurrent, to<double>(dataCurrent), timeNow);

                    //current speed -> Win 7 copy uses 1 sec update interval instead
                    Opt<std::wstring> bps = perf->getBytesPerSecond();
                    Opt<std::wstring> ips = perf->getItemsPerSecond();
                    pnl.m_panelGraphBytes->setAttributes(pnl.m_panelGraphBytes->getAttributes().setCornerText(bps ? *bps : L"", Graph2D::CORNER_TOP_LEFT));
                    pnl.m_panelGraphItems->setAttributes(pnl.m_panelGraphItems->getAttributes().setCornerText(ips ? *ips : L"", Graph2D::CORNER_TOP_LEFT));
                    //setText(*pnl.m_staticTextSpeed, perf->getBytesPerSecond(), &layoutChanged);

                    //remaining time: display with relative error of 10% - based on samples taken every 0.5 sec only
                    //-> call more often than once per second to correctly show last few seconds countdown, but don't call too often to avoid occasional jitter
                    Opt<std::wstring> rt = perf->getRemainingTime(to<double>(dataTotal - dataCurrent));
                    setText(*pnl.m_staticTextRemTime, rt ? *rt : L"-", &layoutChanged);
                }
        }
        break;
    }

    pnl.m_panelGraphBytes->Refresh();
    pnl.m_panelGraphItems->Refresh();

    //time elapsed
    const long timeElapSec = timeNow / 1000;
    setText(*pnl.m_staticTextTimeElapsed,
            timeElapSec < 3600 ?
            wxTimeSpan::Seconds(timeElapSec).Format(   L"%M:%S") :
            wxTimeSpan::Seconds(timeElapSec).Format(L"%H:%M:%S"), &layoutChanged);

    //adapt layout after content changes above
    if (layoutChanged)
    {
        pnl.m_panelProgress->Layout();
        //small statistics panels:
        //pnl.m_panelItemsProcessed->Layout();
        pnl.m_panelItemsRemaining->Layout();
        pnl.m_panelTimeRemaining ->Layout();
        //pnl.m_panelTimeElapsed->Layout(); -> needed?
    }

#ifdef ZEN_WIN
    //workaround Windows 7 bug messing up z-order after temporary application hangs: https://sourceforge.net/tracker/index.php?func=detail&aid=3376523&group_id=234430&atid=1093080
    //This is still needed no matter if wxDialog or wxPanel is used! (2013-07)
    if (parentFrame_)
        if (evaluateZorder(*this, *parentFrame_) == ZORDER_WRONG)
        {
            HWND hProgress = static_cast<HWND>(this->GetHWND());
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
            /*
            ZEN_ON_SCOPE_EXIT(resumeTimer()); -> crashes on Fedora; WHY???
            => likely compiler bug!!!
               1. no crash on Fedora for: ZEN_ON_SCOPE_EXIT(this->resumeTimer());
               1. no crash if we derive from wxFrame instead of template "TopLevelDialog"
               2. no crash on Ubuntu GCC
               3. following makes GCC crash already during compilation: auto dfd = zen::makeGuard([this]{ resumeTimer(); });
            */

            stopTimer();

            while (paused_)
            {
                wxTheApp->Yield(); //receive UI message that end pause OR forceful termination!
                //*first* refresh GUI (removing flicker) before sleeping!
                boost::this_thread::sleep(boost::posix_time::milliseconds(UI_UPDATE_INTERVAL));
            }
            //if SyncProgressDialogImpl::OnClose() already wxWindow::Destroy() on OS X then this instance is already toast!
            if (wereDead)
                return; //GTFO and don't call this->resumeTimer()

            resumeTimer();
        }
        else
            /*
                /|\
                 |   keep this sequence to ensure one full progress update before entering pause mode!
                \|/
            */
            wxTheApp->Yield(); //receive UI message that sets pause status OR forceful termination!
    }
    else
        this->Update(); //don't wait until next idle event (who knows what blocking process comes next?)
}


template <class TopLevelDialog>
void SyncProgressDialogImpl<TopLevelDialog>::updateDialogStatus() //depends on "syncStat_, paused_, finalResult"
{
    const wxString dlgStatusTxt = getDialogPhaseText(syncStat_, paused_, finalResult);

    pnl.m_staticTextPhase->SetLabel(dlgStatusTxt);

    //status bitmap
    if (syncStat_) //sync running
    {
        auto setStatusBitmap = [&](const wchar_t* bmpName)
        {
            pnl.m_animCtrlSyncing->Hide();
            pnl.m_bitmapStatus->SetBitmap(getResourceImage(bmpName));
            pnl.m_bitmapStatus->SetToolTip(dlgStatusTxt);
            pnl.m_bitmapStatus->Show();
        };

        if (paused_)
            setStatusBitmap(L"status_pause");
        else
            switch (syncStat_->currentPhase())
            {
                case ProcessCallback::PHASE_NONE:
                    pnl.m_animCtrlSyncing->Hide();
                    pnl.m_bitmapStatus->Hide();
                    break;

                case ProcessCallback::PHASE_SCANNING:
                    setStatusBitmap(L"status_scanning");
                    break;

                case ProcessCallback::PHASE_COMPARING_CONTENT:
                    setStatusBitmap(L"status_binary_compare");
                    break;

                case ProcessCallback::PHASE_SYNCHRONIZING:
                    pnl.m_bitmapStatus->SetBitmap(getResourceImage(L"status_syncing"));
                    pnl.m_bitmapStatus->SetToolTip(dlgStatusTxt);
                    pnl.m_bitmapStatus->Show();
                    pnl.m_animCtrlSyncing->Show();
                    pnl.m_animCtrlSyncing->SetToolTip(dlgStatusTxt);
                    break;
            }
    }
    else //sync finished
    {
        auto setStatusBitmap = [&](const wchar_t* bmpName, const std::wstring& tooltip)
        {
            pnl.m_animCtrlSyncing->Hide();
            pnl.m_bitmapStatus->SetBitmap(getResourceImage(bmpName));
            pnl.m_bitmapStatus->Show();
            pnl.m_bitmapStatus->SetToolTip(tooltip);
        };
        switch (finalResult)
        {
            case RESULT_ABORTED:
                setStatusBitmap(L"status_aborted", _("Synchronization aborted"));
                break;

            case RESULT_FINISHED_WITH_ERROR:
                setStatusBitmap(L"status_finished_errors", _("Synchronization completed with errors"));
                break;

            case RESULT_FINISHED_WITH_WARNINGS:
                setStatusBitmap(L"status_finished_warnings", _("Synchronization completed with warnings"));
                break;

            case RESULT_FINISHED_WITH_SUCCESS:
                setStatusBitmap(L"status_finished_success", _("Synchronization completed successfully"));
                break;
        }
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

                case RESULT_FINISHED_WITH_WARNINGS:
                case RESULT_FINISHED_WITH_SUCCESS:
                    taskbar_->setStatus(Taskbar::STATUS_NORMAL);
                    break;
            }
    }

    //pause button
    if (syncStat_) //sync running
        pnl.m_buttonPause->SetLabel(paused_ ? _("&Continue") : _("&Pause"));

    pnl.Layout();
    this->Refresh(); //a few pixels below the status text need refreshing
}


template <class TopLevelDialog>
void SyncProgressDialogImpl<TopLevelDialog>::closeWindowDirectly() //this should really be called: do not call back + schedule deletion
{
    paused_ = false; //you never know?
    //ATTENTION: dialog may live a little longer, so watch callbacks!
    //e.g. wxGTK calls OnIconize after wxWindow::Close() (better not ask why) and before physical destruction! => indirectly calls updateDialogStatus(), which reads syncStat_!!!
    syncStat_ = nullptr;
    abortCb_  = nullptr;
    //resumeFromSystray(); -> NO, instead ~SyncProgressDialogImpl() makes sure that main dialog is shown again!

    this->Close(); //generate close event: do NOT destroy window unconditionally!
}


template <class TopLevelDialog>
void SyncProgressDialogImpl<TopLevelDialog>::processHasFinished(SyncResult resultId, const ErrorLog& log) //essential to call this in StatusHandler derived class destructor
{
    //at the LATEST(!) to prevent access to currentStatusHandler
    //enable okay and close events; may be set in this method ONLY

#if (defined __WXGTK__ || defined __WXOSX__)
    //In wxWidgets 2.9.3 upwards, the wxWindow::Reparent() below fails on GTK and OS X if window is frozen! http://forums.codeblocks.org/index.php?topic=13388.45
#else
    wxWindowUpdateLocker dummy(this); //badly needed on Windows
#endif

    paused_ = false; //you never know?

    //update numbers one last time (as if sync were still running)
    notifyProgressChange(); //make one last graph entry at the *current* time
    updateGuiInt(false);

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
            const int   itemsCurrent = syncStat_->getObjectsCurrent(syncStat_->currentPhase());
            const int   itemsTotal   = syncStat_->getObjectsTotal  (syncStat_->currentPhase());
            const Int64 dataCurrent  = syncStat_->getDataCurrent   (syncStat_->currentPhase());
            const Int64 dataTotal    = syncStat_->getDataTotal     (syncStat_->currentPhase());
            assert(dataCurrent <= dataTotal);

            //set overall speed (instead of current speed)
            const long timeDelta = timeElapsed.Time() - phaseStartMs; //we need to consider "time within current phase" not total "timeElapsed"!

            const wxString overallBytesPerSecond = timeDelta == 0 ? wxString() : filesizeToShortString(dataCurrent * 1000 / timeDelta) + _("/sec");
            const wxString overallItemsPerSecond = timeDelta == 0 ? wxString() : replaceCpy(_("%x items"), L"%x", formatThreeDigitPrecision(itemsCurrent * 1000.0 / timeDelta)) + _("/sec");

            pnl.m_panelGraphBytes->setAttributes(pnl.m_panelGraphBytes->getAttributes().setCornerText(overallBytesPerSecond, Graph2D::CORNER_TOP_LEFT));
            pnl.m_panelGraphItems->setAttributes(pnl.m_panelGraphItems->getAttributes().setCornerText(overallItemsPerSecond, Graph2D::CORNER_TOP_LEFT));

            //show new element "items processed"
            pnl.m_panelItemsProcessed->Show();
            pnl.m_staticTextProcessedObj ->SetLabel(toGuiString(itemsCurrent));
            pnl.m_staticTextDataProcessed->SetLabel(L"(" + filesizeToShortString(dataCurrent) + L")");

            //hide remaining elements...
            if (itemsCurrent == itemsTotal && //...if everything was processed successfully
                dataCurrent == dataTotal)
                pnl.m_panelItemsRemaining->Hide();
        }
        break;
    }

    //------- change class state -------
    finalResult = resultId;

    syncStat_ = nullptr;
    abortCb_  = nullptr;
    //----------------------------------

    updateDialogStatus();
    setExternalStatus(getDialogPhaseText(syncStat_, paused_, finalResult), wxString());

    resumeFromSystray(); //if in tray mode...

    this->EnableCloseButton(true);

    pnl.m_bpButtonMinimizeToTray->Hide();
    pnl.m_buttonCancel->Disable();
    pnl.m_buttonCancel->Hide();
    pnl.m_buttonPause->Disable();
    pnl.m_buttonPause->Hide();
    pnl.m_buttonClose->Show();
    pnl.m_buttonClose->Enable();

    pnl.m_buttonClose->SetFocus();

    pnl.bSizerExecFinished->Show(false);

    //set std order after button visibility was set
    setStandardButtonOrder(*pnl.bSizerStdButtons, StdButtons().setAffirmative(pnl.m_buttonClose));

    //hide current operation status
    pnl.bSizerStatusText->Show(false);

    //show and prepare final statistics
    pnl.m_notebookResult->Show();

#if defined ZEN_WIN || defined ZEN_LINUX
    pnl.m_staticlineFooter->Hide(); //win: m_notebookResult already has a window frame
#endif

    //hide remaining time
    pnl.m_panelTimeRemaining->Hide();

    //1. re-arrange graph into results listbook
    const bool wasDetached = pnl.bSizerRoot->Detach(pnl.m_panelProgress);
    assert(wasDetached);
    (void)wasDetached;
    pnl.m_panelProgress->Reparent(pnl.m_notebookResult);
    pnl.m_notebookResult->AddPage(pnl.m_panelProgress, _("Statistics"), true);

    //2. log file
    const size_t posLog = 1;
    assert(pnl.m_notebookResult->GetPageCount() == 1);
    LogPanel* logPanel = new LogPanel(pnl.m_notebookResult, log); //owned by m_notebookResult
    pnl.m_notebookResult->AddPage(logPanel, _("Log"), false);
    //bSizerHoldStretch->Insert(0, logPanel, 1, wxEXPAND);

    //show log instead of graph if errors occurred! (not required for ignored warnings)
    if (log.getItemCount(TYPE_ERROR | TYPE_FATAL_ERROR) > 0)
        pnl.m_notebookResult->ChangeSelection(posLog);

    //this->Fit(); //not a good idea: will shrink even if window is maximized or was enlarged by the user
    pnl.Layout();

    pnl.m_panelProgress->Layout();
    //small statistics panels:
    pnl.m_panelItemsProcessed->Layout();
    pnl.m_panelItemsRemaining->Layout();
    //pnl.m_panelTimeRemaining->Layout();
    //pnl.m_panelTimeElapsed->Layout(); -> needed?

    //play (optional) sound notification after sync has completed -> only play when waiting on results dialog, seems to be pointless otherwise!
    switch (finalResult)
    {
        case SyncProgressDialog::RESULT_ABORTED:
            break;
        case SyncProgressDialog::RESULT_FINISHED_WITH_ERROR:
        case SyncProgressDialog::RESULT_FINISHED_WITH_WARNINGS:
        case SyncProgressDialog::RESULT_FINISHED_WITH_SUCCESS:
        {
            const Zstring soundFile = getResourceDir() + Zstr("Sync_Complete.wav");
            if (fileExists(soundFile))
                wxSound::Play(utfCvrtTo<wxString>(soundFile), wxSOUND_ASYNC); //warning: this may fail and show a wxWidgets error message! => must not play when running FFS as a service!
        }
        break;
    }

    //Raise(); -> don't! user may be watching a movie in the meantime ;) note: resumeFromSystray() also calls Raise()!
}


template <class TopLevelDialog>
void SyncProgressDialogImpl<TopLevelDialog>::OnOkay(wxCommandEvent& event)
{
    this->Close(); //generate close event: do NOT destroy window unconditionally!
}


template <class TopLevelDialog>
void SyncProgressDialogImpl<TopLevelDialog>::OnCancel(wxCommandEvent& event)
{
    paused_ = false;
    updateDialogStatus(); //update status + pause button

    //no Layout() or UI-update here to avoid cascaded Yield()-call
    if (abortCb_)
        abortCb_->requestAbortion();
}


template <class TopLevelDialog>
void SyncProgressDialogImpl<TopLevelDialog>::OnPause(wxCommandEvent& event)
{
    paused_ = !paused_;
    updateDialogStatus(); //update status + pause button
}


template <class TopLevelDialog>
void SyncProgressDialogImpl<TopLevelDialog>::OnClose(wxCloseEvent& event)
{
    //this event handler may be called *during* sync, e.g. due to a system shutdown (Windows), anytime (OS X)
    //try to stop sync gracefully and cross fingers:
    if (abortCb_)
        abortCb_->requestAbortion();
    //Note: we must NOT veto dialog destruction, else we will cancel system shutdown if this dialog is application main window (like in batch mode)

    notifyWindowTerminate_(); //don't wait until delayed "Destroy()" finally calls destructor -> avoid calls to processHasFinished()/closeWindowDirectly()

    paused_ = false; //[!] we could be pausing here!

    //now that we notified window termination prematurely, and since processHasFinished()/closeWindowDirectly() won't be called, make sure we don't call back, too!
    //e.g. the second notifyWindowTerminate_() in ~SyncProgressDialogImpl()!!!
    syncStat_ = nullptr;
    abortCb_  = nullptr;

    wereDead = true;
    this->Destroy(); //wxWidgets OS X: simple "delete"!!!!!!!
}


template <class TopLevelDialog>
void SyncProgressDialogImpl<TopLevelDialog>::OnIconize(wxIconizeEvent& event)
{
    /*
    	propagate progress dialog minimize/maximize to parent
    	-----------------------------------------------------
        Fedora/Debian/Ubuntu:
    		- wxDialog cannot be minimized
    		- worse, wxGTK sends stray iconize events *after* wxDialog::Destroy()
    		- worse, on Fedora an iconize event is issued directly after calling Close()
    		- worse, even wxDialog::Hide() causes iconize event!
    			=> nothing to do
    	SUSE:
    		- wxDialog can be minimized (it just vanishes!) and in general also minimizes parent: except for our progress wxDialog!!!
    		- worse, wxDialog::Hide() causes iconize event
    		- probably the same issues with stray iconize events like Fedora/Debian/Ubuntu
    		- minimize button is always shown, even if wxMINIMIZE_BOX is omitted!
    			=> nothing to do
    	Mac OS X:
    		- wxDialog can be minimized and automatically minimizes parent
    		- no iconize events seen by wxWidgets!
    			=> nothing to do
    	Windows:
    		- wxDialog can be minimized but does not also minimize parent
    		- iconize events only seen for manual minimize
    			=> propagate event to parent
    */
#ifdef ZEN_WIN
    if (parentFrame_)
        if (parentFrame_->IsIconized() != event.IsIconized()) //caveat: if window is maximized calling Iconize(false) will erroneously un-maximize!
            parentFrame_->Iconize(event.IsIconized());
#endif
    event.Skip();
}


template <class TopLevelDialog>
void SyncProgressDialogImpl<TopLevelDialog>::minimizeToTray()
{
    if (!trayIcon.get())
    {
        trayIcon = make_unique<FfsTrayIcon>([this] { this->resumeFromSystray(); }); //FfsTrayIcon lifetime is a subset of "this"'s lifetime!
        //we may destroy FfsTrayIcon even while in the FfsTrayIcon callback!!!!

        updateGuiInt(false); //set tray tooltip + progress: e.g. no updates while paused

        this->Hide();
        if (parentFrame_)
            parentFrame_->Hide();
#ifdef ZEN_MAC
        //hide dock icon: else user is able to forcefully show the hidden main dialog by clicking on the icon!!
        ProcessSerialNumber psn = { 0, kCurrentProcess };
        ::TransformProcessType(&psn, kProcessTransformToUIElementApplication);
#endif
    }
}


template <class TopLevelDialog>
void SyncProgressDialogImpl<TopLevelDialog>::resumeFromSystray()
{
    if (trayIcon)
    {
        trayIcon.reset();

        if (parentFrame_)
        {
            //if (parentFrame_->IsIconized()) //caveat: if window is maximized calling Iconize(false) will erroneously un-maximize!
            //    parentFrame_->Iconize(false);
            parentFrame_->Show();
            parentFrame_->Raise();
        }

        //if (IsIconized()) //caveat: if window is maximized calling Iconize(false) will erroneously un-maximize!
        //    Iconize(false);
        this->Show();
        this->Raise();
        this->SetFocus();

        updateDialogStatus(); //restore Windows 7 task bar status   (e.g. required in pause mode)
        updateGuiInt(false);  //restore Windows 7 task bar progress (e.g. required in pause mode)

#ifdef ZEN_MAC
        ProcessSerialNumber psn = { 0, kCurrentProcess };
        ::TransformProcessType(&psn, kProcessTransformToForegroundApplication); //show dock icon again
        ::SetFrontProcess(&psn); //why isn't this covered by wxWindows::Raise()??
#endif
    }
}

//########################################################################################

SyncProgressDialog* createProgressDialog(zen::AbortCallback& abortCb,
                                         const std::function<void()>& notifyWindowTerminate, //note: user closing window cannot be prevented on OS X! (And neither on Windows during system shutdown!)
                                         const zen::Statistics& syncStat,
                                         wxFrame* parentWindow, //may be nullptr
                                         bool showProgress,
                                         const wxString& jobName,
                                         const std::wstring& execWhenFinished,
                                         std::vector<std::wstring>& execFinishedHistory)
{
    if (parentWindow) //sync from GUI
        return new SyncProgressDialogImpl<wxDialog>(wxDEFAULT_DIALOG_STYLE | wxMAXIMIZE_BOX | wxMINIMIZE_BOX | wxRESIZE_BORDER,
        [&](wxDialog& progDlg) { return parentWindow; }, abortCb,
    notifyWindowTerminate, syncStat, parentWindow, showProgress, jobName, execWhenFinished, execFinishedHistory);
    else //FFS batch job
        return new SyncProgressDialogImpl<wxFrame>(wxDEFAULT_FRAME_STYLE,
        [](wxFrame& progDlg) { return &progDlg; }, abortCb,
    notifyWindowTerminate, syncStat, parentWindow, showProgress, jobName, execWhenFinished, execFinishedHistory);
}
