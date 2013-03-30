// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) Zenju (zenju AT gmx DOT de) - All Rights Reserved        *
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

    const wxBitmap& getImage(const wxString& name) const;

    //global image resource objects
    wxAnimation aniWink;
    wxAnimation aniSync;
    wxIcon      programIcon;

private:
    GlobalResources();
    GlobalResources(const GlobalResources&);
    GlobalResources& operator=(const GlobalResources&);

    std::map<wxString, wxBitmap> bitmaps;
};


inline
const wxBitmap& getResourceImage(const wxString& name) { return GlobalResources::instance().getImage(name); }

#endif // RESOURCES_H_INCLUDED
