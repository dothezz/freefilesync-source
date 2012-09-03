// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) ZenJu (zenju AT gmx DOT de) - All Rights Reserved        *
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
std::wstring toGuiString(NumberType number); //format integer number including thousands separator

std::wstring utcToLocalTimeString(Int64 utcTime);





















//--------------- inline impelementation -------------------------------------------
namespace ffs_Impl
{
std::wstring includeNumberSeparator(const std::wstring& number);
}

template <class NumberType> inline
std::wstring toGuiString(NumberType number)
{
    //assert_static(IsInteger<NumberType>::value); -> doesn't work for UInt64
    return ffs_Impl::includeNumberSeparator(zen::numberTo<std::wstring>(number));
}
}

#endif // UTIL_H_INCLUDED
