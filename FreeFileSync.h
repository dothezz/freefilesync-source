#ifndef FREEFILESYNC_H_INCLUDED
#define FREEFILESYNC_H_INCLUDED

#include <wx/string.h>
#include <set>
#include <vector>
#include <wx/dir.h>
#include <windows.h>
#include "library/gmp/include/gmpxx.h"
#include <wx/log.h>

using namespace std;

enum SyncDirection
{
    SyncDirLeft,
    SyncDirRight,
    SyncDirNone
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
    wxString fileSize;
    wxString lastWriteTime;
    FILETIME lastWriteTimeUTC;
} FileInfo;

enum ObjectType
{
    IsNothing,
    IsDirectory,
    IsFile
};

struct FileDescrLine
{
    FileDescrLine() : objType(IsNothing) {};

    wxString filename;  // == directory + relFilename
    wxString directory; //directory to be synced
    wxString relFilename; //filename without directory that is being synchronized
    wxString lastWriteTime;
    FILETIME lastWriteTimeUTC;
    wxString fileSize;
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
    FileOnLeftSideOnly,
    FileOnRightSideOnly,
    RightFileNewer,
    LeftFileNewer,
    FilesDifferent,
    FilesEqual
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
    CompareByMD5,
    CompareByTimeAndSize
};

//interface for status updates (can be implemented by UI or commandline)
//overwrite virtual methods for respective functionality
class StatusUpdater
{
public:
    StatusUpdater() : abortionRequested(false) {}
    virtual ~StatusUpdater() {}

    //these three methods have to be implemented in the derived classes to handle error and status information
    virtual void updateStatus(const wxString& text) = 0;
    virtual void updateProgressIndicator(double number) = 0;
    virtual int  reportError(const wxString& text) = 0;

    //this method is triggered repeatedly and can be used to refresh the ui by dispatching pending events
    virtual void triggerUI_Refresh() {}

    void requestAbortion() //opportunity to abort must be implemented in the three virtual status and error methods (for example in triggerUI_Refresh())
    {                      //currently used by the UI status information screen, when button "Abort is pressed"
        abortionRequested = true;
    }
    static const int Continue = -1;
    static const int Retry = -2;

protected:
    bool abortionRequested;
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

typedef WINSHELLAPI int (*DLLFUNC)(LPSHFILEOPSTRUCT lpFileOp);


class FreeFileSync
{
public:
    FreeFileSync();
    ~FreeFileSync();

    friend class MainDialog;
    friend class GetAllFilesFull;
    friend class CopyThread;

    //main function for compare
    static void getModelDiff(FileCompareResult& output, const wxString& dirLeft, const wxString& dirRight, CompareVariant cmpVar, StatusUpdater* updateClass);

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

    static mpz_class calcTotalBytesToTransfer(const FileCompareResult& fileCmpResult, const SyncConfiguration& config);
    static void swapGrids(FileCompareResult& grid);

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
    void copyfileMultithreaded(const wxString& source, const wxString& target, StatusUpdater* updateClass);


    static void getBytesToTransfer(mpz_t& result, const FileCompareLine& fileCmpLine, const SyncConfiguration& config);

    static void generateFileAndFolderDescriptions(DirectoryDescrType& output, const wxString& directory, StatusUpdater* updateClass = 0);

    static void getFileInformation(FileInfo& output, const wxString& filename);

    static wxString calculateMD5Hash(const wxString& filename);

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
