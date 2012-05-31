// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) ZenJu (zhnmju123 AT gmx DOT de) - All Rights Reserved    *
// **************************************************************************

#ifndef PARALLEL_SCAN_H_INCLUDED
#define PARALLEL_SCAN_H_INCLUDED

#include <map>
#include <set>
#include "hard_filter.h"
#include "../structures.h"
#include "../file_hierarchy.h"

namespace zen
{
struct DirectoryKey
{
    DirectoryKey(const Zstring& dirnameFull,
                 const HardFilter::FilterRef& filter,
                 SymLinkHandling handleSymlinks) :
        dirnameFull_(dirnameFull),
        filter_(filter),
        handleSymlinks_(handleSymlinks) {}

    Zstring dirnameFull_;
    HardFilter::FilterRef filter_; //filter interface: always bound by design!
    SymLinkHandling handleSymlinks_;
};

inline
bool operator<(const DirectoryKey& lhs, const DirectoryKey& rhs)
{
    if (lhs.handleSymlinks_ != rhs.handleSymlinks_)
        return lhs.handleSymlinks_ < rhs.handleSymlinks_;

    if (!EqualFilename()(lhs.dirnameFull_, rhs.dirnameFull_))
        return LessFilename()(lhs.dirnameFull_, rhs.dirnameFull_);

    return *lhs.filter_ < *rhs.filter_;
}


struct DirectoryValue
{
    DirContainer dirCont;
    std::set<Zstring> failedReads; //relative postfixed names of directories that could not be read (empty string for root), e.g. access denied, or temporal network drop
};


class FillBufferCallback
{
public:
    virtual ~FillBufferCallback() {}

    enum HandleError
    {
        ON_ERROR_RETRY,
        ON_ERROR_IGNORE
    };
    virtual HandleError reportError (const std::wstring& errorText) = 0;                 //may throw!
    virtual void        reportStatus(const std::wstring& statusMsg, int itemsTotal) = 0; //
};

//attention: ensure directory filtering is applied later to exclude filtered directories which have been kept as parent folders

void fillBuffer(const std::set<DirectoryKey>& keysToRead, //in
                std::map<DirectoryKey, DirectoryValue>& buf, //out
                FillBufferCallback& callback,
                size_t updateInterval); //unit: [ms]
}

#endif // PARALLEL_SCAN_H_INCLUDED
