// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) ZenJu (zhnmju123 AT gmx DOT de) - All Rights Reserved    *
// **************************************************************************

#ifndef STANDARDPATHS_H_INCLUDED
#define STANDARDPATHS_H_INCLUDED

#include <wx/stdpaths.h>
#include <zen/zstring.h>
#include <zen/file_handling.h>
#include <wx+/string_conv.h>

namespace zen
{
//------------------------------------------------------------------------------
//global program directories
//------------------------------------------------------------------------------
Zstring getResourceDir(); //resource directory WITH path separator at end
Zstring getConfigDir();   //config directory WITH path separator at end
//------------------------------------------------------------------------------

Zstring getLauncher();    //full path to application launcher C:\...\FreeFileSync.exe
bool isPortableVersion();













//---------------- implementation ----------------
namespace impl
{
inline
const Zstring& getBinaryDir() //directory containing executable WITH path separator at end
{
    static Zstring instance = beforeLast(toZ(wxStandardPaths::Get().GetExecutablePath()), FILE_NAME_SEPARATOR) + Zstring(FILE_NAME_SEPARATOR); //extern linkage!
    return instance;
}

#ifdef FFS_WIN
inline
Zstring getInstallDir() //root install directory WITH path separator at end
{
    return beforeLast(beforeLast(getBinaryDir(), FILE_NAME_SEPARATOR), FILE_NAME_SEPARATOR) + FILE_NAME_SEPARATOR;
}
#endif
}


inline
bool isPortableVersion()
{
#ifdef FFS_WIN
    static const bool isPortable = !fileExists(impl::getInstallDir() + L"uninstall.exe"); //this check is a bit lame...

#elif defined FFS_LINUX
    static const bool isPortable = !endsWith(impl::getBinaryDir(), "/bin/"); //this check is a bit lame...
#endif
    return isPortable;
}


inline
Zstring getResourceDir()
{
#ifdef FFS_WIN
    return impl::getInstallDir();

#elif defined FFS_LINUX
    if (isPortableVersion())
        return impl::getBinaryDir();
    else //use OS' standard paths
    {
        Zstring resourceDir = toZ(wxStandardPathsBase::Get().GetResourcesDir());

        if (!endsWith(resourceDir, FILE_NAME_SEPARATOR))
            resourceDir += FILE_NAME_SEPARATOR;

        return resourceDir;
    }
#endif
}


inline
Zstring getConfigDir()
{
    if (isPortableVersion())
#ifdef FFS_WIN
        return impl::getInstallDir();
#elif defined FFS_LINUX
        //wxString(wxT(".")) + zToWx(FILE_NAME_SEPARATOR) -> don't use current working directory
        //avoid surprises with GlobalSettings.xml being newly created in each working directory
        return impl::getBinaryDir();
#endif
    else //use OS' standard paths
    {
        Zstring userDirectory = toZ(wxStandardPathsBase::Get().GetUserDataDir());

        if (!dirExists(userDirectory))
            try
            {
                createDirectory(userDirectory); //only top directory needs to be created: no recursion necessary
            }
            catch (const FileError&) {}

        if (!endsWith(userDirectory, FILE_NAME_SEPARATOR))
            userDirectory += FILE_NAME_SEPARATOR;

        return userDirectory;
    }
}


inline
Zstring getLauncher()
{
#ifdef FFS_WIN
    return impl::getInstallDir() + Zstr("FreeFileSync.exe");
#elif defined FFS_LINUX
    return impl::getBinaryDir() + Zstr("FreeFileSync");
#endif
}
}

#endif // STANDARDPATHS_H_INCLUDED
