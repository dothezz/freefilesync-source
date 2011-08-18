// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) 2008-2011 ZenJu (zhnmju123 AT gmx.de)                    *
// **************************************************************************

#include "file_hierarchy.h"
#include "shared/build_info.h"

using namespace zen;

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


template <class V, class Predicate> inline
void vector_remove_if(V& vec, Predicate p)
{
	vec.erase(std::remove_if(vec.begin(), vec.end(), p), vec.end());
}


void removeEmptyRec(HierarchyObject& hierObj)
{
	auto isEmpty = [](const FileSystemObject& fsObj) { return fsObj.isEmpty(); };

	vector_remove_if(hierObj.refSubFiles(), isEmpty);
	vector_remove_if(hierObj.refSubLinks(), isEmpty);
	vector_remove_if(hierObj.refSubDirs (), isEmpty);

    //recurse
    std::for_each(hierObj.refSubDirs().begin(), hierObj.refSubDirs().end(), removeEmptyRec);
}


void BaseDirMapping::removeEmpty(BaseDirMapping& baseDir)
{
    removeEmptyRec(baseDir);
}


SyncOperation FileSystemObject::getSyncOperation(
    CompareFilesResult cmpResult,
    bool selectedForSynchronization,
    SyncDirectionIntern syncDir)
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

        case FILE_DIFFERENT_METADATA:
            switch (syncDir)
            {
                case SYNC_DIR_INT_LEFT:
                    return SO_COPY_METADATA_TO_LEFT;
                case SYNC_DIR_INT_RIGHT:
                    return SO_COPY_METADATA_TO_RIGHT;
                case SYNC_DIR_INT_NONE:
                    return SO_DO_NOTHING;
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