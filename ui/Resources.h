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

    static wxBitmap* bitmapLeftArrow;
    static wxBitmap* bitmapStartSync;
    static wxBitmap* bitmapRightArrow;
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

    static wxAnimation* animationMoney;

private:
    //resource mapping
    static map<wxString, wxBitmap*> bitmapResource;
};


#endif // RESOURCES_H_INCLUDED
