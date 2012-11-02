// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) Zenju (zenju AT gmx DOT de) - All Rights Reserved        *
// **************************************************************************

#ifndef DEBUGNEW_H_INCLUDED
#define DEBUGNEW_H_INCLUDED

#ifndef _MSC_VER
#error currently for use with MSC only
#endif

/*
Better std::bad_alloc
---------------------
overwrite "operator new" to automatically write mini dump and get info about bytes requested

1. Compile "debug_new.cpp"

Minidumps http://msdn.microsoft.com/en-us/library/windows/desktop/ee416349(v=vs.85).aspx
----------------------------------------------------------------------------------------
1. Compile "debug_new.cpp"
2. Compile "release" build with:
	- C/C++ -> General: Debug Information Format: "Program Database" (/Zi).
	- C/C++ -> Optimization: Omit Frame Pointers: No (/Oy-) - avoid call stack mess up!
	- Linker -> Debugging: Generate Debug Info: Yes (/DEBUG)
	- Linker -> Optimization: References: Yes (/OPT:REF).
	- Linker -> Optimization: Enable COMDAT Folding: Yes (/OPT:ICF).
Optional:
	- C/C++ -> Optimization: Disabled (/Od)
*/

namespace debug_tools
{
void writeMinidump();
}

#endif // DEBUGNEW_H_INCLUDED
