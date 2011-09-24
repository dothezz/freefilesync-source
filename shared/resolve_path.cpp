#include "resolve_path.h"
#include <wx/utils.h>
#include <wx/datetime.h>
#include "string_conv.h"
#include "loki/ScopeGuard.h"
#include <map>

#ifdef FFS_WIN
#include "dll_loader.h"
#include <Shlobj.h>
#include <wx/msw/wrapwin.h> //includes "windows.h"
#include "long_path_prefix.h"
#ifdef _MSC_VER
#pragma comment(lib, "Mpr.lib")
#endif

#elif defined FFS_LINUX
#include <map>
#include "file_traverser.h"
#include "stdlib.h"
#endif

using namespace zen;


namespace
{
#ifdef FFS_WIN
Zstring resolveBrokenNetworkMap(const Zstring& dirname) //circumvent issue with disconnected network maps that could be activated by a simple explorer double click
{
    /*
    ATTENTION: it is not safe to call ::WNetGetConnection() for every network share:

    network type				 |::WNetGetConnection rv   | lpRemoteName				     | existing UNC path
    -----------------------------|-------------------------|---------------------------------|----------------
    inactive local network share | ERROR_CONNECTION_UNAVAIL| \\192.168.1.27\new2			 | YES
    WebDrive					 | NO_ERROR				   | \\Webdrive-ZenJu\GNU		     | NO
    Box.net (WebDav)			 | NO_ERROR				   | \\www.box.net\DavWWWRoot\dav    | YES
    NetDrive					 | ERROR_NOT_CONNECTED     | <empty>						 | NO
    */

    if (dirname.size() >= 2 && iswalpha(dirname[0]) && dirname[1] == L':')
    {
        Zstring driveLetter(dirname.c_str(), 2); //e.g.: "Q:"

        //if (::GetFileAttributes((driveLetter + L'\\').c_str()) == INVALID_FILE_ATTRIBUTES) <- this will seriously block if network is not reachable!!!
        {
            DWORD bufferSize = 10000;
            std::vector<wchar_t> remoteNameBuffer(bufferSize);
            DWORD rv = ::WNetGetConnection(driveLetter.c_str(),  //__in     LPCTSTR lpLocalName in the form "<driveletter>:"
                                           &remoteNameBuffer[0], //__out    LPTSTR lpRemoteName,
                                           &bufferSize);         //__inout  LPDWORD lpnLength
            if (rv == ERROR_CONNECTION_UNAVAIL) //remoteNameBuffer will be filled nevertheless!
            {
                Zstring networkShare = &remoteNameBuffer[0];
                if (!networkShare.empty())
                    return networkShare + (dirname.c_str() + 2);  //replace "Q:\subdir" by "\\server\share\subdir"
            }
        }
    }
    return dirname;
}


Zstring resolveRelativePath(Zstring relativeName)
{
    relativeName = resolveBrokenNetworkMap(relativeName);

    std::vector<Zchar> fullPath(10000);

    const DWORD rv = ::GetFullPathName(applyLongPathPrefix(relativeName).c_str(), //__in   LPCTSTR lpFileName,
                                       static_cast<DWORD>(fullPath.size()), //__in   DWORD nBufferLength,
                                       &fullPath[0],       //__out  LPTSTR lpBuffer,
                                       NULL);              //__out  LPTSTR *lpFilePart
    if (rv == 0 || rv >= fullPath.size()) //theoretically, rv can never be == fullPath.size()
        //ERROR! Don't do anything
        return relativeName;

    return &fullPath[0];
}

#elif defined FFS_LINUX
Zstring resolveRelativePath(const Zstring& relativeName) //additional: resolves symbolic links!!!
{
    char absolutePath[PATH_MAX + 1];
    if (::realpath(relativeName.c_str(), absolutePath) == NULL)
        //ERROR! Don't do anything
        return relativeName;

    return Zstring(absolutePath);
}
#endif


#ifdef FFS_WIN
class CsidlConstants
{
public:
    typedef std::map<Zstring, Zstring, LessFilename> CsidlToDirMap; //case-insensitive comparison

    static const CsidlToDirMap& get()
    {
        static CsidlConstants inst;
        return inst.csidlToDir;
    }

private:
    CsidlConstants()
    {
        auto addCsidl = [&](int csidl, const Zstring& paramName)
        {
            wchar_t buffer[MAX_PATH];
            if (SUCCEEDED(::SHGetFolderPath(NULL,                           //__in   HWND hwndOwner,
                                            csidl | CSIDL_FLAG_DONT_VERIFY, //__in   int nFolder,
                                            NULL,						    //__in   HANDLE hToken,
                                            0 /* == SHGFP_TYPE_CURRENT*/,   //__in   DWORD dwFlags,
                                            buffer)))					  	//__out  LPTSTR pszPath
            {
                Zstring dirname = buffer;
                if (!dirname.empty())
                    csidlToDir.insert(std::make_pair(paramName, dirname));
            }
        };

        addCsidl(CSIDL_DESKTOPDIRECTORY,        L"csidl_Desktop");        // C:\Users\username\Desktop
        addCsidl(CSIDL_COMMON_DESKTOPDIRECTORY, L"csidl_PublicDesktop"); // C:\Users\All Users\Desktop

        addCsidl(CSIDL_MYMUSIC,          L"csidl_MyMusic");         // C:\Users\username\My Documents\My Music
        addCsidl(CSIDL_COMMON_MUSIC,     L"csidl_PublicMusic");     // C:\Users\All Users\Documents\My Music

        addCsidl(CSIDL_MYPICTURES,       L"csidl_MyPictures");      // C:\Users\username\My Documents\My Pictures
        addCsidl(CSIDL_COMMON_PICTURES,  L"csidl_PublicPictures");  // C:\Users\All Users\Documents\My Pictures

        addCsidl(CSIDL_MYVIDEO,          L"csidl_MyVideo");         // C:\Users\username\My Documents\My Videos
        addCsidl(CSIDL_COMMON_VIDEO,     L"csidl_PublicVideo");     // C:\Users\All Users\Documents\My Videos

        addCsidl(CSIDL_PERSONAL,         L"csidl_MyDocuments");     // C:\Users\username\My Documents
        addCsidl(CSIDL_COMMON_DOCUMENTS, L"csidl_PublicDocuments"); // C:\Users\All Users\Documents

        addCsidl(CSIDL_STARTMENU,        L"csidl_StartMenu");       // C:\Users\username\Start Menu
        addCsidl(CSIDL_COMMON_STARTMENU, L"csidl_PublicStartMenu"); // C:\Users\All Users\Start Menu

        addCsidl(CSIDL_FAVORITES,        L"csidl_Favorites");       // C:\Users\username\Favorites
        addCsidl(CSIDL_COMMON_FAVORITES, L"csidl_PublicFavorites"); // C:\Users\All Users\Favoriten

        addCsidl(CSIDL_TEMPLATES,        L"csidl_Templates");       // C:\Users\username\Templates
        addCsidl(CSIDL_COMMON_TEMPLATES, L"csidl_PublicTemplates"); // C:\Users\All Users\Templates

        addCsidl(CSIDL_RESOURCES,        L"csidl_Resources");       // C:\Windows\Resources

        //CSIDL_APPDATA        covered by %AppData%
        //CSIDL_LOCAL_APPDATA  covered by %LocalAppData%
        //CSIDL_COMMON_APPDATA covered by %ProgramData%

        //CSIDL_PROFILE covered by %UserProfile%
    }

    CsidlConstants(const CsidlConstants&);
    CsidlConstants& operator=(const CsidlConstants&);

    CsidlToDirMap csidlToDir;
};
#endif


wxString getEnvValue(const wxString& envName) //return empty on error
{
    //try to apply environment variables
    wxString envValue;
    if (wxGetEnv(envName, &envValue))
    {
        //some postprocessing:
        trim(envValue); //remove leading, trailing blanks

        //remove leading, trailing double-quotes
        if (startsWith(envValue, L"\"") &&
            endsWith(envValue, L"\"") &&
            envValue.length() >= 2)
            envValue = wxString(envValue.c_str() + 1, envValue.length() - 2);
    }
    return envValue;
}


bool replaceMacro(wxString& macro) //macro without %-characters, return true if replaced successfully
{
    if (macro.IsEmpty())
        return false;

    //there are equally named environment variables %TIME%, %DATE% existing, so replace these first!
    if (macro.CmpNoCase(wxT("time")) == 0)
    {
        macro = wxDateTime::Now().FormatISOTime();
        macro.Replace(wxT(":"), wxT(""));
        return true;
    }

    if (macro.CmpNoCase(wxT("date")) == 0)
    {
        macro = wxDateTime::Now().FormatISODate();
        return true;
    }

    auto processPhrase = [&](const wchar_t* phrase, const wchar_t* format) -> bool
    {
        if (macro.CmpNoCase(phrase) != 0)
            return false;
        macro = wxDateTime::Now().Format(format);
        return true;
    };

    if (processPhrase(L"weekday", L"%A")) return true;
    if (processPhrase(L"day"    , L"%d")) return true;
    if (processPhrase(L"month"  , L"%B")) return true;
    if (processPhrase(L"week"   , L"%U")) return true;
    if (processPhrase(L"year"   , L"%Y")) return true;
    if (processPhrase(L"hour"   , L"%H")) return true;
    if (processPhrase(L"min"    , L"%M")) return true;
    if (processPhrase(L"sec"    , L"%S")) return true;

    //try to apply environment variables
    {
        wxString envValue = getEnvValue(macro);
        if (!envValue.empty())
        {
            macro = envValue;
            return true;
        }
    }

#ifdef FFS_WIN
    //try to resolve CSIDL values
    {
        auto csidlMap = CsidlConstants::get();
        auto iter = csidlMap.find(toZ(macro));
        if (iter != csidlMap.end())
        {
            macro = toWx(iter->second);
            return true;
        }
    }
#endif

    return false;
}


void expandMacros(wxString& text)
{
    const wxChar SEPARATOR = '%';

    if (text.Find(SEPARATOR) != wxNOT_FOUND)
    {
        wxString prefix  = text.BeforeFirst(SEPARATOR);
        wxString postfix = text.AfterFirst(SEPARATOR);
        if (postfix.Find(SEPARATOR) != wxNOT_FOUND)
        {
            wxString potentialMacro = postfix.BeforeFirst(SEPARATOR);
            wxString rest           = postfix.AfterFirst(SEPARATOR); //text == prefix + SEPARATOR + potentialMacro + SEPARATOR + rest

            if (replaceMacro(potentialMacro))
            {
                expandMacros(rest);
                text = prefix + potentialMacro + rest;
            }
            else
            {
                rest = SEPARATOR + rest;
                expandMacros(rest);
                text = prefix + SEPARATOR + potentialMacro + rest;
            }
        }
    }
}


#ifdef FFS_LINUX
class TraverseMedia : public zen::TraverseCallback
{
public:
    typedef std::map<Zstring, Zstring> DeviceList; //device name -> device path mapping

    TraverseMedia(DeviceList& devices) : devices_(devices) {}

    virtual void onFile(const Zchar* shortName, const Zstring& fullName, const FileInfo& details) {}
    virtual void onSymlink(const Zchar* shortName, const Zstring& fullName, const SymlinkInfo& details) {}
    virtual ReturnValDir onDir(const Zchar* shortName, const Zstring& fullName)
    {
        devices_.insert(std::make_pair(shortName, fullName));
        return Loki::Int2Type<ReturnValDir::TRAVERSING_DIR_IGNORE>(); //DON'T traverse into subdirs
    }
    virtual HandleError onError(const std::wstring& errorText) { return TRAV_ERROR_IGNORE; }

private:
    DeviceList& devices_;
};
#endif


Zstring volumenNameToPath(const Zstring& volumeName) //return empty string on error
{
#ifdef FFS_WIN
    std::vector<wchar_t> volGuid(10000);

    HANDLE hVol = ::FindFirstVolume(&volGuid[0], static_cast<DWORD>(volGuid.size()));
    if (hVol != INVALID_HANDLE_VALUE)
    {
        LOKI_ON_BLOCK_EXIT2(::FindVolumeClose(hVol));

        do
        {
            std::vector<wchar_t> volName(MAX_PATH + 1);

            if (::GetVolumeInformation(&volGuid[0],    //__in_opt   LPCTSTR lpRootPathName,
                                       &volName[0],    //__out      LPTSTR lpVolumeNameBuffer,
                                       static_cast<DWORD>(volName.size()), //__in       DWORD nVolumeNameSize,
                                       NULL,           //__out_opt  LPDWORD lpVolumeSerialNumber,
                                       NULL,           //__out_opt  LPDWORD lpMaximumComponentLength,
                                       NULL,           //__out_opt  LPDWORD lpFileSystemFlags,
                                       NULL,           //__out      LPTSTR lpFileSystemNameBuffer,
                                       0))             //__in       DWORD nFileSystemNameSize
            {
                if (EqualFilename()(volumeName, Zstring(&volName[0])))
                {
                    //GetVolumePathNamesForVolumeName is not available for Windows 2000!
                    typedef	BOOL (WINAPI *GetVolumePathNamesForVolumeNameWFunc)(LPCWSTR lpszVolumeName,
                                                                                LPWCH lpszVolumePathNames,
                                                                                DWORD cchBufferLength,
                                                                                PDWORD lpcchReturnLength);

                    const util::DllFun<GetVolumePathNamesForVolumeNameWFunc> getVolumePathNamesForVolumeName(L"kernel32.dll", "GetVolumePathNamesForVolumeNameW");
                    if (getVolumePathNamesForVolumeName)
                    {
                        std::vector<wchar_t> volPath(10000);

                        DWORD returnedLen = 0;
                        if (getVolumePathNamesForVolumeName(&volGuid[0],     //__in   LPCTSTR lpszVolumeName,
                                                            &volPath[0],     //__out  LPTSTR lpszVolumePathNames,
                                                            static_cast<DWORD>(volPath.size()), //__in   DWORD cchBufferLength,
                                                            &returnedLen))  //__out  PDWORD lpcchReturnLength
                        {
                            return &volPath[0]; //return first path name in double-null terminated list!
                        }
                    }
                    return &volGuid[0]; //GUID looks ugly, but should be working correctly
                }
            }
        }
        while (::FindNextVolume(hVol, &volGuid[0], static_cast<DWORD>(volGuid.size())));
    }

#elif defined FFS_LINUX
    //due to the naming convention on Linux /media/<volume name> this function is not that useful, but...

    TraverseMedia::DeviceList deviceList;

    TraverseMedia traverser(deviceList);
    traverseFolder("/media", false, traverser); //traverse one level

    TraverseMedia::DeviceList::const_iterator iter = deviceList.find(volumeName);
    if (iter != deviceList.end())
        return iter->second;
#endif
    return Zstring();
}


#ifdef FFS_WIN
Zstring volumePathToName(const Zstring& volumePath) //return empty string on error
{
    const DWORD bufferSize = MAX_PATH + 1;
    std::vector<wchar_t> volName(bufferSize);

    if (::GetVolumeInformation(volumePath.c_str(), //__in_opt   LPCTSTR lpRootPathName,
                               &volName[0],        //__out      LPTSTR lpVolumeNameBuffer,
                               bufferSize,         //__in       DWORD nVolumeNameSize,
                               NULL,               //__out_opt  LPDWORD lpVolumeSerialNumber,
                               NULL,               //__out_opt  LPDWORD lpMaximumComponentLength,
                               NULL,               //__out_opt  LPDWORD lpFileSystemFlags,
                               NULL,               //__out      LPTSTR lpFileSystemNameBuffer,
                               0))                 //__in       DWORD nFileSystemNameSize
    {
        return &volName[0];
    }
    return Zstring();
}
#endif


void expandVolumeName(Zstring& text)  // [volname]:\folder       [volname]\folder       [volname]folder     -> C:\folder
{
    Zstring before;
    Zstring volname;
    Zstring after;

    size_t posStart = text.find(Zstr("["));
    if (posStart != Zstring::npos)
    {
        size_t posEnd = text.find(Zstr("]"), posStart);
        if (posEnd != Zstring::npos)
        {
            before  = Zstring(text.c_str(), posStart);
            volname = Zstring(text.c_str() + posStart + 1, posEnd - posStart - 1);
            after   = Zstring(text.c_str() + posEnd + 1);

            if (startsWith(after, ':'))
                after = afterFirst(after, ':');
            if (startsWith(after, FILE_NAME_SEPARATOR))
                after = afterFirst(after, FILE_NAME_SEPARATOR);
        }
    }

    if (volname.empty())
        return;

    Zstring volPath = volumenNameToPath(volname); //return empty string on error
    if (volPath.empty())
        return;

    if (!volPath.EndsWith(FILE_NAME_SEPARATOR))
        volPath += FILE_NAME_SEPARATOR;

    text = before + volPath + after;
}
}


#ifdef FFS_WIN
std::vector<Zstring> zen::getDirectoryAliases(Zstring dirname)
{
    trim(dirname, true, false);

    std::vector<Zstring> output;

    if (dirname.empty())
        return output;

    //1. replace volume path by volume name: c:\dirname ->  [SYSTEM]\dirname
    if (dirname.size() >= 3 &&
        std::iswalpha(dirname[0]) &&
        dirname[1] == L':' &&
        dirname[2] == L'\\')
    {
        Zstring volname = volumePathToName(Zstring(dirname.c_str(), 3));
        if (!volname.empty())
            output.push_back(L"[" + volname + L"]" + Zstring(dirname.c_str() + 2));
    }

    //2. replace volume name by volume path: [SYSTEM]\dirname -> c:\dirname
    {
        Zstring testVolname = dirname;
        expandVolumeName(testVolname);
        if (testVolname != dirname)
            output.push_back(testVolname);
    }

    //3. environment variables: C:\Users\username -> %USERPROFILE%
    {
        std::map<Zstring, Zstring> envToDir;

        //get list of useful variables
        auto addEnvVar = [&](const wxString& envName)
        {
            wxString envVal = getEnvValue(envName); //return empty on error
            if (!envVal.empty())
                envToDir.insert(std::make_pair(toZ(envName), toZ(envVal)));
        };
        addEnvVar(L"AllUsersProfile");  // C:\ProgramData
        addEnvVar(L"AppData");          // C:\Users\username\AppData\Roaming
        addEnvVar(L"LocalAppData");     // C:\Users\username\AppData\Local
        addEnvVar(L"ProgramData");      // C:\ProgramData
        addEnvVar(L"ProgramFiles");     // C:\Program Files
        addEnvVar(L"ProgramFiles(x86)");// C:\Program Files (x86)
        addEnvVar(L"Public");           // C:\Users\Public
        addEnvVar(L"UserProfile");      // C:\Users\username
        addEnvVar(L"WinDir");           // C:\Windows
        addEnvVar(L"Temp");             // C:\Windows\Temp

        //add CSIDL values: http://msdn.microsoft.com/en-us/library/bb762494(v=vs.85).aspx
        auto csidlMap = CsidlConstants::get();
        envToDir.insert(csidlMap.begin(), csidlMap.end());

        Zstring tmp = dirname;
        ::makeUpper(tmp);
        std::for_each(envToDir.begin(), envToDir.end(),
                      [&](const std::pair<Zstring, Zstring>& entry)
        {
            Zstring tmp2 = entry.second; //case-insensitive "startsWith()"
            ::makeUpper(tmp2);           //
            if (startsWith(tmp, tmp2))
                output.push_back(L"%" + entry.first + L"%" + (dirname.c_str() + tmp2.size()));
        });
    }

    //5. replace (all) macros: //%USERPROFILE% -> C:\Users\username
    {
        wxString testMacros = toWx(dirname);
        expandMacros(testMacros);
        if (toZ(testMacros) != dirname)
            output.push_back(toZ(testMacros));
    }

    return output;
}
#endif


Zstring zen::getFormattedDirectoryName(const Zstring& dirname)
{
    //Formatting is needed since functions expect the directory to end with '\' to be able to split the relative names.
    //note: don't combine directory formatting with wxFileName, as it doesn't respect //?/ - prefix!

    wxString dirnameTmp = toWx(dirname);
    expandMacros(dirnameTmp);

    Zstring output = toZ(dirnameTmp);

    expandVolumeName(output);

    //remove leading/trailing whitespace
    trim(output, true, false);
    while (endsWith(output, " ")) //don't remove all whitespace from right, e.g. 0xa0 may be used as part of dir name
        output.resize(output.size() - 1);

    if (output.empty()) //an empty string will later be returned as "\"; this is not desired
        return Zstring();

    /*
    resolve relative names; required by:
    WINDOWS:
     - \\?\-prefix which needs absolute names
     - Volume Shadow Copy: volume name needs to be part of each filename
     - file icon buffer (at least for extensions that are actually read from disk, e.g. "exe")
     - ::SHFileOperation(): Using relative path names is not thread safe
    WINDOWS/LINUX:
     - detection of dependent directories, e.g. "\" and "C:\test"
     */
    output = resolveRelativePath(output);

    if (!output.EndsWith(FILE_NAME_SEPARATOR))
        output += FILE_NAME_SEPARATOR;

    return output;
}
