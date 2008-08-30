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
#include "Resources.h"

using namespace std;

//configure filter dialog
const int OkayButtonPressed           = 25;

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

bool updateUI_IsAllowed();  //test if a specific amount of time is over
void updateUI_Now();        //do the updating


extern int leadingPanel;

class CompareStatusUpdater;

class MainDialog : public GUI_Generated
{
public:
    MainDialog(wxFrame* frame);
    ~MainDialog();

private:
    friend class SyncDialog;
    friend class FilterDlg;
    friend class CompareStatusUpdater;
    friend class SyncStatusUpdater;

    void readConfigurationFromHD(const wxString& filename);
    void writeConfigurationToHD(const wxString& filename);

    void loadResourceFiles();
    void unloadResourceFiles();

    void updateViewFilterButtons();
    void updateFilterButton();

    void synchronizeFolders(FileCompareResult& grid, const SyncConfiguration config);

    static wxString evaluateCmpResult(const CompareFilesResult result, const bool selectedForSynchronization);

    //main method for putting gridData on UI: maps data respecting current view settings
    void writeGrid(const FileCompareResult& gridData, bool useUI_GridCache =  false);

    void mapFileModelToUI(UI_Grid& output, const FileCompareResult& fileCmpResult);
    void updateStatusInformation(const UI_Grid& output);

    void filterRangeManual(int begin, int end, int leadingRow);

    //Events
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

    void OnGrid3SelectCell(wxGridEvent& event);
    void OnGrid3SelectRange(wxGridRangeSelectEvent& event);
    void OnGrid3LeftMouseUp(wxEvent& event);
    void OnIdleToFilterManually(wxEvent& event);

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

    void OnAbortCompare(        wxCommandEvent& event);
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
    //global application variables are stored here:

    //technical representation of grid-data
    FileCompareResult currentGridData;

    //UI view of currentGridData
    UI_Grid currentUI_View;

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

    //other options
    bool useRecycleBin; //use Recycle bin when deleting or overwriting files while synchronizing

//***********************************************

    wxFrame* parent;

    //variables for manual filtering of m_grid3
    int selectedRangeBegin;
    int selectedRangeEnd;
    int selectionLead;
    bool filteringPending;

    CompareStatusUpdater* cmpStatusUpdaterTmp;  //used only by the abort button when comparing
};

//######################################################################################

//classes handling sync and compare error as well as status information

class CompareStatusUpdater : public StatusUpdater
{
public:
    CompareStatusUpdater(MainDialog* dlg, wxStatusBar* mainWindowBar);
    ~CompareStatusUpdater();

    void updateStatus(const wxString& text);
    void updateProgressIndicator(double number);
    int reportError(const wxString& text);

    void triggerUI_Refresh();

private:
    MainDialog* mainDialog;
    wxStatusBar* statusBar;
    bool suppressUI_Errormessages;

    unsigned int numberOfScannedObjects;
};

class SyncStatus;

class SyncStatusUpdater : public StatusUpdater
{
public:
    SyncStatusUpdater(wxWindow* dlg, double gaugeTotalElements);
    ~SyncStatusUpdater();

    void updateStatus(const wxString& text);
    void updateProgressIndicator(double number);
    int reportError(const wxString& text);

    void triggerUI_Refresh();

private:
    SyncStatus* syncStatusFrame;

    bool suppressUI_Errormessages;
    wxArrayString unhandledErrors;   //list of non-resolved errors
};


#endif // MAINDIALOG_H
