// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) 2008-2011 ZenJu (zhnmju123 AT gmx.de)                    *
// **************************************************************************

#ifndef RESOURCES_H_INCLUDED
#define RESOURCES_H_INCLUDED

#include <wx/bitmap.h>
#include <wx/animate.h>
#include <wx/string.h>
#include <map>


class GlobalResources
{
public:
    static const wxBitmap& getImage(const wxString& name)
    {
        const GlobalResources& inst = instance();
        return inst.getImageInt(name);
    }

    static const GlobalResources& instance();

    //global image resource objects
    wxAnimation* animationMoney;
    wxAnimation* animationSync;
    wxIcon* programIcon;

private:
    GlobalResources();
    ~GlobalResources();
    GlobalResources(const GlobalResources&);
    GlobalResources& operator=(const GlobalResources&);

    const wxBitmap& getImageInt(const wxString& name) const;


    //resource mapping
    std::map<wxString, wxBitmap*> bitmapResource;
};

#endif // RESOURCES_H_INCLUDED
