/***************************************************************
 * Name:      FreeFileSyncApp.h
 * Purpose:   Defines Application Class
 * Author:    ZenJu (zhnmju123@gmx.de)
 * Created:   2008-07-16
 **************************************************************/

#ifndef FREEFILESYNCAPP_H
#define FREEFILESYNCAPP_H

#include <wx/app.h>
#include "library/localization.h"
#include "library/processXml.h"


class Application : public wxApp
{
public:
    bool OnInit();
    int  OnRun();
    int  OnExit();
    bool OnExceptionInMainLoop();
    void OnStartApplication(wxIdleEvent& event);

private:
    void runBatchMode(const wxString& filename, xmlAccess::XmlGlobalSettings& globalSettings);

    FreeFileSync::CustomLocale programLanguage;

    int returnValue;
    xmlAccess::XmlGlobalSettings globalSettings; //settings used by GUI, batch mode or both
};

#endif // FREEFILESYNCAPP_H
