// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) ZenJu (zhnmju123 AT gmx DOT de) - All Rights Reserved    *
// **************************************************************************

#ifndef FILETRAVERSER_H_INCLUDED
#define FILETRAVERSER_H_INCLUDED

#include <memory>
#include "zstring.h"
#include "int64.h"
#include "file_id_def.h"


//advanced file traverser returning metadata and hierarchical information on files and directories

namespace zen
{
class TraverseCallback
{
public:
    virtual ~TraverseCallback() {}

    struct FileInfo
    {
        UInt64 fileSize;        //unit: bytes!
        Int64 lastWriteTimeRaw; //number of seconds since Jan. 1st 1970 UTC
        FileId id;              //optional: may be initial!
    };

    struct SymlinkInfo
    {
        Int64   lastWriteTimeRaw; //number of seconds since Jan. 1st 1970 UTC
        Zstring targetPath;       //may be empty if something goes wrong
        bool    dirLink;          //"true": point to dir; "false": point to file (or broken Link on Linux)
    };

    enum HandleError
    {
        TRAV_ERROR_RETRY,
        TRAV_ERROR_IGNORE
    };

    //overwrite these virtual methods
    virtual std::shared_ptr<TraverseCallback>  //nullptr: ignore directory, non-nullptr: traverse into
    /**/                onDir    (const Zchar* shortName, const Zstring& fullName) = 0;
    virtual void        onFile   (const Zchar* shortName, const Zstring& fullName, const FileInfo&    details) = 0;
    virtual void        onSymlink(const Zchar* shortName, const Zstring& fullName, const SymlinkInfo& details) = 0;
    virtual HandleError onError  (const std::wstring& errorText) = 0;
};


#ifdef FFS_WIN
struct DstHackCallback
{
    virtual ~DstHackCallback() {}
    virtual void requestUiRefresh(const Zstring& filename) = 0; //applying DST hack imposes significant one-time performance drawback => callback to inform user
};
#else
struct DstHackCallback; //DST hack not required on Linux
#endif

//custom traverser with detail information about files
//directory may end with PATH_SEPARATOR
void traverseFolder(const Zstring& directory, //throw();
                    bool followSymlinks,
                    TraverseCallback& sink,
                    DstHackCallback* dstCallback = NULL); //apply DST hack if callback is supplied
//followSymlinks:
//"true":  Symlinks dereferenced and reported via onFile() and onDir() => onSymlink not used!
//"false": Symlinks directly reported via onSymlink(), directory symlinks are not followed
}

#endif // FILETRAVERSER_H_INCLUDED
