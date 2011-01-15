// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) 2008-2011 ZenJu (zhnmju123 AT gmx.de)                    *
// **************************************************************************
//
#ifndef STRINGCONV_H_INCLUDED
#define STRINGCONV_H_INCLUDED

#include <wx/string.h>
#include "zstring.h"

namespace ffs3
{
//conversion from Zstring to wxString
wxString zToWx(const Zstring& str);
wxString zToWx(const Zchar* str);
wxString zToWx(Zchar ch);
//conversion from wxString to Zstring
Zstring wxToZ(const wxString& str);
Zstring wxToZ(const wxChar* str);
Zstring wxToZ(wxChar ch);




























//---------------Inline Implementation---------------------------------------------------
inline
wxString zToWx(const Zstring& str)
{
#ifdef FFS_WIN
    return wxString(str.c_str(), str.length());
#elif defined FFS_LINUX
    return wxString::FromUTF8(str.c_str(), str.length());
#endif
}


inline
wxString zToWx(const Zchar* str)
{
#ifdef FFS_WIN
    return str;
#elif defined FFS_LINUX
    return wxString::FromUTF8(str);
#endif
}


inline
wxString zToWx(Zchar ch)
{
    return zToWx(Zstring(ch));
}

//-----------------------------------------------------------------
inline
Zstring wxToZ(const wxString& str)
{
#ifdef FFS_WIN
    return Zstring(str.c_str(), str.length());
#elif defined FFS_LINUX
    return Zstring(str.ToUTF8());
#endif
}


inline
Zstring wxToZ(const wxChar* str)
{
#ifdef FFS_WIN
    return str;
#elif defined FFS_LINUX
    return Zstring(wxString(str).ToUTF8());
#endif
}


inline
Zstring wxToZ(wxChar ch)
{
    return wxToZ(wxString(ch));
}
}

#endif // STRINGCONV_H_INCLUDED
