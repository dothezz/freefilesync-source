#include "comparison.h"
#include "library/globalFunctions.h"
#include <wx/intl.h>
#include "library/globalFunctions.h"
#include <wx/ffile.h>
#include <wx/msgdlg.h>
#include <wx/log.h>
#include "library/multithreading.h"
#include "algorithm.h"

#ifdef FFS_WIN
#include <windows.h>
#endif  //FFS_WIN

using namespace FreeFileSync;

CompareProcess::CompareProcess(bool lineBreakOnMessages, StatusHandler* handler) :
        statusUpdater(handler)
{
    if (lineBreakOnMessages)
        optionalLineBreak = wxT("\n");
}


struct FileInfo
{
    wxULongLong fileSize; //unit: bytes!
    wxString lastWriteTime;
    wxULongLong lastWriteTimeRaw; //unit: seconds!
};


class GetAllFilesFull : public wxDirTraverser
{
public:
    GetAllFilesFull(DirectoryDescrType& output, wxString dirThatIsSearched, const wxString& optionalLineBreak, StatusHandler* updateClass = 0);

    wxDirTraverseResult OnFile(const wxString& filename);

    wxDirTraverseResult OnDir(const wxString& dirname);

    wxDirTraverseResult OnOpenError(const wxString& openerrorname);

private:
    DirectoryDescrType& m_output;
    wxString directory;
    int prefixLength;
    FileInfo currentFileInfo;
    FileDescrLine fileDescr;
    const wxString m_optionalLineBreak;
    StatusHandler* statusUpdater;
};


inline
wxString formatTime(unsigned int number)
{
    assert (number < 100);

    if (number <= 9)
    {
        wxChar result[3];

        *result = '0';
        result[1] = '0' + number;
        result[2] = 0;

        return result;
    }
    else
    {
        wxString result;
        if (result.Printf(wxT("%d"), number) < 0)
            throw RuntimeException(_("Error when converting int to wxString"));
        return result;
    }
}


void getFileInformation(FileInfo& output, const wxString& filename)
{
#ifdef FFS_WIN
    WIN32_FIND_DATA winFileInfo;
    FILETIME localFileTime;
    SYSTEMTIME time;
    HANDLE fileHandle;

    if ((fileHandle = FindFirstFile(filename.c_str(), &winFileInfo)) == INVALID_HANDLE_VALUE)
        throw FileError(wxString(_("Could not retrieve file info for: ")) + wxT("\"") + filename + wxT("\""));

    FindClose(fileHandle);

    if (FileTimeToLocalFileTime(
                &winFileInfo.ftLastWriteTime, //pointer to UTC file time to convert
                &localFileTime 	              //pointer to converted file time
            ) == 0)
        throw RuntimeException(_("Error converting FILETIME to local FILETIME"));

    if (FileTimeToSystemTime(
                &localFileTime, //pointer to file time to convert
                &time 	        //pointer to structure to receive system time
            ) == 0)
        throw RuntimeException(_("Error converting FILETIME to SYSTEMTIME"));

    output.lastWriteTime = globalFunctions::numberToWxString(time.wYear) + wxT("-") +
                           formatTime(time.wMonth)      + wxT("-") +
                           formatTime(time.wDay)        + wxT("  ") +
                           formatTime(time.wHour)       + wxT(":") +
                           formatTime(time.wMinute)     + wxT(":") +
                           formatTime(time.wSecond);

    //local time
    output.lastWriteTimeRaw = wxULongLong(localFileTime.dwHighDateTime, localFileTime.dwLowDateTime);

    //reduce precision to 1 second (FILETIME has unit 10^-7 s)
    output.lastWriteTimeRaw/= 10000000; // <- time is used for comparison only: unit switched to seconds

    output.fileSize = wxULongLong(winFileInfo.nFileSizeHigh, winFileInfo.nFileSizeLow);

#else
    struct stat fileInfo;
    if (stat(filename.c_str(), &fileInfo) != 0)
        throw FileError(wxString(_("Could not retrieve file info for: ")) + wxT("\"") + filename + wxT("\""));

    tm* timeinfo;
    timeinfo = localtime(&fileInfo.st_mtime);
    char buffer [50];
    strftime(buffer,50,"%Y-%m-%d  %H:%M:%S",timeinfo);

    //local time
    output.lastWriteTime = buffer;

    //UTC time; unit: 1 second
    output.lastWriteTimeRaw = fileInfo.st_mtime;

    output.fileSize = fileInfo.st_size;
#endif
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

    static const unsigned int bufferSize = 1024 * 512; //512 kb seems to be the perfect buffer size
    unsigned char* buffer1;
    unsigned char* buffer2;
};


bool filesHaveSameContent(const wxString& filename1, const wxString& filename2)
{
    static MemoryAllocator memory;

    wxFFile file1(filename1.c_str(), wxT("rb"));
    if (!file1.IsOpened())
        throw FileError(wxString(_("Could not open file: ")) + wxT("\"") + filename1 + wxT("\""));

    wxFFile file2(filename2.c_str(), wxT("rb"));
    if (!file2.IsOpened()) //NO cleanup necessary for wxFFile
        throw FileError(wxString(_("Could not open file: ")) + wxT("\"") + filename2 + wxT("\""));

    do
    {
        size_t length1 = file1.Read(memory.buffer1, memory.bufferSize);
        if (file1.Error()) throw FileError(wxString(_("Error reading file: ")) + wxT("\"") + filename1 + wxT("\""));

        size_t length2 = file2.Read(memory.buffer2, memory.bufferSize);
        if (file2.Error()) throw FileError(wxString(_("Error reading file: ")) + wxT("\"") + filename2 + wxT("\""));

        if (length1 != length2 || memcmp(memory.buffer1, memory.buffer2, length1) != 0)
            return false;
    }
    while (!file1.Eof());

    if (!file2.Eof())
        return false;

    return true;
}


//handle execution of a method while updating the UI
class UpdateWhileComparing : public UpdateWhileExecuting
{
public:
    UpdateWhileComparing() {}
    ~UpdateWhileComparing() {}

    wxString file1;
    wxString file2;
    bool success;
    wxString errorMessage;
    bool result;

private:
    void longRunner() //virtual method implementation
    {
        try
        {
            result = filesHaveSameContent(file1, file2);
            success = true;
        }
        catch (FileError& error)
        {
            success = false;
            errorMessage = error.show();
        }
    }
};


bool filesHaveSameContentMultithreaded(const wxString& filename1, const wxString& filename2, StatusHandler* updateClass)
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

    return cmpAndUpdate.result;
}


void calcTotalDataForCompare(int& objectsTotal, double& dataTotal, const FileCompareResult& grid, const set<int>& rowsToCompare)
{
    dataTotal = 0;

    for (set<int>::iterator i = rowsToCompare.begin(); i != rowsToCompare.end(); ++i)
    {
        const FileCompareLine& gridline = grid[*i];

        dataTotal+= gridline.fileDescrLeft.fileSize.ToDouble();
        dataTotal+= gridline.fileDescrRight.fileSize.ToDouble();
    }

    objectsTotal = rowsToCompare.size() * 2;
}


void generateFileAndFolderDescriptions(DirectoryDescrType& output, const wxString& directory, const wxString& optionalLineBreak, StatusHandler* updateClass)
{
    assert (updateClass);

    while (true)
    {
        output.clear();

        //get all files and folders from directory (and subdirectories) + information
        GetAllFilesFull traverser(output, FreeFileSync::getFormattedDirectoryName(directory), optionalLineBreak, updateClass);
        wxDir dir(directory);

        if (dir.Traverse(traverser) != (size_t)-1)
            break;  //traversed successfully
        else
        {
            ErrorHandler::Response rv = updateClass->reportError(wxString(_("Error traversing directory ")) + wxT("\"") + directory + wxT("\""));
            if (rv == ErrorHandler::CONTINUE_NEXT)
                break;
            else if (rv == ErrorHandler::RETRY)
                ;   //continue with loop
            else
                assert (false);
        }
    }
}


struct DescrBufferLine
{
    wxString directoryName;
    DirectoryDescrType* directoryDesc;

#ifdef FFS_WIN
    //Windows does NOT distinguish between upper/lower-case
    bool operator>(const DescrBufferLine& b ) const
    {
        return (directoryName.CmpNoCase(b.directoryName) > 0);
    }
    bool operator<(const DescrBufferLine& b) const
    {
        return (directoryName.CmpNoCase(b.directoryName) < 0);
    }
    bool operator==(const DescrBufferLine& b) const
    {
        return (directoryName.CmpNoCase(b.directoryName) == 0);
    }

#elif defined FFS_LINUX
    //Linux DOES distinguish between upper/lower-case
    bool operator>(const DescrBufferLine& b ) const
    {
        return (directoryName.Cmp(b.directoryName) > 0);
    }
    bool operator<(const DescrBufferLine& b) const
    {
        return (directoryName.Cmp(b.directoryName) < 0);
    }
    bool operator==(const DescrBufferLine& b) const
    {
        return (directoryName.Cmp(b.directoryName) == 0);
    }
#else
    adapt this
#endif

};


class DirectoryDescrBuffer  //buffer multiple scans of the same directories
{
public:
    ~DirectoryDescrBuffer()
    {
        //clean up
        for (set<DescrBufferLine>::iterator i = buffer.begin(); i != buffer.end(); ++i)
            delete i->directoryDesc;
    }

    const DirectoryDescrType* getDirectoryDescription(const wxString& directory, const wxString& optionalLineBreak, StatusHandler* statusUpdater)
    {
        DescrBufferLine bufferEntry;
        bufferEntry.directoryName = directory;

        set<DescrBufferLine>::iterator entryFound;
        if ((entryFound = buffer.find(bufferEntry)) != buffer.end())
        {
            //entry found in buffer; return
            return entryFound->directoryDesc;
        }
        else
        {
            //entry not found; create new one
            bufferEntry.directoryDesc = new DirectoryDescrType;
            buffer.insert(bufferEntry); //exception safety: insert into buffer right after creation!

            generateFileAndFolderDescriptions(*bufferEntry.directoryDesc, directory, optionalLineBreak, statusUpdater); //exceptions may be thrown!
            return bufferEntry.directoryDesc;
        }
    }

private:
    set<DescrBufferLine> buffer;
};


void CompareProcess::startCompareProcess(const vector<FolderPair>& directoryPairsFormatted,
        const CompareVariant cmpVar,
        FileCompareResult& output) throw(AbortThisProcess)
{
#ifndef __WXDEBUG__
    wxLogNull noWxLogs; //hide wxWidgets log messages in release build
#endif
    assert (statusUpdater);

//################################################################################################################################################

    FileCompareResult output_tmp; //write to output not before END of process!

    try
    {
        //inform about the total amount of data that will be processed from now on
        statusUpdater->initNewProcess(-1, 0, StatusHandler::PROCESS_SCANNING); //it's not known how many files will be scanned => -1 objects

        //do basis scan: only result lines of type FILE_UNDEFINED need to be determined
        performBaseComparison(directoryPairsFormatted,
                              output_tmp);

        if (cmpVar == CMP_BY_TIME_SIZE)
        {
            for (FileCompareResult::iterator i = output_tmp.begin(); i != output_tmp.end(); ++i)
            {
                if (i->cmpResult == FILE_UNDEFINED)
                {
                    //last write time may differ by up to 2 seconds (NTFS vs FAT32)
                    if (sameFileTime(i->fileDescrLeft.lastWriteTimeRaw, i->fileDescrRight.lastWriteTimeRaw))
                    {
                        if (i->fileDescrLeft.fileSize == i->fileDescrRight.fileSize)
                            i->cmpResult = FILE_EQUAL;
                        else
                            i->cmpResult = FILE_DIFFERENT;
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
//################################################################################################################################################
        else if (cmpVar == CMP_BY_CONTENT)
        {
            set<int> rowsToCompareBytewise; //compare of file content happens AFTER finding corresponding files
            //in order to separate into two processes (scanning and comparing)

            //pre-check: files have different content if they have a different filesize
            for (FileCompareResult::iterator i = output_tmp.begin(); i != output_tmp.end(); ++i)
            {
                if (i->cmpResult == FILE_UNDEFINED)
                {
                    if (i->fileDescrLeft.fileSize != i->fileDescrRight.fileSize)
                        i->cmpResult = FILE_DIFFERENT;
                    else
                        rowsToCompareBytewise.insert(i - output_tmp.begin());
                }
            }

            int objectsTotal = 0;
            double dataTotal = 0;
            calcTotalDataForCompare(objectsTotal, dataTotal, output_tmp, rowsToCompareBytewise);

            statusUpdater->initNewProcess(objectsTotal, dataTotal, StatusHandler::PROCESS_COMPARING_CONTENT);

            set<int> rowsToDelete;  //if errors occur during file access and user skips, these rows need to be deleted from result

            //compare files (that have same size) bytewise...
            for (set<int>::iterator i = rowsToCompareBytewise.begin(); i != rowsToCompareBytewise.end(); ++i)
            {
                FileCompareLine& gridline = output_tmp[*i];

                statusUpdater->updateStatusText(wxString(_("Comparing content of files ")) + optionalLineBreak + wxT("\"") + gridline.fileDescrLeft.relativeName + wxT("\""));

                //check files that exist in left and right model but have different content
                while (true)
                {
                    //trigger display refresh
                    statusUpdater->requestUiRefresh();

                    try
                    {
                        if (filesHaveSameContentMultithreaded(gridline.fileDescrLeft.fullName, gridline.fileDescrRight.fullName, statusUpdater))
                            gridline.cmpResult = FILE_EQUAL;
                        else
                            gridline.cmpResult = FILE_DIFFERENT;

                        statusUpdater->updateProcessedData(2, (gridline.fileDescrLeft.fileSize * 2).ToDouble());
                        break;
                    }
                    catch (FileError& error)
                    {
                        ErrorHandler::Response rv = statusUpdater->reportError(error.show());
                        if (rv == ErrorHandler::CONTINUE_NEXT)
                        {
                            rowsToDelete.insert(*i);
                            break;
                        }
                        else if (rv == ErrorHandler::RETRY)
                            ;   //continue with loop
                        else
                            assert (false);
                    }
                }
            }

            //delete invalid rows that have no valid cmpResult
            if (rowsToDelete.size() > 0)
                removeRowsFromVector(output_tmp, rowsToDelete);
        }
        else assert(false);


        statusUpdater->requestUiRefresh();
    }
    catch (const RuntimeException& theException)
    {
        wxMessageBox(theException.show(), _("An exception occured!"), wxOK | wxICON_ERROR);
        return;
    }
    catch (std::bad_alloc& e)
    {
        wxMessageBox(wxString(_("System out of memory!")) + wxT(" ") + wxString::From8BitData(e.what()), _("An exception occured!"), wxOK | wxICON_ERROR);
        return;
    }

    //only if everything was processed correctly output is written to!
    output_tmp.swap(output);
}


void CompareProcess::performBaseComparison(const vector<FolderPair>& directoryPairsFormatted,
        FileCompareResult& output)
{
    //buffer accesses to the same directories; useful when multiple folder pairs are used
    DirectoryDescrBuffer descriptionBuffer;

    //process one folder pair after each other
    for (vector<FolderPair>::const_iterator pair = directoryPairsFormatted.begin(); pair != directoryPairsFormatted.end(); ++pair)
    {
        //retrieve sets of files (with description data)
        const DirectoryDescrType* directoryLeft  = descriptionBuffer.getDirectoryDescription(pair->leftDirectory, optionalLineBreak, statusUpdater);
        const DirectoryDescrType* directoryRight = descriptionBuffer.getDirectoryDescription(pair->rightDirectory, optionalLineBreak, statusUpdater);

        statusUpdater->forceUiRefresh();
        FileCompareLine newline;

        //find files/folders that exist in left file model but not in right model
        for (DirectoryDescrType::iterator i = directoryLeft->begin(); i != directoryLeft->end(); ++i)
            if (directoryRight->find(*i) == directoryRight->end())
            {
                newline.fileDescrLeft            = *i;
                newline.fileDescrRight           = FileDescrLine();
                newline.fileDescrRight.directory = pair->rightDirectory;
                newline.cmpResult                = FILE_LEFT_SIDE_ONLY;
                output.push_back(newline);
            }

        for (DirectoryDescrType::iterator j = directoryRight->begin(); j != directoryRight->end(); ++j)
        {
            DirectoryDescrType::iterator i;

            //find files/folders that exist in right file model but not in left model
            if ((i = directoryLeft->find(*j)) == directoryLeft->end())
            {
                newline.fileDescrLeft           = FileDescrLine();
                newline.fileDescrLeft.directory = pair->leftDirectory;  //directory info is needed when creating new directories
                newline.fileDescrRight          = *j;
                newline.cmpResult               = FILE_RIGHT_SIDE_ONLY;
                output.push_back(newline);
            }
            //find files/folders that exist in left and right file model
            else
            {   //files...
                if (i->objType == FileDescrLine::TYPE_FILE && j->objType == FileDescrLine::TYPE_FILE)
                {
                    newline.fileDescrLeft  = *i;
                    newline.fileDescrRight = *j;
                    newline.cmpResult      = FILE_UNDEFINED; //not yet determined!
                    output.push_back(newline);
                }
                //directories...
                else if (i->objType == FileDescrLine::TYPE_DIRECTORY && j->objType == FileDescrLine::TYPE_DIRECTORY)
                {
                    newline.fileDescrLeft  = *i;
                    newline.fileDescrRight = *j;
                    newline.cmpResult      = FILE_EQUAL;
                    output.push_back(newline);
                }
                //if we have a nameclash between a file and a directory: split into two separate rows
                else if (i->objType != j->objType)
                {
                    newline.fileDescrLeft            = *i;
                    newline.fileDescrRight           = FileDescrLine();
                    newline.fileDescrRight.directory = pair->rightDirectory;
                    newline.cmpResult                = FILE_LEFT_SIDE_ONLY;
                    output.push_back(newline);

                    newline.fileDescrLeft           = FileDescrLine();
                    newline.fileDescrLeft.directory = pair->leftDirectory;
                    newline.fileDescrRight          = *j;
                    newline.cmpResult               = FILE_RIGHT_SIDE_ONLY;
                    output.push_back(newline);
                }
                else assert (false);
            }
        }
    }
    statusUpdater->requestUiRefresh();
}


GetAllFilesFull::GetAllFilesFull(DirectoryDescrType& output, wxString dirThatIsSearched, const wxString& optionalLineBreak, StatusHandler* updateClass) :
        m_output(output),
        directory(dirThatIsSearched),
        m_optionalLineBreak(optionalLineBreak),
        statusUpdater(updateClass)
{
    assert(updateClass);
    prefixLength = directory.Len();
}


wxDirTraverseResult GetAllFilesFull::OnFile(const wxString& filename)
{
    fileDescr.fullName     = filename;
    fileDescr.directory    = directory;
    fileDescr.relativeName = filename.Mid(prefixLength);
    while (true)
    {
        try
        {
            getFileInformation(currentFileInfo, filename);
            break;
        }
        catch (FileError& error)
        {
            //if (updateClass) -> is mandatory
            ErrorHandler::Response rv = statusUpdater->reportError(error.show());
            if ( rv == ErrorHandler::CONTINUE_NEXT)
                return wxDIR_CONTINUE;
            else if (rv == ErrorHandler::RETRY)
                ;   //continue with loop
            else
                assert (false);
        }
    }

    fileDescr.lastWriteTime    = currentFileInfo.lastWriteTime;
    fileDescr.lastWriteTimeRaw = currentFileInfo.lastWriteTimeRaw;
    fileDescr.fileSize         = currentFileInfo.fileSize;
    fileDescr.objType          = FileDescrLine::TYPE_FILE;
    m_output.insert(fileDescr);

    //update UI/commandline status information
    statusUpdater->updateStatusText(wxString(_("Scanning ")) + m_optionalLineBreak + wxT("\"") + filename + wxT("\"")); //NO performance issue at all
    //add 1 element to the progress indicator
    statusUpdater->updateProcessedData(1, 0); //NO performance issue at all
    //trigger display refresh
    statusUpdater->requestUiRefresh();

    return wxDIR_CONTINUE;
}


wxDirTraverseResult GetAllFilesFull::OnDir(const wxString& dirname)
{
#ifdef FFS_WIN
    if (    dirname.EndsWith(wxT("\\RECYCLER")) ||
            dirname.EndsWith(wxT("\\System Volume Information")))
        return wxDIR_IGNORE;
#endif  // FFS_WIN

    fileDescr.fullName     = dirname;
    fileDescr.directory    = directory;
    fileDescr.relativeName = dirname.Mid(prefixLength);
    fileDescr.lastWriteTime    = wxEmptyString;   //irrelevant for directories
    fileDescr.lastWriteTimeRaw = wxULongLong(0);  //irrelevant for directories
    fileDescr.fileSize         = wxULongLong(0);  //currently used by getBytesToTransfer
    fileDescr.objType          = FileDescrLine::TYPE_DIRECTORY;
    m_output.insert(fileDescr);

    //update UI/commandline status information
    statusUpdater->updateStatusText(wxString(_("Scanning ")) + m_optionalLineBreak + wxT("\"") + dirname + wxT("\"")); //NO performance issue at all
    //add 1 element to the progress indicator
    statusUpdater->updateProcessedData(1, 0);     //NO performance issue at all
    //trigger display refresh
    statusUpdater->requestUiRefresh();

    return wxDIR_CONTINUE;
}


wxDirTraverseResult GetAllFilesFull::OnOpenError(const wxString& openerrorname)
{
    wxMessageBox(openerrorname, _("Error"));
    return wxDIR_IGNORE;
}


bool FreeFileSync::foldersAreValidForComparison(const vector<FolderPair>& folderPairs, wxString& errorMessage)
{
    errorMessage.Clear();

    for (vector<FolderPair>::const_iterator i = folderPairs.begin(); i != folderPairs.end(); ++i)
    {
        const wxString leftFolderName  = getFormattedDirectoryName(i->leftDirectory);
        const wxString rightFolderName = getFormattedDirectoryName(i->rightDirectory);

        //check if folder name is empty
        if (leftFolderName.IsEmpty() || rightFolderName.IsEmpty())
        {
            errorMessage = _("Please fill all empty directory fields.");
            return false;
        }

        //check if folder exists
        if (!wxDirExists(leftFolderName))
        {
            errorMessage = wxString(_("Directory does not exist: ")) + wxT("\"") + leftFolderName + wxT("\"");
            return false;
        }
        if (!wxDirExists(rightFolderName))
        {
            errorMessage = wxString(_("Directory does not exist: ")) + wxT("\"") + rightFolderName + wxT("\"");
            return false;
        }
    }
    return true;
}


bool dependencyExists(const vector<wxString>& folders, const wxString& newFolder, wxString& warningMessage)
{
    warningMessage.Clear();

    for (vector<wxString>::const_iterator i = folders.begin(); i != folders.end(); ++i)
        if (newFolder.StartsWith(*i) || i->StartsWith(newFolder))
        {
            warningMessage = wxString(_("Directories are dependent: ")) +
                             wxT("\"") + *i + wxT("\"") + wxT(", ") + wxT("\"") + newFolder + wxT("\"");
            return true;
        }

    return false;
}


bool FreeFileSync::foldersHaveDependencies(const vector<FolderPair>& folderPairs, wxString& warningMessage)
{
    warningMessage.Clear();

    vector<wxString> folders;
    for (vector<FolderPair>::const_iterator i = folderPairs.begin(); i != folderPairs.end(); ++i)
    {
        const wxString leftFolderName  = getFormattedDirectoryName(i->leftDirectory);
        const wxString rightFolderName = getFormattedDirectoryName(i->rightDirectory);

        if (dependencyExists(folders, leftFolderName, warningMessage))
            return true;
        folders.push_back(leftFolderName);

        if (dependencyExists(folders, rightFolderName, warningMessage))
            return true;
        folders.push_back(rightFolderName);
    }

    return false;
}

