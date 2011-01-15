// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) 2008-2011 ZenJu (zhnmju123 AT gmx.de)                    *
// **************************************************************************
//
#ifndef REALTIMESYNCAPP_H
#define REALTIMESYNCAPP_H

#include <wx/app.h>
#include <memory>

class Application : public wxApp
{
public:
    virtual bool OnInit();
    virtual int OnRun();
    virtual bool OnExceptionInMainLoop();

private:
    void OnStartApplication(wxIdleEvent& event);

    virtual wxLayoutDirection GetLayoutDirection() const //disable RTL languages for now...
    {
        return wxLayout_LeftToRight;
    }

};

#endif // REALTIMESYNCAPP_H
