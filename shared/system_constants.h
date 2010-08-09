// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) 2008-2010 ZenJu (zhnmju123 AT gmx.de)                    *
// **************************************************************************
//
#ifndef SYSTEMCONSTANTS_H_INCLUDED
#define SYSTEMCONSTANTS_H_INCLUDED

#include "zstring.h"
#include <wx/string.h>

namespace common
{
//------------------------------------------------
//      GLOBALS
//------------------------------------------------
#ifdef FFS_WIN
const DefaultChar FILE_NAME_SEPARATOR = '\\';
static const wxChar* const LINE_BREAK = wxT("\r\n"); //internal linkage
#elif defined FFS_LINUX
const DefaultChar FILE_NAME_SEPARATOR = '/';
static const wxChar* const LINE_BREAK = wxT("\n");
#endif
}


#endif // SYSTEMCONSTANTS_H_INCLUDED
