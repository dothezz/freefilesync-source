#include "resources.h"
#include <wx/wfstream.h>
#include <wx/zipstrm.h>
#include <wx/image.h>
#include <wx/icon.h>
#include "globalFunctions.h"

#ifdef FFS_WIN
wxChar GlobalResources::fileNameSeparator = '\\';
#elif defined FFS_LINUX
wxChar GlobalResources::fileNameSeparator = '/';
#else
assert(false);
#endif

//these two global variables are language-dependent => cannot be set statically! See CustomLocale
const wxChar* GlobalResources::decimalPoint       = wxEmptyString;
const wxChar* GlobalResources::thousandsSeparator = wxEmptyString;


//command line parameters
const wxChar* GlobalResources::paramCompare       = wxT("comp");
const wxChar* GlobalResources::paramSync          = wxT("sync");
const wxChar* GlobalResources::paramInclude       = wxT("incl");
const wxChar* GlobalResources::paramExclude       = wxT("excl");
const wxChar* GlobalResources::paramContinueError = wxT("continue");
const wxChar* GlobalResources::paramRecycler      = wxT("recycler");
const wxChar* GlobalResources::paramSilent        = wxT("silent");

const wxChar* GlobalResources::valueSizeDate      = wxT("SIZEDATE");
const wxChar* GlobalResources::valueContent       = wxT("CONTENT");


map<wxString, wxBitmap*> GlobalResources::bitmapResource;

wxBitmap* GlobalResources::bitmapArrowLeft       = 0;
wxBitmap* GlobalResources::bitmapArrowRight      = 0;
wxBitmap* GlobalResources::bitmapArrowLeftCr     = 0;
wxBitmap* GlobalResources::bitmapArrowRightCr    = 0;
wxBitmap* GlobalResources::bitmapArrowNone       = 0;
wxBitmap* GlobalResources::bitmapStartSync       = 0;
wxBitmap* GlobalResources::bitmapStartSyncDis    = 0;
wxBitmap* GlobalResources::bitmapDeleteLeft      = 0;
wxBitmap* GlobalResources::bitmapDeleteRight     = 0;
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
wxBitmap* GlobalResources::bitmapStatusEdge      = 0;

wxAnimation* GlobalResources::animationMoney     = 0;
wxAnimation* GlobalResources::animationSync      = 0;
wxIcon*      GlobalResources::programIcon        = 0;


void GlobalResources::loadResourceFiles()
{
    //map, allocate and initialize pictures
    bitmapResource[wxT("left arrow.png")]        = (bitmapArrowLeft       = new wxBitmap(wxNullBitmap));
    bitmapResource[wxT("right arrow.png")]       = (bitmapArrowRight      = new wxBitmap(wxNullBitmap));
    bitmapResource[wxT("left arrow create.png")] = (bitmapArrowLeftCr     = new wxBitmap(wxNullBitmap));
    bitmapResource[wxT("right arrow create.png")]= (bitmapArrowRightCr    = new wxBitmap(wxNullBitmap));
    bitmapResource[wxT("no arrow.png")]          = (bitmapArrowNone       = new wxBitmap(wxNullBitmap));
    bitmapResource[wxT("start sync.png")]        = (bitmapStartSync       = new wxBitmap(wxNullBitmap));
    bitmapResource[wxT("start sync dis.png")]    = (bitmapStartSyncDis    = new wxBitmap(wxNullBitmap));
    bitmapResource[wxT("left delete.png")]       = (bitmapDeleteLeft      = new wxBitmap(wxNullBitmap));
    bitmapResource[wxT("right delete.png")]      = (bitmapDeleteRight     = new wxBitmap(wxNullBitmap));
    bitmapResource[wxT("email.png")]             = (bitmapEmail           = new wxBitmap(wxNullBitmap));
    bitmapResource[wxT("about.png")]             = (bitmapAbout           = new wxBitmap(wxNullBitmap));
    bitmapResource[wxT("website.png")]           = (bitmapWebsite         = new wxBitmap(wxNullBitmap));
    bitmapResource[wxT("exit.png")]              = (bitmapExit            = new wxBitmap(wxNullBitmap));
    bitmapResource[wxT("sync.png")]              = (bitmapSync            = new wxBitmap(wxNullBitmap));
    bitmapResource[wxT("compare.png")]           = (bitmapCompare         = new wxBitmap(wxNullBitmap));
    bitmapResource[wxT("sync disabled.png")]     = (bitmapSyncDisabled    = new wxBitmap(wxNullBitmap));
    bitmapResource[wxT("swap.png")]              = (bitmapSwap            = new wxBitmap(wxNullBitmap));
    bitmapResource[wxT("help.png")]              = (bitmapHelp            = new wxBitmap(wxNullBitmap));
    bitmapResource[wxT("leftOnly.png")]          = (bitmapLeftOnly        = new wxBitmap(wxNullBitmap));
    bitmapResource[wxT("leftNewer.png")]         = (bitmapLeftNewer       = new wxBitmap(wxNullBitmap));
    bitmapResource[wxT("different.png")]         = (bitmapDifferent       = new wxBitmap(wxNullBitmap));
    bitmapResource[wxT("rightNewer.png")]        = (bitmapRightNewer      = new wxBitmap(wxNullBitmap));
    bitmapResource[wxT("rightOnly.png")]         = (bitmapRightOnly       = new wxBitmap(wxNullBitmap));
    bitmapResource[wxT("leftOnlyDeact.png")]     = (bitmapLeftOnlyDeact   = new wxBitmap(wxNullBitmap));
    bitmapResource[wxT("leftNewerDeact.png")]    = (bitmapLeftNewerDeact  = new wxBitmap(wxNullBitmap));
    bitmapResource[wxT("differentDeact.png")]    = (bitmapDifferentDeact  = new wxBitmap(wxNullBitmap));
    bitmapResource[wxT("rightNewerDeact.png")]   = (bitmapRightNewerDeact = new wxBitmap(wxNullBitmap));
    bitmapResource[wxT("rightOnlyDeact.png")]    = (bitmapRightOnlyDeact  = new wxBitmap(wxNullBitmap));
    bitmapResource[wxT("equal.png")]             = (bitmapEqual           = new wxBitmap(wxNullBitmap));
    bitmapResource[wxT("equalDeact.png")]        = (bitmapEqualDeact      = new wxBitmap(wxNullBitmap));
    bitmapResource[wxT("include.png")]           = (bitmapInclude         = new wxBitmap(wxNullBitmap));
    bitmapResource[wxT("exclude.png")]           = (bitmapExclude         = new wxBitmap(wxNullBitmap));
    bitmapResource[wxT("filter active.png")]     = (bitmapFilterOn        = new wxBitmap(wxNullBitmap));
    bitmapResource[wxT("filter not active.png")] = (bitmapFilterOff       = new wxBitmap(wxNullBitmap));
    bitmapResource[wxT("warning.png")]           = (bitmapWarning         = new wxBitmap(wxNullBitmap));
    bitmapResource[wxT("small arrow up.png"])    = (bitmapSmallUp         = new wxBitmap(wxNullBitmap));
    bitmapResource[wxT("small arrow down.png")]  = (bitmapSmallDown       = new wxBitmap(wxNullBitmap));
    bitmapResource[wxT("save.png")]              = (bitmapSave            = new wxBitmap(wxNullBitmap));
    bitmapResource[wxT("FFS.png")]               = (bitmapFFS             = new wxBitmap(wxNullBitmap));
    bitmapResource[wxT("deleteFile.png")]        = (bitmapDeleteFile      = new wxBitmap(wxNullBitmap));
    bitmapResource[wxT("gpl.png")]               = (bitmapGPL             = new wxBitmap(wxNullBitmap));
    bitmapResource[wxT("statusPause.png")]       = (bitmapStatusPause     = new wxBitmap(wxNullBitmap));
    bitmapResource[wxT("statusError.png")]       = (bitmapStatusError     = new wxBitmap(wxNullBitmap));
    bitmapResource[wxT("statusSuccess.png")]     = (bitmapStatusSuccess   = new wxBitmap(wxNullBitmap));
    bitmapResource[wxT("statusWarning.png")]     = (bitmapStatusWarning   = new wxBitmap(wxNullBitmap));
    bitmapResource[wxT("statusScanning.png")]    = (bitmapStatusScanning  = new wxBitmap(wxNullBitmap));
    bitmapResource[wxT("statusComparing.png")]   = (bitmapStatusComparing = new wxBitmap(wxNullBitmap));
    bitmapResource[wxT("statusSyncing.png")]     = (bitmapStatusSyncing   = new wxBitmap(wxNullBitmap));
    bitmapResource[wxT("logo.png")]              = (bitmapLogo            = new wxBitmap(wxNullBitmap));
    bitmapResource[wxT("finished.png")]          = (bitmapFinished        = new wxBitmap(wxNullBitmap));
    bitmapResource[wxT("statusEdge.png")]        = (bitmapStatusEdge      = new wxBitmap(wxNullBitmap));

    wxFileInputStream input(wxT("Resources.dat"));
    if (!input.IsOk()) throw RuntimeException(_("Unable to load Resources.dat!"));

    wxZipInputStream resourceFile(input);

    wxZipEntry* entry;
    map<wxString, wxBitmap*>::iterator bmp;
    while ((entry = resourceFile.GetNextEntry()))
    {
        wxString name = entry->GetName();

        //just to be sure: search if entry is available in map
        if ((bmp = bitmapResource.find(name)) != bitmapResource.end())
            *(bmp->second) = wxBitmap(wxImage(resourceFile, wxBITMAP_TYPE_PNG));
    }

    //load all the other resource files
    animationMoney = new wxAnimation(wxNullAnimation);
    animationSync  = new wxAnimation(wxNullAnimation);

    animationMoney->LoadFile(wxT("Resources.a01"));
    animationSync->LoadFile(wxT("Resources.a02"));

#ifdef FFS_WIN
    programIcon = new wxIcon(wxT("ffsIcon1"));
#else
#include "FreeFileSync.xpm"
    programIcon = new wxIcon(FreeFileSync_xpm);
#endif
}


void GlobalResources::unloadResourceFiles()
{
    //free bitmap resources
    for (map<wxString, wxBitmap*>::iterator i = bitmapResource.begin(); i != bitmapResource.end(); ++i)
        delete i->second;

    //free other resources
    delete animationMoney;
    delete animationSync;
    delete programIcon;
}
