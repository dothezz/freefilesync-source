#ifndef FILETRAVERSER_H_INCLUDED
#define FILETRAVERSER_H_INCLUDED

#include "zstring.h"
#include <set>
#include <memory>
#include <wx/longlong.h>

//advanced file traverser returning metadata and hierarchical information on files and directories

namespace FreeFileSync
{
class TraverseCallback
{
public:
    virtual ~TraverseCallback() {}

    enum ReturnValue
    {
        TRAVERSING_STOP,
        TRAVERSING_CONTINUE
    };

    struct FileInfo
    {
        wxULongLong fileSize;        //unit: bytes!
        wxLongLong lastWriteTimeRaw; //number of seconds since Jan. 1st 1970 UTC
    };

    class ReturnValDir
    {
    public:
        //some proxy classes
        class Stop {};
        class Ignore {};
        class Continue {};

        ReturnValDir(const Stop&) : returnCode(TRAVERSING_STOP), subDirCb(NULL) {}
        ReturnValDir(const Ignore&) : returnCode(TRAVERSING_IGNORE_DIR), subDirCb(NULL) {}
        ReturnValDir(const Continue&, TraverseCallback* subDirCallback) : returnCode(TRAVERSING_CONTINUE), subDirCb(subDirCallback) {}


        enum ReturnValueEnh
        {
            TRAVERSING_STOP,
            TRAVERSING_IGNORE_DIR,
            TRAVERSING_CONTINUE
        };

        const ReturnValueEnh returnCode;
        TraverseCallback* const subDirCb;
    };

    //overwrite these virtual methods
    virtual ReturnValue  onError(const wxString& errorText) = 0;
    virtual ReturnValue  onFile(const DefaultChar* shortName, const Zstring& fullName, const FileInfo& details) = 0;
    virtual ReturnValDir onDir(const DefaultChar*  shortName, const Zstring& fullName) = 0;
};

//custom traverser with detail information about files
void traverseFolder(const Zstring& directory, const bool traverseDirectorySymlinks, TraverseCallback* sink); //throw()
}

#endif // FILETRAVERSER_H_INCLUDED
