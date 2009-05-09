#include "customGrid.h"
#include "globalFunctions.h"
#include "resources.h"
#include <wx/dc.h>
#include "../algorithm.h"
#include "resources.h"
#include <typeinfo>
#include "../ui/gridView.h"

#ifdef FFS_WIN
#include <wx/icon.h>
#include <wx/msw/wrapwin.h> //includes "windows.h"

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
            COLOR_BLUE(       80,  110, 255),
            COLOR_GREY(       212, 208, 200),
            COLOR_LIGHT_RED(  235, 57,  61),
            COLOR_LIGHT_BLUE( 63,  206, 233),
            COLOR_LIGHT_GREEN(54,  218, 2),
            gridDataView(NULL),
            lastNrRows(initialRows),
            lastNrCols(initialCols) {}


    virtual ~CustomGridTable() {}


    void setGridDataTable(GridView* gridDataView)
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
    const wxColour COLOR_LIGHT_RED;
    const wxColour COLOR_LIGHT_BLUE;
    const wxColour COLOR_LIGHT_GREEN;

    const GridView* gridDataView; //(very fast) access to underlying grid data :)

private:
    virtual const wxColour& getRowColor(int row) = 0; //rows that are filtered out are shown in different color

    int       lastNrRows;
    int       lastNrCols;
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
                case xmlAccess::FULL_NAME:
                    return gridLine->fileDescrLeft.fullName.c_str();
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
                case xmlAccess::FULL_NAME:
                    return gridLine->fileDescrLeft.fullName.c_str();
                case xmlAccess::FILENAME: //filename
                    return wxString(gridLine->fileDescrLeft.relativeName.c_str()).AfterLast(GlobalResources::FILE_NAME_SEPARATOR);
                case xmlAccess::REL_PATH: //relative path
                    return wxString(gridLine->fileDescrLeft.relativeName.c_str()).BeforeLast(GlobalResources::FILE_NAME_SEPARATOR);
                case xmlAccess::DIRECTORY:
                    return gridDataView->getFolderPair(row).leftDirectory.c_str();
                case xmlAccess::SIZE: //file size
                    return globalFunctions::includeNumberSeparator(gridLine->fileDescrLeft.fileSize.ToString());
                case xmlAccess::DATE: //date
                    return FreeFileSync::utcTimeToLocalString(gridLine->fileDescrLeft.lastWriteTimeRaw);
                }
            }
        }
        //if data is not found:
        return wxEmptyString;
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
                case xmlAccess::FULL_NAME:
                    return gridLine->fileDescrRight.fullName.c_str();
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
                case xmlAccess::FULL_NAME:
                    return gridLine->fileDescrRight.fullName.c_str();
                case xmlAccess::FILENAME: //filename
                    return wxString(gridLine->fileDescrRight.relativeName.c_str()).AfterLast(GlobalResources::FILE_NAME_SEPARATOR);
                case xmlAccess::REL_PATH: //relative path
                    return wxString(gridLine->fileDescrRight.relativeName.c_str()).BeforeLast(GlobalResources::FILE_NAME_SEPARATOR);
                case xmlAccess::DIRECTORY:
                    return gridDataView->getFolderPair(row).rightDirectory.c_str();
                case xmlAccess::SIZE: //file size
                    return globalFunctions::includeNumberSeparator(gridLine->fileDescrRight.fileSize.ToString());
                case xmlAccess::DATE: //date
                    return FreeFileSync::utcTimeToLocalString(gridLine->fileDescrRight.lastWriteTimeRaw);
                }
            }
        }
        //if data is not found:
        return wxEmptyString;
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
    CustomGridTableMiddle() : CustomGridTable(0, GetNumberCols()) {} //attention: static binding to virtual GetNumberCols() in a Constructor!

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
        const FileCompareLine* gridLine = getRawData(row);
        if (gridLine)
        {
            switch (gridLine->cmpResult)
            {
            case FILE_LEFT_SIDE_ONLY:
                return wxT("<|");
            case FILE_RIGHT_SIDE_ONLY:
                return wxT("|>");
            case FILE_RIGHT_NEWER:
                return wxT(">>");
            case FILE_LEFT_NEWER:
                return wxT("<<");
            case FILE_DIFFERENT:
                return wxT("!=");
            case FILE_EQUAL:
                return wxT("==");
            default:
                assert (false);
                return wxEmptyString;
            }
        }
        //if data is not found:
        return wxEmptyString;
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
            else
                switch (gridLine->cmpResult)
                {
                case FILE_LEFT_SIDE_ONLY:
                case FILE_RIGHT_SIDE_ONLY:
                    return COLOR_LIGHT_GREEN;
                case FILE_LEFT_NEWER:
                case FILE_RIGHT_NEWER:
                    return COLOR_LIGHT_BLUE;
                case FILE_DIFFERENT:
                    return COLOR_LIGHT_RED;
                default:
                    return *wxWHITE;
                }
        }
        return *wxWHITE;
    }
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
        currentSortColumn(-1),
        sortMarker(NULL)
{
    //set color of selections
    wxColour darkBlue(40, 35, 140);
    SetSelectionBackground(darkBlue);
    SetSelectionForeground(*wxWHITE);

    //enhance grid functionality; identify leading grid by keyboard input or scroll action
    Connect(wxEVT_KEY_DOWN,               wxEventHandler(CustomGrid::onGridAccess), NULL, this);
    Connect(wxEVT_SCROLLWIN_TOP,          wxEventHandler(CustomGrid::onGridAccess), NULL, this);
    Connect(wxEVT_SCROLLWIN_BOTTOM,       wxEventHandler(CustomGrid::onGridAccess), NULL, this);
    Connect(wxEVT_SCROLLWIN_LINEUP,       wxEventHandler(CustomGrid::onGridAccess), NULL, this);
    Connect(wxEVT_SCROLLWIN_LINEDOWN,     wxEventHandler(CustomGrid::onGridAccess), NULL, this);
    Connect(wxEVT_SCROLLWIN_PAGEUP,       wxEventHandler(CustomGrid::onGridAccess), NULL, this);
    Connect(wxEVT_SCROLLWIN_PAGEDOWN,     wxEventHandler(CustomGrid::onGridAccess), NULL, this);
    Connect(wxEVT_SCROLLWIN_THUMBTRACK,   wxEventHandler(CustomGrid::onGridAccess), NULL, this);
    Connect(wxEVT_SCROLLWIN_THUMBRELEASE, wxEventHandler(CustomGrid::onGridAccess), NULL, this);
    Connect(wxEVT_GRID_LABEL_LEFT_CLICK,  wxEventHandler(CustomGrid::onGridAccess), NULL, this);
    GetGridWindow()->Connect(wxEVT_LEFT_DOWN, wxEventHandler(CustomGrid::onGridAccess), NULL, this);

    GetGridWindow()->Connect(wxEVT_ENTER_WINDOW, wxEventHandler(CustomGrid::adjustGridHeights), NULL, this);
}


bool CustomGrid::isLeadGrid() const
{
    return isLeading;
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

        default:
            return false;
        }
    }
    catch (std::bad_cast&) {}

    return false;
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


void CustomGrid::onGridAccess(wxEvent& event)
{
    if (!isLeading)
    {
        isLeading = true;

        //notify other grids of new user focus
        if (m_gridLeft != this)
            m_gridLeft->isLeading = false;
        if (m_gridMiddle != this)
            m_gridMiddle->isLeading = false;
        if (m_gridRight != this)
            m_gridRight->isLeading = false;

        wxGrid::SetFocus();
    }

    if (gridsShouldBeCleared(event))
    {
        m_gridLeft->ClearSelection();
        m_gridRight->ClearSelection();
    }

    //support for additional short-cuts
    additionalGridCommands(event, this); //event.Skip is handled here!
}


//workaround: ensure that all grids are properly aligned: add some extra window space to grids that have no horizontal scrollbar
void CustomGrid::adjustGridHeights(wxEvent& event)
{
    if (m_gridLeft && m_gridRight && m_gridMiddle)
    {
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
}


void CustomGrid::setSortMarker(const int sortColumn, const wxBitmap* bitmap)
{
    currentSortColumn = sortColumn;
    sortMarker        = bitmap;
}


void CustomGrid::DrawColLabel(wxDC& dc, int col)
{
    wxGrid::DrawColLabel(dc, col);

    if (col == currentSortColumn)
        dc.DrawBitmap(*sortMarker, GetColRight(col) - 16 - 2, 2, true); //respect 2-pixel border
}

//############################################################################################
//CustomGrid specializations

template <bool leftSide, bool showFileIcons>
class GridCellRenderer : public wxGridCellStringRenderer
{
public:
    GridCellRenderer(CustomGridTableRim* gridDataTable) : m_gridDataTable(gridDataTable) {};


    virtual void Draw(wxGrid& grid,
                      wxGridCellAttr& attr,
                      wxDC& dc,
                      const wxRect& rect,
                      int row, int col,
                      bool isSelected)
    {
#ifdef FFS_WIN
        //############## show windows explorer file icons ######################

        if (showFileIcons) //evaluate at compile time
        {
            const int ICON_SIZE = 16; //size in pixel

            if (    m_gridDataTable->getTypeAtPos(col) == xmlAccess::FILENAME &&
                    rect.GetWidth() >= ICON_SIZE)
            {
                //retrieve grid data
                const FileCompareLine* rowData = m_gridDataTable->getRawData(row);
                if (rowData) //valid row
                {
                    const DefaultChar* filename;
                    if (leftSide) //evaluate at compile time
                        filename = rowData->fileDescrLeft.fullName.c_str();
                    else
                        filename = rowData->fileDescrRight.fullName.c_str();

                    if (*filename != DefaultChar(0)) //test if filename is empty
                    {
                        // Get the file icon.
                        SHFILEINFO fileInfo;
                        fileInfo.hIcon = 0; //initialize hIcon

                        if (SHGetFileInfo(filename, //NOTE: CoInitializeEx()/CoUninitialize() implicitly called by wxWidgets on program startup!
                                          0,
                                          &fileInfo,
                                          sizeof(fileInfo),
                                          SHGFI_ICON | SHGFI_SMALLICON))
                        {
                            //clear area where icon will be placed
                            wxRect rectShrinked(rect);
                            rectShrinked.SetWidth(ICON_SIZE + 2); //add 2 pixel border
                            wxGridCellRenderer::Draw(grid, attr, dc, rectShrinked, row, col, isSelected);

                            //draw icon
                            if (fileInfo.hIcon != 0) //fix for weird error: SHGetFileInfo() might return successfully WITHOUT filling fileInfo.hIcon!!
                            {                        //bug report: https://sourceforge.net/tracker/?func=detail&aid=2768004&group_id=234430&atid=1093080
                                wxIcon icon;
                                icon.SetHICON(static_cast<WXHICON>(fileInfo.hIcon));
                                icon.SetSize(ICON_SIZE, ICON_SIZE);

                                dc.DrawIcon(icon, rectShrinked.GetX() + 2, rectShrinked.GetY());

                                if (!DestroyIcon(fileInfo.hIcon))
                                    throw RuntimeException(wxString(wxT("Error deallocating Icon handle!\n\n")) + FreeFileSync::getLastErrorFormatted());
                            }

                            //draw rest
                            rectShrinked.SetWidth(rect.GetWidth() - ICON_SIZE - 2);
                            rectShrinked.SetX(rect.GetX() + ICON_SIZE + 2);
                            wxGridCellStringRenderer::Draw(grid, attr, dc, rectShrinked, row, col, isSelected);
                            return;
                        }
                    }
                }
            }
        }
        //default
        wxGridCellStringRenderer::Draw(grid, attr, dc, rect, row, col, isSelected);

#elif defined FFS_LINUX
        wxGridCellStringRenderer::Draw(grid, attr, dc, rect, row, col, isSelected);
#endif
    }

private:
    const CustomGridTableRim* const m_gridDataTable;
};



void CustomGridRim::updateGridSizes()
{
    assert(gridDataTable);
    gridDataTable->updateGridSizes();
}


xmlAccess::ColumnAttributes CustomGridRim::getDefaultColumnAttributes()
{
    xmlAccess::ColumnAttributes defaultColumnSettings;

    xmlAccess::ColumnAttrib newEntry;
    newEntry.type     = xmlAccess::FULL_NAME;
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
    assert(gridDataTable);
    gridDataTable->setupColumns(newPositions);

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
    assert(gridDataTable);
    return gridDataTable->getTypeAtPos(pos);
}


wxString CustomGridRim::getTypeName(xmlAccess::ColumnTypes colType)
{
    switch (colType)
    {
    case xmlAccess::FULL_NAME:
        return _("Full name");
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
    default:
        return wxEmptyString;
    }
}


//----------------------------------------------------------------------------------------
CustomGridLeft::CustomGridLeft(wxWindow *parent,
                               wxWindowID id,
                               const wxPoint& pos,
                               const wxSize& size,
                               long style,
                               const wxString& name) :
        CustomGridRim(parent, id, pos, size, style, name) {}


bool CustomGridLeft::CreateGrid(int numRows, int numCols, wxGrid::wxGridSelectionModes selmode)
{
    //use custom wxGridTableBase class for management of large sets of formatted data.
    //This is done in CreateGrid instead of SetTable method since source code is generated and wxFormbuilder invokes CreatedGrid by default.
    gridDataTable = new CustomGridTableLeft;
    SetTable(gridDataTable, true, wxGrid::wxGridSelectRows);  //give ownership to wxGrid: gridDataTable is deleted automatically in wxGrid destructor

    return true;
}


void CustomGridLeft::initSettings(const bool showFileIcons,
                                  CustomGrid* gridLeft,
                                  CustomGrid* gridRight,
                                  CustomGrid* gridMiddle,
                                  GridView* gridDataView)
{
    //these grids will scroll together
    m_gridLeft   = gridLeft;
    m_gridRight  = gridRight;
    m_gridMiddle = gridMiddle;

    //set underlying grid data
    assert(gridDataTable);
    gridDataTable->setGridDataTable(gridDataView);

    if (showFileIcons)
        SetDefaultRenderer(new GridCellRenderer<true, true>(gridDataTable)); //SetDefaultRenderer takes ownership!
    else
        SetDefaultRenderer(new GridCellRenderer<true, false>(gridDataTable));
}


//this method is called when grid view changes: useful for parallel updating of multiple grids
void CustomGridLeft::DoPrepareDC(wxDC& dc)
{
    wxScrollHelper::DoPrepareDC(dc);

    int x, y = 0;
    if (isLeadGrid())  //avoid back coupling
    {
        GetViewStart(&x, &y);
        m_gridMiddle->Scroll(-1, y); //scroll in y-direction only
        m_gridRight->Scroll(x, y);
    }
}


//----------------------------------------------------------------------------------------
CustomGridRight::CustomGridRight(wxWindow *parent,
                                 wxWindowID id,
                                 const wxPoint& pos,
                                 const wxSize& size,
                                 long style,
                                 const wxString& name) :
        CustomGridRim(parent, id, pos, size, style, name) {}


bool CustomGridRight::CreateGrid(int numRows, int numCols, wxGrid::wxGridSelectionModes selmode)
{
    gridDataTable = new CustomGridTableRight;
    SetTable(gridDataTable, true, wxGrid::wxGridSelectRows);  //give ownership to wxGrid: gridDataTable is deleted automatically in wxGrid destructor

    return true;
}


void CustomGridRight::initSettings(const bool showFileIcons,
                                   CustomGrid* gridLeft,
                                   CustomGrid* gridRight,
                                   CustomGrid* gridMiddle,
                                   GridView* gridDataView)
{
    //these grids will scroll together
    m_gridLeft   = gridLeft;
    m_gridRight  = gridRight;
    m_gridMiddle = gridMiddle;

    //set underlying grid data
    assert(gridDataTable);
    gridDataTable->setGridDataTable(gridDataView);

    if (showFileIcons)
        SetDefaultRenderer(new GridCellRenderer<false, true>(gridDataTable)); //SetDefaultRenderer takes ownership!
    else
        SetDefaultRenderer(new GridCellRenderer<false, false>(gridDataTable));
}


//this method is called when grid view changes: useful for parallel updating of multiple grids
void CustomGridRight::DoPrepareDC(wxDC& dc)
{
    wxScrollHelper::DoPrepareDC(dc);

    int x, y = 0;
    if (isLeadGrid())  //avoid back coupling
    {
        GetViewStart(&x, &y);
        m_gridLeft->Scroll(x, y);
        m_gridMiddle->Scroll(-1, y);
    }
}


//----------------------------------------------------------------------------------------
CustomGridMiddle::CustomGridMiddle(wxWindow *parent,
                                   wxWindowID id,
                                   const wxPoint& pos,
                                   const wxSize& size,
                                   long style,
                                   const wxString& name) :
        CustomGrid(parent, id, pos, size, style, name),
        gridDataTable(NULL)
{
    const wxString header = _("Legend");
    wxString toolTip = header + wxT("\n") +
                       wxString().Pad(header.Len(), wxChar('-')) + wxT("\n") +
                       _("<|	file on left side only\n") +
                       _("|>	file on right side only\n") +
                       _("<<	left file is newer\n") +
                       _(">>	right file is newer\n") +
                       _("!=	files are different\n") +
                       _("==	files are equal\n\n");
    GetGridWindow()->SetToolTip(toolTip);
}


void CustomGridMiddle::initSettings(CustomGrid* gridLeft,
                                    CustomGrid* gridRight,
                                    CustomGrid* gridMiddle,
                                    FreeFileSync::GridView* gridDataView)
{
    //these grids will scroll together
    m_gridLeft   = gridLeft;
    m_gridRight  = gridRight;
    m_gridMiddle = gridMiddle;

    //set underlying grid data
    assert(gridDataTable);
    gridDataTable->setGridDataTable(gridDataView);

#ifdef FFS_LINUX //get rid of scrollbars; Linux: change policy for GtkScrolledWindow
    GtkWidget* gridWidget = wxWindow::m_widget;
    GtkScrolledWindow* scrolledWindow = GTK_SCROLLED_WINDOW(gridWidget);
    gtk_scrolled_window_set_policy(scrolledWindow, GTK_POLICY_NEVER, GTK_POLICY_NEVER);
#endif
}


#ifdef FFS_WIN //get rid of scrollbars; Windows: overwrite virtual method
void CustomGridMiddle::SetScrollbar(int orientation, int position, int thumbSize, int range, bool refresh)
{
    wxWindow::SetScrollbar(orientation, 0, 0, 0, refresh);
}
#endif

void CustomGridMiddle::updateGridSizes()
{
    assert(gridDataTable);
    gridDataTable->updateGridSizes();
}


class GridCellRendererMiddle : public wxGridCellStringRenderer
{
public:
    GridCellRendererMiddle(CustomGridTable* gridDataTable) : m_gridDataTable(gridDataTable) {};


    virtual void Draw(wxGrid& grid,
                      wxGridCellAttr& attr,
                      wxDC& dc,
                      const wxRect& rect,
                      int row, int col,
                      bool isSelected)
    {
        //retrieve grid data
        const FileCompareLine* rowData = m_gridDataTable->getRawData(row);
        if (!rowData) //no valid row
        {
            wxGridCellStringRenderer::Draw(grid, attr, dc, rect, row, col, isSelected);
            return;
        }

        const int shift = std::min(11 + 3, rect.GetWidth()); //11 is width of checkbox image

        wxRect rectShrinked(rect);

        //clean first block of rect that will receive image of checkbox
        rectShrinked.SetWidth(shift);
        wxGridCellRenderer::Draw(grid, attr, dc, rectShrinked, row, col, isSelected);

        //print image into first block
        rectShrinked.SetX(rect.GetX() + 1);
        if (rowData->selectedForSynchronization)
            dc.DrawLabel(wxEmptyString, *globalResource.bitmapCheckBoxTrue, rectShrinked, wxALIGN_LEFT | wxALIGN_CENTER_VERTICAL);
        else
            dc.DrawLabel(wxEmptyString, *globalResource.bitmapCheckBoxFalse, rectShrinked, wxALIGN_LEFT | wxALIGN_CENTER_VERTICAL);

        //print second block (default): display compare result
        rectShrinked.SetWidth(rect.GetWidth() - shift);
        rectShrinked.SetX(rect.GetX() + shift);
        wxGridCellStringRenderer::Draw(grid, attr, dc, rectShrinked, row, col, isSelected);
    }

private:
    const CustomGridTable* const m_gridDataTable;
};


bool CustomGridMiddle::CreateGrid(int numRows, int numCols, wxGrid::wxGridSelectionModes selmode)
{
    gridDataTable = new CustomGridTableMiddle;
    SetTable(gridDataTable, true, wxGrid::wxGridSelectRows);  //give ownership to wxGrid: gridDataTable is deleted automatically in wxGrid destructor

    //display checkboxes (representing bool values) if row is enabled for synchronization
    SetDefaultRenderer(new GridCellRendererMiddle(gridDataTable)); //SetDefaultRenderer takes ownership!

    return true;
}


//this method is called when grid view changes: useful for parallel updating of multiple grids
void CustomGridMiddle::DoPrepareDC(wxDC& dc)
{
    wxScrollHelper::DoPrepareDC(dc);

    int x, y = 0;
    if (isLeadGrid())  //avoid back coupling
    {
        GetViewStart(&x, &y);
        m_gridLeft->Scroll(-1, y);
        m_gridRight->Scroll(-1, y);
    }
}

