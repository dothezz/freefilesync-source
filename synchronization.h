#ifndef SYNCHRONIZATION_H_INCLUDED
#define SYNCHRONIZATION_H_INCLUDED

#include "structures.h"
#include "library/processXml.h"
#include <memory>

#ifdef FFS_WIN
#include "shared/shadow.h"
#endif

class StatusHandler;
class wxBitmap;


namespace FreeFileSync
{
    class SyncStatistics
    {
    public:
        SyncStatistics(const FileComparison&   fileCmp);
        SyncStatistics(const FolderComparison& folderCmp);

        int         getCreate(   bool inclLeft = true, bool inclRight = true) const;
        int         getOverwrite(bool inclLeft = true, bool inclRight = true) const;
        int         getDelete(   bool inclLeft = true, bool inclRight = true) const;
        int         getConflict() const;
        wxULongLong getDataToProcess() const;

    private:
        void init();
        void getNumbers(const FileCompareLine& fileCmpLine);

        int createLeft,    createRight;
        int overwriteLeft, overwriteRight;
        int deleteLeft,    deleteRight;
        int conflict;
        wxULongLong dataToProcess;
    };

    bool synchronizationNeeded(const FolderComparison& folderCmp);


    enum SyncOperation
    {
        SO_CREATE_NEW_LEFT,
        SO_CREATE_NEW_RIGHT,
        SO_DELETE_LEFT,
        SO_DELETE_RIGHT,
        SO_OVERWRITE_LEFT,
        SO_OVERWRITE_RIGHT,
        SO_DO_NOTHING,
        SO_UNRESOLVED_CONFLICT
    };
    SyncOperation getSyncOperation(const CompareFilesResult cmpResult,
                                   const bool selectedForSynchronization,
                                   const SyncDirection syncDir); //evaluate comparison result and sync direction

    SyncOperation getSyncOperation(const FileCompareLine& line); //convenience function


    //class handling synchronization process
    class SyncProcess
    {
    public:
        SyncProcess(const DeletionPolicy handleDeletion,
                    const wxString& custDelFolder,
                    const bool copyFileSymLinks,
                    const bool traverseDirSymLinks,
                    xmlAccess::WarningMessages& warnings,
                    StatusHandler* handler);

        void startSynchronizationProcess(FolderComparison& folderCmp);

    private:
        void synchronizeFile(const FileCompareLine& cmpLine, const FolderPair& folderPair, const Zstring& altDeletionDir);   //false if nothing was done
        void synchronizeFolder(const FileCompareLine& cmpLine, const FolderPair& folderPair, const Zstring& altDeletionDir); //false if nothing was done

        void removeFile(const FileDescrLine& fildesc, const Zstring& altDeletionDir);
        void removeFolder(const FileDescrLine& fildesc, const Zstring& altDeletionDir);

        void copyFileUpdating(const Zstring& source, const Zstring& target, const wxULongLong& sourceFileSize);

        const DeletionPolicy m_handleDeletion;
        const Zstring sessionDeletionDirectory; //ends with path separator

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
