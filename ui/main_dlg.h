// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) ZenJu (zhnmju123 AT gmx DOT de) - All Rights Reserved    *
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

class FolderHistory;
class DirectoryPair;
class CompareStatus;
class DirectoryPairFirst;


class MainDialog : public MainDialogGenerated
{
public:
    MainDialog(const std::vector<wxString>& cfgFileNames, //default behavior, application start
               xmlAccess::XmlGlobalSettings& settings);

    MainDialog(const std::vector<wxString>& referenceFiles,
               const xmlAccess::XmlGuiConfig& guiCfg,
               xmlAccess::XmlGlobalSettings& settings,
               bool startComparison);

    ~MainDialog();

    void disableAllElements(bool enableAbort); //dis-/enables all elements (except abort button) that might receive user input
    void enableAllElements();                   //during long-running processes: comparison, deletion

    void onQueryEndSession(); //last chance to do something before getting killed!

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

    void init(const xmlAccess::XmlGuiConfig& guiCfg,
              xmlAccess::XmlGlobalSettings& settings,
              bool startComparison);

    //configuration load/save
    void setLastUsedConfig(const wxString& filename, const xmlAccess::XmlGuiConfig& guiConfig);
    void setLastUsedConfig(const std::vector<wxString>& filenames, const xmlAccess::XmlGuiConfig& guiConfig);

    xmlAccess::XmlGuiConfig getConfig() const;
    void setConfig(const xmlAccess::XmlGuiConfig& newGuiCfg);

    void loadConfiguration(const wxString& filename);
    void loadConfiguration(const std::vector<wxString>& filenames);

    bool trySaveConfig(const wxString* fileName); //return true if saved successfully
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
    void updateGui();
    void updateGridViewData();

    //context menu functions
    std::vector<zen::FileSystemObject*> getGridSelection(bool fromLeft = true, bool fromRight = true) const;
    std::vector<zen::FileSystemObject*> getTreeSelection() const;

    void setSyncDirManually(const std::vector<zen::FileSystemObject*>& selection, zen::SyncDirection direction);
    void setManualFilter(const std::vector<zen::FileSystemObject*>& selection, bool setIncluded);
    void copySelectionToClipboard();
    void deleteSelectedFiles(const std::vector<zen::FileSystemObject*>& selectionLeft,
                             const std::vector<zen::FileSystemObject*>& selectionRight);

    void openExternalApplication(const zen::FileSystemObject* fsObj, bool leftSide, const wxString& commandline); //fsObj is optional!

    //work to be done in idle time
    void OnIdleEvent(wxEvent& event);

    //delayed status information restore
    void pushStatusInformation(const wxString& text);
    void clearStatusBar();

    //events
    void onGridButtonEvent         (wxKeyEvent& event);
    void onTreeButtonEvent         (wxKeyEvent& event);
    void OnContextSetLayout        (wxMouseEvent& event);
    void OnGlobalKeyEvent          (wxKeyEvent& event);

    void OnCompSettingsContext(wxMouseEvent& event);
    void OnSyncSettingsContext(wxMouseEvent& event);
    void OnGlobalFilterContext(wxCommandEvent& event);

    void applyCompareConfig(bool changePreviewStatus = true);

    //context menu handler methods
    void onMainGridContext(zen::GridClickEvent& event);
    void onNaviGridContext(zen::GridClickEvent& event);

    void onNaviSelection(zen::GridRangeSelectEvent& event);

    void onNaviPanelFilesDropped(zen::FileDropEvent& event);
    void OnDirSelected(wxFileDirPickerEvent& event);

    void onCheckRows       (zen::CheckRowsEvent&     event);
    void onSetSyncDirection(zen::SyncDirectionEvent& event);

    void onGridDoubleClick    (zen::GridClickEvent& event);
    void onGridLabelLeftClick (zen::GridClickEvent& event);
    void onGridLabelContext(zen::GridClickEvent& event);

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

    void OnResizeFolderPairs(   wxEvent& event);
    void OnResizeConfigPanel(   wxEvent& event);
    void OnResizeViewPanel(     wxEvent& event);
    void OnResizeStatisticsPanel(wxEvent& event);
    void OnHideFilteredButton(  wxCommandEvent& event);
    void OnConfigureFilter(     wxCommandEvent& event);
    void OnSwapSides(           wxCommandEvent& event);
    void OnCompare(             wxCommandEvent& event);
    void OnSyncSettings(        wxCommandEvent& event);
    void OnCmpSettings(         wxCommandEvent& event);
    void OnStartSync(           wxCommandEvent& event);
    void OnClose(               wxCloseEvent&   event);

    void updateGuiAfterFilterChange(int delay);

    void excludeExtension(const Zstring& extension);
    void excludeItems(const std::vector<zen::FileSystemObject*>& selection);

    void updateStatistics();

    void updateSyncEnabledStatus();

    void OnAddFolderPair(       wxCommandEvent& event);
    void OnRemoveFolderPair(    wxCommandEvent& event);
    void OnRemoveTopFolderPair( wxCommandEvent& event);

    void applyFilterConfig();
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
    xmlAccess::XmlGlobalSettings* globalSettings; //always bound!

    //UI view of FolderComparison structure (partially owns folderCmp)
    std::shared_ptr<zen::GridView> gridDataView; //always bound!
    std::shared_ptr<zen::TreeView> treeDataView; //

    //the prime data structure of this tool *bling*:
    zen::FolderComparison folderCmp; //optional!: sync button not available if empty

    void clearGrid(bool refreshGrid = true);

    //-------------------------------------
    //functional configuration
    xmlAccess::XmlGuiConfig currentCfg;

    //folder pairs:
    std::unique_ptr<DirectoryPairFirst> firstFolderPair; //always bound!!!
    std::vector<DirectoryPair*> additionalFolderPairs; //additional pairs to the first pair
    //-------------------------------------


    //***********************************************
    //status information
    wxLongLong lastStatusChange;
    std::stack<wxString> stackObjects;

    //compare status panel (hidden on start, shown when comparing)
    std::unique_ptr<CompareStatus> compareStatus; //always bound

    bool cleanedUp;

    bool processingGlobalKeyEvent; //indicator to notify recursion in OnGlobalKeyEvent()

    bool syncPreviewEnabled; //toggle to display configuration preview instead of comparison result
    //use this methods when changing values!
    void enablePreview(bool value);

    wxAuiManager auiMgr; //implement dockable GUI design

    wxString defaultPerspective;

    zen::Int64 manualTimeSpanFrom, manualTimeSpanTo; //buffer manual time span selection at session level

    std::shared_ptr<FolderHistory> folderHistoryLeft;  //shared by all wxComboBox dropdown controls
    std::shared_ptr<FolderHistory> folderHistoryRight; //always bound!
};

#endif // MAINDIALOG_H
