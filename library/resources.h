#ifndef RESOURCES_H_INCLUDED
#define RESOURCES_H_INCLUDED

#include <wx/bitmap.h>
#include <wx/animate.h>
#include <wx/string.h>
#include <map>


class GlobalResources
{
public:
    GlobalResources();
    ~GlobalResources();

    void load();

    static const wxChar FILE_NAME_SEPARATOR;

    //language dependent global variables: need to be initialized by CustomLocale on program startup and language switch
    static const wxChar* DECIMAL_POINT;
    static const wxChar* THOUSANDS_SEPARATOR;

    //image resource objects
    wxBitmap* bitmapArrowLeft;
    wxBitmap* bitmapArrowRight;
    wxBitmap* bitmapArrowLeftCr;
    wxBitmap* bitmapArrowRightCr;
    wxBitmap* bitmapArrowNone;
    wxBitmap* bitmapStartSync;
    wxBitmap* bitmapStartSyncDis;
    wxBitmap* bitmapDeleteLeft;
    wxBitmap* bitmapDeleteRight;
    wxBitmap* bitmapEmail;
    wxBitmap* bitmapAbout;
    wxBitmap* bitmapWebsite;
    wxBitmap* bitmapExit;
    wxBitmap* bitmapSync;
    wxBitmap* bitmapCompare;
    wxBitmap* bitmapSyncDisabled;
    wxBitmap* bitmapSwap;
    wxBitmap* bitmapHelp;
    wxBitmap* bitmapEqual;
    wxBitmap* bitmapEqualAct;
    wxBitmap* bitmapEqualDeact;
    wxBitmap* bitmapLeftOnly;
    wxBitmap* bitmapLeftOnlyAct;
    wxBitmap* bitmapLeftOnlyDeact;
    wxBitmap* bitmapLeftNewer;
    wxBitmap* bitmapLeftNewerAct;
    wxBitmap* bitmapLeftNewerDeact;
    wxBitmap* bitmapDifferent;
    wxBitmap* bitmapDifferentAct;
    wxBitmap* bitmapDifferentDeact;
    wxBitmap* bitmapRightNewer;
    wxBitmap* bitmapRightNewerAct;
    wxBitmap* bitmapRightNewerDeact;
    wxBitmap* bitmapRightOnly;
    wxBitmap* bitmapRightOnlyAct;
    wxBitmap* bitmapRightOnlyDeact;
    wxBitmap* bitmapInclude;
    wxBitmap* bitmapExclude;
    wxBitmap* bitmapFilterOn;
    wxBitmap* bitmapFilterOff;
    wxBitmap* bitmapWarning;
    wxBitmap* bitmapSmallUp;
    wxBitmap* bitmapSmallDown;
    wxBitmap* bitmapSave;
    wxBitmap* bitmapFFS;
    wxBitmap* bitmapDeleteFile;
    wxBitmap* bitmapGPL;
    wxBitmap* bitmapStatusPause;
    wxBitmap* bitmapStatusError;
    wxBitmap* bitmapStatusSuccess;
    wxBitmap* bitmapStatusWarning;
    wxBitmap* bitmapStatusScanning;
    wxBitmap* bitmapStatusComparing;
    wxBitmap* bitmapStatusSyncing;
    wxBitmap* bitmapLogo;
    wxBitmap* bitmapFinished;
    wxBitmap* bitmapStatusEdge;
    wxBitmap* bitmapAddFolderPair;
    wxBitmap* bitmapRemoveFolderPair;
    wxBitmap* bitmapRemoveFolderPairD;
    wxBitmap* bitmapLink;
    wxBitmap* bitmapBackground;
    wxBitmap* bitmapCompareSmall;
    wxBitmap* bitmapSyncSmall;
    wxBitmap* bitmapClockSmall;
    wxBitmap* bitmapClock;
    wxBitmap* bitmapFilter;
    wxBitmap* bitmapBatch;
    wxBitmap* bitmapBatchSmall;
    wxBitmap* bitmapMoveUp;
    wxBitmap* bitmapMoveDown;
    wxBitmap* bitmapCheckBoxTrue;
    wxBitmap* bitmapCheckBoxFalse;
    wxBitmap* bitmapSettings;
    wxBitmap* bitmapSettingsSmall;

    wxAnimation* animationMoney;
    wxAnimation* animationSync;

    wxIcon* programIcon;

private:
    //resource mapping
    std::map<wxString, wxBitmap*> bitmapResource;
};


extern GlobalResources globalResource;  //loads bitmap resources on program startup

#endif // RESOURCES_H_INCLUDED
