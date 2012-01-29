// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) ZenJu (zhnmju123 AT gmx DOT de) - All Rights Reserved    *
// **************************************************************************

#ifndef STRINGCONV_H_INCLUDED
#define STRINGCONV_H_INCLUDED

#include <zen/utf8.h>
#include <wx/string.h>
#include <zen/zstring.h>

namespace zen
{
//conversion between Zstring and wxString
inline wxString toWx(const Zstring&  str) { return utf8CvrtTo<wxString>(str); }
inline Zstring   toZ(const wxString& str) { return utf8CvrtTo<Zstring>(str); }
}

#endif // STRINGCONV_H_INCLUDED
