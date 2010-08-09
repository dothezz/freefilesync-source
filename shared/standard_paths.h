// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) 2008-2010 ZenJu (zhnmju123 AT gmx.de)                    *
// **************************************************************************
//
#ifndef STANDARDPATHS_H_INCLUDED
#define STANDARDPATHS_H_INCLUDED

#include <wx/string.h>


namespace ffs3
{
//------------------------------------------------------------------------------
//global program directories
//------------------------------------------------------------------------------
const wxString& getBinaryDir();   //directory containing executable WITH path separator at end
const wxString& getResourceDir(); //resource directory WITH path separator at end
const wxString& getConfigDir();   //config directory WITH path separator at end
//------------------------------------------------------------------------------

bool isPortableVersion();
}

#endif // STANDARDPATHS_H_INCLUDED
