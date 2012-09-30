// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) Zenju (zenju AT gmx DOT de) - All Rights Reserved        *
// **************************************************************************

#include "custom_grid.h"
#include <wx/dc.h>
#include <wx/settings.h>
#include <zen/i18n.h>
#include <zen/file_error.h>
#include <zen/basic_math.h>
#include <zen/format_unit.h>
#include <wx+/tooltip.h>
#include <wx+/string_conv.h>
#include <wx+/rtl.h>
#include <wx+/image_tools.h>
#include "../file_hierarchy.h"
#include "../lib/resources.h"

using namespace zen;
using namespace gridview;


const wxEventType zen::EVENT_GRID_CHECK_ROWS     = wxNewEventType();
const wxEventType zen::EVENT_GRID_SYNC_DIRECTION = wxNewEventType();

namespace
{
const wxColour COLOR_ORANGE      (238, 201,   0);
const wxColour COLOR_GREY        (212, 208, 200);
const wxColour COLOR_YELLOW      (247, 252,  62);
const wxColour COLOR_YELLOW_LIGHT(253, 252, 169);
const wxColour COLOR_CMP_RED     (255, 185, 187);
const wxColour COLOR_SYNC_BLUE   (185, 188, 255);
const wxColour COLOR_SYNC_GREEN  (196, 255, 185);
const wxColour COLOR_NOT_ACTIVE  (228, 228, 228); //light grey


const Zstring ICON_FILE_FOLDER = Zstr("folder");

const int CHECK_BOX_IMAGE = 12; //width of checkbox image
const int CHECK_BOX_SPACE_LEFT = 2;
const int CHECK_BOX_WIDTH = CHECK_BOX_SPACE_LEFT + CHECK_BOX_IMAGE; //width of first block

const size_t ROW_COUNT_NO_DATA = 10;

/*
class hierarchy:
                                 GridDataBase
                                    /|\
                     ________________|________________
                    |                                |
               GridDataRim                           |
                   /|\                               |
          __________|__________                      |
         |                    |                      |
  GridDataLeftRim      GridDataRight           GridDataMiddle
*/



void refreshCell(Grid& grid, size_t row, ColumnType colType)
{
    wxRect cellArea = grid.getCellArea(row, colType); //returns empty rect if column not found; absolute coordinates!
    if (cellArea.height > 0)
    {
        cellArea.SetTopLeft(grid.CalcScrolledPosition(cellArea.GetTopLeft()));
        grid.getMainWin().RefreshRect(cellArea, false);
    }
}


std::pair<ptrdiff_t, ptrdiff_t> getVisibleRows(Grid& grid) //returns range [from, to)
{
    const wxSize clientSize = grid.getMainWin().GetClientSize();
    if (clientSize.GetHeight() > 0)
    {
        wxPoint topLeft = grid.CalcUnscrolledPosition(wxPoint(0, 0));
        wxPoint bottom  = grid.CalcUnscrolledPosition(wxPoint(0, clientSize.GetHeight() - 1));

        ptrdiff_t rowFrom = grid.getRowAtPos(topLeft.y); //returns < 0 if column not found; absolute coordinates!
        if (rowFrom >= 0)
        {
            ptrdiff_t rowEnd = grid.getRowAtPos(bottom.y); //returns < 0 if column not found; absolute coordinates!
            if (rowEnd < 0)
                rowEnd = grid.getRowCount();
            else
                ++rowEnd;
            return std::make_pair(rowFrom, rowEnd);
        }
    }
    return std::make_pair(0, 0);
}


Zstring getExtension(const Zstring& shortName)
{
    return contains(shortName, Zchar('.')) ? afterLast(shortName, Zchar('.')) : Zstring();
};


class IconUpdater;
class GridEventManager;


struct IconManager
{
    IconManager(IconBuffer::IconSize sz) : iconBuffer(sz) {}

    IconBuffer iconBuffer;
    std::unique_ptr<IconUpdater> iconUpdater; //bind ownership to GridDataRim<>!
};

//########################################################################################################

class GridDataBase : public GridData
{
public:
    GridDataBase(Grid& grid, const std::shared_ptr<const zen::GridView>& gridDataView) : grid_(grid), gridDataView_(gridDataView) {}

    void holdOwnership(const std::shared_ptr<GridEventManager>& evtMgr) { evtMgr_ = evtMgr; }

protected:
    Grid& refGrid() { return grid_; }
    const Grid& refGrid() const { return grid_; }

    const GridView* getGridDataView() const { return gridDataView_.get(); }

    const FileSystemObject* getRawData(size_t row) const
    {
        if (auto view = getGridDataView())
            return view->getObject(row);
        return nullptr;
    }

private:
    virtual size_t getRowCount() const
    {
        if (gridDataView_)
        {
            if (gridDataView_->rowsTotal() == 0)
                return ROW_COUNT_NO_DATA;
            return gridDataView_->rowsOnView();
        }
        else
            return ROW_COUNT_NO_DATA;

        //return std::max(MIN_ROW_COUNT, gridDataView_ ? gridDataView_->rowsOnView() : 0);
    }

    std::shared_ptr<GridEventManager> evtMgr_;
    Grid& grid_;
    std::shared_ptr<const GridView> gridDataView_;
};

//########################################################################################################

template <SelectedSide side>
class GridDataRim : public GridDataBase
{
public:
    GridDataRim(const std::shared_ptr<const zen::GridView>& gridDataView, Grid& grid) : GridDataBase(grid, gridDataView) {}

    void setIconManager(const std::shared_ptr<IconManager>& iconMgr) { iconMgr_ = iconMgr; }

    void addIconsToBeLoaded(std::vector<Zstring>& newLoad) //loads all (not yet) drawn icons
    {
        //don't check too often! give worker thread some time to fetch data
        if (iconMgr_)
        {
            const auto& rowsOnScreen = getVisibleRows(refGrid());

            //loop over all visible rows
            const ptrdiff_t firstRow = rowsOnScreen.first;
            const ptrdiff_t rowCount = rowsOnScreen.second - firstRow;

            for (ptrdiff_t i = 0; i < rowCount; ++i)
            {
                //alternate when adding rows: first, last, first + 1, last - 1 ... -> Icon buffer will then load reversely, i.e. from inside out
                const ptrdiff_t currentRow = firstRow + (i % 2 == 0 ?
                                                         i / 2 :
                                                         rowCount - 1 - (i - 1) / 2);

                if (isFailedLoad(currentRow)) //find failed attempts to load icon
                {
                    const Zstring fileName = getIconFile(currentRow);
                    if (!fileName.empty())
                    {
                        //test if they are already loaded in buffer:
                        if (iconMgr_->iconBuffer.requestFileIcon(fileName))
                        {
                            //do a *full* refresh for *every* failed load to update partial DC updates while scrolling
                            refreshCell(refGrid(), currentRow, static_cast<ColumnType>(COL_TYPE_FILENAME));
                            setFailedLoad(currentRow, false);
                        }
                        else //not yet in buffer: mark for async. loading
                            newLoad.push_back(fileName);
                    }
                }
            }
        }
    }

    void setFailedLoad(size_t row, bool failed)
    {
        if (failedLoads.size() != refGrid().getRowCount())
            failedLoads.resize(refGrid().getRowCount());

        if (row < failedLoads.size())
            failedLoads[row] = failed;
    }

    bool isFailedLoad(size_t row) const { return row < failedLoads.size() ? failedLoads[row] != 0 : false; }

protected:
    virtual void renderRowBackgound(wxDC& dc, const wxRect& rect, size_t row, bool enabled, bool selected, bool hasFocus)
    {
        if (enabled)
        {
            if (selected)
                dc.GradientFillLinear(rect, getColorSelectionGradientFrom(), getColorSelectionGradientTo(), wxEAST);
            //ignore focus
            else
            {
                //alternate background color to improve readability (while lacking cell borders)
                if (getRowDisplayType(row) == DISP_TYPE_NORMAL && row % 2 == 0)
                {
                    //accessibility, support high-contrast schemes => work with user-defined background color!
                    const auto backCol = getBackGroundColor(row);

                    auto incChannel = [](unsigned char c, int diff) { return static_cast<unsigned char>(std::max(0, std::min(255, c + diff))); };

                    auto getAdjustedColor = [&](int diff)
                    {
                        return wxColor(incChannel(backCol.Red  (), diff),
                                       incChannel(backCol.Green(), diff),
                                       incChannel(backCol.Blue (), diff));
                    };

                    auto colorDist = [](const wxColor& lhs, const wxColor& rhs) //just some metric
                    {
                        return numeric::power<2>(static_cast<int>(lhs.Red  ()) - static_cast<int>(rhs.Red  ())) +
                               numeric::power<2>(static_cast<int>(lhs.Green()) - static_cast<int>(rhs.Green())) +
                               numeric::power<2>(static_cast<int>(lhs.Blue ()) - static_cast<int>(rhs.Blue ()));
                    };

                    const int signLevel = colorDist(backCol, *wxBLACK) < colorDist(backCol, *wxWHITE) ? 1 : -1; //brighten or darken

                    const wxColor colOutter = getAdjustedColor(signLevel * 14); //just some very faint gradient to avoid visual distraction
                    const wxColor colInner  = getAdjustedColor(signLevel * 11); //

                    //clearArea(dc, rect, backColAlt);

                    //add some nice background gradient
                    wxRect rectUpper = rect;
                    rectUpper.height /= 2;
                    wxRect rectLower = rect;
                    rectLower.y += rectUpper.height;
                    rectLower.height -= rectUpper.height;
                    dc.GradientFillLinear(rectUpper, colOutter, colInner, wxSOUTH);
                    dc.GradientFillLinear(rectLower, colOutter, colInner, wxNORTH);
                }
                else
                    clearArea(dc, rect, getBackGroundColor(row));
            }
        }
        else
            clearArea(dc, rect, wxSystemSettings::GetColour(wxSYS_COLOUR_BTNFACE));
    }

    wxColor getBackGroundColor(size_t row) const
    {
        //accessibility: always set both foreground AND background colors!
        // => harmonize with renderCell()!

        switch (getRowDisplayType(row))
        {
            case DISP_TYPE_NORMAL:
                break;
            case DISP_TYPE_FOLDER:
                return COLOR_GREY;
            case DISP_TYPE_SYMLINK:
                return COLOR_ORANGE;
            case DISP_TYPE_INACTIVE:
                return COLOR_NOT_ACTIVE;

        }
        return wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOW);
    }

private:
    enum DisplayType
    {
        DISP_TYPE_NORMAL,
        DISP_TYPE_FOLDER,
        DISP_TYPE_SYMLINK,
        DISP_TYPE_INACTIVE,
    };

    DisplayType getRowDisplayType(size_t row) const
    {
        const FileSystemObject* fsObj = getRawData(row);
        if (!fsObj )
            return DISP_TYPE_NORMAL;

        //mark filtered rows
        if (!fsObj->isActive())
            return DISP_TYPE_INACTIVE;

        if (fsObj->isEmpty<side>()) //always show not existing files/dirs/symlinks as empty
            return DISP_TYPE_NORMAL;

        DisplayType output = DISP_TYPE_NORMAL;
        //mark directories and symlinks
        struct GetRowType : public FSObjectVisitor
        {
            GetRowType(DisplayType& result) : result_(result) {}

            virtual void visit(const FileMapping& fileObj) {}
            virtual void visit(const SymLinkMapping& linkObj)
            {
                result_ = DISP_TYPE_SYMLINK;
            }
            virtual void visit(const DirMapping& dirObj)
            {
                result_ = DISP_TYPE_FOLDER;
            }
        private:
            DisplayType& result_;
        } getType(output);
        fsObj->accept(getType);
        return output;
    }

    virtual wxString getValue(size_t row, ColumnType colType) const
    {
        if (const FileSystemObject* fsObj = getRawData(row))
        {
            struct GetTextValue : public FSObjectVisitor
            {
                GetTextValue(ColumnTypeRim colType, const FileSystemObject& fso) : colType_(colType), fsObj_(fso) {}

                virtual void visit(const FileMapping& fileObj)
                {
                    switch (colType_)
                    {
                        case COL_TYPE_FULL_PATH:
                            value = toWx(appendSeparator(beforeLast(fileObj.getBaseDirPf<side>() + fileObj.getObjRelativeName(), FILE_NAME_SEPARATOR)));
                            break;
                        case COL_TYPE_FILENAME: //filename
                            value = toWx(fileObj.getShortName<side>());
                            break;
                        case COL_TYPE_REL_PATH: //relative path
                            value = toWx(beforeLast(fileObj.getObjRelativeName(), FILE_NAME_SEPARATOR)); //returns empty string if ch not found
                            break;
                        case COL_TYPE_DIRECTORY:
                            value = toWx(fileObj.getBaseDirPf<side>());
                            break;
                        case COL_TYPE_SIZE: //file size
                            if (!fsObj_.isEmpty<side>())
                                value = zen::toGuiString(fileObj.getFileSize<side>());

                            // -> test file id
                            //if (!fsObj_.isEmpty<side>())
                            //	value = toGuiString(fileObj.getFileId<side>().second) + L" " + toGuiString(fileObj.getFileId<side>().first);
                            break;
                        case COL_TYPE_DATE: //date
                            if (!fsObj_.isEmpty<side>())
                                value = zen::utcToLocalTimeString(fileObj.getLastWriteTime<side>());
                            break;
                        case COL_TYPE_EXTENSION: //file extension
                            value = toWx(getExtension(fileObj.getShortName<side>()));
                            break;
                    }
                }

                virtual void visit(const SymLinkMapping& linkObj)
                {
                    switch (colType_)
                    {
                        case COL_TYPE_FULL_PATH:
                            value = toWx(appendSeparator(beforeLast(linkObj.getBaseDirPf<side>() + linkObj.getObjRelativeName(), FILE_NAME_SEPARATOR)));
                            break;
                        case COL_TYPE_FILENAME: //filename
                            value = toWx(linkObj.getShortName<side>());
                            break;
                        case COL_TYPE_REL_PATH: //relative path
                            value = toWx(beforeLast(linkObj.getObjRelativeName(), FILE_NAME_SEPARATOR)); //returns empty string if ch not found
                            break;
                        case COL_TYPE_DIRECTORY:
                            value = toWx(linkObj.getBaseDirPf<side>());
                            break;
                        case COL_TYPE_SIZE: //file size
                            if (!fsObj_.isEmpty<side>())
                                value = _("<Symlink>");
                            break;
                        case COL_TYPE_DATE: //date
                            if (!fsObj_.isEmpty<side>())
                                value = zen::utcToLocalTimeString(linkObj.getLastWriteTime<side>());
                            break;
                        case COL_TYPE_EXTENSION: //file extension
                            value = toWx(getExtension(linkObj.getShortName<side>()));
                            break;
                    }
                }

                virtual void visit(const DirMapping& dirObj)
                {
                    switch (colType_)
                    {
                        case COL_TYPE_FULL_PATH:
                            value = toWx(appendSeparator(beforeLast(dirObj.getBaseDirPf<side>() + dirObj.getObjRelativeName(), FILE_NAME_SEPARATOR)));
                            break;
                        case COL_TYPE_FILENAME:
                            value = toWx(dirObj.getShortName<side>());
                            break;
                        case COL_TYPE_REL_PATH:
                            value = toWx(beforeLast(dirObj.getObjRelativeName(), FILE_NAME_SEPARATOR)); //returns empty string if ch not found
                            break;
                        case COL_TYPE_DIRECTORY:
                            value = toWx(dirObj.getBaseDirPf<side>());
                            break;
                        case COL_TYPE_SIZE: //file size
                            if (!fsObj_.isEmpty<side>())
                                value = _("<Folder>");
                            break;
                        case COL_TYPE_DATE: //date
                            if (!fsObj_.isEmpty<side>())
                                value = wxEmptyString;
                            break;
                        case COL_TYPE_EXTENSION: //file extension
                            value = wxEmptyString;
                            break;
                    }
                }
                ColumnTypeRim colType_;
                wxString value;

                const FileSystemObject& fsObj_;
            } getVal(static_cast<ColumnTypeRim>(colType), *fsObj);
            fsObj->accept(getVal);
            return getVal.value;
        }
        //if data is not found:
        return wxEmptyString;
    }

    static const int CELL_BORDER = 2;


    virtual void renderCell(Grid& grid, wxDC& dc, const wxRect& rect, size_t row, ColumnType colType)
    {
        wxRect rectTmp = rect;

        //draw horizontal border if required
        auto dispTp = getRowDisplayType(row);
        if (dispTp == getRowDisplayType(row + 1) && dispTp != DISP_TYPE_NORMAL)
        {
            const wxColor colorGridLine = wxColour(192, 192, 192); //light grey
            wxDCPenChanger dummy2(dc, wxPen(colorGridLine, 1, wxSOLID));
            dc.DrawLine(rect.GetBottomLeft(),  rect.GetBottomRight() + wxPoint(1, 0));
            rectTmp.height -= 1;
        }
        //wxRect rectTmp = drawCellBorder(dc, rect);

        const bool isActive = [&]() -> bool
        {
            if (const FileSystemObject* fsObj = this->getRawData(row))
                return fsObj->isActive();
            return true;
        }();

        //draw file icon
        if (static_cast<ColumnTypeRim>(colType) == COL_TYPE_FILENAME &&
            iconMgr_)
        {
            rectTmp.x     += CELL_BORDER;
            rectTmp.width -= CELL_BORDER;

            const int iconSize = iconMgr_->iconBuffer.getSize();
            if (rectTmp.GetWidth() >= iconSize)
            {
                //  Partitioning:
                //   _______________________________
                //  | border | icon | border | text |
                //   -------------------------------

                const Zstring fileName = getIconFile(row);
                if (!fileName.empty())
                {
                    wxIcon icon;

                    //first check if it is a directory icon:
                    if (fileName == ICON_FILE_FOLDER)
                        icon = iconMgr_->iconBuffer.genericDirIcon();
                    else //retrieve file icon
                    {
                        if (!iconMgr_->iconBuffer.requestFileIcon(fileName, &icon)) //returns false if icon is not in buffer
                        {
                            icon = iconMgr_->iconBuffer.genericFileIcon(); //better than nothing
                            setFailedLoad(row, true); //save status of failed icon load -> used for async. icon loading
                            //falsify only! we want to avoid writing incorrect success values when only partially updating the DC, e.g. when scrolling,
                            //see repaint behavior of ::ScrollWindow() function!
                        }
                    }

                    if (icon.IsOk())
                    {
                        //center icon if it is too small
                        const int posX = rectTmp.GetX() + std::max(0, (iconSize - icon.GetWidth()) / 2);
                        const int posY = rectTmp.GetY() + std::max(0, (rectTmp.GetHeight() - icon.GetHeight()) / 2);

                        drawIconRtlNoMirror(dc, icon, wxPoint(posX, posY), buffer);

                        //convert icon to greyscale if row is not active
                        if (!isActive)
                        {
                            wxBitmap bmp(icon.GetWidth(), icon.GetHeight());
                            wxMemoryDC memDc(bmp);
                            memDc.Blit(0, 0, icon.GetWidth(), icon.GetHeight(), &dc, posX, posY); //blit in

                            bmp = wxBitmap(bmp.ConvertToImage().ConvertToGreyscale(1.0/3, 1.0/3, 1.0/3)); //treat all channels equally!
                            memDc.SelectObject(bmp);

                            dc.Blit(posX, posY, icon.GetWidth(), icon.GetHeight(), &memDc, 0, 0); //blit out
                        }
                    }
                }
            }
            rectTmp.x     += iconSize;
            rectTmp.width -= iconSize;
        }

        std::unique_ptr<wxDCTextColourChanger> dummy3;
        if (getRowDisplayType(row) != DISP_TYPE_NORMAL)
            dummy3 = make_unique<wxDCTextColourChanger>(dc, *wxBLACK); //accessibility: always set both foreground AND background colors!

        //draw text
        if (static_cast<ColumnTypeRim>(colType) == COL_TYPE_SIZE && grid.GetLayoutDirection() != wxLayout_RightToLeft)
        {
            //have file size right-justified (but don't change for RTL languages)
            rectTmp.width -= CELL_BORDER;
            drawCellText(dc, rectTmp, getValue(row, colType), isActive, wxALIGN_RIGHT | wxALIGN_CENTER_VERTICAL);
        }
        else
        {
            rectTmp.x     += CELL_BORDER;
            rectTmp.width -= CELL_BORDER;
            drawCellText(dc, rectTmp, getValue(row, colType), isActive, wxALIGN_LEFT | wxALIGN_CENTER_VERTICAL);
        }
    }

    virtual size_t getBestSize(wxDC& dc, size_t row, ColumnType colType)
    {
        //  Partitioning:
        //   ________________________________________
        //  | border | icon | border | text | border |
        //   ----------------------------------------

        int bestSize = 0;
        if (static_cast<ColumnTypeRim>(colType) == COL_TYPE_FILENAME && iconMgr_)
            bestSize += CELL_BORDER + iconMgr_->iconBuffer.getSize();

        bestSize += CELL_BORDER + dc.GetTextExtent(getValue(row, colType)).GetWidth() + CELL_BORDER;

        return bestSize; // + 1 pix for cell border line -> not used anymore!
    }

    virtual wxString getColumnLabel(ColumnType colType) const
    {
        switch (static_cast<ColumnTypeRim>(colType))
        {
            case COL_TYPE_FULL_PATH:
                return _("Full path");
            case COL_TYPE_FILENAME:
                return _("Name"); //= short name
            case COL_TYPE_REL_PATH:
                return _("Relative path");
            case COL_TYPE_DIRECTORY:
                return _("Base folder");
            case COL_TYPE_SIZE:
                return _("Size");
            case COL_TYPE_DATE:
                return _("Date");
            case COL_TYPE_EXTENSION:
                return _("Extension");
        }
        return wxEmptyString;
    }

    virtual void renderColumnLabel(Grid& tree, wxDC& dc, const wxRect& rect, ColumnType colType, bool highlighted)
    {
        wxRect rectInside = drawColumnLabelBorder(dc, rect);
        drawColumnLabelBackground(dc, rectInside, highlighted);

        const int COLUMN_BORDER_LEFT = 4;

        rectInside.x     += COLUMN_BORDER_LEFT;
        rectInside.width -= COLUMN_BORDER_LEFT;
        drawColumnLabelText(dc, rectInside, getColumnLabel(colType));

        //draw sort marker
        if (getGridDataView())
        {
            auto sortInfo = getGridDataView()->getSortInfo();
            if (sortInfo)
            {
                if (colType == static_cast<ColumnType>(sortInfo->type_) && (side == LEFT_SIDE) == sortInfo->onLeft_)
                {
                    const wxBitmap& marker = GlobalResources::getImage(sortInfo->ascending_ ? L"sortAscending" : L"sortDescending");
                    wxPoint markerBegin = rectInside.GetTopLeft() + wxPoint((rectInside.width - marker.GetWidth()) / 2, 0);
                    dc.DrawBitmap(marker, markerBegin, true); //respect 2-pixel border
                }
            }
        }
    }

    Zstring getIconFile(size_t row) const  //return ICON_FILE_FOLDER if row points to a folder
    {
        const FileSystemObject* fsObj = getRawData(row);
        if (fsObj && !fsObj->isEmpty<side>())
        {
            struct GetIcon : public FSObjectVisitor
            {
                virtual void visit(const FileMapping& fileObj)
                {
                    iconName = fileObj.getFullName<side>();
                }
                virtual void visit(const SymLinkMapping& linkObj)
                {
                    iconName = linkObj.getLinkType<side>() == LinkDescriptor::TYPE_DIR ?
                               ICON_FILE_FOLDER :
                               linkObj.getFullName<side>();
                }
                virtual void visit(const DirMapping& dirObj)
                {
                    iconName = ICON_FILE_FOLDER;
                }

                Zstring iconName;
            } getIcon;
            fsObj->accept(getIcon);
            return getIcon.iconName;
        }
        return Zstring();
    }

    virtual wxString getToolTip(size_t row, ColumnType colType) const
    {
        wxString toolTip;

        const FileSystemObject* fsObj = getRawData(row);
        if (fsObj && !fsObj->isEmpty<side>())
        {
            toolTip = toWx(getGridDataView() && getGridDataView()->getFolderPairCount() > 1 ?
                           fsObj->getFullName<side>() :
                           fsObj->getRelativeName<side>());

            struct AssembleTooltip : public FSObjectVisitor
            {
                AssembleTooltip(wxString& tipMsg) : tipMsg_(tipMsg) {}

                virtual void visit(const FileMapping& fileObj)
                {
                    tipMsg_ += L"\n" +
                               _("Size:") + L" " + zen::filesizeToShortString(to<Int64>(fileObj.getFileSize<side>())) + L"\n" +
                               _("Date:") + L" " + zen::utcToLocalTimeString(fileObj.getLastWriteTime<side>());
                }

                virtual void visit(const SymLinkMapping& linkObj)
                {
                    tipMsg_ += L"\n" +
                               _("Date:") + L" " + zen::utcToLocalTimeString(linkObj.getLastWriteTime<side>());
                }

                virtual void visit(const DirMapping& dirObj) {}

                wxString& tipMsg_;
            } assembler(toolTip);
            fsObj->accept(assembler);
        }
        return toolTip;
    }

    std::shared_ptr<IconManager> iconMgr_; //optional
    std::vector<char> failedLoads; //effectively a vector<bool> of size "number of rows"
    std::unique_ptr<wxBitmap> buffer; //avoid costs of recreating this temporal variable
};


class GridDataLeft : public GridDataRim<LEFT_SIDE>
{
public:
    GridDataLeft(const std::shared_ptr<const zen::GridView>& gridDataView, Grid& grid) : GridDataRim<LEFT_SIDE>(gridDataView, grid) {}

    void setNavigationMarker(std::vector<const HierarchyObject*>&& markedFiles,
                             std::vector<const HierarchyObject*>&& markedContainer)
    {
        markedFiles_    .swap(markedFiles);
        markedContainer_.swap(markedContainer);
    }

private:
    virtual void renderRowBackgound(wxDC& dc, const wxRect& rect, size_t row, bool enabled, bool selected, bool hasFocus)
    {
        GridDataRim<LEFT_SIDE>::renderRowBackgound(dc, rect, row, enabled, selected, hasFocus);

        //mark rows selected on navigation grid:
        if (enabled && !selected)
        {
            const bool markRow = [&]() -> bool
            {
                if (const FileSystemObject* fsObj = getRawData(row))
                {
                    if (dynamic_cast<const FileMapping*>(fsObj) || dynamic_cast<const SymLinkMapping*>(fsObj))
                    {
                        for (auto iter = markedFiles_.begin(); iter != markedFiles_.end(); ++iter)
                            if (*iter == &(fsObj->parent())) //mark files/links wich have the given parent
                                return true;
                    }
                    else if (auto dirObj = dynamic_cast<const DirMapping*>(fsObj))
                    {
                        for (auto iter = markedContainer_.begin(); iter != markedContainer_.end(); ++iter)
                            if (*iter == dirObj) //mark directories which *are* the given HierarchyObject*
                                return true;
                    }

                    for (auto iter = markedContainer_.begin(); iter != markedContainer_.end(); ++iter)
                    {
                        //mark all objects which have the HierarchyObject as *any* matching ancestor
                        const HierarchyObject* parent = &(fsObj->parent());
                        for (;;)
                        {
                            if (*iter == parent)
                                return true;

                            if (auto dirObj = dynamic_cast<const DirMapping*>(parent))
                                parent = &(dirObj->parent());
                            else
                                break;
                        }
                    }
                }
                return false;
            }();

            if (markRow)
            {
                //const wxColor COLOR_TREE_SELECTION_GRADIENT = wxColor(101, 148, 255); //H:158 S:255 V:178
                const wxColor COLOR_TREE_SELECTION_GRADIENT = getColorSelectionGradientFrom();

                wxRect rectTmp = rect;
                rectTmp.width /= 20;
                dc.GradientFillLinear(rectTmp, COLOR_TREE_SELECTION_GRADIENT, GridDataRim<LEFT_SIDE>::getBackGroundColor(row), wxEAST);
            }
        }
    }

    std::vector<const HierarchyObject*> markedFiles_;     //mark files/symlinks directly within a container
    std::vector<const HierarchyObject*> markedContainer_; //mark full container including all child-objects
    //DO NOT DEREFERENCE!!!! NOT GUARANTEED TO BE VALID!!!
};


class GridDataRight : public GridDataRim<RIGHT_SIDE>
{
public:
    GridDataRight(const std::shared_ptr<const zen::GridView>& gridDataView, Grid& grid) : GridDataRim<RIGHT_SIDE>(gridDataView, grid) {}
};


//########################################################################################################

class GridDataMiddle : public GridDataBase
{
public:
    GridDataMiddle(const std::shared_ptr<const zen::GridView>& gridDataView, Grid& grid) :
        GridDataBase(grid, gridDataView),
        showSyncAction_(true) {}

    void onSelectBegin(const wxPoint& clientPos, size_t row, ColumnType colType)
    {
        if (static_cast<ColumnTypeMiddle>(colType) == COL_TYPE_MIDDLE_VALUE &&
            row < refGrid().getRowCount())
        {
            refGrid().clearSelection();
            dragSelection.reset(new std::pair<size_t, BlockPosition>(row, mousePosToBlock(clientPos, row)));
        }
    }

    void onSelectEnd(size_t rowFrom, size_t rowTo) //we cannot reuse row from "onSelectBegin": rowFrom and rowTo may be different if user is holding shift
    {
        refGrid().clearSelection();

        //issue custom event
        if (dragSelection)
        {
            if (rowFrom < refGrid().getRowCount() &&
                rowTo   < refGrid().getRowCount()) //row is -1 on capture lost!
            {
                if (wxEvtHandler* evtHandler = refGrid().GetEventHandler())
                    switch (dragSelection->second)
                    {
                        case BLOCKPOS_CHECK_BOX:
                        {
                            const FileSystemObject* fsObj = getRawData(dragSelection->first);
                            const bool setIncluded = fsObj ? !fsObj->isActive() : true;

                            CheckRowsEvent evt(rowFrom, rowTo, setIncluded);
                            evtHandler->ProcessEvent(evt);
                        }
                        break;
                        case BLOCKPOS_LEFT:
                        {
                            SyncDirectionEvent evt(rowFrom, rowTo, SYNC_DIR_LEFT);
                            evtHandler->ProcessEvent(evt);
                        }
                        break;
                        case BLOCKPOS_MIDDLE:
                        {
                            SyncDirectionEvent evt(rowFrom, rowTo, SYNC_DIR_NONE);
                            evtHandler->ProcessEvent(evt);
                        }
                        break;
                        case BLOCKPOS_RIGHT:
                        {
                            SyncDirectionEvent evt(rowFrom, rowTo, SYNC_DIR_RIGHT);
                            evtHandler->ProcessEvent(evt);
                        }
                        break;
                    }
            }
            dragSelection.reset();
        }
    }

    void onMouseMovement(const wxPoint& clientPos, size_t row, ColumnType colType)
    {
        //manage block highlighting and custom tooltip
        if (dragSelection)
        {
            toolTip.hide(); //handle custom tooltip
        }
        else
        {
            if (static_cast<ColumnTypeMiddle>(colType) == COL_TYPE_MIDDLE_VALUE)
            {
                if (highlight) //refresh old highlight
                    refreshCell(refGrid(), highlight->first, static_cast<ColumnType>(COL_TYPE_MIDDLE_VALUE));

                highlight.reset(new std::pair<size_t, BlockPosition>(row, mousePosToBlock(clientPos, row)));
                refreshCell(refGrid(), highlight->first, static_cast<ColumnType>(COL_TYPE_MIDDLE_VALUE));

                //show custom tooltip
                showToolTip(row, refGrid().getMainWin().ClientToScreen(clientPos));
            }
            else
                onMouseLeave();
        }
    }

    void onMouseLeave()
    {
        if (highlight)
        {
            refreshCell(refGrid(), highlight->first, static_cast<ColumnType>(COL_TYPE_MIDDLE_VALUE));
            highlight.reset();
        }

        toolTip.hide(); //handle custom tooltip
    }

    void showSyncAction(bool value) { showSyncAction_ = value; }

private:
    virtual wxString getValue(size_t row, ColumnType colType) const
    {
        if (static_cast<ColumnTypeMiddle>(colType) == COL_TYPE_MIDDLE_VALUE)
        {
            if (const FileSystemObject* fsObj = getRawData(row))
                return showSyncAction_ ? getSymbol(fsObj->getSyncOperation()) : getSymbol(fsObj->getCategory());
        }
        return wxEmptyString;
    }


    virtual void renderRowBackgound(wxDC& dc, const wxRect& rect, size_t row, bool enabled, bool selected, bool hasFocus)
    {
        drawCellBackground(dc, rect, enabled, selected, hasFocus, getBackGroundColor(row));
    }

    virtual void renderCell(Grid& grid, wxDC& dc, const wxRect& rect, size_t row, ColumnType colType)
    {
        switch (static_cast<ColumnTypeMiddle>(colType))
        {
            case COL_TYPE_MIDDLE_VALUE:
            {
                if (const FileSystemObject* fsObj = getRawData(row))
                {
                    //wxRect rectInside = drawCellBorder(dc, rect);
                    wxRect rectInside = rect;

                    rectInside.width -= CHECK_BOX_SPACE_LEFT;
                    rectInside.x += CHECK_BOX_SPACE_LEFT;

                    //draw checkbox
                    wxRect checkBoxArea = rectInside;
                    checkBoxArea.SetWidth(CHECK_BOX_IMAGE);

                    const bool          rowHighlighted = dragSelection ? row == dragSelection->first : highlight ? row == highlight->first : false;
                    const BlockPosition highlightBlock = dragSelection ? dragSelection->second       : highlight ? highlight->second       : BLOCKPOS_CHECK_BOX;

                    if (rowHighlighted && highlightBlock == BLOCKPOS_CHECK_BOX)
                        drawBitmapRtlMirror(dc, GlobalResources::getImage(fsObj->isActive() ? L"checkboxTrueFocus" : L"checkboxFalseFocus"), checkBoxArea, wxALIGN_CENTER, buffer);
                    else //default
                        drawBitmapRtlMirror(dc, GlobalResources::getImage(fsObj->isActive() ? L"checkboxTrue"      : L"checkboxFalse"     ), checkBoxArea, wxALIGN_CENTER, buffer);

                    rectInside.width -= CHECK_BOX_IMAGE;
                    rectInside.x += CHECK_BOX_IMAGE;

                    //synchronization preview
                    if (showSyncAction_)
                    {
                        if (rowHighlighted && highlightBlock != BLOCKPOS_CHECK_BOX)
                            switch (highlightBlock)
                            {
                                case BLOCKPOS_CHECK_BOX:
                                    break;
                                case BLOCKPOS_LEFT:
                                    drawBitmapRtlMirror(dc, getSyncOpImage(fsObj->testSyncOperation(SYNC_DIR_LEFT)), rectInside, wxALIGN_LEFT | wxALIGN_CENTER_VERTICAL, buffer);
                                    break;
                                case BLOCKPOS_MIDDLE:
                                    drawBitmapRtlMirror(dc, getSyncOpImage(fsObj->testSyncOperation(SYNC_DIR_NONE)), rectInside, wxALIGN_CENTER, buffer);
                                    break;
                                case BLOCKPOS_RIGHT:
                                    drawBitmapRtlMirror(dc, getSyncOpImage(fsObj->testSyncOperation(SYNC_DIR_RIGHT)), rectInside, wxALIGN_RIGHT | wxALIGN_CENTER_VERTICAL, buffer);
                                    break;
                            }
                        else //default
                            drawBitmapRtlMirror(dc, getSyncOpImage(fsObj->getSyncOperation()), rectInside, wxALIGN_CENTER, buffer);
                    }
                    else //comparison results view
                        drawBitmapRtlMirror(dc, getCmpResultImage(fsObj->getCategory()), rectInside, wxALIGN_CENTER, buffer);
                }
            }
            break;
            case COL_TYPE_BORDER:
                clearArea(dc, rect, wxSystemSettings::GetColour(wxSYS_COLOUR_3DSHADOW));
                break;
        }
    }

    virtual wxString getColumnLabel(ColumnType colType) const { return wxEmptyString; }

    virtual void renderColumnLabel(Grid& tree, wxDC& dc, const wxRect& rect, ColumnType colType, bool highlighted)
    {
        switch (static_cast<ColumnTypeMiddle>(colType))
        {
            case COL_TYPE_MIDDLE_VALUE:
            {
                wxRect rectInside = drawColumnLabelBorder(dc, rect);
                drawColumnLabelBackground(dc, rectInside, highlighted);

                const wxBitmap& cmpIcon  = GlobalResources::getImage(L"compareSmall");
                const wxBitmap& syncIcon = GlobalResources::getImage(L"syncSmall");

                const int space = 8;
                const int imageWidthTotal = cmpIcon.GetWidth() + space + syncIcon.GetWidth();

                const int posX = (rectInside.GetWidth () - imageWidthTotal) / 2;
                const int posY = (rectInside.GetHeight() - cmpIcon.GetHeight()) / 2;
                assert(cmpIcon.GetHeight() == syncIcon.GetHeight());

                dc.DrawBitmap(showSyncAction_ ? greyScale(cmpIcon) : cmpIcon, posX, posY, true);
                dc.DrawBitmap(showSyncAction_ ? syncIcon : greyScale(syncIcon), posX + cmpIcon.GetWidth() + space, posY, true);
            }
            break;

            case COL_TYPE_BORDER:
                drawCellBackground(dc, rect, true, false, true, wxSystemSettings::GetColour(wxSYS_COLOUR_3DSHADOW));
                break;
        }
    }

    wxColor getBackGroundColor(size_t row) const
    {
        if (const FileSystemObject* fsObj = getRawData(row))
        {
            if (!fsObj->isActive())
                return COLOR_NOT_ACTIVE;
            else
            {
                if (showSyncAction_) //synchronization preview
                {
                    switch (fsObj->getSyncOperation()) //evaluate comparison result and sync direction
                    {
                        case SO_DO_NOTHING:
                            //return COLOR_NOT_ACTIVE;
                        case SO_EQUAL:
                            break; //usually white

                        case SO_CREATE_NEW_LEFT:
                        case SO_OVERWRITE_LEFT:
                        case SO_DELETE_LEFT:
                        case SO_MOVE_LEFT_SOURCE:
                        case SO_MOVE_LEFT_TARGET:
                        case SO_COPY_METADATA_TO_LEFT:
                            return COLOR_SYNC_BLUE;

                        case SO_CREATE_NEW_RIGHT:
                        case SO_OVERWRITE_RIGHT:
                        case SO_DELETE_RIGHT:
                        case SO_MOVE_RIGHT_SOURCE:
                        case SO_MOVE_RIGHT_TARGET:
                        case SO_COPY_METADATA_TO_RIGHT:
                            return COLOR_SYNC_GREEN;

                        case SO_UNRESOLVED_CONFLICT:
                            return COLOR_YELLOW;
                    }
                }
                else //comparison results view
                {
                    switch (fsObj->getCategory())
                    {
                        case FILE_LEFT_SIDE_ONLY:
                        case FILE_LEFT_NEWER:
                            return COLOR_SYNC_BLUE; //COLOR_CMP_BLUE;

                        case FILE_RIGHT_SIDE_ONLY:
                        case FILE_RIGHT_NEWER:
                            return COLOR_SYNC_GREEN; //COLOR_CMP_GREEN;

                        case FILE_DIFFERENT:
                            return COLOR_CMP_RED;
                        case FILE_EQUAL:
                            break; //usually white
                        case FILE_CONFLICT:
                        case FILE_DIFFERENT_METADATA: //= sub-category of equal, but hint via background that sync direction follows conflict-setting
                            return COLOR_YELLOW;
                            //return COLOR_YELLOW_LIGHT;
                    }
                }
            }
        }
        return wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOW);
    }

    enum BlockPosition //each cell can be divided into four blocks concerning mouse selections
    {
        BLOCKPOS_CHECK_BOX,
        BLOCKPOS_LEFT,
        BLOCKPOS_MIDDLE,
        BLOCKPOS_RIGHT
    };

    //determine blockposition within cell
    BlockPosition mousePosToBlock(const wxPoint& clientPos, size_t row) const
    {
        const int absX = refGrid().CalcUnscrolledPosition(clientPos).x;

        const wxRect rect = refGrid().getCellArea(row, static_cast<ColumnType>(COL_TYPE_MIDDLE_VALUE)); //returns empty rect if column not found; absolute coordinates!
        if (rect.width > CHECK_BOX_WIDTH && rect.height > 0)
        {
            const FileSystemObject* const fsObj = getRawData(row);
            if (fsObj && showSyncAction_ &&
                fsObj->getSyncOperation() != SO_EQUAL) //in sync-preview equal files shall be treated as in cmp result, i.e. as full checkbox
            {
                // cell:
                //  ----------------------------------
                // | checkbox | left | middle | right|
                //  ----------------------------------

                const double blockWidth = (rect.GetWidth() - CHECK_BOX_WIDTH) / 3.0;
                if (rect.GetX() + CHECK_BOX_WIDTH <= absX && absX < rect.GetX() + rect.GetWidth())
                {
                    if (absX < rect.GetX() + CHECK_BOX_WIDTH + blockWidth)
                        return BLOCKPOS_LEFT;
                    else if (absX < rect.GetX() + CHECK_BOX_WIDTH + 2 * blockWidth)
                        return BLOCKPOS_MIDDLE;
                    else
                        return BLOCKPOS_RIGHT;
                }
            }

        }
        return BLOCKPOS_CHECK_BOX;
    }

    void showToolTip(size_t row, wxPoint posScreen)
    {
        if (const FileSystemObject* fsObj = getRawData(row))
        {
            if (showSyncAction_) //synchronization preview
            {
                const wchar_t* imageName = [&]() -> const wchar_t*
                {
                    const SyncOperation syncOp = fsObj->getSyncOperation();
                    switch (syncOp)
                    {
                        case SO_CREATE_NEW_LEFT:
                            return L"createLeft";
                        case SO_CREATE_NEW_RIGHT:
                            return L"createRight";
                        case SO_DELETE_LEFT:
                            return L"deleteLeft";
                        case SO_DELETE_RIGHT:
                            return L"deleteRight";
                        case SO_MOVE_LEFT_SOURCE:
                            return L"moveLeftSource";
                        case SO_MOVE_LEFT_TARGET:
                            return L"moveLeftTarget";
                        case SO_MOVE_RIGHT_SOURCE:
                            return L"moveRightSource";
                        case SO_MOVE_RIGHT_TARGET:
                            return L"moveRightTarget";
                        case SO_OVERWRITE_LEFT:
                            return L"updateLeft";
                        case SO_COPY_METADATA_TO_LEFT:
                            return L"moveLeft";
                        case SO_OVERWRITE_RIGHT:
                            return L"updateRight";
                        case SO_COPY_METADATA_TO_RIGHT:
                            return L"moveRight";
                        case SO_DO_NOTHING:
                            return L"none";
                        case SO_EQUAL:
                            return L"equal";
                        case SO_UNRESOLVED_CONFLICT:
                            return L"conflict";
                    };
                    assert(false);
                    return L"";
                }();
                const auto& img = mirrorIfRtl(GlobalResources::getImage(imageName));
                toolTip.show(getSyncOpDescription(*fsObj), posScreen, &img);
            }
            else
            {
                const wchar_t* imageName = [&]() -> const wchar_t*
                {
                    const CompareFilesResult cmpRes = fsObj->getCategory();
                    switch (cmpRes)
                    {
                        case FILE_LEFT_SIDE_ONLY:
                            return L"leftOnly";
                        case FILE_RIGHT_SIDE_ONLY:
                            return L"rightOnly";
                        case FILE_LEFT_NEWER:
                            return L"leftNewer";
                        case FILE_RIGHT_NEWER:
                            return L"rightNewer";
                        case FILE_DIFFERENT:
                            return L"different";
                        case FILE_EQUAL:
                        case FILE_DIFFERENT_METADATA: //= sub-category of equal
                            return L"equal";
                        case FILE_CONFLICT:
                            return L"conflict";
                    }
                    assert(false);
                    return L"";
                }();
                const auto& img = mirrorIfRtl(GlobalResources::getImage(imageName));
                toolTip.show(getCategoryDescription(*fsObj), posScreen, &img);
            }
        }
        else
            toolTip.hide(); //if invalid row...
    }

    virtual wxString getToolTip(ColumnType colType) const { return showSyncAction_ ? _("Action") : _("Category"); }

    bool showSyncAction_;
    std::unique_ptr<std::pair<size_t, BlockPosition>> highlight; //(row, block) current mouse highlight
    std::unique_ptr<std::pair<size_t, BlockPosition>> dragSelection; //(row, block)
    std::unique_ptr<wxBitmap> buffer; //avoid costs of recreating this temporal variable
    zen::Tooltip toolTip;
};

//########################################################################################################

const wxEventType EVENT_ALIGN_SCROLLBARS = wxNewEventType();

class GridEventManager : private wxEvtHandler
{
public:
    GridEventManager(Grid& gridL,
                     Grid& gridC,
                     Grid& gridR,
                     GridDataLeft& provLeft,
                     GridDataMiddle& provMiddle,
                     GridDataRight& provRight) :
        gridL_(gridL), gridC_(gridC), gridR_(gridR),
        provLeft_(provLeft), provMiddle_(provMiddle), provRight_(provRight)
    {
        gridL_.Connect(EVENT_GRID_COL_RESIZE, GridColumnResizeEventHandler(GridEventManager::onResizeColumnL), nullptr, this);
        gridR_.Connect(EVENT_GRID_COL_RESIZE, GridColumnResizeEventHandler(GridEventManager::onResizeColumnR), nullptr, this);

        gridL_.getMainWin().Connect(wxEVT_KEY_DOWN, wxKeyEventHandler  (GridEventManager::onKeyDownL), nullptr, this);
        gridC_.getMainWin().Connect(wxEVT_KEY_DOWN, wxKeyEventHandler  (GridEventManager::onKeyDownC), nullptr, this);
        gridR_.getMainWin().Connect(wxEVT_KEY_DOWN, wxKeyEventHandler  (GridEventManager::onKeyDownR), nullptr, this);

        gridC_.getMainWin().Connect(wxEVT_MOTION,       wxMouseEventHandler(GridEventManager::onCenterMouseMovement), nullptr, this);
        gridC_.getMainWin().Connect(wxEVT_LEAVE_WINDOW, wxMouseEventHandler(GridEventManager::onCenterMouseLeave   ), nullptr, this);

        gridC_.Connect(EVENT_GRID_MOUSE_LEFT_DOWN, GridClickEventHandler      (GridEventManager::onCenterSelectBegin), nullptr, this);
        gridC_.Connect(EVENT_GRID_SELECT_RANGE,    GridRangeSelectEventHandler(GridEventManager::onCenterSelectEnd  ), nullptr, this);

        //clear selection of other grid when selecting on
        gridL_.Connect(EVENT_GRID_SELECT_RANGE,    GridRangeSelectEventHandler(GridEventManager::onGridSelectionL), nullptr, this);
        gridR_.Connect(EVENT_GRID_SELECT_RANGE,    GridRangeSelectEventHandler(GridEventManager::onGridSelectionR), nullptr, this);

        //parallel grid scrolling: do NOT use DoPrepareDC() to align grids! GDI resource leak! Use regular paint event instead:
        gridL_.getMainWin().Connect(wxEVT_PAINT, wxEventHandler(GridEventManager::onPaintGridL), NULL, this);
        gridC_.getMainWin().Connect(wxEVT_PAINT, wxEventHandler(GridEventManager::onPaintGridC), NULL, this);
        gridR_.getMainWin().Connect(wxEVT_PAINT, wxEventHandler(GridEventManager::onPaintGridR), NULL, this);

        gridL_.Connect(wxEVT_SCROLLWIN_THUMBTRACK, wxEventHandler(GridEventManager::onGridAccessL), NULL, this);
        gridL_.Connect(wxEVT_SCROLLWIN_PAGEUP,     wxEventHandler(GridEventManager::onGridAccessL), NULL, this);
        gridL_.Connect(wxEVT_SCROLLWIN_PAGEDOWN,   wxEventHandler(GridEventManager::onGridAccessL), NULL, this);
        gridL_.Connect(wxEVT_SCROLLWIN_TOP,        wxEventHandler(GridEventManager::onGridAccessL), NULL, this);
        gridL_.Connect(wxEVT_SCROLLWIN_BOTTOM,     wxEventHandler(GridEventManager::onGridAccessL), NULL, this);
        gridL_.Connect(wxEVT_SCROLLWIN_LINEUP,     wxEventHandler(GridEventManager::onGridAccessL), NULL, this);
        gridL_.Connect(wxEVT_SCROLLWIN_LINEDOWN,   wxEventHandler(GridEventManager::onGridAccessL), NULL, this);

        gridR_.Connect(wxEVT_SCROLLWIN_THUMBTRACK, wxEventHandler(GridEventManager::onGridAccessR), NULL, this);
        gridR_.Connect(wxEVT_SCROLLWIN_PAGEUP,     wxEventHandler(GridEventManager::onGridAccessR), NULL, this);
        gridR_.Connect(wxEVT_SCROLLWIN_PAGEDOWN,   wxEventHandler(GridEventManager::onGridAccessR), NULL, this);
        gridR_.Connect(wxEVT_SCROLLWIN_TOP,        wxEventHandler(GridEventManager::onGridAccessR), NULL, this);
        gridR_.Connect(wxEVT_SCROLLWIN_BOTTOM,     wxEventHandler(GridEventManager::onGridAccessR), NULL, this);
        gridR_.Connect(wxEVT_SCROLLWIN_LINEUP,     wxEventHandler(GridEventManager::onGridAccessR), NULL, this);
        gridR_.Connect(wxEVT_SCROLLWIN_LINEDOWN,   wxEventHandler(GridEventManager::onGridAccessR), NULL, this);

        Connect(EVENT_ALIGN_SCROLLBARS, wxEventHandler(GridEventManager::onAlignScrollBars), NULL, this);
    }

private:
    void onCenterSelectBegin(GridClickEvent& event)
    {

        provMiddle_.onSelectBegin(event.GetPosition(), event.row_, event.colType_);
        event.Skip();
    }

    void onCenterSelectEnd(GridRangeSelectEvent& event)
    {
        if (event.positive_) //we do NOT want to react on GridRangeSelectEvent() within Grid::clearSelectionAll() directly following right mouse click!
            provMiddle_.onSelectEnd(event.rowFrom_, event.rowTo_);
        event.Skip();
    }

    void onCenterMouseMovement(wxMouseEvent& event)
    {
        const wxPoint& topLeftAbs = gridC_.CalcUnscrolledPosition(event.GetPosition());
        const int row = gridC_.getRowAtPos(topLeftAbs.y); //returns < 0 if column not found; absolute coordinates!
        if (auto colInfo = gridC_.getColumnAtPos(topLeftAbs.x)) //(column type, component position)
        {
            //redirect mouse movement to middle grid component
            provMiddle_.onMouseMovement(event.GetPosition(), row, colInfo->first);
        }
        event.Skip();
    }

    void onCenterMouseLeave(wxMouseEvent& event)
    {
        provMiddle_.onMouseLeave();
        event.Skip();
    }

    void onGridSelectionL(GridRangeSelectEvent& event) { onGridSelection(gridL_, gridR_); event.Skip(); }
    void onGridSelectionR(GridRangeSelectEvent& event) { onGridSelection(gridR_, gridL_); event.Skip(); }

    void onGridSelection(const Grid& grid, Grid& other)
    {
        if (!wxGetKeyState(WXK_CONTROL)) //clear other grid unless user is holding CTRL
            other.clearSelection();
    }

    void onKeyDownL(wxKeyEvent& event) {  onKeyDown(event, gridL_); }
    void onKeyDownC(wxKeyEvent& event) {  onKeyDown(event, gridC_); }
    void onKeyDownR(wxKeyEvent& event) {  onKeyDown(event, gridR_); }

    void onKeyDown(wxKeyEvent& event, const Grid& grid)
    {
        int keyCode = event.GetKeyCode();
        if (wxTheApp->GetLayoutDirection() == wxLayout_RightToLeft)
        {
            if (keyCode == WXK_LEFT)
                keyCode = WXK_RIGHT;
            else if (keyCode == WXK_RIGHT)
                keyCode = WXK_LEFT;
            else if (keyCode == WXK_NUMPAD_LEFT)
                keyCode = WXK_NUMPAD_RIGHT;
            else if (keyCode == WXK_NUMPAD_RIGHT)
                keyCode = WXK_NUMPAD_LEFT;
        }

        //skip middle component when navigating via keyboard

        const auto row = grid.getGridCursor().first;

        if (event.ShiftDown())
            ;
        else if (event.ControlDown())
            ;
        else
            switch (keyCode)
            {
                case WXK_LEFT:
                case WXK_NUMPAD_LEFT:
                    gridL_.setGridCursor(row);
                    gridL_.SetFocus();
                    return; //swallow event

                case WXK_RIGHT:
                case WXK_NUMPAD_RIGHT:
                    gridR_.setGridCursor(row);
                    gridR_.SetFocus();
                    return; //swallow event
            }

        event.Skip();
    }

    void onResizeColumnL(GridColumnResizeEvent& event) { resizeOtherSide(gridL_, gridR_, event.colType_, event.offset_); }
    void onResizeColumnR(GridColumnResizeEvent& event) { resizeOtherSide(gridR_, gridL_, event.colType_, event.offset_); }

    void resizeOtherSide(const Grid& src, Grid& trg, ColumnType type, ptrdiff_t offset)
    {
        //find stretch factor of resized column: type is unique due to makeConsistent()!
        std::vector<Grid::ColumnAttribute> cfgSrc = src.getColumnConfig();
        auto iter = std::find_if(cfgSrc.begin(), cfgSrc.end(), [&](Grid::ColumnAttribute& ca) { return ca.type_ == type; });
        if (iter == cfgSrc.end())
            return;
        const ptrdiff_t stretchSrc = iter->stretch_;

        //we do not propagate resizings on stretched columns to the other side: awkward user experience
        if (stretchSrc > 0)
            return;

        //apply resized offset to other side, but only if stretch factors match!
        std::vector<Grid::ColumnAttribute> cfgTrg = trg.getColumnConfig();
        std::for_each(cfgTrg.begin(), cfgTrg.end(), [&](Grid::ColumnAttribute& ca)
        {
            if (ca.type_ == type && ca.stretch_ == stretchSrc)
                ca.offset_ = offset;
        });
        trg.setColumnConfig(cfgTrg);
    }

    void onGridAccessL(wxEvent& event) { gridL_.SetFocus(); event.Skip(); }
    void onGridAccessR(wxEvent& event) { gridR_.SetFocus(); event.Skip(); }

    void onPaintGridL(wxEvent& event) { onPaintGrid(gridL_); event.Skip(); }
    void onPaintGridC(wxEvent& event) { onPaintGrid(gridC_); event.Skip(); }
    void onPaintGridR(wxEvent& event) { onPaintGrid(gridR_); event.Skip(); }

    void onPaintGrid(const Grid& grid)
    {
        //align scroll positions of all three grids *synchronously* during paint event! (wxGTK has visible delay when this is done asynchronously, no delay on Windows)

        //determine lead grid
        const Grid* lead = nullptr;
        Grid* follow1    = nullptr;
        Grid* follow2    = nullptr;
        auto setGrids = [&](const Grid& l, Grid& f1, Grid& f2) { lead = &l; follow1 = &f1; follow2 = &f2; };

        if (wxWindow::FindFocus() == &gridC_.getMainWin())
            setGrids(gridC_, gridL_, gridR_);
        else if (wxWindow::FindFocus() == &gridR_.getMainWin())
            setGrids(gridR_, gridL_, gridC_);
        else //default: left panel
            setGrids(gridL_, gridC_, gridR_);

        //align other grids only while repainting the lead grid to avoid scrolling and updating a grid at the same time!
        if (lead != &grid) return;

        auto scroll = [](Grid& target, int y) //support polling
        {
            //scroll vertically only - scrolling horizontally becomes annoying if left and right sides have different widths;
            //e.g. h-scroll on left would be undone when scrolling vertically on right which doesn't have a h-scrollbar
            int yOld = 0;
            target.GetViewStart(nullptr, &yOld);
            if (yOld != y)
                target.Scroll(-1, y);
        };
        int y = 0;
        lead->GetViewStart(nullptr, &y);
        scroll(*follow1, y);
        scroll(*follow2, y);

        //harmonize placement of horizontal scrollbar to avoid grids getting out of sync!
        //since this affects the grid that is currently repainted as well, we do work asynchronously!
        //avoids at least this problem: remaining graphics artifact when changing from Grid::SB_SHOW_ALWAYS to Grid::SB_SHOW_NEVER at location of old scrollbar (Windows only)
        wxCommandEvent alignEvent(EVENT_ALIGN_SCROLLBARS);
        AddPendingEvent(alignEvent); //waits until next idle event - may take up to a second if the app is busy on wxGTK!
    }

    void onAlignScrollBars(wxEvent& event)
    {
        auto needsHorizontalScrollbars = [](Grid& grid) -> bool
        {
            const wxWindow& mainWin = grid.getMainWin();
            return mainWin.GetVirtualSize().GetWidth() > mainWin.GetClientSize().GetWidth();
            //assuming Grid::updateWindowSizes() does its job well, this should suffice!
            //CAVEAT: if horizontal and vertical scrollbar are circular dependent from each other
            //(h-scrollbar is shown due to v-scrollbar consuming horizontal width, ect...)
            //while in fact both are NOT needed, this special case results in a bogus need for scrollbars!
            //see https://sourceforge.net/tracker/?func=detail&aid=3514183&group_id=234430&atid=1093083
            // => since we're outside the Grid abstraction, we should not duplicate code to handle this special case as it seems to be insignificant
        };

        Grid::ScrollBarStatus sbStatusX = needsHorizontalScrollbars(gridL_) ||
                                          needsHorizontalScrollbars(gridR_) ?
                                          Grid::SB_SHOW_ALWAYS : Grid::SB_SHOW_NEVER;
        gridL_.showScrollBars(sbStatusX, Grid::SB_SHOW_NEVER);
        gridC_.showScrollBars(sbStatusX, Grid::SB_SHOW_NEVER);
        gridR_.showScrollBars(sbStatusX, Grid::SB_SHOW_AUTOMATIC);
    }

    Grid& gridL_;
    Grid& gridC_;
    Grid& gridR_;
    GridDataLeft&   provLeft_;
    GridDataMiddle& provMiddle_;
    GridDataRight& provRight_;
};
}

//########################################################################################################

void gridview::init(Grid& gridLeft, Grid& gridCenter, Grid& gridRight, const std::shared_ptr<const zen::GridView>& gridDataView)
{
    auto provLeft_   = std::make_shared<GridDataLeft  >(gridDataView, gridLeft);
    auto provMiddle_ = std::make_shared<GridDataMiddle>(gridDataView, gridCenter);
    auto provRight_  = std::make_shared<GridDataRight >(gridDataView, gridRight);

    gridLeft  .setDataProvider(provLeft_);   //data providers reference grid =>
    gridCenter.setDataProvider(provMiddle_); //ownership must belong *exclusively* to grid!
    gridRight .setDataProvider(provRight_);

    auto evtMgr = std::make_shared<GridEventManager>(gridLeft, gridCenter, gridRight, *provLeft_, *provMiddle_, *provRight_);
    provLeft_  ->holdOwnership(evtMgr);
    provMiddle_->holdOwnership(evtMgr);
    provRight_ ->holdOwnership(evtMgr);

    gridCenter.enableColumnMove  (false);
    gridCenter.enableColumnResize(false);

    gridCenter.showRowLabel(false);
    gridRight .showRowLabel(false);

    //gridLeft  .showScrollBars(Grid::SB_SHOW_AUTOMATIC, Grid::SB_SHOW_NEVER); -> redundant: configuration happens in GridEventManager::onAlignScrollBars()
    //gridCenter.showScrollBars(Grid::SB_SHOW_NEVER,     Grid::SB_SHOW_NEVER);

    gridCenter.SetSize(60 /*+ 2 * 5*/, -1);
    std::vector<Grid::ColumnAttribute> attribMiddle;
    //attribMiddle.push_back(Grid::ColumnAttribute(static_cast<ColumnType>(COL_TYPE_BORDER), 5, 0, true));
    attribMiddle.push_back(Grid::ColumnAttribute(static_cast<ColumnType>(COL_TYPE_MIDDLE_VALUE), 60, 0, true));
    //attribMiddle.push_back(Grid::ColumnAttribute(static_cast<ColumnType>(COL_TYPE_BORDER), 5, 0, true));
    gridCenter.setColumnConfig(attribMiddle);
}


namespace
{
std::vector<ColumnAttributeRim> makeConsistent(const std::vector<ColumnAttributeRim>& attribs)
{
    std::set<ColumnTypeRim> usedTypes;

    std::vector<ColumnAttributeRim> output;
    //remove duplicates
    std::copy_if(attribs.begin(), attribs.end(), std::back_inserter(output),
    [&](const ColumnAttributeRim& a) { return usedTypes.insert(a.type_).second; });

    //make sure each type is existing! -> should *only* be a problem if user manually messes with globalsettings.xml
    const auto& defAttr = getDefaultColumnAttributesLeft();
    std::copy_if(defAttr.begin(), defAttr.end(), std::back_inserter(output),
    [&](const ColumnAttributeRim& a) { return usedTypes.insert(a.type_).second; });

    return output;
}
}

std::vector<Grid::ColumnAttribute> gridview::convertConfig(const std::vector<ColumnAttributeRim>& attribs)
{
    const auto& attribClean = makeConsistent(attribs);

    std::vector<Grid::ColumnAttribute> output;
    std::transform(attribClean.begin(), attribClean.end(), std::back_inserter(output),
    [&](const ColumnAttributeRim& ca) { return Grid::ColumnAttribute(static_cast<ColumnType>(ca.type_), ca.offset_, ca.stretch_, ca.visible_); });

    return output;
}


std::vector<ColumnAttributeRim> gridview::convertConfig(const std::vector<Grid::ColumnAttribute>& attribs)
{
    std::vector<ColumnAttributeRim> output;

    std::transform(attribs.begin(), attribs.end(), std::back_inserter(output),
    [&](const Grid::ColumnAttribute& ca) { return ColumnAttributeRim(static_cast<ColumnTypeRim>(ca.type_), ca.offset_, ca.stretch_, ca.visible_); });

    return makeConsistent(output);
}


namespace
{
class IconUpdater : private wxEvtHandler //update file icons periodically: use SINGLE instance to coordinate left and right grid in parallel
{
public:
    IconUpdater(GridDataLeft& provLeft, GridDataRight& provRight, IconBuffer& iconBuffer) : provLeft_(provLeft), provRight_(provRight), iconBuffer_(iconBuffer)
    {
        timer.Connect(wxEVT_TIMER, wxEventHandler(IconUpdater::loadIconsAsynchronously), nullptr, this);
        timer.Start(50); //timer interval in ms
    }

private:
    void loadIconsAsynchronously(wxEvent& event) //loads all (not yet) drawn icons
    {
        std::vector<Zstring> newLoad;
        provLeft_ .addIconsToBeLoaded(newLoad); //loads all (not yet) drawn icons
        provRight_.addIconsToBeLoaded(newLoad); //
        iconBuffer_.setWorkload(newLoad);
    }

    GridDataLeft& provLeft_;
    GridDataRight& provRight_;
    IconBuffer& iconBuffer_;
    wxTimer timer;
};
}

void gridview::setupIcons(Grid& gridLeft, Grid& gridCenter, Grid& gridRight, bool show, IconBuffer::IconSize sz)
{
    auto* provLeft  = dynamic_cast<GridDataLeft*>(gridLeft .getDataProvider());
    auto* provRight = dynamic_cast<GridDataRight*>(gridRight.getDataProvider());

    if (provLeft && provRight)
    {
        int newRowHeight = 0;
        if (show)
        {
            auto iconMgr = std::make_shared<IconManager>(sz);
            iconMgr->iconUpdater.reset(new IconUpdater(*provLeft, *provRight, iconMgr->iconBuffer));

            provLeft ->setIconManager(iconMgr);
            provRight->setIconManager(iconMgr);
            newRowHeight = iconMgr->iconBuffer.getSize() + 1; //+ 1 for line between rows
        }
        else
        {
            provLeft ->setIconManager(nullptr);
            provRight->setIconManager(nullptr);
            newRowHeight = IconBuffer(IconBuffer::SIZE_SMALL).getSize() + 1; //+ 1 for line between rows
        }
        gridLeft  .setRowHeight(newRowHeight);
        gridCenter.setRowHeight(newRowHeight);
        gridRight .setRowHeight(newRowHeight);
    }
    else
        assert(false);
}


void gridview::clearSelection(Grid& gridLeft, Grid& gridCenter, Grid& gridRight)
{
    gridLeft  .clearSelection();
    gridCenter.clearSelection();
    gridRight .clearSelection();
}

void gridview::refresh(Grid& gridLeft, Grid& gridCenter, Grid& gridRight)
{
    gridLeft  .Refresh();
    gridCenter.Refresh();
    gridRight .Refresh();
}


void gridview::setNavigationMarker(Grid& gridLeft,
                                   std::vector<const HierarchyObject*>&& markedFiles,
                                   std::vector<const HierarchyObject*>&& markedContainer)
{
    if (auto* provLeft  = dynamic_cast<GridDataLeft*>(gridLeft.getDataProvider()))
        provLeft->setNavigationMarker(std::move(markedFiles), std::move(markedContainer));
    else
        assert(false);
    gridLeft.Refresh();
}


void gridview::showSyncAction(Grid& gridCenter, bool value)
{
    if (auto* provMiddle = dynamic_cast<GridDataMiddle*>(gridCenter.getDataProvider()))
        provMiddle->showSyncAction(value);
    else
        assert(false);
}

wxBitmap zen::getSyncOpImage(SyncOperation syncOp)
{
    switch (syncOp) //evaluate comparison result and sync direction
    {
        case SO_CREATE_NEW_LEFT:
            return GlobalResources::getImage(L"createLeftSmall");
        case SO_CREATE_NEW_RIGHT:
            return GlobalResources::getImage(L"createRightSmall");
        case SO_DELETE_LEFT:
            return GlobalResources::getImage(L"deleteLeftSmall");
        case SO_DELETE_RIGHT:
            return GlobalResources::getImage(L"deleteRightSmall");
        case SO_MOVE_LEFT_SOURCE:
            return GlobalResources::getImage(L"moveLeftSourceSmall");
        case SO_MOVE_LEFT_TARGET:
            return GlobalResources::getImage(L"moveLeftTargetSmall");
        case SO_MOVE_RIGHT_SOURCE:
            return GlobalResources::getImage(L"moveRightSourceSmall");
        case SO_MOVE_RIGHT_TARGET:
            return GlobalResources::getImage(L"moveRightTargetSmall");
        case SO_OVERWRITE_RIGHT:
            return GlobalResources::getImage(L"updateRightSmall");
        case SO_COPY_METADATA_TO_RIGHT:
            return GlobalResources::getImage(L"moveRightSmall");
        case SO_OVERWRITE_LEFT:
            return GlobalResources::getImage(L"updateLeftSmall");
        case SO_COPY_METADATA_TO_LEFT:
            return GlobalResources::getImage(L"moveLeftSmall");
        case SO_DO_NOTHING:
            return GlobalResources::getImage(L"noneSmall");
        case SO_EQUAL:
            return GlobalResources::getImage(L"equalSmall");
        case SO_UNRESOLVED_CONFLICT:
            return GlobalResources::getImage(L"conflictSmall");
    }
    return wxNullBitmap;
}


wxBitmap zen::getCmpResultImage(CompareFilesResult cmpResult)
{
    switch (cmpResult)
    {
        case FILE_LEFT_SIDE_ONLY:
            return GlobalResources::getImage(L"leftOnlySmall");
        case FILE_RIGHT_SIDE_ONLY:
            return GlobalResources::getImage(L"rightOnlySmall");
        case FILE_LEFT_NEWER:
            return GlobalResources::getImage(L"leftNewerSmall");
        case FILE_RIGHT_NEWER:
            return GlobalResources::getImage(L"rightNewerSmall");
        case FILE_DIFFERENT:
            return GlobalResources::getImage(L"differentSmall");
        case FILE_EQUAL:
        case FILE_DIFFERENT_METADATA: //= sub-category of equal
            return GlobalResources::getImage(L"equalSmall");
        case FILE_CONFLICT:
            return GlobalResources::getImage(L"conflictSmall");
    }
    return wxNullBitmap;
}
