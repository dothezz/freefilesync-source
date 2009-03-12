#ifndef CUSTOMGRID_H_INCLUDED
#define CUSTOMGRID_H_INCLUDED

#include <vector>
#include <wx/grid.h>
#include "../FreeFileSync.h"
#include "processXml.h"

using namespace FreeFileSync;


class CustomGridTable;
class CustomGridTableMiddle;
//##################################################################################

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

    //overwrite virtual method to finally get rid of the scrollbars
    virtual void SetScrollbar(int orientation, int position, int thumbSize, int range, bool refresh = true);

    virtual void DrawColLabel(wxDC& dc, int col);

    void initSettings(bool enableScrollbars,
                      CustomGrid* gridLeft,
                      CustomGrid* gridRight,
                      CustomGrid* gridMiddle,
                      GridView* gridRefUI,
                      FileCompareResult* gridData);

    //notify wxGrid that underlying table size has changed
    void updateGridSizes();

    //set sort direction indicator on UI
    void setSortMarker(const int sortColumn, const wxBitmap* bitmap = &wxNullBitmap);

    //set visibility, position and width of columns
    static xmlAccess::ColumnAttributes getDefaultColumnAttributes();
    xmlAccess::ColumnAttributes getColumnAttributes();
    void setColumnAttributes(const xmlAccess::ColumnAttributes& attr);

    xmlAccess::ColumnTypes getTypeAtPos(unsigned pos) const;

    static wxString getTypeName(xmlAccess::ColumnTypes colType);

    void setLeadGrid(const wxGrid* newLead);

protected:
    //set visibility, position and width of columns
    xmlAccess::ColumnAttributes columnSettings;

    void adjustGridHeights();

    const wxGrid* leadGrid; //grid that has user focus
    bool scrollbarsEnabled;
    CustomGrid* m_gridLeft;
    CustomGrid* m_gridRight;
    CustomGrid* m_gridMiddle;

    CustomGridTable* gridDataTable;

    int currentSortColumn;
    const wxBitmap* sortMarker;
};


class CustomGridLeft : public CustomGrid
{
public:
    CustomGridLeft(wxWindow *parent,
                   wxWindowID id,
                   const wxPoint& pos   = wxDefaultPosition,
                   const wxSize& size   = wxDefaultSize,
                   long style           = wxWANTS_CHARS,
                   const wxString& name = wxGridNameStr);

    ~CustomGridLeft() {}

    //this method is called when grid view changes: useful for parallel updating of multiple grids
    virtual void DoPrepareDC(wxDC& dc);

    virtual bool CreateGrid(int numRows, int numCols, wxGrid::wxGridSelectionModes selmode = wxGrid::wxGridSelectCells);
};


class CustomGridMiddle : public CustomGrid
{
public:
    CustomGridMiddle(wxWindow *parent,
                     wxWindowID id,
                     const wxPoint& pos   = wxDefaultPosition,
                     const wxSize& size   = wxDefaultSize,
                     long style           = wxWANTS_CHARS,
                     const wxString& name = wxGridNameStr);

    ~CustomGridMiddle() {}

    //this method is called when grid view changes: useful for parallel updating of multiple grids
    virtual void DoPrepareDC(wxDC& dc);

    virtual bool CreateGrid(int numRows, int numCols, wxGrid::wxGridSelectionModes selmode = wxGrid::wxGridSelectCells);

private:

    class GridCellRendererAddCheckbox : public wxGridCellStringRenderer
    {
    public:
        GridCellRendererAddCheckbox(CustomGridTableMiddle* gridDataTable) : m_gridDataTable(gridDataTable) {};


        virtual void Draw(wxGrid& grid,
                          wxGridCellAttr& attr,
                          wxDC& dc,
                          const wxRect& rect,
                          int row, int col,
                          bool isSelected);

    private:
        CustomGridTableMiddle* m_gridDataTable;
    };
};


class CustomGridRight : public CustomGrid
{
public:
    CustomGridRight(wxWindow *parent,
                    wxWindowID id,
                    const wxPoint& pos   = wxDefaultPosition,
                    const wxSize& size   = wxDefaultSize,
                    long style           = wxWANTS_CHARS,
                    const wxString& name = wxGridNameStr);

    ~CustomGridRight() {}

    //this method is called when grid view changes: useful for parallel updating of multiple grids
    virtual void DoPrepareDC(wxDC& dc);

    virtual bool CreateGrid(int numRows, int numCols, wxGrid::wxGridSelectionModes selmode = wxGrid::wxGridSelectCells);
};

#endif // CUSTOMGRID_H_INCLUDED
