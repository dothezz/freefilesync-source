#include "systemFunctions.h"

#ifdef FFS_WIN
#include <wx/msw/wrapwin.h> //includes "windows.h"

#elif defined FFS_LINUX
#include <string.h>
#include <errno.h>
#endif



#ifdef FFS_WIN
wxString FreeFileSync::getLastErrorFormatted(const unsigned long lastError) //try to get additional Windows error information
{
    //determine error code if none was specified
    const unsigned long lastErrorCode = lastError == 0 ? ::GetLastError() : lastError;

    wxString output = wxString(wxT("Windows Error Code ")) + wxString::Format(wxT("%u"), lastErrorCode);

    WCHAR buffer[1001];
    if (::FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_MAX_WIDTH_MASK, 0, lastErrorCode, 0, buffer, 1001, NULL) != 0)
        output += wxString(wxT(": ")) + buffer;
    return output;
}

#elif defined FFS_LINUX
wxString FreeFileSync::getLastErrorFormatted(const int lastError) //try to get additional Linux error information
{
    //determine error code if none was specified
    const int lastErrorCode = lastError == 0 ? errno : lastError;

    wxString output = wxString(wxT("Linux Error Code ")) + wxString::Format(wxT("%i"), lastErrorCode);
    output += wxString(wxT(": ")) + ::strerror(lastErrorCode);
    return output;
}
#endif
