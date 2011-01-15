// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) 2008-2011 ZenJu (zhnmju123 AT gmx.de)                    *
// **************************************************************************
//
#ifndef BUILDINFO_H_INCLUDED
#define BUILDINFO_H_INCLUDED

namespace util
{
//determine build info
//seems to be safer than checking for _WIN64 (defined on windows for 64-bit compilations only) while _WIN32 is always defined (even for x64 compiler!)
static const bool is32BitBuild = sizeof(void*) == 4;
static const bool is64BitBuild = sizeof(void*) == 8;
}

#endif // BUILDINFO_H_INCLUDED
