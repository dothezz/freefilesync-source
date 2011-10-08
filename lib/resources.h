// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) 2008-2011 ZenJu (zhnmju123 AT gmx.de)                    *
// **************************************************************************

#ifndef RESOURCES_H_INCLUDED
#define RESOURCES_H_INCLUDED

#include <map>
#include <wx/bitmap.h>
#include <wx/animate.h>
#include <wx/icon.h>


class GlobalResources
{
public:
    static const GlobalResources& instance();

    static const wxBitmap& getImage(const wxString& name) { return instance().getImageInt(name); }

    //global image resource objects
    wxAnimation animationMoney;
    wxAnimation animationSync;
    wxIcon      programIcon;

private:
    GlobalResources();
    GlobalResources(const GlobalResources&);
    GlobalResources& operator=(const GlobalResources&);

    const wxBitmap& getImageInt(const wxString& name) const;

    std::map<wxString, wxBitmap> bitmaps;
};

#endif // RESOURCES_H_INCLUDED
