#ifndef CUSTOMGRID_H_INCLUDED
#define CUSTOMGRID_H_INCLUDED

#include <vector>
#include <wx/grid.h>
#include "../FreeFileSync.h"

using namespace std;

class CustomGridTableBase;

//##################################################################################

extern int leadingPanel;

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

    bool CreateGrid(int numRows, int numCols, wxGrid::wxGridSelectionModes selmode = wxGrid::wxGridSelectCells);

    void deactivateScrollbars();

    //overwrite virtual method to finally get rid of the scrollbars
    void SetScrollbar(int orientation, int position, int thumbSize, int range, bool refresh = true);

    //this method is called when grid view changes: useful for parallel updating of multiple grids
    void DoPrepareDC(wxDC& dc);

    void setScrollFriends(CustomGrid* grid1, CustomGrid* grid2, CustomGrid* grid3);

    void setGridDataTable(GridView* gridRefUI, FileCompareResult* gridData);

    void updateGridSizes();

    //set sort direction indicator on UI
    void setSortMarker(const int sortColumn, const wxBitmap* bitmap = &wxNullBitmap);

    void DrawColLabel(wxDC& dc, int col);

private:
    void adjustGridHeights();

    bool scrollbarsEnabled;

    CustomGrid* m_grid1;
    CustomGrid* m_grid2;
    CustomGrid* m_grid3;

    CustomGridTableBase* gridDataTable;

    int currentSortColumn;
    const wxBitmap* sortMarker;
};

#endif // CUSTOMGRID_H_INCLUDED
