#ifndef SYNCHRONIZATION_H_INCLUDED
#define SYNCHRONIZATION_H_INCLUDED

#include "fileHierarchy.h"
#include "library/processXml.h"
#include <memory>

#ifdef FFS_WIN
#include "shared/shadow.h"
#endif

class StatusHandler;


namespace FreeFileSync
{
    class SyncStatistics
    {
    public:
        SyncStatistics(const BaseDirMapping&   baseDir);
        SyncStatistics(const FolderComparison& folderCmp);

        int         getCreate(   bool inclLeft = true, bool inclRight = true) const;
        int         getOverwrite(bool inclLeft = true, bool inclRight = true) const;
        int         getDelete(   bool inclLeft = true, bool inclRight = true) const;
        int         getConflict() const;
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
        wxULongLong dataToProcess;
        int rowsTotal;
    };

    bool synchronizationNeeded(const FolderComparison& folderCmp);

    SyncOperation getSyncOperation(const CompareFilesResult cmpResult,
                                   const bool selectedForSynchronization,
                                   const SyncDirection syncDir); //evaluate comparison result and sync direction


    SyncOperation getSyncOperation(const FileSystemObject& fsObj); //convenience function


    struct FolderPairSyncCfg
    {
        FolderPairSyncCfg(const DeletionPolicy handleDel,
                          const wxString& custDelDir) :
                handleDeletion(handleDel),
                custDelFolder(custDelDir) {}

        DeletionPolicy handleDeletion;
        wxString custDelFolder;
    };
    std::vector<FolderPairSyncCfg> extractSyncCfg(const MainConfiguration& mainCfg);


    //class handling synchronization process
    class SyncProcess
    {
    public:
        SyncProcess(const bool copyFileSymLinks,
                    const bool traverseDirSymLinks,
                    xmlAccess::OptionalDialogs& warnings,
                    const bool verifyCopiedFiles,
                    StatusHandler* handler);

        //CONTRACT: syncConfig must have SAME SIZE folderCmp and correspond per row!
        void startSynchronizationProcess(const std::vector<FolderPairSyncCfg>& syncConfig, FolderComparison& folderCmp);

    private:
    template <bool deleteOnly>
    class SyncRecursively;

        struct DeletionHandling
        {
            DeletionHandling(const DeletionPolicy handleDel,
                             const wxString& custDelFolder);

            DeletionPolicy handleDeletion;
            Zstring currentDelFolder; //alternate deletion folder for current folder pair (with timestamp, ends with path separator)
            //preloaded status texts:
            const Zstring txtMoveFileUserDefined;
            const Zstring txtMoveFolderUserDefined;
        };

        void syncRecursively(HierarchyObject& hierObj);
        void synchronizeFile(FileMapping& fileObj, const DeletionHandling& delHandling) const;
        void synchronizeFolder(DirMapping& dirObj, const DeletionHandling& delHandling) const;

        template <FreeFileSync::SelectedSide side>
        void removeFile(const FileMapping& fileObj, const DeletionHandling& delHandling, bool showStatusUpdate) const;

        template <FreeFileSync::SelectedSide side>
        void removeFolder(const DirMapping& dirObj, const DeletionHandling& delHandling) const;

        void copyFileUpdating(const Zstring& source, const Zstring& target, const wxULongLong& sourceFileSize) const;
        void verifyFileCopy(const Zstring& source, const Zstring& target) const;

        const bool m_copyFileSymLinks;
        const bool m_traverseDirSymLinks;
        const bool m_verifyCopiedFiles;

        //warnings
        xmlAccess::OptionalDialogs& m_warnings;

#ifdef FFS_WIN
        //shadow copy buffer
        std::auto_ptr<ShadowCopy> shadowCopyHandler;
#endif

        StatusHandler* const statusUpdater;

        //preload status texts
        const Zstring txtCopyingFile;
        const Zstring txtOverwritingFile;
        const Zstring txtCreatingFolder;
        const Zstring txtDeletingFile;
        const Zstring txtDeletingFolder;
        const Zstring txtMoveToRecycler;
        const Zstring txtVerifying;
    };
}

#endif // SYNCHRONIZATION_H_INCLUDED
