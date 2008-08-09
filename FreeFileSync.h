#ifndef FREEFILESYNC_H_INCLUDED
#define FREEFILESYNC_H_INCLUDED

#include <wx/string.h>
#include <set>
#include <vector>
#include <wx/dir.h>
#include <windows.h>
#include "library\GMP\include\gmpxx.h"

using namespace std;

enum SyncDirection
{
    syncDirLeft,
    syncDirRight,
    syncDirNone
};

struct SyncConfiguration
{
    SyncDirection exLeftSideOnly;
    SyncDirection exRightSideOnly;
    SyncDirection leftNewer;
    SyncDirection rightNewer;
    SyncDirection different;
};

typedef struct
{
    string fileSize;
    string lastWriteTime;
    FILETIME lastWriteTimeUTC;
} FileInfo;

enum ObjectType
{
    isNothing,
    isDirectory,
    isFile
};

struct FileDescrLine
{
    FileDescrLine() : objType(isNothing) {};

    wxString filename;  // == directory + relFilename
    wxString directory; //directory to be synced
    wxString relFilename; //filename without directory that is being synchronized
    wxString lastWriteTime;
    FILETIME lastWriteTimeUTC;
    wxString fileSize;
    ObjectType objType; //is it a file or directory or initial?

    //the following operators are needed by template class "set"
    //DO NOT CHANGE THESE RELATIONS!!!
    bool operator>(const FileDescrLine& b ) const
    {   //make lowercase! Windows does NOT distinguish upper/lower-case
        return (wxString(relFilename).MakeLower() > wxString(b.relFilename).MakeLower());
    }
    bool operator<(const FileDescrLine& b) const
    {   //make lowercase! Windows does NOT distinguish upper/lower-case
        return (wxString(relFilename).MakeLower() < wxString(b.relFilename).MakeLower());
    }
    bool operator==(const FileDescrLine& b) const
    {   //make lowercase! Windows does NOT distinguish upper/lower-case
        return (wxString(relFilename).MakeLower() == wxString(b.relFilename).MakeLower());
    }
};
typedef set<FileDescrLine> DirectoryDescrType;


enum CompareFilesResult
{
    fileOnLeftSideOnly,
    fileOnRightSideOnly,
    rightFileNewer,
    leftFileNewer,
    filesDifferent,
    filesEqual
};

struct FileCompareLine
{
    FileCompareLine() : selectedForSynchronization(true) {}

    FileDescrLine fileDescrLeft;
    FileDescrLine fileDescrRight;

    CompareFilesResult cmpResult;
    bool selectedForSynchronization;
};
typedef vector<FileCompareLine> FileCompareResult;


enum CompareVariant
{
    compareByMD5,
    compareByTimeAndSize
};


class wxGetAllFiles : public wxDirTraverser
{
public:
    wxGetAllFiles(DirectoryDescrType& output, wxString dirThatIsSearched);

    wxDirTraverseResult OnFile(const wxString& filename);

    wxDirTraverseResult OnDir(const wxString& dirname);

    wxDirTraverseResult OnOpenError(const wxString& openerrorname);

private:
    DirectoryDescrType& m_output;
    wxString directory;
    int prefixLength;
    FileInfo currentFileInfo;
    FileDescrLine fileDescr;
};

typedef WINSHELLAPI int (*DLLFUNC)(LPSHFILEOPSTRUCT lpFileOp);

class FreeFileSync
{
public:
    FreeFileSync();
    ~FreeFileSync();

    friend class wxGetAllFiles;

    //windows file copy functionality
    void moveToRecycleBin(const char* filename);
    static void deleteDirectoryCompletely(const char* directory);
    static void removeFile(const char* filename);
    static void copyOverwriting(const char* source, const char* target);
    static void copyfile(const char* source, const char* target);
    static void copyCreatingDirs(const char* source, const char* target);
    static void createDirectoriesRecursively(const wxString& directory, int level = 0); //level is used internally only

    static wxString formatFilesizeToShortString(const mpz_class& filesize);
    static void getBytesToTransfer(mpz_t& result, const FileCompareLine& fileCmpLine, const SyncConfiguration& config);
    static mpz_class calcTotalBytesToTransfer(const FileCompareResult& fileCmpResult, const SyncConfiguration& config);

    bool synchronizeFile(const FileCompareLine& filename, const SyncConfiguration& config);   // true if successful
    bool synchronizeFolder(const FileCompareLine& filename, const SyncConfiguration& config); // true if successful

    static void swapGrids(FileCompareResult& grid);

    static void getModelDiff(FileCompareResult& output, const wxString& dirLeft, const wxString& dirRight, CompareVariant cmpVar);

private:
    static void generateFileAndFolderDescriptions(DirectoryDescrType& output, const wxString& directory);

    static void getFileInformation(FileInfo& output, const wxString& filename);

    static wxString calculateMD5Hash(const wxString& filename);

    bool SHFileOperationNotFound;
    HINSTANCE hinstShell;
    DLLFUNC fileOperation;
};

class fileError
{
public:
    fileError(string txt) : errorMessage(txt) {}

    string show()
    {
        return errorMessage;
    }

private:
    string errorMessage;
};

#endif // FREEFILESYNC_H_INCLUDED
