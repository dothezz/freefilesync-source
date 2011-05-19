// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) 2008-2011 ZenJu (zhnmju123 AT gmx.de)                    *
// **************************************************************************
//
#ifndef SYNCHRONIZATION_H_INCLUDED
#define SYNCHRONIZATION_H_INCLUDED

#include "file_hierarchy.h"
#include "library/process_xml.h"
#include "library/status_handler.h"


namespace zen
{

class SyncStatistics
{
public:
    SyncStatistics(const HierarchyObject&  hierObj);
    SyncStatistics(const FolderComparison& folderCmp);

    int getCreate() const;
    template <SelectedSide side> int getCreate() const;

    int getOverwrite() const;
    template <SelectedSide side> int getOverwrite() const;

    int getDelete() const;
    template <SelectedSide side> int getDelete() const;

    int getConflict() const;

    typedef std::vector<std::pair<Zstring, wxString> > ConflictTexts; // Pair(filename/conflict text)
    const ConflictTexts& getFirstConflicts() const; //get first few sync conflicts

    zen::UInt64 getDataToProcess() const;
    size_t getRowCount() const;

private:
    void init();

    void getNumbersRecursively(const HierarchyObject& hierObj);

    void getFileNumbers(const FileMapping& fileObj);
    void getLinkNumbers(const SymLinkMapping& linkObj);
    void getDirNumbers(const DirMapping& dirObj);

    int createLeft,    createRight;
    int overwriteLeft, overwriteRight;
    int deleteLeft,    deleteRight;
    int conflict;
    ConflictTexts firstConflicts; //save the first few conflict texts to display as a warning message
    zen::UInt64 dataToProcess;
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
                ProcessCallback& handler);

    //CONTRACT: syncConfig must have SAME SIZE folderCmp and correspond per row!
    void startSynchronizationProcess(const std::vector<FolderPairSyncCfg>& syncConfig, FolderComparison& folderCmp);

private:
    friend class SynchronizeFolderPair;

    const bool verifyCopiedFiles_;
    const bool copyLockedFiles_;
    const bool copyFilePermissions_;

    //warnings
    xmlAccess::OptionalDialogs& m_warnings;

    ProcessCallback& procCallback;
};


























// inline implementation
template <>
inline
int SyncStatistics::getCreate<LEFT_SIDE>() const
{
    return createLeft;
}

template <>
inline
int SyncStatistics::getCreate<RIGHT_SIDE>() const
{
    return createRight;
}

inline
int SyncStatistics::getCreate() const
{
    return getCreate<LEFT_SIDE>() + getCreate<RIGHT_SIDE>();
}

template <>
inline
int SyncStatistics::getOverwrite<LEFT_SIDE>() const
{
    return overwriteLeft;
}

template <>
inline
int SyncStatistics::getOverwrite<RIGHT_SIDE>() const
{
    return overwriteRight;
}

inline
int SyncStatistics::getOverwrite() const
{
    return getOverwrite<LEFT_SIDE>() + getOverwrite<RIGHT_SIDE>();
}


template <>
inline
int SyncStatistics::getDelete<LEFT_SIDE>() const
{
    return deleteLeft;
}

template <>
inline
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
