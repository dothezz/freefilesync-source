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
        SyncProcess(bool useRecycler, bool lineBreakOnMessages, StatusHandler* handler);

        void startSynchronizationProcess(FileCompareResult& grid, const SyncConfiguration& config)  throw(AbortThisProcess);

    private:
        bool synchronizeFile(const FileCompareLine& cmpLine, const SyncConfiguration& config);   //false if nothing had to be done
        bool synchronizeFolder(const FileCompareLine& cmpLine, const SyncConfiguration& config); //false if nothing had to be done

        const bool useRecycleBin;
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
