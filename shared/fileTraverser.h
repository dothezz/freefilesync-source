// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) 2008-2010 ZenJu (zhnmju123 AT gmx.de)                    *
// **************************************************************************
//
#ifndef FILETRAVERSER_H_INCLUDED
#define FILETRAVERSER_H_INCLUDED

#include "zstring.h"
#include <set>
#include <memory>
#include <wx/longlong.h>
#include "loki/TypeManip.h"

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
        enum ReturnValueEnh
        {
            TRAVERSING_DIR_STOP,
            TRAVERSING_DIR_IGNORE,
            TRAVERSING_DIR_CONTINUE
        };

        ReturnValDir(Loki::Int2Type<TRAVERSING_DIR_STOP>) : returnCode(TRAVERSING_DIR_STOP), subDirCb(NULL) {}
        ReturnValDir(Loki::Int2Type<TRAVERSING_DIR_IGNORE>) : returnCode(TRAVERSING_DIR_IGNORE), subDirCb(NULL) {}
        ReturnValDir(Loki::Int2Type<TRAVERSING_DIR_CONTINUE>, TraverseCallback* subDirCallback) : returnCode(TRAVERSING_DIR_CONTINUE), subDirCb(subDirCallback) {}


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
