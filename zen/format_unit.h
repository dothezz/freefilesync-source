// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) Zenju (zenju AT gmx DOT de) - All Rights Reserved        *
// **************************************************************************

#ifndef FMT_UNIT_8702184019487324
#define FMT_UNIT_8702184019487324

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

std::wstring utcToLocalTimeString(Int64 utcTime); //like Windows Explorer would...





















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

#endif
