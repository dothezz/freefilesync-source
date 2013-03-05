// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) Zenju (zenju AT gmx DOT de) - All Rights Reserved        *
// **************************************************************************

#ifndef HELPPROVIDER_H_INCLUDED
#define HELPPROVIDER_H_INCLUDED

#include <wx/help.h>
#include <zen/zstring.h>
#include "ffs_paths.h"

namespace zen
{
//use '/' as path separator!
void displayHelpEntry(wxWindow* parent);
void displayHelpEntry(const wxString& section, wxWindow* parent);













//######################## implementation ########################
inline
wxHelpController& getHelpCtrl()
{
    static wxHelpController controller; //external linkage, despite inline definition!
    static bool initialized = false;
    if (!initialized)
    {
        initialized = true;
        controller.Initialize(utfCvrtTo<wxString>(zen::getResourceDir()) +
#ifdef FFS_WIN
                              L"FreeFileSync.chm");
#elif defined FFS_LINUX || defined FFS_MAC
                              L"Help/FreeFileSync.hhp");
#endif
    }
    return controller;
}


inline
void displayHelpEntry(const wxString& section, wxWindow* parent)
{
    getHelpCtrl().SetParentWindow(parent); //this nicely solves modal issues on OSX with help file going to the background
    getHelpCtrl().DisplaySection(replaceCpy(section, L'/', utfCvrtTo<wxString>(FILE_NAME_SEPARATOR)));
    getHelpCtrl().SetParentWindow(nullptr);
}


inline
void displayHelpEntry(wxWindow* parent)
{
    getHelpCtrl().SetParentWindow(parent);
    getHelpCtrl().DisplayContents();
    getHelpCtrl().SetParentWindow(nullptr);
}
}

#endif // HELPPROVIDER_H_INCLUDED
