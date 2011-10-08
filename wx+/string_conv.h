// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) 2008-2011 ZenJu (zhnmju123 AT gmx.de)                    *
// **************************************************************************

#ifndef STRINGCONV_H_INCLUDED
#define STRINGCONV_H_INCLUDED

#include <zen/utf8.h>
#include <wx/string.h>
#include <zen/zstring.h>

namespace zen
{
inline wxString operator+(const wxString& lhs, const char*   rhs) { return wxString(lhs) += utf8CvrtTo<wxString>(rhs); }
inline wxString operator+(const wxString& lhs, const Zstring& rhs) { return wxString(lhs) += utf8CvrtTo<wxString>(rhs); }


//conversion between Zstring and wxString
inline wxString toWx(const Zstring&  str) { return utf8CvrtTo<wxString>(str); }
inline Zstring   toZ(const wxString& str) { return utf8CvrtTo<Zstring>(str); }
}

#endif // STRINGCONV_H_INCLUDED
