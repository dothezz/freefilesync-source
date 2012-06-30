// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) ZenJu (zhnmju123 AT gmx DOT de) - All Rights Reserved    *
// **************************************************************************

#ifndef LONGPATHPREFIX_H_INCLUDED
#define LONGPATHPREFIX_H_INCLUDED

#include "win.h"
#include "zstring.h"

namespace zen
{
//handle filenames longer-equal 260 (== MAX_PATH) characters by applying \\?\-prefix; see: http://msdn.microsoft.com/en-us/library/aa365247(VS.85).aspx#maxpath
/*
1. path must be absolute
2. if path is smaller than MAX_PATH nothing is changed! caveat: FindFirstFile() "Prepending the string "\\?\" does not allow access to the root directory."
3. path may already contain \\?\-prefix
*/
Zstring applyLongPathPrefix(const Zstring& path); //throw()
Zstring applyLongPathPrefixCreateDir(const Zstring& path); //throw() -> special rule for ::CreateDirectory()/::CreateDirectoryEx(): MAX_PATH - 12(=^ 8.3 filename) is threshold

Zstring removeLongPathPrefix(const Zstring& path); //throw()
}






















//################## implementation ##################

//there are two flavors of long path prefix: one for UNC paths, one for regular paths
const Zstring LONG_PATH_PREFIX     = L"\\\\?\\";
const Zstring LONG_PATH_PREFIX_UNC = L"\\\\?\\UNC";

template <size_t max_path> inline
Zstring applyLongPathPrefixImpl(const Zstring& path)
{
    assert(!path.empty()); //nicely check almost all WinAPI accesses!
    assert(!zen::isWhiteSpace(path[0]));

    if (path.length() >= max_path &&    //maximum allowed path length without prefix is (MAX_PATH - 1)
        !zen::startsWith(path, LONG_PATH_PREFIX))
    {
        if (zen::startsWith(path, L"\\\\")) //UNC-name, e.g. \\zenju-pc\Users
            return LONG_PATH_PREFIX_UNC + zen::afterFirst(path, L'\\'); //convert to \\?\UNC\zenju-pc\Users
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
    return applyLongPathPrefixImpl<MAX_PATH - 12> (path);
}


inline
Zstring zen::removeLongPathPrefix(const Zstring& path) //throw()
{
    if (zen::startsWith(path, LONG_PATH_PREFIX))
    {
        if (zen::startsWith(path, LONG_PATH_PREFIX_UNC)) //UNC-name
            return replaceCpy(path, LONG_PATH_PREFIX_UNC, Zstr("\\"), false);
        else
            return replaceCpy(path, LONG_PATH_PREFIX, Zstr(""), false);
    }
    return path; //fallback
}

#endif //LONGPATHPREFIX_H_INCLUDED
