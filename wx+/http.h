// *****************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under    *
// * GNU General Public License: http://www.gnu.org/licenses/gpl-3.0           *
// * Copyright (C) Zenju (zenju AT freefilesync DOT org) - All Rights Reserved *
// *****************************************************************************

#ifndef HTTP_h_879083425703425702
#define HTTP_h_879083425703425702

#include <zen/file_error.h>

namespace zen
{
/*
    TREAD-SAFETY
    ------------
    Windows: WinInet-based   => may be called from worker thread
    Linux:   wxWidgets-based => don't call from worker thread
*/
class HttpInputStream
{
public:
    std::string readAll(); //throw SysError

    //support zen/serialize.h Unbuffered Input Stream Concept
    size_t tryRead(void* buffer, size_t bytesToRead); //throw SysError; may return short, only 0 means EOF! =>  CONTRACT: bytesToRead > 0!
    size_t getBlockSize() const { return 64 * 1024; }

    class Impl;
    HttpInputStream(std::unique_ptr<Impl>&& pimpl);
    HttpInputStream(HttpInputStream&&) = default;
    ~HttpInputStream();

private:
    std::unique_ptr<Impl> pimpl_;
};


HttpInputStream sendHttpGet (const std::wstring& url, const std::wstring& userAgent); //throw SysError
HttpInputStream sendHttpPost(const std::wstring& url, const std::wstring& userAgent,
                             const std::vector<std::pair<std::string, std::string>>& postParams); //throw SysError
bool internetIsAlive(); //noexcept

std::string xWwwFormUrlEncode(const std::vector<std::pair<std::string, std::string>>& paramPairs);
std::vector<std::pair<std::string, std::string>> xWwwFormUrlDecode(const std::string& str);
}

#endif //HTTP_h_879083425703425702
