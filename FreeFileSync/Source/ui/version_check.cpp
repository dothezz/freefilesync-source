// *****************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under    *
// * GNU General Public License: http://www.gnu.org/licenses/gpl-3.0           *
// * Copyright (C) Zenju (zenju AT freefilesync DOT org) - All Rights Reserved *
// *****************************************************************************

#include "version_check.h"
#include <zen/string_tools.h>
#include <zen/i18n.h>
#include <zen/utf.h>
#include <zen/scope_guard.h>
#include <zen/build_info.h>
#include <zen/basic_math.h>
#include <zen/file_error.h>
#include <zen/thread.h> //std::thread::id
#include <wx+/popup_dlg.h>
#include <wx+/http.h>
#include <wx+/image_resources.h>
#include "../lib/ffs_paths.h"
#include "version_check_impl.h"


using namespace zen;


namespace
{


std::wstring getIso639Language()
{
    assert(std::this_thread::get_id() == mainThreadId); //this function is not thread-safe, consider wxWidgets usage

    const std::wstring localeName(wxLocale::GetLanguageCanonicalName(wxLocale::GetSystemLanguage()));
    if (localeName.empty())
        return std::wstring();

    assert(beforeLast(localeName, L"_", IF_MISSING_RETURN_ALL).size() == 2);
    return beforeLast(localeName, L"_", IF_MISSING_RETURN_ALL);
}


std::wstring getIso3166Country()
{
    assert(std::this_thread::get_id() == mainThreadId); //this function is not thread-safe, consider wxWidgets usage

    const std::wstring localeName(wxLocale::GetLanguageCanonicalName(wxLocale::GetSystemLanguage()));
    if (localeName.empty())
        return std::wstring();

    return afterLast(localeName, L"_", IF_MISSING_RETURN_NONE);
}


//coordinate with get_latest_version_number.php
std::vector<std::pair<std::string, std::string>> geHttpPostParameters()
{
    assert(std::this_thread::get_id() == mainThreadId); //this function is not thread-safe, e.g. consider wxWidgets usage in isPortableVersion()
    std::vector<std::pair<std::string, std::string>> params;

    params.emplace_back("ffs_version", utfCvrtTo<std::string>(zen::ffsVersion));
    params.emplace_back("ffs_type", isPortableVersion() ? "Portable" : "Local");

    params.emplace_back("os_name", "Linux");

    const wxLinuxDistributionInfo distribInfo = wxGetLinuxDistributionInfo();
    assert(contains(distribInfo.Release, L'.'));
    std::vector<wxString> digits = split<wxString>(distribInfo.Release, L'.'); //e.g. "15.04"
    digits.resize(2);
    //distribInfo.Id //e.g. "Ubuntu"

    const int osvMajor = stringTo<int>(digits[0]);
    const int osvMinor = stringTo<int>(digits[1]);

    params.emplace_back("os_version", numberTo<std::string>(osvMajor) + "." + numberTo<std::string>(osvMinor));

#ifdef ZEN_BUILD_32BIT
    params.emplace_back("os_arch", "32");
#elif defined ZEN_BUILD_64BIT
    params.emplace_back("os_arch", "64");
#endif

    const std::string isoLang    = utfCvrtTo<std::string>(getIso639Language());
    const std::string isoCountry = utfCvrtTo<std::string>(getIso3166Country());

    params.emplace_back("language", !isoLang   .empty() ? isoLang    : "zz");
    params.emplace_back("country" , !isoCountry.empty() ? isoCountry : "ZZ");

    return params;
}


enum GetVerResult
{
    GET_VER_SUCCESS,
    GET_VER_NO_CONNECTION, //no internet connection?
    GET_VER_PAGE_NOT_FOUND //version file seems to have moved! => trigger an update!
};

//access is thread-safe on Windows (WinInet), but not on Linux/OS X (wxWidgets)
GetVerResult getOnlineVersion(const std::vector<std::pair<std::string, std::string>>& postParams, std::wstring& version)
{
    try //harmonize with wxHTTP: get_latest_version_number.php must be accessible without https!!!
    {
        const std::string buffer = sendHttpPost(L"http://www.freefilesync.org/get_latest_version_number.php", L"FFS-Update-Check", postParams); //throw FileError
        version = utfCvrtTo<std::wstring>(buffer);
        trim(version);
        return version.empty() ? GET_VER_PAGE_NOT_FOUND : GET_VER_SUCCESS; //empty version possible??
    }
    catch (const FileError&)
    {
        return internetIsAlive() ? GET_VER_PAGE_NOT_FOUND : GET_VER_NO_CONNECTION;
    }
}


std::vector<size_t> parseVersion(const std::wstring& version)
{
    std::vector<size_t> output;
    for (const std::wstring& digit : split(version, FFS_VERSION_SEPARATOR))
        output.push_back(stringTo<size_t>(digit));
    return output;
}


std::wstring getOnlineChangelogDelta()
{
    try //harmonize with wxHTTP: get_latest_changes.php must be accessible without https!!!
    {
        const std::string buffer = sendHttpPost(L"http://www.freefilesync.org/get_latest_changes.php", L"FFS-Update-Check", { { "since", utfCvrtTo<std::string>(zen::ffsVersion) } }); //throw FileError
        return utfCvrtTo<std::wstring>(buffer);
    }
    catch (FileError&) { assert(false); return std::wstring(); }
}


void showUpdateAvailableDialog(wxWindow* parent, const std::wstring& onlineVersion, const std::wstring& onlineChangeLog)
{
    switch (showConfirmationDialog(parent, DialogInfoType::INFO, PopupDialogCfg().
                                   setIcon(getResourceImage(L"download_update")).
                                   setTitle(_("Check for Program Updates")).
                                   setMainInstructions(_("A new version of FreeFileSync is available:")  + L" " + onlineVersion + L"\n" + _("Download now?")).
                                   setDetailInstructions(onlineChangeLog),
                                   _("&Download")))
    {
        case ConfirmationButton::DO_IT:
            wxLaunchDefaultBrowser(L"http://www.freefilesync.org/get_latest.php");
            break;
        case ConfirmationButton::CANCEL:
            break;
    }
}
}


bool zen::haveNewerVersionOnline(const std::wstring& onlineVersion)
{
    std::vector<size_t> current = parseVersion(zen::ffsVersion);
    std::vector<size_t> online  = parseVersion(onlineVersion);

    if (online.empty() || online[0] == 0) //online version string may be "This website has been moved..." In this case better check for an update
        return true;

    return std::lexicographical_compare(current.begin(), current.end(),
                                        online .begin(), online .end());
}


bool zen::updateCheckActive(time_t lastUpdateCheck)
{
    return lastUpdateCheck != getVersionCheckInactiveId();
}


void zen::disableUpdateCheck(time_t& lastUpdateCheck)
{
    lastUpdateCheck = getVersionCheckInactiveId();
}


void zen::checkForUpdateNow(wxWindow* parent, std::wstring& lastOnlineVersion, std::wstring& lastOnlineChangeLog)
{
    std::wstring onlineVersion;
    switch (getOnlineVersion(geHttpPostParameters(), onlineVersion))
    {
        case GET_VER_SUCCESS:
            lastOnlineVersion   = onlineVersion;
            lastOnlineChangeLog = haveNewerVersionOnline(onlineVersion) ? getOnlineChangelogDelta() : L"";

            if (haveNewerVersionOnline(onlineVersion))
                showUpdateAvailableDialog(parent, lastOnlineVersion, lastOnlineChangeLog);
            else
                showNotificationDialog(parent, DialogInfoType::INFO, PopupDialogCfg().
                                       setTitle(_("Check for Program Updates")).
                                       setMainInstructions(_("FreeFileSync is up to date.")));
            break;

        case GET_VER_NO_CONNECTION:
            showNotificationDialog(parent, DialogInfoType::ERROR2, PopupDialogCfg().
                                   setTitle(_("Check for Program Updates")).
                                   setMainInstructions(replaceCpy(_("Unable to connect to %x."), L"%x", L"www.freefilesync.org.")));
            break;

        case GET_VER_PAGE_NOT_FOUND:
            lastOnlineVersion = L"Unknown";
            lastOnlineChangeLog.clear();

            switch (showConfirmationDialog(parent, DialogInfoType::ERROR2, PopupDialogCfg().
                                           setTitle(_("Check for Program Updates")).
                                           setMainInstructions(_("Cannot find current FreeFileSync version number online. A newer version is likely available. Check manually now?")),
                                           _("&Check")))
            {
                case ConfirmationButton::DO_IT:
                    wxLaunchDefaultBrowser(L"http://www.freefilesync.org/get_latest.php");
                    break;
                case ConfirmationButton::CANCEL:
                    break;
            }
            break;
    }
}


struct zen::UpdateCheckResultPrep
{
    const std::vector<std::pair<std::string, std::string>> postParameters { geHttpPostParameters() };
};

//run on main thread:
std::shared_ptr<UpdateCheckResultPrep> zen::periodicUpdateCheckPrepare()
{
    return nullptr;
}


struct zen::UpdateCheckResultAsync
{
};

//run on worker thread:
std::shared_ptr<UpdateCheckResultAsync> zen::periodicUpdateCheckRunAsync(const UpdateCheckResultPrep* resultPrep)
{
    return nullptr;
}


//run on main thread:
void zen::periodicUpdateCheckEval(wxWindow* parent, time_t& lastUpdateCheck, std::wstring& lastOnlineVersion, std::wstring& lastOnlineChangeLog, const UpdateCheckResultAsync* resultAsync)
{
    std::wstring onlineVersion;
    const GetVerResult versionStatus = getOnlineVersion(geHttpPostParameters(), onlineVersion);

    switch (versionStatus)
    {
        case GET_VER_SUCCESS:
            lastUpdateCheck     = getVersionCheckCurrentTime();
            lastOnlineVersion   = onlineVersion;
            lastOnlineChangeLog = haveNewerVersionOnline(onlineVersion) ? getOnlineChangelogDelta() : L"";

            if (haveNewerVersionOnline(onlineVersion))
                showUpdateAvailableDialog(parent, lastOnlineVersion, lastOnlineChangeLog);
            break;

        case GET_VER_NO_CONNECTION:
            break; //ignore this error

        case GET_VER_PAGE_NOT_FOUND:
            lastOnlineVersion = L"Unknown";
            lastOnlineChangeLog.clear();

            switch (showConfirmationDialog(parent, DialogInfoType::ERROR2, PopupDialogCfg().
                                           setTitle(_("Check for Program Updates")).
                                           setMainInstructions(_("Cannot find current FreeFileSync version number online. A newer version is likely available. Check manually now?")),
                                           _("&Check")))
            {
                case ConfirmationButton::DO_IT:
                    wxLaunchDefaultBrowser(L"http://www.freefilesync.org/get_latest.php");
                    break;
                case ConfirmationButton::CANCEL:
                    break;
            }
            break;
    }
}
