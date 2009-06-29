#ifndef RESOURCES_H_INCLUDED
#define RESOURCES_H_INCLUDED

#include <wx/bitmap.h>
#include <wx/animate.h>
#include <wx/string.h>
#include <map>


class GlobalResources
{
public:
    static const GlobalResources& getInstance();

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
    wxBitmap* bitmapAboutSmall;
    wxBitmap* bitmapWebsite;
    wxBitmap* bitmapExit;
    wxBitmap* bitmapCompare;
    wxBitmap* bitmapCompareDisabled;
    wxBitmap* bitmapSync;
    wxBitmap* bitmapSyncDisabled;
    wxBitmap* bitmapSwap;
    wxBitmap* bitmapSwapSmall;
    wxBitmap* bitmapHelp;
    wxBitmap* bitmapLeftOnly;
    wxBitmap* bitmapLeftOnlyAct;
    wxBitmap* bitmapLeftOnlyDeact;
    wxBitmap* bitmapRightOnly;
    wxBitmap* bitmapRightOnlyAct;
    wxBitmap* bitmapRightOnlyDeact;
    wxBitmap* bitmapLeftNewer;
    wxBitmap* bitmapLeftNewerAct;
    wxBitmap* bitmapLeftNewerDeact;
    wxBitmap* bitmapRightNewer;
    wxBitmap* bitmapRightNewerAct;
    wxBitmap* bitmapRightNewerDeact;
    wxBitmap* bitmapEqual;
    wxBitmap* bitmapEqualAct;
    wxBitmap* bitmapEqualDeact;
    wxBitmap* bitmapDifferent;
    wxBitmap* bitmapDifferentAct;
    wxBitmap* bitmapDifferentDeact;
    wxBitmap* bitmapConflictAct;
    wxBitmap* bitmapConflictDeact;
    wxBitmap* bitmapInclude;
    wxBitmap* bitmapExclude;
    wxBitmap* bitmapFilterOn;
    wxBitmap* bitmapFilterOff;
    wxBitmap* bitmapFilterSmall;
    wxBitmap* bitmapWarning;
    wxBitmap* bitmapWarningSmall;
    wxBitmap* bitmapError;
    wxBitmap* bitmapSmallUp;
    wxBitmap* bitmapSmallDown;
    wxBitmap* bitmapSave;
    wxBitmap* bitmapLoad;
    wxBitmap* bitmapFFS;
    wxBitmap* bitmapFFSPaused;
    wxBitmap* bitmapDeleteFile;
    wxBitmap* bitmapGPL;
    wxBitmap* bitmapStatusPause;
    wxBitmap* bitmapStatusError;
    wxBitmap* bitmapStatusSuccess;
    wxBitmap* bitmapStatusWarning;
    wxBitmap* bitmapStatusScanning;
    wxBitmap* bitmapStatusBinCompare;
    wxBitmap* bitmapStatusSyncing;
    wxBitmap* bitmapLogo;
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
    wxBitmap* bitmapRecycler;
    wxBitmap* bitmapShift;
    wxBitmap* bitmapSyncCfg;
    wxBitmap* bitmapPreview;
    wxBitmap* bitmapPreviewDisabled;
    wxBitmap* bitmapQuestion;
    wxBitmap* bitmapChina;
    wxBitmap* bitmapHolland;
    wxBitmap* bitmapEngland;
    wxBitmap* bitmapFrance;
    wxBitmap* bitmapGermany;
    wxBitmap* bitmapHungary;
    wxBitmap* bitmapItaly;
    wxBitmap* bitmapJapan;
    wxBitmap* bitmapPoland;
    wxBitmap* bitmapPortugal;
    wxBitmap* bitmapBrazil;
    wxBitmap* bitmapSlovakia;
    wxBitmap* bitmapSpain;
    wxBitmap* bitmapRussia;
    wxBitmap* bitmapSyncDirLeftAct;
    wxBitmap* bitmapSyncDirLeftDeact;
    wxBitmap* bitmapSyncDirRightAct;
    wxBitmap* bitmapSyncDirRightDeact;
    wxBitmap* bitmapSyncDirNoneAct;
    wxBitmap* bitmapSyncDirNoneDeact;
    wxBitmap* bitmapSyncDirLeftSmall;
    wxBitmap* bitmapSyncDirRightSmall;
    wxBitmap* bitmapSyncDirNoneSmall;
    wxBitmap* bitmapCreateLeftSmall;
    wxBitmap* bitmapCreateRightSmall;
    wxBitmap* bitmapDeleteLeftSmall;
    wxBitmap* bitmapDeleteRightSmall;
    wxBitmap* bitmapLeftOnlySmall;
    wxBitmap* bitmapRightOnlySmall;
    wxBitmap* bitmapLeftNewerSmall;
    wxBitmap* bitmapRightNewerSmall;
    wxBitmap* bitmapEqualSmall;
    wxBitmap* bitmapDifferentSmall;
    wxBitmap* bitmapConflictSmall;
    wxBitmap* bitmapCreate;
    wxBitmap* bitmapUpdate;
    wxBitmap* bitmapDelete;
    wxBitmap* bitmapData;
    wxBitmap* bitmapCmpViewSmall;
    wxBitmap* bitmapSyncViewSmall;
    wxBitmap* bitmapSwitchViewSmall;

    wxAnimation* animationMoney;
    wxAnimation* animationSync;

    wxIcon* programIcon;

    void load() const; //loads bitmap resources on program startup: logical const!

private:
    GlobalResources();
    ~GlobalResources();

    //resource mapping
    mutable std::map<wxString, wxBitmap*> bitmapResource;
};

#endif // RESOURCES_H_INCLUDED
