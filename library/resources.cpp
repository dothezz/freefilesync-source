#include "resources.h"
#include <wx/wfstream.h>
#include <wx/zipstrm.h>
#include <wx/image.h>
#include <wx/icon.h>
#include <wx/mstream.h>
#include "globalFunctions.h"

#ifdef FFS_WIN
const wxChar GlobalResources::FILE_NAME_SEPARATOR = '\\';
#elif defined FFS_LINUX
const wxChar GlobalResources::FILE_NAME_SEPARATOR = '/';
#else
assert(false);
#endif

//these two global variables are language-dependent => cannot be set statically! See CustomLocale
const wxChar* GlobalResources::DECIMAL_POINT       = wxEmptyString;
const wxChar* GlobalResources::THOUSANDS_SEPARATOR = wxEmptyString;

GlobalResources globalResource;  //init resources on program startup

GlobalResources::GlobalResources()
{
    //map, allocate and initialize pictures
    bitmapResource[wxT("left arrow.png")]         = (bitmapArrowLeft         = new wxBitmap(wxNullBitmap));
    bitmapResource[wxT("right arrow.png")]        = (bitmapArrowRight        = new wxBitmap(wxNullBitmap));
    bitmapResource[wxT("left arrow create.png")]  = (bitmapArrowLeftCr       = new wxBitmap(wxNullBitmap));
    bitmapResource[wxT("right arrow create.png")] = (bitmapArrowRightCr      = new wxBitmap(wxNullBitmap));
    bitmapResource[wxT("no arrow.png")]           = (bitmapArrowNone         = new wxBitmap(wxNullBitmap));
    bitmapResource[wxT("start sync.png")]         = (bitmapStartSync         = new wxBitmap(wxNullBitmap));
    bitmapResource[wxT("start sync dis.png")]     = (bitmapStartSyncDis      = new wxBitmap(wxNullBitmap));
    bitmapResource[wxT("left delete.png")]        = (bitmapDeleteLeft        = new wxBitmap(wxNullBitmap));
    bitmapResource[wxT("right delete.png")]       = (bitmapDeleteRight       = new wxBitmap(wxNullBitmap));
    bitmapResource[wxT("email.png")]              = (bitmapEmail             = new wxBitmap(wxNullBitmap));
    bitmapResource[wxT("about.png")]              = (bitmapAbout             = new wxBitmap(wxNullBitmap));
    bitmapResource[wxT("website.png")]            = (bitmapWebsite           = new wxBitmap(wxNullBitmap));
    bitmapResource[wxT("exit.png")]               = (bitmapExit              = new wxBitmap(wxNullBitmap));
    bitmapResource[wxT("sync.png")]               = (bitmapSync              = new wxBitmap(wxNullBitmap));
    bitmapResource[wxT("compare.png")]            = (bitmapCompare           = new wxBitmap(wxNullBitmap));
    bitmapResource[wxT("sync disabled.png")]      = (bitmapSyncDisabled      = new wxBitmap(wxNullBitmap));
    bitmapResource[wxT("swap.png")]               = (bitmapSwap              = new wxBitmap(wxNullBitmap));
    bitmapResource[wxT("help.png")]               = (bitmapHelp              = new wxBitmap(wxNullBitmap));
    bitmapResource[wxT("equal.png")]              = (bitmapEqual             = new wxBitmap(wxNullBitmap));
    bitmapResource[wxT("equalAct.png")]           = (bitmapEqualAct          = new wxBitmap(wxNullBitmap));
    bitmapResource[wxT("equalDeact.png")]         = (bitmapEqualDeact        = new wxBitmap(wxNullBitmap));
    bitmapResource[wxT("leftOnly.png")]           = (bitmapLeftOnly          = new wxBitmap(wxNullBitmap));
    bitmapResource[wxT("leftOnlyAct.png")]        = (bitmapLeftOnlyAct       = new wxBitmap(wxNullBitmap));
    bitmapResource[wxT("leftOnlyDeact.png")]      = (bitmapLeftOnlyDeact     = new wxBitmap(wxNullBitmap));
    bitmapResource[wxT("leftNewer.png")]          = (bitmapLeftNewer         = new wxBitmap(wxNullBitmap));
    bitmapResource[wxT("leftNewerAct.png")]       = (bitmapLeftNewerAct      = new wxBitmap(wxNullBitmap));
    bitmapResource[wxT("leftNewerDeact.png")]     = (bitmapLeftNewerDeact    = new wxBitmap(wxNullBitmap));
    bitmapResource[wxT("different.png")]          = (bitmapDifferent         = new wxBitmap(wxNullBitmap));
    bitmapResource[wxT("differentAct.png")]       = (bitmapDifferentAct      = new wxBitmap(wxNullBitmap));
    bitmapResource[wxT("differentDeact.png")]     = (bitmapDifferentDeact    = new wxBitmap(wxNullBitmap));
    bitmapResource[wxT("rightNewer.png")]         = (bitmapRightNewer        = new wxBitmap(wxNullBitmap));
    bitmapResource[wxT("rightNewerAct.png")]      = (bitmapRightNewerAct     = new wxBitmap(wxNullBitmap));
    bitmapResource[wxT("rightNewerDeact.png")]    = (bitmapRightNewerDeact   = new wxBitmap(wxNullBitmap));
    bitmapResource[wxT("rightOnly.png")]          = (bitmapRightOnly         = new wxBitmap(wxNullBitmap));
    bitmapResource[wxT("rightOnlyAct.png")]       = (bitmapRightOnlyAct      = new wxBitmap(wxNullBitmap));
    bitmapResource[wxT("rightOnlyDeact.png")]     = (bitmapRightOnlyDeact    = new wxBitmap(wxNullBitmap));
    bitmapResource[wxT("include.png")]            = (bitmapInclude           = new wxBitmap(wxNullBitmap));
    bitmapResource[wxT("exclude.png")]            = (bitmapExclude           = new wxBitmap(wxNullBitmap));
    bitmapResource[wxT("filter active.png")]      = (bitmapFilterOn          = new wxBitmap(wxNullBitmap));
    bitmapResource[wxT("filter not active.png")]  = (bitmapFilterOff         = new wxBitmap(wxNullBitmap));
    bitmapResource[wxT("warning.png")]            = (bitmapWarning           = new wxBitmap(wxNullBitmap));
    bitmapResource[wxT("warningSmall.png")]       = (bitmapWarningSmall      = new wxBitmap(wxNullBitmap));
    bitmapResource[wxT("error.png")]              = (bitmapError             = new wxBitmap(wxNullBitmap));
    bitmapResource[wxT("small arrow up.png"])     = (bitmapSmallUp           = new wxBitmap(wxNullBitmap));
    bitmapResource[wxT("small arrow down.png")]   = (bitmapSmallDown         = new wxBitmap(wxNullBitmap));
    bitmapResource[wxT("save.png")]               = (bitmapSave              = new wxBitmap(wxNullBitmap));
    bitmapResource[wxT("load.png")]               = (bitmapLoad              = new wxBitmap(wxNullBitmap));
    bitmapResource[wxT("FFS.png")]                = (bitmapFFS               = new wxBitmap(wxNullBitmap));
    bitmapResource[wxT("FFS paused.png")]         = (bitmapFFSPaused         = new wxBitmap(wxNullBitmap));
    bitmapResource[wxT("deleteFile.png")]         = (bitmapDeleteFile        = new wxBitmap(wxNullBitmap));
    bitmapResource[wxT("gpl.png")]                = (bitmapGPL               = new wxBitmap(wxNullBitmap));
    bitmapResource[wxT("statusPause.png")]        = (bitmapStatusPause       = new wxBitmap(wxNullBitmap));
    bitmapResource[wxT("statusError.png")]        = (bitmapStatusError       = new wxBitmap(wxNullBitmap));
    bitmapResource[wxT("statusSuccess.png")]      = (bitmapStatusSuccess     = new wxBitmap(wxNullBitmap));
    bitmapResource[wxT("statusWarning.png")]      = (bitmapStatusWarning     = new wxBitmap(wxNullBitmap));
    bitmapResource[wxT("statusScanning.png")]     = (bitmapStatusScanning    = new wxBitmap(wxNullBitmap));
    bitmapResource[wxT("statusComparing.png")]    = (bitmapStatusComparing   = new wxBitmap(wxNullBitmap));
    bitmapResource[wxT("statusSyncing.png")]      = (bitmapStatusSyncing     = new wxBitmap(wxNullBitmap));
    bitmapResource[wxT("logo.png")]               = (bitmapLogo              = new wxBitmap(wxNullBitmap));
    bitmapResource[wxT("finished.png")]           = (bitmapFinished          = new wxBitmap(wxNullBitmap));
    bitmapResource[wxT("statusEdge.png")]         = (bitmapStatusEdge        = new wxBitmap(wxNullBitmap));
    bitmapResource[wxT("add pair.png")]           = (bitmapAddFolderPair     = new wxBitmap(wxNullBitmap));
    bitmapResource[wxT("remove pair.png")]        = (bitmapRemoveFolderPair  = new wxBitmap(wxNullBitmap));
    bitmapResource[wxT("remove pair disabl.png")] = (bitmapRemoveFolderPairD = new wxBitmap(wxNullBitmap));
    bitmapResource[wxT("link.png")]               = (bitmapLink              = new wxBitmap(wxNullBitmap));
    bitmapResource[wxT("background.png")]         = (bitmapBackground        = new wxBitmap(wxNullBitmap));
    bitmapResource[wxT("compare_small.png")]      = (bitmapCompareSmall      = new wxBitmap(wxNullBitmap));
    bitmapResource[wxT("sync_small.png")]         = (bitmapSyncSmall         = new wxBitmap(wxNullBitmap));
    bitmapResource[wxT("clock_small.png")]        = (bitmapClockSmall        = new wxBitmap(wxNullBitmap));
    bitmapResource[wxT("clock.png")]              = (bitmapClock             = new wxBitmap(wxNullBitmap));
    bitmapResource[wxT("filter.png")]             = (bitmapFilter            = new wxBitmap(wxNullBitmap));
    bitmapResource[wxT("batch.png")]              = (bitmapBatch             = new wxBitmap(wxNullBitmap));
    bitmapResource[wxT("batch_small.png")]        = (bitmapBatchSmall        = new wxBitmap(wxNullBitmap));
    bitmapResource[wxT("move up.png")]            = (bitmapMoveUp            = new wxBitmap(wxNullBitmap));
    bitmapResource[wxT("move down.png")]          = (bitmapMoveDown          = new wxBitmap(wxNullBitmap));
    bitmapResource[wxT("checkbox true.png")]      = (bitmapCheckBoxTrue      = new wxBitmap(wxNullBitmap));
    bitmapResource[wxT("checkbox false.png")]     = (bitmapCheckBoxFalse     = new wxBitmap(wxNullBitmap));
    bitmapResource[wxT("settings.png")]           = (bitmapSettings          = new wxBitmap(wxNullBitmap));
    bitmapResource[wxT("settings_small.png")]     = (bitmapSettingsSmall     = new wxBitmap(wxNullBitmap));

    //init all the other resource files
    animationMoney = new wxAnimation(wxNullAnimation);
    animationSync  = new wxAnimation(wxNullAnimation);

    programIcon = &wxNullIcon;
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
    std::vector<unsigned char> data;
    data.reserve(10000);

    int newValue = 0;
    while ((newValue = zipInput.GetC()) != wxEOF)
        data.push_back(newValue);

    wxMemoryInputStream seekAbleStream(&data.front(), data.size()); //stream does not take ownership of data

    animation->Load(seekAbleStream, wxANIMATION_TYPE_GIF);
}


void GlobalResources::load()
{
    wxFileInputStream input(wxT("Resources.dat"));
    if (input.IsOk()) //if not... we don't want to react too harsh here
    {
        //activate support for .png files
        wxImage::AddHandler(new wxPNGHandler); //ownership passed

        wxZipInputStream resourceFile(input);

        wxZipEntry* entry = NULL;
        std::map<wxString, wxBitmap*>::iterator bmp;
        while ((entry = resourceFile.GetNextEntry()))
        {
            wxString name = entry->GetName();

            //search if entry is available in map
            if ((bmp = bitmapResource.find(name)) != bitmapResource.end())
                *(bmp->second) = wxBitmap(wxImage(resourceFile, wxBITMAP_TYPE_PNG));
            else if (name == wxT("money.gif"))
                loadAnimFromZip(resourceFile, animationMoney);
            else if (name == wxT("working.gif"))
                loadAnimFromZip(resourceFile, animationSync);
        }
    }

#ifdef FFS_WIN
    programIcon = new wxIcon(wxT("ffsIcon1"));
#else
#include "FreeFileSync.xpm"
    programIcon = new wxIcon(FreeFileSync_xpm);
#endif
}
