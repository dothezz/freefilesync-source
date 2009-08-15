#include "comparison.h"
#include "shared/globalFunctions.h"
#include <wx/intl.h>
#include <wx/timer.h>
#include <wx/ffile.h>
#include <wx/msgdlg.h>
#include <wx/log.h>
#include "algorithm.h"
#include <wx/thread.h>
#include <memory>
#include "library/statusHandler.h"
#include "shared/fileHandling.h"
#include "shared/systemFunctions.h"
#include "shared/fileTraverser.h"
#include "library/filter.h"

using namespace FreeFileSync;


class GetAllFilesFull : public FreeFileSync::TraverseCallback
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


    virtual ReturnValue onFile(const DefaultChar* shortName, const Zstring& fullName, const FileInfo& details)
    {
        //apply filter before processing (use relative name!)
        if (filterInstance)
            if (    !filterInstance->matchesFileFilterIncl(fullName.c_str() + prefixLength) ||
                    filterInstance->matchesFileFilterExcl(fullName.c_str() + prefixLength))
            {
                statusHandler->requestUiRefresh();
                return TRAVERSING_CONTINUE;
            }

        FileDescrLine fileDescr;
        fileDescr.fullName         = fullName;
        fileDescr.relativeName     = fullName.zsubstr(prefixLength);
        fileDescr.lastWriteTimeRaw = details.lastWriteTimeRaw;
        fileDescr.fileSize         = details.fileSize;
        fileDescr.objType          = FileDescrLine::TYPE_FILE;
        m_output.push_back(fileDescr);

        //assemble status message (performance optimized)  = textScanning + wxT("\"") + fullName + wxT("\"")
        const unsigned int statusTextMaxLen = 2000;
        wxChar statusText[statusTextMaxLen];
        wxChar* position = statusText;
        if (textScanning.length() + fullName.length() + 2 < statusTextMaxLen) //leave room for 0 terminating char!
        {
            writeText(textScanning.c_str(), textScanning.length(), position);
            writeText(wxT("\""), 1, position);
            writeText(fullName.c_str(), fullName.length(), position);
            writeText(wxT("\""), 1, position);
        }
        *position = 0;

        //update UI/commandline status information
        statusHandler->updateStatusText(statusText);
        //add 1 element to the progress indicator
        statusHandler->updateProcessedData(1, 0); //NO performance issue at all
        //trigger display refresh
        statusHandler->requestUiRefresh();

        return TRAVERSING_CONTINUE;
    }


    virtual ReturnValDir onDir(const DefaultChar* shortName, const Zstring& fullName)
    {
        //apply filter before processing (use relative name!)
        if (filterInstance)
        {
            if (!filterInstance->matchesDirFilterIncl(fullName.c_str() + prefixLength))
            {
                statusHandler->requestUiRefresh(); //if not included: CONTINUE traversing subdirs
                return ReturnValDir(ReturnValDir::Continue(), this);
            }
            else if (filterInstance->matchesDirFilterExcl(fullName.c_str() + prefixLength))
            {
                statusHandler->requestUiRefresh(); //if excluded: do NOT traverse subdirs
                return ReturnValDir::Ignore();
            }
        }
#ifdef FFS_WIN
        if (    fullName.EndsWith(wxT("\\RECYCLER")) ||
                fullName.EndsWith(wxT("\\System Volume Information")))
        {
            statusHandler->requestUiRefresh();
            return ReturnValDir::Ignore();
        }
#endif  // FFS_WIN

        FileDescrLine fileDescr;
        fileDescr.fullName         = fullName;
        fileDescr.relativeName     = fullName.zsubstr(prefixLength);
        fileDescr.lastWriteTimeRaw = 0;  //irrelevant for directories
        fileDescr.fileSize         = 0;  //currently used by getBytesToTransfer
        fileDescr.objType          = FileDescrLine::TYPE_DIRECTORY;
        m_output.push_back(fileDescr);

        //assemble status message (performance optimized)  = textScanning + wxT("\"") + fullName + wxT("\"")
        const unsigned int statusTextMaxLen = 2000;
        wxChar statusText[statusTextMaxLen];
        wxChar* position = statusText;
        if (textScanning.length() + fullName.length() + 2 < statusTextMaxLen) //leave room for 0 terminating char!
        {
            writeText(textScanning.c_str(), textScanning.length(), position);
            writeText(wxT("\""), 1, position);
            writeText(fullName.c_str(), fullName.length(), position);
            writeText(wxT("\""), 1, position);
        }
        *position = 0;

        //update UI/commandline status information
        statusHandler->updateStatusText(statusText);
        //add 1 element to the progress indicator
        statusHandler->updateProcessedData(1, 0);     //NO performance issue at all
        //trigger display refresh
        statusHandler->requestUiRefresh();

        return ReturnValDir(ReturnValDir::Continue(), this);
    }


    virtual ReturnValue onError(const wxString& errorText)
    {
        while (true)
        {
            switch (statusHandler->reportError(errorText))
            {
            case ErrorHandler::IGNORE_ERROR:
                return TRAVERSING_CONTINUE;
            case ErrorHandler::RETRY:
                break; //I have to admit "retry" is a bit of a fake here... at least the user has opportunity to abort!
            }
        }

        return TRAVERSING_CONTINUE;
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

            if (FreeFileSync::dirExists(directoryFormatted)) //folder existence already checked in startCompareProcess(): do not treat as error when arriving here!
            {
                //get all files and folders from directoryFormatted (and subdirectories)
                GetAllFilesFull traverser(*bufferEntry.directoryDesc, directoryFormatted, filterInstance, m_statusUpdater); //exceptions may be thrown!
                traverseFolder(directoryFormatted, m_traverseDirectorySymlinks, &traverser);
            }

            return bufferEntry.directoryDesc;
        }
    }

private:
    std::set<DescrBufferLine> buffer;

    const bool m_traverseDirectorySymlinks;
    const FilterProcess* const filterInstance; //may be NULL!
    StatusHandler* m_statusUpdater;
};


void foldersAreValidForComparison(const std::vector<FolderPair>& folderPairs, StatusHandler* statusUpdater)
{
    bool checkEmptyDirnameActive = true; //check for empty dirs just once
    const wxString additionalInfo = _("You can ignore the error to consider not existing directories as empty.");

    for (std::vector<FolderPair>::const_iterator i = folderPairs.begin(); i != folderPairs.end(); ++i)
    {
        const Zstring leftFolderName  = getFormattedDirectoryName(i->leftDirectory);
        const Zstring rightFolderName = getFormattedDirectoryName(i->rightDirectory);

        //check if folder name is empty
        if (leftFolderName.empty() || rightFolderName.empty())
        {
            if (checkEmptyDirnameActive)
            {
                checkEmptyDirnameActive = false;
                while (true)
                {
                    const ErrorHandler::Response rv = statusUpdater->reportError(wxString(_("Please fill all empty directory fields.")) + wxT("\n\n") +
                                                      + wxT("(") + additionalInfo + wxT(")"));
                    if (rv == ErrorHandler::IGNORE_ERROR)
                        break;
                    else if (rv == ErrorHandler::RETRY)
                        ;  //continue with loop
                    else
                        assert (false);
                }
            }
        }

        //check if folders exist
        if (!leftFolderName.empty())
            while (!FreeFileSync::dirExists(leftFolderName))
            {
                ErrorHandler::Response rv = statusUpdater->reportError(wxString(_("Directory does not exist:")) + wxT("\n") +
                                            wxT("\"") + leftFolderName + wxT("\"") + wxT("\n\n") +
                                            FreeFileSync::getLastErrorFormatted() + wxT(" ") + additionalInfo);
                if (rv == ErrorHandler::IGNORE_ERROR)
                    break;
                else if (rv == ErrorHandler::RETRY)
                    ;  //continue with loop
                else
                    assert (false);
            }

        if (!rightFolderName.empty())
            while (!FreeFileSync::dirExists(rightFolderName))
            {
                ErrorHandler::Response rv = statusUpdater->reportError(wxString(_("Directory does not exist:")) + wxT("\n") +
                                            wxT("\"") + rightFolderName + wxT("\"") + wxT("\n\n") +
                                            FreeFileSync::getLastErrorFormatted() + wxT(" ") + additionalInfo);
                if (rv == ErrorHandler::IGNORE_ERROR)
                    break;
                else if (rv == ErrorHandler::RETRY)
                    ;  //continue with loop
                else
                    assert (false);
            }
    }
}


bool dependencyExists(const std::vector<Zstring>& folders, const Zstring& newFolder, wxString& warningMessage)
{
    if (!newFolder.empty()) //empty folders names might be accepted by user
    {
        warningMessage.Clear();

        for (std::vector<Zstring>::const_iterator i = folders.begin(); i != folders.end(); ++i)
            if (!i->empty()) //empty folders names might be accepted by user
                if (newFolder.StartsWith(*i) || i->StartsWith(newFolder))
                {
                    warningMessage = wxString(_("Directories are dependent! Be careful when setting up synchronization rules:")) + wxT("\n") +
                                     wxT("\"") + i->c_str() + wxT("\",\n") +
                                     wxT("\"") + newFolder.c_str() + wxT("\"");
                    return true;
                }
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
                               xmlAccess::WarningMessages& warnings,
                               const FilterProcess* filter, //may be NULL
                               StatusHandler* handler) :
        fileTimeTolerance(fileTimeTol),
        ignoreOneHourDifference(ignoreOneHourDiff),
        m_warnings(warnings),
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


//callback functionality for status updates while comparing
class CompareCallback
{
public:
    virtual ~CompareCallback() {}
    virtual void updateCompareStatus(const wxLongLong& totalBytesTransferred) = 0;
};


bool filesHaveSameContent(const Zstring& filename1, const Zstring& filename2, CompareCallback* callback)
{
    static MemoryAllocator memory;

    wxFFile file1(filename1.c_str(), wxT("rb"));
    if (!file1.IsOpened())
        throw FileError(wxString(_("Error reading file:")) + wxT(" \"") + filename1.c_str() + wxT("\""));

    wxFFile file2(filename2.c_str(), wxT("rb"));
    if (!file2.IsOpened()) //NO cleanup necessary for (wxFFile) file1
        throw FileError(wxString(_("Error reading file:")) + wxT(" \"") + filename2.c_str() + wxT("\""));

    wxLongLong bytesCompared;
    do
    {
        const size_t length1 = file1.Read(memory.buffer1, MemoryAllocator::bufferSize);
        if (file1.Error()) throw FileError(wxString(_("Error reading file:")) + wxT(" \"") + filename1.c_str() + wxT("\""));

        const size_t length2 = file2.Read(memory.buffer2, MemoryAllocator::bufferSize);
        if (file2.Error()) throw FileError(wxString(_("Error reading file:")) + wxT(" \"") + filename2.c_str() + wxT("\""));

        if (length1 != length2 || memcmp(memory.buffer1, memory.buffer2, length1) != 0)
            return false;

        bytesCompared += length1 * 2;

        //send progress updates
        callback->updateCompareStatus(bytesCompared);
    }
    while (!file1.Eof());

    if (!file2.Eof())
        return false;

    return true;
}


//callback implementation
class CmpCallbackImpl : public CompareCallback
{
public:
    CmpCallbackImpl(StatusHandler* handler, wxLongLong& bytesComparedLast) :
            m_handler(handler),
            m_bytesComparedLast(bytesComparedLast) {}

    virtual void updateCompareStatus(const wxLongLong& totalBytesTransferred)
    {   //called every 512 kB

        //inform about the (differential) processed amount of data
        m_handler->updateProcessedData(0, totalBytesTransferred - m_bytesComparedLast);
        m_bytesComparedLast = totalBytesTransferred;

        m_handler->requestUiRefresh(); //exceptions may be thrown here!
    }

private:
    StatusHandler* m_handler;
    wxLongLong& m_bytesComparedLast;
};


bool filesHaveSameContentUpdating(const Zstring& filename1, const Zstring& filename2, const wxULongLong& totalBytesToCmp, StatusHandler* handler)
{
    wxLongLong bytesComparedLast; //amount of bytes that have been compared and communicated to status handler

    CmpCallbackImpl callback(handler, bytesComparedLast);

    bool sameContent = true;
    try
    {
        sameContent = filesHaveSameContent(filename1, filename2, &callback);
    }
    catch (...)
    {
        //error situation: undo communication of processed amount of data
        handler->updateProcessedData(0, bytesComparedLast * -1);

        throw;
    }

    //inform about the (remaining) processed amount of data
    handler->updateProcessedData(0, globalFunctions::convertToSigned(totalBytesToCmp) - bytesComparedLast);

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

    //format directory pairs: ensure they end with globalFunctions::FILE_NAME_SEPARATOR!
    std::vector<FolderPair> directoryPairsFormatted;
    for (std::vector<FolderPair>::const_iterator i = directoryPairs.begin(); i != directoryPairs.end(); ++i)
        directoryPairsFormatted.push_back(
            FolderPair(FreeFileSync::getFormattedDirectoryName(i->leftDirectory),
                       FreeFileSync::getFormattedDirectoryName(i->rightDirectory)));

    //-------------------some basic checks:------------------------------------------

    //check if folders are valid
    foldersAreValidForComparison(directoryPairsFormatted, statusUpdater);

    //check if folders have dependencies
    {
        wxString warningMessage;
        if (foldersHaveDependencies(directoryPairsFormatted, warningMessage))
            statusUpdater->reportWarning(warningMessage.c_str(), m_warnings.warningDependentFolders);
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


//--------------------assemble conflict descriptions---------------------------

//check for very old dates or dates in the future
wxString getConflictInvalidDate(const Zstring& fileNameFull, const wxLongLong& utcTime)
{
    wxString msg = _("File %x has an invalid date!");
    msg.Replace(wxT("%x"), wxString(wxT("\"")) + fileNameFull.c_str() + wxT("\""));
    msg += wxString(wxT("\n\n")) + _("Date") + wxT(": ") + utcTimeToLocalString(utcTime, fileNameFull);
    return wxString(_("Conflict detected:")) + wxT("\n") + msg;
}


//check for changed files with same modification date
wxString getConflictSameDateDiffSize(const FileCompareLine& cmpLine)
{
    //some beautification...
    wxString left = wxString(_("Left")) + wxT(": ");
    wxString right = wxString(_("Right")) + wxT(": ");
    const int maxPref = std::max(left.length(), right.length());
    left.Pad(maxPref - left.length(), wxT(' '), true);
    right.Pad(maxPref - right.length(), wxT(' '), true);

    wxString msg = _("Files %x have the same date but a different size!");
    msg.Replace(wxT("%x"), wxString(wxT("\"")) + cmpLine.fileDescrLeft.relativeName.c_str() + wxT("\""));
    msg += wxT("\n\n");
    msg += left + wxT("\t") + _("Date") + wxT(": ") + utcTimeToLocalString(cmpLine.fileDescrLeft.lastWriteTimeRaw,
            cmpLine.fileDescrLeft.fullName) + wxT(" \t") + _("Size") + wxT(": ") + cmpLine.fileDescrLeft.fileSize.ToString() + wxT("\n");
    msg += right + wxT("\t") + _("Date") + wxT(": ") + utcTimeToLocalString(cmpLine.fileDescrRight.lastWriteTimeRaw,
            cmpLine.fileDescrRight.fullName) + wxT(" \t") + _("Size") + wxT(": ") + cmpLine.fileDescrRight.fileSize.ToString();
    return wxString(_("Conflict detected:")) + wxT("\n") + msg;
}


//check for files that have a difference in file modification date below 1 hour when DST check is active
wxString getConflictChangeWithinHour(const FileCompareLine& cmpLine)
{
    //some beautification...
    wxString left = wxString(_("Left")) + wxT(": ");
    wxString right = wxString(_("Right")) + wxT(": ");
    const int maxPref = std::max(left.length(), right.length());
    left.Pad(maxPref - left.length(), wxT(' '), true);
    right.Pad(maxPref - right.length(), wxT(' '), true);

    wxString msg = _("Files %x have a file time difference of less than 1 hour! It's not safe to decide which one is newer due to Daylight Saving Time issues.");
    msg.Replace(wxT("%x"), wxString(wxT("\"")) + cmpLine.fileDescrLeft.relativeName.c_str() + wxT("\""));
    msg += wxT("\n\n");
    msg += left + wxT("\t") + _("Date") + wxT(": ") + utcTimeToLocalString(cmpLine.fileDescrLeft.lastWriteTimeRaw, cmpLine.fileDescrLeft.fullName) + wxT("\n");
    msg += right + wxT("\t") + _("Date") + wxT(": ") + utcTimeToLocalString(cmpLine.fileDescrRight.lastWriteTimeRaw, cmpLine.fileDescrRight.fullName);
    return wxString(_("Conflict detected:")) + wxT("\n") + msg;
}
//-----------------------------------------------------------------------------


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
                        if (i->fileDescrLeft.lastWriteTimeRaw < 0 || i->fileDescrLeft.lastWriteTimeRaw > oneYearFromNow)
                            i->conflictDescription = OptionalString(getConflictInvalidDate(i->fileDescrLeft.fullName, i->fileDescrLeft.lastWriteTimeRaw));
                        else
                            i->conflictDescription = OptionalString(getConflictInvalidDate(i->fileDescrRight.fullName, i->fileDescrRight.lastWriteTimeRaw));
                    }
                    else //from this block on all dates are at least "valid"
                    {
                        //last write time may differ by up to 2 seconds (NTFS vs FAT32)
                        if (sameFileTime(i->fileDescrLeft.lastWriteTimeRaw, i->fileDescrRight.lastWriteTimeRaw, fileTimeTolerance))
                        {
                            if (i->fileDescrLeft.fileSize == i->fileDescrRight.fileSize)
                                i->cmpResult = FILE_EQUAL;
                            else
                            {
                                i->cmpResult = FILE_CONFLICT; //same date, different filesize
                                i->conflictDescription = OptionalString(getConflictSameDateDiffSize(*i));
                            }
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
                                    i->conflictDescription = OptionalString(getConflictChangeWithinHour(*i));
                                }
                                else //exact +/- 1-hour detected: treat as equal
                                {
                                    if (i->fileDescrLeft.fileSize == i->fileDescrRight.fileSize)
                                        i->cmpResult = FILE_EQUAL;
                                    else
                                    {
                                        i->cmpResult = FILE_CONFLICT; //same date, different filesize
                                        i->conflictDescription = OptionalString(getConflictSameDateDiffSize(*i));
                                    }
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
                    {
                        i->cmpResult = FILE_CONFLICT; //same date, different filesize
                        i->conflictDescription = OptionalString(getConflictSameDateDiffSize(*i));
                    }
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


void getBytesToCompare(const FolderComparison& grid, const FolderCompRef& rowsToCompare, int& objectsTotal, wxULongLong& dataTotal)
{
    objectsTotal = 0;
    dataTotal    = 0;

    for (FolderComparison::const_iterator j = grid.begin(); j != grid.end(); ++j)
    {
        const FileComparison& fileCmp = j->fileCmp;

        const std::set<int>& index = rowsToCompare[j - grid.begin()];
        for (std::set<int>::const_iterator i = index.begin(); i != index.end(); ++i)
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
        for (std::set<int>::const_iterator i = index.begin(); i != index.end(); ++i)
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
    for (DirectoryDescrType::const_iterator i = directoryLeft->begin(); i != directoryLeft->end(); ++i)
    {
        //find files/folders that exist in left file tree but not in right one
        DirectoryDescrType::const_iterator j = custom_binary_search(directoryRight->begin(), directoryRight->end(), *i);
        if (j == directoryRight->end())
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
