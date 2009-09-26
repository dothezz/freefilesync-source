#ifndef ALGORITHM_H_INCLUDED
#define ALGORITHM_H_INCLUDED

#include "fileHierarchy.h"

class ErrorHandler;
class wxComboBox;
class wxTextCtrl;
class wxDirPickerCtrl;
class wxScrolledWindow;


namespace FreeFileSync
{
    wxString formatFilesizeToShortString(const wxLongLong& filesize);
    wxString formatFilesizeToShortString(const wxULongLong& filesize);
    wxString formatFilesizeToShortString(const double filesize);

    wxString includeNumberSeparator(const wxString& number);

    void setDirectoryName(const wxString& dirname, wxTextCtrl* txtCtrl, wxDirPickerCtrl* dirPicker);
    void setDirectoryName(const wxString& dirname, wxComboBox* txtCtrl, wxDirPickerCtrl* dirPicker);
    void scrollToBottom(wxScrolledWindow* scrWindow);

    void swapGrids2(const MainConfiguration& config, FolderComparison& folderCmp);

    void redetermineSyncDirection(const SyncConfiguration& config, HierarchyObject& baseDirectory);
    void redetermineSyncDirection(const MainConfiguration& currentMainCfg, FolderComparison& folderCmp);

    void setSyncDirection(SyncDirection newDirection, FileSystemObject& fsObj); //set new direction (recursively)

    void applyFiltering(const MainConfiguration& currentMainCfg, FolderComparison& folderCmp);

    //manual deletion of files on main grid
    std::pair<wxString, int> deleteFromGridAndHDPreview(           //returns wxString with elements to be deleted and total count
        const std::vector<FileSystemObject*>& rowsToDeleteOnLeft,  //all pointers need to be bound!
        const std::vector<FileSystemObject*>& rowsToDeleteOnRight, //
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
    void deleteFromGridAndHD(FolderComparison& folderCmp,                        //attention: rows will be physically deleted!
                             std::vector<FileSystemObject*>& rowsToDeleteOnLeft,  //refresh GUI grid after deletion to remove invalid rows
                             std::vector<FileSystemObject*>& rowsToDeleteOnRight, //all pointers need to be bound!
                             const bool deleteOnBothSides,
                             const bool useRecycleBin,
                             const MainConfiguration& mainConfig,
                             DeleteFilesHandler* statusHandler);


    wxString utcTimeToLocalString(const wxLongLong& utcTime, const Zstring& filename);
}

#endif // ALGORITHM_H_INCLUDED
