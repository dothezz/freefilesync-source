#ifndef SYNCHRONIZATION_H_INCLUDED
#define SYNCHRONIZATION_H_INCLUDED

#include "structures.h"
#include "library/processXml.h"
#include <memory>

#ifdef FFS_WIN
#include "library/shadow.h"
#endif

class StatusHandler;
class wxBitmap;


namespace FreeFileSync
{
    void calcTotalBytesToSync(const FolderComparison& folderCmp,
                              int& objectsToCreate,
                              int& objectsToOverwrite,
                              int& objectsToDelete,
                              int& conflicts,
                              wxULongLong& dataToProcess);

    bool synchronizationNeeded(const FolderComparison& folderCmp);


    enum SyncOperation
    {
        SO_CREATE_NEW_LEFT,
        SO_CREATE_NEW_RIGHT,
        SO_DELETE_LEFT,
        SO_DELETE_RIGHT,
        SO_OVERWRITE_RIGHT,
        SO_OVERWRITE_LEFT,
        SO_DO_NOTHING,
        SO_UNRESOLVED_CONFLICT
    };
    SyncOperation getSyncOperation(const CompareFilesResult cmpResult,
                                   const bool selectedForSynchronization,
                                   const SyncDirection syncDir); //evaluate comparison result and sync direction

    const wxBitmap& getSyncOpImage(const CompareFilesResult cmpResult,
                                   const bool selectedForSynchronization,
                                   const SyncDirection syncDir);

    //class handling synchronization process
    class SyncProcess
    {
    public:
        SyncProcess(const bool useRecycler,
                    const bool copyFileSymLinks,
                    const bool traverseDirSymLinks,
                    xmlAccess::WarningMessages& warnings,
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
        xmlAccess::WarningMessages& m_warnings;

#ifdef FFS_WIN
        //shadow copy buffer
        std::auto_ptr<ShadowCopy> shadowCopyHandler;
#endif

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
