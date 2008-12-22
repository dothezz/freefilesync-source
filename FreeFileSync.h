#ifndef FREEFILESYNC_H_INCLUDED
#define FREEFILESYNC_H_INCLUDED

#include <wx/string.h>
#include <set>
#include <vector>
#include <wx/dir.h>
#include <wx/log.h>
#include "library/multithreading.h"

using namespace std;


enum CompareVariant
{
    CMP_BY_CONTENT,
    CMP_BY_TIME_SIZE
};

struct SyncConfiguration
{
    SyncConfiguration() :
            exLeftSideOnly(SYNC_DIR_RIGHT),
            exRightSideOnly(SYNC_DIR_RIGHT),
            leftNewer(SYNC_DIR_RIGHT),
            rightNewer(SYNC_DIR_RIGHT),
            different(SYNC_DIR_RIGHT) {}

    enum Direction
    {
        SYNC_DIR_LEFT,
        SYNC_DIR_RIGHT,
        SYNC_DIR_NONE
    };

    Direction exLeftSideOnly;
    Direction exRightSideOnly;
    Direction leftNewer;
    Direction rightNewer;
    Direction different;
};


struct MainConfiguration
{
    MainConfiguration();

    //Compare setting
    CompareVariant compareVar;

    //Synchronisation settings
    SyncConfiguration syncConfiguration;

    //Filter setting
    bool filterIsActive;
    wxString includeFilter;
    wxString excludeFilter;


    //misc options
    bool useRecycleBin;   //use Recycle bin when deleting or overwriting files while synchronizing
    bool continueOnError; //hides error messages during synchronization
};


struct FileDescrLine
{
    FileDescrLine() : objType(TYPE_NOTHING) {}

    enum ObjectType
    {
        TYPE_NOTHING,
        TYPE_DIRECTORY,
        TYPE_FILE
    };

    wxString fullName;  // == directory + relativeName
    wxString directory; //directory to be synced
    wxString relativeName; //fullName without directory that is being synchronized
    //Note on performance: Keep redundant information "directory" and "relativeName"! Extracting info from "fullName" results in noticeable performance loss!
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
        return (relativeName.CmpNoCase(b.relativeName) > 0);
    }
    bool operator<(const FileDescrLine& b) const
    {
        return (relativeName.CmpNoCase(b.relativeName) < 0);
    }
    bool operator==(const FileDescrLine& b) const
    {
        return (relativeName.CmpNoCase(b.relativeName) == 0);
    }

#elif defined FFS_LINUX
    //Linux DOES distinguish between upper/lower-case
    bool operator>(const FileDescrLine& b ) const
    {
        return (relativeName.Cmp(b.relativeName) > 0);
    }
    bool operator<(const FileDescrLine& b) const
    {
        return (relativeName.Cmp(b.relativeName) < 0);
    }
    bool operator==(const FileDescrLine& b) const
    {
        return (relativeName.Cmp(b.relativeName) == 0);
    }
#else
    adapt this
#endif
};
typedef set<FileDescrLine> DirectoryDescrType;


enum CompareFilesResult
{
    FILE_LEFT_SIDE_ONLY,
    FILE_RIGHT_SIDE_ONLY,
    FILE_RIGHT_NEWER,
    FILE_LEFT_NEWER,
    FILE_DIFFERENT,
    FILE_EQUAL,

    FILE_UNDEFINED
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
typedef vector<GridViewLine> GridView;  //vector of references to lines in FileCompareResult


struct FolderPair
{
    wxString leftDirectory;
    wxString rightDirectory;
};


class RecycleBin;

namespace FreeFileSync
{
    //main functions for compare
    bool foldersAreValidForComparison(const vector<FolderPair>& folderPairs, wxString& errorMessage);
    void startCompareProcess(const vector<FolderPair>& directoryPairsFormatted, const CompareVariant cmpVar, FileCompareResult& output, StatusHandler* statusUpdater);

    //main function for synchronization
    void startSynchronizationProcess(FileCompareResult& grid, const SyncConfiguration& config, StatusHandler* statusUpdater, const bool useRecycleBin);

    bool recycleBinExists(); //test existence of Recycle Bin API on current system

    void deleteOnGridAndHD(FileCompareResult& grid, const set<int>& rowsToDelete, ErrorHandler* errorHandler, const bool useRecycleBin);
    void addSubElements(set<int>& subElements, const FileCompareResult& grid, const FileCompareLine& relevantRow);

    void filterCurrentGridData(FileCompareResult& currentGridData, const wxString& includeFilter, const wxString& excludeFilter);
    void removeFilterOnCurrentGridData(FileCompareResult& currentGridData);

    wxString formatFilesizeToShortString(const wxULongLong& filesize);
    wxString formatFilesizeToShortString(const double filesize);
    wxString getFormattedDirectoryName(const wxString& dirname);

    void calcTotalBytesToSync(int& objectsToCreate,
                              int& objectsToOverwrite,
                              int& objectsToDelete,
                              double& dataToProcess,
                              const FileCompareResult& fileCmpResult,
                              const SyncConfiguration& config);

    void swapGrids(FileCompareResult& grid);

    void adjustModificationTimes(const wxString& parentDirectory, const int timeInSeconds, ErrorHandler* errorHandler);

    const wxString FfsLastConfigFile = wxT("LastRun.ffs_gui");
    const wxString FfsGlobalSettingsFile = wxT("GlobalSettings.xml");


//+++++++++++++++++++SUBROUTINES++++++++++++++++++++++++++
    //create comparison result table and fill relation except for files existing on both sides
    void performBaseComparison(const vector<FolderPair>& directoryPairsFormatted,
                               FileCompareResult& output,
                               StatusHandler* statusUpdater);

    bool synchronizeFile(const FileCompareLine& cmpLine, const SyncConfiguration& config, const bool useRecycleBin, StatusHandler* statusUpdater);   // false if nothing had to be done
    bool synchronizeFolder(const FileCompareLine& cmpLine, const SyncConfiguration& config, const bool useRecycleBin, StatusHandler* statusUpdater); // false if nothing had to be done

    //file functionality
    void removeDirectory(const wxString& directory, const bool useRecycleBin);
    void removeFile(const wxString& filename, const bool useRecycleBin);
    void copyfileMultithreaded(const wxString& source, const wxString& target, StatusHandler* updateClass);
    void createDirectory(const wxString& directory, int level = 0); //level is used internally only

    //misc
    vector<wxString> compoundStringToFilter(const wxString& filterString); //convert compound string, separated by ';' or '\n' into formatted vector of wxStrings

    extern RecycleBin recycler;
}


class FileError //Exception class used to notify file/directory copy/delete errors
{
public:
    FileError(const wxString& txt) : errorMessage(txt) {}

    wxString show() const
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
