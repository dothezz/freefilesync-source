// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) 2008-2010 ZenJu (zhnmju123 AT gmx.de)                    *
// **************************************************************************
//
#include "standardPaths.h"
#include <wx/stdpaths.h>
#include <wx/filename.h>
#include "systemConstants.h"
#include "stringConv.h"

using namespace FreeFileSync;

bool FreeFileSync::isPortableVersion()
{
#ifdef FFS_WIN
    static const bool isPortable = !wxFileExists(FreeFileSync::getBinaryDir() + wxT("uninstall.exe")); //this check is a bit lame...
#elif defined FFS_LINUX
    static const bool isPortable = !FreeFileSync::getBinaryDir().EndsWith(wxT("/bin/")); //this check is a bit lame...
#endif
    return isPortable;
}


const wxString& FreeFileSync::getBinaryDir()
{
    static wxString instance = wxFileName(wxStandardPaths::Get().GetExecutablePath()).GetPath() + zToWx(globalFunctions::FILE_NAME_SEPARATOR);
    return instance;
}


const wxString& FreeFileSync::getResourceDir()
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

            if (!resourceDir.EndsWith(zToWx(globalFunctions::FILE_NAME_SEPARATOR)))
                resourceDir += zToWx(globalFunctions::FILE_NAME_SEPARATOR);
        }
    }

    return resourceDir;
#endif
}


const wxString& FreeFileSync::getConfigDir()
{
    static wxString userDirectory;

    static bool isInitalized = false; //poor man's singleton...
    if (!isInitalized)
    {
        isInitalized = true;

        if (isPortableVersion())
            //userDirectory = wxString(wxT(".")) + zToWx(globalFunctions::FILE_NAME_SEPARATOR); //use current working directory
            userDirectory = getBinaryDir(); //avoid surprises with GlobalSettings.xml being newly created in each working directory
        else //use OS' standard paths
        {
            userDirectory = wxStandardPathsBase::Get().GetUserDataDir();

            if (!wxDirExists(userDirectory))
                ::wxMkdir(userDirectory); //only top directory needs to be created: no recursion necessary

            if (!userDirectory.EndsWith(zToWx(globalFunctions::FILE_NAME_SEPARATOR)))
                userDirectory += zToWx(globalFunctions::FILE_NAME_SEPARATOR);
        }
    }

    return userDirectory;
}
