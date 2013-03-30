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
#include <zen/basic_math.h>
#include <zen/format_unit.h>
#include <wx+/mouse_move_dlg.h>
#include <wx+/toggle_button.h>
#include <wx+/image_tools.h>
#include <wx+/graph.h>
#include <wx+/context_menu.h>
#include <wx+/no_flicker.h>
#include <wx+/font_size.h>
#include <zen/file_handling.h>
#include "gui_generated.h"
#include "../lib/ffs_paths.h"
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
const int WINDOW_REMAINING_TIME = 60000; //some use cases have dropouts of 40 seconds -> 60 sec. window size handles them well
const int WINDOW_BYTES_PER_SEC  =  5000; //
}


class CompareProgressDialog::Pimpl : public CompareProgressDlgGenerated
{
public:
    Pimpl(wxTopLevelWindow& parentWindow);

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
};


CompareProgressDialog::Pimpl::Pimpl(wxTopLevelWindow& parentWindow) :
    CompareProgressDlgGenerated(&parentWindow),
    parentWindow_(parentWindow),
    syncStat_(nullptr),
    lastStatCallSpeed  (-1000000) //some big number
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
            if (perf)
            {
                if (numeric::dist(lastStatCallSpeed, timeElapsed.Time()) >= 500)
                {
                    lastStatCallSpeed = timeElapsed.Time();

                    perf->addSample(objectsCurrent, to<double>(dataCurrent), timeElapsed.Time());

                    //current speed -> Win 7 copy uses 1 sec update interval
                    setText(*m_staticTextSpeed, perf->getBytesPerSecond(), &layoutChanged);
                }

                warn_static("more often: eveyr 100 ms?")
                //remaining time
                setText(*m_staticTextRemTime, perf->getRemainingTime(to<double>(dataTotal - dataCurrent)), &layoutChanged);
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
CompareProgressDialog::CompareProgressDialog(wxTopLevelWindow& parentWindow) :
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

    zen::move(output, 0, -1); //move image right one pixel
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
                std::for_each(it->message.begin(), it->message.end(),
                              [&](wchar_t c)
                {
                    typedef Line Line; //workaround MSVC compiler bug!

                    if (c == L'\n')
                    {
                        if (!lastCharNewline) //do not reference empty lines!
                            viewRef.push_back(Line(&*it, rowNumber));
                        ++rowNumber;
                        lastCharNewline = true;
                    }
                    else
                        lastCharNewline = false;
                });
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
                    drawCellText(dc, rectTmp, getValue(row, colType), grid.IsEnabled(), wxALIGN_CENTER);
                    break;

                case COL_TYPE_MSG_CATEGORY:
                    if (entry.firstLine)
                        switch (entry.type)
                        {
                            case TYPE_INFO:
                                dc.DrawLabel(wxString(), getResourceImage(L"msg_small_info"), rectTmp, wxALIGN_CENTER);
                                break;
                            case TYPE_WARNING:
                                dc.DrawLabel(wxString(), getResourceImage(L"msg_small_warning"), rectTmp, wxALIGN_CENTER);
                                break;
                            case TYPE_ERROR:
                            case TYPE_FATAL_ERROR:
                                dc.DrawLabel(wxString(), getResourceImage(L"msg_small_error"), rectTmp, wxALIGN_CENTER);
                                break;
                        }
                    break;

                case COL_TYPE_MSG_TEXT:
                {
                    rectTmp.x     += COLUMN_BORDER_LEFT;
                    rectTmp.width -= COLUMN_BORDER_LEFT;
                    drawCellText(dc, rectTmp, getValue(row, colType), grid.IsEnabled());
                }
                break;
            }
    }

    virtual size_t getBestSize(wxDC& dc, size_t row, ColumnType colType)
    {
        // -> synchronize renderCell() <-> getBestSize()

        MessageView::LogEntryView entry = {};
        if (msgView_ && msgView_->getEntry(row, entry))
            switch (static_cast<ColumnTypeMsg>(colType))
            {
                case COL_TYPE_MSG_TIME:
                    return 2 * COLUMN_BORDER_LEFT + dc.GetTextExtent(getValue(row, colType)).GetWidth();

                case COL_TYPE_MSG_CATEGORY:
                    return getResourceImage(L"msg_small_info").GetWidth();

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
        return getResourceImage(L"msg_small_info").GetWidth();
    }

    static int getRowDefaultHeight(const Grid& grid)
    {
        return std::max(getResourceImage(L"msg_small_info").GetHeight(), grid.getMainWin().GetCharHeight() + 2) + 1; //+ some space + bottom border
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


class LogControl : public LogControlGenerated
{
public:
    LogControl(wxWindow* parent, const ErrorLog& log) : LogControlGenerated(parent),
        msgView(std::make_shared<MessageView>(log))
    {
        const int errorCount   = log.getItemCount(TYPE_ERROR | TYPE_FATAL_ERROR);
        const int warningCount = log.getItemCount(TYPE_WARNING);
        const int infoCount    = log.getItemCount(TYPE_INFO);

        m_bpButtonErrors  ->init(buttonPressed ("msg_error"  ), buttonReleased("msg_error"  ), _("Error"  ) + wxString::Format(L" (%d)", errorCount  ));
        m_bpButtonWarnings->init(buttonPressed ("msg_warning"), buttonReleased("msg_warning"), _("Warning") + wxString::Format(L" (%d)", warningCount));
        m_bpButtonInfo    ->init(buttonPressed ("msg_info"   ), buttonReleased("msg_info"   ), _("Info"   ) + wxString::Format(L" (%d)", infoCount   ));

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
        m_gridMessages->getMainWin().Connect(wxEVT_KEY_DOWN, wxKeyEventHandler(LogControl::onGridButtonEvent), nullptr, this);

        m_gridMessages->Connect(EVENT_GRID_MOUSE_RIGHT_UP, GridClickEventHandler(LogControl::onMsgGridContext), nullptr, this);

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
            wxMessageBox(_("Out of memory!") + L" " + utfCvrtTo<std::wstring>(e.what()), _("Error"), wxOK | wxICON_ERROR);
        }
    }

    std::shared_ptr<MessageView> msgView; //bound!
};

//########################################################################################

namespace
{
class GraphDataBytes : public GraphData
{
public:
    void addRecord(double currentBytes, long timeMs)
    {
        data.insert(data.end(), std::make_pair(timeMs, currentBytes));
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
        auto it = data.lower_bound(x * 1000);
        if (it == data.end())
            return data.empty() ? 0 : (--data.end())->second;
        return it->second;
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
        assert(1 < a && a < 2);
        return bestFit(a, 1, 2) * e;
    }

    virtual wxString formatText(double value, double optimalBlockSize) const { return filesizeToShortString(Int64(value)); };
};


struct LabelFormatterTimeElapsed : public LabelFormatter
{
    virtual double getOptimalBlockSize(double secProposed) const
    {
        const double stepsSec[] = { 10, 20, 30, 60 }; //10 sec: minimum block size; no 15: avoid flicker between 10<->15<->20 sec blocks
        if (secProposed <= 60)
            return numeric::nearMatch(secProposed, std::begin(stepsSec), std::end(stepsSec));

        const double stepsMin[] = { 1, 2, 5, 10, 15, 20, 30, 60 }; //nice numbers for minutes
        if (secProposed <= 3600)
            return 60.0 * numeric::nearMatch(secProposed / 60, std::begin(stepsMin), std::end(stepsMin));

        if (secProposed <= 3600 * 24)
            return nextNiceNumber(secProposed / 3600) * 3600;

        return nextNiceNumber(secProposed / (24 * 3600)) * 24 * 3600; //round up to full days
    }

    virtual wxString formatText(double timeElapsed, double optimalBlockSize) const
    {
        return timeElapsed < 60 ?
               replaceCpy(_P("1 sec", "%x sec", numeric::round(timeElapsed)), L"%x", zen::numberTo<std::wstring>(numeric::round(timeElapsed))) :
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


class SyncProgressDialog::Pimpl : public SyncProgressDlgGenerated
{
public:
    Pimpl(AbortCallback& abortCb,
          const Statistics& syncStat,
          MainDialog* parentWindow,
          const wxString& jobName,
          const std::wstring& execWhenFinished,
          std::vector<std::wstring>& execFinishedHistory);
    ~Pimpl();

    void initNewPhase();
    void notifyProgressChange();
    void updateGui(bool allowYield = true);

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
    //help calculate total speed
    long phaseStartMs; //begin of current phase in [ms]

    std::shared_ptr<GraphDataBytes> graphDataBytes;
    std::shared_ptr<GraphDataConstLine> graphDataBytesTotal;

    wxString titelTextBackup;
    std::unique_ptr<FfsTrayIcon> trayIcon; //optional: if filled all other windows should be hidden and conversely
    std::unique_ptr<Taskbar> taskbar_;
};


SyncProgressDialog::Pimpl::Pimpl(AbortCallback& abortCb,
                                 const Statistics& syncStat,
                                 MainDialog* parentWindow,
                                 const wxString& jobName,
                                 const std::wstring& execWhenFinished,
                                 std::vector<std::wstring>& execFinishedHistory) :
    SyncProgressDlgGenerated(parentWindow,
                             wxID_ANY,
                             parentWindow ? wxString() : (wxString(L"FreeFileSync - ") + _("Folder Comparison and Synchronization")),
                             wxDefaultPosition, wxDefaultSize,
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
    lastStatCallSpeed(-1000000), //some big number
    phaseStartMs(0)
{
#ifdef FFS_WIN
    new MouseMoveWindow(*this); //allow moving main dialog by clicking (nearly) anywhere...; ownership passed to "this"
#endif

    assert(m_buttonClose->GetId() == wxID_OK); //we cannot use wxID_CLOSE else Esc key won't work: yet another wxWidgets bug??

    setRelativeFontSize(*m_staticTextPhase, 1.5);

    if (mainDialog)
    {
        titelTextBackup = mainDialog->GetTitle(); //save old title (will be used as progress indicator)
        mainDialog->disableAllElements(false); //disable all child elements
    }

    m_animationControl1->SetAnimation(GlobalResources::instance().aniSync);
    m_animationControl1->Play();

    SetIcon(GlobalResources::instance().programIcon);

    //initialize gauge
    m_gauge1->SetRange(GAUGE_FULL_RANGE);
    m_gauge1->SetValue(0);

    warn_static("not honored on osx")

    EnableCloseButton(false);

    if (IsShown()) //don't steal focus when starting in sys-tray!
        m_buttonAbort->SetFocus();

    timeElapsed.Start(); //measure total time

    try //try to get access to Windows 7/Ubuntu taskbar
    {
        taskbar_ = make_unique<Taskbar>(mainDialog ? *static_cast<wxTopLevelWindow*>(mainDialog) : *this);
    }
    catch (const TaskbarNotAvailable&) {}

    //hide "processed" statistics until end of process
    m_listbookResult          ->Hide();
    m_staticTextLabelItemsProc->Show(false);
    bSizerItemsProc           ->Show(false);
    m_buttonClose             ->Show(false);
    Layout();

    //register key event
    Connect(wxEVT_CHAR_HOOK, wxKeyEventHandler(Pimpl::OnKeyPressed), nullptr, this);

    //init graph
    graphDataBytes      = std::make_shared<GraphDataBytes>();
    graphDataBytesTotal = std::make_shared<GraphDataConstLine>();

    m_panelGraph->setAttributes(Graph2D::MainAttributes().
                                setLabelX(Graph2D::X_LABEL_BOTTOM, 20, std::make_shared<LabelFormatterTimeElapsed>()).
                                setLabelY(Graph2D::Y_LABEL_RIGHT,  70, std::make_shared<LabelFormatterBytes>()).
                                setSelectionMode(Graph2D::SELECT_NONE));

    m_panelGraph->setData(graphDataBytes,
                          Graph2D::CurveAttributes().setLineWidth(2).
                          setColor     (wxColor(  0, 192,   0)).  //medium green
                          fillCurveArea(wxColor(192, 255, 192))); //faint green

    m_panelGraph->addData(graphDataBytesTotal, Graph2D::CurveAttributes().setLineWidth(2).setColor(wxColor(0, 64, 0))); //dark green

    //allow changing on completion command
    m_comboBoxExecFinished->initHistory(execFinishedHistory, execFinishedHistory.size()); //-> we won't use addItemHistory() later
    m_comboBoxExecFinished->setValue(execWhenFinished);

    updateDialogStatus(); //null-status will be shown while waiting for dir locks (if at all)

    Fit();
}


SyncProgressDialog::Pimpl::~Pimpl()
{
    if (mainDialog)
    {
        mainDialog->enableAllElements();
        mainDialog->SetTitle(titelTextBackup); //restore title text

        //make sure main dialog is shown again if still "minimized to systray"! see SyncProgressDialog::closeWindowDirectly()
        if (mainDialog->IsIconized()) //caveat: if window is maximized calling Iconize(false) will erroneously un-maximize!
            mainDialog->Iconize(false);

        mainDialog->Show();
    }
}


void SyncProgressDialog::Pimpl::OnKeyPressed(wxKeyEvent& event)
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
        else if (m_buttonClose->IsShown())
        {
            if (wxEvtHandler* handler = m_buttonClose->GetEventHandler())
                handler->ProcessEvent(dummy);
            return;
        }
    }

    event.Skip();
}


void SyncProgressDialog::Pimpl::initNewPhase()
{
    updateDialogStatus(); //evaluates "syncStat_->currentPhase()"

    //reset graph (e.g. after binary comparison)
    graphDataBytes->clear();
    notifyProgressChange();

    //start new measurement
    perf = make_unique<PerfCheck>(WINDOW_REMAINING_TIME, WINDOW_BYTES_PER_SEC);
    lastStatCallSpeed   = -1000000; //some big number

    phaseStartMs = timeElapsed.Time();

    //set to 0 even if totalDataToProcess is 0: due to a bug in wxGauge::SetValue, it doesn't change to determinate mode when setting the old value again
    //so give updateGui() a chance to set a different value
    m_gauge1->SetValue(0);

    updateGui(false);
}


void SyncProgressDialog::Pimpl::notifyProgressChange() //noexcept!
{
    if (syncStat_)
    {
        switch (syncStat_->currentPhase())
        {
            case ProcessCallback::PHASE_NONE:
                assert(false);
            case ProcessCallback::PHASE_SCANNING:
                break;
            case ProcessCallback::PHASE_COMPARING_CONTENT:
            case ProcessCallback::PHASE_SYNCHRONIZING:
            {
                const double currentData = to<double>(syncStat_->getDataCurrent(syncStat_->currentPhase()));

                //add sample for perf measurements + calc. of remaining time
                graphDataBytes->addRecord(currentData, timeElapsed.Time());
            }
            break;
        }
    }
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


void SyncProgressDialog::Pimpl::setExternalStatus(const wxString& status, const wxString& progress) //progress may be empty!
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


void SyncProgressDialog::Pimpl::updateGui(bool allowYield)
{
    assert(syncStat_);
    if (!syncStat_) //no sync running!!
        return;

    //wxWindowUpdateLocker dummy(this); -> not needed

    bool layoutChanged = false; //avoid screen flicker by calling layout() only if necessary

    //sync status text
    setText(*m_staticTextStatus, replaceCpy(syncStat_->currentStatusText(), L'\n', L' ')); //no layout update for status texts!

    switch (syncStat_->currentPhase()) //no matter if paused or not
    {
        case ProcessCallback::PHASE_NONE:
        case ProcessCallback::PHASE_SCANNING:
            //dialog caption, taskbar, systray tooltip
            setExternalStatus(getDialogPhaseText(syncStat_, paused_, finalResult), toGuiString(syncStat_->getObjectsCurrent(ProcessCallback::PHASE_SCANNING))); //status text may be "paused"!

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
            setExternalStatus(getDialogPhaseText(syncStat_, paused_, finalResult), fractionToString(fraction)); //status text may be "paused"!

            //progress indicators
            m_gauge1->SetValue(numeric::round(fraction * GAUGE_FULL_RANGE));
            if (trayIcon.get()) trayIcon->setProgress(fraction);
            if (taskbar_.get()) taskbar_->setProgress(fraction);

            //constant line graph
            graphDataBytesTotal->setValue(to<double>(dataTotal));

            //remaining objects and data
            setText(*m_staticTextRemainingObj, toGuiString(objectsTotal - objectsCurrent), &layoutChanged);
            setText(*m_staticTextDataRemaining, L"(" + filesizeToShortString(dataTotal - dataCurrent) + L")", &layoutChanged);
            //it's possible data remaining becomes shortly negative if last file synced has ADS data and the dataTotal was not yet corrected!

            //remaining time and speed
            assert(perf);
            if (perf)
            {
                if (numeric::dist(lastStatCallSpeed, timeElapsed.Time()) >= 500)
                {
                    lastStatCallSpeed = timeElapsed.Time();

                    perf->addSample(objectsCurrent, to<double>(dataCurrent), timeElapsed.Time());

                    //current speed -> Win 7 copy uses 1 sec update interval
                    setText(*m_staticTextSpeed, perf->getBytesPerSecond(), &layoutChanged);
                }
                //remaining time: display with relative error of 10% - based on samples taken every 0.5 sec only - accuracy of prediction grows with time
                setText(*m_staticTextRemTime, perf->getRemainingTime(to<double>(dataTotal - dataCurrent)), &layoutChanged);
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
            ZEN_ON_SCOPE_EXIT(resumeTimer());
            while (paused_)
            {
                wxMilliSleep(UI_UPDATE_INTERVAL);
                updateUiNow(); //receive UI message that end pause
            }
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


std::wstring SyncProgressDialog::Pimpl::getExecWhenFinishedCommand() const
{
    return m_comboBoxExecFinished->getValue();
}


void SyncProgressDialog::Pimpl::updateDialogStatus() //depends on "syncStat_, paused_, finalResult"
{
    const wxString dlgStatusTxt = getDialogPhaseText(syncStat_, paused_, finalResult);

    m_staticTextPhase->SetLabel(dlgStatusTxt);

    //status bitmap
    if (syncStat_) //sync running
    {
        if (paused_)
            m_bitmapStatus->SetBitmap(getResourceImage(L"statusPause"));
        else
            switch (syncStat_->currentPhase())
            {
                case ProcessCallback::PHASE_NONE:
                    break;

                case ProcessCallback::PHASE_SCANNING:
                    m_bitmapStatus->SetBitmap(getResourceImage(L"statusScanning"));
                    break;

                case ProcessCallback::PHASE_COMPARING_CONTENT:
                    m_bitmapStatus->SetBitmap(getResourceImage(L"statusBinaryCompare"));
                    break;

                case ProcessCallback::PHASE_SYNCHRONIZING:
                    m_bitmapStatus->SetBitmap(getResourceImage(L"statusSyncing"));
                    break;
            }

        m_bitmapStatus->SetToolTip(dlgStatusTxt);
    }
    else //sync finished
        switch (finalResult)
        {
            case RESULT_ABORTED:
                m_bitmapStatus->SetBitmap(getResourceImage(L"statusAborted"));
                m_bitmapStatus->SetToolTip(_("Synchronization aborted!"));
                break;

            case RESULT_FINISHED_WITH_ERROR:
                m_bitmapStatus->SetBitmap(getResourceImage(L"statusFinishedErrors"));
                m_bitmapStatus->SetToolTip(_("Synchronization completed with errors!"));
                break;

            case RESULT_FINISHED_WITH_WARNINGS:
                m_bitmapStatus->SetBitmap(getResourceImage(L"statusFinishedWarnings"));
                m_bitmapStatus->SetToolTip(_("Synchronization completed with warnings."));
                break;

            case RESULT_FINISHED_WITH_SUCCESS:
                m_bitmapStatus->SetBitmap(getResourceImage(L"statusFinishedSuccess"));
                m_bitmapStatus->SetToolTip(_("Synchronization completed successfully."));
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

                case RESULT_FINISHED_WITH_WARNINGS:
                case RESULT_FINISHED_WITH_SUCCESS:
                    taskbar_->setStatus(Taskbar::STATUS_NORMAL);
                    break;
            }
    }

    //pause button
    if (syncStat_) //sync running
    {
        if (paused_)
            m_buttonPause->SetLabel(_("Continue"));
        else
            m_buttonPause->SetLabel(_("Pause"));
    }

    Layout();
}

warn_static("osx: minimize to systray?")


void SyncProgressDialog::Pimpl::closeWindowDirectly() //this should really be called: do not call back + schedule deletion
{
    paused_ = false; //you never know?

    //ATTENTION: dialog may live a little longer, so cut callbacks!
    //e.g. wxGTK calls OnIconize after wxWindow::Close() (better not ask why) and before physical destruction! => indirectly calls updateDialogStatus(), which reads syncStat_!!!

    //------- change class state -------
    abortCb_  = nullptr; //avoid callback to (maybe) deleted parent process
    syncStat_ = nullptr; //set *after* last call to "updateGui"
    //----------------------------------

    //resumeFromSystray(); -> NO, instead ~Pimpl() makes sure that main dialog is shown again!

    Close();
}


void SyncProgressDialog::Pimpl::processHasFinished(SyncResult resultId, const ErrorLog& log) //essential to call this in StatusHandler derived class destructor
{
    //at the LATEST(!) to prevent access to currentStatusHandler
    //enable okay and close events; may be set in this method ONLY

    wxWindowUpdateLocker dummy(this); //badly needed

    paused_ = false; //you never know?

    //update numbers one last time (as if sync were still running)
    notifyProgressChange(); //make one last graph entry at the *current* time
    updateGui(false);

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
            assert(dataCurrent <= dataTotal);

            //set overall speed (instead of current speed)
            auto getOverallBytesPerSecond = [&]() -> wxString
            {
                const long timeDelta = timeElapsed.Time() - phaseStartMs; //we need to consider "time within current phase" not total "timeElapsed"!
                if (timeDelta != 0)
                    return filesizeToShortString(dataCurrent * 1000 / timeDelta) + _("/sec");
                return L"-"; //fallback
            };

            m_staticTextSpeed->SetLabel(getOverallBytesPerSecond());

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
    syncStat_   = nullptr; //set *after* last call to "updateGui"
    finalResult = resultId;
    //----------------------------------

    updateDialogStatus();
    setExternalStatus(getDialogPhaseText(syncStat_, paused_, finalResult), wxString());

    resumeFromSystray(); //if in tray mode...

    EnableCloseButton(true);

    m_buttonAbort->Disable();
    m_buttonAbort->Hide();
    m_buttonPause->Disable();
    m_buttonPause->Hide();
    m_buttonClose->Show();
    m_buttonClose->Enable();

    if (IsShown()) //don't steal focus when residing in sys-tray!
        m_buttonClose->SetFocus();

    m_animationControl1->Stop();
    m_animationControl1->Hide();

    //hide current operation status
    m_staticTextStatus->Hide();

    bSizerExecFinished->Show(false);

    //show and prepare final statistics
    m_listbookResult->Show();

#ifdef FFS_WIN
    m_staticlineHeader->Hide(); //win: m_listbookResult already has a window frame
#endif

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
    bSizerRoot->Detach(m_panelProgress);
    m_panelProgress->Reparent(m_listbookResult);
#ifdef FFS_LINUX //does not seem to be required on Win or OS X
    wxTheApp->Yield(); //wxGTK 2.9.3 fails miserably at "reparent" whithout this
#endif
    m_listbookResult->AddPage(m_panelProgress, _("Statistics"), true); //AddPage() takes ownership!

    //2. log file
    const size_t posLog = 1;
    LogControl* logControl = new LogControl(m_listbookResult, log); //owned by m_listbookResult
    m_listbookResult->AddPage(logControl, _("Logging"), false);
    //bSizerHoldStretch->Insert(0, logControl, 1, wxEXPAND);

    //show log instead of graph if errors occurred! (not required for ignored warnings)
    if (log.getItemCount(TYPE_ERROR | TYPE_FATAL_ERROR) > 0)
        m_listbookResult->ChangeSelection(posLog);

    Layout();

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

    //Raise(); -> don't! user may be watching a movie in the meantime ;)
}


void SyncProgressDialog::Pimpl::OnOkay(wxCommandEvent& event)
{
    Close(); //generate close event: do NOT destroy window unconditionally!
}


void SyncProgressDialog::Pimpl::OnAbort(wxCommandEvent& event)
{
    paused_ = false;
    updateDialogStatus(); //update status + pause button

    //no Layout() or UI-update here to avoid cascaded Yield()-call
    if (abortCb_)
        abortCb_->requestAbortion();
}


void SyncProgressDialog::Pimpl::OnPause(wxCommandEvent& event)
{
    paused_ = !paused_;
    updateDialogStatus(); //update status + pause button
}


void SyncProgressDialog::Pimpl::OnClose(wxCloseEvent& event)
{
    //this event handler may be called due to a system shutdown DURING synchronization!
    //try to stop sync gracefully and cross fingers:
    if (abortCb_)
        abortCb_->requestAbortion();
    //Note: we must NOT veto dialog destruction, else we will cancel system shutdown if this dialog is application main window (as in batch mode)

    isZombie = true; //it "lives" until cleanup in next idle event
    Destroy();
}


void SyncProgressDialog::Pimpl::OnIconize(wxIconizeEvent& event)
{
    if (isZombie) return; //wxGTK sends iconize event *after* wxWindow::Destroy, sigh...

    if (event.IsIconized()) //ATTENTION: iconize event is also triggered on "Restore"! (at least under Linux)
        minimizeToTray();
    else
        resumeFromSystray(); //may be initiated by "show desktop" although all windows are hidden!
}


void SyncProgressDialog::Pimpl::OnResumeFromTray(wxCommandEvent& event)
{
    resumeFromSystray();
}


void SyncProgressDialog::Pimpl::minimizeToTray()
{
    if (!trayIcon.get())
    {
        trayIcon = make_unique<FfsTrayIcon>();
        trayIcon->Connect(FFS_REQUEST_RESUME_TRAY_EVENT, wxCommandEventHandler(SyncProgressDialog::Pimpl::OnResumeFromTray), nullptr, this);
        //tray icon has shorter lifetime than this => no need to disconnect event later
    }

    if (syncStat_)
        updateGui(false); //set tray tooltip + progress: e.g. no updates while paused

    Hide();
    if (mainDialog)
        mainDialog->Hide();
}


void SyncProgressDialog::Pimpl::resumeFromSystray()
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
        updateGui(false); //restore Windows 7 task bar progress (e.g. required in pause mode)
}


//########################################################################################

//redirect to implementation
SyncProgressDialog::SyncProgressDialog(AbortCallback& abortCb,
                                       const Statistics& syncStat,
                                       MainDialog* parentWindow,
                                       bool showProgress,
                                       const wxString& jobName,
                                       const std::wstring& execWhenFinished,
                                       std::vector<std::wstring>& execFinishedHistory) :
    pimpl(new Pimpl(abortCb, syncStat, parentWindow, jobName, execWhenFinished, execFinishedHistory))
{
    if (showProgress)
    {
        pimpl->Show();
        pimpl->updateGui(false); //clear gui flicker, remove dummy texts: window must be visible to make this work!
    }
    else
        pimpl->minimizeToTray();
}

SyncProgressDialog::~SyncProgressDialog()
{
    //DON'T delete pimpl! it will be deleted by the user clicking "OK/Cancel" -> (wxWindow::Destroy())
}

wxWindow* SyncProgressDialog::getAsWindow()
{
    return pimpl;
}

void SyncProgressDialog::initNewPhase()
{
    pimpl->initNewPhase();
}

void SyncProgressDialog::notifyProgressChange()
{
    pimpl->notifyProgressChange();
}

void SyncProgressDialog::updateGui()
{
    pimpl->updateGui();
}

std::wstring SyncProgressDialog::getExecWhenFinishedCommand() const
{
    return pimpl->getExecWhenFinishedCommand();
}

void SyncProgressDialog::stopTimer()
{
    return pimpl->stopTimer();
}

void SyncProgressDialog::resumeTimer()
{
    return pimpl->resumeTimer();
}

void SyncProgressDialog::processHasFinished(SyncResult resultId, const ErrorLog& log)
{
    pimpl->processHasFinished(resultId, log);
}

void SyncProgressDialog::closeWindowDirectly() //don't wait for user (silent mode)
{
    pimpl->closeWindowDirectly();
}
