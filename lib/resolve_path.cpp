#include "resolve_path.h"
#include <wx/utils.h>
#include <zen/time.h>
#include <wx+/string_conv.h>
#include <map>
#include <set>
#include <zen/scope_guard.h>
#include <zen/thread.h>

#ifdef FFS_WIN
#include <zen/dll.h>
#include <Shlobj.h>
#include <zen/win.h> //includes "windows.h"
#include <zen/long_path_prefix.h>
#ifdef _MSC_VER
#pragma comment(lib, "Mpr.lib")
#endif

#elif defined FFS_LINUX
#include <zen/file_traverser.h>
#include <unistd.h>
#endif

using namespace zen;


namespace
{
#ifdef FFS_WIN
Zstring resolveRelativePath(Zstring relativeName) //note: ::GetFullPathName() is documented not threadsafe!
{
    const DWORD bufferSize = 10000;
    std::vector<Zchar> fullPath(bufferSize);

    const DWORD rv = ::GetFullPathName(applyLongPathPrefix(relativeName).c_str(), //__in   LPCTSTR lpFileName,
                                       bufferSize,   //__in   DWORD nBufferLength,
                                       &fullPath[0], //__out  LPTSTR lpBuffer,
                                       NULL);        //__out  LPTSTR *lpFilePart
    if (rv == 0 || rv >= bufferSize) //theoretically, rv can never be == bufferSize
        //ERROR! Don't do anything
        return relativeName;

    return Zstring(&fullPath[0], rv);
}

#elif defined FFS_LINUX
Zstring resolveRelativePath(const Zstring& relativeName)
{
    //unfortunately ::realpath only resolves *existing* relative paths, so we have resolve to absolute by ourselves

    //http://linux.die.net/man/2/path_resolution
    if (!startsWith(relativeName, FILE_NAME_SEPARATOR)) //absolute names are exactly those starting with a '/'
    {
        std::vector<char> buffer(10000);
        if (::getcwd(&buffer[0], buffer.size()) != NULL)
        {
            Zstring workingDir = &buffer[0];
            if (!endsWith(workingDir, FILE_NAME_SEPARATOR))
                workingDir += FILE_NAME_SEPARATOR;

            return workingDir + relativeName;
        }
    }
    return relativeName;

    /*
        char* absPath = ::realpath(relativeName.c_str(), NULL);
        if (!absPath)
            return relativeName; //ERROR! Don't do anything
        ZEN_ON_BLOCK_EXIT(::free(absPath));

        return Zstring(absPath);
        */
}
#endif


#ifdef FFS_WIN
class CsidlConstants
{
public:
    typedef std::map<Zstring, Zstring, LessFilename> CsidlToDirMap; //case-insensitive comparison

    static const CsidlToDirMap& get()
    {
        static CsidlConstants inst; //potential MT solved by intializing at startup, see below
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

//caveat: function scope static initialization is not thread-safe in VS 2010! => make sure to call at app start!
namespace
{
struct Dummy { Dummy() { CsidlConstants::get(); }} blah;
}
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
        macro = formatTime<wxString>(L"%H%M%S");
        return true;
    }

    if (macro.CmpNoCase(wxT("date")) == 0)
    {
        macro = formatTime<wxString>(FORMAT_ISO_DATE);
        return true;
    }

    auto processPhrase = [&](const wchar_t* phrase, const wchar_t* format) -> bool
    {
        if (macro.CmpNoCase(phrase) != 0)
            return false;

        macro = formatTime<wxString>(format);
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
    virtual std::shared_ptr<TraverseCallback> onDir(const Zchar* shortName, const Zstring& fullName)
    {
        devices_.insert(std::make_pair(shortName, fullName));
        return nullptr; //DON'T traverse into subdirs
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
        ZEN_ON_BLOCK_EXIT(::FindVolumeClose(hVol));

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
                    typedef	BOOL (WINAPI* GetVolumePathNamesForVolumeNameWFunc)(LPCWSTR lpszVolumeName,
                                                                                LPWCH lpszVolumePathNames,
                                                                                DWORD cchBufferLength,
                                                                                PDWORD lpcchReturnLength);

                    const SysDllFun<GetVolumePathNamesForVolumeNameWFunc> getVolumePathNamesForVolumeName(L"kernel32.dll", "GetVolumePathNamesForVolumeNameW");
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
//attention: this call may seriously block if network volume is not available!!!
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
    //this would be a nice job for a C++11 regex...


    //we only expect the [.*] pattern at the beginning => do not touch dir names like "C:\somedir\[stuff]"
    trim(text, true, false);

    if (startsWith(text, Zstr("[")))
    {
        size_t posEnd = text.find(Zstr("]"));
        if (posEnd != Zstring::npos)
        {
            Zstring volname = Zstring(text.c_str() + 1, posEnd - 1);
            Zstring after   = Zstring(text.c_str() + posEnd + 1);

            if (startsWith(after, Zstr(':')))
                after = afterFirst(after, Zstr(':'));
            if (startsWith(after, FILE_NAME_SEPARATOR))
                after = afterFirst(after, FILE_NAME_SEPARATOR);

            //[.*] pattern was found...
            if (!volname.empty())
            {
                Zstring volPath = volumenNameToPath(volname); //return empty string on error
                if (!volPath.empty())
                {
                    if (!endsWith(volPath, FILE_NAME_SEPARATOR))
                        volPath += FILE_NAME_SEPARATOR;

                    text = volPath + after;
                    //successfully replaced pattern
                    return;
                }
            }
            //error: did not find corresponding volume name:

            /*make sure directory creation will fail later if attempted, instead of inconveniently interpreting this string as a relative name!
                    [FFS USB]\FreeFileSync   will be resolved as
                 ?:\[FFS USB]\FreeFileSync\  - Windows
               /.../[FFS USB]/FreeFileSync/  - Linux
                            instead of:
               C:\Program Files\FreeFileSync\[FFS USB]\FreeFileSync\
                                                                                        */
#ifdef FFS_WIN
            text = L"?:\\[" + volname + L"]\\" + after;
#elif defined FFS_LINUX
            text = "/.../[" + volname + "]/" + after;
#endif
            return;
        }
    }
}
}


#ifdef FFS_WIN
void getDirectoryAliasesRecursive(const Zstring& dirname, std::set<Zstring, LessFilename>& output)
{
    //1. replace volume path by volume name: c:\dirname ->  [SYSTEM]\dirname
    if (dirname.size() >= 3 &&
        std::iswalpha(dirname[0]) &&
        dirname[1] == L':' &&
        dirname[2] == L'\\')
    {
        //attention: "volumePathToName()" will seriously block if network volume is not available!!!
        boost::unique_future<Zstring> futVolName = zen::async([=] { return volumePathToName(Zstring(dirname.c_str(), 3)); });
        if (futVolName.timed_wait(boost::posix_time::seconds(1)))
        {
            Zstring volname = futVolName.get();
            if (!volname.empty())
                output.insert(L"[" + volname + L"]" + Zstring(dirname.c_str() + 2));
        }
    }

    //2. replace volume name by volume path: [SYSTEM]\dirname -> c:\dirname
    {
        Zstring testVolname = dirname;
        expandVolumeName(testVolname);
        if (testVolname != dirname)
            if (output.insert(testVolname).second)
                getDirectoryAliasesRecursive(testVolname, output); //recurse!
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
                output.insert(L"%" + entry.first + L"%" + (dirname.c_str() + tmp2.size()));
        });
    }

    //4. replace (all) macros: %USERPROFILE% -> C:\Users\username
    {
        wxString testMacros = toWx(dirname);
        expandMacros(testMacros);
        if (toZ(testMacros) != dirname)
            if (output.insert(toZ(testMacros)).second)
                getDirectoryAliasesRecursive(toZ(testMacros), output); //recurse!
    }
}


std::vector<Zstring> zen::getDirectoryAliases(const Zstring& dirString)
{
    Zstring dirname = dirString;
    trim(dirname, true, false);
    if (dirname.empty())
        return std::vector<Zstring>();

    std::set<Zstring, LessFilename> tmp;
    getDirectoryAliasesRecursive(dirname, tmp);

    tmp.erase(dirname);
    tmp.erase(Zstring());

    return std::vector<Zstring>(tmp.begin(), tmp.end());
}
#endif


Zstring zen::getFormattedDirectoryName(const Zstring& dirString) // throw()
{
    //Formatting is needed since functions expect the directory to end with '\' to be able to split the relative names.

    wxString dirnameTmp = toWx(dirString);
    expandMacros(dirnameTmp);

    Zstring output = toZ(dirnameTmp);

    expandVolumeName(output);

    //remove leading/trailing whitespace
    trim(output, true, false);
    while (endsWith(output, " ")) //don't remove all whitespace from right, e.g. 0xa0 may be used as part of dir name
        output.resize(output.size() - 1);

    if (output.empty()) //an empty string would later be resolved as "\"; this is not desired
        return Zstring();

    /*
    resolve relative names; required by:
    WINDOWS:
     - \\?\-prefix which needs absolute names
     - Volume Shadow Copy: volume name needs to be part of each filename
     - file icon buffer (at least for extensions that are actually read from disk, like "exe")
     - ::SHFileOperation(): Using relative path names is not thread safe
    WINDOWS/LINUX:
     - detection of dependent directories, e.g. "\" and "C:\test"
     */
    output = resolveRelativePath(output);

    if (!endsWith(output, FILE_NAME_SEPARATOR))
        output += FILE_NAME_SEPARATOR;

    return output;
}


#ifdef FFS_WIN
void zen::loginNetworkShare(const Zstring& dirnameOrig, bool allowUserInteraction) //throw() - user interaction: show OS password prompt
{
    /*
    ATTENTION: it is not safe to retrieve UNC path via ::WNetGetConnection() for every type of network share:

    network type				 |::WNetGetConnection rv   | lpRemoteName				     | existing UNC path
    -----------------------------|-------------------------|---------------------------------|----------------
    inactive local network share | ERROR_CONNECTION_UNAVAIL| \\192.168.1.27\new2			 | YES
    WebDrive					 | NO_ERROR				   | \\Webdrive-ZenJu\GNU		     | NO
    Box.net (WebDav)			 | NO_ERROR				   | \\www.box.net\DavWWWRoot\dav    | YES
    NetDrive					 | ERROR_NOT_CONNECTED     | <empty>						 | NO
    */

    //if (::GetFileAttributes((driveLetter + L'\\').c_str()) == INVALID_FILE_ATTRIBUTES) <- this will seriously block if network is not reachable!!!

    const Zstring dirname = removeLongPathPrefix(dirnameOrig);

    //1. local path
    if (dirname.size() >= 2 && iswalpha(dirname[0]) && dirname[1] == L':')
    {
        Zstring driveLetter(dirname.c_str(), 2); //e.g.: "Q:"
        {
            DWORD bufferSize = 10000;
            std::vector<wchar_t> remoteNameBuffer(bufferSize);

            //map local -> remote

            //note: following function call does NOT block!
            DWORD rv = ::WNetGetConnection(driveLetter.c_str(),  //__in     LPCTSTR lpLocalName in the form "<driveletter>:"
                                           &remoteNameBuffer[0], //__out    LPTSTR lpRemoteName,
                                           &bufferSize);         //__inout  LPDWORD lpnLength
            if (rv == ERROR_CONNECTION_UNAVAIL) //remoteNameBuffer will be filled nevertheless!
            {
                //ERROR_CONNECTION_UNAVAIL: network mapping is existing, but not connected

                Zstring networkShare = &remoteNameBuffer[0];
                if (!networkShare.empty())
                {
                    NETRESOURCE trgRes = {};
                    trgRes.dwType = RESOURCETYPE_DISK;
                    trgRes.lpLocalName  = const_cast<LPWSTR>(driveLetter.c_str());  //lpNetResource is marked "__in", seems WNetAddConnection2 is not const correct!
                    trgRes.lpRemoteName = const_cast<LPWSTR>(networkShare.c_str()); //

                    //note: following function call may block heavily if network is not reachable!!!
                    DWORD rv2 = ::WNetAddConnection2(&trgRes, // __in  LPNETRESOURCE lpNetResource,
                                                     NULL,    // __in  LPCTSTR lpPassword,
                                                     NULL,    // __in  LPCTSTR lpUsername,
                                                     allowUserInteraction ? CONNECT_INTERACTIVE : 0); //__in  DWORD dwFlags
                    if (rv2 == NO_ERROR)
                        return; //mapping reestablished

                    //restoring connection failed for some reason...
                    //we could use full UNC path instead: networkShare + (dirname.c_str() + 2);  //replace "Q:\subdir" by "\\server\share\subdir"
                }
            }
        }
    }
    //2. UNC path
    else if (startsWith(dirname, L"\\\\"))
    {
        const Zstring networkShare = [&]() -> Zstring //extract prefix "\\server\share"
        {
            size_t pos = dirname.find('\\', 2);
            if (pos == Zstring::npos)
                return Zstring();
            pos = dirname.find('\\', pos + 1);
            return pos == Zstring::npos ? dirname : Zstring(dirname.c_str(), pos);
        }();

        if (!networkShare.empty())
        {
            //::WNetGetResourceInformation seems to fail with ERROR_BAD_NET_NAME even for existing unconnected network shares!
            // => unconditionally try to connect to network share, seems we cannot reliably detect connection status otherwise

            NETRESOURCE trgRes  = {};
            trgRes.dwType       = RESOURCETYPE_DISK;
            trgRes.lpRemoteName = const_cast<LPWSTR>(networkShare.c_str()); //trgRes is "__in"

            //note: following function call may block heavily if network is not reachable!!!
            DWORD rv2 = ::WNetAddConnection2(&trgRes, // __in  LPNETRESOURCE lpNetResource,
                                             NULL,    // __in  LPCTSTR lpPassword,
                                             NULL,    // __in  LPCTSTR lpUsername,
                                             allowUserInteraction ? CONNECT_INTERACTIVE : 0); //__in  DWORD dwFlags
            if (rv2 == NO_ERROR)
                return; //mapping reestablished

            /*
            NETRESOURCE nr  = {};
            nr.dwType       = RESOURCETYPE_DISK;
            nr.lpRemoteName = const_cast<LPWSTR>(networkShare.c_str()); //nr is "__in"

            DWORD bufferSize = sizeof(NETRESOURCE) + 20000;
            std::vector<char> buffer(bufferSize);

            LPTSTR relPath = NULL;

            //note: following function call may block heavily if network is not reachable!!!
            const DWORD rv = WNetGetResourceInformation(&nr,         // __in     LPNETRESOURCE lpNetResource,
                                                &buffer[0],  // __out    LPVOID lpBuffer,
                                                &bufferSize, // __inout  LPDWORD lpcbBuffer,
                                                &relPath);   // __out    LPTSTR *lplpSystem
            if (rv == NO_ERROR)
            {
            //NO_ERROR: network share is existing, *either* connected or disconnected

            //we have no way to check if network is already connected, so let's try to connect anyway:

            NETRESOURCE& trgRes = reinterpret_cast<NETRESOURCE&>(buffer[0]);

            if (trgRes.dwUsage & RESOURCEUSAGE_CONNECTABLE)
            {
            //note: following function call may block heavily if network is not reachable!!!
            DWORD rv2 = ::WNetAddConnection2(&trgRes, // __in  LPNETRESOURCE lpNetResource,
                                             NULL,    // __in  LPCTSTR lpPassword,
                                             NULL,    // __in  LPCTSTR lpUsername,
                                             allowUserInteraction ? CONNECT_INTERACTIVE : 0); //__in  DWORD dwFlags
            if (rv2 == NO_ERROR)
                return; //mapping reestablished
            }
            }
            */
        }
    }
}
#endif
