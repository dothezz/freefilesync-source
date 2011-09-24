// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) 2008-2011 ZenJu (zhnmju123 AT gmx.de)                    *
// **************************************************************************

#ifndef LONGPATHPREFIX_H_INCLUDED
#define LONGPATHPREFIX_H_INCLUDED

#include "zstring.h"

#ifndef FFS_WIN
#error use in windows build only!
#endif

namespace zen
{
//handle filenames longer-equal 260 (== MAX_PATH) characters by applying \\?\-prefix (Reference: http://msdn.microsoft.com/en-us/library/aa365247(VS.85).aspx#maxpath)
/*
1. path must be absolute
2. if path is smaller than MAX_PATH nothing is changed!
3. path may already contain \\?\-prefix
*/
Zstring applyLongPathPrefix(const Zstring& path); //throw()
Zstring applyLongPathPrefixCreateDir(const Zstring& path); //throw() -> special rule for ::CreateDirectoryEx(): MAX_PATH - 12(=^ 8.3 filename) is threshold

Zstring removeLongPathPrefix(const Zstring& path); //throw()
}






















//################## implementation ##################

//there are two flavors of long path prefix: one for UNC paths, one for regular paths
const Zstring LONG_PATH_PREFIX = Zstr("\\\\?\\");
const Zstring LONG_PATH_PREFIX_UNC = Zstr("\\\\?\\UNC");

template <size_t max_path>
inline
Zstring applyLongPathPrefixImpl(const Zstring& path)
{
    assert(!path.empty()); //nicely check almost all WinAPI accesses!
    if (path.length() >= max_path &&    //maximum allowed path length without prefix is (MAX_PATH - 1)
        !path.StartsWith(LONG_PATH_PREFIX))
    {
        if (path.StartsWith(Zstr("\\\\"))) //UNC-name, e.g. \\zenju-pc\Users
            return LONG_PATH_PREFIX_UNC + path.AfterFirst(Zchar('\\')); //convert to \\?\UNC\zenju-pc\Users
        else
            return LONG_PATH_PREFIX + path; //prepend \\?\ prefix
    }

    return path; //fallback
}


inline
Zstring zen::applyLongPathPrefix(const Zstring& path)
{
    return applyLongPathPrefixImpl<MAX_PATH>(path);
}


inline
Zstring zen::applyLongPathPrefixCreateDir(const Zstring& path) //throw()
{
    //special rule for ::CreateDirectoryEx(): MAX_PATH - 12(=^ 8.3 filename) is threshold
    return applyLongPathPrefixImpl < MAX_PATH - 12 > (path);
}


inline
Zstring zen::removeLongPathPrefix(const Zstring& path) //throw()
{
    if (path.StartsWith(LONG_PATH_PREFIX))
    {
        if (path.StartsWith(LONG_PATH_PREFIX_UNC)) //UNC-name
            return Zstring(path).Replace(LONG_PATH_PREFIX_UNC, Zstr("\\"), false);
        else
            return Zstring(path).Replace(LONG_PATH_PREFIX, Zstr(""), false);
    }

    return path; //fallback
}

#endif //LONGPATHPREFIX_H_INCLUDED
