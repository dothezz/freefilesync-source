#include "customGrid.h"
#include "globalFunctions.h"
#include "resources.h"
#include <wx/dc.h>
#include "../algorithm.h"
#include "resources.h"
#include <typeinfo>
#include "../ui/gridView.h"
#include "../synchronization.h"

#ifdef FFS_WIN
#include <wx/timer.h>
#include <wx/icon.h>
#include "iconBuffer.h"
#include "statusHandler.h"
#include <cmath>

#elif defined FFS_LINUX
#include <gtk/gtk.h>
#endif

using namespace FreeFileSync;


const unsigned int MIN_ROW_COUNT = 15;

//class containing pure grid data: basically the same as wxGridStringTable, but adds cell formatting

/*
class hierarchy:
                              CustomGridTable
                                    /|\
                     ________________|________________
                    |                                |
           CustomGridTableRim                        |
                   /|\                               |
          __________|__________                      |
         |                    |                      |
CustomGridTableLeft  CustomGridTableRight  CustomGridTableMiddle
*/

class CustomGridTable : public wxGridTableBase
{
public:
    CustomGridTable(int initialRows = 0, int initialCols = 0) : //note: initialRows/initialCols MUST match with GetNumberRows()/GetNumberCols() at initialization!!!
            wxGridTableBase(),
            COLOR_BLUE(      80,  110, 255),
            COLOR_GREY(      212, 208, 200),
            COLOR_CMP_RED(   249, 163, 165),
            COLOR_CMP_BLUE(  144, 232, 246),
            COLOR_CMP_GREEN( 147, 253, 159),
            COLOR_SYNC_BLUE( 201, 203, 247),
            COLOR_SYNC_GREEN(197, 248, 190),
            COLOR_YELLOW(    247, 252,  62),
            gridDataView(NULL),
            lastNrRows(initialRows),
            lastNrCols(initialCols) {}


    virtual ~CustomGridTable() {}


    void setGridDataTable(const GridView* gridDataView)
    {
        this->gridDataView = gridDataView;
    }


//###########################################################################
//grid standard input output methods, redirected directly to gridData to improve performance

    virtual int GetNumberRows()
    {
        if (gridDataView)
            return std::max(gridDataView->elementsOnView(), MIN_ROW_COUNT);
        else
            return 0; //grid is initialized with zero number of rows
    }


    virtual bool IsEmptyCell( int row, int col )
    {
        return false; //avoid overlapping cells

        //return (GetValue(row, col) == wxEmptyString);
    }


    virtual void SetValue(int row, int col, const wxString& value)
    {
        assert (false); //should not be used, since values are retrieved directly from gridDataView
    }

    //update dimensions of grid: no need for InsertRows(), AppendRows(), DeleteRows() anymore!!!
    void updateGridSizes()
    {
        const int currentNrRows = GetNumberRows();

        if (lastNrRows < currentNrRows)
        {
            if (GetView())
            {
                wxGridTableMessage msg(this,
                                       wxGRIDTABLE_NOTIFY_ROWS_APPENDED,
                                       currentNrRows - lastNrRows);

                GetView()->ProcessTableMessage( msg );
            }
        }
        else if (lastNrRows > currentNrRows)
        {
            if (GetView())
            {
                wxGridTableMessage msg(this,
                                       wxGRIDTABLE_NOTIFY_ROWS_DELETED,
                                       0,
                                       lastNrRows - currentNrRows);

                GetView()->ProcessTableMessage( msg );
            }
        }
        lastNrRows = currentNrRows;

        const int currentNrCols = GetNumberCols();

        if (lastNrCols < currentNrCols)
        {
            if (GetView())
            {
                wxGridTableMessage msg(this,
                                       wxGRIDTABLE_NOTIFY_COLS_APPENDED,
                                       currentNrCols - lastNrCols);

                GetView()->ProcessTableMessage( msg );
            }
        }
        else if (lastNrCols > currentNrCols)
        {
            if (GetView())
            {
                wxGridTableMessage msg(this,
                                       wxGRIDTABLE_NOTIFY_COLS_DELETED,
                                       0,
                                       lastNrCols - currentNrCols);

                GetView()->ProcessTableMessage( msg );
            }
        }
        lastNrCols = currentNrCols;
    }
//###########################################################################


    virtual wxGridCellAttr* GetAttr(int row, int col, wxGridCellAttr::wxAttrKind  kind)
    {
        const wxColour& color = getRowColor(row);

        //add color to some rows
        wxGridCellAttr* result = wxGridTableBase::GetAttr(row, col, kind);
        if (result)
        {
            if (result->GetBackgroundColour() == color)
            {
                return result;
            }
            else //grid attribute might be referenced by other nodes, so clone it!
            {
                wxGridCellAttr* attr = result->Clone(); //attr has ref-count 1
                result->DecRef();
                result = attr;
            }
        }
        else
            result = new wxGridCellAttr; //created with ref-count 1

        result->SetBackgroundColour(color);

        return result;
    }


    const FileCompareLine* getRawData(const unsigned int row) const
    {
        if (gridDataView && row < gridDataView->elementsOnView())
        {
            return &(*gridDataView)[row];
        }
        return NULL;
    }

protected:
    const wxColour COLOR_BLUE;
    const wxColour COLOR_GREY;
    const wxColour COLOR_CMP_RED;
    const wxColour COLOR_CMP_BLUE;
    const wxColour COLOR_CMP_GREEN;
    const wxColour COLOR_SYNC_BLUE;
    const wxColour COLOR_SYNC_GREEN;
    const wxColour COLOR_YELLOW;

    const GridView* gridDataView; //(very fast) access to underlying grid data :)

private:
    virtual const wxColour& getRowColor(int row) = 0; //rows that are filtered out are shown in different color

    int lastNrRows;
    int lastNrCols;
};


class CustomGridTableRim : public CustomGridTable
{
public:
    virtual int GetNumberCols()
    {
        return columnPositions.size();
    }

    virtual wxString GetColLabelValue( int col )
    {
        return CustomGridRim::getTypeName(getTypeAtPos(col));
    }


    void setupColumns(const std::vector<xmlAccess::ColumnTypes>& positions)
    {
        columnPositions = positions;
        updateGridSizes(); //add or remove columns
    }


    xmlAccess::ColumnTypes getTypeAtPos(unsigned pos) const
    {
        if (pos < columnPositions.size())
            return columnPositions[pos];
        else
            return xmlAccess::ColumnTypes(1000);
    }

    virtual Zstring getFileName(const unsigned int row) const = 0;

private:
    std::vector<xmlAccess::ColumnTypes> columnPositions;
};


class CustomGridTableLeft : public CustomGridTableRim
{
public:
    virtual wxString GetValue(int row, int col)
    {
        const FileCompareLine* gridLine = getRawData(row);
        if (gridLine)
        {
            if (gridLine->fileDescrLeft.objType == FileDescrLine::TYPE_DIRECTORY)
            {
                switch (getTypeAtPos(col))
                {
                case xmlAccess::FULL_PATH:
                    return wxString(gridLine->fileDescrLeft.fullName.c_str());
                case xmlAccess::FILENAME:
                    return wxEmptyString;
                case xmlAccess::REL_PATH:
                    return gridLine->fileDescrLeft.relativeName.c_str();
                case xmlAccess::DIRECTORY:
                    return gridDataView->getFolderPair(row).leftDirectory.c_str();
                case xmlAccess::SIZE: //file size
                    return _("<Directory>");
                case xmlAccess::DATE: //date
                    return wxEmptyString;
                }
            }
            else if (gridLine->fileDescrLeft.objType == FileDescrLine::TYPE_FILE)
            {
                switch (getTypeAtPos(col))
                {
                case xmlAccess::FULL_PATH:
                    return wxString(gridLine->fileDescrLeft.fullName.c_str()).BeforeLast(FreeFileSync::FILE_NAME_SEPARATOR);
                case xmlAccess::FILENAME: //filename
                    return wxString(gridLine->fileDescrLeft.relativeName.c_str()).AfterLast(FreeFileSync::FILE_NAME_SEPARATOR);
                case xmlAccess::REL_PATH: //relative path
                    return wxString(gridLine->fileDescrLeft.relativeName.c_str()).BeforeLast(FreeFileSync::FILE_NAME_SEPARATOR);
                case xmlAccess::DIRECTORY:
                    return gridDataView->getFolderPair(row).leftDirectory.c_str();
                case xmlAccess::SIZE: //file size
                    return globalFunctions::includeNumberSeparator(gridLine->fileDescrLeft.fileSize.ToString());
                case xmlAccess::DATE: //date
                    return FreeFileSync::utcTimeToLocalString(gridLine->fileDescrLeft.lastWriteTimeRaw, gridLine->fileDescrLeft.fullName);
                }
            }
        }
        //if data is not found:
        return wxEmptyString;
    }


    virtual Zstring getFileName(const unsigned int row) const
    {
        const FileCompareLine* gridLine = getRawData(row);
        if (gridLine)
            return Zstring(gridLine->fileDescrLeft.fullName);
        else
            return Zstring();
    }


private:
    virtual const wxColour& getRowColor(int row) //rows that are filtered out are shown in different color
    {
        const FileCompareLine* gridLine = getRawData(row);
        if (gridLine)
        {
            //mark filtered rows
            if (!gridLine->selectedForSynchronization)
                return COLOR_BLUE;
            //mark directories
            else if (gridLine->fileDescrLeft.objType == FileDescrLine::TYPE_DIRECTORY)
                return COLOR_GREY;
            else
                return *wxWHITE;
        }
        return *wxWHITE;
    }
};


class CustomGridTableRight : public CustomGridTableRim
{
public:
    virtual wxString GetValue(int row, int col)
    {
        const FileCompareLine* gridLine = getRawData(row);
        if (gridLine)
        {
            if (gridLine->fileDescrRight.objType == FileDescrLine::TYPE_DIRECTORY)
            {
                switch (getTypeAtPos(col))
                {
                case xmlAccess::FULL_PATH:
                    return wxString(gridLine->fileDescrRight.fullName.c_str());
                case xmlAccess::FILENAME: //filename
                    return wxEmptyString;
                case xmlAccess::REL_PATH: //relative path
                    return gridLine->fileDescrRight.relativeName.c_str();
                case xmlAccess::DIRECTORY:
                    return gridDataView->getFolderPair(row).rightDirectory.c_str();
                case xmlAccess::SIZE: //file size
                    return _("<Directory>");
                case xmlAccess::DATE: //date
                    return wxEmptyString;
                }
            }
            else if (gridLine->fileDescrRight.objType == FileDescrLine::TYPE_FILE)
            {
                switch (getTypeAtPos(col))
                {
                case xmlAccess::FULL_PATH:
                    return wxString(gridLine->fileDescrRight.fullName.c_str()).BeforeLast(FreeFileSync::FILE_NAME_SEPARATOR);
                case xmlAccess::FILENAME: //filename
                    return wxString(gridLine->fileDescrRight.relativeName.c_str()).AfterLast(FreeFileSync::FILE_NAME_SEPARATOR);
                case xmlAccess::REL_PATH: //relative path
                    return wxString(gridLine->fileDescrRight.relativeName.c_str()).BeforeLast(FreeFileSync::FILE_NAME_SEPARATOR);
                case xmlAccess::DIRECTORY:
                    return gridDataView->getFolderPair(row).rightDirectory.c_str();
                case xmlAccess::SIZE: //file size
                    return globalFunctions::includeNumberSeparator(gridLine->fileDescrRight.fileSize.ToString());
                case xmlAccess::DATE: //date
                    return FreeFileSync::utcTimeToLocalString(gridLine->fileDescrRight.lastWriteTimeRaw, gridLine->fileDescrRight.fullName);
                }
            }
        }
        //if data is not found:
        return wxEmptyString;
    }


    virtual Zstring getFileName(const unsigned int row) const
    {
        const FileCompareLine* gridLine = getRawData(row);
        if (gridLine)
            return Zstring(gridLine->fileDescrRight.fullName);
        else
            return Zstring();
    }


private:
    virtual const wxColour& getRowColor(int row) //rows that are filtered out are shown in different color
    {
        const FileCompareLine* gridLine = getRawData(row);
        if (gridLine)
        {
            //mark filtered rows
            if (!gridLine->selectedForSynchronization)
                return COLOR_BLUE;
            //mark directories
            else if (gridLine->fileDescrRight.objType == FileDescrLine::TYPE_DIRECTORY)
                return COLOR_GREY;
            else
                return *wxWHITE;
        }
        return *wxWHITE;
    }
};


class CustomGridTableMiddle : public CustomGridTable
{
public:
//middle grid is created (first wxWidgets internal call to GetNumberCols()) with one column
    CustomGridTableMiddle() :
            CustomGridTable(0, GetNumberCols()), //attention: static binding to virtual GetNumberCols() in a Constructor!
            syncPreviewActive(false) {}

    virtual int GetNumberCols()
    {
        return 1;
    }

    virtual wxString GetColLabelValue( int col )
    {
        return wxEmptyString;
    }

    virtual wxString GetValue(int row, int col)
    {
        return wxEmptyString;
    }

    void enableSyncPreview(bool value)
    {
        syncPreviewActive = value;
    }

    bool syncPreviewIsActive() const
    {
        return syncPreviewActive;
    }

private:
    virtual const wxColour& getRowColor(int row) //rows that are filtered out are shown in different color
    {
        const FileCompareLine* gridLine = getRawData(row);
        if (gridLine)
        {
            //mark filtered rows
            if (!gridLine->selectedForSynchronization)
                return COLOR_BLUE;

            if (syncPreviewActive) //synchronization preview
            {
                switch (gridLine->direction)
                {
                case SYNC_DIR_LEFT:
                    return COLOR_SYNC_BLUE;
                case SYNC_DIR_RIGHT:
                    return COLOR_SYNC_GREEN;
                case SYNC_DIR_NONE:
                    return *wxWHITE;
                case SYNC_UNRESOLVED_CONFLICT:
                    return COLOR_YELLOW;
                }
            }
            else //comparison results view
            {
                switch (gridLine->cmpResult)
                {
                case FILE_LEFT_SIDE_ONLY:
                case FILE_RIGHT_SIDE_ONLY:
                    return COLOR_CMP_GREEN;
                case FILE_LEFT_NEWER:
                case FILE_RIGHT_NEWER:
                    return COLOR_CMP_BLUE;
                case FILE_DIFFERENT:
                    return COLOR_CMP_RED;
                case FILE_EQUAL:
                    return *wxWHITE;
                case FILE_CONFLICT:
                    return COLOR_YELLOW;
                }
            }
        }

        //fallback
        return *wxWHITE;
    }

    bool syncPreviewActive; //determines wheter grid shall show compare result or sync preview
};

//########################################################################################################


CustomGrid::CustomGrid(wxWindow *parent,
                       wxWindowID id,
                       const wxPoint& pos,
                       const wxSize& size,
                       long style,
                       const wxString& name) :
        wxGrid(parent, id, pos, size, style, name),
        m_gridLeft(NULL),
        m_gridMiddle(NULL),
        m_gridRight(NULL),
        isLeading(false),
        m_marker(-1, ASCENDING)
{
    //set color of selections
    wxColour darkBlue(40, 35, 140);
    SetSelectionBackground(darkBlue);
    SetSelectionForeground(*wxWHITE);
}


void CustomGrid::initSettings(CustomGridLeft*   gridLeft,
                              CustomGridMiddle* gridMiddle,
                              CustomGridRight*  gridRight,
                              const GridView*   gridDataView)
{
    assert(this == gridLeft || this == gridRight || this == gridMiddle);

    //these grids will scroll together
    m_gridLeft   = gridLeft;
    m_gridRight  = gridRight;
    m_gridMiddle = gridMiddle;

    //set underlying grid data
    setGridDataTable(gridDataView);

    //enhance grid functionality; identify leading grid by keyboard input or scroll action
    Connect(wxEVT_KEY_DOWN,                      wxEventHandler(CustomGrid::onGridAccess), NULL, this);
    Connect(wxEVT_SCROLLWIN_TOP,                 wxEventHandler(CustomGrid::onGridAccess), NULL, this);
    Connect(wxEVT_SCROLLWIN_BOTTOM,              wxEventHandler(CustomGrid::onGridAccess), NULL, this);
    Connect(wxEVT_SCROLLWIN_LINEUP,              wxEventHandler(CustomGrid::onGridAccess), NULL, this);
    Connect(wxEVT_SCROLLWIN_LINEDOWN,            wxEventHandler(CustomGrid::onGridAccess), NULL, this);
    Connect(wxEVT_SCROLLWIN_PAGEUP,              wxEventHandler(CustomGrid::onGridAccess), NULL, this);
    Connect(wxEVT_SCROLLWIN_PAGEDOWN,            wxEventHandler(CustomGrid::onGridAccess), NULL, this);
    Connect(wxEVT_SCROLLWIN_THUMBTRACK,          wxEventHandler(CustomGrid::onGridAccess), NULL, this);
    Connect(wxEVT_SCROLLWIN_THUMBRELEASE,        wxEventHandler(CustomGrid::onGridAccess), NULL, this);
    Connect(wxEVT_GRID_LABEL_LEFT_CLICK,         wxEventHandler(CustomGrid::onGridAccess), NULL, this);
    GetGridWindow()->Connect(wxEVT_LEFT_DOWN,    wxEventHandler(CustomGrid::onGridAccess), NULL, this);
    GetGridWindow()->Connect(wxEVT_RIGHT_DOWN,    wxEventHandler(CustomGrid::onGridAccess), NULL, this);
    GetGridWindow()->Connect(wxEVT_ENTER_WINDOW, wxEventHandler(CustomGrid::adjustGridHeights), NULL, this);
}


bool CustomGrid::isLeadGrid() const
{
    return isLeading;
}


void CustomGrid::RefreshCell(int row, int col)
{
    wxRect rectScrolled(CellToRect(row, col));

    CalcScrolledPosition(rectScrolled.x, rectScrolled.y, &rectScrolled.x, &rectScrolled.y);

    GetGridWindow()->RefreshRect(rectScrolled); //note: CellToRect() and YToRow work on m_gridWindow NOT on the whole grid!
}


void CustomGrid::DoPrepareDC(wxDC& dc)
{
    wxScrollHelper::DoPrepareDC(dc);

    if (isLeadGrid())  //avoid back coupling
        alignOtherGrids(m_gridLeft, m_gridMiddle, m_gridRight); //scroll other grids
}


inline
void moveCursorWhileSelecting(const int anchor, const int oldPos, const int newPos, wxGrid* grid)
{   //note: all positions are valid in this context!

    grid->SetGridCursor(newPos, grid->GetGridCursorCol());
    grid->MakeCellVisible(newPos, grid->GetGridCursorCol());

    if (oldPos < newPos)
    {
        for (int i = oldPos; i < std::min(anchor, newPos); ++i)
            grid->DeselectRow(i); //remove selection

        for (int i = std::max(oldPos, anchor); i <= newPos; ++i)
            grid->SelectRow(i, true); //add to selection
    }
    else
    {
        for (int i = std::max(newPos, anchor) + 1; i <= oldPos; ++i)
            grid->DeselectRow(i); //remove selection

        for (int i = newPos; i <= std::min(oldPos, anchor); ++i)
            grid->SelectRow(i, true); //add to selection
    }
}


inline
void additionalGridCommands(wxEvent& event, wxGrid* grid)
{
    static int anchorRow = 0;
    assert(grid->GetNumberRows() != 0);

    try
    {
        const wxKeyEvent& keyEvent = dynamic_cast<const wxKeyEvent&> (event);

        if (keyEvent.ShiftDown())
        {
            //ensure cursorOldPos is always a valid row!
            const int cursorOldPos = std::max(std::min(grid->GetGridCursorRow(), grid->GetNumberRows() - 1), 0);

            //support for shift + PageUp and shift + PageDown
            switch (keyEvent.GetKeyCode())
            {
            case WXK_UP: //move grid cursor also
            {
                const int cursorNewPos = std::max(cursorOldPos - 1, 0);
                moveCursorWhileSelecting(anchorRow, cursorOldPos, cursorNewPos, grid);
            }
            return; //no event.Skip()

            case WXK_DOWN: //move grid cursor also
            {
                const int cursorNewPos = std::min(cursorOldPos + 1, grid->GetNumberRows() - 1);
                moveCursorWhileSelecting(anchorRow, cursorOldPos, cursorNewPos, grid);
            }
            return; //no event.Skip()

            case WXK_PAGEUP:
            case WXK_NUMPAD_PAGEUP:
            {
                const int rowsPerPage  = grid->GetGridWindow()->GetSize().GetHeight() / grid->GetDefaultRowSize();
                const int cursorNewPos = std::max(cursorOldPos - rowsPerPage, 0);
                moveCursorWhileSelecting(anchorRow, cursorOldPos, cursorNewPos, grid);
            }
            return; //no event.Skip()

            case WXK_PAGEDOWN:
            case WXK_NUMPAD_PAGEDOWN:
            {
                const int rowsPerPage  = grid->GetGridWindow()->GetSize().GetHeight() / grid->GetDefaultRowSize();
                const int cursorNewPos = std::min(cursorOldPos + rowsPerPage, grid->GetNumberRows() - 1);
                moveCursorWhileSelecting(anchorRow, cursorOldPos, cursorNewPos, grid);
            }
            return; //no event.Skip()

            case WXK_HOME:
            case WXK_NUMPAD_HOME:
            {
                const int cursorNewPos = 0;
                moveCursorWhileSelecting(anchorRow, cursorOldPos, cursorNewPos, grid);
            }
            return; //no event.Skip()

            case WXK_END:
            case WXK_NUMPAD_END:
            {
                const int cursorNewPos = grid->GetNumberRows() - 1;
                moveCursorWhileSelecting(anchorRow, cursorOldPos, cursorNewPos, grid);
            }
            return; //no event.Skip()

            }
        }
        else //button without shift is pressed
        {
            switch (keyEvent.GetKeyCode())
            {
            case WXK_HOME:
            case WXK_NUMPAD_HOME:
                grid->SetGridCursor(0, grid->GetGridCursorCol());
                grid->MakeCellVisible(0, grid->GetGridCursorCol());
                return; //no event.Skip()

            case WXK_END:
            case WXK_NUMPAD_END:
                grid->SetGridCursor(grid->GetNumberRows() - 1, grid->GetGridCursorCol());
                grid->MakeCellVisible(grid->GetNumberRows() - 1, grid->GetGridCursorCol());
                return; //no event.Skip()
            }
        }
    }
    catch (std::bad_cast&) {}

    anchorRow = grid->GetGridCursorRow();
    event.Skip();
}


inline
bool gridsShouldBeCleared(const wxEvent& event)
{
    try
    {
        const wxMouseEvent& mouseEvent = dynamic_cast<const wxMouseEvent&> (event);

        if (mouseEvent.ControlDown() || mouseEvent.ShiftDown())
            return false;

        if (mouseEvent.ButtonDown(wxMOUSE_BTN_LEFT))
            return true;

        return false;
    }
    catch (std::bad_cast&) {}

    try
    {
        const wxKeyEvent& keyEvent = dynamic_cast<const wxKeyEvent&> (event);

        if (keyEvent.ControlDown() || keyEvent.ShiftDown())
            return false;

        switch (keyEvent.GetKeyCode())
        {
        case WXK_SPACE:
        case WXK_TAB:
        case WXK_RETURN:
        case WXK_ESCAPE:
        case WXK_NUMPAD_ENTER:
        case WXK_LEFT:
        case WXK_UP:
        case WXK_RIGHT:
        case WXK_DOWN:
        case WXK_PAGEUP:
        case WXK_PAGEDOWN:
        case WXK_NUMPAD_PAGEUP:
        case WXK_NUMPAD_PAGEDOWN:
        case WXK_HOME:
        case WXK_END:
        case WXK_NUMPAD_HOME:
        case WXK_NUMPAD_END:
            return true;
        }

        return false;
    }
    catch (std::bad_cast&) {}

    return false;
}


void CustomGrid::onGridAccess(wxEvent& event)
{
    if (!isLeading)
    {
        //notify other grids of new user focus
        m_gridLeft->isLeading   = m_gridLeft   == this;
        m_gridMiddle->isLeading = m_gridMiddle == this;
        m_gridRight->isLeading  = m_gridRight  == this;

        wxGrid::SetFocus();
    }

    //clear grids
    if (gridsShouldBeCleared(event))
    {
        m_gridLeft->ClearSelection();
        m_gridMiddle->ClearSelection();
        m_gridRight->ClearSelection();
    }

    //update row labels NOW (needed when scrolling if buttons keep being pressed)
    m_gridLeft->GetGridRowLabelWindow()->Update();
    m_gridRight->GetGridRowLabelWindow()->Update();

    //support for additional short-cuts
    additionalGridCommands(event, this); //event.Skip is handled here!
}


//workaround: ensure that all grids are properly aligned: add some extra window space to grids that have no horizontal scrollbar
void CustomGrid::adjustGridHeights(wxEvent& event)
{
    //m_gridLeft, m_gridRight, m_gridMiddle not NULL because called after initSettings()

    int y1 = 0;
    int y2 = 0;
    int y3 = 0;
    int dummy = 0;

    m_gridLeft->GetViewStart(&dummy, &y1);
    m_gridRight->GetViewStart(&dummy, &y2);
    m_gridMiddle->GetViewStart(&dummy, &y3);

    if (y1 != y2 || y2 != y3)
    {
        int yMax = std::max(y1, std::max(y2, y3));

        if (m_gridLeft->isLeadGrid())  //do not handle case (y1 == yMax) here!!! Avoid back coupling!
            m_gridLeft->SetMargins(0, 0);
        else if (y1 < yMax)
            m_gridLeft->SetMargins(0, 30);

        if (m_gridRight->isLeadGrid())
            m_gridRight->SetMargins(0, 0);
        else if (y2 < yMax)
            m_gridRight->SetMargins(0, 30);

        if (m_gridMiddle->isLeadGrid())
            m_gridMiddle->SetMargins(0, 0);
        else if (y3 < yMax)
            m_gridMiddle->SetMargins(0, 30);

        m_gridLeft->ForceRefresh();
        m_gridRight->ForceRefresh();
        m_gridMiddle->ForceRefresh();
    }
}


void CustomGrid::setSortMarker(SortMarker marker)
{
    m_marker = marker;
}


void CustomGrid::DrawColLabel(wxDC& dc, int col)
{
    wxGrid::DrawColLabel(dc, col);

    if (col == m_marker.first)
    {
        if (m_marker.second == ASCENDING)
            dc.DrawBitmap(*GlobalResources::getInstance().bitmapSmallUp, GetColRight(col) - 16 - 2, 2, true); //respect 2-pixel border
        else
            dc.DrawBitmap(*GlobalResources::getInstance().bitmapSmallDown, GetColRight(col) - 16 - 2, 2, true); //respect 2-pixel border
    }
}


std::set<int> CustomGrid::getAllSelectedRows() const
{
    std::set<int> output;

    const wxArrayInt selectedRows = this->GetSelectedRows();
    if (!selectedRows.IsEmpty())
    {
        for (unsigned int i = 0; i < selectedRows.GetCount(); ++i)
            output.insert(selectedRows[i]);
    }

    if (!this->GetSelectedCols().IsEmpty())    //if a column is selected this is means all rows are marked for deletion
    {
        for (int k = 0; k < const_cast<CustomGrid*>(this)->GetNumberRows(); ++k) //messy wxGrid implementation...
            output.insert(k);
    }

    const wxGridCellCoordsArray singlySelected = this->GetSelectedCells();
    if (!singlySelected.IsEmpty())
    {
        for (unsigned int k = 0; k < singlySelected.GetCount(); ++k)
            output.insert(singlySelected[k].GetRow());
    }

    const wxGridCellCoordsArray tmpArrayTop = this->GetSelectionBlockTopLeft();
    if (!tmpArrayTop.IsEmpty())
    {
        wxGridCellCoordsArray tmpArrayBottom = this->GetSelectionBlockBottomRight();

        unsigned int arrayCount = tmpArrayTop.GetCount();

        if (arrayCount == tmpArrayBottom.GetCount())
        {
            for (unsigned int i = 0; i < arrayCount; ++i)
            {
                const int rowTop    = tmpArrayTop[i].GetRow();
                const int rowBottom = tmpArrayBottom[i].GetRow();

                for (int k = rowTop; k <= rowBottom; ++k)
                    output.insert(k);
            }
        }
    }

    //some exception: also add current cursor row to selection if there are no others... hopefully improving usability
    if (output.empty() && this->isLeadGrid())
        output.insert(const_cast<CustomGrid*>(this)->GetCursorRow()); //messy wxGrid implementation...

    return output;
}


//############################################################################################
//CustomGrid specializations

#ifdef FFS_WIN
template <bool showFileIcons>
class GridCellRenderer : public wxGridCellStringRenderer
{
public:
    GridCellRenderer(CustomGridRim::LoadSuccess& loadIconSuccess, const CustomGridTableRim* gridDataTable) :
            m_loadIconSuccess(loadIconSuccess),
            m_gridDataTable(gridDataTable) {}


    virtual void Draw(wxGrid& grid,
                      wxGridCellAttr& attr,
                      wxDC& dc,
                      const wxRect& rect, //unscrolled rect
                      int row, int col,
                      bool isSelected)
    {
        //############## show windows explorer file icons ######################

        if (showFileIcons) //evaluate at compile time
        {
            if (    m_gridDataTable->getTypeAtPos(col) == xmlAccess::FILENAME &&
                    rect.GetWidth() >= IconBuffer::ICON_SIZE)
            {
                //retrieve grid data
                const Zstring fileName = m_gridDataTable->getFileName(row);
                if (!fileName.empty())
                {
                    //  Partitioning:
                    //   _____________________
                    //  | 2 pix | icon | rest |
                    //   ---------------------

                    //clear area where icon will be placed
                    wxRect rectShrinked(rect);
                    rectShrinked.SetWidth(IconBuffer::ICON_SIZE + 2); //add 2 pixel border
                    wxGridCellRenderer::Draw(grid, attr, dc, rectShrinked, row, col, isSelected);

                    //try to draw icon
                    wxIcon icon;
                    const bool iconLoaded = IconBuffer::getInstance().requestIcon(fileName, &icon); //returns false if icon is not in buffer
                    if (iconLoaded)
                        dc.DrawIcon(icon, rectShrinked.GetX() + 2, rectShrinked.GetY());

                    //-----------------------------------------------------------------------------------------------
                    //only mark as successful if icon was drawn fully!
                    //(attention: when scrolling, rows get partially updated, which can result in the upper half being blank!)

                    //rect where icon was placed
                    wxRect iconRect(rect); //unscrolled
                    iconRect.x += 2;
                    iconRect.SetWidth(IconBuffer::ICON_SIZE);

                    //convert to scrolled coordinates
                    grid.CalcScrolledPosition(iconRect.x, iconRect.y, &iconRect.x, &iconRect.y);

                    bool iconDrawnFully = false;
                    wxRegionIterator regionsInv(grid.GetGridWindow()->GetUpdateRegion());
                    while (regionsInv)
                    {
                        if (regionsInv.GetRect().Contains(iconRect))
                        {
                            iconDrawnFully = true;
                            break;
                        }
                        ++regionsInv;
                    }
                    //-----------------------------------------------------------------------------------------------


                    //save status of last icon load -> used for async. icon loading
                    m_loadIconSuccess[row] = iconLoaded && iconDrawnFully;

                    //draw rest
                    wxRect rest(rect); //unscrolled
                    rest.x     += IconBuffer::ICON_SIZE + 2;
                    rest.width -= IconBuffer::ICON_SIZE + 2;

                    wxGridCellStringRenderer::Draw(grid, attr, dc, rest, row, col, isSelected);
                    return;
                }
            }
        }
        //default
        wxGridCellStringRenderer::Draw(grid, attr, dc, rect, row, col, isSelected);
    }

private:
    CustomGridRim::LoadSuccess& m_loadIconSuccess;
    const CustomGridTableRim* const m_gridDataTable;
};
#endif

//----------------------------------------------------------------------------------------


CustomGridRim::CustomGridRim(wxWindow *parent,
                             wxWindowID id,
                             const wxPoint& pos,
                             const wxSize& size,
                             long style,
                             const wxString& name) :
        CustomGrid(parent, id, pos, size, style, name)
#ifdef FFS_WIN
        , fileIconsAreEnabled(false)
#endif
{}


void CustomGridRim::updateGridSizes()
{
    assert(getGridDataTable());
    getGridDataTable()->updateGridSizes();
}


xmlAccess::ColumnAttributes CustomGridRim::getDefaultColumnAttributes()
{
    xmlAccess::ColumnAttributes defaultColumnSettings;

    xmlAccess::ColumnAttrib newEntry;
    newEntry.type     = xmlAccess::FULL_PATH;
    newEntry.visible  = false;
    newEntry.position = 0;
    newEntry.width    = 150;
    defaultColumnSettings.push_back(newEntry);

    newEntry.type     = xmlAccess::DIRECTORY;
    newEntry.position = 1;
    newEntry.width    = 140;
    defaultColumnSettings.push_back(newEntry);

    newEntry.type     = xmlAccess::REL_PATH;
    newEntry.visible  = true;
    newEntry.position = 2;
    newEntry.width    = 118;
    defaultColumnSettings.push_back(newEntry);

    newEntry.type     = xmlAccess::FILENAME;
    newEntry.position = 3;
    newEntry.width    = 138;
    defaultColumnSettings.push_back(newEntry);

    newEntry.type     = xmlAccess::SIZE;
    newEntry.position = 4;
    newEntry.width    = 70;
    defaultColumnSettings.push_back(newEntry);

    newEntry.type     = xmlAccess::DATE;
    newEntry.position = 5;
    newEntry.width    = 113;
    defaultColumnSettings.push_back(newEntry);

    return defaultColumnSettings;
}


xmlAccess::ColumnAttributes CustomGridRim::getColumnAttributes()
{
    std::sort(columnSettings.begin(), columnSettings.end(), xmlAccess::sortByPositionAndVisibility);

    xmlAccess::ColumnAttributes output;
    xmlAccess::ColumnAttrib newEntry;
    for (unsigned int i = 0; i < columnSettings.size(); ++i)
    {
        newEntry = columnSettings[i];
        if (newEntry.visible)
            newEntry.width = GetColSize(i); //hidden columns are sorted to the end of vector!
        output.push_back(newEntry);
    }

    return output;
}


void CustomGridRim::setColumnAttributes(const xmlAccess::ColumnAttributes& attr)
{
    //remove special alignment for column "size"
    for (int i = 0; i < GetNumberCols(); ++i)
        if (getTypeAtPos(i) == xmlAccess::SIZE)
        {
            wxGridCellAttr* cellAttributes = GetOrCreateCellAttr(0, i);
            cellAttributes->SetAlignment(wxALIGN_LEFT,wxALIGN_CENTRE);
            SetColAttr(i, cellAttributes);
            break;
        }
//----------------------------------------------------------------------------------

    columnSettings.clear();
    if (attr.size() == 0)
    {   //default settings:
        columnSettings = getDefaultColumnAttributes();
    }
    else
    {
        for (unsigned int i = 0; i < xmlAccess::COLUMN_TYPE_COUNT; ++i)
        {
            xmlAccess::ColumnAttrib newEntry;

            if (i < attr.size())
                newEntry = attr[i];
            else
            {
                newEntry.type     = xmlAccess::FILENAME;
                newEntry.visible  = true;
                newEntry.position = i;
                newEntry.width    = 100;
            }
            columnSettings.push_back(newEntry);
        }

        std::sort(columnSettings.begin(), columnSettings.end(), xmlAccess::sortByType);
        for (unsigned int i = 0; i < xmlAccess::COLUMN_TYPE_COUNT; ++i) //just be sure that each type exists only once
            columnSettings[i].type = xmlAccess::ColumnTypes(i);

        std::sort(columnSettings.begin(), columnSettings.end(), xmlAccess::sortByPositionOnly);
        for (unsigned int i = 0; i < xmlAccess::COLUMN_TYPE_COUNT; ++i) //just be sure that positions are numbered correctly
            columnSettings[i].position = i;
    }

    std::sort(columnSettings.begin(), columnSettings.end(), xmlAccess::sortByPositionAndVisibility);
    std::vector<xmlAccess::ColumnTypes> newPositions;
    for (unsigned int i = 0; i < columnSettings.size() && columnSettings[i].visible; ++i)  //hidden columns are sorted to the end of vector!
        newPositions.push_back(columnSettings[i].type);

    //set column positions
    assert(getGridDataTable());
    getGridDataTable()->setupColumns(newPositions);

    //set column width (set them after setupColumns!)
    for (unsigned int i = 0; i < newPositions.size(); ++i)
        SetColSize(i, columnSettings[i].width);

//--------------------------------------------------------------------------------------------------------
    //set special alignment for column "size"
    for (int i = 0; i < GetNumberCols(); ++i)
        if (getTypeAtPos(i) == xmlAccess::SIZE)
        {
            wxGridCellAttr* cellAttributes = GetOrCreateCellAttr(0, i);
            cellAttributes->SetAlignment(wxALIGN_RIGHT,wxALIGN_CENTRE);
            SetColAttr(i, cellAttributes); //make filesize right justified on grids
            break;
        }

    ClearSelection();
    ForceRefresh();
}


xmlAccess::ColumnTypes CustomGridRim::getTypeAtPos(unsigned pos) const
{
    assert(getGridDataTable());
    return getGridDataTable()->getTypeAtPos(pos);
}


wxString CustomGridRim::getTypeName(xmlAccess::ColumnTypes colType)
{
    switch (colType)
    {
    case xmlAccess::FULL_PATH:
        return _("Full path");
    case xmlAccess::FILENAME:
        return _("Filename");
    case xmlAccess::REL_PATH:
        return _("Relative path");
    case xmlAccess::DIRECTORY:
        return _("Directory");
    case xmlAccess::SIZE:
        return _("Size");
    case xmlAccess::DATE:
        return _("Date");
    }

    return wxEmptyString; //dummy
}


CustomGridTableRim* CustomGridRim::getGridDataTable()
{   //let the non-const call the const version: see Meyers Effective C++
    return const_cast<CustomGridTableRim*>(static_cast<const CustomGridRim*>(this)->getGridDataTable());
}


#ifdef FFS_WIN
void CustomGridRim::enableFileIcons(const bool value)
{
    fileIconsAreEnabled = value;

    if (value)
        SetDefaultRenderer(new GridCellRenderer<true>(loadIconSuccess, getGridDataTable())); //SetDefaultRenderer takes ownership!
    else
        SetDefaultRenderer(new GridCellRenderer<false>(loadIconSuccess, getGridDataTable()));

    Refresh();
}


CustomGridRim::VisibleRowRange CustomGridRim::getVisibleRows()
{
    int dummy  = -1;
    int height = -1;
    GetGridWindow()->GetClientSize(&dummy, &height);

    if (height >= 0)
    {
        int topRowY = -1;
        CalcUnscrolledPosition(0, 0, &dummy, &topRowY);

        if (topRowY >= 0)
        {
            const int topRow    = YToRow(topRowY);
            const int rowCount  = static_cast<int>(ceil(height / static_cast<double>(GetDefaultRowSize()))); // = height / rowHeight rounded up
            const int bottomRow = topRow + rowCount - 1;

            return VisibleRowRange(topRow, bottomRow); //"top" means here top of the screen: => smaller value
        }
    }

    return VisibleRowRange(0, 0);
}


void CustomGridRim::getIconsToBeLoaded(std::vector<Zstring>& newLoad) //loads all (not yet) drawn icons
{
    newLoad.clear();

    if (fileIconsAreEnabled) //don't check too often! give worker thread some time to fetch data
    {
        const CustomGridTableRim* gridDataTable = getGridDataTable();
        const VisibleRowRange rowsOnScreen = getVisibleRows();
        const int totalCols = const_cast<CustomGridTableRim*>(gridDataTable)->GetNumberCols();
        const int totalRows = const_cast<CustomGridTableRim*>(gridDataTable)->GetNumberRows();

        //loop over all visible rows
        const int firstRow = rowsOnScreen.first;
        const int lastRow  = std::min(int(rowsOnScreen.second), totalRows - 1);
        const int rowNo = lastRow - firstRow + 1;

        for (int i = 0; i < rowNo; ++i)
        {
            //alternate when adding rows: first, last, first + 1, last - 1 ...
            const int currentRow = i % 2 == 0 ?
                                   firstRow + i / 2 :
                                   lastRow - (i - 1) / 2;

            LoadSuccess::const_iterator j = loadIconSuccess.find(currentRow);
            if (j != loadIconSuccess.end() && j->second == false) //find failed attempts to load icon
            {
                const Zstring fileName = gridDataTable->getFileName(currentRow);
                if (!fileName.empty())
                {
                    //test if they are already loaded in buffer:
                    if (FreeFileSync::IconBuffer::getInstance().requestIcon(fileName))
                    {
                        //exists in buffer: refresh Row
                        for (int k = 0; k < totalCols; ++k)
                            if (gridDataTable->getTypeAtPos(k) == xmlAccess::FILENAME)
                            {
                                RefreshCell(currentRow, k);
                                break;
                            }
                    }
                    else //not yet in buffer: mark for async. loading
                    {
                        newLoad.push_back(fileName);
                    }
                }
            }
        }
    }
}

//----------------------------------------------------------------------------------------


//update file icons periodically: use SINGLE instance to coordinate left and right grid at once
IconUpdater::IconUpdater(CustomGridLeft* leftGrid, CustomGridRight* rightGrid) :
        m_leftGrid(leftGrid),
        m_rightGrid(rightGrid),
        m_timer(new wxTimer)      //connect timer event for async. icon loading
{
    m_timer->Connect(wxEVT_TIMER, wxEventHandler(IconUpdater::loadIconsAsynchronously), NULL, this);
    m_timer->Start(50); //timer interval
}


void IconUpdater::loadIconsAsynchronously(wxEvent& event) //loads all (not yet) drawn icons
{
    std::vector<Zstring> iconsLeft;
    m_leftGrid->getIconsToBeLoaded(iconsLeft);

    std::vector<Zstring> newLoad;
    m_rightGrid->getIconsToBeLoaded(newLoad);

    globalFunctions::mergeVectors(iconsLeft, newLoad);

    FreeFileSync::IconBuffer::getInstance().setWorkload(newLoad); //attention: newLoad is invalidated after this call!!!

    //event.Skip();
}
#endif

//----------------------------------------------------------------------------------------


CustomGridLeft::CustomGridLeft(wxWindow *parent,
                               wxWindowID id,
                               const wxPoint& pos,
                               const wxSize& size,
                               long style,
                               const wxString& name) :
        CustomGridRim(parent, id, pos, size, style, name),
        gridDataTable(NULL) {}


bool CustomGridLeft::CreateGrid(int numRows, int numCols, wxGrid::wxGridSelectionModes selmode)
{
    //use custom wxGridTableBase class for management of large sets of formatted data.
    //This is done in CreateGrid instead of SetTable method since source code is generated and wxFormbuilder invokes CreatedGrid by default.
    gridDataTable = new CustomGridTableLeft;
    SetTable(gridDataTable, true, wxGrid::wxGridSelectRows);  //give ownership to wxGrid: gridDataTable is deleted automatically in wxGrid destructor

    return true;
}


void CustomGridLeft::setGridDataTable(const GridView* gridDataView)
{
    //set underlying grid data
    assert(gridDataTable);
    gridDataTable->setGridDataTable(gridDataView);
}


const CustomGridTableRim* CustomGridLeft::getGridDataTable() const
{
    return gridDataTable;
}


//this method is called when grid view changes: useful for parallel updating of multiple grids
void CustomGridLeft::alignOtherGrids(CustomGrid* gridLeft, CustomGrid* gridMiddle, CustomGrid* gridRight)
{
    int x = 0;
    int y = 0;
    GetViewStart(&x, &y);
    gridMiddle->Scroll(-1, y); //scroll in y-direction only
    gridRight->Scroll(x, y);
}


//----------------------------------------------------------------------------------------
CustomGridRight::CustomGridRight(wxWindow *parent,
                                 wxWindowID id,
                                 const wxPoint& pos,
                                 const wxSize& size,
                                 long style,
                                 const wxString& name) :
        CustomGridRim(parent, id, pos, size, style, name),
        gridDataTable(NULL) {}


bool CustomGridRight::CreateGrid(int numRows, int numCols, wxGrid::wxGridSelectionModes selmode)
{
    gridDataTable = new CustomGridTableRight;
    SetTable(gridDataTable, true, wxGrid::wxGridSelectRows);  //give ownership to wxGrid: gridDataTable is deleted automatically in wxGrid destructor

    return true;
}


void CustomGridRight::setGridDataTable(const GridView* gridDataView)
{
    //set underlying grid data
    assert(gridDataTable);
    gridDataTable->setGridDataTable(gridDataView);
}


const CustomGridTableRim* CustomGridRight::getGridDataTable() const
{
    return gridDataTable;
}


//this method is called when grid view changes: useful for parallel updating of multiple grids
void CustomGridRight::alignOtherGrids(CustomGrid* gridLeft, CustomGrid* gridMiddle, CustomGrid* gridRight)
{
    int x = 0;
    int y = 0;
    GetViewStart(&x, &y);
    gridLeft->Scroll(x, y);
    gridMiddle->Scroll(-1, y);
}


//----------------------------------------------------------------------------------------
class GridCellRendererMiddle : public wxGridCellStringRenderer
{
public:
    GridCellRendererMiddle(const CustomGridMiddle* middleGrid) : m_gridMiddle(middleGrid) {};

    virtual void Draw(wxGrid& grid,
                      wxGridCellAttr& attr,
                      wxDC& dc,
                      const wxRect& rect,
                      int row, int col,
                      bool isSelected);

private:
    const CustomGridMiddle* const m_gridMiddle;
};


//define new event types
const wxEventType FFS_CHECK_ROWS_EVENT     = wxNewEventType(); //attention! do NOT place in header to keep (generated) id unique!
const wxEventType FFS_SYNC_DIRECTION_EVENT = wxNewEventType();

const int CHECK_BOX_IMAGE = 11; //width of checkbox image
const int CHECK_BOX_WIDTH = CHECK_BOX_IMAGE + 3; //width of first block

// cell:
//  ----------------------------------
// | checkbox | left | middle | right|
//  ----------------------------------


CustomGridMiddle::CustomGridMiddle(wxWindow *parent,
                                   wxWindowID id,
                                   const wxPoint& pos,
                                   const wxSize& size,
                                   long style,
                                   const wxString& name) :
        CustomGrid(parent, id, pos, size, style, name),
        selectionRowBegin(-1),
        selectionPos(BLOCKPOS_CHECK_BOX),
        highlightedRow(-1),
        highlightedPos(BLOCKPOS_CHECK_BOX),
        gridDataTable(NULL)
{
    //connect events for dynamic selection of sync direction
    GetGridWindow()->Connect(wxEVT_MOTION, wxMouseEventHandler(CustomGridMiddle::OnMouseMovement), NULL, this);
    GetGridWindow()->Connect(wxEVT_LEAVE_WINDOW, wxMouseEventHandler(CustomGridMiddle::OnLeaveWindow), NULL, this);
    GetGridWindow()->Connect(wxEVT_LEFT_UP, wxMouseEventHandler(CustomGridMiddle::OnLeftMouseUp), NULL, this);
    GetGridWindow()->Connect(wxEVT_LEFT_DOWN, wxMouseEventHandler(CustomGridMiddle::OnLeftMouseDown), NULL, this);
}


bool CustomGridMiddle::CreateGrid(int numRows, int numCols, wxGrid::wxGridSelectionModes selmode)
{
    gridDataTable = new CustomGridTableMiddle;
    SetTable(gridDataTable, true, wxGrid::wxGridSelectRows);  //give ownership to wxGrid: gridDataTable is deleted automatically in wxGrid destructor

    //display checkboxes (representing bool values) if row is enabled for synchronization
    SetDefaultRenderer(new GridCellRendererMiddle(this)); //SetDefaultRenderer takes ownership!

    return true;
}


#ifdef FFS_WIN //get rid of scrollbars; Windows: overwrite virtual method
void CustomGridMiddle::SetScrollbar(int orientation, int position, int thumbSize, int range, bool refresh)
{
    wxWindow::SetScrollbar(orientation, 0, 0, 0, refresh);
}
#endif


void CustomGridMiddle::setGridDataTable(const GridView* gridDataView) //called once on startup
{
    //set underlying grid data
    assert(gridDataTable);
    gridDataTable->setGridDataTable(gridDataView);

#ifdef FFS_LINUX //get rid of scrollbars; Linux: change policy for GtkScrolledWindow
    GtkWidget* gridWidget = wxWindow::m_widget;
    GtkScrolledWindow* scrolledWindow = GTK_SCROLLED_WINDOW(gridWidget);
    gtk_scrolled_window_set_policy(scrolledWindow, GTK_POLICY_NEVER, GTK_POLICY_NEVER);
#endif
}


void CustomGridMiddle::OnMouseMovement(wxMouseEvent& event)
{
    const int highlightedRowOld = highlightedRow;

    if (selectionRowBegin == -1) //change highlightning only if currently not dragging mouse
    {
        highlightedRow = mousePosToRow(event.GetPosition(), &highlightedPos);
        if (highlightedRow >= 0)
            RefreshCell(highlightedRow, 0);
        if (    highlightedRowOld >= 0 &&
                highlightedRow != highlightedRowOld)
            RefreshCell(highlightedRowOld, 0);
    }

    event.Skip();
}


void CustomGridMiddle::OnLeaveWindow(wxMouseEvent& event)
{
    highlightedRow = -1;
    highlightedPos = BLOCKPOS_CHECK_BOX;
    Refresh();
}


void CustomGridMiddle::OnLeftMouseDown(wxMouseEvent& event)
{
    selectionRowBegin = mousePosToRow(event.GetPosition(), &selectionPos);
    event.Skip();
}


void CustomGridMiddle::OnLeftMouseUp(wxMouseEvent& event)
{
    const int rowEndFiltering = mousePosToRow(event.GetPosition());

    if (0 <= selectionRowBegin && 0 <= rowEndFiltering)
    {
        switch (selectionPos)
        {
        case BLOCKPOS_CHECK_BOX:
        {
            //create a custom event
            FFSCheckRowsEvent evt(selectionRowBegin, rowEndFiltering);
            AddPendingEvent(evt);
        }
        break;
        case BLOCKPOS_LEFT:
        {
            //create a custom event
            FFSSyncDirectionEvent evt(selectionRowBegin, rowEndFiltering, SYNC_DIR_LEFT);
            AddPendingEvent(evt);
        }
        break;
        case BLOCKPOS_MIDDLE:
        {
            //create a custom event
            FFSSyncDirectionEvent evt(selectionRowBegin, rowEndFiltering, SYNC_DIR_NONE);
            AddPendingEvent(evt);
        }
        break;
        case BLOCKPOS_RIGHT:
        {
            //create a custom event
            FFSSyncDirectionEvent evt(selectionRowBegin, rowEndFiltering, SYNC_DIR_RIGHT);
            AddPendingEvent(evt);
        }
        break;
        }
    }
    selectionRowBegin = -1;
    selectionPos = BLOCKPOS_CHECK_BOX;

    ClearSelection();
    event.Skip();
}


int CustomGridMiddle::mousePosToRow(const wxPoint pos, BlockPosition* block)
{
    int row = -1;
    int x = -1;
    int y = -1;
    CalcUnscrolledPosition( pos.x, pos.y, &x, &y );
    if (x >= 0 && y >= 0)
    {
        row = YToRow(y);

        //determine blockposition within cell (optional)
        if (block)
        {
            *block = BLOCKPOS_CHECK_BOX; //default
            if (gridDataTable->syncPreviewIsActive() &&
                    row >= 0)
            {
                // cell:
                //  ----------------------------------
                // | checkbox | left | middle | right|
                //  ----------------------------------

                const wxRect rect = CellToRect(row, 0);
                if (rect.GetWidth() > CHECK_BOX_WIDTH)
                {
                    const double blockWidth = (rect.GetWidth() - CHECK_BOX_WIDTH) / 3.0;
                    if (rect.GetX() + CHECK_BOX_WIDTH <= x && x < rect.GetX() + rect.GetWidth())
                    {
                        if (x - (rect.GetX() + CHECK_BOX_WIDTH) < blockWidth)
                            *block = BLOCKPOS_LEFT;
                        else if (x - (rect.GetX() + CHECK_BOX_WIDTH) < 2 * blockWidth)
                            *block = BLOCKPOS_MIDDLE;
                        else
                            *block = BLOCKPOS_RIGHT;
                    }
                }
            }
        }
    }
    return row;
}


void CustomGridMiddle::enableSyncPreview(bool value)
{
    assert(gridDataTable);
    gridDataTable->enableSyncPreview(value);

    //update legend
    wxString toolTip;

    if (gridDataTable->syncPreviewIsActive()) //synchronization preview
    {
        const wxString header = _("Synchronization Preview");
        toolTip = header + wxT("\n") + wxString().Pad(header.Len(), wxChar('-')) + wxT("\n");
        toolTip += wxString(_("<-	copy to left side\n")) +
                   _("->	copy to right side\n") +
                   wxT(" ")+ _("-	do not copy\n") +
                   _("flash	conflict\n");
    }
    else //compare results view
    {
        const wxString header = _("Comparison Result");
        toolTip = header + wxT("\n") + wxString().Pad(header.Len(), wxChar('-')) + wxT("\n");
        toolTip += wxString(_("<|	file on left side only\n")) +
                   _("|>	file on right side only\n") +
                   _("<<	left file is newer\n") +
                   _(">>	right file is newer\n") +
                   _("!=	files are different\n") +
                   _("==	files are equal\n") +
                   _("flash	conflict\n");
    }
    GetGridColLabelWindow()->SetToolTip(toolTip);
}


void CustomGridMiddle::updateGridSizes()
{
    assert(gridDataTable);
    gridDataTable->updateGridSizes();
}


void GridCellRendererMiddle::Draw(wxGrid& grid,
                                  wxGridCellAttr& attr,
                                  wxDC& dc,
                                  const wxRect& rect,
                                  int row, int col,
                                  bool isSelected)
{
    //retrieve grid data
    const FileCompareLine* const rowData = m_gridMiddle->gridDataTable->getRawData(row);
    if (rowData != NULL) //if valid row...
    {
        if (rect.GetWidth() > CHECK_BOX_WIDTH)
        {
            wxRect rectShrinked(rect);

            //clean first block of rect that will receive image of checkbox
            rectShrinked.SetWidth(CHECK_BOX_WIDTH);
            wxGridCellRenderer::Draw(grid, attr, dc, rectShrinked, row, col, isSelected);

            //print image into first block
            rectShrinked.SetX(rect.GetX() + 1);
            bool selected = rowData->selectedForSynchronization;

            //HIGHLIGHTNING:
            if (    row == m_gridMiddle->highlightedRow &&
                    m_gridMiddle->highlightedPos == CustomGridMiddle::BLOCKPOS_CHECK_BOX)
                selected = !selected;

            if (selected)
                dc.DrawLabel(wxEmptyString, *GlobalResources::getInstance().bitmapCheckBoxTrue, rectShrinked, wxALIGN_LEFT | wxALIGN_CENTER_VERTICAL);
            else
                dc.DrawLabel(wxEmptyString, *GlobalResources::getInstance().bitmapCheckBoxFalse, rectShrinked, wxALIGN_LEFT | wxALIGN_CENTER_VERTICAL);

            //clean remaining block of rect that will receive image of checkbox/directions
            rectShrinked.SetWidth(rect.GetWidth() - CHECK_BOX_WIDTH);
            rectShrinked.SetX(rect.GetX() + CHECK_BOX_WIDTH);
            wxGridCellRenderer::Draw(grid, attr, dc, rectShrinked, row, col, isSelected);

            //print remaining block
            if (m_gridMiddle->gridDataTable->syncPreviewIsActive()) //synchronization preview
            {
                //print sync direction into second block

                //HIGHLIGHTNING:
                if (row == m_gridMiddle->highlightedRow && m_gridMiddle->highlightedPos != CustomGridMiddle::BLOCKPOS_CHECK_BOX)
                    switch (m_gridMiddle->highlightedPos)
                    {
                    case CustomGridMiddle::BLOCKPOS_CHECK_BOX:
                        break;
                    case CustomGridMiddle::BLOCKPOS_LEFT:
                        dc.DrawLabel(wxEmptyString, getSyncOpImage(rowData->cmpResult, true, SYNC_DIR_LEFT), rectShrinked, wxALIGN_LEFT | wxALIGN_CENTER_VERTICAL);
                        break;
                    case CustomGridMiddle::BLOCKPOS_MIDDLE:
                        dc.DrawLabel(wxEmptyString, getSyncOpImage(rowData->cmpResult, true, SYNC_DIR_NONE), rectShrinked, wxALIGN_CENTER | wxALIGN_CENTER_VERTICAL);
                        break;
                    case CustomGridMiddle::BLOCKPOS_RIGHT:
                        dc.DrawLabel(wxEmptyString, getSyncOpImage(rowData->cmpResult, true, SYNC_DIR_RIGHT), rectShrinked, wxALIGN_RIGHT | wxALIGN_CENTER_VERTICAL);
                        break;
                    }
                else //default
                {
                    const wxBitmap& syncOpIcon = getSyncOpImage(rowData->cmpResult, rowData->selectedForSynchronization, rowData->direction);
                    dc.DrawLabel(wxEmptyString, syncOpIcon, rectShrinked, wxALIGN_CENTER | wxALIGN_CENTER_VERTICAL);
                }
            }
            else //comparison results view
            {
                switch (rowData->cmpResult)
                {
                case FILE_LEFT_SIDE_ONLY:
                    dc.DrawLabel(wxEmptyString, *GlobalResources::getInstance().bitmapLeftOnlySmall, rectShrinked, wxALIGN_CENTER | wxALIGN_CENTER_VERTICAL);
                    break;
                case FILE_RIGHT_SIDE_ONLY:
                    dc.DrawLabel(wxEmptyString, *GlobalResources::getInstance().bitmapRightOnlySmall, rectShrinked, wxALIGN_CENTER | wxALIGN_CENTER_VERTICAL);
                    break;
                case FILE_LEFT_NEWER:
                    dc.DrawLabel(wxEmptyString, *GlobalResources::getInstance().bitmapLeftNewerSmall, rectShrinked, wxALIGN_CENTER | wxALIGN_CENTER_VERTICAL);
                    break;
                case FILE_RIGHT_NEWER:
                    dc.DrawLabel(wxEmptyString, *GlobalResources::getInstance().bitmapRightNewerSmall, rectShrinked, wxALIGN_CENTER | wxALIGN_CENTER_VERTICAL);
                    break;
                case FILE_DIFFERENT:
                    dc.DrawLabel(wxEmptyString, *GlobalResources::getInstance().bitmapDifferentSmall, rectShrinked, wxALIGN_CENTER | wxALIGN_CENTER_VERTICAL);
                    break;
                case FILE_EQUAL:
                    dc.DrawLabel(wxEmptyString, *GlobalResources::getInstance().bitmapEqualSmall, rectShrinked, wxALIGN_CENTER | wxALIGN_CENTER_VERTICAL);
                    break;
                case FILE_CONFLICT:
                    dc.DrawLabel(wxEmptyString, *GlobalResources::getInstance().bitmapConflictSmall, rectShrinked, wxALIGN_CENTER | wxALIGN_CENTER_VERTICAL);
                    break;
                }
            }

            return;
        }
    }

    //fallback
    wxGridCellStringRenderer::Draw(grid, attr, dc, rect, row, col, isSelected);
}


//this method is called when grid view changes: useful for parallel updating of multiple grids
void CustomGridMiddle::alignOtherGrids(CustomGrid* gridLeft, CustomGrid* gridMiddle, CustomGrid* gridRight)
{
    int x = 0;
    int y = 0;
    GetViewStart(&x, &y);
    gridLeft->Scroll(-1, y);
    gridRight->Scroll(-1, y);
}


void CustomGridMiddle::DrawColLabel(wxDC& dc, int col)
{
    CustomGrid::DrawColLabel(dc, col);

    const wxRect rect(GetColLeft(col), 0, GetColWidth(col), GetColLabelSize());

    if (gridDataTable->syncPreviewIsActive())
        dc.DrawLabel(wxEmptyString, *GlobalResources::getInstance().bitmapSyncViewSmall, rect, wxALIGN_CENTER_HORIZONTAL | wxALIGN_CENTER_VERTICAL);
    else
        dc.DrawLabel(wxEmptyString, *GlobalResources::getInstance().bitmapCmpViewSmall, rect, wxALIGN_CENTER_HORIZONTAL | wxALIGN_CENTER_VERTICAL);
}

