#include "resolve_path.h"
#include <wx/utils.h>
#include <wx/datetime.h>
#include "string_conv.h"
#include "loki/ScopeGuard.h"

#ifdef FFS_WIN
#include "dll_loader.h"
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
            if (rv == NO_ERROR ||
				 rv == ERROR_CONNECTION_UNAVAIL) //remoteNameBuffer will be filled nevertheless!
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
    wxString envValue;
    if (wxGetEnv(macro, &envValue))
    {
        macro = envValue;

        //some postprocessing:
        macro.Trim(true);  //remove leading, trailing blanks
        macro.Trim(false); //

        //remove leading, trailing double-quotes
        if (macro.StartsWith(wxT("\"")) &&
            macro.EndsWith(wxT("\"")) &&
            macro.length() >= 2)
            macro = wxString(macro.c_str() + 1, macro.length() - 2);
        return true;
    }

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


Zstring getVolumePath(const Zstring& volumeName) //empty string on error
{
#ifdef FFS_WIN
    std::vector<wchar_t> volGuid(10000);

    HANDLE hVol = ::FindFirstVolume(&volGuid[0], static_cast<DWORD>(volGuid.size()));
    if (hVol != INVALID_HANDLE_VALUE)
    {
        Loki::ScopeGuard dummy = Loki::MakeGuard(::FindVolumeClose, hVol);
        (void)dummy;

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

                    static const GetVolumePathNamesForVolumeNameWFunc getVolumePathNamesForVolumeName =
                        util::getDllFun<GetVolumePathNamesForVolumeNameWFunc>(L"kernel32.dll", "GetVolumePathNamesForVolumeNameW");

                    if (getVolumePathNamesForVolumeName != NULL)
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

    Zstring volPath = getVolumePath(volname); //return empty string on error
    if (volPath.empty())
        return;

    if (!volPath.EndsWith(FILE_NAME_SEPARATOR))
        volPath += FILE_NAME_SEPARATOR;

    text = before + volPath + after;
}
}


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
