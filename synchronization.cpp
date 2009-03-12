#include "synchronization.h"
#include <wx/intl.h>
#include <wx/msgdlg.h>
#include <wx/log.h>
#include "library/multithreading.h"
#include "library/resources.h"
#include "algorithm.h"

#ifdef FFS_WIN
#include <wx/msw/wrapwin.h> //includes "windows.h"
#elif defined FFS_LINUX
#include <utime.h>
#endif  // FFS_LINUX

using namespace FreeFileSync;


inline
SyncConfiguration::Direction getSyncDirection(const CompareFilesResult cmpResult, const SyncConfiguration& config)
{
    switch (cmpResult)
    {
    case FILE_LEFT_SIDE_ONLY:
        return config.exLeftSideOnly;
        break;

    case FILE_RIGHT_SIDE_ONLY:
        return config.exRightSideOnly;
        break;

    case FILE_RIGHT_NEWER:
        return config.rightNewer;
        break;

    case FILE_LEFT_NEWER:
        return config.leftNewer;
        break;

    case FILE_DIFFERENT:
        return config.different;
        break;

    default:
        assert (false);
    }
    return SyncConfiguration::SYNC_DIR_NONE;
}


inline
bool getBytesToTransfer(int& objectsToCreate,
                        int& objectsToOverwrite,
                        int& objectsToDelete,
                        double& dataToProcess,
                        const FileCompareLine& fileCmpLine,
                        const SyncConfiguration& config)
{   //false if nothing has to be done

    objectsToCreate    = 0;  //always initialize variables
    objectsToOverwrite = 0;
    objectsToDelete    = 0;
    dataToProcess      = 0;

    //do not add filtered entries
    if (!fileCmpLine.selectedForSynchronization)
        return false;


    switch (fileCmpLine.cmpResult)
    {
    case FILE_LEFT_SIDE_ONLY:
        //get data to process
        switch (getSyncDirection(fileCmpLine.cmpResult, config))
        {
        case SyncConfiguration::SYNC_DIR_LEFT:   //delete file on left
            dataToProcess   = 0;
            objectsToDelete = 1;
            break;
        case SyncConfiguration::SYNC_DIR_RIGHT:  //copy from left to right
            dataToProcess = fileCmpLine.fileDescrLeft.fileSize.ToDouble();
            objectsToCreate = 1;
            break;
        case SyncConfiguration::SYNC_DIR_NONE:
            return false;
        }
        break;

    case FILE_RIGHT_SIDE_ONLY:
        switch (getSyncDirection(fileCmpLine.cmpResult, config))
        {
        case SyncConfiguration::SYNC_DIR_LEFT:   //copy from right to left
            dataToProcess   = fileCmpLine.fileDescrRight.fileSize.ToDouble();;
            objectsToCreate = 1;
            break;
        case SyncConfiguration::SYNC_DIR_RIGHT:  //delete file on right
            dataToProcess   = 0;
            objectsToDelete = 1;
            break;
        case SyncConfiguration::SYNC_DIR_NONE:
            return false;
        }
        break;

    case FILE_LEFT_NEWER:
    case FILE_RIGHT_NEWER:
    case FILE_DIFFERENT:
        //get data to process
        switch (getSyncDirection(fileCmpLine.cmpResult, config))
        {
        case SyncConfiguration::SYNC_DIR_LEFT:   //copy from right to left
            dataToProcess = fileCmpLine.fileDescrRight.fileSize.ToDouble();
            objectsToOverwrite = 1;
            break;
        case SyncConfiguration::SYNC_DIR_RIGHT:  //copy from left to right
            dataToProcess = fileCmpLine.fileDescrLeft.fileSize.ToDouble();
            objectsToOverwrite = 1;
            break;
        case SyncConfiguration::SYNC_DIR_NONE:
            return false;
        }
        break;

    case FILE_EQUAL:
        return false;
    default:
        assert(false);
        return false;
    };

    return true;
}


void FreeFileSync::calcTotalBytesToSync(const FileCompareResult& fileCmpResult,
                                        const SyncConfiguration& config,
                                        int& objectsToCreate,
                                        int& objectsToOverwrite,
                                        int& objectsToDelete,
                                        double& dataToProcess)
{
    objectsToCreate    = 0;
    objectsToOverwrite = 0;
    objectsToDelete    = 0;
    dataToProcess      = 0;

    int toCreate    = 0;
    int toOverwrite = 0;
    int toDelete    = 0;
    double data     = 0;

    for (FileCompareResult::const_iterator i = fileCmpResult.begin(); i != fileCmpResult.end(); ++i)
    {   //only sum up sizes of files AND directories
        if (getBytesToTransfer(toCreate, toOverwrite, toDelete, data, *i, config))
        {
            objectsToCreate    += toCreate;
            objectsToOverwrite += toOverwrite;
            objectsToDelete    += toDelete;
            dataToProcess      += data;
        }
    }
}


bool FreeFileSync::synchronizationNeeded(const FileCompareResult& fileCmpResult, const SyncConfiguration& config)
{
    int objectsToCreate    = 0;
    int objectsToOverwrite = 0;
    int objectsToDelete    = 0;
    double dataToProcess   = 0;
    FreeFileSync::calcTotalBytesToSync(fileCmpResult,
                                       config,
                                       objectsToCreate,
                                       objectsToOverwrite,
                                       objectsToDelete,
                                       dataToProcess);
    return objectsToCreate + objectsToOverwrite + objectsToDelete != 0;
}


//test if more then 50% of files will be deleted/overwritten
bool significantDifferenceDetected(const FileCompareResult& fileCmpResult, const SyncConfiguration& config)
{
    int objectsToCreate    = 0;
    int objectsToOverwrite = 0;
    int objectsToDelete    = 0;
    double dataToProcess   = 0;
    FreeFileSync::calcTotalBytesToSync(fileCmpResult,
                                       config,
                                       objectsToCreate,
                                       objectsToOverwrite,
                                       objectsToDelete,
                                       dataToProcess);

    const int changedFiles = objectsToOverwrite + objectsToDelete;

    return changedFiles >= 10 && changedFiles > .5 * fileCmpResult.size();
}


SyncProcess::SyncProcess(const bool useRecycler, bool& warningSignificantDifference, StatusHandler* handler) :
        useRecycleBin(useRecycler),
        m_warningSignificantDifference(warningSignificantDifference),
        statusUpdater(handler),
        txtCopyingFile(_("Copying file %x to %y")),
        txtOverwritingFile(_("Copying file %x overwriting %y")),
        txtCreatingFolder(_("Creating folder %x")),
        txtDeletingFile(_("Deleting file %x")),
        txtDeletingFolder(_("Deleting folder %x"))
{
    txtCopyingFile.Replace(    wxT("%x"), wxT("\n\"%x\""), false);
    txtCopyingFile.Replace(    wxT("%y"), wxT("\n\"%y\""), false);
    txtOverwritingFile.Replace(wxT("%x"), wxT("\n\"%x\""), false);
    txtOverwritingFile.Replace(wxT("%y"), wxT("\n\"%y\""), false);
    txtCreatingFolder.Replace( wxT("%x"), wxT("\n\"%x\""), false);
    txtDeletingFile.Replace(   wxT("%x"), wxT("\n\"%x\""), false);
    txtDeletingFolder.Replace( wxT("%x"), wxT("\n\"%x\""), false);
}


void copyfileMultithreaded(const Zstring& source, const Zstring& target, StatusHandler* updateClass);


inline
bool SyncProcess::synchronizeFile(const FileCompareLine& cmpLine, const SyncConfiguration& config)
{   //return false if nothing had to be done

    if (!cmpLine.selectedForSynchronization) return false;

    Zstring statusText;
    Zstring target;

    //synchronize file:
    switch (cmpLine.cmpResult)
    {
    case FILE_LEFT_SIDE_ONLY:
        switch (config.exLeftSideOnly)
        {
        case SyncConfiguration::SYNC_DIR_LEFT:   //delete files on left
            statusText = txtDeletingFile;
            statusText.Replace(wxT("%x"), cmpLine.fileDescrLeft.fullName, false);
            statusUpdater->updateStatusText(statusText);

            removeFile(cmpLine.fileDescrLeft.fullName, useRecycleBin);
            break;
        case SyncConfiguration::SYNC_DIR_RIGHT:  //copy files to right
            target = cmpLine.fileDescrRight.directory + cmpLine.fileDescrLeft.relativeName;

            statusText = txtCopyingFile;
            statusText.Replace(wxT("%x"), cmpLine.fileDescrLeft.fullName, false);
            statusText.Replace(wxT("%y"), target, false);
            statusUpdater->updateStatusText(statusText);

            copyfileMultithreaded(cmpLine.fileDescrLeft.fullName, target, statusUpdater);
            break;
        case SyncConfiguration::SYNC_DIR_NONE:
            return false;
        default:
            assert (false);
        }
        break;

    case FILE_RIGHT_SIDE_ONLY:
        switch (config.exRightSideOnly)
        {
        case SyncConfiguration::SYNC_DIR_LEFT:   //copy files to left
            target = cmpLine.fileDescrLeft.directory + cmpLine.fileDescrRight.relativeName;

            statusText = txtCopyingFile;
            statusText.Replace(wxT("%x"), cmpLine.fileDescrRight.fullName, false);
            statusText.Replace(wxT("%y"), target, false);
            statusUpdater->updateStatusText(statusText);

            copyfileMultithreaded(cmpLine.fileDescrRight.fullName, target, statusUpdater);
            break;
        case SyncConfiguration::SYNC_DIR_RIGHT:  //delete files on right
            statusText = txtDeletingFile;
            statusText.Replace(wxT("%x"), cmpLine.fileDescrRight.fullName, false);
            statusUpdater->updateStatusText(statusText);

            removeFile(cmpLine.fileDescrRight.fullName, useRecycleBin);
            break;
        case SyncConfiguration::SYNC_DIR_NONE:
            return false;
        default:
            assert (false);
        }
        break;

    case FILE_LEFT_NEWER:
    case FILE_RIGHT_NEWER:
    case FILE_DIFFERENT:
        switch (getSyncDirection(cmpLine.cmpResult, config))
        {
        case SyncConfiguration::SYNC_DIR_LEFT:   //copy from right to left
            statusText = txtOverwritingFile;
            statusText.Replace(wxT("%x"), cmpLine.fileDescrRight.fullName, false);
            statusText.Replace(wxT("%y"), cmpLine.fileDescrLeft.fullName, false);
            statusUpdater->updateStatusText(statusText);

            removeFile(cmpLine.fileDescrLeft.fullName, useRecycleBin);  //only used if switch activated by user, else file is simply deleted
            copyfileMultithreaded(cmpLine.fileDescrRight.fullName, cmpLine.fileDescrLeft.fullName, statusUpdater);
            break;
        case SyncConfiguration::SYNC_DIR_RIGHT:  //copy from left to right
            statusText = txtOverwritingFile;
            statusText.Replace(wxT("%x"), cmpLine.fileDescrLeft.fullName, false);
            statusText.Replace(wxT("%y"), cmpLine.fileDescrRight.fullName, false);
            statusUpdater->updateStatusText(statusText);

            removeFile(cmpLine.fileDescrRight.fullName, useRecycleBin);  //only used if switch activated by user, else file is simply deleted
            copyfileMultithreaded(cmpLine.fileDescrLeft.fullName, cmpLine.fileDescrRight.fullName, statusUpdater);
            break;
        case SyncConfiguration::SYNC_DIR_NONE:
            return false;
        default:
            assert (false);
        }
        break;

    case FILE_EQUAL:
        return false;

    default:
        assert (false);
    }
    return true;
}


inline
bool SyncProcess::synchronizeFolder(const FileCompareLine& cmpLine, const SyncConfiguration& config)
{   //false if nothing was to be done
    assert (statusUpdater);

    if (!cmpLine.selectedForSynchronization) return false;

    Zstring statusText;
    Zstring target;

    //synchronize folders:
    switch (cmpLine.cmpResult)
    {
    case FILE_LEFT_SIDE_ONLY:
        switch (config.exLeftSideOnly)
        {
        case SyncConfiguration::SYNC_DIR_LEFT:   //delete folders on left
            statusText = txtDeletingFolder;
            statusText.Replace(wxT("%x"), cmpLine.fileDescrLeft.fullName, false);
            statusUpdater->updateStatusText(statusText);

            removeDirectory(cmpLine.fileDescrLeft.fullName, useRecycleBin);
            break;
        case SyncConfiguration::SYNC_DIR_RIGHT:  //create folders on right
            target = cmpLine.fileDescrRight.directory + cmpLine.fileDescrLeft.relativeName;

            statusText = txtCreatingFolder;
            statusText.Replace(wxT("%x"), target, false);
            statusUpdater->updateStatusText(statusText);

            //some check to catch the error that directory on source has been deleted externally after "compare"...
            if (!wxDirExists(cmpLine.fileDescrLeft.fullName))
                throw FileError(Zstring(_("Error: Source directory does not exist anymore:")) + wxT("\n\"") + cmpLine.fileDescrLeft.fullName + wxT("\""));
            createDirectory(target);
            copyFolderAttributes(cmpLine.fileDescrLeft.fullName, target);
            break;
        case SyncConfiguration::SYNC_DIR_NONE:
            return false;
        default:
            assert (false);
        }
        break;

    case FILE_RIGHT_SIDE_ONLY:
        switch (config.exRightSideOnly)
        {
        case SyncConfiguration::SYNC_DIR_LEFT:   //create folders on left
            target = cmpLine.fileDescrLeft.directory + cmpLine.fileDescrRight.relativeName;

            statusText = txtCreatingFolder;
            statusText.Replace(wxT("%x"), target, false);
            statusUpdater->updateStatusText(statusText);

            //some check to catch the error that directory on source has been deleted externally after "compare"...
            if (!wxDirExists(cmpLine.fileDescrRight.fullName))
                throw FileError(Zstring(_("Error: Source directory does not exist anymore:")) + wxT("\n\"") + cmpLine.fileDescrRight.fullName + wxT("\""));
            createDirectory(target);
            copyFolderAttributes(cmpLine.fileDescrRight.fullName, target);
            break;
        case SyncConfiguration::SYNC_DIR_RIGHT:  //delete folders on right
            statusText = txtDeletingFolder;
            statusText.Replace(wxT("%x"), cmpLine.fileDescrRight.fullName, false);
            statusUpdater->updateStatusText(statusText);

            removeDirectory(cmpLine.fileDescrRight.fullName, useRecycleBin);
            break;
        case SyncConfiguration::SYNC_DIR_NONE:
            return false;
        default:
            assert (false);
        }
        break;

    case FILE_EQUAL:
        return false;
    case FILE_RIGHT_NEWER:
    case FILE_LEFT_NEWER:
    case FILE_DIFFERENT:
    default:
        assert (false);
    }
    return true;
}


inline
bool deletionImminent(const FileCompareLine& line, const SyncConfiguration& config)
{   //test if current sync-line will result in deletion of files    -> used to avoid disc space bottlenecks
    if (    line.cmpResult == FILE_LEFT_SIDE_ONLY && config.exLeftSideOnly == SyncConfiguration::SYNC_DIR_LEFT ||
            line.cmpResult == FILE_RIGHT_SIDE_ONLY && config.exRightSideOnly == SyncConfiguration::SYNC_DIR_RIGHT)
        return true;
    else
        return false;
}


class RemoveAtExit //this class ensures, that the result of the method below is ALWAYS written on exit, even if exceptions are thrown!
{
public:
    RemoveAtExit(FileCompareResult& grid) :
            gridToWrite(grid) {}

    ~RemoveAtExit()
    {
        removeRowsFromVector(gridToWrite, rowsProcessed);
    }

    void removeRow(int nr)
    {
        rowsProcessed.insert(nr);
    }

private:
    FileCompareResult& gridToWrite;
    std::set<int> rowsProcessed;
};


//synchronizes while processing rows in grid and returns all rows that have not been synced
void SyncProcess::startSynchronizationProcess(FileCompareResult& grid, const SyncConfiguration& config) throw(AbortThisProcess)
{
    assert (statusUpdater);

#ifndef __WXDEBUG__
    wxLogNull noWxLogs; //prevent wxWidgets logging
#endif

    //some basic checks:
    //test existence of Recycle Bin
    if (useRecycleBin && !FreeFileSync::recycleBinExists())
    {
        statusUpdater->reportFatalError(_("Unable to initialize Recycle Bin!"));
        return; //should be obsolete!
    }

    //check if more than 50% of files/dirs are to be overwritten/deleted
    if (m_warningSignificantDifference) //test if check should be executed
    {
        if (significantDifferenceDetected(grid, config))
        {
            bool dontShowAgain = false;
            statusUpdater->reportWarning(_("Significant difference detected: More than 50% of files will be overwritten/deleted!"),
                                         dontShowAgain);
            m_warningSignificantDifference = !dontShowAgain;
        }
    }


    RemoveAtExit markForRemoval(grid); //ensure that grid is always written to, even if method is exitted via exceptions

    //inform about the total amount of data that will be processed from now on
    int objectsToCreate    = 0;
    int objectsToOverwrite = 0;
    int objectsToDelete    = 0;
    double dataToProcess   = 0;
    calcTotalBytesToSync(grid,
                         config,
                         objectsToCreate,
                         objectsToOverwrite,
                         objectsToDelete,
                         dataToProcess);

    statusUpdater->initNewProcess(objectsToCreate + objectsToOverwrite + objectsToDelete, dataToProcess, StatusHandler::PROCESS_SYNCHRONIZING);

    try
    {
        // it should never happen, that a directory on left side has same name as file on right side. startCompareProcess() should take care of this
        // and split into two "exists on one side only" cases
        // Note: this case is not handled by this tool as this is considered to be a bug and must be solved by the user

        //synchronize folders first; advantage: in case of deletions the whole folder is moved to recycle bin instead of single files
        for (FileCompareResult::const_iterator i = grid.begin(); i != grid.end(); ++i)
        {
            if (    i->fileDescrLeft.objType == FileDescrLine::TYPE_DIRECTORY ||
                    i->fileDescrRight.objType == FileDescrLine::TYPE_DIRECTORY)
            {

                while (true)
                {   //trigger display refresh
                    statusUpdater->requestUiRefresh();

                    try
                    {
                        if (synchronizeFolder(*i, config))
                            //progress indicator update
                            //indicator is updated only if directory is sync'ed correctly (and if some work was done)!
                            statusUpdater->updateProcessedData(1, 0);  //each call represents one processed file/directory

                        markForRemoval.removeRow(i - grid.begin());
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
        bool deleteLoop = true;
        for (int k = 0; k < 2; ++k) //loop over all files twice; reason: first delete, then copy (no sorting necessary: performance)
        {
            deleteLoop = !k;   //-> first loop: delete files, second loop: copy files

            for (FileCompareResult::const_iterator i = grid.begin(); i != grid.end(); ++i)
            {
                if (    i->fileDescrLeft.objType == FileDescrLine::TYPE_FILE ||
                        i->fileDescrRight.objType == FileDescrLine::TYPE_FILE)
                {
                    if (    deleteLoop && deletionImminent(*i, config) ||
                            !deleteLoop && !deletionImminent(*i, config))
                    {
                        while (true)
                        {    //trigger display refresh
                            statusUpdater->requestUiRefresh();

                            try
                            {
                                if (synchronizeFile(*i, config))
                                {
                                    //progress indicator update
                                    //indicator is updated only if file is sync'ed correctly (and if some sync was done)!
                                    int objectsToCreate    = 0;
                                    int objectsToOverwrite = 0;
                                    int objectsToDelete    = 0;
                                    double dataToProcess   = 0;

                                    if (getBytesToTransfer(objectsToCreate,
                                                           objectsToOverwrite,
                                                           objectsToDelete,
                                                           dataToProcess,
                                                           *i,
                                                           config))  //update status if some work was done (answer is always "yes" in this context)
                                        statusUpdater->updateProcessedData(objectsToCreate + objectsToOverwrite + objectsToDelete, dataToProcess);
                                }

                                markForRemoval.removeRow(i - grid.begin());
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
    catch (const RuntimeException& theException)
    {
        statusUpdater->reportFatalError(theException.show().c_str());
        return; //should be obsolete!
    }
}


//###########################################################################################


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
        //create folders first (see http://sourceforge.net/tracker/index.php?func=detail&aid=2628943&group_id=234430&atid=1093080)
        try
        {
            const Zstring targetDir = target.BeforeLast(GlobalResources::FILE_NAME_SEPARATOR);
            if (!wxDirExists(targetDir.c_str()))
            {
                FreeFileSync::createDirectory(targetDir);
            }
        }
        catch (FileError& e)
        {
            success = false;
            errorMessage = e.show();
            return;
        }


#ifdef FFS_WIN
//const DWORD COPY_FILE_COPY_SYMLINK = 0x00000800;

        if (!CopyFileEx( //same performance as CopyFile()
                    source.c_str(),
                    target.c_str(),
                    NULL,
                    NULL,
                    NULL,
                    COPY_FILE_FAIL_IF_EXISTS))
        {
            success = false;
            errorMessage = Zstring(_("Error copying file:")) + wxT("\n\"") + source +  wxT("\" -> \"") + target + wxT("\"");
            errorMessage += Zstring(wxT("\n\n"));
            errorMessage += FreeFileSync::getLastErrorFormatted(); //some additional windows error information
            return;
        }

#elif defined FFS_LINUX //copying files with Linux does not preserve the modification time => adapt after copying
        if (!wxCopyFile(source.c_str(), target.c_str(), false)) //abort if file exists
        {
            success = false;
            errorMessage = Zstring(_("Error copying file:")) + wxT("\n\"") + source +  wxT("\" -> \"") + target + wxT("\"");
            return;
        }

        struct stat fileInfo;
        if (stat(source.c_str(), &fileInfo) != 0) //read modification time from source file
        {
            success = false;
            errorMessage = Zstring(_("Could not retrieve file info for:")) + wxT("\n\"") + source + wxT("\"");
            return;
        }

        utimbuf newTimes;
        newTimes.actime  = fileInfo.st_mtime;
        newTimes.modtime = fileInfo.st_mtime;

        if (utime(target.c_str(), &newTimes) != 0)
        {
            success = false;
            errorMessage = Zstring(_("Error changing modification time:")) + wxT("\n\"") + target + wxT("\"");
            return;
        }
#endif  // FFS_LINUX

        success = true;
    }
};


void copyfileMultithreaded(const Zstring& source, const Zstring& target, StatusHandler* updateClass)
{
    static UpdateWhileCopying copyAndUpdate; //single instantiation: thread enters wait phase after each execution

    copyAndUpdate.waitUntilReady();

    //longRunner is called from thread, but no mutex needed here, since thread is in waiting state!
    copyAndUpdate.source = source;
    copyAndUpdate.target = target;

    copyAndUpdate.execute(updateClass);

    //no mutex needed here since longRunner is finished
    if (!copyAndUpdate.success)
        throw FileError(copyAndUpdate.errorMessage);
}
