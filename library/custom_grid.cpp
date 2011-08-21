// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) 2008-2011 ZenJu (zhnmju123 AT gmx.de)                    *
// **************************************************************************

#include "custom_grid.h"
#include "resources.h"
#include <wx/dc.h>
#include "../shared/util.h"
#include "../shared/string_conv.h"
#include "resources.h"
#include <typeinfo>
#include "../ui/grid_view.h"
#include "../synchronization.h"
#include "../shared/custom_tooltip.h"
#include "../shared/i18n.h"
#include <wx/dcclient.h>
#include "icon_buffer.h"
#include <wx/icon.h>
#include <wx/tooltip.h>
#include <wx/settings.h>
#include "../shared/i18n.h"

#ifdef FFS_WIN
#include <wx/timer.h>
#include "status_handler.h"
#include <cmath>

#elif defined FFS_LINUX
#include <gtk/gtk.h>
#endif

using namespace zen;


const size_t MIN_ROW_COUNT = 15;

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
        gridDataView(NULL),
        lastNrRows(initialRows),
        lastNrCols(initialCols) {}


    virtual ~CustomGridTable() {}


    void setGridDataTable(const GridView* view)
    {
        this->gridDataView = view;
    }


    //###########################################################################
    //grid standard input output methods, redirected directly to gridData to improve performance

    virtual int GetNumberRows()
    {
        if (gridDataView)
            return static_cast<int>(std::max(gridDataView->rowsOnView(), MIN_ROW_COUNT));
        else
            return 0; //grid is initialized with zero number of rows
    }


    virtual bool IsEmptyCell(int row, int col)
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
        const std::pair<wxColour, wxColour> color = getRowColor(row);

        //add color to some rows
        wxGridCellAttr* result = wxGridTableBase::GetAttr(row, col, kind);
        if (result)
        {
            if (result->GetTextColour()       == color.first &&
                result->GetBackgroundColour() == color.second)
            {
                return result;
            }
            else //grid attribute might be referenced by other elements, so clone it!
            {
                wxGridCellAttr* attr = result->Clone(); //attr has ref-count 1
                result->DecRef();
                result = attr;
            }
        }
        else
            result = new wxGridCellAttr; //created with ref-count 1

        result->SetTextColour      (color.first);
        result->SetBackgroundColour(color.second);

        return result;
    }


    const FileSystemObject* getRawData(size_t row) const
    {
        if (gridDataView)
            return gridDataView->getObject(row); //returns NULL if request is not valid or not data found

        return NULL;
    }

protected:
    static const wxColour COLOR_BLUE;
    static const wxColour COLOR_GREY;
    static const wxColour COLOR_ORANGE;
    static const wxColour COLOR_CMP_RED;
    static const wxColour COLOR_CMP_BLUE;
    static const wxColour COLOR_CMP_GREEN;
    static const wxColour COLOR_SYNC_BLUE;
    static const wxColour COLOR_SYNC_BLUE_LIGHT;
    static const wxColour COLOR_SYNC_GREEN;
    static const wxColour COLOR_SYNC_GREEN_LIGHT;
    static const wxColour COLOR_YELLOW;
    static const wxColour COLOR_YELLOW_LIGHT;

    const GridView* gridDataView; //(very fast) access to underlying grid data :)

private:
    virtual const std::pair<wxColour, wxColour> getRowColor(int row) = 0; //rows that are filtered out are shown in different color: <foreground, background>

    int lastNrRows;
    int lastNrCols;
};

//see http://www.latiumsoftware.com/en/articles/00015.php#12 for "safe" colors
const wxColour CustomGridTable::COLOR_ORANGE(    238, 201, 0);
const wxColour CustomGridTable::COLOR_BLUE(      80,  110, 255);
const wxColour CustomGridTable::COLOR_GREY(      212, 208, 200);
const wxColour CustomGridTable::COLOR_CMP_RED(   249, 163, 165);
const wxColour CustomGridTable::COLOR_CMP_BLUE(  144, 232, 246);
const wxColour CustomGridTable::COLOR_CMP_GREEN( 147, 253, 159);
const wxColour CustomGridTable::COLOR_SYNC_BLUE( 201, 203, 247);
const wxColour CustomGridTable::COLOR_SYNC_BLUE_LIGHT(201, 225, 247);
const wxColour CustomGridTable::COLOR_SYNC_GREEN(197, 248, 190);
const wxColour CustomGridTable::COLOR_SYNC_GREEN_LIGHT(226, 248, 190);
const wxColour CustomGridTable::COLOR_YELLOW(    247, 252,  62);
const wxColour CustomGridTable::COLOR_YELLOW_LIGHT(253, 252, 169);


class CustomGridTableRim : public CustomGridTable
{
public:
    virtual ~CustomGridTableRim() {}

    virtual int GetNumberCols()
    {
        return static_cast<int>(columnPositions.size());
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


    xmlAccess::ColumnTypes getTypeAtPos(size_t pos) const
    {
        if (pos < columnPositions.size())
            return columnPositions[pos];
        else
            return xmlAccess::DIRECTORY;
    }

    //get filename in order to retrieve the icon from it
    virtual Zstring getIconFile(size_t row) const = 0;  //return "folder" if row points to a folder

protected:
    template <SelectedSide side>
    wxString GetValueSub(int row, int col)
    {
        const FileSystemObject* fsObj = getRawData(row);
        if (fsObj)
        {
            if (!fsObj->isEmpty<side>())
            {
                struct GetValue : public FSObjectVisitor
                {
                    GetValue(xmlAccess::ColumnTypes colType) : colType_(colType) {}
                    virtual void visit(const FileMapping& fileObj)
                    {
                        switch (colType_)
                        {
                            case xmlAccess::FULL_PATH:
                                value = toWx(fileObj.getFullName<side>().BeforeLast(FILE_NAME_SEPARATOR));
                                break;
                            case xmlAccess::FILENAME: //filename
                                value = toWx(fileObj.getShortName<side>());
                                break;
                            case xmlAccess::REL_PATH: //relative path
                                value = toWx(fileObj.getParentRelativeName());
                                break;
                            case xmlAccess::DIRECTORY:
                                value = toWx(fileObj.getBaseDirPf<side>());
                                break;
                            case xmlAccess::SIZE: //file size
                                value = zen::toStringSep(fileObj.getFileSize<side>());
                                break;
                            case xmlAccess::DATE: //date
                                value = zen::utcTimeToLocalString(fileObj.getLastWriteTime<side>());
                                break;
                            case xmlAccess::EXTENSION: //file extension
                                value = toWx(fileObj.getExtension<side>());
                                break;
                        }
                    }

                    virtual void visit(const SymLinkMapping& linkObj)
                    {
                        switch (colType_)
                        {
                            case xmlAccess::FULL_PATH:
                                value = toWx(linkObj.getFullName<side>().BeforeLast(FILE_NAME_SEPARATOR));
                                break;
                            case xmlAccess::FILENAME: //filename
                                value = toWx(linkObj.getShortName<side>());
                                break;
                            case xmlAccess::REL_PATH: //relative path
                                value = toWx(linkObj.getParentRelativeName());
                                break;
                            case xmlAccess::DIRECTORY:
                                value = toWx(linkObj.getBaseDirPf<side>());
                                break;
                            case xmlAccess::SIZE: //file size
                                value = _("<Symlink>");
                                break;
                            case xmlAccess::DATE: //date
                                value = zen::utcTimeToLocalString(linkObj.getLastWriteTime<side>());
                                break;
                            case xmlAccess::EXTENSION: //file extension
                                value = wxEmptyString;
                                break;
                        }
                    }

                    virtual void visit(const DirMapping& dirObj)
                    {
                        switch (colType_)
                        {
                            case xmlAccess::FULL_PATH:
                                value = toWx(dirObj.getFullName<side>());
                                break;
                            case xmlAccess::FILENAME:
                                value = toWx(dirObj.getShortName<side>());
                                break;
                            case xmlAccess::REL_PATH:
                                value = toWx(dirObj.getParentRelativeName());
                                break;
                            case xmlAccess::DIRECTORY:
                                value = toWx(dirObj.getBaseDirPf<side>());
                                break;
                            case xmlAccess::SIZE: //file size
                                value = _("<Directory>");
                                break;
                            case xmlAccess::DATE: //date
                                value = wxEmptyString;
                                break;
                            case xmlAccess::EXTENSION: //file extension
                                value = wxEmptyString;
                                break;
                        }
                    }
                    xmlAccess::ColumnTypes colType_;
                    wxString value;
                } getVal(getTypeAtPos(col));
                fsObj->accept(getVal);
                return getVal.value;
            }
        }
        //if data is not found:
        return wxEmptyString;
    }

    template <SelectedSide side>
    Zstring getIconFileImpl(size_t row) const  //return "folder" if row points to a folder
    {
        const FileSystemObject* fsObj = getRawData(row);
        if (fsObj && !fsObj->isEmpty<side>())
        {
            struct GetIcon : public FSObjectVisitor
            {
                virtual void visit(const FileMapping& fileObj)
                {
                    //Optimization: if filename exists on both sides, always use left side's file:
                    //Icon should be the same on both sides anyway...
                    if (!fileObj.isEmpty<LEFT_SIDE>() && !fileObj.isEmpty<RIGHT_SIDE>())
                        iconName = fileObj.getFullName<LEFT_SIDE>();
                    else
                        iconName = fileObj.getFullName<side>();
                }
                virtual void visit(const SymLinkMapping& linkObj)
                {
                    iconName = linkObj.getLinkType<side>() == LinkDescriptor::TYPE_DIR ?
                               Zstr("folder") :
                               linkObj.getFullName<side>();
                }
                virtual void visit(const DirMapping& dirObj)
                {
                    iconName = Zstr("folder");
                }

                Zstring iconName;
            } getIcon;
            fsObj->accept(getIcon);
            return getIcon.iconName;
        }

        return Zstring();
    }


private:
    virtual const std::pair<wxColour, wxColour> getRowColor(int row) //rows that are filtered out are shown in different color: <foreground, background>
    {
        std::pair<wxColour, wxColour> result(wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOWTEXT),
                                             wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOW));

        const FileSystemObject* fsObj = getRawData(row);
        if (fsObj)
        {
            //mark filtered rows
            if (!fsObj->isActive())
            {
                result.first = *wxBLACK;
                result.second = COLOR_BLUE;
            }
            else
            {
                //mark directories and symlinks
                struct GetRowColor : public FSObjectVisitor
                {
                    GetRowColor(wxColour& foreground, wxColour& background) : foreground_(foreground), background_(background) {}

                    virtual void visit(const FileMapping& fileObj) {}
                    virtual void visit(const SymLinkMapping& linkObj)
                    {
                        foreground_ = *wxBLACK;
                        background_ = COLOR_ORANGE;
                    }
                    virtual void visit(const DirMapping& dirObj)
                    {
                        foreground_ = *wxBLACK;
                        background_ = COLOR_GREY;
                    }

                private:
                    wxColour& foreground_;
                    wxColour& background_;
                } getCol(result.first, result.second);
                fsObj->accept(getCol);
            }
        }

        return result;
    }

    std::vector<xmlAccess::ColumnTypes> columnPositions;
};


class CustomGridTableLeft : public CustomGridTableRim
{
public:

    virtual wxString GetValue(int row, int col)
    {
        return CustomGridTableRim::GetValueSub<LEFT_SIDE>(row, col);
    }

    virtual Zstring getIconFile(size_t row) const  //return "folder" if row points to a folder
    {
        return getIconFileImpl<LEFT_SIDE>(row);
    }
};


class CustomGridTableRight : public CustomGridTableRim
{
public:
    virtual wxString GetValue(int row, int col)
    {
        return CustomGridTableRim::GetValueSub<RIGHT_SIDE>(row, col);
    }

    virtual Zstring getIconFile(size_t row) const //return "folder" if row points to a folder
    {
        return getIconFileImpl<RIGHT_SIDE>(row);
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

    virtual wxString GetValue(int row, int col) //method used for exporting .csv file only!
    {
        const FileSystemObject* fsObj = getRawData(row);
        if (fsObj)
        {
            if (syncPreviewActive) //synchronization preview
                return getSymbol(fsObj->getSyncOperation());
            else
                return getSymbol(fsObj->getCategory());
        }
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
    virtual const std::pair<wxColour, wxColour> getRowColor(int row) //rows that are filtered out are shown in different color: <foreground, background>
    {
        std::pair<wxColour, wxColour> result(wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOWTEXT),
                                             wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOW));

        const FileSystemObject* fsObj = getRawData(row);
        if (fsObj)
        {
            //mark filtered rows
            if (!fsObj->isActive())
            {
                result.first  = *wxBLACK;;
                result.second = COLOR_BLUE;
            }
            else
            {
                if (syncPreviewActive) //synchronization preview
                {
                    switch (fsObj->getSyncOperation()) //evaluate comparison result and sync direction
                    {
                        case SO_DO_NOTHING:
                        case SO_EQUAL:
                            break;//usually white
                        case SO_CREATE_NEW_LEFT:
                        case SO_OVERWRITE_LEFT:
                        case SO_DELETE_LEFT:
                            result.first  = *wxBLACK;
                            result.second = COLOR_SYNC_BLUE;
                            break;
                        case SO_COPY_METADATA_TO_LEFT:
                            result.first  = *wxBLACK;
                            result.second = COLOR_SYNC_BLUE_LIGHT;
                            break;
                        case SO_CREATE_NEW_RIGHT:
                        case SO_OVERWRITE_RIGHT:
                        case SO_DELETE_RIGHT:
                            result.first  = *wxBLACK;
                            result.second = COLOR_SYNC_GREEN;
                            break;
                        case SO_COPY_METADATA_TO_RIGHT:
                            result.first  = *wxBLACK;
                            result.second = COLOR_SYNC_GREEN_LIGHT;
                            break;
                        case SO_UNRESOLVED_CONFLICT:
                            result.first  = *wxBLACK;
                            result.second = COLOR_YELLOW;
                            break;
                    }
                }
                else //comparison results view
                {
                    switch (fsObj->getCategory())
                    {
                        case FILE_LEFT_SIDE_ONLY:
                        case FILE_RIGHT_SIDE_ONLY:
                            result.first  = *wxBLACK;
                            result.second = COLOR_CMP_GREEN;
                            break;
                        case FILE_LEFT_NEWER:
                        case FILE_RIGHT_NEWER:
                            result.first  = *wxBLACK;
                            result.second = COLOR_CMP_BLUE;
                            break;
                        case FILE_DIFFERENT:
                            result.first  = *wxBLACK;
                            result.second = COLOR_CMP_RED;
                            break;
                        case FILE_EQUAL:
                            break;//usually white
                        case FILE_CONFLICT:
                            result.first  = *wxBLACK;
                            result.second = COLOR_YELLOW;
                            break;
                        case FILE_DIFFERENT_METADATA:
                            result.first  = *wxBLACK;
                            result.second = COLOR_YELLOW_LIGHT;
                            break;
                    }
                }
            }
        }

        return result;
    }

    bool syncPreviewActive; //determines wheter grid shall show compare result or sync preview
};

//########################################################################################################


CustomGrid::CustomGrid(wxWindow*       parent,
                       wxWindowID      id,
                       const wxPoint&  pos,
                       const wxSize&   size,
                       long            style,
                       const wxString& name) :
    wxGrid(parent, id, pos, size, style, name),
    m_gridLeft(NULL),
    m_gridMiddle(NULL),
    m_gridRight(NULL),
    isLeading(false),
    m_marker(-1, ASCENDING)
{
    //wxColour darkBlue(40, 35, 140); -> user default colors instead!
    //SetSelectionBackground(wxSystemSettings::GetColour(wxSYS_COLOUR_HIGHLIGHT));
    //SetSelectionForeground(*wxWHITE);
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
    Connect(wxEVT_SET_FOCUS,                     wxEventHandler(CustomGrid::onGridAccess), NULL, this); //used by grid text-search
    GetGridWindow()->Connect(wxEVT_LEFT_DOWN,    wxEventHandler(CustomGrid::onGridAccess), NULL, this);
    GetGridWindow()->Connect(wxEVT_RIGHT_DOWN,   wxEventHandler(CustomGrid::onGridAccess), NULL, this);

    GetGridWindow()->Connect(wxEVT_ENTER_WINDOW, wxEventHandler(CustomGrid::adjustGridHeights), NULL, this);

    //parallel grid scrolling: do NOT use DoPrepareDC() to align grids! GDI resource leak! Use regular paint event instead:
    GetGridWindow()->Connect(wxEVT_PAINT, wxEventHandler(CustomGrid::OnPaintGrid), NULL, this);
}


void CustomGrid::release() //release connection to zen::GridView
{
    assert(getGridDataTable());
    getGridDataTable()->setGridDataTable(NULL); //kind of "disable" griddatatable; don't delete it with SetTable(NULL)! May be used by wxGridCellStringRenderer
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


void CustomGrid::OnPaintGrid(wxEvent& event)
{
    if (isLeadGrid())  //avoid back coupling
        alignOtherGrids(m_gridLeft, m_gridMiddle, m_gridRight); //scroll other grids
    event.Skip();
}


void moveCursorWhileSelecting(int anchor, int oldPos, int newPos, wxGrid* grid)
{
    //note: all positions are valid in this context!

    grid->SetGridCursor(  newPos, grid->GetGridCursorCol());
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


void execGridCommands(wxEvent& event, wxGrid* grid)
{
    static int anchorRow = 0;
    if (grid->GetNumberRows() == 0 ||
        grid->GetNumberCols() == 0)
        return;

    const wxKeyEvent* keyEvent = dynamic_cast<const wxKeyEvent*> (&event);
    if (keyEvent)
    {
        //ensure cursorOldPos is always a valid row!
        const int cursorOldPos    = std::max(std::min(grid->GetGridCursorRow(), grid->GetNumberRows() - 1), 0);
        const int cursorOldColumn = std::max(std::min(grid->GetGridCursorCol(), grid->GetNumberCols() - 1), 0);

        const bool shiftPressed  = keyEvent->ShiftDown();
        const bool ctrlPressed   = keyEvent->ControlDown();
        const int keyCode        = keyEvent->GetKeyCode();

        //SHIFT + X
        if (shiftPressed)
            switch (keyCode)
            {
                case WXK_UP:
                case WXK_NUMPAD_UP:
                {
                    const int cursorNewPos = std::max(cursorOldPos - 1, 0);
                    moveCursorWhileSelecting(anchorRow, cursorOldPos, cursorNewPos, grid);
                    return; //no event.Skip()
                }
                case WXK_DOWN:
                case WXK_NUMPAD_DOWN:
                {
                    const int cursorNewPos = std::min(cursorOldPos + 1, grid->GetNumberRows() - 1);
                    moveCursorWhileSelecting(anchorRow, cursorOldPos, cursorNewPos, grid);
                    return; //no event.Skip()
                }
                case WXK_LEFT:
                case WXK_NUMPAD_LEFT:
                {
                    const int cursorColumn = std::max(cursorOldColumn - 1, 0);
                    grid->SetGridCursor(cursorOldPos, cursorColumn);
                    grid->MakeCellVisible(cursorOldPos, cursorColumn);
                    return; //no event.Skip()
                }
                case WXK_RIGHT:
                case WXK_NUMPAD_RIGHT:
                {
                    const int cursorColumn = std::min(cursorOldColumn + 1, grid->GetNumberCols() - 1);
                    grid->SetGridCursor(cursorOldPos, cursorColumn);
                    grid->MakeCellVisible(cursorOldPos, cursorColumn);
                    return; //no event.Skip()
                }
                case WXK_PAGEUP:
                case WXK_NUMPAD_PAGEUP:
                {
                    const int rowsPerPage  = grid->GetGridWindow()->GetSize().GetHeight() / grid->GetDefaultRowSize();
                    const int cursorNewPos = std::max(cursorOldPos - rowsPerPage, 0);
                    moveCursorWhileSelecting(anchorRow, cursorOldPos, cursorNewPos, grid);
                    return; //no event.Skip()
                }
                case WXK_PAGEDOWN:
                case WXK_NUMPAD_PAGEDOWN:
                {
                    const int rowsPerPage  = grid->GetGridWindow()->GetSize().GetHeight() / grid->GetDefaultRowSize();
                    const int cursorNewPos = std::min(cursorOldPos + rowsPerPage, grid->GetNumberRows() - 1);
                    moveCursorWhileSelecting(anchorRow, cursorOldPos, cursorNewPos, grid);
                    return; //no event.Skip()
                }
                case WXK_HOME:
                case WXK_NUMPAD_HOME:
                {
                    const int cursorNewPos = 0;
                    moveCursorWhileSelecting(anchorRow, cursorOldPos, cursorNewPos, grid);
                    return; //no event.Skip()
                }
                case WXK_END:
                case WXK_NUMPAD_END:
                {
                    const int cursorNewPos = grid->GetNumberRows() - 1;
                    moveCursorWhileSelecting(anchorRow, cursorOldPos, cursorNewPos, grid);
                    return; //no event.Skip()
                }
            }

        //CTRL + X
        if (ctrlPressed)
            switch (keyCode)
            {
                case WXK_UP:
                case WXK_NUMPAD_UP:
                {
                    grid->SetGridCursor(0, grid->GetGridCursorCol());
                    grid->MakeCellVisible(0, grid->GetGridCursorCol());
                    return; //no event.Skip()
                }
                case WXK_DOWN:
                case WXK_NUMPAD_DOWN:
                {
                    grid->SetGridCursor(grid->GetNumberRows() - 1, grid->GetGridCursorCol());
                    grid->MakeCellVisible(grid->GetNumberRows() - 1, grid->GetGridCursorCol());
                    return; //no event.Skip()
                }
                case WXK_LEFT:
                case WXK_NUMPAD_LEFT:
                {
                    grid->SetGridCursor(grid->GetGridCursorRow(), 0);
                    grid->MakeCellVisible(grid->GetGridCursorRow(), 0);
                    return; //no event.Skip()
                }
                case WXK_RIGHT:
                case WXK_NUMPAD_RIGHT:
                {
                    grid->SetGridCursor(grid->GetGridCursorRow(), grid->GetNumberCols() - 1);
                    grid->MakeCellVisible(grid->GetGridCursorRow(), grid->GetNumberCols() - 1);
                    return; //no event.Skip()
                }
            }

        //button with or without control keys pressed
        switch (keyCode)
        {
            case WXK_HOME:
            case WXK_NUMPAD_HOME:
            {
                grid->SetGridCursor(0, grid->GetGridCursorCol());
                grid->MakeCellVisible(0, grid->GetGridCursorCol());
                return; //no event.Skip()
            }
            case WXK_END:
            case WXK_NUMPAD_END:
            {
                grid->SetGridCursor(grid->GetNumberRows() - 1, grid->GetGridCursorCol());
                grid->MakeCellVisible(grid->GetNumberRows() - 1, grid->GetGridCursorCol());
                return; //no event.Skip()
            }

            case WXK_PAGEUP:
            case WXK_NUMPAD_PAGEUP:
            {
                const int rowsPerPage  = grid->GetGridWindow()->GetSize().GetHeight() / grid->GetDefaultRowSize();
                const int cursorNewPos = std::max(cursorOldPos - rowsPerPage, 0);
                grid->SetGridCursor(cursorNewPos, grid->GetGridCursorCol());
                grid->MakeCellVisible(cursorNewPos, grid->GetGridCursorCol());
                return; //no event.Skip()
            }
            case WXK_PAGEDOWN:
            case WXK_NUMPAD_PAGEDOWN:
            {
                const int rowsPerPage  = grid->GetGridWindow()->GetSize().GetHeight() / grid->GetDefaultRowSize();
                const int cursorNewPos = std::min(cursorOldPos + rowsPerPage, grid->GetNumberRows() - 1);
                grid->SetGridCursor(cursorNewPos, grid->GetGridCursorCol());
                grid->MakeCellVisible(cursorNewPos, grid->GetGridCursorCol());
                return; //no event.Skip()
            }
        }

        //button without additonal control keys pressed
        if (keyEvent->GetModifiers() == wxMOD_NONE)
            switch (keyCode)
            {
                case WXK_UP:
                case WXK_NUMPAD_UP:
                {
                    const int cursorNewPos = std::max(cursorOldPos - 1, 0);
                    grid->SetGridCursor(cursorNewPos, grid->GetGridCursorCol());
                    grid->MakeCellVisible(cursorNewPos, grid->GetGridCursorCol());
                    return; //no event.Skip()
                }
                case WXK_DOWN:
                case WXK_NUMPAD_DOWN:
                {
                    const int cursorNewPos = std::min(cursorOldPos + 1, grid->GetNumberRows() - 1);
                    grid->SetGridCursor(cursorNewPos, grid->GetGridCursorCol());
                    grid->MakeCellVisible(cursorNewPos, grid->GetGridCursorCol());
                    return; //no event.Skip()
                }
                case WXK_LEFT:
                case WXK_NUMPAD_LEFT:
                {
                    const int cursorColumn = std::max(cursorOldColumn - 1, 0);
                    grid->SetGridCursor(cursorOldPos, cursorColumn);
                    grid->MakeCellVisible(cursorOldPos, cursorColumn);
                    return; //no event.Skip()
                }
                case WXK_RIGHT:
                case WXK_NUMPAD_RIGHT:
                {
                    const int cursorColumn = std::min(cursorOldColumn + 1, grid->GetNumberCols() - 1);
                    grid->SetGridCursor(cursorOldPos, cursorColumn);
                    grid->MakeCellVisible(cursorOldPos, cursorColumn);
                    return; //no event.Skip()
                }
            }
    }

    anchorRow = grid->GetGridCursorRow();
    event.Skip(); //let event delegate!
}


inline
bool gridsShouldBeCleared(const wxEvent& event)
{
    const wxMouseEvent* mouseEvent = dynamic_cast<const wxMouseEvent*>(&event);
    if (mouseEvent)
    {
        if (mouseEvent->ControlDown() || mouseEvent->ShiftDown())
            return false;

        if (mouseEvent->ButtonDown(wxMOUSE_BTN_LEFT))
            return true;

        return false;
    }
    else
    {
        const wxKeyEvent* keyEvent = dynamic_cast<const wxKeyEvent*>(&event);
        if (keyEvent)
        {
            if (keyEvent->ControlDown() || keyEvent->AltDown() || keyEvent->ShiftDown())
                return false;

            switch (keyEvent->GetKeyCode())
            {
                    //default navigation keys
                case WXK_UP:
                case WXK_DOWN:
                case WXK_LEFT:
                case WXK_RIGHT:
                case WXK_PAGEUP:
                case WXK_PAGEDOWN:
                case WXK_HOME:
                case WXK_END:
                case WXK_NUMPAD_UP:
                case WXK_NUMPAD_DOWN:
                case WXK_NUMPAD_LEFT:
                case WXK_NUMPAD_RIGHT:
                case WXK_NUMPAD_PAGEUP:
                case WXK_NUMPAD_PAGEDOWN:
                case WXK_NUMPAD_HOME:
                case WXK_NUMPAD_END:
                    //other keys
                case WXK_TAB:
                case WXK_RETURN:
                case WXK_NUMPAD_ENTER:
                case WXK_ESCAPE:
                    return true;
            }

            return false;
        }
    }

    return false;
}


void CustomGrid::onGridAccess(wxEvent& event)
{
    if (!isLeading)
    {
        //notify other grids of new user focus
        m_gridLeft  ->isLeading = m_gridLeft   == this;
        m_gridMiddle->isLeading = m_gridMiddle == this;
        m_gridRight ->isLeading = m_gridRight  == this;

        wxGrid::SetFocus();
    }

    //clear grids
    if (gridsShouldBeCleared(event))
    {
        m_gridLeft  ->ClearSelection();
        m_gridMiddle->ClearSelection();
        m_gridRight ->ClearSelection();
    }

    //update row labels NOW (needed when scrolling if buttons keep being pressed)
    m_gridLeft ->GetGridRowLabelWindow()->Update();
    m_gridRight->GetGridRowLabelWindow()->Update();

    //support for custom short-cuts (overwriting wxWidgets functionality!)
    execGridCommands(event, this); //event.Skip is handled here!
}


//workaround: ensure that all grids are properly aligned: add some extra window space to grids that have no horizontal scrollbar
void CustomGrid::adjustGridHeights(wxEvent& event)
{
    //m_gridLeft, m_gridRight, m_gridMiddle not NULL because called after initSettings()

    int y1 = 0;
    int y2 = 0;
    int y3 = 0;
    int dummy = 0;

    m_gridLeft  ->GetViewStart(&dummy, &y1);
    m_gridRight ->GetViewStart(&dummy, &y2);
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

        m_gridLeft  ->ForceRefresh();
        m_gridRight ->ForceRefresh();
        m_gridMiddle->ForceRefresh();
    }
}


void CustomGrid::updateGridSizes()
{
    if(getGridDataTable())
        getGridDataTable()->updateGridSizes();
}


void CustomGridRim::updateGridSizes()
{
    CustomGrid::updateGridSizes();

    //set row label size

    //SetRowLabelSize(wxGRID_AUTOSIZE); -> we can do better
    wxClientDC dc(GetGridRowLabelWindow());
    dc.SetFont(GetLabelFont());

    wxArrayString lines;
    lines.push_back(GetRowLabelValue(GetNumberRows()));

    long width = 0;
    long dummy = 0;
    GetTextBoxSize(dc, lines, &width, &dummy);

    width += 8;
    SetRowLabelSize(width);
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
            dc.DrawBitmap(GlobalResources::instance().getImage(wxT("smallUp")), GetColRight(col) - 16 - 2, 2, true); //respect 2-pixel border
        else
            dc.DrawBitmap(GlobalResources::instance().getImage(wxT("smallDown")), GetColRight(col) - 16 - 2, 2, true); //respect 2-pixel border
    }
}


std::pair<int, int> CustomGrid::mousePosToCell(wxPoint pos)
{
    int x = -1;
    int y = -1;
    CalcUnscrolledPosition(pos.x, pos.y, &x, &y);

    std::pair<int, int> output(-1, -1);
    if (x >= 0 && y >= 0)
    {
        output.first  = YToRow(y);
        output.second = XToCol(x);
    }
    return output;
}


std::set<size_t> CustomGrid::getAllSelectedRows() const
{
    std::set<size_t> output;

    const wxArrayInt selectedRows = this->GetSelectedRows();
    if (!selectedRows.IsEmpty())
    {
        for (size_t i = 0; i < selectedRows.GetCount(); ++i)
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
        for (size_t k = 0; k < singlySelected.GetCount(); ++k)
            output.insert(singlySelected[k].GetRow());
    }

    const wxGridCellCoordsArray tmpArrayTop = this->GetSelectionBlockTopLeft();
    if (!tmpArrayTop.IsEmpty())
    {
        wxGridCellCoordsArray tmpArrayBottom = this->GetSelectionBlockBottomRight();

        size_t arrayCount = tmpArrayTop.GetCount();

        if (arrayCount == tmpArrayBottom.GetCount())
        {
            for (size_t i = 0; i < arrayCount; ++i)
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

        if (showFileIcons && //evaluate at compile time
            m_gridDataTable->getTypeAtPos(col) == xmlAccess::FILENAME)
        {
            if (rect.GetWidth() >= IconBuffer::ICON_SIZE)
            {
                //  Partitioning:
                //   _____________________
                //  | 2 pix | icon | rest |
                //   ---------------------

                //clear area where icon will be placed
                wxRect rectShrinked(rect);
                rectShrinked.SetWidth(IconBuffer::ICON_SIZE + LEFT_BORDER); //add 2 pixel border
                wxGridCellRenderer::Draw(grid, attr, dc, rectShrinked, row, col, isSelected);

                //draw rest
                wxRect rest(rect); //unscrolled
                rest.x     += IconBuffer::ICON_SIZE + LEFT_BORDER;
                rest.width -= IconBuffer::ICON_SIZE + LEFT_BORDER;
                wxGridCellStringRenderer::Draw(grid, attr, dc, rest, row, col, isSelected);

                //try to draw icon
                //retrieve grid data
                const Zstring fileName = m_gridDataTable->getIconFile(row);
                if (!fileName.empty())
                {
                    //first check if it is a directory icon:
                    if (fileName == Zstr("folder"))
                    {
                        dc.DrawIcon(IconBuffer::getDirectoryIcon(), rectShrinked.GetX() + LEFT_BORDER, rectShrinked.GetY());
                        m_loadIconSuccess[row] = true; //save status of last icon load -> used for async. icon loading
                    }
                    else //retrieve file icon
                    {
                        wxIcon icon;
                        bool iconDrawnFully = false;
                        {
                            const bool loadSuccess = IconBuffer::getInstance().requestFileIcon(fileName, &icon); //returns false if icon is not in buffer
                            if (loadSuccess)
                            {
                                //-----------------------------------------------------------------------------------------------
                                //only mark as successful if icon was drawn fully!
                                //(attention: when scrolling, rows get partially updated, which can result in the upper half being blank!)

                                //rect where icon was placed
                                wxRect iconRect(rect); //unscrolled
                                iconRect.x += LEFT_BORDER;
                                iconRect.SetWidth(IconBuffer::ICON_SIZE);

                                //convert to scrolled coordinates
                                grid.CalcScrolledPosition(iconRect.x, iconRect.y, &iconRect.x, &iconRect.y);

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
                            }
                            else
                                icon = IconBuffer::getFileIcon(); //better than nothing
                        }

                        if (icon.IsOk())
                            dc.DrawIcon(icon, rectShrinked.GetX() + LEFT_BORDER, rectShrinked.GetY());

                        //-----------------------------------------------------------------------------------------------
                        //save status of last icon load -> used for async. icon loading
                        m_loadIconSuccess[row] = iconDrawnFully;
                    }
                }
                return;
            }
        }

        //default
        wxGridCellStringRenderer::Draw(grid, attr, dc, rect, row, col, isSelected);
    }


    virtual wxSize GetBestSize(wxGrid& grid,       //adapt reported width if file icons are shown
                               wxGridCellAttr& attr,
                               wxDC& dc,
                               int row, int col)
    {
        if (showFileIcons && //evaluate at compile time
            m_gridDataTable->getTypeAtPos(col) == xmlAccess::FILENAME)
        {
            wxSize rv = wxGridCellStringRenderer::GetBestSize(grid, attr, dc, row, col);
            rv.SetWidth(rv.GetWidth() + LEFT_BORDER + IconBuffer::ICON_SIZE);
            return rv;
        }

        //default
        return wxGridCellStringRenderer::GetBestSize(grid, attr, dc, row, col);
    }


private:
    CustomGridRim::LoadSuccess& m_loadIconSuccess;
    const CustomGridTableRim* const m_gridDataTable;

    static const int LEFT_BORDER = 2;
};

//----------------------------------------------------------------------------------------

CustomGridRim::CustomGridRim(wxWindow* parent,
                             wxWindowID id,
                             const wxPoint& pos,
                             const wxSize& size,
                             long style,
                             const wxString& name) :
    CustomGrid(parent, id, pos, size, style, name), fileIconsAreEnabled(false), otherGrid(NULL)
{
    Connect(wxEVT_GRID_COL_SIZE, wxGridSizeEventHandler(CustomGridRim::OnResizeColumn), NULL, this); //row-based tooltip
}


void CustomGridRim::setOtherGrid(CustomGridRim* other) //call during initialization!
{
    otherGrid = other;
}


void CustomGridRim::OnResizeColumn(wxGridSizeEvent& event)
{
    //Resize columns on both sides in parallel
    const int thisCol = event.GetRowOrCol();

    if (!otherGrid || thisCol < 0 || thisCol >= GetNumberCols()) return;

    const xmlAccess::ColumnTypes thisColType = getTypeAtPos(thisCol);

    for (int i = 0; i < otherGrid->GetNumberCols(); ++i)
        if (otherGrid->getTypeAtPos(i) == thisColType)
        {
            otherGrid->SetColSize(i, GetColSize(thisCol));
            otherGrid->ForceRefresh();
            break;
        }
}


//this method is called when grid view changes: useful for parallel updating of multiple grids
void CustomGridRim::alignOtherGrids(CustomGrid* gridLeft, CustomGrid* gridMiddle, CustomGrid* gridRight)
{
    if (!otherGrid) return;

    int x = 0;
    int y = 0;
    GetViewStart(&x, &y);
    gridMiddle->Scroll(-1, y);
    otherGrid->Scroll(x, y);
}


template <SelectedSide side>
void CustomGridRim::setTooltip(const wxMouseEvent& event)
{
    const int hoveredRow = mousePosToCell(event.GetPosition()).first;

    wxString toolTip;
    if (hoveredRow >= 0 && getGridDataTable() != NULL)
    {
        const FileSystemObject* const fsObj = getGridDataTable()->getRawData(hoveredRow);
        if (fsObj && !fsObj->isEmpty<side>())
        {
            struct AssembleTooltip : public FSObjectVisitor
            {
                AssembleTooltip(wxString& tipMsg) : tipMsg_(tipMsg) {}

                virtual void visit(const FileMapping& fileObj)
                {
                    tipMsg_ = toWx(fileObj.getRelativeName<side>()) + "\n" +
                              _("Size") + ": " + zen::formatFilesizeToShortString(fileObj.getFileSize<side>()) + "\n" +
                              _("Date") + ": " + zen::utcTimeToLocalString(fileObj.getLastWriteTime<side>());
                }

                virtual void visit(const SymLinkMapping& linkObj)
                {
                    tipMsg_ = toWx(linkObj.getRelativeName<side>()) + "\n" +
                              _("Date") + ": " + zen::utcTimeToLocalString(linkObj.getLastWriteTime<side>());
                }

                virtual void visit(const DirMapping& dirObj)
                {
                    tipMsg_ = toWx(dirObj.getRelativeName<side>());
                }

                wxString& tipMsg_;
            } assembler(toolTip);
            fsObj->accept(assembler);
        }
    }


    wxToolTip* tt = GetGridWindow()->GetToolTip();

    const wxString currentTip = tt ? tt->GetTip() : wxString();
    if (toolTip != currentTip)
    {
        if (toolTip.IsEmpty())
            GetGridWindow()->SetToolTip(NULL); //wxGTK doesn't allow wxToolTip with empty text!
        else
        {
            //wxWidgets bug: tooltip multiline property is defined by first tooltip text containing newlines or not (same is true for maximum width)
            if (!tt)
                GetGridWindow()->SetToolTip(new wxToolTip(wxT("a                                                                b\n\
                                                           a                                                                b"))); //ugly, but is working (on Windows)
            tt = GetGridWindow()->GetToolTip(); //should be bound by now
            if (tt)
                tt->SetTip(toolTip);
        }
    }
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

    newEntry.type     = xmlAccess::EXTENSION;
    newEntry.visible  = false;
    newEntry.position = 6;
    newEntry.width    = 60;
    defaultColumnSettings.push_back(newEntry);

    return defaultColumnSettings;
}


xmlAccess::ColumnAttributes CustomGridRim::getColumnAttributes()
{
    std::sort(columnSettings.begin(), columnSettings.end(), xmlAccess::sortByPositionAndVisibility);

    xmlAccess::ColumnAttributes output;
    xmlAccess::ColumnAttrib newEntry;
    for (size_t i = 0; i < columnSettings.size(); ++i)
    {
        newEntry = columnSettings[i];
        if (newEntry.visible)
            newEntry.width = GetColSize(static_cast<int>(i)); //hidden columns are sorted to the end of vector!
        output.push_back(newEntry);
    }

    return output;
}


void CustomGridRim::setColumnAttributes(const xmlAccess::ColumnAttributes& attr)
{
    //remove special alignment for column "size"
    if (GetLayoutDirection() != wxLayout_RightToLeft) //don't change for RTL languages
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
    {
        //default settings:
        columnSettings = getDefaultColumnAttributes();
    }
    else
    {
        for (size_t i = 0; i < xmlAccess::COLUMN_TYPE_COUNT; ++i)
        {
            xmlAccess::ColumnAttrib newEntry;

            if (i < attr.size())
                newEntry = attr[i];
            else //fix corrupted data:
            {
                newEntry.type     = static_cast<xmlAccess::ColumnTypes>(xmlAccess::COLUMN_TYPE_COUNT); //sort additional rows to the end
                newEntry.visible  = false;
                newEntry.position = i;
                newEntry.width    = 100;
            }
            columnSettings.push_back(newEntry);
        }

        std::sort(columnSettings.begin(), columnSettings.end(), xmlAccess::sortByType);
        for (size_t i = 0; i < xmlAccess::COLUMN_TYPE_COUNT; ++i) //just be sure that each type exists only once
            columnSettings[i].type = static_cast<xmlAccess::ColumnTypes>(i);

        std::sort(columnSettings.begin(), columnSettings.end(), xmlAccess::sortByPositionOnly);
        for (size_t i = 0; i < xmlAccess::COLUMN_TYPE_COUNT; ++i) //just be sure that positions are numbered correctly
            columnSettings[i].position = i;
    }

    std::sort(columnSettings.begin(), columnSettings.end(), xmlAccess::sortByPositionAndVisibility);
    std::vector<xmlAccess::ColumnTypes> newPositions;
    for (size_t i = 0; i < columnSettings.size() && columnSettings[i].visible; ++i)  //hidden columns are sorted to the end of vector!
        newPositions.push_back(columnSettings[i].type);

    //set column positions
    if (getGridDataTableRim())
        getGridDataTableRim()->setupColumns(newPositions);

    //set column width (set them after setupColumns!)
    for (size_t i = 0; i < newPositions.size(); ++i)
        SetColSize(static_cast<int>(i), columnSettings[i].width);

    //--------------------------------------------------------------------------------------------------------
    //set special alignment for column "size"
    if (GetLayoutDirection() != wxLayout_RightToLeft) //don't change for RTL languages
        for (int i = 0; i < GetNumberCols(); ++i)
            if (getTypeAtPos(i) == xmlAccess::SIZE)
            {
                wxGridCellAttr* cellAttributes = GetOrCreateCellAttr(0, i);
                cellAttributes->SetAlignment(wxALIGN_RIGHT, wxALIGN_CENTRE);
                SetColAttr(i, cellAttributes); //make filesize right justified on grids
                break;
            }

    ClearSelection();
    ForceRefresh();
}


xmlAccess::ColumnTypes CustomGridRim::getTypeAtPos(size_t pos) const
{
    if (getGridDataTableRim())
        return getGridDataTableRim()->getTypeAtPos(pos);
    else
        return xmlAccess::DIRECTORY;
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
        case xmlAccess::EXTENSION:
            return _("Extension");
    }

    return wxEmptyString; //dummy
}


void CustomGridRim::autoSizeColumns()  //performance optimized column resizer (analog to wxGrid::AutoSizeColumns()
{
    for (int col = 0; col < GetNumberCols(); ++col)
    {
        if (col < 0)
            return;

        int rowMax = -1;
        size_t lenMax = 0;
        for (int row = 0; row < GetNumberRows(); ++row)
            if (GetCellValue(row, col).size() > lenMax)
            {
                lenMax = GetCellValue(row, col).size();
                rowMax = row;
            }

        wxCoord extentMax = 0;

        //calculate width of (most likely) widest cell
        wxClientDC dc(GetGridWindow());
        if (rowMax > -1)
        {
            wxGridCellAttr* attr = GetCellAttr(rowMax, col);
            if (attr)
            {
                wxGridCellRenderer* renderer = attr->GetRenderer(this, rowMax, col);
                if (renderer)
                {
                    const wxSize size = renderer->GetBestSize(*this, *attr, dc, rowMax, col);
                    extentMax = std::max(extentMax, size.x);
                    renderer->DecRef();
                }
                attr->DecRef();
            }
        }

        //consider column label
        dc.SetFont(GetLabelFont());
        wxCoord w = 0;
        wxCoord h = 0;
        dc.GetMultiLineTextExtent(GetColLabelValue(col), &w, &h );
        if (GetColLabelTextOrientation() == wxVERTICAL)
            w = h;
        extentMax = std::max(extentMax, w);

        extentMax += 15; //leave some space around text

        SetColSize(col, extentMax);

    }
    Refresh();
}


void CustomGridRim::enableFileIcons(const bool value)
{
    fileIconsAreEnabled = value;

    if (value)
        SetDefaultRenderer(new GridCellRenderer<true>(loadIconSuccess, getGridDataTableRim())); //SetDefaultRenderer takes ownership!
    else
        SetDefaultRenderer(new GridCellRenderer<false>(loadIconSuccess, getGridDataTableRim()));

    Refresh();
}


CustomGridRim::VisibleRowRange CustomGridRim::getVisibleRows()
{
    int dummy  = -1;
    int height = -1;
    GetGridWindow()->GetClientSize(&dummy, &height);

    if (height >= 0)
    {
        const int topRow = mousePosToCell(wxPoint(0, 0)).first;
        if (topRow >= 0)
        {
            const int rowCount  = static_cast<int>(ceil(height / static_cast<double>(GetDefaultRowSize()))); // = height / rowHeight rounded up
            const int bottomRow = topRow + rowCount - 1;

            return VisibleRowRange(topRow, bottomRow); //"top" means here top of the screen: => smaller value
        }
    }

    return VisibleRowRange(0, 0);
}


inline
CustomGridTableRim* CustomGridRim::getGridDataTableRim() const
{
    return dynamic_cast<CustomGridTableRim*>(getGridDataTable()); //I'm tempted to use a static cast here...
}


void CustomGridRim::getIconsToBeLoaded(std::vector<Zstring>& newLoad) //loads all (not yet) drawn icons
{
    newLoad.clear();

    if (fileIconsAreEnabled) //don't check too often! give worker thread some time to fetch data
    {
        const CustomGridTableRim* gridDataTable = getGridDataTableRim();
        if (!gridDataTable) return;

        const VisibleRowRange rowsOnScreen = getVisibleRows();
        const int totalCols = const_cast<CustomGridTableRim*>(gridDataTable)->GetNumberCols();
        const int totalRows = const_cast<CustomGridTableRim*>(gridDataTable)->GetNumberRows();

        //loop over all visible rows
        const int firstRow = static_cast<int>(rowsOnScreen.first);
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
                const Zstring fileName = gridDataTable->getIconFile(currentRow);
                if (!fileName.empty())
                {
                    //test if they are already loaded in buffer:
                    if (zen::IconBuffer::getInstance().requestFileIcon(fileName))
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
    m_timer->Start(50); //timer interval in ms
}


IconUpdater::~IconUpdater() {} //non-inline destructor for std::auto_ptr to work with forward declaration


void IconUpdater::loadIconsAsynchronously(wxEvent& event) //loads all (not yet) drawn icons
{
    std::vector<Zstring> iconsLeft;
    m_leftGrid->getIconsToBeLoaded(iconsLeft);

    std::vector<Zstring> newLoad;
    m_rightGrid->getIconsToBeLoaded(newLoad);

    //merge vectors
    newLoad.insert(newLoad.end(), iconsLeft.begin(), iconsLeft.end());

    zen::IconBuffer::getInstance().setWorkload(newLoad);

    //event.Skip();
}

//----------------------------------------------------------------------------------------


CustomGridLeft::CustomGridLeft(wxWindow* parent,
                               wxWindowID id,
                               const wxPoint& pos,
                               const wxSize& size,
                               long style,
                               const wxString& name) :
    CustomGridRim(parent, id, pos, size, style, name)
{
    GetGridWindow()->Connect(wxEVT_MOTION, wxMouseEventHandler(CustomGridLeft::OnMouseMovement), NULL, this); //row-based tooltip
}


bool CustomGridLeft::CreateGrid(int numRows, int numCols, wxGrid::wxGridSelectionModes selmode)
{
    //use custom wxGridTableBase class for management of large sets of formatted data.
    //This is done in CreateGrid instead of SetTable method since source code is generated and wxFormbuilder invokes CreatedGrid by default.
    SetTable(new CustomGridTableLeft, true, wxGrid::wxGridSelectRows);  //give ownership to wxGrid: gridDataTable is deleted automatically in wxGrid destructor
    return true;
}


void CustomGridLeft::initSettings(CustomGridLeft*   gridLeft,  //create connection with zen::GridView
                                  CustomGridMiddle* gridMiddle,
                                  CustomGridRight*  gridRight,
                                  const zen::GridView* gridDataView)
{
    //set underlying grid data
    assert(getGridDataTable());
    getGridDataTable()->setGridDataTable(gridDataView);

    CustomGridRim::setOtherGrid(gridRight);

    CustomGridRim::initSettings(gridLeft, gridMiddle, gridRight, gridDataView);
}


void CustomGridLeft::OnMouseMovement(wxMouseEvent& event)
{
    CustomGridRim::setTooltip<LEFT_SIDE>(event);
    event.Skip();
}


CustomGridTable* CustomGridLeft::getGridDataTable() const
{
    return static_cast<CustomGridTable*>(GetTable()); //one of the few cases where no dynamic_cast is required!
}


//----------------------------------------------------------------------------------------
CustomGridRight::CustomGridRight(wxWindow* parent,
                                 wxWindowID id,
                                 const wxPoint& pos,
                                 const wxSize& size,
                                 long style,
                                 const wxString& name) :
    CustomGridRim(parent, id, pos, size, style, name)
{
    GetGridWindow()->Connect(wxEVT_MOTION, wxMouseEventHandler(CustomGridRight::OnMouseMovement), NULL, this); //row-based tooltip
}


bool CustomGridRight::CreateGrid(int numRows, int numCols, wxGrid::wxGridSelectionModes selmode)
{
    SetTable(new CustomGridTableRight, true, wxGrid::wxGridSelectRows);  //give ownership to wxGrid: gridDataTable is deleted automatically in wxGrid destructor
    return true;
}


void CustomGridRight::initSettings(CustomGridLeft*   gridLeft,  //create connection with zen::GridView
                                   CustomGridMiddle* gridMiddle,
                                   CustomGridRight*  gridRight,
                                   const zen::GridView* gridDataView)
{
    //set underlying grid data
    assert(getGridDataTable());
    getGridDataTable()->setGridDataTable(gridDataView);

    CustomGridRim::setOtherGrid(gridLeft);

    CustomGridRim::initSettings(gridLeft, gridMiddle, gridRight, gridDataView);
}


void CustomGridRight::OnMouseMovement(wxMouseEvent& event)
{
    CustomGridRim::setTooltip<RIGHT_SIDE>(event);
    event.Skip();
}


CustomGridTable* CustomGridRight::getGridDataTable() const
{
    return static_cast<CustomGridTable*>(GetTable()); //one of the few cases where no dynamic_cast is required!
}


//----------------------------------------------------------------------------------------
class GridCellRendererMiddle : public wxGridCellStringRenderer
{
public:
    GridCellRendererMiddle(const CustomGridMiddle& middleGrid) : m_gridMiddle(middleGrid) {};

    virtual void Draw(wxGrid& grid,
                      wxGridCellAttr& attr,
                      wxDC& dc,
                      const wxRect& rect,
                      int row, int col,
                      bool isSelected);

private:
    const CustomGridMiddle& m_gridMiddle;
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


CustomGridMiddle::CustomGridMiddle(wxWindow* parent,
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
    toolTip(new CustomTooltip)
{
    SetLayoutDirection(wxLayout_LeftToRight);                          //
    GetGridWindow()->SetLayoutDirection(wxLayout_LeftToRight);         //avoid mirroring this dialog in RTL languages like Hebrew or Arabic
    GetGridColLabelWindow()->SetLayoutDirection(wxLayout_LeftToRight); //

    //connect events for dynamic selection of sync direction
    GetGridWindow()->Connect(wxEVT_MOTION,       wxMouseEventHandler(CustomGridMiddle::OnMouseMovement), NULL, this);
    GetGridWindow()->Connect(wxEVT_LEAVE_WINDOW, wxMouseEventHandler(CustomGridMiddle::OnLeaveWindow),   NULL, this);
    GetGridWindow()->Connect(wxEVT_LEFT_UP,      wxMouseEventHandler(CustomGridMiddle::OnLeftMouseUp),   NULL, this);
    GetGridWindow()->Connect(wxEVT_LEFT_DOWN,    wxMouseEventHandler(CustomGridMiddle::OnLeftMouseDown), NULL, this);
}


CustomGridMiddle::~CustomGridMiddle() {} //non-inline destructor for std::auto_ptr to work with forward declaration


bool CustomGridMiddle::CreateGrid(int numRows, int numCols, wxGrid::wxGridSelectionModes selmode)
{
    SetTable(new CustomGridTableMiddle, true, wxGrid::wxGridSelectRows);  //give ownership to wxGrid: gridDataTable is deleted automatically in wxGrid destructor

    //display checkboxes (representing bool values) if row is enabled for synchronization
    SetDefaultRenderer(new GridCellRendererMiddle(*this)); //SetDefaultRenderer takes ownership!

    return true;
}


void CustomGridMiddle::initSettings(CustomGridLeft*   gridLeft,  //create connection with zen::GridView
                                    CustomGridMiddle* gridMiddle,
                                    CustomGridRight*  gridRight,
                                    const zen::GridView* gridDataView)
{
    //set underlying grid data
    assert(getGridDataTable());
    getGridDataTable()->setGridDataTable(gridDataView);

#ifdef FFS_LINUX //get rid of scrollbars; Linux: change policy for GtkScrolledWindow
    GtkWidget* gridWidget = wxWindow::m_widget;
    GtkScrolledWindow* scrolledWindow = GTK_SCROLLED_WINDOW(gridWidget);
    gtk_scrolled_window_set_policy(scrolledWindow, GTK_POLICY_NEVER, GTK_POLICY_NEVER);
#endif

    CustomGrid::initSettings(gridLeft, gridMiddle, gridRight, gridDataView);
}


CustomGridTable* CustomGridMiddle::getGridDataTable() const
{
    return static_cast<CustomGridTable*>(GetTable()); //one of the few cases where no dynamic_cast is required!
}


inline
CustomGridTableMiddle* CustomGridMiddle::getGridDataTableMiddle() const
{
    return static_cast<CustomGridTableMiddle*>(getGridDataTable());
}


#ifdef FFS_WIN //get rid of scrollbars; Windows: overwrite virtual method
void CustomGridMiddle::SetScrollbar(int orientation, int position, int thumbSize, int range, bool refresh)
{
    wxWindow::SetScrollbar(orientation, 0, 0, 0, refresh);
}
#endif


void CustomGridMiddle::OnMouseMovement(wxMouseEvent& event)
{
    const int rowOld = highlightedRow;
    const BlockPosition posOld = highlightedPos;


    if (selectionRowBegin == -1) //change highlightning only if currently not dragging mouse
    {
        highlightedRow = mousePosToRow(event.GetPosition(), &highlightedPos);

        if (rowOld != highlightedRow)
        {
            RefreshCell(highlightedRow, 0);
            RefreshCell(rowOld, 0);
        }
        else if (posOld != highlightedPos)
            RefreshCell(highlightedRow, 0);

        //handle tooltip
        showToolTip(highlightedRow, GetGridWindow()->ClientToScreen(event.GetPosition()));
    }

    event.Skip();
}


void CustomGridMiddle::OnLeaveWindow(wxMouseEvent& event)
{
    highlightedRow = -1;
    highlightedPos = BLOCKPOS_CHECK_BOX;
    Refresh();

    //handle tooltip
    toolTip->hide();
}


void CustomGridMiddle::showToolTip(int rowNumber, wxPoint pos)
{
    if (!getGridDataTableMiddle())
        return;

    const FileSystemObject* const fsObj = getGridDataTableMiddle()->getRawData(rowNumber);
    if (fsObj == NULL) //if invalid row...
    {
        toolTip->hide();
        return;
    }

    if (getGridDataTableMiddle()->syncPreviewIsActive()) //synchronization preview
    {
        const SyncOperation syncOp = fsObj->getSyncOperation();
        switch (syncOp)
        {
            case SO_CREATE_NEW_LEFT:
                toolTip->show(getDescription(syncOp), pos, &GlobalResources::instance().getImage(wxT("syncCreateLeftAct")));
                break;
            case SO_CREATE_NEW_RIGHT:
                toolTip->show(getDescription(syncOp), pos, &GlobalResources::instance().getImage(wxT("syncCreateRightAct")));
                break;
            case SO_DELETE_LEFT:
                toolTip->show(getDescription(syncOp), pos, &GlobalResources::instance().getImage(wxT("syncDeleteLeftAct")));
                break;
            case SO_DELETE_RIGHT:
                toolTip->show(getDescription(syncOp), pos, &GlobalResources::instance().getImage(wxT("syncDeleteRightAct")));
                break;
            case SO_OVERWRITE_LEFT:
            case SO_COPY_METADATA_TO_LEFT:
                toolTip->show(getDescription(syncOp), pos, &GlobalResources::instance().getImage(wxT("syncDirLeftAct")));
                break;
            case SO_OVERWRITE_RIGHT:
            case SO_COPY_METADATA_TO_RIGHT:
                toolTip->show(getDescription(syncOp), pos, &GlobalResources::instance().getImage(wxT("syncDirRightAct")));
                break;
            case SO_DO_NOTHING:
                toolTip->show(getDescription(syncOp), pos, &GlobalResources::instance().getImage(wxT("syncDirNoneAct")));
                break;
            case SO_EQUAL:
                toolTip->show(getDescription(syncOp), pos, &GlobalResources::instance().getImage(wxT("equalAct")));
                break;
            case SO_UNRESOLVED_CONFLICT:
                toolTip->show(fsObj->getSyncOpConflict(), pos, &GlobalResources::instance().getImage(wxT("conflictAct")));
                break;
        };
    }
    else
    {
        const CompareFilesResult cmpRes = fsObj->getCategory();
        switch (cmpRes)
        {
            case FILE_LEFT_SIDE_ONLY:
                toolTip->show(getDescription(cmpRes), pos, &GlobalResources::instance().getImage(wxT("leftOnlyAct")));
                break;
            case FILE_RIGHT_SIDE_ONLY:
                toolTip->show(getDescription(cmpRes), pos, &GlobalResources::instance().getImage(wxT("rightOnlyAct")));
                break;
            case FILE_LEFT_NEWER:
                toolTip->show(getDescription(cmpRes), pos, &GlobalResources::instance().getImage(wxT("leftNewerAct")));
                break;
            case FILE_RIGHT_NEWER:
                toolTip->show(getDescription(cmpRes), pos, &GlobalResources::instance().getImage(wxT("rightNewerAct")));
                break;
            case FILE_DIFFERENT:
                toolTip->show(getDescription(cmpRes), pos, &GlobalResources::instance().getImage(wxT("differentAct")));
                break;
            case FILE_EQUAL:
                toolTip->show(getDescription(cmpRes), pos, &GlobalResources::instance().getImage(wxT("equalAct")));
                break;
            case FILE_DIFFERENT_METADATA:
                toolTip->show(getDescription(cmpRes), pos, &GlobalResources::instance().getImage(wxT("conflictAct")));
                break;
            case FILE_CONFLICT:
                toolTip->show(fsObj->getCatConflict(), pos, &GlobalResources::instance().getImage(wxT("conflictAct")));
                break;
        }
    }
}


void CustomGridMiddle::OnLeftMouseDown(wxMouseEvent& event)
{
    selectionRowBegin = mousePosToRow(event.GetPosition(), &selectionPos);
    event.Skip();
}


void CustomGridMiddle::OnLeftMouseUp(wxMouseEvent& event)
{
    //int selRowEnd = mousePosToCell(event.GetPosition()).first;
    //-> use visibly marked rows instead! with wxWidgets 2.8.12 there is no other way than IsInSelection()
    int selRowEnd = -1;
    if (0 <= selectionRowBegin && selectionRowBegin < GetNumberRows())
    {
        for (int i = selectionRowBegin; i < GetNumberRows() && IsInSelection(i, 0); ++i)
            selRowEnd = i;

        if (selRowEnd == selectionRowBegin)
            for (int i = selectionRowBegin; i >= 0 && IsInSelection(i, 0); --i)
                selRowEnd = i;
    }

    if (0 <= selectionRowBegin && 0 <= selRowEnd)
    {
        switch (selectionPos)
        {
            case BLOCKPOS_CHECK_BOX:
            {
                //create a custom event
                FFSCheckRowsEvent evt(selectionRowBegin, selRowEnd);
                AddPendingEvent(evt);
            }
            break;
            case BLOCKPOS_LEFT:
            {
                //create a custom event
                FFSSyncDirectionEvent evt(selectionRowBegin, selRowEnd, SYNC_DIR_LEFT);
                AddPendingEvent(evt);
            }
            break;
            case BLOCKPOS_MIDDLE:
            {
                //create a custom event
                FFSSyncDirectionEvent evt(selectionRowBegin, selRowEnd, SYNC_DIR_NONE);
                AddPendingEvent(evt);
            }
            break;
            case BLOCKPOS_RIGHT:
            {
                //create a custom event
                FFSSyncDirectionEvent evt(selectionRowBegin, selRowEnd, SYNC_DIR_RIGHT);
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


int CustomGridMiddle::mousePosToRow(wxPoint pos, BlockPosition* block)
{
    if (!getGridDataTableMiddle())
        return 0;

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

            if (row >= 0)
            {
                const FileSystemObject* const fsObj = getGridDataTableMiddle()->getRawData(row);
                if (fsObj != NULL && //if valid row...
                    getGridDataTableMiddle()->syncPreviewIsActive() &&
                    fsObj->getSyncOperation() != SO_EQUAL) //in sync-preview equal files shall be treated as in cmp result, i.e. as full checkbox
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
    }
    return row;
}


void CustomGridMiddle::enableSyncPreview(bool value)
{
    assert(getGridDataTableMiddle());
    getGridDataTableMiddle()->enableSyncPreview(value);

    if (value)
        GetGridColLabelWindow()->SetToolTip(_("Synchronization Preview"));
    else
        GetGridColLabelWindow()->SetToolTip(_("Comparison Result"));
}


void GridCellRendererMiddle::Draw(wxGrid& grid,
                                  wxGridCellAttr& attr,
                                  wxDC& dc,
                                  const wxRect& rect,
                                  int row, int col,
                                  bool isSelected)
{
    //retrieve grid data
    const FileSystemObject* const fsObj = m_gridMiddle.getGridDataTableMiddle() ? m_gridMiddle.getGridDataTableMiddle()->getRawData(row) :  NULL;
    if (fsObj != NULL) //if valid row...
    {
        if (rect.GetWidth() > CHECK_BOX_WIDTH)
        {
            const bool rowIsHighlighted = row == m_gridMiddle.highlightedRow;

            wxRect rectShrinked(rect);

            //clean first block of rect that will receive image of checkbox
            rectShrinked.SetWidth(CHECK_BOX_WIDTH);
            wxGridCellRenderer::Draw(grid, attr, dc, rectShrinked, row, col, isSelected);

            //print checkbox into first block
            rectShrinked.SetX(rect.GetX() + 1);

            //HIGHLIGHTNING (checkbox):
            if (rowIsHighlighted &&
                m_gridMiddle.highlightedPos == CustomGridMiddle::BLOCKPOS_CHECK_BOX)
            {
                if (fsObj->isActive())
                    dc.DrawLabel(wxEmptyString, GlobalResources::instance().getImage(wxT("checkboxTrueFocus")), rectShrinked, wxALIGN_LEFT | wxALIGN_CENTER_VERTICAL);
                else
                    dc.DrawLabel(wxEmptyString, GlobalResources::instance().getImage(wxT("checkboxFalseFocus")), rectShrinked, wxALIGN_LEFT | wxALIGN_CENTER_VERTICAL);
            }
            //default
            else if (fsObj->isActive())
                dc.DrawLabel(wxEmptyString, GlobalResources::instance().getImage(wxT("checkboxTrue")), rectShrinked, wxALIGN_LEFT | wxALIGN_CENTER_VERTICAL);
            else
                dc.DrawLabel(wxEmptyString, GlobalResources::instance().getImage(wxT("checkboxFalse")), rectShrinked, wxALIGN_LEFT | wxALIGN_CENTER_VERTICAL);

            //clean remaining block of rect that will receive image of checkbox/directions
            rectShrinked.SetWidth(rect.GetWidth() - CHECK_BOX_WIDTH);
            rectShrinked.SetX(rect.GetX() + CHECK_BOX_WIDTH);
            wxGridCellRenderer::Draw(grid, attr, dc, rectShrinked, row, col, isSelected);

            //print remaining block
            if (m_gridMiddle.getGridDataTableMiddle()->syncPreviewIsActive()) //synchronization preview
            {
                //print sync direction into second block

                //HIGHLIGHTNING (sync direction):
                if (rowIsHighlighted &&
                    m_gridMiddle.highlightedPos != CustomGridMiddle::BLOCKPOS_CHECK_BOX) //don't allow changing direction for "=="-files
                    switch (m_gridMiddle.highlightedPos)
                    {
                        case CustomGridMiddle::BLOCKPOS_CHECK_BOX:
                            break;
                        case CustomGridMiddle::BLOCKPOS_LEFT:
                            dc.DrawLabel(wxEmptyString, getSyncOpImage(fsObj->testSyncOperation(true, SYNC_DIR_LEFT)), rectShrinked, wxALIGN_LEFT | wxALIGN_CENTER_VERTICAL);
                            break;
                        case CustomGridMiddle::BLOCKPOS_MIDDLE:
                            dc.DrawLabel(wxEmptyString, getSyncOpImage(fsObj->testSyncOperation(true, SYNC_DIR_NONE)), rectShrinked, wxALIGN_CENTER | wxALIGN_CENTER_VERTICAL);
                            break;
                        case CustomGridMiddle::BLOCKPOS_RIGHT:
                            dc.DrawLabel(wxEmptyString, getSyncOpImage(fsObj->testSyncOperation(true, SYNC_DIR_RIGHT)), rectShrinked, wxALIGN_RIGHT | wxALIGN_CENTER_VERTICAL);
                            break;
                    }
                else //default
                {
                    const wxBitmap& syncOpIcon = getSyncOpImage(fsObj->getSyncOperation());
                    dc.DrawLabel(wxEmptyString, syncOpIcon, rectShrinked, wxALIGN_CENTER | wxALIGN_CENTER_VERTICAL);
                }
            }
            else //comparison results view
            {
                switch (fsObj->getCategory())
                {
                    case FILE_LEFT_SIDE_ONLY:
                        dc.DrawLabel(wxEmptyString, GlobalResources::instance().getImage(wxT("leftOnlySmall")), rectShrinked, wxALIGN_CENTER | wxALIGN_CENTER_VERTICAL);
                        break;
                    case FILE_RIGHT_SIDE_ONLY:
                        dc.DrawLabel(wxEmptyString, GlobalResources::instance().getImage(wxT("rightOnlySmall")), rectShrinked, wxALIGN_CENTER | wxALIGN_CENTER_VERTICAL);
                        break;
                    case FILE_LEFT_NEWER:
                        dc.DrawLabel(wxEmptyString, GlobalResources::instance().getImage(wxT("leftNewerSmall")), rectShrinked, wxALIGN_CENTER | wxALIGN_CENTER_VERTICAL);
                        break;
                    case FILE_RIGHT_NEWER:
                        dc.DrawLabel(wxEmptyString, GlobalResources::instance().getImage(wxT("rightNewerSmall")), rectShrinked, wxALIGN_CENTER | wxALIGN_CENTER_VERTICAL);
                        break;
                    case FILE_DIFFERENT:
                        dc.DrawLabel(wxEmptyString, GlobalResources::instance().getImage(wxT("differentSmall")), rectShrinked, wxALIGN_CENTER | wxALIGN_CENTER_VERTICAL);
                        break;
                    case FILE_EQUAL:
                        dc.DrawLabel(wxEmptyString, GlobalResources::instance().getImage(wxT("equalSmall")), rectShrinked, wxALIGN_CENTER | wxALIGN_CENTER_VERTICAL);
                        break;
                    case FILE_CONFLICT:
                    case FILE_DIFFERENT_METADATA:
                        dc.DrawLabel(wxEmptyString, GlobalResources::instance().getImage(wxT("conflictSmall")), rectShrinked, wxALIGN_CENTER | wxALIGN_CENTER_VERTICAL);
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

    if (!getGridDataTableMiddle())
        return;

    const wxRect rect(GetColLeft(col), 0, GetColWidth(col), GetColLabelSize());

    if (getGridDataTableMiddle()->syncPreviewIsActive())
        dc.DrawLabel(wxEmptyString, GlobalResources::instance().getImage(wxT("syncViewSmall")), rect, wxALIGN_CENTER_HORIZONTAL | wxALIGN_CENTER_VERTICAL);
    else
        dc.DrawLabel(wxEmptyString, GlobalResources::instance().getImage(wxT("cmpViewSmall")), rect, wxALIGN_CENTER_HORIZONTAL | wxALIGN_CENTER_VERTICAL);
}


const wxBitmap& zen::getSyncOpImage(SyncOperation syncOp)
{
    switch (syncOp) //evaluate comparison result and sync direction
    {
        case SO_CREATE_NEW_LEFT:
            return GlobalResources::instance().getImage(wxT("createLeftSmall"));
        case SO_CREATE_NEW_RIGHT:
            return GlobalResources::instance().getImage(wxT("createRightSmall"));
        case SO_DELETE_LEFT:
            return GlobalResources::instance().getImage(wxT("deleteLeftSmall"));
        case SO_DELETE_RIGHT:
            return GlobalResources::instance().getImage(wxT("deleteRightSmall"));
        case SO_OVERWRITE_RIGHT:
        case SO_COPY_METADATA_TO_RIGHT:
            return GlobalResources::instance().getImage(wxT("syncDirRightSmall"));
        case SO_OVERWRITE_LEFT:
        case SO_COPY_METADATA_TO_LEFT:
            return GlobalResources::instance().getImage(wxT("syncDirLeftSmall"));
        case SO_DO_NOTHING:
            return GlobalResources::instance().getImage(wxT("syncDirNoneSmall"));
        case SO_EQUAL:
            return GlobalResources::instance().getImage(wxT("equalSmall"));
        case SO_UNRESOLVED_CONFLICT:
            return GlobalResources::instance().getImage(wxT("conflictSmall"));
    }

    return wxNullBitmap; //dummy
}

