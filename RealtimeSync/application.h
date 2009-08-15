/***************************************************************
 * Purpose:   Defines Application Class
 * Author:    ZenJu (zhnmju123@gmx.de)
 * Created:   2009-07-06
 * Copyright: ZenJu (http://sourceforge.net/projects/freefilesync/)
 **************************************************************/

#ifndef REALTIMESYNCAPP_H
#define REALTIMESYNCAPP_H

#include <wx/app.h>

class Application : public wxApp
{
public:
    virtual bool OnInit();

private:
    void OnStartApplication(wxIdleEvent& event);
};

#endif // REALTIMESYNCAPP_H
