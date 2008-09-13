#include <wx/arrstr.h>
#include <wx/dir.h>
#include <wx/msgdlg.h>
#include "FreeFileSync.h"
#include "library\md5.h"
#include <stdexcept> //for std::runtime_error
#include "library/globalFunctions.h"
#include "library/gmp/include\gmp.h"
#include <wx/filename.h>
#include <fstream>
#include "ui\resources.h"

using namespace globalFunctions;

const wxString FreeFileSync::FFS_ConfigFileID = "FFS_CONFIG";
const wxString FreeFileSync::FFS_LastConfigFile = "LastRun.FFS";

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


bool filetimeCmpEqual(const FILETIME a,const FILETIME b)
{
    if (a.dwHighDateTime == b.dwHighDateTime && a.dwLowDateTime == b.dwLowDateTime)
        return (true);
    else
        return (false);
}

void FreeFileSync::getFileInformation(FileInfo& output, const wxString& filename)
{
//
//    wxFileName file(filename);
//
//    wxDateTime lastWriteTime;
//    wxULongLong filesize;
//
//
//
//    if (file.FileExists())
//    {
//        if (!file.GetTimes(NULL, &lastWriteTime, NULL) || ((filesize = file.GetSize()) == wxInvalidSize))
//            throw FileError(wxString(_("Could not retrieve file info for: ")) + "\"" + filename + "\"");
//    output.lastWriteTime = lastWriteTime.FormatISODate();
//
//    output.lastWriteTimeUTC.dwHighDateTime = 0;
//    output.lastWriteTimeUTC.dwLowDateTime  = 0;
//    }
//
//    output.fileSize = filesize.ToString();
//


    WIN32_FIND_DATA winFileInfo;
    FILETIME localFileTime;
    SYSTEMTIME time;
    HANDLE fileHandle;

    if ((fileHandle = FindFirstFile(filename.c_str(), &winFileInfo)) == INVALID_HANDLE_VALUE)
        throw FileError(wxString(_("Could not retrieve file info for: ")) + "\"" + filename + "\"");

    FindClose(fileHandle);

    if (FileTimeToLocalFileTime(
                &winFileInfo.ftLastWriteTime,	// pointer to UTC file time to convert
                &localFileTime 	// pointer to converted file time
            ) == 0)
        throw std::runtime_error(_("Error converting FILETIME to local FILETIME"));


    if (FileTimeToSystemTime(
                &localFileTime, // pointer to file time to convert
                &time 	 // pointer to structure to receive system time
            ) == 0)
        throw std::runtime_error(_("Error converting FILETIME to SYSTEMTIME"));

    output.lastWriteTime = numberToWxString(time.wYear) + "-" +
                           formatTime(time.wMonth) + "-" +
                           formatTime(time.wDay) + "  " +
                           formatTime(time.wHour) + ":" +
                           formatTime(time.wMinute) + ":" +
                           formatTime(time.wSecond);

    //UTC times
    output.lastWriteTimeUTC = wxULongLong(winFileInfo.ftLastWriteTime.dwHighDateTime, winFileInfo.ftLastWriteTime.dwLowDateTime);

    output.fileSize = wxULongLong(winFileInfo.nFileSizeHigh, winFileInfo.nFileSizeLow);
}


string FreeFileSync::calculateMD5Hash(const wxString& filename)
{
    const unsigned int bufferSize = 8192;

    char md5_output[33];
    unsigned char signature[16];
    unsigned char buffer[bufferSize];
    md5_t state;

    md5_init(&state);
    FILE* inputFile = fopen( filename.c_str(), "rb");
    if (!inputFile)
        throw FileError(wxString(_("Could not open file: ")) + "\"" + filename + "\"");
    do
    {
        unsigned int length = fread(buffer, 1, bufferSize, inputFile);
        if (!ferror(inputFile)) md5_process(&state, buffer, length);
    }
    while (!feof(inputFile));

    fclose(inputFile);

    md5_finish(&state, signature);

    for (int i=0;i<16;i++) sprintf(md5_output + i*2, "%02x", signature[i]);

    return md5_output;
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
class UpdateWhileCalculatingMD5 : public UpdateWhileExecuting
{
public:
    UpdateWhileCalculatingMD5() {}
    ~UpdateWhileCalculatingMD5() {}

    wxString file;
    bool success;
    wxString errorMessage;
    string result;

private:
    void longRunner() //virtual method implementation
    {
        try
        {
            result = FreeFileSync::calculateMD5Hash(file);
            success = true;
        }
        catch (FileError& error)
        {
            success = false;
            errorMessage = error.show();
        }
    }
};


string FreeFileSync::calculateMD5HashMultithreaded(const wxString& filename, StatusUpdater* updateClass)
{
    static UpdateWhileCalculatingMD5 calcAndUpdate;    //single instantiation: after each execution thread enters wait phase

    calcAndUpdate.waitUntilReady();

    //longRunner is called from thread, but no mutex needed here, since thread is in waiting state!
    calcAndUpdate.file = filename;

    calcAndUpdate.execAndUpdate(updateClass);

    //no mutex needed here since longRunner is finished
    if (!calcAndUpdate.success)
        throw FileError(calcAndUpdate.errorMessage);

    return calcAndUpdate.result;
}


void calcTotalDataForMD5(int& objectsTotal, double& dataTotal, const FileCompareResult& grid, const set<int>& rowsForMD5)
{
    dataTotal = 0;

    for (set<int>::iterator i = rowsForMD5.begin(); i != rowsForMD5.end(); ++i)
    {
        const FileCompareLine& gridline = grid[*i];

        dataTotal+= gridline.fileDescrLeft.fileSize.ToDouble();
        dataTotal+= gridline.fileDescrRight.fileSize.ToDouble();
    }

    objectsTotal = rowsForMD5.size() * 2;
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
        generateFileAndFolderDescriptions(directoryLeft, dirLeft, statusUpdater);
        generateFileAndFolderDescriptions(directoryRight, dirRight, statusUpdater);

        FileCompareLine newline;

        set<int> delayedMD5calculation; //md5 calculation happens AFTER compare in order to separate into two processe (needed by progress indicators)

        //find files/folders that exist in left file model but not in right model
        for (DirectoryDescrType::iterator i = directoryLeft.begin(); i != directoryLeft.end(); i++)
            if (directoryRight.find(*i) == directoryRight.end())
            {
                newline.fileDescrLeft            = *i;
                newline.fileDescrRight           = FileDescrLine();
                newline.fileDescrRight.directory = dirRight;
                newline.cmpResult                = fileOnLeftSideOnly;
                output_tmp.push_back(newline);
            }

        for (DirectoryDescrType::iterator j = directoryRight.begin(); j != directoryRight.end(); j++)
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
                    if (i->lastWriteTimeUTC != j->lastWriteTimeUTC ||
                            i->fileSize != j->fileSize)
                    {
                        newline.fileDescrLeft  = *i;
                        newline.fileDescrRight = *j;

                        if (i->lastWriteTimeUTC == j->lastWriteTimeUTC)
                            newline.cmpResult      = filesDifferent;
                        else if (i->lastWriteTimeUTC < j->lastWriteTimeUTC)
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
                else if (cmpVar == compareByMD5)
                {   //check files that exist in left and right model but have different content

                    //check filesize first!
                    if (i->fileSize == j->fileSize)
                    {
                        newline.fileDescrLeft  = *i;
                        newline.fileDescrRight = *j;
                        //newline.cmpResult    = ...;   //not yet determined
                        output_tmp.push_back(newline);

                        //md5 needed only if filesizes are the same
                        delayedMD5calculation.insert(output_tmp.size() - 1); //save index of row, to calculate cmpResult later
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
        //calculate MD5 checksums and set value "cmpResult"
        if (cmpVar == compareByMD5)
        {
            int objectsTotal = 0;
            double dataTotal = 0;
            calcTotalDataForMD5(objectsTotal, dataTotal, output_tmp, delayedMD5calculation);

            statusUpdater->initNewProcess(objectsTotal, dataTotal, FreeFileSync::calcMD5Process);

            set<int> rowsToDelete;  //if errors occur during file access and user skips, these rows need to be deleted from result

            for (set<int>::iterator i = delayedMD5calculation.begin(); i != delayedMD5calculation.end(); ++i)
            {
                FileCompareLine& gridline = output_tmp[*i];

                //check files that exist in left and right model but have different checksums
                string leftFileHash;
                string rightFileHash;

                while (true)
                {
                    //trigger display refresh
                    statusUpdater->triggerUI_Refresh();

                    try
                    {
                        if (leftFileHash.empty())
                        {
                            statusUpdater->updateStatusText(wxString(_("Reading content of ") + gridline.fileDescrLeft.filename));
                            leftFileHash = calculateMD5HashMultithreaded(gridline.fileDescrLeft.filename, statusUpdater);
                            statusUpdater->updateProcessedData(1, gridline.fileDescrLeft.fileSize.ToDouble());
                        }

                        if (rightFileHash.empty())
                        {
                            statusUpdater->updateStatusText(wxString(_("Reading content of ") + gridline.fileDescrRight.filename));
                            rightFileHash = calculateMD5HashMultithreaded(gridline.fileDescrRight.filename, statusUpdater);
                            statusUpdater->updateProcessedData(1, gridline.fileDescrRight.fileSize.ToDouble());
                        }

                        if (leftFileHash == rightFileHash)
                            gridline.cmpResult = filesEqual;
                        else
                            gridline.cmpResult = filesDifferent;

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
    fileDescr.lastWriteTimeUTC = currentFileInfo.lastWriteTimeUTC;
    fileDescr.fileSize         = currentFileInfo.fileSize;
    fileDescr.objType          = isFile;
    m_output.insert(fileDescr);

    //update UI/commandline status information
    statusUpdater->updateStatusText(wxString(_("Scanning ")) + filename); // NO performance issue at all
    //add 1 element to the progress indicator
    statusUpdater->updateProcessedData(1, 0);     // NO performance issue at all
    //trigger display refresh
    statusUpdater->triggerUI_Refresh();

    return wxDIR_CONTINUE;
}


wxDirTraverseResult GetAllFilesFull::OnDir(const wxString& dirname)
{
    if (dirname.EndsWith("\\RECYCLER"))
        return wxDIR_IGNORE;
    if (dirname.EndsWith("\\System Volume Information"))
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
    fileDescr.lastWriteTimeUTC = currentFileInfo.lastWriteTimeUTC;
    fileDescr.fileSize         = wxULongLong(0);     //currentFileInfo.fileSize should be "0" as well, but just to be sure... currently used by getBytesToTransfer
    fileDescr.objType          = isDirectory;
    m_output.insert(fileDescr);

    //update UI/commandline status information
    statusUpdater->updateStatusText(wxString(_("Scanning ")) + dirname); // NO performance issue at all
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


void FreeFileSync::moveToRecycleBin(const wxString& filename)
{
    bool aborted = false;
    SHFILEOPSTRUCT fileOp;

    wxString filenameDoubleNull = filename + char(0);

    fileOp.hwnd   = NULL;
    fileOp.wFunc  = FO_DELETE;
    fileOp.pFrom  = filenameDoubleNull.c_str();
    fileOp.pTo    = NULL;
    fileOp.fFlags = FOF_ALLOWUNDO | FOF_NOCONFIRMATION | FOF_SILENT;
    fileOp.fAnyOperationsAborted = aborted;
    fileOp.hNameMappings         = NULL;
    fileOp.lpszProgressTitle     = NULL;

    if (fileOperation(&fileOp   //Pointer to an SHFILEOPSTRUCT structure that contains information the function needs to carry out.
                     ) != 0 || aborted) throw FileError(wxString(_("Error moving file ")) + "\"" + filename + "\"" + _(" to recycle bin!"));
}


inline
void FreeFileSync::removeFile(const wxString& filename)
{
    if (!wxFileExists(filename)) return; //this is NOT an error situation: the manual deletion relies on it!

    if (recycleBinAvailable)
    {
        moveToRecycleBin(filename);
        return;
    }

#ifdef FFS_WIN
    if (!SetFileAttributes(
                filename.c_str(), // address of filename
                FILE_ATTRIBUTE_NORMAL	 	// address of attributes to set
            )) throw FileError(wxString(_("Error deleting file ")) + "\"" + filename + "\"");
#endif  // FFS_WIN

    if (!wxRemoveFile(filename))
        throw FileError(wxString(_("Error deleting file ")) + "\"" + filename + "\"");
}


void FreeFileSync::removeDirectory(const wxString& directory)
{
    if (!wxDirExists(directory)) return; //this is NOT an error situation: the manual deletion relies on it!

    if (recycleBinAvailable)
    {
        moveToRecycleBin(directory);
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

        if (!wxRmdir(dirList[j]))
            throw FileError(wxString(_("Error deleting directory ")) + "\"" + dirList[j] + "\"");
}


inline
void FreeFileSync::copyfile(const wxString& source, const wxString& target)
{
    if (!wxCopyFile(source, target, false)) //return false if file exists
        throw FileError(wxString(_("Error copying file ")) + "\"" + source + "\"" + _(" to ") + "\"" + target + "\"");
}


inline
void FreeFileSync::copyOverwriting(const wxString& source, const wxString& target)
{
    if (!SetFileAttributes(
                target.c_str(), // address of filename
                FILE_ATTRIBUTE_NORMAL	 	// address of attributes to set
            )) throw FileError(wxString(_("Error overwriting file ")) + "\"" + target + "\"");

    if (!wxCopyFile(source, target, true)) //if file exists it will be overwritten
        throw FileError(wxString(_("Error copying file ")) + "\"" + source + "\"" + _(" to ") + "\"" + target + "\"");
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


void FreeFileSync::copyCreatingDirs(const wxString& source, const wxString& target)
{
    wxString targetPath = wxFileName(target).GetPath();
    createDirectory(targetPath);

    if (!wxCopyFile(source, target, false)) //error if file exists
        throw FileError(wxString(_("Error copying file ")) + "\"" + source + "\"" + _(" to ") + "\"" + target + "\"");
}

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

private:
    void longRunner() //virtual method implementation
    {
        success = wxCopyFile(source, target, false); //abort if file exists
    }

    wxLogNull* noWxLogs;
};


void FreeFileSync::copyfileMultithreaded(const wxString& source, const wxString& target, StatusUpdater* updateClass)
{
    static UpdateWhileCopying copyAndUpdate;    //single instantiation: after each execution thread enters wait phase

    copyAndUpdate.waitUntilReady();

    //longRunner is called from thread, but no mutex needed here, since thread is in waiting state!
    copyAndUpdate.source = source;
    copyAndUpdate.target = target;

    copyAndUpdate.execAndUpdate(updateClass);

    //no mutex needed here since longRunner is finished
    if (!copyAndUpdate.success)
        throw FileError(wxString(_("Error copying file ")) + "\"" + source + "\"" + _(" to ") + "\"" + target + "\"");
}


FreeFileSync::FreeFileSync()
        : recycleBinAvailable(false)
{
    // Get a handle to the DLL module containing Recycle Bin functionality
    hinstShell = LoadLibrary("shell32.dll");
    if (hinstShell == NULL)
        recycleBinAvailable = false;
    else
    {
        fileOperation  = (DLLFUNC)GetProcAddress(hinstShell, "SHFileOperation");
        if (fileOperation == NULL )
        {
            FreeLibrary(hinstShell);
            recycleBinAvailable = false;
        }
        else
            recycleBinAvailable = true;
    }

    //prevent wxWidgets logging
    noWxLogs = new wxLogNull;
}


FreeFileSync::~FreeFileSync()
{
    if (recycleBinAvailable)
        FreeLibrary(hinstShell);

    delete noWxLogs;
}


bool FreeFileSync::recycleBinExists()
{
    FreeFileSync dummyObject;
    return dummyObject.recycleBinAvailable;
}


bool FreeFileSync::setRecycleBinUsage(bool activate)
{   //If recycleBin is not available it mustn't be activated: This should NEVER happen!
    if (!recycleBinAvailable && activate)
        throw std::runtime_error("Initialization of Recycle Bin failed! It cannot be used!");
    else
        recycleBinAvailable = activate;
    return true;
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


void FreeFileSync::wxULongLongToMpz(mpz_t& output, const wxULongLong& input)
{
    mpz_set_ui(output, input.GetHi());
    mpz_mul_ui(output, output, 65536);
    mpz_mul_ui(output, output, 65536);
    mpz_add_ui(output, output, input.GetLo());
}


bool getBytesToTransfer(double& result, const FileCompareLine& fileCmpLine, const SyncConfiguration& config)
{   //false if nothing has to be done

    result = 0;  //always initialize variables

    //do not add filtered entries
    if (!fileCmpLine.selectedForSynchronization)
        return false;

    switch (fileCmpLine.cmpResult)
    {
    case fileOnLeftSideOnly:
    case fileOnRightSideOnly:
    case leftFileNewer:
    case rightFileNewer:
    case filesDifferent:
        switch (getSyncDirection(fileCmpLine.cmpResult, config))
        {
        case syncDirLeft:   //copy from right to left
            result = fileCmpLine.fileDescrRight.fileSize.ToDouble();
            return true;
        case syncDirRight:  //copy from left to right
            result = fileCmpLine.fileDescrLeft.fileSize.ToDouble();
            return true;
        case syncDirNone:
            return false;
        }
        break;

    case filesEqual:
        return false;
    };

    return true;
}


void FreeFileSync::calcTotalBytesToSync(int& objectsTotal, double& dataTotal, const FileCompareResult& fileCmpResult, const SyncConfiguration& config)
{
    objectsTotal = 0;
    dataTotal    = 0;

    double tmp = 0;

    for (FileCompareResult::const_iterator i = fileCmpResult.begin(); i != fileCmpResult.end(); ++i)
    {   //only sum up sizes of files AND directories
        if (getBytesToTransfer(tmp, *i, config))
        {
            dataTotal+= tmp;
            objectsTotal++;
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
            statusUpdater->updateStatusText(wxString(_("Deleting file ") + filename.fileDescrLeft.filename));
            removeFile(filename.fileDescrLeft.filename);
            break;
        case syncDirRight:  //copy files to right
            target = filename.fileDescrRight.directory + filename.fileDescrLeft.relFilename.c_str();
            statusUpdater->updateStatusText(wxString(_("Copying file ")) + filename.fileDescrLeft.filename +
                                            _(" to ") + target);

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
            statusUpdater->updateStatusText(wxString(_("Copying file ")) + filename.fileDescrRight.filename +
                                            _(" to ") + target);

            copyfileMultithreaded(filename.fileDescrRight.filename, target, statusUpdater);
            break;
        case syncDirRight:  //delete files on right
            statusUpdater->updateStatusText(wxString(_("Deleting file ") + filename.fileDescrRight.filename));
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
            statusUpdater->updateStatusText(wxString(_("Copying file ")) + filename.fileDescrRight.filename +
                                            _(" overwriting ") + filename.fileDescrLeft.filename);

            removeFile(filename.fileDescrLeft.filename);  //only used if switch activated by user, else file is simply deleted
            copyfileMultithreaded(filename.fileDescrRight.filename, filename.fileDescrLeft.filename, statusUpdater);
            break;
        case syncDirRight:  //copy from left to right
            statusUpdater->updateStatusText(wxString(_("Copying file ")) + filename.fileDescrLeft.filename +
                                            _(" overwriting ") + filename.fileDescrRight.filename);

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
            statusUpdater->updateStatusText(wxString(_("Deleting folder ") + filename.fileDescrLeft.filename));
            removeDirectory(filename.fileDescrLeft.filename);
            break;
        case syncDirRight:  //create folders on right
            target = filename.fileDescrRight.directory + filename.fileDescrLeft.relFilename;
            statusUpdater->updateStatusText(wxString(_("Creating folder ") + target));

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
            statusUpdater->updateStatusText(wxString(_("Creating folder ") + target));

            //some check to catch the error that directory on source has been deleted externally after the "compare"...
            if (!wxDirExists(filename.fileDescrRight.filename))
                throw FileError(wxString(_("Error: Source directory does not exist anymore: ")) + filename.fileDescrRight.filename);
            createDirectory(target);
            break;
        case syncDirRight:  //delete folders on right
            statusUpdater->updateStatusText(wxString(_("Deleting folder ") + filename.fileDescrRight.filename));
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


wxString FreeFileSync::formatFilesizeToShortString(const mpz_class& filesize)
{
    mpf_class nrOfBytes = filesize;

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

    mp_exp_t exponent = 0;

    wxString temp;
    if (unit == " Byte")        //no decimal places in case of bytes
    {
        temp = nrOfBytes.get_str(exponent, 10, 3);
        if (!temp.Len()) //if nrOfBytes is zero GMP returns an empty string
            temp = "0";
    }
    else
    {
        temp = wxString(nrOfBytes.get_str(exponent, 10, 3) + "000").substr(0, 3); //returns mantisse of length 3 in all cases

        if (exponent < 0 || exponent > 3)
            temp = _("Error");
        else
            switch (exponent)
            {
            case 0:
                temp = wxString("0") + GlobalResources::floatingPointSeparator + temp.substr(0, 2); //shorten mantisse as a 0 will be inserted
                break;
            case 1:
            case 2:
                temp.insert(exponent, GlobalResources::floatingPointSeparator);
                break;
            case 3:
                break;
            }
    }
    temp+= unit;
    return temp;
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
    for (FileCompareResult::iterator i = currentGridData.begin(); i != currentGridData.end(); i++)
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
    for (FileCompareResult::iterator i = currentGridData.begin(); i != currentGridData.end(); i++)
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


class AlwaysWriteResult //this class ensures, that the result of the method below is ALWAYS written on exit, even if exceptions were thrown!
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
    int objectsTotal = 0;
    double dataTotal = 0;
    FreeFileSync::calcTotalBytesToSync(objectsTotal, dataTotal, grid, config);
    statusUpdater->initNewProcess(objectsTotal, dataTotal, FreeFileSync::synchronizeFilesProcess);

    try
    {
        FreeFileSync fileSyncObject;    //currently only needed for recycle bin
        fileSyncObject.setRecycleBinUsage(useRecycleBin);

        // it should never happen, that a directory on left side has same name as file on right side. GetModelDiff should take care of this
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
                                    //indicator is updated only if file is synched correctly (and if some sync was done)!
                                    double processedData = 0;
                                    if (getBytesToTransfer(processedData, *i, config))  //update status if some work was done (answer is always "yes" in this context)
                                        statusUpdater->updateProcessedData(1, processedData);
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
    FreeFileSync fileSyncObject;    //currently only needed for recycle bin
    fileSyncObject.setRecycleBinUsage(useRecycleBin);

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

