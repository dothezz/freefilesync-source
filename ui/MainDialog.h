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

#include "../library/wxWidgets.h"
#include "guiGenerated.h"
#include "../FreeFileSync.h"

#include "syncDialog.h"
#include "smallDialogs.h"
#include "resources.h"
#include <wx/dnd.h>
#include <wx/config.h>

using namespace std;

const wxString constFilteredOut = "(-)";

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

bool updateUI_IsAllowed();        //test if a specific amount of time is over
void updateUI_Now();              //do the updating

extern int leadingPanel;

class CompareStatusUpdater;
class FileDropEvent;

class MainDialog : public GuiGenerated
{
public:
    MainDialog(wxFrame* frame, const wxString& cfgFileName);
    ~MainDialog();

private:
    friend class SyncDialog;
    friend class FilterDlg;
    friend class CompareStatusUpdater;
    friend class SyncStatusUpdater;
    friend class FileDropEvent;

    void readConfigurationFromHD(const wxString& filename, bool programStartup = false);
    void writeConfigurationToHD(const wxString& filename);
    void loadDefaultConfiguration();

    void updateViewFilterButtons();
    void updateFilterButton();

    void addCfgFileToHistory(const wxString& filename);

    static wxString evaluateCmpResult(const CompareFilesResult result, const bool selectedForSynchronization);

    //main method for putting gridData on UI: maps data respecting current view settings
    void writeGrid(const FileCompareResult& gridData, bool useUI_GridCache =  false);
    void mapFileModelToUI(UI_Grid& output, const FileCompareResult& fileCmpResult);
    void updateStatusInformation(const UI_Grid& output);

    void filterRangeManual(const set<int>& rowsToFilterOnUI_View, int leadingRow);

    void deleteFilesOnGrid(wxGrid* grid);

    //work to be done in idle time
    void OnIdleEvent(wxEvent& event);

    //delayed status information restore
    void pushStatusInformation(const wxString& text);
    void writeStatusInformation(const wxString& text);
    void clearStatusBar();

    //events
    void onGrid1access(wxEvent& event);
    void onGrid2access(wxEvent& event);
    void onGrid3access(wxEvent& event);

    void onGrid1ButtonEvent(wxKeyEvent& event);
    void onGrid2ButtonEvent(wxKeyEvent& event);
    void onGrid3ButtonEvent(wxKeyEvent& event);

    void OnEnterLeftDir(wxCommandEvent& event);
    void OnEnterRightDir(wxCommandEvent& event);
    void OnDirChangedPanel1(wxFileDirPickerEvent& event);
    void OnDirChangedPanel2(wxFileDirPickerEvent& event);

    //manual filtering of rows:
    void OnGridSelectCell(wxGridEvent& event);
    void OnGrid3SelectRange(wxGridRangeSelectEvent& event);
    void OnGrid3LeftMouseUp(wxEvent& event);
    void OnGrid3LeftMouseDown(wxEvent& event);

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

    void OnSaveConfig(          wxCommandEvent& event);
    void OnLoadConfiguration(   wxCommandEvent& event);
    void OnChoiceKeyEvent(      wxKeyEvent& event );

    void onResizeMainWindow(wxEvent& event);
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

    //ui settings
    int widthNotMaximized;
    int heightNotMaximized;
    int posXNotMaximized;
    int posYNotMaximized;

    //other options
    bool useRecycleBin;     //use Recycle bin when deleting or overwriting files while synchronizing
    bool hideErrorMessages; //hides error messages during synchronization

//***********************************************

    wxFrame* parent;

    //status information
    wxLongLong lastStatusChange;
    int        stackObjects;

    //save the last used config filenames
    wxConfig* cfgFileHistory;
    vector<wxString> cfgFileNames;
    static const int CfgHistroyLength = 10;

    //variables for manual filtering of m_grid3
    int selectedRange3Begin;  //only used for grid 3
    int selectedRange3End;    //only used for grid 3
    int selectionLead;  //used on all three grids
    bool filteringInitialized;
    bool filteringPending;

    CompareStatusUpdater* cmpStatusUpdaterTmp;  //used only by the abort button when comparing
};

//######################################################################################


class FileDropEvent : public wxFileDropTarget
{
public:
    FileDropEvent(MainDialog* dlg, int grid) :
            mainDlg(dlg),
            targetGrid(grid)
    {
        assert(grid == 1 || grid == 2);
    }

    ~FileDropEvent() {}

    //overwritten virtual method
    bool OnDropFiles(wxCoord x, wxCoord y, const wxArrayString& filenames);

private:
    MainDialog* mainDlg;
    int targetGrid;
};

//######################################################################################

//classes handling sync and compare error as well as status information
class CompareStatus;
class SyncStatus;

class CompareStatusUpdater : public StatusUpdater
{
public:
    CompareStatusUpdater(MainDialog* dlg);
    ~CompareStatusUpdater();

    void updateStatusText(const wxString& text);
    void initNewProcess(int objectsTotal, double dataTotal, int processID);
    void updateProcessedData(int objectsProcessed, double dataProcessed);
    int reportError(const wxString& text);

    void triggerUI_Refresh();

private:
    MainDialog* mainDialog;
    bool suppressUI_Errormessages;
    CompareStatus* statusPanel;
    int currentProcess;
};


class SyncStatusUpdater : public StatusUpdater
{
public:
    SyncStatusUpdater(wxWindow* dlg, bool hideErrorMessages);
    ~SyncStatusUpdater();

    void updateStatusText(const wxString& text);
    void initNewProcess(int objectsTotal, double dataTotal, int processID);
    void updateProcessedData(int objectsProcessed, double dataProcessed);
    int reportError(const wxString& text);

    void triggerUI_Refresh();

private:
    SyncStatus* syncStatusFrame;

    bool suppressUI_Errormessages;
    wxArrayString unhandledErrors;   //list of non-resolved errors
};


#endif // MAINDIALOG_H
