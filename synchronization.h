// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) ZenJu (zenju AT gmx DOT de) - All Rights Reserved        *
// **************************************************************************

#ifndef SYNCHRONIZATION_H_INCLUDED
#define SYNCHRONIZATION_H_INCLUDED

#include <zen/time.h>
#include "file_hierarchy.h"
#include "lib/process_xml.h"
#include "process_callback.h"


namespace zen
{
class SyncStatistics //this class counts *logical* operations, (create, update, delete + bytes), *not* disk accesses!
{
    //-> note the fundamental difference to counting disk accesses!
public:
    SyncStatistics(const HierarchyObject&  hierObj);
    SyncStatistics(const FolderComparison& folderCmp);
    SyncStatistics(const FileMapping& fileObj);

    int getCreate() const;
    template <SelectedSide side> int getCreate() const;

    int getUpdate() const;
    template <SelectedSide side> int getUpdate() const;

    int getDelete() const;
    template <SelectedSide side> int getDelete() const;

    int getConflict() const { return static_cast<int>(conflictMsgs.size()); }

    typedef std::vector<std::pair<Zstring, std::wstring> > ConflictTexts; // Pair(filename/conflict text)
    const ConflictTexts& getConflictMessages() const { return conflictMsgs; }

    Int64 getDataToProcess() const { return dataToProcess; }
    size_t     getRowCount() const { return rowsTotal; }

private:
    //static const size_t MAX_CONFLICTS = 3;
    void init();

    void recurse(const HierarchyObject& hierObj);

    void calcStats(const FileMapping& fileObj);
    void calcStats(const SymLinkMapping& linkObj);
    void calcStats(const DirMapping& dirObj);

    int createLeft, createRight;
    int updateLeft, updateRight;
    int deleteLeft, deleteRight;
    //    int conflict;
    ConflictTexts conflictMsgs; //conflict texts to display as a warning message
    Int64 dataToProcess;
    size_t rowsTotal;
};


struct FolderPairSyncCfg
{
    FolderPairSyncCfg(bool automaticMode,
                      const DeletionPolicy handleDel,
                      const Zstring& versioningDirFmt,
                      int versionCountLimit) :
        inAutomaticMode(automaticMode),
        handleDeletion(handleDel),
        versioningFolder(versioningDirFmt),
        versionCountLimit_(versionCountLimit) {}

    bool inAutomaticMode; //update database if in automatic mode
    DeletionPolicy handleDeletion;
    Zstring versioningFolder; //formatted directory name
    int versionCountLimit_;
};
std::vector<FolderPairSyncCfg> extractSyncCfg(const MainConfiguration& mainCfg);

class SynchronizeFolderPair;

//FFS core routine:
void synchronize(const TimeComp& timeStamp,
                 xmlAccess::OptionalDialogs& warnings,
                 bool verifyCopiedFiles,
                 bool copyLockedFiles,
                 bool copyFilePermissions,
                 bool transactionalFileCopy,
                 bool runWithBackgroundPriority,

                 const std::vector<FolderPairSyncCfg>& syncConfig, //CONTRACT: syncConfig and folderCmp correspond row-wise!
                 FolderComparison& folderCmp,                      //
                 ProcessCallback& callback);










// -----------  implementation ----------------
template <> inline
int SyncStatistics::getCreate<LEFT_SIDE>() const
{
    return createLeft;
}

template <> inline
int SyncStatistics::getCreate<RIGHT_SIDE>() const
{
    return createRight;
}

inline
int SyncStatistics::getCreate() const
{
    return getCreate<LEFT_SIDE>() + getCreate<RIGHT_SIDE>();
}

template <> inline
int SyncStatistics::getUpdate<LEFT_SIDE>() const
{
    return updateLeft;
}

template <> inline
int SyncStatistics::getUpdate<RIGHT_SIDE>() const
{
    return updateRight;
}

inline
int SyncStatistics::getUpdate() const
{
    return getUpdate<LEFT_SIDE>() + getUpdate<RIGHT_SIDE>();
}


template <> inline
int SyncStatistics::getDelete<LEFT_SIDE>() const
{
    return deleteLeft;
}

template <> inline
int SyncStatistics::getDelete<RIGHT_SIDE>() const
{
    return deleteRight;
}

inline
int SyncStatistics::getDelete() const
{
    return getDelete<LEFT_SIDE>() + getDelete<RIGHT_SIDE>();
}
}

#endif // SYNCHRONIZATION_H_INCLUDED
