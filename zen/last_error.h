// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) Zenju (zenju AT gmx DOT de) - All Rights Reserved        *
// **************************************************************************

#ifndef SYSTEMFUNCTIONS_H_INCLUDED
#define SYSTEMFUNCTIONS_H_INCLUDED

#include <string>
#include "utf.h"
#include "i18n.h"

#ifdef FFS_WIN
#include "win.h" //includes "windows.h"

#elif defined FFS_LINUX || defined FFS_MAC
#include <cstring>
#include <cerrno>
#endif


namespace zen
{
//evaluate GetLastError()/errno and assemble specific error message
#ifdef FFS_WIN
typedef DWORD ErrorCode;
#elif defined FFS_LINUX || defined FFS_MAC
typedef int ErrorCode;
#endif

std::wstring getLastErrorFormatted(ErrorCode lastError = 0);
ErrorCode getLastError();

bool errorCodeForNotExisting(ErrorCode lastError); //check for "not existing" aliases









//######################## implementation ########################
inline
ErrorCode getLastError()
{
#ifdef FFS_WIN
    return ::GetLastError();
#elif defined FFS_LINUX || defined FFS_MAC
    return errno; //don't use "::", errno is a macro!
#endif
}

inline
std::wstring getLastErrorFormatted(ErrorCode lastError)
{
    //determine error code if none was specified
    if (lastError == 0)
        lastError = getLastError();

    std::wstring output = _("Error Code %x:");
    replace(output, L"%x", numberTo<std::wstring>(lastError));
#ifdef FFS_WIN
    LPWSTR buffer = nullptr;
    if (::FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM    |
                        FORMAT_MESSAGE_MAX_WIDTH_MASK |
                        FORMAT_MESSAGE_IGNORE_INSERTS | //important: without this flag ::FormatMessage() will fail if message contains placeholders
                        FORMAT_MESSAGE_ALLOCATE_BUFFER, nullptr, lastError, 0, reinterpret_cast<LPWSTR>(&buffer), 0, nullptr) != 0)
    {
        if (buffer) //"don't trust nobody"
        {
            output += L" ";
            output += buffer;
            ::LocalFree(buffer);
        }
    }
    ::SetLastError(lastError); //restore last error

#elif defined FFS_LINUX || defined FFS_MAC
    output += L" ";
    output += utfCvrtTo<std::wstring>(::strerror(lastError));

    errno = lastError; //restore errno
#endif
    return output;
}


inline
bool errorCodeForNotExisting(ErrorCode lastError)
{
#ifdef FFS_WIN
    return lastError == ERROR_FILE_NOT_FOUND ||
           lastError == ERROR_PATH_NOT_FOUND ||
           lastError == ERROR_BAD_NETPATH    ||
           lastError == ERROR_NETNAME_DELETED;
#elif defined FFS_LINUX || defined FFS_MAC
    return lastError == ENOENT;
#endif
}
}

#endif // SYSTEMFUNCTIONS_H_INCLUDED
