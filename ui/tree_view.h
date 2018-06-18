// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) Zenju (zenju AT gmx DOT de) - All Rights Reserved        *
// **************************************************************************

#ifndef TREE_H_INCLUDED_841703190201835280256673425
#define TREE_H_INCLUDED_841703190201835280256673425

#include <functional>
#include <zen/optional.h>
#include <wx+/grid.h>
#include "column_attr.h"
#include "../file_hierarchy.h"

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
        Node(int percent, UInt64 bytes, int itemCount, unsigned int level, NodeStatus status) :
            percent_(percent), level_(level), status_(status), bytes_(bytes), itemCount_(itemCount) {}
        virtual ~Node() {}

        const int percent_; //[0, 100]
        const unsigned int level_;
        const NodeStatus status_;
        const UInt64 bytes_;
        const int itemCount_;
    };

    struct FilesNode : public Node
    {
        FilesNode(int percent, UInt64 bytes, int itemCount, unsigned int level, const std::vector<FileSystemObject*>& filesAndLinks) : Node(percent, bytes, itemCount, level, STATUS_EMPTY), filesAndLinks_(filesAndLinks)  {}
        std::vector<FileSystemObject*> filesAndLinks_; //files or symlinks; pointers are bound!
    };

    struct DirNode : public Node
    {
        DirNode(int percent, UInt64 bytes, int itemCount, unsigned int level, NodeStatus status, DirPair& dirObj) : Node(percent, bytes, itemCount, level, status), dirObj_(dirObj) {}
        DirPair& dirObj_;
    };

    struct RootNode : public Node
    {
        RootNode(int percent, UInt64 bytes, int itemCount, NodeStatus status, BaseDirPair& baseDirObj) : Node(percent, bytes, itemCount, 0, status), baseDirObj_(baseDirObj) {}
        BaseDirPair& baseDirObj_;
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
        Container() : itemCountGross(), itemCountNet(), firstFileId(nullptr) {}
        UInt64 bytesGross;
        UInt64 bytesNet;  //bytes for files on view in this directory only
        int itemCountGross;
        int itemCountNet;   //number of files on view for in this directory only

        std::vector<DirNodeImpl> subDirs;
        FileSystemObject::ObjectId firstFileId; //weak pointer to first FilePair or SymlinkPair
        //- "compress" algorithm may hide file nodes for directories with a single included file, i.e. itemCountGross == itemCountNet == 1
        //- a HierarchyObject* would a better fit, but we need weak pointer semantics!
        //- a std::vector<FileSystemObject::ObjectId> would be a better design, but we don't want a second memory structure as large as custom grid!
    };

    struct DirNodeImpl : public Container
    {
        DirNodeImpl() : objId(nullptr) {}
        FileSystemObject::ObjectId objId; //weak pointer to DirPair
    };

    struct RootNodeImpl : public Container
    {
        RootNodeImpl() {}
        std::shared_ptr<BaseDirPair> baseDirObj;
    };

    enum NodeType
    {
        TYPE_ROOT,      //-> RootNodeImpl
        TYPE_DIRECTORY, //-> DirNodeImpl
        TYPE_FILES      //-> Container
    };

    struct TreeLine
    {
        TreeLine(unsigned int level, int percent, const Container* node, enum NodeType type) : level_(level), percent_(percent), node_(node), type_(type) {}

        unsigned int level_;
        int percent_;      //[0, 100]
        const Container* node_; //
        NodeType type_;         //we choose to increase size of "flatTree" rather than "folderCmpView" by not using dynamic polymorphism!
    };

    static void compressNode(Container& cont);
    template <class Function>
    static void extractVisibleSubtree(HierarchyObject& hierObj, Container& cont, Function includeObject);
    void getChildren(const Container& cont, unsigned int level, std::vector<TreeLine>& output);
    template <class Predicate> void updateView(Predicate pred);
    void applySubView(std::vector<RootNodeImpl>&& newView);

    template <bool ascending> static void sortSingleLevel(std::vector<TreeLine>& items, ColumnTypeNavi columnType);
    template <bool ascending> struct LessShortName;

    std::vector<TreeLine> flatTree; //collapsable/expandable sub-tree of folderCmpView -> always sorted!
    /*             /|\
                    | (update...)
                    |                         */
    std::vector<RootNodeImpl> folderCmpView; //partial view on folderCmp -> unsorted (cannot be, because files are not a separate entity)
    std::function<bool(const FileSystemObject& fsObj)> lastViewFilterPred; //buffer view filter predicate for lazy evaluation of files/symlinks corresponding to a TYPE_FILES node
    /*             /|\
                    | (update...)
                    |                         */
    std::vector<std::shared_ptr<BaseDirPair>> folderCmp; //full raw data

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
