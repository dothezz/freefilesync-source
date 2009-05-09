#ifndef FFS_FILTER_H_INCLUDED
#define FFS_FILTER_H_INCLUDED

#include "../structures.h"


namespace FreeFileSync
{
    void filterGridData(FolderComparison& folderCmp, const wxString& includeFilter, const wxString& excludeFilter);
    void includeAllRowsOnGrid(FolderComparison& folderCmp);
    void excludeAllRowsOnGrid(FolderComparison& folderCmp);
}


#endif // FFS_FILTER_H_INCLUDED
