// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) Zenju (zenju AT gmx DOT de) - All Rights Reserved        *
// **************************************************************************

#ifndef FREEFILESYNCAPP_H
#define FREEFILESYNCAPP_H

#include <vector>
#include <wx/app.h>
//#include "lib/process_xml.h"
#include "lib/return_codes.h"


class Application : public wxApp
{
public:
    Application() : returnCode(zen::FFS_RC_SUCCESS) {}

private:
    virtual bool OnInit();
    virtual int  OnRun();
    virtual int  OnExit() { return 0; }
    virtual bool OnExceptionInMainLoop() { throw; } //just re-throw and avoid display of additional messagebox: it will be caught in OnRun()

#ifdef FFS_MAC
    virtual void MacOpenFiles(const wxArrayString& filenames);
    virtual void MacNewFile();
#endif

    void onEnterEventLoop(wxEvent& event);
    void onQueryEndSession(wxEvent& event);
    void launch(const std::vector<wxString>& commandArgs);

    zen::FfsReturnCode returnCode;
};

#endif // FREEFILESYNCAPP_H
