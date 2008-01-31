#ifndef ALGORITHM_H_INCLUDED
#define ALGORITHM_H_INCLUDED

#include "FreeFileSync.h"
#include "library/statusHandler.h"


namespace FreeFileSync
{
    wxString formatFilesizeToShortString(const wxULongLong& filesize);
    wxString formatFilesizeToShortString(const double filesize);
    Zstring getFormattedDirectoryName(const Zstring& dirname);

    void swapGrids(FileCompareResult& grid);

    void adjustModificationTimes(const Zstring& parentDirectory, const int timeInSeconds, ErrorHandler* errorHandler) throw(AbortThisProcess);

    void deleteOnGridAndHD(FileCompareResult& grid, const set<int>& rowsToDelete, ErrorHandler* errorHandler, const bool useRecycleBin) throw(AbortThisProcess);
    void addSubElements(set<int>& subElements, const FileCompareResult& grid, const FileCompareLine& relevantRow);

    void filterCurrentGridData(FileCompareResult& currentGridData, const wxString& includeFilter, const wxString& excludeFilter);
    void removeFilterOnCurrentGridData(FileCompareResult& currentGridData);

    bool sameFileTime(const time_t a, const time_t b);

    wxString utcTimeToLocalString(const time_t utcTime);

    //enhanced binary search template: returns an iterator
    template <class ForwardIterator, class T>
    ForwardIterator custom_binary_search (ForwardIterator first, ForwardIterator last, const T& value)
    {
        first = lower_bound(first, last, value);
        if (first!=last && !(value<*first))
            return first;
        else
            return last;
    }

#ifdef FFS_WIN
//detect if FAT/FAT32 drive needs a +-1h time shift after daylight saving time (DST) switch due to known windows bug:
//http://www.codeproject.com/KB/datetime/dstbugs.aspx

//NO performance issue: debug build: 50 ms for 200000 files processed in for-loop
    void checkForDSTChange(const FileCompareResult& gridData, const vector<FolderPair>& directoryPairsFormatted, int& timeShift, wxString& driveName);
#endif  //FFS_WIN

}

#endif // ALGORITHM_H_INCLUDED
