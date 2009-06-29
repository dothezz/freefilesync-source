#ifndef GRIDVIEW_H_INCLUDED
#define GRIDVIEW_H_INCLUDED

#include "../structures.h"


namespace FreeFileSync
{
    //gui view of FolderComparison
    class GridView
    {
    public:
        GridView(FolderComparison& results);

        const FileCompareLine& operator[] (unsigned row) const;
        FileCompareLine& operator[] (unsigned row);

        //unsigned getResultsIndex(const unsigned viewIndex); //convert index on GridView to index on FolderComparison

        unsigned int elementsOnView() const; //only the currently visible elements

        bool refGridIsEmpty() const;

        //convert view references to FolderCompRef
        void viewRefToFolderRef(const std::set<int>& viewRef, FolderCompRef& output);

        const FolderPair getFolderPair(const unsigned int row) const;

        struct StatusInfo
        {
            StatusInfo();

            bool existsLeftOnly;
            bool existsRightOnly;
            bool existsLeftNewer;
            bool existsRightNewer;
            bool existsDifferent;
            bool existsEqual;
            bool existsConflict;

            bool existsSyncDirLeft;
            bool existsSyncDirRight;
            bool existsSyncDirNone;

            unsigned int filesOnLeftView;
            unsigned int foldersOnLeftView;
            unsigned int filesOnRightView;
            unsigned int foldersOnRightView;

            unsigned int objectsTotal;

            wxULongLong filesizeLeftView;
            wxULongLong filesizeRightView;
        };

        StatusInfo update(const bool hideFiltered, const bool syncPreviewActive);

        //UI View Filter settings
        //compare result
        bool leftOnlyFilesActive;
        bool rightOnlyFilesActive;
        bool leftNewerFilesActive;
        bool rightNewerFilesActive;
        bool differentFilesActive;
        bool equalFilesActive;
        bool conflictFilesActive;
        //sync preview
        bool syncDirLeftActive;
        bool syncDirRightActive;
        bool syncDirNoneActive;


        //sorting...
        enum SortType
        {
            SORT_BY_REL_NAME,
            SORT_BY_FILENAME,
            SORT_BY_FILESIZE,
            SORT_BY_DATE,
            SORT_BY_CMP_RESULT,
            SORT_BY_DIRECTORY,
            SORT_BY_SYNC_DIRECTION
        };

        void sortView(const SortType type, const bool onLeft, const bool ascending);

    private:
        template <bool syncPreviewActive>
        StatusInfo update_sub(const bool hideFiltered);

        struct RefIndex
        {
            unsigned int folderIndex;
            unsigned int rowIndex;
        };

        std::vector<RefIndex> refView;
        FolderComparison& folderCmp;
    };


//############################################################################
//inline implementation
    inline
    const FileCompareLine& GridView::operator[] (unsigned row) const
    {
        const unsigned int folderInd = refView[row].folderIndex;
        const unsigned int rowInd    = refView[row].rowIndex;

        return folderCmp[folderInd].fileCmp[rowInd];
    }

    inline
    FileCompareLine& GridView::operator[] (unsigned row)
    {
        //code re-use of const method: see Meyers Effective C++
        return const_cast<FileCompareLine&>(static_cast<const GridView&>(*this).operator[](row));
    }


    inline
    unsigned int GridView::elementsOnView() const
    {
        return refView.size();
    }


    inline
    const FolderPair GridView::getFolderPair(const unsigned int row) const
    {
        const unsigned int folderInd = refView[row].folderIndex;
        const FolderCompareLine& folderCmpLine = folderCmp[folderInd];
        return folderCmpLine.syncPair;
    }
}


#endif // GRIDVIEW_H_INCLUDED
