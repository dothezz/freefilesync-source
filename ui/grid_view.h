// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) ZenJu (zhnmju123 AT gmx DOT de) - All Rights Reserved    *
// **************************************************************************

#ifndef GRIDVIEW_H_INCLUDED
#define GRIDVIEW_H_INCLUDED

#include <set>
#include "column_attr.h"
#include "../file_hierarchy.h"


namespace zen
{
//grid view of FolderComparison
class GridView
{
public:
    //direct data access via row number
    const FileSystemObject* getObject(size_t row) const;  //returns NULL if object is not found; complexity: constant!
    /**/
    FileSystemObject* getObject(size_t row);        //
    size_t rowsOnView() const { return viewRef  .size(); } //only visible elements
    size_t rowsTotal () const { return sortedRef.size(); } //total rows available

    //get references to FileSystemObject: no NULL-check needed! Everything's bound.
    void getAllFileRef(const std::set<size_t>& rows, std::vector<FileSystemObject*>& output);

    struct StatusCmpResult
    {
        StatusCmpResult();

        bool existsLeftOnly;
        bool existsRightOnly;
        bool existsLeftNewer;
        bool existsRightNewer;
        bool existsDifferent;
        bool existsEqual;
        bool existsConflict;

        unsigned int filesOnLeftView;
        unsigned int foldersOnLeftView;
        unsigned int filesOnRightView;
        unsigned int foldersOnRightView;

        zen::UInt64 filesizeLeftView;
        zen::UInt64 filesizeRightView;
    };

    //comparison results view
    StatusCmpResult updateCmpResult(bool hideFiltered,
                                    bool leftOnlyFilesActive,
                                    bool rightOnlyFilesActive,
                                    bool leftNewerFilesActive,
                                    bool rightNewerFilesActive,
                                    bool differentFilesActive,
                                    bool equalFilesActive,
                                    bool conflictFilesActive);

    struct StatusSyncPreview
    {
        StatusSyncPreview();

        bool existsSyncCreateLeft;
        bool existsSyncCreateRight;
        bool existsSyncDeleteLeft;
        bool existsSyncDeleteRight;
        bool existsSyncDirLeft;
        bool existsSyncDirRight;
        bool existsSyncDirNone;
        bool existsSyncEqual;
        bool existsConflict;

        unsigned int filesOnLeftView;
        unsigned int foldersOnLeftView;
        unsigned int filesOnRightView;
        unsigned int foldersOnRightView;

        zen::UInt64 filesizeLeftView;
        zen::UInt64 filesizeRightView;
    };

    //synchronization preview
    StatusSyncPreview updateSyncPreview(bool hideFiltered,
                                        bool syncCreateLeftActive,
                                        bool syncCreateRightActive,
                                        bool syncDeleteLeftActive,
                                        bool syncDeleteRightActive,
                                        bool syncDirOverwLeftActive,
                                        bool syncDirOverwRightActive,
                                        bool syncDirNoneActive,
                                        bool syncEqualActive,
                                        bool conflictFilesActive);

    void setData(FolderComparison& newData);
    void removeInvalidRows();                //remove rows that have been deleted meanwhile: call after manual deletion and synchronization!

    //sorting...
    bool static getDefaultSortDirection(zen::ColumnTypeRim type); //true: ascending; false: descending

    void sortView(zen::ColumnTypeRim type, bool onLeft, bool ascending); //always call this method for sorting, never sort externally!

    struct SortInfo
    {
        SortInfo(zen::ColumnTypeRim type, bool onLeft, bool ascending) : type_(type), onLeft_(onLeft), ascending_(ascending) {}
        zen::ColumnTypeRim type_;
        bool onLeft_;
        bool ascending_;
    };
    const SortInfo* getSortInfo() const { return currentSort.get(); } //return NULL if currently not sorted

    int findRowDirect(FileSystemObject::ObjectIdConst objId) const; // find an object's row position on view list directly, return < 0 if not found
    int findRowFirstChild(const HierarchyObject* hierObj)    const; // find first child of DirMapping or BaseDirMapping *on sorted sub view*
    //"hierObj" may be invalid, it is NOT dereferenced, return < 0 if not found

private:
    struct RefIndex
    {
        RefIndex(unsigned int folderInd, FileSystemObject::ObjectId id) :
            folderIndex(folderInd),
            objId(id) {}
        unsigned int folderIndex;
        FileSystemObject::ObjectId objId;
    };

    template <class Predicate> void updateView(Predicate pred);


    zen::hash_map<FileSystemObject::ObjectIdConst, size_t> rowPositions; //find row positions on sortedRef directly
    zen::hash_map<const HierarchyObject*, size_t> rowPositionsFirstChild; //find first child on sortedRef of a hierarchy object
    //NEVER DEREFERENCE HierarchyObject*!!! lookup only!

    std::vector<FileSystemObject::ObjectId> viewRef;  //partial view on sortedRef
    /*             /|\
                    | (update...)
                    |                         */
    std::vector<RefIndex> sortedRef; //flat view of weak pointers on folderCmp; may be sorted
    /*             /|\
                    | (setData...)
                    |                         */
    //std::shared_ptr<FolderComparison> folderCmp; //actual comparison data: owned by GridView!

    class SerializeHierarchy;

    //sorting classes
    template <bool ascending>
    class LessRelativeName;

    template <bool ascending, SelectedSide side>
    class LessShortFileName;

    template <bool ascending, SelectedSide side>
    class LessFilesize;

    template <bool ascending, SelectedSide side>
    class LessFiletime;

    template <bool ascending, SelectedSide side>
    class LessExtension;

    template <bool ascending>
    class LessCmpResult;

    template <bool ascending>
    class LessSyncDirection;

    std::unique_ptr<SortInfo> currentSort;
};










//############################################################################
//inline implementation

inline
const FileSystemObject* GridView::getObject(size_t row) const
{
    if (row < rowsOnView())
        return FileSystemObject::retrieve(viewRef[row]);
    else
        return NULL;
}

inline
FileSystemObject* GridView::getObject(size_t row)
{
    //code re-use of const method: see Meyers Effective C++
    return const_cast<FileSystemObject*>(static_cast<const GridView&>(*this).getObject(row));
}
}


#endif // GRIDVIEW_H_INCLUDED
