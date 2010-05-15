// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) 2008-2010 ZenJu (zhnmju123 AT gmx.de)                    *
// **************************************************************************
//
#ifndef STATICASSERT_H_INCLUDED
#define STATICASSERT_H_INCLUDED

//Reference: Compile-Time Assertions, C/C++ Users Journal November, 2004 (http://pera-software.com/articles/compile-time-assertions.pdf)

#ifdef NDEBUG
//If not debugging, assert does nothing.
#define assert_static(x)	((void)0)

#else /* debugging enabled */

#define assert_static(e) \
do { \
enum { assert_static__ = 1/(static_cast<int>(e)) }; \
} while (0)
#endif

#endif // STATICASSERT_H_INCLUDED
