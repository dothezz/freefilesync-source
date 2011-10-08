// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) 2008-2011 ZenJu (zhnmju123 AT gmx.de)                    *
// **************************************************************************

#ifndef ALGORITHM_H_INCLUDED
#define ALGORITHM_H_INCLUDED

#include "file_hierarchy.h"
#include "lib/soft_filter.h"

namespace zen
{
void swapGrids(const MainConfiguration& config, FolderComparison& folderCmp);

struct DeterminationProblem //callback
{
    virtual ~DeterminationProblem() {}
    virtual void reportWarning(const wxString& text) = 0;
};
std::vector<DirectionConfig> extractDirectionCfg(const MainConfiguration& mainCfg);

void redetermineSyncDirection(const DirectionConfig& directConfig, BaseDirMapping& baseDirectory, DeterminationProblem* handler); //handler may be NULL
void redetermineSyncDirection(const MainConfiguration& mainCfg, FolderComparison& folderCmp, DeterminationProblem* handler);

void setSyncDirectionRec(SyncDirection newDirection, FileSystemObject& fsObj); //set new direction (recursively)

bool allElementsEqual(const FolderComparison& folderCmp);

//filtering
void applyFiltering  (FolderComparison& folderCmp, const MainConfiguration& mainCfg); //full filter apply
void addHardFiltering(BaseDirMapping& baseMap, const Zstring& excludeFilter);     //exclude additional entries only
void addSoftFiltering(BaseDirMapping& baseMap, const SoftFilter& timeSizeFilter); //exclude additional entries only

void applyTimeSpanFilter(FolderComparison& folderCmp, const Int64& timeFrom, const Int64& timeTo); //overwrite current active/inactive settings

void setActiveStatus(bool newStatus, FolderComparison& folderCmp); //activate or deactivate all rows
void setActiveStatus(bool newStatus, FileSystemObject& fsObj);     //activate or deactivate row: (not recursively anymore)


//manual deletion of files on main grid
std::pair<wxString, int> deleteFromGridAndHDPreview(           //returns wxString with elements to be deleted and total count of selected(!) objects, NOT total files/dirs!
    const std::vector<FileSystemObject*>& rowsToDeleteOnLeft,  //all pointers need to be bound!
    const std::vector<FileSystemObject*>& rowsToDeleteOnRight, //
    bool deleteOnBothSides);

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
    virtual void notifyDeletion(const Zstring& currentObject) = 0; //called for each file/folder that has been deleted
};
void deleteFromGridAndHD(std::vector<FileSystemObject*>& rowsToDeleteOnLeft,  //refresh GUI grid after deletion to remove invalid rows
                         std::vector<FileSystemObject*>& rowsToDeleteOnRight, //all pointers need to be bound!
                         FolderComparison& folderCmp,                         //attention: rows will be physically deleted!
                         const std::vector<DirectionConfig>& directCfgs,
                         bool deleteOnBothSides,
                         bool useRecycleBin,
                         DeleteFilesHandler& statusHandler);
}

#endif // ALGORITHM_H_INCLUDED
