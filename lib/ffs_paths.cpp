// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) Zenju (zenju AT gmx DOT de) - All Rights Reserved        *
// **************************************************************************

#include "ffs_paths.h"
#include <zen/file_handling.h>
#include <wx/stdpaths.h>
#include <wx+/string_conv.h>

#ifdef FFS_MAC
#include <vector>
#include <zen/scope_guard.h>
#include <zen/osx_string.h>
//keep in .cpp file to not pollute global namespace! e.g. with UInt64:
#include <ApplicationServices/ApplicationServices.h> //LSFindApplicationForInfo
#endif

using namespace zen;


namespace
{
#if defined FFS_WIN || defined FFS_LINUX
inline
Zstring getExecutableDir() //directory containing executable WITH path separator at end
{
    return appendSeparator(beforeLast(utfCvrtTo<Zstring>(wxStandardPaths::Get().GetExecutablePath()), FILE_NAME_SEPARATOR));
}
#endif

#ifdef FFS_WIN
inline
Zstring getInstallDir() //root install directory WITH path separator at end
{
    return appendSeparator(beforeLast(beforeLast(getExecutableDir(), FILE_NAME_SEPARATOR), FILE_NAME_SEPARATOR));
}
#endif


#ifdef FFS_WIN
inline
bool isPortableVersion() { return !fileExists(getInstallDir() + L"uninstall.exe"); } //this check is a bit lame...
#elif defined FFS_LINUX
inline
bool isPortableVersion() { return !endsWith(getExecutableDir(), "/bin/"); } //this check is a bit lame...
#endif
}


bool zen::manualProgramUpdateRequired()
{
#if defined FFS_WIN || defined FFS_MAC
    return true;
#elif defined FFS_LINUX
    return isPortableVersion(); //locally installed version is updated by system
#endif
}


Zstring zen::getResourceDir()
{
#ifdef FFS_WIN
    return getInstallDir();
#elif defined FFS_LINUX
    if (isPortableVersion())
        return getExecutableDir();
    else //use OS' standard paths
        return appendSeparator(toZ(wxStandardPathsBase::Get().GetResourcesDir()));
#elif defined FFS_MAC
    return appendSeparator(toZ(wxStandardPathsBase::Get().GetResourcesDir()));
#endif
}


Zstring zen::getConfigDir()
{
#ifdef FFS_WIN
    if (isPortableVersion())
        return getInstallDir();
#elif defined FFS_LINUX
    if (isPortableVersion())
        return getExecutableDir();
#elif defined FFS_MAC
    //portable apps do not seem common on OS - fine with me: http://theocacao.com/document.page/319
#endif
    //use OS' standard paths
    Zstring userDirectory = toZ(wxStandardPathsBase::Get().GetUserDataDir());

    if (!dirExists(userDirectory))
        try
        {
            makeDirectory(userDirectory); //throw FileError
        }
        catch (const FileError&) {}

    return appendSeparator(userDirectory);
}


//this function is called by RealtimeSync!!!
Zstring zen::getFreeFileSyncLauncher()
{
#ifdef FFS_WIN
    return getInstallDir() + Zstr("FreeFileSync.exe");

#elif defined FFS_LINUX
    return getExecutableDir() + Zstr("FreeFileSync");

#elif defined FFS_MAC
    CFURLRef appURL = nullptr;
    ZEN_ON_SCOPE_EXIT(if (appURL) ::CFRelease(appURL));

    if (::LSFindApplicationForInfo(kLSUnknownCreator, // OSType inCreator,
                                   CFSTR("net.SourceForge.FreeFileSync"),//CFStringRef inBundleID,
                                   nullptr,           //CFStringRef inName,
                                   nullptr,           //FSRef *outAppRef,
                                   &appURL) == noErr) //CFURLRef *outAppURL
        if (appURL)
            if (CFURLRef absUrl = ::CFURLCopyAbsoluteURL(appURL))
            {
                ZEN_ON_SCOPE_EXIT(::CFRelease(absUrl));

                if (CFStringRef path = ::CFURLCopyFileSystemPath(absUrl, kCFURLPOSIXPathStyle))
                {
                    ZEN_ON_SCOPE_EXIT(::CFRelease(path));
                    return appendSeparator(osx::cfStringToZstring(path)) + "Contents/MacOS/FreeFileSync";
                }
            }
    return Zstr("./FreeFileSync"); //fallback: at least give some hint...
#endif
}
