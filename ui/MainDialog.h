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
#include <map>

class CompareStatusHandler;
class CompareStatus;
class MainFolderDragDrop;
class FolderPairPanel;
class CustomGrid;
class FFSCheckRowsEvent;
class FFSSyncDirectionEvent;
class SyncPreview;
class IconUpdater;

namespace FreeFileSync
{
    class CustomLocale;
}


class MainDialog : public MainDialogGenerated
{
    friend class CompareStatusHandler;
    friend class MainFolderDragDrop;
    friend class SyncPreview;

//IDs for context menu items
    enum ContextIDRim //context menu for left and right grids
    {
        CONTEXT_FILTER_TEMP = 10,
        CONTEXT_EXCLUDE_EXT,
        CONTEXT_EXCLUDE_OBJ,
        CONTEXT_CLIPBOARD,
        CONTEXT_EXPLORER,
        CONTEXT_DELETE_FILES,
        CONTEXT_SYNC_DIR_LEFT,
        CONTEXT_SYNC_DIR_NONE,
        CONTEXT_SYNC_DIR_RIGHT
    };

    enum ContextIDRimLabel//context menu for column settings
    {
        CONTEXT_CUSTOMIZE_COLUMN_LEFT,
        CONTEXT_CUSTOMIZE_COLUMN_RIGHT
    };

    enum ContextIDMiddle//context menu for middle grid
    {
        CONTEXT_CHECK_ALL = 20,
        CONTEXT_UNCHECK_ALL
    };

    enum ContextIDMiddleLabel
    {
        CONTEXT_COMPARISON_RESULT = 30,
        CONTEXT_SYNC_PREVIEW
    };


public:
    MainDialog(wxFrame* frame,
               const wxString& cfgFileName,
               FreeFileSync::CustomLocale* language,
               xmlAccess::XmlGlobalSettings& settings);

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

    void addFolderPair(const Zstring& leftDir, const Zstring& rightDir, bool addFront = false);
    void addFolderPair(const std::vector<FreeFileSync::FolderPair>& newPairs, bool addFront = false);
    void removeFolderPair(const int pos, bool refreshLayout = true); //keep it an int, allow negative values!
    void clearFolderPairs();

    //main method for putting gridDataView on UI: updates data respecting current view settings
    void updateGuiGrid();

    void updateGridViewData();

    //context menu functions
    std::set<int> getSelectedRows(const CustomGrid* grid) const;
    std::set<int> getSelectedRows() const;
    void setSyncDirManually(const std::set<int>& rowsToSetOnUiTable, const FreeFileSync::SyncDirection dir);
    void filterRangeManually(const std::set<int>& rowsToFilterOnUiTable, const int leadingRow);
    void copySelectionToClipboard(const CustomGrid* selectedGrid);
    void openWithFileManager(const int rowNumber, const bool leftSide);
    void deleteSelectedFiles();

    //work to be done in idle time
    void OnIdleEvent(wxEvent& event);

    //delayed status information restore
    void pushStatusInformation(const wxString& text);
    void clearStatusBar();

    //events
    void onGridLeftButtonEvent(        wxKeyEvent& event);
    void onGridRightButtonEvent(       wxKeyEvent& event);
    void onGridMiddleButtonEvent(      wxKeyEvent& event);
    void OnContextRim(                 wxGridEvent& event);
    void OnContextRimSelection(        wxCommandEvent& event);
    void OnContextRimLabelLeft(        wxGridEvent& event);
    void OnContextRimLabelRight(       wxGridEvent& event);
    void OnContextRimLabelSelection(   wxCommandEvent& event);
    void OnContextMiddle(              wxGridEvent& event);
    void OnContextMiddleSelection(     wxCommandEvent& event);
    void OnContextMiddleLabel(         wxGridEvent& event);
    void OnContextMiddleLabelSelection(wxCommandEvent& event);

    void OnDirSelected(wxFileDirPickerEvent& event);

    void requestShutdown(); //try to exit application

    void OnCheckRows(FFSCheckRowsEvent& event);
    void OnSetSyncDirection(FFSSyncDirectionEvent& event);

    void OnLeftGridDoubleClick( wxGridEvent& event);
    void OnRightGridDoubleClick(wxGridEvent& event);
    void OnSortLeftGrid(        wxGridEvent& event);
    void OnSortMiddleGrid(      wxGridEvent& event);
    void OnSortRightGrid(       wxGridEvent& event);

    void OnLeftOnlyFiles(       wxCommandEvent& event);
    void OnRightOnlyFiles(      wxCommandEvent& event);
    void OnLeftNewerFiles(      wxCommandEvent& event);
    void OnRightNewerFiles(     wxCommandEvent& event);
    void OnEqualFiles(          wxCommandEvent& event);
    void OnDifferentFiles(      wxCommandEvent& event);
    void OnConflictFiles(       wxCommandEvent& event);

    void OnSyncDirLeft(         wxCommandEvent& event);
    void OnSyncDirRight(        wxCommandEvent& event);
    void OnSyncDirNone(         wxCommandEvent& event);

    void OnSaveConfig(          wxCommandEvent& event);
    void OnLoadConfig(          wxCommandEvent& event);
    void OnLoadFromHistory(     wxCommandEvent& event);
    bool trySaveConfig(); //return true if saved successfully

    void loadConfiguration(const wxString& filename);
    void OnCfgHistoryKeyEvent(  wxKeyEvent& event);
    void OnFolderHistoryKeyEvent(wxKeyEvent& event);
    void OnRegularUpdateCheck(  wxIdleEvent& event);
    void OnLayoutWindowAsync(   wxIdleEvent& event);

    void refreshGridAfterFilterChange(const int delay);

    void onResizeMainWindow(    wxEvent& event);
    void OnAbortCompare(        wxCommandEvent& event);
    void OnFilterButton(        wxCommandEvent& event);
    void OnHideFilteredButton(  wxCommandEvent& event);
    void OnConfigureFilter(     wxHyperlinkEvent& event);
    void OnShowHelpDialog(      wxCommandEvent& event);
    void OnSwapSides(           wxCommandEvent& event);
    void OnCompareByTimeSize(   wxCommandEvent& event);
    void OnCompareByContent(    wxCommandEvent& event);
    void OnCompare(             wxCommandEvent& event);
    void OnSwitchView(          wxCommandEvent& event);
    void OnSyncSettings(        wxCommandEvent& event);
    void OnStartSync(           wxCommandEvent& event);
    void OnClose(               wxCloseEvent&   event);
    void OnQuit(                wxCommandEvent& event);

    void calculatePreview();

    void OnAddFolderPair(       wxCommandEvent& event);
    void OnRemoveFolderPair(    wxCommandEvent& event);
    void OnRemoveTopFolderPair( wxCommandEvent& event);

    //menu events
    void OnMenuSaveConfig(      wxCommandEvent& event);
    void OnMenuLoadConfig(      wxCommandEvent& event);
    void OnMenuGlobalSettings(  wxCommandEvent& event);
    void OnMenuExportFileList(  wxCommandEvent& event);
    void OnMenuBatchJob(        wxCommandEvent& event);
    void OnMenuCheckVersion(    wxCommandEvent& event);
    void OnMenuAbout(           wxCommandEvent& event);
    void OnMenuQuit(            wxCommandEvent& event);
    void OnMenuLanguageSwitch(  wxCommandEvent& event);

    void switchProgramLanguage(const int langID);

    typedef int MenuItemID;
    typedef int LanguageID;
    std::map<MenuItemID, LanguageID> languageMenuItemMap; //needed to attach menu item events

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


//***********************************************
    std::auto_ptr<wxMenu> contextMenu;

    FreeFileSync::CustomLocale* programLanguage;

    //status information
    wxLongLong lastStatusChange;
    std::stack<wxString> stackObjects;

    //compare status panel (hidden on start, shown when comparing)
    CompareStatus* compareStatus;

    //save the last used config filename history
    std::vector<wxString> cfgFileNames;

    //used when saving configuration
    wxString currentConfigFileName;

    //temporal variables used by exclude via context menu
    wxString exFilterCandidateExtension;
    struct FilterObject
    {
        wxString relativeName;
        FreeFileSync::FileDescrLine::ObjectType type;
    };
    std::vector<FilterObject> exFilterCandidateObj;

    CompareStatusHandler* cmpStatusHandlerTmp;  //used only by the abort button when comparing

    bool cleanedUp; //determines if destructor code was already executed

    //remember last sort executed (for determination of sort order)
    int lastSortColumn;
    const wxGrid* lastSortGrid;

    //support for drag and drop
    std::auto_ptr<MainFolderDragDrop> dragDropOnLeft;
    std::auto_ptr<MainFolderDragDrop> dragDropOnRight;

#ifdef FFS_WIN
    //update icons periodically: one updater instance for both left and right grids
    std::auto_ptr<IconUpdater> updateFileIcons;
#endif

    //encapsulation of handling of sync preview
    std::auto_ptr<SyncPreview> syncPreview;
};


class SyncPreview //encapsulates MainDialog functionality for synchronization preview (friend class)
{
    friend class MainDialog;

public:
    void enablePreview(bool value);
    bool previewIsEnabled();

    void enableSynchronization(bool value);
    bool synchronizationIsEnabled();

private:
    SyncPreview(MainDialog* mainDlg);

    MainDialog* mainDlg_;
    bool syncPreviewEnabled; //toggle to display configuration preview instead of comparison result
    bool synchronizationEnabled; //determines whether synchronization should be allowed
};

#endif // MAINDIALOG_H
