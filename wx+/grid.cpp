// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) ZenJu (zhnmju123 AT gmx DOT de) - All Rights Reserved    *
// **************************************************************************

#include "grid.h"
#include <cassert>
#include <ctime>
#include <set>
#include <wx/dcbuffer.h> //for macro: wxALWAYS_NATIVE_DOUBLE_BUFFER
#include <wx/settings.h>
#include <wx/listbox.h>
#include <wx/tooltip.h>
#include <wx/timer.h>
#include <wx/utils.h>
#include <zen/string_tools.h>
#include <zen/scope_guard.h>
#include "format_unit.h"

#ifdef FFS_LINUX
#include <gtk/gtk.h>
#endif


using namespace zen;

wxColor zen::getColorSelectionGradientFrom() { return wxColor(137, 172, 255); } //blue: H:158 S:255 V:196
wxColor zen::getColorSelectionGradientTo  () { return wxColor(225, 234, 255); } //      H:158 S:255 V:240

void zen::clearArea(wxDC& dc, const wxRect& rect, const wxColor& col)
{
    wxDCPenChanger   dummy (dc, col);
    wxDCBrushChanger dummy2(dc, col);
    dc.DrawRectangle(rect);
}

namespace
{
//------------ Grid Constants ------------------------------------------------------------------------------------
const double MOUSE_DRAG_ACCELERATION = 1.5; //unit: [rows / (pixel * sec)] -> same value as Explorer!
const int DEFAULT_ROW_HEIGHT       = 20;
const int DEFAULT_COL_LABEL_HEIGHT = 25;
const int COLUMN_BORDER_LEFT       = 4; //for left-aligned text
const int COLUMN_LABEL_BORDER      = COLUMN_BORDER_LEFT;
const int COLUMN_MOVE_DELAY        = 5; //unit: [pixel] (from Explorer)
const int COLUMN_MIN_WIDTH         = 40; //only honored when resizing manually!
const int ROW_LABEL_BORDER         = 3;
const int COLUMN_RESIZE_TOLERANCE  = 6; //unit [pixel]

const wxColor COLOR_SELECTION_GRADIENT_NO_FOCUS_FROM = wxColour(192, 192, 192); //light grey  wxSystemSettings::GetColour(wxSYS_COLOUR_BTNSHADOW);
const wxColor COLOR_SELECTION_GRADIENT_NO_FOCUS_TO   = wxColour(228, 228, 228);

const wxColor COLOR_LABEL_GRADIENT_FROM = wxColour(200, 200, 200); //light grey
const wxColor COLOR_LABEL_GRADIENT_TO = *wxWHITE;

const wxColor COLOR_LABEL_GRADIENT_FROM_FOCUS = getColorSelectionGradientFrom();
const wxColor COLOR_LABEL_GRADIENT_TO_FOCUS   = COLOR_LABEL_GRADIENT_TO;

//wxColor getColorRowLabel         () { return wxPanel::GetClassDefaultAttributes  ().colBg; } //
wxColor getColorMainWinBackground() { return wxListBox::GetClassDefaultAttributes().colBg; } //cannot be initialized statically on wxGTK!

const wxColor colorGridLine = wxColour(192, 192, 192); //light grey
//----------------------------------------------------------------------------------------------------------------


//a fix for a poor wxWidgets implementation (wxAutoBufferedPaintDC skips one pixel on left side when RTL layout is active)
#ifndef wxALWAYS_NATIVE_DOUBLE_BUFFER
#error we need this one!
#endif

#if wxALWAYS_NATIVE_DOUBLE_BUFFER
struct BufferedPaintDC : public wxPaintDC { BufferedPaintDC(wxWindow& wnd, std::unique_ptr<wxBitmap>& buffer) : wxPaintDC(&wnd) {} };

#else
class BufferedPaintDC : public wxMemoryDC
{
public:
    BufferedPaintDC(wxWindow& wnd, std::unique_ptr<wxBitmap>& buffer) : buffer_(buffer), paintDc(&wnd)
    {
        const wxSize clientSize = wnd.GetClientSize();
        if (!buffer_ || clientSize != wxSize(buffer->GetWidth(), buffer->GetHeight()))
            buffer.reset(new wxBitmap(clientSize.GetWidth(), clientSize.GetHeight()));

        SelectObject(*buffer);

        if (paintDc.IsOk())
            SetLayoutDirection(paintDc.GetLayoutDirection());
    }

    ~BufferedPaintDC()
    {
        paintDc.SetLayoutDirection(wxLayout_LeftToRight); //workaround bug in wxDC::Blit()
        SetLayoutDirection(wxLayout_LeftToRight);         //

        const wxPoint origin = GetDeviceOrigin();
        paintDc.Blit(0, 0, buffer_->GetWidth(), buffer_->GetHeight(), this, -origin.x, -origin.y);
    }

private:
    std::unique_ptr<wxBitmap>& buffer_;
    wxPaintDC paintDc;
};
#endif


//another fix for yet another poor wxWidgets implementation (wxDCClipper does *not* stack)
hash_map<wxDC*, wxRect> clippingAreas;

class DcClipper
{
public:
    DcClipper(wxDC& dc, const wxRect& r) : dc_(dc)
    {
        auto iter = clippingAreas.find(&dc);
        if (iter != clippingAreas.end())
        {
            oldRect.reset(new wxRect(iter->second));

            wxRect tmp = r;
            tmp.Intersect(*oldRect);    //better safe than sorry
            dc_.SetClippingRegion(tmp); //
            iter->second = tmp;
        }
        else
        {
            dc_.SetClippingRegion(r);
            clippingAreas.insert(std::make_pair(&dc_, r));
        }
    }

    ~DcClipper()
    {
        dc_.DestroyClippingRegion();
        if (oldRect.get() != NULL)
        {
            dc_.SetClippingRegion(*oldRect);
            clippingAreas[&dc_] = *oldRect;
        }
        else
            clippingAreas.erase(&dc_);
    }

private:
    std::unique_ptr<wxRect> oldRect;
    wxDC& dc_;
};
}

//----------------------------------------------------------------------------------------------------------------
const wxEventType zen::EVENT_GRID_MOUSE_LEFT_DOUBLE     = wxNewEventType();
const wxEventType zen::EVENT_GRID_COL_LABEL_MOUSE_LEFT  = wxNewEventType();
const wxEventType zen::EVENT_GRID_COL_LABEL_MOUSE_RIGHT = wxNewEventType();
const wxEventType zen::EVENT_GRID_COL_RESIZE            = wxNewEventType();
const wxEventType zen::EVENT_GRID_MOUSE_LEFT_DOWN       = wxNewEventType();
const wxEventType zen::EVENT_GRID_MOUSE_LEFT_UP         = wxNewEventType();
const wxEventType zen::EVENT_GRID_MOUSE_RIGHT_DOWN      = wxNewEventType();
const wxEventType zen::EVENT_GRID_MOUSE_RIGHT_UP        = wxNewEventType();
const wxEventType zen::EVENT_GRID_SELECT_RANGE          = wxNewEventType();
//----------------------------------------------------------------------------------------------------------------

void GridData::renderRowBackgound(wxDC& dc, const wxRect& rect, int row, bool enabled, bool selected, bool hasFocus)
{
    drawCellBackground(dc, rect, enabled, selected, hasFocus, getColorMainWinBackground());
}


void GridData::renderCell(Grid& grid, wxDC& dc, const wxRect& rect, int row, ColumnType colType)
{
    wxRect rectTmp = drawCellBorder(dc, rect);

    rectTmp.x     += COLUMN_BORDER_LEFT;
    rectTmp.width -= COLUMN_BORDER_LEFT;
    drawCellText(dc, rectTmp, getValue(row, colType), grid.IsEnabled());
}


size_t GridData::getBestSize(wxDC& dc, int row, ColumnType colType)
{
    return dc.GetTextExtent(getValue(row, colType)).GetWidth() + 2 * COLUMN_BORDER_LEFT; //some border on left and right side
}


wxRect GridData::drawCellBorder(wxDC& dc, const wxRect& rect) //returns remaining rectangle
{
    wxDCPenChanger dummy2(dc, wxPen(colorGridLine, 1, wxSOLID));
    dc.DrawLine(rect.GetBottomLeft(),  rect.GetBottomRight());
    dc.DrawLine(rect.GetBottomRight(), rect.GetTopRight() + wxPoint(0, -1));

    return wxRect(rect.GetTopLeft(), wxSize(rect.width - 1, rect.height - 1));
}


void GridData::drawCellBackground(wxDC& dc, const wxRect& rect, bool enabled, bool selected, bool hasFocus, const wxColor& backgroundColor)
{
    if (enabled)
    {
        if (selected)
        {
            if (hasFocus)
                dc.GradientFillLinear(rect, getColorSelectionGradientFrom(), getColorSelectionGradientTo(), wxEAST);
            else
                dc.GradientFillLinear(rect, COLOR_SELECTION_GRADIENT_NO_FOCUS_FROM, COLOR_SELECTION_GRADIENT_NO_FOCUS_TO, wxEAST);
        }
        else
            clearArea(dc, rect, backgroundColor);
    }
    else
        clearArea(dc, rect, wxSystemSettings::GetColour(wxSYS_COLOUR_BTNFACE));
}


void GridData::drawCellText(wxDC& dc, const wxRect& rect, const wxString& text, bool enabled, int alignment)
{
    wxDCTextColourChanger dummy(dc, enabled ? dc.GetTextForeground() : wxSystemSettings::GetColour(wxSYS_COLOUR_GRAYTEXT));

    DcClipper clip(dc, rect); //wxDC::DrawLabel doesn't care about width, WTF?
    dc.DrawLabel(text, rect, alignment);
}


void GridData::renderColumnLabel(Grid& grid, wxDC& dc, const wxRect& rect, ColumnType colType, bool highlighted)
{
    wxRect rectTmp = drawColumnLabelBorder(dc, rect);
    drawColumnLabelBackground(dc, rectTmp, highlighted);

    rectTmp.x     += COLUMN_BORDER_LEFT;
    rectTmp.width -= COLUMN_BORDER_LEFT;
    drawColumnLabelText(dc, rectTmp, getColumnLabel(colType));
}


wxRect GridData::drawColumnLabelBorder(wxDC& dc, const wxRect& rect) //returns remaining rectangle
{
    //draw white line
    {
        wxDCPenChanger dummy(dc, *wxWHITE_PEN);
        dc.DrawLine(rect.GetTopLeft(), rect.GetBottomLeft());
    }

    //draw border (with gradient)
    {
        wxDCPenChanger dummy(dc, wxPen(wxSystemSettings::GetColour(wxSYS_COLOUR_3DSHADOW), 1, wxSOLID));
        dc.GradientFillLinear(wxRect(rect.GetTopRight(), rect.GetBottomRight()), dc.GetPen().GetColour(), COLOR_LABEL_GRADIENT_TO, wxNORTH);
        dc.DrawLine(rect.GetBottomLeft(), rect.GetBottomRight() + wxPoint(1, 0));
    }

    return wxRect(rect.x + 1, rect.y, rect.width - 2, rect.height - 1); //we really don't like wxRect::Deflate, do we?
}


void GridData::drawColumnLabelBackground(wxDC& dc, const wxRect& rect, bool highlighted)
{
    if (highlighted)
        dc.GradientFillLinear(rect, COLOR_LABEL_GRADIENT_FROM_FOCUS, COLOR_LABEL_GRADIENT_TO_FOCUS, wxNORTH);
    else //regular background gradient
        dc.GradientFillLinear(rect, COLOR_LABEL_GRADIENT_FROM, COLOR_LABEL_GRADIENT_TO, wxNORTH); //clear overlapping cells
}


void GridData::drawColumnLabelText(wxDC& dc, const wxRect& rect, const wxString& text)
{
    DcClipper clip(dc, rect); //wxDC::DrawLabel doesn't care about witdh, WTF?
    dc.DrawLabel(text, rect, wxALIGN_LEFT | wxALIGN_CENTER_VERTICAL);
}

//----------------------------------------------------------------------------------------------------------------

/*
                  SubWindow
                     /|\
                      |
     -----------------------------------
    |            |           |          |
CornerWin  RowLabelWin  ColLabelWin  MainWin

*/

class Grid::SubWindow : public wxWindow
{
public:
    SubWindow(Grid& parent) :
        wxWindow(&parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxWANTS_CHARS | wxBORDER_NONE, wxPanelNameStr),
        parent_(parent)
    {
        Connect(wxEVT_PAINT, wxPaintEventHandler(SubWindow::onPaintEvent), NULL, this);
        Connect(wxEVT_SIZE, wxEventHandler(SubWindow::onSizeEvent),  NULL, this);
        //http://wiki.wxwidgets.org/Flicker-Free_Drawing
        Connect(wxEVT_ERASE_BACKGROUND, wxEraseEventHandler(SubWindow::onEraseBackGround), NULL, this);

        //SetDoubleBuffered(true); slow as hell!

#if wxCHECK_VERSION(2, 9, 1)
        SetBackgroundStyle(wxBG_STYLE_PAINT);
#else
        SetBackgroundStyle(wxBG_STYLE_CUSTOM);
#endif

        Connect(wxEVT_SET_FOCUS,  wxFocusEventHandler(SubWindow::onFocus), NULL, this);
        Connect(wxEVT_KILL_FOCUS, wxFocusEventHandler(SubWindow::onFocus), NULL, this);

        Connect(wxEVT_LEFT_DOWN,    wxMouseEventHandler(SubWindow::onMouseLeftDown  ), NULL, this);
        Connect(wxEVT_LEFT_UP,      wxMouseEventHandler(SubWindow::onMouseLeftUp    ), NULL, this);
        Connect(wxEVT_LEFT_DCLICK,  wxMouseEventHandler(SubWindow::onMouseLeftDouble), NULL, this);
        Connect(wxEVT_RIGHT_DOWN,   wxMouseEventHandler(SubWindow::onMouseRightDown ), NULL, this);
        Connect(wxEVT_RIGHT_UP,     wxMouseEventHandler(SubWindow::onMouseRightUp   ), NULL, this);
        Connect(wxEVT_MOTION,       wxMouseEventHandler(SubWindow::onMouseMovement  ), NULL, this);
        Connect(wxEVT_LEAVE_WINDOW, wxMouseEventHandler(SubWindow::onLeaveWindow    ), NULL, this);
        Connect(wxEVT_MOUSE_CAPTURE_LOST, wxMouseCaptureLostEventHandler(SubWindow::onMouseCaptureLost), NULL, this);

        Connect(wxEVT_CHAR,       wxKeyEventHandler(SubWindow::onChar   ), NULL, this);
        Connect(wxEVT_KEY_UP,     wxKeyEventHandler(SubWindow::onKeyUp  ), NULL, this);
        Connect(wxEVT_KEY_DOWN,   wxKeyEventHandler(SubWindow::onKeyDown), NULL, this);
    }

    Grid& refParent() { return parent_; }
    const Grid& refParent() const { return parent_; }

    template <class T>
    bool sendEventNow(T& event) //return "true" if a suitable event handler function was found and executed, and the function did not call wxEvent::Skip.
    {
        if (wxEvtHandler* evtHandler = parent_.GetEventHandler())
            return evtHandler->ProcessEvent(event);
        return false;
    }

protected:
    void setToolTip(const wxString& text) //proper fix for wxWindow
    {
        wxToolTip* tt = GetToolTip();

        const wxString oldText = tt ? tt->GetTip() : wxString();
        if (text != oldText)
        {
            if (text.IsEmpty())
                SetToolTip(NULL); //wxGTK doesn't allow wxToolTip with empty text!
            else
            {
                //wxWidgets bug: tooltip multiline property is defined by first tooltip text containing newlines or not (same is true for maximum width)
                if (!tt)
                    SetToolTip(new wxToolTip(wxT("a                                                                b\n\
                                                           a                                                                b"))); //ugly, but is working (on Windows)
                tt = GetToolTip(); //should be bound by now
                if (tt)
                    tt->SetTip(text);
            }
        }
    }

private:
    virtual void render(wxDC& dc, const wxRect& rect) = 0;

    virtual void onFocus(wxFocusEvent& event) { event.Skip(); }

    virtual void onMouseLeftDown  (wxMouseEvent& event) { event.Skip(); }
    virtual void onMouseLeftUp    (wxMouseEvent& event) { event.Skip(); }
    virtual void onMouseLeftDouble(wxMouseEvent& event) { event.Skip(); }
    virtual void onMouseRightDown (wxMouseEvent& event) { event.Skip(); }
    virtual void onMouseRightUp   (wxMouseEvent& event) { event.Skip(); }
    virtual void onMouseMovement  (wxMouseEvent& event) { event.Skip(); }
    virtual void onLeaveWindow    (wxMouseEvent& event) { event.Skip(); }
    virtual void onMouseCaptureLost(wxMouseCaptureLostEvent& event) { event.Skip(); }

    virtual void onChar   (wxKeyEvent& event) { event.Skip(); }
    virtual void onKeyUp  (wxKeyEvent& event) { event.Skip(); }
    virtual void onKeyDown(wxKeyEvent& event) { event.Skip(); }

    void onPaintEvent(wxPaintEvent& evt)
    {
        //wxAutoBufferedPaintDC dc(this); -> this one happily fucks up for RTL layout by not drawing the first column (x = 0)!
        BufferedPaintDC dc(*this, buffer);

        assert(GetSize() == GetClientSize());

        const wxRegion& updateReg = GetUpdateRegion();
        for (wxRegionIterator iter = updateReg; iter; ++iter)
            render(dc, iter.GetRect());
    }

    void onSizeEvent(wxEvent& evt)
    {
        Refresh();
        evt.Skip();
    }

    void onEraseBackGround(wxEraseEvent& evt) {}

    Grid& parent_;
    std::unique_ptr<wxBitmap> buffer;
};

//----------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------

class Grid::CornerWin : public SubWindow
{
public:
    CornerWin(Grid& parent) : SubWindow(parent) {}

private:
    virtual bool AcceptsFocus() const { return false; }

    virtual void render(wxDC& dc, const wxRect& rect)
    {
        const wxRect& clientRect = GetClientRect();

        dc.GradientFillLinear(clientRect, COLOR_LABEL_GRADIENT_FROM, COLOR_LABEL_GRADIENT_TO, wxNORTH);

        dc.SetPen(wxPen(wxSystemSettings::GetColour(wxSYS_COLOUR_3DSHADOW), 1, wxSOLID));

        {
            wxDCPenChanger dummy(dc, COLOR_LABEL_GRADIENT_TO);
            dc.DrawLine(clientRect.GetTopLeft(), clientRect.GetTopRight());
        }

        dc.GradientFillLinear(wxRect(clientRect.GetBottomLeft (), clientRect.GetTopLeft ()), dc.GetPen().GetColour(), COLOR_LABEL_GRADIENT_TO, wxNORTH);
        dc.GradientFillLinear(wxRect(clientRect.GetBottomRight(), clientRect.GetTopRight()), dc.GetPen().GetColour(), COLOR_LABEL_GRADIENT_TO, wxNORTH);

        dc.DrawLine(clientRect.GetBottomLeft(), clientRect.GetBottomRight());

        wxRect rectShrinked = clientRect;
        rectShrinked.Deflate(1);
        dc.SetPen(*wxWHITE_PEN);

        //dc.DrawLine(clientRect.GetTopLeft(), clientRect.GetTopRight() + wxPoint(1, 0));
        dc.DrawLine(rectShrinked.GetTopLeft(), rectShrinked.GetBottomLeft() + wxPoint(0, 1));
    }
};

//----------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------

class Grid::RowLabelWin : public SubWindow
{
public:
    RowLabelWin(Grid& parent) :
        SubWindow(parent),
        rowHeight(DEFAULT_ROW_HEIGHT) {}

    int getBestWidth(int rowFrom, int rowTo)
    {
        wxClientDC dc(this);

        int bestWidth = 0;
        for (int i = rowFrom; i <= rowTo; ++i)
            bestWidth = std::max(bestWidth, dc.GetTextExtent(formatRow(i)).GetWidth() + 2 * ROW_LABEL_BORDER);
        return bestWidth;
    }

    int getLogicalHeight() const { return refParent().getRowCount() * rowHeight; }

    int getRowAtPos(int posY) const //returns < 0 if row not found
    {
        if (posY >= 0 && rowHeight > 0)
        {
            const int row = posY / rowHeight;
            if (row < static_cast<int>(refParent().getRowCount()))
                return row;
        }
        return -1;
    }

    int getRowHeight() const { return rowHeight; }
    void setRowHeight(int height) { rowHeight = height; }

    wxRect getRowLabelArea(int row) const //returns empty rect if row not found
    {
        return wxRect(wxPoint(0, rowHeight * row),
                      wxSize(GetClientSize().GetWidth(), rowHeight));
    }

    std::pair<int, int> getRowsOnClient(const wxRect& clientRect) const //returns range [begin, end)
    {
        const int yFrom = refParent().CalcUnscrolledPosition(clientRect.GetTopLeft    ()).y;
        const int yTo   = refParent().CalcUnscrolledPosition(clientRect.GetBottomRight()).y;

        const int rowBegin = std::max(yFrom / rowHeight, 0);
        const int rowEnd   = std::min((yTo  / rowHeight) + 1, static_cast<int>(refParent().getRowCount()));

        return std::make_pair(rowBegin, rowEnd);
    }

private:
    static wxString formatRow(int row) { return toStringSep(row + 1); } //convert number to std::wstring including thousands separator

    virtual bool AcceptsFocus() const { return false; }

    virtual void render(wxDC& dc, const wxRect& rect)
    {
        if (IsEnabled())
            clearArea(dc, rect, getColorMainWinBackground());
        else
            clearArea(dc, rect, wxSystemSettings::GetColour(wxSYS_COLOUR_BTNFACE));

        wxFont labelFont = GetFont();
        labelFont.SetWeight(wxFONTWEIGHT_BOLD);
        dc.SetFont(labelFont);
        dc.SetTextForeground(wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOWTEXT));

        const std::pair<int, int> rowRange = getRowsOnClient(rect); //returns range [begin, end)

        for (int row = rowRange.first; row < rowRange.second; ++row)
        {
            wxRect singleLabelArea = getRowLabelArea(row);
            if (singleLabelArea.GetHeight() > 0)
            {
                singleLabelArea.y = refParent().CalcScrolledPosition(singleLabelArea.GetTopLeft()).y;
                drawRowLabel(dc, singleLabelArea, row);
            }
        }
    }

    void drawRowLabel(wxDC& dc, const wxRect& rect, int row)
    {
        //clearArea(dc, rect, getColorRowLabel());
        dc.GradientFillLinear(rect, COLOR_LABEL_GRADIENT_FROM, COLOR_LABEL_GRADIENT_TO, wxWEST); //clear overlapping cells

        //label text
        wxRect textRect = rect;
        textRect.Deflate(1);
        {
            DcClipper clip(dc, textRect); //wxDC::DrawLabel doesn't care about with, WTF?
            dc.DrawLabel(formatRow(row), textRect, wxALIGN_CENTRE);
        }

        //border lines
        {
            wxDCPenChanger dummy(dc, *wxWHITE_PEN);
            dc.DrawLine(rect.GetTopLeft(), rect.GetTopRight());
        }
        {
            wxDCPenChanger dummy(dc, wxPen(wxSystemSettings::GetColour(wxSYS_COLOUR_3DSHADOW), 1, wxSOLID));
            dc.DrawLine(rect.GetTopLeft(),     rect.GetBottomLeft());
            dc.DrawLine(rect.GetBottomLeft(),  rect.GetBottomRight());
            dc.DrawLine(rect.GetBottomRight(), rect.GetTopRight() + wxPoint(0, -1));
        }
    }

    virtual void onMouseLeftDown(wxMouseEvent& event) { refParent().redirectRowLabelEvent(event); }
    virtual void onMouseMovement(wxMouseEvent& event) { refParent().redirectRowLabelEvent(event); }
    virtual void onMouseLeftUp  (wxMouseEvent& event) { refParent().redirectRowLabelEvent(event); }

    int rowHeight;
};


namespace
{
class ColumnResizing
{
public:
    ColumnResizing(wxWindow& wnd, int col, size_t compPos, int startWidth, int clientPosX) :
        wnd_(wnd),
        col_(col),
        compPos_(compPos),
        startWidth_(startWidth),
        clientPosX_(clientPosX) { wnd_.CaptureMouse(); }
    ~ColumnResizing() { if (wnd_.HasCapture()) wnd_.ReleaseMouse(); }

    int    getColumn      () const { return col_; }
    size_t getComponentPos() const { return compPos_; }
    int    getStartWidth  () const { return startWidth_; }
    int    getStartPosX   () const { return clientPosX_; }

private:
    wxWindow& wnd_;
    const int    col_;
    const size_t compPos_;
    const int    startWidth_;
    const int    clientPosX_;
};


class ColumnMove
{
public:
    ColumnMove(wxWindow& wnd, int colFrom, size_t compPos, int clientPosX) :
        wnd_(wnd),
        colFrom_(colFrom),
        compPos_(compPos),
        colTo_(colFrom),
        clientPosX_(clientPosX),
        singleClick_(true) { wnd_.CaptureMouse(); }
    ~ColumnMove() { if (wnd_.HasCapture()) wnd_.ReleaseMouse(); }

    int    getColumnFrom() const { return colFrom_; }
    int&   refColumnTo() { return colTo_; }
    size_t getComponentPos() const { return compPos_; }
    int    getStartPosX () const { return clientPosX_; }

    bool isRealMove() const { return !singleClick_; }
    bool setRealMove() { return singleClick_ = false; }

private:
    wxWindow& wnd_;
    const int    colFrom_;
    const size_t compPos_;
    int colTo_;
    const int clientPosX_;
    bool singleClick_;
};
}

//----------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------

class Grid::ColLabelWin : public SubWindow
{
public:
    ColLabelWin(Grid& parent) : SubWindow(parent), highlight(-1, 0) {}

private:
    virtual bool AcceptsFocus() const { return false; }

    virtual void render(wxDC& dc, const wxRect& rect)
    {
        if (IsEnabled())
            clearArea(dc, rect, getColorMainWinBackground());
        else
            clearArea(dc, rect, wxSystemSettings::GetColour(wxSYS_COLOUR_BTNFACE));

        wxFont labelFont = GetFont();
        labelFont.SetWeight(wxFONTWEIGHT_BOLD);
        dc.SetFont(labelFont);
        dc.SetTextForeground(wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOWTEXT));

        const int colLabelHeight = refParent().colLabelHeight;

        wxPoint labelAreaTL(refParent().CalcScrolledPosition(wxPoint(0, 0)).x, 0); //client coordinates

        std::vector<std::vector<VisibleColumn>> compAbsWidths = refParent().getAbsoluteWidths(); //resolve negative/stretched widths
        for (auto iterComp = compAbsWidths.begin(); iterComp != compAbsWidths.end(); ++iterComp)
            for (auto iterCol = iterComp->begin(); iterCol != iterComp->end(); ++iterCol)
            {
                const size_t col = iterCol - iterComp->begin();
                const int width  = iterCol->width_; //don't use unsigned for calculations!

                if (labelAreaTL.x > rect.GetRight())
                    return; //done
                if (labelAreaTL.x + width > rect.x)
                    drawColumnLabel(dc, wxRect(labelAreaTL, wxSize(width, colLabelHeight)), col, iterCol->type_, iterComp - compAbsWidths.begin());
                labelAreaTL.x += width;
            }
    }

    void drawColumnLabel(wxDC& dc, const wxRect& rect, int col, ColumnType colType, size_t compPos)
    {
        if (auto dataView = refParent().getDataProvider(compPos))
        {
            const bool isHighlighted = activeResizing ? col == activeResizing->getColumn    () && compPos == activeResizing->getComponentPos() : //highlight column on mouse-over
                                       activeMove     ? col == activeMove    ->getColumnFrom() && compPos == activeMove    ->getComponentPos() :
                                       /**/             col == highlight.first                 && compPos == highlight.second;

            DcClipper clip(dc, rect);
            dataView->renderColumnLabel(refParent(), dc, rect, colType, isHighlighted);

            //draw move target location
            if (refParent().columnMoveAllowed(compPos))
                if (activeMove && activeMove->isRealMove() && activeMove->getComponentPos() == compPos)
                {
                    if (col + 1 == activeMove->refColumnTo()) //handle pos 1, 2, .. up to "at end" position
                        dc.GradientFillLinear(wxRect(rect.GetTopRight(), rect.GetBottomRight() + wxPoint(-2, 0)), *wxBLUE, COLOR_LABEL_GRADIENT_TO, wxNORTH);
                    else if (col == activeMove->refColumnTo() && col == 0) //pos 0
                        dc.GradientFillLinear(wxRect(rect.GetTopLeft(), rect.GetBottomLeft() + wxPoint(2, 0)), *wxBLUE, COLOR_LABEL_GRADIENT_TO, wxNORTH);
                }
        }
    }

    virtual void onMouseLeftDown(wxMouseEvent& event)
    {
        if (FindFocus() != &refParent().getMainWin())
            refParent().getMainWin().SetFocus();

        activeResizing.reset();
        activeMove.reset();

        if (Opt<ColAction> action = refParent().clientPosToColumnAction(event.GetPosition()))
        {
            if (action->wantResize)
            {
                if (event.LeftDClick()) //auto-size visible range on double-click
                {
                    const int bestWidth = refParent().getBestColumnSize(action->col, action->compPos); //return -1 on error
                    if (bestWidth >= 0)
                        refParent().setColWidth(action->col, action->compPos, std::max(COLUMN_MIN_WIDTH, bestWidth));
                }
                else
                {
                    if (Opt<size_t> colWidth = refParent().getAbsoluteWidth(action->col, action->compPos))
                        activeResizing.reset(new ColumnResizing(*this, action->col, action->compPos, *colWidth, event.GetPosition().x));
                }
            }
            else //a move or single click
                activeMove.reset(new ColumnMove(*this, action->col, action->compPos, event.GetPosition().x));
        }
        event.Skip();
    }

    virtual void onMouseLeftUp(wxMouseEvent& event)
    {
        activeResizing.reset(); //nothing else to do, actual work done by onMouseMovement()

        if (activeMove)
        {
            const size_t compPos = activeMove->getComponentPos();
            if (activeMove->isRealMove())
            {
                if (refParent().columnMoveAllowed(compPos))
                {
                    const int colFrom = activeMove->getColumnFrom();
                    int       colTo   = activeMove->refColumnTo();

                    if (colTo > colFrom) //simulate "colFrom" deletion
                        --colTo;

                    refParent().moveColumn(colFrom, colTo, compPos);
                }
            }
            else //notify single label click
            {
                const Opt<ColumnType> colType = refParent().colToType(activeMove->getColumnFrom(), compPos);
                if (colType)
                {
                    GridClickEvent clickEvent(EVENT_GRID_COL_LABEL_MOUSE_LEFT, event, -1, *colType, compPos);
                    sendEventNow(clickEvent);
                }
            }
            activeMove.reset();
        }

        refParent().updateWindowSizes(); //looks strange if done during onMouseMovement()
        refParent().Refresh();
        event.Skip();
    }

    virtual void onMouseCaptureLost(wxMouseCaptureLostEvent& event)
    {
        activeResizing.reset();
        activeMove.reset();
        Refresh();
        //event.Skip(); -> we DID handle it!
    }

    virtual void onMouseLeftDouble(wxMouseEvent& event)
    {
        const Opt<ColAction> action = refParent().clientPosToColumnAction(event.GetPosition());
        if (action && action->wantResize)
        {
            //auto-size visible range on double-click
            const int bestWidth = refParent().getBestColumnSize(action->col, action->compPos); //return -1 on error
            if (bestWidth >= 0)
            {
                const size_t newWidth = std::max(COLUMN_MIN_WIDTH, bestWidth);
                refParent().setColWidth(action->col, action->compPos, newWidth);

                const Opt<ColumnType> colType = refParent().colToType(action->col, action->compPos);
                if (colType)
                {
                    //notify column resize
                    GridColumnResizeEvent sizeEvent(newWidth, *colType, action->compPos);
                    sendEventNow(sizeEvent);
                }
            }
        }
        event.Skip();
    }

    virtual void onMouseMovement(wxMouseEvent& event)
    {
        if (activeResizing)
        {
            const int    col     = activeResizing->getColumn();
            const size_t compPos = activeResizing->getComponentPos();

            if (Opt<size_t> colWidth = refParent().getAbsoluteWidth(col, compPos))
            {
                const int newWidth = std::max(COLUMN_MIN_WIDTH, activeResizing->getStartWidth() + event.GetPosition().x - activeResizing->getStartPosX());
                if (newWidth != static_cast<int>(*colWidth))
                {
                    refParent().setColWidth(col, compPos, newWidth);

                    const Opt<ColumnType> colType = refParent().colToType(col, compPos);
                    if (colType)
                    {
                        //notify column resize
                        GridColumnResizeEvent sizeEvent(newWidth, *colType, compPos);
                        sendEventNow(sizeEvent);
                    }

                    refParent().Refresh();
                }
            }
        }
        else if (activeMove)
        {
            const int clientPosX = event.GetPosition().x;
            if (std::abs(clientPosX - activeMove->getStartPosX()) > COLUMN_MOVE_DELAY) //real move (not a single click)
            {
                activeMove->setRealMove();

                const int col = refParent().clientPosToMoveTargetColumn(event.GetPosition(), activeMove->getComponentPos());
                if (col >= 0)
                    activeMove->refColumnTo() = col;
            }
        }
        else
        {
            const Opt<ColAction> action = refParent().clientPosToColumnAction(event.GetPosition());
            if (action)
            {
                highlight.first  = action->col;
                highlight.second = action->compPos;

                if (action->wantResize)
                    SetCursor(wxCURSOR_SIZEWE); //set window-local only! :)
                else
                    SetCursor(*wxSTANDARD_CURSOR);
            }
            else
            {
                highlight.first = -1;
                SetCursor(*wxSTANDARD_CURSOR);
            }
        }

        //change tooltip
        const wxString toolTip = [&]() -> wxString
        {
            const wxPoint absPos = refParent().CalcUnscrolledPosition(event.GetPosition());

            const auto colInfo = refParent().getColumnAtPos(absPos.x); //returns (column type, compPos), column < 0 if not found
            if (colInfo)
            {
                auto prov = refParent().getDataProvider(colInfo->second);
                if (prov)
                    return prov->getToolTip(colInfo->first);
            }
            return wxString();
        }();
        setToolTip(toolTip);

        Refresh();
        event.Skip();
    }

    virtual void onLeaveWindow(wxMouseEvent& event)
    {
        highlight.first = -1; //onLeaveWindow() does not respect mouse capture! -> however highlight is drawn unconditionally during move/resize!
        Refresh();
        event.Skip();
    }

    virtual void onMouseRightDown(wxMouseEvent& event)
    {
        const Opt<ColAction> action = refParent().clientPosToColumnAction(event.GetPosition());
        if (action)
        {
            const Opt<ColumnType> colType = refParent().colToType(action->col, action->compPos);
            if (colType)
            {
                //notify right click
                GridClickEvent clickEvent(EVENT_GRID_COL_LABEL_MOUSE_RIGHT, event, -1, *colType, action->compPos);
                sendEventNow(clickEvent);
            }
        }
        event.Skip();
    }

    std::unique_ptr<ColumnResizing> activeResizing;
    std::unique_ptr<ColumnMove>     activeMove;
    std::pair<int, size_t> highlight; //(column, component) mouse-over, row < 0 if none
};

//----------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------

class Grid::MainWin : public SubWindow
{
public:
    MainWin(Grid& parent,
            RowLabelWin& rowLabelWin,
            ColLabelWin& colLabelWin) : SubWindow(parent),
        rowLabelWin_(rowLabelWin),
        colLabelWin_(colLabelWin),
        cursor(-1, -1),
        selectionAnchor(-1) {}

    void makeRowVisible(int row)
    {
        const wxRect labelRect = rowLabelWin_.getRowLabelArea(row); //returns empty rect if column not found
        if (labelRect.height > 0)
        {
            int scrollPosX = 0;
            refParent().GetViewStart(&scrollPosX, NULL);

            int pixelsPerUnitY = 0;
            refParent().GetScrollPixelsPerUnit(NULL, &pixelsPerUnitY);
            if (pixelsPerUnitY <= 0) return;

            const int clientPosY = refParent().CalcScrolledPosition(labelRect.GetTopLeft()).y;
            if (clientPosY < 0)
            {
                const int scrollPosY = labelRect.GetTopLeft().y / pixelsPerUnitY;
                refParent().Scroll(scrollPosX, scrollPosY);
                refParent().updateWindowSizes(); //may show horizontal scroll bar
            }
            else if (clientPosY + labelRect.GetHeight() > rowLabelWin_.GetClientSize().GetHeight())
            {
                auto execScroll = [&](int clientHeight)
                {
                    const int scrollPosY = std::ceil((labelRect.GetTopLeft().y - clientHeight +
                                                      labelRect.GetHeight()) / static_cast<double>(pixelsPerUnitY));
                    refParent().Scroll(scrollPosX, scrollPosY);
                    refParent().updateWindowSizes(); //may show horizontal scroll bar
                };

                const int clientHeightBefore = rowLabelWin_.GetClientSize().GetHeight();
                execScroll(clientHeightBefore);

                //client height may decrease after scroll due to a new horizontal scrollbar, resulting in a partially visible last row
                const int clientHeightAfter = rowLabelWin_.GetClientSize().GetHeight();
                if (clientHeightAfter < clientHeightBefore)
                    execScroll(clientHeightAfter);
            }
        }
    }

    void setCursor(int row, size_t compPos)
    {
        cursor = std::make_pair(row, compPos);
        selectionAnchor = row;
    }
    std::pair<int, size_t> getCursor() const { return cursor; } // (row, component position)

private:
    virtual void render(wxDC& dc, const wxRect& rect)
    {
        if (IsEnabled())
            clearArea(dc, rect, getColorMainWinBackground());
        else
            clearArea(dc, rect, wxSystemSettings::GetColour(wxSYS_COLOUR_BTNFACE));

        dc.SetFont(GetFont());
        dc.SetTextForeground(wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOWTEXT));

        const int rowHeight = rowLabelWin_.getRowHeight();

        const wxPoint topLeft     = refParent().CalcUnscrolledPosition(rect.GetTopLeft());
        const wxPoint bottomRight = refParent().CalcUnscrolledPosition(rect.GetBottomRight());

        const int rowFirst = std::max(topLeft    .y / rowHeight, 0); // [rowFirst, rowLast)
        const int rowLast  = std::min(bottomRight.y / rowHeight + 1, static_cast<int>(refParent().getRowCount()));

        wxPoint cellAreaTL(refParent().CalcScrolledPosition(wxPoint(0, 0))); //client coordinates

        std::vector<std::vector<VisibleColumn>> compAbsWidths = refParent().getAbsoluteWidths(); //resolve negative/stretched widths
        for (auto iterComp = compAbsWidths.begin(); iterComp != compAbsWidths.end(); ++iterComp)
        {
            const int compWidth = std::accumulate(iterComp->begin(), iterComp->end(), 0,
            [](int val, const VisibleColumn& vc) { return val + vc.width_; });

            const size_t compPos = iterComp - compAbsWidths.begin();
            if (auto prov = refParent().getDataProvider(compPos))
            {
                //draw background lines
                {
                    DcClipper dummy(dc, rect); //solve issues with drawBackground() painting in area outside of rect (which is not also refreshed by renderCell()) -> keep small scope!
                    for (int row = rowFirst; row < rowLast; ++row)
                        drawBackground(*prov, dc, wxRect(cellAreaTL + wxPoint(0, row * rowHeight), wxSize(compWidth, rowHeight)), row, compPos);
                }

                //draw single cells
                for (auto iterCol = iterComp->begin(); iterCol != iterComp->end(); ++iterCol)
                {
                    const int width  = iterCol->width_; //don't use unsigned for calculations!

                    if (cellAreaTL.x > rect.GetRight())
                        return; //done

                    if (cellAreaTL.x + width > rect.x)
                        for (int row = rowFirst; row < rowLast; ++row)
                        {
                            const wxRect& cellRect = wxRect(cellAreaTL.x, cellAreaTL.y + row * rowHeight, width, rowHeight);
                            DcClipper clip(dc, cellRect);
                            prov->renderCell(refParent(), dc, cellRect, row, iterCol->type_);
                        }
                    cellAreaTL.x += width;
                }
            }
            else
                cellAreaTL.x += compWidth;
        }
    }

    void drawBackground(GridData& prov, wxDC& dc, const wxRect& rect, int row, size_t compPos)
    {
        Grid& grid = refParent();
        //check if user is currently selecting with mouse
        bool drawSelection = grid.isSelected(row, compPos);
        if (activeSelection)
        {
            const int rowFrom = std::min(activeSelection->getStartRow(), activeSelection->getCurrentRow());
            const int rowTo   = std::max(activeSelection->getStartRow(), activeSelection->getCurrentRow());

            if (compPos == activeSelection->getComponentPos() && rowFrom <= row && row <= rowTo)
                drawSelection = activeSelection->isPositiveSelect(); //overwrite default
        }

        prov.renderRowBackgound(dc, rect, row, grid.IsEnabled(), drawSelection, wxWindow::FindFocus() == &grid.getMainWin());
    }

    virtual void onMouseLeftDown (wxMouseEvent& event) { onMouseDown(event); }
    virtual void onMouseLeftUp   (wxMouseEvent& event) { onMouseUp  (event); }
    virtual void onMouseRightDown(wxMouseEvent& event) { onMouseDown(event); }
    virtual void onMouseRightUp  (wxMouseEvent& event) { onMouseUp  (event); }

    virtual void onMouseLeftDouble(wxMouseEvent& event)
    {
        const wxPoint absPos = refParent().CalcUnscrolledPosition(event.GetPosition());
        const int  row     = rowLabelWin_.getRowAtPos(absPos.y); //return -1 if no row at this position
        const auto colInfo = refParent().getColumnAtPos(absPos.x); //returns (column type, compPos), column < 0 if not found
        if (colInfo)
        {
            //notify event
            GridClickEvent mouseEvent(EVENT_GRID_MOUSE_LEFT_DOUBLE, event, row, colInfo->first, colInfo->second);
            sendEventNow(mouseEvent);
        }
        event.Skip();
    }

    void onMouseDown(wxMouseEvent& event) //handle left and right mouse button clicks (almost) the same
    {
        if (FindFocus() != this) //doesn't seem to happen automatically for right mouse button
            SetFocus();

        const wxPoint absPos = refParent().CalcUnscrolledPosition(event.GetPosition());

        const int        row     = rowLabelWin_.getRowAtPos(absPos.y); //return -1 if no row at this position
        const auto       colInfo = refParent().getColumnAtPos(absPos.x); //returns (column type, compPos), column < 0 if not found
        const ColumnType colType = colInfo ? colInfo->first  : DUMMY_COLUMN_TYPE;
        const int        compPos = colInfo ? colInfo->second : -1;

        //notify event
        GridClickEvent mouseEvent(event.RightDown() ? EVENT_GRID_MOUSE_RIGHT_DOWN : EVENT_GRID_MOUSE_LEFT_DOWN, event, row, colType, compPos);
        if (!sendEventNow(mouseEvent)) //if event was not processed externally...
        {
            if (!event.RightDown() || !refParent().isSelected(row, compPos)) //do NOT start a new selection if user right-clicks on a selected area!
            {
                if (row >= 0 && compPos >= 0)
                    cursor = std::make_pair(row, compPos);

                if (event.ControlDown())
                {
                    if (row >= 0 && compPos >= 0)
                        activeSelection.reset(new MouseSelection(*this, row, compPos, !refParent().isSelected(row, compPos)));
                    selectionAnchor = row;
                }
                else if (event.ShiftDown())
                {
                    if (row >= 0 && compPos >= 0)
                        activeSelection.reset(new MouseSelection(*this, selectionAnchor, compPos, true));
                    refParent().clearSelectionAll();
                }
                else
                {
                    if (row >= 0 && compPos >= 0)
                        activeSelection.reset(new MouseSelection(*this, row, compPos, true));
                    selectionAnchor = row;
                    refParent().clearSelectionAll();
                }
            }
            Refresh();
        }

        event.Skip(); //allow changin focus
    }

    void onMouseUp(wxMouseEvent& event)
    {
        //const int currentRow = clientPosToRow(event.GetPosition()); -> this one may point to row which is not in visible area!

        if (activeSelection)
        {
            const int    rowFrom  = activeSelection->getStartRow();
            const int    rowTo    = activeSelection->getCurrentRow();
            const size_t compPos  = activeSelection->getComponentPos();
            const bool   positive = activeSelection->isPositiveSelect();
            refParent().selectRange(rowFrom, rowTo, compPos, positive);

            cursor.first = activeSelection->getCurrentRow(); //slight deviation from Explorer: change cursor while dragging mouse! -> unify behavior with shift + direction keys

            activeSelection.reset();
        }

        //this one may point to row which is not in visible area!
        const wxPoint absPos = refParent().CalcUnscrolledPosition(event.GetPosition());

        const int        row     = rowLabelWin_.getRowAtPos(absPos.y); //return -1 if no row at this position
        const auto       colInfo = refParent().getColumnAtPos(absPos.x); //returns (column type, compPos), column < 0 if not found

        const ColumnType colType = colInfo ? colInfo->first  : DUMMY_COLUMN_TYPE; //we probably should notify even if colInfo is invalid!
        const int        compPos = colInfo ? colInfo->second : 0;

        //notify event
        GridClickEvent mouseEvent(event.RightUp() ? EVENT_GRID_MOUSE_RIGHT_UP : EVENT_GRID_MOUSE_LEFT_UP, event, row, colType, compPos);
        sendEventNow(mouseEvent);

        Refresh();
        event.Skip(); //allow changing focus
    }

    virtual void onMouseCaptureLost(wxMouseCaptureLostEvent& event)
    {
        activeSelection.reset();
        Refresh();
        //event.Skip(); -> we DID handle it!
    }

    virtual void onMouseMovement(wxMouseEvent& event)
    {
        if (activeSelection)
            activeSelection->evalMousePos(); //eval on both mouse movement + timer event!

        //change tooltip
        const wxString toolTip = [&]() -> wxString
        {
            const wxPoint absPos = refParent().CalcUnscrolledPosition(event.GetPosition());

            const int  row     = rowLabelWin_.getRowAtPos(absPos.y); //return -1 if no row at this position
            const auto colInfo = refParent().getColumnAtPos(absPos.x); //returns (column type, compPos), column < 0 if not found
            if (colInfo && row >= 0)
            {
                auto prov = refParent().getDataProvider(colInfo->second);
                if (prov)
                    return prov->getToolTip(row, colInfo->first);
            }
            return wxString();
        }();

        setToolTip(toolTip);

        event.Skip();
    }

    virtual void onKeyDown(wxKeyEvent& event)
    {
        int keyCode = event.GetKeyCode();
        if (GetLayoutDirection() == wxLayout_RightToLeft)
        {
            if (keyCode == WXK_LEFT)
                keyCode = WXK_RIGHT;
            else if (keyCode == WXK_RIGHT)
                keyCode = WXK_LEFT;
            else if (keyCode == WXK_NUMPAD_LEFT)
                keyCode = WXK_NUMPAD_RIGHT;
            else if (keyCode == WXK_NUMPAD_RIGHT)
                keyCode = WXK_NUMPAD_LEFT;
        }

        const int rowCount = refParent().getRowCount();

        auto setSingleSelection = [&](int row, int compPos)
        {
            numeric::restrict(row,     0, rowCount - 1);
            numeric::restrict(compPos, 0, static_cast<int>(refParent().comp.size()) - 1);
            refParent().setGridCursor(row, compPos);
        };

        auto setSelectionRange = [&](int row)
        {
            numeric::restrict(row, 0, rowCount - 1);

            cursor.first = row;
            if (selectionAnchor < 0)
                selectionAnchor = row;
            this->makeRowVisible(row);

            auto& comp = refParent().comp;
            std::for_each(comp.begin(), comp.end(), [](Grid::Component& c) { c.selection.clear(); }); //clear selection, do NOT fire event
            refParent().selectRange(selectionAnchor, row, cursor.second); //set new selection + fire event
        };

        switch (keyCode)
        {
            case WXK_UP:
            case WXK_NUMPAD_UP:
                if (event.ShiftDown())
                    setSelectionRange(cursor.first - 1);
                else if (event.ControlDown())
                    refParent().scrollDelta(0, -1);
                else
                    setSingleSelection(cursor.first - 1, cursor.second);
                break;

            case WXK_DOWN:
            case WXK_NUMPAD_DOWN:
                if (event.ShiftDown())
                    setSelectionRange(cursor.first + 1);
                else if (event.ControlDown())
                    refParent().scrollDelta(0, 1);
                else
                    setSingleSelection(cursor.first + 1, cursor.second);
                break;

            case WXK_LEFT:
            case WXK_NUMPAD_LEFT:
                if (event.ControlDown())
                    refParent().scrollDelta(-1, 0);
                else if (event.ShiftDown())
                    ;
                else
                    setSingleSelection(cursor.first, cursor.second - 1);
                break;

            case WXK_RIGHT:
            case WXK_NUMPAD_RIGHT:
                if (event.ControlDown())
                    refParent().scrollDelta(1, 0);
                else if (event.ShiftDown())
                    ;
                else
                    setSingleSelection(cursor.first, cursor.second + 1);
                break;

            case WXK_HOME:
            case WXK_NUMPAD_HOME:
                if (event.ShiftDown())
                    setSelectionRange(0);
                else if (event.ControlDown())
                    setSingleSelection(0, 0);
                else
                    setSingleSelection(0, cursor.second);
                break;

            case WXK_END:
            case WXK_NUMPAD_END:
                if (event.ShiftDown())
                    setSelectionRange(rowCount - 1);
                else if (event.ControlDown())
                    setSingleSelection(rowCount - 1, refParent().comp.size() - 1);
                else
                    setSingleSelection(rowCount - 1, cursor.second);
                break;

            case WXK_PAGEUP:
            case WXK_NUMPAD_PAGEUP:
                if (event.ShiftDown())
                    setSelectionRange(cursor.first - GetClientSize().GetHeight() / rowLabelWin_.getRowHeight());
                else if (event.ControlDown())
                    ;
                else
                    setSingleSelection(cursor.first - GetClientSize().GetHeight() / rowLabelWin_.getRowHeight(), cursor.second);
                break;

            case WXK_PAGEDOWN:
            case WXK_NUMPAD_PAGEDOWN:
                if (event.ShiftDown())
                    setSelectionRange(cursor.first + GetClientSize().GetHeight() / rowLabelWin_.getRowHeight());
                else if (event.ControlDown())
                    ;
                else
                    setSingleSelection(cursor.first + GetClientSize().GetHeight() / rowLabelWin_.getRowHeight(), cursor.second);
                break;

            case 'A':  //Ctrl + A - select all
                if (event.ControlDown())
                    refParent().selectRange(0, rowCount, cursor.second);
                break;

            case WXK_NUMPAD_ADD: //CTRL + '+' - auto-size all
                if (event.ControlDown())
                    refParent().autoSizeColumns(cursor.second);
                break;
        }

        Refresh();
        event.Skip();
    }

    virtual void onFocus(wxFocusEvent& event) { Refresh(); event.Skip(); }

    class MouseSelection : private wxEvtHandler
    {
    public:
        MouseSelection(MainWin& wnd,
                       int rowStart,
                       size_t compPos,
                       bool positiveSelect) : wnd_(wnd), rowStart_(rowStart), compPos_(compPos), rowCurrent_(rowStart), positiveSelect_(positiveSelect), toScrollX(0), toScrollY(0), tickCountLast(clock())
        {
            wnd_.CaptureMouse();
            timer.Connect(wxEVT_TIMER, wxEventHandler(MouseSelection::onTimer), NULL, this);
            timer.Start(100); //timer interval in ms
            evalMousePos();
        }
        ~MouseSelection() { if (wnd_.HasCapture()) wnd_.ReleaseMouse(); }

        int    getStartRow     () const { return rowStart_; }
        size_t getComponentPos () const { return compPos_; }
        int    getCurrentRow   () const { return rowCurrent_; }
        bool   isPositiveSelect() const { return positiveSelect_; } //are we selecting or unselecting?

        void evalMousePos()
        {
            const clock_t now = clock();
            const double deltaTime = static_cast<double>(now - tickCountLast) / CLOCKS_PER_SEC; //unit: [sec]
            tickCountLast = now;

            wxMouseState mouseState = wxGetMouseState();
            const wxPoint clientPos = wnd_.ScreenToClient(wxPoint(mouseState.GetX(), mouseState.GetY()));
            const wxSize clientSize = wnd_.GetClientSize();

            //scroll while dragging mouse
            const int overlapPixY = clientPos.y < 0 ? clientPos.y :
                                    clientPos.y >= clientSize.GetHeight() ? clientPos.y - (clientSize.GetHeight() - 1) : 0;
            const int overlapPixX = clientPos.x < 0 ? clientPos.x :
                                    clientPos.x >= clientSize.GetWidth() ? clientPos.x - (clientSize.GetWidth() - 1) : 0;

            int pixelsPerUnitY = 0;
            wnd_.refParent().GetScrollPixelsPerUnit(NULL, &pixelsPerUnitY);
            if (pixelsPerUnitY <= 0) return;

            const double mouseDragSpeedIncScrollU = pixelsPerUnitY > 0 ? MOUSE_DRAG_ACCELERATION * wnd_.rowLabelWin_.getRowHeight() / pixelsPerUnitY : 0; //unit: [scroll units / (pixel * sec)]

            auto autoScroll = [&](int overlapPix, double& toScroll)
            {
                if (overlapPix != 0)
                {
                    const double scrollSpeed = overlapPix * mouseDragSpeedIncScrollU; //unit: [scroll units / sec]
                    toScroll += scrollSpeed * deltaTime;
                }
                else
                    toScroll = 0;
            };

            autoScroll(overlapPixX, toScrollX);
            autoScroll(overlapPixY, toScrollY);

            if (toScrollX != 0 || toScrollY != 0)
            {
                wnd_.refParent().scrollDelta(static_cast<int>(toScrollX), static_cast<int>(toScrollY)); //
                toScrollX -= static_cast<int>(toScrollX); //rounds down for positive numbers, up for negative,
                toScrollY -= static_cast<int>(toScrollY); //exactly what we want
            }

            {
                //select current row *after* scrolling
                wxPoint clientPosTrimmed = clientPos;
                numeric::restrict(clientPosTrimmed.y, 0, clientSize.GetHeight() - 1); //do not select row outside client window!

                wxPoint absPos = wnd_.refParent().CalcUnscrolledPosition(clientPosTrimmed);
                int currentRow = wnd_.rowLabelWin_.getRowAtPos(absPos.y); //return -1 if no row at this position

                //make sure "current row" is always at a valid position while moving!
                if (currentRow < 0)
                    currentRow = static_cast<int>(wnd_.refParent().getRowCount()) - 1; //seems, we hit the empty space at the end

                if (currentRow >= 0 && rowCurrent_ != currentRow)
                {
                    rowCurrent_ = currentRow;
                    wnd_.Refresh();
                }
            }
        }

    private:
        void onTimer(wxEvent& event) { evalMousePos(); }

        MainWin& wnd_;
        const int rowStart_;
        const size_t compPos_;
        int rowCurrent_;
        const bool positiveSelect_;
        wxTimer timer;
        double toScrollX; //count outstanding scroll units to scroll while dragging mouse
        double toScrollY; //
        clock_t tickCountLast;
    };

    virtual void ScrollWindow(int dx, int dy, const wxRect* rect)
    {
        wxWindow::ScrollWindow(dx, dy, rect);
        rowLabelWin_.ScrollWindow(0, dy, rect);
        colLabelWin_.ScrollWindow(dx, 0, rect);

        refParent().updateWindowSizes(false); //row label width has changed -> do *not* update scrollbars: recursion on wxGTK!
        rowLabelWin_.Update(); //update while dragging scroll thumb
    }

    RowLabelWin& rowLabelWin_;
    ColLabelWin& colLabelWin_;

    std::unique_ptr<MouseSelection> activeSelection; //bound while user is selecting with mouse

    std::pair<int, int> cursor; //(row, component position) -1 if none
    int selectionAnchor;        //-1 if none
};

//----------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------

Grid::Grid(wxWindow* parent,
           wxWindowID id,
           const wxPoint& pos,
           const wxSize& size,
           long style,
           const wxString& name) : wxScrolledWindow(parent, id, pos, size, style | wxWANTS_CHARS, name),
    showScrollbarX(true),
    showScrollbarY(true),
    colLabelHeight(DEFAULT_COL_LABEL_HEIGHT),
    drawRowLabel(true),
    comp(1),
    colSizeOld(0)
{
    Connect(wxEVT_PAINT,            wxPaintEventHandler(Grid::onPaintEvent     ), NULL, this);
    Connect(wxEVT_ERASE_BACKGROUND, wxEraseEventHandler(Grid::onEraseBackGround), NULL, this);
    Connect(wxEVT_SIZE,             wxEventHandler     (Grid::onSizeEvent      ), NULL, this);

    cornerWin_   = new CornerWin  (*this); //
    rowLabelWin_ = new RowLabelWin(*this); //owership handled by "this"
    colLabelWin_ = new ColLabelWin(*this); //
    mainWin_     = new MainWin    (*this, *rowLabelWin_, *colLabelWin_); //

    SetTargetWindow(mainWin_);

    SetInitialSize(size); //"Most controls will use this to set their initial size" -> why not

    assert(GetClientSize() == GetSize()); //borders are NOT allowed for Grid
    //reason: updateWindowSizes() wants to use "GetSize()" as a "GetClientSize()" including scrollbars
}


void Grid::updateWindowSizes(bool updateScrollbar)
{
    /* We have to deal with a nasty circular dependency:

    	mainWin_->GetClientSize()
    	  /|\
    	SetScrollbars -> show/hide scrollbars depending on whether client size is big enough
    	  /|\
    	GetClientRect(); -> possibly trimmed by scrollbars
    	  /|\
    	mainWin_->GetClientSize()  -> also trimmed, since it's a sub-window !

    */

    //update scrollbars: showing/hiding scrollbars changes client size!
    if (updateScrollbar)
    {
        //help SetScrollbars() do a better job
        //mainWin_->SetSize(std::max(0, GetSize().GetWidth () - rowLabelWidth), -> not working!
        //                  std::max(0, GetSize().GetHeight() - colLabelHeight));

        int scrollPosX = 0;
        int scrollPosY = 0;
        GetViewStart(&scrollPosX, &scrollPosY); //preserve current scroll position

        const int pixPerScrollUnitY = std::max(1, rowLabelWin_->getRowHeight());
        const int pixPerScrollUnitX = pixPerScrollUnitY;

        const int scrollUnitsX = std::ceil(static_cast<double>(      getMinAbsoluteWidthTotal()) / pixPerScrollUnitX);
        const int scrollUnitsY = std::ceil(static_cast<double>(rowLabelWin_->getLogicalHeight()) / pixPerScrollUnitY);

        SetScrollbars(pixPerScrollUnitX, pixPerScrollUnitY, //another abysmal wxWidgets design decision: why is precision needlessly reduced to "pixelsPerUnit"????
                      scrollUnitsX, scrollUnitsY,
                      scrollPosX, scrollPosY);
    }

    const wxRect clientRect = GetClientRect();

    const int mainWinHeight = std::max(0, clientRect.height - colLabelHeight);

    int rowLabelWidth = 0; //calculate optimal row label width
    if (drawRowLabel)
    {
        const int heightTotal = rowLabelWin_->getLogicalHeight();
        if (heightTotal > 0)
        {
            int yFrom = CalcUnscrolledPosition(wxPoint(0, 0)).y;
            int yTo   = CalcUnscrolledPosition(wxPoint(0, mainWinHeight - 1)).y ;
            numeric::restrict(yFrom, 0, heightTotal - 1);
            numeric::restrict(yTo,   0, heightTotal - 1);

            const int rowFrom = rowLabelWin_->getRowAtPos(yFrom);
            const int rowTo   = rowLabelWin_->getRowAtPos(yTo);
            if (rowFrom >= 0 && rowTo >= 0)
                rowLabelWidth = rowLabelWin_->getBestWidth(rowFrom, rowTo);
        }
    }

    const int mainWinWidth = std::max(0, clientRect.width - rowLabelWidth);

    cornerWin_  ->SetSize(0, 0, rowLabelWidth, colLabelHeight);
    rowLabelWin_->SetSize(0, colLabelHeight, rowLabelWidth, mainWinHeight);
    colLabelWin_->SetSize(rowLabelWidth, 0, mainWinWidth, colLabelHeight);
    mainWin_    ->SetSize(rowLabelWidth, colLabelHeight, mainWinWidth, mainWinHeight);
}


void Grid::onPaintEvent(wxPaintEvent& event) { wxPaintDC dc(this); }


void Grid::setColumnLabelHeight(int height)
{
    colLabelHeight = std::max(0, height);
    updateWindowSizes();
}


void Grid::showRowLabel(bool show)
{
    drawRowLabel = show;
    updateWindowSizes();
}


std::vector<int> Grid::getSelectedRows(size_t compPos) const
{
    return compPos < comp.size() ? comp[compPos].selection.get() : std::vector<int>();
}


void Grid::scrollDelta(int deltaX, int deltaY)
{
    int scrollPosX = 0;
    int scrollPosY = 0;
    GetViewStart(&scrollPosX, &scrollPosY);

    scrollPosX += deltaX;
    scrollPosY += deltaY;

    //const int unitsTotalX = GetScrollLines(wxHORIZONTAL);
    //const int unitsTotalY = GetScrollLines(wxVERTICAL);

    //if (unitsTotalX <= 0 || unitsTotalY <= 0) return; -> premature
    //numeric::restrict(scrollPosX, 0, unitsTotalX - 1); //make sure scroll target is in valid range
    //numeric::restrict(scrollPosY, 0, unitsTotalY - 1); //

    Scroll(scrollPosX, scrollPosY);
    updateWindowSizes(); //may show horizontal scroll bar
}


void Grid::redirectRowLabelEvent(wxMouseEvent& event)
{
    event.m_x = 0;
    mainWin_->ProcessEvent(event);

    if (event.ButtonDown() && wxWindow::FindFocus() != mainWin_)
        mainWin_->SetFocus();
}


size_t Grid::getRowCount() const
{
    return comp.empty() ? 0 : comp.front().dataView_ ? comp.front().dataView_->getRowCount() : 0;
}


void Grid::Refresh(bool eraseBackground, const wxRect* rect)
{
    const size_t rowCountNew = getRowCount();
    if (colSizeOld != rowCountNew)
    {
        colSizeOld = rowCountNew;
        std::for_each(comp.begin(), comp.end(), [&](Component& c) { c.selection.init(rowCountNew); });
        updateWindowSizes();
    }
    wxScrolledWindow::Refresh(eraseBackground, rect);
}


void Grid::setRowHeight(int height)
{
    rowLabelWin_->setRowHeight(height);
    updateWindowSizes();
}


void Grid::setColumnConfig(const std::vector<Grid::ColumnAttribute>& attr, size_t compPos)
{
    if (compPos < comp.size())
    {
        //hold ownership of non-visible columns
        comp[compPos].oldColAttributes = attr;

        std::vector<VisibleColumn> visibleCols;
        std::for_each(attr.begin(), attr.end(),
                      [&](const ColumnAttribute& ca)
        {
            if (ca.visible_)
                visibleCols.push_back(Grid::VisibleColumn(ca.type_, ca.width_));
        });

        //set ownership of visible columns
        comp[compPos].visibleCols = visibleCols;

        updateWindowSizes();
        Refresh();
    }
}


std::vector<Grid::ColumnAttribute> Grid::getColumnConfig(size_t compPos) const
{
    if (compPos < comp.size())
    {
        auto iterVcols    = comp[compPos].visibleCols.begin();
        auto iterVcolsend = comp[compPos].visibleCols.end();

        std::set<ColumnType> visibleTypes;
        std::transform(iterVcols, iterVcolsend, std::inserter(visibleTypes, visibleTypes.begin()),
        [](const VisibleColumn& vc) { return vc.type_; });

        //get non-visible columns (+ outdated visible ones)
        std::vector<ColumnAttribute> output = comp[compPos].oldColAttributes;

        //update visible columns but keep order of non-visible ones!
        std::for_each(output.begin(), output.end(),
                      [&](ColumnAttribute& ca)
        {
            if (visibleTypes.find(ca.type_) != visibleTypes.end())
            {
                if (iterVcols != iterVcolsend)
                {
                    ca.visible_ = true; //paranoia
                    ca.type_    = iterVcols->type_;
                    ca.width_   = iterVcols->width_;
                    ++iterVcols;
                }
            }
            else
                ca.visible_ = false; //paranoia
        });

        return output;
    }
    return std::vector<ColumnAttribute>();
}


void Grid::showScrollBars(bool horizontal, bool vertical)
{
#ifdef FFS_WIN
    showScrollbarX = horizontal;
    showScrollbarY = vertical;
    updateWindowSizes();

#elif defined FFS_LINUX //get rid of scrollbars, but preserve scrolling behavior!
    GtkWidget* gridWidget = wxWindow::m_widget;
    GtkScrolledWindow* scrolledWindow = GTK_SCROLLED_WINDOW(gridWidget);
    gtk_scrolled_window_set_policy(scrolledWindow,
                                   horizontal ? GTK_POLICY_AUTOMATIC : GTK_POLICY_NEVER,
                                   vertical   ? GTK_POLICY_AUTOMATIC : GTK_POLICY_NEVER);
#endif
}


#ifdef FFS_WIN //get rid of scrollbars, but preserve scrolling behavior!
void Grid::SetScrollbar(int orientation, int position, int thumbSize, int range, bool refresh)
{
    if ((orientation == wxHORIZONTAL && !showScrollbarX) || (orientation == wxVERTICAL && !showScrollbarY))
        wxWindow::SetScrollbar(orientation, 0, 0, 0, refresh);
    else
        wxWindow::SetScrollbar(orientation, position, thumbSize, range, refresh);
}


#ifndef WM_MOUSEHWHEEL //MinGW is clueless...
#define WM_MOUSEHWHEEL                  0x020E
#endif

WXLRESULT Grid::MSWDefWindowProc(WXUINT nMsg, WXWPARAM wParam, WXLPARAM lParam)
{
    if (nMsg == WM_MOUSEHWHEEL)
    {
        const int distance  = GET_WHEEL_DELTA_WPARAM(wParam);
        const int delta     = WHEEL_DELTA;
        const int rotations = distance / delta;

        static int linesPerRotation = -1;
        if (linesPerRotation < 0)
            if (!::SystemParametersInfo(SPI_GETWHEELSCROLLLINES, 0, &linesPerRotation, 0))
                linesPerRotation = 3;

        SetFocus();
        scrollDelta(rotations * linesPerRotation, 0); //in scroll units
    }

    return wxScrolledWindow::MSWDefWindowProc(nMsg, wParam, lParam);;
}
#endif


wxWindow& Grid::getCornerWin  () { return *cornerWin_;   }
wxWindow& Grid::getRowLabelWin() { return *rowLabelWin_; }
wxWindow& Grid::getColLabelWin() { return *colLabelWin_; }
wxWindow& Grid::getMainWin    () { return *mainWin_;     }


wxRect Grid::getColumnLabelArea(ColumnType colType, size_t compPos) const
{
    std::vector<std::vector<VisibleColumn>> compAbsWidths = getAbsoluteWidths(); //resolve negative/stretched widths
    if (compPos < compAbsWidths.size())
    {
        auto iterComp = compAbsWidths.begin() + compPos;

        auto iterCol = std::find_if(iterComp->begin(), iterComp->end(), [&](const VisibleColumn& vc) { return vc.type_ == colType; });
        if (iterCol != iterComp->end())
        {
            int posX = std::accumulate(compAbsWidths.begin(), iterComp, 0,
                                       [](int val, const std::vector<VisibleColumn>& cols)
            {
                return val + std::accumulate(cols.begin(), cols.end(), 0, [](int val2, const VisibleColumn& vc) { return val2 + vc.width_; });
            });

            posX += std::accumulate(iterComp->begin(), iterCol, 0, [](int val, const VisibleColumn& vc) { return val + vc.width_; });

            return wxRect(wxPoint(posX, 0), wxSize(iterCol->width_, colLabelHeight));
        }
    }
    return wxRect();
}


Opt<Grid::ColAction> Grid::clientPosToColumnAction(const wxPoint& pos) const
{
    const int absPosX = CalcUnscrolledPosition(pos).x;
    if (absPosX >= 0)
    {
        int accuWidth = 0;

        std::vector<std::vector<VisibleColumn>> compAbsWidths = getAbsoluteWidths(); //resolve negative/stretched widths
        for (auto iterComp = compAbsWidths.begin(); iterComp != compAbsWidths.end(); ++iterComp)
        {
            const size_t compPos =  iterComp - compAbsWidths.begin();
            const int resizeTolerance = columnResizeAllowed(compPos) ? COLUMN_RESIZE_TOLERANCE : 0;

            for (auto iterCol = iterComp->begin(); iterCol != iterComp->end(); ++iterCol)
            {
                const size_t col = iterCol - iterComp->begin();

                accuWidth += iterCol->width_;
                if (std::abs(absPosX - accuWidth) < resizeTolerance)
                {
                    ColAction out = {};
                    out.wantResize = true;
                    out.col        = col;
                    out.compPos    = compPos;
                    return out;
                }
                else if (absPosX < accuWidth)
                {
                    ColAction out = {};
                    out.wantResize = false;
                    out.col        = col;
                    out.compPos    = compPos;
                    return out;
                }
            }
        }
    }
    return NoValue();
}


void Grid::moveColumn(size_t colFrom, size_t colTo, size_t compPos)
{
    if (compPos < comp.size())
    {
        auto& visibleCols = comp[compPos].visibleCols;
        if (colFrom < visibleCols.size() &&
            colTo   < visibleCols.size() &&
            colTo != colFrom)
        {
            const auto colAtt = visibleCols[colFrom];
            visibleCols.erase (visibleCols.begin() + colFrom);
            visibleCols.insert(visibleCols.begin() + colTo, colAtt);
        }
    }
}


int Grid::clientPosToMoveTargetColumn(const wxPoint& pos, size_t compPos) const
{
    std::vector<std::vector<VisibleColumn>> compAbsWidths = getAbsoluteWidths(); //resolve negative/stretched widths
    if (compPos < compAbsWidths.size())
    {
        auto iterComp = compAbsWidths.begin() + compPos;
        const int absPosX = CalcUnscrolledPosition(pos).x;

        int accuWidth = std::accumulate(compAbsWidths.begin(), iterComp, 0,
                                        [](int val, const std::vector<VisibleColumn>& cols)
        {
            return val + std::accumulate(cols.begin(), cols.end(), 0,
            [](int val2, const VisibleColumn& vc) { return val2 + vc.width_; });
        });

        for (auto iterCol = iterComp->begin(); iterCol != iterComp->end(); ++iterCol)
        {
            const int width  = iterCol->width_; //beware dreaded unsigned conversions!
            accuWidth += width;

            if (absPosX < accuWidth - width / 2)
                return iterCol - iterComp->begin();
        }
        return iterComp->size();
    }
    return -1;
}


Opt<ColumnType> Grid::colToType(size_t col, size_t compPos) const
{
    if (compPos < comp.size())
    {
        auto& visibleCols = comp[compPos].visibleCols;
        if (col < visibleCols.size())
            return visibleCols[col].type_;
    }
    return NoValue();
}


int Grid::getRowAtPos(int posY) const { return rowLabelWin_->getRowAtPos(posY); }


Opt<std::pair<ColumnType, size_t>> Grid::getColumnAtPos(int posX) const
{
    if (posX >= 0)
    {
        std::vector<std::vector<VisibleColumn>> compAbsWidths = getAbsoluteWidths(); //resolve negative/stretched widths

        int accWidth = 0;
        for (auto iterComp = compAbsWidths.begin(); iterComp != compAbsWidths.end(); ++iterComp)
            for (auto iterCol = iterComp->begin(); iterCol != iterComp->end(); ++iterCol)
            {
                accWidth += iterCol->width_;
                if (posX < accWidth)
                {
                    const size_t compPos = iterComp - compAbsWidths.begin();
                    return std::make_pair(iterCol->type_, compPos);
                }
            }
    }
    return NoValue();
}


wxRect Grid::getCellArea(int row, ColumnType colType, size_t compPos) const
{
    const wxRect& colArea = getColumnLabelArea(colType, compPos);
    const wxRect& rowArea = rowLabelWin_->getRowLabelArea(row);
    return wxRect(wxPoint(colArea.x, rowArea.y), wxSize(colArea.width, rowArea.height));
}


void Grid::setGridCursor(int row, size_t compPos)
{
    if (compPos < comp.size())
    {
        std::for_each(comp.begin(), comp.end(), [](Grid::Component& c) { c.selection.clear(); }); //clear selection, do NOT fire event
        selectRange(row, row, compPos); //set new selection + fire event

        mainWin_->setCursor(row, compPos);
        mainWin_->makeRowVisible(row);
        mainWin_->Refresh();
    }
}


void Grid::selectRange(int rowFrom, int rowTo, size_t compPos, bool positive)
{
    if (compPos < comp.size())
    {
        comp[compPos].selection.selectRange(rowFrom, rowTo, positive);

        //notify event
        GridRangeSelectEvent selectionEvent(rowFrom, rowTo, compPos, positive);
        if (wxEvtHandler* evtHandler = GetEventHandler())
            evtHandler->ProcessEvent(selectionEvent);
    }
}


void Grid::clearSelectionAll()
{
    std::for_each(comp.begin(), comp.end(), [](Grid::Component& c) { c.selection.clear(); });

    //notify event
    GridRangeSelectEvent unselectionEvent(-1, -1, static_cast<size_t>(-1), false);
    if (wxEvtHandler* evtHandler = GetEventHandler())
        evtHandler->ProcessEvent(unselectionEvent);
}


void Grid::scrollTo(int row)
{
    const wxRect labelRect = rowLabelWin_->getRowLabelArea(row); //returns empty rect if column not found
    if (labelRect.height > 0)
    {
        int pixelsPerUnitY = 0;
        GetScrollPixelsPerUnit(NULL, &pixelsPerUnitY);
        if (pixelsPerUnitY >= 0)
        {
            int scrollPosX = 0;
            GetViewStart(&scrollPosX, NULL);
            int scrollPosY = labelRect.GetTopLeft().y / pixelsPerUnitY;

            Scroll(scrollPosX, scrollPosY);
            updateWindowSizes(); //may show horizontal scroll bar
            Refresh();
        }
    }
}


std::pair<int, size_t> Grid::getGridCursor() const
{
    return mainWin_->getCursor();
}


int Grid::getBestColumnSize(size_t col, size_t compPos) const
{
    if (compPos < comp.size())
    {
        auto& visibleCols = comp[compPos].visibleCols;
        auto dataView = comp[compPos].dataView_;
        if (dataView && col < visibleCols.size())
        {
            const ColumnType type = visibleCols[col].type_;

            wxClientDC dc(mainWin_);
            dc.SetFont(mainWin_->GetFont());

            size_t maxSize = 0;
            const std::pair<int, int> rowRange = rowLabelWin_->getRowsOnClient(mainWin_->GetClientRect()); //returns range [begin, end)

            for (int row = rowRange.first; row < rowRange.second; ++row)
                maxSize = std::max(maxSize, dataView->getBestSize(dc, row, type));

            return maxSize;
        }
    }
    return -1;
}


void Grid::autoSizeColumns(size_t compPos)
{
    if (compPos < comp.size() && comp[compPos].allowColumnResize)
    {
        auto& visibleCols = comp[compPos].visibleCols;
        for (auto iter = visibleCols.begin(); iter != visibleCols.end(); ++iter)
        {
            const int col = iter - visibleCols.begin();
            const int bestWidth = getBestColumnSize(col, compPos); //return -1 on error
            if (bestWidth >= 0)
            {
                const size_t newWidth = std::max(COLUMN_MIN_WIDTH, bestWidth);
                iter->width_ = newWidth;

                //notify column resize (asynchronously!)
                GridColumnResizeEvent sizeEvent(newWidth, iter->type_, compPos);
                wxEvtHandler* evtHandler = GetEventHandler();
                if (evtHandler)
                    evtHandler->AddPendingEvent(sizeEvent);
            }
        }
        updateWindowSizes();
        Refresh();
    }
}


int Grid::getMinAbsoluteWidthTotal() const
{
    int minWidthTotal = 0;
    //bool haveStretchedCols = false;
    std::for_each(comp.begin(), comp.end(),
                  [&](const Component& c)
    {
        std::for_each(c.visibleCols.begin(), c.visibleCols.end(),
                      [&](const VisibleColumn& vc)
        {
            if (vc.width_ >= 0)
                minWidthTotal += vc.width_;
            else
            {
                //haveStretchedCols = true;
                minWidthTotal += COLUMN_MIN_WIDTH; //use "min width" if column is stretched
            }
        });
    });
    return minWidthTotal;
}


std::vector<std::vector<Grid::VisibleColumn>> Grid::getAbsoluteWidths() const //evaluate negative widths as stretched absolute values! structure matches "comp"
{
    std::vector<std::vector<VisibleColumn>> output;

    std::vector<std::pair<int, VisibleColumn*>> stretchedCols; //(factor, column to stretch)
    int factorTotal   = 0;
    int minWidthTotal = 0;

    output.reserve(comp.size());
    std::for_each(comp.begin(), comp.end(),
                  [&](const Component& c)
    {
        output.push_back(std::vector<VisibleColumn>());
        auto& compWidths = output.back();

        compWidths.reserve(c.visibleCols.size());
        std::for_each(c.visibleCols.begin(), c.visibleCols.end(),
                      [&](const VisibleColumn& vc)
        {
            if (vc.width_ >= 0)
            {
                compWidths.push_back(Grid::VisibleColumn(vc.type_, vc.width_));
                minWidthTotal += vc.width_;
            }
            else //stretched column
            {
                compWidths.push_back(Grid::VisibleColumn(vc.type_, COLUMN_MIN_WIDTH)); //use "min width" if column is stretched
                minWidthTotal += COLUMN_MIN_WIDTH;

                stretchedCols.push_back(std::make_pair(vc.width_, &compWidths.back()));
                factorTotal += vc.width_;
            }
        });
    });

    if (!stretchedCols.empty())
    {
        const int widthToFill = mainWin_->GetClientSize().GetWidth() - minWidthTotal;
        if (widthToFill > 0)
        {
            int widthRemaining = widthToFill;
            for (auto iter = stretchedCols.begin(); iter != stretchedCols.end(); ++iter)
            {
                const int addWidth = (widthToFill * iter->first) / factorTotal; //round down
                iter->second->width_ += addWidth;
                widthRemaining -= addWidth;
            }

            if (widthRemaining > 0) //should be empty, except for rounding errors
                stretchedCols.back().second->width_ += widthRemaining;
        }
    }
    return output;
}
