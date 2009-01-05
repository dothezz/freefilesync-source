#ifndef SYNCHRONIZATION_H_INCLUDED
#define SYNCHRONIZATION_H_INCLUDED

#include "FreeFileSync.h"


namespace FreeFileSync
{
    void calcTotalBytesToSync(int& objectsToCreate,
                              int& objectsToOverwrite,
                              int& objectsToDelete,
                              double& dataToProcess,
                              const FileCompareResult& fileCmpResult,
                              const SyncConfiguration& config);

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
        wxString optionalLineBreak; //optional line break for status messages (used by GUI mode only)
    };
}

#endif // SYNCHRONIZATION_H_INCLUDED
