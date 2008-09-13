#ifndef FREEFILESYNC_H_INCLUDED
#define FREEFILESYNC_H_INCLUDED

#include <wx/string.h>
#include <set>
#include <vector>
#include <wx/dir.h>
#include <windows.h>
#include "library/gmp/include/gmpxx.h"
#include <wx/log.h>
#include "library/multithreading.h"

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


struct FileInfo
{
    wxULongLong fileSize;
    wxString lastWriteTime;
    wxULongLong lastWriteTimeUTC;
};

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
    wxULongLong lastWriteTimeUTC;
    wxULongLong fileSize;
    ObjectType objType; //is it a file or directory or initial?

    //the following operators are needed by template class "set"
    //DO NOT CHANGE THESE RELATIONS!!!

#ifdef FFS_WIN
    //Windows does NOT distinguish between upper/lower-case
    bool operator>(const FileDescrLine& b ) const
    {
        return (relFilename.CmpNoCase(b.relFilename) > 0);
    }
    bool operator<(const FileDescrLine& b) const
    {
        return (relFilename.CmpNoCase(b.relFilename) < 0);
    }
    bool operator==(const FileDescrLine& b) const
    {
        return (relFilename.CmpNoCase(b.relFilename) == 0);
    }
#endif  // FFS_WIN

#ifdef FFS_LINUX
    //Linux DOES distinguish between upper/lower-case
    bool operator>(const FileDescrLine& b ) const
    {
        return (relFilename.Cmp(b.relFilename) > 0);
    }
    bool operator<(const FileDescrLine& b) const
    {
        return (relFilename.Cmp(b.relFilename) < 0);
    }
    bool operator==(const FileDescrLine& b) const
    {
        return (relFilename.Cmp(b.relFilename) == 0);
    }
#endif  // FFS_LINUX
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


class GetAllFilesFull : public wxDirTraverser
{
public:
    GetAllFilesFull(DirectoryDescrType& output, wxString dirThatIsSearched, StatusUpdater* updateClass = 0);

    wxDirTraverseResult OnFile(const wxString& filename);

    wxDirTraverseResult OnDir(const wxString& dirname);

    wxDirTraverseResult OnOpenError(const wxString& openerrorname);

private:
    DirectoryDescrType& m_output;
    wxString directory;
    int prefixLength;
    FileInfo currentFileInfo;
    FileDescrLine fileDescr;
    StatusUpdater* statusUpdater;
};


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
            rowToSkipIndex++;
            if (rowToSkipIndex != rowsToRemove.end())
                rowToSkip = *rowToSkipIndex;
        }
    }
    grid.swap(temp);
}


typedef WINSHELLAPI int (*DLLFUNC)(LPSHFILEOPSTRUCT lpFileOp);

class FreeFileSync
{
public:
    FreeFileSync();
    ~FreeFileSync();

    friend class MainDialog;
    friend class GetAllFilesFull;
    friend class CopyThread;

    //identifiers of different processed
    static const int scanningFilesProcess    = 1;
    static const int calcMD5Process          = 2;
    static const int synchronizeFilesProcess = 3;

    //main function for compare
    static void startCompareProcess(FileCompareResult& output, const wxString& dirLeft, const wxString& dirRight, CompareVariant cmpVar, StatusUpdater* statusUpdater);

    //main function for synchronization
    static void startSynchronizationProcess(FileCompareResult& grid, const SyncConfiguration& config, StatusUpdater* statusUpdater, bool useRecycleBin);

    static bool recycleBinExists();         //test existence of Recycle Bin API
    bool setRecycleBinUsage(bool activate); //enables/disables Recycle Bin usage (but only if usage is possible at all): RV: Setting was successful or not

    static void deleteOnGridAndHD(FileCompareResult& grid, const set<int>& rowsToDelete, StatusUpdater* statusUpdater, bool useRecycleBin);
    static void addSubElements(set<int>& subElements, const FileCompareResult& grid, const FileCompareLine& relevantRow);

    static void filterCurrentGridData(FileCompareResult& currentGridData, const wxString& includeFilter, const wxString& excludeFilter);
    static void removeFilterOnCurrentGridData(FileCompareResult& currentGridData);

    static wxString formatFilesizeToShortString(const mpz_class& filesize);
    static wxString getFormattedDirectoryName(const wxString& dirname);

    static void calcTotalBytesToSync(int& objectsTotal, double& dataTotal, const FileCompareResult& fileCmpResult, const SyncConfiguration& config);

    static void swapGrids(FileCompareResult& grid);

    static void wxULongLongToMpz(mpz_t& output, const wxULongLong& input);

    static string calculateMD5Hash(const wxString& filename);
    static string calculateMD5HashMultithreaded(const wxString& filename, StatusUpdater* updateClass);

    static bool isFFS_ConfigFile(const wxString& filename);

    static const wxString FFS_ConfigFileID;
    static const wxString FFS_LastConfigFile;

private:
    bool synchronizeFile(const FileCompareLine& filename, const SyncConfiguration& config, StatusUpdater* statusUpdater);   // false if nothing was to be done
    bool synchronizeFolder(const FileCompareLine& filename, const SyncConfiguration& config, StatusUpdater* statusUpdater); // false if nothing was to be done

    //file copy functionality -> keep instance-bound to to be able to prevent wxWidgets-logging
    void removeDirectory(const wxString& directory);
    void removeFile(const wxString& filename);
    void copyOverwriting(const wxString& source, const wxString& target);
    void copyfile(const wxString& source, const wxString& target);
    void copyCreatingDirs(const wxString& source, const wxString& target);
    void createDirectory(const wxString& directory, int level = 0); //level is used internally only
    //some special file functions
    void moveToRecycleBin(const wxString& filename);
    static void copyfileMultithreaded(const wxString& source, const wxString& target, StatusUpdater* updateClass);

    static void generateFileAndFolderDescriptions(DirectoryDescrType& output, const wxString& directory, StatusUpdater* updateClass = 0);

    static void getFileInformation(FileInfo& output, const wxString& filename);

    bool recycleBinAvailable;
    wxLogNull* noWxLogs;
    HINSTANCE hinstShell;
    DLLFUNC fileOperation;
};


class FileError //Exception class used to notify file/directory copy/delete errors
{
public:
    FileError(const wxString& txt) : errorMessage(txt) {}

    wxString show()
    {
        return errorMessage;
    }

private:
    wxString errorMessage;
};


class AbortThisProcess  //Exception class used to abort the "compare" and "sync" process
{
public:
    AbortThisProcess() {}
    ~AbortThisProcess() {}
};

#endif // FREEFILESYNC_H_INCLUDED
