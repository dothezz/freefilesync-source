// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) 2008-2011 ZenJu (zhnmju123 AT gmx.de)                    *
// **************************************************************************

#include "last_error.h"
#include "string_utf8.h"
#include "i18n.h"

#ifdef FFS_WIN
#include <wx/msw/wrapwin.h> //includes "windows.h"

#elif defined FFS_LINUX
#include <cstring>
#include <cerrno>
#endif

using namespace zen;


#ifdef FFS_WIN
std::wstring zen::getLastErrorFormatted(unsigned long lastError) //try to get additional Windows error information
{
    //determine error code if none was specified
    if (lastError == 0)
        lastError = ::GetLastError();

    std::wstring output = _("Windows Error Code %x:");
    replace(output, L"%x", toString<std::wstring>(lastError));

    LPWSTR buffer = NULL;
    if (::FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM    |
                        FORMAT_MESSAGE_MAX_WIDTH_MASK |
                        FORMAT_MESSAGE_IGNORE_INSERTS | //important: without this flag ::FormatMessage() will fail if message contains placeholders
                        FORMAT_MESSAGE_ALLOCATE_BUFFER, NULL, lastError, 0, reinterpret_cast<LPWSTR>(&buffer), 0, NULL) != 0)
    {
        if (buffer) //just to be sure
        {
            output += L" ";
            output += buffer;
            ::LocalFree(buffer);
        }
    }
    ::SetLastError(lastError); //restore last error
    return output;
}

#elif defined FFS_LINUX
std::wstring zen::getLastErrorFormatted(int lastError) //try to get additional Linux error information
{
    //determine error code if none was specified
    if (lastError == 0)
        lastError = errno; //don't use "::", errno is a macro!

    std::wstring output = _("Linux Error Code %x:");
    replace(output, L"%x", toString<std::wstring>(lastError));

    output += L" ";
    output += utf8CvrtTo<std::wstring>(::strerror(lastError));

    errno = lastError; //restore errno
    return output;
}
#endif
