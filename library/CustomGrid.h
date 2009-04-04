#ifndef CUSTOMGRID_H_INCLUDED
#define CUSTOMGRID_H_INCLUDED

#include <vector>
#include <wx/grid.h>
#include "../FreeFileSync.h"
#include "processXml.h"

using namespace FreeFileSync;


class CustomGridTable;
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

    void initSettings(const bool enableScrollbars,
                      const bool showFileIcons,
                      CustomGrid* gridLeft,
                      CustomGrid* gridRight,
                      CustomGrid* gridMiddle,
                      GridView* gridRefUI,
                      FileCompareResult* gridData);

    virtual void initGridRenderer(const bool showFileIcons) = 0;

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

    const wxGrid* getLeadGrid();
    bool isLeadGrid();

protected:
    void onGridAccess(wxEvent& event);
    void adjustGridHeights(wxEvent& event);

    xmlAccess::ColumnAttributes columnSettings; //set visibility, position and width of columns

    const wxGrid* leadGrid; //grid that has user focus
    bool scrollbarsEnabled;
    CustomGrid* m_gridLeft;
    CustomGrid* m_gridMiddle;
    CustomGrid* m_gridRight;

    CustomGridTable* gridDataTable;

    int currentSortColumn;
    const wxBitmap* sortMarker;
};

//############## SPECIALIZATIONS ###################

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

    virtual void initGridRenderer(const bool showFileIcons);
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

    virtual void initGridRenderer(const bool showFileIcons) {}
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

    virtual void initGridRenderer(const bool showFileIcons);
};

#endif // CUSTOMGRID_H_INCLUDED
