#ifndef FREEFILESYNC_H_INCLUDED
#define FREEFILESYNC_H_INCLUDED

#include "library/wxWidgets.h"
#include <wx/string.h>
#include <set>
#include <vector>
#include <wx/dir.h>
#include <wx/log.h>
#include "library/multithreading.h"

using namespace std;

enum SyncDirection
{
    syncDirLeft,
    syncDirRight,
    syncDirNone
};

enum CompareVariant
{
    compareByContent,
    compareByTimeAndSize
};

struct SyncConfiguration
{
    SyncDirection exLeftSideOnly;
    SyncDirection exRightSideOnly;
    SyncDirection leftNewer;
    SyncDirection rightNewer;
    SyncDirection different;
};

struct Configuration
{   //Compare setting
    CompareVariant compareVar;

    //Synchronisation settings
    SyncConfiguration syncConfiguration;

    //Filter setting
    wxString includeFilter;
    wxString excludeFilter;
    bool hideFiltered;
    bool filterIsActive;

    //other options
    bool useRecycleBin;     //use Recycle bin when deleting or overwriting files while synchronizing
    bool continueOnError; //hides error messages during synchronization
};

struct FileInfo
{
    wxULongLong fileSize;
    wxString lastWriteTime;
    wxULongLong lastWriteTimeRaw;
};

enum ObjectType
{
    isNothing,
    isDirectory,
    isFile
};

struct FileDescrLine
{
    FileDescrLine() : objType(isNothing) {}

    wxString filename;  // == directory + relFilename
    wxString directory; //directory to be synced
    wxString relFilename; //filename without directory that is being synchronized
    wxString lastWriteTime;
    wxULongLong lastWriteTimeRaw;
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

#elif defined FFS_LINUX
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
#else
    assert(false);
#endif
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


typedef int GridViewLine;
typedef vector<int> GridView;  //vector of references to lines in FileCompareResult


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

bool updateUI_IsAllowed(); //test if a specific amount of time is over
void updateUI_Now();       //do the updating


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

class RecycleBin;


class FreeFileSync
{
public:
    FreeFileSync(bool useRecycleBin);
    ~FreeFileSync();

    //identifiers of different processes
    static const int scanningFilesProcess      = 1;
    static const int compareFileContentProcess = 2;
    static const int synchronizeFilesProcess   = 3;

    //main function for compare
    static void startCompareProcess(FileCompareResult& output, const wxString& dirLeft, const wxString& dirRight, CompareVariant cmpVar, StatusUpdater* statusUpdater);

    //main function for synchronization
    static void startSynchronizationProcess(FileCompareResult& grid, const SyncConfiguration& config, StatusUpdater* statusUpdater, bool useRecycleBin);

    static bool recycleBinExists(); //test existence of Recycle Bin API on current system

    static void deleteOnGridAndHD(FileCompareResult& grid, const set<int>& rowsToDelete, StatusUpdater* statusUpdater, bool useRecycleBin);
    static void addSubElements(set<int>& subElements, const FileCompareResult& grid, const FileCompareLine& relevantRow);

    static void filterCurrentGridData(FileCompareResult& currentGridData, const wxString& includeFilter, const wxString& excludeFilter);
    static void removeFilterOnCurrentGridData(FileCompareResult& currentGridData);

    static wxString formatFilesizeToShortString(const wxULongLong& filesize);
    static wxString formatFilesizeToShortString(const double filesize);
    static wxString getFormattedDirectoryName(const wxString& dirname);

    static void calcTotalBytesToSync(int& objectsToCreate,
                                     int& objectsToOverwrite,
                                     int& objectsToDelete,
                                     double& dataToProcess,
                                     const FileCompareResult& fileCmpResult,
                                     const SyncConfiguration& config);

    static void swapGrids(FileCompareResult& grid);

    static bool isFFS_ConfigFile(const wxString& filename);

    static const wxString FFS_ConfigFileID;
    static const wxString FFS_LastConfigFile;

    static void getFileInformation(FileInfo& output, const wxString& filename);

private:
    bool synchronizeFile(const FileCompareLine& filename, const SyncConfiguration& config, StatusUpdater* statusUpdater);   // false if nothing had to be done
    bool synchronizeFolder(const FileCompareLine& filename, const SyncConfiguration& config, StatusUpdater* statusUpdater); // false if nothing had to be done

    //file copy functionality -> keep instance-bound to to be able to prevent wxWidgets-logging
    void removeDirectory(const wxString& directory);
    void removeFile(const wxString& filename);
    void copyfileMultithreaded(const wxString& source, const wxString& target, StatusUpdater* updateClass);
    void createDirectory(const wxString& directory, int level = 0); //level is used internally only

    //some special file functions
    static void generateFileAndFolderDescriptions(DirectoryDescrType& output, const wxString& directory, StatusUpdater* updateClass = 0);

    bool recycleBinShouldBeUsed;
    static RecycleBin recycler;
    wxLogNull* noWxLogs;
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
