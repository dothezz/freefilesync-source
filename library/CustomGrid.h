#ifndef CUSTOMGRID_H_INCLUDED
#define CUSTOMGRID_H_INCLUDED

#include <vector>
#include <wx/grid.h>
#include "processXml.h"
#include <map>
#include <memory>
#include <set>

class CustomGridTableRim;
class CustomGridTableLeft;
class CustomGridTableRight;
class CustomGridTableMiddle;
class GridCellRendererMiddle;
class wxTimer;
class CustomTooltip;
class CustomGridRim;
class CustomGridLeft;
class CustomGridMiddle;
class CustomGridRight;


namespace FreeFileSync
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
    CustomGrid(wxWindow *parent,
               wxWindowID id,
               const wxPoint& pos   = wxDefaultPosition,
               const wxSize& size   = wxDefaultSize,
               long style           = wxWANTS_CHARS,
               const wxString& name = wxGridNameStr);

    virtual ~CustomGrid() {}

    void initSettings(CustomGridLeft*   gridLeft,
                      CustomGridMiddle* gridMiddle,
                      CustomGridRight*  gridRight,
                      const FreeFileSync::GridView* gridDataView);

    std::set<unsigned int> getAllSelectedRows() const;

    //set sort direction indicator on UI
    typedef int SortColumn;

    enum SortDirection
    {
        ASCENDING,
        DESCENDING,
    };

    typedef std::pair<SortColumn, SortDirection> SortMarker;
    void setSortMarker(SortMarker marker);

    bool isLeadGrid() const;

protected:
    void RefreshCell(int row, int col);

    virtual void DrawColLabel(wxDC& dc, int col);

private:
    virtual void setGridDataTable(const FreeFileSync::GridView* gridDataView) = 0;

//this method is called when grid view changes: useful for parallel updating of multiple grids
    virtual void DoPrepareDC(wxDC& dc);

    virtual void alignOtherGrids(CustomGrid* gridLeft, CustomGrid* gridMiddle, CustomGrid* gridRight) = 0;

    void onGridAccess(wxEvent& event);
    void adjustGridHeights(wxEvent& event);

    CustomGrid* m_gridLeft;
    CustomGrid* m_gridMiddle;
    CustomGrid* m_gridRight;

    bool isLeading; //identify grid that has user focus

    SortMarker m_marker;
};


template <bool showFileIcons>
class GridCellRenderer;


//-----------------------------------------------------------
#ifdef FFS_WIN
class IconUpdater : private wxEvtHandler //update file icons periodically: use SINGLE instance to coordinate left and right grid at once
{
public:
    IconUpdater(CustomGridLeft* leftGrid, CustomGridRight* rightGrid);

private:
    void loadIconsAsynchronously(wxEvent& event); //loads all (not yet) drawn icons

    CustomGridRim* m_leftGrid;
    CustomGridRim* m_rightGrid;

    std::auto_ptr<wxTimer> m_timer; //user timer event to periodically update icons: better than idle event because also active when scrolling! :)
};
#endif


//############## SPECIALIZATIONS ###################
class CustomGridRim : public CustomGrid
{
    friend class IconUpdater;
    template <bool showFileIcons>
    friend class GridCellRenderer;

public:
    CustomGridRim(wxWindow *parent,
                  wxWindowID id,
                  const wxPoint& pos,
                  const wxSize& size,
                  long style,
                  const wxString& name);

    //notify wxGrid that underlying table size has changed
    void updateGridSizes();

    //set visibility, position and width of columns
    static xmlAccess::ColumnAttributes getDefaultColumnAttributes();
    xmlAccess::ColumnAttributes getColumnAttributes();
    void setColumnAttributes(const xmlAccess::ColumnAttributes& attr);

    xmlAccess::ColumnTypes getTypeAtPos(unsigned pos) const;
    static wxString getTypeName(xmlAccess::ColumnTypes colType);

#ifdef FFS_WIN
    void enableFileIcons(const bool value);
#endif

private:
    CustomGridTableRim* getGridDataTable();
    virtual const CustomGridTableRim* getGridDataTable() const = 0;

#ifdef FFS_WIN
    //asynchronous icon loading
    void getIconsToBeLoaded(std::vector<Zstring>& newLoad); //loads all (not yet) drawn icons

    typedef unsigned int FromRow;
    typedef unsigned int ToRow;
    typedef std::pair<FromRow, ToRow> VisibleRowRange;
    VisibleRowRange getVisibleRows();


    typedef unsigned int RowNumber;
    typedef bool         IconLoaded;
    typedef std::map<RowNumber, IconLoaded> LoadSuccess;
    LoadSuccess loadIconSuccess; //save status of last icon load when drawing on GUI

    bool fileIconsAreEnabled;
#endif

    xmlAccess::ColumnAttributes columnSettings; //set visibility, position and width of columns
};


class CustomGridLeft : public CustomGridRim
{
public:
    CustomGridLeft(wxWindow *parent,
                   wxWindowID id,
                   const wxPoint& pos   = wxDefaultPosition,
                   const wxSize& size   = wxDefaultSize,
                   long style           = wxWANTS_CHARS,
                   const wxString& name = wxGridNameStr);

    virtual bool CreateGrid(int numRows, int numCols, wxGrid::wxGridSelectionModes selmode = wxGrid::wxGridSelectCells);

private:
    virtual void setGridDataTable(const FreeFileSync::GridView* gridDataView);
    virtual const CustomGridTableRim* getGridDataTable() const;

    //this method is called when grid view changes: useful for parallel updating of multiple grids
    virtual void alignOtherGrids(CustomGrid* gridLeft, CustomGrid* gridMiddle, CustomGrid* gridRight);

    CustomGridTableLeft* gridDataTable;
};


class CustomGridRight : public CustomGridRim
{
public:
    CustomGridRight(wxWindow *parent,
                    wxWindowID id,
                    const wxPoint& pos   = wxDefaultPosition,
                    const wxSize& size   = wxDefaultSize,
                    long style           = wxWANTS_CHARS,
                    const wxString& name = wxGridNameStr);

    virtual bool CreateGrid(int numRows, int numCols, wxGrid::wxGridSelectionModes selmode = wxGrid::wxGridSelectCells);

private:
    virtual void setGridDataTable(const FreeFileSync::GridView* gridDataView);
    virtual const CustomGridTableRim* getGridDataTable() const;

    //this method is called when grid view changes: useful for parallel updating of multiple grids
    virtual void alignOtherGrids(CustomGrid* gridLeft, CustomGrid* gridMiddle, CustomGrid* gridRight);

    CustomGridTableRight* gridDataTable;
};


class CustomGridMiddle : public CustomGrid
{
    friend class GridCellRendererMiddle;

public:
    CustomGridMiddle(wxWindow *parent,
                     wxWindowID id,
                     const wxPoint& pos   = wxDefaultPosition,
                     const wxSize& size   = wxDefaultSize,
                     long style           = wxWANTS_CHARS,
                     const wxString& name = wxGridNameStr);

    virtual bool CreateGrid(int numRows, int numCols, wxGrid::wxGridSelectionModes selmode = wxGrid::wxGridSelectCells);

    void enableSyncPreview(bool value);

    //notify wxGrid that underlying table size has changed
    void updateGridSizes();

private:
#ifdef FFS_WIN //get rid of scrollbars; Windows: overwrite virtual method
    virtual void SetScrollbar(int orientation, int position, int thumbSize, int range, bool refresh = true);
#endif

    virtual void setGridDataTable(const FreeFileSync::GridView* gridDataView);

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

    CustomGridTableMiddle* gridDataTable;

    std::auto_ptr<CustomTooltip> toolTip;
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
    FFSSyncDirectionEvent(const int from, const int to, const FreeFileSync::SyncDirection dir) :
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
    const FreeFileSync::SyncDirection direction;
};

typedef void (wxEvtHandler::*FFSSyncDirectionEventFunction)(FFSSyncDirectionEvent&);

#define FFSSyncDirectionEventHandler(func) \
    (wxObjectEventFunction)(wxEventFunction)wxStaticCastEvent(FFSSyncDirectionEventFunction, &func)


#endif // CUSTOMGRID_H_INCLUDED
