// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) ZenJu (zenju AT gmx DOT de) - All Rights Reserved        *
// **************************************************************************

#ifndef SOFT_FILTER_H_INCLUDED
#define SOFT_FILTER_H_INCLUDED

#include <algorithm>
#include <limits>
#include "../structures.h"
#include <wx/stopwatch.h>

namespace zen
{
/*
Semantics of SoftFilter:
1. It potentially may match only one side => it MUST NOT be applied while traversing a single folder to avoid mismatches
2. => it is applied after traversing and just marks rows, (NO deletions after comparison are allowed)
3. => equivalent to a user temporarily (de-)selecting rows -> not relevant for <Automatic>-mode! ;)
*/

class SoftFilter
{
public:
    SoftFilter(size_t timeSpan, UnitTime unitTimeSpan,
               size_t sizeMin,  UnitSize unitSizeMin,
               size_t sizeMax,  UnitSize unitSizeMax);

    bool matchTime(Int64 writeTime) const { return timeFrom_ <= writeTime; }
    bool matchSize(UInt64 fileSize) const { return sizeMin_ <= fileSize && fileSize <= sizeMax_; }
    bool matchFolder() const { return timeFrom_ == std::numeric_limits<Int64>::min(); }
    //if date filter is active we deactivate all folders: effectively gets rid of empty folders!

    bool isNull() const; //filter is equivalent to NullFilter, but may be technically slower

    //small helper method: merge two soft filters
    friend SoftFilter combineFilters(const SoftFilter& first, const SoftFilter& second);

private:
    SoftFilter(const Int64& timeFrom,
               const UInt64& sizeMin,
               const UInt64& sizeMax);

    Int64  timeFrom_; //unit: UTC, seconds
    UInt64 sizeMin_; //unit: bytes
    UInt64 sizeMax_; //unit: bytes
};
}





















// ----------------------- implementation -----------------------
namespace zen
{
inline
SoftFilter::SoftFilter(size_t timeSpan, UnitTime unitTimeSpan,
                       size_t sizeMin,  UnitSize unitSizeMin,
                       size_t sizeMax,  UnitSize unitSizeMax)
{
    resolveUnits(timeSpan, unitTimeSpan,
                 sizeMin, unitSizeMin,
                 sizeMax, unitSizeMax,
                 timeFrom_,
                 sizeMin_,
                 sizeMax_);
}

inline
SoftFilter::SoftFilter(const Int64& timeFrom,
                       const UInt64& sizeMin,
                       const UInt64& sizeMax) :
    timeFrom_(timeFrom),
    sizeMin_ (sizeMin),
    sizeMax_ (sizeMax)  {}

inline
SoftFilter combineFilters(const SoftFilter& first, const SoftFilter& second)
{
    return SoftFilter(std::max(first.timeFrom_, second.timeFrom_),
                      std::max(first.sizeMin_,  second.sizeMin_),
                      std::min(first.sizeMax_,  second.sizeMax_));
}

inline
bool SoftFilter::isNull() const //filter is equivalent to NullFilter, but may be technically slower
{
    return timeFrom_ == std::numeric_limits<Int64>::min() &&
           sizeMin_  == 0U &&
           sizeMax_  == std::numeric_limits<UInt64>::max();
}
}

#endif // SOFT_FILTER_H_INCLUDED
