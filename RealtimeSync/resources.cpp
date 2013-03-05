// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) Zenju (zenju AT gmx DOT de) - All Rights Reserved        *
// **************************************************************************

#include "resources.h"
#include <memory>
#include <zen/utf.h>
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
    wxFFileInputStream input(utfCvrtTo<wxString>(zen::getResourceDir()) + L"Resources.zip");
    if (input.IsOk()) //if not... we don't want to react too harsh here
    {
        //activate support for .png files
        wxImage::AddHandler(new wxPNGHandler); //ownership passed

        wxZipInputStream resourceFile(input, wxConvUTF8);
        //do NOT rely on wxConvLocal! May result in "Cannot convert from the charset 'Unknown encoding (-1)'!"

        while (true)
        {
            std::unique_ptr<wxZipEntry> entry(resourceFile.GetNextEntry());
            if (entry.get() == nullptr)
                break;

            const wxString name = entry->GetName();

            //generic image loading
            if (endsWith(name, L".png"))
                bitmaps.insert(std::make_pair(name, wxImage(resourceFile, wxBITMAP_TYPE_PNG)));
        }
    }

#ifdef FFS_WIN
    //for compatibility it seems we need to stick with a "real" icon
    programIcon = wxIcon(L"A_PROGRAM_ICON");

#elif defined FFS_LINUX || defined FFS_MAC
    //use big logo bitmap for better quality
    programIcon.CopyFromBitmap(getImageInt(L"RealtimeSync"));
#endif
}


const wxBitmap& GlobalResources::getImageInt(const wxString& name) const
{
    auto it = bitmaps.find(!contains(name, L'.') ? //assume .png ending if nothing else specified
                           name + L".png" :
                           name);
    if (it != bitmaps.end())
        return it->second;
    else
    {
        assert(false);
        return wxNullBitmap;
    }
}
