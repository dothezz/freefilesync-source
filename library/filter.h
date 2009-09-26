#ifndef FFS_FILTER_H_INCLUDED
#define FFS_FILTER_H_INCLUDED

#include <wx/string.h>
#include "../shared/zstring.h"
#include <set>
#include "../fileHierarchy.h"


namespace FreeFileSync
{
    class FilterProcess //relative filtering
    {
    public:
        FilterProcess(const wxString& includeFilter, const wxString& excludeFilter);

        bool passFileFilter(const DefaultChar* relFilename) const;
        bool passDirFilter(const DefaultChar* relDirname, bool* subObjMightMatch) const; //subObjMightMatch: file/dir in subdirectories could(!) match
        //note: variable is only set if passDirFilter returns false!
        void filterAll(HierarchyObject& baseDirectory) const; //filter complete data: files and dirs

        static void setActiveStatus(bool newStatus, FolderComparison& folderCmp); //activate or deactivate all rows
        static void setActiveStatus(bool newStatus, FileSystemObject& fsObj);     //activate or deactivate row

        static const FilterProcess& nullFilter(); //filter equivalent to include '*', exclude ''
        bool operator==(const FilterProcess& other) const;
        bool operator!=(const FilterProcess& other) const;
        bool operator<(const FilterProcess& other) const;

    private:
        std::set<Zstring> filterFileIn;
        std::set<Zstring> filterFolderIn;
        std::set<Zstring> filterFileEx;
        std::set<Zstring> filterFolderEx;
    };
}


#endif // FFS_FILTER_H_INCLUDED
