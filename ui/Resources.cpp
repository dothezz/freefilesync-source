#include "resources.h"
#include <wx/wfstream.h>
#include <wx/zipstrm.h>
#include <wx/image.h>
#include <stdexcept> //for std::runtime_error

#ifdef FFS_WIN
wxChar GlobalResources::fileNameSeparator = '\\';
#endif  // FFS_WIN

#ifdef FFS_LINUX
wxChar GlobalResources::fileNameSeparator = '/';
#endif  // FFS_LINUX

const wxChar* GlobalResources::floatingPointSeparator = "";
const wxChar* GlobalResources::numberSeparator = "";


map<wxString, wxBitmap*> GlobalResources::bitmapResource;

wxBitmap* GlobalResources::bitmapLeftArrow       = 0;
wxBitmap* GlobalResources::bitmapStartSync       = 0;
wxBitmap* GlobalResources::bitmapRightArrow      = 0;
wxBitmap* GlobalResources::bitmapDelete          = 0;
wxBitmap* GlobalResources::bitmapEmail           = 0;
wxBitmap* GlobalResources::bitmapAbout           = 0;
wxBitmap* GlobalResources::bitmapWebsite         = 0;
wxBitmap* GlobalResources::bitmapExit            = 0;
wxBitmap* GlobalResources::bitmapSync            = 0;
wxBitmap* GlobalResources::bitmapCompare         = 0;
wxBitmap* GlobalResources::bitmapSyncDisabled    = 0;
wxBitmap* GlobalResources::bitmapSwap            = 0;
wxBitmap* GlobalResources::bitmapHelp            = 0;
wxBitmap* GlobalResources::bitmapLeftOnly        = 0;
wxBitmap* GlobalResources::bitmapLeftNewer       = 0;
wxBitmap* GlobalResources::bitmapDifferent       = 0;
wxBitmap* GlobalResources::bitmapRightNewer      = 0;
wxBitmap* GlobalResources::bitmapRightOnly       = 0;
wxBitmap* GlobalResources::bitmapLeftOnlyDeact   = 0;
wxBitmap* GlobalResources::bitmapLeftNewerDeact  = 0;
wxBitmap* GlobalResources::bitmapDifferentDeact  = 0;
wxBitmap* GlobalResources::bitmapRightNewerDeact = 0;
wxBitmap* GlobalResources::bitmapRightOnlyDeact  = 0;
wxBitmap* GlobalResources::bitmapEqual           = 0;
wxBitmap* GlobalResources::bitmapEqualDeact      = 0;
wxBitmap* GlobalResources::bitmapInclude         = 0;
wxBitmap* GlobalResources::bitmapExclude         = 0;
wxBitmap* GlobalResources::bitmapFilterOn        = 0;
wxBitmap* GlobalResources::bitmapFilterOff       = 0;
wxBitmap* GlobalResources::bitmapWarning         = 0;
wxBitmap* GlobalResources::bitmapSmallUp         = 0;
wxBitmap* GlobalResources::bitmapSmallDown       = 0;
wxBitmap* GlobalResources::bitmapSave            = 0;
wxBitmap* GlobalResources::bitmapFFS             = 0;
wxBitmap* GlobalResources::bitmapDeleteFile      = 0;
wxBitmap* GlobalResources::bitmapGPL             = 0;
wxBitmap* GlobalResources::bitmapStatusPause     = 0;
wxBitmap* GlobalResources::bitmapStatusError     = 0;
wxBitmap* GlobalResources::bitmapStatusSuccess   = 0;
wxBitmap* GlobalResources::bitmapStatusWarning   = 0;
wxBitmap* GlobalResources::bitmapStatusScanning  = 0;
wxBitmap* GlobalResources::bitmapStatusComparing = 0;
wxBitmap* GlobalResources::bitmapStatusSyncing   = 0;
wxBitmap* GlobalResources::bitmapLogo            = 0;
wxBitmap* GlobalResources::bitmapFinished        = 0;

wxAnimation* GlobalResources::animationMoney     = 0;
wxAnimation* GlobalResources::animationSync      = 0;


void GlobalResources::loadResourceFiles()
{
    floatingPointSeparator = _(".");
    numberSeparator = _(",");


    //map, allocate and initialize pictures
    bitmapResource["left arrow.png"]        = (bitmapLeftArrow       = new wxBitmap(wxNullBitmap));
    bitmapResource["start sync.png"]        = (bitmapStartSync       = new wxBitmap(wxNullBitmap));
    bitmapResource["right arrow.png"]       = (bitmapRightArrow      = new wxBitmap(wxNullBitmap));
    bitmapResource["delete.png"]            = (bitmapDelete          = new wxBitmap(wxNullBitmap));
    bitmapResource["email.png"]             = (bitmapEmail           = new wxBitmap(wxNullBitmap));
    bitmapResource["about.png"]             = (bitmapAbout           = new wxBitmap(wxNullBitmap));
    bitmapResource["website.png"]           = (bitmapWebsite         = new wxBitmap(wxNullBitmap));
    bitmapResource["exit.png"]              = (bitmapExit            = new wxBitmap(wxNullBitmap));
    bitmapResource["sync.png"]              = (bitmapSync            = new wxBitmap(wxNullBitmap));
    bitmapResource["compare.png"]           = (bitmapCompare         = new wxBitmap(wxNullBitmap));
    bitmapResource["sync disabled.png"]     = (bitmapSyncDisabled    = new wxBitmap(wxNullBitmap));
    bitmapResource["swap.png"]              = (bitmapSwap            = new wxBitmap(wxNullBitmap));
    bitmapResource["help.png"]              = (bitmapHelp            = new wxBitmap(wxNullBitmap));
    bitmapResource["leftOnly.png"]          = (bitmapLeftOnly        = new wxBitmap(wxNullBitmap));
    bitmapResource["leftNewer.png"]         = (bitmapLeftNewer       = new wxBitmap(wxNullBitmap));
    bitmapResource["different.png"]         = (bitmapDifferent       = new wxBitmap(wxNullBitmap));
    bitmapResource["rightNewer.png"]        = (bitmapRightNewer      = new wxBitmap(wxNullBitmap));
    bitmapResource["rightOnly.png"]         = (bitmapRightOnly       = new wxBitmap(wxNullBitmap));
    bitmapResource["leftOnlyDeact.png"]     = (bitmapLeftOnlyDeact   = new wxBitmap(wxNullBitmap));
    bitmapResource["leftNewerDeact.png"]    = (bitmapLeftNewerDeact  = new wxBitmap(wxNullBitmap));
    bitmapResource["differentDeact.png"]    = (bitmapDifferentDeact  = new wxBitmap(wxNullBitmap));
    bitmapResource["rightNewerDeact.png"]   = (bitmapRightNewerDeact = new wxBitmap(wxNullBitmap));
    bitmapResource["rightOnlyDeact.png"]    = (bitmapRightOnlyDeact  = new wxBitmap(wxNullBitmap));
    bitmapResource["equal.png"]             = (bitmapEqual           = new wxBitmap(wxNullBitmap));
    bitmapResource["equalDeact.png"]        = (bitmapEqualDeact      = new wxBitmap(wxNullBitmap));
    bitmapResource["include.png"]           = (bitmapInclude         = new wxBitmap(wxNullBitmap));
    bitmapResource["exclude.png"]           = (bitmapExclude         = new wxBitmap(wxNullBitmap));
    bitmapResource["filter active.png"]     = (bitmapFilterOn        = new wxBitmap(wxNullBitmap));
    bitmapResource["filter not active.png"] = (bitmapFilterOff       = new wxBitmap(wxNullBitmap));
    bitmapResource["warning.png"]           = (bitmapWarning         = new wxBitmap(wxNullBitmap));
    bitmapResource["small arrow up.png"]    = (bitmapSmallUp         = new wxBitmap(wxNullBitmap));
    bitmapResource["small arrow down.png"]  = (bitmapSmallDown       = new wxBitmap(wxNullBitmap));
    bitmapResource["save.png"]              = (bitmapSave            = new wxBitmap(wxNullBitmap));
    bitmapResource["FFS.png"]               = (bitmapFFS             = new wxBitmap(wxNullBitmap));
    bitmapResource["deleteFile.png"]        = (bitmapDeleteFile      = new wxBitmap(wxNullBitmap));
    bitmapResource["gpl.png"]               = (bitmapGPL             = new wxBitmap(wxNullBitmap));
    bitmapResource["statusPause.png"]       = (bitmapStatusPause     = new wxBitmap(wxNullBitmap));
    bitmapResource["statusError.png"]       = (bitmapStatusError     = new wxBitmap(wxNullBitmap));
    bitmapResource["statusSuccess.png"]     = (bitmapStatusSuccess   = new wxBitmap(wxNullBitmap));
    bitmapResource["statusWarning.png"]     = (bitmapStatusWarning   = new wxBitmap(wxNullBitmap));
    bitmapResource["statusScanning.png"]    = (bitmapStatusScanning  = new wxBitmap(wxNullBitmap));
    bitmapResource["statusComparing.png"]   = (bitmapStatusComparing = new wxBitmap(wxNullBitmap));
    bitmapResource["statusSyncing.png"]     = (bitmapStatusSyncing   = new wxBitmap(wxNullBitmap));
    bitmapResource["logo.png"]              = (bitmapLogo            = new wxBitmap(wxNullBitmap));
    bitmapResource["finished.png"]          = (bitmapFinished        = new wxBitmap(wxNullBitmap));

    animationMoney = new wxAnimation(wxNullAnimation);
    animationSync  = new wxAnimation(wxNullAnimation);

    wxFileInputStream input("Resources.dat");
    if (!input.IsOk()) throw runtime_error(_("Unable to load Resources.dat!"));

    wxZipInputStream resourceFile(input);

    wxZipEntry* entry;
    map<wxString, wxBitmap*>::iterator bmp;
    while (entry = resourceFile.GetNextEntry())
    {
        wxString name = entry->GetName();

        //just to be sure: search if entry is available in map
        if ((bmp = bitmapResource.find(name)) != bitmapResource.end())
            *(bmp->second) = wxBitmap(wxImage(resourceFile, wxBITMAP_TYPE_PNG));
    }

    animationMoney->LoadFile("Resources.a01");
    animationSync->LoadFile("Resources.a02");
}


void GlobalResources::unloadResourceFiles()
{
    //free bitmap resources
    for (map<wxString, wxBitmap*>::iterator i = bitmapResource.begin(); i != bitmapResource.end(); ++i)
        delete i->second;

    //free other resources
    delete animationMoney;
    delete animationSync;
}
