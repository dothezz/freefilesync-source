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
#include "../library/resources.h"
#include "../library/misc.h"
#include <wx/dnd.h>
#include <wx/config.h>
#include <stack>

using namespace std;

//IDs for context menu items
enum ContextItem
{
    contextManualFilter = 10,
    contextCopyClipboard,
    contextOpenExplorer,
    contextDeleteFiles
};

extern int leadingPanel;

class CompareStatusUpdater;
class FileDropEvent;

class MainDialog : public GuiGenerated
{
    friend class CompareStatusUpdater;
    friend class FileDropEvent;

public:
    MainDialog(wxFrame* frame, const wxString& cfgFileName, CustomLocale* language);
    ~MainDialog();

private:
    void readConfigurationFromHD(const wxString& filename, bool programStartup = false);
    void writeConfigurationToHD(const wxString& filename);
    void loadDefaultConfiguration();

    void updateViewFilterButtons();
    void updateFilterButton(wxBitmapButton* filterButton, bool isActive);
    void updateCompareButtons();

    void addCfgFileToHistory(const wxString& filename);

    //main method for putting gridData on UI: maps data respecting current view settings
    void writeGrid(const FileCompareResult& gridData, bool useUI_GridCache =  false);
    void mapGridDataToUI(GridView& output, const FileCompareResult& fileCmpResult);
    void updateStatusInformation(const GridView& output);

    //context menu functions
    set<int> getSelectedRows();
    void filterRangeManual(const set<int>& rowsToFilterOnUI_View);
    void copySelectionToClipboard(const set<int>& selectedRows, int selectedGrid);
    void openWithFileBrowser(int rowNumber, int gridNr);
    void deleteFilesOnGrid(const set<int>& rowsToDeleteOnUI);

    //work to be done in idle time
    void OnIdleEvent(wxEvent& event);

    //delayed status information restore
    void pushStatusInformation(const wxString& text);
    void clearStatusBar();

    //events
    void onGrid1access(wxEvent& event);
    void onGrid2access(wxEvent& event);
    void onGrid3access(wxEvent& event);

    void onGrid1ButtonEvent(wxKeyEvent& event);
    void onGrid2ButtonEvent(wxKeyEvent& event);
    void onGrid3ButtonEvent(wxKeyEvent& event);
    void OnOpenContextMenu(wxGridEvent& event);

    void OnEnterLeftDir(wxCommandEvent& event);
    void OnEnterRightDir(wxCommandEvent& event);
    void OnDirChangedPanel1(wxFileDirPickerEvent& event);
    void OnDirChangedPanel2(wxFileDirPickerEvent& event);

    //manual filtering of rows:
    void OnGridSelectCell(wxGridEvent& event);
    void OnGrid3LeftMouseUp(wxEvent& event);
    void OnGrid3LeftMouseDown(wxEvent& event);

    void onContextMenuSelection(wxCommandEvent& event);

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

    void OnBatchJob(            wxCommandEvent& event);
    void OnSaveConfig(          wxCommandEvent& event);
    void OnLoadConfiguration(   wxCommandEvent& event);
    void OnChoiceKeyEvent(      wxKeyEvent& event );

    void onResizeMainWindow(    wxEvent& event);
    void OnAbortCompare(        wxCommandEvent& event);
    void OnFilterButton(        wxCommandEvent& event);
    void OnHideFilteredButton(  wxCommandEvent& event);
    void OnConfigureFilter(     wxHyperlinkEvent& event);
    void OnShowHelpDialog(      wxCommandEvent& event);
    void OnSwapDirs(            wxCommandEvent& event);
    void OnCompareByTimeSize(   wxCommandEvent& event);
    void OnCompareByContent(    wxCommandEvent& event);
    void OnCompare(             wxCommandEvent& event);
    void OnSync(                wxCommandEvent& event);
    void OnClose(               wxCloseEvent&   event);
    void OnQuit(                wxCommandEvent& event);
    void OnAbout(               wxCommandEvent& event);

    //menu events
    void OnMenuExportFileList(  wxCommandEvent& event);
    void OnMenuBatchJob(        wxCommandEvent& event);
    void OnMenuAbout(           wxCommandEvent& event);
    void OnMenuQuit(            wxCommandEvent& event);
    void OnMenuLangEnglish(     wxCommandEvent& event);
    void OnMenuLangGerman(      wxCommandEvent& event);

    void enableSynchronization(bool value);

//***********************************************
    //global application variables are stored here:

    //technical representation of grid-data
    FileCompareResult currentGridData;

    //UI view of currentGridData
    GridView gridRefUI;

    Configuration cfg;

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

//***********************************************
    wxMenu* contextMenu;

    CustomLocale* programLanguage;

    //status information
    wxLongLong lastStatusChange;
    stack<wxString> stackObjects;

    //save the last used config filenames
    wxConfig* cfgFileHistory;
    vector<wxString> cfgFileNames;
    static const int CfgHistroyLength = 10;

    //variables for manual filtering of m_grid3
    bool filteringInitialized;
    bool filteringPending;

    bool synchronizationEnabled; //determines whether synchronization should be allowed

    bool restartOnExit; //restart dialog on exit (currently used, when language is changed)

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
    CompareStatus* statusPanel;
    bool continueOnError;
    int currentProcess;
};


class SyncStatusUpdater : public StatusUpdater
{
public:
    SyncStatusUpdater(wxWindow* dlg, bool continueOnError);
    ~SyncStatusUpdater();

    void updateStatusText(const wxString& text);
    void initNewProcess(int objectsTotal, double dataTotal, int processID);
    void updateProcessedData(int objectsProcessed, double dataProcessed);
    int reportError(const wxString& text);

    void triggerUI_Refresh();

private:
    SyncStatus* syncStatusFrame;

    bool continueError;
    wxArrayString unhandledErrors;   //list of non-resolved errors
};


#endif // MAINDIALOG_H
