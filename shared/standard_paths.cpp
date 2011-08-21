// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) 2008-2011 ZenJu (zhnmju123 AT gmx.de)                    *
// **************************************************************************

#include "standard_paths.h"
#include <wx/stdpaths.h>
#include "string_conv.h"

using namespace zen;


namespace
{
const wxString& getBinaryDir() //directory containing executable WITH path separator at end
{
    static wxString instance = beforeLast(wxStandardPaths::Get().GetExecutablePath(), FILE_NAME_SEPARATOR) + toWx(Zstring(FILE_NAME_SEPARATOR));
    return instance;
}

#ifdef FFS_WIN
wxString getInstallDir() //root install directory WITH path separator at end
{
    return getBinaryDir().BeforeLast(FILE_NAME_SEPARATOR).BeforeLast(FILE_NAME_SEPARATOR) + FILE_NAME_SEPARATOR;
}
#endif
}


bool zen::isPortableVersion()
{
#ifdef FFS_WIN
    static const bool isPortable = !wxFileExists(getInstallDir() + wxT("uninstall.exe")); //this check is a bit lame...
#elif defined FFS_LINUX
    static const bool isPortable = !::getBinaryDir().EndsWith(wxT("/bin/")); //this check is a bit lame...
#endif
    return isPortable;
}


wxString zen::getResourceDir()
{
#ifdef FFS_WIN
    return getInstallDir();
#elif defined FFS_LINUX
    if (isPortableVersion())
        return getBinaryDir();
    else //use OS' standard paths
    {
        wxString resourceDir = wxStandardPathsBase::Get().GetResourcesDir();

        if (!endsWith(resourceDir, FILE_NAME_SEPARATOR))
            resourceDir += FILE_NAME_SEPARATOR;

        return resourceDir;
    }
#endif
}


wxString zen::getConfigDir()
{
    if (isPortableVersion())
#ifdef FFS_WIN
        return getInstallDir();
#elif defined FFS_LINUX
        //wxString(wxT(".")) + zToWx(FILE_NAME_SEPARATOR) -> don't use current working directory
        //avoid surprises with GlobalSettings.xml being newly created in each working directory
        return getBinaryDir();
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


wxString zen::getLauncher()
{
#ifdef FFS_WIN
    return getInstallDir() + wxT("FreeFileSync.exe");
#elif defined FFS_LINUX
    return getBinaryDir() + wxT("FreeFileSync");
#endif
}
