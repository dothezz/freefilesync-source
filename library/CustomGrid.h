#ifndef CUSTOMGRID_H_INCLUDED
#define CUSTOMGRID_H_INCLUDED

#include <vector>
#include <wx/grid.h>
#include "../FreeFileSync.h"

using namespace FreeFileSync;


class CustomGridTableBase;

//##################################################################################

extern const wxGrid* leadGrid; //this global variable is not very nice...

class CustomGrid : public wxGrid
{
public:
    CustomGrid(wxWindow *parent,
               wxWindowID id,
               const wxPoint& pos   = wxDefaultPosition,
               const wxSize& size   = wxDefaultSize,
               long style           = wxWANTS_CHARS,
               const wxString& name = wxGridNameStr);

    ~CustomGrid();

    virtual bool CreateGrid(int numRows, int numCols, wxGrid::wxGridSelectionModes selmode = wxGrid::wxGridSelectCells);
    //overwrite virtual method to finally get rid of the scrollbars
    virtual void SetScrollbar(int orientation, int position, int thumbSize, int range, bool refresh = true);
    //this method is called when grid view changes: useful for parallel updating of multiple grids
    virtual void DoPrepareDC(wxDC& dc);
    virtual void DrawColLabel(wxDC& dc, int col);

    void deactivateScrollbars();
    void setScrollFriends(CustomGrid* gridLeft, CustomGrid* gridRight, CustomGrid* gridMiddle);
    void setGridDataTable(GridView* gridRefUI, FileCompareResult* gridData);
    void updateGridSizes();
    //set sort direction indicator on UI
    void setSortMarker(const int sortColumn, const wxBitmap* bitmap = &wxNullBitmap);

private:
    void adjustGridHeights();

    bool scrollbarsEnabled;

    CustomGrid* m_gridLeft;
    CustomGrid* m_gridRight;
    CustomGrid* m_gridMiddle;

    CustomGridTableBase* gridDataTable;

    int currentSortColumn;
    const wxBitmap* sortMarker;
};

#endif // CUSTOMGRID_H_INCLUDED
