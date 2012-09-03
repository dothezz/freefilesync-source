// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) ZenJu (zenju AT gmx DOT de) - All Rights Reserved        *
// **************************************************************************

#include <set>
#include "tree_view.h"
#include <wx/settings.h>
#include <wx/menu.h>
#include <zen/i18n.h>
#include <zen/utf.h>
#include <zen/stl_tools.h>
#include <wx+/format_unit.h>
#include <wx+/rtl.h>
#include <wx+/context_menu.h>
#include "../lib/icon_buffer.h"
#include "../lib/resources.h"

using namespace zen;


inline
void TreeView::compressNode(Container& cont) //remove single-element sub-trees -> gain clarity + usability (call *after* inclusion check!!!)
{
    if (cont.subDirs.empty() || //single files node or...
        (cont.firstFile == nullptr && //single dir node...
         cont.subDirs.size() == 1  && //
         cont.subDirs[0].firstFile == nullptr && //...that is empty
         cont.subDirs[0].subDirs.empty()))       //
    {
        cont.subDirs.clear();
        cont.firstFile = nullptr;
    }
}


template <class Function> //(const FileSystemObject&) -> bool
void TreeView::extractVisibleSubtree(HierarchyObject& hierObj,  //in
                                     TreeView::Container& cont, //out
                                     Function pred)
{
    auto getBytes = [](const FileMapping& fileObj) -> UInt64 //MSVC screws up miserably if we put this lambda into std::for_each
    {
        //give accumulated bytes the semantics of a sync preview!
        if (fileObj.isActive())
            switch (fileObj.getSyncDir())
            {
                case SYNC_DIR_LEFT:
                    return fileObj.getFileSize<RIGHT_SIDE>();
                case SYNC_DIR_RIGHT:
                    return fileObj.getFileSize<LEFT_SIDE>();
                case SYNC_DIR_NONE:
                    break;
            }
        return std::max(fileObj.getFileSize<LEFT_SIDE>(), fileObj.getFileSize<RIGHT_SIDE>());
    };


    cont.firstFile = nullptr;
    std::for_each(hierObj.refSubFiles().begin(), hierObj.refSubFiles().end(),
                  [&](FileMapping& fileObj)
    {
        if (pred(fileObj))
        {
            cont.bytesNet += getBytes(fileObj);

            if (!cont.firstFile)
                cont.firstFile = fileObj.getId();
        }
    });
    cont.bytesGross += cont.bytesNet;

    if (!cont.firstFile)
        std::find_if(hierObj.refSubLinks().begin(), hierObj.refSubLinks().end(),
                     [&](SymLinkMapping& linkObj) -> bool
    {
        if (pred(linkObj))
        {
            cont.firstFile = linkObj.getId();
            return true;
        }
        return false;
    });

    cont.subDirs.reserve(hierObj.refSubDirs().size()); //avoid expensive reallocations!

    std::for_each(hierObj.refSubDirs().begin(), hierObj.refSubDirs().end(),
                  [&cont, pred](DirMapping& subDirObj)
    {
        cont.subDirs.push_back(TreeView::DirNodeImpl()); //
        auto& subDirView = cont.subDirs.back();
        TreeView::extractVisibleSubtree(subDirObj, subDirView, pred);
        cont.bytesGross += subDirView.bytesGross;

        if (pred(subDirObj) || subDirView.firstFile || !subDirView.subDirs.empty())
        {
            subDirView.objId = subDirObj.getId();
            compressNode(subDirView);
        }
        else
            cont.subDirs.pop_back();
    });
}


namespace
{
//generate nice percentage numbers which precisely sum up to 100
void calcPercentage(std::vector<std::pair<UInt64, int*>>& workList)
{
    const UInt64 total = std::accumulate(workList.begin(), workList.end(), UInt64(),
    [](UInt64 sum, const std::pair<UInt64, int*>& pair) { return sum + pair.first; });

    if (total == 0U) //this case doesn't work with the error minimizing algorithm below
    {
        std::for_each(workList.begin(), workList.end(), [](std::pair<UInt64, int*>& pair) { *pair.second = 0; });
        return;
    }

    int remainingPercent = 100;
    std::for_each(workList.begin(), workList.end(),
                  [&](std::pair<UInt64, int*>& pair)
    {
        *pair.second = to<double>(pair.first) * 100 / to<double>(total); //round down
        remainingPercent -= *pair.second;
    });

    //find #remainingPercent items with largest absolute error
    remainingPercent = std::min(remainingPercent, static_cast<int>(workList.size()));
    if (remainingPercent > 0)
    {
        std::nth_element(workList.begin(), workList.begin() + remainingPercent - 1, workList.end(),
                         [total](const std::pair<UInt64, int*>& lhs, const std::pair<UInt64, int*>& rhs)
        {
            //return std::abs(*lhs.second - to<double>(lhs.first) * 100 / total) > std::abs(*rhs.second - to<double>(rhs.first) * 100 / total);
            return (to<double>(lhs.first) - to<double>(rhs.first)) * 100 / to<double>(total) > *lhs.second - *rhs.second;
        });

        //distribute remaining percent so that overall error is minimized as much as possible
        std::for_each(workList.begin(), workList.begin() + remainingPercent, [&](std::pair<UInt64, int*>& pair) { ++*pair.second; });
    }
}
}


template <bool ascending>
struct TreeView::LessShortName
{
    bool operator()(const TreeLine& lhs, const TreeLine& rhs)
    {
        //files last (irrespective of sort direction)
        if (lhs.type_ == TreeView::TYPE_FILES)
            return false;
        else if (rhs.type_ == TreeView::TYPE_FILES)
            return true;

        if (lhs.type_ != rhs.type_)       //
            return lhs.type_ < rhs.type_; //shouldn't happen! Root nodes are never sorted

        switch (lhs.type_)
        {
            case TreeView::TYPE_ROOT:
                return false;

            case TreeView::TYPE_DIRECTORY:
            {
                const auto* dirObjL = dynamic_cast<const DirMapping*>(FileSystemObject::retrieve(static_cast<const TreeView::DirNodeImpl*>(lhs.node_)->objId));
                const auto* dirObjR = dynamic_cast<const DirMapping*>(FileSystemObject::retrieve(static_cast<const TreeView::DirNodeImpl*>(rhs.node_)->objId));

                if (!dirObjL)  //might be pathologic, but it's covered
                    return false;
                else if (!dirObjR)
                    return true;

                return makeSortDirection(LessFilename(), Int2Type<ascending>())(dirObjL->getObjShortName(), dirObjR->getObjShortName());
            }

            case TreeView::TYPE_FILES:
                break;
        }
        assert(false);
        return false; //:= all equal
    }
};


template <bool ascending>
void TreeView::sortSingleLevel(std::vector<TreeLine>& items, ColumnTypeNavi columnType)
{
    auto getBytes = [](const TreeLine& line) -> UInt64
    {
        switch (line.type_)
        {
            case TreeView::TYPE_ROOT:
            case TreeView::TYPE_DIRECTORY:
                return line.node_->bytesGross;
            case TreeView::TYPE_FILES:
                return line.node_->bytesNet;
        }
        assert(false);
        return 0U;
    };

    const auto lessBytes = [&](const TreeLine& lhs, const TreeLine& rhs) { return getBytes(lhs) < getBytes(rhs); };

    switch (columnType)
    {
        case COL_TYPE_NAVI_BYTES:
            std::sort(items.begin(), items.end(), makeSortDirection(lessBytes, Int2Type<ascending>()));
            break;

        case COL_TYPE_NAVI_DIRECTORY:
            std::sort(items.begin(), items.end(), LessShortName<ascending>());
            break;
    }
}


void TreeView::getChildren(const Container& cont, size_t level, std::vector<TreeLine>& output)
{
    output.clear();
    output.reserve(cont.subDirs.size() + 1); //keep pointers in "workList" valid
    std::vector<std::pair<UInt64, int*>> workList;

    std::for_each(cont.subDirs.begin(), cont.subDirs.end(),
                  [&output, level, &workList](const DirNodeImpl& subDir)
    {
        output.push_back(TreeView::TreeLine(level, 0, &subDir, TreeView::TYPE_DIRECTORY));
        workList.push_back(std::make_pair(subDir.bytesGross, &output.back().percent_));
    });

    if (cont.firstFile)
    {
        output.push_back(TreeLine(level, 0, &cont, TreeView::TYPE_FILES));
        workList.push_back(std::make_pair(cont.bytesNet, &output.back().percent_));
    }
    calcPercentage(workList);

    if (sortAscending)
        sortSingleLevel<true>(output, sortColumn);
    else
        sortSingleLevel<false>(output, sortColumn);
}


void TreeView::applySubView(std::vector<RootNodeImpl>&& newView)
{
    //preserve current node expansion status
    auto getHierAlias = [](const TreeView::TreeLine& tl) -> const HierarchyObject*
    {
        switch (tl.type_)
        {
            case TreeView::TYPE_ROOT:
                return static_cast<const RootNodeImpl*>(tl.node_)->baseMap.get();

            case TreeView::TYPE_DIRECTORY:
                if (auto dirObj = dynamic_cast<const DirMapping*>(FileSystemObject::retrieve(static_cast<const DirNodeImpl*>(tl.node_)->objId)))
                    return dirObj;
                break;

            case TreeView::TYPE_FILES:
                break; //none!!!
        }
        return nullptr;
    };

    zen::hash_set<const HierarchyObject*> expandedNodes;
    if (!flatTree.empty())
    {
        auto iter = flatTree.begin();
        for (auto iterNext = flatTree.begin() + 1; iterNext != flatTree.end(); ++iterNext, ++iter)
            if (iter->level_ < iterNext->level_)
                if (auto hierObj = getHierAlias(*iter))
                    expandedNodes.insert(hierObj);
    }

    //update view on full data
    folderCmpView.swap(newView); //newView may be an alias for folderCmpView! see sorting!

    //set default flat tree
    flatTree.clear();

    if (folderCmp.size() == 1) //single folder pair case (empty pairs were already removed!) do NOT use folderCmpView for this check!
    {
        if (!folderCmpView.empty()) //it may really be!
            getChildren(folderCmpView[0], 0, flatTree); //do not show root
    }
    else
    {
        std::vector<std::pair<UInt64, int*>> workList;
        flatTree.reserve(folderCmpView.size()); //keep pointers in "workList" valid

        std::for_each(folderCmpView.begin(), folderCmpView.end(),
                      [&](const RootNodeImpl& root)
        {
            flatTree.push_back(TreeView::TreeLine(0, 0, &root, TreeView::TYPE_ROOT));
            workList.push_back(std::make_pair(root.bytesGross, &flatTree.back().percent_));
        });

        calcPercentage(workList);
    }

    //restore node expansion status
    for (size_t row = 0; row < flatTree.size(); ++row) //flatTree size changes within loop!
    {
        const TreeLine& line = flatTree[row];

        if (auto hierObj = getHierAlias(line))
            if (expandedNodes.find(hierObj) != expandedNodes.end())
            {
                std::vector<TreeLine> newLines;
                getChildren(*line.node_, line.level_ + 1, newLines);

                flatTree.insert(flatTree.begin() + row + 1, newLines.begin(), newLines.end());
            }
    }
}


template <class Predicate>
void TreeView::updateView(Predicate pred)
{
    //update view on full data
    std::vector<RootNodeImpl> newView;
    newView.reserve(folderCmp.size()); //avoid expensive reallocations!

    std::for_each(folderCmp.begin(), folderCmp.end(),
                  [&](const std::shared_ptr<BaseDirMapping>& baseObj)
    {
        newView.push_back(TreeView::RootNodeImpl());
        RootNodeImpl& root = newView.back();
        this->extractVisibleSubtree(*baseObj, root, pred); //"this->" is bogus for a static method, but GCC screws this one up

        //warning: the following lines are almost 1:1 copy from extractVisibleSubtree:
        //however we *cannot* reuse code here; this were only possible if we could replace "std::vector<RootNodeImpl>" by "Container"!
        if (root.firstFile || !root.subDirs.empty())
        {
            root.baseMap = baseObj;
            this->compressNode(root); //"this->" required by two-pass lookup as enforced by GCC 4.7
        }
        else
            newView.pop_back();
    });

    applySubView(std::move(newView));
}


void TreeView::setSortDirection(ColumnTypeNavi colType, bool ascending) //apply permanently!
{
    sortColumn    = colType;
    sortAscending = ascending;

    //reapply current view
    applySubView(std::move(folderCmpView));
}


bool TreeView::getDefaultSortDirection(ColumnTypeNavi colType)
{
    switch (colType)
    {
        case COL_TYPE_NAVI_BYTES:
            return false;
        case COL_TYPE_NAVI_DIRECTORY:
            return true;
    }
    assert(false);
    return true;
}


TreeView::NodeStatus TreeView::getStatus(size_t row) const
{
    if (row < flatTree.size())
    {
        if (row + 1 < flatTree.size() && flatTree[row + 1].level_ > flatTree[row].level_)
            return TreeView::STATUS_EXPANDED;

        //it's either reduced or empty
        switch (flatTree[row].type_)
        {
            case TreeView::TYPE_DIRECTORY:
            case TreeView::TYPE_ROOT:
                return flatTree[row].node_->firstFile || !flatTree[row].node_->subDirs.empty() ? TreeView::STATUS_REDUCED : TreeView::STATUS_EMPTY;

            case TreeView::TYPE_FILES:
                return TreeView::STATUS_EMPTY;
        }
    }
    return TreeView::STATUS_EMPTY;
}


void TreeView::expandNode(size_t row)
{
    if (row < flatTree.size())
    {
        std::vector<TreeLine> newLines;

        switch (flatTree[row].type_)
        {
            case TreeView::TYPE_ROOT:
            case TreeView::TYPE_DIRECTORY:
                getChildren(*flatTree[row].node_, flatTree[row].level_ + 1, newLines);
                break;
            case TreeView::TYPE_FILES:
                break;
        }
        flatTree.insert(flatTree.begin() + row + 1, newLines.begin(), newLines.end());
    }
}


void TreeView::reduceNode(size_t row)
{
    if (row < flatTree.size())
    {
        const size_t parentLevel = flatTree[row].level_;

        bool done = false;
        flatTree.erase(std::remove_if(flatTree.begin() + row + 1, flatTree.end(),
                                      [&](const TreeLine& line) -> bool
        {
            if (done)
                return false;
            if (line.level_ > parentLevel)
                return true;
            else
            {
                done = true;
                return false;
            }
        }), flatTree.end());
    }
}


ptrdiff_t TreeView::getParent(size_t row) const
{
    if (row < flatTree.size())
    {
        const size_t level = flatTree[row].level_;

        while (row-- > 0)
            if (flatTree[row].level_ < level)
                return row;
    }
    return -1;
}


void TreeView::updateCmpResult(bool hideFiltered,
                               bool leftOnlyFilesActive,
                               bool rightOnlyFilesActive,
                               bool leftNewerFilesActive,
                               bool rightNewerFilesActive,
                               bool differentFilesActive,
                               bool equalFilesActive,
                               bool conflictFilesActive)
{
    updateView([&](const FileSystemObject& fsObj) -> bool
    {
        if (hideFiltered && !fsObj.isActive())
            return false;

        switch (fsObj.getCategory())
        {
            case FILE_LEFT_SIDE_ONLY:
                return leftOnlyFilesActive;
            case FILE_RIGHT_SIDE_ONLY:
                return rightOnlyFilesActive;
            case FILE_LEFT_NEWER:
                return leftNewerFilesActive;
            case FILE_RIGHT_NEWER:
                return rightNewerFilesActive;
            case FILE_DIFFERENT:
                return differentFilesActive;
            case FILE_EQUAL:
            case FILE_DIFFERENT_METADATA: //= sub-category of equal
                return equalFilesActive;
            case FILE_CONFLICT:
                return conflictFilesActive;
        }
        assert(false);
        return true;
    });
}


void TreeView::updateSyncPreview(bool hideFiltered,
                                 bool syncCreateLeftActive,
                                 bool syncCreateRightActive,
                                 bool syncDeleteLeftActive,
                                 bool syncDeleteRightActive,
                                 bool syncDirOverwLeftActive,
                                 bool syncDirOverwRightActive,
                                 bool syncDirNoneActive,
                                 bool syncEqualActive,
                                 bool conflictFilesActive)
{
    updateView([&](const FileSystemObject& fsObj) -> bool
    {
        if (hideFiltered && !fsObj.isActive())
            return false;

        switch (fsObj.getSyncOperation())
        {
            case SO_CREATE_NEW_LEFT:
            case SO_MOVE_LEFT_TARGET:
                return syncCreateLeftActive;
            case SO_CREATE_NEW_RIGHT:
            case SO_MOVE_RIGHT_TARGET:
                return syncCreateRightActive;
            case SO_DELETE_LEFT:
            case SO_MOVE_LEFT_SOURCE:
                return syncDeleteLeftActive;
            case SO_DELETE_RIGHT:
            case SO_MOVE_RIGHT_SOURCE:
                return syncDeleteRightActive;
            case SO_OVERWRITE_RIGHT:
            case SO_COPY_METADATA_TO_RIGHT:
                return syncDirOverwRightActive;
            case SO_OVERWRITE_LEFT:
            case SO_COPY_METADATA_TO_LEFT:
                return syncDirOverwLeftActive;
            case SO_DO_NOTHING:
                return syncDirNoneActive;
            case SO_EQUAL:
                return syncEqualActive;
            case SO_UNRESOLVED_CONFLICT:
                return conflictFilesActive;
        }
        assert(false);
        return true;
    });
}


void TreeView::setData(FolderComparison& newData)
{
    std::vector<TreeLine    >().swap(flatTree);      //free mem
    std::vector<RootNodeImpl>().swap(folderCmpView); //
    folderCmp = newData;

    //remove truly empty folder pairs as early as this: we want to distinguish single/multiple folder pair cases by looking at "folderCmp"
    vector_remove_if(folderCmp, [](const std::shared_ptr<BaseDirMapping>& baseObj)
    {
        return baseObj->getBaseDirPf<LEFT_SIDE >().empty() &&
               baseObj->getBaseDirPf<RIGHT_SIDE>().empty();
    });
}


std::unique_ptr<TreeView::Node> TreeView::getLine(size_t row) const
{
    if (row < flatTree.size())
    {
        const auto level = flatTree[row].level_;

        const int percent = flatTree[row].percent_;
        switch (flatTree[row].type_)
        {
            case TreeView::TYPE_ROOT:
            {
                const auto* root = static_cast<const TreeView::RootNodeImpl*>(flatTree[row].node_);
                return make_unique<TreeView::RootNode>(percent, getStatus(row), root->bytesGross, *(root->baseMap));
            }
            break;

            case TreeView::TYPE_DIRECTORY:
            {
                const auto* dir = static_cast<const TreeView::DirNodeImpl*>(flatTree[row].node_);
                if (auto dirObj = dynamic_cast<DirMapping*>(FileSystemObject::retrieve(dir->objId)))
                    return make_unique<TreeView::DirNode>(percent, level, getStatus(row), dir->bytesGross, *dirObj);
            }
            break;

            case TreeView::TYPE_FILES:
            {
                const auto* parentDir = flatTree[row].node_;
                if (auto firstFile = FileSystemObject::retrieve(parentDir->firstFile))
                    return make_unique<TreeView::FilesNode>(percent, level, parentDir->bytesNet, *firstFile);
            }
            break;
        }
    }
    return nullptr;
}

//##########################################################################################################

namespace
{
const wxColour COLOR_LEVEL0(0xcc, 0xcc, 0xff);
const wxColour COLOR_LEVEL1(0xcc, 0xff, 0xcc);
const wxColour COLOR_LEVEL2(0xff, 0xff, 0x99);

const wxColour COLOR_LEVEL3(0xcc, 0xff, 0xff);
const wxColour COLOR_LEVEL4(0xff, 0xcc, 0xff);
const wxColour COLOR_LEVEL5(0x99, 0xff, 0xcc);

const wxColour COLOR_LEVEL6(0xcc, 0xcc, 0x99);
const wxColour COLOR_LEVEL7(0xff, 0xcc, 0xcc);
const wxColour COLOR_LEVEL8(0xcc, 0xff, 0x99);

const wxColour COLOR_LEVEL9 (0xff, 0xff, 0xcc);
const wxColour COLOR_LEVEL10(0xcc, 0xcc, 0xcc);
const wxColour COLOR_LEVEL11(0xff, 0xcc, 0x99);

const wxColour COLOR_PERCENTAGE_BORDER(198, 198, 198);
const wxColour COLOR_PERCENTAGE_BACKGROUND(0xf8, 0xf8, 0xf8);

//const wxColor COLOR_TREE_SELECTION_GRADIENT_FROM = wxColor( 89, 255,  99); //green: HSV: 88, 255, 172
//const wxColor COLOR_TREE_SELECTION_GRADIENT_TO   = wxColor(225, 255, 227); //       HSV: 88, 255, 240
const wxColor COLOR_TREE_SELECTION_GRADIENT_FROM = getColorSelectionGradientFrom();
const wxColor COLOR_TREE_SELECTION_GRADIENT_TO   = getColorSelectionGradientTo  ();


class GridDataNavi : private wxEvtHandler, public GridData
{
public:
    GridDataNavi(Grid& grid, const std::shared_ptr<TreeView>& treeDataView) : treeDataView_(treeDataView),
        fileIcon(IconBuffer(IconBuffer::SIZE_SMALL).genericFileIcon()),
        dirIcon (IconBuffer(IconBuffer::SIZE_SMALL).genericDirIcon ()),
        rootBmp(GlobalResources::getImage(L"rootFolder").ConvertToImage().Scale(fileIcon.GetWidth(), fileIcon.GetHeight(), wxIMAGE_QUALITY_HIGH)),
        widthNodeIcon(dirIcon.GetWidth()),
        widthLevelStep(widthNodeIcon),
        widthNodeStatus(GlobalResources::getImage(L"nodeExpanded").GetWidth()),
        grid_(grid),
        showPercentBar(true)
    {
        grid.getMainWin().Connect(wxEVT_KEY_DOWN, wxKeyEventHandler(GridDataNavi::onKeyDown), nullptr, this);
        grid.Connect(EVENT_GRID_MOUSE_LEFT_DOWN,       GridClickEventHandler(GridDataNavi::onMouseLeft          ), nullptr, this);
        grid.Connect(EVENT_GRID_MOUSE_LEFT_DOUBLE,     GridClickEventHandler(GridDataNavi::onMouseLeftDouble    ), nullptr, this);
        grid.Connect(EVENT_GRID_COL_LABEL_MOUSE_RIGHT, GridClickEventHandler(GridDataNavi::onGridLabelContext), nullptr, this );
        grid.Connect(EVENT_GRID_COL_LABEL_MOUSE_LEFT,  GridClickEventHandler(GridDataNavi::onGridLabelLeftClick ), nullptr, this );
    }

    void setShowPercentage(bool value) { showPercentBar = value; grid_.Refresh(); }
    bool getShowPercentage() const { return showPercentBar; }

private:
    virtual size_t getRowCount() const { return treeDataView_ ? treeDataView_->linesTotal() : 0; }

    virtual wxString getValue(size_t row, ColumnType colType) const
    {
        if (treeDataView_)
        {
            if (std::unique_ptr<TreeView::Node> node = treeDataView_->getLine(row))
                switch (static_cast<ColumnTypeNavi>(colType))
                {
                    case COL_TYPE_NAVI_BYTES:
                        return filesizeToShortString(to<Int64>(node->bytes_));

                    case COL_TYPE_NAVI_DIRECTORY:
                        if (const TreeView::RootNode* root = dynamic_cast<const TreeView::RootNode*>(node.get()))
                        {
                            const wxString dirLeft  = utfCvrtTo<wxString>(beforeLast(root->baseMap_.getBaseDirPf<LEFT_SIDE >(), FILE_NAME_SEPARATOR));
                            const wxString dirRight = utfCvrtTo<wxString>(beforeLast(root->baseMap_.getBaseDirPf<RIGHT_SIDE>(), FILE_NAME_SEPARATOR));

                            if (dirLeft.empty())
                                return dirRight;
                            else if (dirRight.empty())
                                return dirLeft;
                            else
                                return utfCvrtTo<wxString>(dirLeft + L" \x2212 " + dirRight); //\x2212 = unicode minus
                        }
                        else if (const TreeView::DirNode* dir = dynamic_cast<const TreeView::DirNode*>(node.get()))
                            return utfCvrtTo<wxString>(dir->dirObj_.getObjShortName());
                        else if (dynamic_cast<const TreeView::FilesNode*>(node.get()))
                            return _("Files");
                        break;
                }
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

        if (treeDataView_) //draw sort marker
        {
            auto sortInfo = treeDataView_->getSortDirection();
            if (colType == static_cast<ColumnType>(sortInfo.first))
            {
                const wxBitmap& marker = GlobalResources::getImage(sortInfo.second ? L"sortAscending" : L"sortDescending");
                wxPoint markerBegin = rectInside.GetTopLeft() + wxPoint((rectInside.width - marker.GetWidth()) / 2, 0);
                dc.DrawBitmap(marker, markerBegin, true); //respect 2-pixel border
            }
        }
    }

    static const int CELL_BORDER = 2;

    virtual void renderRowBackgound(wxDC& dc, const wxRect& rect, size_t row, bool enabled, bool selected, bool hasFocus)
    {
        if (enabled)
        {
            if (selected)
                dc.GradientFillLinear(rect, COLOR_TREE_SELECTION_GRADIENT_FROM, COLOR_TREE_SELECTION_GRADIENT_TO, wxEAST);
            //ignore focus
            else
                clearArea(dc, rect, wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOW));
        }
        else
            clearArea(dc, rect, wxSystemSettings::GetColour(wxSYS_COLOUR_BTNFACE));
    }

    virtual void renderCell(Grid& grid, wxDC& dc, const wxRect& rect, size_t row, ColumnType colType)
    {
        //wxRect rectTmp= drawCellBorder(dc, rect);
        wxRect rectTmp = rect;

        //  Partitioning:
        //   ___________________________________________________________________________________________
        //  | space | border | percentage bar | 2 x border | node status | border |icon | border | rest |
        //   --------------------------------------------------------------------------------------------
        // -> synchronize renderCell() <-> getBestSize() <-> onMouseLeft()

        if (static_cast<ColumnTypeNavi>(colType) == COL_TYPE_NAVI_DIRECTORY && treeDataView_)
        {
            if (std::unique_ptr<TreeView::Node> node = treeDataView_->getLine(row))
            {
                ////clear first secion:
                //clearArea(dc, wxRect(rect.GetTopLeft(), wxSize(
                //                         node->level_ * widthLevelStep + CELL_BORDER + //width
                //                         (showPercentBar ? widthPercentBar + 2 * CELL_BORDER : 0) + //
                //                         widthNodeStatus + CELL_BORDER + widthNodeIcon + CELL_BORDER, //
                //                         rect.height)), wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOW));

                //consume space
                rectTmp.x     += static_cast<int>(node->level_) * widthLevelStep;
                rectTmp.width -= static_cast<int>(node->level_) * widthLevelStep;

                rectTmp.x     += CELL_BORDER;
                rectTmp.width -= CELL_BORDER;

                if (rectTmp.width > 0)
                {
                    //percentage bar
                    if (showPercentBar)
                    {
                        const wxColour brushCol = [&]() -> wxColour
                        {
                            switch (node->level_ % 12)
                            {
                                case 0:
                                    return COLOR_LEVEL0;
                                case 1:
                                    return COLOR_LEVEL1;
                                case 2:
                                    return COLOR_LEVEL2;
                                case 3:
                                    return COLOR_LEVEL3;
                                case 4:
                                    return COLOR_LEVEL4;
                                case 5:
                                    return COLOR_LEVEL5;
                                case 6:
                                    return COLOR_LEVEL6;
                                case 7:
                                    return COLOR_LEVEL7;
                                case 8:
                                    return COLOR_LEVEL8;
                                case 9:
                                    return COLOR_LEVEL9;
                                case 10:
                                    return COLOR_LEVEL10;
                                default:
                                    return COLOR_LEVEL11;
                            }
                        }();

                        const wxRect areaPerc(rectTmp.x, rectTmp.y + 2, widthPercentBar, rectTmp.height - 4);
                        {
                            //background
                            wxDCPenChanger dummy(dc, *wxTRANSPARENT_PEN);
                            wxDCBrushChanger dummy2(dc, COLOR_PERCENTAGE_BACKGROUND);
                            dc.DrawRectangle(areaPerc);

                            //inner area
                            dc.SetBrush(brushCol);

                            wxRect areaPercTmp = areaPerc;
                            areaPercTmp.width -= 2; //do not include left/right border
                            areaPercTmp.x += 1;     //
                            areaPercTmp.width *= node->percent_ / 100.0;
                            dc.DrawRectangle(areaPercTmp);

                            //outer border
                            dc.SetPen(COLOR_PERCENTAGE_BORDER);
                            dc.SetBrush(*wxTRANSPARENT_BRUSH);
                            dc.DrawRectangle(areaPerc);
                        }

                        wxDCTextColourChanger dummy3(dc, *wxBLACK); //accessibility: always set both foreground AND background colors!
                        dc.DrawLabel(numberTo<wxString>(node->percent_) + L"%", areaPerc, wxALIGN_CENTER);

                        rectTmp.x     += widthPercentBar + 2 * CELL_BORDER;
                        rectTmp.width -= widthPercentBar + 2 * CELL_BORDER;
                    }
                    if (rectTmp.width > 0)
                    {
                        //node status
                        auto drawStatus = [&](const wchar_t* image)
                        {
                            const wxBitmap& bmp = GlobalResources::getImage(image);

                            wxRect rectStat(rectTmp.GetTopLeft(), wxSize(bmp.GetWidth(), bmp.GetHeight()));
                            rectStat.y += (rectTmp.height - rectStat.height) / 2;

                            //clearArea(dc, rectStat, wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOW));
                            clearArea(dc, rectStat, *wxWHITE); //accessibility: always set both foreground AND background colors!
                            drawBitmapRtlMirror(dc, bmp, rectStat, wxALIGN_CENTER, buffer);
                        };

                        switch (node->status_)
                        {
                            case TreeView::STATUS_EXPANDED:
                                drawStatus(L"nodeExpanded");
                                break;
                            case TreeView::STATUS_REDUCED:
                                drawStatus(L"nodeReduced");
                                break;
                            case TreeView::STATUS_EMPTY:
                                break;
                        }

                        rectTmp.x     += widthNodeStatus + CELL_BORDER;
                        rectTmp.width -= widthNodeStatus + CELL_BORDER;
                        if (rectTmp.width > 0)
                        {
                            bool isActive = true;
                            //icon
                            if (dynamic_cast<const TreeView::RootNode*>(node.get()))
                                drawBitmapRtlNoMirror(dc, rootBmp, rectTmp, wxALIGN_LEFT | wxALIGN_CENTER_VERTICAL, buffer);
                            else if (auto dir = dynamic_cast<const TreeView::DirNode*>(node.get()))
                            {
                                drawIconRtlNoMirror(dc, dirIcon, rectTmp.GetTopLeft() + wxPoint(0, (rectTmp.height - dirIcon.GetHeight()) / 2), buffer);
                                isActive = dir->dirObj_.isActive();
                            }
                            else if (dynamic_cast<const TreeView::FilesNode*>(node.get()))
                                drawIconRtlNoMirror(dc, fileIcon, rectTmp.GetTopLeft() + wxPoint(0, (rectTmp.height - fileIcon.GetHeight()) / 2), buffer);

                            //convert icon to greyscale if row is not active
                            if (!isActive)
                            {
                                wxBitmap bmp(widthNodeIcon, rectTmp.height);
                                wxMemoryDC memDc(bmp);
                                memDc.Blit(0, 0, widthNodeIcon, rectTmp.height, &dc, rectTmp.x, rectTmp.y); //blit in

                                bmp = wxBitmap(bmp.ConvertToImage().ConvertToGreyscale(1.0/3, 1.0/3, 1.0/3)); //treat all channels equally!
                                memDc.SelectObject(bmp);

                                dc.Blit(rectTmp.x, rectTmp.y, widthNodeIcon, rectTmp.height, &memDc, 0, 0); //blit out
                            }

                            rectTmp.x     += widthNodeIcon + CELL_BORDER;
                            rectTmp.width -= widthNodeIcon + CELL_BORDER;

                            if (rectTmp.width > 0)
                                drawCellText(dc, rectTmp, getValue(row, colType), grid.IsEnabled(), wxALIGN_LEFT | wxALIGN_CENTER_VERTICAL);
                        }
                    }
                }
            }
        }
        else
        {
            int alignment = wxALIGN_LEFT | wxALIGN_CENTER_VERTICAL;

            //have file size right-justified (but don't change for RTL languages)
            if (static_cast<ColumnTypeNavi>(colType) == COL_TYPE_NAVI_BYTES && grid.GetLayoutDirection() != wxLayout_RightToLeft)
            {
                rectTmp.width -= 2 * CELL_BORDER;
                alignment = wxALIGN_RIGHT | wxALIGN_CENTER_VERTICAL;
            }
            else //left-justified
            {
                rectTmp.x     += 2 * CELL_BORDER;
                rectTmp.width -= 2 * CELL_BORDER;
            }

            drawCellText(dc, rectTmp, getValue(row, colType), grid.IsEnabled(), alignment);
        }
    }

    virtual size_t getBestSize(wxDC& dc, size_t row, ColumnType colType)
    {
        // -> synchronize renderCell() <-> getBestSize() <-> onMouseLeft()

        if (static_cast<ColumnTypeNavi>(colType) == COL_TYPE_NAVI_DIRECTORY && treeDataView_)
        {
            if (std::unique_ptr<TreeView::Node> node = treeDataView_->getLine(row))
                return node->level_ * widthLevelStep + CELL_BORDER + (showPercentBar ? widthPercentBar + 2 * CELL_BORDER : 0) + widthNodeStatus + CELL_BORDER
                       + widthNodeIcon + CELL_BORDER + dc.GetTextExtent(getValue(row, colType)).GetWidth() +
                       CELL_BORDER; //additional border from right
            else
                return 0;
        }
        else
            return 2 * CELL_BORDER + dc.GetTextExtent(getValue(row, colType)).GetWidth() +
                   2 * CELL_BORDER; //include border from right!
    }

    virtual wxString getColumnLabel(ColumnType colType) const
    {
        switch (static_cast<ColumnTypeNavi>(colType))
        {
            case COL_TYPE_NAVI_BYTES:
                return _("Size");
            case COL_TYPE_NAVI_DIRECTORY:
                return _("Name");
        }
        return wxEmptyString;
    }

    void onMouseLeft(GridClickEvent& event)
    {
        if (treeDataView_)
        {
            bool clickOnNodeStatus = false;
            if (static_cast<ColumnTypeNavi>(event.colType_) == COL_TYPE_NAVI_DIRECTORY)
                if (std::unique_ptr<TreeView::Node> node = treeDataView_->getLine(event.row_))
                {
                    const int absX = grid_.CalcUnscrolledPosition(event.GetPosition()).x;
                    const wxRect cellArea = grid_.getCellArea(event.row_, event.colType_);
                    if (cellArea.width > 0 && cellArea.height > 0)
                    {
                        const int tolerance = 1;
                        const int xNodeStatusFirst = -tolerance + cellArea.x + static_cast<int>(node->level_) * widthLevelStep + CELL_BORDER + (showPercentBar ? widthPercentBar + 2 * CELL_BORDER : 0);
                        const int xNodeStatusLast  = xNodeStatusFirst + widthNodeStatus + 2 * tolerance;
                        // -> synchronize renderCell() <-> getBestSize() <-> onMouseLeft()

                        if (xNodeStatusFirst <= absX && absX < xNodeStatusLast)
                            clickOnNodeStatus = true;
                    }
                }
            //--------------------------------------------------------------------------------------------------

            if (clickOnNodeStatus && event.row_ >= 0)
                switch (treeDataView_->getStatus(event.row_))
                {
                    case TreeView::STATUS_EXPANDED:
                        return reduceNode(event.row_);
                    case TreeView::STATUS_REDUCED:
                        return expandNode(event.row_);
                    case TreeView::STATUS_EMPTY:
                        break;
                }
        }
        event.Skip();
    }

    void onMouseLeftDouble(GridClickEvent& event)
    {
        if (event.row_ >= 0 && treeDataView_)
            switch (treeDataView_->getStatus(event.row_))
            {
                case TreeView::STATUS_EXPANDED:
                    return reduceNode(event.row_);
                case TreeView::STATUS_REDUCED:
                    return expandNode(event.row_);
                case TreeView::STATUS_EMPTY:
                    break;
            }
        event.Skip();
    }

    void onKeyDown(wxKeyEvent& event)
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

        const size_t rowCount = grid_.getRowCount();
        if (rowCount == 0) return;

        size_t row =  grid_.getGridCursor().first;
        if (event.ShiftDown())
            ;
        else if (event.ControlDown())
            ;
        else
            switch (keyCode)
            {
                case WXK_LEFT:
                case WXK_NUMPAD_LEFT:
                    if (treeDataView_)
                        switch (treeDataView_->getStatus(row))
                        {
                            case TreeView::STATUS_EXPANDED:
                                return reduceNode(row);
                            case TreeView::STATUS_REDUCED:
                            case TreeView::STATUS_EMPTY:

                                const int parentRow = treeDataView_->getParent(row);
                                if (parentRow >= 0)
                                    grid_.setGridCursor(parentRow);
                                break;
                        }
                    return; //swallow event

                case WXK_RIGHT:
                case WXK_NUMPAD_RIGHT:
                    if (treeDataView_)
                        switch (treeDataView_->getStatus(row))
                        {
                            case TreeView::STATUS_EXPANDED:
                                grid_.setGridCursor(std::min(rowCount - 1, row + 1));
                                break;
                            case TreeView::STATUS_REDUCED:
                                return expandNode(row);
                            case TreeView::STATUS_EMPTY:
                                break;
                        }
                    return; //swallow event
            }

        event.Skip();
    }

    void onGridLabelContext(GridClickEvent& event)
    {
        ContextMenu menu;

        //--------------------------------------------------------------------------------------------------------
        auto toggleColumn = [&](const Grid::ColumnAttribute& ca)
        {
            auto colAttr = grid_.getColumnConfig();

            for (auto iter = colAttr.begin(); iter != colAttr.end(); ++iter)
                if (iter->type_ == ca.type_)
                {
                    iter->visible_ = !ca.visible_;
                    grid_.setColumnConfig(colAttr);
                    return;
                }
        };

        const auto& colAttr = grid_.getColumnConfig();
        for (auto iter = colAttr.begin(); iter != colAttr.end(); ++iter)
        {
            const Grid::ColumnAttribute& ca = *iter;

            menu.addCheckBox(getColumnLabel(ca.type_), [ca, toggleColumn]() { toggleColumn(ca); },
            ca.visible_, ca.type_ != static_cast<ColumnType>(COL_TYPE_NAVI_DIRECTORY)); //do not allow user to hide file name column!
        }
        //--------------------------------------------------------------------------------------------------------
        menu.addCheckBox(_("Percentage"), [this] { setShowPercentage(!getShowPercentage()); }, getShowPercentage());
        //--------------------------------------------------------------------------------------------------------
        menu.addSeparator();

        auto setDefaultColumns = [&]
        {
            setShowPercentage(defaultValueShowPercentage);
            grid_.setColumnConfig(treeview::convertConfig(getDefaultColumnAttributesNavi()));
        };
        menu.addItem(_("&Default"), setDefaultColumns); //'&' -> reuse text from "default" buttons elsewhere

        menu.popup(grid_);

        event.Skip();
    }

    void onGridLabelLeftClick(GridClickEvent& event)
    {
        if (treeDataView_)
        {
            const auto colTypeNavi = static_cast<ColumnTypeNavi>(event.colType_);
            bool sortAscending = TreeView::getDefaultSortDirection(colTypeNavi);

            const auto sortInfo = treeDataView_->getSortDirection();
            if (sortInfo.first == colTypeNavi)
                sortAscending = !sortInfo.second;

            treeDataView_->setSortDirection(colTypeNavi, sortAscending);
            grid_.clearSelection();
            grid_.Refresh();
        }
    }

    void expandNode(size_t row)
    {
        treeDataView_->expandNode(row);
        grid_.Refresh(); //this one clears selection (changed row count)
        grid_.setGridCursor(row);
        //grid_.autoSizeColumns(); -> doesn't look as good as expected
    }

    void reduceNode(size_t row)
    {
        treeDataView_->reduceNode(row);
        grid_.Refresh(); //this one clears selection (changed row count)
        grid_.setGridCursor(row);
        //grid_.autoSizeColumns(); -> doesn't look as good as expected
    }

    std::shared_ptr<TreeView> treeDataView_;
    const wxIcon fileIcon;
    const wxIcon dirIcon;
    const wxBitmap rootBmp;
    std::unique_ptr<wxBitmap> buffer; //avoid costs of recreating this temporal variable
    const int widthNodeIcon;
    const int widthLevelStep;
    const int widthNodeStatus;
    static const int widthPercentBar = 60;
    Grid& grid_;
    bool showPercentBar;
};
}


void treeview::init(Grid& grid, const std::shared_ptr<TreeView>& treeDataView)
{
    grid.setDataProvider(std::make_shared<GridDataNavi>(grid, treeDataView));
    grid.showRowLabel(false);
    grid.setRowHeight(IconBuffer(IconBuffer::SIZE_SMALL).getSize() + 2); //add some space
}


void treeview::setShowPercentage(Grid& grid, bool value)
{
    if (auto* prov = dynamic_cast<GridDataNavi*>(grid.getDataProvider()))
        prov->setShowPercentage(value);
    else
        assert(false);
}


bool treeview::getShowPercentage(const Grid& grid)
{
    if (auto* prov = dynamic_cast<const GridDataNavi*>(grid.getDataProvider()))
        return prov->getShowPercentage();
    assert(false);
    return true;
}


namespace
{
std::vector<ColumnAttributeNavi> makeConsistent(const std::vector<ColumnAttributeNavi>& attribs)
{
    std::set<ColumnTypeNavi> usedTypes;

    std::vector<ColumnAttributeNavi> output;
    //remove duplicates
    std::copy_if(attribs.begin(), attribs.end(), std::back_inserter(output),
    [&](const ColumnAttributeNavi& a) { return usedTypes.insert(a.type_).second; });

    //make sure each type is existing!
    const auto& defAttr = getDefaultColumnAttributesNavi();
    std::copy_if(defAttr.begin(), defAttr.end(), std::back_inserter(output),
    [&](const ColumnAttributeNavi& a) { return usedTypes.insert(a.type_).second; });

    return output;
}
}

std::vector<Grid::ColumnAttribute> treeview::convertConfig(const std::vector<ColumnAttributeNavi>& attribs)
{
    const auto& attribClean = makeConsistent(attribs);

    std::vector<Grid::ColumnAttribute> output;
    std::transform(attribClean.begin(), attribClean.end(), std::back_inserter(output),
    [&](const ColumnAttributeNavi& ca) { return Grid::ColumnAttribute(static_cast<ColumnType>(ca.type_), ca.offset_, ca.stretch_, ca.visible_); });

    return output;
}


std::vector<ColumnAttributeNavi> treeview::convertConfig(const std::vector<Grid::ColumnAttribute>& attribs)
{
    std::vector<ColumnAttributeNavi> output;

    std::transform(attribs.begin(), attribs.end(), std::back_inserter(output),
    [&](const Grid::ColumnAttribute& ca) { return ColumnAttributeNavi(static_cast<ColumnTypeNavi>(ca.type_), ca.offset_, ca.stretch_, ca.visible_); });

    return makeConsistent(output);
}
