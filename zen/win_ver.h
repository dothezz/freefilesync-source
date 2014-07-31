// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) Zenju (zenju AT gmx DOT de) - All Rights Reserved        *
// **************************************************************************

#ifndef WINDOWS_VERSION_HEADER_238470348254325
#define WINDOWS_VERSION_HEADER_238470348254325

#include <cstdint>
#include "win.h" //includes "windows.h"

namespace zen
{
std::uint64_t getOsVersion();
std::uint64_t toBigOsNumber(DWORD high, DWORD low);

//version overview: http://msdn.microsoft.com/en-us/library/ms724834(VS.85).aspx
const std::uint64_t osVersionWin81         = toBigOsNumber(6, 3);
const std::uint64_t osVersionWin8          = toBigOsNumber(6, 2);
const std::uint64_t osVersionWin7          = toBigOsNumber(6, 1);
const std::uint64_t osVersionWinVista      = toBigOsNumber(6, 0);
const std::uint64_t osVersionWinServer2003 = toBigOsNumber(5, 2);
const std::uint64_t osVersionWinXp         = toBigOsNumber(5, 1);

inline bool win81OrLater        () { return getOsVersion() >= osVersionWin81;         }
inline bool win8OrLater         () { return getOsVersion() >= osVersionWin8;          }
inline bool win7OrLater         () { return getOsVersion() >= osVersionWin7;          }
inline bool vistaOrLater        () { return getOsVersion() >= osVersionWinVista;      }
inline bool winServer2003orLater() { return getOsVersion() >= osVersionWinServer2003; }
inline bool winXpOrLater        () { return getOsVersion() >= osVersionWinXp;         }








//######################### implementation #########################
inline
std::uint64_t toBigOsNumber(DWORD high, DWORD low)
{
    ULARGE_INTEGER tmp = {};
    tmp.HighPart = high;
    tmp.LowPart  = low;

    static_assert(sizeof(tmp) == sizeof(std::uint64_t), "");
    return tmp.QuadPart;
}


inline
std::uint64_t getOsVersion()
{
    OSVERSIONINFO osvi = {};
    osvi.dwOSVersionInfoSize = sizeof(osvi);
    if (!::GetVersionEx(&osvi)) //38 ns per call! (yes, that's nano!) -> we do NOT miss C++11 thread safe statics right now...
        return 0;
    return toBigOsNumber(osvi.dwMajorVersion, osvi.dwMinorVersion);
}
}

#endif //WINDOWS_VERSION_HEADER_238470348254325
