#ifndef SYNCHRONIZATION_H_INCLUDED
#define SYNCHRONIZATION_H_INCLUDED

#include "structures.h"

class StatusHandler;


namespace FreeFileSync
{
    void calcTotalBytesToSync(const FolderComparison& folderCmp,
                              int& objectsToCreate,
                              int& objectsToOverwrite,
                              int& objectsToDelete,
                              int& conflicts,
                              wxULongLong& dataToProcess);

    bool synchronizationNeeded(const FolderComparison& folderCmp);

    void redetermineSyncDirection(const SyncConfiguration& config, FolderComparison& folderCmp);

    //class handling synchronization process
    class SyncProcess
    {
    public:
        SyncProcess(const bool useRecycler,
                    const bool copyFileSymLinks,
                    const bool traverseDirSymLinks,
                    bool& warningSignificantDifference,
                    bool& warningNotEnoughDiskSpace,
                    bool& warningUnresolvedConflict,
                    StatusHandler* handler);

        void startSynchronizationProcess(FolderComparison& folderCmp);

    private:
        bool synchronizeFile(const FileCompareLine& cmpLine, const FolderPair& folderPair);   //false if nothing was done
        bool synchronizeFolder(const FileCompareLine& cmpLine, const FolderPair& folderPair); //false if nothing was done

        void copyFileUpdating(const Zstring& source, const Zstring& target, const wxULongLong& sourceFileSize);

        const bool m_useRecycleBin;
        const bool m_copyFileSymLinks;
        const bool m_traverseDirSymLinks;

        //warnings
        bool& m_warningSignificantDifference;
        bool& m_warningNotEnoughDiskSpace;
        bool& m_warningUnresolvedConflict;

        StatusHandler* statusUpdater;

        //preload status texts
        const Zstring txtCopyingFile;
        const Zstring txtOverwritingFile;
        const Zstring txtCreatingFolder;
        const Zstring txtDeletingFile;
        const Zstring txtDeletingFolder;
    };
}

#endif // SYNCHRONIZATION_H_INCLUDED
