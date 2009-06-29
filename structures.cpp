#include "structures.h"
#include "library/fileHandling.h"
#include <wx/intl.h>
#include <wx/stdpaths.h>
#include <wx/filename.h>

using namespace FreeFileSync;


#ifdef FFS_WIN
const wxChar FreeFileSync::FILE_NAME_SEPARATOR = '\\';
#elif defined FFS_LINUX
const wxChar FreeFileSync::FILE_NAME_SEPARATOR = '/';
#else
assert(false);
#endif

//these two global variables are language-dependent => cannot be set constant! See CustomLocale
const wxChar* FreeFileSync::DECIMAL_POINT       = wxEmptyString;
const wxChar* FreeFileSync::THOUSANDS_SEPARATOR = wxEmptyString;


wxString assembleFileForUserData(const wxString fileName)
{
    static const bool isPortableVersion = !wxFileExists(wxT("uninstall.exe")); //this check is a bit lame...

    if (isPortableVersion) //use same directory as executable
        return getInstallationDir() + FILE_NAME_SEPARATOR + fileName;
    else //usen OS' standard paths
    {
        wxString userDirectory = wxStandardPathsBase::Get().GetUserDataDir();

        if (!userDirectory.EndsWith(wxString(FreeFileSync::FILE_NAME_SEPARATOR)))
            userDirectory += FreeFileSync::FILE_NAME_SEPARATOR;

        if (!wxDirExists(userDirectory))
            try
            {
                FreeFileSync::createDirectory(userDirectory.c_str(), wxEmptyString, false);
            }
            catch (FreeFileSync::FileError&)
                {}

        return userDirectory + fileName;
    }
}


//save user configuration in OS' standard application folders
const wxString& FreeFileSync::getLastConfigFile()
{
    static wxString instance = assembleFileForUserData(wxT("LastRun.ffs_gui"));
    return instance;
}


const wxString& FreeFileSync::getGlobalConfigFile()
{
    static wxString instance = assembleFileForUserData(wxT("GlobalSettings.xml"));
    return instance;
}


const wxString& FreeFileSync::getDefaultLogDirectory()
{
    static wxString instance = assembleFileForUserData(wxT("Logs"));
    return instance;
}


const wxString& FreeFileSync::getLastErrorTxtFile()
{
    static wxString instance = assembleFileForUserData(wxT("LastError.txt"));
    return instance;
}


const wxString& FreeFileSync::getInstallationDir()
{
    static wxString instance = wxFileName(wxStandardPaths::Get().GetExecutablePath()).GetPath();
    return instance;
}


MainConfiguration::MainConfiguration() :
        compareVar(CMP_BY_TIME_SIZE),
        filterIsActive(false),        //do not filter by default
        includeFilter(wxT("*")),      //include all files/folders
        excludeFilter(wxEmptyString), //exclude nothing
        useRecycleBin(FreeFileSync::recycleBinExists()) {} //enable if OS supports it; else user will have to activate first and then get an error message


SyncConfiguration::Variant SyncConfiguration::getVariant()
{
    if (    exLeftSideOnly  == SYNC_DIR_RIGHT &&
            exRightSideOnly == SYNC_DIR_RIGHT &&
            leftNewer       == SYNC_DIR_RIGHT &&
            rightNewer      == SYNC_DIR_RIGHT &&
            different       == SYNC_DIR_RIGHT)
        return MIRROR;    //one way ->

    else if (exLeftSideOnly  == SYNC_DIR_RIGHT &&
             exRightSideOnly == SYNC_DIR_NONE  &&
             leftNewer       == SYNC_DIR_RIGHT &&
             rightNewer      == SYNC_DIR_NONE  &&
             different       == SYNC_DIR_NONE)
        return UPDATE;    //Update ->

    else if (exLeftSideOnly  == SYNC_DIR_RIGHT &&
             exRightSideOnly == SYNC_DIR_LEFT  &&
             leftNewer       == SYNC_DIR_RIGHT &&
             rightNewer      == SYNC_DIR_LEFT  &&
             different       == SYNC_DIR_NONE)
        return TWOWAY;    //two way <->
    else
        return CUSTOM;    //other
}


wxString SyncConfiguration::getVariantName()
{
    switch (getVariant())
    {
    case MIRROR:
        return _("Mirror ->>");
    case UPDATE:
        return _("Update ->");
    case TWOWAY:
        return _("Two way <->");
    case CUSTOM:
        return _("Custom");
    }

    return _("Error");
}


void SyncConfiguration::setVariant(const Variant var)
{
    switch (var)
    {
    case MIRROR:
        exLeftSideOnly  = SYNC_DIR_RIGHT;
        exRightSideOnly = SYNC_DIR_RIGHT;
        leftNewer       = SYNC_DIR_RIGHT;
        rightNewer      = SYNC_DIR_RIGHT;
        different       = SYNC_DIR_RIGHT;
        break;
    case UPDATE:
        exLeftSideOnly  = SYNC_DIR_RIGHT;
        exRightSideOnly = SYNC_DIR_NONE;
        leftNewer       = SYNC_DIR_RIGHT;
        rightNewer      = SYNC_DIR_NONE;
        different       = SYNC_DIR_NONE;
        break;
    case TWOWAY:
        exLeftSideOnly  = SYNC_DIR_RIGHT;
        exRightSideOnly = SYNC_DIR_LEFT;
        leftNewer       = SYNC_DIR_RIGHT;
        rightNewer      = SYNC_DIR_LEFT;
        different       = SYNC_DIR_NONE;
        break;
    case CUSTOM:
        assert(false);
        break;
    }
}
