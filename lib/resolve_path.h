// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) Zenju (zenju AT gmx DOT de) - All Rights Reserved        *
// **************************************************************************

#ifndef RESOLVE_PATH_H_INCLUDED
#define RESOLVE_PATH_H_INCLUDED

#include <vector>
#include <zen/zstring.h>

namespace zen
{
/*
FULL directory format:
	- expand macros
	- expand volume path by name
	- convert relative paths into absolute
	- trim whitespace and append file name separator
*/
Zstring getFormattedDirectoryName(const Zstring& dirString); //throw() - non-blocking! no I/O! not thread-safe!!!(see ::GetFullPathName())

//macro substitution only
Zstring expandMacros(const Zstring& text);

std::vector<Zstring> getDirectoryAliases(const Zstring& dirString);

#ifdef FFS_WIN
//*blocks* if network is not reachable or when showing login prompt dialog!
void loginNetworkShare(const Zstring& dirname, bool allowUserInteraction); //throw() - user interaction: show OS password prompt
#endif
}


#endif // RESOLVE_PATH_H_INCLUDED
