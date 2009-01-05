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

#include "guiGenerated.h"
#include "syncDialog.h"
#include "smallDialogs.h"
#include "../library/resources.h"
#include "../library/misc.h"
#include <wx/dnd.h>
#include <wx/config.h>
#include <stack>
#include "../library/processXml.h"

using namespace std;

//IDs for context menu items
enum ContextItem
{
    CONTEXT_FILTER_TEMP = 10,
    CONTEXT_EXCLUDE_EXT,
    CONTEXT_EXCLUDE_OBJ,
    CONTEXT_CLIPBOARD,
    CONTEXT_EXPLORER,
    CONTEXT_DELETE_FILES
};

extern int leadingPanel; //better keep this an int! event.GetEventObject() does NOT always return m_grid1, m_grid2, m_grid3!

class CompareStatusHandler;
class FileDropEvent;
class TiXmlElement;

class MainDialog : public GuiGenerated
{
    friend class CompareStatusHandler;
    friend class FileDropEvent;

public:
    MainDialog(wxFrame* frame, const wxString& cfgFileName, CustomLocale* language, xmlAccess::XmlGlobalSettings& settings);
    ~MainDialog();

private:
    //configuration load/save
    bool readConfigurationFromXml(const wxString& filename, bool programStartup = false);
    bool writeConfigurationToXml(const wxString& filename);

    void readGlobalSettings();
    void writeGlobalSettings();

    void updateViewFilterButtons();
    void updateFilterButton(wxBitmapButton* filterButton, bool isActive);
    void updateCompareButtons();

    void addCfgFileToHistory(const wxString& filename);

    void addFolderPair(const wxString& leftDir, const wxString& rightDir);
    void removeFolderPair(bool removeAll = false);

    //main method for putting gridData on UI: maps data respecting current view settings
    void writeGrid(const FileCompareResult& gridData);
    void mapGridDataToUI(GridView& output, const FileCompareResult& fileCmpResult);
    void updateStatusInformation(const GridView& output);

    //context menu functions
    set<int> getSelectedRows();
    void filterRangeTemp(const set<int>& rowsToFilterOnUI_View);
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

    void OnWriteDirManually(wxCommandEvent& event);
    void OnDirSelected(wxFileDirPickerEvent& event);

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

    void OnSaveConfig(          wxCommandEvent& event);
    void OnLoadConfig(          wxCommandEvent& event);
    void loadConfiguration(const wxString& filename);
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

    void OnAddFolderPair(       wxCommandEvent& event);
    void OnRemoveFolderPair(    wxCommandEvent& event);

    //menu events
    void OnMenuSaveConfig(      wxCommandEvent& event);
    void OnMenuLoadConfig(      wxCommandEvent& event);
    void OnMenuExportFileList(  wxCommandEvent& event);
    void OnMenuAdjustFileTimes( wxCommandEvent& event);
    void OnMenuBatchJob(        wxCommandEvent& event);
    void OnMenuAbout(           wxCommandEvent& event);
    void OnMenuQuit(            wxCommandEvent& event);
    void OnMenuLangEnglish(     wxCommandEvent& event);
    void OnMenuLangGerman(      wxCommandEvent& event);
    void OnMenuLangFrench(      wxCommandEvent& event);
    void OnMenuLangJapanese(    wxCommandEvent& event);
    void OnMenuLangDutch(       wxCommandEvent& event);

    void enableSynchronization(bool value);

//***********************************************
    //application variables are stored here:

    //global settings used by GUI and batch mode
    xmlAccess::XmlGlobalSettings& globalSettings;

    //technical representation of grid-data
    FileCompareResult currentGridData;

    //UI view of currentGridData
    GridView gridRefUI;

//-------------------------------------
    //functional configuration
    MainConfiguration cfg;

    //folder pairs:
    //m_directoryLeft, m_directoryRight
    vector<FolderPairGenerated*> additionalFolderPairs; //additional pairs to the standard pair

    //gui settings
    int widthNotMaximized;
    int heightNotMaximized;
    int posXNotMaximized;
    int posYNotMaximized;
    bool hideFilteredElements;
//-------------------------------------

    //convenience method to get all folder pairs (unformatted)
    void getFolderPairs(vector<FolderPair>& output,  bool formatted = false);

    //UI View Filter settings
    bool leftOnlyFilesActive;
    bool leftNewerFilesActive;
    bool differentFilesActive;
    bool equalFilesActive;
    bool rightNewerFilesActive;
    bool rightOnlyFilesActive;

//***********************************************
    wxMenu* contextMenu;

    CustomLocale* programLanguage;

    //status information
    wxLongLong lastStatusChange;
    stack<wxString> stackObjects;

    //compare status panel (hidden on start, shown when comparing)
    CompareStatus* compareStatus;

    //save the last used config filenames
    wxConfig* cfgFileHistory;
    vector<wxString> cfgFileNames;
    static const int CfgHistroyLength = 10;

    //variables for filtering of m_grid3
    bool filteringInitialized;
    bool filteringPending;

    //temporal variables used by exclude via context menu
    wxString exFilterCandidateExtension;
    struct FilterObject
    {
        wxString relativeName;
        FileDescrLine::ObjectType type;
    };
    vector<FilterObject> exFilterCandidateObj;

    bool synchronizationEnabled; //determines whether synchronization should be allowed

    bool restartOnExit; //restart dialog on exit (currently used, when language is changed)

    CompareStatusHandler* cmpStatusHandlerTmp;  //used only by the abort button when comparing
};

//######################################################################################


class FileDropEvent : public wxFileDropTarget
{
public:
    FileDropEvent(MainDialog* dlg, const wxPanel* obj) :
            mainDlg(dlg),
            dropTarget(obj)
    {}

    ~FileDropEvent() {}

    //overwritten virtual method
    bool OnDropFiles(wxCoord x, wxCoord y, const wxArrayString& filenames);

private:
    MainDialog* mainDlg;
    const wxPanel* dropTarget;
};

//######################################################################################

//classes handling sync and compare error as well as status information

class CompareStatusHandler : public StatusHandler
{
public:
    CompareStatusHandler(MainDialog* dlg);
    ~CompareStatusHandler();

    void updateStatusText(const wxString& text);
    void initNewProcess(int objectsTotal, double dataTotal, Process processID);
    void updateProcessedData(int objectsProcessed, double dataProcessed);
    ErrorHandler::Response reportError(const wxString& text);

    void forceUiRefresh();

private:
    void abortThisProcess();

    MainDialog* mainDialog;
    bool ignoreErrors;
    Process currentProcess;
};


class SyncStatusHandler : public StatusHandler
{
public:
    SyncStatusHandler(wxWindow* dlg, bool ignoreAllErrors);
    ~SyncStatusHandler();

    void updateStatusText(const wxString& text);
    void initNewProcess(int objectsTotal, double dataTotal, Process processID);
    void updateProcessedData(int objectsProcessed, double dataProcessed);
    ErrorHandler::Response reportError(const wxString& text);

    void forceUiRefresh();

private:
    void abortThisProcess();

    SyncStatus* syncStatusFrame;
    bool ignoreErrors;
    wxArrayString unhandledErrors;   //list of non-resolved errors
};


#endif // MAINDIALOG_H
