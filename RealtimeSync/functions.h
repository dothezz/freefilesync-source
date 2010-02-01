#ifndef FUNCTIONS_H_INCLUDED
#define FUNCTIONS_H_INCLUDED

#include <wx/string.h>

class wxTextCtrl;
class wxDirPickerCtrl;


namespace RealtimeSync
{
void setDirectoryName(const wxString& dirname, wxTextCtrl* txtCtrl, wxDirPickerCtrl* dirPicker);
}

#endif // FUNCTIONS_H_INCLUDED
