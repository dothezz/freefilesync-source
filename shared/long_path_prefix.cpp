// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) 2008-2011 ZenJu (zhnmju123 AT gmx.de)                    *
// **************************************************************************
//
#include "long_path_prefix.h"
#include <wx/msw/wrapwin.h> //includes "windows.h"


//there are two flavors of long path prefix: one for UNC paths, one for regular paths
const Zstring LONG_PATH_PREFIX = Zstr("\\\\?\\");
const Zstring LONG_PATH_PREFIX_UNC = Zstr("\\\\?\\UNC");

template <size_t max_path>
inline
Zstring applyLongPathPrefixImpl(const Zstring& path)
{
    if (path.length() >= max_path &&    //maximum allowed path length without prefix is (MAX_PATH - 1)
        !path.StartsWith(LONG_PATH_PREFIX))
    {
        if (path.StartsWith(Zstr("\\\\"))) //UNC-name, e.g. \\zenju-pc\Users
            return LONG_PATH_PREFIX_UNC + path.AfterFirst(Zchar('\\')); //convert to \\?\UNC\zenju-pc\Users
        else
            return LONG_PATH_PREFIX + path; //prepend \\?\ prefix
    }

    //fallback
    return path;
}


Zstring ffs3::applyLongPathPrefix(const Zstring& path)
{
    return applyLongPathPrefixImpl<MAX_PATH>(path);
}


Zstring ffs3::applyLongPathPrefixCreateDir(const Zstring& path) //throw()
{
    //special rule for ::CreateDirectoryEx(): MAX_PATH - 12(=^ 8.3 filename) is threshold
    return applyLongPathPrefixImpl<MAX_PATH - 12>(path);
}


Zstring ffs3::removeLongPathPrefix(const Zstring& path) //throw()
{
    if (path.StartsWith(LONG_PATH_PREFIX))
    {
        if (path.StartsWith(LONG_PATH_PREFIX_UNC)) //UNC-name
            return Zstring(path).Replace(LONG_PATH_PREFIX_UNC, Zstr("\\"), false);
        else
            return Zstring(path).Replace(LONG_PATH_PREFIX, Zstr(""), false);
    }

    //fallback
    return path;
}

