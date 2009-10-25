#ifndef UTIL_H_INCLUDED
#define UTIL_H_INCLUDED

#include "../shared/zstring.h"
#include <wx/string.h>
#include <wx/longlong.h>

class wxComboBox;
class wxTextCtrl;
class wxDirPickerCtrl;
class wxScrolledWindow;


namespace FreeFileSync
{
wxString formatFilesizeToShortString(const wxLongLong& filesize);
wxString formatFilesizeToShortString(const wxULongLong& filesize);
wxString formatFilesizeToShortString(const double filesize);

wxString fromatPercentage(const wxLongLong& dividend, const wxLongLong& divisor);

wxString includeNumberSeparator(const wxString& number);

void setDirectoryName(const wxString& dirname, wxTextCtrl* txtCtrl, wxDirPickerCtrl* dirPicker);
void setDirectoryName(const wxString& dirname, wxComboBox* txtCtrl, wxDirPickerCtrl* dirPicker);
void scrollToBottom(wxScrolledWindow* scrWindow);

wxString utcTimeToLocalString(const wxLongLong& utcTime, const Zstring& filename);
}


#endif // UTIL_H_INCLUDED
