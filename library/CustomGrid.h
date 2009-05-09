#ifndef CUSTOMGRID_H_INCLUDED
#define CUSTOMGRID_H_INCLUDED

#include <vector>
#include <wx/grid.h>
#include "../structures.h"
#include "processXml.h"


class CustomGridTable;
class CustomGridTableRim;
class CustomGridTableMiddle;

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

    //notify wxGrid that underlying table size has changed
    void updateGridSizes();

    //set visibility, position and width of columns
    static xmlAccess::ColumnAttributes getDefaultColumnAttributes();
    xmlAccess::ColumnAttributes getColumnAttributes();
    void setColumnAttributes(const xmlAccess::ColumnAttributes& attr);

    xmlAccess::ColumnTypes getTypeAtPos(unsigned pos) const;

    static wxString getTypeName(xmlAccess::ColumnTypes colType);

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

    void initSettings(const bool showFileIcons,  //workaround: though this coding better belongs into a constructor
                      CustomGrid* gridLeft,      //this is not possible due to source code generation (information not available at time of construction)
                      CustomGrid* gridRight,
                      CustomGrid* gridMiddle,
                      FreeFileSync::GridView* gridDataView);

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

    void initSettings(const bool showFileIcons,  //workaround: though this coding better belongs into a constructor
                      CustomGrid* gridLeft,      //this is not possible due to source code generation (information not available at time of construction)
                      CustomGrid* gridRight,
                      CustomGrid* gridMiddle,
                      FreeFileSync::GridView* gridDataView);

    //this method is called when grid view changes: useful for parallel updating of multiple grids
    virtual void DoPrepareDC(wxDC& dc);
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

    virtual bool CreateGrid(int numRows, int numCols, wxGrid::wxGridSelectionModes selmode = wxGrid::wxGridSelectCells);

    void initSettings(CustomGrid* gridLeft,  //workaround: though this coding better belongs into a constructor
                      CustomGrid* gridRight, //this is not possible due to source code generation (information not available at time of construction)
                      CustomGrid* gridMiddle,
                      FreeFileSync::GridView* gridDataView);

    //notify wxGrid that underlying table size has changed
    void updateGridSizes();

#ifdef FFS_WIN //get rid of scrollbars; Windows: overwrite virtual method
    virtual void SetScrollbar(int orientation, int position, int thumbSize, int range, bool refresh = true);
#endif

    //this method is called when grid view changes: useful for parallel updating of multiple grids
    virtual void DoPrepareDC(wxDC& dc);

private:
    CustomGridTableMiddle* gridDataTable;
};

#endif // CUSTOMGRID_H_INCLUDED
