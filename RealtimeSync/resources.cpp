#include "resources.h"
#include <wx/wfstream.h>
#include <wx/zipstrm.h>
#include <wx/image.h>
#include <wx/icon.h>
#include <memory>
#include "../shared/standardPaths.h"
#include "../shared/systemConstants.h"

using namespace FreeFileSync;


const GlobalResources& GlobalResources::getInstance()
{
    static GlobalResources instance;
    return instance;
}


GlobalResources::GlobalResources()
{
    //map, allocate and initialize pictures
    bitmapResource[wxT("start red.png")]          = (bitmapStart             = new wxBitmap(wxNullBitmap));
    bitmapResource[wxT("add pair.png")]           = (bitmapAddFolderPair     = new wxBitmap(wxNullBitmap));
    bitmapResource[wxT("remove pair.png")]        = (bitmapRemoveFolderPair  = new wxBitmap(wxNullBitmap));

    programIcon    = new wxIcon(wxNullIcon);
}


GlobalResources::~GlobalResources()
{
    //free bitmap resources
    for (std::map<wxString, wxBitmap*>::iterator i = bitmapResource.begin(); i != bitmapResource.end(); ++i)
        delete i->second;

    //free other resources
    delete programIcon;
}


void GlobalResources::load() const
{
    wxFFileInputStream input(FreeFileSync::getInstallationDir() + wxT("Resources.dat"));
    if (input.IsOk()) //if not... we don't want to react too harsh here
    {
        //activate support for .png files
        wxImage::AddHandler(new wxPNGHandler); //ownership passed

        wxZipInputStream resourceFile(input);

        std::map<wxString, wxBitmap*>::iterator bmp;
        while (true)
        {
            std::auto_ptr<wxZipEntry> entry(resourceFile.GetNextEntry());
            if (entry.get() == NULL)
                break;

            const wxString name = entry->GetName();

            //search if entry is available in map
            if ((bmp = bitmapResource.find(name)) != bitmapResource.end())
                *(bmp->second) = wxBitmap(wxImage(resourceFile, wxBITMAP_TYPE_PNG));
        }
    }

#ifdef FFS_WIN
    *programIcon = wxIcon(wxT("A_PROGRAM_ICON"));
#else
#include "RealtimeSync.xpm"
    *programIcon = wxIcon(RealtimeSync_xpm);
#endif
}


const wxBitmap& GlobalResources::getImageByName(const wxString& imageName) const
{
    std::map<wxString, wxBitmap*>::const_iterator bmp = bitmapResource.find(imageName);
    if (bmp != bitmapResource.end())
        return *bmp->second;
    else
        return wxNullBitmap;
}

