// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) ZenJu (zhnmju123 AT gmx DOT de) - All Rights Reserved    *
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
//version overview: http://msdn.microsoft.com/en-us/library/ms724834(VS.85).aspx

//2000        is version 5.0
//XP          is version 5.1
//Server 2003 is version 5.2
//Vista       is version 6.0
//Seven       is version 6.1

namespace impl
{
inline
bool winXyOrLater(DWORD major, DWORD minor)
{
    OSVERSIONINFO osvi = {};
    osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
    if (::GetVersionEx(&osvi)) //38 ns per call! (yes, that's nano!) -> we do NOT miss C++11 thread safe statics right now...
        return osvi.dwMajorVersion > major ||
               (osvi.dwMajorVersion == major && osvi.dwMinorVersion >= minor);
    return false;
}
}

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
