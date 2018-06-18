// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl-3.0        *
// * Copyright (C) Zenju (zenju AT gmx DOT de) - All Rights Reserved        *
// **************************************************************************

#include "check_version.h"
#include <zen/string_tools.h>
#include <zen/i18n.h>
#include <zen/utf.h>
#include <zen/scope_guard.h>
#include <zen/build_info.h>
#include <wx/timer.h>
#include <wx/utils.h>
#include <wx+/popup_dlg.h>
#include "../version/version.h"

#ifdef ZEN_WIN
    #include <zen/win.h> //tame wininet include
    #include <zen/win_ver.h>
    #include <wininet.h>

#elif defined ZEN_LINUX
    #include <wx/protocol/http.h>
    #include <wx/sstream.h>
#elif defined ZEN_MAC
    #include <wx/protocol/http.h>
    #include <wx/sstream.h>
    #include <CoreServices/CoreServices.h> //Gestalt()
#endif

using namespace zen;


namespace
{
std::wstring getIso639Language()
{
#ifdef ZEN_WIN //use a more reliable function than wxWidgets:
    const int bufSize = 10;
    wchar_t buf[bufSize] = {};
    int rv = ::GetLocaleInfo(LOCALE_USER_DEFAULT,   //_In_       LCID Locale,
                             LOCALE_SISO639LANGNAME,//_In_       LCTYPE LCType,
                             buf,                   //_Out_opt_  LPTSTR lpLCData,
                             bufSize);              //_In_       int cchData
    if (0 < rv && rv < bufSize)
        return buf; //MSDN: "This can be a 3-letter code for languages that don't have a 2-letter code"!
    else assert(false);
#endif
    const std::wstring localeName(wxLocale::GetLanguageCanonicalName(wxLocale::GetSystemLanguage()));
    if (localeName.empty())
        return std::wstring();

    assert(beforeLast(localeName, L"_", IF_MISSING_RETURN_ALL).size() == 2);
    return beforeLast(localeName, L"_", IF_MISSING_RETURN_ALL);
}

std::wstring getIso3166Country()
{
#ifdef ZEN_WIN //use a more reliable function than wxWidgets:
    const int bufSize = 10;
    wchar_t buf[bufSize] = {};
    int rv = ::GetLocaleInfo(LOCALE_USER_DEFAULT,    //_In_       LCID Locale,
                             LOCALE_SISO3166CTRYNAME,//_In_       LCTYPE LCType,
                             buf,                    //_Out_opt_  LPTSTR lpLCData,
                             bufSize);               //_In_       int cchData
    if (0 < rv && rv < bufSize)
        return buf; //MSDN: "This can also return a number, such as "029" for Caribbean."!
    else assert(false);
#endif
    const std::wstring localeName(wxLocale::GetLanguageCanonicalName(wxLocale::GetSystemLanguage()));
    if (localeName.empty())
        return std::wstring();

    return afterLast(localeName, L"_", IF_MISSING_RETURN_NONE);
}


std::wstring getUserAgentName() //coordinate with on_check_latest_version.php
{
    std::wstring agentName = std::wstring(L"FreeFileSync (") + zen::ffsVersion;

#ifdef ZEN_WIN
    agentName += L" Windows";
    const auto osvMajor = getOsVersion().major;
    const auto osvMinor = getOsVersion().minor;

#elif defined ZEN_LINUX
    int osvMajor = 0;
    int osvMinor = 0;
    agentName += L" Linux";

#elif defined ZEN_MAC
    agentName += L" Mac";
    SInt32 osvMajor = 0;
    SInt32 osvMinor = 0;
    ::Gestalt(gestaltSystemVersionMajor, &osvMajor);
    ::Gestalt(gestaltSystemVersionMinor, &osvMinor);
#endif
    agentName += L" " + numberTo<std::wstring>(osvMajor) + L"." + numberTo<std::wstring>(osvMinor);

    agentName +=
#ifdef ZEN_WIN
        running64BitWindows()
#elif defined ZEN_LINUX || defined ZEN_MAC
        zen::is64BitBuild
#endif
        ? L" 64" : L" 32";

    const std::wstring isoLang    = getIso639Language();
    const std::wstring isoCountry = getIso3166Country();
    agentName += L" " + (!isoLang   .empty() ? isoLang    : L"zz");
    agentName += L" " + (!isoCountry.empty() ? isoCountry : L"ZZ");

    agentName += L")";
    return agentName;
}

#ifdef ZEN_WIN
class InternetConnectionError {};

class WinInetAccess //1. uses IE proxy settings! :) 2. follows HTTP redirects by default
{
public:
    WinInetAccess(const wchar_t* url) //throw InternetConnectionError (if url cannot be reached; no need to also call readBytes())
    {
        //::InternetAttemptConnect(0) -> not working as expected: succeeds even when there is no internet connection!

        hInternet = ::InternetOpen(getUserAgentName().c_str(),   //_In_  LPCTSTR lpszAgent,
                                   INTERNET_OPEN_TYPE_PRECONFIG, //_In_  DWORD dwAccessType,
                                   nullptr,	//_In_  LPCTSTR lpszProxyName,
                                   nullptr,	//_In_  LPCTSTR lpszProxyBypass,
                                   0);		//_In_  DWORD dwFlags
        if (!hInternet)
            throw InternetConnectionError();
        zen::ScopeGuard guardInternet = zen::makeGuard([&] { ::InternetCloseHandle(hInternet); });

        hRequest = ::InternetOpenUrl(hInternet, //_In_  HINTERNET hInternet,
                                     url,		//_In_  LPCTSTR lpszUrl,
                                     nullptr,   //_In_  LPCTSTR lpszHeaders,
                                     0,			//_In_  DWORD dwHeadersLength,
                                     INTERNET_FLAG_KEEP_CONNECTION | INTERNET_FLAG_RELOAD | INTERNET_FLAG_NO_UI, //_In_  DWORD dwFlags,
                                     0);		  //_In_  DWORD_PTR dwContext
        if (!hRequest) //won't fail due to unreachable url here! There is no substitute for HTTP_QUERY_STATUS_CODE!!!
            throw InternetConnectionError();
        zen::ScopeGuard guardRequest = zen::makeGuard([&] { ::InternetCloseHandle(hRequest); });

        DWORD statusCode = 0;
        DWORD bufferLength = sizeof(statusCode);
        if (!::HttpQueryInfo(hRequest,		//_In_     HINTERNET hRequest,
                             HTTP_QUERY_STATUS_CODE | HTTP_QUERY_FLAG_NUMBER, //_In_     DWORD dwInfoLevel,
                             &statusCode,	//_Inout_  LPVOID lpvBuffer,
                             &bufferLength,	//_Inout_  LPDWORD lpdwBufferLength,
                             nullptr))		//_Inout_  LPDWORD lpdwIndex
            throw InternetConnectionError();

        if (statusCode != HTTP_STATUS_OK)
            throw InternetConnectionError(); //e.g. 404 - HTTP_STATUS_NOT_FOUND

        guardRequest .dismiss();
        guardInternet.dismiss();
    }

    ~WinInetAccess()
    {
        ::InternetCloseHandle(hRequest);
        ::InternetCloseHandle(hInternet);
    }

    template <class OutputIterator>
    OutputIterator readBytes(OutputIterator result) //throw InternetConnectionError
    {
        //internet says "HttpQueryInfo() + HTTP_QUERY_CONTENT_LENGTH" not supported by all http servers...
        const DWORD bufferSize = 64 * 1024;
        std::vector<char> buffer(bufferSize);
        for (;;)
        {
            DWORD bytesRead = 0;
            if (!::InternetReadFile(hRequest, 	 //_In_   HINTERNET hFile,
                                    &buffer[0],  //_Out_  LPVOID lpBuffer,
                                    bufferSize,  //_In_   DWORD dwNumberOfBytesToRead,
                                    &bytesRead)) //_Out_  LPDWORD lpdwNumberOfBytesRead
                throw InternetConnectionError();
            if (bytesRead == 0)
                return result;

            result = std::copy(buffer.begin(), buffer.begin() + bytesRead, result);
        }
    }

private:
    HINTERNET hInternet;
    HINTERNET hRequest;
};


inline
bool canAccessUrl(const wchar_t* url) //throw ()
{
    try
    {
        (void)WinInetAccess(url); //throw InternetConnectionError
        return true;
    }
    catch (const InternetConnectionError&) { return false; }
}


template <class OutputIterator> inline
OutputIterator readBytesUrl(const wchar_t* url, OutputIterator result) //throw InternetConnectionError
{
    return WinInetAccess(url).readBytes(result); //throw InternetConnectionError
}

#else
inline
bool canAccessUrl(const wxString& server, const wxString& page, int timeout) //throw ()
{
    wxWindowDisabler dummy;
    wxHTTP webAccess;
    webAccess.SetHeader(L"content-type", L"text/html; charset=utf-8");
    webAccess.SetHeader(L"USER-AGENT", getUserAgentName());

    webAccess.SetTimeout(timeout); //default: 10 minutes(WTF are these wxWidgets people thinking???)...

    if (webAccess.Connect(server)) //will *not* fail for non-reachable url here!
    {
        std::unique_ptr<wxInputStream> httpStream(webAccess.GetInputStream(page)); //call before checking wxHTTP::GetResponse()
        const int rs = webAccess.GetResponse();

        if (rs == 301 || //http://en.wikipedia.org/wiki/List_of_HTTP_status_codes#3xx_Redirection
            rs == 302 ||
            rs == 303 ||
            rs == 307 ||
            rs == 308)
            return true;

        if (rs == 200) //HTTP_STATUS_OK
            return true;
    }
    return false;
}


bool getStringFromUrl(const wxString& server, const wxString& page, int timeout, wxString* output, int level = 0) //true on successful connection
{
    wxWindowDisabler dummy;
    wxHTTP webAccess;
    webAccess.SetHeader(L"content-type", L"text/html; charset=utf-8");
    webAccess.SetHeader(L"USER-AGENT", getUserAgentName());

    webAccess.SetTimeout(timeout); //default: 10 minutes(WTF are these wxWidgets people thinking???)...

    if (webAccess.Connect(server)) //will *not* fail for non-reachable url here!
    {
        //wxApp::IsMainLoopRunning(); // should return true

        std::unique_ptr<wxInputStream> httpStream(webAccess.GetInputStream(page));
        //must be deleted BEFORE webAccess is closed
        const int rs = webAccess.GetResponse();

        if (rs == 301 || //http://en.wikipedia.org/wiki/List_of_HTTP_status_codes#3xx_Redirection
            rs == 302 ||
            rs == 303 ||
            rs == 307 ||
            rs == 308)
            if (level < 5) //"A user agent should not automatically redirect a request more than five times, since such redirections usually indicate an infinite loop."
            {

                wxString newLocation = webAccess.GetHeader(L"Location");
                if (!newLocation.empty())
                {
                    assert(!startsWith(newLocation, L"https:")); //not supported by wxHTTP!
                    if (startsWith(newLocation, L"http://"))
                        newLocation = afterFirst(newLocation, L"://", IF_MISSING_RETURN_NONE);
                    const wxString serverNew =       beforeFirst(newLocation, L'/', IF_MISSING_RETURN_ALL);
                    const wxString pageNew   = L'/' + afterFirst(newLocation, L'/', IF_MISSING_RETURN_NONE);

                    return getStringFromUrl(serverNew, pageNew, timeout, output, level + 1);
                }
            }

        if (rs == 200) //HTTP_STATUS_OK
            if (httpStream && webAccess.GetError() == wxPROTO_NOERR)
            {
                if (output)
                {
                    output->clear();
                    wxStringOutputStream outStream(output);
                    httpStream->Read(outStream);
                }
                return true;
            }
    }
    return false;
}
#endif


enum GetVerResult
{
    GET_VER_SUCCESS,
    GET_VER_NO_CONNECTION, //no internet connection or homepage down?
    GET_VER_PAGE_NOT_FOUND //version file seems to have moved! => trigger an update!
};

GetVerResult getOnlineVersion(wxString& version)
{
#ifdef ZEN_WIN //internet access supporting proxy connections
    std::vector<char> output;
    try
    {
        //harmonize with wxHTTP: latest_version.txt does not use https!!!
        readBytesUrl(L"http://www.freefilesync.org/latest_version.txt", std::back_inserter(output)); //throw InternetConnectionError
    }
    catch (const InternetConnectionError&)
    {
        return canAccessUrl(L"http://www.google.com/") ? GET_VER_PAGE_NOT_FOUND : GET_VER_NO_CONNECTION;
    }
    output.push_back('\0');
    version = utfCvrtTo<wxString>(&output[0]);

#else
    if (!getStringFromUrl(L"www.freefilesync.org", L"/latest_version.txt", 5, &version))
    {
        return canAccessUrl(L"www.google.com", L"/", 1) ? GET_VER_PAGE_NOT_FOUND : GET_VER_NO_CONNECTION;
    }
#endif
    trim(version); //Windows: remove trailing blank and newline
    return version.empty() ? GET_VER_PAGE_NOT_FOUND : GET_VER_SUCCESS; //empty version possible??
}


std::vector<size_t> parseVersion(const wxString& version)
{
    std::vector<wxString> digits = split(version, FFS_VERSION_SEPERATOR);

    std::vector<size_t> output;
    std::transform(digits.begin(), digits.end(), std::back_inserter(output), [&](const wxString& d) { return stringTo<size_t>(d); });
    return output;
}


/*constexpr*/ long getInactiveCheckId()
{
    //use current version to calculate a changing number for the inactive state near UTC begin, in order to always check for updates after installing a new version
    //=> convert version into 11-based *unique* number (this breaks lexicographical version ordering, but that's irrelevant!)
    long id = 0;
    const wchar_t* first = zen::ffsVersion;
    const wchar_t* last = first + zen::strLength(ffsVersion);
    std::for_each(first, last, [&](wchar_t c)
    {
        id *= 11;
        if (L'0' <= c && c <= L'9')
            id += c - L'0';
        else
        {
            assert(c == FFS_VERSION_SEPERATOR);
            id += 10;
        }
    });
    assert(0 < id && id < 3600 * 24 * 365); //as long as value is within a year after UTC begin (1970) there's no risk to clash with *current* time
    return id;
}
}


bool zen::isNewerFreeFileSyncVersion(const wxString& onlineVersion)
{
    std::vector<size_t> current = parseVersion(zen::ffsVersion);
    std::vector<size_t> online  = parseVersion(onlineVersion);

    if (online.empty() || online[0] == 0) //online version string may be "This website has been moved..." In this case better check for an update
        return true;

    return std::lexicographical_compare(current.begin(), current.end(),
                                        online .begin(), online .end());
}


bool zen::updateCheckActive(long lastUpdateCheck)
{
    return lastUpdateCheck != getInactiveCheckId();
}


void zen::disableUpdateCheck(long& lastUpdateCheck)
{
    lastUpdateCheck = getInactiveCheckId();
}


void zen::checkForUpdateNow(wxWindow* parent, wxString& lastOnlineVersion)
{
    wxString onlineVersion;
    switch (getOnlineVersion(onlineVersion))
    {
        case GET_VER_SUCCESS:
            lastOnlineVersion = onlineVersion;
            if (isNewerFreeFileSyncVersion(onlineVersion))
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
                                   setTitle(("Check for Program Updates")).
                                   setMainInstructions(_("Unable to connect to www.freefilesync.org.")));
            break;

        case GET_VER_PAGE_NOT_FOUND:
            lastOnlineVersion = L"unknown";
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


void zen::checkForUpdatePeriodically(wxWindow* parent, long& lastUpdateCheck, wxString& lastOnlineVersion, const std::function<void()>& onBeforeInternetAccess)
{
    if (updateCheckActive(lastUpdateCheck))
    {
        if (wxGetLocalTime() >= lastUpdateCheck + 7 * 24 * 3600) //check weekly
        {
            onBeforeInternetAccess(); //notify client before (potentially) blocking some time
            wxString onlineVersion;
            switch (getOnlineVersion(onlineVersion))
            {
                case GET_VER_SUCCESS:
                    lastUpdateCheck = wxGetLocalTime();
                    lastOnlineVersion = onlineVersion;

                    if (isNewerFreeFileSyncVersion(onlineVersion))
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
                    lastOnlineVersion = L"unknown";
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
    }
}
