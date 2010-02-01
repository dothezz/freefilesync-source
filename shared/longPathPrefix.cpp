#include "longPathPrefix.h"
#include <boost/scoped_array.hpp>
#include <wx/msw/wrapwin.h> //includes "windows.h"
#include "fileError.h"
#include "systemFunctions.h"
#include "stringConv.h"
#include <wx/intl.h>

namespace
{
Zstring getFullPathName(const Zstring& relativeName, size_t proposedBufferSize = 1000)
{
    using namespace FreeFileSync;

    boost::scoped_array<DefaultChar> fullPath(new DefaultChar[proposedBufferSize]);
    const DWORD rv = ::GetFullPathName(
                         relativeName.c_str(), //__in   LPCTSTR lpFileName,
                         proposedBufferSize,   //__in   DWORD nBufferLength,
                         fullPath.get(),       //__out  LPTSTR lpBuffer,
                         NULL);                //__out  LPTSTR *lpFilePart
    if (rv == 0 || rv == proposedBufferSize)
        throw FileError(wxString(_("Error resolving full path name:")) + wxT("\n\"") + zToWx(relativeName) + wxT("\"") +
                        wxT("\n\n") + FreeFileSync::getLastErrorFormatted());
    if (rv > proposedBufferSize)
        return getFullPathName(relativeName, rv);

    return fullPath.get();
}
}


Zstring FreeFileSync::resolveRelativePath(const Zstring& path) //throw()
{
    try
    {
        return getFullPathName(path);
    }
    catch (...)
    {
        return path;
    }
}


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


Zstring FreeFileSync::applyLongPathPrefix(const Zstring& path)
{
    return applyLongPathPrefixImpl<MAX_PATH>(path);
}


Zstring FreeFileSync::applyLongPathPrefixCreateDir(const Zstring& path) //throw()
{
    //special rule for ::CreateDirectoryEx(): MAX_PATH - 12(=^ 8.3 filename) is threshold
    return applyLongPathPrefixImpl<MAX_PATH - 12>(path);
}


Zstring FreeFileSync::removeLongPathPrefix(const Zstring& path) //throw()
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

