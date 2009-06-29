#ifndef RECYCLER_H_INCLUDED
#define RECYCLER_H_INCLUDED

#include <wx/dir.h>
#include "zstring.h"
#include "fileError.h"


namespace FreeFileSync
{
    struct FileInfo
    {
        wxULongLong fileSize;        //unit: bytes!
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


    class CopyFileCallback //callback functionality
    {
    public:
        virtual ~CopyFileCallback() {}

        enum Response
        {
            CONTINUE,
            CANCEL
        };
        virtual Response updateCopyStatus(const wxULongLong& totalBytesTransferred) = 0;
    };

    class ShadowCopy;
#ifdef FFS_WIN
    void copyFile(const Zstring& sourceFile,
                  const Zstring& targetFile,
                  const bool copyFileSymLinks,
                  ShadowCopy* shadowCopyHandler = NULL, //supply handler for making shadow copies
                  CopyFileCallback* callback = NULL);

#elif defined FFS_LINUX
    void copyFile(const Zstring& sourceFile,
                                const Zstring& targetFile,
                                const bool copyFileSymLinks,
                                CopyFileCallback* callback);
#endif
}


#endif // RECYCLER_H_INCLUDED
