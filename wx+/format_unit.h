// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) ZenJu (zhnmju123 AT gmx DOT de) - All Rights Reserved    *
// **************************************************************************

#ifndef UTIL_H_INCLUDED
#define UTIL_H_INCLUDED

#include <string>
#include <zen/string_tools.h>
#include <zen/int64.h>

namespace zen
{
std::wstring filesizeToShortString(Int64 filesize);
std::wstring remainingTimeToShortString(double timeInSec);
std::wstring fractionToShortString(double fraction); //within [0, 1]

template <class NumberType>
std::wstring toStringSep(NumberType number); //convert number to std::wstring including thousands separator

std::wstring utcToLocalTimeString(Int64 utcTime); //throw std::runtime_error



























//--------------- inline impelementation -------------------------------------------
namespace ffs_Impl
{
std::wstring includeNumberSeparator(const std::wstring& number);
}

template <class NumberType> inline
std::wstring toStringSep(NumberType number)
{
    return ffs_Impl::includeNumberSeparator(zen::numberTo<std::wstring>(number));
}
}

#endif // UTIL_H_INCLUDED
