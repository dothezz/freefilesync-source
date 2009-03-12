#ifndef RECYCLER_H_INCLUDED
#define RECYCLER_H_INCLUDED

#include "globalFunctions.h"
#include <wx/dir.h>
#include "zstring.h"

class FileError //Exception class used to notify file/directory copy/delete errors
{
public:
    FileError(const Zstring& message) :
            errorMessage(message) {}

    Zstring show() const
    {
        return errorMessage;
    }

private:
    Zstring errorMessage;
};


namespace FreeFileSync
{
    struct FileInfo
    {
        wxULongLong fileSize;    //unit: bytes!
        time_t lastWriteTimeRaw; //number of seconds since Jan. 1st 1970 UTC
    };

    //traverser interface
    class FullDetailFileTraverser
    {
    public:
        virtual ~FullDetailFileTraverser() {}
        virtual wxDirTraverseResult OnFile(const Zstring& filename, const FileInfo& details) = 0;
        virtual wxDirTraverseResult OnDir(const Zstring& dirname) = 0;
        virtual wxDirTraverseResult OnError(const Zstring& errorText) = 0;
    };

    //custom traverser with detail information about files
    void traverseInDetail(const Zstring& directory, const bool traverseSymbolicLinks, FullDetailFileTraverser* sink);
    void getAllFilesAndDirs(const Zstring& sourceDir, std::vector<Zstring>& files, std::vector<Zstring>& directories) throw(FileError);

    //recycler
    bool recycleBinExists(); //test existence of Recycle Bin API on current system

    //file handling
    void removeDirectory(const Zstring& directory, const bool useRecycleBin);
    void removeFile(const Zstring& filename, const bool useRecycleBin);
    void createDirectory(const Zstring& directory, const int level = 0); //level is used internally only
    void copyFolderAttributes(const Zstring& source, const Zstring& target);

#ifdef FFS_WIN
    bool isFatDrive(const Zstring& directoryName);
#endif  //FFS_WIN
}


#endif // RECYCLER_H_INCLUDED
