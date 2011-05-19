// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) 2008-2011 ZenJu (zhnmju123 AT gmx.de)                    *
// **************************************************************************
//
#include "grid_view.h"
#include "sorting.h"
#include "../synchronization.h"
#include <boost/bind.hpp>

using namespace zen;


GridView::StatusCmpResult::StatusCmpResult() :
    existsLeftOnly(false),
    existsRightOnly(false),
    existsLeftNewer(false),
    existsRightNewer(false),
    existsDifferent(false),
    existsEqual(false),
    existsConflict(false),

    filesOnLeftView(0),
    foldersOnLeftView(0),
    filesOnRightView(0),
    foldersOnRightView(0) {}


template <class StatusResult>
void getNumbers(const FileSystemObject& fsObj, StatusResult& result)
{
    struct GetValues : public FSObjectVisitor
    {
        GetValues(StatusResult& res) : result_(res) {}

        virtual void visit(const FileMapping& fileObj)
        {
            if (!fileObj.isEmpty<LEFT_SIDE>())
            {
                result_.filesizeLeftView += fileObj.getFileSize<LEFT_SIDE>();
                ++result_.filesOnLeftView;
            }
            if (!fileObj.isEmpty<RIGHT_SIDE>())
            {
                result_.filesizeRightView += fileObj.getFileSize<RIGHT_SIDE>();
                ++result_.filesOnRightView;
            }
        }

        virtual void visit(const SymLinkMapping& linkObj)
        {
            if (!linkObj.isEmpty<LEFT_SIDE>())
                ++result_.filesOnLeftView;

            if (!linkObj.isEmpty<RIGHT_SIDE>())
                ++result_.filesOnRightView;
        }

        virtual void visit(const DirMapping& dirObj)
        {
            if (!dirObj.isEmpty<LEFT_SIDE>())
                ++result_.foldersOnLeftView;

            if (!dirObj.isEmpty<RIGHT_SIDE>())
                ++result_.foldersOnRightView;
        }
        StatusResult& result_;
    } getVal(result);
    fsObj.accept(getVal);
}


GridView::StatusCmpResult GridView::updateCmpResult(bool hideFiltered, //maps sortedRef to viewRef
                                                    bool leftOnlyFilesActive,
                                                    bool rightOnlyFilesActive,
                                                    bool leftNewerFilesActive,
                                                    bool rightNewerFilesActive,
                                                    bool differentFilesActive,
                                                    bool equalFilesActive,
                                                    bool conflictFilesActive)
{
    StatusCmpResult output;

    viewRef.clear();

    for (std::vector<RefIndex>::const_iterator j = sortedRef.begin(); j != sortedRef.end(); ++j)
    {
        const FileSystemObject* fsObj = getReferencedRow(*j);
        if (fsObj)
        {
            //hide filtered row, if corresponding option is set
            if (hideFiltered && !fsObj->isActive())
                continue;

            switch (fsObj->getCategory())
            {
                case FILE_LEFT_SIDE_ONLY:
                    output.existsLeftOnly = true;
                    if (!leftOnlyFilesActive) continue;
                    break;
                case FILE_RIGHT_SIDE_ONLY:
                    output.existsRightOnly = true;
                    if (!rightOnlyFilesActive) continue;
                    break;
                case FILE_LEFT_NEWER:
                    output.existsLeftNewer = true;
                    if (!leftNewerFilesActive) continue;
                    break;
                case FILE_RIGHT_NEWER:
                    output.existsRightNewer = true;
                    if (!rightNewerFilesActive) continue;
                    break;
                case FILE_DIFFERENT:
                    output.existsDifferent = true;
                    if (!differentFilesActive) continue;
                    break;
                case FILE_EQUAL:
                    output.existsEqual = true;
                    if (!equalFilesActive) continue;
                    break;
                case FILE_CONFLICT:
                case FILE_DIFFERENT_METADATA: //no extra button on screen
                    output.existsConflict = true;
                    if (!conflictFilesActive) continue;
                    break;
            }

            //calculate total number of bytes for each side
            getNumbers(*fsObj, output);

            viewRef.push_back(*j);
        }
    }

    return output;
}


GridView::StatusSyncPreview::StatusSyncPreview() :
    existsSyncCreateLeft(false),
    existsSyncCreateRight(false),
    existsSyncDeleteLeft(false),
    existsSyncDeleteRight(false),
    existsSyncDirLeft(false),
    existsSyncDirRight(false),
    existsSyncDirNone(false),
    existsSyncEqual(false),
    existsConflict(false),

    filesOnLeftView(0),
    foldersOnLeftView(0),
    filesOnRightView(0),
    foldersOnRightView(0) {}


GridView::StatusSyncPreview GridView::updateSyncPreview(bool hideFiltered, //maps sortedRef to viewRef
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
    StatusSyncPreview output;

    viewRef.clear();

    for (std::vector<RefIndex>::const_iterator j = sortedRef.begin(); j != sortedRef.end(); ++j)
    {
        const FileSystemObject* fsObj = getReferencedRow(*j);
        if (fsObj)
        {
            //hide filtered row, if corresponding option is set
            if (hideFiltered && !fsObj->isActive())
                continue;

            switch (fsObj->getSyncOperation()) //evaluate comparison result and sync direction
            {
                case SO_CREATE_NEW_LEFT:
                    output.existsSyncCreateLeft = true;
                    if (!syncCreateLeftActive) continue;
                    break;
                case SO_CREATE_NEW_RIGHT:
                    output.existsSyncCreateRight = true;
                    if (!syncCreateRightActive) continue;
                    break;
                case SO_DELETE_LEFT:
                    output.existsSyncDeleteLeft = true;
                    if (!syncDeleteLeftActive) continue;
                    break;
                case SO_DELETE_RIGHT:
                    output.existsSyncDeleteRight = true;
                    if (!syncDeleteRightActive) continue;
                    break;
                case SO_OVERWRITE_RIGHT:
                case SO_COPY_METADATA_TO_RIGHT: //no extra button on screen
                    output.existsSyncDirRight = true;
                    if (!syncDirOverwRightActive) continue;
                    break;
                case SO_OVERWRITE_LEFT:
                case SO_COPY_METADATA_TO_LEFT: //no extra button on screen
                    output.existsSyncDirLeft = true;
                    if (!syncDirOverwLeftActive) continue;
                    break;
                case SO_DO_NOTHING:
                    output.existsSyncDirNone = true;
                    if (!syncDirNoneActive) continue;
                    break;
                case SO_EQUAL:
                    output.existsSyncEqual = true;
                    if (!syncEqualActive) continue;
                    break;
                case SO_UNRESOLVED_CONFLICT:
                    output.existsConflict = true;
                    if (!conflictFilesActive) continue;
                    break;
            }

            //calculate total number of bytes for each side
            getNumbers(*fsObj, output);

            viewRef.push_back(*j);
        }
    }

    return output;
}


void GridView::getAllFileRef(const std::set<size_t>& guiRows, std::vector<FileSystemObject*>& output)
{
    std::set<size_t>::const_iterator upperEnd = guiRows.lower_bound(rowsOnView()); //loop over valid rows only!

    output.clear();
    output.reserve(guiRows.size());
    for (std::set<size_t>::const_iterator i = guiRows.begin(); i != upperEnd; ++i)
    {
        FileSystemObject* fsObj = getReferencedRow(viewRef[*i]);
        if (fsObj)
            output.push_back(fsObj);
    }
}


inline
bool GridView::isInvalidRow(const RefIndex& ref) const
{
    return getReferencedRow(ref) == NULL;
}


void GridView::removeInvalidRows()
{
    viewRef.clear();

    //remove rows that have been deleted meanwhile
    sortedRef.erase(std::remove_if(sortedRef.begin(), sortedRef.end(),
                                   boost::bind(&GridView::isInvalidRow, this, _1)), sortedRef.end());
}


void GridView::clearAllRows()
{
    viewRef.clear();
    sortedRef.clear();
    folderCmp.clear();
}


class GridView::SerializeHierarchy
{
public:
    SerializeHierarchy(std::vector<GridView::RefIndex>& sortedRef, size_t index) :
        index_(index),
        sortedRef_(sortedRef) {}

    void execute(const HierarchyObject& hierObj)
    {
        //add file references
        std::for_each(hierObj.useSubFiles().begin(), hierObj.useSubFiles().end(), *this);

        //add symlink references
        std::for_each(hierObj.useSubLinks().begin(), hierObj.useSubLinks().end(), *this);

        //add dir references
        std::for_each(hierObj.useSubDirs().begin(), hierObj.useSubDirs().end(), *this);
    }

    void operator()(const FileMapping& fileObj)
    {
        sortedRef_.push_back(RefIndex(index_, fileObj.getId()));
    }

    void operator()(const SymLinkMapping& linkObj)
    {
        sortedRef_.push_back(RefIndex(index_, linkObj.getId()));
    }

    void operator()(const DirMapping& dirObj)
    {
        sortedRef_.push_back(RefIndex(index_, dirObj.getId()));
        execute(dirObj); //add recursion here to list sub-objects directly below parent!
    }

private:
    size_t index_;
    std::vector<GridView::RefIndex>& sortedRef_;
};


void GridView::setData(FolderComparison& newData)
{
    viewRef.clear();
    sortedRef.clear();
    folderCmp.swap(newData);

    //fill sortedRef
    for (FolderComparison::const_iterator j = folderCmp.begin(); j != folderCmp.end(); ++j)
        SerializeHierarchy(sortedRef, j - folderCmp.begin()).execute(*j);
}


//------------------------------------ SORTING TEMPLATES ------------------------------------------------
template <bool ascending>
class GridView::SortByDirectory : public std::binary_function<RefIndex, RefIndex, bool>
{
public:
    bool operator()(const RefIndex a, const RefIndex b) const
    {
        return ascending ?
               a.folderIndex < b.folderIndex :
               a.folderIndex > b.folderIndex;
    }
};


template <bool ascending, zen::SelectedSide side>
class GridView::SortByRelName : public std::binary_function<RefIndex, RefIndex, bool>
{
public:
    SortByRelName(const GridView& view) : m_view(view) {}

    bool operator()(const RefIndex a, const RefIndex b) const
    {
        //presort by folder pair
        if (a.folderIndex != b.folderIndex)
            return ascending ?
                   a.folderIndex < b.folderIndex :
                   a.folderIndex > b.folderIndex;

        const FileSystemObject* fsObjA = m_view.getReferencedRow(a);
        const FileSystemObject* fsObjB = m_view.getReferencedRow(b);
        if (fsObjA == NULL) //invalid rows shall appear at the end
            return false;
        else if (fsObjB == NULL)
            return true;

        return sortByRelativeName<ascending, side>(*fsObjA, *fsObjB);
    }
private:
    const GridView& m_view;
};


template <bool ascending, zen::SelectedSide side>
class GridView::SortByFileName : public std::binary_function<RefIndex, RefIndex, bool>
{
public:
    SortByFileName(const GridView& view) : m_view(view) {}

    bool operator()(const RefIndex a, const RefIndex b) const
    {
        const FileSystemObject* fsObjA = m_view.getReferencedRow(a);
        const FileSystemObject* fsObjB = m_view.getReferencedRow(b);
        if (fsObjA == NULL) //invalid rows shall appear at the end
            return false;
        else if (fsObjB == NULL)
            return true;

        return sortByFileName<ascending, side>(*fsObjA, *fsObjB);
    }
private:
    const GridView& m_view;
};


template <bool ascending, zen::SelectedSide side>
class GridView::SortByFileSize : public std::binary_function<RefIndex, RefIndex, bool>
{
public:
    SortByFileSize(const GridView& view) : m_view(view) {}

    bool operator()(const RefIndex a, const RefIndex b) const
    {
        const FileSystemObject* fsObjA = m_view.getReferencedRow(a);
        const FileSystemObject* fsObjB = m_view.getReferencedRow(b);
        if (fsObjA == NULL) //invalid rows shall appear at the end
            return false;
        else if (fsObjB == NULL)
            return true;

        return sortByFileSize<ascending, side>(*fsObjA, *fsObjB);
    }
private:
    const GridView& m_view;
};


template <bool ascending, zen::SelectedSide side>
class GridView::SortByDate : public std::binary_function<RefIndex, RefIndex, bool>
{
public:
    SortByDate(const GridView& view) : m_view(view) {}

    bool operator()(const RefIndex a, const RefIndex b) const
    {
        const FileSystemObject* fsObjA = m_view.getReferencedRow(a);
        const FileSystemObject* fsObjB = m_view.getReferencedRow(b);
        if (fsObjA == NULL) //invalid rows shall appear at the end
            return false;
        else if (fsObjB == NULL)
            return true;

        return sortByDate<ascending, side>(*fsObjA, *fsObjB);
    }
private:
    const GridView& m_view;
};


template <bool ascending, zen::SelectedSide side>
class GridView::SortByExtension : public std::binary_function<RefIndex, RefIndex, bool>
{
public:
    SortByExtension(const GridView& view) : m_view(view) {}

    bool operator()(const RefIndex a, const RefIndex b) const
    {
        const FileSystemObject* fsObjA = m_view.getReferencedRow(a);
        const FileSystemObject* fsObjB = m_view.getReferencedRow(b);
        if (fsObjA == NULL) //invalid rows shall appear at the end
            return false;
        else if (fsObjB == NULL)
            return true;

        return sortByExtension<ascending, side>(*fsObjA, *fsObjB);
    }
private:
    const GridView& m_view;
};


template <bool ascending>
class GridView::SortByCmpResult : public std::binary_function<RefIndex, RefIndex, bool>
{
public:
    SortByCmpResult(const GridView& view) : m_view(view) {}

    bool operator()(const RefIndex a, const RefIndex b) const
    {
        const FileSystemObject* fsObjA = m_view.getReferencedRow(a);
        const FileSystemObject* fsObjB = m_view.getReferencedRow(b);
        if (fsObjA == NULL) //invalid rows shall appear at the end
            return false;
        else if (fsObjB == NULL)
            return true;

        return sortByCmpResult<ascending>(*fsObjA, *fsObjB);
    }
private:
    const GridView& m_view;
};


template <bool ascending>
class GridView::SortBySyncDirection : public std::binary_function<RefIndex, RefIndex, bool>
{
public:
    SortBySyncDirection(const GridView& view) : m_view(view) {}

    bool operator()(const RefIndex a, const RefIndex b) const
    {
        const FileSystemObject* fsObjA = m_view.getReferencedRow(a);
        const FileSystemObject* fsObjB = m_view.getReferencedRow(b);
        if (fsObjA == NULL) //invalid rows shall appear at the end
            return false;
        else if (fsObjB == NULL)
            return true;

        return sortBySyncDirection<ascending>(*fsObjA, *fsObjB);
    }
private:
    const GridView& m_view;
};

//-------------------------------------------------------------------------------------------------------
bool GridView::getDefaultDirection(SortType type) //true: ascending; false: descending
{
    switch (type)
    {
        case SORT_BY_FILESIZE:
        case SORT_BY_DATE:
            return false;

        case SORT_BY_REL_NAME:
        case SORT_BY_FILENAME:
        case SORT_BY_EXTENSION:
        case SORT_BY_CMP_RESULT:
        case SORT_BY_DIRECTORY:
        case SORT_BY_SYNC_DIRECTION:
            return true;
    }
    assert(false);
    return true;
}


void GridView::sortView(SortType type, bool onLeft, bool ascending)
{
    viewRef.clear();

    switch (type)
    {
        case SORT_BY_REL_NAME:
            if      ( ascending &&  onLeft) std::sort(sortedRef.begin(), sortedRef.end(), SortByRelName<true,  LEFT_SIDE>(*this));
            else if ( ascending && !onLeft) std::sort(sortedRef.begin(), sortedRef.end(), SortByRelName<true,  RIGHT_SIDE>(*this));
            else if (!ascending &&  onLeft) std::sort(sortedRef.begin(), sortedRef.end(), SortByRelName<false, LEFT_SIDE >(*this));
            else if (!ascending && !onLeft) std::sort(sortedRef.begin(), sortedRef.end(), SortByRelName<false, RIGHT_SIDE>(*this));
            break;
        case SORT_BY_FILENAME:
            if      ( ascending &&  onLeft) std::sort(sortedRef.begin(), sortedRef.end(), SortByFileName<true,  LEFT_SIDE >(*this));
            else if ( ascending && !onLeft) std::sort(sortedRef.begin(), sortedRef.end(), SortByFileName<true,  RIGHT_SIDE>(*this));
            else if (!ascending &&  onLeft) std::sort(sortedRef.begin(), sortedRef.end(), SortByFileName<false, LEFT_SIDE >(*this));
            else if (!ascending && !onLeft) std::sort(sortedRef.begin(), sortedRef.end(), SortByFileName<false, RIGHT_SIDE>(*this));
            break;
        case SORT_BY_FILESIZE:
            if      ( ascending &&  onLeft) std::sort(sortedRef.begin(), sortedRef.end(), SortByFileSize<true,  LEFT_SIDE >(*this));
            else if ( ascending && !onLeft) std::sort(sortedRef.begin(), sortedRef.end(), SortByFileSize<true,  RIGHT_SIDE>(*this));
            else if (!ascending &&  onLeft) std::sort(sortedRef.begin(), sortedRef.end(), SortByFileSize<false, LEFT_SIDE >(*this));
            else if (!ascending && !onLeft) std::sort(sortedRef.begin(), sortedRef.end(), SortByFileSize<false, RIGHT_SIDE>(*this));
            break;
        case SORT_BY_DATE:
            if      ( ascending &&  onLeft) std::sort(sortedRef.begin(), sortedRef.end(), SortByDate<true,  LEFT_SIDE >(*this));
            else if ( ascending && !onLeft) std::sort(sortedRef.begin(), sortedRef.end(), SortByDate<true,  RIGHT_SIDE>(*this));
            else if (!ascending &&  onLeft) std::sort(sortedRef.begin(), sortedRef.end(), SortByDate<false, LEFT_SIDE >(*this));
            else if (!ascending && !onLeft) std::sort(sortedRef.begin(), sortedRef.end(), SortByDate<false, RIGHT_SIDE>(*this));
            break;
        case SORT_BY_EXTENSION:
            if      ( ascending &&  onLeft) std::stable_sort(sortedRef.begin(), sortedRef.end(), SortByExtension<true,  LEFT_SIDE >(*this));
            else if ( ascending && !onLeft) std::stable_sort(sortedRef.begin(), sortedRef.end(), SortByExtension<true,  RIGHT_SIDE>(*this));
            else if (!ascending &&  onLeft) std::stable_sort(sortedRef.begin(), sortedRef.end(), SortByExtension<false, LEFT_SIDE >(*this));
            else if (!ascending && !onLeft) std::stable_sort(sortedRef.begin(), sortedRef.end(), SortByExtension<false, RIGHT_SIDE>(*this));
            break;
        case SORT_BY_CMP_RESULT:
            if      ( ascending) std::stable_sort(sortedRef.begin(), sortedRef.end(), SortByCmpResult<true >(*this));
            else if (!ascending) std::stable_sort(sortedRef.begin(), sortedRef.end(), SortByCmpResult<false>(*this));
            break;
        case SORT_BY_SYNC_DIRECTION:
            if      ( ascending) std::stable_sort(sortedRef.begin(), sortedRef.end(), SortBySyncDirection<true >(*this));
            else if (!ascending) std::stable_sort(sortedRef.begin(), sortedRef.end(), SortBySyncDirection<false>(*this));
            break;
        case SORT_BY_DIRECTORY:
            if      ( ascending) std::stable_sort(sortedRef.begin(), sortedRef.end(), SortByDirectory<true>());
            else if (!ascending) std::stable_sort(sortedRef.begin(), sortedRef.end(), SortByDirectory<false>());
            break;
    }
}

