// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) 2008-2010 ZenJu (zhnmju123 AT gmx.de)                    *
// **************************************************************************
//
#include "system_func.h"

#ifdef FFS_WIN
#include <wx/msw/wrapwin.h> //includes "windows.h"

#elif defined FFS_LINUX
#include <cstring>
#include <cerrno>
#endif



#ifdef FFS_WIN
wxString ffs3::getLastErrorFormatted(unsigned long lastError) //try to get additional Windows error information
{
    //determine error code if none was specified
    if (lastError == 0)
        lastError = ::GetLastError();

    wxString output = wxString(wxT("Windows Error Code ")) + wxString::Format(wxT("%u"), lastError);

    WCHAR buffer[1001];
    if (::FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_MAX_WIDTH_MASK, 0, lastError, 0, buffer, 1001, NULL) != 0)
        output += wxString(wxT(": ")) + buffer;

        ::SetLastError(lastError); //restore last error
    return output;
}

#elif defined FFS_LINUX
wxString ffs3::getLastErrorFormatted(int lastError) //try to get additional Linux error information
{
    //determine error code if none was specified
    if (lastError == 0)
        lastError = errno; //don't use "::", errno is a macro!

    wxString output = wxString(wxT("Linux Error Code ")) + wxString::Format(wxT("%i"), lastError);
    output += wxString(wxT(": ")) + wxString::FromUTF8(::strerror(lastError));

    errno = lastError; //restore errno
    return output;
}
#endif
