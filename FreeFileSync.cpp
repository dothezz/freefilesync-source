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

using namespace GlobalFunctions;

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


bool filetimeCmpSmallerThan(const FILETIME a, const FILETIME b)
{
    if (a.dwHighDateTime != b.dwHighDateTime)
        return (a.dwHighDateTime < b.dwHighDateTime);
    else
        return (a.dwLowDateTime < b.dwLowDateTime);
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
    WIN32_FIND_DATA winFileInfo;
    FILETIME localFileTime;
    SYSTEMTIME time;
    HANDLE fileHandle;

    if ((fileHandle = FindFirstFile(filename.c_str(), &winFileInfo)) == INVALID_HANDLE_VALUE)
        throw FileError(wxString(_("Could not retrieve file info for: ")) + "\"" + filename + "\"");

    FindClose(fileHandle);

    /*    if (FileTimeToLocalFileTime(
                    &winFileInfo.ftCreationTime,	// pointer to UTC file time to convert
                    &localFileTime 	// pointer to converted file time
                ) == 0)
            throw std::runtime_error("Error converting FILETIME to local FILETIME");

        if (FileTimeToSystemTime(
                    &localFileTime, // pointer to file time to convert
                    &time 	 // pointer to structure to receive system time
                ) == 0)
            throw std::runtime_error("Error converting FILETIME to SYSTEMTIME");

        output.creationTime = numberToString(time.wYear) + "." +
                              formatTime(time.wMonth) + "." +
                              formatTime(time.wDay) + "  " +
                              formatTime(time.wHour) + ":" +
                              formatTime(time.wMinute) + ":" +
                              formatTime(time.wSecond);*/

//*****************************************************************************
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
    output.lastWriteTimeUTC = winFileInfo.ftLastWriteTime;

    mpz_t largeInt;
    mpz_init_set_ui(largeInt, winFileInfo.nFileSizeHigh);
    mpz_mul_ui(largeInt, largeInt, 65536);
    mpz_mul_ui(largeInt, largeInt, 65536);
    mpz_add_ui(largeInt, largeInt, winFileInfo.nFileSizeLow);
    output.fileSize = mpz_get_str(0, 10, largeInt);
    mpz_clear(largeInt);
}


wxString FreeFileSync::calculateMD5Hash(const wxString& filename)
{
    const unsigned int bufferSize = 4096;

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


void FreeFileSync::getModelDiff(FileCompareResult& output, const wxString& dirLeft, const wxString& dirRight, CompareVariant cmpVar, StatusUpdater* updateClass)
{
    assert (updateClass);

    try
    {
        //retrieve sets of files (with description data)
        DirectoryDescrType directoryLeft;
        DirectoryDescrType directoryRight;
        generateFileAndFolderDescriptions(directoryLeft, dirLeft, updateClass);
        generateFileAndFolderDescriptions(directoryRight, dirRight, updateClass);

        FileCompareLine gridLine;

        output.clear();

        //find files/folders that exist in left file model but not in right model
        for (DirectoryDescrType::iterator i = directoryLeft.begin(); i != directoryLeft.end(); i++)
            if (directoryRight.find(*i) == directoryRight.end())
            {
                gridLine.fileDescrLeft            = *i;
                gridLine.fileDescrRight           = FileDescrLine();
                gridLine.fileDescrRight.directory = dirRight;
                gridLine.cmpResult                = FileOnLeftSideOnly;
                output.push_back(gridLine);
            }

        for (DirectoryDescrType::iterator j = directoryRight.begin(); j != directoryRight.end(); j++)
        {
            DirectoryDescrType::iterator i;

            //find files/folders that exist in right file model but not in left model
            if ((i = directoryLeft.find(*j)) == directoryLeft.end())
            {
                gridLine.fileDescrLeft           = FileDescrLine();
                gridLine.fileDescrLeft.directory = dirLeft;
                gridLine.fileDescrRight          = *j;
                gridLine.cmpResult               = FileOnRightSideOnly;
                output.push_back(gridLine);
            }
            //find files that exist in left and right file model
            else
            {       //objType != isNothing
                if (i->objType == IsDirectory && j->objType == IsDirectory)
                {
                    gridLine.fileDescrLeft  = *i;
                    gridLine.fileDescrRight = *j;
                    gridLine.cmpResult      = FilesEqual;
                    output.push_back(gridLine);
                }
                //if we have a nameclash between a file and a directory: split into separate rows
                else if (i->objType != j->objType)
                {
                    gridLine.fileDescrLeft            = *i;
                    gridLine.fileDescrRight           = FileDescrLine();
                    gridLine.fileDescrRight.directory = dirRight;
                    gridLine.cmpResult                = FileOnLeftSideOnly;
                    output.push_back(gridLine);

                    gridLine.fileDescrLeft           = FileDescrLine();
                    gridLine.fileDescrLeft.directory = dirLeft;
                    gridLine.fileDescrRight          = *j;
                    gridLine.cmpResult               = FileOnRightSideOnly;
                    output.push_back(gridLine);
                }
                else if (cmpVar == CompareByTimeAndSize)
                {
                    //check files that exist in left and right model but have different properties
                    if (!filetimeCmpEqual(i->lastWriteTimeUTC, j->lastWriteTimeUTC) ||
                            i->fileSize != j->fileSize)
                    {
                        gridLine.fileDescrLeft  = *i;
                        gridLine.fileDescrRight = *j;

                        if (filetimeCmpEqual(i->lastWriteTimeUTC, j->lastWriteTimeUTC))
                            gridLine.cmpResult      = FilesDifferent;
                        else if (filetimeCmpSmallerThan(i->lastWriteTimeUTC, j->lastWriteTimeUTC))
                            gridLine.cmpResult      = RightFileNewer;
                        else
                            gridLine.cmpResult      = LeftFileNewer;
                        output.push_back(gridLine);
                    }
                    else
                    {
                        gridLine.fileDescrLeft  = *i;
                        gridLine.fileDescrRight = *j;
                        gridLine.cmpResult      = FilesEqual;
                        output.push_back(gridLine);
                    }
                }
                else if (cmpVar == CompareByMD5)
                {
                    while (true)
                    {
                        try
                        {
                            //check files that exist in left and right model but have different checksums
                            if (j->fileSize != i->fileSize || calculateMD5Hash(i->filename) != calculateMD5Hash(j->filename))
                            {
                                gridLine.fileDescrLeft  = *i;
                                gridLine.fileDescrRight = *j;

                                gridLine.cmpResult      = FilesDifferent;
                                output.push_back(gridLine);
                            }
                            else
                            {
                                gridLine.fileDescrLeft  = *i;
                                gridLine.fileDescrRight = *j;
                                gridLine.cmpResult      = FilesEqual;
                                output.push_back(gridLine);
                            }
                            break;
                        }
                        catch (FileError& error)
                        {
                            //if (updateClass) -> is mandatory
                            int rv = updateClass->reportError(error.show());
                            if ( rv == StatusUpdater::Continue)
                                break;
                            else if (rv == StatusUpdater::Retry)
                                ;   //continue with loop
                            else
                                assert (false);
                        }
                    }
                }
                else assert (false);
            }
        }
        updateClass->triggerUI_Refresh();
    }
    catch (std::runtime_error& theException)
    {
        wxMessageBox(_(theException.what()), _("An exception occured!"), wxOK | wxICON_ERROR);
        return;
    }
}


void FreeFileSync::swapGrids(FileCompareResult& grid)
{
    FileDescrLine tmp;

    for (FileCompareResult::iterator i = grid.begin(); i != grid.end(); ++i)
    {
        //swap compare result
        if (i->cmpResult == FileOnLeftSideOnly)
            i->cmpResult = FileOnRightSideOnly;
        else if (i->cmpResult == FileOnRightSideOnly)
            i->cmpResult = FileOnLeftSideOnly;
        else if (i->cmpResult == RightFileNewer)
            i->cmpResult = LeftFileNewer;
        else if (i->cmpResult == LeftFileNewer)
            i->cmpResult = RightFileNewer;

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
            if ( rv == StatusUpdater::Continue)
                return wxDIR_CONTINUE;
            else if (rv == StatusUpdater::Retry)
                ;   //continue with loop
            else
                assert (false);
        }
    }

    fileDescr.lastWriteTime    = currentFileInfo.lastWriteTime;
    fileDescr.lastWriteTimeUTC = currentFileInfo.lastWriteTimeUTC;
    fileDescr.fileSize         = currentFileInfo.fileSize;
    fileDescr.objType          = IsFile;
    m_output.insert(fileDescr);

    //update UI/commandline status information
    statusUpdater->updateStatus(filename);         // NO performance issue at all
    //add 1 element to the progress indicator
    statusUpdater->updateProgressIndicator(1);     // NO performance issue at all
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
            if ( rv == StatusUpdater::Continue)
                return wxDIR_IGNORE;
            else if (rv == StatusUpdater::Retry)
                ;   //continue with loop
            else
                assert (false);
        }
    }
    fileDescr.lastWriteTime    = currentFileInfo.lastWriteTime;
    fileDescr.lastWriteTimeUTC = currentFileInfo.lastWriteTimeUTC;
    fileDescr.fileSize         = "0";     //currentFileInfo.fileSize should be "0" as well, but just to be sure...  <- isn't needed anyway...
    fileDescr.objType          = IsDirectory;
    m_output.insert(fileDescr);

    //update UI/commandline status information
    statusUpdater->updateStatus(dirname);          // NO performance issue at all
    //add 1 element to the progress indicator
    statusUpdater->updateProgressIndicator(1);     // NO performance issue at all
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

    if (!SetFileAttributes(
                filename.c_str(), // address of filename
                FILE_ATTRIBUTE_NORMAL	 	// address of attributes to set
            )) throw FileError(wxString(_("Error deleting file ")) + "\"" + filename + "\"");


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
    wxString createFirstDir = wxDir(directory).GetName().BeforeLast(FileNameSeparator);

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

class CopyThread : public wxThread
{
public:
    CopyThread(const wxString& sourceFile, const wxString& targetFile) :
            source(sourceFile),
            target(targetFile)
    {}

    ~CopyThread() {}

    ExitCode Entry()
    {
        bool success = (CopyFile(
                            source.c_str(),	// pointer to name of an existing file
                            target.c_str(),	// pointer to filename to copy to
                            TRUE 	// break if file exists
                        ));

        copyFileCritSec.Enter();

        //report status to main thread
        threadIsFinished    = true;
        threadWasSuccessful = success;

        copyFileCritSec.Leave();

        return 0;
    }

    static wxCriticalSection copyFileCritSec;

    //shared thread data    -> protect with critical section!
    static bool threadIsFinished;
    static bool threadWasSuccessful;
    //

private:
    const wxString& source;
    const wxString& target;
};

wxCriticalSection CopyThread::copyFileCritSec;
bool CopyThread::threadIsFinished    = true;
bool CopyThread::threadWasSuccessful = true;



void FreeFileSync::copyfileMultithreaded(const wxString& source, const wxString& target, StatusUpdater* updateClass)
{
    //at this point threadIsRunning is not shared between threads
    CopyThread::threadIsFinished = false;

    //create thread for copying of (large) files
    CopyThread* copyFileThread = new CopyThread(source, target);    //quite faster than joinable threads

    copyFileThread->Create();
    copyFileThread->SetPriority(80);
    copyFileThread->Run();

    bool processCompleted;
    while (true)
    {
        CopyThread::copyFileCritSec.Enter();
        processCompleted = CopyThread::threadIsFinished; //always put shared data into mutextes/critical sections
        CopyThread::copyFileCritSec.Leave();

        if (processCompleted)   //if completed the data is not shared anymore
        {
            if (!CopyThread::threadWasSuccessful)
            {
                throw FileError(wxString(_("Error copying file ")) + "\"" + source + "\"" + _(" to ") + "\"" + target + "\"");
            }
            else return;
        }

        updateClass->triggerUI_Refresh();
    }
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

    //prevent logging of wxWidgets
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
    case FileOnLeftSideOnly:
        return config.exLeftSideOnly;
        break;

    case FileOnRightSideOnly:
        return config.exRightSideOnly;
        break;

    case RightFileNewer:
        return config.rightNewer;
        break;

    case LeftFileNewer:
        return config.leftNewer;
        break;

    case FilesDifferent:
        return config.different;
        break;

    default:
        assert (false);
    }
    return SyncDirNone;
}


void FreeFileSync::getBytesToTransfer(mpz_t& result, const FileCompareLine& fileCmpLine, const SyncConfiguration& config)
{
    mpz_set_ui(result, 0);  //always initialize variables

    //do not add filtered entries
    if (!fileCmpLine.selectedForSynchronization)
        return;

    int returnValue = 0;

    switch (fileCmpLine.cmpResult)
    {
    case FileOnLeftSideOnly:
        if (config.exLeftSideOnly == SyncDirRight)
            //copy files to right
            returnValue = mpz_set_str(result, fileCmpLine.fileDescrLeft.fileSize.c_str(), 10);
        break;

    case FileOnRightSideOnly:
        if (config.exRightSideOnly == SyncDirLeft)
            //copy files to left
            returnValue = mpz_set_str(result, fileCmpLine.fileDescrRight.fileSize.c_str(), 10);
        break;

    case LeftFileNewer:
    case RightFileNewer:
    case FilesDifferent:
        switch (getSyncDirection(fileCmpLine.cmpResult, config))
        {
        case SyncDirLeft:   //copy from right to left
            returnValue = mpz_set_str(result, fileCmpLine.fileDescrRight.fileSize.c_str(), 10);
            break;
        case SyncDirRight:  //copy from left to right
            returnValue = mpz_set_str(result, fileCmpLine.fileDescrLeft.fileSize.c_str(), 10);
            break;
        case SyncDirNone:
            break;
        }
        break;

    case FilesEqual:
        break;
    };
    assert (returnValue == 0);
}


mpz_class FreeFileSync::calcTotalBytesToTransfer(const FileCompareResult& fileCmpResult, const SyncConfiguration& config)
{
    mpz_t largeTmpInt, result_c;
    mpz_init(largeTmpInt);
    mpz_init(result_c);

    for (FileCompareResult::const_iterator i = fileCmpResult.begin(); i != fileCmpResult.end(); ++i)
    {   //only sum up sizes of files, not directories
        if (i->fileDescrLeft.objType == IsFile || i->fileDescrRight.objType == IsFile)
        {
            getBytesToTransfer(largeTmpInt, *i, config);
            mpz_add(result_c, result_c, largeTmpInt); //much faster than converting this to mpz_class for each iteration
        }
    }

    mpz_class result(result_c);

    mpz_clear(largeTmpInt);
    mpz_clear(result_c);

    return result;
}


bool FreeFileSync::synchronizeFile(const FileCompareLine& filename, const SyncConfiguration& config, StatusUpdater* statusUpdater)
{
    assert (statusUpdater);

    if (!filename.selectedForSynchronization) return false;

    wxString target;

    //synchronize file:
    switch (filename.cmpResult)
    {
    case FileOnLeftSideOnly:
        switch (config.exLeftSideOnly)
        {
        case SyncDirLeft:   //delete files on left
            statusUpdater->updateStatus(wxString(_("Deleting file ") + filename.fileDescrLeft.filename));
            removeFile(filename.fileDescrLeft.filename);
            break;
        case SyncDirRight:  //copy files to right
            target = filename.fileDescrRight.directory + filename.fileDescrLeft.relFilename.c_str();
            statusUpdater->updateStatus(wxString(_("Copying file ")) + filename.fileDescrLeft.filename +
                                        _(" to ") + target);

            copyfileMultithreaded(filename.fileDescrLeft.filename, target, statusUpdater);
            break;
        case SyncDirNone:
            return false;
            break;
        default:
            assert (false);
        }
        break;

    case FileOnRightSideOnly:
        switch (config.exRightSideOnly)
        {
        case SyncDirLeft:   //copy files to left
            target = filename.fileDescrLeft.directory + filename.fileDescrRight.relFilename;
            statusUpdater->updateStatus(wxString(_("Copying file ")) + filename.fileDescrRight.filename +
                                        _(" to ") + target);

            copyfileMultithreaded(filename.fileDescrRight.filename, target, statusUpdater);
            break;
        case SyncDirRight:  //delete files on right
            statusUpdater->updateStatus(wxString(_("Deleting file ") + filename.fileDescrRight.filename));
            removeFile(filename.fileDescrRight.filename);
            break;
        case SyncDirNone:
            return false;
            break;
        default:
            assert (false);
        }
        break;

    case LeftFileNewer:
    case RightFileNewer:
    case FilesDifferent:
        switch (getSyncDirection(filename.cmpResult, config))
        {
        case SyncDirLeft:   //copy from right to left
            statusUpdater->updateStatus(wxString(_("Copying file ")) + filename.fileDescrRight.filename +
                                        _(" overwriting ") + filename.fileDescrLeft.filename);

            removeFile(filename.fileDescrLeft.filename);  //only used if switch activated by user, else file is simply deleted
            copyfileMultithreaded(filename.fileDescrRight.filename, filename.fileDescrLeft.filename, statusUpdater);
            break;
        case SyncDirRight:  //copy from left to right
            statusUpdater->updateStatus(wxString(_("Copying file ")) + filename.fileDescrLeft.filename +
                                        _(" overwriting ") + filename.fileDescrRight.filename);

            removeFile(filename.fileDescrRight.filename);  //only used if switch activated by user, else file is simply deleted
            copyfileMultithreaded(filename.fileDescrLeft.filename, filename.fileDescrRight.filename, statusUpdater);
            break;
        case SyncDirNone:
            return false;
            break;
        default:
            assert (false);
        }
        break;

    case FilesEqual:
        return false;
        break;

    default:
        assert (false);
    }
    return true;
}


bool FreeFileSync::synchronizeFolder(const FileCompareLine& filename, const SyncConfiguration& config, StatusUpdater* statusUpdater)
{
    assert (statusUpdater);

    if (!filename.selectedForSynchronization) return false;

    wxString target;

    //synchronize folders:
    switch (filename.cmpResult)
    {
    case FileOnLeftSideOnly:
        switch (config.exLeftSideOnly)
        {
        case SyncDirLeft:   //delete folders on left
            statusUpdater->updateStatus(wxString(_("Deleting folder ") + filename.fileDescrLeft.filename));
            removeDirectory(filename.fileDescrLeft.filename);
            break;
        case SyncDirRight:  //create folders on right
            target = filename.fileDescrRight.directory + filename.fileDescrLeft.relFilename;
            statusUpdater->updateStatus(wxString(_("Creating folder ") + target));

            //some check to catch the error that directory on source has been deleted externally after the "compare"...
            if (!wxDirExists(filename.fileDescrLeft.filename))
                throw FileError(wxString(_("Error: Source directory does not exist anymore: ")) + "\"" + filename.fileDescrLeft.filename + "\"");
            createDirectory(target);
            break;
        case SyncDirNone:
            return false;
            break;
        default:
            assert (false);
        }
        break;

    case FileOnRightSideOnly:
        switch (config.exRightSideOnly)
        {
        case SyncDirLeft:   //create folders on left
            target = filename.fileDescrLeft.directory + filename.fileDescrRight.relFilename;
            statusUpdater->updateStatus(wxString(_("Creating folder ") + target));

            //some check to catch the error that directory on source has been deleted externally after the "compare"...
            if (!wxDirExists(filename.fileDescrRight.filename))
                throw FileError(wxString(_("Error: Source directory does not exist anymore: ")) + filename.fileDescrRight.filename);
            createDirectory(target);
            break;
        case SyncDirRight:  //delete folders on right
            statusUpdater->updateStatus(wxString(_("Deleting folder ") + filename.fileDescrRight.filename));
            removeDirectory(filename.fileDescrRight.filename);
            break;
        case SyncDirNone:
            return false;
            break;
        default:
            assert (false);
        }
        break;

    case FilesEqual:
        return false;
        break;
    case RightFileNewer:
    case LeftFileNewer:
    case FilesDifferent:
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
                temp = wxString("0") + FloatingPointSeparator + temp.substr(0, 2); //shorten mantisse as a 0 will be inserted
                break;
            case 1:
            case 2:
                temp.insert(exponent, FloatingPointSeparator);
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
        if (i->fileDescrLeft.objType != IsNothing)
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

        if (i->fileDescrRight.objType != IsNothing)
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

    result.Append(FileNameSeparator);
    return result;
}


inline
bool deletionImminent(const FileCompareLine& line, const SyncConfiguration& config)
{   //test if current sync-line will result in deletion of files    -> used to avoid disc space bottlenecks
    if ((line.cmpResult == FileOnLeftSideOnly && config.exLeftSideOnly == SyncDirLeft) ||
            (line.cmpResult == FileOnRightSideOnly && config.exRightSideOnly == SyncDirRight))
        return true;
    else
        return false;
}


void FreeFileSync::startSynchronizationProcess(FileCompareResult& grid, const SyncConfiguration& config, StatusUpdater* statusUpdater, bool useRecycleBin)
{
    assert (statusUpdater);

    try
    {
        FreeFileSync fileSyncObject;    //currently only needed for recycle bin
        fileSyncObject.setRecycleBinUsage(useRecycleBin);

        FileCompareResult resultsGrid;

        // it should never happen, that a directory on left side has same name as file on right side. GetModelDiff should take care of this
        // and split into two "exists on one side only" cases
        // Note: this case is not handled by this tool as this is considered to be a bug and must be solved by the user

        //synchronize folders:
        for (FileCompareResult::const_iterator i = grid.begin(); i != grid.end(); ++i)
        {
            if (i->fileDescrLeft.objType == IsDirectory || i->fileDescrRight.objType == IsDirectory)
            {
                while (true)
                {    //trigger display refresh
                    statusUpdater->triggerUI_Refresh();

                    try
                    {
                        if (fileSyncObject.synchronizeFolder(*i, config, statusUpdater))
                            //progress indicator update
                            //indicator is updated only if directory is synched correctly  (and if some sync was done)!
                            statusUpdater->updateProgressIndicator(0);  //each call represents one processed file/directory
                        break;
                    }
                    catch (FileError& error)
                    {
                        //if (updateClass) -> is mandatory
                        int rv = statusUpdater->reportError(error.show());

                        if ( rv == StatusUpdater::Continue)
                        {
                            resultsGrid.push_back(*i); //append folders that have not been synced successfully
                            break;
                        }
                        else if (rv == StatusUpdater::Retry)
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
                if (i->fileDescrLeft.objType == IsFile || i->fileDescrRight.objType == IsFile)
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
                                    mpz_t largeTmpInt;
                                    mpz_init(largeTmpInt);
                                    FreeFileSync::getBytesToTransfer(largeTmpInt, *i, config);
                                    statusUpdater->updateProgressIndicator(mpz_get_d(largeTmpInt));
                                    mpz_clear(largeTmpInt);
                                }
                                break;
                            }
                            catch (FileError& error)
                            {
                                //if (updateClass) -> is mandatory
                                int rv = statusUpdater->reportError(error.show());

                                if ( rv == StatusUpdater::Continue)
                                {
                                    resultsGrid.push_back(*i); //append files that have not been synced successfully
                                    break;
                                }
                                else if (rv == StatusUpdater::Retry)
                                    ;   //continue with loop
                                else
                                    assert (false);
                            }
                        }
                    }
                }
            }
        }

        //return files that couldn't be processed
        grid = resultsGrid;
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

    if (relevantRow.fileDescrLeft.objType == IsDirectory)
        relevantDirectory = relevantRow.fileDescrLeft.relFilename;

    else if (relevantRow.fileDescrRight.objType == IsDirectory)
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
                if (currentCmpLine.fileDescrLeft.objType == IsFile)
                    fileSyncObject.removeFile(currentCmpLine.fileDescrLeft.filename);
                else if (currentCmpLine.fileDescrLeft.objType == IsDirectory)
                    fileSyncObject.removeDirectory(currentCmpLine.fileDescrLeft.filename);

                if (currentCmpLine.fileDescrRight.objType == IsFile)
                    fileSyncObject.removeFile(currentCmpLine.fileDescrRight.filename);
                else if (currentCmpLine.fileDescrRight.objType == IsDirectory)
                    fileSyncObject.removeDirectory(currentCmpLine.fileDescrRight.filename);

                rowsToDeleteInGrid.insert(*i);

                break;
            }
            catch (FileError& error)
            {

                //if (updateClass) -> is mandatory
                int rv = statusUpdater->reportError(error.show());

                if (rv == StatusUpdater::Continue)
                    break;

                else if (rv == StatusUpdater::Retry)
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

    //for (set<int>::reverse_iterator i = rowsToDeleteInGrid.rbegin(); i != rowsToDeleteInGrid.rend(); ++i)
    //    grid.erase(grid.begin() + *i);

    //Note: the following lines are a performance optimization for deleting elements from a vector. It is incredibly faster to create a new
    //vector and leave specific elements out than to delete row by row and force recopying of most elements for each single deletion (linear vs quadratic runtime)

    FileCompareResult temp;
    int rowNr = 0;
    int rowToSkip = -1;

    set<int>::iterator rowToSkipIndex = rowsToDeleteInGrid.begin();

    if (rowToSkipIndex != rowsToDeleteInGrid.end())
        rowToSkip = *rowToSkipIndex;

    for (FileCompareResult::iterator i = grid.begin(); i != grid.end(); ++i, ++rowNr)
    {
        if (rowNr != rowToSkip)
            temp.push_back(*i);
        else
        {
            rowToSkipIndex++;
            if (rowToSkipIndex != rowsToDeleteInGrid.end())
                rowToSkip = *rowToSkipIndex;
        }
    }
    grid.swap(temp);
}


