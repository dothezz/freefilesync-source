// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) Zenju (zenju AT gmx DOT de) - All Rights Reserved        *
// **************************************************************************

#ifndef NORM_FILTER_H_INCLUDED
#define NORM_FILTER_H_INCLUDED

#include "hard_filter.h"
#include "soft_filter.h"

namespace zen
{
struct NormalizedFilter //grade-a filter: global/local filter settings combined, units resolved, ready for use
{
    NormalizedFilter(const HardFilter::FilterRef& hf, const SoftFilter& sf) : nameFilter(hf), timeSizeFilter(sf) {}

    //"hard" filter: relevant during comparison, physically skips files
    HardFilter::FilterRef nameFilter;
    //"soft" filter: relevant after comparison; equivalent to user selection
    SoftFilter timeSizeFilter;
};


//combine global and local filters via "logical and"
NormalizedFilter normalizeFilters(const FilterConfig& global, const FilterConfig& local);

inline
bool isNullFilter(const FilterConfig& filterCfg)
{
    return NameFilter::isNull(filterCfg.includeFilter, filterCfg.excludeFilter) &&
           SoftFilter(filterCfg.timeSpan, filterCfg.unitTimeSpan,
                      filterCfg.sizeMin,  filterCfg.unitSizeMin,
                      filterCfg.sizeMax,  filterCfg.unitSizeMax).isNull();
}




























// ----------------------- implementation -----------------------
inline
NormalizedFilter normalizeFilters(const FilterConfig& global, const FilterConfig& local)
{
    HardFilter::FilterRef globalName = std::make_shared<NameFilter>(global.includeFilter, global.excludeFilter);
    HardFilter::FilterRef localName  = std::make_shared<NameFilter>(local .includeFilter, local .excludeFilter);

    SoftFilter globalTimeSize(global.timeSpan, global.unitTimeSpan,
                              global.sizeMin,  global.unitSizeMin,
                              global.sizeMax,  global.unitSizeMax);

    SoftFilter localTimeSize(local.timeSpan, local.unitTimeSpan,
                             local.sizeMin,  local.unitSizeMin,
                             local.sizeMax,  local.unitSizeMax);

    return NormalizedFilter(combineFilters(globalName,     localName),
                            combineFilters(globalTimeSize, localTimeSize));
}
}

#endif //NORM_FILTER_H_INCLUDED
