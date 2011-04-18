// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) 2008-2011 ZenJu (zhnmju123 AT gmx.de)                    *
// **************************************************************************
//
#include "standard_paths.h"
#include <wx/stdpaths.h>
#include "system_constants.h"
#include "string_conv.h"

using namespace ffs3;


bool ffs3::isPortableVersion()
{
#ifdef FFS_WIN
    static const bool isPortable = !wxFileExists(ffs3::getBinaryDir() + wxT("uninstall.exe")); //this check is a bit lame...
#elif defined FFS_LINUX
    static const bool isPortable = !ffs3::getBinaryDir().EndsWith(wxT("/bin/")); //this check is a bit lame...
#endif
    return isPortable;
}


const wxString& ffs3::getBinaryDir()
{
    static wxString instance = zToWx(wxToZ(wxStandardPaths::Get().GetExecutablePath()).BeforeLast(common::FILE_NAME_SEPARATOR) + common::FILE_NAME_SEPARATOR);
    return instance;
}


const wxString& ffs3::getResourceDir()
{
#ifdef FFS_WIN
    return getBinaryDir();
#elif defined FFS_LINUX
    static wxString resourceDir;

    static bool isInitalized = false; //poor man's singleton...
    if (!isInitalized)
    {
        isInitalized = true;

        if (isPortableVersion())
            resourceDir = getBinaryDir();
        else //use OS' standard paths
        {
            resourceDir = wxStandardPathsBase::Get().GetResourcesDir();

            if (!resourceDir.EndsWith(zToWx(common::FILE_NAME_SEPARATOR)))
                resourceDir += zToWx(common::FILE_NAME_SEPARATOR);
        }
    }

    return resourceDir;
#endif
}


const wxString& ffs3::getConfigDir()
{
    static wxString userDirectory;

    static bool isInitalized = false; //poor man's singleton...
    if (!isInitalized)
    {
        isInitalized = true;

        if (isPortableVersion())
            //userDirectory = wxString(wxT(".")) + zToWx(common::FILE_NAME_SEPARATOR); //use current working directory
            userDirectory = getBinaryDir(); //avoid surprises with GlobalSettings.xml being newly created in each working directory
        else //use OS' standard paths
        {
            userDirectory = wxStandardPathsBase::Get().GetUserDataDir();

            if (!wxDirExists(userDirectory))
                ::wxMkdir(userDirectory); //only top directory needs to be created: no recursion necessary

            if (!userDirectory.EndsWith(zToWx(common::FILE_NAME_SEPARATOR)))
                userDirectory += zToWx(common::FILE_NAME_SEPARATOR);
        }
    }

    return userDirectory;
}
