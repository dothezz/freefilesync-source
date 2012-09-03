// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) ZenJu (zenju AT gmx DOT de) - All Rights Reserved        *
// **************************************************************************

#ifndef TREE_H_INCLUDED_841703190201835280256673425
#define TREE_H_INCLUDED_841703190201835280256673425

#include <wx+/grid.h>
#include "../file_hierarchy.h"
#include "column_attr.h"
#include <zen/optional.h>

namespace zen
{
//tree view of FolderComparison
class TreeView
{
public:
    TreeView() :
        sortColumn(defaultValueLastSortColumn),
        sortAscending(defaultValueLastSortAscending) {}

    void setData(FolderComparison& newData); //set data, taking (partial) ownership

    //apply view filter: comparison results
    void updateCmpResult(bool hideFiltered,
                         bool leftOnlyFilesActive,
                         bool rightOnlyFilesActive,
                         bool leftNewerFilesActive,
                         bool rightNewerFilesActive,
                         bool differentFilesActive,
                         bool equalFilesActive,
                         bool conflictFilesActive);

    //apply view filter: synchronization preview
    void updateSyncPreview(bool hideFiltered,
                           bool syncCreateLeftActive,
                           bool syncCreateRightActive,
                           bool syncDeleteLeftActive,
                           bool syncDeleteRightActive,
                           bool syncDirOverwLeftActive,
                           bool syncDirOverwRightActive,
                           bool syncDirNoneActive,
                           bool syncEqualActive,
                           bool conflictFilesActive);

    enum NodeStatus
    {
        STATUS_EXPANDED,
        STATUS_REDUCED,
        STATUS_EMPTY
    };

    //---------------------------------------------------------------------
    struct Node
    {
        Node(int percent, size_t level, NodeStatus status, UInt64 bytes) :
            percent_(percent), level_(level), status_(status), bytes_(bytes) {}
        virtual ~Node() {}

        const int percent_; //[0, 100]
        const size_t level_;
        const NodeStatus status_;
        const UInt64 bytes_;
    };

    struct FilesNode : public Node
    {
        FilesNode(int percent, size_t level, UInt64 bytes, FileSystemObject& firstFile) : Node(percent, level, STATUS_EMPTY, bytes), firstFile_(firstFile)  {}
        FileSystemObject& firstFile_; //or symlink
    };

    struct DirNode : public Node
    {
        DirNode(int percent, size_t level, NodeStatus status, UInt64 bytes, DirMapping& dirObj) : Node(percent, level, status, bytes), dirObj_(dirObj) {}
        DirMapping& dirObj_;
    };

    struct RootNode : public Node
    {
        RootNode(int percent, NodeStatus status, UInt64 bytes, BaseDirMapping& baseMap) : Node(percent, 0, status, bytes), baseMap_(baseMap) {}
        BaseDirMapping& baseMap_;
    };

    std::unique_ptr<Node> getLine(size_t row) const; //return nullptr on error
    size_t linesTotal() const { return flatTree.size(); }

    void expandNode(size_t row);
    void reduceNode(size_t row);
    NodeStatus getStatus(size_t row) const;
    ptrdiff_t getParent(size_t row) const; //return < 0 if none

    void setSortDirection(ColumnTypeNavi colType, bool ascending); //apply permanently!
    std::pair<ColumnTypeNavi, bool> getSortDirection() { return std::make_pair(sortColumn, sortAscending); }
    static bool getDefaultSortDirection(ColumnTypeNavi colType); //ascending?

private:
    struct DirNodeImpl;

    struct Container
    {
        Container() : firstFile(nullptr) {}
        UInt64 bytesGross;
        UInt64 bytesNet;  //files in this directory only
        std::vector<DirNodeImpl> subDirs;
        FileSystemObject::ObjectId firstFile; //weak pointer to first FileMapping or SymLinkMapping
    };

    struct DirNodeImpl : public Container
    {
        DirNodeImpl() : objId(nullptr) {}
        FileSystemObject::ObjectId objId; //weak pointer to DirMapping
    };

    struct RootNodeImpl : public Container
    {
        RootNodeImpl() {}
        std::shared_ptr<BaseDirMapping> baseMap;
    };

    enum NodeType
    {
        TYPE_ROOT,      //-> RootNodeImpl
        TYPE_DIRECTORY, //-> DirNodeImpl
        TYPE_FILES      //-> Container
    };

    struct TreeLine
    {
        TreeLine(size_t level, int percent, const Container* node, enum NodeType type) : level_(level), percent_(percent), node_(node), type_(type) {}

        size_t level_;
        int percent_;      //[0, 100]
        const Container* node_; //
        NodeType type_;    //we choose to increase size of "flatTree" rather than "folderCmpView" by not using dynamic polymorphism!
    };

    static void compressNode(Container& cont);
    template <class Function>
    static void extractVisibleSubtree(HierarchyObject& hierObj, Container& cont, Function includeObject);
    void getChildren(const Container& cont,  size_t level, std::vector<TreeLine>& output);
    template <class Predicate> void updateView(Predicate pred);
    void applySubView(std::vector<RootNodeImpl>&& newView);

    template <bool ascending> static void sortSingleLevel(std::vector<TreeLine>& items, ColumnTypeNavi columnType);
    template <bool ascending> struct LessShortName;

    std::vector<TreeLine> flatTree; //collapsable/expandable sub-tree of folderCmpView -> always sorted!
    /*             /|\
                    | (update...)
                    |                         */
    std::vector<RootNodeImpl> folderCmpView; //partial view on folderCmp -> unsorted (cannot be, because files are not a separate entity)
    /*             /|\
                    | (update...)
                    |                         */
    std::vector<std::shared_ptr<BaseDirMapping>> folderCmp; //full raw data

    ColumnTypeNavi sortColumn;
    bool sortAscending;
};


namespace treeview
{
void init(Grid& grid, const std::shared_ptr<TreeView>& treeDataView);

void setShowPercentage(Grid& grid, bool value);
bool getShowPercentage(const Grid& grid);

std::vector<Grid::ColumnAttribute> convertConfig(const std::vector<ColumnAttributeNavi  >& attribs); //+ make consistent
std::vector<ColumnAttributeNavi>   convertConfig(const std::vector<Grid::ColumnAttribute>& attribs); //
}
}

#endif //TREE_H_INCLUDED_841703190201835280256673425
