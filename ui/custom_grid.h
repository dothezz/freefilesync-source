// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) ZenJu (zhnmju123 AT gmx DOT de) - All Rights Reserved    *
// **************************************************************************

#ifndef CUSTOMGRID_H_INCLUDED
#define CUSTOMGRID_H_INCLUDED

#include <wx+/grid.h>
#include "grid_view.h"
#include "column_attr.h"
#include "../lib/icon_buffer.h"

namespace zen
{
//setup grid to show grid view within three components:
namespace gridview
{
static const size_t COMP_LEFT   = 0;
static const size_t COMP_MIDDLE = 1;
static const size_t COMP_RIGHT  = 2;

void init(Grid& grid, const std::shared_ptr<const GridView>& gridDataView);

std::vector<Grid::ColumnAttribute> convertConfig(const std::vector<ColumnAttributeRim>& attribs); //+ make consistent
std::vector<ColumnAttributeRim>    convertConfig(const std::vector<Grid::ColumnAttribute>& attribs); //

void setSyncPreviewActive(Grid& grid, bool value);

void setupIcons(Grid& grid, bool show, IconBuffer::IconSize sz);

void clearSelection(Grid& grid); //clear all components

//mark rows selected in navigation/compressed tree and navigate to leading object
void setNavigationMarker(Grid& grid,
                         std::vector<const HierarchyObject*>&& markedFiles,      //mark files/symlinks directly within a container
                         std::vector<const HierarchyObject*>&& markedContainer); //mark full container including child-objects
}

wxBitmap getSyncOpImage(SyncOperation syncOp);
wxBitmap getCmpResultImage(CompareFilesResult cmpResult);


//---------- custom events for middle grid ----------

//(UN-)CHECKING ROWS FROM SYNCHRONIZATION
extern const wxEventType EVENT_GRID_CHECK_ROWS;
//SELECTING SYNC DIRECTION
extern const wxEventType EVENT_GRID_SYNC_DIRECTION;

struct CheckRowsEvent : public wxCommandEvent
{
    CheckRowsEvent(ptrdiff_t rowFrom, ptrdiff_t rowTo, bool setIncluded) : wxCommandEvent(EVENT_GRID_CHECK_ROWS), rowFrom_(rowFrom), rowTo_(rowTo), setIncluded_(setIncluded) {}
    virtual wxEvent* Clone() const { return new CheckRowsEvent(*this); }

    const ptrdiff_t rowFrom_;
    const ptrdiff_t rowTo_;
    const bool setIncluded_;
};


struct SyncDirectionEvent : public wxCommandEvent
{
    SyncDirectionEvent(ptrdiff_t rowFrom, ptrdiff_t rowTo, SyncDirection direction) : wxCommandEvent(EVENT_GRID_SYNC_DIRECTION), rowFrom_(rowFrom), rowTo_(rowTo), direction_(direction) {}
    virtual wxEvent* Clone() const { return new SyncDirectionEvent(*this); }

    const ptrdiff_t rowFrom_;
    const ptrdiff_t rowTo_;
    const SyncDirection direction_;
};

typedef void (wxEvtHandler::*CheckRowsEventFunction)(CheckRowsEvent&);
typedef void (wxEvtHandler::*SyncDirectionEventFunction)(SyncDirectionEvent&);

#define CheckRowsEventHandler(func) \
    (wxObjectEventFunction)(wxEventFunction)wxStaticCastEvent(CheckRowsEventFunction, &func)

#define SyncDirectionEventHandler(func) \
    (wxObjectEventFunction)(wxEventFunction)wxStaticCastEvent(SyncDirectionEventFunction, &func)
}

#endif // CUSTOMGRID_H_INCLUDED
