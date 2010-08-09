// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) 2008-2010 ZenJu (zhnmju123 AT gmx.de)                    *
// **************************************************************************
//
#ifndef ASSERTSTATIC_H_INCLUDED
#define ASSERTSTATIC_H_INCLUDED

//compile time assert based on Loki (http://loki-lib.sourceforge.net)

#ifdef NDEBUG

#define assert_static(x)	((void)0)

#else /* debugging enabled */
namespace StaticCheckImpl
{
template<int>
struct CompileTimeError;

template<>
struct CompileTimeError<true> {};
}

#define LOKI_CONCAT( X, Y ) LOKI_CONCAT_SUB( X, Y )
#define LOKI_CONCAT_SUB( X, Y ) X##Y

#define assert_static(expr)  \
       enum { LOKI_CONCAT(loki_enum_dummy_value, __LINE__) = sizeof(StaticCheckImpl::CompileTimeError<static_cast<bool>(expr) >) }

/*#define assert_static(expr) \
 	{ Loki::CompileTimeError<((expr) != 0)> Static_Assert_Has_Failed; (void)Static_Assert_Has_Failed; } */

#endif

#endif //ASSERTSTATIC_H_INCLUDED
