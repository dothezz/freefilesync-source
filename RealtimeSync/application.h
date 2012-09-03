// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) ZenJu (zenju AT gmx DOT de) - All Rights Reserved        *
// **************************************************************************

#ifndef REALTIMESYNCAPP_H
#define REALTIMESYNCAPP_H

#include <wx/app.h>

class Application : public wxApp
{
public:
    virtual bool OnInit();
    virtual int OnRun();
    virtual bool OnExceptionInMainLoop();

private:
    void OnStartApplication(wxIdleEvent& event);
    //virtual wxLayoutDirection GetLayoutDirection() const { return wxLayout_LeftToRight; }
};

#endif // REALTIMESYNCAPP_H
