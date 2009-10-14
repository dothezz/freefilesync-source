#include "standardPaths.h"
#include <wx/stdpaths.h>
#include <wx/filename.h>
#include "systemConstants.h"
#include "stringConv.h"

using namespace FreeFileSync;


wxString assembleFileForUserData(const wxString fileName)
{
    static const bool isPortableVersion = !wxFileExists(FreeFileSync::getInstallationDir() + zToWx(globalFunctions::FILE_NAME_SEPARATOR) + wxT("uninstall.exe")); //this check is a bit lame...

    if (isPortableVersion) //use current working directory
        return wxString(wxT(".")) + zToWx(globalFunctions::FILE_NAME_SEPARATOR) + fileName;
    else //usen OS' standard paths
    {
        wxString userDirectory = wxStandardPathsBase::Get().GetUserDataDir();

        if (!userDirectory.EndsWith(zToWx(globalFunctions::FILE_NAME_SEPARATOR)))
            userDirectory += zToWx(globalFunctions::FILE_NAME_SEPARATOR);

        if (!wxDirExists(userDirectory))
            ::wxMkdir(userDirectory); //only top directory needs to be created: no recursion necessary

        return userDirectory + fileName;
    }
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


const wxString& FreeFileSync::getConfigDir()
{
    static wxString instance = assembleFileForUserData(wxEmptyString);
    return instance;
}
