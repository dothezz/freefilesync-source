// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) 2008-2011 ZenJu (zhnmju123 AT gmx.de)                    *
// **************************************************************************

#ifndef MAINDIALOG_H
#define MAINDIALOG_H

#include "gui_generated.h"
#include <stack>
#include "../library/process_xml.h"
#include <memory>
#include <map>
#include <set>
#include <wx/aui/aui.h>
#include "../shared/int64.h"

class FolderHistory;
class CustomGrid;
class FFSCheckRowsEvent;
class FFSSyncDirectionEvent;
class IconUpdater;
class DirectoryPair;
class DirectoryPairFirst;
class CompareStatus;
class SyncStatusHandler;
class PanelMoveWindow;


namespace zen
{
class CustomLocale;
class GridView;
}


class MainDialog : public MainDialogGenerated
{
public:
    MainDialog(const wxString& cfgFileName,
               xmlAccess::XmlGlobalSettings& settings);

    MainDialog(const xmlAccess::XmlGuiConfig& guiCfg,
               xmlAccess::XmlGlobalSettings& settings,
               bool startComparison);

    ~MainDialog();

    void disableAllElements(bool enableAbort); //dis-/enables all elements (except abort button) that might receive user input
    void enableAllElements();                   //during long-running processes: comparison, deletion

private:
    friend class CompareStatusHandler;
    friend class SyncStatusHandler;
    friend class ManualDeletionHandler;
    friend class DirectoryPairFirst;
    friend class DirectoryNameMainImpl;
    template <class GuiPanel>
    friend class FolderPairCallback;
    friend class PanelMoveWindow;

    MainDialog();

    void init(const xmlAccess::XmlGuiConfig guiCfg,
              xmlAccess::XmlGlobalSettings& settings,
              bool startComparison);

    void cleanUp(bool saveLastUsedConfig);

    //configuration load/save
    void setLastUsedConfig(const wxString& filename, const xmlAccess::XmlGuiConfig& guiConfig);
    void setLastUsedConfig(const std::vector<wxString>& filenames, const xmlAccess::XmlGuiConfig& guiConfig);

    xmlAccess::XmlGuiConfig getConfig() const;
    void setConfig(const xmlAccess::XmlGuiConfig& newGuiCfg);

    void loadConfiguration(const wxString& filename);
    void loadConfiguration(const std::vector<wxString>& filenames);

    bool trySaveConfig(); //return true if saved successfully
    bool saveOldConfig(); //return false on user abort

    static const wxString& lastRunConfigName();

    xmlAccess::XmlGuiConfig lastConfigurationSaved; //support for: "Save changed configuration?" dialog
    //used when saving configuration
    std::vector<wxString> activeConfigFiles; //name of currently loaded config file (may be more than 1)

    void readGlobalSettings();
    void writeGlobalSettings();

    void initViewFilterButtons();
    void updateFilterButtons();

    void addFileToCfgHistory(const std::vector<wxString>& filenames);

    void addFolderPair(const std::vector<zen::FolderPairEnh>& newPairs, bool addFront = false);
    void removeAddFolderPair(size_t pos);
    void clearAddFolderPairs();

    void updateGuiForFolderPair(); //helper method: add usability by showing/hiding buttons related to folder pairs

    //main method for putting gridDataView on UI: updates data respecting current view settings
    void updateGuiGrid();
    void updateGridViewData();

    //context menu functions
    std::set<size_t> getSelectedRows(const CustomGrid* grid) const;
    std::set<size_t> getSelectedRows() const;
    void setSyncDirManually(const std::set<size_t>& rowsToSetOnUiTable, const zen::SyncDirection dir);
    void filterRangeManually(const std::set<size_t>& rowsToFilterOnUiTable, int leadingRow);
    void copySelectionToClipboard(CustomGrid& selectedGrid);
    void deleteSelectedFiles(const std::set<size_t>& viewSelectionLeft, const std::set<size_t>& viewSelectionRight);

    void openExternalApplication(const wxString& commandline);
    void openExternalApplication(size_t rowNumber, bool leftSide, const wxString& commandline);

    //work to be done in idle time
    void OnIdleEvent(wxEvent& event);

    //delayed status information restore
    void pushStatusInformation(const wxString& text);
    void clearStatusBar();

    //events
    void onGridLeftButtonEvent(        wxKeyEvent& event);
    void onGridRightButtonEvent(       wxKeyEvent& event);
    void onGridMiddleButtonEvent(      wxKeyEvent& event);
    void OnContextRimLabelLeft(        wxGridEvent& event);
    void OnContextRimLabelRight(       wxGridEvent& event);
    void OnContextMiddle(              wxGridEvent& event);
    void OnContextMiddleLabel(         wxGridEvent& event);
    void OnContextSetLayout(           wxMouseEvent& event);
    void OnGlobalKeyEvent(             wxKeyEvent& event);

    void OnContextSelectCompVariant(      wxMouseEvent& event);
    void OnContextSelectSyncVariant(      wxMouseEvent& event);

    void applyCompareConfig(bool globalLevel = true);

    void OnSetCompVariant(wxCommandEvent& event);
    void OnSetSyncVariant(wxCommandEvent& event);

    //context menu handler methods
    void OnContextFilterTemp        (wxCommandEvent& event);
    void OnContextExcludeExtension  (wxCommandEvent& event);
    void OnContextExcludeObject     (wxCommandEvent& event);
    void OnContextOpenWith          (wxCommandEvent& event);
    void OnContextDeleteFiles       (wxCommandEvent& event);
    void OnContextSyncDirLeft       (wxCommandEvent& event);
    void OnContextSyncDirNone       (wxCommandEvent& event);
    void OnContextSyncDirRight      (wxCommandEvent& event);
    void OnContextCustColumnLeft    (wxCommandEvent& event);
    void OnContextCustColumnRight   (wxCommandEvent& event);
    void OnContextSelectTimeSpan    (wxCommandEvent& event);
    void OnContextAutoAdjustLeft    (wxCommandEvent& event);
    void OnContextAutoAdjustRight   (wxCommandEvent& event);
    void OnContextSetIconSize    (wxCommandEvent& event);
    void OnContextIncludeAll        (wxCommandEvent& event);
    void OnContextExcludeAll        (wxCommandEvent& event);
    void OnContextComparisonView    (wxCommandEvent& event);
    void OnContextSyncView          (wxCommandEvent& event);
    void OnContextSetLayoutReset    (wxCommandEvent& event);
    void OnContextSetLayoutShowPanel(wxCommandEvent& event);

    void OnContextRim(wxGridEvent& event);
    void OnDirSelected(wxFileDirPickerEvent& event);

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

    void OnSyncCreateLeft(      wxCommandEvent& event);
    void OnSyncCreateRight(     wxCommandEvent& event);
    void OnSyncDeleteLeft(      wxCommandEvent& event);
    void OnSyncDeleteRight(     wxCommandEvent& event);
    void OnSyncDirLeft(         wxCommandEvent& event);
    void OnSyncDirRight(        wxCommandEvent& event);
    void OnSyncDirNone(         wxCommandEvent& event);

    void OnNewConfig(           wxCommandEvent& event);
    void OnSaveConfig(          wxCommandEvent& event);
    void OnLoadConfig(          wxCommandEvent& event);
    void OnLoadFromHistory(     wxCommandEvent& event);

    void OnCfgHistoryKeyEvent(  wxKeyEvent& event);
    void OnRegularUpdateCheck(  wxIdleEvent& event);
    void OnLayoutWindowAsync(   wxIdleEvent& event);

    void refreshGridAfterFilterChange(int delay);

    void OnResize(              wxSizeEvent& event);
    //void OnResizeTopButtons(    wxEvent& event);
    void OnResizeFolderPairs(   wxEvent& event);
    void OnResizeConfigPanel(   wxEvent& event);
    void OnResizeViewPanel(     wxEvent& event);
    void OnResizeStatisticsPanel(wxEvent& event);
    void OnHideFilteredButton(  wxCommandEvent& event);
    void OnConfigureFilter(     wxCommandEvent& event);
    void OnSwapSides(           wxCommandEvent& event);
    void OnCompare(             wxCommandEvent& event);
    void OnSwitchView(          wxCommandEvent& event);
    void OnSyncSettings(        wxCommandEvent& event);
    void OnCmpSettings(         wxCommandEvent& event);
    void OnStartSync(           wxCommandEvent& event);
    void OnClose(               wxCloseEvent&   event);

    void OnGlobalFilterOpenContext(wxCommandEvent& event);
    void OnGlobalFilterRemConfirm(wxCommandEvent& event);

    void updateStatistics();

    void OnAddFolderPair(       wxCommandEvent& event);
    void OnRemoveFolderPair(    wxCommandEvent& event);
    void OnRemoveTopFolderPair( wxCommandEvent& event);

    void updateFilterConfig();
    void applySyncConfig();

    //menu events
    void OnMenuGlobalSettings(  wxCommandEvent& event);
    void OnMenuExportFileList(  wxCommandEvent& event);
    void OnMenuBatchJob(        wxCommandEvent& event);
    void OnMenuCheckVersion(    wxCommandEvent& event);
    void OnMenuAbout(           wxCommandEvent& event);
    void OnShowHelp(            wxCommandEvent& event);
    void OnMenuQuit(            wxCommandEvent& event);
    void OnMenuLanguageSwitch(  wxCommandEvent& event);

    void switchProgramLanguage(const int langID);

    typedef int MenuItemID;
    typedef int LanguageID;
    std::map<MenuItemID, LanguageID> languageMenuItemMap; //needed to attach menu item events

    //***********************************************
    //application variables are stored here:

    //global settings used by GUI and batch mode
    xmlAccess::XmlGlobalSettings* globalSettings; //always bound

    //UI view of FolderComparison structure
    std::unique_ptr<zen::GridView> gridDataView;

    //-------------------------------------
    //functional configuration
    xmlAccess::XmlGuiConfig currentCfg;

    //folder pairs:
    std::unique_ptr<DirectoryPairFirst> firstFolderPair; //always bound!!!
    std::vector<DirectoryPair*> additionalFolderPairs; //additional pairs to the first pair
    //-------------------------------------


    //***********************************************
    std::unique_ptr<wxMenu> contextMenu;

    //status information
    wxLongLong lastStatusChange;
    std::stack<wxString> stackObjects;

    //compare status panel (hidden on start, shown when comparing)
    std::unique_ptr<CompareStatus> compareStatus; //always bound

    bool cleanedUp;

    //remember last sort executed (for determination of sort order)
    int lastSortColumn;
    const wxGrid* lastSortGrid;

    //update icons periodically: one updater instance for both left and right grids
    std::unique_ptr<IconUpdater> updateFileIcons;

    bool processingGlobalKeyEvent; //indicator to notify recursion in OnGlobalKeyEvent()

    //encapsulation of handling of sync preview
    class SyncPreview //encapsulates MainDialog functionality for synchronization preview (friend class)
    {
    public:
        SyncPreview(MainDialog* mainDlg);

        void enablePreview(bool value);
        bool previewIsEnabled() const;

        void enableSynchronization(bool value);
        bool synchronizationIsEnabled() const;

    private:
        MainDialog* mainDlg_;
        bool syncPreviewEnabled; //toggle to display configuration preview instead of comparison result
        bool synchronizationEnabled; //determines whether synchronization should be allowed
    };
    std::unique_ptr<SyncPreview> syncPreview; //always bound

    wxAuiManager auiMgr; //implement dockable GUI design

    wxString defaultPerspective;

    zen::Int64 manualTimeSpanFrom, manualTimeSpanTo; //buffer manual time span selection at session level

    std::shared_ptr<FolderHistory> folderHistoryLeft;  //shared by all wxComboBox dropdown controls
    std::shared_ptr<FolderHistory> folderHistoryRight; //always bound!
};

#endif // MAINDIALOG_H
