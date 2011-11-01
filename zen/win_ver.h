// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) 2008-2011 ZenJu (zhnmju123 AT gmx.de)                    *
// **************************************************************************

#ifndef WINDOWS_VERSION_HEADER_238470348254325
#define WINDOWS_VERSION_HEADER_238470348254325

#include "win.h"

namespace zen
{
bool winXpOrLater();
bool winServer2003orLater();
bool vistaOrLater();
bool win7OrLater();


















//######################### implementation #########################
namespace impl
{
inline
bool winXyOrLater(DWORD major, DWORD minor) //migrate: hold version data as static variable, as soon as C++11 thread safe statics are available in VS
{
    OSVERSIONINFO osvi = {};
    osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
    if (::GetVersionEx(&osvi))
        return osvi.dwMajorVersion > major ||
               (osvi.dwMajorVersion == major && osvi.dwMinorVersion >= minor);
    return false;
}
}

//version overview: http://msdn.microsoft.com/en-us/library/ms724834(VS.85).aspx

//2000        is version 5.0
//XP          is version 5.1
//Server 2003 is version 5.2
//Vista       is version 6.0
//Seven       is version 6.1

inline
bool winXpOrLater() { return impl::winXyOrLater(5, 1); }

inline
bool winServer2003orLater() { return impl::winXyOrLater(5, 2); }

inline
bool vistaOrLater() { return impl::winXyOrLater(6, 0); }

inline
bool win7OrLater() { return impl::winXyOrLater(6, 1); }
}

#endif //WINDOWS_VERSION_HEADER_238470348254325
