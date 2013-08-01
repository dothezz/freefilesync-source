// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) Zenju (zenju AT gmx DOT de) - All Rights Reserved        *
// **************************************************************************

#ifndef LAST_ERROR_H_3284791347018951324534
#define LAST_ERROR_H_3284791347018951324534

#include <string>
#include "utf.h"
#include "i18n.h"
#include "scope_guard.h"

#ifdef ZEN_WIN
#include "win.h" //includes "windows.h"

#elif defined ZEN_LINUX || defined ZEN_MAC
#include <cstring>
#include <cerrno>
#endif


namespace zen
{
//evaluate GetLastError()/errno and assemble specific error message
#ifdef ZEN_WIN
typedef DWORD ErrorCode;
#elif defined ZEN_LINUX || defined ZEN_MAC
typedef int ErrorCode;
#else
#error define a platform!
#endif

ErrorCode getLastError();

std::wstring formatSystemError(const std::wstring& functionName, ErrorCode lastError);


//A low-level exception class giving (non-translated) detail information only - same conceptional level like "GetLastError()"!
class SysError
{
public:
    explicit SysError(const std::wstring& msg) : msg_(msg) {}
    const std::wstring& toString() const { return msg_; }

private:
    std::wstring msg_;
};








//######################## implementation ########################
inline
ErrorCode getLastError()
{
#ifdef ZEN_WIN
    return ::GetLastError();
#elif defined ZEN_LINUX || defined ZEN_MAC
    return errno; //don't use "::", errno is a macro!
#endif
}


std::wstring formatSystemError(const std::wstring& functionName, long long lastError); //not implemented! intentional overload ambiguity to catch usage errors with HRESULT!


inline
std::wstring formatSystemError(const std::wstring& functionName, ErrorCode lastError)
{
    //determine error code if none was specified -> still required??
    if (lastError == 0)
        lastError = getLastError();

    std::wstring output = replaceCpy(_("Error Code %x:"), L"%x", numberTo<std::wstring>(lastError));

#ifdef ZEN_WIN
    LPWSTR buffer = nullptr;
    if (::FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM    |
                        FORMAT_MESSAGE_MAX_WIDTH_MASK |
                        FORMAT_MESSAGE_IGNORE_INSERTS | //important: without this flag ::FormatMessage() will fail if message contains placeholders
                        FORMAT_MESSAGE_ALLOCATE_BUFFER, nullptr, lastError, 0, reinterpret_cast<LPWSTR>(&buffer), 0, nullptr) != 0)
        if (buffer) //"don't trust nobody"
        {
            ZEN_ON_SCOPE_EXIT(::LocalFree(buffer));
            output += L" ";
            output += buffer;
        }
    ::SetLastError(lastError); //restore last error

#elif defined ZEN_LINUX || defined ZEN_MAC
    output += L" ";
    output += utfCvrtTo<std::wstring>(::strerror(lastError));

    errno = lastError; //restore errno
#endif
    if (!endsWith(output, L" ")) //Windows messages seem to end with a blank...
        output += L" ";
    output += L"(" + functionName + L")";

    return output;
}
}

#endif //LAST_ERROR_H_3284791347018951324534
