#include "resolve_path.h"
#include <boost/scoped_array.hpp>
#include <wx/utils.h>
#include <wx/datetime.h>
#include "string_conv.h"
#include "system_constants.h"
#include "loki/ScopeGuard.h"

#ifdef FFS_WIN
#include "dll_loader.h"
#include <wx/msw/wrapwin.h> //includes "windows.h"
#include "long_path_prefix.h"

#elif defined FFS_LINUX
#include <map>
#include "file_traverser.h"
#include "stdlib.h"
#endif

using namespace ffs3;
using namespace common;


namespace
{
#ifdef FFS_WIN
Zstring resolveRelativePath(const Zstring& relativeName, DWORD proposedBufferSize = 1000)
{
    boost::scoped_array<Zchar> fullPath(new Zchar[proposedBufferSize]);
    const DWORD rv = ::GetFullPathName(
                         applyLongPathPrefix(relativeName).c_str(), //__in   LPCTSTR lpFileName,
                         proposedBufferSize, //__in   DWORD nBufferLength,
                         fullPath.get(),     //__out  LPTSTR lpBuffer,
                         NULL);              //__out  LPTSTR *lpFilePart
    if (rv == 0 || rv == proposedBufferSize)
        //ERROR! Don't do anything
        return relativeName;
    if (rv > proposedBufferSize)
        return resolveRelativePath(relativeName, rv);

    return fullPath.get();
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

    if (macro.CmpNoCase(wxT("weekday")) == 0)
    {
        macro = wxDateTime::Now().Format(wxT("%A"));
        return true;
    }

    if (macro.CmpNoCase(wxT("month")) == 0)
    {
        macro = wxDateTime::Now().Format(wxT("%B"));
        return true;
    }

    if (macro.CmpNoCase(wxT("week")) == 0)
    {
        macro = wxDateTime::Now().Format(wxT("%U"));
        return true;
    }

    if (macro.CmpNoCase(wxT("year")) == 0)
    {
        macro = wxDateTime::Now().Format(wxT("%Y"));
        return true;
    }

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
class TraverseMedia : public ffs3::TraverseCallback
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
    virtual void onError(const wxString& errorText) {}

private:
    DeviceList& devices_;
};
#endif


Zstring getVolumePath(const Zstring& volumeName) //empty string on error
{
#ifdef FFS_WIN
    const size_t volGuidSize = 10000;
    boost::scoped_array<wchar_t> volGuid(new wchar_t[volGuidSize]);

    HANDLE hVol = ::FindFirstVolume(volGuid.get(), volGuidSize);
    if (hVol != INVALID_HANDLE_VALUE)
    {
        Loki::ScopeGuard dummy = Loki::MakeGuard(::FindVolumeClose, hVol);
        (void)dummy;

        do
        {
            const size_t volNameSize = MAX_PATH + 1;
            boost::scoped_array<wchar_t> volName(new wchar_t[volNameSize]);

            if (::GetVolumeInformation(volGuid.get(), //__in_opt   LPCTSTR lpRootPathName,
                                       volName.get(), //__out      LPTSTR lpVolumeNameBuffer,
                                       volNameSize,   //__in       DWORD nVolumeNameSize,
                                       NULL,          //__out_opt  LPDWORD lpVolumeSerialNumber,
                                       NULL,          //__out_opt  LPDWORD lpMaximumComponentLength,
                                       NULL,          //__out_opt  LPDWORD lpFileSystemFlags,
                                       NULL,          //__out      LPTSTR lpFileSystemNameBuffer,
                                       0))            //__in       DWORD nFileSystemNameSize
            {
                if (EqualFilename()(volumeName, Zstring(volName.get())))
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
                        const DWORD volPathSize = 10000;
                        boost::scoped_array<wchar_t> volPath(new wchar_t[volPathSize]);

                        DWORD returnedLen = 0;
                        if (getVolumePathNamesForVolumeName(volGuid.get(), //__in   LPCTSTR lpszVolumeName,
                                                            volPath.get(), //__out  LPTSTR lpszVolumePathNames,
                                                            volPathSize,   //__in   DWORD cchBufferLength,
                                                            &returnedLen)) //__out  PDWORD lpcchReturnLength
                        {
                            return volPath.get(); //return first path name in double-null terminated list!
                        }
                    }
                    return volGuid.get(); //GUID looks ugly, but should be working correctly
                }
            }
        }
        while (::FindNextVolume(hVol, volGuid.get(), volGuidSize));
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

            if (after.StartsWith(Zstr(":")))
                after = after.AfterFirst(Zstr(':'));
            if (after.StartsWith(Zstring() + FILE_NAME_SEPARATOR))
                after = after.AfterFirst(FILE_NAME_SEPARATOR);
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


Zstring ffs3::getFormattedDirectoryName(const Zstring& dirname)
{
    //Formatting is needed since functions expect the directory to end with '\' to be able to split the relative names.
    //note: don't combine directory formatting with wxFileName, as it doesn't respect //?/ - prefix!

    wxString dirnameTmp = zToWx(dirname);
    expandMacros(dirnameTmp);

    Zstring output = wxToZ(dirnameTmp);

    expandVolumeName(output);

    output.Trim();

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
