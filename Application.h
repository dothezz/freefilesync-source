// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) ZenJu (zenju AT gmx DOT de) - All Rights Reserved        *
// **************************************************************************

#ifndef FREEFILESYNCAPP_H
#define FREEFILESYNCAPP_H

#include <wx/app.h>
#include "lib/process_xml.h"
#include "lib/return_codes.h"


class Application : public wxApp
{
private:
    virtual bool OnInit();
    virtual int  OnRun();
    virtual int  OnExit() { return 0; }
    virtual bool OnExceptionInMainLoop();

    void OnStartApplication(wxIdleEvent& event);
    void OnQueryEndSession(wxEvent& event);

    zen::FfsReturnCode returnCode;
};

#endif // FREEFILESYNCAPP_H
