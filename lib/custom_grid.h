// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) 2008-2011 ZenJu (zhnmju123 AT gmx.de)                    *
// **************************************************************************

#ifndef CUSTOMGRID_H_INCLUDED
#define CUSTOMGRID_H_INCLUDED

#include <vector>
#include <wx/grid.h>
#include <wx+/tooltip.h>
#include "process_xml.h"
#include <memory>
#include <set>
#include "../file_hierarchy.h"
#include "icon_buffer.h"


class CustomGridTable;
class CustomGridTableRim;
class CustomGridTableLeft;
class CustomGridTableRight;
class CustomGridTableMiddle;
class GridCellRendererMiddle;
class wxTimer;
class CustomGridRim;
class CustomGridLeft;
class CustomGridMiddle;
class CustomGridRight;


namespace zen
{
class GridView;

const wxBitmap& getSyncOpImage(SyncOperation syncOp);
}
//##################################################################################

/*
class hierarchy:
                        CustomGrid
                            /|\
                 ____________|____________
                |                        |
          CustomGridRim                  |
               /|\                       |
        ________|_______                 |
       |                |                |
CustomGridLeft  CustomGridRight  CustomGridMiddle
*/

class CustomGrid : public wxGrid
{
public:
    CustomGrid(wxWindow* parent,
               wxWindowID id,
               const wxPoint& pos   = wxDefaultPosition,
               const wxSize& size   = wxDefaultSize,
               long style           = wxWANTS_CHARS,
               const wxString& name = wxGridNameStr);

    virtual ~CustomGrid() {}

    virtual void initSettings(CustomGridLeft*   gridLeft,  //create connection with zen::GridView
                              CustomGridMiddle* gridMiddle,
                              CustomGridRight*  gridRight,
                              const zen::GridView* gridDataView);

    void release(); //release connection to zen::GridView

    std::set<size_t> getAllSelectedRows() const;

    //set sort direction indicator on UI
    typedef int SortColumn;

    //notify wxGrid that underlying table size has changed
    virtual void updateGridSizes();

    enum SortDirection
    {
        ASCENDING,
        DESCENDING
    };
    typedef std::pair<SortColumn, SortDirection> SortMarker;
    void setSortMarker(SortMarker marker);

    bool isLeadGrid() const;

    void setIconManager(const std::shared_ptr<zen::IconBuffer>& iconBuffer);

protected:
    void RefreshCell(int row, int col);
    virtual void DrawColLabel(wxDC& dc, int col);
    std::pair<int, int> mousePosToCell(wxPoint pos); //returns (row/column) pair

    virtual CustomGridTable* getGridDataTable() const = 0;

private:
    void onGridAccess(wxEvent& event);

    //this method is called when grid view changes: useful for parallel updating of multiple grids
    void OnPaintGrid(wxEvent& event);

    virtual void alignOtherGrids(CustomGrid* gridLeft, CustomGrid* gridMiddle, CustomGrid* gridRight) = 0;

    void adjustGridHeights(wxEvent& event);
    virtual void enableFileIcons(const std::shared_ptr<zen::IconBuffer>& iconBuffer) = 0;

    CustomGrid* m_gridLeft;
    CustomGrid* m_gridMiddle;
    CustomGrid* m_gridRight;

    bool isLeading; //identify grid that has user focus

    SortMarker m_marker;
};


class GridCellRenderer;


//-----------------------------------------------------------
class IconUpdater : private wxEvtHandler //update file icons periodically: use SINGLE instance to coordinate left and right grid at once
{
public:
    IconUpdater(CustomGridLeft* leftGrid, CustomGridRight* rightGrid);
    ~IconUpdater();

private:
    void loadIconsAsynchronously(wxEvent& event); //loads all (not yet) drawn icons

    CustomGridRim* m_leftGrid;
    CustomGridRim* m_rightGrid;

    std::unique_ptr<wxTimer> m_timer; //user timer event to periodically update icons: better than idle event because also active when scrolling! :)
};


//############## SPECIALIZATIONS ###################
class CustomGridRim : public CustomGrid
{
    friend class IconUpdater;
    friend class GridCellRenderer;

public:
    CustomGridRim(wxWindow* parent,
                  wxWindowID id,
                  const wxPoint& pos,
                  const wxSize& size,
                  long style,
                  const wxString& name);

    //set visibility, position and width of columns
    static xmlAccess::ColumnAttributes getDefaultColumnAttributes();
    xmlAccess::ColumnAttributes getColumnAttributes();
    void setColumnAttributes(const xmlAccess::ColumnAttributes& attr);

    xmlAccess::ColumnTypes getTypeAtPos(size_t pos) const;
    static wxString getTypeName(xmlAccess::ColumnTypes colType);

    void autoSizeColumns();        //performance optimized column resizer

    virtual void updateGridSizes();

protected:
    template <zen::SelectedSide side>
    void setTooltip(const wxMouseEvent& event);

    void setOtherGrid(CustomGridRim* other); //call during initialization!

private:
    CustomGridTableRim* getGridDataTableRim() const;
    virtual void enableFileIcons(const std::shared_ptr<zen::IconBuffer>& iconBuffer);

    void OnResizeColumn(wxGridSizeEvent& event);

    //this method is called when grid view changes: useful for parallel updating of multiple grids
    virtual void alignOtherGrids(CustomGrid* gridLeft, CustomGrid* gridMiddle, CustomGrid* gridRight);

    //asynchronous icon loading
    void getIconsToBeLoaded(std::vector<Zstring>& newLoad); //loads all (not yet) drawn icons

    typedef size_t RowBegin;
    typedef size_t RowEnd;
    std::pair<RowBegin, RowEnd> getVisibleRows(); //return [first, last) number pair

    typedef size_t RowNumber;
    typedef std::set<RowNumber> FailedIconLoad;
    FailedIconLoad failedLoads; //save status of last icon load when drawing on GUI

    std::shared_ptr<zen::IconBuffer> iconBuffer_;

    xmlAccess::ColumnAttributes columnSettings; //set visibility, position and width of columns
    CustomGridRim* otherGrid; //sibling grid on other side
};


class CustomGridLeft : public CustomGridRim
{
public:
    CustomGridLeft(wxWindow* parent,
                   wxWindowID id,
                   const wxPoint& pos   = wxDefaultPosition,
                   const wxSize& size   = wxDefaultSize,
                   long style           = wxWANTS_CHARS,
                   const wxString& name = wxGridNameStr);

    virtual bool CreateGrid(int numRows, int numCols, wxGrid::wxGridSelectionModes selmode = wxGrid::wxGridSelectCells);

    virtual void initSettings(CustomGridLeft*   gridLeft,  //create connection with zen::GridView
                              CustomGridMiddle* gridMiddle,
                              CustomGridRight*  gridRight,
                              const zen::GridView* gridDataView);

private:
    void OnMouseMovement(wxMouseEvent& event);
    virtual CustomGridTable* getGridDataTable() const;
};


class CustomGridRight : public CustomGridRim
{
public:
    CustomGridRight(wxWindow* parent,
                    wxWindowID id,
                    const wxPoint& pos   = wxDefaultPosition,
                    const wxSize& size   = wxDefaultSize,
                    long style           = wxWANTS_CHARS,
                    const wxString& name = wxGridNameStr);

    virtual bool CreateGrid(int numRows, int numCols, wxGrid::wxGridSelectionModes selmode = wxGrid::wxGridSelectCells);

    virtual void initSettings(CustomGridLeft*   gridLeft,  //create connection with zen::GridView
                              CustomGridMiddle* gridMiddle,
                              CustomGridRight*  gridRight,
                              const zen::GridView* gridDataView);

private:
    void OnMouseMovement(wxMouseEvent& event);
    virtual CustomGridTable* getGridDataTable() const;
};


class CustomGridMiddle : public CustomGrid
{
    friend class GridCellRendererMiddle;

public:
    CustomGridMiddle(wxWindow* parent,
                     wxWindowID id,
                     const wxPoint& pos   = wxDefaultPosition,
                     const wxSize& size   = wxDefaultSize,
                     long style           = wxWANTS_CHARS,
                     const wxString& name = wxGridNameStr);

    ~CustomGridMiddle();

    virtual bool CreateGrid(int numRows, int numCols, wxGrid::wxGridSelectionModes selmode = wxGrid::wxGridSelectCells);

    virtual void initSettings(CustomGridLeft*   gridLeft,  //create connection with zen::GridView
                              CustomGridMiddle* gridMiddle,
                              CustomGridRight*  gridRight,
                              const zen::GridView* gridDataView);

    void enableSyncPreview(bool value);

private:
    virtual CustomGridTable* getGridDataTable() const;
    CustomGridTableMiddle* getGridDataTableMiddle() const;

    virtual void enableFileIcons(const std::shared_ptr<zen::IconBuffer>& iconBuffer) {};
#ifdef FFS_WIN //get rid of scrollbars; Windows: overwrite virtual method
    virtual void SetScrollbar(int orientation, int position, int thumbSize, int range, bool refresh = true);
#endif

    //this method is called when grid view changes: useful for parallel updating of multiple grids
    virtual void alignOtherGrids(CustomGrid* gridLeft, CustomGrid* gridMiddle, CustomGrid* gridRight);

    virtual void DrawColLabel(wxDC& dc, int col);

    void OnMouseMovement(wxMouseEvent& event);
    void OnLeaveWindow(wxMouseEvent& event);
    void OnLeftMouseDown(wxMouseEvent& event);
    void OnLeftMouseUp(wxMouseEvent& event);

    void showToolTip(int rowNumber, wxPoint pos);

    //small helper methods
    enum BlockPosition //each cell can be divided into four blocks concerning mouse selections
    {
        BLOCKPOS_CHECK_BOX,
        BLOCKPOS_LEFT,
        BLOCKPOS_MIDDLE,
        BLOCKPOS_RIGHT
    };
    int mousePosToRow(const wxPoint pos, BlockPosition* block = NULL);

    //variables for selecting sync direction
    int selectionRowBegin;
    BlockPosition selectionPos;

    //variables for highlightning on mouse-over
    int highlightedRow;
    BlockPosition highlightedPos;

    zen::Tooltip toolTip;
};

//custom events for middle grid:

//--------------------------------------------------------------------------------------------
//(UN-)CHECKING ROWS FROM SYNCHRONIZATION

extern const wxEventType FFS_CHECK_ROWS_EVENT; //define new event type

class FFSCheckRowsEvent : public wxCommandEvent
{
public:
    FFSCheckRowsEvent(const int from, const int to) :
        wxCommandEvent(FFS_CHECK_ROWS_EVENT),
        rowFrom(from),
        rowTo(to) {}

    virtual wxEvent* Clone() const
    {
        return new FFSCheckRowsEvent(rowFrom, rowTo);
    }

    const int rowFrom;
    const int rowTo;
};

typedef void (wxEvtHandler::*FFSCheckRowsEventFunction)(FFSCheckRowsEvent&);

#define FFSCheckRowsEventHandler(func) \
    (wxObjectEventFunction)(wxEventFunction)wxStaticCastEvent(FFSCheckRowsEventFunction, &func)

//--------------------------------------------------------------------------------------------
//SELECTING SYNC DIRECTION

extern const wxEventType FFS_SYNC_DIRECTION_EVENT; //define new event type

class FFSSyncDirectionEvent : public wxCommandEvent
{
public:
    FFSSyncDirectionEvent(const int from, const int to, const zen::SyncDirection dir) :
        wxCommandEvent(FFS_SYNC_DIRECTION_EVENT),
        rowFrom(from),
        rowTo(to),
        direction(dir) {}

    virtual wxEvent* Clone() const
    {
        return new FFSSyncDirectionEvent(rowFrom, rowTo, direction);
    }

    const int rowFrom;
    const int rowTo;
    const zen::SyncDirection direction;
};

typedef void (wxEvtHandler::*FFSSyncDirectionEventFunction)(FFSSyncDirectionEvent&);

#define FFSSyncDirectionEventHandler(func) \
    (wxObjectEventFunction)(wxEventFunction)wxStaticCastEvent(FFSSyncDirectionEventFunction, &func)


#endif // CUSTOMGRID_H_INCLUDED
