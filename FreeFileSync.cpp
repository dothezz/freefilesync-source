#include <wx/arrstr.h>
#include <wx/dir.h>
#include <wx/msgdlg.h>
#include "FreeFileSync.h"
#include "library/globalFunctions.h"
#include <wx/filename.h>
#include "library/resources.h"
#include <sys/stat.h>
#include <wx/ffile.h>

#ifdef FFS_WIN
#include <windows.h>
#endif  // FFS_WIN

#ifdef FFS_LINUX
#include <utime.h>
#endif  // FFS_LINUX


using namespace globalFunctions;


MainConfiguration::MainConfiguration() :
        compareVar(CMP_BY_TIME_SIZE),
        filterIsActive(false),        //do not filter by default
        includeFilter(wxT("*")),      //include all files/folders
        excludeFilter(wxEmptyString), //exclude nothing
        useRecycleBin(FreeFileSync::recycleBinExists()), //enable if OS supports it; else user will have to activate first and then get an error message
        continueOnError(false)
{}


struct FileInfo
{
    wxULongLong fileSize; //unit: bytes!
    wxString lastWriteTime;
    wxULongLong lastWriteTimeRaw; //unit: seconds!
};


//some special file functions
void generateFileAndFolderDescriptions(DirectoryDescrType& output, const wxString& directory, StatusHandler* updateClass = 0);
void getFileInformation(FileInfo& output, const wxString& filename);


//Note: the following lines are a performance optimization for deleting elements from a vector. It is incredibly faster to create a new
//vector and leave specific elements out than to delete row by row and force recopying of most elements for each single deletion (linear vs quadratic runtime)
template <class T>
void removeRowsFromVector(vector<T>& grid, const set<int>& rowsToRemove)
{
    vector<T> temp;
    int rowToSkip = -1; //keep it an INT!

    set<int>::iterator rowToSkipIndex = rowsToRemove.begin();

    if (rowToSkipIndex != rowsToRemove.end())
        rowToSkip = *rowToSkipIndex;

    for (int i = 0; i < int(grid.size()); ++i)
    {
        if (i != rowToSkip)
            temp.push_back(grid[i]);
        else
        {
            ++rowToSkipIndex;
            if (rowToSkipIndex != rowsToRemove.end())
                rowToSkip = *rowToSkipIndex;
        }
    }
    grid.swap(temp);
}


class GetAllFilesFull : public wxDirTraverser
{
public:
    GetAllFilesFull(DirectoryDescrType& output, wxString dirThatIsSearched, StatusHandler* updateClass = 0);

    wxDirTraverseResult OnFile(const wxString& filename);

    wxDirTraverseResult OnDir(const wxString& dirname);

    wxDirTraverseResult OnOpenError(const wxString& openerrorname);

private:
    DirectoryDescrType& m_output;
    wxString directory;
    int prefixLength;
    FileInfo currentFileInfo;
    FileDescrLine fileDescr;
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

    output.lastWriteTime = numberToWxString(time.wYear) + wxT("-") +
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


void generateFileAndFolderDescriptions(DirectoryDescrType& output, const wxString& directory, StatusHandler* updateClass)
{
    assert (updateClass);

    while (true)
    {
        output.clear();

        //get all files and folders from directory (and subdirectories) + information
        GetAllFilesFull traverser(output, FreeFileSync::getFormattedDirectoryName(directory), updateClass);
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

    const DirectoryDescrType* getDirectoryDescription(const wxString& directory, StatusHandler* statusUpdater)
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

            generateFileAndFolderDescriptions(*bufferEntry.directoryDesc, directory, statusUpdater); //exceptions may be thrown!
            return bufferEntry.directoryDesc;
        }
    }

private:
    set<DescrBufferLine> buffer;
};


void FreeFileSync::startCompareProcess(const vector<FolderPair>& directoryPairsFormatted,
                                       const CompareVariant cmpVar,
                                       FileCompareResult& output,
                                       StatusHandler* statusUpdater)
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
        FreeFileSync::performBaseComparison(directoryPairsFormatted,
                                            output_tmp,
                                            statusUpdater);

        if (cmpVar == CMP_BY_TIME_SIZE)
        {
            for (FileCompareResult::iterator i = output_tmp.begin(); i != output_tmp.end(); ++i)
            {
                if (i->cmpResult == FILE_UNDEFINED)
                {
                    //last write time may differ by up to 2 seconds (NTFS vs FAT32)
                    bool sameWriteTime;
                    if (i->fileDescrLeft.lastWriteTimeRaw < i->fileDescrRight.lastWriteTimeRaw)
                        sameWriteTime = (i->fileDescrRight.lastWriteTimeRaw - i->fileDescrLeft.lastWriteTimeRaw <= 2);
                    else
                        sameWriteTime = (i->fileDescrLeft.lastWriteTimeRaw - i->fileDescrRight.lastWriteTimeRaw <= 2);

                    if (sameWriteTime)
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

            //compair files (that have same size) bytewise...
            for (set<int>::iterator i = rowsToCompareBytewise.begin(); i != rowsToCompareBytewise.end(); ++i)
            {
                FileCompareLine& gridline = output_tmp[*i];

                statusUpdater->updateStatusText(wxString(_("Comparing content of files ")) + wxT("\"") + gridline.fileDescrLeft.relativeName + wxT("\""));

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


void FreeFileSync::performBaseComparison(const vector<FolderPair>& directoryPairsFormatted,
        FileCompareResult& output,
        StatusHandler* statusUpdater)
{
    //buffer accesses to the same directories; useful when multiple folder pairs are used
    DirectoryDescrBuffer descriptionBuffer;

    //process one folder pair after each other
    for (vector<FolderPair>::const_iterator pair = directoryPairsFormatted.begin(); pair != directoryPairsFormatted.end(); ++pair)
    {
        //retrieve sets of files (with description data)
        const DirectoryDescrType* directoryLeft  = descriptionBuffer.getDirectoryDescription(pair->leftDirectory, statusUpdater);
        const DirectoryDescrType* directoryRight = descriptionBuffer.getDirectoryDescription(pair->rightDirectory, statusUpdater);

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


void FreeFileSync::swapGrids(FileCompareResult& grid)
{
    FileDescrLine tmp;

    for (FileCompareResult::iterator i = grid.begin(); i != grid.end(); ++i)
    {
        //swap compare result
        if (i->cmpResult == FILE_LEFT_SIDE_ONLY)
            i->cmpResult = FILE_RIGHT_SIDE_ONLY;
        else if (i->cmpResult == FILE_RIGHT_SIDE_ONLY)
            i->cmpResult = FILE_LEFT_SIDE_ONLY;
        else if (i->cmpResult == FILE_RIGHT_NEWER)
            i->cmpResult = FILE_LEFT_NEWER;
        else if (i->cmpResult == FILE_LEFT_NEWER)
            i->cmpResult = FILE_RIGHT_NEWER;

        //swap file descriptors
        tmp = i->fileDescrLeft;
        i->fileDescrLeft = i->fileDescrRight;
        i->fileDescrRight = tmp;
    }
}


GetAllFilesFull::GetAllFilesFull(DirectoryDescrType& output, wxString dirThatIsSearched, StatusHandler* updateClass)
        : m_output(output), directory(dirThatIsSearched), statusUpdater(updateClass)
{
    assert(updateClass);
    prefixLength = directory.Len();
}


wxDirTraverseResult GetAllFilesFull::OnFile(const wxString& filename)
{
    fileDescr.fullName     = filename;
    fileDescr.directory    = directory;
    fileDescr.relativeName = filename.Mid(prefixLength, wxSTRING_MAXLEN);
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
    statusUpdater->updateStatusText(wxString(_("Scanning ")) + wxT("\"") + filename + wxT("\"")); //NO performance issue at all
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
    fileDescr.relativeName = dirname.Mid(prefixLength, wxSTRING_MAXLEN);
    fileDescr.lastWriteTime    = wxEmptyString;   //irrelevant for directories
    fileDescr.lastWriteTimeRaw = wxULongLong(0);  //irrelevant for directories
    fileDescr.fileSize         = wxULongLong(0);  //currently used by getBytesToTransfer
    fileDescr.objType          = FileDescrLine::TYPE_DIRECTORY;
    m_output.insert(fileDescr);

    //update UI/commandline status information
    statusUpdater->updateStatusText(wxString(_("Scanning ")) + wxT("\"") + dirname + wxT("\"")); //NO performance issue at all
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


//#####################################################################################################
//file functions

class GetAllFilesSimple : public wxDirTraverser
{
public:
    GetAllFilesSimple(wxArrayString& files, wxArrayString& subDirectories) :
            m_files(files),
            m_dirs(subDirectories)
    {}

    wxDirTraverseResult OnDir(const wxString& dirname)
    {
        m_dirs.Add(dirname);
        return wxDIR_CONTINUE;
    }

    wxDirTraverseResult OnFile(const wxString& filename)
    {
        m_files.Add(filename);
        return wxDIR_CONTINUE;
    }

    wxDirTraverseResult OnOpenError(const wxString& openerrorname)
    {
        wxMessageBox(openerrorname, _("Error"));
        return wxDIR_IGNORE;
    }

private:
    wxArrayString& m_files;
    wxArrayString& m_dirs;
};


class RecycleBin
{
public:
    RecycleBin() :
            recycleBinAvailable(false)
    {
#ifdef FFS_WIN
        recycleBinAvailable = true;
#endif  // FFS_WIN
    }

    ~RecycleBin() {}

    bool recycleBinExists()
    {
        return recycleBinAvailable;
    }

    void moveToRecycleBin(const wxString& filename)
    {
        if (!recycleBinAvailable)   //this method should ONLY be called if recycle bin is available
            throw RuntimeException(_("Initialization of Recycle Bin failed! It cannot be used!"));

#ifdef FFS_WIN
        wxString filenameDoubleNull = filename + wxChar(0);

        SHFILEOPSTRUCT fileOp;
        fileOp.hwnd   = NULL;
        fileOp.wFunc  = FO_DELETE;
        fileOp.pFrom  = filenameDoubleNull.c_str();
        fileOp.pTo    = NULL;
        fileOp.fFlags = FOF_ALLOWUNDO | FOF_NOCONFIRMATION | FOF_SILENT;
        fileOp.fAnyOperationsAborted = false;
        fileOp.hNameMappings         = NULL;
        fileOp.lpszProgressTitle     = NULL;

        if (SHFileOperation(&fileOp   //pointer to an SHFILEOPSTRUCT structure that contains information the function needs to carry out
                           ) != 0 || fileOp.fAnyOperationsAborted) throw FileError(wxString(_("Error moving to recycle bin: ")) + wxT("\"") + filename + wxT("\""));
#endif  // FFS_WIN
    }

private:
    bool recycleBinAvailable;
};


RecycleBin FreeFileSync::recycler;


inline
void FreeFileSync::removeFile(const wxString& filename, const bool useRecycleBin)
{
    if (!wxFileExists(filename)) return; //this is NOT an error situation: the manual deletion relies on it!

    if (useRecycleBin)
    {
        recycler.moveToRecycleBin(filename);
        return;
    }

#ifdef FFS_WIN
    if (!SetFileAttributes(
                filename.c_str(),     //address of filename
                FILE_ATTRIBUTE_NORMAL //attributes to set
            )) throw FileError(wxString(_("Error deleting file ")) + wxT("\"") + filename + wxT("\""));
#endif  //FFS_WIN

    if (!wxRemoveFile(filename))
        throw FileError(wxString(_("Error deleting file ")) + wxT("\"") + filename + wxT("\""));
}


void FreeFileSync::removeDirectory(const wxString& directory, const bool useRecycleBin)
{
    if (!wxDirExists(directory)) return; //this is NOT an error situation: the manual deletion relies on it!

    if (useRecycleBin)
    {
        recycler.moveToRecycleBin(directory);
        return;
    }

    wxArrayString fileList;
    wxArrayString dirList;

    {   //own scope for directory access to not disturb deletion!
        wxDir dir(directory);

        //get all files and directories from current directory (and subdirectories)
        GetAllFilesSimple traverser(fileList, dirList);
        if (dir.Traverse(traverser) == (size_t)-1)
            throw FileError(wxString(_("Error deleting directory ")) + wxT("\"") + directory + wxT("\""));
    }

    for (unsigned int j = 0; j < fileList.GetCount(); ++j)
        removeFile(fileList[j], useRecycleBin);

    dirList.Insert(directory, 0);   //this directory will be deleted last

    for (int j = int(dirList.GetCount()) - 1; j >= 0 ; --j)
    {
#ifdef FFS_WIN
        if (!SetFileAttributes(
                    dirList[j].c_str(),   // address of directory name
                    FILE_ATTRIBUTE_NORMAL // attributes to set
                )) throw FileError(wxString(_("Error deleting directory ")) + wxT("\"") + dirList[j] + wxT("\""));
#endif  // FFS_WIN

        if (!wxRmdir(dirList[j]))
            throw FileError(wxString(_("Error deleting directory ")) + wxT("\"") + dirList[j] + wxT("\""));
    }
}


void FreeFileSync::createDirectory(const wxString& directory, int level)
{
    if (wxDirExists(directory))
        return;

    if (level == 50) //catch endless loop
        return;

    //try to create directory
    if (wxMkdir(directory))
        return;

    //if not successfull try to create containing folders first
    wxString createFirstDir = wxDir(directory).GetName().BeforeLast(GlobalResources::fileNameSeparator);

    //call function recursively
    if (createFirstDir.IsEmpty()) return;
    createDirectory(createFirstDir, level + 1);

    //now creation should be possible
    if (!wxMkdir(directory))
    {
        if (level == 0)
            throw FileError(wxString(_("Error creating directory ")) + wxT("\"") + directory + wxT("\""));
    }
}

/*
void FreeFileSync::copyCreatingDirs(const wxString& source, const wxString& target)
{
    wxString targetPath = wxFileName(target).GetPath();
    createDirectory(targetPath);

    if (!wxCopyFile(source, target, false)) //error if file exists
        throw FileError(wxString(_("Error copying file ")) + "\"" + source + "\"" + _(" to ") + "\"" + target + "\"");
}
*/
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
    void longRunner() //virtual method implementation
    {
        if (!wxCopyFile(source, target, false)) //abort if file exists
        {
            success = false;
            errorMessage = wxString(_("Error copying file ")) + wxT("\"") + source + wxT("\"") + _(" to ") + wxT("\"") + target + wxT("\"");
            return;
        }

#ifdef FFS_LINUX //copying files with Linux does not preserve the modification time => adapt after copying
        struct stat fileInfo;
        if (stat(source.c_str(), &fileInfo) != 0) //read modification time from source file
        {
            success = false;
            errorMessage = wxString(_("Could not retrieve file info for: ")) + wxT("\"") + source + wxT("\"");
            return;
        }

        utimbuf newTimes;
        newTimes.actime  = fileInfo.st_mtime;
        newTimes.modtime = fileInfo.st_mtime;

        if (utime(target.c_str(), &newTimes) != 0)
        {
            success = false;
            errorMessage = wxString(_("Error adapting modification time of file ")) + wxT("\"") + target + wxT("\"");
            return;
        }
#endif  // FFS_LINUX

        success = true;
    }
};


void FreeFileSync::copyfileMultithreaded(const wxString& source, const wxString& target, StatusHandler* updateClass)
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


bool FreeFileSync::recycleBinExists()
{
    return recycler.recycleBinExists();
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


bool FreeFileSync::synchronizeFile(const FileCompareLine& cmpLine, const SyncConfiguration& config, const bool useRecycleBin, StatusHandler* statusUpdater)
{   //false if nothing was to be done
    assert (statusUpdater);

    if (!cmpLine.selectedForSynchronization) return false;

    wxString target;

    //synchronize file:
    switch (cmpLine.cmpResult)
    {
    case FILE_LEFT_SIDE_ONLY:
        switch (config.exLeftSideOnly)
        {
        case SyncConfiguration::SYNC_DIR_LEFT:   //delete files on left
            statusUpdater->updateStatusText(wxString(_("Deleting file ")) + wxT("\"") + cmpLine.fileDescrLeft.fullName + wxT("\""));
            removeFile(cmpLine.fileDescrLeft.fullName, useRecycleBin);
            break;
        case SyncConfiguration::SYNC_DIR_RIGHT:  //copy files to right
            target = cmpLine.fileDescrRight.directory + cmpLine.fileDescrLeft.relativeName;
            statusUpdater->updateStatusText(wxString(_("Copying file ")) + wxT("\"") + cmpLine.fileDescrLeft.fullName + wxT("\"") +
                                            _(" to ") + wxT("\"") + target + wxT("\""));

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
            statusUpdater->updateStatusText(wxString(_("Copying file ")) + wxT("\"") + cmpLine.fileDescrRight.fullName + wxT("\"") +
                                            _(" to ") + wxT("\"") + target + wxT("\""));

            copyfileMultithreaded(cmpLine.fileDescrRight.fullName, target, statusUpdater);
            break;
        case SyncConfiguration::SYNC_DIR_RIGHT:  //delete files on right
            statusUpdater->updateStatusText(wxString(_("Deleting file ")) + wxT("\"") + cmpLine.fileDescrRight.fullName + wxT("\""));
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
            statusUpdater->updateStatusText(wxString(_("Copying file ")) + wxT("\"") + cmpLine.fileDescrRight.fullName + wxT("\"") +
                                            _(" overwriting ") + wxT("\"") + cmpLine.fileDescrLeft.fullName + wxT("\""));

            removeFile(cmpLine.fileDescrLeft.fullName, useRecycleBin);  //only used if switch activated by user, else file is simply deleted
            copyfileMultithreaded(cmpLine.fileDescrRight.fullName, cmpLine.fileDescrLeft.fullName, statusUpdater);
            break;
        case SyncConfiguration::SYNC_DIR_RIGHT:  //copy from left to right
            statusUpdater->updateStatusText(wxString(_("Copying file ")) + wxT("\"") + cmpLine.fileDescrLeft.fullName + wxT("\"") +
                                            _(" overwriting ") + wxT("\"") + cmpLine.fileDescrRight.fullName + wxT("\""));

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


bool FreeFileSync::synchronizeFolder(const FileCompareLine& cmpLine, const SyncConfiguration& config, const bool useRecycleBin, StatusHandler* statusUpdater)
{   //false if nothing was to be done
    assert (statusUpdater);

    if (!cmpLine.selectedForSynchronization) return false;

    wxString target;

    //synchronize folders:
    switch (cmpLine.cmpResult)
    {
    case FILE_LEFT_SIDE_ONLY:
        switch (config.exLeftSideOnly)
        {
        case SyncConfiguration::SYNC_DIR_LEFT:   //delete folders on left
            statusUpdater->updateStatusText(wxString(_("Deleting folder ")) + wxT("\"") + cmpLine.fileDescrLeft.fullName + wxT("\""));
            removeDirectory(cmpLine.fileDescrLeft.fullName, useRecycleBin);
            break;
        case SyncConfiguration::SYNC_DIR_RIGHT:  //create folders on right
            target = cmpLine.fileDescrRight.directory + cmpLine.fileDescrLeft.relativeName;
            statusUpdater->updateStatusText(wxString(_("Creating folder ")) + wxT("\"") + target + wxT("\""));

            //some check to catch the error that directory on source has been deleted externally after the "compare"...
            if (!wxDirExists(cmpLine.fileDescrLeft.fullName))
                throw FileError(wxString(_("Error: Source directory does not exist anymore: ")) + wxT("\"") + cmpLine.fileDescrLeft.fullName + wxT("\""));
            createDirectory(target);
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
            statusUpdater->updateStatusText(wxString(_("Creating folder ")) + wxT("\"") + target + wxT("\""));

            //some check to catch the error that directory on source has been deleted externally after the "compare"...
            if (!wxDirExists(cmpLine.fileDescrRight.fullName))
                throw FileError(wxString(_("Error: Source directory does not exist anymore: ")) + wxT("\"") + cmpLine.fileDescrRight.fullName + wxT("\""));
            createDirectory(target);
            break;
        case SyncConfiguration::SYNC_DIR_RIGHT:  //delete folders on right
            statusUpdater->updateStatusText(wxString(_("Deleting folder ")) + wxT("\"") + cmpLine.fileDescrRight.fullName + wxT("\""));
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


wxString FreeFileSync::formatFilesizeToShortString(const wxULongLong& filesize)
{
    return formatFilesizeToShortString(filesize.ToDouble());
}


wxString FreeFileSync::formatFilesizeToShortString(const double filesize)
{
    double nrOfBytes = filesize;

    wxString unit = _(" Byte");
    if (nrOfBytes > 999)
    {
        nrOfBytes/= 1024;
        unit = _(" kB");
        if (nrOfBytes > 999)
        {
            nrOfBytes/= 1024;
            unit = _(" MB");
            if (nrOfBytes > 999)
            {
                nrOfBytes/= 1024;
                unit = _(" GB");
                if (nrOfBytes > 999)
                {
                    nrOfBytes/= 1024;
                    unit = _(" TB");
                    if (nrOfBytes > 999)
                    {
                        nrOfBytes/= 1024;
                        unit = _(" PB");
                    }
                }
            }
        }
    }

    wxString temp;
    if (unit == _(" Byte")) //no decimal places in case of bytes
    {
        double integer = 0;
        modf(nrOfBytes, &integer); //get integer part of nrOfBytes
        temp = numberToWxString(int(integer));
    }
    else
    {
        nrOfBytes*= 100;    //we want up to two decimal places
        double integer = 0;
        modf(nrOfBytes, &integer); //get integer part of nrOfBytes

        temp = numberToWxString(int(integer));

        int length = temp.Len();

        switch (length)
        {
        case 0:
            temp = _("Error");
            break;
        case 1:
            temp = wxString(wxT("0")) + GlobalResources::decimalPoint + wxT("0") + temp;
            break; //0,01
        case 2:
            temp = wxString(wxT("0")) + GlobalResources::decimalPoint + temp;
            break; //0,11
        case 3:
            temp.insert(1, GlobalResources::decimalPoint);
            break; //1,11
        case 4:
            temp = temp.substr(0, 3);
            temp.insert(2, GlobalResources::decimalPoint);
            break; //11,1
        case 5:
            temp = temp.substr(0, 3);
            break; //111
        default:
            return _("Error");
        }
    }
    return (temp + unit);
}


void compoundStringToTable(const wxString& compoundInput, const wxChar* delimiter, vector<wxString>& output)
{
    output.clear();
    wxString input(compoundInput);

    //make sure input ends with delimiter - no problem with empty strings here
    if (!input.EndsWith(delimiter))
        input+= delimiter;

    unsigned int indexStart = 0;
    unsigned int indexEnd   = 0;
    while ((indexEnd = input.find(delimiter, indexStart )) != string::npos)
    {
        if (indexStart != indexEnd) //do not add empty strings
        {
            wxString newEntry = input.substr(indexStart, indexEnd - indexStart);

            newEntry.Trim(true);  //remove whitespace characters from right
            newEntry.Trim(false); //remove whitespace characters from left

            if (!newEntry.IsEmpty())
                output.push_back(newEntry);
        }
        indexStart = indexEnd + 1;
    }
}


inline
void formatFilterString(wxString& filter)
{
#ifdef FFS_WIN
    //Windows does NOT distinguish between upper/lower-case
    filter.MakeLower();
#elif defined FFS_LINUX
    //Linux DOES distinguish between upper/lower-case
//nothing to do here
#else
    assert(false);
#endif
}


inline
void formatFilenameString(wxString& filename)
{
#ifdef FFS_WIN
    //Windows does NOT distinguish between upper/lower-case
    filename.MakeLower();
#elif defined FFS_LINUX
    //Linux DOES distinguish between upper/lower-case
//nothing to do here
#else
    assert(false);
#endif
}


inline
void mergeVectors(vector<wxString>& changing, const vector<wxString>& input)
{
    for (vector<wxString>::const_iterator i = input.begin(); i != input.end(); ++i)
        changing.push_back(*i);
}


vector<wxString> FreeFileSync::compoundStringToFilter(const wxString& filterString)
{
    //delimiters may be ';' or '\n'
    vector<wxString> filterList;
    vector<wxString> filterPreProcessing;
    compoundStringToTable(filterString, wxT(";"), filterPreProcessing);

    for (vector<wxString>::const_iterator i = filterPreProcessing.begin(); i != filterPreProcessing.end(); ++i)
    {
        vector<wxString> newEntries;
        compoundStringToTable(*i, wxT("\n"), newEntries);
        mergeVectors(filterList, newEntries);
    }

    return filterList;
}


void FreeFileSync::filterCurrentGridData(FileCompareResult& currentGridData, const wxString& includeFilter, const wxString& excludeFilter)
{
    //load filter into vectors of strings
    //delimiters may be ';' or '\n'
    vector<wxString> includeList = FreeFileSync::compoundStringToFilter(includeFilter);
    vector<wxString> excludeList = FreeFileSync::compoundStringToFilter(excludeFilter);

    //format entries
    for (vector<wxString>::iterator i = includeList.begin(); i != includeList.end(); ++i)
        formatFilterString(*i);
    for (vector<wxString>::iterator i = excludeList.begin(); i != excludeList.end(); ++i)
        formatFilterString(*i);

//##############################################################

    //filter currentGridData
    for (FileCompareResult::iterator i = currentGridData.begin(); i != currentGridData.end(); ++i)
    {
        wxString filenameLeft  = i->fileDescrLeft.fullName;
        wxString filenameRight = i->fileDescrRight.fullName;

        formatFilenameString(filenameLeft);
        formatFilenameString(filenameRight);

        //process include filters
        if (i->fileDescrLeft.objType != FileDescrLine::TYPE_NOTHING)
        {
            bool includedLeft = false;
            for (vector<wxString>::const_iterator j = includeList.begin(); j != includeList.end(); ++j)
                if (filenameLeft.Matches(*j))
                {
                    includedLeft = true;
                    break;
                }

            if (!includedLeft)
            {
                i->selectedForSynchronization = false;
                continue;
            }
        }

        if (i->fileDescrRight.objType != FileDescrLine::TYPE_NOTHING)
        {
            bool includedRight = false;
            for (vector<wxString>::const_iterator j = includeList.begin(); j != includeList.end(); ++j)
                if (filenameRight.Matches(*j))
                {
                    includedRight = true;
                    break;
                }

            if (!includedRight)
            {
                i->selectedForSynchronization = false;
                continue;
            }
        }

        //process exclude filters
        bool excluded = false;
        for (vector<wxString>::const_iterator j = excludeList.begin(); j != excludeList.end(); ++j)
            if (filenameLeft.Matches(*j) || filenameRight.Matches(*j))
            {
                excluded = true;
                break;
            }

        if (excluded)
        {
            i->selectedForSynchronization = false;
            continue;
        }

        i->selectedForSynchronization = true;
    }
}


void FreeFileSync::removeFilterOnCurrentGridData(FileCompareResult& currentGridData)
{
    //remove all filters on currentGridData
    for (FileCompareResult::iterator i = currentGridData.begin(); i != currentGridData.end(); ++i)
        i->selectedForSynchronization = true;
}


wxString FreeFileSync::getFormattedDirectoryName(const wxString& dirname)
{   //Formatting is needed since functions in FreeFileSync.cpp expect the directory to end with '\' to be able to split the relative names.
    //Also improves usability.

    wxString dirnameTmp = dirname;
    dirnameTmp.Trim(true);  //remove whitespace characters from right
    dirnameTmp.Trim(false); //remove whitespace characters from left

    if (dirnameTmp.IsEmpty()) //an empty string is interpreted as "\"; this is not desired
        return wxEmptyString;

    //let wxWidgets do the directory formatting, e.g. replace '/' with '\' for Windows
    wxString result = wxDir(dirnameTmp).GetName();

    result.Append(GlobalResources::fileNameSeparator);
    return result;
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
void FreeFileSync::startSynchronizationProcess(FileCompareResult& grid, const SyncConfiguration& config, StatusHandler* statusUpdater, const bool useRecycleBin)
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
                        if (FreeFileSync::synchronizeFolder(*i, config, useRecycleBin, statusUpdater))
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

                        if ( rv == ErrorHandler::CONTINUE_NEXT)
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
                    if (deleteLoop && deletionImminent(*i, config) ||
                            !deleteLoop && !deletionImminent(*i, config))
                    {
                        while (true)
                        {    //trigger display refresh
                            statusUpdater->requestUiRefresh();

                            try
                            {
                                if (FreeFileSync::synchronizeFile(*i, config, useRecycleBin, statusUpdater))
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

                                if ( rv == ErrorHandler::CONTINUE_NEXT)
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


//add(!) all files and subfolder gridlines that are dependent from the directory
void FreeFileSync::addSubElements(set<int>& subElements, const FileCompareResult& grid, const FileCompareLine& relevantRow)
{
    wxString relevantDirectory;

    if (relevantRow.fileDescrLeft.objType == FileDescrLine::TYPE_DIRECTORY)
        relevantDirectory = relevantRow.fileDescrLeft.relativeName + GlobalResources::fileNameSeparator; //fileNameSeparator needed to exclude subfile/dirs only

    else if (relevantRow.fileDescrRight.objType == FileDescrLine::TYPE_DIRECTORY)
        relevantDirectory = relevantRow.fileDescrRight.relativeName + GlobalResources::fileNameSeparator;

    else
        return;

    for (FileCompareResult::const_iterator i = grid.begin(); i != grid.end(); ++i)
        if (    i->fileDescrLeft.relativeName.StartsWith(relevantDirectory) ||
                i->fileDescrRight.relativeName.StartsWith(relevantDirectory))
            subElements.insert(i - grid.begin());
}


void FreeFileSync::deleteOnGridAndHD(FileCompareResult& grid, const set<int>& rowsToDelete, ErrorHandler* errorHandler, const bool useRecycleBin)
{
    //remove deleted rows from grid
    AlwaysWriteToGrid writeOutput(grid);  //ensure that grid is always written to, even if method is exitted via exceptions

    //remove from hd
    for (set<int>::iterator i = rowsToDelete.begin(); i != rowsToDelete.end(); ++i)
    {
        const FileCompareLine& currentCmpLine = grid[*i];

        while (true)
        {
            try
            {
                if (currentCmpLine.fileDescrLeft.objType == FileDescrLine::TYPE_FILE)
                    FreeFileSync::removeFile(currentCmpLine.fileDescrLeft.fullName, useRecycleBin);
                else if (currentCmpLine.fileDescrLeft.objType == FileDescrLine::TYPE_DIRECTORY)
                    FreeFileSync::removeDirectory(currentCmpLine.fileDescrLeft.fullName, useRecycleBin);

                if (currentCmpLine.fileDescrRight.objType == FileDescrLine::TYPE_FILE)
                    FreeFileSync::removeFile(currentCmpLine.fileDescrRight.fullName, useRecycleBin);
                else if (currentCmpLine.fileDescrRight.objType == FileDescrLine::TYPE_DIRECTORY)
                    FreeFileSync::removeDirectory(currentCmpLine.fileDescrRight.fullName, useRecycleBin);

                //remove deleted row from grid
                writeOutput.rowProcessedSuccessfully(*i);

                //retrieve all files and subfolder gridlines that are dependent from this deleted entry
                set<int> additionalRowsToDelete;
                addSubElements(additionalRowsToDelete, grid, grid[*i]);

                //...and remove them also
                for (set<int>::iterator j = additionalRowsToDelete.begin(); j != additionalRowsToDelete.end(); ++j)
                    writeOutput.rowProcessedSuccessfully(*j);

                break;
            }
            catch (FileError& error)
            {
                //if (updateClass) -> is mandatory
                ErrorHandler::Response rv = errorHandler->reportError(error.show());

                if (rv == ErrorHandler::CONTINUE_NEXT)
                    break;

                else if (rv == ErrorHandler::RETRY)
                    ;   //continue in loop
                else
                    assert (false);
            }
        }
    }
}


bool FreeFileSync::foldersAreValidForComparison(const vector<FolderPair>& folderPairs, wxString& errorMessage)
{
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


void FreeFileSync::adjustModificationTimes(const wxString& parentDirectory, const int timeInSeconds, ErrorHandler* errorHandler)
{
    if (timeInSeconds == 0)
        return;

    wxArrayString fileList;
    wxArrayString dirList;

    while (true)  //own scope for directory access to not disturb file access!
    {
        fileList.Clear();
        dirList.Clear();

        //get all files and folders from directory (and subdirectories)
        GetAllFilesSimple traverser(fileList, dirList);

        wxDir dir(parentDirectory);
        if (dir.Traverse(traverser) != (size_t)-1)
            break;  //traversed successfully
        else
        {
            ErrorHandler::Response rv = errorHandler->reportError(wxString(_("Error traversing directory ")) + wxT("\"") + parentDirectory + wxT("\""));
            if (rv == ErrorHandler::CONTINUE_NEXT)
                break;
            else if (rv == ErrorHandler::RETRY)
                ;   //continue with loop
            else
                assert (false);
        }
    }


    //this part is only a bit slower than direct Windows API-access!
    wxDateTime modTime;
    for (unsigned int j = 0; j < fileList.GetCount(); ++j)
    {
        while (true)  //own scope for directory access to not disturb file access!
        {
            try
            {
                wxFileName file(fileList[j]);
                if (!file.GetTimes(NULL, &modTime, NULL)) //get modification time
                    throw FileError(wxString(_("Error changing modification time: ")) + wxT("\"") + fileList[j] + wxT("\""));

                modTime.Add(wxTimeSpan(0, 0, timeInSeconds, 0));

                if (!file.SetTimes(NULL, &modTime, NULL)) //get modification time
                    throw FileError(wxString(_("Error changing modification time: ")) + wxT("\"") + fileList[j] + wxT("\""));

                break;
            }
            catch (const FileError& error)
            {
                ErrorHandler::Response rv = errorHandler->reportError(error.show());
                if (rv == ErrorHandler::CONTINUE_NEXT)
                    break;
                else if (rv == ErrorHandler::RETRY)
                    ;   //continue with loop
                else
                    assert (false);
            }
        }
    }
}








