/***************************************************************
 * Purpose:   Defines Application Class
 * Author:    ZenJu (zhnmju123@gmx.de)
 * Created:   2009-07-06
 * Copyright: ZenJu (http://sourceforge.net/projects/freefilesync/)
 **************************************************************/

#ifndef REALTIMESYNCAPP_H
#define REALTIMESYNCAPP_H

#include <wx/app.h>
#include <wx/help.h>
#include <memory>

class Application : public wxApp
{
public:
    virtual bool OnInit();
    virtual int OnRun();
    virtual bool OnExceptionInMainLoop();

private:
    void OnStartApplication(wxIdleEvent& event);

    std::auto_ptr<wxHelpController> helpController; //global help controller
};

#endif // REALTIMESYNCAPP_H
