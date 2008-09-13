#include "customGrid.h"

const unsigned int MinimumRows = 15;

//class containing pure grid data: basically the same as wxGridStringTable, but adds cell formatting
class CustomGridTableBase : public wxGridStringTable
{
public:
    CustomGridTableBase(int numRows, int numCols) : wxGridStringTable(numRows, numCols), gridIdentifier(0), currentUI_ViewPtr(0), lastNrRows(MinimumRows)
    {
        lightBlue = new wxColour(80, 110, 255);
    }

    ~CustomGridTableBase()
    {
        delete lightBlue;
    }


    void setGridDataTable(UI_Grid* currentUI_ViewPtr)
    {
        this->currentUI_ViewPtr = currentUI_ViewPtr;
    }

    void SetGridIdentifier(int id)
    {
        gridIdentifier = id;
    }

//###########################################################################
//grid standard input output methods, redirected directly to UIGrid to improve performance

    virtual int GetNumberRows()
    {
        if (currentUI_ViewPtr)
            return max(currentUI_ViewPtr->size(), MinimumRows);
        return MinimumRows; //grid is initialized with this number of rows
    }

    virtual bool IsEmptyCell( int row, int col )
    {
        return (GetValue(row, col) == wxEmptyString);
    }

    virtual wxString GetValue( int row, int col )
    {
        if (currentUI_ViewPtr)
        {
            if (currentUI_ViewPtr->size() > unsigned(row))
            {
                switch (gridIdentifier)
                {
                case 1:
                    if (4 > col)
                    {
                        switch (col)
                        {
                        case 0:
                            return (*currentUI_ViewPtr)[row].leftFilename;
                        case 1:
                            return (*currentUI_ViewPtr)[row].leftRelativePath;
                        case 2:
                            return (*currentUI_ViewPtr)[row].leftSize;
                        case 3:
                            return (*currentUI_ViewPtr)[row].leftDate;
                        }
                    }
                    break;

                case 2:
                    if (4 > col)
                    {
                        switch (col)
                        {
                        case 0:
                            return (*currentUI_ViewPtr)[row].rightFilename;
                        case 1:
                            return (*currentUI_ViewPtr)[row].rightRelativePath;
                        case 2:
                            return (*currentUI_ViewPtr)[row].rightSize;
                        case 3:
                            return (*currentUI_ViewPtr)[row].rightDate;
                        }
                    }
                    break;

                case 3:
                    if (1 > col)
                    {
                        return (*currentUI_ViewPtr)[row].cmpResult;
                    }
                    break;

                default:
                    break;
                }
            }
        }
        //if data not found in UIgrid table:
        return wxEmptyString;
    }

    virtual void SetValue( int row, int col, const wxString& value )
    {
        assert (false); //should not be used, since values are retrieved directly from currentUI_ViewPtr
    }
    virtual void Clear()
    {
        assert (false); // we don't want to use this, since the visible grid is directly connected to currentUI_ViewPtr}
    }
    virtual bool InsertRows( size_t pos = 0, size_t numRows = 1 )
    {
        assert (false); // we don't want to use this, since the visible grid is directly connected to currentUI_ViewPtr}
        return true;
    }
    virtual bool AppendRows( size_t numRows = 1 )
    {
        assert (false); // we don't want to use this, since the visible grid is directly connected to currentUI_ViewPtr}
        return true;
    }
    virtual bool DeleteRows( size_t pos = 0, size_t numRows = 1 )
    {
        assert (false); // we don't want to use this, since the visible grid is directly connected to currentUI_ViewPtr}
        return true;
    }

    //update dimensions of grid: no need for InsertRows, AppendRows, DeleteRows anymore!!!
    void updateGridSizes()
    {
        if (currentUI_ViewPtr)
        {
            int currentNrRows = GetNumberRows();

            if (lastNrRows < currentNrRows)
            {
                if ( GetView() )
                {
                    wxGridTableMessage msg(this,
                                           wxGRIDTABLE_NOTIFY_ROWS_APPENDED,
                                           currentNrRows - lastNrRows);

                    GetView()->ProcessTableMessage( msg );
                }
            }
            else if (lastNrRows > currentNrRows)
            {
                if ( GetView() )
                {
                    wxGridTableMessage msg(this,
                                           wxGRIDTABLE_NOTIFY_ROWS_DELETED,
                                           0,
                                           lastNrRows - currentNrRows);

                    GetView()->ProcessTableMessage( msg );
                }
            }
            lastNrRows = currentNrRows;
        }
    }
//###########################################################################


    bool markThisRow(int row)
    {
        if (currentUI_ViewPtr)
        {
            if (currentUI_ViewPtr->size() > unsigned(row))
            {
                if ((*currentUI_ViewPtr)[row].cmpResult == constFilteredOut)
                    return true;
            }
        }
        return false;
    }

    wxGridCellAttr* GetAttr(int row, int col, wxGridCellAttr::wxAttrKind  kind)
    {
        wxGridCellAttr* result = wxGridTableBase::GetAttr(row, col, kind);

        //  MARK FILTERED ROWS:
        if (result)
        {   //if kind is not a cell or row attribute, we have to clone the cell attribute, since we don't want to change e.g. all column attribs
            if (result->GetKind() != wxGridCellAttr::Cell && result->GetKind() != wxGridCellAttr::Row)
            {
                wxGridCellAttr* attr = result->Clone();
                result->DecRef();
                result = attr;
            }

            if (markThisRow(row))
                result->SetBackgroundColour(*lightBlue);
            else
                result->SetBackgroundColour(*wxWHITE);
        }
        else
        {
            result = new wxGridCellAttr;
            if (markThisRow(row))
                result->SetBackgroundColour(*lightBlue);
        }

        return result;
    }

private:
    wxColour* lightBlue;
    int       gridIdentifier;
    UI_Grid*  currentUI_ViewPtr; //(fast) access to underlying grid data :)
    int       lastNrRows;
};



//########################################################################################################

CustomGrid::CustomGrid( wxWindow *parent,
                        wxWindowID id,
                        const wxPoint& pos,
                        const wxSize& size,
                        long style,
                        const wxString& name)
        : wxGrid(parent, id, pos, size, style, name),
        scrollbarsEnabled(true),
        m_grid1(0), m_grid2(0), m_grid3(0),
        gridDataTable(0),
        currentSortColumn(-1),
        sortMarker(0) {}

CustomGrid::~CustomGrid() {}

bool CustomGrid::CreateGrid(int numRows, int numCols, wxGrid::wxGridSelectionModes selmode)
{
    //use custom wxGridTableBase class for management of large sets of formatted data.
    //This is done in CreateGrid instead of SetTable method since source code is generated and wxFormbuilder invokes CreatedGrid by default.

    gridDataTable = new CustomGridTableBase(numRows, numCols);
    SetTable(gridDataTable, true, selmode);  //give ownership to CustomGrid: gridDataTable is deleted automatically in CustomGrid destructor
    return true;
}


void CustomGrid::deactivateScrollbars()
{
    scrollbarsEnabled = false;
}


//overwrite virtual method to finally get rid of the scrollbars
void CustomGrid::SetScrollbar(int orientation, int position, int thumbSize, int range, bool refresh)
{
    if (scrollbarsEnabled)
        wxWindow::SetScrollbar(orientation, position, thumbSize, range, refresh);
    else
        wxWindow::SetScrollbar(orientation, 0, 0, 0, refresh);
}


//this method is called when grid view changes: useful for parallel updating of multiple grids
void CustomGrid::DoPrepareDC(wxDC& dc)
{
    wxScrollHelper::DoPrepareDC(dc);

    int x, y = 0;
    if (leadingPanel == 1 && this == m_grid1)   //avoid back coupling
    {
        GetViewStart(&x, &y);
        m_grid2->Scroll(x, y);
        m_grid3->Scroll(-1, y); //scroll in y-direction only
    }
    else if (leadingPanel == 2 && this == m_grid2)   //avoid back coupling
    {
        GetViewStart(&x, &y);
        m_grid1->Scroll(x, y);
        m_grid3->Scroll(-1, y);
    }
    else if (leadingPanel == 3 && this == m_grid3)   //avoid back coupling
    {
        GetViewStart(&x, &y);
        m_grid1->Scroll(-1, y);
        m_grid2->Scroll(-1, y);
    }
}

//these classes will scroll together, hence the name ;)
void CustomGrid::setScrollFriends(CustomGrid* grid1, CustomGrid* grid2, CustomGrid* grid3)
{
    m_grid1 = grid1;
    m_grid2 = grid2;
    m_grid3 = grid3;

    assert(gridDataTable);
    if (this == m_grid1)
        gridDataTable->SetGridIdentifier(1);
    else if (this == m_grid2)
        gridDataTable->SetGridIdentifier(2);
    else if (this == m_grid3)
        gridDataTable->SetGridIdentifier(3);
    else
        assert (false);
}


void CustomGrid::setGridDataTable(UI_Grid* currentUI_ViewPtr)
{
    //set underlying grid data
    assert(gridDataTable);
    gridDataTable->setGridDataTable(currentUI_ViewPtr);
}


void CustomGrid::updateGridSizes()
{
    assert(gridDataTable);
    gridDataTable->updateGridSizes();
}


void CustomGrid::setSortMarker(const int sortColumn, const wxBitmap* bitmap)
{
    currentSortColumn = sortColumn;
    sortMarker        = bitmap;
}


void CustomGrid::DrawColLabel( wxDC& dc, int col )
{
    assert(0 <= col && col < 4);

    wxGrid::DrawColLabel(dc, col);

    if (col == currentSortColumn)
    {
        dc.DrawBitmap(*sortMarker, GetColRight(col) - 16 - 2, 2, true); //respect 2-pixel border
    }
}
