/***************************************************************
 * Name:      FreeFileSyncMain.h
 * Purpose:   Defines Application Frame
 * Author:    ZenJu (zhnmju123@gmx.de)
 * Created:   2008-07-16
 * Copyright: ZenJu ()
 * License:
 **************************************************************/

#ifndef MAINDIALOG_H
#define MAINDIALOG_H

#include "..\library\wxWidgets.h"
#include "GUI_Generated.h"
#include "..\FreeFileSync.h"

#include "SyncDialog.h"
#include "SmallDialogs.h"
#include <map>

using namespace std;

//synchronization dialog
const int StartSynchronizationProcess = 15;

//configure filter dialog
const int OkayButtonPressed           = 25;

//sync error dialog
const int ContinueButtonPressed       = 35;
const int AbortButtonPressed          = 45;

const wxString ConstFilteredOut = "(-)";

struct UI_GridLine
{
    wxString leftFilename;
    wxString leftRelativePath;
    wxString leftSize;
    wxString leftDate;

    wxString cmpResult;

    wxString rightFilename;
    wxString rightRelativePath;
    wxString rightSize;
    wxString rightDate;

    unsigned int linkToCurrentGridData; //rownumber of corresponding row in currentGridData
};
typedef vector<UI_GridLine> UI_Grid;


extern int leadingPanel;


class MainDialog : public GUI_Generated
{
public:
    MainDialog(wxFrame* frame);
    ~MainDialog();

private:
    friend class SyncDialog;
    friend class FilterDlg;
    friend class AboutDlg;
    friend class SyncErrorDlg;

    void readConfigurationFromHD();
    void writeConfigurationToHD();

    void loadResourceFiles();
    void unloadResourceFiles();

    void onGrid1access(wxEvent& event);
    void onGrid2access(wxEvent& event);
    void onGrid3access(wxEvent& event);

    void onGrid1ButtonEvent(wxKeyEvent& event);
    void onGrid2ButtonEvent(wxKeyEvent& event);

    void OnEnterLeftDir(wxCommandEvent& event);
    void OnEnterRightDir(wxCommandEvent& event);
    void OnDirChangedPanel1(wxFileDirPickerEvent& event);
    void OnDirChangedPanel2(wxFileDirPickerEvent& event);
    void onFilesDroppedPanel1(wxDropFilesEvent& event);
    void onFilesDroppedPanel2(wxDropFilesEvent& event);

    void mapFileModelToUI(UI_Grid& output, const FileCompareResult& fileCmpResult);
    void updateViewFilterButtons();
    void updateFilterButton();

    void synchronizeFolders(FileCompareResult& grid, const SyncConfiguration config);

    static wxString evaluateCmpResult(const CompareFilesResult result, const bool selectedForSynchronization);
    void writeGrid(const FileCompareResult& gridData, bool useUI_GridCache =  false);

    void OnMarkRangeOnGrid3(    wxEvent&     event);
    void OnDeselectRow(         wxGridEvent& event);
    void OnLeftGridDoubleClick( wxGridEvent& event);
    void OnRightGridDoubleClick(wxGridEvent& event);
    void OnSortLeftGrid(        wxGridEvent& event);
    void OnSortRightGrid(       wxGridEvent& event);

    void OnLeftOnlyFiles(       wxCommandEvent& event);
    void OnLeftNewerFiles(      wxCommandEvent& event);
    void OnDifferentFiles(      wxCommandEvent& event);
    void OnRightNewerFiles(     wxCommandEvent& event);
    void OnRightOnlyFiles(      wxCommandEvent& event);
    void OnEqualFiles(          wxCommandEvent& event);

    void OnFilterButton(        wxCommandEvent& event);
    void OnHideFilteredButton(  wxCommandEvent& event);
    void OnConfigureFilter(     wxHyperlinkEvent& event);
    void OnShowHelpDialog(      wxCommandEvent& event);
    void OnSwapDirs(            wxCommandEvent& event);
    void OnChangeCompareVariant(wxCommandEvent& event);
    void OnCompare(             wxCommandEvent& event);
    void OnSync(                wxCommandEvent& event);
    void OnClose(               wxCloseEvent&   event);
    void OnQuit(                wxCommandEvent& event);
    void OnAbout(               wxCommandEvent& event);

//***********************************************
    //application variables:

    //technical representation of grid-data
    FileCompareResult currentGridData;

    //UI view of currentGridData
    UI_Grid currentUI_View;    //necessary if user double-clicks on a row when filters are activated

    //Synchronisation settings
    SyncConfiguration syncConfiguration;

    //Filter setting
    wxString includeFilter;
    wxString excludeFilter;
    bool hideFiltered;
    bool filterIsActive;

    //UI View Filter settings
    bool leftOnlyFilesActive;
    bool leftNewerFilesActive;
    bool differentFilesActive;
    bool equalFilesActive;
    bool rightNewerFilesActive;
    bool rightOnlyFilesActive;

//***********************************************

    wxFrame* parent;

    //resources
    map<wxString, wxBitmap*> bitmapResource;

    wxBitmap* bitmapLeftArrow;
    wxBitmap* bitmapStartSync;
    wxBitmap* bitmapRightArrow;
    wxBitmap* bitmapDelete;
    wxBitmap* bitmapEmail;
    wxBitmap* bitmapAbout;
    wxBitmap* bitmapWebsite;
    wxBitmap* bitmapExit;
    wxBitmap* bitmapSync;
    wxBitmap* bitmapCompare;
    wxBitmap* bitmapSyncDisabled;
    wxBitmap* bitmapSwap;
    wxBitmap* bitmapHelp;
    wxBitmap* bitmapLeftOnly;
    wxBitmap* bitmapLeftNewer;
    wxBitmap* bitmapDifferent;
    wxBitmap* bitmapRightNewer;
    wxBitmap* bitmapRightOnly;
    wxBitmap* bitmapLeftOnlyDeact;
    wxBitmap* bitmapLeftNewerDeact;
    wxBitmap* bitmapDifferentDeact;
    wxBitmap* bitmapRightNewerDeact;
    wxBitmap* bitmapRightOnlyDeact;
    wxBitmap* bitmapEqual;
    wxBitmap* bitmapEqualDeact;
    wxBitmap* bitmapInclude;
    wxBitmap* bitmapExclude;
    wxBitmap* bitmapFilterOn;
    wxBitmap* bitmapFilterOff;
    wxBitmap* bitmapWarning;
    wxBitmap* bitmapSmallUp;
    wxBitmap* bitmapSmallDown;

    wxAnimation* animationMoney;
};


#endif // MAINDIALOG_H
