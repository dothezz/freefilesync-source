// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) ZenJu (zhnmju123 AT gmx DOT de) - All Rights Reserved    *
// **************************************************************************

#ifndef SYNCHRONIZATION_H_INCLUDED
#define SYNCHRONIZATION_H_INCLUDED

#include "file_hierarchy.h"
#include "lib/process_xml.h"
#include "process_callback.h"
#include <zen/process_status.h>


namespace zen
{

class SyncStatistics //this class counts logical operations (create, update, delete) + bytes
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

    int getConflict() const { return conflict; }

    typedef std::vector<std::pair<Zstring, std::wstring> > ConflictTexts; // Pair(filename/conflict text)
    const ConflictTexts& getFirstConflicts() const { return firstConflicts; }

    zen::Int64 getDataToProcess() const { return dataToProcess; }
    size_t     getRowCount()      const { return rowsTotal; }

private:
    void init();

    void recurse(const HierarchyObject& hierObj);

    void getFileNumbers(const FileMapping& fileObj);
    void getLinkNumbers(const SymLinkMapping& linkObj);
    void getDirNumbers(const DirMapping& dirObj);

    int createLeft, createRight;
    int updateLeft, updateRight;
    int deleteLeft, deleteRight;
    int conflict;
    ConflictTexts firstConflicts; //save the first few conflict texts to display as a warning message
    zen::Int64 dataToProcess;
    size_t rowsTotal;
};


class SynchronizeFolderPair;

struct FolderPairSyncCfg
{
    FolderPairSyncCfg(bool automaticMode,
                      const DeletionPolicy handleDel,
                      const Zstring& custDelDir);

    bool inAutomaticMode; //update database if in automatic mode
    DeletionPolicy handleDeletion;
    Zstring custDelFolder; //formatted(!) directory name
};
std::vector<FolderPairSyncCfg> extractSyncCfg(const MainConfiguration& mainCfg);


//class handling synchronization process
class SyncProcess
{
public:
    SyncProcess(xmlAccess::OptionalDialogs& warnings,
                bool verifyCopiedFiles,
                bool copyLockedFiles,
                bool copyFilePermissions,
                bool transactionalFileCopy,
                bool runWithBackgroundPriority,
                ProcessCallback& handler);

    //CONTRACT: syncConfig must have SAME SIZE folderCmp and correspond row-wise!
    void startSynchronizationProcess(const std::vector<FolderPairSyncCfg>& syncConfig, FolderComparison& folderCmp);

private:
    SyncProcess(const SyncProcess&);
    SyncProcess& operator=(const SyncProcess&);

    friend class SynchronizeFolderPair;

    const bool verifyCopiedFiles_;
    const bool copyLockedFiles_;
    const bool copyFilePermissions_;
    const bool transactionalFileCopy_;

    //warnings
    xmlAccess::OptionalDialogs& m_warnings;
    ProcessCallback& procCallback;

    std::unique_ptr<ScheduleForBackgroundProcessing> procBackground;
};


























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
