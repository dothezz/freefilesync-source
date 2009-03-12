#ifndef ALGORITHM_H_INCLUDED
#define ALGORITHM_H_INCLUDED

#include "FreeFileSync.h"
#include "library/statusHandler.h"
#include "library/resources.h"


namespace FreeFileSync
{
    wxString formatFilesizeToShortString(const wxULongLong& filesize);
    wxString formatFilesizeToShortString(const double filesize);
    Zstring getFormattedDirectoryName(const Zstring& dirname);

    bool endsWithPathSeparator(const Zstring& name);

    void swapGrids(FileCompareResult& grid);

    void addSubElements(const FileCompareResult& grid, const FileCompareLine& relevantRow, std::set<int>& subElements);

    //manual deletion of files on main grid
    wxString deleteFromGridAndHDPreview(const FileCompareResult& grid,
                                        const std::set<int>& rowsToDeleteOnLeft,
                                        const std::set<int>& rowsToDeleteOnRight,
                                        const bool deleteOnBothSides);

    void deleteFromGridAndHD(FileCompareResult& grid,
                             const std::set<int>& rowsToDeleteOnLeft,
                             const std::set<int>& rowsToDeleteOnRight,
                             const bool deleteOnBothSides,
                             const bool useRecycleBin,
                             ErrorHandler* errorHandler);

    void filterGridData(FileCompareResult& currentGridData, const wxString& includeFilter, const wxString& excludeFilter);
    void includeAllRowsOnGrid(FileCompareResult& currentGridData);
    void excludeAllRowsOnGrid(FileCompareResult& currentGridData);

    wxString utcTimeToLocalString(const time_t utcTime);

    //enhanced binary search template: returns an iterator
    template <class ForwardIterator, class T>
    ForwardIterator custom_binary_search (ForwardIterator first, ForwardIterator last, const T& value)
    {
        first = lower_bound(first, last, value);
        if (first != last && !(value < *first))
            return first;
        else
            return last;
    }

#ifdef FFS_WIN
    Zstring getLastErrorFormatted(const unsigned long lastError = 0); //try to get additional windows error information

//detect if FAT/FAT32 drive needs a +-1h time shift after daylight saving time (DST) switch due to known windows bug:
//http://www.codeproject.com/KB/datetime/dstbugs.aspx

//NO performance issue: debug build: 50 ms for 200000 files processed in for-loop
    void checkForDSTChange(const FileCompareResult& gridData, const std::vector<FolderPair>& directoryPairsFormatted, int& timeShift, wxString& driveName);
#endif  //FFS_WIN

}


inline
bool FreeFileSync::endsWithPathSeparator(const Zstring& name)
{
    const size_t len = name.length();
    return len && (name[len - 1] == GlobalResources::FILE_NAME_SEPARATOR);
}



#endif // ALGORITHM_H_INCLUDED
