// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) Zenju (zenju AT gmx DOT de) - All Rights Reserved        *
// **************************************************************************

#ifndef RESOURCES_H_INCLUDED_870857342085670826521345
#define RESOURCES_H_INCLUDED_870857342085670826521345

#include <map>
#include <wx/bitmap.h>
#include <wx/icon.h>


class GlobalResources
{
public:
    static const GlobalResources& instance();

    static const wxBitmap& getImage(const wxString& name) { return instance().getImageInt(name); }

    //image resource objects
    wxIcon programIcon;

private:
    GlobalResources();
    GlobalResources(const GlobalResources&); //=delete
    GlobalResources& operator=(const GlobalResources&); //=delete

    const wxBitmap& getImageInt(const wxString& name) const;

    std::map<wxString, wxBitmap> bitmaps;
};

#endif //RESOURCES_H_INCLUDED_870857342085670826521345
