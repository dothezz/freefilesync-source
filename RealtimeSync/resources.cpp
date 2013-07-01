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

#ifdef ZEN_WIN
    //for compatibility it seems we need to stick with a "real" icon
    programIconRTS = wxIcon(L"A_RTS_ICON");

#elif defined ZEN_LINUX
    programIconRTS.CopyFromBitmap(getImage(L"RealtimeSync"));

#elif defined ZEN_MAC
    assert(getImage(L"RealtimeSync").GetWidth () == getImage(L"RealtimeSync").GetHeight() &&
           getImage(L"RealtimeSync").GetWidth() % 128 == 0);
    //wxWidgets' bitmap to icon conversion on OS X can only deal with very specific sizes
    programIconRTS.CopyFromBitmap(getImage(L"RealtimeSync").ConvertToImage().Scale(128, 128, wxIMAGE_QUALITY_HIGH));
#endif

}


const wxBitmap& GlobalResources::getImage(const wxString& name) const
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
