// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) 2008-2011 ZenJu (zhnmju123 AT gmx.de)                    *
// **************************************************************************

#include "resources.h"
#include <memory>
#include <wx/wfstream.h>
#include <wx/zipstrm.h>
#include <wx/image.h>
#include "../lib/ffs_paths.h"

using namespace zen;


const GlobalResources& GlobalResources::instance()
{
    static GlobalResources inst;
    return inst;
}


GlobalResources::GlobalResources()
{
    wxFFileInputStream input(toWx(zen::getResourceDir()) + wxT("Resources.zip"));
    if (input.IsOk()) //if not... we don't want to react too harsh here
    {
        //activate support for .png files
        wxImage::AddHandler(new wxPNGHandler); //ownership passed

        wxZipInputStream resourceFile(input);

        while (true)
        {
            std::unique_ptr<wxZipEntry> entry(resourceFile.GetNextEntry());
            if (entry.get() == NULL)
                break;

            const wxString name = entry->GetName();

            //generic image loading
            if (name.EndsWith(wxT(".png")))
                bitmaps.insert(std::make_pair(name, wxImage(resourceFile, wxBITMAP_TYPE_PNG)));
        }
    }

#ifdef FFS_WIN
    //for compatibility it seems we need to stick with a "real" icon
    programIcon = wxIcon(wxT("A_PROGRAM_ICON"));
#else
    //use big logo bitmap for better quality
    programIcon.CopyFromBitmap(getImageInt(wxT("RealtimeSync.png")));
#endif

}


const wxBitmap& GlobalResources::getImageInt(const wxString& name) const
{
    auto iter = bitmaps.find(name.Find(L'.') == wxNOT_FOUND ? //assume .png ending if nothing else specified
                             name + wxT(".png") :
                             name);
    if (iter != bitmaps.end())
        return iter->second;
    else
    {
        assert(false);
        return wxNullBitmap;
    }
}
