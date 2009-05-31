#ifndef CUSTOMGRID_H_INCLUDED
#define CUSTOMGRID_H_INCLUDED

#include <vector>
#include <wx/grid.h>
#include "../structures.h"
#include "processXml.h"


class CustomGridTable;
class CustomGridTableRim;
class CustomGridTableMiddle;
class GridCellRendererMiddle;

namespace FreeFileSync
{
    class GridView;
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

    virtual void DrawColLabel(wxDC& dc, int col);

    std::set<int> getAllSelectedRows() const;

    //set sort direction indicator on UI
    void setSortMarker(const int sortColumn, const wxBitmap* bitmap = &wxNullBitmap);

    bool isLeadGrid() const;

protected:
    CustomGrid* m_gridLeft;
    CustomGrid* m_gridMiddle;
    CustomGrid* m_gridRight;

private:
    void onGridAccess(wxEvent& event);
    void adjustGridHeights(wxEvent& event);

    bool isLeading; //identify grid that has user focus
    int currentSortColumn;
    const wxBitmap* sortMarker;
};


//############## SPECIALIZATIONS ###################
class CustomGridRim : public CustomGrid
{
public:
    CustomGridRim(wxWindow *parent,
                  wxWindowID id,
                  const wxPoint& pos,
                  const wxSize& size,
                  long style,
                  const wxString& name) :
            CustomGrid(parent, id, pos, size, style, name),
            gridDataTable(NULL) {}

    ~CustomGridRim() {}

    void initSettings(const bool showFileIcons,  //workaround: though this coding better belongs into a constructor
                      CustomGrid* gridLeft,      //this is not possible due to source code generation (information not available at time of construction)
                      CustomGrid* gridRight,
                      CustomGrid* gridMiddle,
                      const FreeFileSync::GridView* gridDataView);

    //notify wxGrid that underlying table size has changed
    void updateGridSizes();

    //set visibility, position and width of columns
    static xmlAccess::ColumnAttributes getDefaultColumnAttributes();
    xmlAccess::ColumnAttributes getColumnAttributes();
    void setColumnAttributes(const xmlAccess::ColumnAttributes& attr);

    xmlAccess::ColumnTypes getTypeAtPos(unsigned pos) const;
    static wxString getTypeName(xmlAccess::ColumnTypes colType);

    virtual void enableFileIcons(const bool value) = 0;

protected:
    CustomGridTableRim* gridDataTable;

private:
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

    ~CustomGridLeft() {}

    virtual bool CreateGrid(int numRows, int numCols, wxGrid::wxGridSelectionModes selmode = wxGrid::wxGridSelectCells);

    virtual void enableFileIcons(const bool value);

    //this method is called when grid view changes: useful for parallel updating of multiple grids
    virtual void DoPrepareDC(wxDC& dc);
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

    ~CustomGridRight() {}

    virtual bool CreateGrid(int numRows, int numCols, wxGrid::wxGridSelectionModes selmode = wxGrid::wxGridSelectCells);

    virtual void enableFileIcons(const bool value);

    //this method is called when grid view changes: useful for parallel updating of multiple grids
    virtual void DoPrepareDC(wxDC& dc);
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

    ~CustomGridMiddle() {}

    virtual bool CreateGrid(int numRows, int numCols, wxGrid::wxGridSelectionModes selmode = wxGrid::wxGridSelectCells);

    void initSettings(CustomGrid* gridLeft,  //workaround: though this coding better belongs into a constructor
                      CustomGrid* gridRight, //this is not possible due to source code generation (information not available at time of construction)
                      CustomGrid* gridMiddle,
                      const FreeFileSync::GridView* gridDataView);

    void enableSyncPreview(bool value);

    //notify wxGrid that underlying table size has changed
    void updateGridSizes();

#ifdef FFS_WIN //get rid of scrollbars; Windows: overwrite virtual method
    virtual void SetScrollbar(int orientation, int position, int thumbSize, int range, bool refresh = true);
#endif

    //this method is called when grid view changes: useful for parallel updating of multiple grids
    virtual void DoPrepareDC(wxDC& dc);

private:
    void OnMouseMovement(wxMouseEvent& event);
    void OnLeaveWindow(wxMouseEvent& event);
    void OnLeftMouseDown(wxMouseEvent& event);
    void OnLeftMouseUp(wxMouseEvent& event);

    //small helper methods
    void RefreshRow(int row);
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
