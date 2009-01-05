#include "fileHandling.h"
#include <wx/intl.h>
#include <wx/msgdlg.h>
#include "resources.h"

#ifdef FFS_WIN
#include <windows.h>
#endif  // FFS_WIN


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

    bool moveToRecycleBin(const wxString& filename)
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
bool moveToRecycleBin(const wxString& filename) throw(RuntimeException)
{
    return recyclerInstance.moveToRecycleBin(filename);
}


inline
void FreeFileSync::removeFile(const wxString& filename, const bool useRecycleBin)
{
    if (!wxFileExists(filename)) return; //this is NOT an error situation: the manual deletion relies on it!

    if (useRecycleBin)
    {
        if (!moveToRecycleBin(filename))
            throw FileError(wxString(_("Error moving to recycle bin: ")) + wxT("\"") + filename + wxT("\""));
        return;
    }

#ifdef FFS_WIN
    if (!SetFileAttributes(
                filename.c_str(),     //address of filename
                FILE_ATTRIBUTE_NORMAL //attributes to set
            )) throw FileError(wxString(_("Error deleting file: ")) + wxT("\"") + filename + wxT("\""));
#endif  //FFS_WIN

    if (!wxRemoveFile(filename))
        throw FileError(wxString(_("Error deleting file: ")) + wxT("\"") + filename + wxT("\""));
}


void FreeFileSync::removeDirectory(const wxString& directory, const bool useRecycleBin)
{
    if (!wxDirExists(directory)) return; //this is NOT an error situation: the manual deletion relies on it!

    if (useRecycleBin)
    {
        if (!moveToRecycleBin(directory))
            throw FileError(wxString(_("Error moving to recycle bin: ")) + wxT("\"") + directory + wxT("\""));
        return;
    }

    wxArrayString fileList;
    wxArrayString dirList;

    try
    {
        //should be executed in own scope so that directory access does not disturb deletion!
        getAllFilesAndDirs(directory, fileList, dirList);
    }
    catch (const FileError& e)
    {
        throw FileError(wxString(_("Error deleting directory: ")) + wxT("\"") + directory + wxT("\""));
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
                )) throw FileError(wxString(_("Error deleting directory: ")) + wxT("\"") + dirList[j] + wxT("\""));
#endif  // FFS_WIN

        if (!wxRmdir(dirList[j]))
            throw FileError(wxString(_("Error deleting directory: ")) + wxT("\"") + dirList[j] + wxT("\""));
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
    wxString createFirstDir = wxDir(directory).GetName().BeforeLast(GlobalResources::FILE_NAME_SEPARATOR);

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


class GetAllFilesSimple : public wxDirTraverser
{
public:
    GetAllFilesSimple(wxArrayString& files, wxArrayString& subDirectories) :
            m_files(files),
            m_dirs(subDirectories) {}


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


void FreeFileSync::getAllFilesAndDirs(const wxString& sourceDir, wxArrayString& files, wxArrayString& directories) throw(FileError)
{
    files.Clear();
    directories.Clear();

    //get all files and directories from current directory (and subdirectories)
    wxDir dir(sourceDir);
    GetAllFilesSimple traverser(files, directories);

    if (dir.Traverse(traverser) == (size_t)-1)
        throw FileError(wxString(_("Error traversing directory ")) + wxT("\"") + sourceDir + wxT("\""));
}


