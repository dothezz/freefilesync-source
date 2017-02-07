// *****************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under    *
// * GNU General Public License: http://www.gnu.org/licenses/gpl-3.0           *
// * Copyright (C) Zenju (zenju AT freefilesync DOT org) - All Rights Reserved *
// *****************************************************************************

#include "ffs_paths.h"
#include <zen/file_access.h>
#include <wx/stdpaths.h>
#include <wx/app.h>
#include <wx+/string_conv.h>


using namespace zen;


namespace
{
inline
Zstring getExecutablePathPf() //directory containing executable WITH path separator at end
{
    return appendSeparator(beforeLast(utfCvrtTo<Zstring>(wxStandardPaths::Get().GetExecutablePath()), FILE_NAME_SEPARATOR, IF_MISSING_RETURN_NONE));
}
}




bool zen::isPortableVersion()
{
    return !endsWith(getExecutablePathPf(), "/bin/");  //this check is a bit lame...

}


Zstring zen::getResourceDirPf()
{
    //make independent from wxWidgets global variable "appname"; support being called by RealTimeSync
    auto appName = wxTheApp->GetAppName();
    wxTheApp->SetAppName(L"FreeFileSync");
    ZEN_ON_SCOPE_EXIT(wxTheApp->SetAppName(appName));

    if (isPortableVersion())
        return getExecutablePathPf();
    else //use OS' standard paths
        return appendSeparator(toZ(wxStandardPathsBase::Get().GetResourcesDir()));
}


Zstring zen::getConfigDirPathPf()
{
    //make independent from wxWidgets global variable "appname"; support being called by RealTimeSync
    auto appName = wxTheApp->GetAppName();
    wxTheApp->SetAppName(L"FreeFileSync");
    ZEN_ON_SCOPE_EXIT(wxTheApp->SetAppName(appName));

    if (isPortableVersion())
        return getExecutablePathPf();
    //use OS' standard paths
    Zstring configDirPath = toZ(wxStandardPaths::Get().GetUserDataDir());
    try
    {
        createDirectoryIfMissingRecursion(configDirPath); //throw FileError
    }
    catch (const FileError&) { assert(false); }

    return appendSeparator(configDirPath);
}


//this function is called by RealTimeSync!!!
Zstring zen::getFreeFileSyncLauncherPath()
{
    return getExecutablePathPf() + Zstr("FreeFileSync");

}
