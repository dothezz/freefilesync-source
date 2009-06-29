#include "gridView.h"
#include "sorting.h"

using FreeFileSync::GridView;


GridView::GridView(FreeFileSync::FolderComparison& results) :
        leftOnlyFilesActive(false),
        rightOnlyFilesActive(false),
        leftNewerFilesActive(false),
        rightNewerFilesActive(false),
        differentFilesActive(false),
        equalFilesActive(false),
        conflictFilesActive(false),
        syncDirLeftActive(false),
        syncDirRightActive(false),
        syncDirNoneActive(false),
        folderCmp(results) {}


GridView::StatusInfo::StatusInfo() :
        existsLeftOnly(false),
        existsRightOnly(false),
        existsLeftNewer(false),
        existsRightNewer(false),
        existsDifferent(false),
        existsEqual(false),
        existsConflict(false),

        existsSyncDirLeft(false),
        existsSyncDirRight(false),
        existsSyncDirNone(false),

        filesOnLeftView(0),
        foldersOnLeftView(0),
        filesOnRightView(0),
        foldersOnRightView(0),
        objectsTotal(0) {}

template <bool syncPreviewActive>
GridView::StatusInfo GridView::update_sub(const bool hideFiltered)
{
    StatusInfo output;

    refView.clear();

    for (FolderComparison::const_iterator j = folderCmp.begin(); j != folderCmp.end(); ++j)
    {
        const FileComparison& fileCmp = j->fileCmp;

        RefIndex newEntry;
        newEntry.folderIndex = j - folderCmp.begin();

        for (FileComparison::const_iterator i = fileCmp.begin(); i != fileCmp.end(); ++i)
        {
            //process UI filter settings
            if (syncPreviewActive) //synchronization preview
            {
                //exclude result "=="
                if (i->cmpResult == FILE_EQUAL) //note: consider "objectsTotal"
                    continue;

                output.objectsTotal++;

                //hide filtered row, if corresponding option is set
                if (hideFiltered && !i->selectedForSynchronization) //keep AFTER "objectsTotal++"
                    continue;


                switch (i->direction)
                {
                case SYNC_DIR_LEFT:
                    output.existsSyncDirLeft = true;
                    if (!syncDirLeftActive) continue;
                    break;
                case SYNC_DIR_RIGHT:
                    output.existsSyncDirRight = true;
                    if (!syncDirRightActive) continue;
                    break;
                case SYNC_DIR_NONE:
                    output.existsSyncDirNone = true;
                    if (!syncDirNoneActive) continue;
                    break;
                case SYNC_UNRESOLVED_CONFLICT:
                    output.existsConflict = true;
                    if (!conflictFilesActive) continue;
                    break;
                }
            }
            else //comparison results view
            {
                output.objectsTotal++;

                //hide filtered row, if corresponding option is set
                if (hideFiltered && !i->selectedForSynchronization)
                    continue;

                switch (i->cmpResult)
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
                    output.existsConflict = true;
                    if (!conflictFilesActive) continue;
                    break;
                }
            }

            //calculate total number of bytes for each side
            if (i->fileDescrLeft.objType == FileDescrLine::TYPE_FILE)
            {
                output.filesizeLeftView += i->fileDescrLeft.fileSize;
                ++output.filesOnLeftView;
            }
            else if (i->fileDescrLeft.objType == FileDescrLine::TYPE_DIRECTORY)
                ++output.foldersOnLeftView;

            if (i->fileDescrRight.objType == FileDescrLine::TYPE_FILE)
            {
                output.filesizeRightView += i->fileDescrRight.fileSize;
                ++output.filesOnRightView;
            }
            else if (i->fileDescrRight.objType == FileDescrLine::TYPE_DIRECTORY)
                ++output.foldersOnRightView;

            newEntry.rowIndex = i - fileCmp.begin();
            refView.push_back(newEntry);
        }

//        //add some empty line after each folder pair
//        RefIndex emptyLine;
//        emptyLine.folderIndex = -1;
//        emptyLine.rowIndex    = 0;
//        refView.push_back(emptyLine);
    }

    return output;
}


GridView::StatusInfo GridView::update(const bool hideFiltered, const bool syncPreviewActive)
{
    return syncPreviewActive ?
           update_sub<true>(hideFiltered) :
           update_sub<false>(hideFiltered);
}


void GridView::viewRefToFolderRef(const std::set<int>& viewRef, FreeFileSync::FolderCompRef& output)
{
    output.clear();
    for (int i = 0; i < int(folderCmp.size()); ++i)
        output.push_back(std::set<int>());      //avoid copy by value for full set<int>

    for (std::set<int>::const_iterator i = viewRef.begin(); i != viewRef.end(); ++i)
    {
        const unsigned int folder = refView[*i].folderIndex;
        const unsigned int row    = refView[*i].rowIndex;

        output[folder].insert(row);
    }
}


bool GridView::refGridIsEmpty() const
{
    for (FolderComparison::const_iterator j = folderCmp.begin(); j != folderCmp.end(); ++j)
        if (!j->fileCmp.empty()) return false;

    return true;
}


template <typename CompareFct>
void bubbleSort(FreeFileSync::FolderComparison& folderCmp, CompareFct compare)
{
    for (int i = folderCmp.size() - 2; i >= 0; --i)
    {
        bool swapped = false;
        for (int j = 0; j <= i; ++j)
            if (compare(folderCmp[j + 1], folderCmp[j]))
            {
                folderCmp[j + 1].swap(folderCmp[j]);
                swapped = true;
            }

        if (!swapped)
            return;
    }
}


void GridView::sortView(const SortType type, const bool onLeft, const bool ascending)
{
    using namespace FreeFileSync;

    if (type == SORT_BY_DIRECTORY)
    {
        //specialization: use custom sorting function based on FolderComparison::swap()
        //bubble sort is no performance issue since number of folder pairs should be "very small"
        if      (ascending  &&  onLeft) bubbleSort(folderCmp, sortByDirectory<ASCENDING,  SORT_ON_LEFT>);
        else if (ascending  && !onLeft) bubbleSort(folderCmp, sortByDirectory<ASCENDING,  SORT_ON_RIGHT>);
        else if (!ascending &&  onLeft) bubbleSort(folderCmp, sortByDirectory<DESCENDING, SORT_ON_LEFT>);
        else if (!ascending && !onLeft) bubbleSort(folderCmp, sortByDirectory<DESCENDING, SORT_ON_RIGHT>);

        //then sort by relative name
        GridView::sortView(SORT_BY_REL_NAME, onLeft, ascending);
        return;
    }


    for (FolderComparison::iterator j = folderCmp.begin(); j != folderCmp.end(); ++j)
    {
        FileComparison& fileCmp = j->fileCmp;

        switch (type)
        {
        case SORT_BY_REL_NAME:
            if      ( ascending &&  onLeft) std::sort(fileCmp.begin(), fileCmp.end(), sortByRelativeName<ASCENDING,  SORT_ON_LEFT>);
            else if ( ascending && !onLeft) std::sort(fileCmp.begin(), fileCmp.end(), sortByRelativeName<ASCENDING,  SORT_ON_RIGHT>);
            else if (!ascending &&  onLeft) std::sort(fileCmp.begin(), fileCmp.end(), sortByRelativeName<DESCENDING, SORT_ON_LEFT>);
            else if (!ascending && !onLeft) std::sort(fileCmp.begin(), fileCmp.end(), sortByRelativeName<DESCENDING, SORT_ON_RIGHT>);
            break;
        case SORT_BY_FILENAME:
            if      ( ascending &&  onLeft) std::sort(fileCmp.begin(), fileCmp.end(), sortByFileName<ASCENDING,  SORT_ON_LEFT>);
            else if ( ascending && !onLeft) std::sort(fileCmp.begin(), fileCmp.end(), sortByFileName<ASCENDING,  SORT_ON_RIGHT>);
            else if (!ascending &&  onLeft) std::sort(fileCmp.begin(), fileCmp.end(), sortByFileName<DESCENDING, SORT_ON_LEFT>);
            else if (!ascending && !onLeft) std::sort(fileCmp.begin(), fileCmp.end(), sortByFileName<DESCENDING, SORT_ON_RIGHT>);
            break;
        case SORT_BY_FILESIZE:
            if      ( ascending &&  onLeft) std::sort(fileCmp.begin(), fileCmp.end(), sortByFileSize<ASCENDING,  SORT_ON_LEFT>);
            else if ( ascending && !onLeft) std::sort(fileCmp.begin(), fileCmp.end(), sortByFileSize<ASCENDING,  SORT_ON_RIGHT>);
            else if (!ascending &&  onLeft) std::sort(fileCmp.begin(), fileCmp.end(), sortByFileSize<DESCENDING, SORT_ON_LEFT>);
            else if (!ascending && !onLeft) std::sort(fileCmp.begin(), fileCmp.end(), sortByFileSize<DESCENDING, SORT_ON_RIGHT>);
            break;
        case SORT_BY_DATE:
            if      ( ascending &&  onLeft) std::sort(fileCmp.begin(), fileCmp.end(), sortByDate<ASCENDING,  SORT_ON_LEFT>);
            else if ( ascending && !onLeft) std::sort(fileCmp.begin(), fileCmp.end(), sortByDate<ASCENDING,  SORT_ON_RIGHT>);
            else if (!ascending &&  onLeft) std::sort(fileCmp.begin(), fileCmp.end(), sortByDate<DESCENDING, SORT_ON_LEFT>);
            else if (!ascending && !onLeft) std::sort(fileCmp.begin(), fileCmp.end(), sortByDate<DESCENDING, SORT_ON_RIGHT>);
            break;
        case SORT_BY_CMP_RESULT:
            if      ( ascending) std::sort(fileCmp.begin(), fileCmp.end(), sortByCmpResult<ASCENDING>);
            else if (!ascending) std::sort(fileCmp.begin(), fileCmp.end(), sortByCmpResult<DESCENDING>);
            break;
        case SORT_BY_SYNC_DIRECTION:
            if      ( ascending) std::sort(fileCmp.begin(), fileCmp.end(), sortBySyncDirection<ASCENDING>);
            else if (!ascending) std::sort(fileCmp.begin(), fileCmp.end(), sortBySyncDirection<DESCENDING>);
            break;
        default:
            assert(false);
        }
    }
}

