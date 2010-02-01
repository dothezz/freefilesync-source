#ifndef LONGPATHPREFIX_H_INCLUDED
#define LONGPATHPREFIX_H_INCLUDED

#ifndef FFS_WIN
use in windows build only!
#endif

#include "zstring.h"

namespace FreeFileSync
{

Zstring resolveRelativePath(const Zstring& path); //throw()

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

#endif // LONGPATHPREFIX_H_INCLUDED
