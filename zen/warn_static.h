// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) Zenju (zenju AT gmx DOT de) - All Rights Reserved        *
// **************************************************************************

#ifndef WARN_STATIC_HEADER_08724567834560832745
#define WARN_STATIC_HEADER_08724567834560832745

/*
Portable Compile-Time Warning
-----------------------------
Usage:
    warn_static("my message")
*/

#ifdef _MSC_VER
#define MAKE_STRING_SUB(NUM)   #NUM
#define MAKE_STRING(NUM) MAKE_STRING_SUB(NUM)

#define warn_static(TXT) \
    __pragma(message (__FILE__ "(" MAKE_STRING(__LINE__) "): Warning: " ## TXT))

#elif defined __GNUC__
#define ZEN_CONCAT_SUB(X, Y) X ## Y
#define ZEN_CONCAT(X, Y) ZEN_CONCAT_SUB(X, Y)

#define warn_static(TXT) \
    typedef int STATIC_WARNING __attribute__ ((deprecated)); \
    enum { ZEN_CONCAT(warn_static_dummy_value, __LINE__) = sizeof(STATIC_WARNING) };
#endif


#endif //WARN_STATIC_HEADER_08724567834560832745