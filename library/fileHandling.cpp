#include "fileHandling.h"
#include <wx/intl.h>
#include <wx/msgdlg.h>
#include "resources.h"

#ifdef FFS_WIN
#include <wx/msw/wrapwin.h> //includes "windows.h"
#endif  // FFS_WIN

inline
bool endsWithPathSeparator(const wxChar* name)
{
    size_t len = wxStrlen(name);
    return len && (name[len - 1] == GlobalResources::FILE_NAME_SEPARATOR);
}


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

    bool moveToRecycleBin(const Zstring& filename)
    {
        if (!recycleBinAvailable)   //this method should ONLY be called if recycle bin is available
            throw RuntimeException(_("Initialization of Recycle Bin failed!"));

#ifdef FFS_WIN
        Zstring filenameDoubleNull = filename + wxChar(0);

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
                           ) != 0 || fileOp.fAnyOperationsAborted) return false;
#else
        assert(false);
#endif

        return true;
    }

private:
    bool recycleBinAvailable;
};

//global instance of recycle bin
RecycleBin recyclerInstance;


bool FreeFileSync::recycleBinExists()
{
    return recyclerInstance.recycleBinExists();
}


inline
bool moveToRecycleBin(const Zstring& filename) throw(RuntimeException)
{
    return recyclerInstance.moveToRecycleBin(filename);
}


inline
void FreeFileSync::removeFile(const Zstring& filename, const bool useRecycleBin)
{
    if (!wxFileExists(filename)) return; //this is NOT an error situation: the manual deletion relies on it!

    if (useRecycleBin)
    {
        if (!moveToRecycleBin(filename))
            throw FileError(wxString(_("Error moving to recycle bin:")) + wxT(" \"") + filename.c_str() + wxT("\""));
        return;
    }

#ifdef FFS_WIN
    if (!SetFileAttributes(
                filename.c_str(),     //address of filename
                FILE_ATTRIBUTE_NORMAL //attributes to set
            )) throw FileError(wxString(_("Error deleting file:")) + wxT(" \"") + filename.c_str() + wxT("\""));
#endif  //FFS_WIN

    if (!wxRemoveFile(filename))
        throw FileError(wxString(_("Error deleting file:")) + wxT(" \"") + filename.c_str() + wxT("\""));
}


void FreeFileSync::removeDirectory(const Zstring& directory, const bool useRecycleBin)
{
    if (!wxDirExists(directory)) return; //this is NOT an error situation: the manual deletion relies on it!

    if (useRecycleBin)
    {
        if (!moveToRecycleBin(directory))
            throw FileError(wxString(_("Error moving to recycle bin:")) + wxT(" \"") + directory.c_str() + wxT("\""));
        return;
    }

    vector<Zstring> fileList;
    vector<Zstring> dirList;

    try
    {   //should be executed in own scope so that directory access does not disturb deletion!
        getAllFilesAndDirs(directory, fileList, dirList);
    }
    catch (const FileError& e)
    {
        throw FileError(wxString(_("Error deleting directory:")) + wxT(" \"") + directory.c_str() + wxT("\""));
    }

    for (unsigned int j = 0; j < fileList.size(); ++j)
        removeFile(fileList[j], false);

    dirList.insert(dirList.begin(), directory);   //this directory will be deleted last

    for (int j = int(dirList.size()) - 1; j >= 0 ; --j)
    {
#ifdef FFS_WIN
        if (!SetFileAttributes(
                    dirList[j].c_str(),     // address of directory name
                    FILE_ATTRIBUTE_NORMAL)) // attributes to set
            throw FileError(wxString(_("Error deleting directory:")) + wxT(" \"") + dirList[j].c_str() + wxT("\""));
#endif  // FFS_WIN

        if (!wxRmdir(dirList[j]))
            throw FileError(wxString(_("Error deleting directory:")) + wxT(" \"") + dirList[j].c_str() + wxT("\""));
    }
}


void FreeFileSync::createDirectory(const Zstring& directory, const int level)
{
    if (wxDirExists(directory))
        return;

    if (level == 50) //catch endless recursion
        return;

    //try to create directory
    if (wxMkdir(directory))
        return;

    //if not successfull try to create parent folders first
    Zstring parentDir;
    if (endsWithPathSeparator(directory.c_str())) //may be valid on first level of recursion at most! Usually never true!
    {
        parentDir = directory.BeforeLast(GlobalResources::FILE_NAME_SEPARATOR);
        parentDir = parentDir.BeforeLast(GlobalResources::FILE_NAME_SEPARATOR);
    }
    else
        parentDir = directory.BeforeLast(GlobalResources::FILE_NAME_SEPARATOR);

    if (parentDir.empty()) return;

    //call function recursively
    createDirectory(parentDir, level + 1);

    //now creation should be possible
    if (!wxMkdir(directory))
    {
        if (level == 0)
            throw FileError(wxString(_("Error creating directory:")) + wxT(" \"") + directory.c_str() + wxT("\""));
    }
}


void FreeFileSync::copyFolderAttributes(const Zstring& source, const Zstring& target)
{
#ifdef FFS_WIN
    DWORD attr = GetFileAttributes(source.c_str()); // address of the name of a file or directory
    if (attr ==  0xFFFFFFFF)
        throw FileError(wxString(_("Error reading file attributes:")) + wxT(" \"") + source.c_str() + wxT("\""));

    if (!SetFileAttributes(
                target.c_str(), // address of filename
                attr))          // address of attributes to set
        throw FileError(wxString(_("Error writing file attributes:")) + wxT(" \"") + target.c_str() + wxT("\""));
#elif defined FFS_LINUX
//Linux doesn't use attributes for files or folders
#endif
}


class GetAllFilesSimple : public wxDirTraverser
{
public:
    GetAllFilesSimple(vector<Zstring>& files, vector<Zstring>& subDirectories) :
            m_files(files),
            m_dirs(subDirectories) {}

    wxDirTraverseResult OnDir(const wxString& dirname)
    {
        m_dirs.push_back(dirname);
        return wxDIR_CONTINUE;
    }

    wxDirTraverseResult OnFile(const wxString& filename)
    {
        m_files.push_back(filename);
        return wxDIR_CONTINUE;
    }

    wxDirTraverseResult OnOpenError(const wxString& openerrorname)
    {
        wxMessageBox(openerrorname, _("Error"));
        return wxDIR_IGNORE;
    }

private:
    vector<Zstring>& m_files;
    vector<Zstring>& m_dirs;
};


void FreeFileSync::getAllFilesAndDirs(const Zstring& sourceDir, vector<Zstring>& files, vector<Zstring>& directories) throw(FileError)
{
    files.clear();
    directories.clear();

    //get all files and directories from current directory (and subdirectories)
    wxDir dir(sourceDir);
    GetAllFilesSimple traverser(files, directories);

    if (dir.Traverse(traverser) == (size_t)-1)
        throw FileError(wxString(_("Error traversing directory:")) + wxT(" \"") + sourceDir.c_str() + wxT("\""));
}


#ifdef FFS_WIN
class CloseOnExit
{
public:
    CloseOnExit(HANDLE searchHandle) : m_searchHandle(searchHandle) {}

    ~CloseOnExit()
    {
        FindClose(m_searchHandle);
    }

private:
    HANDLE m_searchHandle;
};


inline
void getWin32FileInformation(const WIN32_FIND_DATA& input, FreeFileSync::FileInfo& output)
{
    //convert UTC FILETIME to ANSI C format (number of seconds since Jan. 1st 1970 UTC)
    wxULongLong writeTimeLong(input.ftLastWriteTime.dwHighDateTime, input.ftLastWriteTime.dwLowDateTime);
    writeTimeLong /= 10000000;    //reduce precision to 1 second (FILETIME has unit 10^-7 s)
    writeTimeLong -= wxULongLong(2, 3054539008UL);   //timeshift between ansi C time and FILETIME in seconds == 11644473600s
    output.lastWriteTimeRaw = writeTimeLong.GetLo(); //it should fit into a 32bit variable now
    assert(writeTimeLong.GetHi() == 0);

    output.fileSize = wxULongLong(input.nFileSizeHigh, input.nFileSizeLow);
}

#elif defined FFS_LINUX
class EnhancedFileTraverser : public wxDirTraverser
{
public:
    EnhancedFileTraverser(FreeFileSync::FullDetailFileTraverser* sink) : m_sink(sink) {}

    virtual wxDirTraverseResult OnFile(const wxString& filename)
    {
        struct stat fileInfo;
        if (stat(filename.c_str(), &fileInfo) != 0)
            return m_sink->OnError(Zstring(_("Could not retrieve file info for:")) + wxT(" \"") + filename.c_str() + wxT("\""));

        FreeFileSync::FileInfo details;
        details.lastWriteTimeRaw = fileInfo.st_mtime; //UTC time(ANSI C format); unit: 1 second
        details.fileSize         = fileInfo.st_size;

        return m_sink->OnFile(filename, details);
    }

    virtual wxDirTraverseResult OnDir(const wxString& dirname)
    {
        return m_sink->OnDir(dirname);
    }

    virtual wxDirTraverseResult OnOpenError(const wxString& errorText)
    {
        return m_sink->OnError(errorText);
    }

private:
    FreeFileSync::FullDetailFileTraverser* m_sink;
};
#endif


bool FreeFileSync::traverseInDetail(const Zstring& directory, FullDetailFileTraverser* sink, const int level)
{
#ifdef FFS_WIN
    if (level == 50) //catch endless recursion
    {
        if (sink->OnError(Zstring(_("Error traversing directory:")) + wxT(" ") + directory) == wxDIR_STOP)
            return false;
        else
            return true;
    }

    Zstring directoryFormatted = directory;
    if (!endsWithPathSeparator(directoryFormatted.c_str()))
        directoryFormatted += GlobalResources::FILE_NAME_SEPARATOR;

    const Zstring filespec = directoryFormatted + wxT("*.*");

    WIN32_FIND_DATA fileMetaData;
    HANDLE searchHandle = FindFirstFile(filespec.c_str(), //pointer to name of file to search for
                                        &fileMetaData);   //pointer to returned information

    if (searchHandle == INVALID_HANDLE_VALUE)
    {
        if (GetLastError() == ERROR_FILE_NOT_FOUND)
            return true;
        //else: we have a problem...
        if (sink->OnError(Zstring(_("Error traversing directory:")) + wxT(" ") + directoryFormatted) == wxDIR_STOP)
            return false;
        else
            return true;
    }
    CloseOnExit dummy(searchHandle);

    do
    {   //don't return "." and ".."
        const wxChar* name = fileMetaData.cFileName;
        if (    name[0] == wxChar('.') &&
                ((name[1] == wxChar('.') && name[2] == wxChar('\0')) ||
                 name[1] == wxChar('\0')))
            continue;

        const Zstring fullName = directoryFormatted +  name;
        if (fileMetaData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) //a directory...
        {
            switch (sink->OnDir(fullName))
            {
            case wxDIR_IGNORE:
                break;
            case wxDIR_CONTINUE:
                if (!traverseInDetail(fullName, sink, level + 1))
                    return false;
                else
                    break;
            case wxDIR_STOP:
                return false;
            default:
                assert(false);
                break;
            }
        }
        else //a file...
        {
            FileInfo details;
            getWin32FileInformation(fileMetaData, details);

            switch (sink->OnFile(fullName, details))
            {
            case wxDIR_IGNORE:
            case wxDIR_CONTINUE:
                break;
            case wxDIR_STOP:
                return false;
            default:
                assert(false);
                break;
            }
        }
    }
    while (FindNextFile(searchHandle,	 // handle to search
                        &fileMetaData)); // pointer to structure for data on found file

    if (GetLastError() == ERROR_NO_MORE_FILES)
        return true; //everything okay
    else //an error occured
    {
        if (sink->OnError(Zstring(_("Error traversing directory:")) + wxT(" ") + directoryFormatted) == wxDIR_STOP)
            return false;
        else
            return true;
    }
#elif defined FFS_LINUX

    //use standard file traverser and enrich output with additional file information
    //could be improved with own traversing algorithm for optimized performance
    EnhancedFileTraverser traverser(sink);

    wxDir dir(directory);
    if (dir.IsOpened())
        dir.Traverse(traverser);

    return true;
#else
    adapt this
#endif
}







