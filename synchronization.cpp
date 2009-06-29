#include "synchronization.h"
#include <wx/intl.h>
#include <wx/msgdlg.h>
#include <wx/log.h>
#include "library/resources.h"
#include "algorithm.h"
#include "library/globalFunctions.h"
#include "library/statusHandler.h"
#include "library/fileHandling.h"
#include <utility>

using namespace FreeFileSync;


inline
bool getBytesToTransfer(const FileCompareLine& fileCmpLine,
                        int& objectsToCreate,
                        int& objectsToOverwrite,
                        int& objectsToDelete,
                        int& conflicts,
                        wxULongLong& dataToProcess)
{   //return false if nothing has to be done

    objectsToCreate    = 0;  //always initialize variables
    objectsToOverwrite = 0;
    objectsToDelete    = 0;
    conflicts          = 0;
    dataToProcess      = 0;

    switch (FreeFileSync::getSyncOperation(fileCmpLine.cmpResult, fileCmpLine.selectedForSynchronization, fileCmpLine.direction)) //evaluate comparison result and sync direction
    {
    case SO_CREATE_NEW_LEFT:
        dataToProcess   = fileCmpLine.fileDescrRight.fileSize;
        objectsToCreate = 1;
        return true;

    case SO_CREATE_NEW_RIGHT:
        dataToProcess   = fileCmpLine.fileDescrLeft.fileSize;
        objectsToCreate = 1;
        return true;

    case SO_DELETE_LEFT:
    case SO_DELETE_RIGHT:
        dataToProcess   = 0;
        objectsToDelete = 1;
        return true;

    case SO_OVERWRITE_RIGHT:
        dataToProcess      = fileCmpLine.fileDescrLeft.fileSize;
        objectsToOverwrite = 1;
        return true;

    case SO_OVERWRITE_LEFT:
        dataToProcess      = fileCmpLine.fileDescrRight.fileSize;
        objectsToOverwrite = 1;
        return true;

    case SO_DO_NOTHING:
        return false;

    case SO_UNRESOLVED_CONFLICT:
        conflicts = 1;
        return true;
    }

    return false; //dummy
}


//runs at folder pair level
void calcBytesToSync(const FileComparison& fileCmp,
                     int& objectsToCreate,
                     int& objectsToOverwrite,
                     int& objectsToDelete,
                     int& conflicts,
                     wxULongLong& dataToProcess)
{
    objectsToCreate    = 0;
    objectsToOverwrite = 0;
    objectsToDelete    = 0;
    conflicts          = 0;
    dataToProcess      = 0;

    int toCreate     = 0;
    int toOverwrite  = 0;
    int toDelete     = 0;
    int newConflicts = 0;
    wxULongLong data;

    for (FileComparison::const_iterator i = fileCmp.begin(); i != fileCmp.end(); ++i)
    {   //only sum up sizes of files AND directories
        if (getBytesToTransfer(*i, toCreate, toOverwrite, toDelete, newConflicts, data))
        {
            objectsToCreate    += toCreate;
            objectsToOverwrite += toOverwrite;
            objectsToDelete    += toDelete;
            conflicts          += newConflicts;
            dataToProcess      += data;
        }
    }
}


//aggregate over all folder pairs
void FreeFileSync::calcTotalBytesToSync(const FolderComparison& folderCmp,
                                        int& objectsToCreate,
                                        int& objectsToOverwrite,
                                        int& objectsToDelete,
                                        int& conflicts,
                                        wxULongLong& dataToProcess)
{
    objectsToCreate    = 0;
    objectsToOverwrite = 0;
    objectsToDelete    = 0;
    conflicts          = 0;
    dataToProcess      = 0;

    for (FolderComparison::const_iterator j = folderCmp.begin(); j != folderCmp.end(); ++j)
    {
        const FileComparison& fileCmp = j->fileCmp;

        int toCreate    = 0;
        int toOverwrite = 0;
        int toDelete    = 0;
        int newConflics = 0;
        wxULongLong data;

        calcBytesToSync(fileCmp, toCreate, toOverwrite, toDelete, newConflics, data);

        objectsToCreate    += toCreate;
        objectsToOverwrite += toOverwrite;
        objectsToDelete    += toDelete;
        conflicts          += newConflics;
        dataToProcess      += data;
    }
}


template <bool recyclerUsed>
std::pair<wxLongLong, wxLongLong> spaceNeededSub(const FileComparison& fileCmp)
{
    wxLongLong spaceNeededLeft;
    wxLongLong spaceNeededRight;

    for (FileComparison::const_iterator i = fileCmp.begin(); i != fileCmp.end(); ++i)
    {
        if (i->selectedForSynchronization) //do not add filtered entries
        {
            //get data to process
            switch (i->direction)
            {
            case SYNC_DIR_LEFT:   //copy from right to left
                if (!recyclerUsed)
                    spaceNeededLeft -= globalFunctions::convertToSigned(i->fileDescrLeft.fileSize);
                spaceNeededLeft += globalFunctions::convertToSigned(i->fileDescrRight.fileSize);
                break;

            case SYNC_DIR_RIGHT:  //copy from left to right
                if (!recyclerUsed)
                    spaceNeededRight -= globalFunctions::convertToSigned(i->fileDescrRight.fileSize);
                spaceNeededRight += globalFunctions::convertToSigned(i->fileDescrLeft.fileSize);
                break;

            case SYNC_DIR_NONE:
            case SYNC_UNRESOLVED_CONFLICT:
                break;
            }
        }
    }

    return std::pair<wxLongLong, wxLongLong>(spaceNeededLeft, spaceNeededRight);
}


std::pair<wxLongLong, wxLongLong> freeDiskSpaceNeeded(const FileComparison& fileCmp, const bool recyclerUsed)
{
    if (recyclerUsed)
        return spaceNeededSub<true>(fileCmp);
    else
        return spaceNeededSub<false>(fileCmp);
}


bool unresolvedConflictsExisting(const FolderComparison& folderCmp)
{
    for (FolderComparison::const_iterator j = folderCmp.begin(); j != folderCmp.end(); ++j)
    {
        const FileComparison& fileCmp = j->fileCmp;
        for (FileComparison::const_iterator i = fileCmp.begin(); i != fileCmp.end(); ++i)
            if (i->direction == SYNC_UNRESOLVED_CONFLICT)
                return true;
    }
    return false;
}


bool FreeFileSync::synchronizationNeeded(const FolderComparison& folderCmp)
{
    int objectsToCreate     = 0;
    int objectsToOverwrite  = 0;
    int objectsToDelete     = 0;
    int conflicts           = 0;
    wxULongLong dataToProcess;

    FreeFileSync::calcTotalBytesToSync(folderCmp,
                                       objectsToCreate,
                                       objectsToOverwrite,
                                       objectsToDelete,
                                       conflicts,
                                       dataToProcess);

    return objectsToCreate + objectsToOverwrite + objectsToDelete + conflicts != 0;
}


//test if more then 50% of files will be deleted/overwritten
bool significantDifferenceDetected(const FileComparison& fileCmp)
{
    int objectsToCreate     = 0;
    int objectsToOverwrite  = 0;
    int objectsToDelete     = 0;
    int conflicts           = 0;
    wxULongLong dataToProcess;

    calcBytesToSync(fileCmp,
                    objectsToCreate,
                    objectsToOverwrite,
                    objectsToDelete,
                    conflicts,
                    dataToProcess);

    const int changedFiles = objectsToCreate + objectsToOverwrite + objectsToDelete + conflicts; //include objectsToCreate also!

    return changedFiles >= 10 && changedFiles > 0.5 * fileCmp.size();
}


SyncOperation FreeFileSync::getSyncOperation( //evaluate comparison result and sync direction
    const CompareFilesResult cmpResult,
    const bool selectedForSynchronization,
    const SyncDirection syncDir)
{
    if (!selectedForSynchronization) return SO_DO_NOTHING;

    switch (cmpResult)
    {
    case FILE_LEFT_SIDE_ONLY:
        switch (syncDir)
        {
        case SYNC_DIR_LEFT:
            return SO_DELETE_LEFT; //delete files on left
        case SYNC_DIR_RIGHT:
            return SO_CREATE_NEW_RIGHT; //copy files to right
        case SYNC_DIR_NONE:
            return SO_DO_NOTHING;
        case SYNC_UNRESOLVED_CONFLICT:
            assert(false);
        }
        break;

    case FILE_RIGHT_SIDE_ONLY:
        switch (syncDir)
        {
        case SYNC_DIR_LEFT:
            return SO_CREATE_NEW_LEFT; //copy files to left
        case SYNC_DIR_RIGHT:
            return SO_DELETE_RIGHT; //delete files on right
        case SYNC_DIR_NONE:
            return SO_DO_NOTHING;
        case SYNC_UNRESOLVED_CONFLICT:
            assert(false);
        }
        break;

    case FILE_LEFT_NEWER:
    case FILE_RIGHT_NEWER:
    case FILE_DIFFERENT:
    case FILE_CONFLICT:
        switch (syncDir)
        {
        case SYNC_DIR_LEFT:
            return SO_OVERWRITE_LEFT; //copy from right to left
        case SYNC_DIR_RIGHT:
            return SO_OVERWRITE_RIGHT; //copy from left to right
        case SYNC_DIR_NONE:
            return SO_DO_NOTHING;
        case SYNC_UNRESOLVED_CONFLICT:
            return SO_UNRESOLVED_CONFLICT;
        }
        break;

    case FILE_EQUAL:
        assert(syncDir == SYNC_DIR_NONE);
        return SO_DO_NOTHING;
    }

    return SO_DO_NOTHING; //dummy
}


const wxBitmap& FreeFileSync::getSyncOpImage(const CompareFilesResult cmpResult,
        const bool selectedForSynchronization,
        const SyncDirection syncDir)
{
    switch (getSyncOperation(cmpResult, selectedForSynchronization, syncDir)) //evaluate comparison result and sync direction
    {
    case SO_CREATE_NEW_LEFT:
        return *GlobalResources::getInstance().bitmapCreateLeftSmall;
    case SO_CREATE_NEW_RIGHT:
        return *GlobalResources::getInstance().bitmapCreateRightSmall;
    case SO_DELETE_LEFT:
        return *GlobalResources::getInstance().bitmapDeleteLeftSmall;
    case SO_DELETE_RIGHT:
        return *GlobalResources::getInstance().bitmapDeleteRightSmall;
    case SO_OVERWRITE_RIGHT:
        return *GlobalResources::getInstance().bitmapSyncDirRightSmall;
    case SO_OVERWRITE_LEFT:
        return *GlobalResources::getInstance().bitmapSyncDirLeftSmall;
    case SO_DO_NOTHING:
        return *GlobalResources::getInstance().bitmapSyncDirNoneSmall;
    case SO_UNRESOLVED_CONFLICT:
        return *GlobalResources::getInstance().bitmapConflictSmall;
    }

    return wxNullBitmap; //dummy
}


SyncProcess::SyncProcess(const bool useRecycler,
                         const bool copyFileSymLinks,
                         const bool traverseDirSymLinks,
                         xmlAccess::WarningMessages& warnings,
                         StatusHandler* handler) :
        m_useRecycleBin(useRecycler),
        m_copyFileSymLinks(copyFileSymLinks),
        m_traverseDirSymLinks(traverseDirSymLinks),
        m_warnings(warnings),
#ifdef FFS_WIN
        shadowCopyHandler(new ShadowCopy),
#endif
        statusUpdater(handler),
        txtCopyingFile(Zstring(_("Copying file %x to %y")).Replace(wxT("%x"), wxT("\"%x\""), false).Replace(wxT("%y"), wxT("\n\"%y\""), false)),
        txtOverwritingFile(Zstring(_("Copying file %x to %y overwriting target")).Replace(wxT("%x"), wxT("\"%x\""), false).Replace(wxT("%y"), wxT("\n\"%y\""), false)),
        txtCreatingFolder(Zstring(_("Creating folder %x")).Replace( wxT("%x"), wxT("\n\"%x\""), false)),
        txtDeletingFile(useRecycler ? Zstring(_("Moving %x to Recycle Bin")).Replace(wxT("%x"), wxT("\"%x\""), false) :
                        Zstring(_("Deleting file %x")).Replace(wxT("%x"), wxT("\n\"%x\""), false)),
        txtDeletingFolder(useRecycler ? Zstring(_("Moving %x to Recycle Bin")).Replace( wxT("%x"), wxT("\"%x\""), false) :
                          Zstring(_("Deleting folder %x")).Replace( wxT("%x"), wxT("\n\"%x\""), false)) {}


inline
bool SyncProcess::synchronizeFile(const FileCompareLine& cmpLine, const FolderPair& folderPair)
{   //return false if nothing had to be done

    Zstring statusText;
    Zstring target;

    switch (getSyncOperation(cmpLine.cmpResult, cmpLine.selectedForSynchronization, cmpLine.direction)) //evaluate comparison result and sync direction
    {
    case SO_CREATE_NEW_LEFT:
        target = folderPair.leftDirectory + cmpLine.fileDescrRight.relativeName.c_str();

        statusText = txtCopyingFile;
        statusText.Replace(wxT("%x"), cmpLine.fileDescrRight.fullName.AfterLast(FreeFileSync::FILE_NAME_SEPARATOR), false);
        statusText.Replace(wxT("%y"), target.BeforeLast(FreeFileSync::FILE_NAME_SEPARATOR), false);
        statusUpdater->updateStatusText(statusText);

        copyFileUpdating(cmpLine.fileDescrRight.fullName, target, cmpLine.fileDescrRight.fileSize);
        return true;

    case SO_CREATE_NEW_RIGHT:
        target = folderPair.rightDirectory + cmpLine.fileDescrLeft.relativeName.c_str();

        statusText = txtCopyingFile;
        statusText.Replace(wxT("%x"), cmpLine.fileDescrLeft.fullName.AfterLast(FreeFileSync::FILE_NAME_SEPARATOR), false);
        statusText.Replace(wxT("%y"), target.BeforeLast(FreeFileSync::FILE_NAME_SEPARATOR), false);
        statusUpdater->updateStatusText(statusText);

        copyFileUpdating(cmpLine.fileDescrLeft.fullName, target, cmpLine.fileDescrLeft.fileSize);
        return true;

    case SO_DELETE_LEFT:
        statusText = txtDeletingFile;
        statusText.Replace(wxT("%x"), cmpLine.fileDescrLeft.fullName, false);
        statusUpdater->updateStatusText(statusText);

        removeFile(cmpLine.fileDescrLeft.fullName, m_useRecycleBin);
        return true;

    case SO_DELETE_RIGHT:
        statusText = txtDeletingFile;
        statusText.Replace(wxT("%x"), cmpLine.fileDescrRight.fullName, false);
        statusUpdater->updateStatusText(statusText);

        removeFile(cmpLine.fileDescrRight.fullName, m_useRecycleBin);
        return true;

    case SO_OVERWRITE_RIGHT:
        statusText = txtOverwritingFile;
        statusText.Replace(wxT("%x"), cmpLine.fileDescrLeft.fullName.AfterLast(FreeFileSync::FILE_NAME_SEPARATOR), false);
        statusText.Replace(wxT("%y"), cmpLine.fileDescrRight.fullName.BeforeLast(FreeFileSync::FILE_NAME_SEPARATOR), false);
        statusUpdater->updateStatusText(statusText);

        removeFile(cmpLine.fileDescrRight.fullName, m_useRecycleBin);  //only used if switch activated by user, else file is simply deleted
        copyFileUpdating(cmpLine.fileDescrLeft.fullName, cmpLine.fileDescrRight.fullName, cmpLine.fileDescrLeft.fileSize);
        return true;

    case SO_OVERWRITE_LEFT:
        statusText = txtOverwritingFile;
        statusText.Replace(wxT("%x"), cmpLine.fileDescrRight.fullName.AfterLast(FreeFileSync::FILE_NAME_SEPARATOR), false);
        statusText.Replace(wxT("%y"), cmpLine.fileDescrLeft.fullName.BeforeLast(FreeFileSync::FILE_NAME_SEPARATOR), false);
        statusUpdater->updateStatusText(statusText);

        removeFile(cmpLine.fileDescrLeft.fullName, m_useRecycleBin);  //only used if switch activated by user, else file is simply deleted
        copyFileUpdating(cmpLine.fileDescrRight.fullName, cmpLine.fileDescrLeft.fullName, cmpLine.fileDescrRight.fileSize);
        return true;

    case SO_DO_NOTHING:
    case SO_UNRESOLVED_CONFLICT:
        return false;
    }

    return false; //dummy
}


inline
bool SyncProcess::synchronizeFolder(const FileCompareLine& cmpLine, const FolderPair& folderPair)
{   //return false if nothing had to be done

    Zstring statusText;
    Zstring target;

    //synchronize folders:
    switch (getSyncOperation(cmpLine.cmpResult, cmpLine.selectedForSynchronization, cmpLine.direction)) //evaluate comparison result and sync direction
    {
    case SO_CREATE_NEW_LEFT:
        target = folderPair.leftDirectory + cmpLine.fileDescrRight.relativeName.c_str();

        statusText = txtCreatingFolder;
        statusText.Replace(wxT("%x"), target, false);
        statusUpdater->updateStatusText(statusText);

        //some check to catch the error that directory on source has been deleted externally after "compare"...
        if (!wxDirExists(cmpLine.fileDescrRight.fullName))
            throw FileError(Zstring(_("Error: Source directory does not exist anymore:")) + wxT("\n\"") + cmpLine.fileDescrRight.fullName + wxT("\""));
        createDirectory(target, cmpLine.fileDescrRight.fullName, !m_traverseDirSymLinks); //traverse symlinks <=> !copy symlinks
        return true;

    case SO_CREATE_NEW_RIGHT:
        target = folderPair.rightDirectory + cmpLine.fileDescrLeft.relativeName.c_str();

        statusText = txtCreatingFolder;
        statusText.Replace(wxT("%x"), target, false);
        statusUpdater->updateStatusText(statusText);

        //some check to catch the error that directory on source has been deleted externally after "compare"...
        if (!wxDirExists(cmpLine.fileDescrLeft.fullName))
            throw FileError(Zstring(_("Error: Source directory does not exist anymore:")) + wxT("\n\"") + cmpLine.fileDescrLeft.fullName + wxT("\""));
        createDirectory(target, cmpLine.fileDescrLeft.fullName, !m_traverseDirSymLinks); //traverse symlinks <=> !copy symlinks
        return true;

    case SO_DELETE_LEFT:
        statusText = txtDeletingFolder;
        statusText.Replace(wxT("%x"), cmpLine.fileDescrLeft.fullName, false);
        statusUpdater->updateStatusText(statusText);

        removeDirectory(cmpLine.fileDescrLeft.fullName, m_useRecycleBin);
        return true;

    case SO_DELETE_RIGHT:
        statusText = txtDeletingFolder;
        statusText.Replace(wxT("%x"), cmpLine.fileDescrRight.fullName, false);
        statusUpdater->updateStatusText(statusText);

        removeDirectory(cmpLine.fileDescrRight.fullName, m_useRecycleBin);
        return true;

    case SO_OVERWRITE_RIGHT:
    case SO_OVERWRITE_LEFT:
    case SO_UNRESOLVED_CONFLICT:
        assert(false);
    case SO_DO_NOTHING:
        return false;
    }

    return false; //dummy
}


inline
bool deletionImminent(const FileCompareLine& line)
{   //test if current sync-line will result in deletion of files    -> used to avoid disc space bottlenecks
    if (    (line.cmpResult == FILE_LEFT_SIDE_ONLY && line.direction == SYNC_DIR_LEFT) ||
            (line.cmpResult == FILE_RIGHT_SIDE_ONLY && line.direction == SYNC_DIR_RIGHT))
        return true;
    else
        return false;
}


class RemoveAtExit //this class ensures, that the result of the method below is ALWAYS written on exit, even if exceptions are thrown!
{
public:
    RemoveAtExit(FileComparison& fileCmp) :
            gridToWrite(fileCmp) {}

    ~RemoveAtExit()
    {
        globalFunctions::removeRowsFromVector(rowsToDelete, gridToWrite);
    }

    void markRow(int nr)
    {
        rowsToDelete.insert(nr);
    }

private:
    FileComparison& gridToWrite;
    std::set<int> rowsToDelete;
};


//synchronize and returns all rows that have not been synced
void SyncProcess::startSynchronizationProcess(FolderComparison& folderCmp)
{
#ifndef __WXDEBUG__
    wxLogNull noWxLogs; //prevent wxWidgets logging
#endif

    //inform about the total amount of data that will be processed from now on
    int objectsToCreate      = 0;
    int objectsToOverwrite   = 0;
    int objectsToDelete      = 0;
    int conflicts            = 0;
    wxULongLong dataToProcess;

    FreeFileSync::calcTotalBytesToSync(folderCmp,
                                       objectsToCreate,
                                       objectsToOverwrite,
                                       objectsToDelete,
                                       conflicts,
                                       dataToProcess);

    statusUpdater->initNewProcess(objectsToCreate + objectsToOverwrite + objectsToDelete, //keep at beginning so that all gui elements are initialized properly
                                  globalFunctions::convertToSigned(dataToProcess),
                                  StatusHandler::PROCESS_SYNCHRONIZING);

    //-------------------some basic checks:------------------------------------------

    //test existence of Recycle Bin
    if (m_useRecycleBin && !FreeFileSync::recycleBinExists())
    {
        statusUpdater->reportFatalError(_("Unable to initialize Recycle Bin!"));
        return; //should be obsolete!
    }

    for (FolderComparison::const_iterator j = folderCmp.begin(); j != folderCmp.end(); ++j)
    {
        const FileComparison& fileCmp = j->fileCmp;

        //check if more than 50% of total number of files/dirs are to be created/overwritten/deleted
        if (m_warnings.warningSignificantDifference) //test if check should be executed
        {
            if (significantDifferenceDetected(fileCmp))
            {
                bool dontShowAgain = false;
                statusUpdater->reportWarning(Zstring(_("Significant difference detected:")) + wxT("\n") +
                                             j->syncPair.leftDirectory + wxT(" <-> ") + j->syncPair.rightDirectory + wxT("\n\n") +
                                             _("More than 50% of the total number of files will be copied or deleted!"),
                                             dontShowAgain);
                m_warnings.warningSignificantDifference = !dontShowAgain;
            }
        }

        //check for sufficient free diskspace in left directory
        if (m_warnings.warningNotEnoughDiskSpace)
        {
            const std::pair<wxLongLong, wxLongLong> spaceNeeded = freeDiskSpaceNeeded(fileCmp, m_useRecycleBin);

            wxLongLong freeDiskSpaceLeft;
            if (wxGetDiskSpace(j->syncPair.leftDirectory.c_str(), NULL, &freeDiskSpaceLeft))
            {
                if (freeDiskSpaceLeft < spaceNeeded.first)
                {
                    bool dontShowAgain = false;
                    statusUpdater->reportWarning(Zstring(_("Not enough free disk space available in:")) + wxT("\n") +
                                                 j->syncPair.leftDirectory + wxT("\n\n") +
                                                 _("Total required free disk space:") + wxT(" ") + formatFilesizeToShortString(spaceNeeded.first).c_str(),
                                                 dontShowAgain);
                    m_warnings.warningNotEnoughDiskSpace = !dontShowAgain;
                }
            }

            //check for sufficient free diskspace in right directory
            if (m_warnings.warningNotEnoughDiskSpace)
            {
                wxLongLong freeDiskSpaceRight;
                if (wxGetDiskSpace(j->syncPair.rightDirectory.c_str(), NULL, &freeDiskSpaceRight))
                {
                    if (freeDiskSpaceRight < spaceNeeded.second)
                    {
                        bool dontShowAgain = false;
                        statusUpdater->reportWarning(Zstring(_("Not enough free disk space available in:")) + wxT("\n") +
                                                     j->syncPair.rightDirectory + wxT("\n\n") +
                                                     _("Total required free disk space:") + wxT(" ") + formatFilesizeToShortString(spaceNeeded.second).c_str(),
                                                     dontShowAgain);
                        m_warnings.warningNotEnoughDiskSpace = !dontShowAgain;
                    }
                }
            }
        }
    }

    //check if unresolved conflicts exist
    if (m_warnings.warningUnresolvedConflicts) //test if check should be executed
    {
        if (conflicts > 0)
        {
            bool dontShowAgain = false;
            statusUpdater->reportWarning(_("Unresolved conflicts existing! \n\nYou can ignore conflicts and continue synchronization."),
                                         dontShowAgain);
            m_warnings.warningUnresolvedConflicts = !dontShowAgain;
        }
    }

    //-------------------end of basic checks------------------------------------------


    try
    {
        //loop over folder pairs
        for (FolderComparison::iterator j = folderCmp.begin(); j != folderCmp.end(); ++j)
        {
            FileComparison& fileCmp = j->fileCmp;

            //ensure that folderCmp is always written to, even if method is left via exceptions
            RemoveAtExit removeRowsAtExit(fileCmp); //note: running at individual folder pair level!

            const FolderPair& currentPair = j->syncPair;

            // it should never happen, that a directory on left side has same name as file on right side. startCompareProcess() should take care of this
            // and split into two "exists on one side only" cases
            // Note: this case is not handled by this tool as this is considered to be a bug and must be solved by the user

            //synchronize folders first; advantage: in case of deletions the whole folder is moved to recycle bin instead of single files
            for (FileComparison::const_iterator i = fileCmp.begin(); i != fileCmp.end(); ++i)
            {
                if (    i->fileDescrLeft.objType == FileDescrLine::TYPE_DIRECTORY ||
                        i->fileDescrRight.objType == FileDescrLine::TYPE_DIRECTORY)
                {
                    while (true)
                    {   //trigger display refresh
                        statusUpdater->requestUiRefresh();

                        try
                        {
                            if (synchronizeFolder(*i, currentPair))
                                //progress indicator update
                                //indicator is updated only if directory is sync'ed correctly (and if some work was done)!
                                statusUpdater->updateProcessedData(1, 0);  //each call represents one processed file/directory

                            removeRowsAtExit.markRow(i - fileCmp.begin());
                            break;
                        }
                        catch (FileError& error)
                        {
                            ErrorHandler::Response rv = statusUpdater->reportError(error.show());

                            if ( rv == ErrorHandler::IGNORE_ERROR)
                                break;
                            else if (rv == ErrorHandler::RETRY)
                                ;  //continue with loop
                            else
                                assert (false);
                        }
                    }
                }
            }

            //PERF_START;

            //synchronize files:
            for (int k = 0; k < 2; ++k) //loop over all files twice; reason: first delete, then copy (no sorting necessary: performance)
            {
                bool deleteLoop = true;
                deleteLoop = k == 0;   //-> first loop: delete files, second loop: copy files

                for (FileComparison::const_iterator i = fileCmp.begin(); i != fileCmp.end(); ++i)
                {
                    //skip processing of unresolved errors (do not remove row from GUI grid)
                    if (i->direction == SYNC_UNRESOLVED_CONFLICT && i->selectedForSynchronization)
                        continue;

                    if (    i->fileDescrLeft.objType == FileDescrLine::TYPE_FILE ||
                            i->fileDescrRight.objType == FileDescrLine::TYPE_FILE)
                    {
                        if (    (deleteLoop && deletionImminent(*i)) ||
                                (!deleteLoop && !deletionImminent(*i)))
                        {
                            while (true)
                            {    //trigger display refresh
                                statusUpdater->requestUiRefresh();

                                try
                                {
                                    if (synchronizeFile(*i, currentPair))
                                    {
                                        //progress indicator update
                                        //indicator is updated only if file is sync'ed correctly (and if some sync was done)!
                                        int objectsToCreate    = 0;
                                        int objectsToOverwrite = 0;
                                        int objectsToDelete    = 0;
                                        int conflictDummy      = 0;
                                        wxULongLong dataToProcess;

                                        if (getBytesToTransfer(*i,
                                                               objectsToCreate,
                                                               objectsToOverwrite,
                                                               objectsToDelete,
                                                               conflictDummy,
                                                               dataToProcess))  //update status if some work was done (answer is always "yes" in this context)
                                            statusUpdater->updateProcessedData(objectsToCreate + objectsToOverwrite + objectsToDelete, 0); //processed data is communicated in subfunctions!
                                    }

                                    removeRowsAtExit.markRow(i - fileCmp.begin());
                                    break;
                                }
                                catch (FileError& error)
                                {
                                    ErrorHandler::Response rv = statusUpdater->reportError(error.show());

                                    if ( rv == ErrorHandler::IGNORE_ERROR)
                                        break;
                                    else if (rv == ErrorHandler::RETRY)
                                        ;   //continue with loop
                                    else
                                        assert (false);
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    catch (const RuntimeException& theException)
    {
        statusUpdater->reportFatalError(theException.show().c_str());
        return; //should be obsolete!
    }
}


//###########################################################################################
//callback functionality for smooth progress indicators

class WhileCopying : public FreeFileSync::CopyFileCallback //callback functionality
{
public:

    WhileCopying(wxLongLong& bytesTransferredLast, StatusHandler* statusHandler) :
            m_bytesTransferredLast(bytesTransferredLast),
            m_statusHandler(statusHandler) {}

    virtual Response updateCopyStatus(const wxULongLong& totalBytesTransferred)
    {
        //convert to signed
        const wxLongLong totalBytes = globalFunctions::convertToSigned(totalBytesTransferred);

        //inform about the (differential) processed amount of data
        m_statusHandler->updateProcessedData(0, totalBytes - m_bytesTransferredLast);
        m_bytesTransferredLast = totalBytes;

#ifdef FFS_WIN
        m_statusHandler->requestUiRefresh(false); //don't allow throwing exception within this call
        if (m_statusHandler->abortIsRequested())
            return CopyFileCallback::CANCEL;
#elif defined FFS_LINUX
        m_statusHandler->requestUiRefresh(); //exceptions may be thrown here!
#endif
        return CopyFileCallback::CONTINUE;
    }

private:
    wxLongLong& m_bytesTransferredLast;
    StatusHandler* m_statusHandler;
};


//copy file while executing statusUpdater->requestUiRefresh() calls
void SyncProcess::copyFileUpdating(const Zstring& source, const Zstring& target, const wxULongLong& totalBytesToCpy)
{
    //create folders first (see http://sourceforge.net/tracker/index.php?func=detail&aid=2628943&group_id=234430&atid=1093080)
    const Zstring targetDir = target.BeforeLast(FreeFileSync::FILE_NAME_SEPARATOR);
    if (!wxDirExists(targetDir.c_str()))
    {
        const Zstring templateDir = source.BeforeLast(FreeFileSync::FILE_NAME_SEPARATOR);
        FreeFileSync::createDirectory(targetDir, templateDir, !m_traverseDirSymLinks); //might throw FileError() exception
    }

    //start of (possibly) long-running copy process: ensure status updates are performed regularly
    wxLongLong totalBytesTransferred;
    WhileCopying callback(totalBytesTransferred, statusUpdater);

    try
    {
#ifdef FFS_WIN
        FreeFileSync::copyFile(source, target, m_copyFileSymLinks, shadowCopyHandler.get(), &callback);
#elif defined FFS_LINUX
        FreeFileSync::copyFile(source, target, m_copyFileSymLinks, &callback);
#endif
    }
    catch (...)
    {
        //error situation: undo communication of processed amount of data
        statusUpdater->updateProcessedData(0, totalBytesTransferred * -1 );
        statusUpdater->requestUiRefresh(true); //windows build: abort if requested, don't show error message if cancelled by user!
        throw;
    }

    //inform about the (remaining) processed amount of data
    statusUpdater->updateProcessedData(0, globalFunctions::convertToSigned(totalBytesToCpy) - totalBytesTransferred);
}

/*
//##### OLD IMPLEMENTATION WITHOUT SMOOTH PROGRESS INDICATOR ################

#ifdef FFS_LINUX
//handle execution of a method while updating the UI
class UpdateWhileCopying : public UpdateWhileExecuting
{
public:
    UpdateWhileCopying() {}
    ~UpdateWhileCopying() {}

    Zstring source;
    Zstring target;
    bool success;
    Zstring errorMessage;

private:
    virtual void longRunner()
    {
        try
        {
            FreeFileSync::copyFile(source, target);
        }
        catch (const FileError& e)
        {
            success = false;
            errorMessage = e.show();
            return;
        }

        success = true;
    }
};

void copyFileUpdating(const Zstring& source, const Zstring& target, const wxULongLong& totalBytesToCpy, StatusHandler* updateClass)
{
    //create folders first (see http://sourceforge.net/tracker/index.php?func=detail&aid=2628943&group_id=234430&atid=1093080)
    const Zstring targetDir = target.BeforeLast(FreeFileSync::FILE_NAME_SEPARATOR);
    if (!wxDirExists(targetDir.c_str()))
    {
        const Zstring templateDir = source.BeforeLast(FreeFileSync::FILE_NAME_SEPARATOR);
        FreeFileSync::createDirectory(targetDir, templateDir); //might throw FileError() exception
    }

//update while copying via additional worker thread
    static UpdateWhileCopying copyAndUpdate; //single instantiation: thread enters wait phase after each execution

    copyAndUpdate.waitUntilReady();

    //longRunner is called from thread, but no mutex needed here, since thread is in waiting state!
    copyAndUpdate.source = source;
    copyAndUpdate.target = target;

    copyAndUpdate.execute(updateClass);

    //no mutex needed here since longRunner is finished
    if (!copyAndUpdate.success)
        throw FileError(copyAndUpdate.errorMessage);

    //inform about the processed amount of data
    updateClass->updateProcessedData(0, totalBytesToCpy.ToDouble());
#endif
}
*/
