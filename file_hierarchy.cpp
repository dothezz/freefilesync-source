// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) 2008-2011 ZenJu (zhnmju123 AT gmx.de)                    *
// **************************************************************************

#include "file_hierarchy.h"
#include <zen/i18n.h>
#include <zen/utf8.h>

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

namespace
{
SyncOperation proposedSyncOperation(CompareFilesResult cmpResult,
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


template <class Predicate> inline
bool hasDirectChild(const HierarchyObject& hierObj, Predicate p)
{
    return std::find_if(hierObj.refSubFiles().begin(), hierObj.refSubFiles().end(), p) != hierObj.refSubFiles().end() ||
           std::find_if(hierObj.refSubLinks().begin(), hierObj.refSubLinks().end(), p) != hierObj.refSubLinks().end() ||
           std::find_if(hierObj.refSubDirs(). begin(), hierObj.refSubDirs(). end(), p) != hierObj.refSubDirs ().end();
}
}


SyncOperation FileSystemObject::testSyncOperation(SyncDirection testSyncDir, bool active) const
{
    return proposedSyncOperation(getCategory(), active, testSyncDir, getSyncOpConflict());
}


SyncOperation FileSystemObject::getSyncOperation() const
{
    return FileSystemObject::testSyncOperation(syncDir, selectedForSynchronization);
    //no *not* make a virtual call to testSyncOperation()! See FileMapping::testSyncOperation()!
}


//SyncOperation DirMapping::testSyncOperation() const -> not required: we do NOT want to consider child elements when testing!


SyncOperation DirMapping::getSyncOperation() const
{
    if (!syncOpUpToDate)
    {
        syncOpUpToDate = true;
        //redetermine...

        //suggested operation *not* considering child elements
        syncOpBuffered = FileSystemObject::getSyncOperation();

        //action for child elements may occassionally have to overwrite parent task:
        switch (syncOpBuffered)
        {
            case SO_OVERWRITE_LEFT:
            case SO_OVERWRITE_RIGHT:
            case SO_MOVE_LEFT_SOURCE:
            case SO_MOVE_LEFT_TARGET:
            case SO_MOVE_RIGHT_SOURCE:
            case SO_MOVE_RIGHT_TARGET:
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
                    if (hasDirectChild(*this,
                                       [](const FileSystemObject& fsObj) -> bool { const SyncOperation op = fsObj.getSyncOperation(); return  op == SO_CREATE_NEW_LEFT || op == SO_MOVE_LEFT_TARGET; }))
                        syncOpBuffered = SO_CREATE_NEW_LEFT;
                    //2. cancel parent deletion if only a single child is not also scheduled for deletion
                    else if (syncOpBuffered == SO_DELETE_RIGHT &&
                             hasDirectChild(*this,
                                            [](const FileSystemObject& fsObj) -> bool
                {
                    if (fsObj.isEmpty()) return false; //fsObj may already be empty because it once contained a "move source"
                        const SyncOperation op = fsObj.getSyncOperation(); return op != SO_DELETE_RIGHT && op != SO_MOVE_RIGHT_SOURCE;
                    }))
                    syncOpBuffered = SO_DO_NOTHING;
                }
                else if (isEmpty<RIGHT_SIDE>())
                {
                    if (hasDirectChild(*this,
                                       [](const FileSystemObject& fsObj) -> bool  { const SyncOperation op = fsObj.getSyncOperation();  return  op == SO_CREATE_NEW_RIGHT || op == SO_MOVE_RIGHT_TARGET;  }))
                        syncOpBuffered = SO_CREATE_NEW_RIGHT;
                    else if (syncOpBuffered == SO_DELETE_LEFT &&
                             hasDirectChild(*this,
                                            [](const FileSystemObject& fsObj) -> bool
                {
                    if (fsObj.isEmpty()) return false;
                        const SyncOperation op = fsObj.getSyncOperation();
                        return op != SO_DELETE_LEFT && op != SO_MOVE_LEFT_SOURCE;
                    }))
                    syncOpBuffered = SO_DO_NOTHING;
                }
            }
            break;
        }
    }
    return syncOpBuffered;
}


SyncOperation FileMapping::testSyncOperation(SyncDirection testSyncDir, bool active) const
{
    SyncOperation op = FileSystemObject::testSyncOperation(testSyncDir, active);

    /*
        check whether we can optimize "create + delete" via "move":
        note: as long as we consider "create + delete" cases only, detection of renamed files, should be fine even for "binary" comparison variant!
    */
    if (const FileSystemObject* refFile = dynamic_cast<const FileMapping*>(FileSystemObject::retrieve(moveFileRef))) //we expect a "FileMapping", but only need a "FileSystemObject"
    {
        SyncOperation opRef = refFile->FileSystemObject::getSyncOperation(); //do *not* make a virtual call!

        if (op    == SO_CREATE_NEW_LEFT &&
            opRef == SO_DELETE_LEFT)
            op = SO_MOVE_LEFT_TARGET;
        else if (op    == SO_DELETE_LEFT &&
                 opRef == SO_CREATE_NEW_LEFT)
            op = SO_MOVE_LEFT_SOURCE;
        else if (op    == SO_CREATE_NEW_RIGHT &&
                 opRef == SO_DELETE_RIGHT)
            op = SO_MOVE_RIGHT_TARGET;
        else if (op    == SO_DELETE_RIGHT &&
                 opRef == SO_CREATE_NEW_RIGHT)
            op = SO_MOVE_RIGHT_SOURCE;
    }
    return op;
}


SyncOperation FileMapping::getSyncOperation() const
{
    return FileMapping::testSyncOperation(getSyncDir(), isActive());
}


std::wstring zen::getCategoryDescription(CompareFilesResult cmpRes)
{
    switch (cmpRes)
    {
        case FILE_LEFT_SIDE_ONLY:
            return _("File/folder exists on left side only");
        case FILE_RIGHT_SIDE_ONLY:
            return _("File/folder exists on right side only");
        case FILE_LEFT_NEWER:
            return _("Left file is newer");
        case FILE_RIGHT_NEWER:
            return _("Right file is newer");
        case FILE_DIFFERENT:
            return _("Files have different content");
        case FILE_EQUAL:
            return _("Both sides are equal");
        case FILE_DIFFERENT_METADATA:
            return _("Files/folders differ in attributes only");
        case FILE_CONFLICT:
            return _("Conflict/file cannot be categorized");
    }
    assert(false);
    return std::wstring();
}


std::wstring zen::getCategoryDescription(const FileSystemObject& fsObj)
{
    const CompareFilesResult cmpRes = fsObj.getCategory();
    if (cmpRes == FILE_CONFLICT)
        return fsObj.getCatConflict();

    return getCategoryDescription(cmpRes);
}


std::wstring zen::getSyncOpDescription(SyncOperation op)
{
    switch (op)
    {
        case SO_CREATE_NEW_LEFT:
            return _("Copy new file/folder to left");
        case SO_CREATE_NEW_RIGHT:
            return _("Copy new file/folder to right");
        case SO_DELETE_LEFT:
            return _("Delete left file/folder");
        case SO_DELETE_RIGHT:
            return _("Delete right file/folder");
        case SO_MOVE_LEFT_SOURCE:
        case SO_MOVE_LEFT_TARGET:
            return _("Move file on left");
        case SO_MOVE_RIGHT_SOURCE:
        case SO_MOVE_RIGHT_TARGET:
            return _("Move file on right");
        case SO_OVERWRITE_LEFT:
            return _("Overwrite left file/folder with right one");
        case SO_OVERWRITE_RIGHT:
            return _("Overwrite right file/folder with left one");
        case SO_DO_NOTHING:
            return _("Do nothing");
        case SO_EQUAL:
            return _("Both sides are equal");
        case SO_COPY_METADATA_TO_LEFT:
            return _("Copy file attributes only to left");
        case SO_COPY_METADATA_TO_RIGHT:
            return _("Copy file attributes only to right");
        case SO_UNRESOLVED_CONFLICT: //not used on GUI, but in .csv
            _("Conflict/file cannot be categorized");
    }
    assert(false);
    return std::wstring();
}


std::wstring zen::getSyncOpDescription(const FileSystemObject& fsObj)
{
    const SyncOperation op = fsObj.getSyncOperation();
    switch (op)
    {
        case SO_CREATE_NEW_LEFT:
        case SO_CREATE_NEW_RIGHT:
        case SO_DELETE_LEFT:
        case SO_DELETE_RIGHT:
        case SO_OVERWRITE_LEFT:
        case SO_OVERWRITE_RIGHT:
        case SO_DO_NOTHING:
        case SO_EQUAL:
        case SO_COPY_METADATA_TO_LEFT:
        case SO_COPY_METADATA_TO_RIGHT:
            return getSyncOpDescription(op); //use generic description

        case SO_MOVE_LEFT_SOURCE:
        case SO_MOVE_LEFT_TARGET:
        case SO_MOVE_RIGHT_SOURCE:
        case SO_MOVE_RIGHT_TARGET:
            if (const FileMapping* sourceFile = dynamic_cast<const FileMapping*>(&fsObj))
                if (const FileMapping* targetFile = dynamic_cast<const FileMapping*>(FileSystemObject::retrieve(sourceFile->getMoveRef())))
                {
                    const bool onLeft   = op == SO_MOVE_LEFT_SOURCE || op == SO_MOVE_LEFT_TARGET;
                    const bool isSource = op == SO_MOVE_LEFT_SOURCE || op == SO_MOVE_RIGHT_SOURCE;

                    if (!isSource)
                        std::swap(sourceFile, targetFile);

                    auto getRelName = [&](const FileSystemObject& fso, bool leftSide) { return leftSide ? fso.getRelativeName<LEFT_SIDE>() : fso.getRelativeName<RIGHT_SIDE>(); };

                    const Zstring relSource = getRelName(*sourceFile,  onLeft);
                    const Zstring relTarget = getRelName(*targetFile, !onLeft);

                    return getSyncOpDescription(op) + L"\n" +
                           (EqualFilename()(beforeLast(relSource, FILE_NAME_SEPARATOR), beforeLast(relTarget, FILE_NAME_SEPARATOR)) ? //returns empty string if ch not found
                            //detected pure "rename"
                            L"\"" + utf8CvrtTo<std::wstring>(afterLast(relSource, FILE_NAME_SEPARATOR)) + L"\"" + L" ->\n" + //show short name only
                            L"\"" + utf8CvrtTo<std::wstring>(afterLast(relTarget, FILE_NAME_SEPARATOR)) + L"\"" :
                            //"move" or "move + rename"
                            L"\"" + utf8CvrtTo<std::wstring>(relSource) + L"\"" + L" ->\n" +
                            L"\"" + utf8CvrtTo<std::wstring>(relTarget) + L"\"");
                    //attention: ::SetWindowText() doesn't handle tab characters correctly in combination with certain file names, so don't use them
                }
            break;
        case SO_UNRESOLVED_CONFLICT:
            return fsObj.getSyncOpConflict();
    }

    assert(false);
    return std::wstring();
}
