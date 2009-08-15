#include "functions.h"
#include <wx/textctrl.h>
#include <wx/filepicker.h>
//#include "../shared/globalFunctions.h"
#include "../shared/fileHandling.h"


void RealtimeSync::setDirectoryName(const wxString& dirname, wxTextCtrl* txtCtrl, wxDirPickerCtrl* dirPicker)
{
    txtCtrl->SetValue(dirname);
    const Zstring leftDirFormatted = FreeFileSync::getFormattedDirectoryName(dirname.c_str());
    if (wxDirExists(leftDirFormatted.c_str()))
        dirPicker->SetPath(leftDirFormatted.c_str());
}

