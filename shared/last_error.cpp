// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) 2008-2011 ZenJu (zhnmju123 AT gmx.de)                    *
// **************************************************************************
//
#include "last_error.h"
#include "string_tools.h"
#include "i18n.h"

#ifdef FFS_WIN
#include <wx/msw/wrapwin.h> //includes "windows.h"

#elif defined FFS_LINUX
#include <cstring>
#include <cerrno>
#endif



#ifdef FFS_WIN
wxString zen::getLastErrorFormatted(unsigned long lastError) //try to get additional Windows error information
{
    //determine error code if none was specified
    if (lastError == 0)
        lastError = ::GetLastError();

    wxString output = _("Windows Error Code %x:");
    output.Replace(wxT("%x"), zen::toString<wxString>(lastError));

    LPWSTR buffer = NULL;
    if (::FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM    |
                        FORMAT_MESSAGE_MAX_WIDTH_MASK |
                        FORMAT_MESSAGE_IGNORE_INSERTS | //important: without this flag ::FormatMessage() will fail if message contains placeholders
                        FORMAT_MESSAGE_ALLOCATE_BUFFER, NULL, lastError, 0, reinterpret_cast<LPWSTR>(&buffer), 0, NULL) != 0)
    {
        if (buffer) //just to be sure
        {
            output += wxT(" ");
            output += buffer;
            ::LocalFree(buffer);
        }
    }
    ::SetLastError(lastError); //restore last error
    return output;
}

#elif defined FFS_LINUX
wxString zen::getLastErrorFormatted(int lastError) //try to get additional Linux error information
{
    //determine error code if none was specified
    if (lastError == 0)
        lastError = errno; //don't use "::", errno is a macro!

    wxString output = _("Linux Error Code %x:");
    output.Replace(wxT("%x"), zen::toString<wxString>(lastError));

    output += wxT(" ") + wxString::FromUTF8(::strerror(lastError));

    errno = lastError; //restore errno
    return output;
}
#endif
