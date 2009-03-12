#include "customGrid.h"
#include "globalFunctions.h"
#include "resources.h"
#include <wx/dc.h>
#include "../algorithm.h"
#include "resources.h"


const unsigned int MIN_ROW_COUNT = 15;

//class containing pure grid data: basically the same as wxGridStringTable, but adds cell formatting
class CustomGridTable : public wxGridTableBase
{
public:
    CustomGridTable() :
            wxGridTableBase(),
            blue(80, 110, 255),
            grey(212, 208, 200),
            lightRed(235, 57, 61),
            lightBlue(63, 206, 233),
            lightGreen(54, 218, 2),
            gridRefUI(NULL),
            gridData(NULL),
            lastNrRows(0),
            lastNrCols(0) {}


    virtual ~CustomGridTable() {}


    void setGridDataTable(GridView* gridRefUI, FileCompareResult* gridData)
    {
        this->gridRefUI = gridRefUI;
        this->gridData  = gridData;
    }


//###########################################################################
//grid standard input output methods, redirected directly to gridData to improve performance

    virtual int GetNumberRows()
    {
        if (gridRefUI)
            return std::max(gridRefUI->size(), MIN_ROW_COUNT);
        else
            return 0; //grid is initialized with zero number of rows
    }


    virtual int GetNumberCols() //virtual used by middle grid!
    {
        return columnPositions.size();
    }


    virtual bool IsEmptyCell( int row, int col )
    {
        return (GetValue(row, col) == wxEmptyString);
    }


    virtual wxString GetValue(int row, int col) = 0;


    virtual void SetValue(int row, int col, const wxString& value)
    {
        assert (false); //should not be used, since values are retrieved directly from gridRefUI
    }

    virtual void Clear()
    {
        assert (false); // we don't want to use this, since the visible grid is directly connected to gridRefUI}
    }

    virtual bool InsertRows(size_t pos = 0, size_t numRows = 1)
    {
        assert (false); // we don't want to use this, since the visible grid is directly connected to gridRefUI}
        return true;
    }

    virtual bool AppendRows(size_t numRows = 1)
    {
        assert (false); // we don't want to use this, since the visible grid is directly connected to gridRefUI}
        return true;
    }

    virtual bool DeleteRows(size_t pos = 0, size_t numRows = 1)
    {
        assert (false); // we don't want to use this, since the visible grid is directly connected to gridRefUI}
        return true;
    }

    //update dimensions of grid: no need for InsertRows, AppendRows, DeleteRows anymore!!!
    void updateGridSizes()
    {
        if (gridRefUI)
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
        }

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


    virtual wxString GetColLabelValue( int col )
    {
        return CustomGrid::getTypeName(getTypeAtPos(col));
    }


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


protected:
    virtual const wxColour& getRowColor(int row) = 0; //rows that are filtered out are shown in different color

    std::vector<xmlAccess::ColumnTypes> columnPositions;

    wxColour  blue;
    wxColour  grey;
    wxColour  lightRed;
    wxColour  lightBlue;
    wxColour  lightGreen;
    GridView* gridRefUI; //(very fast) access to underlying grid data :)
    FileCompareResult* gridData;
    int       lastNrRows;
    int       lastNrCols;
};


class CustomGridTableLeft : public CustomGridTable
{
public:
    CustomGridTableLeft() : CustomGridTable() {}
    ~CustomGridTableLeft() {}

    virtual const wxColour& getRowColor(int row) //rows that are filtered out are shown in different color
    {
        if (gridRefUI && unsigned(row) < gridRefUI->size())
        {
            const FileCompareLine cmpLine = (*gridData)[(*gridRefUI)[row]];

            //mark filtered rows
            if (!cmpLine.selectedForSynchronization)
                return blue;
            //mark directories
            else if (cmpLine.fileDescrLeft.objType == FileDescrLine::TYPE_DIRECTORY)
                return grey;
            else
                return *wxWHITE;
        }
        return *wxWHITE;
    }


    //virtual impl.
    wxString GetValue(int row, int col)
    {
        if (gridRefUI)
        {
            if (unsigned(row) < gridRefUI->size())
            {
                const FileCompareLine& gridLine = (*gridData)[(*gridRefUI)[row]];

                if (gridLine.fileDescrLeft.objType == FileDescrLine::TYPE_DIRECTORY)
                {
                    switch (getTypeAtPos(col))
                    {
                    case xmlAccess::FULL_NAME:
                        return gridLine.fileDescrLeft.fullName.c_str();
                    case xmlAccess::FILENAME: //filename
                        return wxEmptyString;
                    case xmlAccess::REL_PATH: //relative path
                        return gridLine.fileDescrLeft.relativeName.c_str();
                    case xmlAccess::SIZE: //file size
                        return _("<Directory>");
                    case xmlAccess::DATE: //date
                        return wxEmptyString;
                    }
                }
                else if (gridLine.fileDescrLeft.objType == FileDescrLine::TYPE_FILE)
                {
                    switch (getTypeAtPos(col))
                    {
                    case xmlAccess::FULL_NAME:
                        return gridLine.fileDescrLeft.fullName.c_str();
                    case xmlAccess::FILENAME: //filename
                        return gridLine.fileDescrLeft.relativeName.AfterLast(GlobalResources::FILE_NAME_SEPARATOR).c_str();
                    case xmlAccess::REL_PATH: //relative path
                        return gridLine.fileDescrLeft.relativeName.BeforeLast(GlobalResources::FILE_NAME_SEPARATOR).c_str();
                    case xmlAccess::SIZE: //file size
                    {
                        wxString fileSize = gridLine.fileDescrLeft.fileSize.ToString(); //tmp string
                        return globalFunctions::includeNumberSeparator(fileSize);
                    }
                    case xmlAccess::DATE: //date
                        return FreeFileSync::utcTimeToLocalString(gridLine.fileDescrLeft.lastWriteTimeRaw);
                    }
                }
            }
        }
        //if data is not found:
        return wxEmptyString;
    }
};


class CustomGridTableMiddle : public CustomGridTable
{
public:
    CustomGridTableMiddle() : CustomGridTable()
    {
        lastNrCols = 1; //ensure CustomGridTable::updateGridSizes() is working correctly
    }
    ~CustomGridTableMiddle() {}

    //virtual impl.
    int GetNumberCols()
    {
        return 1;
    }


    int selectedForSynchronization(const unsigned int row) // 0 == false, 1 == true, -1 == not defined
    {
        if (gridRefUI && row < gridRefUI->size())
        {
            const FileCompareLine cmpLine = (*gridData)[(*gridRefUI)[row]];

            if (cmpLine.selectedForSynchronization)
                return 1;
            else
                return 0;
        }
        return -1;
    }


    virtual const wxColour& getRowColor(int row) //rows that are filtered out are shown in different color
    {
        if (gridRefUI && unsigned(row) < gridRefUI->size())
        {
            const FileCompareLine cmpLine = (*gridData)[(*gridRefUI)[row]];

            //mark filtered rows
            if (!cmpLine.selectedForSynchronization)
                return blue;
            else
                switch (cmpLine.cmpResult)
                {
                case FILE_LEFT_SIDE_ONLY:
                case FILE_RIGHT_SIDE_ONLY:
                    return lightGreen;
                case FILE_LEFT_NEWER:
                case FILE_RIGHT_NEWER:
                    return lightBlue;
                case FILE_DIFFERENT:
                    return lightRed;
                default:
                    return *wxWHITE;
                }
        }
        return *wxWHITE;
    }


    virtual wxString GetValue(int row, int col)
    {
        if (gridRefUI)
        {
            if (unsigned(row) < gridRefUI->size())
            {
                const FileCompareLine& gridLine = (*gridData)[(*gridRefUI)[row]];

                switch (gridLine.cmpResult)
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
        }
        //if data is not found:
        return wxEmptyString;
    }
};


class CustomGridTableRight : public CustomGridTable
{
public:
    CustomGridTableRight() : CustomGridTable() {}
    ~CustomGridTableRight() {}

    virtual const wxColour& getRowColor(int row) //rows that are filtered out are shown in different color
    {
        if (gridRefUI && unsigned(row) < gridRefUI->size())
        {
            const FileCompareLine cmpLine = (*gridData)[(*gridRefUI)[row]];

            //mark filtered rows
            if (!cmpLine.selectedForSynchronization)
                return blue;
            //mark directories
            else if (cmpLine.fileDescrRight.objType == FileDescrLine::TYPE_DIRECTORY)
                return grey;
            else
                return *wxWHITE;
        }
        return *wxWHITE;
    }

    //virtual impl.
    wxString GetValue(int row, int col)
    {
        if (gridRefUI)
        {
            if (unsigned(row) < gridRefUI->size())
            {
                const FileCompareLine& gridLine = (*gridData)[(*gridRefUI)[row]];

                if (gridLine.fileDescrRight.objType == FileDescrLine::TYPE_DIRECTORY)
                {
                    switch (getTypeAtPos(col))
                    {
                    case xmlAccess::FULL_NAME:
                        return gridLine.fileDescrRight.fullName.c_str();
                    case xmlAccess::FILENAME: //filename
                        return wxEmptyString;
                    case xmlAccess::REL_PATH: //relative path
                        return gridLine.fileDescrRight.relativeName.c_str();
                    case xmlAccess::SIZE: //file size
                        return _("<Directory>");
                    case xmlAccess::DATE: //date
                        return wxEmptyString;
                    }
                }
                else if (gridLine.fileDescrRight.objType == FileDescrLine::TYPE_FILE)
                {
                    switch (getTypeAtPos(col))
                    {
                    case xmlAccess::FULL_NAME:
                        return gridLine.fileDescrRight.fullName.c_str();
                    case xmlAccess::FILENAME: //filename
                        return gridLine.fileDescrRight.relativeName.AfterLast(GlobalResources::FILE_NAME_SEPARATOR).c_str();
                    case xmlAccess::REL_PATH: //relative path
                        return gridLine.fileDescrRight.relativeName.BeforeLast(GlobalResources::FILE_NAME_SEPARATOR).c_str();
                    case xmlAccess::SIZE: //file size
                    {
                        wxString fileSize = gridLine.fileDescrRight.fileSize.ToString(); //tmp string
                        return globalFunctions::includeNumberSeparator(fileSize);
                    }
                    case xmlAccess::DATE: //date
                        return FreeFileSync::utcTimeToLocalString(gridLine.fileDescrRight.lastWriteTimeRaw);
                    }
                }
            }
        }
        //if data is not found:
        return wxEmptyString;
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
        leadGrid(NULL),
        scrollbarsEnabled(true),
        m_gridLeft(NULL), m_gridRight(NULL), m_gridMiddle(NULL),
        gridDataTable(NULL),
        currentSortColumn(-1),
        sortMarker(NULL)
{
    //set color of selections
    wxColour darkBlue(40, 35, 140);
    SetSelectionBackground(darkBlue);
    SetSelectionForeground(*wxWHITE);
}


void CustomGrid::initSettings(bool enableScrollbars,
                              CustomGrid* gridLeft,
                              CustomGrid* gridRight,
                              CustomGrid* gridMiddle,
                              GridView* gridRefUI,
                              FileCompareResult* gridData)
{
    scrollbarsEnabled = enableScrollbars;

    //these grids will scroll together
    assert(gridLeft && gridRight && gridMiddle);
    m_gridLeft   = gridLeft;
    m_gridRight  = gridRight;
    m_gridMiddle = gridMiddle;

    //set underlying grid data
    assert(gridDataTable);
    gridDataTable->setGridDataTable(gridRefUI, gridData);
}


//overwrite virtual method to finally get rid of the scrollbars
void CustomGrid::SetScrollbar(int orientation, int position, int thumbSize, int range, bool refresh)
{
    if (scrollbarsEnabled)
        wxWindow::SetScrollbar(orientation, position, thumbSize, range, refresh);
    else
        wxWindow::SetScrollbar(orientation, 0, 0, 0, refresh);
}


//workaround: ensure that all grids are properly aligned: add some extra window space to grids that have no horizontal scrollbar
void CustomGrid::adjustGridHeights() //m_gridLeft, m_gridRight, m_gridMiddle are not NULL in this context
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

        if (leadGrid == m_gridLeft)  //do not handle case (y1 == yMax) here!!! Avoid back coupling!
            m_gridLeft->SetMargins(0, 0);
        else if (y1 < yMax)
            m_gridLeft->SetMargins(0, 50);

        if (leadGrid == m_gridRight)
            m_gridRight->SetMargins(0, 0);
        else if (y2 < yMax)
            m_gridRight->SetMargins(0, 50);

        if (leadGrid == m_gridMiddle)
            m_gridMiddle->SetMargins(0, 0);
        else if (y3 < yMax)
            m_gridMiddle->SetMargins(0, 50);

        m_gridLeft->ForceRefresh();
        m_gridRight->ForceRefresh();
        m_gridMiddle->ForceRefresh();
    }
}


void CustomGrid::setLeadGrid(const wxGrid* newLead)
{
    leadGrid = newLead;
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
    wxGrid::DrawColLabel(dc, col);

    if (col == currentSortColumn)
        dc.DrawBitmap(*sortMarker, GetColRight(col) - 16 - 2, 2, true); //respect 2-pixel border
}


xmlAccess::ColumnAttributes CustomGrid::getDefaultColumnAttributes()
{
    xmlAccess::ColumnAttributes defaultColumnSettings;

    xmlAccess::ColumnAttrib newEntry;

    newEntry.type     = xmlAccess::FULL_NAME;
    newEntry.visible  = false;
    newEntry.position = 0;
    newEntry.width    = 150;
    defaultColumnSettings.push_back(newEntry);

    newEntry.type     = xmlAccess::FILENAME;
    newEntry.visible  = true;
    newEntry.position = 1;
    newEntry.width    = 138;
    defaultColumnSettings.push_back(newEntry);

    newEntry.type     = xmlAccess::REL_PATH;
    newEntry.position = 2;
    newEntry.width    = 118;
    defaultColumnSettings.push_back(newEntry);

    newEntry.type     = xmlAccess::SIZE;
    newEntry.position = 3;
    newEntry.width    = 67;
    defaultColumnSettings.push_back(newEntry);

    newEntry.type     = xmlAccess::DATE;
    newEntry.position = 4;
    newEntry.width    = 113;
    defaultColumnSettings.push_back(newEntry);

    return defaultColumnSettings;
}


xmlAccess::ColumnAttributes CustomGrid::getColumnAttributes()
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


void CustomGrid::setColumnAttributes(const xmlAccess::ColumnAttributes& attr)
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


xmlAccess::ColumnTypes CustomGrid::getTypeAtPos(unsigned pos) const
{
    assert(gridDataTable);
    return gridDataTable->getTypeAtPos(pos);
}


wxString CustomGrid::getTypeName(xmlAccess::ColumnTypes colType)
{
    switch (colType)
    {
    case xmlAccess::FULL_NAME:
        return _("Full name");
    case xmlAccess::FILENAME:
        return _("Filename");
    case xmlAccess::REL_PATH:
        return _("Relative path");
    case xmlAccess::SIZE:
        return _("Size");
    case xmlAccess::DATE:
        return _("Date");
    default:
        return wxEmptyString;
    }
}

//############################################################################################
//CustomGrid specializations

CustomGridLeft::CustomGridLeft(wxWindow *parent,
                               wxWindowID id,
                               const wxPoint& pos,
                               const wxSize& size,
                               long style,
                               const wxString& name) :
        CustomGrid(parent, id, pos, size, style, name) {}


bool CustomGridLeft::CreateGrid(int numRows, int numCols, wxGrid::wxGridSelectionModes selmode)
{
    //use custom wxGridTableBase class for management of large sets of formatted data.
    //This is done in CreateGrid instead of SetTable method since source code is generated and wxFormbuilder invokes CreatedGrid by default.
    gridDataTable = new CustomGridTableLeft();
    SetTable(gridDataTable, true, wxGrid::wxGridSelectRows);  //give ownership to wxGrid: gridDataTable is deleted automatically in wxGrid destructor
    return true;
}


//this method is called when grid view changes: useful for parallel updating of multiple grids
void CustomGridLeft::DoPrepareDC(wxDC& dc)
{
    wxScrollHelper::DoPrepareDC(dc);

    int x, y = 0;
    if (this == leadGrid)  //avoid back coupling
    {
        GetViewStart(&x, &y);
        m_gridMiddle->Scroll(-1, y); //scroll in y-direction only
        m_gridRight->Scroll(x, y);
        adjustGridHeights(); //keep here to ensure m_gridLeft, m_gridRight, m_gridMiddle != NULL
    }
}


//----------------------------------------------------------------------------------------
CustomGridMiddle::CustomGridMiddle(wxWindow *parent,
                                   wxWindowID id,
                                   const wxPoint& pos,
                                   const wxSize& size,
                                   long style,
                                   const wxString& name) :
        CustomGrid(parent, id, pos, size, style, name) {}


bool CustomGridMiddle::CreateGrid(int numRows, int numCols, wxGrid::wxGridSelectionModes selmode)
{
    CustomGridTableMiddle* newTable = new CustomGridTableMiddle();
    gridDataTable = newTable;
    SetTable(gridDataTable, true, wxGrid::wxGridSelectRows);  //give ownership to wxGrid: gridDataTable is deleted automatically in wxGrid destructor

    //display checkboxes (representing bool values) if row is enabled for synchronization
    SetDefaultRenderer(new GridCellRendererAddCheckbox(newTable)); //SetDefaultRenderer takes ownership!

    return true;
}


//this method is called when grid view changes: useful for parallel updating of multiple grids
void CustomGridMiddle::DoPrepareDC(wxDC& dc)
{
    wxScrollHelper::DoPrepareDC(dc);

    int x, y = 0;
    if (this == leadGrid)  //avoid back coupling
    {
        GetViewStart(&x, &y);
        m_gridLeft->Scroll(-1, y);
        m_gridRight->Scroll(-1, y);
        adjustGridHeights(); //keep here to ensure m_gridLeft, m_gridRight, m_gridMiddle != NULL
    }
}


void CustomGridMiddle::GridCellRendererAddCheckbox::Draw(wxGrid& grid,
        wxGridCellAttr& attr,
        wxDC& dc,
        const wxRect& rect,
        int row, int col,
        bool isSelected)
{
    const int selected = m_gridDataTable->selectedForSynchronization(row);
    if (selected < 0) //no valid row
    {
        wxGridCellStringRenderer::Draw(grid, attr, dc, rect, row, col, isSelected);
        return;
    }

    const int shift = std::min(11 + 3, rect.GetWidth()); //11 is width of checkbox image

    wxRect rectShrinked(rect);

    //clean first block of rect that will receive image of checkbox
    rectShrinked.SetWidth(shift);
    dc.SetPen(*wxWHITE_PEN);
    dc.SetBrush(*wxWHITE_BRUSH);
    dc.DrawRectangle(rectShrinked);

    //print image into first block
    rectShrinked.SetX(1);
    if (selected > 0)
        dc.DrawLabel(wxEmptyString, *globalResource.bitmapCheckBoxTrue, rectShrinked, wxALIGN_LEFT | wxALIGN_CENTER_VERTICAL);
    else //if (selected == 0) -> redundant
        dc.DrawLabel(wxEmptyString, *globalResource.bitmapCheckBoxFalse, rectShrinked, wxALIGN_LEFT | wxALIGN_CENTER_VERTICAL);

    //print second block (default): display compare result
    rectShrinked.SetWidth(rect.GetWidth() - shift);
    rectShrinked.SetX(shift);
    wxGridCellStringRenderer::Draw(grid, attr, dc, rectShrinked, row, col, isSelected);
}


//----------------------------------------------------------------------------------------
CustomGridRight::CustomGridRight(wxWindow *parent,
                                 wxWindowID id,
                                 const wxPoint& pos,
                                 const wxSize& size,
                                 long style,
                                 const wxString& name) :
        CustomGrid(parent, id, pos, size, style, name) {}


bool CustomGridRight::CreateGrid(int numRows, int numCols, wxGrid::wxGridSelectionModes selmode)
{
    gridDataTable = new CustomGridTableRight();
    SetTable(gridDataTable, true, wxGrid::wxGridSelectRows);  //give ownership to wxGrid: gridDataTable is deleted automatically in wxGrid destructor
    return true;
}


//this method is called when grid view changes: useful for parallel updating of multiple grids
void CustomGridRight::DoPrepareDC(wxDC& dc)
{
    wxScrollHelper::DoPrepareDC(dc);

    int x, y = 0;
    if (this == leadGrid)  //avoid back coupling
    {
        GetViewStart(&x, &y);
        m_gridLeft->Scroll(x, y);
        m_gridMiddle->Scroll(-1, y);
        adjustGridHeights(); //keep here to ensure m_gridLeft, m_gridRight, m_gridMiddle != NULL
    }
}
