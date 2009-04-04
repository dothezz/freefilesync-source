#ifndef SYNCHRONIZATION_H_INCLUDED
#define SYNCHRONIZATION_H_INCLUDED

#include "FreeFileSync.h"
#include "library/statusHandler.h"


namespace FreeFileSync
{
    void calcTotalBytesToSync(const FileCompareResult& fileCmpResult,
                              const SyncConfiguration& config,
                              int& objectsToCreate,
                              int& objectsToOverwrite,
                              int& objectsToDelete,
                              double& dataToProcess);

    bool synchronizationNeeded(const FileCompareResult& fileCmpResult, const SyncConfiguration& config);

    //class handling synchronization process
    class SyncProcess
    {
    public:
        SyncProcess(const bool useRecycler,
                    const bool copyFileSymLinks,
                    const bool traverseDirSymLinks,
                    bool& warningSignificantDifference,
                    StatusHandler* handler);

        void startSynchronizationProcess(FileCompareResult& grid, const SyncConfiguration& config)  throw(AbortThisProcess);

    private:
        bool synchronizeFile(const FileCompareLine& cmpLine, const SyncConfiguration& config);   //false if nothing had to be done
        bool synchronizeFolder(const FileCompareLine& cmpLine, const SyncConfiguration& config); //false if nothing had to be done

        void copyFileUpdating(const Zstring& source, const Zstring& target, const wxULongLong& sourceFileSize);

        const bool m_useRecycleBin;
        const bool m_copyFileSymLinks;
        const bool m_traverseDirSymLinks;
        bool& m_warningSignificantDifference;
        StatusHandler* statusUpdater;

        //preload status texts
        Zstring txtCopyingFile;
        Zstring txtOverwritingFile;
        Zstring txtCreatingFolder;
        Zstring txtDeletingFile;
        Zstring txtDeletingFolder;
    };
}

#endif // SYNCHRONIZATION_H_INCLUDED
