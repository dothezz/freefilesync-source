#ifndef ALGORITHM_H_INCLUDED
#define ALGORITHM_H_INCLUDED

#include "structures.h"
#include "library/resources.h"

class ErrorHandler;


namespace FreeFileSync
{
    wxString formatFilesizeToShortString(const wxLongLong& filesize);
    wxString formatFilesizeToShortString(const wxULongLong& filesize);
    wxString formatFilesizeToShortString(const double filesize);

    Zstring getFormattedDirectoryName(const Zstring& dirname);

    bool endsWithPathSeparator(const Zstring& name);

    void swapGrids(FolderComparison& folderCmp);

    void addSubElements(const FileComparison& fileCmp, const FileCompareLine& relevantRow, std::set<int>& subElements);

    //manual deletion of files on main grid: runs at individual directory pair level
    wxString deleteFromGridAndHDPreview(const FileComparison& fileCmp,
                                        const std::set<int>& rowsToDeleteOnLeft,
                                        const std::set<int>& rowsToDeleteOnRight,
                                        const bool deleteOnBothSides);

    void deleteFromGridAndHD(FileComparison& fileCmp,
                             const std::set<int>& rowsToDeleteOnLeft,
                             const std::set<int>& rowsToDeleteOnRight,
                             const bool deleteOnBothSides,
                             const bool useRecycleBin,
                             ErrorHandler* errorHandler);


    wxString utcTimeToLocalString(const wxLongLong& utcTime);

    //enhanced binary search template: returns an iterator
    template <class ForwardIterator, class T>
    inline
    ForwardIterator custom_binary_search (ForwardIterator first, ForwardIterator last, const T& value)
    {
        first = lower_bound(first, last, value);
        if (first != last && !(value < *first))
            return first;
        else
            return last;
    }

#ifdef FFS_WIN
    Zstring getLastErrorFormatted(const unsigned long lastError = 0); //try to get additional Windows error information
#elif defined FFS_LINUX
    Zstring getLastErrorFormatted(const int lastError = 0); //try to get additional Linux error information
#endif
}


inline
bool FreeFileSync::endsWithPathSeparator(const Zstring& name)
{
    const size_t len = name.length();
    return len && (name[len - 1] == GlobalResources::FILE_NAME_SEPARATOR);
}



#endif // ALGORITHM_H_INCLUDED
