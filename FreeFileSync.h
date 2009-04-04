#ifndef FREEFILESYNC_H_INCLUDED
#define FREEFILESYNC_H_INCLUDED

#include <wx/string.h>
#include <set>
#include <vector>
#include "library/fileHandling.h"
#include "library/zstring.h"

namespace FreeFileSync
{
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
        MainConfiguration() :
                compareVar(CMP_BY_TIME_SIZE),
                filterIsActive(false),        //do not filter by default
                includeFilter(wxT("*")),      //include all files/folders
                excludeFilter(wxEmptyString), //exclude nothing
                useRecycleBin(FreeFileSync::recycleBinExists()) {} //enable if OS supports it; else user will have to activate first and then get an error message

        //Compare setting
        CompareVariant compareVar;

        //Synchronisation settings
        SyncConfiguration syncConfiguration;

        //Filter setting
        bool filterIsActive;
        wxString includeFilter;
        wxString excludeFilter;

        //misc options
        bool useRecycleBin; //use Recycle bin when deleting or overwriting files while synchronizing
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

        Zstring fullName;  // == directory + relativeName
        Zstring directory; //directory to be synced + separator
        Zsubstr relativeName; //fullName without directory that is being synchronized
        //Note on performance: Keep redundant information "directory" and "relativeName"!
        //Extracting info from "fullName" instead would result in noticeable performance loss, with only limited memory reduction (note ref. counting strings)!
        wxLongLong lastWriteTimeRaw; //number of seconds since Jan. 1st 1970 UTC, same semantics like time_t (== signed long)
        wxULongLong fileSize;
        ObjectType objType; //is it a file or directory or initial?

        //the following operators are needed by template class "set"
        //DO NOT CHANGE THESE RELATIONS!!!
        bool operator < (const FileDescrLine& b) const
        {
            //quick check based on string length: we are not interested in a lexicographical order!
            const size_t aLength = relativeName.length();
            const size_t bLength = b.relativeName.length();
            if (aLength != bLength)
                return aLength < bLength;
#ifdef FFS_WIN //Windows does NOT distinguish between upper/lower-case
            return FreeFileSync::compareStringsWin32(relativeName.c_str(), b.relativeName.c_str()) < 0;  //implementing a (reverse) comparison manually is a lot slower!
#elif defined FFS_LINUX //Linux DOES distinguish between upper/lower-case
            return defaultCompare(relativeName.c_str(), b.relativeName.c_str()) < 0;
#endif
        }
    };
    typedef std::vector<FileDescrLine> DirectoryDescrType;


    enum CompareFilesResult
    {
        FILE_LEFT_SIDE_ONLY,
        FILE_RIGHT_SIDE_ONLY,
        FILE_LEFT_NEWER,
        FILE_RIGHT_NEWER,
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
    typedef std::vector<FileCompareLine> FileCompareResult;


    typedef int GridViewLine;
    typedef std::vector<GridViewLine> GridView;  //vector of references to lines in FileCompareResult


    struct FolderPair
    {
        Zstring leftDirectory;
        Zstring rightDirectory;
    };


    class AbortThisProcess  //Exception class used to abort the "compare" and "sync" process
    {
    public:
        AbortThisProcess() {}
        ~AbortThisProcess() {}
    };

    const wxString LAST_CONFIG_FILE = wxT("LastRun.ffs_gui");
    const wxString GLOBAL_CONFIG_FILE = wxT("GlobalSettings.xml");
}

#endif // FREEFILESYNC_H_INCLUDED
