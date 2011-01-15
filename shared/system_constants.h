// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) 2008-2011 ZenJu (zhnmju123 AT gmx.de)                    *
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
const Zchar FILE_NAME_SEPARATOR = '\\';
const wxChar LINE_BREAK[] = wxT("\r\n"); //internal linkage
#elif defined FFS_LINUX
const Zchar FILE_NAME_SEPARATOR = '/';
const wxChar LINE_BREAK[] = wxT("\n");
#endif

const char BYTE_ORDER_MARK_UTF8[] = "\xEF\xBB\xBF";
}


#endif // SYSTEMCONSTANTS_H_INCLUDED
