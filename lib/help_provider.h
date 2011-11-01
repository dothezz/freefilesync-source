// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) 2008-2011 ZenJu (zhnmju123 AT gmx.de)                    *
// **************************************************************************

#ifndef HELPPROVIDER_H_INCLUDED
#define HELPPROVIDER_H_INCLUDED

#include <wx/help.h>
#include "ffs_paths.h"

namespace zen
{
void displayHelpEntry(const wxString& section = wxEmptyString);













//######################## implementation ########################
inline
wxHelpController& getHelpCtrl()
{
    static wxHelpController controller; //external linkage, despite inline definition!
    static bool initialized = false;
    if (!initialized)
    {
        initialized = true;
        controller.Initialize(toWx(zen::getResourceDir()) +
#ifdef FFS_WIN
                              L"FreeFileSync.chm");
#elif defined FFS_LINUX
                              L"Help/FreeFileSync.hhp");
#endif
    }
    return controller;
}


inline
void displayHelpEntry(const wxString& section)
{
    if (section.empty())
        getHelpCtrl().DisplayContents();
    else
        getHelpCtrl().DisplaySection(section);
}
}

#endif // HELPPROVIDER_H_INCLUDED
