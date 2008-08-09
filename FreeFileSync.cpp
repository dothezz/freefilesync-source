#include <wx/arrstr.h>
#include <wx/dir.h>
#include <wx/msgdlg.h>
#include "FreeFileSync.h"
#include "library\md5.h"
#include <stdexcept> //for std::runtime_error
#include "library\globalfunctions.h"
#include "library\GMP\include\gmp.h"
#include <wx/filename.h>

using namespace GlobalFunctions;

inline
string formatTime(unsigned int number)
{
    assert (number < 100);
    char result[3];

    if (number <= 9)
    {
        *result = '0';
        result[1] = '0' + number;
        result[2] = 0;
    }
    else
        sprintf(result, "%u", number);
    return result;
}

bool filetimeCmpSmallerThan(const FILETIME a,const FILETIME b)
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
        throw fileError((wxString(_("Could not retrieve file info for: ")) + "\"" + filename.c_str() + "\"").c_str());

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

    output.lastWriteTime = numberToString(time.wYear) + "-" +
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
        throw std::runtime_error((wxString(_("Could not open file: ")) + "\"" + filename + "\"").c_str());
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


void FreeFileSync::generateFileAndFolderDescriptions(DirectoryDescrType& output, const wxString& directory)
{
    output.clear();

    //get all files and folders from directory (and subdirectories)
    wxGetAllFiles traverser(output, directory);
    wxDir dir(directory);
    dir.Traverse(traverser);
}


void FreeFileSync::getModelDiff(FileCompareResult& output, const wxString& dirLeft, const wxString& dirRight, CompareVariant cmpVar)
{
    //retrieve sets of files (with description data)
    DirectoryDescrType directoryLeft;
    DirectoryDescrType directoryRight;
    generateFileAndFolderDescriptions(directoryLeft, dirLeft);
    generateFileAndFolderDescriptions(directoryRight, dirRight);

    FileCompareLine gridLine;

    output.clear();

    //find files/folders that exist in left file model but not in right model
    for (DirectoryDescrType::iterator i = directoryLeft.begin(); i != directoryLeft.end(); i++)
        if (directoryRight.find(*i) == directoryRight.end())
        {
            gridLine.fileDescrLeft            = *i;
            gridLine.fileDescrRight           = FileDescrLine();
            gridLine.fileDescrRight.directory = dirRight;
            gridLine.cmpResult                = fileOnLeftSideOnly;
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
            gridLine.cmpResult               = fileOnRightSideOnly;
            output.push_back(gridLine);
        }
        //find files that exist in left and right file model
        else
        {       //objType != isNothing
            if (i->objType == isDirectory && j->objType == isDirectory)
            {
                gridLine.fileDescrLeft  = *i;
                gridLine.fileDescrRight = *j;
                gridLine.cmpResult      = filesEqual;
                output.push_back(gridLine);
            }
            //if we have a nameclash between a file and a directory: split into separate rows
            else if (i->objType != j->objType)
            {
                gridLine.fileDescrLeft            = *i;
                gridLine.fileDescrRight           = FileDescrLine();
                gridLine.fileDescrRight.directory = dirRight;
                gridLine.cmpResult                = fileOnLeftSideOnly;
                output.push_back(gridLine);

                gridLine.fileDescrLeft           = FileDescrLine();
                gridLine.fileDescrLeft.directory = dirLeft;
                gridLine.fileDescrRight          = *j;
                gridLine.cmpResult               = fileOnRightSideOnly;
                output.push_back(gridLine);
            }
            else if (cmpVar == compareByTimeAndSize)
            {
                //check files that exist in left and right model but have different properties
                if (!filetimeCmpEqual(i->lastWriteTimeUTC, j->lastWriteTimeUTC) ||
                        i->fileSize != j->fileSize)
                {
                    gridLine.fileDescrLeft  = *i;
                    gridLine.fileDescrRight = *j;

                    if (filetimeCmpEqual(i->lastWriteTimeUTC, j->lastWriteTimeUTC))
                        gridLine.cmpResult      = filesDifferent;
                    else if (filetimeCmpSmallerThan(i->lastWriteTimeUTC, j->lastWriteTimeUTC))
                        gridLine.cmpResult      = rightFileNewer;
                    else
                        gridLine.cmpResult      = leftFileNewer;
                    output.push_back(gridLine);
                }
                else
                {
                    gridLine.fileDescrLeft  = *i;
                    gridLine.fileDescrRight = *j;
                    gridLine.cmpResult      = filesEqual;
                    output.push_back(gridLine);
                }
            }
            else if (cmpVar == compareByMD5)
            {
                //check files that exist in left and right model but have different checksums
                if (j->fileSize != i->fileSize || calculateMD5Hash(i->filename) != calculateMD5Hash(j->filename))
                {
                    gridLine.fileDescrLeft  = *i;
                    gridLine.fileDescrRight = *j;

                    gridLine.cmpResult      = filesDifferent;
                    output.push_back(gridLine);
                }
                else
                {
                    gridLine.fileDescrLeft  = *i;
                    gridLine.fileDescrRight = *j;
                    gridLine.cmpResult      = filesEqual;
                    output.push_back(gridLine);
                }

            }
            else assert (false);
        }
    }
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


wxGetAllFiles::wxGetAllFiles(DirectoryDescrType& output, wxString dirThatIsSearched)
        : m_output(output), directory(dirThatIsSearched)
{
    prefixLength = directory.Len();
}


wxDirTraverseResult wxGetAllFiles::OnFile(const wxString& filename)
{
    fileDescr.filename         = filename;
    fileDescr.directory        = directory;
    fileDescr.relFilename      = filename.Mid(prefixLength, wxSTRING_MAXLEN);

    FreeFileSync::getFileInformation(currentFileInfo, filename);
    fileDescr.lastWriteTime    = currentFileInfo.lastWriteTime.c_str();
    fileDescr.lastWriteTimeUTC = currentFileInfo.lastWriteTimeUTC;
    fileDescr.fileSize         = currentFileInfo.fileSize;
    fileDescr.objType          = isFile;
    m_output.insert(fileDescr);

    return wxDIR_CONTINUE;
}


wxDirTraverseResult wxGetAllFiles::OnDir(const wxString& dirname)
{
    if (dirname.EndsWith("\\RECYCLER"))
        return wxDIR_IGNORE;
    if (dirname.EndsWith("\\System Volume Information"))
        return wxDIR_IGNORE;

    fileDescr.filename         = dirname;
    fileDescr.directory        = directory;
    fileDescr.relFilename      = dirname.Mid(prefixLength, wxSTRING_MAXLEN);

    FreeFileSync::getFileInformation(currentFileInfo, dirname);
    fileDescr.lastWriteTime    = currentFileInfo.lastWriteTime.c_str();
    fileDescr.lastWriteTimeUTC = currentFileInfo.lastWriteTimeUTC;
    fileDescr.fileSize         = "0";     //currentFileInfo.fileSize should be "0" as well, but just to be sure...
    fileDescr.objType          = isDirectory;
    m_output.insert(fileDescr);

    return wxDIR_CONTINUE;
}


wxDirTraverseResult wxGetAllFiles::OnOpenError(const wxString& openerrorname)
{
    wxMessageBox(openerrorname, _("Error opening file"));
    return wxDIR_IGNORE;
}


//#####################################################################################################

inline
void FreeFileSync::removeFile(const char* filename)
{
    if (!SetFileAttributes(
                filename, // address of filename
                FILE_ATTRIBUTE_NORMAL	 	// address of attributes to set
            )) throw fileError(string(_("Error deleting file ")) + filename);

    if (!DeleteFile(filename)) throw fileError(string(_("Error deleting file ")) + filename);
}


class wxGetDirsOnly : public wxDirTraverser
{
public:
    wxGetDirsOnly(wxArrayString& output)
            : m_output(output) {}

    wxDirTraverseResult OnDir(const wxString& dirname)
    {
        m_output.Add(dirname);
        return wxDIR_CONTINUE;
    }

    wxDirTraverseResult OnFile(const wxString& filename)
    {
        return wxDIR_CONTINUE;
    }

    wxDirTraverseResult OnOpenError(const wxString& openerrorname)
    {
        wxMessageBox(openerrorname, _("Error opening file"));
        return wxDIR_IGNORE;
    }

private:
    wxArrayString& m_output;
    int prefixLength;
};


void FreeFileSync::deleteDirectoryCompletely(const char* directory)
{
    {
        wxDir dir(directory);

        //get all files from directory (and subdirectories)
        wxArrayString fileList;
        wxGetDirsOnly traverser(fileList);
        dir.Traverse(traverser);

        for (int j = 0; j <= int(fileList.GetCount()) - 1; ++j)
            removeFile(fileList[j].c_str());


        //get all directories from directory (and subdirectories)
        wxArrayString dirList;
        wxGetDirsOnly traverser2(dirList);
        dir.Traverse(traverser2);

        for (int j = int(dirList.GetCount()) - 1; j >= 0 ; --j)
            if (!RemoveDirectory(dirList[j].c_str()// address of directory to remove
                                ))  throw fileError(string(_("Error deleting directory ")) + dirList[j].c_str());
    }
    if (!RemoveDirectory(
                directory // address of directory to remove
            ))  throw fileError(string(_("Error deleting directory ")) + directory);

}


void FreeFileSync::copyOverwriting(const char* source, const char* target)
{
    if (!SetFileAttributes(
                target, // address of filename
                FILE_ATTRIBUTE_NORMAL	 	// address of attributes to set
            )) throw fileError(string(_("Error overwriting file ")) + target);

    if (!CopyFile(
                source,	// pointer to name of an existing file
                target,	// pointer to filename to copy to
                FALSE 	// overwrite if file exists
            ))  throw fileError(string(_("Error copying file ")) + source + _(" to ") + target);
}


void FreeFileSync::createDirectoriesRecursively(const wxString& directory, int level)
{
    if (level == 50)    //catch endless loop
        return;


//try to create directory
    if (CreateDirectory(
                directory.c_str(),	// pointer to a directory path string
                NULL // pointer to a security descriptor
            )) return;

    //if not successfull try to create containing folders first
    wxString createFirstDir;
    if (level == 0 && directory.Last() == wxChar('\\'))
        createFirstDir = (directory.BeforeLast(wxChar('\\'))).BeforeLast('\\');
    else
        createFirstDir = directory.BeforeLast('\\');

    //call function recursively
    if (createFirstDir.IsEmpty()) return;
    createDirectoriesRecursively(createFirstDir, level + 1);

    //now creation should be possible
    if (!CreateDirectory(
                directory.c_str(),	// pointer to a directory path string
                NULL // pointer to a security descriptor
            ))
    {
        if (level == 0)
            throw fileError(string(_("Error creating directory ")) + directory.c_str());
        else
            return;
    }
}


void FreeFileSync::copyfile(const char* source, const char* target)
{
    if (!CopyFile(
                source,	// pointer to name of an existing file
                target,	// pointer to filename to copy to
                TRUE 	// break if file exists
            )) throw fileError(string(_("Error copying file ")) + source + _(" to ") + target);
}


void FreeFileSync::copyCreatingDirs(const char* source, const char* target)
{
    wxString targetPath = wxFileName(wxString(target)).GetPath();
    if (!wxDirExists(targetPath))
        createDirectoriesRecursively(targetPath);

    if (!CopyFile(
                source,	// pointer to name of an existing file
                target,	// pointer to filename to copy to
                TRUE 	// break if file exists
            )) throw fileError(string(_("Error copying file ")) + source + _(" to ") + target);
}


FreeFileSync::FreeFileSync() : SHFileOperationNotFound(false)
{
    // Get a handle to the DLL module.
    hinstShell = LoadLibrary("shell32.dll");
    if (hinstShell == NULL)
    {
        SHFileOperationNotFound = true;
    }
    else
    {

        fileOperation  = (DLLFUNC)GetProcAddress(hinstShell, "SHFileOperation");
        if (fileOperation == NULL )
        {
            FreeLibrary(hinstShell);
            SHFileOperationNotFound = true;
        }
    }
}


FreeFileSync::~FreeFileSync()
{
    if (!SHFileOperationNotFound)
        FreeLibrary(hinstShell);
}


void FreeFileSync::moveToRecycleBin(const char* filename)
{
    if (SHFileOperationNotFound)
    {   //fallback: needed for Vista for example
        removeFile(filename);
        return;
    }

    BOOL aborted = false;
    SHFILEOPSTRUCT fileOp;

    char filenameDoubleNull[MAX_PATH + 1];
    strcpy (filenameDoubleNull, filename);
    filenameDoubleNull[strlen(filenameDoubleNull) + 1] = 0;

    fileOp.hwnd   = NULL;
    fileOp.wFunc  = FO_DELETE;
    fileOp.pFrom  = filenameDoubleNull;
    fileOp.pTo    = NULL;
    fileOp.fFlags = FOF_ALLOWUNDO | FOF_NOCONFIRMATION | FOF_SILENT;
    fileOp.fAnyOperationsAborted = aborted;
    fileOp.hNameMappings         = NULL;
    fileOp.lpszProgressTitle     = NULL;

    if (fileOperation(&fileOp   //Pointer to an SHFILEOPSTRUCT structure that contains information the function needs to carry out.
                     ) != 0 || aborted) throw fileError(string(_("Error moving file ")) + filename + _(" to recycle bin!"));
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


void FreeFileSync::getBytesToTransfer(mpz_t& result, const FileCompareLine& fileCmpLine, const SyncConfiguration& config)
{
    mpz_set_ui(result, 0);  //always initialize variables
    int returnValue = 0;

    switch (fileCmpLine.cmpResult)
    {
    case fileOnLeftSideOnly:
        if (config.exLeftSideOnly == syncDirRight)
            //copy files to right
            returnValue = mpz_set_str(result, fileCmpLine.fileDescrLeft.fileSize.c_str(), 10);
        break;

    case fileOnRightSideOnly:
        if (config.exRightSideOnly == syncDirLeft)
            //copy files to left
            returnValue = mpz_set_str(result, fileCmpLine.fileDescrRight.fileSize.c_str(), 10);
        break;

    case leftFileNewer:
    case rightFileNewer:
    case filesDifferent:
        switch (getSyncDirection(fileCmpLine.cmpResult, config))
        {
        case syncDirLeft:   //copy from right to left
            returnValue = mpz_set_str(result, fileCmpLine.fileDescrRight.fileSize.c_str(), 10);
            break;
        case syncDirRight:  //copy from left to right
            returnValue = mpz_set_str(result, fileCmpLine.fileDescrLeft.fileSize.c_str(), 10);
            break;
        case syncDirNone:
            break;
        }
        break;

    case filesEqual:
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
    {
        getBytesToTransfer(largeTmpInt, *i, config);
        mpz_add(result_c, result_c, largeTmpInt); //much faster than converting this to mpz_class for each iteration
    }

    mpz_class result(result_c);

    mpz_clear(largeTmpInt);
    mpz_clear(result_c);

    return result;
}


bool FreeFileSync::synchronizeFile(const FileCompareLine& filename, const SyncConfiguration& config)
{
    if (!filename.selectedForSynchronization) return false;

    wxString target;

    //synchronize file:
    switch (filename.cmpResult)
    {
    case fileOnLeftSideOnly:
        switch (config.exLeftSideOnly)
        {
        case syncDirLeft:   //delete files on left
            if (wxFileExists(filename.fileDescrLeft.filename.c_str()))
                moveToRecycleBin(filename.fileDescrLeft.filename.c_str());
            break;
        case syncDirRight:  //copy files to right
            target = filename.fileDescrRight.directory + filename.fileDescrLeft.relFilename.c_str();
            FreeFileSync::copyfile(filename.fileDescrLeft.filename.c_str(), target.c_str());
            break;
        case syncDirNone:
            return false;
            break;
        default:
            assert (false);
        }
        break;

    case fileOnRightSideOnly:
        switch (config.exRightSideOnly)
        {
        case syncDirLeft:   //copy files to left
            target = filename.fileDescrLeft.directory + filename.fileDescrRight.relFilename.c_str();
            FreeFileSync::copyfile(filename.fileDescrRight.filename.c_str(), target.c_str());
            break;
        case syncDirRight:  //delete files on right
            if (wxFileExists(filename.fileDescrRight.filename.c_str()))
                moveToRecycleBin(filename.fileDescrRight.filename.c_str());
            break;
        case syncDirNone:
            return false;
            break;
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
            FreeFileSync::copyOverwriting(filename.fileDescrRight.filename.c_str(), filename.fileDescrLeft.filename.c_str());
            break;
        case syncDirRight:  //copy from left to right
            FreeFileSync::copyOverwriting(filename.fileDescrLeft.filename.c_str(), filename.fileDescrRight.filename.c_str());
            break;
        case syncDirNone:
            return false;
            break;
        default:
            assert (false);
        }
        break;

    case filesEqual:
        //return false;  commented out: ONLY files that have not been sync'ed should appear on the results grid
        //else: files that HAVE been synced are missing... this does not seem to be consistent either
        break;

    default:
        assert (false);
    }
    return (true);
}


bool FreeFileSync::synchronizeFolder(const FileCompareLine& filename, const SyncConfiguration& config)
{
    if (!filename.selectedForSynchronization) return false;

    wxString target ;

    //synchronize folders:
    switch (filename.cmpResult)
    {
    case fileOnLeftSideOnly:
        switch (config.exLeftSideOnly)
        {
        case syncDirLeft:   //delete folders on left
            if (wxDirExists(filename.fileDescrLeft.filename.c_str()))
            {
                if (SHFileOperationNotFound)
                    deleteDirectoryCompletely(filename.fileDescrLeft.filename.c_str());
                else
                    moveToRecycleBin(filename.fileDescrLeft.filename.c_str());
            }

            break;
        case syncDirRight:  //create folders on right
            target = filename.fileDescrRight.directory + filename.fileDescrLeft.relFilename.c_str();
            if (!wxDirExists(target.c_str()))
                createDirectoriesRecursively(target);
            break;
        case syncDirNone:
            return false;
            break;
        default:
            assert (false);
        }
        break;

    case fileOnRightSideOnly:
        switch (config.exRightSideOnly)
        {
        case syncDirLeft:   //create folders on left
            target = filename.fileDescrLeft.directory + filename.fileDescrRight.relFilename.c_str();
            if (!wxDirExists(target.c_str()))
                createDirectoriesRecursively(target);
            break;
        case syncDirRight:  //delete folders on right
            if (wxDirExists(filename.fileDescrRight.filename.c_str()))
            {
                if (SHFileOperationNotFound)
                    deleteDirectoryCompletely(filename.fileDescrRight.filename.c_str());
                else
                    moveToRecycleBin(filename.fileDescrRight.filename.c_str());
            }
            break;
        case syncDirNone:
            return false;
            break;
        default:
            assert (false);
        }
        break;

    case filesEqual:
        //return false;  commented out: ONLY directories that have not been sync'ed should appear on the results grid
        //else: folders that HAVE been synced are missing... this does not seem to be consistent either
        break;
    case rightFileNewer:
    case leftFileNewer:
    case filesDifferent:
    default:
        assert (false);
    }
    return (true);
}


wxString FreeFileSync::formatFilesizeToShortString(const mpz_class& filesize)
{
    const char* DigitsSeparator = _(".");

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

    string temp;
    if (unit == " Byte")        //no decimal places in case of bytes
    {
        temp = nrOfBytes.get_str(exponent, 10, 3);
        if (!temp.length()) //if nrOfBytes is zero GMP returns an empty string
            temp = "0";
    }
    else
    {
        temp = string(nrOfBytes.get_str(exponent, 10, 3) + "000").substr(0, 3); //returns mantisse of length 3 in all cases

        if (exponent < 0 || exponent > 3)
            temp = _("Error");
        else
            switch (exponent)
            {
            case 0:
                temp = string("0") + DigitsSeparator + temp.substr(0, 2); //shorten mantisse as a 0 will be inserted
                break;
            case 1:
            case 2:
                temp.insert(exponent, DigitsSeparator);
                break;
            case 3:
                break;
            }
    }
    temp+= unit;
    return temp.c_str();
}


