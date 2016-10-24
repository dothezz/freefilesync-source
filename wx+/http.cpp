// *****************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under    *
// * GNU General Public License: http://www.gnu.org/licenses/gpl-3.0           *
// * Copyright (C) Zenju (zenju AT freefilesync DOT org) - All Rights Reserved *
// *****************************************************************************

#include "http.h"

    #include <wx/app.h>
    #include <zen/thread.h> //std::thread::id
    #include <wx/protocol/http.h>

using namespace zen;


namespace
{


std::string sendHttpRequestImpl(const std::wstring& url, //throw SysError
                                const std::wstring& userAgent,
                                const std::string* postParams, //issue POST if bound, GET otherwise
                                int level = 0)
{
    assert(!startsWith(makeUpperCopy(url), L"HTTPS:")); //not supported by wxHTTP!
    const std::wstring urlFmt = startsWith(makeUpperCopy(url), L"HTTP://") ? afterFirst(url, L"://", IF_MISSING_RETURN_NONE) : url;
    const std::wstring server =       beforeFirst(urlFmt, L'/', IF_MISSING_RETURN_ALL);
    const std::wstring page   = L'/' + afterFirst(urlFmt, L'/', IF_MISSING_RETURN_NONE);

    assert(std::this_thread::get_id() == mainThreadId);
    assert(wxApp::IsMainLoopRunning());

    wxHTTP webAccess;
    webAccess.SetHeader(L"User-Agent", userAgent);
    webAccess.SetTimeout(10 /*[s]*/); //default: 10 minutes: WTF are these wxWidgets people thinking???

    if (!webAccess.Connect(server)) //will *not* fail for non-reachable url here!
        throw SysError(L"wxHTTP::Connect");

    if (postParams)
        if (!webAccess.SetPostText(L"application/x-www-form-urlencoded", utfCvrtTo<wxString>(*postParams)))
            throw SysError(L"wxHTTP::SetPostText");

    std::unique_ptr<wxInputStream> httpStream(webAccess.GetInputStream(page)); //must be deleted BEFORE webAccess is closed
    const int sc = webAccess.GetResponse();

    //http://en.wikipedia.org/wiki/List_of_HTTP_status_codes#3xx_Redirection
    if (sc / 100 == 3) //e.g. 301, 302, 303, 307... we're not too greedy since we check location, too!
    {
        if (level < 5) //"A user agent should not automatically redirect a request more than five times, since such redirections usually indicate an infinite loop."
        {
            const std::wstring newUrl(webAccess.GetHeader(L"Location"));
            if (!newUrl.empty())
                return sendHttpRequestImpl(newUrl, userAgent, postParams, level + 1);
        }
        throw SysError(L"Unresolvable redirect.");
    }

    if (sc != 200) //HTTP_STATUS_OK
        throw SysError(replaceCpy<std::wstring>(L"HTTP status code %x.", L"%x", numberTo<std::wstring>(sc)));

    if (!httpStream || webAccess.GetError() != wxPROTO_NOERR)
        throw SysError(L"wxHTTP::GetError");

    std::string buffer;
    int newValue = 0;
    while ((newValue = httpStream->GetC()) != wxEOF)
        buffer.push_back(static_cast<char>(newValue));
    return buffer;
}


//encode into "application/x-www-form-urlencoded"
std::string urlencode(const std::string& str)
{
    std::string out;
    for (const char c : str) //follow PHP spec: https://github.com/php/php-src/blob/master/ext/standard/url.c#L500
        if (c == ' ')
            out += '+';
        else if (('0' <= c && c <= '9') ||
                 ('A' <= c && c <= 'Z') ||
                 ('a' <= c && c <= 'z') ||
                 c == '-' || c == '.' || c == '_') //note: "~" is encoded by PHP!
            out += c;
        else
        {
            const char hexDigits[] = "0123456789ABCDEF";
            out += '%';
            out += hexDigits[static_cast<unsigned char>(c) / 16];
            out += hexDigits[static_cast<unsigned char>(c) % 16];
        }
    return out;
}
}


std::string zen::sendHttpPost(const std::wstring& url, const std::wstring& userAgent, const std::vector<std::pair<std::string, std::string>>& postParams) //throw SysError
{
    //convert post parameters into "application/x-www-form-urlencoded"
    std::string flatParams;
    for (const auto& pair : postParams)
        flatParams += urlencode(pair.first) + '=' + urlencode(pair.second) + '&';
    //encode both key and value: https://www.w3.org/TR/html401/interact/forms.html#h-17.13.4.1
    if (!flatParams.empty())
        flatParams.pop_back();

    return sendHttpRequestImpl(url, userAgent, &flatParams); //throw SysError
}


std::string zen::sendHttpGet(const std::wstring& url, const std::wstring& userAgent) //throw SysError
{
    return sendHttpRequestImpl(url, userAgent, nullptr); //throw SysError
}


bool zen::internetIsAlive() //noexcept
{
    assert(std::this_thread::get_id() == mainThreadId);

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
