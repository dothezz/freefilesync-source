// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) 2008-2011 ZenJu (zhnmju123 AT gmx.de)                    *
// **************************************************************************

#include "file_hierarchy.h"
#include "shared/build_info.h"

using namespace zen;


void HierarchyObject::removeEmptyRec()
{
    bool haveEmpty = false;
    auto isEmpty = [&](const FileSystemObject& fsObj) -> bool
    {
        bool objEmpty = fsObj.isEmpty();
        if (objEmpty)
            haveEmpty = true;
        return objEmpty;
    };

    refSubFiles().remove_if(isEmpty);
    refSubLinks().remove_if(isEmpty);
    refSubDirs ().remove_if(isEmpty);

    if (haveEmpty) //notify if actual deletion happened
        notifySyncCfgChanged(); //mustn't call this in ~FileSystemObject(), since parent, usually a DirMapping, is already partially destroyed and existing as a pure HierarchyObject!

    //recurse
    std::for_each(refSubDirs().begin(), refSubDirs().end(), std::mem_fun_ref(&HierarchyObject::removeEmptyRec));
}


SyncOperation FileSystemObject::getSyncOperation(
    CompareFilesResult cmpResult,
    bool selectedForSynchronization,
    SyncDirection syncDir,
    const std::wstring& syncDirConflict)
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
                case SYNC_DIR_LEFT:
                    return SO_DELETE_LEFT; //delete files on left
                case SYNC_DIR_RIGHT:
                    return SO_CREATE_NEW_RIGHT; //copy files to right
                case SYNC_DIR_NONE:
                    return syncDirConflict.empty() ? SO_DO_NOTHING : SO_UNRESOLVED_CONFLICT;
            }
            break;

        case FILE_RIGHT_SIDE_ONLY:
            switch (syncDir)
            {
                case SYNC_DIR_LEFT:
                    return SO_CREATE_NEW_LEFT; //copy files to left
                case SYNC_DIR_RIGHT:
                    return SO_DELETE_RIGHT; //delete files on right
                case SYNC_DIR_NONE:
                    return syncDirConflict.empty() ? SO_DO_NOTHING : SO_UNRESOLVED_CONFLICT;
            }
            break;

        case FILE_LEFT_NEWER:
        case FILE_RIGHT_NEWER:
        case FILE_DIFFERENT:
        case FILE_CONFLICT:
            switch (syncDir)
            {
                case SYNC_DIR_LEFT:
                    return SO_OVERWRITE_LEFT; //copy from right to left
                case SYNC_DIR_RIGHT:
                    return SO_OVERWRITE_RIGHT; //copy from left to right
                case SYNC_DIR_NONE:
                    return syncDirConflict.empty() ? SO_DO_NOTHING : SO_UNRESOLVED_CONFLICT;
            }
            break;

        case FILE_DIFFERENT_METADATA:
            switch (syncDir)
            {
                case SYNC_DIR_LEFT:
                    return SO_COPY_METADATA_TO_LEFT;
                case SYNC_DIR_RIGHT:
                    return SO_COPY_METADATA_TO_RIGHT;
                case SYNC_DIR_NONE:
                    return syncDirConflict.empty() ? SO_DO_NOTHING : SO_UNRESOLVED_CONFLICT;
            }
            break;

        case FILE_EQUAL:
            assert(syncDir == SYNC_DIR_NONE);
            return SO_EQUAL;
    }

    return SO_DO_NOTHING; //dummy
}


namespace
{
template <class Predicate> inline
bool hasDirectChild(const HierarchyObject& hierObj, Predicate p)
{
    return std::find_if(hierObj.refSubFiles().begin(), hierObj.refSubFiles().end(), p) != hierObj.refSubFiles().end() ||
           std::find_if(hierObj.refSubLinks().begin(), hierObj.refSubLinks().end(), p) != hierObj.refSubLinks().end() ||
           std::find_if(hierObj.refSubDirs(). begin(), hierObj.refSubDirs(). end(), p) != hierObj.refSubDirs ().end();
}
}


SyncOperation DirMapping::getSyncOperation() const
{
    if (!syncOpUpToDate)
    {
        syncOpUpToDate = true;
        //redetermine...

        //suggested operation for directory only
        syncOpBuffered = FileSystemObject::getSyncOperation();

        //action for child elements may occassionally have to overwrite parent task:
        switch (syncOpBuffered)
        {
            case SO_OVERWRITE_LEFT:
            case SO_OVERWRITE_RIGHT:
                assert(false);
            case SO_CREATE_NEW_LEFT:
            case SO_CREATE_NEW_RIGHT:
            case SO_COPY_METADATA_TO_LEFT:
            case SO_COPY_METADATA_TO_RIGHT:
            case SO_EQUAL:
                break; //take over suggestion, no problem for child-elements
            case SO_DELETE_LEFT:
            case SO_DELETE_RIGHT:
            case SO_DO_NOTHING:
            case SO_UNRESOLVED_CONFLICT:
            {
                if (isEmpty<LEFT_SIDE>())
                {
                    //1. if at least one child-element is to be created, make sure parent folder is created also
                    //note: this automatically fulfills "create parent folders even if excluded";
                    //see http://sourceforge.net/tracker/index.php?func=detail&aid=2628943&group_id=234430&atid=1093080
                    if (hasDirectChild(*this, [](const FileSystemObject& fsObj) { return fsObj.getSyncOperation() == SO_CREATE_NEW_LEFT; }))
                    syncOpBuffered = SO_CREATE_NEW_LEFT;
                    //2. cancel parent deletion if only a single child is not also scheduled for deletion
                    else if (syncOpBuffered == SO_DELETE_RIGHT &&
                    hasDirectChild(*this, [](const FileSystemObject& fsObj) { return fsObj.getSyncOperation() != SO_DELETE_RIGHT; }))
                    syncOpBuffered = SO_DO_NOTHING;
                }
                else if (isEmpty<RIGHT_SIDE>())
                {
                    if (hasDirectChild(*this, [](const FileSystemObject& fsObj) { return fsObj.getSyncOperation() == SO_CREATE_NEW_RIGHT; }))
                    syncOpBuffered = SO_CREATE_NEW_RIGHT;
                    else if (syncOpBuffered == SO_DELETE_LEFT &&
                    hasDirectChild(*this, [](const FileSystemObject& fsObj) { return fsObj.getSyncOperation() != SO_DELETE_LEFT; }))
                    syncOpBuffered = SO_DO_NOTHING;
                }
            }
            break;
        }
    }
    return syncOpBuffered;
}
