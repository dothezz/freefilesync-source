#include "synchronization.h"
#include <wx/intl.h>
#include <wx/msgdlg.h>
#include <wx/log.h>
#include "library/multithreading.h"

#ifdef FFS_LINUX
#include <utime.h>
#endif  // FFS_LINUX

using namespace FreeFileSync;


SyncProcess::SyncProcess(bool useRecycler, bool lineBreakOnMessages, StatusHandler* handler) :
        useRecycleBin(useRecycler),
        statusUpdater(handler),
        txtCopyingFile(_("Copying file \"%x\" to \"%y\"")),
        txtOverwritingFile(_("Copying file \"%x\" overwriting \"%y\"")),
        txtCreatingFolder(_("Creating folder \"%x\"")),
        txtDeletingFile(_("Deleting file \"%x\"")),
        txtDeletingFolder(_("Deleting folder \"%x\""))
{
    if (lineBreakOnMessages)
        optionalLineBreak = wxT("\n");
}


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

void copyfileMultithreaded(const wxString& source, const wxString& target, StatusHandler* updateClass);


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
                throw FileError(wxString(_("Error: Source directory does not exist anymore:")) + wxT(" \"") + cmpLine.fileDescrLeft.fullName.c_str() + wxT("\""));
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
                throw FileError(wxString(_("Error: Source directory does not exist anymore:")) + wxT(" \"") + cmpLine.fileDescrRight.fullName.c_str() + wxT("\""));
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
    if (    (line.cmpResult == FILE_LEFT_SIDE_ONLY && config.exLeftSideOnly == SyncConfiguration::SYNC_DIR_LEFT) ||
            (line.cmpResult == FILE_RIGHT_SIDE_ONLY && config.exRightSideOnly == SyncConfiguration::SYNC_DIR_RIGHT))
        return true;
    else
        return false;
}


class AlwaysWriteToGrid //this class ensures, that the result of the method below is ALWAYS written on exit, even if exceptions are thrown!
{
public:
    AlwaysWriteToGrid(FileCompareResult& grid) :
            gridToWrite(grid)
    {}

    ~AlwaysWriteToGrid()
    {
        removeRowsFromVector(gridToWrite, rowsProcessed);
    }

    void rowProcessedSuccessfully(int nr)
    {
        rowsProcessed.insert(nr);
    }

private:
    FileCompareResult& gridToWrite;
    set<int> rowsProcessed;
};


//synchronizes while processing rows in grid and returns all rows that have not been synced
void SyncProcess::startSynchronizationProcess(FileCompareResult& grid, const SyncConfiguration& config) throw(AbortThisProcess)
{
    assert (statusUpdater);

#ifndef __WXDEBUG__
    wxLogNull noWxLogs; //prevent wxWidgets logging
#endif

    AlwaysWriteToGrid writeOutput(grid);  //ensure that grid is always written to, even if method is exitted via exceptions

    //inform about the total amount of data that will be processed from now on
    int objectsToCreate    = 0;
    int objectsToOverwrite = 0;
    int objectsToDelete    = 0;
    double dataToProcess   = 0;
    calcTotalBytesToSync(objectsToCreate,
                         objectsToOverwrite,
                         objectsToDelete,
                         dataToProcess,
                         grid,
                         config);

    statusUpdater->initNewProcess(objectsToCreate + objectsToOverwrite + objectsToDelete, dataToProcess, StatusHandler::PROCESS_SYNCHRONIZING);

    try
    {
        // it should never happen, that a directory on left side has same name as file on right side. startCompareProcess() should take care of this
        // and split into two "exists on one side only" cases
        // Note: this case is not handled by this tool as this is considered to be a bug and must be solved by the user

        //synchronize folders:
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
                            //indicator is updated only if directory is synched correctly  (and if some sync was done)!
                            statusUpdater->updateProcessedData(1, 0);  //each call represents one processed file/directory

                        writeOutput.rowProcessedSuccessfully(i - grid.begin());
                        break;
                    }
                    catch (FileError& error)
                    {
                        //if (updateClass) -> is mandatory
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

                                writeOutput.rowProcessedSuccessfully(i - grid.begin());
                                break;
                            }
                            catch (FileError& error)
                            {
                                //if (updateClass) -> is mandatory
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
        wxMessageBox(theException.show(), _("An exception occured!"), wxOK | wxICON_ERROR);
        return;
    }
}


//###########################################################################################


//handle execution of a method while updating the UI
class UpdateWhileCopying : public UpdateWhileExecuting
{
public:
    UpdateWhileCopying() {}
    ~UpdateWhileCopying() {}

    wxString source;
    wxString target;
    bool success;
    wxString errorMessage;

private:
    virtual void longRunner() //virtual method implementation
    {
        if (!wxCopyFile(source, target, false)) //abort if file exists
        {
            success = false;
            errorMessage = wxString(_("Error copying file:")) + wxT(" \"") + source +  wxT("\" -> \"") + target + wxT("\"");
            return;
        }

#ifdef FFS_LINUX //copying files with Linux does not preserve the modification time => adapt after copying
        struct stat fileInfo;
        if (stat(source.c_str(), &fileInfo) != 0) //read modification time from source file
        {
            success = false;
            errorMessage = wxString(_("Could not retrieve file info for:")) + wxT(" \"") + source + wxT("\"");
            return;
        }

        utimbuf newTimes;
        newTimes.actime  = fileInfo.st_mtime;
        newTimes.modtime = fileInfo.st_mtime;

        if (utime(target.c_str(), &newTimes) != 0)
        {
            success = false;
            errorMessage = wxString(_("Error changing modification time:")) + wxT(" \"") + target + wxT("\"");
            return;
        }
#endif  // FFS_LINUX

        success = true;
    }
};


void copyfileMultithreaded(const wxString& source, const wxString& target, StatusHandler* updateClass)
{
    static UpdateWhileCopying copyAndUpdate; //single instantiation: after each execution thread enters wait phase

    copyAndUpdate.waitUntilReady();

    //longRunner is called from thread, but no mutex needed here, since thread is in waiting state!
    copyAndUpdate.source = source;
    copyAndUpdate.target = target;

    copyAndUpdate.execute(updateClass);

    //no mutex needed here since longRunner is finished
    if (!copyAndUpdate.success)
        throw FileError(copyAndUpdate.errorMessage);
}


void FreeFileSync::calcTotalBytesToSync(int& objectsToCreate,
                                        int& objectsToOverwrite,
                                        int& objectsToDelete,
                                        double& dataToProcess,
                                        const FileCompareResult& fileCmpResult,
                                        const SyncConfiguration& config)
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
            objectsToCreate+=    toCreate;
            objectsToOverwrite+= toOverwrite;
            objectsToDelete+=    toDelete;
            dataToProcess+=      data;
        }
    }
}
