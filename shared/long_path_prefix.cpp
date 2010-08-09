// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) 2008-2010 ZenJu (zhnmju123 AT gmx.de)                    *
// **************************************************************************
//
#include "long_path_prefix.h"
#include <wx/msw/wrapwin.h> //includes "windows.h"


//there are two flavors of long path prefix: one for UNC paths, one for regular paths
const Zstring LONG_PATH_PREFIX = DefaultStr("\\\\?\\");
const Zstring LONG_PATH_PREFIX_UNC = DefaultStr("\\\\?\\UNC");

template <size_t max_path>
inline
Zstring applyLongPathPrefixImpl(const Zstring& path)
{
    if (    path.length() >= max_path &&    //maximum allowed path length without prefix is (MAX_PATH - 1)
            !path.StartsWith(LONG_PATH_PREFIX))
    {
        if (path.StartsWith(DefaultStr("\\\\"))) //UNC-name, e.g. \\zenju-pc\Users
            return LONG_PATH_PREFIX_UNC + path.AfterFirst(DefaultChar('\\')); //convert to \\?\UNC\zenju-pc\Users
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
        Zstring finalPath = path;
        if (path.StartsWith(LONG_PATH_PREFIX_UNC)) //UNC-name
            finalPath.Replace(LONG_PATH_PREFIX_UNC, DefaultStr("\\"), false);
        else
            finalPath.Replace(LONG_PATH_PREFIX, DefaultStr(""), false);
        return finalPath;
    }

    //fallback
    return path;
}

