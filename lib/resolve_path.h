// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) 2008-2011 ZenJu (zhnmju123 AT gmx.de)                    *
// **************************************************************************

#ifndef RESOLVE_PATH_H_INCLUDED
#define RESOLVE_PATH_H_INCLUDED

#include <zen/zstring.h>


namespace zen
{
Zstring getFormattedDirectoryName(const Zstring& dirString); //throw() - non-blocking! no I/O!

#ifdef FFS_WIN
std::vector<Zstring> getDirectoryAliases(const Zstring& dirString);

//this call may block if network is not reachable or when showing login prompt!
void loginNetworkShare(const Zstring& dirname, bool allowUserInteraction); //throw() - user interaction: show OS password prompt
#endif
}


#endif // RESOLVE_PATH_H_INCLUDED
