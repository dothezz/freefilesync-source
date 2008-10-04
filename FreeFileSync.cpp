#include <wx/arrstr.h>
#include <wx/dir.h>
#include <wx/msgdlg.h>
#include "FreeFileSync.h"
#include "library/globalFunctions.h"
#include <wx/filename.h>
#include <fstream>
#include "library/resources.h"
#include <sys/stat.h>

#ifdef FFS_WIN
#include <windows.h>
#endif  // FFS_WIN

#ifdef FFS_LINUX
#include <utime.h>
#endif  // FFS_LINUX


using namespace globalFunctions;

const wxString FreeFileSync::FFS_ConfigFileID   = "FFS_CONFIG";
const wxString FreeFileSync::FFS_LastConfigFile = "LastRun.ffs";

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
            throw std::runtime_error("Error when converting int to wxString");
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
        throw FileError(wxString(_("Could not retrieve file info for: ")) + "\"" + filename + "\"");

    FindClose(fileHandle);

    if (FileTimeToLocalFileTime(
                &winFileInfo.ftLastWriteTime, // pointer to UTC file time to convert
                &localFileTime 	              // pointer to converted file time
            ) == 0)
        throw std::runtime_error(_("Error converting FILETIME to local FILETIME"));


    if (FileTimeToSystemTime(
                &localFileTime, // pointer to file time to convert
                &time 	        // pointer to structure to receive system time
            ) == 0)
        throw std::runtime_error(_("Error converting FILETIME to SYSTEMTIME"));

    output.lastWriteTime = numberToWxString(time.wYear) + "-" +
                           formatTime(time.wMonth) + "-" +
                           formatTime(time.wDay) + "  " +
                           formatTime(time.wHour) + ":" +
                           formatTime(time.wMinute) + ":" +
                           formatTime(time.wSecond);

    //UTC time
    output.lastWriteTimeRaw = wxULongLong(winFileInfo.ftLastWriteTime.dwHighDateTime, winFileInfo.ftLastWriteTime.dwLowDateTime);

    //reduce precision to 1 second (FILETIME has unit 10^-7 s)
    output.lastWriteTimeRaw/= 10000000; // <- time is used for comparison only: unit switched to seconds

    output.fileSize = wxULongLong(winFileInfo.nFileSizeHigh, winFileInfo.nFileSizeLow);

#else
    struct stat fileInfo;
    if (stat(filename.c_str(), &fileInfo) != 0)
        throw FileError(wxString(_("Could not retrieve file info for: ")) + "\"" + filename + "\"");

    tm* timeinfo;
    timeinfo = localtime(&fileInfo.st_mtime);
    char buffer [50];
    strftime(buffer,50,"%Y-%m-%d  %H:%M:%S",timeinfo);

    //local time
    output.lastWriteTime = buffer;
    //unit: 1 second
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

    static const unsigned int bufferSize = 1024*256; //256 kb seems to be the perfect buffer size
    unsigned char* buffer1;
    unsigned char* buffer2;
};


bool filesHaveSameContent(const wxString& filename1, const wxString& filename2)
{
    static MemoryAllocator memory;

    FILE* file1 = fopen(filename1.c_str(), "rb");
    if (!file1)
        throw FileError(wxString(_("Could not open file: ")) + "\"" + filename1 + "\"");

    FILE* file2 = fopen(filename2.c_str(), "rb");
    if (!file2)
    {
        fclose(file1);  //NEVER forget clean-up
        throw FileError(wxString(_("Could not open file: ")) + "\"" + filename2 + "\"");
    }

    bool returnValue = true;
    do
    {
        unsigned int length1 = fread(memory.buffer1, 1, memory.bufferSize, file1);
        if (ferror(file1)) throw FileError(wxString(_("Error reading file: ")) + "\"" + filename1 + "\"");

        unsigned int length2 = fread(memory.buffer2, 1, memory.bufferSize, file2);
        if (ferror(file2)) throw FileError(wxString(_("Error reading file: ")) + "\"" + filename2 + "\"");

        if (length1 != length2 || memcmp(memory.buffer1, memory.buffer2, length1) != 0)
        {
            returnValue = false;
            break;
        }
    }
    while (!feof(file1));

    if (!feof(file2))
        returnValue = false;

    fclose(file1);
    fclose(file2);

    return returnValue;
}


void FreeFileSync::generateFileAndFolderDescriptions(DirectoryDescrType& output, const wxString& directory, StatusUpdater* updateClass)
{
    assert (updateClass);

    output.clear();

    //get all files and folders from directory (and subdirectories) + information
    GetAllFilesFull traverser(output, getFormattedDirectoryName(directory), updateClass);
    wxDir dir(directory);
    dir.Traverse(traverser);
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


void FreeFileSync::startCompareProcess(FileCompareResult& output, const wxString& dirLeft, const wxString& dirRight, CompareVariant cmpVar, StatusUpdater* statusUpdater)
{
    assert (statusUpdater);

//################################################################################################################################################

    //inform about the total amount of data that will be processed from now on
    statusUpdater->initNewProcess(-1, 0, FreeFileSync::scanningFilesProcess);    //it's not known how many files will be scanned => -1 objects

    FileCompareResult output_tmp;   //write to output not before END of process!

    try
    {
        //retrieve sets of files (with description data)
        DirectoryDescrType directoryLeft;
        DirectoryDescrType directoryRight;

        //unsigned int startTime = GetTickCount();
        generateFileAndFolderDescriptions(directoryLeft, dirLeft, statusUpdater);
        generateFileAndFolderDescriptions(directoryRight, dirRight, statusUpdater);
        //wxMessageBox(numberToWxString(unsigned(GetTickCount()) - startTime));

        FileCompareLine newline;

        set<int> delayedContentCompare; //compare of file content happens AFTER finding corresponding files
        //in order to separate into two processe (needed by progress indicators)

        //find files/folders that exist in left file model but not in right model
        for (DirectoryDescrType::iterator i = directoryLeft.begin(); i != directoryLeft.end(); ++i)
            if (directoryRight.find(*i) == directoryRight.end())
            {
                newline.fileDescrLeft            = *i;
                newline.fileDescrRight           = FileDescrLine();
                newline.fileDescrRight.directory = dirRight;
                newline.cmpResult                = fileOnLeftSideOnly;
                output_tmp.push_back(newline);
            }

        for (DirectoryDescrType::iterator j = directoryRight.begin(); j != directoryRight.end(); ++j)
        {
            DirectoryDescrType::iterator i;

            //find files/folders that exist in right file model but not in left model
            if ((i = directoryLeft.find(*j)) == directoryLeft.end())
            {
                newline.fileDescrLeft           = FileDescrLine();
                newline.fileDescrLeft.directory = dirLeft;
                newline.fileDescrRight          = *j;
                newline.cmpResult               = fileOnRightSideOnly;
                output_tmp.push_back(newline);
            }
            //find files that exist in left and right file model
            else
            {   //objType != isNothing
                if (i->objType == isDirectory && j->objType == isDirectory)
                {
                    newline.fileDescrLeft  = *i;
                    newline.fileDescrRight = *j;
                    newline.cmpResult      = filesEqual;
                    output_tmp.push_back(newline);
                }
                //if we have a nameclash between a file and a directory: split into separate rows
                else if (i->objType != j->objType)
                {
                    newline.fileDescrLeft            = *i;
                    newline.fileDescrRight           = FileDescrLine();
                    newline.fileDescrRight.directory = dirRight;
                    newline.cmpResult                = fileOnLeftSideOnly;
                    output_tmp.push_back(newline);

                    newline.fileDescrLeft           = FileDescrLine();
                    newline.fileDescrLeft.directory = dirLeft;
                    newline.fileDescrRight          = *j;
                    newline.cmpResult               = fileOnRightSideOnly;
                    output_tmp.push_back(newline);
                }
                else if (cmpVar == compareByTimeAndSize)
                {  //check files that exist in left and right model but have different properties
                    if (i->lastWriteTimeRaw != j->lastWriteTimeRaw ||
                            i->fileSize != j->fileSize)
                    {
                        newline.fileDescrLeft  = *i;
                        newline.fileDescrRight = *j;

                        if (i->lastWriteTimeRaw == j->lastWriteTimeRaw)
                            newline.cmpResult      = filesDifferent;
                        else if (i->lastWriteTimeRaw < j->lastWriteTimeRaw)
                            newline.cmpResult      = rightFileNewer;
                        else
                            newline.cmpResult      = leftFileNewer;
                        output_tmp.push_back(newline);
                    }
                    else
                    {
                        newline.fileDescrLeft  = *i;
                        newline.fileDescrRight = *j;
                        newline.cmpResult      = filesEqual;
                        output_tmp.push_back(newline);
                    }
                }
                else if (cmpVar == compareByContent)
                {   //check files that exist in left and right model but have different content

                    //check filesize first!
                    if (i->fileSize == j->fileSize)
                    {
                        newline.fileDescrLeft  = *i;
                        newline.fileDescrRight = *j;
                        //newline.cmpResult    = ...;   //not yet determined
                        output_tmp.push_back(newline);

                        //compare by content is only needed if filesizes are the same
                        delayedContentCompare.insert(output_tmp.size() - 1); //save index of row, to calculate cmpResult later
                    }
                    else
                    {
                        newline.fileDescrLeft  = *i;
                        newline.fileDescrRight = *j;
                        newline.cmpResult      = filesDifferent;
                        output_tmp.push_back(newline);
                    }
                }
                else assert (false);
            }
        }

//################################################################################################################################################

        //compare file contents and set value "cmpResult"

        //unsigned int startTime3 = GetTickCount();
        if (cmpVar == compareByContent)
        {
            int objectsTotal = 0;
            double dataTotal = 0;
            calcTotalDataForCompare(objectsTotal, dataTotal, output_tmp, delayedContentCompare);

            statusUpdater->initNewProcess(objectsTotal, dataTotal, FreeFileSync::compareFileContentProcess);

            set<int> rowsToDelete;  //if errors occur during file access and user skips, these rows need to be deleted from result

            for (set<int>::iterator i = delayedContentCompare.begin(); i != delayedContentCompare.end(); ++i)
            {
                FileCompareLine& gridline = output_tmp[*i];

                statusUpdater->updateStatusText(wxString(_("Comparing content of files ")) + "\"" + gridline.fileDescrLeft.relFilename + "\"");

                //check files that exist in left and right model but have different content
                while (true)
                {
                    //trigger display refresh
                    statusUpdater->triggerUI_Refresh();

                    try
                    {
                        if (filesHaveSameContentMultithreaded(gridline.fileDescrLeft.filename, gridline.fileDescrRight.filename, statusUpdater))
                            gridline.cmpResult = filesEqual;
                        else
                            gridline.cmpResult = filesDifferent;

                        statusUpdater->updateProcessedData(2, (gridline.fileDescrLeft.fileSize + gridline.fileDescrRight.fileSize).ToDouble());
                        break;
                    }
                    catch (FileError& error)
                    {
                        //if (updateClass) -> is mandatory
                        int rv = statusUpdater->reportError(error.show());
                        if ( rv == StatusUpdater::continueNext)
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

        statusUpdater->triggerUI_Refresh();
    }
    catch (std::runtime_error& theException)
    {
        wxMessageBox(_(theException.what()), _("An exception occured!"), wxOK | wxICON_ERROR);
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
        if (i->cmpResult == fileOnLeftSideOnly)
            i->cmpResult = fileOnRightSideOnly;
        else if (i->cmpResult == fileOnRightSideOnly)
            i->cmpResult = fileOnLeftSideOnly;
        else if (i->cmpResult == rightFileNewer)
            i->cmpResult = leftFileNewer;
        else if (i->cmpResult == leftFileNewer)
            i->cmpResult = rightFileNewer;

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
    fileDescr.objType          = isFile;
    m_output.insert(fileDescr);

    //update UI/commandline status information
    statusUpdater->updateStatusText(wxString(_("Scanning ")) + "\"" + filename + "\""); // NO performance issue at all
    //add 1 element to the progress indicator
    statusUpdater->updateProcessedData(1, 0);     // NO performance issue at all
    //trigger display refresh
    statusUpdater->triggerUI_Refresh();

    return wxDIR_CONTINUE;
}


wxDirTraverseResult GetAllFilesFull::OnDir(const wxString& dirname)
{
    if (dirname.EndsWith("\\RECYCLER") ||
            dirname.EndsWith("\\System Volume Information"))
        return wxDIR_IGNORE;

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
    fileDescr.objType          = isDirectory;
    m_output.insert(fileDescr);

    //update UI/commandline status information
    statusUpdater->updateStatusText(wxString(_("Scanning ")) + "\"" + dirname + "\""); // NO performance issue at all
    //add 1 element to the progress indicator
    statusUpdater->updateProcessedData(1, 0);     // NO performance issue at all
    //trigger display refresh
    statusUpdater->triggerUI_Refresh();

    return wxDIR_CONTINUE;
}


wxDirTraverseResult GetAllFilesFull::OnOpenError(const wxString& openerrorname)
{
    wxMessageBox(openerrorname, _("Error opening file"));
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
        wxMessageBox(openerrorname, _("Error opening file"));
        return wxDIR_IGNORE;
    }

private:
    wxArrayString& allFiles;
    wxArrayString& subdirs;
};


#ifdef FFS_WIN
typedef WINSHELLAPI int (*DllFileOP)(LPSHFILEOPSTRUCT lpFileOp);
#endif  // FFS_WIN


class RecycleBin
{
public:
    RecycleBin() :
            recycleBinAvailable(false)
    {
#ifdef FFS_WIN
        // Get a handle to the DLL module containing Recycle Bin functionality
        hinstShell = LoadLibrary("shell32.dll");
        if (hinstShell != NULL)
        {
            fileOperation  = (DllFileOP)GetProcAddress(hinstShell, "SHFileOperation");
            if (fileOperation == NULL )
            {
                FreeLibrary(hinstShell);
                recycleBinAvailable = false;
            }
            else
                recycleBinAvailable = true;
        }
#endif  // FFS_WIN
    }

    ~RecycleBin()
    {
#ifdef FFS_WIN
        if (recycleBinAvailable)
            FreeLibrary(hinstShell);
#endif  // FFS_WIN
    }

    bool recycleBinExists()
    {
        return recycleBinAvailable;
    }

    void moveToRecycleBin(const wxString& filename)
    {
        if (!recycleBinAvailable)   //this method should ONLY be called if recycle bin is available
            throw std::runtime_error("Initialization of Recycle Bin failed! It cannot be used!");

#ifdef FFS_WIN
        SHFILEOPSTRUCT fileOp;

        wxString filenameDoubleNull = filename + char(0);

        fileOp.hwnd   = NULL;
        fileOp.wFunc  = FO_DELETE;
        fileOp.pFrom  = filenameDoubleNull.c_str();
        fileOp.pTo    = NULL;
        fileOp.fFlags = FOF_ALLOWUNDO | FOF_NOCONFIRMATION | FOF_SILENT;
        fileOp.fAnyOperationsAborted = false;
        fileOp.hNameMappings         = NULL;
        fileOp.lpszProgressTitle     = NULL;

        if (fileOperation(&fileOp   //pointer to an SHFILEOPSTRUCT structure that contains information the function needs to carry out
                         ) != 0 || fileOp.fAnyOperationsAborted) throw FileError(wxString(_("Error moving file ")) + "\"" + filename + "\"" + _(" to recycle bin!"));
#endif  // FFS_WIN
    }

private:
    bool recycleBinAvailable;

#ifdef FFS_WIN
    HINSTANCE hinstShell;
    DllFileOP fileOperation;
#endif  // FFS_WIN
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
                filename.c_str(),     // address of filename
                FILE_ATTRIBUTE_NORMAL // attributes to set
            )) throw FileError(wxString(_("Error deleting file ")) + "\"" + filename + "\"");
#endif  // FFS_WIN

    if (!wxRemoveFile(filename))
        throw FileError(wxString(_("Error deleting file ")) + "\"" + filename + "\"");
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
        dir.Traverse(traverser);
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
                )) throw FileError(wxString(_("Error deleting directory ")) + "\"" + dirList[j] + "\"");
#endif  // FFS_WIN

        if (!wxRmdir(dirList[j]))
            throw FileError(wxString(_("Error deleting directory ")) + "\"" + dirList[j] + "\"");
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
            throw FileError(wxString(_("Error creating directory ")) + "\"" + directory + "\"");
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
    UpdateWhileCopying()
    {   //prevent wxWidgets logging
        noWxLogs = new wxLogNull;
    }
    ~UpdateWhileCopying()
    {
        delete noWxLogs;
    }
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
            errorMessage = wxString(_("Error copying file ")) + "\"" + source + "\"" + _(" to ") + "\"" + target + "\"";
            return;
        }

#ifdef FFS_LINUX //copying files with Linux does not preserve the modification time => adapt after copying
        struct stat fileInfo;
        if (stat(source.c_str(), &fileInfo) != 0) //read modification time from source file
        {
            success = false;
            errorMessage = wxString(_("Could not retrieve file info for: ")) + "\"" + source + "\"";
            return;
        }

        utimbuf newTimes;
        newTimes.actime  = fileInfo.st_mtime;
        newTimes.modtime = fileInfo.st_mtime;

        if (utime(target.c_str(), &newTimes) != 0)
        {
            success = false;
            errorMessage = wxString(_("Error adapting modification time of file ")) + "\"" + target + "\"";
            return;
        }
#endif  // FFS_LINUX

        success = true;
    }

    wxLogNull* noWxLogs;
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
        recycleBinShouldBeUsed(useRecycleBin)
{
    //prevent wxWidgets logging
    noWxLogs = new wxLogNull;
}


FreeFileSync::~FreeFileSync()
{
    delete noWxLogs;
}


bool FreeFileSync::recycleBinExists()
{
    return recycler.recycleBinExists();
}


inline
SyncDirection getSyncDirection(const CompareFilesResult cmpResult, const SyncConfiguration& config)
{
    switch (cmpResult)
    {
    case fileOnLeftSideOnly:
        return config.exLeftSideOnly;
        break;

    case fileOnRightSideOnly:
        return config.exRightSideOnly;
        break;

    case rightFileNewer:
        return config.rightNewer;
        break;

    case leftFileNewer:
        return config.leftNewer;
        break;

    case filesDifferent:
        return config.different;
        break;

    default:
        assert (false);
    }
    return syncDirNone;
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
    case fileOnLeftSideOnly:
        //get data to process
        switch (getSyncDirection(fileCmpLine.cmpResult, config))
        {
        case syncDirLeft:   //delete file on left
            dataToProcess   = 0;
            objectsToDelete = 1;
            break;
        case syncDirRight:  //copy from left to right
            dataToProcess = fileCmpLine.fileDescrLeft.fileSize.ToDouble();
            objectsToCreate = 1;
            break;
        case syncDirNone:
            return false;
        }
        break;

    case fileOnRightSideOnly:
        switch (getSyncDirection(fileCmpLine.cmpResult, config))
        {
        case syncDirLeft:   //copy from right to left
            dataToProcess   = fileCmpLine.fileDescrRight.fileSize.ToDouble();;
            objectsToCreate = 1;
            break;
        case syncDirRight:  //delete file on right
            dataToProcess   = 0;
            objectsToDelete = 1;
            break;
        case syncDirNone:
            return false;
        }
        break;

    case leftFileNewer:
    case rightFileNewer:
    case filesDifferent:
        //get data to process
        switch (getSyncDirection(fileCmpLine.cmpResult, config))
        {
        case syncDirLeft:   //copy from right to left
            dataToProcess = fileCmpLine.fileDescrRight.fileSize.ToDouble();
            objectsToOverwrite = 1;
            break;
        case syncDirRight:  //copy from left to right
            dataToProcess = fileCmpLine.fileDescrLeft.fileSize.ToDouble();
            objectsToOverwrite = 1;
            break;
        case syncDirNone:
            return false;
        }
        break;

    case filesEqual:
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
    case fileOnLeftSideOnly:
        switch (config.exLeftSideOnly)
        {
        case syncDirLeft:   //delete files on left
            statusUpdater->updateStatusText(wxString(_("Deleting file ")) + "\"" + filename.fileDescrLeft.filename + "\"");
            removeFile(filename.fileDescrLeft.filename);
            break;
        case syncDirRight:  //copy files to right
            target = filename.fileDescrRight.directory + filename.fileDescrLeft.relFilename;
            statusUpdater->updateStatusText(wxString(_("Copying file ")) + "\"" + filename.fileDescrLeft.filename + "\"" +
                                            _(" to ") + "\"" + target + "\"");

            copyfileMultithreaded(filename.fileDescrLeft.filename, target, statusUpdater);
            break;
        case syncDirNone:
            return false;
        default:
            assert (false);
        }
        break;

    case fileOnRightSideOnly:
        switch (config.exRightSideOnly)
        {
        case syncDirLeft:   //copy files to left
            target = filename.fileDescrLeft.directory + filename.fileDescrRight.relFilename;
            statusUpdater->updateStatusText(wxString(_("Copying file ")) + "\"" + filename.fileDescrRight.filename + "\"" +
                                            _(" to ") + "\"" + target + "\"");

            copyfileMultithreaded(filename.fileDescrRight.filename, target, statusUpdater);
            break;
        case syncDirRight:  //delete files on right
            statusUpdater->updateStatusText(wxString(_("Deleting file ")) + "\"" + filename.fileDescrRight.filename + "\"");
            removeFile(filename.fileDescrRight.filename);
            break;
        case syncDirNone:
            return false;
        default:
            assert (false);
        }
        break;

    case leftFileNewer:
    case rightFileNewer:
    case filesDifferent:
        switch (getSyncDirection(filename.cmpResult, config))
        {
        case syncDirLeft:   //copy from right to left
            statusUpdater->updateStatusText(wxString(_("Copying file ")) + "\"" + filename.fileDescrRight.filename + "\"" +
                                            _(" overwriting ") + "\"" + filename.fileDescrLeft.filename + "\"");

            removeFile(filename.fileDescrLeft.filename);  //only used if switch activated by user, else file is simply deleted
            copyfileMultithreaded(filename.fileDescrRight.filename, filename.fileDescrLeft.filename, statusUpdater);
            break;
        case syncDirRight:  //copy from left to right
            statusUpdater->updateStatusText(wxString(_("Copying file ")) + "\"" + filename.fileDescrLeft.filename + "\"" +
                                            _(" overwriting ") + "\"" + filename.fileDescrRight.filename + "\"");

            removeFile(filename.fileDescrRight.filename);  //only used if switch activated by user, else file is simply deleted
            copyfileMultithreaded(filename.fileDescrLeft.filename, filename.fileDescrRight.filename, statusUpdater);
            break;
        case syncDirNone:
            return false;
        default:
            assert (false);
        }
        break;

    case filesEqual:
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
    case fileOnLeftSideOnly:
        switch (config.exLeftSideOnly)
        {
        case syncDirLeft:   //delete folders on left
            statusUpdater->updateStatusText(wxString(_("Deleting folder ")) + "\"" + filename.fileDescrLeft.filename + "\"");
            removeDirectory(filename.fileDescrLeft.filename);
            break;
        case syncDirRight:  //create folders on right
            target = filename.fileDescrRight.directory + filename.fileDescrLeft.relFilename;
            statusUpdater->updateStatusText(wxString(_("Creating folder ")) + "\"" + target + "\"");

            //some check to catch the error that directory on source has been deleted externally after the "compare"...
            if (!wxDirExists(filename.fileDescrLeft.filename))
                throw FileError(wxString(_("Error: Source directory does not exist anymore: ")) + "\"" + filename.fileDescrLeft.filename + "\"");
            createDirectory(target);
            break;
        case syncDirNone:
            return false;
        default:
            assert (false);
        }
        break;

    case fileOnRightSideOnly:
        switch (config.exRightSideOnly)
        {
        case syncDirLeft:   //create folders on left
            target = filename.fileDescrLeft.directory + filename.fileDescrRight.relFilename;
            statusUpdater->updateStatusText(wxString(_("Creating folder ")) + "\"" + target + "\"");

            //some check to catch the error that directory on source has been deleted externally after the "compare"...
            if (!wxDirExists(filename.fileDescrRight.filename))
                throw FileError(wxString(_("Error: Source directory does not exist anymore: ")) + "\"" + filename.fileDescrRight.filename + "\"");
            createDirectory(target);
            break;
        case syncDirRight:  //delete folders on right
            statusUpdater->updateStatusText(wxString(_("Deleting folder ")) + "\"" + filename.fileDescrRight.filename + "\"");
            removeDirectory(filename.fileDescrRight.filename);
            break;
        case syncDirNone:
            return false;
        default:
            assert (false);
        }
        break;

    case filesEqual:
        return false;
    case rightFileNewer:
    case leftFileNewer:
    case filesDifferent:
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

    wxString unit = " Byte";
    if (nrOfBytes > 999)
    {
        nrOfBytes/= 1024;
        unit = " kB";
        if (nrOfBytes > 999)
        {
            nrOfBytes/= 1024;
            unit = " MB";
            if (nrOfBytes > 999)
            {
                nrOfBytes/= 1024;
                unit = " GB";
                if (nrOfBytes > 999)
                {
                    nrOfBytes/= 1024;
                    unit = " TB";
                    if (nrOfBytes > 999)
                    {
                        nrOfBytes/= 1024;
                        unit = " PB";
                    }
                }
            }
        }
    }

    wxString temp;
    if (unit == " Byte") //no decimal places in case of bytes
    {
        double integer = 0;
        modf(nrOfBytes, &integer); //get integer part of nrOfBytes
        temp = numberToWxString(int(integer));
        //    if (!temp.Len()) //if nrOfBytes is zero GMP returns an empty string
        //      temp = "0";
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
            temp = "Error";
            break;
        case 1:
            temp = wxString("0") + GlobalResources::decimalPoint + "0" + temp;
            break; //0,01
        case 2:
            temp = wxString("0") + GlobalResources::decimalPoint + temp;
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
            return "Error";
        }
    }
    return (temp + unit);
}


void FreeFileSync::filterCurrentGridData(FileCompareResult& currentGridData, const wxString& includeFilter, const wxString& excludeFilter)
{
    wxString includeFilterTmp(includeFilter);
    wxString excludeFilterTmp(excludeFilter);

    //make sure filters end with ";" - no problem with empty strings here
    if (!includeFilterTmp.EndsWith(";"))
        includeFilterTmp.Append(';');
    if (!excludeFilterTmp.EndsWith(";"))
        excludeFilterTmp.Append(';');

    //load filters into vectors
    vector<wxString> includeList;
    vector<wxString> excludeList;

    unsigned int indexStart = 0;
    unsigned int indexEnd = 0;
    while ((indexEnd = includeFilterTmp.find(';', indexStart )) != string::npos)
    {
        if (indexStart != indexEnd) //do not add empty strings
            includeList.push_back( includeFilterTmp.substr(indexStart, indexEnd - indexStart) );
        indexStart = indexEnd + 1;
    }

    indexStart = 0;
    indexEnd = 0;
    while ((indexEnd = excludeFilterTmp.find(';', indexStart )) != string::npos)
    {
        if (indexStart != indexEnd) //do not add empty strings
            excludeList.push_back( excludeFilterTmp.substr(indexStart, indexEnd - indexStart) );
        indexStart = indexEnd + 1;
    }

//###########################

    //filter currentGridData
    for (FileCompareResult::iterator i = currentGridData.begin(); i != currentGridData.end(); ++i)
    {
        //process include filters
        if (i->fileDescrLeft.objType != isNothing)
        {
            bool includedLeft = false;
            for (vector<wxString>::const_iterator j = includeList.begin(); j != includeList.end(); ++j)
                if (i->fileDescrLeft.filename.Matches(*j))
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

        if (i->fileDescrRight.objType != isNothing)
        {
            bool includedRight = false;
            for (vector<wxString>::const_iterator j = includeList.begin(); j != includeList.end(); ++j)
                if (i->fileDescrRight.filename.Matches(*j))
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
            if (i->fileDescrLeft.filename.Matches(*j) ||
                    i->fileDescrRight.filename.Matches(*j))
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
{  //this formatting is needed since functions in FreeFileSync.cpp expect the directory to end with '\' to be able to split the relative names
    //Actually all it needs, is the length of the directory

    //let wxWidgets do the directory formatting, e.g. replace '/' with '\' for Windows
    wxString result = wxDir(dirname).GetName();

    result.Append(GlobalResources::fileNameSeparator);
    return result;
}


inline
bool deletionImminent(const FileCompareLine& line, const SyncConfiguration& config)
{   //test if current sync-line will result in deletion of files    -> used to avoid disc space bottlenecks
    if ((line.cmpResult == fileOnLeftSideOnly && config.exLeftSideOnly == syncDirLeft) ||
            (line.cmpResult == fileOnRightSideOnly && config.exRightSideOnly == syncDirRight))
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
            if (i->fileDescrLeft.objType == isDirectory || i->fileDescrRight.objType == isDirectory)
            {
                while (true)
                {   //trigger display refresh
                    statusUpdater->triggerUI_Refresh();

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
        for (int k = 0; k < 2; ++k) //loop over all files twice; reason: first delete, then copy (no sorting necessary: performance)
        {
            bool deleteLoop = !k;   //-> first loop: delete files, second loop: copy files

            for (FileCompareResult::const_iterator i = grid.begin(); i != grid.end(); ++i)
            {
                if (i->fileDescrLeft.objType == isFile || i->fileDescrRight.objType == isFile)
                {
                    if (deleteLoop && deletionImminent(*i, config) ||
                            !deleteLoop && !deletionImminent(*i, config))
                    {
                        while (true)
                        {    //trigger display refresh
                            statusUpdater->triggerUI_Refresh();

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
    catch (std::runtime_error& theException)
    {
        wxMessageBox(_(theException.what()), _("An exception occured!"), wxOK | wxICON_ERROR);
        return;
    }
}


bool FreeFileSync::isFFS_ConfigFile(const wxString& filename)
{
    ifstream config(filename.c_str());
    if (!config)
        return false;

    char bigBuffer[10000];

    //read FFS identifier
    config.get(bigBuffer, FreeFileSync::FFS_ConfigFileID.Len() + 1);

    return (FFS_ConfigFileID == wxString(bigBuffer));
}


//add(!) all files and subfolder gridlines that are dependent from the directory
void FreeFileSync::addSubElements(set<int>& subElements, const FileCompareResult& grid, const FileCompareLine& relevantRow)
{
    wxString relevantDirectory;

    if (relevantRow.fileDescrLeft.objType == isDirectory)
        relevantDirectory = relevantRow.fileDescrLeft.relFilename;

    else if (relevantRow.fileDescrRight.objType == isDirectory)
        relevantDirectory = relevantRow.fileDescrRight.relFilename;

    else
        return;

    for (FileCompareResult::const_iterator i = grid.begin(); i != grid.end(); ++i)
        if (i->fileDescrLeft.relFilename.StartsWith(relevantDirectory) ||
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
                if (currentCmpLine.fileDescrLeft.objType == isFile)
                    fileSyncObject.removeFile(currentCmpLine.fileDescrLeft.filename);
                else if (currentCmpLine.fileDescrLeft.objType == isDirectory)
                    fileSyncObject.removeDirectory(currentCmpLine.fileDescrLeft.filename);

                if (currentCmpLine.fileDescrRight.objType == isFile)
                    fileSyncObject.removeFile(currentCmpLine.fileDescrRight.filename);
                else if (currentCmpLine.fileDescrRight.objType == isDirectory)
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
                    ;   //continue with loop
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


void updateUI_Now()
{
    //process UI events and prevent application from "not responding"   -> NO performance issue!
    wxTheApp->Yield();

    //    while (wxTheApp->Pending())
    //        wxTheApp->Dispatch();
}


bool updateUI_IsAllowed()
{
    static wxLongLong lastExec = 0;

    wxLongLong newExec = wxGetLocalTimeMillis();

    if (newExec - lastExec >= uiUpdateInterval)  //perform ui updates not more often than necessary
    {
        lastExec = newExec;
        return true;
    }
    return false;
}
