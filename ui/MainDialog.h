/***************************************************************
 * Name:      mainDialog.h
 * Purpose:   Main Application Dialog
 * Author:    ZenJu (zhnmju123@gmx.de)
 * Created:   2008-07-16
 **************************************************************/

#ifndef MAINDIALOG_H
#define MAINDIALOG_H

#include "guiGenerated.h"
#include <stack>
#include "../library/processXml.h"
#include "gridView.h"
#include <memory>

class CompareStatusHandler;
class CompareStatus;
class CustomLocale;
class MainFolderDragDrop;
class FolderPairPanel;
class CustomGrid;


class MainDialog : public MainDialogGenerated
{
    friend class CompareStatusHandler;
    friend class MainFolderDragDrop;

//IDs for context menu items
    enum //context menu for left and right grids
    {
        CONTEXT_FILTER_TEMP = 10,
        CONTEXT_EXCLUDE_EXT,
        CONTEXT_EXCLUDE_OBJ,
        CONTEXT_CLIPBOARD,
        CONTEXT_EXPLORER,
        CONTEXT_DELETE_FILES,
    };

    enum //context menu for middle grid
    {
        CONTEXT_CHECK_ALL,
        CONTEXT_UNCHECK_ALL
    };

    enum //context menu for column settings
    {
        CONTEXT_CUSTOMIZE_COLUMN_LEFT,
        CONTEXT_CUSTOMIZE_COLUMN_RIGHT
    };


public:
    MainDialog(wxFrame* frame, const wxString& cfgFileName, CustomLocale* language, xmlAccess::XmlGlobalSettings& settings);
    ~MainDialog();

private:
    void cleanUp();

    //configuration load/save
    bool readConfigurationFromXml(const wxString& filename, bool programStartup = false);
    bool writeConfigurationToXml(const wxString& filename);
    xmlAccess::XmlGuiConfig getCurrentConfiguration() const;

    xmlAccess::XmlGuiConfig lastConfigurationSaved; //support for: "Save changed configuration?" dialog

    void readGlobalSettings();
    void writeGlobalSettings();

    void updateViewFilterButtons();
    void updateFilterButton(wxBitmapButton* filterButton, bool isActive);
    void updateCompareButtons();

    void addFileToCfgHistory(const wxString& filename);
    void addLeftFolderToHistory(const wxString& leftFolder);
    void addRightFolderToHistory(const wxString& rightFolder);

    void addFolderPair(const Zstring& leftDir, const Zstring& rightDir);
    void addFolderPair(const std::vector<FreeFileSync::FolderPair>& newPairs);
    void removeFolderPair(const int pos, bool refreshLayout = true); //keep it an int, allow negative values!
    void clearFolderPairs();

    //main method for putting gridDataView on UI: updates data respecting current view settings
    void updateGuiGrid();

    void updateGridViewData();

    //context menu functions
    std::set<int> getSelectedRows(const CustomGrid* grid);
    void filterRangeManually(const std::set<int>& rowsToFilterOnUiTable);
    void copySelectionToClipboard(const CustomGrid* selectedGrid);
    void openWithFileManager(const int rowNumber, const bool leftSide);
    void deleteSelectedFiles();

    //work to be done in idle time
    void OnIdleEvent(wxEvent& event);

    //delayed status information restore
    void pushStatusInformation(const wxString& text);
    void clearStatusBar();

    //events
    void onGridLeftButtonEvent(wxKeyEvent& event);
    void onGridRightButtonEvent(wxKeyEvent& event);
    void onGridMiddleButtonEvent(wxKeyEvent& event);
    void OnContextMenu(wxGridEvent& event);
    void OnContextMenuSelection(wxCommandEvent& event);
    void OnContextMenuMiddle(wxGridEvent& event);
    void OnContextMenuMiddleSelection(wxCommandEvent& event);
    void OnContextColumnLeft(wxGridEvent& event);
    void OnContextColumnRight(wxGridEvent& event);
    void OnContextColumnSelection(wxCommandEvent& event);

    void OnDirSelected(wxFileDirPickerEvent& event);

    void requestShutdown(); //try to exit application

    //manual filtering of rows:
    void OnGridSelectCell(wxGridEvent& event);
    void OnGrid3LeftMouseUp(wxEvent& event);
    void OnGrid3LeftMouseDown(wxEvent& event);

    void OnLeftGridDoubleClick( wxGridEvent& event);
    void OnRightGridDoubleClick(wxGridEvent& event);
    void OnSortLeftGrid(        wxGridEvent& event);
    void OnSortMiddleGrid(      wxGridEvent& event);
    void OnSortRightGrid(       wxGridEvent& event);

    void OnLeftOnlyFiles(       wxCommandEvent& event);
    void OnLeftNewerFiles(      wxCommandEvent& event);
    void OnDifferentFiles(      wxCommandEvent& event);
    void OnRightNewerFiles(     wxCommandEvent& event);
    void OnRightOnlyFiles(      wxCommandEvent& event);
    void OnEqualFiles(          wxCommandEvent& event);

    void OnSaveConfig(          wxCommandEvent& event);
    void OnLoadConfig(          wxCommandEvent& event);
    void OnLoadFromHistory(     wxCommandEvent& event);
    void loadConfiguration(const wxString& filename);
    void OnCfgHistoryKeyEvent(  wxKeyEvent& event);
    void OnFolderHistoryKeyEvent(wxKeyEvent& event);
    void OnRegularUpdateCheck(  wxIdleEvent& event);

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
    void OnMenuGlobalSettings(  wxCommandEvent& event);
    void OnMenuExportFileList(  wxCommandEvent& event);
    void OnMenuBatchJob(        wxCommandEvent& event);
    void OnMenuCheckVersion(    wxCommandEvent& event);
    void OnMenuAbout(           wxCommandEvent& event);
    void OnMenuQuit(            wxCommandEvent& event);
    void OnMenuLangChineseSimp( wxCommandEvent& event);
    void OnMenuLangDutch(       wxCommandEvent& event);
    void OnMenuLangEnglish(     wxCommandEvent& event);
    void OnMenuLangFrench(      wxCommandEvent& event);
    void OnMenuLangGerman(      wxCommandEvent& event);
    void OnMenuLangHungarian(   wxCommandEvent& event);
    void OnMenuLangItalian(     wxCommandEvent& event);
    void OnMenuLangJapanese(    wxCommandEvent& event);
    void OnMenuLangPolish(      wxCommandEvent& event);
    void OnMenuLangPortuguese(  wxCommandEvent& event);
    void OnMenuLangPortugueseBrazil(wxCommandEvent& event);
    void OnMenuLangSlovenian(   wxCommandEvent& event);
    void OnMenuLangSpanish(     wxCommandEvent& event);

    void switchProgramLanguage(const int langID);
    void enableSynchronization(bool value);

//***********************************************
    //application variables are stored here:

    //global settings used by GUI and batch mode
    xmlAccess::XmlGlobalSettings& globalSettings;

    //technical representation of grid-data
    FreeFileSync::FolderComparison currentGridData;

    //UI view of currentGridData
    FreeFileSync::GridView gridDataView;

//-------------------------------------
    //functional configuration
    FreeFileSync::MainConfiguration cfg;

    //folder pairs:
    //m_directoryLeft, m_directoryRight
    std::vector<FolderPairPanel*> additionalFolderPairs; //additional pairs to the standard pair

    //gui settings
    int widthNotMaximized;
    int heightNotMaximized;
    int posXNotMaximized;
    int posYNotMaximized;
    bool hideFilteredElements;
    bool ignoreErrors;
//-------------------------------------

    //convenience method to get all folder pairs (unformatted)
    std::vector<FreeFileSync::FolderPair> getFolderPairs() const;

    //UI View Filter settings
    bool leftOnlyFilesActive;
    bool leftNewerFilesActive;
    bool differentFilesActive;
    bool equalFilesActive;
    bool rightNewerFilesActive;
    bool rightOnlyFilesActive;

//***********************************************
    std::auto_ptr<wxMenu> contextMenu;

    CustomLocale* programLanguage;

    //status information
    wxLongLong lastStatusChange;
    std::stack<wxString> stackObjects;

    //compare status panel (hidden on start, shown when comparing)
    CompareStatus* compareStatus;

    //save the last used config filename history
    std::vector<wxString> cfgFileNames;

    //used when saving configuration
    wxString proposedConfigFileName;

    //variables for filtering of m_grid3
    bool filteringInitialized;
    bool filteringPending;

    //temporal variables used by exclude via context menu
    wxString exFilterCandidateExtension;
    struct FilterObject
    {
        wxString relativeName;
        FreeFileSync::FileDescrLine::ObjectType type;
    };
    std::vector<FilterObject> exFilterCandidateObj;

    bool synchronizationEnabled; //determines whether synchronization should be allowed

    CompareStatusHandler* cmpStatusHandlerTmp;  //used only by the abort button when comparing

    bool cleanedUp; //determines if destructor code was already executed

    //remember last sort executed (for determination of sort order)
    int lastSortColumn;
    const wxGrid* lastSortGrid;

    //support for drag and drop
    std::auto_ptr<MainFolderDragDrop> dragDropOnLeft;
    std::auto_ptr<MainFolderDragDrop> dragDropOnRight;
};

#endif // MAINDIALOG_H
