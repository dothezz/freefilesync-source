#include "comparison.h"
#include "library/globalFunctions.h"
#include <wx/intl.h>
#include "library/globalFunctions.h"
#include <wx/ffile.h>
#include <wx/msgdlg.h>
#include <wx/log.h>
#include "algorithm.h"
#include <wx/thread.h>
#include <memory>
#include "library/statusHandler.h"
#include "library/fileHandling.h"
#include "synchronization.h"
#include "library/filter.h"

#ifdef FFS_WIN
#include <wx/msw/wrapwin.h> //includes "windows.h"
#endif  //FFS_WIN

using namespace FreeFileSync;


class GetAllFilesFull : public FullDetailFileTraverser
{
public:
    GetAllFilesFull(DirectoryDescrType& output, const Zstring& dirThatIsSearched, const FilterProcess* filter, StatusHandler* handler) :
            m_output(output),
            directory(dirThatIsSearched),
            textScanning(Zstring(_("Scanning:")) + wxT(" \n")),
            filterInstance(filter),
            statusHandler(handler)
    {
        prefixLength = directory.length();
    }


    inline
    void writeText(const wxChar* text, const int length, wxChar*& currentPos)
    {
        memcpy(currentPos, text, length * sizeof(wxChar));
        currentPos += length;
    }


    wxDirTraverseResult OnFile(const Zstring& fullFileName, const FileInfo& details) //virtual impl.
    {
        //apply filter before processing (use relative name!)
        if (filterInstance && !filterInstance->matchesFileFilter(fullFileName.c_str() + prefixLength))
            return wxDIR_CONTINUE;

        FileDescrLine fileDescr;
        fileDescr.fullName         = fullFileName;
        fileDescr.relativeName     = fullFileName.zsubstr(prefixLength);
        fileDescr.lastWriteTimeRaw = details.lastWriteTimeRaw;
        fileDescr.fileSize         = details.fileSize;
        fileDescr.objType          = FileDescrLine::TYPE_FILE;
        m_output.push_back(fileDescr);

        //assemble status message (performance optimized)  = textScanning + wxT("\"") + fullFileName + wxT("\"")
        const unsigned int statusTextMaxLen = 2000;
        wxChar statusText[statusTextMaxLen];
        wxChar* position = statusText;
        if (textScanning.length() + fullFileName.length() + 2 < statusTextMaxLen) //leave room for 0 terminating char!
        {
            writeText(textScanning.c_str(), textScanning.length(), position);
            writeText(wxT("\""), 1, position);
            writeText(fullFileName.c_str(), fullFileName.length(), position);
            writeText(wxT("\""), 1, position);
        }
        *position = 0;

        //update UI/commandline status information
        statusHandler->updateStatusText(statusText);
        //add 1 element to the progress indicator
        statusHandler->updateProcessedData(1, 0); //NO performance issue at all
        //trigger display refresh
        statusHandler->requestUiRefresh();

        return wxDIR_CONTINUE;
    }


    wxDirTraverseResult OnDir(const Zstring& fullDirName) //virtual impl.
    {
        //apply filter before processing (use relative name!)
        if (filterInstance && !filterInstance->matchesDirFilter(fullDirName.c_str() + prefixLength))
            return wxDIR_IGNORE;

#ifdef FFS_WIN
        if (    fullDirName.EndsWith(wxT("\\RECYCLER")) ||
                fullDirName.EndsWith(wxT("\\System Volume Information")))
            return wxDIR_IGNORE;
#endif  // FFS_WIN

        FileDescrLine fileDescr;
        fileDescr.fullName         = fullDirName;
        fileDescr.relativeName     = fullDirName.zsubstr(prefixLength);
        fileDescr.lastWriteTimeRaw = 0;  //irrelevant for directories
        fileDescr.fileSize         = 0;  //currently used by getBytesToTransfer
        fileDescr.objType          = FileDescrLine::TYPE_DIRECTORY;
        m_output.push_back(fileDescr);

        //assemble status message (performance optimized)  = textScanning + wxT("\"") + fullDirName + wxT("\"")
        const unsigned int statusTextMaxLen = 2000;
        wxChar statusText[statusTextMaxLen];
        wxChar* position = statusText;
        if (textScanning.length() + fullDirName.length() + 2 < statusTextMaxLen) //leave room for 0 terminating char!
        {
            writeText(textScanning.c_str(), textScanning.length(), position);
            writeText(wxT("\""), 1, position);
            writeText(fullDirName.c_str(), fullDirName.length(), position);
            writeText(wxT("\""), 1, position);
        }
        *position = 0;

        //update UI/commandline status information
        statusHandler->updateStatusText(statusText);
        //add 1 element to the progress indicator
        statusHandler->updateProcessedData(1, 0);     //NO performance issue at all
        //trigger display refresh
        statusHandler->requestUiRefresh();

        return wxDIR_CONTINUE;
    }


    wxDirTraverseResult OnError(const Zstring& errorText) //virtual impl.
    {
        while (true)
        {
            ErrorHandler::Response rv = statusHandler->reportError(errorText);
            if (rv == ErrorHandler::IGNORE_ERROR)
                return wxDIR_CONTINUE;
            else if (rv == ErrorHandler::RETRY)
                ;   //I have to admit "retry" is a bit of a fake here... at least the user has opportunity to abort!
            else
            {
                assert (false);
                return wxDIR_CONTINUE;
            }
        }

        return wxDIR_CONTINUE;
    }

private:
    DirectoryDescrType& m_output;
    Zstring directory;
    int prefixLength;
    const Zstring textScanning;
    const FilterProcess* const filterInstance; //may be NULL!
    StatusHandler* statusHandler;
};


struct DescrBufferLine
{
    Zstring directoryName;
    DirectoryDescrType* directoryDesc;

    bool operator < (const DescrBufferLine& b) const
    {
#ifdef FFS_WIN //Windows does NOT distinguish between upper/lower-case
        return (directoryName.CmpNoCase(b.directoryName) < 0);
#elif defined FFS_LINUX //Linux DOES distinguish between upper/lower-case
        return (directoryName.Cmp(b.directoryName) < 0);
#endif
    }
};


class DirectoryDescrBuffer  //buffer multiple scans of the same directories
{
public:
    DirectoryDescrBuffer(const bool traverseDirectorySymlinks, const FilterProcess* filter, StatusHandler* statusUpdater) :
            m_traverseDirectorySymlinks(traverseDirectorySymlinks),
            filterInstance(filter),
            m_statusUpdater(statusUpdater) {}

    ~DirectoryDescrBuffer()
    {
        //clean up
        for (std::set<DescrBufferLine>::iterator i = buffer.begin(); i != buffer.end(); ++i)
            delete i->directoryDesc;
    }

    DirectoryDescrType* getDirectoryDescription(const Zstring& directoryFormatted)
    {
        DescrBufferLine bufferEntry;
        bufferEntry.directoryName = directoryFormatted;

        std::set<DescrBufferLine>::iterator entryFound = buffer.find(bufferEntry);
        if (entryFound != buffer.end())
        {
            //entry found in buffer; return
            return entryFound->directoryDesc;
        }
        else
        {
            //entry not found; create new one
            bufferEntry.directoryDesc = new DirectoryDescrType;
            buffer.insert(bufferEntry); //exception safety: insert into buffer right after creation!

            //get all files and folders from directoryFormatted (and subdirectories)
            GetAllFilesFull traverser(*bufferEntry.directoryDesc, directoryFormatted, filterInstance, m_statusUpdater); //exceptions may be thrown!
            traverseInDetail(directoryFormatted, m_traverseDirectorySymlinks, &traverser);

            return bufferEntry.directoryDesc;
        }
    }

private:
    std::set<DescrBufferLine> buffer;

    const bool m_traverseDirectorySymlinks;
    const FilterProcess* const filterInstance; //may be NULL!
    StatusHandler* m_statusUpdater;
};


bool foldersAreValidForComparison(const std::vector<FolderPair>& folderPairs, wxString& errorMessage)
{
    errorMessage.Clear();

    for (std::vector<FolderPair>::const_iterator i = folderPairs.begin(); i != folderPairs.end(); ++i)
    {
        const Zstring leftFolderName  = getFormattedDirectoryName(i->leftDirectory);
        const Zstring rightFolderName = getFormattedDirectoryName(i->rightDirectory);

        //check if folder name is empty
        if (leftFolderName.empty() || rightFolderName.empty())
        {
            errorMessage = _("Please fill all empty directory fields.");
            return false;
        }

        //check if folder exists
        if (!wxDirExists(leftFolderName))
        {
            errorMessage = wxString(_("Directory does not exist:")) + wxT(" \"") + leftFolderName + wxT("\"");
            return false;
        }
        if (!wxDirExists(rightFolderName))
        {
            errorMessage = wxString(_("Directory does not exist:")) + wxT(" \"") + rightFolderName + wxT("\"");
            return false;
        }
    }
    return true;
}


bool dependencyExists(const std::vector<Zstring>& folders, const Zstring& newFolder, wxString& warningMessage)
{
    warningMessage.Clear();

    for (std::vector<Zstring>::const_iterator i = folders.begin(); i != folders.end(); ++i)
        if (newFolder.StartsWith(*i) || i->StartsWith(newFolder))
        {
            warningMessage = wxString(_("Directories are dependent! Be careful when setting up synchronization rules:")) + wxT("\n") +
                             wxT("\"") + *i + wxT("\",\n") +
                             wxT("\"") + newFolder + wxT("\"");
            return true;
        }
    return false;
}


bool foldersHaveDependencies(const std::vector<FolderPair>& folderPairs, wxString& warningMessage)
{
    warningMessage.Clear();

    std::vector<Zstring> folders;
    for (std::vector<FolderPair>::const_iterator i = folderPairs.begin(); i != folderPairs.end(); ++i)
    {
        const Zstring leftFolderName  = getFormattedDirectoryName(i->leftDirectory);
        const Zstring rightFolderName = getFormattedDirectoryName(i->rightDirectory);

        if (dependencyExists(folders, leftFolderName, warningMessage))
            return true;
        folders.push_back(leftFolderName);

        if (dependencyExists(folders, rightFolderName, warningMessage))
            return true;
        folders.push_back(rightFolderName);
    }

    return false;
}


CompareProcess::CompareProcess(const bool traverseSymLinks,
                               const unsigned int fileTimeTol,
                               const bool ignoreOneHourDiff,
                               bool& warningDependentFolders,
                               const FilterProcess* filter, //may be NULL
                               StatusHandler* handler) :
        fileTimeTolerance(fileTimeTol),
        ignoreOneHourDifference(ignoreOneHourDiff),
        m_warningDependentFolders(warningDependentFolders),
        statusUpdater(handler),
        txtComparingContentOfFiles(Zstring(_("Comparing content of files %x")).Replace(wxT("%x"), wxT("\n\"%x\""), false))
{
    descriptionBuffer = new DirectoryDescrBuffer(traverseSymLinks, filter, handler);
}


CompareProcess::~CompareProcess()
{
    delete descriptionBuffer;
}


struct MemoryAllocator
{
    MemoryAllocator()
    {
        buffer1 = new unsigned char[bufferSize];
        buffer2 = new unsigned char[bufferSize];
    }

    ~MemoryAllocator()
    {
        delete [] buffer1;
        delete [] buffer2;
    }

    static const unsigned int bufferSize = 512 * 1024; //512 kb seems to be the perfect buffer size
    unsigned char* buffer1;
    unsigned char* buffer2;
};


//callback functionality
struct CallBackData
{
    StatusHandler* handler;
    wxLongLong bytesComparedLast;
};

//callback function for status updates whily comparing
typedef void (*CompareCallback)(const wxLongLong&, void*);


void compareContentCallback(const wxLongLong& totalBytesTransferred, void* data)
{   //called every 512 kB
    CallBackData* sharedData = static_cast<CallBackData*>(data);

    //inform about the (differential) processed amount of data
    sharedData->handler->updateProcessedData(0, totalBytesTransferred - sharedData->bytesComparedLast);
    sharedData->bytesComparedLast = totalBytesTransferred;

    sharedData->handler->requestUiRefresh(); //exceptions may be thrown here!
}


bool filesHaveSameContent(const Zstring& filename1, const Zstring& filename2, CompareCallback callback, void* data)
{
    static MemoryAllocator memory;

    wxFFile file1(filename1.c_str(), wxT("rb"));
    if (!file1.IsOpened())
        throw FileError(Zstring(_("Error reading file:")) + wxT(" \"") + filename1 + wxT("\""));

    wxFFile file2(filename2.c_str(), wxT("rb"));
    if (!file2.IsOpened()) //NO cleanup necessary for (wxFFile) file1
        throw FileError(Zstring(_("Error reading file:")) + wxT(" \"") + filename2 + wxT("\""));

    wxLongLong bytesCompared;
    do
    {
        const size_t length1 = file1.Read(memory.buffer1, MemoryAllocator::bufferSize);
        if (file1.Error()) throw FileError(Zstring(_("Error reading file:")) + wxT(" \"") + filename1 + wxT("\""));

        const size_t length2 = file2.Read(memory.buffer2, MemoryAllocator::bufferSize);
        if (file2.Error()) throw FileError(Zstring(_("Error reading file:")) + wxT(" \"") + filename2 + wxT("\""));

        if (length1 != length2 || memcmp(memory.buffer1, memory.buffer2, length1) != 0)
            return false;

        bytesCompared += length1 * 2;

        //send progress updates
        callback(bytesCompared, data);
    }
    while (!file1.Eof());

    if (!file2.Eof())
        return false;

    return true;
}


bool filesHaveSameContentUpdating(const Zstring& filename1, const Zstring& filename2, const wxULongLong& totalBytesToCmp, StatusHandler* handler)
{
    CallBackData sharedData;
    sharedData.handler = handler;
//data.bytesTransferredLast = //amount of bytes that have been compared and communicated to status handler

    bool sameContent = true;
    try
    {
        sameContent = filesHaveSameContent(filename1, filename2, compareContentCallback, &sharedData);
    }
    catch (...)
    {
        //error situation: undo communication of processed amount of data
        handler->updateProcessedData(0, sharedData.bytesComparedLast * -1);

        throw;
    }

    //inform about the (remaining) processed amount of data
    handler->updateProcessedData(0, globalFunctions::convertToSigned(totalBytesToCmp) - sharedData.bytesComparedLast);

    return sameContent;
}


/* OLD IMPLEMENTATION USING A WORKER THREAD

//handle execution of a method while updating the UI
class UpdateWhileComparing : public UpdateWhileExecuting
{
public:
    UpdateWhileComparing() {}
    ~UpdateWhileComparing() {}

    Zstring file1;
    Zstring file2;
    bool success;
    Zstring errorMessage;
    bool sameContent;

private:
    void longRunner() //virtual method implementation
    {
        try
        {
            sameContent = filesHaveSameContent(file1, file2);
            success = true;
        }
        catch (FileError& error)
        {
            success = false;
            errorMessage = error.show();
        }
    }
};


bool filesHaveSameContentUpdating(const Zstring& filename1, const Zstring& filename2, const wxULongLong& totalBytesToCmp, StatusHandler* updateClass)
{
    static UpdateWhileComparing cmpAndUpdate; //single instantiation: thread enters wait phase after each execution

    cmpAndUpdate.waitUntilReady();

    //longRunner is called from thread, but no mutex needed here, since thread is in waiting state!
    cmpAndUpdate.file1 = filename1;
    cmpAndUpdate.file2 = filename2;

    cmpAndUpdate.execute(updateClass);

    //no mutex needed here since longRunner is finished
    if (!cmpAndUpdate.success)
        throw FileError(cmpAndUpdate.errorMessage);

    //inform about the processed amount of data
    updateClass->updateProcessedData(0, totalBytesToCmp.ToDouble());

    return cmpAndUpdate.sameContent;
}*/


void CompareProcess::startCompareProcess(const std::vector<FolderPair>& directoryPairs,
        const CompareVariant cmpVar,
        const SyncConfiguration& config,
        FolderComparison& output)
{
#ifndef __WXDEBUG__
    wxLogNull noWxLogs; //hide wxWidgets log messages in release build
#endif

    //PERF_START;

    //init process: keep at beginning so that all gui elements are initialized properly
    statusUpdater->initNewProcess(-1, 0, StatusHandler::PROCESS_SCANNING); //it's not known how many files will be scanned => -1 objects

    //format directory pairs: ensure they end with GlobalResources::FILE_NAME_SEPARATOR!
    std::vector<FolderPair> directoryPairsFormatted;
    for (std::vector<FolderPair>::const_iterator i = directoryPairs.begin(); i != directoryPairs.end(); ++i)
        directoryPairsFormatted.push_back(
            FolderPair(FreeFileSync::getFormattedDirectoryName(i->leftDirectory),
                       FreeFileSync::getFormattedDirectoryName(i->rightDirectory)));

    //-------------------some basic checks:------------------------------------------

    //check if folders are valid
    wxString errorMessage;
    if (!foldersAreValidForComparison(directoryPairsFormatted, errorMessage))
    {
        statusUpdater->reportFatalError(errorMessage.c_str());
        return; //should be obsolete!
    }

    //check if folders have dependencies
    if (m_warningDependentFolders) //test if check should be executed
    {
        wxString warningMessage;
        if (foldersHaveDependencies(directoryPairsFormatted, warningMessage))
        {
            bool dontShowAgain = false;
            statusUpdater->reportWarning(warningMessage.c_str(),
                                         dontShowAgain);
            m_warningDependentFolders = !dontShowAgain;
        }
    }

    //-------------------end of basic checks------------------------------------------


    try
    {
        FolderComparison output_tmp; //write to output not before END of process!
        switch (cmpVar)
        {
        case CMP_BY_TIME_SIZE:
            compareByTimeSize(directoryPairsFormatted, output_tmp);
            break;
        case CMP_BY_CONTENT:
            compareByContent(directoryPairsFormatted, output_tmp);
            break;
        }

        //actually this is the initial determination
        FreeFileSync::redetermineSyncDirection(config, output_tmp);

        //only if everything was processed correctly output is written to!
        //note: output mustn't change during this process to be in sync with GUI grid view!!!
        output_tmp.swap(output);
    }
    catch (const RuntimeException& theException)
    {
        statusUpdater->reportFatalError(theException.show().c_str());
        return; //should be obsolete!
    }
    catch (std::bad_alloc& e)
    {
        statusUpdater->reportFatalError((wxString(_("System out of memory!")) + wxT(" ") + wxString::From8BitData(e.what())).c_str());
        return; //should be obsolete!
    }
}


const CompareFilesResult FILE_UNDEFINED = CompareFilesResult(42);


inline
bool sameFileTime(const wxLongLong& a, const wxLongLong& b, const unsigned tolerance)
{
    if (a < b)
        return b - a <= tolerance;
    else
        return a - b <= tolerance;
}


void CompareProcess::compareByTimeSize(const std::vector<FolderPair>& directoryPairsFormatted, FolderComparison& output)
{
    //process one folder pair after each other
    for (std::vector<FolderPair>::const_iterator pair = directoryPairsFormatted.begin(); pair != directoryPairsFormatted.end(); ++pair)
    {
        FolderCompareLine newEntry;
        newEntry.syncPair = *pair;
        output.push_back(newEntry); //attention: push_back() copies by value!!! performance: append BEFORE writing values into fileCmp!

        FileComparison& fileCmp = output.back().fileCmp;

        //do basis scan: only result lines of type FILE_UNDEFINED (files that exist on both sides) need to be determined after this call
        this->performBaseComparison(*pair, fileCmp);

        //categorize files that exist on both sides
        for (FileComparison::iterator i = fileCmp.begin(); i != fileCmp.end(); ++i)
            if (i->cmpResult == FILE_UNDEFINED)
            {
                if (i->fileDescrLeft.lastWriteTimeRaw != i->fileDescrRight.lastWriteTimeRaw)
                {
                    //number of seconds since Jan 1st 1970 + 1 year (needn't be too precise)
                    static const long oneYearFromNow = wxGetUTCTime() + 365 * 24 * 3600;

                    //check for erroneous dates (but only if dates are not (EXACTLY) the same)
                    if (    i->fileDescrLeft.lastWriteTimeRaw  < 0 || //earlier than Jan 1st 1970
                            i->fileDescrRight.lastWriteTimeRaw < 0 || //earlier than Jan 1st 1970
                            i->fileDescrLeft.lastWriteTimeRaw  > oneYearFromNow || //dated more than one year in future
                            i->fileDescrRight.lastWriteTimeRaw > oneYearFromNow)   //dated more than one year in future
                    {
                        i->cmpResult = FILE_CONFLICT;
                    }
                    else //from this block on all dates are at least "valid"
                    {
                        //last write time may differ by up to 2 seconds (NTFS vs FAT32)
                        if (sameFileTime(i->fileDescrLeft.lastWriteTimeRaw, i->fileDescrRight.lastWriteTimeRaw, fileTimeTolerance))
                        {
                            if (i->fileDescrLeft.fileSize == i->fileDescrRight.fileSize)
                                i->cmpResult = FILE_EQUAL;
                            else
                                i->cmpResult = FILE_CONFLICT; //same date, different filesize
                        }
                        else
                        {
                            //finally: DST +/- 1-hour check: test if time diff is exactly +/- 1-hour (respecting 2 second FAT precision)
                            if (ignoreOneHourDifference && sameFileTime(i->fileDescrLeft.lastWriteTimeRaw, i->fileDescrRight.lastWriteTimeRaw, 3600 + 2))
                            {
                                //date diff < 1 hour is a conflict: it's not safe to determine which file is newer
                                if (sameFileTime(i->fileDescrLeft.lastWriteTimeRaw, i->fileDescrRight.lastWriteTimeRaw, 3600 - 2 - 1))
                                {
                                    i->cmpResult = FILE_CONFLICT;
                                }
                                else //exact +/- 1-hour detected: treat as equal
                                {
                                    if (i->fileDescrLeft.fileSize == i->fileDescrRight.fileSize)
                                        i->cmpResult = FILE_EQUAL;
                                    else
                                        i->cmpResult = FILE_CONFLICT; //same date, different filesize
                                }
                            }
                            else
                            {
                                if (i->fileDescrLeft.lastWriteTimeRaw < i->fileDescrRight.lastWriteTimeRaw)
                                    i->cmpResult = FILE_RIGHT_NEWER;
                                else
                                    i->cmpResult = FILE_LEFT_NEWER;
                            }
                        }
                    }
                }
                else //same write time
                {
                    if (i->fileDescrLeft.fileSize == i->fileDescrRight.fileSize)
                        i->cmpResult = FILE_EQUAL;
                    else
                        i->cmpResult = FILE_CONFLICT; //same date, different filesize
                }
            }
    }
}


class RemoveAtExit //this class ensures, that the result of the method below is ALWAYS written on exit, even if exceptions are thrown!
{
public:
    RemoveAtExit(FileComparison& fileCmp) :
            gridToWrite(fileCmp) {}

    ~RemoveAtExit()
    {
        removeRowsFromVector(gridToWrite, rowsToDelete);
    }

    void markRow(int nr)
    {
        rowsToDelete.insert(nr);
    }

private:
    FileComparison& gridToWrite;
    std::set<int> rowsToDelete;
};


void getBytesToCompare(const FolderComparison& grid, const FolderCompRef& rowsToCompare, int& objectsTotal, wxULongLong& dataTotal)
{
    objectsTotal = 0;
    dataTotal    = 0;

    for (FolderComparison::const_iterator j = grid.begin(); j != grid.end(); ++j)
    {
        const FileComparison& fileCmp = j->fileCmp;

        const std::set<int>& index = rowsToCompare[j - grid.begin()];
        for (std::set<int>::iterator i = index.begin(); i != index.end(); ++i)
        {
            const FileCompareLine& line = fileCmp[*i];
            dataTotal += line.fileDescrLeft.fileSize;
            dataTotal += line.fileDescrRight.fileSize;
        }

        objectsTotal += index.size() * 2;
    }
}


void CompareProcess::compareByContent(const std::vector<FolderPair>& directoryPairsFormatted, FolderComparison& output)
{
    //PERF_START;

    //process one folder pair after each other
    for (std::vector<FolderPair>::const_iterator pair = directoryPairsFormatted.begin(); pair != directoryPairsFormatted.end(); ++pair)
    {
        FolderCompareLine newEntry;
        newEntry.syncPair = *pair;
        output.push_back(newEntry); //attention: push_back() copies by value!!! performance: append BEFORE writing values into fileCmp!

        FileComparison& fileCmp = output.back().fileCmp;

        //do basis scan: only result lines of type FILE_UNDEFINED (files that exist on both sides) need to be determined after this call
        this->performBaseComparison(*pair, fileCmp);
    }

    //finish categorization...

    FolderCompRef rowsToCompareBytewise; //content comparison of file content happens AFTER finding corresponding files
    //in order to separate into two processes (scanning and comparing)

    for (FolderComparison::iterator j = output.begin(); j != output.end(); ++j)
    {
        FileComparison& fileCmp = j->fileCmp;

        std::set<int> newEntry;

        for (FileComparison::iterator i = fileCmp.begin(); i != fileCmp.end(); ++i)
        {
            if (i->cmpResult == FILE_UNDEFINED)
            {  //pre-check: files have different content if they have a different filesize
                if (i->fileDescrLeft.fileSize != i->fileDescrRight.fileSize)
                    i->cmpResult = FILE_DIFFERENT;
                else
                    newEntry.insert(i - fileCmp.begin());
            }
        }
        rowsToCompareBytewise.push_back(newEntry);
    }

    int objectsTotal = 0;
    wxULongLong dataTotal;
    getBytesToCompare(output, rowsToCompareBytewise, objectsTotal, dataTotal);

    statusUpdater->initNewProcess(objectsTotal,
                                  globalFunctions::convertToSigned(dataTotal),
                                  StatusHandler::PROCESS_COMPARING_CONTENT);

    //compare files (that have same size) bytewise...
    for (FolderComparison::iterator j = output.begin(); j != output.end(); ++j)
    {
        FileComparison& fileCmp = j->fileCmp;

        //mark erroneous rows for deletion from output
        RemoveAtExit removeRowsAtExit(fileCmp); //note: running at individual folder pair level!

        const std::set<int>& index = rowsToCompareBytewise[j - output.begin()];
        for (std::set<int>::iterator i = index.begin(); i != index.end(); ++i)
        {
            FileCompareLine& gridline = fileCmp[*i];

            Zstring statusText = txtComparingContentOfFiles;
            statusText.Replace(wxT("%x"), gridline.fileDescrLeft.relativeName.c_str(), false);
            statusUpdater->updateStatusText(statusText);

            //check files that exist in left and right model but have different content
            while (true)
            {
                //trigger display refresh
                statusUpdater->requestUiRefresh();

                try
                {
                    if (filesHaveSameContentUpdating(gridline.fileDescrLeft.fullName,
                                                     gridline.fileDescrRight.fullName,
                                                     gridline.fileDescrLeft.fileSize * 2,
                                                     statusUpdater))
                        gridline.cmpResult = FILE_EQUAL;
                    else
                        gridline.cmpResult = FILE_DIFFERENT;

                    statusUpdater->updateProcessedData(2, 0); //processed data is communicated in subfunctions!
                    break;
                }
                catch (FileError& error)
                {
                    ErrorHandler::Response rv = statusUpdater->reportError(error.show());
                    if (rv == ErrorHandler::IGNORE_ERROR)
                    {
                        removeRowsAtExit.markRow(*i);
                        break;
                    }
                    else if (rv == ErrorHandler::RETRY)
                        ;   //continue with loop
                    else
                        assert (false);
                }
            }
        }
    }
}


class ThreadSorting : public wxThread
{
public:
    ThreadSorting(DirectoryDescrType* directory) :
            wxThread(wxTHREAD_JOINABLE),
            m_directory(directory)
    {
        if (Create() != wxTHREAD_NO_ERROR)
            throw RuntimeException(wxString(wxT("Error creating thread for sorting!")));
    }

    ~ThreadSorting() {}


    ExitCode Entry()
    {
        std::sort(m_directory->begin(), m_directory->end());
        return 0;
    }

private:
    DirectoryDescrType* m_directory;
};


void CompareProcess::performBaseComparison(const FolderPair& pair, FileComparison& output)
{
    //PERF_START;
    //retrieve sets of files (with description data)
    DirectoryDescrType* directoryLeft  = descriptionBuffer->getDirectoryDescription(pair.leftDirectory);
    DirectoryDescrType* directoryRight = descriptionBuffer->getDirectoryDescription(pair.rightDirectory);

    statusUpdater->updateStatusText(_("Generating file list..."));
    statusUpdater->forceUiRefresh(); //keep total number of scanned files up to date
    //PERF_STOP;

    //we use binary search when comparing the directory structures: so sort() first
    if (wxThread::GetCPUCount() >= 2) //do it the multithreaded way:
    {
        //no synchronization (multithreading) needed here: directoryLeft and directoryRight are disjunct
        //reference counting Zstring also shouldn't be an issue, as no strings are deleted during std::sort()
        std::auto_ptr<ThreadSorting> sortLeft(new ThreadSorting(directoryLeft));
        std::auto_ptr<ThreadSorting> sortRight(new ThreadSorting(directoryRight));

        if (sortLeft->Run() != wxTHREAD_NO_ERROR)
            throw RuntimeException(wxString(wxT("Error starting thread for sorting!")));

        if (directoryLeft != directoryRight) //attention: might point to the same vector because of buffer!
        {
            if (sortRight->Run() != wxTHREAD_NO_ERROR)
                throw RuntimeException(wxString(wxT("Error starting thread for sorting!")));

            if (sortRight->Wait() != 0)
                throw RuntimeException(wxString(wxT("Error waiting for thread (sorting)!")));
        }

        if (sortLeft->Wait() != 0)
            throw RuntimeException(wxString(wxT("Error waiting for thread (sorting)!")));
    }
    else //single threaded
    {
        std::sort(directoryLeft->begin(), directoryLeft->end());

        if (directoryLeft != directoryRight) //attention: might point to the same vector because of buffer!
            std::sort(directoryRight->begin(), directoryRight->end());
    }
    //PERF_STOP;

    //reserve some space to avoid too many vector reallocations: doesn't make much sense for multiple folder pairs, but doesn't hurt either
    output.reserve(output.size() + unsigned(std::max(directoryLeft->size(), directoryRight->size()) * 1.2));

    //begin base comparison
    FileCompareLine newline(FILE_UNDEFINED, SYNC_DIR_NONE, true);
    DirectoryDescrType::iterator j;
    for (DirectoryDescrType::const_iterator i = directoryLeft->begin(); i != directoryLeft->end(); ++i)
    {    //find files/folders that exist in left file tree but not in right one
        if ((j = custom_binary_search(directoryRight->begin(), directoryRight->end(), *i)) == directoryRight->end())
        {
            newline.fileDescrLeft            = *i;
            newline.fileDescrRight           = FileDescrLine();
            newline.cmpResult                = FILE_LEFT_SIDE_ONLY;
            output.push_back(newline);
        }
        //find files/folders that exist on left and right side
        else
        {
            const FileDescrLine::ObjectType typeLeft  = i->objType;
            const FileDescrLine::ObjectType typeRight = j->objType;

            //files...
            if (typeLeft == FileDescrLine::TYPE_FILE && typeRight == FileDescrLine::TYPE_FILE)
            {
                newline.fileDescrLeft  = *i;
                newline.fileDescrRight = *j;
                newline.cmpResult      = FILE_UNDEFINED; //not yet determined!
                output.push_back(newline);
            }
            //directories...
            else if (typeLeft == FileDescrLine::TYPE_DIRECTORY && typeRight == FileDescrLine::TYPE_DIRECTORY)
            {
                newline.fileDescrLeft  = *i;
                newline.fileDescrRight = *j;
                newline.cmpResult      = FILE_EQUAL;
                output.push_back(newline);
            }
            //if we have a nameclash between a file and a directory: split into two separate rows
            else
            {
                assert (typeLeft != typeRight);

                newline.fileDescrLeft            = *i;
                newline.fileDescrRight           = FileDescrLine();
                newline.cmpResult                = FILE_LEFT_SIDE_ONLY;
                output.push_back(newline);

                newline.fileDescrLeft           = FileDescrLine();
                newline.fileDescrRight          = *j;
                newline.cmpResult               = FILE_RIGHT_SIDE_ONLY;
                output.push_back(newline);
            }
        }
    }


    for (DirectoryDescrType::const_iterator j = directoryRight->begin(); j != directoryRight->end(); ++j)
    {
        //find files/folders that exist in right file model but not in left model
        if (custom_binary_search(directoryLeft->begin(), directoryLeft->end(), *j) == directoryLeft->end())
        {
            newline.fileDescrLeft           = FileDescrLine();
            newline.fileDescrRight          = *j;
            newline.cmpResult               = FILE_RIGHT_SIDE_ONLY;
            output.push_back(newline);
        }
    }

    //PERF_STOP
}
