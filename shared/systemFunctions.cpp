#include "systemFunctions.h"

#ifdef FFS_WIN
#include <wx/msw/wrapwin.h> //includes "windows.h"

#elif defined FFS_LINUX
#include <string.h>
#include <errno.h>
#endif



#ifdef FFS_WIN
wxString FreeFileSync::getLastErrorFormatted(unsigned long lastError) //try to get additional Windows error information
{
    //determine error code if none was specified
    if (lastError == 0)
        lastError = ::GetLastError();

    wxString output = wxString(wxT("Windows Error Code ")) + wxString::Format(wxT("%u"), lastError);

    WCHAR buffer[1001];
    if (::FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_MAX_WIDTH_MASK, 0, lastError, 0, buffer, 1001, NULL) != 0)
        output += wxString(wxT(": ")) + buffer;
    return output;
}

#elif defined FFS_LINUX
wxString FreeFileSync::getLastErrorFormatted(int lastError) //try to get additional Linux error information
{
    //determine error code if none was specified
    if (lastError == 0)
        lastError = errno; //don't use :: errno is a macro!

    wxString output = wxString(wxT("Linux Error Code ")) + wxString::Format(wxT("%i"), lastError);
    output += wxString(wxT(": ")) + wxString::FromUTF8(::strerror(lastError));
    return output;
}
#endif
