// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) Zenju (zenju AT gmx DOT de) - All Rights Reserved        *
// **************************************************************************

#ifndef WINDOWS_VERSION_HEADER_238470348254325
#define WINDOWS_VERSION_HEADER_238470348254325

#include <utility>
#include "win.h" //includes "windows.h"

namespace zen
{
	struct OsVersion
	{
		OsVersion() : major(), minor() {}
		OsVersion(DWORD high, DWORD low) : major(high), minor(low) {}

			DWORD major;
			DWORD minor;
	};
	inline bool operator< (const OsVersion& lhs, const OsVersion& rhs) { return lhs.major != rhs.major ? lhs.major < rhs.major : lhs.minor < rhs.minor; }
	inline bool operator==(const OsVersion& lhs, const OsVersion& rhs) { return lhs.major == rhs.major && lhs.minor == rhs.minor; }


//version overview: http://msdn.microsoft.com/en-us/library/ms724834(VS.85).aspx
const OsVersion osVersionWin81        (6, 3);
const OsVersion osVersionWin8         (6, 2);
const OsVersion osVersionWin7         (6, 1);
const OsVersion osVersionWinVista     (6, 0);
const OsVersion osVersionWinServer2003(5, 2);
const OsVersion osVersionWinXp        (5, 1);

/*
	NOTE: there are two basic APIs to check Windows version: (empiric study following)
		GetVersionEx      -> reports version considering compatibility mode (and compatibility setting in app manifest since Windows 8.1)
		VerifyVersionInfo -> always reports *real* Windows Version
*/

//GetVersionEx()-based APIs:
OsVersion getOsVersion();
inline bool win81OrLater        () { using namespace std::rel_ops; return getOsVersion() >= osVersionWin81;         }
inline bool win8OrLater         () { using namespace std::rel_ops; return getOsVersion() >= osVersionWin8;          }
inline bool win7OrLater         () { using namespace std::rel_ops; return getOsVersion() >= osVersionWin7;          }
inline bool vistaOrLater        () { using namespace std::rel_ops; return getOsVersion() >= osVersionWinVista;      }
inline bool winServer2003orLater() { using namespace std::rel_ops; return getOsVersion() >= osVersionWinServer2003; }
inline bool winXpOrLater        () { using namespace std::rel_ops; return getOsVersion() >= osVersionWinXp;         }

//VerifyVersionInfo()-based APIs:
bool isRealOsVersion(const OsVersion& ver);






//######################### implementation #########################
inline
OsVersion getOsVersion()
{
    OSVERSIONINFO osvi = {};
    osvi.dwOSVersionInfoSize = sizeof(osvi);
    if (!::GetVersionEx(&osvi)) //38 ns per call! (yes, that's nano!) -> we do NOT miss C++11 thread-safe statics right now...
	{
		assert(false);
		return OsVersion();
	}
    return OsVersion(osvi.dwMajorVersion, osvi.dwMinorVersion);
}


inline
bool isRealOsVersion(const OsVersion& ver)
{
    OSVERSIONINFOEX verInfo = {};
    verInfo.dwOSVersionInfoSize = sizeof(verInfo);
    verInfo.dwMajorVersion = ver.major;
    verInfo.dwMinorVersion = ver.minor;

	//Syntax: http://msdn.microsoft.com/en-us/library/windows/desktop/ms725491%28v=vs.85%29.aspx
    DWORDLONG conditionMask = 0;
    VER_SET_CONDITION(conditionMask, VER_MAJORVERSION, VER_EQUAL);
    VER_SET_CONDITION(conditionMask, VER_MINORVERSION, VER_EQUAL);

    const bool rv = ::VerifyVersionInfo(&verInfo, VER_MAJORVERSION | VER_MINORVERSION, conditionMask)
           == TRUE; //silence VC "performance warnings"
	assert(rv || GetLastError() == ERROR_OLD_WIN_VERSION);

	return rv;
}
}

#endif //WINDOWS_VERSION_HEADER_238470348254325
