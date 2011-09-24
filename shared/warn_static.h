// **************************************************************************
// * This file is part of the zenXML project. It is distributed under the   *
// * Boost Software License, Version 1.0. See accompanying file             *
// * LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt.       *
// * Copyright (C) 2011 ZenJu (zhnmju123 AT gmx.de)                         *
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
#define LOKI_CONCAT( X, Y ) LOKI_CONCAT_SUB( X, Y )
#define LOKI_CONCAT_SUB( X, Y ) X##Y

#define warn_static(TXT) \
    typedef int STATIC_WARNING __attribute__ ((deprecated)); \
    enum { LOKI_CONCAT(warn_static_dummy_value, __LINE__) = sizeof(STATIC_WARNING) };
#endif


#endif //WARN_STATIC_HEADER_08724567834560832745