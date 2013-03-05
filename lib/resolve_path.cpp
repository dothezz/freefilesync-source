#include "resolve_path.h"
#include <set> //not necessarily included by <map>!
#include <map>
#include <zen/time.h>
#include <zen/thread.h>
#include <zen/utf.h>
#include <wx/utils.h> //wxGetEnv

#ifdef FFS_WIN
#include <zen/win.h> //includes "windows.h"
#include <Shlobj.h>
#include <zen/long_path_prefix.h>
#include <zen/file_handling.h>
#ifdef _MSC_VER
#pragma comment(lib, "Mpr.lib")
#endif

#elif defined FFS_LINUX || defined FFS_MAC
#include <stdlib.h> //getenv()
#endif

using namespace zen;


namespace
{
#ifdef FFS_WIN
Zstring resolveRelativePath(const Zstring& relativeName) //note: ::GetFullPathName() is documented not threadsafe!
{
    const DWORD bufferSize = 10000;
    std::vector<wchar_t> buffer(bufferSize);

    const DWORD charsWritten = ::GetFullPathName(applyLongPathPrefix(relativeName).c_str(), //__in   LPCTSTR lpFileName,
                                                 bufferSize, //__in   DWORD nBufferLength,
                                                 &buffer[0], //__out  LPTSTR lpBuffer,
                                                 nullptr);   //__out  LPTSTR *lpFilePart
    if (charsWritten == 0 || charsWritten >= bufferSize) //theoretically, charsWritten cannot be == "bufferSize"
        return relativeName; //ERROR! Don't do anything

    return removeLongPathPrefix(Zstring(&buffer[0], charsWritten)); //GetFullPathName() preserves long path prefix -> a low-level detail we don't want to leak out!
}

#elif defined FFS_LINUX || defined FFS_MAC
Zstring resolveRelativePath(const Zstring& relativeName)
{
    //http://linux.die.net/man/2/path_resolution
    if (!startsWith(relativeName, FILE_NAME_SEPARATOR)) //absolute names are exactly those starting with a '/'
    {
        /*
        basic support for '~': strictly speaking this is a shell-layer feature, so "realpath()" won't handle it
        http://www.gnu.org/software/bash/manual/html_node/Tilde-Expansion.html

        http://linux.die.net/man/3/getpwuid: An application that wants to determine its user's home directory
        should inspect the value of HOME (rather than the value getpwuid(getuid())->pw_dir) since this allows
        the user to modify their notion of "the home directory" during a login session.
        */
        if (startsWith(relativeName, "~/") || relativeName == "~")
        {
            const char* homeDir = ::getenv("HOME");
            if (!homeDir)
                return relativeName; //error! no further processing!

            if (startsWith(relativeName, "~/"))
                return appendSeparator(homeDir) + afterFirst(relativeName, '/');
            else if (relativeName == "~")
                return homeDir;
        }

        //we cannot use ::realpath() since it resolves *existing* relative paths only!
        std::vector<char> buffer(10000);
        if (::getcwd(&buffer[0], buffer.size()) != nullptr)
            return appendSeparator(&buffer[0]) + relativeName;
    }
    return relativeName;
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
            wchar_t buffer[MAX_PATH] = {};
            if (SUCCEEDED(::SHGetFolderPath(nullptr,                        //__in   HWND hwndOwner,
                                            csidl | CSIDL_FLAG_DONT_VERIFY, //__in   int nFolder,
                                            nullptr,				        //__in   HANDLE hToken,
                                            0 /* == SHGFP_TYPE_CURRENT*/,   //__in   DWORD dwFlags,
                                            buffer)))					  	//__out  LPTSTR pszPath
            {
                Zstring dirname = buffer;
                if (!dirname.empty())
                    csidlToDir.insert(std::make_pair(paramName, dirname));
            }
        };

        addCsidl(CSIDL_DESKTOPDIRECTORY,        L"csidl_Desktop");       // C:\Users\<user>\Desktop
        addCsidl(CSIDL_COMMON_DESKTOPDIRECTORY, L"csidl_PublicDesktop"); // C:\Users\All Users\Desktop

        addCsidl(CSIDL_FAVORITES,        L"csidl_Favorites");       // C:\Users\<user>\Favorites
        addCsidl(CSIDL_COMMON_FAVORITES, L"csidl_PublicFavorites"); // C:\Users\<user>\Favorites; unused? -> http://blogs.msdn.com/b/oldnewthing/archive/2012/09/04/10346022.aspx

        addCsidl(CSIDL_PERSONAL,         L"csidl_MyDocuments");     // C:\Users\<user>\Documents
        addCsidl(CSIDL_COMMON_DOCUMENTS, L"csidl_PublicDocuments"); // C:\Users\Public\Documents

        addCsidl(CSIDL_MYMUSIC,          L"csidl_MyMusic");         // C:\Users\<user>\Music
        addCsidl(CSIDL_COMMON_MUSIC,     L"csidl_PublicMusic");     // C:\Users\Public\Music

        addCsidl(CSIDL_MYPICTURES,       L"csidl_MyPictures");      // C:\Users\<user>\Pictures
        addCsidl(CSIDL_COMMON_PICTURES,  L"csidl_PublicPictures");  // C:\Users\Public\Pictures

        addCsidl(CSIDL_MYVIDEO,          L"csidl_MyVideos");        // C:\Users\<user>\Videos
        addCsidl(CSIDL_COMMON_VIDEO,     L"csidl_PublicVideos");    // C:\Users\Public\Videos

        addCsidl(CSIDL_NETHOOD,          L"csidl_Nethood");         // C:\Users\<user>\AppData\Roaming\Microsoft\Windows\Network Shortcuts

        addCsidl(CSIDL_PROGRAMS,         L"csidl_Programs");        // C:\Users\<user>\AppData\Roaming\Microsoft\Windows\Start Menu\Programs
        addCsidl(CSIDL_COMMON_PROGRAMS,  L"csidl_PublicPrograms");  // C:\ProgramData\Microsoft\Windows\Start Menu\Programs

        addCsidl(CSIDL_RESOURCES,        L"csidl_Resources");       // C:\Windows\Resources

        addCsidl(CSIDL_STARTMENU,        L"csidl_StartMenu");       // C:\Users\<user>\AppData\Roaming\Microsoft\Windows\Start Menu
        addCsidl(CSIDL_COMMON_STARTMENU, L"csidl_PublicStartMenu"); // C:\ProgramData\Microsoft\Windows\Start Menu

        addCsidl(CSIDL_STARTUP,          L"csidl_Startup");         // C:\Users\<user>\AppData\Roaming\Microsoft\Windows\Start Menu\Programs\StartUp
        addCsidl(CSIDL_COMMON_STARTUP,   L"csidl_PublicStartup");   // C:\ProgramData\Microsoft\Windows\Start Menu\Programs\StartUp

        addCsidl(CSIDL_TEMPLATES,        L"csidl_Templates");       // C:\Users\<user>\AppData\Roaming\Microsoft\Windows\Templates
        addCsidl(CSIDL_COMMON_TEMPLATES, L"csidl_PublicTemplates"); // C:\ProgramData\Microsoft\Windows\Templates

        /*
        CSIDL_APPDATA				covered by %AppData%
        CSIDL_LOCAL_APPDATA			covered by %LocalAppData% -> not on XP!
        CSIDL_COMMON_APPDATA		covered by %ProgramData%  -> not on XP!
        CSIDL_PROFILE				covered by %UserProfile%
        CSIDL_WINDOWS				covered by %WinDir%
        CSIDL_SYSTEM				covered by %WinDir%
        CSIDL_SYSTEMX86				covered by %WinDir%
        CSIDL_PROGRAM_FILES			covered by %ProgramFiles%
        CSIDL_PROGRAM_FILES_COMMON	covered by %CommonProgramFiles%
        CSIDL_PROGRAM_FILESX86			covered by %ProgramFiles(x86)%       -> not on XP!
        CSIDL_PROGRAM_FILES_COMMONX86	covered by %CommonProgramFiles(x86)% -> not on XP!
        CSIDL_ADMINTOOLS			not relevant?
        CSIDL_COMMON_ADMINTOOLS		not relevant?
        */
    }

    CsidlConstants(const CsidlConstants&);
    CsidlConstants& operator=(const CsidlConstants&);

    CsidlToDirMap csidlToDir;
};

//caveat: function scope static initialization is not thread-safe in VS 2010! => make sure to call at app start!
struct Dummy { Dummy() { CsidlConstants::get(); }} blah;
#endif


std::unique_ptr<Zstring> getEnvironmentVar(const Zstring& envName) //return nullptr if not found
{
    wxString value;
    if (!wxGetEnv(utfCvrtTo<wxString>(envName), &value))
        return nullptr;

    //some postprocessing:
    trim(value); //remove leading, trailing blanks

    //remove leading, trailing double-quotes
    if (startsWith(value, L"\"") &&
        endsWith  (value, L"\"") &&
        value.length() >= 2)
        value = wxString(value.c_str() + 1, value.length() - 2);

    return make_unique<Zstring>(utfCvrtTo<Zstring>(value));
}


std::unique_ptr<Zstring> resolveMacro(const Zstring& macro, //macro without %-characters
                                      const std::vector<std::pair<Zstring, Zstring>>& ext) //return nullptr if not resolved
{
    auto equalNoCase = [](const Zstring& lhs, const Zstring& rhs) { return utfCvrtTo<wxString>(lhs).CmpNoCase(utfCvrtTo<wxString>(rhs)) == 0; };

    //there exist environment variables named %TIME%, %DATE% so check for our internal macros first!
    if (equalNoCase(macro, Zstr("time")))
        return make_unique<Zstring>(formatTime<Zstring>(Zstr("%H%M%S")));

    if (equalNoCase(macro, Zstr("date")))
        return make_unique<Zstring>(formatTime<Zstring>(FORMAT_ISO_DATE));

    if (equalNoCase(macro, Zstr("timestamp")))
        return make_unique<Zstring>(formatTime<Zstring>(Zstr("%Y-%m-%d %H%M%S"))); //e.g. "2012-05-15 131513"

    std::unique_ptr<Zstring> cand;
    auto processPhrase = [&](const Zchar* phrase, const Zchar* format) -> bool
    {
        if (!equalNoCase(macro, phrase))
            return false;

        cand = make_unique<Zstring>(formatTime<Zstring>(format));
        return true;
    };

    if (processPhrase(Zstr("weekday"), Zstr("%A"))) return cand;
    if (processPhrase(Zstr("day"    ), Zstr("%d"))) return cand;
    if (processPhrase(Zstr("month"  ), Zstr("%m"))) return cand;
    if (processPhrase(Zstr("week"   ), Zstr("%U"))) return cand;
    if (processPhrase(Zstr("year"   ), Zstr("%Y"))) return cand;
    if (processPhrase(Zstr("hour"   ), Zstr("%H"))) return cand;
    if (processPhrase(Zstr("min"    ), Zstr("%M"))) return cand;
    if (processPhrase(Zstr("sec"    ), Zstr("%S"))) return cand;

    //check domain-specific extensions
    {
        auto it = std::find_if(ext.begin(), ext.end(), [&](const std::pair<Zstring, Zstring>& p) { return equalNoCase(macro, p.first); });
        if (it != ext.end())
            return make_unique<Zstring>(it->second);
    }

    //try to resolve as environment variable
    if (std::unique_ptr<Zstring> value = getEnvironmentVar(macro))
        return value;

#ifdef FFS_WIN
    //try to resolve as CSIDL value
    {
        const auto& csidlMap = CsidlConstants::get();
        auto it = csidlMap.find(macro);
        if (it != csidlMap.end())
            return make_unique<Zstring>(it->second);
    }
#endif

    return nullptr;
}

const Zchar MACRO_SEP = Zstr('%');

//returns expanded or original string
Zstring expandMacros(const Zstring& text, const std::vector<std::pair<Zstring, Zstring>>& ext)
{
    if (contains(text, MACRO_SEP))
    {
        Zstring prefix = beforeFirst(text, MACRO_SEP);
        Zstring rest   = afterFirst (text, MACRO_SEP);
        if (contains(rest, MACRO_SEP))
        {
            Zstring potentialMacro = beforeFirst(rest, MACRO_SEP);
            Zstring postfix        = afterFirst (rest, MACRO_SEP); //text == prefix + MACRO_SEP + potentialMacro + MACRO_SEP + postfix

            if (std::unique_ptr<Zstring> value = resolveMacro(potentialMacro, ext))
                return prefix + *value + expandMacros(postfix, ext);
            else
                return prefix + MACRO_SEP + potentialMacro + expandMacros(MACRO_SEP + postfix, ext);
        }
    }
    return text;
}
}


Zstring zen::expandMacros(const Zstring& text) { return ::expandMacros(text, std::vector<std::pair<Zstring, Zstring>>()); }


namespace
{
#ifdef FFS_WIN
//networks and cdrom excluded - this should not block
Zstring getPathByVolumenName(const Zstring& volumeName) //return empty string on error
{
    //FindFirstVolume(): traverses volumes on local hard disks only!
    //GetLogicalDriveStrings(): traverses all *logical* volumes, including CD-ROM, FreeOTFE virtual volumes

    const DWORD bufferSize = ::GetLogicalDriveStrings(0, nullptr);
    std::vector<wchar_t> buffer(bufferSize);

    const DWORD rv = ::GetLogicalDriveStrings(bufferSize,  //__in   DWORD nBufferLength,
                                              &buffer[0]); //__out  LPTSTR lpBuffer
    if (0 < rv && rv < bufferSize)
    {
        //search for matching path in parallel until first hit
        RunUntilFirstHit<Zstring> findFirstMatch;

        for (const wchar_t* it = &buffer[0]; *it != 0; it += strLength(it) + 1) //list terminated by empty c-string
        {
            const Zstring path = it;

            findFirstMatch.addJob([path, volumeName]() -> std::unique_ptr<Zstring>
            {
                UINT type = ::GetDriveType(path.c_str()); //non-blocking call!
                if (type == DRIVE_REMOTE || type == DRIVE_CDROM)
                    return nullptr;

                //next call seriously blocks for non-existing network drives!
                std::vector<wchar_t> volName(MAX_PATH + 1); //docu says so

                if (::GetVolumeInformation(path.c_str(),        //__in_opt   LPCTSTR lpRootPathName,
                &volName[0], //__out      LPTSTR lpVolumeNameBuffer,
                static_cast<DWORD>(volName.size()), //__in       DWORD nVolumeNameSize,
                nullptr,     //__out_opt  LPDWORD lpVolumeSerialNumber,
                nullptr,     //__out_opt  LPDWORD lpMaximumComponentLength,
                nullptr,     //__out_opt  LPDWORD lpFileSystemFlags,
                nullptr,     //__out      LPTSTR lpFileSystemNameBuffer,
                0))          //__in       DWORD nFileSystemNameSize
                    if (EqualFilename()(volumeName, Zstring(&volName[0])))
                        return zen::make_unique<Zstring>(path);
                return nullptr;
            });
        }
        if (auto result = findFirstMatch.get()) //blocks until ready
            return *result;
    }

    return Zstring();
}


//networks and cdrom excluded - this should not block
Zstring getVolumeName(const Zstring& volumePath) //return empty string on error
{
    UINT rv = ::GetDriveType(volumePath.c_str()); //non-blocking call!
    if (rv != DRIVE_REMOTE && rv != DRIVE_CDROM)
    {
        std::vector<wchar_t> buffer(MAX_PATH + 1);
        if (::GetVolumeInformation(volumePath.c_str(), //__in_opt   LPCTSTR lpRootPathName,
                                   &buffer[0],         //__out      LPTSTR lpVolumeNameBuffer,
                                   static_cast<DWORD>(buffer.size()), //__in       DWORD nVolumeNameSize,
                                   nullptr,            //__out_opt  LPDWORD lpVolumeSerialNumber,
                                   nullptr,            //__out_opt  LPDWORD lpMaximumComponentLength,
                                   nullptr,            //__out_opt  LPDWORD lpFileSystemFlags,
                                   nullptr,            //__out      LPTSTR lpFileSystemNameBuffer,
                                   0))                 //__in       DWORD nFileSystemNameSize
            return &buffer[0];
    }
    return Zstring();
}
#endif


//expand volume name if possible, return original input otherwise
Zstring expandVolumeName(const Zstring& text)  // [volname]:\folder       [volname]\folder       [volname]folder     -> C:\folder
{
    //this would be a nice job for a C++11 regex...

    //we only expect the [.*] pattern at the beginning => do not touch dir names like "C:\somedir\[stuff]"
    Zstring textTmp = text;
    trim(textTmp, true, false);

    if (startsWith(textTmp, Zstr("[")))
    {
        size_t posEnd = textTmp.find(Zstr("]"));
        if (posEnd != Zstring::npos)
        {
            Zstring volname = Zstring(textTmp.c_str() + 1, posEnd - 1);
            Zstring rest    = Zstring(textTmp.c_str() + posEnd + 1);

            if (startsWith(rest, Zstr(':')))
                rest = afterFirst(rest, Zstr(':'));
            if (startsWith(rest, FILE_NAME_SEPARATOR))
                rest = afterFirst(rest, FILE_NAME_SEPARATOR);
#ifdef FFS_WIN
            //[.*] pattern was found...
            if (!volname.empty())
            {
                Zstring volPath = getPathByVolumenName(volname); //should not block?!
                if (!volPath.empty())
                    return appendSeparator(volPath) + rest; //successfully replaced pattern
            }
            //error: did not find corresponding volume name:

            /*make sure directory creation will fail later if attempted, instead of inconveniently interpreting this string as a relative name!
                    [FFS USB]\FreeFileSync   will be resolved as
                 ?:\[FFS USB]\FreeFileSync\  - Windows
               /.../[FFS USB]/FreeFileSync/  - Linux
                            instead of:
               C:\Program Files\FreeFileSync\[FFS USB]\FreeFileSync\                                                                                        */
            return L"?:\\[" + volname + L"]\\" + rest;

#elif defined FFS_LINUX || defined FFS_MAC //neither supported nor needed
            return "/.../[" + volname + "]/" + rest;
#endif
        }
    }
    return text;
}
}


void getDirectoryAliasesRecursive(const Zstring& dirname, std::set<Zstring, LessFilename>& output)
{
#ifdef FFS_WIN
    //1. replace volume path by volume name: c:\dirname ->  [SYSTEM]\dirname
    if (dirname.size() >= 3 &&
        std::iswalpha(dirname[0]) &&
        dirname[1] == L':' &&
        dirname[2] == L'\\')
    {
        Zstring volname = getVolumeName(Zstring(dirname.c_str(), 3)); //should not block
        if (!volname.empty())
            output.insert(L"[" + volname + L"]" + Zstring(dirname.c_str() + 2));
    }

    //2. replace volume name by volume path: [SYSTEM]\dirname -> c:\dirname
    {
        Zstring testVolname = expandVolumeName(dirname); //should not block
        if (testVolname != dirname)
            if (output.insert(testVolname).second)
                getDirectoryAliasesRecursive(testVolname, output); //recurse!
    }
#endif

    //3. environment variables: C:\Users\username -> %USERPROFILE%
    {
        std::map<Zstring, Zstring> envToDir;

        //get list of useful variables
        auto addEnvVar = [&](const Zstring& envName)
        {
            if (std::unique_ptr<Zstring> value = getEnvironmentVar(envName))
                envToDir.insert(std::make_pair(envName, *value));
        };
#ifdef FFS_WIN
        addEnvVar(L"AllUsersProfile");  // C:\ProgramData
        addEnvVar(L"AppData");          // C:\Users\username\AppData\Roaming
        addEnvVar(L"LocalAppData");     // C:\Users\username\AppData\Local
        addEnvVar(L"ProgramData");      // C:\ProgramData
        addEnvVar(L"ProgramFiles");     // C:\Program Files
        addEnvVar(L"ProgramFiles(x86)");// C:\Program Files (x86)
        addEnvVar(L"CommonProgramFiles");      // C:\Program Files\Common Files
        addEnvVar(L"CommonProgramFiles(x86)"); // C:\Program Files (x86)\Common Files
        addEnvVar(L"Public");           // C:\Users\Public
        addEnvVar(L"UserProfile");      // C:\Users\username
        addEnvVar(L"WinDir");           // C:\Windows
        addEnvVar(L"Temp");             // C:\Windows\Temp

        //add CSIDL values: http://msdn.microsoft.com/en-us/library/bb762494(v=vs.85).aspx
        const auto& csidlMap = CsidlConstants::get();
        envToDir.insert(csidlMap.begin(), csidlMap.end());

#elif defined FFS_LINUX || defined FFS_MAC
        addEnvVar("HOME"); //Linux: /home/zenju  Mac: /Users/zenju
#endif
        //substitute paths by symbolic names
        auto pathStartsWith = [](const Zstring& path, const Zstring& prefix) -> bool
        {
#if defined FFS_WIN || defined FFS_MAC
            Zstring tmp = path;
            Zstring tmp2 = prefix;
            ::makeUpper(tmp);
            ::makeUpper(tmp2);
            return startsWith(tmp, tmp2);
#elif defined FFS_LINUX
            return startsWith(path, prefix);
#endif
        };
        std::for_each(envToDir.begin(), envToDir.end(),
                      [&](const std::pair<Zstring, Zstring>& entry)
        {
            if (pathStartsWith(dirname, entry.second))
                output.insert(MACRO_SEP + entry.first + MACRO_SEP + (dirname.c_str() + entry.second.size()));
        });
    }

    //4. replace (all) macros: %USERPROFILE% -> C:\Users\username
    {
        Zstring testMacros = expandMacros(dirname);
        if (testMacros != dirname)
            if (output.insert(testMacros).second)
                getDirectoryAliasesRecursive(testMacros, output); //recurse!
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


Zstring zen::getFormattedDirectoryName(const Zstring& dirString) // throw()
{
    //formatting is needed since functions expect the directory to end with '\' to be able to split the relative names.

    Zstring dirname = expandMacros(dirString);

    dirname = expandVolumeName(dirname); //should not block

    //remove leading/trailing whitespace
    trim(dirname, true, false);
    while (endsWith(dirname, Zstr(' '))) //don't remove all whitespace from right, e.g. 0xa0 may be used as part of dir name
        dirname.resize(dirname.size() - 1);

    if (dirname.empty()) //an empty string would later be resolved as "\"; this is not desired
        return Zstring();

    /*
    need to resolve relative paths:
    WINDOWS:
     - \\?\-prefix which needs absolute names
     - Volume Shadow Copy: volume name needs to be part of each filename
     - file icon buffer (at least for extensions that are actually read from disk, like "exe")
     - ::SHFileOperation(): Using relative path names is not thread safe
    WINDOWS/LINUX:
     - detection of dependent directories, e.g. "\" and "C:\test"
     */
    dirname = resolveRelativePath(dirname);

    return appendSeparator(dirname);
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
    ____________________________________________________________________________________________________________

    Windows Login Prompt Naming Conventions:
    	user account:	<Domain>\<user>		e.g. WIN-XP\ZenJu
    	network share:	\\<server>\<share>	e.g. \\WIN-XP\test

    Windows Command Line:
    - list *all* active network connections, including deviceless ones which are hidden in Explorer:
    		net use
    - delete active connection:
    		net use /delete \\server\share
    ____________________________________________________________________________________________________________

    Scenario: XP-shared folder is accessed by Win 7 over LAN with access limited to a certain user

    Problems:
    I.   WNetAddConnection2() allows (at least certain) invalid credentials (e.g. username: a/password: a) and establishes an *unusable* connection
    II.  WNetAddConnection2() refuses to overwrite an existing (unusable) connection created in I), but shows prompt repeatedly
    III. WNetAddConnection2() won't bring up the prompt if *wrong* credentials had been entered just recently, even with CONNECT_INTERACTIVE specified! => 2-step proccess
    */

    auto connect = [&](NETRESOURCE& trgRes) //blocks heavily if network is not reachable!!!
    {
        //1. first try to connect without user interaction - blocks!
        DWORD rv = ::WNetAddConnection2(&trgRes, // __in  LPNETRESOURCE lpNetResource,
                                        nullptr, // __in  LPCTSTR lpPassword,
                                        nullptr, // __in  LPCTSTR lpUsername,
                                        0);      //__in  DWORD dwFlags
        if (somethingExists(trgRes.lpRemoteName)) //blocks!
            return; //success: connection usable! -> don't care about "rv"

        if (rv == ERROR_BAD_NETPATH || //Windows 7
            rv == ERROR_BAD_NET_NAME)  //XP
            return; //no need to show a prompt for an unreachable network device

        //2. if first attempt failed, we need to *force* prompt by using CONNECT_PROMPT
        if (allowUserInteraction)
        {
            //avoid problem II.)
            DWORD rv2= WNetCancelConnection2(trgRes.lpRemoteName, //_In_  LPCTSTR lpName,
                                             0,					  //_In_  DWORD dwFlags,
                                             true);				  //_In_  BOOL fForce
            //enforce login prompt
            DWORD rv3 = ::WNetAddConnection2(&trgRes, // __in  LPNETRESOURCE lpNetResource,
                                             nullptr, // __in  LPCTSTR lpPassword,
                                             nullptr, // __in  LPCTSTR lpUsername,
                                             CONNECT_INTERACTIVE | CONNECT_PROMPT); //__in  DWORD dwFlags
            (void)rv2;
            (void)rv3;
            //Sample error codes:
            //53L	ERROR_BAD_NETPATH		The network path was not found.
            //86L	ERROR_INVALID_PASSWORD
            //1219L	ERROR_SESSION_CREDENTIAL_CONFLICT	Multiple connections to a server or shared resource by the same user, using more than one user name, are not allowed. Disconnect all previous connections to the server or shared resource and try again.
            //1326L	ERROR_LOGON_FAILURE	Logon failure: unknown user name or bad password.
        }
    };


    Zstring dirname = removeLongPathPrefix(dirnameOrig);
    trim(dirname, true, false);

    //1. locally mapped network share
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

                    connect(trgRes); //blocks!
                }
            }
        }
    }
    //2. deviceless network connection
    else if (startsWith(dirname, L"\\\\")) //UNC path
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

            connect(trgRes); //blocks!
        }
    }
}
#endif
