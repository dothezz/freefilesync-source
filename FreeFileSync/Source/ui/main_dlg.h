// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) Zenju (zenju AT gmx DOT de) - All Rights Reserved        *
// **************************************************************************

#ifndef MAINDIALOG_H
#define MAINDIALOG_H

#include <map>
#include <set>
#include <stack>
#include <memory>
#include <zen/int64.h>
#include <zen/async_task.h>
#include <wx+/file_drop.h>
#include <wx/aui/aui.h>
#include "gui_generated.h"
#include "custom_grid.h"
#include "tree_view.h"
#include "folder_history_box.h"
#include "../lib/process_xml.h"

class FolderPairFirst;
class FolderPairPanel;
class CompareProgressDialog;


class MainDialog : public MainDialogGenerated
{
public:
    //default behavior, application start, restores last used config
    static void create();

    //when loading dynamically assembled config,
    //when switching language,
    //or switching from batch run to GUI on warnings
    static void create(const xmlAccess::XmlGuiConfig& guiCfg,
                       const std::vector<Zstring>& referenceFiles,
                       const xmlAccess::XmlGlobalSettings* globalSettings, //optional: take over ownership => save on exit
                       bool startComparison);

    void disableAllElements(bool enableAbort); //dis-/enables all elements (except abort button) that might receive user input
    void enableAllElements();                  //during long-running processes: comparison, deletion

    void onQueryEndSession(); //last chance to do something useful before killing the application!

private:
    MainDialog(const xmlAccess::XmlGuiConfig& guiCfg,
               const std::vector<Zstring>& referenceFiles,
               const xmlAccess::XmlGlobalSettings& globalSettings, //take over ownership => save on exit
               bool startComparison);
    ~MainDialog();

    friend class CompareStatusHandler;
    friend class SyncStatusHandler;
    friend class ManualDeletionHandler;
    friend class FolderPairFirst;
    friend class FolderPairPanel;
    friend class DirectoryNameMainImpl;
    template <class GuiPanel>
    friend class FolderPairCallback;
    friend class PanelMoveWindow;

    //configuration load/save
    void setLastUsedConfig(const Zstring& filename, const xmlAccess::XmlGuiConfig& guiConfig);
    void setLastUsedConfig(const std::vector<Zstring>& filenames, const xmlAccess::XmlGuiConfig& guiConfig);

    xmlAccess::XmlGuiConfig getConfig() const;
    void setConfig(const xmlAccess::XmlGuiConfig& newGuiCfg, const std::vector<Zstring>& referenceFiles);

    void setGlobalCfgOnInit(const xmlAccess::XmlGlobalSettings& globalSettings); //messes with Maximize(), window sizes, so call just once!
    xmlAccess::XmlGlobalSettings getGlobalCfgBeforeExit(); //destructive "get" thanks to "Iconize(false), Maximize(false)"

    bool loadConfiguration(const std::vector<Zstring>& filenames); //return true if loaded successfully

    bool trySaveConfig     (const Zstring* fileNameGui); //return true if saved successfully
    bool trySaveBatchConfig(const Zstring* fileNameBatch); //
    bool saveOldConfig(); //return false on user abort

    static const Zstring& lastRunConfigName();

    xmlAccess::XmlGuiConfig lastConfigurationSaved; //support for: "Save changed configuration?" dialog
    //used when saving configuration
    std::vector<Zstring> activeConfigFiles; //name of currently loaded config file (may be more than 1)

    void updateGlobalFilterButton();

    void initViewFilterButtons();
    void setViewFilterDefault();

    void addFileToCfgHistory(const std::vector<Zstring>& filenames); //= update/insert + apply selection
    void removeObsoleteCfgHistoryItems(const std::vector<Zstring>& filenames);
    void removeCfgHistoryItems(const std::vector<Zstring>& filenames);

    void addAddFolderPair(const std::vector<zen::FolderPairEnh>& newPairs, bool addFront = false);
    void removeAddFolderPair(size_t pos);
    void setAddFolderPairs(const std::vector<zen::FolderPairEnh>& newPairs);

    void updateGuiForFolderPair(); //helper method: add usability by showing/hiding buttons related to folder pairs

    //main method for putting gridDataView on UI: updates data respecting current view settings
    void updateGui(); //kitchen-sink update
    void updateGuiDelayedIf(bool condition); // 400 ms delay

    void updateGridViewData();     //
    void updateStatistics();       // more fine-grained updaters
    void updateUnsavedCfgStatus(); //
    void updateTopButtonImages();  //

    //context menu functions
    std::vector<zen::FileSystemObject*> getGridSelection(bool fromLeft = true, bool fromRight = true) const;
    std::vector<zen::FileSystemObject*> getTreeSelection() const;

    void setSyncDirManually(const std::vector<zen::FileSystemObject*>& selection, zen::SyncDirection direction);
    void setFilterManually(const std::vector<zen::FileSystemObject*>& selection, bool setIncluded);
    void copySelectionToClipboard(const std::vector<const zen::Grid*>& gridRefs);
    void deleteSelectedFiles(const std::vector<zen::FileSystemObject*>& selectionLeft,
                             const std::vector<zen::FileSystemObject*>& selectionRight);

    void openExternalApplication(const wxString& commandline, const std::vector<zen::FileSystemObject*>& selection, bool leftSide); //selection may be empty

    //don't use wxWidgets idle handling => repeated idle requests/consumption hogs 100% cpu!
    void onProcessAsyncTasks(wxEvent& event);

    template <class Fun, class Fun2>
    void processAsync(Fun doAsync, Fun2 evalOnGui) { asyncTasks.add(doAsync, evalOnGui); timerForAsyncTasks.Start(50); /*timer interval in [ms] */ }
    template <class Fun, class Fun2>
    void processAsync2(Fun doAsync, Fun2 evalOnGui) { asyncTasks.add2(doAsync, evalOnGui); timerForAsyncTasks.Start(50); /*timer interval in [ms] */ }

    //status bar supports one of the following two states at a time:
    void setStatusBarFullText(const wxString& msg);
    void setStatusBarFileStatistics(size_t filesOnLeftView, size_t foldersOnLeftView, size_t filesOnRightView, size_t foldersOnRightView, zen::UInt64 filesizeLeftView, zen::UInt64 filesizeRightView);

    void flashStatusInformation(const wxString& msg); //temporarily show different status (only valid for setStatusBarFullText)
    void restoreStatusInformation();                  //called automatically after a few seconds

    //events
    void onGridButtonEventL(wxKeyEvent& event);
    void onGridButtonEventC(wxKeyEvent& event);
    void onGridButtonEventR(wxKeyEvent& event);
    void onGridButtonEvent (wxKeyEvent& event, zen::Grid& grid, bool leftSide);

    void onTreeButtonEvent (wxKeyEvent& event);
    void OnContextSetLayout(wxMouseEvent& event);
    void OnGlobalKeyEvent  (wxKeyEvent& event);

    virtual void OnCompSettingsContext(wxMouseEvent& event) override;
    virtual void OnSyncSettingsContext(wxMouseEvent& event) override;
    virtual void OnGlobalFilterContext(wxMouseEvent& event) override;
    virtual void OnViewButtonRightClick(wxMouseEvent& event) override;

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
    void onGridDoubleClickRim(size_t row, bool leftSide);

    void onGridLabelLeftClickC(zen::GridClickEvent& event);
    void onGridLabelLeftClickL(zen::GridClickEvent& event);
    void onGridLabelLeftClickR(zen::GridClickEvent& event);
    void onGridLabelLeftClick(bool onLeft, zen::ColumnTypeRim type);

    void onGridLabelContextL(zen::GridClickEvent& event);
    void onGridLabelContextC(zen::GridClickEvent& event);
    void onGridLabelContextR(zen::GridClickEvent& event);
    void onGridLabelContext(zen::Grid& grid, zen::ColumnTypeRim type, const std::vector<zen::ColumnAttributeRim>& defaultColumnAttributes);

    virtual void OnToggleViewType  (wxCommandEvent& event) override;
    virtual void OnToggleViewButton(wxCommandEvent& event) override;

    virtual void OnConfigNew      (wxCommandEvent& event) override;
    virtual void OnConfigSave     (wxCommandEvent& event) override;
    virtual void OnConfigSaveAs   (wxCommandEvent& event) override;
    virtual void OnSaveAsBatchJob (wxCommandEvent& event) override;
    virtual void OnConfigLoad     (wxCommandEvent& event) override;
    virtual void OnLoadFromHistory(wxCommandEvent& event) override;
    virtual void OnLoadFromHistoryDoubleClick(wxCommandEvent& event);

    void deleteSelectedCfgHistoryItems();

    virtual void OnCfgHistoryRightClick(wxMouseEvent& event) override;
    void OnCfgHistoryKeyEvent  (wxKeyEvent&   event) override;
    void OnRegularUpdateCheck  (wxIdleEvent&  event);
    void OnLayoutWindowAsync   (wxIdleEvent&  event);

    void OnResizeLeftFolderWidth(wxEvent& event);
    void OnResizeConfigPanel    (wxEvent& event);
    void OnResizeViewPanel      (wxEvent& event);
    virtual void OnConfigureFilter      (wxCommandEvent& event) override;
    virtual void OnSwapSides            (wxCommandEvent& event) override;
    virtual void OnCompare              (wxCommandEvent& event) override;
    virtual void OnSyncSettings         (wxCommandEvent& event) override;
    virtual void OnCmpSettings          (wxCommandEvent& event) override;
    virtual void OnStartSync            (wxCommandEvent& event) override;
    virtual void OnClose                (wxCloseEvent&   event) override;

    void filterExtension(const Zstring& extension, bool include);
    void filterShortname(const zen::FileSystemObject& fsObj, bool include);
    void filterItems(const std::vector<zen::FileSystemObject*>& selection, bool include);
    void filterPhrase(const Zstring& phrase, bool include, bool addNewLine);

    virtual void OnAddFolderPair      (wxCommandEvent& event) override;
    void OnRemoveFolderPair   (wxCommandEvent& event);
    virtual void OnRemoveTopFolderPair(wxCommandEvent& event) override;

    void applyFilterConfig();
    void applySyncConfig();

    void showFindPanel(); //CTRL + F
    void hideFindPanel();
    void startFindNext(); //F3

    virtual void OnSearchGridEnter(wxCommandEvent& event) override;
    virtual void OnHideSearchPanel(wxCommandEvent& event) override;
    void OnSearchPanelKeyPressed(wxKeyEvent& event);

    //menu events
    virtual void OnMenuGlobalSettings(wxCommandEvent& event) override;
    virtual void OnMenuExportFileList(wxCommandEvent& event) override;
    virtual void OnMenuFindItem      (wxCommandEvent& event) override;
    virtual void OnMenuCheckVersion  (wxCommandEvent& event) override;
    virtual void OnMenuCheckVersionAutomatically(wxCommandEvent& event) override;
    virtual void OnMenuAbout         (wxCommandEvent& event) override;
    virtual void OnShowHelp          (wxCommandEvent& event) override;
    virtual void OnMenuQuit          (wxCommandEvent& event) override { Close(); }

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
    std::unique_ptr<FolderPairFirst> firstFolderPair; //always bound!!!
    std::vector<FolderPairPanel*> additionalFolderPairs; //additional pairs to the first pair
    //-------------------------------------

    //***********************************************
    //status information
    std::list<wxString> oldStatusMsgs; //the first one is the original/non-flash status message

    //compare status panel (hidden on start, shown when comparing)
    std::unique_ptr<CompareProgressDialog> compareStatus; //always bound

    bool cleanedUp;

    bool processingGlobalKeyEvent; //indicator to notify recursion in OnGlobalKeyEvent()

    //toggle to display configuration preview instead of comparison result:
    //for read access use: m_bpButtonViewTypeSyncAction->isActive()
    //when changing value use:
    void setViewTypeSyncAction(bool value);

    wxAuiManager auiMgr; //implement dockable GUI design

    wxString defaultPerspective;

    zen::Int64 manualTimeSpanFrom, manualTimeSpanTo; //buffer manual time span selection at session level

    std::shared_ptr<FolderHistory> folderHistoryLeft;  //shared by all wxComboBox dropdown controls
    std::shared_ptr<FolderHistory> folderHistoryRight; //always bound!

    //schedule and run long-running tasks asynchronously, but process results on GUI queue
    zen::AsyncTasks asyncTasks;
    wxTimer timerForAsyncTasks; //don't use wxWidgets idle handling => repeated idle requests/consumption hogs 100% cpu!

    std::unique_ptr<zen::FilterConfig> filterCfgOnClipboard; //copy/paste of filter config

    wxWindow* focusWindowAfterSearch; //used to restore focus after search panel is closed
};

#endif // MAINDIALOG_H
