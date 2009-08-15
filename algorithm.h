#ifndef ALGORITHM_H_INCLUDED
#define ALGORITHM_H_INCLUDED

#include "structures.h"

class ErrorHandler;
class wxComboBox;
class wxTextCtrl;
class wxDirPickerCtrl;


namespace FreeFileSync
{
    wxString formatFilesizeToShortString(const wxLongLong& filesize);
    wxString formatFilesizeToShortString(const wxULongLong& filesize);
    wxString formatFilesizeToShortString(const double filesize);

    wxString includeNumberSeparator(const wxString& number);

    void setDirectoryName(const wxString& dirname, wxTextCtrl* txtCtrl, wxDirPickerCtrl* dirPicker);
    void setDirectoryName(const wxString& dirname, wxComboBox* txtCtrl, wxDirPickerCtrl* dirPicker);

    void swapGrids(const SyncConfiguration& config, FolderComparison& folderCmp);

    void redetermineSyncDirection(const SyncConfiguration& config, FolderComparison& folderCmp);

    void addSubElements(const FileComparison& fileCmp, const FileCompareLine& relevantRow, std::set<int>& subElements);

    //manual deletion of files on main grid: runs at individual directory pair level
    std::pair<wxString, int> deleteFromGridAndHDPreview(const FileComparison& fileCmp, //returns wxString with elements to be deleted and total count
            const std::set<int>& rowsToDeleteOnLeft,
            const std::set<int>& rowsToDeleteOnRight,
            const bool deleteOnBothSides);

    class DeleteFilesHandler
    {
    public:
        DeleteFilesHandler() {}
        virtual ~DeleteFilesHandler() {}

        enum Response
        {
            IGNORE_ERROR = 10,
            RETRY
        };
        virtual Response reportError(const wxString& errorMessage) = 0;

        //virtual void totalFilesToDelete(int objectsTotal) = 0; //informs about the total number of files to be deleted
        virtual void deletionSuccessful() = 0;  //called for each file/folder that has been deleted

    };
    void deleteFromGridAndHD(FileComparison& fileCmp,
                             const std::set<int>& rowsToDeleteOnLeft,
                             const std::set<int>& rowsToDeleteOnRight,
                             const bool deleteOnBothSides,
                             const bool useRecycleBin,
                             const SyncConfiguration& syncConfig,
                             DeleteFilesHandler* statusHandler);


    wxString utcTimeToLocalString(const wxLongLong& utcTime, const Zstring& filename);

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
}

#endif // ALGORITHM_H_INCLUDED
