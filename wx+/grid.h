// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) ZenJu (zhnmju123 AT gmx DOT de) - All Rights Reserved    *
// **************************************************************************

#ifndef GENERIC_GRID_HEADER_83470213483173
#define GENERIC_GRID_HEADER_83470213483173

#include <memory>
#include <numeric>
#include <vector>
#include <wx/scrolwin.h>
#include <zen/basic_math.h>
#include <zen/optional.h>

//a user-friendly, extensible and high-performance grid control

namespace zen
{
typedef enum { DUMMY_COLUMN_TYPE = static_cast<size_t>(-1) } ColumnType;

//----- Events -----------------------------------------------------------------------------------------------
extern const wxEventType EVENT_GRID_COL_LABEL_MOUSE_LEFT;  //generates: GridClickEvent
extern const wxEventType EVENT_GRID_COL_LABEL_MOUSE_RIGHT; //
extern const wxEventType EVENT_GRID_COL_RESIZE;   //generates: GridColumnResizeEvent

extern const wxEventType EVENT_GRID_MOUSE_LEFT_DOUBLE; //
extern const wxEventType EVENT_GRID_MOUSE_LEFT_DOWN;   //
extern const wxEventType EVENT_GRID_MOUSE_LEFT_UP;     //generates: GridClickEvent
extern const wxEventType EVENT_GRID_MOUSE_RIGHT_DOWN;  //
extern const wxEventType EVENT_GRID_MOUSE_RIGHT_UP;    //

extern const wxEventType EVENT_GRID_SELECT_RANGE; //generates: GridRangeSelectEvent
//NOTE: neither first nor second row need to match EVENT_GRID_MOUSE_LEFT_DOWN/EVENT_GRID_MOUSE_LEFT_UP: user holding SHIFT; moving out of window...
//=> range always specifies *valid* rows

//example: wnd.Connect(EVENT_GRID_COL_LABEL_LEFT_CLICK, GridClickEventHandler(MyDlg::OnLeftClick), NULL, this);


struct GridClickEvent : public wxMouseEvent
{
    GridClickEvent(wxEventType et, const wxMouseEvent& me, int row, ColumnType colType, size_t compPos) : wxMouseEvent(me), row_(row), colType_(colType), compPos_(compPos) { SetEventType(et); }
    virtual wxEvent* Clone() const { return new GridClickEvent(*this); }

    const int row_;
    const ColumnType colType_;
    const size_t compPos_;
};

struct GridColumnResizeEvent : public wxCommandEvent
{
    GridColumnResizeEvent(int width, ColumnType colType, size_t compPos) : wxCommandEvent(EVENT_GRID_COL_RESIZE), colType_(colType), width_(width), compPos_(compPos) {}
    virtual wxEvent* Clone() const { return new GridColumnResizeEvent(*this); }

    const ColumnType colType_;
    const int        width_;
    const size_t     compPos_;
};

struct GridRangeSelectEvent : public wxCommandEvent
{
    GridRangeSelectEvent(int rowFrom, int rowTo, size_t compPos, bool positive) : wxCommandEvent(EVENT_GRID_SELECT_RANGE), rowFrom_(rowFrom), rowTo_(rowTo), compPos_(compPos), positive_(positive) {}
    virtual wxEvent* Clone() const { return new GridRangeSelectEvent(*this); }

    const int    rowFrom_;
    const int    rowTo_;
    const size_t compPos_;
    const bool   positive_;
};

typedef void (wxEvtHandler::*GridClickEventFunction       )(GridClickEvent&);
typedef void (wxEvtHandler::*GridColumnResizeEventFunction)(GridColumnResizeEvent&);
typedef void (wxEvtHandler::*GridRangeSelectEventFunction )(GridRangeSelectEvent&);

#define GridClickEventHandler(func) \
    (wxObjectEventFunction)(wxEventFunction)wxStaticCastEvent(GridClickEventFunction, &func)

#define GridColumnResizeEventHandler(func) \
    (wxObjectEventFunction)(wxEventFunction)wxStaticCastEvent(GridColumnResizeEventFunction, &func)

#define GridRangeSelectEventHandler(func) \
    (wxObjectEventFunction)(wxEventFunction)wxStaticCastEvent(GridRangeSelectEventFunction, &func)
//------------------------------------------------------------------------------------------------------------
class Grid;
wxColor getColorSelectionGradientFrom();
wxColor getColorSelectionGradientTo();

void clearArea(wxDC& dc, const wxRect& rect, const wxColor& col);

class GridData
{
public:
    virtual ~GridData() {}

    virtual size_t getRowCount() const = 0; //if there are multiple grid components, only the first one will be polled for row count!

    //grid area
    virtual wxString getValue(int row, ColumnType colType) const = 0;
    virtual void renderRowBackgound(wxDC& dc, const wxRect& rect, int row, bool enabled, bool selected, bool hasFocus); //default implementation
    virtual void renderCell(Grid& grid, wxDC& dc, const wxRect& rect, int row, ColumnType colType); //
    virtual size_t getBestSize(wxDC& dc, int row, ColumnType colType); //must correspond to renderCell()!
    virtual wxString getToolTip(int row, ColumnType colType) const { return wxString(); }

    //label area
    virtual wxString getColumnLabel(ColumnType colType) const = 0;
    virtual void renderColumnLabel(Grid& grid, wxDC& dc, const wxRect& rect, ColumnType colType, bool highlighted); //default implementation
    virtual wxString getToolTip(ColumnType colType) const { return wxString(); }

protected: //optional helper routines
    static wxRect drawCellBorder  (wxDC& dc, const wxRect& rect); //returns inner rectangle
    static void drawCellBackground(wxDC& dc, const wxRect& rect, bool enabled, bool selected, bool hasFocus, const wxColor& backgroundColor);
    static void drawCellText      (wxDC& dc, const wxRect& rect, const wxString& text, bool enabled, int alignment = wxALIGN_LEFT | wxALIGN_CENTER_VERTICAL);

    static wxRect drawColumnLabelBorder  (wxDC& dc, const wxRect& rect); //returns inner rectangle
    static void drawColumnLabelBackground(wxDC& dc, const wxRect& rect, bool highlighted);
    static void drawColumnLabelText      (wxDC& dc, const wxRect& rect, const wxString& text);
};


class Grid : public wxScrolledWindow
{
public:
    Grid(wxWindow* parent,
         wxWindowID id        = wxID_ANY,
         const wxPoint& pos   = wxDefaultPosition,
         const wxSize& size   = wxDefaultSize,
         long style           = wxTAB_TRAVERSAL | wxNO_BORDER,
         const wxString& name = wxPanelNameStr);

    size_t getRowCount() const;

    void setRowHeight(int height);

    //grid component := a grid is divided into multiple components each of which is essentially a set of connected columns
    void setComponentCount(size_t count) { comp.resize(count); updateWindowSizes(); }
    size_t getComponentCount() const { return comp.size(); }

    struct ColumnAttribute
    {
        ColumnAttribute(ColumnType type, int width, bool visible = true) : type_(type), width_(width), visible_(visible) {}
        ColumnType type_;
        int width_; //if negative, treat as proportional stretch!
        bool visible_;
    };

    void setColumnConfig(const std::vector<ColumnAttribute>& attr, size_t compPos = 0); //set column count + widths
    std::vector<ColumnAttribute> getColumnConfig(size_t compPos = 0) const;

    void setDataProvider(const std::shared_ptr<GridData>& dataView, size_t compPos = 0) { if (compPos < comp.size()) comp[compPos].dataView_ = dataView; }
    /**/  GridData* getDataProvider(size_t compPos = 0)       { return compPos < comp.size() ? comp[compPos].dataView_.get() : NULL; }
    const GridData* getDataProvider(size_t compPos = 0) const { return compPos < comp.size() ? comp[compPos].dataView_.get() : NULL; }
    //-----------------------------------------------------------------------------

    void setColumnLabelHeight(int height);
    void showRowLabel(bool visible);

    void showScrollBars(bool horizontal, bool vertical);

    std::vector<int> getSelectedRows(size_t compPos = 0) const;
    void clearSelection(size_t compPos = 0) { if (compPos < comp.size()) comp[compPos].selection.clear(); }

    void scrollDelta(int deltaX, int deltaY); //in scroll units

    wxWindow& getCornerWin  ();
    wxWindow& getRowLabelWin();
    wxWindow& getColLabelWin();
    wxWindow& getMainWin    ();

    int getRowAtPos(int posY) const; //returns < 0 if column not found; absolute coordinates!
    Opt<std::pair<ColumnType, size_t>> getColumnAtPos(int posX) const; //returns (column type, component pos)

    wxRect getCellArea(int row, ColumnType colType, size_t compPos = 0) const; //returns empty rect if column not found; absolute coordinates!

    void enableColumnMove  (bool value, size_t compPos = 0) { if (compPos < comp.size()) comp[compPos].allowColumnMove   = value; }
    void enableColumnResize(bool value, size_t compPos = 0) { if (compPos < comp.size()) comp[compPos].allowColumnResize = value; }

    void setGridCursor(int row, size_t compPos = 0); //set + show + select cursor
    std::pair<int, size_t> getGridCursor() const; //(row, component pos) row == -1 if none

    void scrollTo(int row);

    virtual void Refresh(bool eraseBackground = true, const wxRect* rect = NULL);
    virtual bool Enable( bool enable = true) { Refresh(); return wxScrolledWindow::Enable(enable); }
    void autoSizeColumns(size_t compPos = 0);

private:
    void onPaintEvent(wxPaintEvent& event);
    void onEraseBackGround(wxEraseEvent& event) {} //[!]
    void onSizeEvent(wxEvent& evt) { updateWindowSizes(); }

    void updateWindowSizes(bool updateScrollbar = true);

    void redirectRowLabelEvent(wxMouseEvent& event);

#ifdef FFS_WIN
    virtual WXLRESULT MSWDefWindowProc(WXUINT nMsg, WXWPARAM wParam, WXLPARAM lParam); //support horizontal mouse wheel
    void SetScrollbar(int orientation, int position, int thumbSize, int range, bool refresh); //get rid of scrollbars, but preserve scrolling behavior!
#endif

    int getBestColumnSize(size_t col, size_t compPos) const; //return -1 on error

    friend class GridData;
    class SubWindow;
    class CornerWin;
    class RowLabelWin;
    class ColLabelWin;
    class MainWin;

    class Selection
    {
    public:
        void init(size_t rowCount) { rowSelectionValue.resize(rowCount); clear(); }

        std::vector<int> get() const
        {
            std::vector<int> selection;
            for (auto iter = rowSelectionValue.begin(); iter != rowSelectionValue.end(); ++iter)
                if (*iter != 0)
                    selection.push_back(iter - rowSelectionValue.begin());
            return selection;
        }

        void clear() { selectRange(0, rowSelectionValue.size(), false); }

        bool isSelected(size_t row) const { return row < rowSelectionValue.size() ? rowSelectionValue[row] != 0 : false; }

        void selectRange(int rowFrom, int rowTo, bool positive = true) //select [rowFrom, rowTo], very tolerant: trims and swaps if required!
        {
            int rowFirst = std::min(rowFrom, rowTo);
            int rowLast  = std::max(rowFrom, rowTo) + 1;

            numeric::restrict(rowFirst, 0, static_cast<int>(rowSelectionValue.size()));
            numeric::restrict(rowLast,  0, static_cast<int>(rowSelectionValue.size()));

            std::fill(rowSelectionValue.begin() + rowFirst, rowSelectionValue.begin() + rowLast, positive);
        }

    private:
        std::vector<char> rowSelectionValue; //effectively a vector<bool> of size "number of rows"
    };

    struct VisibleColumn
    {
        VisibleColumn(ColumnType type, int width) : type_(type), width_(width) {}
        ColumnType type_;
        int width_; //may be NEGATIVE => treat as proportional stretch! use getAbsoluteWidths() to evaluate!!!
    };

    struct Component
    {
        Component() : allowColumnMove(true), allowColumnResize(true) {}

        std::shared_ptr<GridData> dataView_;
        Selection selection;
        bool allowColumnMove;
        bool allowColumnResize;

        std::vector<VisibleColumn> visibleCols; //individual widths, type and total column count
        std::vector<ColumnAttribute> oldColAttributes; //visible + nonvisible columns; use for conversion in setColumnConfig()/getColumnConfig() *only*!
    };

    int getMinAbsoluteWidthTotal() const; //assigns minimum width to stretched columns
    std::vector<std::vector<VisibleColumn>> getAbsoluteWidths() const; //evaluate negative widths as stretched absolute values! structure matches "comp"

    Opt<size_t> getAbsoluteWidth(size_t col, size_t compPos) const //resolve stretched columns
    {
        const auto& absWidth = getAbsoluteWidths();
        if (compPos < absWidth.size() && col < absWidth[compPos].size())
            return absWidth[compPos][col].width_;
        return NoValue();
    }

    void setColWidth(size_t col, size_t compPos, int width) //width may be >= 0: absolute, or < 0: stretched
    {
        if (compPos < comp.size() && col < comp[compPos].visibleCols.size())
            comp[compPos].visibleCols[col].width_ = width;
    }

    wxRect getColumnLabelArea(ColumnType colType, size_t compPos) const; //returns empty rect if column not found

    void selectRange(int rowFrom, int rowTo, size_t compPos, bool positive = true); //select range + notify event!
    void clearSelectionAll(); //clear selection + notify event

    bool isSelected(int row, size_t compPos) const { return compPos < comp.size() ? comp[compPos].selection.isSelected(row) : false; }

    bool columnMoveAllowed  (size_t compPos) const { return compPos < comp.size() ? comp[compPos].allowColumnMove   : false; }
    bool columnResizeAllowed(size_t compPos) const { return compPos < comp.size() ? comp[compPos].allowColumnResize : false; }

    struct ColAction
    {
        bool wantResize; //"!wantResize" means "move" or "single click"
        size_t col;
        size_t compPos;
    };
    Opt<ColAction> clientPosToColumnAction(const wxPoint& pos) const;
    void moveColumn(size_t colFrom, size_t colTo, size_t compPos);
    int clientPosToMoveTargetColumn(const wxPoint& pos, size_t compPos) const;

    Opt<ColumnType> colToType(size_t col, size_t compPos) const;


    /*
    Visual layout:
        ------------------------------------------------
        |CornerWin   | RowLabelWin:                    |
        |--------------------------  Comp1 | Comp2 ... |   row label and main window are vertically tiled into one or more "components"
        |ColLabelWin | MainWin:                        |
        ------------------------------------------------
    */
    CornerWin*   cornerWin_;
    RowLabelWin* rowLabelWin_;
    ColLabelWin* colLabelWin_;
    MainWin*     mainWin_;

    bool showScrollbarX;
    bool showScrollbarY;

    int colLabelHeight;
    bool drawRowLabel;

    std::vector<Component> comp;
    size_t colSizeOld; //at the time of last Grid::Refresh()
};
}

#endif //GENERIC_GRID_HEADER_83470213483173
