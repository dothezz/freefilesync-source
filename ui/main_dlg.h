// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) 2008-2011 ZenJu (zhnmju123 AT gmx.de)                    *
// **************************************************************************
//
#ifndef MAINDIALOG_H
#define MAINDIALOG_H

#include "gui_generated.h"
#include <stack>
#include "../library/process_xml.h"
#include <memory>
#include <map>
#include <set>
#include <wx/aui/aui.h>


class CustomGrid;
class FFSCheckRowsEvent;
class FFSSyncDirectionEvent;
class IconUpdater;
class DirectoryPair;
class DirectoryPairFirst;
class CompareStatus;
class SyncStatusHandler;
class PanelMoveWindow;


namespace ffs3
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
    bool readConfigurationFromXml(const wxString& filename);
    bool writeConfigurationToXml(const wxString& filename);
    void setLastUsedConfig(const wxString& filename, const xmlAccess::XmlGuiConfig& guiConfig);

    xmlAccess::XmlGuiConfig getCurrentConfiguration() const;
    void setCurrentConfiguration(const xmlAccess::XmlGuiConfig& newGuiCfg);

    static const wxString& lastRunConfigName();

    xmlAccess::XmlGuiConfig lastConfigurationSaved; //support for: "Save changed configuration?" dialog
    //used when saving configuration
    wxString currentConfigFileName;

    void readGlobalSettings();
    void writeGlobalSettings();

    void initViewFilterButtons();
    void updateFilterButtons();

    void addFileToCfgHistory(const wxString& filename);
    void addLeftFolderToHistory(const wxString& leftFolder);
    void addRightFolderToHistory(const wxString& rightFolder);

    void addFolderPair(const std::vector<ffs3::FolderPairEnh>& newPairs, bool addFront = false);
    void removeAddFolderPair(size_t pos);
    void clearAddFolderPairs();

    void updateGuiForFolderPair(); //helper method: add usability by showing/hiding buttons related to folder pairs

    //main method for putting gridDataView on UI: updates data respecting current view settings
    void updateGuiGrid();
    void updateGridViewData();

    //context menu functions
    std::set<size_t> getSelectedRows(const CustomGrid* grid) const;
    std::set<size_t> getSelectedRows() const;
    void setSyncDirManually(const std::set<size_t>& rowsToSetOnUiTable, const ffs3::SyncDirection dir);
    void filterRangeManually(const std::set<size_t>& rowsToFilterOnUiTable, int leadingRow);
    void copySelectionToClipboard(CustomGrid& selectedGrid);
    void deleteSelectedFiles();

    void openExternalApplication(const wxString& commandline);
    void openExternalApplication(size_t rowNumber, bool leftSide, const wxString& commandline);

    //work to be done in idle time
    void OnIdleEvent(wxEvent& event);

    //delayed status information restore
    void pushStatusInformation(const wxString& text);
    void clearStatusBar();

    void disableAllElements(); //dis-/enables all elements (except abort button) that might receive user input during long-running processes: comparison, deletion
    void enableAllElements();  //

    //events
    void onGridLeftButtonEvent(        wxKeyEvent& event);
    void onGridRightButtonEvent(       wxKeyEvent& event);
    void onGridMiddleButtonEvent(      wxKeyEvent& event);
    void OnContextRim(                 wxGridEvent& event);
    void OnContextRimLabelLeft(        wxGridEvent& event);
    void OnContextRimLabelRight(       wxGridEvent& event);
    void OnContextMiddle(              wxGridEvent& event);
    void OnContextMiddleLabel(         wxGridEvent& event);
    void OnContextSetLayout(           wxMouseEvent& event);
    void OnGlobalKeyEvent(             wxKeyEvent& event);

    //context menu handler methods
    void OnContextFilterTemp        (wxCommandEvent& event);
    void OnContextExcludeExtension  (wxCommandEvent& event);
    void OnContextExcludeObject     (wxCommandEvent& event);
    void OnContextCopyClipboard     (wxCommandEvent& event);
    void OnContextOpenWith          (wxCommandEvent& event);
    void OnContextDeleteFiles       (wxCommandEvent& event);
    void OnContextSyncDirLeft       (wxCommandEvent& event);
    void OnContextSyncDirNone       (wxCommandEvent& event);
    void OnContextSyncDirRight      (wxCommandEvent& event);
    void OnContextCustColumnLeft    (wxCommandEvent& event);
    void OnContextCustColumnRight   (wxCommandEvent& event);
    void OnContextAutoAdjustLeft    (wxCommandEvent& event);
    void OnContextAutoAdjustRight   (wxCommandEvent& event);
    void OnContextIncludeAll        (wxCommandEvent& event);
    void OnContextExcludeAll        (wxCommandEvent& event);
    void OnContextComparisonView    (wxCommandEvent& event);
    void OnContextSyncView          (wxCommandEvent& event);
    void OnContextSetLayoutReset    (wxCommandEvent& event);
    void OnContextSetLayoutShowPanel(wxCommandEvent& event);

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
    bool trySaveConfig(); //return true if saved successfully
    bool saveOldConfig(); //return false on user abort

    void loadConfiguration(const wxString& filename);
    void OnCfgHistoryKeyEvent(  wxKeyEvent& event);
    void OnRegularUpdateCheck(  wxIdleEvent& event);
    void OnLayoutWindowAsync(   wxIdleEvent& event);

    void refreshGridAfterFilterChange(const int delay);

    void OnResize(              wxSizeEvent& event);
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
    std::auto_ptr<ffs3::GridView> gridDataView;

    //-------------------------------------
    //functional configuration
    xmlAccess::XmlGuiConfig currentCfg;

    //folder pairs:
    std::auto_ptr<DirectoryPairFirst> firstFolderPair; //always bound!!!
    std::vector<DirectoryPair*> additionalFolderPairs; //additional pairs to the first pair

    //gui settings
    int widthNotMaximized;
    int heightNotMaximized;
    int posXNotMaximized;
    int posYNotMaximized;
    //-------------------------------------


    //***********************************************
    std::auto_ptr<wxMenu> contextMenu;

    //status information
    wxLongLong lastStatusChange;
    std::stack<wxString> stackObjects;

    //compare status panel (hidden on start, shown when comparing)
    std::auto_ptr<CompareStatus> compareStatus; //always bound

    bool cleanedUp;

    //remember last sort executed (for determination of sort order)
    int lastSortColumn;
    const wxGrid* lastSortGrid;

    //update icons periodically: one updater instance for both left and right grids
    std::auto_ptr<IconUpdater> updateFileIcons;

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
    std::auto_ptr<SyncPreview> syncPreview; //always bound

    wxAuiManager auiMgr; //implement dockable GUI design

    wxString defaultPerspective;
};

#endif // MAINDIALOG_H
