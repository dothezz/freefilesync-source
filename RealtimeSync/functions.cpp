// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) 2008-2010 ZenJu (zhnmju123 AT gmx.de)                    *
// **************************************************************************
//
#include "functions.h"
#include <wx/textctrl.h>
#include <wx/filepicker.h>
#include "../shared/string_conv.h"
#include "../shared/file_handling.h"

using namespace ffs3;

void rts::setDirectoryName(const wxString& dirname, wxTextCtrl* txtCtrl, wxDirPickerCtrl* dirPicker)
{
    txtCtrl->SetValue(dirname);
    const Zstring leftDirFormatted = ffs3::getFormattedDirectoryName(wxToZ(dirname));
    if (dirExists(leftDirFormatted))
        dirPicker->SetPath(zToWx(leftDirFormatted));
}

