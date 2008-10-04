#ifndef RESOURCES_H_INCLUDED
#define RESOURCES_H_INCLUDED

#include <wx/bitmap.h>
#include <wx/animate.h>
#include <wx/string.h>
#include <map>

using namespace std;

class GlobalResources
{
public:
    static void loadResourceFiles();
    static void unloadResourceFiles();

    static wxChar fileNameSeparator;

    //language dependent global variables: need to be initialized by CustomLocale on program startup and language switch
    static const wxChar* decimalPoint;
    static const wxChar* thousandsSeparator;

    //command line parameters
    static const wxChar* paramCompare;
    static const wxChar* paramCfg;
    static const wxChar* paramInclude;
    static const wxChar* paramExclude;
    static const wxChar* paramContinueError;
    static const wxChar* paramRecycler;
    static const wxChar* paramSilent;

    static const wxChar* valueSizeDate;
    static const wxChar* valueContent;

    //image resource objects
    static wxBitmap* bitmapLeftArrow;
    static wxBitmap* bitmapRightArrow;
    static wxBitmap* bitmapNoArrow;
    static wxBitmap* bitmapStartSync;
    static wxBitmap* bitmapStartSyncDis;
    static wxBitmap* bitmapDelete;
    static wxBitmap* bitmapEmail;
    static wxBitmap* bitmapAbout;
    static wxBitmap* bitmapWebsite;
    static wxBitmap* bitmapExit;
    static wxBitmap* bitmapSync;
    static wxBitmap* bitmapCompare;
    static wxBitmap* bitmapSyncDisabled;
    static wxBitmap* bitmapSwap;
    static wxBitmap* bitmapHelp;
    static wxBitmap* bitmapLeftOnly;
    static wxBitmap* bitmapLeftNewer;
    static wxBitmap* bitmapDifferent;
    static wxBitmap* bitmapRightNewer;
    static wxBitmap* bitmapRightOnly;
    static wxBitmap* bitmapLeftOnlyDeact;
    static wxBitmap* bitmapLeftNewerDeact;
    static wxBitmap* bitmapDifferentDeact;
    static wxBitmap* bitmapRightNewerDeact;
    static wxBitmap* bitmapRightOnlyDeact;
    static wxBitmap* bitmapEqual;
    static wxBitmap* bitmapEqualDeact;
    static wxBitmap* bitmapInclude;
    static wxBitmap* bitmapExclude;
    static wxBitmap* bitmapFilterOn;
    static wxBitmap* bitmapFilterOff;
    static wxBitmap* bitmapWarning;
    static wxBitmap* bitmapSmallUp;
    static wxBitmap* bitmapSmallDown;
    static wxBitmap* bitmapSave;
    static wxBitmap* bitmapFFS;
    static wxBitmap* bitmapDeleteFile;
    static wxBitmap* bitmapGPL;
    static wxBitmap* bitmapStatusPause;
    static wxBitmap* bitmapStatusError;
    static wxBitmap* bitmapStatusSuccess;
    static wxBitmap* bitmapStatusWarning;
    static wxBitmap* bitmapStatusScanning;
    static wxBitmap* bitmapStatusComparing;
    static wxBitmap* bitmapStatusSyncing;
    static wxBitmap* bitmapLogo;
    static wxBitmap* bitmapFinished;
    static wxBitmap* bitmapStatusEdge;

    static wxAnimation* animationMoney;
    static wxAnimation* animationSync;

    static wxIcon* programIcon;

private:
    //resource mapping
    static map<wxString, wxBitmap*> bitmapResource;
};


#endif // RESOURCES_H_INCLUDED
