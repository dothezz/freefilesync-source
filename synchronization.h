#ifndef SYNCHRONIZATION_H_INCLUDED
#define SYNCHRONIZATION_H_INCLUDED

#include "fileHierarchy.h"
#include "library/processXml.h"

class StatusHandler;


namespace FreeFileSync
{

class SyncStatistics
{
public:
    SyncStatistics(const HierarchyObject&  hierObj);
    SyncStatistics(const FolderComparison& folderCmp);

    int         getCreate(   bool inclLeft = true, bool inclRight = true) const;
    int         getOverwrite(bool inclLeft = true, bool inclRight = true) const;
    int         getDelete(   bool inclLeft = true, bool inclRight = true) const;
    int         getConflict() const;

    typedef std::vector<std::pair<Zstring, wxString> > ConflictTexts; // Pair(filename/conflict text)
    const ConflictTexts& getFirstConflicts() const; //get first few sync conflicts

    wxULongLong getDataToProcess() const;
    int getRowCount() const;

private:
    void init();

    void getNumbersRecursively(const HierarchyObject& hierObj);

    void getFileNumbers(const FileMapping& fileObj);
    void getDirNumbers(const DirMapping& dirObj);

    int createLeft,    createRight;
    int overwriteLeft, overwriteRight;
    int deleteLeft,    deleteRight;
    int conflict;
    ConflictTexts firstConflicts; //save the first few conflict texts to display as a warning message
    wxULongLong dataToProcess;
    int rowsTotal;
};

bool synchronizationNeeded(const FolderComparison& folderCmp);

class SyncRecursively;

struct FolderPairSyncCfg
{
    FolderPairSyncCfg(bool automaticMode,
                      const DeletionPolicy handleDel,
                      const Zstring& custDelDir) :
        inAutomaticMode(automaticMode),
        handleDeletion(handleDel),
        custDelFolder(custDelDir) {}

    bool inAutomaticMode; //update database if in automatic mode
    DeletionPolicy handleDeletion;
    Zstring custDelFolder;
};
std::vector<FolderPairSyncCfg> extractSyncCfg(const MainConfiguration& mainCfg);


//class handling synchronization process
class SyncProcess
{
public:
    SyncProcess(bool copyFileSymLinks,
                bool traverseDirSymLinks,
                xmlAccess::OptionalDialogs& warnings,
                bool verifyCopiedFiles,
                bool copyLockedFiles,
                StatusHandler& handler);

    //CONTRACT: syncConfig must have SAME SIZE folderCmp and correspond per row!
    void startSynchronizationProcess(const std::vector<FolderPairSyncCfg>& syncConfig, FolderComparison& folderCmp);

private:
    friend class SyncRecursively;

    const bool m_copyFileSymLinks;
    const bool m_traverseDirSymLinks;
    const bool m_verifyCopiedFiles;
    const bool copyLockedFiles_;

    //warnings
    xmlAccess::OptionalDialogs& m_warnings;

    StatusHandler& statusUpdater;
};

}

#endif // SYNCHRONIZATION_H_INCLUDED
