// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) 2008-2010 ZenJu (zhnmju123 AT gmx.de)                    *
// **************************************************************************
//
#include "resources.h"
#include <wx/wfstream.h>
#include <wx/zipstrm.h>
#include <wx/image.h>
#include <wx/icon.h>
#include <wx/mstream.h>
#include "../shared/string_conv.h"
#include "../shared/system_constants.h"
#include <memory>
#include "../shared/standard_paths.h"

using namespace ffs3;


const GlobalResources& GlobalResources::getInstance()
{
    static GlobalResources instance;
    return instance;
}


GlobalResources::GlobalResources()
{
    //init all the other resource files
    animationMoney = new wxAnimation(wxNullAnimation);
    animationSync  = new wxAnimation(wxNullAnimation);
    programIcon    = new wxIcon(wxNullIcon);
}


GlobalResources::~GlobalResources()
{
    //free bitmap resources
    for (std::map<wxString, wxBitmap*>::iterator i = bitmapResource.begin(); i != bitmapResource.end(); ++i)
        delete i->second;

    //free other resources
    delete animationMoney;
    delete animationSync;
    delete programIcon;
}


void loadAnimFromZip(wxZipInputStream& zipInput, wxAnimation* animation)
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

    animation->Load(seekAbleStream, wxANIMATION_TYPE_GIF);
}


void GlobalResources::load() const
{
    wxFFileInputStream input(ffs3::getResourceDir() + wxT("Resources.dat"));
    if (input.IsOk()) //if not... we don't want to react too harsh here
    {
        //activate support for .png files
        wxImage::AddHandler(new wxPNGHandler); //ownership passed

        wxZipInputStream resourceFile(input);

        while (true)
        {
            std::auto_ptr<wxZipEntry> entry(resourceFile.GetNextEntry());
            if (entry.get() == NULL)
                break;

            const wxString name = entry->GetName();

            //generic image loading
            if (name.EndsWith(wxT(".png")))
            {
                if (bitmapResource.find(name) == bitmapResource.end()) //avoid duplicate entry: prevent memory leak!
                    bitmapResource[name] = new wxBitmap(wxImage(resourceFile, wxBITMAP_TYPE_PNG));
            }
            else if (name == wxT("money.gif"))
                loadAnimFromZip(resourceFile, animationMoney);
            else if (name == wxT("working.gif"))
                loadAnimFromZip(resourceFile, animationSync);
        }
    }

#ifdef FFS_WIN
    //for compatibility it seems we need to stick with a "real" icon
    *programIcon = wxIcon(wxT("A_PROGRAM_ICON"));
#else
    //#include "FreeFileSync.xpm"
    //*programIcon = wxIcon(FreeFileSync_xpm);

    //use big logo bitmap for better quality
    programIcon->CopyFromBitmap(getImageByName(wxT("FreeFileSync.png")));
#endif
}


const wxBitmap& GlobalResources::getImageByName(const wxString& imageName) const
{
    const std::map<wxString, wxBitmap*>::const_iterator bmp = imageName.Find(wxChar('.')) == wxNOT_FOUND ? //assume .png ending if nothing else specified
            bitmapResource.find(imageName + wxT(".png")) :
            bitmapResource.find(imageName);

    if (bmp != bitmapResource.end())
        return *bmp->second;
    else
        return wxNullBitmap;
}
