// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) 2008-2010 ZenJu (zhnmju123 AT gmx.de)                    *
// **************************************************************************
//
#ifndef SYSTEMFUNCTIONS_H_INCLUDED
#define SYSTEMFUNCTIONS_H_INCLUDED

#include <wx/string.h>


namespace FreeFileSync
{

#ifdef FFS_WIN
wxString getLastErrorFormatted(unsigned long lastError = 0); //try to get additional Windows error information
#elif defined FFS_LINUX
wxString getLastErrorFormatted(int lastError = 0); //try to get additional Linux error information
#endif
}


#endif // SYSTEMFUNCTIONS_H_INCLUDED
