#include "synchronization.h"
#include <stdexcept>
#include <wx/intl.h>
#include <wx/msgdlg.h>
#include <wx/log.h>
#include "shared/stringConv.h"
#include "ui/util.h"
#include "shared/systemConstants.h"
#include "library/statusHandler.h"
#include "shared/fileHandling.h"
#include <wx/file.h>
#include <boost/bind.hpp>
#include "shared/globalFunctions.h"
#include <boost/scoped_array.hpp>

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
    rowsTotal      = 0;
}


SyncStatistics::SyncStatistics(const HierarchyObject&  hierObj)
{
    init();
    getNumbersRecursively(hierObj);
}


SyncStatistics::SyncStatistics(const FolderComparison& folderCmp)
{
    init();
    std::for_each(folderCmp.begin(), folderCmp.end(), boost::bind(&SyncStatistics::getNumbersRecursively, this, _1));
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


int SyncStatistics::getRowCount() const
{
    return rowsTotal;
}


inline
void SyncStatistics::getNumbersRecursively(const HierarchyObject& hierObj)
{
    //process directories
    std::for_each(hierObj.subDirs.begin(), hierObj.subDirs.end(),
                  boost::bind(&SyncStatistics::getDirNumbers, this, _1));

    //process files
    std::for_each(hierObj.subFiles.begin(), hierObj.subFiles.end(),
                  boost::bind(&SyncStatistics::getFileNumbers, this, _1));

    rowsTotal += hierObj.subDirs.size();
    rowsTotal += hierObj.subFiles.size();

    //recurse into sub-dirs
    std::for_each(hierObj.subDirs.begin(), hierObj.subDirs.end(), boost::bind(&SyncStatistics::getNumbersRecursively, this, _1));
}


inline
void SyncStatistics::getFileNumbers(const FileMapping& fileObj)
{
    switch (fileObj.getSyncOperation()) //evaluate comparison result and sync direction
    {
    case SO_CREATE_NEW_LEFT:
        ++createLeft;
        dataToProcess += fileObj.getFileSize<RIGHT_SIDE>();
        break;

    case SO_CREATE_NEW_RIGHT:
        ++createRight;
        dataToProcess += fileObj.getFileSize<LEFT_SIDE>();
        break;

    case SO_DELETE_LEFT:
        ++deleteLeft;
        break;

    case SO_DELETE_RIGHT:
        ++deleteRight;
        break;

    case SO_OVERWRITE_LEFT:
        ++overwriteLeft;
        dataToProcess += fileObj.getFileSize<RIGHT_SIDE>();
        break;

    case SO_OVERWRITE_RIGHT:
        ++overwriteRight;
        dataToProcess += fileObj.getFileSize<LEFT_SIDE>();
        break;

    case SO_DO_NOTHING:
        break;

    case SO_UNRESOLVED_CONFLICT:
        ++conflict;
        break;
    }
}


inline
void SyncStatistics::getDirNumbers(const DirMapping& dirObj)
{
    switch (dirObj.getSyncOperation()) //evaluate comparison result and sync direction
    {
    case SO_CREATE_NEW_LEFT:
        ++createLeft;
        break;

    case SO_CREATE_NEW_RIGHT:
        ++createRight;
        break;

    case SO_DELETE_LEFT:
        ++deleteLeft;
        break;

    case SO_DELETE_RIGHT:
        ++deleteRight;
        break;

    case SO_OVERWRITE_LEFT:
    case SO_OVERWRITE_RIGHT:
    case SO_UNRESOLVED_CONFLICT:
        assert(false);
        break;

    case SO_DO_NOTHING:
        break;
    }
}


std::vector<FreeFileSync::FolderPairSyncCfg> FreeFileSync::extractSyncCfg(const MainConfiguration& mainCfg)
{
    std::vector<FolderPairSyncCfg> output;

    //add main pair
    output.push_back(
        FolderPairSyncCfg(mainCfg.syncConfiguration.automatic,
                          mainCfg.handleDeletion,
                          wxToZ(mainCfg.customDeletionDirectory)));

    //add additional pairs
    for (std::vector<FolderPairEnh>::const_iterator i = mainCfg.additionalPairs.begin(); i != mainCfg.additionalPairs.end(); ++i)
        output.push_back(
            FolderPairSyncCfg(i->altSyncConfig.get() ? i->altSyncConfig->syncConfiguration.automatic : mainCfg.syncConfiguration.automatic,
                              i->altSyncConfig.get() ? i->altSyncConfig->handleDeletion              : mainCfg.handleDeletion,
                              wxToZ(i->altSyncConfig.get() ? i->altSyncConfig->customDeletionDirectory : mainCfg.customDeletionDirectory)));
    return output;
}


template <bool recyclerUsed>
class DiskSpaceNeeded
{
public:
    DiskSpaceNeeded(const BaseDirMapping& baseObj)
    {
        processRecursively(baseObj);
    }

    std::pair<wxLongLong, wxLongLong> getSpaceTotal() const
    {
        return std::make_pair(spaceNeededLeft, spaceNeededRight);
    }

private:
    void processRecursively(const HierarchyObject& hierObj)
    {
        //don't process directories

        //process files
        for (HierarchyObject::SubFileMapping::const_iterator i = hierObj.subFiles.begin(); i != hierObj.subFiles.end(); ++i)
            switch (i->getSyncOperation()) //evaluate comparison result and sync direction
            {
            case SO_CREATE_NEW_LEFT:
                spaceNeededLeft += globalFunctions::convertToSigned(i->getFileSize<RIGHT_SIDE>());
                break;

            case SO_CREATE_NEW_RIGHT:
                spaceNeededRight += globalFunctions::convertToSigned(i->getFileSize<LEFT_SIDE>());
                break;

            case SO_DELETE_LEFT:
                if (!recyclerUsed)
                    spaceNeededLeft -= globalFunctions::convertToSigned(i->getFileSize<LEFT_SIDE>());
                break;

            case SO_DELETE_RIGHT:
                if (!recyclerUsed)
                    spaceNeededRight -= globalFunctions::convertToSigned(i->getFileSize<RIGHT_SIDE>());
                break;

            case SO_OVERWRITE_LEFT:
                if (!recyclerUsed)
                    spaceNeededLeft -= globalFunctions::convertToSigned(i->getFileSize<LEFT_SIDE>());
                spaceNeededLeft += globalFunctions::convertToSigned(i->getFileSize<RIGHT_SIDE>());
                break;

            case SO_OVERWRITE_RIGHT:
                if (!recyclerUsed)
                    spaceNeededRight -= globalFunctions::convertToSigned(i->getFileSize<RIGHT_SIDE>());
                spaceNeededRight += globalFunctions::convertToSigned(i->getFileSize<LEFT_SIDE>());
                break;

            case SO_DO_NOTHING:
            case SO_UNRESOLVED_CONFLICT:
                break;
            }


        //recurse into sub-dirs
        std::for_each(hierObj.subDirs.begin(), hierObj.subDirs.end(), boost::bind(&DiskSpaceNeeded<recyclerUsed>::processRecursively, this, _1));
    }

    wxLongLong spaceNeededLeft;
    wxLongLong spaceNeededRight;
};


std::pair<wxLongLong, wxLongLong> freeDiskSpaceNeeded(const BaseDirMapping& baseDirObj, const DeletionPolicy handleDeletion)
{
    switch (handleDeletion)
    {
    case FreeFileSync::DELETE_PERMANENTLY:
        return DiskSpaceNeeded<false>(baseDirObj).getSpaceTotal();
    case FreeFileSync::MOVE_TO_RECYCLE_BIN:
        return DiskSpaceNeeded<true>(baseDirObj).getSpaceTotal();
    case FreeFileSync::MOVE_TO_CUSTOM_DIRECTORY:
        //warning: this is not necessarily correct! it needs to be checked if user-def recycle bin dir and sync-dir are on same drive
        return DiskSpaceNeeded<true>(baseDirObj).getSpaceTotal();
    }

    assert(false);
    return std::make_pair(2000000000, 2000000000); //dummy
}


bool synchronizationNeeded(const SyncStatistics& statisticsTotal)
{
    return statisticsTotal.getCreate() +
           statisticsTotal.getOverwrite() +
           statisticsTotal.getDelete() +
           //conflicts (unless excluded) justify a sync! Also note: if this method returns false, no warning about unresolved conflicts were shown!
           statisticsTotal.getConflict() != 0;
}


bool FreeFileSync::synchronizationNeeded(const FolderComparison& folderCmp)
{
    const SyncStatistics statisticsTotal(folderCmp);
    return ::synchronizationNeeded(statisticsTotal);
}


//test if user accidentally tries to sync the wrong folders
bool significantDifferenceDetected(const SyncStatistics& folderPairStat)
{
    //initial file copying shall not be detected as major difference
    if (    folderPairStat.getCreate(true, false) == 0 &&
            folderPairStat.getOverwrite()         == 0 &&
            folderPairStat.getDelete()            == 0 &&
            folderPairStat.getConflict()          == 0) return false;
    if (    folderPairStat.getCreate(false, true) == 0 &&
            folderPairStat.getOverwrite()         == 0 &&
            folderPairStat.getDelete()            == 0 &&
            folderPairStat.getConflict()          == 0) return false;

    const int changedRows = folderPairStat.getCreate()    +
                            folderPairStat.getOverwrite() +
                            folderPairStat.getDelete()    +
                            folderPairStat.getConflict();

    return changedRows >= 10 && changedRows > 0.5 * folderPairStat.getRowCount();
}


/*add some postfix to alternate deletion directory: customDir\2009-06-30 12-59-12\ */
Zstring getSessionDeletionDir(const Zstring& customDeletionDirectory)
{
    wxString timeNow = wxDateTime::Now().FormatISOTime();
    timeNow.Replace(wxT(":"), wxT("-"));

    const wxString sessionDir = wxDateTime::Now().FormatISODate() + wxChar(' ') + timeNow;

    Zstring formattedDirectory = FreeFileSync::getFormattedDirectoryName(customDeletionDirectory);
    if (formattedDirectory.empty())
        return Zstring(); //no valid directory for deletion specified (checked later)

    if (!formattedDirectory.EndsWith(globalFunctions::FILE_NAME_SEPARATOR))
        formattedDirectory += globalFunctions::FILE_NAME_SEPARATOR;

    formattedDirectory += wxToZ(sessionDir);

    //ensure that session directory does not yet exist (must be unique)
    if (FreeFileSync::dirExists(formattedDirectory))
    {
        //if it's not unique, add a postfix number
        int postfix = 1;
        while (FreeFileSync::dirExists(formattedDirectory + DefaultStr("_") + numberToZstring(postfix)))
            ++postfix;

        formattedDirectory += Zstring(DefaultStr("_")) + numberToZstring(postfix);
    }

    formattedDirectory += globalFunctions::FILE_NAME_SEPARATOR;
    return formattedDirectory;
}


SyncProcess::SyncProcess(const bool copyFileSymLinks,
                         const bool traverseDirSymLinks,
                         xmlAccess::OptionalDialogs& warnings,
                         const bool verifyCopiedFiles,
                         StatusHandler* handler) :
    m_copyFileSymLinks(copyFileSymLinks),
    m_traverseDirSymLinks(traverseDirSymLinks),
    m_verifyCopiedFiles(verifyCopiedFiles),
    m_warnings(warnings),
#ifdef FFS_WIN
    shadowCopyHandler(new ShadowCopy),
#endif
    statusUpdater(handler),
    txtCopyingFile(wxToZ(_("Copying file %x to %y")).Replace(DefaultStr("%x"), DefaultStr("\"%x\""), false).Replace(DefaultStr("%y"), DefaultStr("\n\"%y\""), false)),
    txtOverwritingFile(wxToZ(_("Copying file %x to %y overwriting target")).Replace(DefaultStr("%x"), DefaultStr("\"%x\""), false).Replace(DefaultStr("%y"), DefaultStr("\n\"%y\""), false)),
    txtCreatingFolder(wxToZ(_("Creating folder %x")).Replace(DefaultStr("%x"), DefaultStr("\n\"%x\""), false)),
    txtDeletingFile(wxToZ(_("Deleting file %x")).Replace(DefaultStr("%x"), DefaultStr("\n\"%x\""), false)),
    txtDeletingFolder(wxToZ(_("Deleting folder %x")).Replace( DefaultStr("%x"), DefaultStr("\n\"%x\""), false)),
    txtMoveToRecycler(wxToZ(_("Moving %x to Recycle Bin")).Replace(DefaultStr("%x"), DefaultStr("\"%x\""), false)),
    txtVerifying(wxToZ(_("Verifying file %x")).Replace(DefaultStr("%x"), DefaultStr("\n\"%x\""), false))
{}


SyncProcess::DeletionHandling::DeletionHandling(const DeletionPolicy handleDel,
        const Zstring& custDelFolder) :
    handleDeletion(handleDel),
    currentDelFolder(getSessionDeletionDir(custDelFolder)), //ends with path separator
    txtMoveFileUserDefined(  wxToZ(_("Moving file %x to user-defined directory %y")).  Replace(DefaultStr("%x"), DefaultStr("\"%x\"\n"), false).Replace(DefaultStr("%y"), Zstring(DefaultStr("\"")) + custDelFolder + DefaultStr("\""), false)),
    txtMoveFolderUserDefined(wxToZ(_("Moving folder %x to user-defined directory %y")).Replace(DefaultStr("%x"), DefaultStr("\"%x\"\n"), false).Replace(DefaultStr("%y"), Zstring(DefaultStr("\"")) + custDelFolder + DefaultStr("\""), false))
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


template <FreeFileSync::SelectedSide side>
inline
void SyncProcess::removeFile(const FileMapping& fileObj, const DeletionHandling& delHandling, bool showStatusUpdate) const
{
    Zstring statusText;

    switch (delHandling.handleDeletion)
    {
    case FreeFileSync::DELETE_PERMANENTLY:
        if (showStatusUpdate) //status information
        {
            statusText = txtDeletingFile;
            statusText.Replace(DefaultStr("%x"), fileObj.getFullName<side>(), false);
            statusUpdater->updateStatusText(statusText);
            statusUpdater->requestUiRefresh(); //trigger display refresh
        }
        FreeFileSync::removeFile(fileObj.getFullName<side>(), false);
        break;
    case FreeFileSync::MOVE_TO_RECYCLE_BIN:
        if (showStatusUpdate) //status information
        {
            statusText = txtMoveToRecycler;
            statusText.Replace(DefaultStr("%x"), fileObj.getFullName<side>(), false);
            statusUpdater->updateStatusText(statusText);
            statusUpdater->requestUiRefresh(); //trigger display refresh
        }
        FreeFileSync::removeFile(fileObj.getFullName<side>(), true);
        break;
    case FreeFileSync::MOVE_TO_CUSTOM_DIRECTORY:
        if (FreeFileSync::fileExists(fileObj.getFullName<side>()))
        {
            if (showStatusUpdate) //status information
            {
                statusText = delHandling.txtMoveFileUserDefined;
                statusText.Replace(DefaultStr("%x"), fileObj.getFullName<side>(), false);
                statusUpdater->updateStatusText(statusText);
                statusUpdater->requestUiRefresh(); //trigger display refresh
            }
            const Zstring targetFile = delHandling.currentDelFolder + fileObj.getRelativeName<side>(); //altDeletionDir ends with path separator
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
            FreeFileSync::moveFile(fileObj.getFullName<side>(), targetFile, &callBack);
        }
        break;
    }
}


void SyncProcess::synchronizeFile(FileMapping& fileObj, const DeletionHandling& delHandling) const
{
    Zstring statusText;
    Zstring target;

    switch (fileObj.getSyncOperation()) //evaluate comparison result and sync direction
    {
    case SO_CREATE_NEW_LEFT:
        target = fileObj.getBaseDirPf<LEFT_SIDE>() + fileObj.getRelativeName<RIGHT_SIDE>();

        statusText = txtCopyingFile;
        statusText.Replace(DefaultStr("%x"), fileObj.getShortName<RIGHT_SIDE>(), false);
        statusText.Replace(DefaultStr("%y"), target.BeforeLast(globalFunctions::FILE_NAME_SEPARATOR), false);
        statusUpdater->updateStatusText(statusText);
        statusUpdater->requestUiRefresh(); //trigger display refresh

        copyFileUpdating(fileObj.getFullName<RIGHT_SIDE>(), target, fileObj.getFileSize<RIGHT_SIDE>());
        break;

    case SO_CREATE_NEW_RIGHT:
        target = fileObj.getBaseDirPf<RIGHT_SIDE>() + fileObj.getRelativeName<LEFT_SIDE>();

        statusText = txtCopyingFile;
        statusText.Replace(DefaultStr("%x"), fileObj.getShortName<LEFT_SIDE>(), false);
        statusText.Replace(DefaultStr("%y"), target.BeforeLast(globalFunctions::FILE_NAME_SEPARATOR), false);
        statusUpdater->updateStatusText(statusText);
        statusUpdater->requestUiRefresh(); //trigger display refresh

        copyFileUpdating(fileObj.getFullName<LEFT_SIDE>(), target, fileObj.getFileSize<LEFT_SIDE>());
        break;

    case SO_DELETE_LEFT:
        removeFile<LEFT_SIDE>(fileObj, delHandling, true); //status updates in subroutine
        break;

    case SO_DELETE_RIGHT:
        removeFile<RIGHT_SIDE>(fileObj, delHandling, true); //status updates in subroutine
        break;

    case SO_OVERWRITE_RIGHT:
        statusText = txtOverwritingFile;
        statusText.Replace(DefaultStr("%x"), fileObj.getShortName<LEFT_SIDE>(), false);
        statusText.Replace(DefaultStr("%y"), fileObj.getFullName<RIGHT_SIDE>().BeforeLast(globalFunctions::FILE_NAME_SEPARATOR), false);
        statusUpdater->updateStatusText(statusText);
        statusUpdater->requestUiRefresh(); //trigger display refresh

        removeFile<RIGHT_SIDE>(fileObj, delHandling, false);
        copyFileUpdating(fileObj.getFullName<LEFT_SIDE>(), fileObj.getFullName<RIGHT_SIDE>(), fileObj.getFileSize<LEFT_SIDE>());
        break;

    case SO_OVERWRITE_LEFT:
        statusText = txtOverwritingFile;
        statusText.Replace(DefaultStr("%x"), fileObj.getShortName<RIGHT_SIDE>(), false);
        statusText.Replace(DefaultStr("%y"), fileObj.getFullName<LEFT_SIDE>().BeforeLast(globalFunctions::FILE_NAME_SEPARATOR), false);
        statusUpdater->updateStatusText(statusText);
        statusUpdater->requestUiRefresh(); //trigger display refresh

        removeFile<LEFT_SIDE>(fileObj, delHandling, false);
        copyFileUpdating(fileObj.getFullName<RIGHT_SIDE>(), fileObj.getFullName<LEFT_SIDE>(), fileObj.getFileSize<RIGHT_SIDE>());
        break;

    case SO_DO_NOTHING:
    case SO_UNRESOLVED_CONFLICT:
        return; //no update on processed data!
    }

    //update FileMapping
    fileObj.synchronizeSides();

    //progress indicator update
    //indicator is updated only if file is sync'ed correctly (and if some sync was done)!
    statusUpdater->updateProcessedData(1, 0); //processed data is communicated in subfunctions!
}


template <FreeFileSync::SelectedSide side>
inline
void SyncProcess::removeFolder(const DirMapping& dirObj, const DeletionHandling& delHandling) const
{
    Zstring statusText;

    switch (delHandling.handleDeletion)
    {
    case FreeFileSync::DELETE_PERMANENTLY:
        //status information
        statusText = txtDeletingFolder;
        statusText.Replace(DefaultStr("%x"), dirObj.getFullName<side>(), false);
        statusUpdater->updateStatusText(statusText);
        statusUpdater->requestUiRefresh(); //trigger display refresh

        FreeFileSync::removeDirectory(dirObj.getFullName<side>(), false);
        break;
    case FreeFileSync::MOVE_TO_RECYCLE_BIN:
        //status information
        statusText = txtMoveToRecycler;
        statusText.Replace(DefaultStr("%x"), dirObj.getFullName<side>(), false);
        statusUpdater->updateStatusText(statusText);
        statusUpdater->requestUiRefresh(); //trigger display refresh

        FreeFileSync::removeDirectory(dirObj.getFullName<side>(), true);
        break;
    case FreeFileSync::MOVE_TO_CUSTOM_DIRECTORY:
        if (FreeFileSync::dirExists(dirObj.getFullName<side>()))
        {
            //status information
            statusText = delHandling.txtMoveFolderUserDefined;
            statusText.Replace(DefaultStr("%x"), dirObj.getFullName<side>(), false);
            statusUpdater->updateStatusText(statusText);
            statusUpdater->requestUiRefresh(); //trigger display refresh

            const Zstring targetDir      = delHandling.currentDelFolder + dirObj.getRelativeName<side>();
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
            FreeFileSync::moveDirectory(dirObj.getFullName<side>(), targetDir, true, &callBack);
        }
        break;
    }
}


void SyncProcess::synchronizeFolder(DirMapping& dirObj, const DeletionHandling& delHandling) const
{
    Zstring statusText;
    Zstring target;

    //synchronize folders:
    switch (dirObj.getSyncOperation()) //evaluate comparison result and sync direction
    {
    case SO_CREATE_NEW_LEFT:
        target = dirObj.getBaseDirPf<LEFT_SIDE>() + dirObj.getRelativeName<RIGHT_SIDE>();

        statusText = txtCreatingFolder;
        statusText.Replace(DefaultStr("%x"), target, false);
        statusUpdater->updateStatusText(statusText);
        statusUpdater->requestUiRefresh(); //trigger display refresh

        //some check to catch the error that directory on source has been deleted externally after "compare"...
        if (!FreeFileSync::dirExists(dirObj.getFullName<RIGHT_SIDE>()))
            throw FileError(wxString(_("Source directory does not exist anymore:")) + wxT("\n\"") + zToWx(dirObj.getFullName<RIGHT_SIDE>()) + wxT("\""));
        createDirectory(target, dirObj.getFullName<RIGHT_SIDE>(), !m_traverseDirSymLinks); //traverse symlinks <=> !copy symlinks
        break;

    case SO_CREATE_NEW_RIGHT:
        target = dirObj.getBaseDirPf<RIGHT_SIDE>() + dirObj.getRelativeName<LEFT_SIDE>();

        statusText = txtCreatingFolder;
        statusText.Replace(DefaultStr("%x"), target, false);
        statusUpdater->updateStatusText(statusText);
        statusUpdater->requestUiRefresh(); //trigger display refresh

        //some check to catch the error that directory on source has been deleted externally after "compare"...
        if (!FreeFileSync::dirExists(dirObj.getFullName<LEFT_SIDE>()))
            throw FileError(wxString(_("Source directory does not exist anymore:")) + wxT("\n\"") + zToWx(dirObj.getFullName<LEFT_SIDE>()) + wxT("\""));
        createDirectory(target, dirObj.getFullName<LEFT_SIDE>(), !m_traverseDirSymLinks); //traverse symlinks <=> !copy symlinks
        break;

    case SO_DELETE_LEFT:
        removeFolder<LEFT_SIDE>(dirObj, delHandling);
        {
            //progress indicator update: DON'T forget to notify about implicitly deleted objects!
            const SyncStatistics subObjects(dirObj);
            //...then remove everything
            dirObj.subFiles.clear();
            dirObj.subDirs.clear();
            statusUpdater->updateProcessedData(subObjects.getCreate() + subObjects.getOverwrite() + subObjects.getDelete(), subObjects.getDataToProcess().ToDouble());
        }
        break;

    case SO_DELETE_RIGHT:
        removeFolder<RIGHT_SIDE>(dirObj, delHandling);
        {
            //progress indicator update: DON'T forget to notify about implicitly deleted objects!
            const SyncStatistics subObjects(dirObj);
            //...then remove everything
            dirObj.subFiles.clear();
            dirObj.subDirs.clear();
            statusUpdater->updateProcessedData(subObjects.getCreate() + subObjects.getOverwrite() + subObjects.getDelete(), subObjects.getDataToProcess().ToDouble());
        }
        break;

    case SO_OVERWRITE_RIGHT:
    case SO_OVERWRITE_LEFT:
    case SO_UNRESOLVED_CONFLICT:
        assert(false);
    case SO_DO_NOTHING:
        return; //no update on processed data!
    }

    //update DirMapping
    dirObj.synchronizeSides();

    //progress indicator update
    //indicator is updated only if directory is sync'ed correctly (and if some work was done)!
    statusUpdater->updateProcessedData(1, 0);  //each call represents one processed file
}


inline
bool deletionImminent(const FileSystemObject& fsObj)
{
    //test if current sync-line will result in deletion of files    -> used to avoid disc space bottlenecks
    const SyncOperation op = fsObj.getSyncOperation();
    return op == FreeFileSync::SO_DELETE_LEFT || op == FreeFileSync::SO_DELETE_RIGHT;
}


class RemoveInvalid
{
public:
    RemoveInvalid(HierarchyObject& hierObj) :
        hierObj_(hierObj) {}

    ~RemoveInvalid()
    {
        FileSystemObject::removeEmptyNonRec(hierObj_);
    }

private:
    HierarchyObject& hierObj_;
};


template <bool deleteOnly> //"true" if files deletion shall happen only
class SyncProcess::SyncRecursively
{
public:
    SyncRecursively(const SyncProcess* const syncProc, const SyncProcess::DeletionHandling& delHandling) :
        syncProc_(syncProc),
        delHandling_(delHandling) {}

    void execute(HierarchyObject& hierObj)
    {
        //enforce removal of invalid entries (where both sides are empty)
        RemoveInvalid dummy(hierObj); //non-recursive

        //synchronize files:
        for (HierarchyObject::SubFileMapping::iterator i = hierObj.subFiles.begin(); i != hierObj.subFiles.end(); ++i)
        {
            if (    ( deleteOnly &&  deletionImminent(*i)) ||
                    (!deleteOnly && !deletionImminent(*i)))
            {
                while (true)
                {
                    try
                    {
                        syncProc_->synchronizeFile(*i, delHandling_);
                        break;
                    }
                    catch (FileError& error)
                    {
                        //User abort when copying files or moving files/directories into custom deletion directory:
                        //windows build: abort if requested, don't show error message if cancelled by user!
                        //linux build: this refresh is not necessary, because user abort triggers an AbortThisProcess() exception without a FileError()
                        syncProc_->statusUpdater->requestUiRefresh(true);

                        ErrorHandler::Response rv = syncProc_->statusUpdater->reportError(error.show());
                        if ( rv == ErrorHandler::IGNORE_ERROR)
                            break;
                        else if (rv == ErrorHandler::RETRY)
                            ;   //continue with loop
                        else
                            throw std::logic_error("Programming Error: Unknown return value!");
                    }
                }
            }
        }

        //synchronize folders:
        for (HierarchyObject::SubDirMapping::iterator i = hierObj.subDirs.begin(); i != hierObj.subDirs.end(); ++i)
        {
            if (deleteOnly) //no need, to process folders more than once!
            {
                while (true)
                {
                    try
                    {
                        syncProc_->synchronizeFolder(*i, delHandling_);
                        break;
                    }
                    catch (FileError& error)
                    {
                        //User abort when copying files or moving files/directories into custom deletion directory:
                        //windows build: abort if requested, don't show error message if cancelled by user!
                        //linux build: this refresh is not necessary, because user abort triggers an AbortThisProcess() exception without a FileError()
                        syncProc_->statusUpdater->requestUiRefresh(true);

                        ErrorHandler::Response rv = syncProc_->statusUpdater->reportError(error.show());
                        if (rv == ErrorHandler::IGNORE_ERROR)
                            break;
                        else if (rv == ErrorHandler::RETRY)
                            ;  //continue with loop
                        else
                            throw std::logic_error("Programming Error: Unknown return value!");
                    }
                }
            }

            //recursive synchronization:
            //if (!i->isEmpty()) -> not necessary, deleted folders have no sub-objects at this point
            execute(*i);
        }
    }

private:
    const SyncProcess* const syncProc_;
    const SyncProcess::DeletionHandling& delHandling_;
};


template <typename Function>
inline
void tryReportingError(StatusHandler& handler, Function cmd)
{
    while (true)
    {
        try
        {
            cmd();
            break;
        }
        catch (FileError& error)
        {
            handler.requestUiRefresh(); //may throw!

            ErrorHandler::Response rv = handler.reportError(error.show()); //may throw!
            if ( rv == ErrorHandler::IGNORE_ERROR)
                break;
            else if (rv == ErrorHandler::RETRY)
                ;   //continue with loop
            else
                throw std::logic_error("Programming Error: Unknown return value!");
        }
    }
}


class UpdateDatabase
{
public:
    UpdateDatabase(const BaseDirMapping& baseMap, StatusHandler& statusHandler) :
        baseMap_(baseMap),
        statusHandler_(statusHandler) {}

    //update sync database after synchronization is finished
    void updateNow()
    {
        //these calls may throw!
        tryReportingError(statusHandler_, boost::bind(saveToDisk, boost::cref(baseMap_), LEFT_SIDE,  baseMap_.getDBFilename<LEFT_SIDE>()));
        tryReportingError(statusHandler_, boost::bind(saveToDisk, boost::cref(baseMap_), RIGHT_SIDE, baseMap_.getDBFilename<RIGHT_SIDE>()));
    };
    //_("You can ignore the error to skip current folder pair."));

private:
    const BaseDirMapping& baseMap_;
    StatusHandler& statusHandler_;
};


//avoid data loss when source directory doesn't (temporarily?) exist anymore AND user chose to ignore errors (else we wouldn't arrive here)
bool dataLossPossible(const Zstring& dirName, const SyncStatistics& folderPairStat)
{
    return folderPairStat.getCreate() +  folderPairStat.getOverwrite() + folderPairStat.getConflict() == 0 &&
           folderPairStat.getDelete() > 0 && //deletions only... (respect filtered items!)
           !dirName.empty() && !FreeFileSync::dirExists(dirName);
}


void SyncProcess::startSynchronizationProcess(const std::vector<FolderPairSyncCfg>& syncConfig, FolderComparison& folderCmp)
{
#ifndef __WXDEBUG__
    wxLogNull noWxLogs; //prevent wxWidgets logging
#endif

    //PERF_START;


    //inform about the total amount of data that will be processed from now on
    const SyncStatistics statisticsTotal(folderCmp);

    //keep at beginning so that all gui elements are initialized properly
    statusUpdater->initNewProcess(statisticsTotal.getCreate() + statisticsTotal.getOverwrite() + statisticsTotal.getDelete(),
                                  globalFunctions::convertToSigned(statisticsTotal.getDataToProcess()),
                                  StatusHandler::PROCESS_SYNCHRONIZING);

    //-------------------some basic checks:------------------------------------------

    if (syncConfig.size() != folderCmp.size())
        throw std::logic_error("Programming Error: Contract violation!");

    for (FolderComparison::const_iterator j = folderCmp.begin(); j != folderCmp.end(); ++j)
    {
        const SyncStatistics statisticsFolderPair(*j);
        const FolderPairSyncCfg& folderPairCfg = syncConfig[j - folderCmp.begin()];

        if (statisticsFolderPair.getOverwrite() + statisticsFolderPair.getDelete() > 0)
        {
            //test existence of Recycle Bin
            if (folderPairCfg.handleDeletion == FreeFileSync::MOVE_TO_RECYCLE_BIN && !FreeFileSync::recycleBinExists())
            {
                statusUpdater->reportFatalError(_("Unable to initialize Recycle Bin!"));
                return; //should be obsolete!
            }

            if (folderPairCfg.handleDeletion == FreeFileSync::MOVE_TO_CUSTOM_DIRECTORY)
            {
                //check if user-defined directory for deletion was specified
                if (FreeFileSync::getFormattedDirectoryName(folderPairCfg.custDelFolder.c_str()).empty())
                {
                    statusUpdater->reportFatalError(_("User-defined directory for deletion was not specified!"));
                    return; //should be obsolete!
                }
            }
        }

        //avoid data loss when source directory doesn't (temporarily?) exist anymore AND user chose to ignore errors(else we wouldn't arrive here)
        if (dataLossPossible(j->getBaseDir<LEFT_SIDE>(), statisticsFolderPair))
        {
            statusUpdater->reportFatalError(wxString(_("Source directory does not exist anymore:")) + wxT("\n\"") + zToWx(j->getBaseDir<LEFT_SIDE>()) + wxT("\""));
            return; //should be obsolete!
        }
        if (dataLossPossible(j->getBaseDir<RIGHT_SIDE>(), statisticsFolderPair))
        {
            statusUpdater->reportFatalError(wxString(_("Source directory does not exist anymore:")) + wxT("\n\"") + zToWx(j->getBaseDir<RIGHT_SIDE>()) + wxT("\"") );
            return; //should be obsolete!
        }

        //check if more than 50% of total number of files/dirs are to be created/overwritten/deleted
        if (significantDifferenceDetected(statisticsFolderPair))
        {
            statusUpdater->reportWarning(wxString(_("Significant difference detected:")) + wxT("\n") +
                                         zToWx(j->getBaseDir<LEFT_SIDE>()) + wxT(" <-> ") + wxT("\n") +
                                         zToWx(j->getBaseDir<RIGHT_SIDE>()) + wxT("\n\n") +
                                         _("More than 50% of the total number of files will be copied or deleted!"),
                                         m_warnings.warningSignificantDifference);
        }

        //check for sufficient free diskspace in left directory
        const std::pair<wxLongLong, wxLongLong> spaceNeeded = freeDiskSpaceNeeded(*j, folderPairCfg.handleDeletion);

        wxLongLong freeDiskSpaceLeft;
        if (wxGetDiskSpace(zToWx(j->getBaseDir<LEFT_SIDE>()), NULL, &freeDiskSpaceLeft))
        {
            if (freeDiskSpaceLeft < spaceNeeded.first)
                statusUpdater->reportWarning(wxString(_("Not enough free disk space available in:")) + wxT("\n") +
                                             wxT("\"") + zToWx(j->getBaseDir<LEFT_SIDE>()) + wxT("\"\n\n") +
                                             _("Total required free disk space:") + wxT(" ") + formatFilesizeToShortString(spaceNeeded.first) + wxT("\n") +
                                             _("Free disk space available:") + wxT(" ") + formatFilesizeToShortString(freeDiskSpaceLeft),
                                             m_warnings.warningNotEnoughDiskSpace);
        }

        //check for sufficient free diskspace in right directory
        wxLongLong freeDiskSpaceRight;
        if (wxGetDiskSpace(zToWx(j->getBaseDir<RIGHT_SIDE>()), NULL, &freeDiskSpaceRight))
        {
            if (freeDiskSpaceRight < spaceNeeded.second)
                statusUpdater->reportWarning(wxString(_("Not enough free disk space available in:")) + wxT("\n") +
                                             wxT("\"") + zToWx(j->getBaseDir<RIGHT_SIDE>()) + wxT("\"\n\n") +
                                             _("Total required free disk space:") + wxT(" ") + formatFilesizeToShortString(spaceNeeded.second) + wxT("\n") +
                                             _("Free disk space available:") + wxT(" ") + formatFilesizeToShortString(freeDiskSpaceRight),
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
        //loop through all directory pairs
        assert(syncConfig.size() == folderCmp.size());
        for (FolderComparison::iterator j = folderCmp.begin(); j != folderCmp.end(); ++j)
        {
            const FolderPairSyncCfg& folderPairCfg = syncConfig[j - folderCmp.begin()];

            //generate name of alternate deletion directory (unique for session AND folder pair)
            const DeletionHandling currentDelHandling(folderPairCfg.handleDeletion, folderPairCfg.custDelFolder);

//------------------------------------------------------------------------------------------
            //execute synchronization recursively

            //loop through all files twice; reason: first delete, then copy
            SyncRecursively<true>( this, currentDelHandling).execute(*j);
            SyncRecursively<false>(this, currentDelHandling).execute(*j);
//------------------------------------------------------------------------------------------

            //update synchronization database (automatic sync only)
            if (folderPairCfg.updateSyncDB)
            {
                UpdateDatabase syncDB(*j, *statusUpdater);
                statusUpdater->updateStatusText(wxToZ(_("Generating database...")));
                statusUpdater->forceUiRefresh();
                syncDB.updateNow();
            }
        }
    }
    catch (const std::exception& e)
    {
        statusUpdater->reportFatalError(wxString::FromAscii(e.what()));
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
void SyncProcess::copyFileUpdating(const Zstring& source, const Zstring& target, const wxULongLong& totalBytesToCpy) const
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

        if (m_verifyCopiedFiles) //verify if data was copied correctly
            verifyFileCopy(source, target);
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


//--------------------- data verification -------------------------

//callback functionality for status updates while verifying
class VerifyCallback
{
public:
    virtual ~VerifyCallback() {}
    virtual void updateStatus() = 0;
};


void verifyFiles(const Zstring& source, const Zstring& target, VerifyCallback* callback) // throw (FileError)
{
    const unsigned int BUFFER_SIZE = 512 * 1024; //512 kb seems to be the perfect buffer size
    static boost::scoped_array<unsigned char> memory1(new unsigned char[BUFFER_SIZE]);
    static boost::scoped_array<unsigned char> memory2(new unsigned char[BUFFER_SIZE]);

#ifdef FFS_WIN
    wxFile file1(source.c_str(), wxFile::read); //don't use buffered file input for verification!
#elif defined FFS_LINUX
    wxFile file1(::open(source.c_str(), O_RDONLY)); //utilize UTF-8 filename
#endif
    if (!file1.IsOpened())
        throw FileError(wxString(_("Error opening file:")) + wxT(" \"") + zToWx(source) + wxT("\""));

#ifdef FFS_WIN
    wxFile file2(target.c_str(), wxFile::read); //don't use buffered file input for verification!
#elif defined FFS_LINUX
    wxFile file2(::open(target.c_str(), O_RDONLY)); //utilize UTF-8 filename
#endif
    if (!file2.IsOpened()) //NO cleanup necessary for (wxFile) file1
        throw FileError(wxString(_("Error opening file:")) + wxT(" \"") + zToWx(target) + wxT("\""));

    do
    {
        const size_t length1 = file1.Read(memory1.get(), BUFFER_SIZE);
        if (file1.Error()) throw FileError(wxString(_("Error reading file:")) + wxT(" \"") + zToWx(source) + wxT("\""));

        const size_t length2 = file2.Read(memory2.get(), BUFFER_SIZE);
        if (file2.Error()) throw FileError(wxString(_("Error reading file:")) + wxT(" \"") + zToWx(target) + wxT("\""));

        if (length1 != length2 || ::memcmp(memory1.get(), memory2.get(), length1) != 0)
        {
            const wxString errorMsg = wxString(_("Data verification error: Source and target file have different content!")) + wxT("\n");
            throw FileError(errorMsg + wxT("\"") + zToWx(source) + wxT("\" -> \n\"") + zToWx(target) + wxT("\""));
        }

        //send progress updates
        callback->updateStatus();
    }
    while (!file1.Eof());

    if (!file2.Eof())
    {
        const wxString errorMsg = wxString(_("Data verification error: Source and target file have different content!")) + wxT("\n");
        throw FileError(errorMsg + wxT("\"") + zToWx(source) + wxT("\" -> \n\"") + zToWx(target) + wxT("\""));
    }
}


class VerifyStatusUpdater : public VerifyCallback
{
public:
    VerifyStatusUpdater(StatusHandler* statusHandler) : m_statusHandler(statusHandler) {}

    virtual void updateStatus()
    {
        m_statusHandler->requestUiRefresh(); //trigger display refresh
    }

private:
    StatusHandler* m_statusHandler;
};


void SyncProcess::verifyFileCopy(const Zstring& source, const Zstring& target) const
{
    Zstring statusText = txtVerifying;
    statusText.Replace(DefaultStr("%x"), target, false);
    statusUpdater->updateStatusText(statusText);
    statusUpdater->requestUiRefresh(); //trigger display refresh

    VerifyStatusUpdater callback(statusUpdater);
    try
    {
        verifyFiles(source, target, &callback);
    }
    catch (FileError& error)
    {
        switch (statusUpdater->reportError(error.show()))
        {
        case ErrorHandler::IGNORE_ERROR:
            break;
        case ErrorHandler::RETRY:
            verifyFileCopy(source, target);
            break;
        }
    }
}






