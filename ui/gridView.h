#ifndef GRIDVIEW_H_INCLUDED
#define GRIDVIEW_H_INCLUDED

#include "../structures.h"


namespace FreeFileSync
{
    //gui view of FolderComparison
    class GridView
    {
    public:
        GridView(FolderComparison& results) : folderCmp(results) {}

        const FileCompareLine& operator[] (unsigned row) const;

        //unsigned getResultsIndex(const unsigned viewIndex); //convert index on GridView to index on FolderComparison

        unsigned int elementsOnView() const;

        unsigned int elementsTotal() const;

        //convert view references to FolderCompRef
        void viewRefToFolderRef(const std::set<int>& viewRef, FolderCompRef& output);

        const FolderPair getFolderPair(const unsigned int row) const;

        struct StatusInfo
        {
            bool existsLeftOnly;
            bool existsRightOnly;
            bool existsLeftNewer;
            bool existsRightNewer;
            bool existsDifferent;
            bool existsEqual;

            unsigned int filesOnLeftView;
            unsigned int foldersOnLeftView;
            unsigned int filesOnRightView;
            unsigned int foldersOnRightView;

            wxULongLong filesizeLeftView;
            wxULongLong filesizeRightView;
        };

        StatusInfo update(const bool includeLeftOnly,
                          const bool includeRightOnly,
                          const bool includeLeftNewer,
                          const bool includeRightNewer,
                          const bool includeDifferent,
                          const bool includeEqual,
                          const bool hideFiltered);

        //sorting...
        enum SortType
        {
            SORT_BY_REL_NAME,
            SORT_BY_FILENAME,
            SORT_BY_FILESIZE,
            SORT_BY_DATE,
            SORT_BY_CMP_RESULT,
            SORT_BY_DIRECTORY
        };

        void sortView(const SortType type, const bool onLeft, const bool ascending);

    private:
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
