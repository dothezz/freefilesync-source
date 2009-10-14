#include "functions.h"
#include <wx/textctrl.h>
#include <wx/filepicker.h>
//#include "../shared/globalFunctions.h"
#include "../shared/stringConv.h"
#include "../shared/fileHandling.h"

using namespace FreeFileSync;

void RealtimeSync::setDirectoryName(const wxString& dirname, wxTextCtrl* txtCtrl, wxDirPickerCtrl* dirPicker)
{
    txtCtrl->SetValue(dirname);
    const Zstring leftDirFormatted = FreeFileSync::getFormattedDirectoryName(wxToZ(dirname));
    if (dirExists(leftDirFormatted))
        dirPicker->SetPath(zToWx(leftDirFormatted));
}

