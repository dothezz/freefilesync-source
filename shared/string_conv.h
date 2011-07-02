// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) 2008-2011 ZenJu (zhnmju123 AT gmx.de)                    *
// **************************************************************************
//
#ifndef STRINGCONV_H_INCLUDED
#define STRINGCONV_H_INCLUDED

#include <string_utf8.h>
#include <wx/string.h>
#include "zstring.h"

namespace zen
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
    return cvrtString<wxString>(str);
#elif defined FFS_LINUX
    return utf8ToWide<wxString>(str);
#endif
}


inline
wxString zToWx(const Zchar* str)
{
#ifdef FFS_WIN
    return cvrtString<wxString>(str);
#elif defined FFS_LINUX
    return utf8ToWide<wxString>(str);
#endif
}


inline
wxString zToWx(Zchar ch)
{
#ifdef FFS_WIN
    return cvrtString<wxString>(ch);
#elif defined FFS_LINUX
    return utf8ToWide<wxString>(ch);
#endif
}

//-----------------------------------------------------------------
inline
Zstring wxToZ(const wxString& str)
{
#ifdef FFS_WIN
    return cvrtString<Zstring>(str);
#elif defined FFS_LINUX
    return wideToUtf8<Zstring>(str);
#endif
}


inline
Zstring wxToZ(const wxChar* str)
{
#ifdef FFS_WIN
    return cvrtString<Zstring>(str);
#elif defined FFS_LINUX
    return wideToUtf8<Zstring>(str);
#endif
}


inline
Zstring wxToZ(wxChar ch)
{
#ifdef FFS_WIN
    return cvrtString<Zstring>(ch);
#elif defined FFS_LINUX
    return wideToUtf8<Zstring>(ch);
#endif
}
}

#endif // STRINGCONV_H_INCLUDED
