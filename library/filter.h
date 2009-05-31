#ifndef FFS_FILTER_H_INCLUDED
#define FFS_FILTER_H_INCLUDED

#include "../structures.h"


namespace FreeFileSync
{
    class FilterProcess //relative filtering
    {
    public:
        FilterProcess(const wxString& includeFilter, const wxString& excludeFilter);

        bool matchesFileFilter(const DefaultChar* relFilename) const;
        bool matchesDirFilter(const DefaultChar* relDirname) const;

        void filterGridData(FolderComparison& folderCmp) const;

        static void includeAllRowsOnGrid(FolderComparison& folderCmp);
        static void excludeAllRowsOnGrid(FolderComparison& folderCmp);

    private:
        std::set<Zstring> filterFileIn;
        std::set<Zstring> filterFolderIn;
        std::set<Zstring> filterFileEx;
        std::set<Zstring> filterFolderEx;
    };
}


#endif // FFS_FILTER_H_INCLUDED