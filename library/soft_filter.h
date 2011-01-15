// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) 2008-2011 ZenJu (zhnmju123 AT gmx.de)                    *
// **************************************************************************
//
#ifndef SOFTFILTER_H_INCLUDED
#define SOFTFILTER_H_INCLUDED

#include "../file_hierarchy.h"
#include <wx/timer.h>
/*
Semantics of SoftFilter:
1. It potentially may match only one side => it MUST NOT be applied while traversing a single folder to avoid mismatches
2. => it is applied after traversing and just marks rows, (NO deletions after comparison are allowed)
3. => not relevant for <Automatic>-mode! ;)

-> SoftFilter is equivalent to a user temporarily (de-)selecting rows
*/

namespace ffs3
{

class SoftFilter
{
public:
    SoftFilter(size_t timeWindow) :
        timeWindow_(timeWindow),
        currentTime(wxGetUTCTime()) {}

//    typedef boost::shared_ptr<const SoftFilter> FilterRef; //always bound by design!

    bool passFilter(const FileMapping& fileMap) const;
    bool passFilter(const DirMapping& dirMap) const;

private:
    const size_t timeWindow_; //point in time from "now" (in seconds) for oldest modification date to be allowed
    const long currentTime;   //number of seconds since GMT 00:00:00 Jan 1st 1970.
};


//SoftFilter::FilterRef combineFilters(const SoftFilter& first,
//                                     const SoftFilter& second);
//
//
//
//

















//---------------Inline Implementation---------------------------------------------------
inline
bool SoftFilter::passFilter(const FileMapping& fileMap) const
{
    return (!fileMap.isEmpty<LEFT_SIDE>() &&
            currentTime <= fileMap.getLastWriteTime<LEFT_SIDE>()  + timeWindow_) ||
           (!fileMap.isEmpty<RIGHT_SIDE>() &&
            currentTime <= fileMap.getLastWriteTime<RIGHT_SIDE>() + timeWindow_);
}


inline
bool SoftFilter::passFilter(const DirMapping& dirMap) const
{
    return false;
}
}

#endif // SOFTFILTER_H_INCLUDED
