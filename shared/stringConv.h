// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) 2008-2010 ZenJu (zhnmju123 AT gmx.de)                    *
// **************************************************************************
//
#ifndef STRINGCONV_H_INCLUDED
#define STRINGCONV_H_INCLUDED

#include <wx/string.h>
#include "zstring.h"

namespace FreeFileSync
{
//conversion from Zstring to wxString
wxString zToWx(const Zstring& str);
wxString zToWx(const DefaultChar* str);
wxString zToWx(DefaultChar ch);
//conversion from wxString to Zstring
Zstring wxToZ(const wxString& str);
Zstring wxToZ(const wxChar* str);
Zstring wxToZ(wxChar ch);




























//---------------Inline Implementation---------------------------------------------------
inline
wxString zToWx(const Zstring& str)
{
#ifdef ZSTRING_CHAR
    return wxString::FromUTF8(str.c_str(), str.length());
#elif defined ZSTRING_WIDE_CHAR
    return wxString(str.c_str(), str.length());
#endif
}


inline
wxString zToWx(const DefaultChar* str)
{
#ifdef ZSTRING_CHAR
    return wxString::FromUTF8(str);
#elif defined ZSTRING_WIDE_CHAR
    return str;
#endif
}


inline
wxString zToWx(DefaultChar ch)
{
    return zToWx(Zstring() + ch);
}

//-----------------------------------------------------------------
inline
Zstring wxToZ(const wxString& str)
{
#ifdef ZSTRING_CHAR
    return Zstring(str.ToUTF8());
#elif defined ZSTRING_WIDE_CHAR
    return Zstring(str.c_str(), str.length());
#endif
}


inline
Zstring wxToZ(const wxChar* str)
{
#ifdef ZSTRING_CHAR
    return Zstring(wxString(str).ToUTF8());
#elif defined ZSTRING_WIDE_CHAR
    return str;
#endif
}


inline
Zstring wxToZ(wxChar ch)
{
    return wxToZ(wxString(ch));
}
}

#endif // STRINGCONV_H_INCLUDED
