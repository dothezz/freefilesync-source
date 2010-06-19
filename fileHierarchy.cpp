// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) 2008-2010 ZenJu (zhnmju123 AT gmx.de)                    *
// **************************************************************************
//
#include "fileHierarchy.h"
#include "shared/buildInfo.h"

using namespace FreeFileSync;

namespace
{
struct LowerID
{
    bool operator()(const FileSystemObject& a, HierarchyObject::ObjectID b) const
    {
        return a.getId() < b;
    }

    bool operator()(const FileSystemObject& a, const FileSystemObject& b) const //used by VC++
    {
        return a.getId() < b.getId();
    }

    bool operator()(HierarchyObject::ObjectID a, const FileSystemObject& b) const
    {
        return a < b.getId();
    }
};
}


const FileSystemObject* HierarchyObject::retrieveById(ObjectID id) const //returns NULL if object is not found
{
    //ATTENTION: HierarchyObject::retrieveById() can only work correctly if the following conditions are fulfilled:
    //1. on each level, files are added first, symlinks, then directories (=> file id < link id < dir id)
    //2. when a directory is added, all subdirectories must be added immediately (recursion) before the next dir on this level is added
    //3. entries may be deleted but NEVER new ones inserted!!!
    //=> this allows for a quasi-binary search by id!

    //See MergeSides::execute()!


    //search within sub-files
    SubFileMapping::const_iterator i = std::lower_bound(subFiles.begin(), subFiles.end(), id, LowerID()); //binary search!
    if (i != subFiles.end())
    {
        //id <= i
        if (LowerID()(id, *i))
            return NULL; // --i < id < i
        else //id found
            return &(*i);
    }

    //search within sub-symlinks
    SubLinkMapping::const_iterator j = std::lower_bound(subLinks.begin(), subLinks.end(), id, LowerID()); //binary search!
    if (j != subLinks.end())
    {
        //id <= i
        if (LowerID()(id, *j))
            return NULL; // --i < id < i
        else //id found
            return &(*j);
    }

    //search within sub-directories
    SubDirMapping::const_iterator k = std::lower_bound(subDirs.begin(), subDirs.end(), id, LowerID()); //binary search!
    if (k != subDirs.end() && !LowerID()(id, *k)) //id == j
        return &(*k);
    else if (k == subDirs.begin()) //either begin() == end() or id < begin()
        return NULL;
    else
        return (--k)->retrieveById(id); //j != begin() and id < j
}


struct IsInvalid
{
    bool operator()(const FileSystemObject& fsObj) const
    {
        return fsObj.isEmpty();
    }
};


void FileSystemObject::removeEmptyNonRec(HierarchyObject& hierObj)
{
    //remove invalid files:
    hierObj.useSubFiles().erase(std::remove_if(hierObj.useSubFiles().begin(), hierObj.useSubFiles().end(), IsInvalid()), hierObj.useSubFiles().end());
    //remove invalid symlinks:
    hierObj.useSubLinks().erase(std::remove_if(hierObj.useSubLinks().begin(), hierObj.useSubLinks().end(), IsInvalid()), hierObj.useSubLinks().end());
    //remove invalid directories:
    hierObj.useSubDirs(). erase(std::remove_if(hierObj.useSubDirs(). begin(), hierObj.useSubDirs(). end(), IsInvalid()), hierObj.useSubDirs(). end());
}


void removeEmptyRec(HierarchyObject& hierObj)
{
    FileSystemObject::removeEmptyNonRec(hierObj);

    //recurse
    std::for_each(hierObj.useSubDirs().begin(), hierObj.useSubDirs().end(), removeEmptyRec);
}


void FileSystemObject::removeEmpty(BaseDirMapping& baseDir)
{
    removeEmptyRec(baseDir);
}


SyncOperation FileSystemObject::getSyncOperation(const CompareFilesResult cmpResult,
        const bool selectedForSynchronization,
        const SyncDirectionIntern syncDir)
{
    if (!selectedForSynchronization)
        return cmpResult == FILE_EQUAL ?
               SO_EQUAL :
               SO_DO_NOTHING;

    switch (cmpResult)
    {
    case FILE_LEFT_SIDE_ONLY:
        switch (syncDir)
        {
        case SYNC_DIR_INT_LEFT:
            return SO_DELETE_LEFT; //delete files on left
        case SYNC_DIR_INT_RIGHT:
            return SO_CREATE_NEW_RIGHT; //copy files to right
        case SYNC_DIR_INT_NONE:
            return SO_DO_NOTHING;
        case SYNC_DIR_INT_CONFLICT:
            return SO_UNRESOLVED_CONFLICT;
        }
        break;

    case FILE_RIGHT_SIDE_ONLY:
        switch (syncDir)
        {
        case SYNC_DIR_INT_LEFT:
            return SO_CREATE_NEW_LEFT; //copy files to left
        case SYNC_DIR_INT_RIGHT:
            return SO_DELETE_RIGHT; //delete files on right
        case SYNC_DIR_INT_NONE:
            return SO_DO_NOTHING;
        case SYNC_DIR_INT_CONFLICT:
            return SO_UNRESOLVED_CONFLICT;
        }
        break;

    case FILE_LEFT_NEWER:
    case FILE_RIGHT_NEWER:
    case FILE_DIFFERENT:
        switch (syncDir)
        {
        case SYNC_DIR_INT_LEFT:
            return SO_OVERWRITE_LEFT; //copy from right to left
        case SYNC_DIR_INT_RIGHT:
            return SO_OVERWRITE_RIGHT; //copy from left to right
        case SYNC_DIR_INT_NONE:
            return SO_DO_NOTHING;
        case SYNC_DIR_INT_CONFLICT:
            return SO_UNRESOLVED_CONFLICT;
        }
        break;

    case FILE_CONFLICT:
        switch (syncDir)
        {
        case SYNC_DIR_INT_LEFT:
            return SO_OVERWRITE_LEFT; //copy from right to left
        case SYNC_DIR_INT_RIGHT:
            return SO_OVERWRITE_RIGHT; //copy from left to right
        case SYNC_DIR_INT_NONE:
        case SYNC_DIR_INT_CONFLICT:
            return SO_UNRESOLVED_CONFLICT;
        }
        break;

    case FILE_EQUAL:
        assert(syncDir == SYNC_DIR_INT_NONE);
        return SO_EQUAL;
    }

    return SO_DO_NOTHING; //dummy
}


const Zstring& FreeFileSync::getSyncDBFilename()
{
    //32 and 64 bit for Linux and Windows builds are binary incompatible! So give them different names
    //make sure they end with ".ffs_db". These files will not be included into comparison when located in base sync directories
#ifdef FFS_WIN
    static Zstring output(Utility::is64BitBuild ?
                          DefaultStr("sync.x64.ffs_db") :
                          DefaultStr("sync.ffs_db"));
#elif defined FFS_LINUX
    //files beginning with dots are hidden e.g. in Nautilus
    static Zstring output(Utility::is64BitBuild ?
                          DefaultStr(".sync.x64.ffs_db") :
                          DefaultStr(".sync.ffs_db"));
#endif
    return output;
}
