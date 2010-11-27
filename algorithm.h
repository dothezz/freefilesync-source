// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) 2008-2010 ZenJu (zhnmju123 AT gmx.de)                    *
// **************************************************************************
//
#ifndef ALGORITHM_H_INCLUDED
#define ALGORITHM_H_INCLUDED

#include "file_hierarchy.h"


namespace ffs3
{
class BaseFilter;

    wxString extractJobName(const wxString& configFilename);

void swapGrids(const MainConfiguration& config, FolderComparison& folderCmp);

struct DeterminationProblem //callback
{
    virtual ~DeterminationProblem() {}
    virtual void reportWarning(const wxString& text) = 0;
};
void redetermineSyncDirection(const SyncConfiguration& config, BaseDirMapping& baseDirectory, DeterminationProblem* handler); //handler may be NULL
void redetermineSyncDirection(const MainConfiguration& currentMainCfg, FolderComparison& folderCmp, DeterminationProblem* handler);

void setSyncDirectionRec(SyncDirection newDirection, FileSystemObject& fsObj); //set new direction (recursively)

bool allElementsEqual(const FolderComparison& folderCmp);

//filtering
void applyFiltering(const MainConfiguration& currentMainCfg, FolderComparison& folderCmp); //full filter apply
void addExcludeFiltering(const Zstring& excludeFilter, FolderComparison& folderCmp); //exclude additional entries only

void setActiveStatus(bool newStatus, FolderComparison& folderCmp); //activate or deactivate all rows
void setActiveStatus(bool newStatus, FileSystemObject& fsObj);     //activate or deactivate row: works recursively!


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
    virtual void deletionSuccessful(size_t deletedItems) = 0;  //called for each file/folder that has been deleted

};
void deleteFromGridAndHD(FolderComparison& folderCmp,                        //attention: rows will be physically deleted!
                         std::vector<FileSystemObject*>& rowsToDeleteOnLeft,  //refresh GUI grid after deletion to remove invalid rows
                         std::vector<FileSystemObject*>& rowsToDeleteOnRight, //all pointers need to be bound!
                         const bool deleteOnBothSides,
                         const bool useRecycleBin,
                         DeleteFilesHandler& statusHandler);
}

#endif // ALGORITHM_H_INCLUDED
