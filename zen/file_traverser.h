// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) ZenJu (zenju AT gmx DOT de) - All Rights Reserved        *
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
struct TraverseCallback
{
    virtual ~TraverseCallback() {}

    struct FileInfo
    {
        UInt64 fileSize;        //unit: bytes!
        Int64 lastWriteTimeRaw; //number of seconds since Jan. 1st 1970 UTC
        FileId id;              //optional: may be initial!
        //bool isFollowedSymlink;
    };

    struct SymlinkInfo
    {
        Int64   lastWriteTimeRaw; //number of seconds since Jan. 1st 1970 UTC
        Zstring targetPath;       //optional: empty if something goes wrong
        bool    dirLink;          //"true": point to dir; "false": point to file (or broken Link on Linux)
    };

    enum HandleLink
    {
        LINK_FOLLOW, //dereferences link, then calls "onDir()" or "onFile()"
        LINK_SKIP
    };

    enum HandleError
    {
        ON_ERROR_RETRY,
        ON_ERROR_IGNORE
    };

    //overwrite these virtual methods
    virtual std::shared_ptr<TraverseCallback>  //nullptr: ignore directory, non-nullptr: traverse into using the (new) callback
    /**/                onDir    (const Zchar* shortName, const Zstring& fullName) = 0;
    virtual void        onFile   (const Zchar* shortName, const Zstring& fullName, const FileInfo&    details) = 0;
    virtual HandleLink  onSymlink(const Zchar* shortName, const Zstring& fullName, const SymlinkInfo& details) = 0;
    virtual HandleError onError  (const std::wstring& msg) = 0;
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
                    TraverseCallback& sink,
                    DstHackCallback* dstCallback = nullptr); //apply DST hack if callback is supplied
}

#endif // FILETRAVERSER_H_INCLUDED
