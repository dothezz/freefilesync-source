// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) Zenju (zenju AT gmx DOT de) - All Rights Reserved        *
// **************************************************************************

#include "resources.h"
#include <memory>
#include <wx/wfstream.h>
#include <wx/zipstrm.h>
#include <wx/image.h>
#include <wx/mstream.h>
#include "ffs_paths.h"

using namespace zen;


const GlobalResources& GlobalResources::instance()
{
    static GlobalResources inst;
    return inst;
}


namespace
{
void loadAnimFromZip(wxZipInputStream& zipInput, wxAnimation& anim)
{
    //Workaround for wxWidgets:
    //construct seekable input stream (zip-input stream is non-seekable) for wxAnimation::Load()
    //luckily this method call is very fast: below measurement precision!
    std::vector<char> data;
    data.reserve(10000);

    int newValue = 0;
    while ((newValue = zipInput.GetC()) != wxEOF)
        data.push_back(newValue);

    wxMemoryInputStream seekAbleStream(&data.front(), data.size()); //stream does not take ownership of data

    anim.Load(seekAbleStream, wxANIMATION_TYPE_GIF);
}
}


GlobalResources::GlobalResources()
{
    wxFFileInputStream input(toWx(zen::getResourceDir()) + L"Resources.zip");
    if (input.IsOk()) //if not... we don't want to react too harsh here
    {
        //activate support for .png files
        wxImage::AddHandler(new wxPNGHandler); //ownership passed

        wxZipInputStream resourceFile(input, wxConvUTF8);
        //do NOT rely on wxConvLocal! ON failure shows unhelpful popup "Cannot convert from the charset 'Unknown encoding (-1)'!"

        while (true)
        {
            std::unique_ptr<wxZipEntry> entry(resourceFile.GetNextEntry()); //take ownership!
            if (entry.get() == nullptr)
                break;

            const wxString name = entry->GetName();

            //generic image loading
            if (name.EndsWith(L".png"))
                bitmaps.insert(std::make_pair(name, wxImage(resourceFile, wxBITMAP_TYPE_PNG)));
            else if (name == L"wink.gif")
                loadAnimFromZip(resourceFile, aniWink);
            else if (name == L"working.gif")
                loadAnimFromZip(resourceFile, aniSync);
        }
    }

#ifdef FFS_WIN
    //for compatibility it seems we need to stick with a "real" icon
    programIcon = wxIcon(L"A_PROGRAM_ICON");
#else
    //use big logo bitmap for better quality
    programIcon.CopyFromBitmap(getImageInt(L"FreeFileSync.png"));
    //attention: this is the reason we need a member getImage -> it must not implicitly create static object instance!!!
    //erroneously calling static object constructor twice will deadlock on Linux!!
#endif
}


const wxBitmap& GlobalResources::getImageInt(const wxString& imageName) const
{
    auto iter = bitmaps.find(!contains(imageName, L'.') ? //assume .png ending if nothing else specified
                             imageName + L".png" :
                             imageName);
    if (iter != bitmaps.end())
        return iter->second;
    else
    {
        assert(false);
        return wxNullBitmap;
    }
}
