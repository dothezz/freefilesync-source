// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) 2008-2010 ZenJu (zhnmju123 AT gmx.de)                    *
// **************************************************************************
//
#ifndef ISNULLFILTER_H_INCLUDED
#define ISNULLFILTER_H_INCLUDED

#include "../structures.h"
#include "../library/filter.h"

namespace FreeFileSync
{

inline
bool isNullFilter(const FilterConfig& filterCfg)
{
     return NameFilter(filterCfg.includeFilter, filterCfg.excludeFilter).isNull();
}

}

#endif // ISNULLFILTER_H_INCLUDED
