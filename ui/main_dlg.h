// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) Zenju (zenju AT gmx DOT de) - All Rights Reserved        *
// **************************************************************************

#ifndef MAINDIALOG_H
#define MAINDIALOG_H

#include <memory>
#include <map>
#include <set>
#include <wx/aui/aui.h>
#include <zen/int64.h>
#include <stack>
#include "gui_generated.h"
#include "../lib/process_xml.h"
#include "custom_grid.h"
#include "tree_view.h"
#include <wx+/file_drop.h>
#include "folder_history_box.h"

//class FolderHistory;
class DirectoryPair;
class CompareStatus;
class DirectoryPairFirst;


class MainDialog : public MainDialogGenerated
{
public:
    //default behavior, application start
    static void create(const std::vector<wxString>& cfgFileNames); //cfgFileNames empty: restore last config; non-empty load/merge given set of config files

    //load dynamically assembled config
    static void create(const xmlAccess::XmlGuiConfig& guiCfg, bool startComparison);

    //when switching language or switching from batch run to GUI on warnings
    static void create(const xmlAccess::XmlGuiConfig& guiCfg,
                       const std::vector<wxString>& referenceFiles,
                       const xmlAccess::XmlGlobalSettings& globalSettings, //take over ownership => save on exit
                       bool startComparison);

    void disableAllElements(bool enableAbort); //dis-/enables all elements (except abort button) that might receive user input
    void enableAllElements();                   //during long-running processes: comparison, deletion

    void onQueryEndSession(); //last chance to do something useful before killing the application!

private:
    static void create_impl(const xmlAccess::XmlGuiConfig& guiCfg,
                            const std::vector<wxString>& referenceFiles,
                            const xmlAccess::XmlGlobalSettings& globalSettings,
                            bool startComparison);

    MainDialog(const xmlAccess::XmlGuiConfig& guiCfg,
               const std::vector<wxString>& referenceFiles,
               const xmlAccess::XmlGlobalSettings& globalSettings, //take over ownership => save on exit
               bool startComparison);
    ~MainDialog();

    friend class CompareStatusHandler;
    friend class SyncStatusHandler;
    friend class ManualDeletionHandler;
    friend class DirectoryPairFirst;
    friend class DirectoryPair;
    friend class DirectoryNameMainImpl;
    template <class GuiPanel>
    friend class FolderPairCallback;
    friend class PanelMoveWindow;

    //configuration load/save
    void setLastUsedConfig(const wxString& filename, const xmlAccess::XmlGuiConfig& guiConfig);
    void setLastUsedConfig(const std::vector<wxString>& filenames, const xmlAccess::XmlGuiConfig& guiConfig);

    xmlAccess::XmlGuiConfig getConfig() const;
    void setConfig(const xmlAccess::XmlGuiConfig& newGuiCfg, const std::vector<wxString>& referenceFiles);

    void setGlobalCfgOnInit(const xmlAccess::XmlGlobalSettings& globalSettings); //messes with Maximize(), window sizes, so call just once!
    xmlAccess::XmlGlobalSettings getGlobalCfgBeforeExit(); //destructive "get" thanks to "Iconize(false), Maximize(false)"

    bool loadConfiguration(const std::vector<wxString>& filenames); //return true if loaded successfully

    bool trySaveConfig(const wxString* fileName); //return true if saved successfully
    bool saveOldConfig(); //return false on user abort

    static const wxString& lastRunConfigName();

    xmlAccess::XmlGuiConfig lastConfigurationSaved; //support for: "Save changed configuration?" dialog
    //used when saving configuration
    std::vector<wxString> activeConfigFiles; //name of currently loaded config file (may be more than 1)

    void initViewFilterButtons(const zen::MainConfiguration& mainCfg);
    void updateFilterButtons();

    void addFileToCfgHistory(const std::vector<wxString>& filenames); //= update/insert + apply selection

    void addFolderPair(const std::vector<zen::FolderPairEnh>& newPairs, bool addFront = false);
    void removeAddFolderPair(size_t pos);
    void clearAddFolderPairs();

    void updateGuiForFolderPair(); //helper method: add usability by showing/hiding buttons related to folder pairs

    //main method for putting gridDataView on UI: updates data respecting current view settings
    void updateGui(); //kitchen-sink update
    void updateGuiDelayedIf(bool condition); // 400 ms delay

    void updateGridViewData();     //
    void updateStatistics();       // more fine-grained updaters
    void updateUnsavedCfgStatus(); //

    //context menu functions
    std::vector<zen::FileSystemObject*> getGridSelection(bool fromLeft = true, bool fromRight = true) const;
    std::vector<zen::FileSystemObject*> getTreeSelection() const;

    void setSyncDirManually(const std::vector<zen::FileSystemObject*>& selection, zen::SyncDirection direction);
    void setFilterManually(const std::vector<zen::FileSystemObject*>& selection, bool setIncluded);
    void copySelectionToClipboard();
    void deleteSelectedFiles(const std::vector<zen::FileSystemObject*>& selectionLeft,
                             const std::vector<zen::FileSystemObject*>& selectionRight);

    void openExternalApplication(const wxString& commandline, const zen::FileSystemObject* fsObj, bool leftSide); //fsObj may be nullptr

    //work to be done in idle time
    void OnIdleEvent(wxEvent& event);

    //delayed status information restore
    void setStatusInformation(const wxString& msg);  //set main status
    void flashStatusInformation(const wxString& msg); //temporarily show different status

    //events
    void onGridButtonEventL(wxKeyEvent& event);
    void onGridButtonEventC(wxKeyEvent& event);
    void onGridButtonEventR(wxKeyEvent& event);
    void onGridButtonEvent (wxKeyEvent& event, zen::Grid& grid, bool leftSide);

    void onTreeButtonEvent (wxKeyEvent& event);
    void OnContextSetLayout(wxMouseEvent& event);
    void OnGlobalKeyEvent  (wxKeyEvent& event);

    void OnCompSettingsContext(wxMouseEvent& event);
    void OnSyncSettingsContext(wxMouseEvent& event);
    void OnGlobalFilterContext(wxCommandEvent& event);

    void applyCompareConfig(bool switchMiddleGrid = false);

    //context menu handler methods
    void onMainGridContextL(zen::GridClickEvent& event);
    void onMainGridContextC(zen::GridClickEvent& event);
    void onMainGridContextR(zen::GridClickEvent& event);
    void onMainGridContextRim(bool leftSide);

    void onNaviGridContext(zen::GridClickEvent& event);

    void onNaviSelection(zen::GridRangeSelectEvent& event);

    void onNaviPanelFilesDropped(zen::FileDropEvent& event);

    void onDirSelected(wxCommandEvent& event);
    void onDirManualCorrection(wxCommandEvent& event);

    void onCheckRows       (zen::CheckRowsEvent&     event);
    void onSetSyncDirection(zen::SyncDirectionEvent& event);

    void onGridDoubleClickL(zen::GridClickEvent& event);
    void onGridDoubleClickR(zen::GridClickEvent& event);
    void onGridDoubleClickRim(int row, bool leftSide);

    void onGridLabelLeftClickC(zen::GridClickEvent& event);
    void onGridLabelLeftClickL(zen::GridClickEvent& event);
    void onGridLabelLeftClickR(zen::GridClickEvent& event);
    void onGridLabelLeftClick(bool onLeft, zen::ColumnTypeRim type);

    void onGridLabelContextL(zen::GridClickEvent& event);
    void onGridLabelContextC(zen::GridClickEvent& event);
    void onGridLabelContextR(zen::GridClickEvent& event);
    void onGridLabelContext(zen::Grid& grid, zen::ColumnTypeRim type, const std::vector<zen::ColumnAttributeRim>& defaultColumnAttributes);

    void OnLeftOnlyFiles  (wxCommandEvent& event);
    void OnRightOnlyFiles (wxCommandEvent& event);
    void OnLeftNewerFiles (wxCommandEvent& event);
    void OnRightNewerFiles(wxCommandEvent& event);
    void OnEqualFiles     (wxCommandEvent& event);
    void OnDifferentFiles (wxCommandEvent& event);
    void OnConflictFiles  (wxCommandEvent& event);

    void OnSyncCreateLeft (wxCommandEvent& event);
    void OnSyncCreateRight(wxCommandEvent& event);
    void OnSyncDeleteLeft (wxCommandEvent& event);
    void OnSyncDeleteRight(wxCommandEvent& event);
    void OnSyncDirLeft    (wxCommandEvent& event);
    void OnSyncDirRight   (wxCommandEvent& event);
    void OnSyncDirNone    (wxCommandEvent& event);

    void OnConfigNew      (wxCommandEvent& event);
    void OnConfigSave     (wxCommandEvent& event);
    void OnConfigSaveAs   (wxCommandEvent& event);
    void OnConfigLoad     (wxCommandEvent& event);
    void OnLoadFromHistory(wxCommandEvent& event);
    void OnLoadFromHistoryDoubleClick(wxCommandEvent& event);

    void OnCfgHistoryKeyEvent(wxKeyEvent& event);
    void OnRegularUpdateCheck(wxIdleEvent& event);
    void OnLayoutWindowAsync (wxIdleEvent& event);

    void OnResizeLeftFolderWidth(wxEvent& event);
    void OnResizeConfigPanel    (wxEvent& event);
    void OnResizeViewPanel      (wxEvent& event);
    void OnResizeStatisticsPanel(wxEvent& event);
    void OnShowExcluded         (wxCommandEvent& event);
    void OnConfigureFilter      (wxCommandEvent& event);
    void OnSwapSides            (wxCommandEvent& event);
    void OnCompare              (wxCommandEvent& event);
    void OnSyncSettings         (wxCommandEvent& event);
    void OnCmpSettings          (wxCommandEvent& event);
    void OnStartSync            (wxCommandEvent& event);
    void OnClose                (wxCloseEvent&   event);

    void excludeExtension(const Zstring& extension);
    void excludeShortname(const zen::FileSystemObject& fsObj);
    void excludeItems(const std::vector<zen::FileSystemObject*>& selection);

    void OnAddFolderPair      (wxCommandEvent& event);
    void OnRemoveFolderPair   (wxCommandEvent& event);
    void OnRemoveTopFolderPair(wxCommandEvent& event);

    void applyFilterConfig();
    void applySyncConfig();

    //menu events
    void OnMenuGlobalSettings(wxCommandEvent& event);
    void OnMenuExportFileList(wxCommandEvent& event);
    void OnMenuBatchJob      (wxCommandEvent& event);
    void OnMenuCheckVersion  (wxCommandEvent& event);
    void OnMenuAbout         (wxCommandEvent& event);
    void OnShowHelp          (wxCommandEvent& event);
    void OnMenuQuit          (wxCommandEvent& event);
    void OnMenuLanguageSwitch(wxCommandEvent& event);

    void switchProgramLanguage(int langID);

    typedef int MenuItemID;
    typedef int LanguageID;
    std::map<MenuItemID, LanguageID> languageMenuItemMap; //needed to attach menu item events

    //***********************************************
    //application variables are stored here:

    //global settings shared by GUI and batch mode
    xmlAccess::XmlGlobalSettings globalCfg;

    //UI view of FolderComparison structure (partially owns folderCmp)
    std::shared_ptr<zen::GridView> gridDataView; //always bound!
    std::shared_ptr<zen::TreeView> treeDataView; //

    //the prime data structure of this tool *bling*:
    zen::FolderComparison folderCmp; //optional!: sync button not available if empty

    void clearGrid();

    //-------------------------------------
    //program configuration
    xmlAccess::XmlGuiConfig currentCfg;

    //folder pairs:
    std::unique_ptr<DirectoryPairFirst> firstFolderPair; //always bound!!!
    std::vector<DirectoryPair*> additionalFolderPairs; //additional pairs to the first pair
    //-------------------------------------

    //***********************************************
    //status information
    wxLongLong lastStatusChange;
    std::vector<wxString> statusMsgStack;

    //compare status panel (hidden on start, shown when comparing)
    std::unique_ptr<CompareStatus> compareStatus; //always bound

    bool cleanedUp;

    bool processingGlobalKeyEvent; //indicator to notify recursion in OnGlobalKeyEvent()

    bool showSyncAction_; //toggle to display configuration preview instead of comparison result
    //use this methods when changing values!
    void showSyncAction(bool value);

    wxAuiManager auiMgr; //implement dockable GUI design

    wxString defaultPerspective;

    zen::Int64 manualTimeSpanFrom, manualTimeSpanTo; //buffer manual time span selection at session level

    std::shared_ptr<FolderHistory> folderHistoryLeft;  //shared by all wxComboBox dropdown controls
    std::shared_ptr<FolderHistory> folderHistoryRight; //always bound!
};

#endif // MAINDIALOG_H
