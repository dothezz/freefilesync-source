// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) ZenJu (zhnmju123 AT gmx DOT de) - All Rights Reserved    *
// **************************************************************************

#ifndef FREEFILESYNCAPP_H
#define FREEFILESYNCAPP_H

#include <wx/app.h>
#include "lib/process_xml.h"
#include "lib/return_codes.h"


class Application : public wxApp
{
public:
    bool OnInit();
    int  OnRun();
    int  OnExit();
    bool OnExceptionInMainLoop();

    void OnStartApplication(wxIdleEvent& event);
    void OnQueryEndSession(wxEvent& event);

private:
    void runGuiMode(const xmlAccess::XmlGuiConfig& guiCfg, xmlAccess::XmlGlobalSettings& settings);
    void runGuiMode(const std::vector<wxString>& cfgFileName, xmlAccess::XmlGlobalSettings& settings);
    void runBatchMode(const Zstring& filename, xmlAccess::XmlGlobalSettings& globSettings);

    xmlAccess::XmlGlobalSettings globalSettings; //settings used by GUI, batch mode or both
    zen::FfsReturnCode returnCode;
};

#endif // FREEFILESYNCAPP_H
