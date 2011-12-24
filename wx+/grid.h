// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) 2008-2011 ZenJu (zhnmju123 AT gmx.de)                    *
// **************************************************************************

#ifndef WX_TREE_LIST_HEADER_8347021348317
#define WX_TREE_LIST_HEADER_8347021348317

#include <set>
#include <memory>
#include <vector>
#include <wx/scrolwin.h>

namespace zen
{
//------------------------------------------------------------------------------------------------------------
class Grid;

class GridDataView
{
public:
    virtual ~GridDataView() {}

    virtual wxString getValue(int row, int col) = 0;
    virtual void renderCell(Grid& grid, wxDC& dc, const wxRect& rect, int row, int col, bool selected); //default implementation

    virtual wxString getColumnLabel(int col) = 0;
    virtual void renderColumnLabel(Grid& tree, wxDC& dc, const wxRect& rect, int col, bool highlighted); //default implementation

protected:
    static wxRect drawCellBorder  (wxDC& dc, const wxRect& rect); //returns inner rectangle
    static void drawCellBackground(wxDC& dc, const wxRect& rect, bool selected, bool enabled, bool hasFocus);
    static void drawCellText      (wxDC& dc, const wxRect& rect, const wxString& text, bool enabled);

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

    //------------ set grid data --------------------------------------------------
    void setRowCount(int rowCount);
    int getRowCount() const;

    void setRowHeight(int height);
    int getRowHeight();

    void setColumnConfig(const std::vector<int>& widths); //set column count + widths
    std::vector<int> getColumnConfig() const;

    void setDataProvider(const std::shared_ptr<GridDataView>& dataView) { dataView_ = dataView; }
    //-----------------------------------------------------------------------------
	
	void setColumnLabelHeight(int height);
	void showRowLabel(bool visible);

	void showScrollBars(bool horizontal, bool vertical);

    std::vector<int> getSelectedRows() const;

private:
    void onSizeEvent(wxEvent& evt) { updateWindowSizes(); }

    void updateWindowSizes();

    int getScrollUnitsTotalX() const;
    int getScrollUnitsTotalY() const;

    int getPixelsPerScrollUnit() const;

    void scrollDelta(int deltaX, int deltaY); //in scroll units

    void redirectRowLabelEvent(wxMouseEvent& event);

#ifdef FFS_WIN
    virtual WXLRESULT MSWDefWindowProc(WXUINT nMsg, WXWPARAM wParam, WXLPARAM lParam); //support horizontal mouse wheel
void SetScrollbar(int orientation, int position, int thumbSize, int range, bool refresh); //get rid of scrollbars, but preserve scrolling behavior!
#endif
    GridDataView* getDataProvider() { return dataView_.get(); }

    static const int defaultRowHeight      = 20;
    static const int defaultColLabelHeight = 25;
    static const int columnBorderLeft  = 4; //for left-aligned text
    static const int columnLabelBorder = columnBorderLeft;
    static const int columnMoveDelay   = 5; //unit [pixel] (Explorer)
    static const int columnMinWidth    = 30;
    static const int rowLabelBorder    = 3;
    static const int columnResizeTolerance = 5; //unit [pixel]
    static const int mouseDragScrollIncrementY = 2; //in scroll units
    static const int mouseDragScrollIncrementX = 1; //

    friend class GridDataView;
    class SubWindow;
    class CornerWin;
    class RowLabelWin;
    class ColLabelWin;
    class MainWin;

    /*
    Visual layout:
        ----------------------------
        |CornerWin   | RowLabelWin |
        |---------------------------
        |ColLabelWin | MainWin     |
        ----------------------------
    */
    CornerWin*   cornerWin_;
    RowLabelWin* rowLabelWin_;
    ColLabelWin* colLabelWin_;
    MainWin*     mainWin_;

    std::shared_ptr<GridDataView> dataView_;

	bool showScrollbarX;
	bool showScrollbarY;

	int colLabelHeight;
	int drawRowLabel;
};
}


#endif //WX_TREE_LIST_HEADER_8347021348317
