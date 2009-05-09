#include "gridView.h"
#include "sorting.h"

using FreeFileSync::GridView;


GridView::StatusInfo GridView::update(
    const bool includeLeftOnly,
    const bool includeRightOnly,
    const bool includeLeftNewer,
    const bool includeRightNewer,
    const bool includeDifferent,
    const bool includeEqual,
    const bool hideFiltered)
{
    StatusInfo output;
    output.existsLeftOnly   = false;
    output.existsRightOnly  = false;
    output.existsLeftNewer  = false;
    output.existsRightNewer = false;
    output.existsDifferent  = false;
    output.existsEqual      = false;

    output.filesOnLeftView    = 0;
    output.foldersOnLeftView  = 0;
    output.filesOnRightView   = 0;
    output.foldersOnRightView = 0;

    refView.clear();

    for (FolderComparison::const_iterator j = folderCmp.begin(); j != folderCmp.end(); ++j)
    {
        const FileComparison& fileCmp = j->fileCmp;

        RefIndex newEntry;
        newEntry.folderIndex = j - folderCmp.begin();

        for (FileComparison::const_iterator i = fileCmp.begin(); i != fileCmp.end(); ++i)
        {
            //hide filtered row, if corresponding option is set
            if (hideFiltered && !i->selectedForSynchronization)
                continue;

            //process UI filter settings
            switch (i->cmpResult)
            {
            case FILE_LEFT_SIDE_ONLY:
                output.existsLeftOnly = true;
                if (!includeLeftOnly) continue;
                break;
            case FILE_RIGHT_SIDE_ONLY:
                output.existsRightOnly = true;
                if (!includeRightOnly) continue;
                break;
            case FILE_LEFT_NEWER:
                output.existsLeftNewer = true;
                if (!includeLeftNewer) continue;
                break;
            case FILE_RIGHT_NEWER:
                output.existsRightNewer = true;
                if (!includeRightNewer) continue;
                break;
            case FILE_DIFFERENT:
                output.existsDifferent = true;
                if (!includeDifferent) continue;
                break;
            case FILE_EQUAL:
                output.existsEqual = true;
                if (!includeEqual) continue;
                break;
            default:
                assert (false);
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


void GridView::viewRefToFolderRef(const std::set<int>& viewRef, FolderCompRef& output)
{
    output.clear();
    for (int i = 0; i < int(folderCmp.size()); ++i)
        output.push_back(std::set<int>());      //avoid copy by value for full set<int>

    for (std::set<int>::iterator i = viewRef.begin(); i != viewRef.end(); ++i)
    {
        const unsigned int folder = refView[*i].folderIndex;
        const unsigned int row    = refView[*i].rowIndex;

        output[folder].insert(row);
    }
}


unsigned int GridView::elementsTotal() const
{
    unsigned int total = 0;
    for (FolderComparison::const_iterator j = folderCmp.begin(); j != folderCmp.end(); ++j)
        total += j->fileCmp.size();

    return total;
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
                std::swap(folderCmp[j + 1].syncPair, folderCmp[j].syncPair);
                folderCmp[j + 1].fileCmp.swap(folderCmp[j].fileCmp);

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
        //specialization: use own sorting function based on vector<FileCompareLine>::swap()
        //bubble sort is no performance issue since number of folder pairs should be "small"
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
        default:
            assert(false);
        }
    }
}

