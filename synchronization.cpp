#include "synchronization.h"
#include <wx/intl.h>
#include <wx/msgdlg.h>
#include <wx/log.h>
#include "library/multithreading.h"
#include "library/resources.h"
#include "algorithm.h"

#ifdef FFS_WIN
#include <wx/msw/wrapwin.h> //includes "windows.h"
#endif

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


SyncProcess::SyncProcess(const bool useRecycler,
                         const bool copyFileSymLinks,
                         const bool traverseDirSymLinks,
                         bool& warningSignificantDifference,
                         StatusHandler* handler) :
        m_useRecycleBin(useRecycler),
        m_copyFileSymLinks(copyFileSymLinks),
        m_traverseDirSymLinks(traverseDirSymLinks),
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

            removeFile(cmpLine.fileDescrLeft.fullName, m_useRecycleBin);
            break;
        case SyncConfiguration::SYNC_DIR_RIGHT:  //copy files to right
            target = cmpLine.fileDescrRight.directory + cmpLine.fileDescrLeft.relativeName.c_str();

            statusText = txtCopyingFile;
            statusText.Replace(wxT("%x"), cmpLine.fileDescrLeft.fullName, false);
            statusText.Replace(wxT("%y"), target, false);
            statusUpdater->updateStatusText(statusText);

            copyFileUpdating(cmpLine.fileDescrLeft.fullName, target, cmpLine.fileDescrLeft.fileSize);
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
            target = cmpLine.fileDescrLeft.directory + cmpLine.fileDescrRight.relativeName.c_str();

            statusText = txtCopyingFile;
            statusText.Replace(wxT("%x"), cmpLine.fileDescrRight.fullName, false);
            statusText.Replace(wxT("%y"), target, false);
            statusUpdater->updateStatusText(statusText);

            copyFileUpdating(cmpLine.fileDescrRight.fullName, target, cmpLine.fileDescrRight.fileSize);
            break;
        case SyncConfiguration::SYNC_DIR_RIGHT:  //delete files on right
            statusText = txtDeletingFile;
            statusText.Replace(wxT("%x"), cmpLine.fileDescrRight.fullName, false);
            statusUpdater->updateStatusText(statusText);

            removeFile(cmpLine.fileDescrRight.fullName, m_useRecycleBin);
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

            removeFile(cmpLine.fileDescrLeft.fullName, m_useRecycleBin);  //only used if switch activated by user, else file is simply deleted
            copyFileUpdating(cmpLine.fileDescrRight.fullName, cmpLine.fileDescrLeft.fullName, cmpLine.fileDescrRight.fileSize);
            break;
        case SyncConfiguration::SYNC_DIR_RIGHT:  //copy from left to right
            statusText = txtOverwritingFile;
            statusText.Replace(wxT("%x"), cmpLine.fileDescrLeft.fullName, false);
            statusText.Replace(wxT("%y"), cmpLine.fileDescrRight.fullName, false);
            statusUpdater->updateStatusText(statusText);

            removeFile(cmpLine.fileDescrRight.fullName, m_useRecycleBin);  //only used if switch activated by user, else file is simply deleted
            copyFileUpdating(cmpLine.fileDescrLeft.fullName, cmpLine.fileDescrRight.fullName, cmpLine.fileDescrLeft.fileSize);
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

            removeDirectory(cmpLine.fileDescrLeft.fullName, m_useRecycleBin);
            break;
        case SyncConfiguration::SYNC_DIR_RIGHT:  //create folders on right
            target = cmpLine.fileDescrRight.directory + cmpLine.fileDescrLeft.relativeName.c_str();

            statusText = txtCreatingFolder;
            statusText.Replace(wxT("%x"), target, false);
            statusUpdater->updateStatusText(statusText);

            //some check to catch the error that directory on source has been deleted externally after "compare"...
            if (!wxDirExists(cmpLine.fileDescrLeft.fullName))
                throw FileError(Zstring(_("Error: Source directory does not exist anymore:")) + wxT("\n\"") + cmpLine.fileDescrLeft.fullName + wxT("\""));
            createDirectory(target, cmpLine.fileDescrLeft.fullName, !m_traverseDirSymLinks); //traverse symlinks <=> !copy symlinks
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
            target = cmpLine.fileDescrLeft.directory + cmpLine.fileDescrRight.relativeName.c_str();

            statusText = txtCreatingFolder;
            statusText.Replace(wxT("%x"), target, false);
            statusUpdater->updateStatusText(statusText);

            //some check to catch the error that directory on source has been deleted externally after "compare"...
            if (!wxDirExists(cmpLine.fileDescrRight.fullName))
                throw FileError(Zstring(_("Error: Source directory does not exist anymore:")) + wxT("\n\"") + cmpLine.fileDescrRight.fullName + wxT("\""));
            createDirectory(target, cmpLine.fileDescrRight.fullName, !m_traverseDirSymLinks); //traverse symlinks <=> !copy symlinks
            break;
        case SyncConfiguration::SYNC_DIR_RIGHT:  //delete folders on right
            statusText = txtDeletingFolder;
            statusText.Replace(wxT("%x"), cmpLine.fileDescrRight.fullName, false);
            statusUpdater->updateStatusText(statusText);

            removeDirectory(cmpLine.fileDescrRight.fullName, m_useRecycleBin);
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
#ifndef __WXDEBUG__
    wxLogNull noWxLogs; //prevent wxWidgets logging
#endif

    //some basic checks:
    //test existence of Recycle Bin
    if (m_useRecycleBin && !FreeFileSync::recycleBinExists())
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
                    if (    (deleteLoop && deletionImminent(*i, config)) ||
                            (!deleteLoop && !deletionImminent(*i, config)))
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
                                        statusUpdater->updateProcessedData(objectsToCreate + objectsToOverwrite + objectsToDelete, 0); //processed data is communicated in subfunctions!
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
//callback functionality for smooth progress indicators

struct CallBackData
{
    StatusHandler* handler;
    wxULongLong bytesTransferredLast;
};

#ifdef FFS_WIN
const DWORD COPY_FILE_COPY_SYMLINK = 0x00000800;

DWORD CALLBACK copyProcessCallback(
    LARGE_INTEGER totalFileSize,
    LARGE_INTEGER totalBytesTransferred,
    LARGE_INTEGER streamSize,
    LARGE_INTEGER streamBytesTransferred,
    DWORD dwStreamNumber,
    DWORD dwCallbackReason,
    HANDLE hSourceFile,
    HANDLE hDestinationFile,
    LPVOID lpData)
{
    //small performance optimization: it seems this callback function is called for every 64 kB (depending on cluster size).
    static unsigned callNr = 0;
    if (++callNr % 50 == 0) //reduce by factor of 50 =^ 10-20 calls/sec
    {
        CallBackData* sharedData = static_cast<CallBackData*>(lpData);

        //inform about the (differential) processed amount of data
        const wxULongLong currentBytesTransferred = wxULongLong(totalBytesTransferred.HighPart, totalBytesTransferred.LowPart);
        sharedData->handler->updateProcessedData(0, (currentBytesTransferred - sharedData->bytesTransferredLast).ToDouble());
        sharedData->bytesTransferredLast = currentBytesTransferred;

        sharedData->handler->requestUiRefresh(false); //don't allow throwing exception within this call

        if (sharedData->handler->abortIsRequested())
            return PROGRESS_CANCEL;
        else
            return PROGRESS_CONTINUE;
    }
    else
        return PROGRESS_CONTINUE;
}

#elif defined FFS_LINUX
void copyProcessCallback(const wxULongLong& totalBytesTransferred, void* data)
{   //called every 512 kB

    CallBackData* sharedData = static_cast<CallBackData*>(data);

    //inform about the (differential) processed amount of data
    sharedData->handler->updateProcessedData(0, (totalBytesTransferred - sharedData->bytesTransferredLast).ToDouble());
    sharedData->bytesTransferredLast = totalBytesTransferred;

    sharedData->handler->requestUiRefresh(); //exceptions may be thrown here!
}
#endif


//copy file while executing statusUpdater->requestUiRefresh() calls
void SyncProcess::copyFileUpdating(const Zstring& source, const Zstring& target, const wxULongLong& totalBytesToCpy)
{
    //create folders first (see http://sourceforge.net/tracker/index.php?func=detail&aid=2628943&group_id=234430&atid=1093080)
    const Zstring targetDir = target.BeforeLast(GlobalResources::FILE_NAME_SEPARATOR);
    if (!wxDirExists(targetDir.c_str()))
    {
        const Zstring templateDir = source.BeforeLast(GlobalResources::FILE_NAME_SEPARATOR);
        FreeFileSync::createDirectory(targetDir, templateDir, !m_traverseDirSymLinks); //might throw FileError() exception
    }

    //start of (possibly) long-running copy process: ensure status updates are performed regularly
#ifdef FFS_WIN //update via call-back function

    CallBackData sharedData;
    sharedData.handler = statusUpdater;
//data.bytesTransferredLast =


    DWORD copyFlags = COPY_FILE_FAIL_IF_EXISTS;

    if (m_copyFileSymLinks) //copy symbolic links instead of the files pointed at
        copyFlags |= COPY_FILE_COPY_SYMLINK;

    if (!CopyFileEx( //same performance as CopyFile()
                source.c_str(),
                target.c_str(),
                copyProcessCallback,
                &sharedData,
                NULL,
                copyFlags))
    {
        //error situation: undo communication of processed amount of data
        statusUpdater->updateProcessedData(0, sharedData.bytesTransferredLast.ToDouble() * -1 );

        statusUpdater->requestUiRefresh(); //abort if requested, don't show error message if cancelled by user!

        Zstring errorMessage = Zstring(_("Error copying file:")) + wxT("\n\"") + source +  wxT("\" -> \"") + target + wxT("\"");
        errorMessage += Zstring(wxT("\n\n"));
        errorMessage += FreeFileSync::getLastErrorFormatted(); //some additional windows error information
        throw FileError(errorMessage);
    }

    //inform about the (remaining) processed amount of data
    statusUpdater->updateProcessedData(0, (totalBytesToCpy - sharedData.bytesTransferredLast).ToDouble());


#elif defined FFS_LINUX //update via call-back function

    CallBackData sharedData;
    sharedData.handler = statusUpdater;
//data.bytesTransferredLast =

    try
    {
        FreeFileSync::copyFile(source,
                               target,
                               m_copyFileSymLinks,
                               copyProcessCallback,
                               &sharedData);
    }
    catch (...)
    {
        //error situation: undo communication of processed amount of data
        statusUpdater->updateProcessedData(0, sharedData.bytesTransferredLast.ToDouble() * -1 );

        throw;
    }

    //inform about the (remaining) processed amount of data
    statusUpdater->updateProcessedData(0, (totalBytesToCpy - sharedData.bytesTransferredLast).ToDouble());
#endif
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
    const Zstring targetDir = target.BeforeLast(GlobalResources::FILE_NAME_SEPARATOR);
    if (!wxDirExists(targetDir.c_str()))
    {
        const Zstring templateDir = source.BeforeLast(GlobalResources::FILE_NAME_SEPARATOR);
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
