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

const wxString FreeFileSync::FfsLastConfigFile = wxT("LastRun.ffs_gui");

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


void FreeFileSync::getFileInformation(FileInfo& output, const wxString& filename)
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


void FreeFileSync::generateFileAndFolderDescriptions(DirectoryDescrType& output, const wxString& directory, StatusUpdater* updateClass)
{
    assert (updateClass);

    while (true)
    {
        output.clear();

        //get all files and folders from directory (and subdirectories) + information
        GetAllFilesFull traverser(output, getFormattedDirectoryName(directory), updateClass);
        wxDir dir(directory);

        if (dir.Traverse(traverser) != (size_t)-1)
            break;  //traversed successfully
        else
        {
            int rv = updateClass->reportError(wxString(_("Error traversing directory ")) + wxT("\"") + directory + wxT("\""));
            if (rv == StatusUpdater::continueNext)
                break;
            else if (rv == StatusUpdater::retry)
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


bool filesHaveSameContentMultithreaded(const wxString& filename1, const wxString& filename2, StatusUpdater* updateClass)
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

    const DirectoryDescrType* getDirectoryDescription(const wxString& directory, StatusUpdater* statusUpdater)
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
            FreeFileSync::generateFileAndFolderDescriptions(*bufferEntry.directoryDesc, directory, statusUpdater);
            buffer.insert(bufferEntry);

            return bufferEntry.directoryDesc;
        }
    }

private:
    set<DescrBufferLine> buffer;
};


void FreeFileSync::startCompareProcess(FileCompareResult& output,
                                       const vector<FolderPair>& directoryPairsFormatted,
                                       const CompareVariant cmpVar,
                                       StatusUpdater* statusUpdater)
{
#ifndef __WXDEBUG__
    wxLogNull dummy; //hide wxWidgets log messages in release build
#endif

    assert (statusUpdater);

//################################################################################################################################################

    //inform about the total amount of data that will be processed from now on
    statusUpdater->initNewProcess(-1, 0, FreeFileSync::scanningFilesProcess); //it's not known how many files will be scanned => -1 objects

    FileCompareResult output_tmp;   //write to output not before END of process!
    set<int> delayedContentCompare; //compare of file content happens AFTER finding corresponding files
    //in order to separate into two processes (needed by progress indicators)

    //buffer accesses to the same directories; useful when multiple folder pairs are used
    DirectoryDescrBuffer descriptionBuffer;

    try
    {
        //process one folder pair after each other
        for (vector<FolderPair>::const_iterator pair = directoryPairsFormatted.begin(); pair != directoryPairsFormatted.end(); ++pair)
        {
            //unsigned int startTime = GetTickCount();

            //retrieve sets of files (with description data)
            const DirectoryDescrType* directoryLeft  = descriptionBuffer.getDirectoryDescription(pair->leftDirectory, statusUpdater);
            const DirectoryDescrType* directoryRight = descriptionBuffer.getDirectoryDescription(pair->rightDirectory, statusUpdater);

            //wxMessageBox(numberToWxString(unsigned(GetTickCount()) - startTime));
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
                    output_tmp.push_back(newline);
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
                    output_tmp.push_back(newline);
                }
                //find files that exist in left and right file model
                else
                {   //objType != TYPE_NOTHING
                    if (i->objType == TYPE_DIRECTORY && j->objType == TYPE_DIRECTORY)
                    {
                        newline.fileDescrLeft  = *i;
                        newline.fileDescrRight = *j;
                        newline.cmpResult      = FILE_EQUAL;
                        output_tmp.push_back(newline);
                    }
                    //if we have a nameclash between a file and a directory: split into separate rows
                    else if (i->objType != j->objType)
                    {
                        newline.fileDescrLeft            = *i;
                        newline.fileDescrRight           = FileDescrLine();
                        newline.fileDescrRight.directory = pair->rightDirectory;
                        newline.cmpResult                = FILE_LEFT_SIDE_ONLY;
                        output_tmp.push_back(newline);

                        newline.fileDescrLeft           = FileDescrLine();
                        newline.fileDescrLeft.directory = pair->leftDirectory;
                        newline.fileDescrRight          = *j;
                        newline.cmpResult               = FILE_RIGHT_SIDE_ONLY;
                        output_tmp.push_back(newline);
                    }
                    else if (cmpVar == CMP_BY_TIME_SIZE)
                    {  //check files that exist in left and right model but have different properties

                        //last write time may differ by up to 2 seconds (NTFS vs FAT32)
                        bool sameWriteTime;
                        if (i->lastWriteTimeRaw < j->lastWriteTimeRaw)
                            sameWriteTime = (j->lastWriteTimeRaw - i->lastWriteTimeRaw <= 2);
                        else
                            sameWriteTime = (i->lastWriteTimeRaw - j->lastWriteTimeRaw <= 2);

                        newline.fileDescrLeft  = *i;
                        newline.fileDescrRight = *j;
                        if (sameWriteTime)
                        {
                            if (i->fileSize == j->fileSize)
                                newline.cmpResult      = FILE_EQUAL;
                            else
                                newline.cmpResult      = FILE_DIFFERENT;
                        }
                        else
                        {
                            if (i->lastWriteTimeRaw < j->lastWriteTimeRaw)
                                newline.cmpResult      = FILE_RIGHT_NEWER;
                            else
                                newline.cmpResult      = FILE_LEFT_NEWER;
                        }
                        output_tmp.push_back(newline);
                    }
                    else if (cmpVar == CMP_BY_CONTENT)
                    {   //check files that exist in left and right model but have different content

                        //check filesize first!
                        if (i->fileSize == j->fileSize)
                        {
                            newline.fileDescrLeft  = *i;
                            newline.fileDescrRight = *j;
                            newline.cmpResult      = FILE_INVALID; //not yet determined
                            output_tmp.push_back(newline);

                            //compare by content is only needed if filesizes are the same
                            delayedContentCompare.insert(output_tmp.size() - 1); //save index of row, to calculate cmpResult later
                        }
                        else
                        {
                            newline.fileDescrLeft  = *i;
                            newline.fileDescrRight = *j;
                            newline.cmpResult      = FILE_DIFFERENT;
                            output_tmp.push_back(newline);
                        }
                    }
                    else assert (false);
                }
            }
        }

//################################################################################################################################################

        //compare file contents and set value "cmpResult"

        //unsigned int startTime3 = GetTickCount();
        if (cmpVar == CMP_BY_CONTENT)
        {
            int objectsTotal = 0;
            double dataTotal = 0;
            calcTotalDataForCompare(objectsTotal, dataTotal, output_tmp, delayedContentCompare);

            statusUpdater->initNewProcess(objectsTotal, dataTotal, FreeFileSync::compareFileContentProcess);

            set<int> rowsToDelete;  //if errors occur during file access and user skips, these rows need to be deleted from result

            for (set<int>::iterator i = delayedContentCompare.begin(); i != delayedContentCompare.end(); ++i)
            {
                FileCompareLine& gridline = output_tmp[*i];

                statusUpdater->updateStatusText(wxString(_("Comparing content of files ")) + wxT("\"") + gridline.fileDescrLeft.relFilename + wxT("\""));

                //check files that exist in left and right model but have different content
                while (true)
                {
                    //trigger display refresh
                    statusUpdater->requestUiRefresh();

                    try
                    {
                        if (filesHaveSameContentMultithreaded(gridline.fileDescrLeft.filename, gridline.fileDescrRight.filename, statusUpdater))
                            gridline.cmpResult = FILE_EQUAL;
                        else
                            gridline.cmpResult = FILE_DIFFERENT;

                        statusUpdater->updateProcessedData(2, (gridline.fileDescrLeft.fileSize + gridline.fileDescrRight.fileSize).ToDouble());
                        break;
                    }
                    catch (FileError& error)
                    {
                        //if (updateClass) -> is mandatory
                        int rv = statusUpdater->reportError(error.show());
                        if (rv == StatusUpdater::continueNext)
                        {
                            rowsToDelete.insert(*i);
                            break;
                        }
                        else if (rv == StatusUpdater::retry)
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
        //wxMessageBox(numberToWxString(unsigned(GetTickCount()) - startTime3));

        statusUpdater->requestUiRefresh();
    }
    catch (const RuntimeException& theException)
    {
        wxMessageBox(theException.show(), _("An exception occured!"), wxOK | wxICON_ERROR);
        return;
    }

    //only if everything was processed correctly output is written to!
    output_tmp.swap(output);
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


GetAllFilesFull::GetAllFilesFull(DirectoryDescrType& output, wxString dirThatIsSearched, StatusUpdater* updateClass)
        : m_output(output), directory(dirThatIsSearched), statusUpdater(updateClass)
{
    assert(updateClass);
    prefixLength = directory.Len();
}


wxDirTraverseResult GetAllFilesFull::OnFile(const wxString& filename)
{
    fileDescr.filename         = filename;
    fileDescr.directory        = directory;
    fileDescr.relFilename      = filename.Mid(prefixLength, wxSTRING_MAXLEN);
    while (true)
    {
        try
        {
            FreeFileSync::getFileInformation(currentFileInfo, filename);
            break;
        }
        catch (FileError& error)
        {
            //if (updateClass) -> is mandatory
            int rv = statusUpdater->reportError(error.show());
            if ( rv == StatusUpdater::continueNext)
                return wxDIR_CONTINUE;
            else if (rv == StatusUpdater::retry)
                ;   //continue with loop
            else
                assert (false);
        }
    }

    fileDescr.lastWriteTime    = currentFileInfo.lastWriteTime;
    fileDescr.lastWriteTimeRaw = currentFileInfo.lastWriteTimeRaw;
    fileDescr.fileSize         = currentFileInfo.fileSize;
    fileDescr.objType          = TYPE_FILE;
    m_output.insert(fileDescr);

    //update UI/commandline status information
    statusUpdater->updateStatusText(wxString(_("Scanning ")) + wxT("\"") + filename + wxT("\"")); // NO performance issue at all
    //add 1 element to the progress indicator
    statusUpdater->updateProcessedData(1, 0);     // NO performance issue at all
    //trigger display refresh
    statusUpdater->requestUiRefresh();

    return wxDIR_CONTINUE;
}


wxDirTraverseResult GetAllFilesFull::OnDir(const wxString& dirname)
{
#ifdef FFS_WIN
    if (dirname.EndsWith(wxT("\\RECYCLER")) ||
            dirname.EndsWith(wxT("\\System Volume Information")))
        return wxDIR_IGNORE;
#endif  // FFS_WIN

    fileDescr.filename    = dirname;
    fileDescr.directory   = directory;
    fileDescr.relFilename = dirname.Mid(prefixLength, wxSTRING_MAXLEN);
    while (true)
    {
        try
        {
            FreeFileSync::getFileInformation(currentFileInfo, dirname);
            break;
        }
        catch (FileError& error)
        {
            //if (updateClass) -> is mandatory
            int rv = statusUpdater->reportError(error.show());
            if ( rv == StatusUpdater::continueNext)
                return wxDIR_IGNORE;
            else if (rv == StatusUpdater::retry)
                ;   //continue with loop
            else
                assert (false);
        }
    }
    fileDescr.lastWriteTime    = currentFileInfo.lastWriteTime;
    fileDescr.lastWriteTimeRaw = currentFileInfo.lastWriteTimeRaw;
    fileDescr.fileSize         = wxULongLong(0);     //currentFileInfo.fileSize should be "0" as well, but just to be sure... currently used by getBytesToTransfer
    fileDescr.objType          = TYPE_DIRECTORY;
    m_output.insert(fileDescr);

    //update UI/commandline status information
    statusUpdater->updateStatusText(wxString(_("Scanning ")) + wxT("\"") + dirname + wxT("\"")); // NO performance issue at all
    //add 1 element to the progress indicator
    statusUpdater->updateProcessedData(1, 0);     // NO performance issue at all
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
            allFiles(files),
            subdirs(subDirectories)
    {}

    wxDirTraverseResult OnDir(const wxString& dirname)
    {
        subdirs.Add(dirname);
        return wxDIR_CONTINUE;
    }

    wxDirTraverseResult OnFile(const wxString& filename)
    {
        allFiles.Add(filename);
        return wxDIR_CONTINUE;
    }

    wxDirTraverseResult OnOpenError(const wxString& openerrorname)
    {
        wxMessageBox(openerrorname, _("Error"));
        return wxDIR_IGNORE;
    }

private:
    wxArrayString& allFiles;
    wxArrayString& subdirs;
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
                           ) != 0 || fileOp.fAnyOperationsAborted) throw FileError(wxString(_("Error moving file to recycle bin: ")) + wxT("\"") + filename + wxT("\""));
#endif  // FFS_WIN
    }

private:
    bool recycleBinAvailable;
};


RecycleBin FreeFileSync::recycler;


inline
void FreeFileSync::removeFile(const wxString& filename)
{
    if (!wxFileExists(filename)) return; //this is NOT an error situation: the manual deletion relies on it!

    if (recycleBinShouldBeUsed)
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


void FreeFileSync::removeDirectory(const wxString& directory)
{
    if (!wxDirExists(directory)) return; //this is NOT an error situation: the manual deletion relies on it!

    if (recycleBinShouldBeUsed)
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
        removeFile(fileList[j]);

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

    if (level == 50)    //catch endless loop
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

#ifndef __WXDEBUG__
    //prevent wxWidgets logging
    wxLogNull noWxLogs;
#endif
};


void FreeFileSync::copyfileMultithreaded(const wxString& source, const wxString& target, StatusUpdater* updateClass)
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


FreeFileSync::FreeFileSync(bool useRecycleBin) :
        recycleBinShouldBeUsed(useRecycleBin) {}

FreeFileSync::~FreeFileSync() {}


bool FreeFileSync::recycleBinExists()
{
    return recycler.recycleBinExists();
}


inline
SyncDirection getSyncDirection(const CompareFilesResult cmpResult, const SyncConfiguration& config)
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
    return SYNC_DIR_NONE;
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
        case SYNC_DIR_LEFT:   //delete file on left
            dataToProcess   = 0;
            objectsToDelete = 1;
            break;
        case SYNC_DIR_RIGHT:  //copy from left to right
            dataToProcess = fileCmpLine.fileDescrLeft.fileSize.ToDouble();
            objectsToCreate = 1;
            break;
        case SYNC_DIR_NONE:
            return false;
        }
        break;

    case FILE_RIGHT_SIDE_ONLY:
        switch (getSyncDirection(fileCmpLine.cmpResult, config))
        {
        case SYNC_DIR_LEFT:   //copy from right to left
            dataToProcess   = fileCmpLine.fileDescrRight.fileSize.ToDouble();;
            objectsToCreate = 1;
            break;
        case SYNC_DIR_RIGHT:  //delete file on right
            dataToProcess   = 0;
            objectsToDelete = 1;
            break;
        case SYNC_DIR_NONE:
            return false;
        }
        break;

    case FILE_LEFT_NEWER:
    case FILE_RIGHT_NEWER:
    case FILE_DIFFERENT:
        //get data to process
        switch (getSyncDirection(fileCmpLine.cmpResult, config))
        {
        case SYNC_DIR_LEFT:   //copy from right to left
            dataToProcess = fileCmpLine.fileDescrRight.fileSize.ToDouble();
            objectsToOverwrite = 1;
            break;
        case SYNC_DIR_RIGHT:  //copy from left to right
            dataToProcess = fileCmpLine.fileDescrLeft.fileSize.ToDouble();
            objectsToOverwrite = 1;
            break;
        case SYNC_DIR_NONE:
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


bool FreeFileSync::synchronizeFile(const FileCompareLine& filename, const SyncConfiguration& config, StatusUpdater* statusUpdater)
{   //false if nothing was to be done
    assert (statusUpdater);

    if (!filename.selectedForSynchronization) return false;

    wxString target;

    //synchronize file:
    switch (filename.cmpResult)
    {
    case FILE_LEFT_SIDE_ONLY:
        switch (config.exLeftSideOnly)
        {
        case SYNC_DIR_LEFT:   //delete files on left
            statusUpdater->updateStatusText(wxString(_("Deleting file ")) + wxT("\"") + filename.fileDescrLeft.filename + wxT("\""));
            removeFile(filename.fileDescrLeft.filename);
            break;
        case SYNC_DIR_RIGHT:  //copy files to right
            target = filename.fileDescrRight.directory + filename.fileDescrLeft.relFilename;
            statusUpdater->updateStatusText(wxString(_("Copying file ")) + wxT("\"") + filename.fileDescrLeft.filename + wxT("\"") +
                                            _(" to ") + wxT("\"") + target + wxT("\""));

            copyfileMultithreaded(filename.fileDescrLeft.filename, target, statusUpdater);
            break;
        case SYNC_DIR_NONE:
            return false;
        default:
            assert (false);
        }
        break;

    case FILE_RIGHT_SIDE_ONLY:
        switch (config.exRightSideOnly)
        {
        case SYNC_DIR_LEFT:   //copy files to left
            target = filename.fileDescrLeft.directory + filename.fileDescrRight.relFilename;
            statusUpdater->updateStatusText(wxString(_("Copying file ")) + wxT("\"") + filename.fileDescrRight.filename + wxT("\"") +
                                            _(" to ") + wxT("\"") + target + wxT("\""));

            copyfileMultithreaded(filename.fileDescrRight.filename, target, statusUpdater);
            break;
        case SYNC_DIR_RIGHT:  //delete files on right
            statusUpdater->updateStatusText(wxString(_("Deleting file ")) + wxT("\"") + filename.fileDescrRight.filename + wxT("\""));
            removeFile(filename.fileDescrRight.filename);
            break;
        case SYNC_DIR_NONE:
            return false;
        default:
            assert (false);
        }
        break;

    case FILE_LEFT_NEWER:
    case FILE_RIGHT_NEWER:
    case FILE_DIFFERENT:
        switch (getSyncDirection(filename.cmpResult, config))
        {
        case SYNC_DIR_LEFT:   //copy from right to left
            statusUpdater->updateStatusText(wxString(_("Copying file ")) + wxT("\"") + filename.fileDescrRight.filename + wxT("\"") +
                                            _(" overwriting ") + wxT("\"") + filename.fileDescrLeft.filename + wxT("\""));

            removeFile(filename.fileDescrLeft.filename);  //only used if switch activated by user, else file is simply deleted
            copyfileMultithreaded(filename.fileDescrRight.filename, filename.fileDescrLeft.filename, statusUpdater);
            break;
        case SYNC_DIR_RIGHT:  //copy from left to right
            statusUpdater->updateStatusText(wxString(_("Copying file ")) + wxT("\"") + filename.fileDescrLeft.filename + wxT("\"") +
                                            _(" overwriting ") + wxT("\"") + filename.fileDescrRight.filename + wxT("\""));

            removeFile(filename.fileDescrRight.filename);  //only used if switch activated by user, else file is simply deleted
            copyfileMultithreaded(filename.fileDescrLeft.filename, filename.fileDescrRight.filename, statusUpdater);
            break;
        case SYNC_DIR_NONE:
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


bool FreeFileSync::synchronizeFolder(const FileCompareLine& filename, const SyncConfiguration& config, StatusUpdater* statusUpdater)
{   //false if nothing was to be done
    assert (statusUpdater);

    if (!filename.selectedForSynchronization) return false;

    wxString target;

    //synchronize folders:
    switch (filename.cmpResult)
    {
    case FILE_LEFT_SIDE_ONLY:
        switch (config.exLeftSideOnly)
        {
        case SYNC_DIR_LEFT:   //delete folders on left
            statusUpdater->updateStatusText(wxString(_("Deleting folder ")) + wxT("\"") + filename.fileDescrLeft.filename + wxT("\""));
            removeDirectory(filename.fileDescrLeft.filename);
            break;
        case SYNC_DIR_RIGHT:  //create folders on right
            target = filename.fileDescrRight.directory + filename.fileDescrLeft.relFilename;
            statusUpdater->updateStatusText(wxString(_("Creating folder ")) + wxT("\"") + target + wxT("\""));

            //some check to catch the error that directory on source has been deleted externally after the "compare"...
            if (!wxDirExists(filename.fileDescrLeft.filename))
                throw FileError(wxString(_("Error: Source directory does not exist anymore: ")) + wxT("\"") + filename.fileDescrLeft.filename + wxT("\""));
            createDirectory(target);
            break;
        case SYNC_DIR_NONE:
            return false;
        default:
            assert (false);
        }
        break;

    case FILE_RIGHT_SIDE_ONLY:
        switch (config.exRightSideOnly)
        {
        case SYNC_DIR_LEFT:   //create folders on left
            target = filename.fileDescrLeft.directory + filename.fileDescrRight.relFilename;
            statusUpdater->updateStatusText(wxString(_("Creating folder ")) + wxT("\"") + target + wxT("\""));

            //some check to catch the error that directory on source has been deleted externally after the "compare"...
            if (!wxDirExists(filename.fileDescrRight.filename))
                throw FileError(wxString(_("Error: Source directory does not exist anymore: ")) + wxT("\"") + filename.fileDescrRight.filename + wxT("\""));
            createDirectory(target);
            break;
        case SYNC_DIR_RIGHT:  //delete folders on right
            statusUpdater->updateStatusText(wxString(_("Deleting folder ")) + wxT("\"") + filename.fileDescrRight.filename + wxT("\""));
            removeDirectory(filename.fileDescrRight.filename);
            break;
        case SYNC_DIR_NONE:
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


vector<wxString> FreeFileSync::compoundStringToTable(const wxString& compoundInput, const wxChar* delimiter)
{
    wxString input(compoundInput);
    vector<wxString> output;

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

    return output;
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


void FreeFileSync::filterCurrentGridData(FileCompareResult& currentGridData, const wxString& includeFilter, const wxString& excludeFilter)
{
    //load filters into vectors
    //delimiters may be ';' or '\n'
    vector<wxString> includeList;
    vector<wxString> includePreProcessing = compoundStringToTable(includeFilter, wxT(";"));
    for (vector<wxString>::const_iterator i = includePreProcessing.begin(); i != includePreProcessing.end(); ++i)
    {
        vector<wxString> newEntries = compoundStringToTable(*i, wxT("\n"));
        mergeVectors(includeList, newEntries);
    }

    vector<wxString> excludeList;
    vector<wxString> excludePreProcessing = compoundStringToTable(excludeFilter, wxT(";"));
    for (vector<wxString>::const_iterator i = excludePreProcessing.begin(); i != excludePreProcessing.end(); ++i)
    {
        vector<wxString> newEntries = compoundStringToTable(*i, wxT("\n"));
        mergeVectors(excludeList, newEntries);
    }

    //format entries
    for (vector<wxString>::iterator i = includeList.begin(); i != includeList.end(); ++i)
        formatFilterString(*i);

    for (vector<wxString>::iterator i = excludeList.begin(); i != excludeList.end(); ++i)
        formatFilterString(*i);

//##############################################################

    //filter currentGridData
    for (FileCompareResult::iterator i = currentGridData.begin(); i != currentGridData.end(); ++i)
    {
        wxString filenameLeft  = i->fileDescrLeft.filename;
        wxString filenameRight = i->fileDescrRight.filename;

        formatFilenameString(filenameLeft);
        formatFilenameString(filenameRight);

        //process include filters
        if (i->fileDescrLeft.objType != TYPE_NOTHING)
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

        if (i->fileDescrRight.objType != TYPE_NOTHING)
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
    if ((line.cmpResult == FILE_LEFT_SIDE_ONLY && config.exLeftSideOnly == SYNC_DIR_LEFT) ||
            (line.cmpResult == FILE_RIGHT_SIDE_ONLY && config.exRightSideOnly == SYNC_DIR_RIGHT))
        return true;
    else
        return false;
}


class AlwaysWriteResult //this class ensures, that the result of the method below is ALWAYS written on exit, even if exceptions are thrown!
{
public:
    AlwaysWriteResult(FileCompareResult& grid) :
            gridToWrite(grid)
    {}

    ~AlwaysWriteResult()
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
void FreeFileSync::startSynchronizationProcess(FileCompareResult& grid, const SyncConfiguration& config, StatusUpdater* statusUpdater, bool useRecycleBin)
{
    assert (statusUpdater);

    AlwaysWriteResult writeOutput(grid);  //ensure that grid is always written to, even if method is exitted via exceptions

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

    statusUpdater->initNewProcess(objectsToCreate + objectsToOverwrite + objectsToDelete, dataToProcess, FreeFileSync::synchronizeFilesProcess);

    try
    {
        FreeFileSync fileSyncObject(useRecycleBin);  //currently only needed for recycle bin and wxLog suppression => do NOT declare static!

        // it should never happen, that a directory on left side has same name as file on right side. startCompareProcess() should take care of this
        // and split into two "exists on one side only" cases
        // Note: this case is not handled by this tool as this is considered to be a bug and must be solved by the user

        //synchronize folders:
        for (FileCompareResult::const_iterator i = grid.begin(); i != grid.end(); ++i)
        {
            if (i->fileDescrLeft.objType == TYPE_DIRECTORY || i->fileDescrRight.objType == TYPE_DIRECTORY)
            {
                while (true)
                {   //trigger display refresh
                    statusUpdater->requestUiRefresh();

                    try
                    {
                        if (fileSyncObject.synchronizeFolder(*i, config, statusUpdater))
                            //progress indicator update
                            //indicator is updated only if directory is synched correctly  (and if some sync was done)!
                            statusUpdater->updateProcessedData(1, 0);  //each call represents one processed file/directory

                        writeOutput.rowProcessedSuccessfully(i - grid.begin());
                        break;
                    }
                    catch (FileError& error)
                    {
                        //if (updateClass) -> is mandatory
                        int rv = statusUpdater->reportError(error.show());

                        if ( rv == StatusUpdater::continueNext)
                            break;
                        else if (rv == StatusUpdater::retry)
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
                if (i->fileDescrLeft.objType == TYPE_FILE || i->fileDescrRight.objType == TYPE_FILE)
                {
                    if (deleteLoop && deletionImminent(*i, config) ||
                            !deleteLoop && !deletionImminent(*i, config))
                    {
                        while (true)
                        {    //trigger display refresh
                            statusUpdater->requestUiRefresh();

                            try
                            {
                                if (fileSyncObject.synchronizeFile(*i, config, statusUpdater))
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
                                int rv = statusUpdater->reportError(error.show());

                                if ( rv == StatusUpdater::continueNext)
                                    break;
                                else if (rv == StatusUpdater::retry)
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

    if (relevantRow.fileDescrLeft.objType == TYPE_DIRECTORY)
        relevantDirectory = relevantRow.fileDescrLeft.relFilename;

    else if (relevantRow.fileDescrRight.objType == TYPE_DIRECTORY)
        relevantDirectory = relevantRow.fileDescrRight.relFilename;

    else
        return;

    for (FileCompareResult::const_iterator i = grid.begin(); i != grid.end(); ++i)
        if (    i->fileDescrLeft.relFilename.StartsWith(relevantDirectory) ||
                i->fileDescrRight.relFilename.StartsWith(relevantDirectory))
            subElements.insert(i - grid.begin());
}


void FreeFileSync::deleteOnGridAndHD(FileCompareResult& grid, const set<int>& rowsToDelete, StatusUpdater* statusUpdater, bool useRecycleBin)
{
    FreeFileSync fileSyncObject(useRecycleBin); //currently only needed for recycle bin and wxLog suppression => do NOT declare static!

    set<int> rowsToDeleteInGrid;

    //remove from hd
    for (set<int>::iterator i = rowsToDelete.begin(); i != rowsToDelete.end(); ++i)
    {
        const FileCompareLine& currentCmpLine = grid[*i];

        while (true)
        {
            try
            {
                if (currentCmpLine.fileDescrLeft.objType == TYPE_FILE)
                    fileSyncObject.removeFile(currentCmpLine.fileDescrLeft.filename);
                else if (currentCmpLine.fileDescrLeft.objType == TYPE_DIRECTORY)
                    fileSyncObject.removeDirectory(currentCmpLine.fileDescrLeft.filename);

                if (currentCmpLine.fileDescrRight.objType == TYPE_FILE)
                    fileSyncObject.removeFile(currentCmpLine.fileDescrRight.filename);
                else if (currentCmpLine.fileDescrRight.objType == TYPE_DIRECTORY)
                    fileSyncObject.removeDirectory(currentCmpLine.fileDescrRight.filename);

                rowsToDeleteInGrid.insert(*i);
                break;
            }
            catch (FileError& error)
            {
                //if (updateClass) -> is mandatory
                int rv = statusUpdater->reportError(error.show());

                if (rv == StatusUpdater::continueNext)
                    break;

                else if (rv == StatusUpdater::retry)
                    ;   //continue in loop
                else
                    assert (false);
            }
        }
    }

    //retrieve all files and subfolder gridlines that are dependent from deleted directories and add them to the list
    set<int> additionalRowsToDelete;
    for (set<int>::iterator i = rowsToDeleteInGrid.begin(); i != rowsToDeleteInGrid.end(); ++i)
        addSubElements(additionalRowsToDelete, grid, grid[*i]);

    for (set<int>::iterator i = additionalRowsToDelete.begin(); i != additionalRowsToDelete.end(); ++i)
        rowsToDeleteInGrid.insert(*i);

    //remove deleted rows from grid
    removeRowsFromVector(grid, rowsToDeleteInGrid);
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
            errorMessage = wxString(_("Directory does not exist. Please select a new one: ")) + wxT("\"") + leftFolderName + wxT("\"");
            return false;
        }
        if (!wxDirExists(rightFolderName))
        {
            errorMessage = wxString(_("Directory does not exist. Please select a new one: ")) + wxT("\"") + rightFolderName + wxT("\"");
            return false;
        }
    }
    return true;
}


