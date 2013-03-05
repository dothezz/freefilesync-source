// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) Zenju (zenju AT gmx DOT de) - All Rights Reserved        *
// **************************************************************************

#include "check_version.h"
#include <memory>
#include <zen/string_tools.h>
#include <zen/i18n.h>
#include <zen/utf.h>
#include <wx/msgdlg.h>
#include <wx/protocol/http.h>
#include <wx/sstream.h>
#include <wx/utils.h>
#include <wx/timer.h>
#include "msg_popup.h"
#include "../version/version.h"
//#include "../lib/ffs_paths.h"
#include <zen/scope_guard.h>

#ifdef FFS_WIN
#include <wininet.h>
#endif

using namespace zen;


namespace
{
#ifdef FFS_WIN
class InternetConnectionError {};

class WinInetAccess //using IE proxy settings! :)
{
public:
    WinInetAccess(const wchar_t* url) //throw InternetConnectionError (if url cannot be reached; no need to also call readBytes())
    {
        //::InternetAttemptConnect(0) -> not working as expected: succeeds even when there is no internet connection!

        hInternet = ::InternetOpen(L"FreeFileSync", //_In_  LPCTSTR lpszAgent,
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
#endif


enum GetVerResult
{
    GET_VER_SUCCESS,
    GET_VER_NO_CONNECTION, //no internet connection or just Sourceforge down?
    GET_VER_PAGE_NOT_FOUND //version file seems to have moved! => trigger an update!
};

GetVerResult getOnlineVersion(wxString& version) //empty string on error;
{
#ifdef FFS_WIN
    //internet access supporting proxy connections
    std::vector<char> output;
    try
    {
        readBytesUrl(L"http://freefilesync.sourceforge.net/latest_version.txt", std::back_inserter(output)); //throw InternetConnectionError
    }
    catch (const InternetConnectionError&)
    {
        return canAccessUrl(L"http://sourceforge.net/") ? GET_VER_PAGE_NOT_FOUND : GET_VER_NO_CONNECTION;
    }

    output.push_back('\0');
    version = utfCvrtTo<wxString>(&output[0]);
    return GET_VER_SUCCESS;

#elif defined FFS_LINUX || defined FFS_MAC
    wxWindowDisabler dummy;

    auto getStringFromUrl = [](const wxString& server, const wxString& page, int timeout, wxString* output) -> bool //true on successful connection
    {
        wxHTTP webAccess;
        webAccess.SetHeader(L"content-type", L"text/html; charset=utf-8");
        webAccess.SetTimeout(timeout); //default: 10 minutes(WTF are they thinking???)...

        if (webAccess.Connect(server)) //will *not* fail for non-reachable url here!
        {
            //wxApp::IsMainLoopRunning(); // should return true

            std::unique_ptr<wxInputStream> httpStream(webAccess.GetInputStream(page));
            //must be deleted BEFORE webAccess is closed

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
    };

    if (getStringFromUrl(L"freefilesync.sourceforge.net", L"/latest_version.txt", 5, &version))
        return GET_VER_SUCCESS;

    const bool canConnectToSf = getStringFromUrl(L"sourceforge.net", L"/", 1, nullptr);
    return canConnectToSf ? GET_VER_PAGE_NOT_FOUND : GET_VER_NO_CONNECTION;
#endif
}


const wchar_t VERSION_SEP = L'.';

std::vector<size_t> parseVersion(const wxString& version)
{
    std::vector<wxString> digits = split(version, VERSION_SEP);

    std::vector<size_t> output;
    std::transform(digits.begin(), digits.end(), std::back_inserter(output), [&](const wxString& d) { return stringTo<size_t>(d); });
    return output;
}


bool haveNewerVersion(const wxString& onlineVersion)
{
    std::vector<size_t> current = parseVersion(zen::currentVersion);
    std::vector<size_t> online  = parseVersion(onlineVersion);

    if (online.empty() || online[0] == 0) //online version may be "This website has been moved..." In this case better check for an update
        return true;

    return std::lexicographical_compare(current.begin(), current.end(),
                                        online .begin(), online .end());
}
}


void zen::checkForUpdateNow(wxWindow* parent)
{
    wxString onlineVersion;
    switch (getOnlineVersion(onlineVersion))
    {
        case GET_VER_SUCCESS:
            if (haveNewerVersion(onlineVersion))
            {
                if (showQuestionDlg(parent, ReturnQuestionDlg::BUTTON_YES | ReturnQuestionDlg::BUTTON_CANCEL,
                                    _("A new version of FreeFileSync is available:")  + L" " + onlineVersion + L"\n\n" + _("Download now?")) == ReturnQuestionDlg::BUTTON_YES)
                    wxLaunchDefaultBrowser(L"http://sourceforge.net/projects/freefilesync/files/freefilesync/v" + onlineVersion + L"/");
            }
            else
                wxMessageBox(_("FreeFileSync is up to date!"), _("Information"), wxICON_INFORMATION, parent);
            break;

        case GET_VER_NO_CONNECTION:
            wxMessageBox(_("Unable to connect to sourceforge.net!"), _("Error"), wxOK | wxICON_ERROR, parent);
            break;

        case GET_VER_PAGE_NOT_FOUND:
            if (showQuestionDlg(parent, ReturnQuestionDlg::BUTTON_YES | ReturnQuestionDlg::BUTTON_CANCEL,
                                _("Current FreeFileSync version number was not found online! Do you want to check manually?")) == ReturnQuestionDlg::BUTTON_YES)
                wxLaunchDefaultBrowser(L"http://sourceforge.net/projects/freefilesync/");
            break;
    }
}


void zen::checkForUpdatePeriodically(wxWindow* parent, long& lastUpdateCheck)
{
    if (lastUpdateCheck != -1)
    {
        if (lastUpdateCheck == 0)
        {
            switch (showQuestionDlg(parent, ReturnQuestionDlg::BUTTON_YES | ReturnQuestionDlg::BUTTON_NO | ReturnQuestionDlg::BUTTON_CANCEL,
                                    _("Do you want FreeFileSync to automatically check for updates every week?") + L"\n" +
                                    _("(Requires an Internet connection!)")))
            {
                case ReturnQuestionDlg::BUTTON_YES:
                    lastUpdateCheck = 123; //some old date (few seconds after 1970) different from 0 and -1
                    checkForUpdatePeriodically(parent, lastUpdateCheck); //check for updates now
                    break;

                case ReturnQuestionDlg::BUTTON_NO:
                    lastUpdateCheck = -1; //don't check for updates anymore
                    break;

                case ReturnQuestionDlg::BUTTON_CANCEL:
                    break;
            }
        }
        else if (wxGetLocalTime() >= lastUpdateCheck + 7 * 24 * 3600) //check weekly
        {
            wxString onlineVersion;
            switch (getOnlineVersion(onlineVersion))
            {
                case GET_VER_SUCCESS:
                    lastUpdateCheck = wxGetLocalTime();

                    if (haveNewerVersion(onlineVersion))
                    {
                        if (showQuestionDlg(parent, ReturnQuestionDlg::BUTTON_YES | ReturnQuestionDlg::BUTTON_CANCEL,
                                            _("A new version of FreeFileSync is available:")  + L" " + onlineVersion + L"\n\n" +
                                            _("Download now?")) == ReturnQuestionDlg::BUTTON_YES)
                            wxLaunchDefaultBrowser(L"http://sourceforge.net/projects/freefilesync/files/freefilesync/v" + onlineVersion + L"/");
                    }
                    break;

                case GET_VER_NO_CONNECTION:
                    break; //ignore this error

                case GET_VER_PAGE_NOT_FOUND:
                    if (showQuestionDlg(parent, ReturnQuestionDlg::BUTTON_YES | ReturnQuestionDlg::BUTTON_CANCEL,
                                        _("Current FreeFileSync version number was not found online! Do you want to check manually?")) == ReturnQuestionDlg::BUTTON_YES)
                        wxLaunchDefaultBrowser(L"http://sourceforge.net/projects/freefilesync/");
                    break;
            }
        }
    }
}
