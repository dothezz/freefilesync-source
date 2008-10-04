#include "customGrid.h"
#include "globalFunctions.h"
#include "resources.h"
#include <wx/dc.h>

const unsigned int MinimumRows = 15;

//class containing pure grid data: basically the same as wxGridStringTable, but adds cell formatting
class CustomGridTableBase : public wxGridStringTable
{
public:
    CustomGridTableBase(int numRows, int numCols) :
            wxGridStringTable(numRows, numCols),
            lightBlue(80, 110, 255),
            gridIdentifier(0),
            gridRefUI(0),
            gridData(0),
            lastNrRows(MinimumRows) {}

    ~CustomGridTableBase() {}


    void setGridDataTable(GridView* gridRefUI, FileCompareResult* gridData)
    {
        this->gridRefUI = gridRefUI;
        this->gridData  = gridData;
    }

    void SetGridIdentifier(int id)
    {
        gridIdentifier = id;
    }

//###########################################################################
//grid standard input output methods, redirected directly to gridData to improve performance

    virtual int GetNumberRows()
    {
        if (gridRefUI)
            return max(gridRefUI->size(), MinimumRows);
        return MinimumRows; //grid is initialized with this number of rows
    }

    virtual bool IsEmptyCell( int row, int col )
    {
        return (GetValue(row, col) == wxEmptyString);
    }


    inline
    wxString evaluateCmpResult(const CompareFilesResult result, const bool selectedForSynchronization)
    {
        if (selectedForSynchronization)
            switch (result)
            {
            case fileOnLeftSideOnly:
                return "<|";
                break;
            case fileOnRightSideOnly:
                return "|>";
                break;
            case rightFileNewer:
                return ">>";
                break;
            case leftFileNewer:
                return "<<";
                break;
            case filesDifferent:
                return "!=";
                break;
            case filesEqual:
                return "==";
                break;
            default:
                assert (false);
                return wxEmptyString;
            }
        else return "(-)";
    }


    virtual wxString GetValue(int row, int col)
    {
        if (gridRefUI)
        {
            if (unsigned(row) < gridRefUI->size())
            {
                const FileCompareLine& gridLine = (*gridData)[(*gridRefUI)[row]];
                wxString fileSize; //tmp string

                switch (gridIdentifier)
                {
                case 1:
                    if (col < 4)
                    {
                        if (gridLine.fileDescrLeft.objType == isDirectory)
                        {
                            switch (col)
                            {
                            case 0: //filename
                                return wxEmptyString;
                            case 1: //relative path
                                return gridLine.fileDescrLeft.relFilename;
                            case 2: //file size
                                return _("<Directory>");
                            case 3: //date
                                return gridLine.fileDescrLeft.lastWriteTime;
                            }
                        }
                        else if (gridLine.fileDescrLeft.objType == isFile)
                        {
                            switch (col)
                            {
                            case 0: //filename
                                return gridLine.fileDescrLeft.relFilename.AfterLast(GlobalResources::fileNameSeparator);
                            case 1: //relative path
                                return gridLine.fileDescrLeft.relFilename.BeforeLast(GlobalResources::fileNameSeparator);
                            case 2: //file size
                                return globalFunctions::includeNumberSeparator(fileSize = gridLine.fileDescrLeft.fileSize.ToString());
                            case 3: //date
                                return gridLine.fileDescrLeft.lastWriteTime;
                            }
                        }
                    }
                    break;

                case 2:
                    if (col < 4)
                    {
                        if (gridLine.fileDescrRight.objType == isDirectory)
                        {
                            switch (col)
                            {
                            case 0: //filename
                                return wxEmptyString;
                            case 1: //relative path
                                return gridLine.fileDescrRight.relFilename;
                            case 2: //file size
                                return _("<Directory>");
                            case 3: //date
                                return gridLine.fileDescrRight.lastWriteTime;
                            }
                        }
                        else if (gridLine.fileDescrRight.objType == isFile)
                        {
                            switch (col)
                            {
                            case 0: //filename
                                return gridLine.fileDescrRight.relFilename.AfterLast(GlobalResources::fileNameSeparator);
                            case 1: //relative path
                                return gridLine.fileDescrRight.relFilename.BeforeLast(GlobalResources::fileNameSeparator);
                            case 2: //file size
                                return globalFunctions::includeNumberSeparator(fileSize = gridLine.fileDescrRight.fileSize.ToString());
                            case 3: //date
                                return gridLine.fileDescrRight.lastWriteTime;
                            }
                        }
                    }
                    break;

                case 3:
                    if (col < 1)
                    {
                        return evaluateCmpResult(gridLine.cmpResult, gridLine.selectedForSynchronization);;
                    }
                    break;

                default:
                    break;
                }
            }
        }
        //if data is not found:
        return wxEmptyString;
    }

    virtual void SetValue( int row, int col, const wxString& value )
    {
        assert (false); //should not be used, since values are retrieved directly from gridRefUI
    }
    virtual void Clear()
    {
        assert (false); // we don't want to use this, since the visible grid is directly connected to gridRefUI}
    }
    virtual bool InsertRows( size_t pos = 0, size_t numRows = 1 )
    {
        assert (false); // we don't want to use this, since the visible grid is directly connected to gridRefUI}
        return true;
    }
    virtual bool AppendRows( size_t numRows = 1 )
    {
        assert (false); // we don't want to use this, since the visible grid is directly connected to gridRefUI}
        return true;
    }
    virtual bool DeleteRows( size_t pos = 0, size_t numRows = 1 )
    {
        assert (false); // we don't want to use this, since the visible grid is directly connected to gridRefUI}
        return true;
    }

    //update dimensions of grid: no need for InsertRows, AppendRows, DeleteRows anymore!!!
    void updateGridSizes()
    {
        if (gridRefUI)
        {
            int currentNrRows = GetNumberRows();

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
        }
    }
//###########################################################################


    bool markThisRow(int row) //rows that are filtered out are shown in different color
    {
        if (gridRefUI)
        {
            if (unsigned(row) < gridRefUI->size())
            {
                if (!(*gridData)[(*gridRefUI)[row]].selectedForSynchronization)
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
                result->SetBackgroundColour(lightBlue);
            else
                result->SetBackgroundColour(*wxWHITE);
        }
        else
        {
            result = new wxGridCellAttr;
            if (markThisRow(row))
                result->SetBackgroundColour(lightBlue);
        }

        return result;
    }

private:
    wxColour  lightBlue;
    int       gridIdentifier;
    GridView* gridRefUI; //(very fast) access to underlying grid data :)
    FileCompareResult* gridData;
    int       lastNrRows;
};



//########################################################################################################

CustomGrid::CustomGrid(wxWindow *parent,
                       wxWindowID id,
                       const wxPoint& pos,
                       const wxSize& size,
                       long style,
                       const wxString& name) :
        wxGrid(parent, id, pos, size, style, name),
        scrollbarsEnabled(true),
        m_grid1(0), m_grid2(0), m_grid3(0),
        gridDataTable(0),
        currentSortColumn(-1),
        sortMarker(0)
{}


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


//ensure that all grids are properly aligned: add some extra window space to grids that have no horizontal scrollbar
void CustomGrid::adjustGridHeights() //m_grid1, m_grid2, m_grid3 are not NULL in this context
{
    int y1 = 0;
    int y2 = 0;
    int y3 = 0;
    int dummy = 0;

    m_grid1->GetViewStart(&dummy, &y1);
    m_grid2->GetViewStart(&dummy, &y2);
    m_grid3->GetViewStart(&dummy, &y3);

    if (y1 != y2 || y2 != y3)
    {
        int yMax = max(y1, max(y2, y3));

        if (leadingPanel == 1)  //do not handle case (y1 == yMax) here!!! Avoid back coupling!
            m_grid1->SetMargins(0, 0);
        else if (y1 < yMax)
            m_grid1->SetMargins(0, 50);

        if (leadingPanel == 2)
            m_grid2->SetMargins(0, 0);
        else if (y2 < yMax)
            m_grid2->SetMargins(0, 50);

        if (leadingPanel == 3)
            m_grid3->SetMargins(0, 0);
        else if (y3 < yMax)
            m_grid3->SetMargins(0, 50);

        m_grid1->ForceRefresh();
        m_grid2->ForceRefresh();
        m_grid3->ForceRefresh();
    }
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
        adjustGridHeights(); //keep here to ensure m_grid1, m_grid2, m_grid3 != NULL
    }
    else if (leadingPanel == 2 && this == m_grid2)   //avoid back coupling
    {
        GetViewStart(&x, &y);
        m_grid1->Scroll(x, y);
        m_grid3->Scroll(-1, y);
        adjustGridHeights(); //keep here to ensure m_grid1, m_grid2, m_grid3 != NULL
    }
    else if (leadingPanel == 3 && this == m_grid3)   //avoid back coupling
    {
        GetViewStart(&x, &y);
        m_grid1->Scroll(-1, y);
        m_grid2->Scroll(-1, y);
        adjustGridHeights(); //keep here to ensure m_grid1, m_grid2, m_grid3 != NULL
    }
}


//these classes will scroll together, hence the name ;)
void CustomGrid::setScrollFriends(CustomGrid* grid1, CustomGrid* grid2, CustomGrid* grid3)
{
    assert(grid1);
    assert(grid2);
    assert(grid3);

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


void CustomGrid::setGridDataTable(GridView* gridRefUI, FileCompareResult* gridData)
{
    //set underlying grid data
    assert(gridDataTable);
    gridDataTable->setGridDataTable(gridRefUI, gridData);
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


void CustomGrid::DrawColLabel(wxDC& dc, int col)
{
    assert(0 <= col && col < 4);

    wxGrid::DrawColLabel(dc, col);

    if (col == currentSortColumn)
    {
        dc.DrawBitmap(*sortMarker, GetColRight(col) - 16 - 2, 2, true); //respect 2-pixel border
    }
}
