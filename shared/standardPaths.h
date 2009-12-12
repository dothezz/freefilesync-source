#ifndef STANDARDPATHS_H_INCLUDED
#define STANDARDPATHS_H_INCLUDED

#include <wx/string.h>


namespace FreeFileSync
{
//------------------------------------------------------------------------------
//global functions
//------------------------------------------------------------------------------
const wxString& getGlobalConfigFile();
const wxString& getDefaultLogDirectory();
const wxString& getLastErrorTxtFile();
const wxString& getInstallationDir(); //FreeFileSync installation directory WITH path separator at end
const wxString& getConfigDir();
//------------------------------------------------------------------------------
}

#endif // STANDARDPATHS_H_INCLUDED
