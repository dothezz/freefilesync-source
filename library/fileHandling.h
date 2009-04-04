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
        wxLongLong lastWriteTimeRaw; //number of seconds since Jan. 1st 1970 UTC
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
    void traverseInDetail(const Zstring& directory, const bool traverseDirectorySymlinks, FullDetailFileTraverser* sink);

    bool fileExists(const Zstring& filename); //replaces wxFileExists()!

    //recycler
    bool recycleBinExists(); //test existence of Recycle Bin API on current system

    //file handling
    void removeDirectory(const Zstring& directory, const bool useRecycleBin);
    void removeFile(const Zstring& filename, const bool useRecycleBin);
    void createDirectory(const Zstring& directory, const Zstring& templateDir, const bool copyDirectorySymLinks);
#ifdef FFS_LINUX
    //callback function for status updates whily copying
    typedef void (*CopyFileCallback)(const wxULongLong& totalBytesTransferred, void* data);

    void copyFile(const Zstring& sourceFile,
                  const Zstring& targetFile,
                  const bool copyFileSymLinks,
                  CopyFileCallback callback = NULL,
                  void* data = NULL);
#endif
}


#endif // RECYCLER_H_INCLUDED
