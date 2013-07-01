// **************************************************************************
// * This file is part of the zen::Xml project. It is distributed under the *
// * Boost Software License: http://www.boost.org/LICENSE_1_0.txt           *
// * Copyright (C) Zenju (zenju AT gmx DOT de) - All Rights Reserved        *
// **************************************************************************

#ifndef ASSERTSTATIC_H_INCLUDED
#define ASSERTSTATIC_H_INCLUDED

/*
//compile time assert based on Loki (http://loki-lib.sourceforge.net)

#ifdef NDEBUG

#define assert_static(x)	//((void)0) -> leads to error when seen in namespace scope!

#else // debugging enabled
namespace static_check_impl
{
template<int>
struct CompileTimeError;

template<>
struct CompileTimeError<true> {};
}

#define LOKI_CONCAT(X, Y) LOKI_CONCAT_SUB(X, Y)
#define LOKI_CONCAT_SUB(X, Y) X ## Y

#define assert_static(expr)  \
    enum { LOKI_CONCAT(loki_enum_dummy_value, __LINE__) = sizeof(StaticCheckImpl::CompileTimeError<static_cast<bool>(expr) >) }

// #define assert_static(expr) \
// { Loki::CompileTimeError<((expr) != 0)> Static_Assert_Has_Failed; (void)Static_Assert_Has_Failed; }

#endif
*/

//C++11: please get rid of this pointless string literal requirement!
#define assert_static(X)  \
    static_assert(X, "Static assert has failed!");

#endif //ASSERTSTATIC_H_INCLUDED
