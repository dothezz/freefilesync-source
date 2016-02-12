// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl-3.0        *
// * Copyright (C) Zenju (zenju AT gmx DOT de) - All Rights Reserved        *
// **************************************************************************

#include "version_check.h"
#include <zen/string_tools.h>
#include <zen/i18n.h>
#include <zen/utf.h>
#include <zen/scope_guard.h>
#include <zen/build_info.h>
#include <zen/basic_math.h>
#include <zen/file_error.h>
#include <wx+/popup_dlg.h>
#include "version_id.h"


    #include <zen/thread.h> //std::thread::id
    #include <wx/protocol/http.h>
    #include <wx/app.h>


using namespace zen;


namespace
{


std::wstring getIso639Language()
{
    //respect thread-safety for WinInetAccess => don't use wxWidgets in the Windows build here!!!

    assert(std::this_thread::get_id() == mainThreadId);

    const std::wstring localeName(wxLocale::GetLanguageCanonicalName(wxLocale::GetSystemLanguage()));
    if (localeName.empty())
        return std::wstring();

    assert(beforeLast(localeName, L"_", IF_MISSING_RETURN_ALL).size() == 2);
    return beforeLast(localeName, L"_", IF_MISSING_RETURN_ALL);
}


std::wstring getIso3166Country()
{
    //respect thread-safety for WinInetAccess => don't use wxWidgets in the Windows build here!!!

    assert(std::this_thread::get_id() == mainThreadId);

    const std::wstring localeName(wxLocale::GetLanguageCanonicalName(wxLocale::GetSystemLanguage()));
    if (localeName.empty())
        return std::wstring();

    return afterLast(localeName, L"_", IF_MISSING_RETURN_NONE);
}


std::string geHttpPostParameters() //must be in application/x-www-form-urlencoded format!!!
{
    //1. coordinate with get_latest_version_number.php
    //2. respect thread-safety for WinInetAccess => don't use wxWidgets in the Windows build here!!!

    std::string params = "ffs_version=" + utfCvrtTo<std::string>(zen::ffsVersion);

    params += "&os_name=Linux";
    assert(std::this_thread::get_id() == mainThreadId);

    const wxLinuxDistributionInfo distribInfo = wxGetLinuxDistributionInfo();
    assert(contains(distribInfo.Release, L'.'));
    std::vector<wxString> digits = split<wxString>(distribInfo.Release, L'.'); //e.g. "15.04"
    digits.resize(2);
    //distribInfo.Id //e.g. "Ubuntu"

    const int osvMajor = stringTo<int>(digits[0]);
    const int osvMinor = stringTo<int>(digits[1]);

    params += "&os_version=" + numberTo<std::string>(osvMajor) + "." + numberTo<std::string>(osvMinor);

    params +=
#ifdef ZEN_BUILD_32BIT
        "&os_arch=32";
#elif defined ZEN_BUILD_64BIT
        "&os_arch=64";
#endif

    const std::string isoLang    = utfCvrtTo<std::string>(getIso639Language());
    const std::string isoCountry = utfCvrtTo<std::string>(getIso3166Country());
    params += "&language=" + (!isoLang   .empty() ? isoLang    : "zz");
    params += "&country="  + (!isoCountry.empty() ? isoCountry : "ZZ");

    return params;
}


std::string sendHttpRequestImpl(const std::wstring& url, //throw FileError
                                const std::string* postParams, //issue POST if bound, GET otherwise
                                int level = 0)
{
    assert(!startsWith(url, L"https:")); //not supported by wxHTTP!
    std::wstring urlFmt = url;
    if (startsWith(urlFmt, L"http://"))
        urlFmt = afterFirst(urlFmt, L"://", IF_MISSING_RETURN_NONE);
    const std::wstring server =       beforeFirst(urlFmt, L'/', IF_MISSING_RETURN_ALL);
    const std::wstring page   = L'/' + afterFirst(urlFmt, L'/', IF_MISSING_RETURN_NONE);

    assert(std::this_thread::get_id() == mainThreadId);
    assert(wxApp::IsMainLoopRunning());

    wxHTTP webAccess;
    webAccess.SetHeader(L"User-Agent", L"FFS-Update-Check");
    webAccess.SetTimeout(10 /*[s]*/); //default: 10 minutes: WTF are these wxWidgets people thinking???

    if (!webAccess.Connect(server)) //will *not* fail for non-reachable url here!
        throw FileError(_("Internet access failed."), L"wxHTTP::Connect");

    if (postParams)
        if (!webAccess.SetPostText(L"application/x-www-form-urlencoded", utfCvrtTo<wxString>(*postParams)))
            throw FileError(_("Internet access failed."), L"wxHTTP::SetPostText");

    std::unique_ptr<wxInputStream> httpStream(webAccess.GetInputStream(page)); //must be deleted BEFORE webAccess is closed
    const int sc = webAccess.GetResponse();

    //http://en.wikipedia.org/wiki/List_of_HTTP_status_codes#3xx_Redirection
    if (sc / 100 == 3) //e.g. 301, 302, 303, 307... we're not too greedy since we check location, too!
    {
        if (level < 5) //"A user agent should not automatically redirect a request more than five times, since such redirections usually indicate an infinite loop."
        {
            const std::wstring newUrl(webAccess.GetHeader(L"Location"));
            if (!newUrl.empty())
                return sendHttpRequestImpl(newUrl, postParams, level + 1);
        }
        throw FileError(_("Internet access failed."), L"Unresolvable redirect.");
    }

    if (sc != 200) //HTTP_STATUS_OK
        throw FileError(_("Internet access failed."), replaceCpy<std::wstring>(L"HTTP status code %x.", L"%x", numberTo<std::wstring>(sc)));

    if (!httpStream || webAccess.GetError() != wxPROTO_NOERR)
        throw FileError(_("Internet access failed."), L"wxHTTP::GetError");

    std::string buffer;
    int newValue = 0;
    while ((newValue = httpStream->GetC()) != wxEOF)
        buffer.push_back(static_cast<char>(newValue));
    return buffer;
}


bool internetIsAlive() //noexcept
{
    const wxString server = L"www.google.com";
    const wxString page   = L"/";

    wxHTTP webAccess;
    webAccess.SetTimeout(10 /*[s]*/); //default: 10 minutes: WTF are these wxWidgets people thinking???

    if (!webAccess.Connect(server)) //will *not* fail for non-reachable url here!
        return false;

    std::unique_ptr<wxInputStream> httpStream(webAccess.GetInputStream(page)); //call before checking wxHTTP::GetResponse()
    const int sc = webAccess.GetResponse();
    //attention: http://www.google.com/ might redirect to "https" => don't follow, just return "true"!!!
    return sc / 100 == 2 || //e.g. 200
           sc / 100 == 3;   //e.g. 301, 302, 303, 307... when in doubt, consider internet alive!
}

#if 0 //not needed yet: -Wunused-function on OS X
inline
std::string sendHttpGet(const std::wstring& url) //throw FileError
{
    return sendHttpRequestImpl(url, nullptr); //throw FileError
}
#endif

inline
std::string sendHttpPost(const std::wstring& url, const std::string& postParams) //throw FileError
{
    return sendHttpRequestImpl(url, &postParams); //throw FileError
}


enum GetVerResult
{
    GET_VER_SUCCESS,
    GET_VER_NO_CONNECTION, //no internet connection?
    GET_VER_PAGE_NOT_FOUND //version file seems to have moved! => trigger an update!
};

//access is thread-safe on Windows (WinInet), but not on Linux/OS X (wxWidgets)
GetVerResult getOnlineVersion(std::wstring& version)
{
    try
    {
        //harmonize with wxHTTP: get_latest_version_number.php must be accessible without https!!!
        const std::string buffer = sendHttpPost(L"http://www.freefilesync.org/get_latest_version_number.php", geHttpPostParameters()); //throw FileError
        version = utfCvrtTo<std::wstring>(buffer);
        trim(version); //Windows: remove trailing blank and newline
        return version.empty() ? GET_VER_PAGE_NOT_FOUND : GET_VER_SUCCESS; //empty version possible??
    }
    catch (const FileError&)
    {
        return internetIsAlive() ? GET_VER_PAGE_NOT_FOUND : GET_VER_NO_CONNECTION;
    }
}


std::vector<size_t> parseVersion(const std::wstring& version)
{
    std::vector<std::wstring> digits = split(version, FFS_VERSION_SEPARATOR);

    std::vector<size_t> output;
    std::transform(digits.begin(), digits.end(), std::back_inserter(output), [&](const std::wstring& d) { return stringTo<size_t>(d); });
    return output;
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
    return lastUpdateCheck != getInactiveCheckId();
}


void zen::disableUpdateCheck(time_t& lastUpdateCheck)
{
    lastUpdateCheck = getInactiveCheckId();
}


void zen::checkForUpdateNow(wxWindow* parent, std::wstring& lastOnlineVersion)
{
    std::wstring onlineVersion;
    switch (getOnlineVersion(onlineVersion))
    {
        case GET_VER_SUCCESS:
            lastOnlineVersion = onlineVersion;
            if (haveNewerVersionOnline(onlineVersion))
            {
                switch (showConfirmationDialog(parent, DialogInfoType::INFO, PopupDialogCfg().
                                               setTitle(_("Check for Program Updates")).
                                               setMainInstructions(_("A new version of FreeFileSync is available:")  + L" " + onlineVersion + L"\n\n" + _("Download now?")),
                                               _("&Download")))
                {
                    case ConfirmationButton::DO_IT:
                        wxLaunchDefaultBrowser(L"http://www.freefilesync.org/get_latest.php");
                        break;
                    case ConfirmationButton::CANCEL:
                        break;
                }
            }
            else
                showNotificationDialog(parent, DialogInfoType::INFO, PopupDialogCfg().
                                       setTitle(_("Check for Program Updates")).
                                       setMainInstructions(_("FreeFileSync is up to date.")));
            break;

        case GET_VER_NO_CONNECTION:
            showNotificationDialog(parent, DialogInfoType::ERROR2, PopupDialogCfg().
                                   setTitle(_("Check for Program Updates")).
                                   setMainInstructions(_("Unable to connect to www.freefilesync.org.")));
            break;

        case GET_VER_PAGE_NOT_FOUND:
            lastOnlineVersion = L"Unknown";
            switch (showConfirmationDialog(parent, DialogInfoType::ERROR2, PopupDialogCfg().
                                           setTitle(_("Check for Program Updates")).
                                           setMainInstructions(_("Cannot find current FreeFileSync version number online. Do you want to check manually?")),
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


bool zen::shouldRunPeriodicUpdateCheck(time_t lastUpdateCheck)
{
    if (updateCheckActive(lastUpdateCheck))
    {
        const time_t now = std::time(nullptr);
        return numeric::dist(now, lastUpdateCheck) >= 7 * 24 * 3600; //check weekly
    }
    return false;
}


struct zen::UpdateCheckResult
{
};


std::shared_ptr<UpdateCheckResult> zen::retrieveOnlineVersion()
{
    return nullptr;
}


void zen::evalPeriodicUpdateCheck(wxWindow* parent, time_t& lastUpdateCheck, std::wstring& lastOnlineVersion, const UpdateCheckResult* result)
{
    std::wstring onlineVersion;
    const GetVerResult versionStatus = getOnlineVersion(onlineVersion);

    switch (versionStatus)
    {
        case GET_VER_SUCCESS:
            lastUpdateCheck = std::time(nullptr);
            lastOnlineVersion = onlineVersion;

            if (haveNewerVersionOnline(onlineVersion))
            {
                switch (showConfirmationDialog(parent, DialogInfoType::INFO, PopupDialogCfg().
                                               setTitle(_("Check for Program Updates")).
                                               setMainInstructions(_("A new version of FreeFileSync is available:")  + L" " + onlineVersion + L"\n\n" + _("Download now?")),
                                               _("&Download")))
                {
                    case ConfirmationButton::DO_IT:
                        wxLaunchDefaultBrowser(L"http://www.freefilesync.org/get_latest.php");
                        break;
                    case ConfirmationButton::CANCEL:
                        break;
                }
            }
            break;

        case GET_VER_NO_CONNECTION:
            break; //ignore this error

        case GET_VER_PAGE_NOT_FOUND:
            lastOnlineVersion = L"Unknown";
            switch (showConfirmationDialog(parent, DialogInfoType::ERROR2, PopupDialogCfg().
                                           setTitle(_("Check for Program Updates")).
                                           setMainInstructions(_("Cannot find current FreeFileSync version number online. Do you want to check manually?")),
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
