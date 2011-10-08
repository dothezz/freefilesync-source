// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) 2008-2011 ZenJu (zhnmju123 AT gmx.de)                    *
// **************************************************************************

#ifndef STANDARDPATHS_H_INCLUDED
#define STANDARDPATHS_H_INCLUDED

#include <wx/stdpaths.h>
#include <zen/zstring.h>
#include <wx+/string_conv.h>

namespace zen
{
//------------------------------------------------------------------------------
//global program directories
//------------------------------------------------------------------------------
wxString getResourceDir(); //resource directory WITH path separator at end
wxString getConfigDir();   //config directory WITH path separator at end
//------------------------------------------------------------------------------

wxString getLauncher();    //full path to application launcher C:\...\FreeFileSync.exe
bool isPortableVersion();












//---------------- implementation ----------------
namespace impl
{
inline
const wxString& getBinaryDir() //directory containing executable WITH path separator at end
{
    static wxString instance = beforeLast(wxStandardPaths::Get().GetExecutablePath(), FILE_NAME_SEPARATOR) + toWx(Zstring(FILE_NAME_SEPARATOR)); //extern linkage!
    return instance;
}

#ifdef FFS_WIN
inline
wxString getInstallDir() //root install directory WITH path separator at end
{
    return getBinaryDir().BeforeLast(FILE_NAME_SEPARATOR).BeforeLast(FILE_NAME_SEPARATOR) + FILE_NAME_SEPARATOR;
}
#endif
}


inline
bool isPortableVersion()
{
#ifdef FFS_WIN
    static const bool isPortable = !wxFileExists(impl::getInstallDir() + wxT("uninstall.exe")); //this check is a bit lame...
#elif defined FFS_LINUX
    static const bool isPortable = !impl::getBinaryDir().EndsWith(wxT("/bin/")); //this check is a bit lame...
#endif
    return isPortable;
}


inline
wxString getResourceDir()
{
#ifdef FFS_WIN
    return impl::getInstallDir();
#elif defined FFS_LINUX
    if (isPortableVersion())
        return impl::getBinaryDir();
    else //use OS' standard paths
    {
        wxString resourceDir = wxStandardPathsBase::Get().GetResourcesDir();

        if (!endsWith(resourceDir, FILE_NAME_SEPARATOR))
            resourceDir += FILE_NAME_SEPARATOR;

        return resourceDir;
    }
#endif
}


inline
wxString getConfigDir()
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
        wxString userDirectory = wxStandardPathsBase::Get().GetUserDataDir();

        if (!wxDirExists(userDirectory))
            ::wxMkdir(userDirectory); //only top directory needs to be created: no recursion necessary

        if (!endsWith(userDirectory, FILE_NAME_SEPARATOR))
            userDirectory += FILE_NAME_SEPARATOR;

        return userDirectory;
    }
}


inline
wxString getLauncher()
{
#ifdef FFS_WIN
    return impl::getInstallDir() + wxT("FreeFileSync.exe");
#elif defined FFS_LINUX
    return impl::getBinaryDir() + wxT("FreeFileSync");
#endif
}
}

#endif // STANDARDPATHS_H_INCLUDED
