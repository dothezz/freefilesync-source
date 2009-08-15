#include "synchronization.h"
#include <wx/intl.h>
#include <wx/msgdlg.h>
#include <wx/log.h>
#include "algorithm.h"
#include "shared/globalFunctions.h"
#include "library/statusHandler.h"
#include "shared/fileHandling.h"

using namespace FreeFileSync;


void SyncStatistics::init()
{
    createLeft     = 0;
    createRight    = 0;
    overwriteLeft  = 0;
    overwriteRight = 0;
    deleteLeft     = 0;
    deleteRight    = 0;
    conflict       = 0;
}


SyncStatistics::SyncStatistics(const FileComparison& fileCmp)
{
    init();

    FileComparison::const_iterator start = fileCmp.begin();
    FileComparison::const_iterator end   = fileCmp.end();
    for (; start != end; ++start)
        getNumbers(*start);
}


SyncStatistics::SyncStatistics(const FolderComparison& folderCmp)
{
    init();

    for (FolderComparison::const_iterator j = folderCmp.begin(); j != folderCmp.end(); ++j)
    {
        const FileComparison& fileCmp = j->fileCmp;

        FileComparison::const_iterator start = fileCmp.begin();
        FileComparison::const_iterator end   = fileCmp.end();
        for (; start != end; ++start)
            getNumbers(*start);
    }
}


int SyncStatistics::getCreate(bool inclLeft, bool inclRight) const
{
    return (inclLeft  ? createLeft  : 0) +
           (inclRight ? createRight : 0);
}


int SyncStatistics::getOverwrite(bool inclLeft, bool inclRight) const
{
    return (inclLeft  ? overwriteLeft  : 0) +
           (inclRight ? overwriteRight : 0);
}


int SyncStatistics::getDelete(bool inclLeft, bool inclRight) const
{
    return (inclLeft  ? deleteLeft  : 0) +
           (inclRight ? deleteRight : 0);
}


int SyncStatistics::getConflict() const
{
    return conflict;
}


wxULongLong SyncStatistics::getDataToProcess() const
{
    return dataToProcess;
}


inline
void SyncStatistics::getNumbers(const FileCompareLine& fileCmpLine)
{
    switch (FreeFileSync::getSyncOperation(fileCmpLine)) //evaluate comparison result and sync direction
    {
    case SO_CREATE_NEW_LEFT:
        ++createLeft;
        dataToProcess += fileCmpLine.fileDescrRight.fileSize;
        break;

    case SO_CREATE_NEW_RIGHT:
        ++createRight;
        dataToProcess += fileCmpLine.fileDescrLeft.fileSize;
        break;

    case SO_DELETE_LEFT:
        ++deleteLeft;
        break;

    case SO_DELETE_RIGHT:
        ++deleteRight;
        break;

    case SO_OVERWRITE_LEFT:
        ++overwriteLeft;
        dataToProcess += fileCmpLine.fileDescrRight.fileSize;
        break;

    case SO_OVERWRITE_RIGHT:
        ++overwriteRight;
        dataToProcess += fileCmpLine.fileDescrLeft.fileSize;
        break;

    case SO_DO_NOTHING:
        break;

    case SO_UNRESOLVED_CONFLICT:
        ++conflict;
        break;
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
            switch (i->syncDir)
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


std::pair<wxLongLong, wxLongLong> freeDiskSpaceNeeded(const FileComparison& fileCmp, const DeletionPolicy handleDeletion)
{
    switch (handleDeletion)
    {
    case FreeFileSync::DELETE_PERMANENTLY:
        return spaceNeededSub<false>(fileCmp);
    case FreeFileSync::MOVE_TO_RECYCLE_BIN:
        return spaceNeededSub<true>(fileCmp);
    case FreeFileSync::MOVE_TO_CUSTOM_DIRECTORY:
        return spaceNeededSub<true>(fileCmp); //this is not necessarily correct! it needs to be checked if user-def recycle bin dir and sync-dir are on same drive
    }

    assert(false);
    return spaceNeededSub<true>(fileCmp); //dummy
}


bool FreeFileSync::synchronizationNeeded(const FolderComparison& folderCmp)
{
    const SyncStatistics statisticsTotal(folderCmp);
    return statisticsTotal.getCreate() + statisticsTotal.getOverwrite() + statisticsTotal.getDelete() != 0;
}


//test if user accidentally tries to sync the wrong folders
bool significantDifferenceDetected(const FileComparison& fileCmp)
{
    const SyncStatistics st(fileCmp);

    //initial file copying shall not be detected as major difference
    if (    st.getCreate(true, false) == 0 &&
            st.getOverwrite()         == 0 &&
            st.getDelete()            == 0 &&
            st.getConflict()          == 0) return false;
    if (    st.getCreate(false, true) == 0 &&
            st.getOverwrite()         == 0 &&
            st.getDelete()            == 0 &&
            st.getConflict()          == 0) return false;

    const int changedRows = st.getCreate()    +
                            st.getOverwrite() +
                            st.getDelete()    +
                            st.getConflict();

    return changedRows >= 10 && changedRows > 0.5 * fileCmp.size();
}


FreeFileSync::SyncOperation FreeFileSync::getSyncOperation( //evaluate comparison result and sync direction
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
        switch (syncDir)
        {
        case SYNC_DIR_LEFT:
            return SO_OVERWRITE_LEFT; //copy from right to left
        case SYNC_DIR_RIGHT:
            return SO_OVERWRITE_RIGHT; //copy from left to right
        case SYNC_DIR_NONE:
            return SO_DO_NOTHING;
        case SYNC_UNRESOLVED_CONFLICT:
            assert(false);
        }
        break;

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


SyncOperation FreeFileSync::getSyncOperation(const FileCompareLine& line) //convenience function
{
    return getSyncOperation(line.cmpResult, line.selectedForSynchronization, line.syncDir);
}


/*add some postfix to alternate deletion directory: customDir\2009-06-30 12-59-12\ */
wxString getSessionDeletionDir(const wxString& customDeletionDirectory)
{
    wxString timeNow = wxDateTime::Now().FormatISOTime();
    timeNow.Replace(wxT(":"), wxT("-"));

    const wxString sessionDir = wxDateTime::Now().FormatISODate() + wxChar(' ') + timeNow;

    wxString formattedDirectory = FreeFileSync::getFormattedDirectoryName(customDeletionDirectory.c_str()).c_str();
    if (formattedDirectory.empty())
        return wxEmptyString; //no valid directory for deletion specified (checked later)

    if (!formattedDirectory.EndsWith(wxString(globalFunctions::FILE_NAME_SEPARATOR)))
        formattedDirectory += globalFunctions::FILE_NAME_SEPARATOR;

    formattedDirectory += sessionDir;

    //ensure that session directory does not yet exist (must be unique)
    if (FreeFileSync::dirExists(formattedDirectory))
    {
        //if it's not unique, add a postfix number
        int postfix = 1;
        while (FreeFileSync::dirExists(formattedDirectory + wxT("_") + globalFunctions::numberToWxString(postfix)))
            ++postfix;

        formattedDirectory += wxT("_") + globalFunctions::numberToWxString(postfix);
    }

    formattedDirectory += globalFunctions::FILE_NAME_SEPARATOR;
    return formattedDirectory;
}


SyncProcess::SyncProcess(const DeletionPolicy handleDeletion,
                         const wxString& custDelFolder,
                         const bool copyFileSymLinks,
                         const bool traverseDirSymLinks,
                         xmlAccess::WarningMessages& warnings,
                         StatusHandler* handler) :
        m_handleDeletion(handleDeletion),
        sessionDeletionDirectory(getSessionDeletionDir(custDelFolder).c_str()),
        m_copyFileSymLinks(copyFileSymLinks),
        m_traverseDirSymLinks(traverseDirSymLinks),
        m_warnings(warnings),
#ifdef FFS_WIN
        shadowCopyHandler(new ShadowCopy),
#endif
        statusUpdater(handler),
        txtCopyingFile(Zstring(_("Copying file %x to %y")).Replace(wxT("%x"), wxT("\"%x\""), false).Replace(wxT("%y"), wxT("\n\"%y\""), false)),
        txtOverwritingFile(Zstring(_("Copying file %x to %y overwriting target")).Replace(wxT("%x"), wxT("\"%x\""), false).Replace(wxT("%y"), wxT("\n\"%y\""), false)),
        txtCreatingFolder(Zstring(_("Creating folder %x")).Replace(wxT("%x"), wxT("\n\"%x\""), false)),

        txtDeletingFile(m_handleDeletion == MOVE_TO_RECYCLE_BIN ? Zstring(_("Moving %x to Recycle Bin")).Replace(wxT("%x"), wxT("\"%x\""), false) :
                        m_handleDeletion == DELETE_PERMANENTLY ? Zstring(_("Deleting file %x")).Replace(wxT("%x"), wxT("\n\"%x\""), false) :
                        Zstring(_("Moving %x to custom directory")).Replace(wxT("%x"), wxT("\"%x\""), false)),

        txtDeletingFolder(m_handleDeletion == MOVE_TO_RECYCLE_BIN ? Zstring(_("Moving %x to Recycle Bin")).Replace( wxT("%x"), wxT("\"%x\""), false) :
                          m_handleDeletion == DELETE_PERMANENTLY ? Zstring(_("Deleting folder %x")).Replace( wxT("%x"), wxT("\n\"%x\""), false) :
                          Zstring(_("Moving %x to custom directory")).Replace(wxT("%x"), wxT("\"%x\""), false))
{}


class MoveFileCallbackImpl : public MoveFileCallback //callback functionality
{
public:
    MoveFileCallbackImpl(StatusHandler* handler) : m_statusHandler(handler) {}

    virtual Response requestUiRefresh()  //DON'T throw exceptions here, at least in Windows build!
    {
#ifdef FFS_WIN
        m_statusHandler->requestUiRefresh(false); //don't allow throwing exception within this call: windows copying callback can't handle this
        if (m_statusHandler->abortIsRequested())
            return MoveFileCallback::CANCEL;
#elif defined FFS_LINUX
        m_statusHandler->requestUiRefresh(); //exceptions may be thrown here!
#endif
        return MoveFileCallback::CONTINUE;
    }

private:
    StatusHandler* m_statusHandler;
};


inline
void SyncProcess::removeFile(const FileDescrLine& fildesc, const Zstring& altDeletionDir)
{
    switch (m_handleDeletion)
    {
    case FreeFileSync::DELETE_PERMANENTLY:
        FreeFileSync::removeFile(fildesc.fullName, false);
        break;
    case FreeFileSync::MOVE_TO_RECYCLE_BIN:
        FreeFileSync::removeFile(fildesc.fullName, true);
        break;
    case FreeFileSync::MOVE_TO_CUSTOM_DIRECTORY:
        if (FreeFileSync::fileExists(fildesc.fullName))
        {
            const Zstring targetFile = altDeletionDir + fildesc.relativeName.c_str(); //altDeletionDir ends with path separator
            const Zstring targetDir  = targetFile.BeforeLast(globalFunctions::FILE_NAME_SEPARATOR);

            if (!FreeFileSync::dirExists(targetDir))
            {
                if (!targetDir.empty()) //kind of pathological ?
                    //lazy creation of alternate deletion directory (including super-directories of targetFile)
                    FreeFileSync::createDirectory(targetDir, Zstring(), false);
                /*symbolic link handling:
                if "not traversing symlinks": fullName == c:\syncdir<symlinks>\some\dirs\leaf<symlink>
                => setting irrelevant
                if "traversing symlinks":     fullName == c:\syncdir<symlinks>\some\dirs<symlinks>\leaf<symlink>
                => setting NEEDS to be false: We want to move leaf, therefore symlinks in "some\dirs" must not interfere */
            }

            MoveFileCallbackImpl callBack(statusUpdater); //if file needs to be copied we need callback functionality to update screen and offer abort
            FreeFileSync::moveFile(fildesc.fullName, targetFile, &callBack);
        }
        break;
    }
}


void SyncProcess::synchronizeFile(const FileCompareLine& cmpLine, const FolderPair& folderPair, const Zstring& altDeletionDir)
{
    Zstring statusText;
    Zstring target;

    switch (getSyncOperation(cmpLine)) //evaluate comparison result and sync direction
    {
    case SO_CREATE_NEW_LEFT:
        target = folderPair.leftDirectory + cmpLine.fileDescrRight.relativeName.c_str();

        statusText = txtCopyingFile;
        statusText.Replace(wxT("%x"), cmpLine.fileDescrRight.fullName.AfterLast(globalFunctions::FILE_NAME_SEPARATOR), false);
        statusText.Replace(wxT("%y"), target.BeforeLast(globalFunctions::FILE_NAME_SEPARATOR), false);
        statusUpdater->updateStatusText(statusText);

        copyFileUpdating(cmpLine.fileDescrRight.fullName, target, cmpLine.fileDescrRight.fileSize);
        break;

    case SO_CREATE_NEW_RIGHT:
        target = folderPair.rightDirectory + cmpLine.fileDescrLeft.relativeName.c_str();

        statusText = txtCopyingFile;
        statusText.Replace(wxT("%x"), cmpLine.fileDescrLeft.fullName.AfterLast(globalFunctions::FILE_NAME_SEPARATOR), false);
        statusText.Replace(wxT("%y"), target.BeforeLast(globalFunctions::FILE_NAME_SEPARATOR), false);
        statusUpdater->updateStatusText(statusText);

        copyFileUpdating(cmpLine.fileDescrLeft.fullName, target, cmpLine.fileDescrLeft.fileSize);
        break;

    case SO_DELETE_LEFT:
        statusText = txtDeletingFile;
        statusText.Replace(wxT("%x"), cmpLine.fileDescrLeft.fullName, false);
        statusUpdater->updateStatusText(statusText);

        removeFile(cmpLine.fileDescrLeft, altDeletionDir);
        break;

    case SO_DELETE_RIGHT:
        statusText = txtDeletingFile;
        statusText.Replace(wxT("%x"), cmpLine.fileDescrRight.fullName, false);
        statusUpdater->updateStatusText(statusText);

        removeFile(cmpLine.fileDescrRight, altDeletionDir);
        break;

    case SO_OVERWRITE_RIGHT:
        statusText = txtOverwritingFile;
        statusText.Replace(wxT("%x"), cmpLine.fileDescrLeft.fullName.AfterLast(globalFunctions::FILE_NAME_SEPARATOR), false);
        statusText.Replace(wxT("%y"), cmpLine.fileDescrRight.fullName.BeforeLast(globalFunctions::FILE_NAME_SEPARATOR), false);
        statusUpdater->updateStatusText(statusText);

        removeFile(cmpLine.fileDescrRight, altDeletionDir);
        copyFileUpdating(cmpLine.fileDescrLeft.fullName, cmpLine.fileDescrRight.fullName, cmpLine.fileDescrLeft.fileSize);
        break;

    case SO_OVERWRITE_LEFT:
        statusText = txtOverwritingFile;
        statusText.Replace(wxT("%x"), cmpLine.fileDescrRight.fullName.AfterLast(globalFunctions::FILE_NAME_SEPARATOR), false);
        statusText.Replace(wxT("%y"), cmpLine.fileDescrLeft.fullName.BeforeLast(globalFunctions::FILE_NAME_SEPARATOR), false);
        statusUpdater->updateStatusText(statusText);

        removeFile(cmpLine.fileDescrLeft, altDeletionDir);
        copyFileUpdating(cmpLine.fileDescrRight.fullName, cmpLine.fileDescrLeft.fullName, cmpLine.fileDescrRight.fileSize);
        break;

    case SO_DO_NOTHING:
    case SO_UNRESOLVED_CONFLICT:
        return; //no update on processed data!
    }

    //progress indicator update
    //indicator is updated only if file is sync'ed correctly (and if some sync was done)!
    statusUpdater->updateProcessedData(1, 0); //processed data is communicated in subfunctions!
}


inline
void SyncProcess::removeFolder(const FileDescrLine& fildesc, const Zstring& altDeletionDir)
{
    switch (m_handleDeletion)
    {
    case FreeFileSync::DELETE_PERMANENTLY:
        FreeFileSync::removeDirectory(fildesc.fullName, false);
        break;
    case FreeFileSync::MOVE_TO_RECYCLE_BIN:
        FreeFileSync::removeDirectory(fildesc.fullName, true);
        break;
    case FreeFileSync::MOVE_TO_CUSTOM_DIRECTORY:
        if (FreeFileSync::dirExists(fildesc.fullName))
        {
            const Zstring targetDir      = altDeletionDir + fildesc.relativeName.c_str();
            const Zstring targetSuperDir = targetDir.BeforeLast(globalFunctions::FILE_NAME_SEPARATOR);;

            if (!FreeFileSync::dirExists(targetSuperDir))
            {
                if (!targetSuperDir.empty()) //kind of pathological ?
                    //lazy creation of alternate deletion directory (including super-directories of targetFile)
                    FreeFileSync::createDirectory(targetSuperDir, Zstring(), false);
                /*symbolic link handling:
                if "not traversing symlinks": fullName == c:\syncdir<symlinks>\some\dirs\leaf<symlink>
                => setting irrelevant
                if "traversing symlinks":     fullName == c:\syncdir<symlinks>\some\dirs<symlinks>\leaf<symlink>
                => setting NEEDS to be false: We want to move leaf, therefore symlinks in "some\dirs" must not interfere */
            }

            MoveFileCallbackImpl callBack(statusUpdater); //if files need to be copied, we need callback functionality to update screen and offer abort
            FreeFileSync::moveDirectory(fildesc.fullName, targetDir, true, &callBack);
        }
        break;
    }
}


void SyncProcess::synchronizeFolder(const FileCompareLine& cmpLine, const FolderPair& folderPair, const Zstring& altDeletionDir)
{
    Zstring statusText;
    Zstring target;

    //synchronize folders:
    switch (getSyncOperation(cmpLine)) //evaluate comparison result and sync direction
    {
    case SO_CREATE_NEW_LEFT:
        target = folderPair.leftDirectory + cmpLine.fileDescrRight.relativeName.c_str();

        statusText = txtCreatingFolder;
        statusText.Replace(wxT("%x"), target, false);
        statusUpdater->updateStatusText(statusText);

        //some check to catch the error that directory on source has been deleted externally after "compare"...
        if (!FreeFileSync::dirExists(cmpLine.fileDescrRight.fullName))
            throw FileError(wxString(_("Error: Source directory does not exist anymore:")) + wxT("\n\"") + cmpLine.fileDescrRight.fullName.c_str() + wxT("\""));
        createDirectory(target, cmpLine.fileDescrRight.fullName, !m_traverseDirSymLinks); //traverse symlinks <=> !copy symlinks
        break;

    case SO_CREATE_NEW_RIGHT:
        target = folderPair.rightDirectory + cmpLine.fileDescrLeft.relativeName.c_str();

        statusText = txtCreatingFolder;
        statusText.Replace(wxT("%x"), target, false);
        statusUpdater->updateStatusText(statusText);

        //some check to catch the error that directory on source has been deleted externally after "compare"...
        if (!FreeFileSync::dirExists(cmpLine.fileDescrLeft.fullName))
            throw FileError(wxString(_("Error: Source directory does not exist anymore:")) + wxT("\n\"") + cmpLine.fileDescrLeft.fullName.c_str() + wxT("\""));
        createDirectory(target, cmpLine.fileDescrLeft.fullName, !m_traverseDirSymLinks); //traverse symlinks <=> !copy symlinks
        break;

    case SO_DELETE_LEFT:
        statusText = txtDeletingFolder;
        statusText.Replace(wxT("%x"), cmpLine.fileDescrLeft.fullName, false);
        statusUpdater->updateStatusText(statusText);

        removeFolder(cmpLine.fileDescrLeft, altDeletionDir);
        break;

    case SO_DELETE_RIGHT:
        statusText = txtDeletingFolder;
        statusText.Replace(wxT("%x"), cmpLine.fileDescrRight.fullName, false);
        statusUpdater->updateStatusText(statusText);

        removeFolder(cmpLine.fileDescrRight, altDeletionDir);
        break;

    case SO_OVERWRITE_RIGHT:
    case SO_OVERWRITE_LEFT:
    case SO_UNRESOLVED_CONFLICT:
        assert(false);
    case SO_DO_NOTHING:
        return; //no update on processed data!
    }

    //progress indicator update
    //indicator is updated only if directory is sync'ed correctly (and if some work was done)!
    statusUpdater->updateProcessedData(1, 0);  //each call represents one processed file
}


inline
bool deletionImminent(const FileCompareLine& line)
{
    //test if current sync-line will result in deletion of files    -> used to avoid disc space bottlenecks
    const SyncOperation op = FreeFileSync::getSyncOperation(line);
    return op == FreeFileSync::SO_DELETE_LEFT || op == FreeFileSync::SO_DELETE_RIGHT;
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
    const SyncStatistics statisticsTotal(folderCmp);

    //keep at beginning so that all gui elements are initialized properly
    statusUpdater->initNewProcess(statisticsTotal.getCreate() + statisticsTotal.getOverwrite() + statisticsTotal.getDelete(),
                                  globalFunctions::convertToSigned(statisticsTotal.getDataToProcess()),
                                  StatusHandler::PROCESS_SYNCHRONIZING);

    //-------------------some basic checks:------------------------------------------

    if (statisticsTotal.getOverwrite() + statisticsTotal.getDelete() > 0)
    {
        //test existence of Recycle Bin
        if (m_handleDeletion == FreeFileSync::MOVE_TO_RECYCLE_BIN && !FreeFileSync::recycleBinExists())
        {
            statusUpdater->reportFatalError(_("Unable to initialize Recycle Bin!"));
            return; //should be obsolete!
        }

        if (m_handleDeletion == FreeFileSync::MOVE_TO_CUSTOM_DIRECTORY)
        {
            //check if alternate directory for deletion was specified
            if (sessionDeletionDirectory.empty())
            {
                statusUpdater->reportFatalError(_("Please specify alternate directory for deletion!"));
                return; //should be obsolete!
            }
        }
    }

    for (FolderComparison::const_iterator j = folderCmp.begin(); j != folderCmp.end(); ++j)
    {
        const FileComparison& fileCmp = j->fileCmp;

        //check if more than 50% of total number of files/dirs are to be created/overwritten/deleted
        if (significantDifferenceDetected(fileCmp))
        {
            statusUpdater->reportWarning(wxString(_("Significant difference detected:")) + wxT("\n") +
                                         j->syncPair.leftDirectory + wxT(" <-> ") + wxT("\n") +
                                         j->syncPair.rightDirectory + wxT("\n\n") +
                                         _("More than 50% of the total number of files will be copied or deleted!"),
                                         m_warnings.warningSignificantDifference);
        }

        //check for sufficient free diskspace in left directory
        const std::pair<wxLongLong, wxLongLong> spaceNeeded = freeDiskSpaceNeeded(fileCmp, m_handleDeletion);

        wxLongLong freeDiskSpaceLeft;
        if (wxGetDiskSpace(j->syncPair.leftDirectory.c_str(), NULL, &freeDiskSpaceLeft))
        {
            if (freeDiskSpaceLeft < spaceNeeded.first)
                statusUpdater->reportWarning(wxString(_("Not enough free disk space available in:")) + wxT("\n") +
                                             j->syncPair.leftDirectory + wxT("\n\n") +
                                             _("Total required free disk space:") + wxT(" ") + formatFilesizeToShortString(spaceNeeded.first).c_str(),
                                             m_warnings.warningNotEnoughDiskSpace);
        }

        //check for sufficient free diskspace in right directory
        wxLongLong freeDiskSpaceRight;
        if (wxGetDiskSpace(j->syncPair.rightDirectory.c_str(), NULL, &freeDiskSpaceRight))
        {
            if (freeDiskSpaceRight < spaceNeeded.second)
                statusUpdater->reportWarning(wxString(_("Not enough free disk space available in:")) + wxT("\n") +
                                             j->syncPair.rightDirectory + wxT("\n\n") +
                                             _("Total required free disk space:") + wxT(" ") + formatFilesizeToShortString(spaceNeeded.second).c_str(),
                                             m_warnings.warningNotEnoughDiskSpace);
        }
    }

    //check if unresolved conflicts exist
    if (statisticsTotal.getConflict() > 0)
        statusUpdater->reportWarning(_("Unresolved conflicts existing! \n\nYou can ignore conflicts and continue synchronization."),
                                     m_warnings.warningUnresolvedConflicts);

    //-------------------end of basic checks------------------------------------------


    try
    {
        //loop over folder pairs
        for (FolderComparison::iterator j = folderCmp.begin(); j != folderCmp.end(); ++j)
        {
            FileComparison& fileCmp = j->fileCmp;
            const FolderPair& currentPair = j->syncPair;

            //generate name of alternate deletion directory (unique for session AND folder pair)
            const Zstring alternateDeletionDir = folderCmp.size() > 1 ? //if multiple folder pairs are used create a subfolder for each pair
                                                 sessionDeletionDirectory + globalFunctions::numberToWxString(j - folderCmp.begin() + 1).c_str() + globalFunctions::FILE_NAME_SEPARATOR :
                                                 sessionDeletionDirectory;

            //ensure that folderCmp is always written to, even if method is left via exceptions
            RemoveAtExit removeRowsAtExit(fileCmp); //note: running at individual folder pair level!

            // it should never happen, that a directory on left side has same name as file on right side. startCompareProcess() should take care of this
            // and split into two "exists on one side only" cases
            // Note: this case is not handled by this tool and must be solved by the user

            //synchronize folders first; advantage: in case of deletions the whole folder is moved to recycle bin instead of single files
            for (FileComparison::const_iterator i = fileCmp.begin(); i != fileCmp.end(); ++i)
            {
                if (    i->fileDescrLeft.objType  == FileDescrLine::TYPE_DIRECTORY ||
                        i->fileDescrRight.objType == FileDescrLine::TYPE_DIRECTORY)
                {
                    while (true)
                    {   //trigger display refresh
                        statusUpdater->requestUiRefresh();

                        try
                        {
                            synchronizeFolder(*i, currentPair, alternateDeletionDir);

                            removeRowsAtExit.markRow(i - fileCmp.begin());
                            break;
                        }
                        catch (FileError& error)
                        {
                            //User abort when copying files or moving files/directories into custom deletion directory:
                            //windows build: abort if requested, don't show error message if cancelled by user!
                            //linux build: this refresh is not necessary, because user abort triggers an AbortThisProcess() exception without a FileError()
                            statusUpdater->requestUiRefresh(true);


                            ErrorHandler::Response rv = statusUpdater->reportError(error.show());

                            if (rv == ErrorHandler::IGNORE_ERROR)
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
                const bool deleteLoop = k == 0; //-> first loop: delete files, second loop: copy files

                for (FileComparison::const_iterator i = fileCmp.begin(); i != fileCmp.end(); ++i)
                {
                    //skip processing of unresolved errors (do not remove row from GUI grid)
                    if (getSyncOperation(*i) == SO_UNRESOLVED_CONFLICT)
                        continue;

                    if (    i->fileDescrLeft.objType  == FileDescrLine::TYPE_FILE ||
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
                                    synchronizeFile(*i, currentPair, alternateDeletionDir);

                                    removeRowsAtExit.markRow(i - fileCmp.begin());
                                    break;
                                }
                                catch (FileError& error)
                                {
                                    //User abort when copying files or moving files/directories into custom deletion directory:
                                    //windows build: abort if requested, don't show error message if cancelled by user!
                                    //linux build: this refresh is not necessary, because user abort triggers an AbortThisProcess() exception without a FileError()
                                    statusUpdater->requestUiRefresh(true);


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
        m_statusHandler->requestUiRefresh(false); //don't allow throwing exception within this call: windows copying callback can't handle this
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
    const Zstring targetDir = target.BeforeLast(globalFunctions::FILE_NAME_SEPARATOR);
    if (!FreeFileSync::dirExists(targetDir))
    {
        if (!targetDir.empty()) //kind of pathological, but empty target might happen
        {
            const Zstring templateDir = source.BeforeLast(globalFunctions::FILE_NAME_SEPARATOR);
            FreeFileSync::createDirectory(targetDir, templateDir, false);
            //symbolic link setting: If traversing into links => don't create
            //if not traversing into links => copy dir symlink of leaf-dir, not relevant here => actually: setting doesn't matter
        }
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
    const Zstring targetDir = target.BeforeLast(globalFunctions::FILE_NAME_SEPARATOR);
    if (!wxDirExists(targetDir.c_str()))
    {
        const Zstring templateDir = source.BeforeLast(globalFunctions::FILE_NAME_SEPARATOR);
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
