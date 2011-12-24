// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) 2008-2011 ZenJu (zhnmju123 AT gmx.de)                    *
// **************************************************************************

#ifndef DEPRECATE_HEADER_2348970348
#define DEPRECATE_HEADER_2348970348

//compiler macros: http://predef.sourceforge.net/precomp.html
#ifdef __GNUC__
#define ZEN_DEPRECATE __attribute__ ((deprecated))

#elif defined _MSC_VER
#define ZEN_DEPRECATE __declspec(deprecated)

#else
#error add your platform here!
#endif

#endif //DEPRECATE_HEADER_2348970348
