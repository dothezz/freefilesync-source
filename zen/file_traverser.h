// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) Zenju (zenju AT gmx DOT de) - All Rights Reserved        *
// **************************************************************************

#ifndef FILETRAVERSER_H_INCLUDED
#define FILETRAVERSER_H_INCLUDED

#include "zstring.h"
#include "int64.h"
#include "file_id_def.h"

//advanced file traverser returning metadata and hierarchical information on files and directories

namespace zen
{
struct TraverseCallback
{
    virtual ~TraverseCallback() {}

    struct SymlinkInfo
    {
        Int64 lastWriteTime; //number of seconds since Jan. 1st 1970 UTC
    };

    struct FileInfo
    {
        FileInfo() : symlinkInfo() {}
        UInt64 fileSize;     //unit: bytes!
        Int64 lastWriteTime; //number of seconds since Jan. 1st 1970 UTC
        FileId id;           //optional: initial if not supported!
        const SymlinkInfo* symlinkInfo; //only filled if file is a followed symlink
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

    virtual void              onFile   (const Zchar* shortName, const Zstring& fullName, const FileInfo&    details) = 0;
    virtual HandleLink        onSymlink(const Zchar* shortName, const Zstring& fullName, const SymlinkInfo& details) = 0;
    virtual TraverseCallback* onDir    (const Zchar* shortName, const Zstring& fullName) = 0;
    //nullptr: ignore directory, non-nullptr: traverse into using the (new) callback => implement releaseDirTraverser() if necessary!
    virtual void releaseDirTraverser(TraverseCallback* trav) {}

    virtual HandleError reportDirError (const std::wstring& msg, size_t retryNumber) = 0; //failed directory traversal -> consider directory data as incomplete!
    virtual HandleError reportItemError(const std::wstring& msg, size_t retryNumber, const Zchar* shortName) = 0; //failed to get data for single file/dir/symlink only!
};


#ifdef ZEN_WIN
struct DstHackCallback
{
    virtual ~DstHackCallback() {}
    virtual void requestUiRefresh(const Zstring& filename) = 0; //applying DST hack imposes significant one-time performance drawback => callback to inform user
};
#elif defined ZEN_LINUX || defined ZEN_MAC
struct DstHackCallback; //DST hack not required on Unix
#endif

//custom traverser with detail information about files
//- client needs to handle duplicate file reports! (FilePlusTraverser fallback, retrying to read directory contents, ...)
//- directory may end with PATH_SEPARATOR
void traverseFolder(const Zstring& dirname, //throw()
                    TraverseCallback& sink,
                    DstHackCallback* dstCallback = nullptr); //apply DST hack if callback is supplied
}

#endif // FILETRAVERSER_H_INCLUDED
