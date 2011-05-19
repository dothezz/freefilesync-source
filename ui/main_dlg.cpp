// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) 2008-2011 ZenJu (zhnmju123 AT gmx.de)                    *
// **************************************************************************
//
#include "main_dlg.h"
#include <iterator>
#include <stdexcept>
#include <wx/clipbrd.h>
#include <wx/dataobj.h>
#include <wx/ffile.h>
#include <wx/imaglist.h>
#include <wx/wupdlock.h>
#include <wx/msgdlg.h>
#include <wx/sound.h>
#include <wx/display.h>
#include <wx/app.h>
#include <boost/bind.hpp>
#include "../shared/system_constants.h"
#include "../library/custom_grid.h"
#include "../shared/custom_button.h"
#include "../shared/custom_combo_box.h"
#include "../shared/dir_picker_i18n.h"
#include "../comparison.h"
#include "../synchronization.h"
#include "../algorithm.h"
#include "../shared/app_main.h"
#include "../shared/util.h"
#include "check_version.h"
#include "gui_status_handler.h"
#include "sync_cfg.h"
#include "../shared/i18n.h"
#include "../shared/string_conv.h"
#include "small_dlgs.h"
#include "../shared/mouse_move_dlg.h"
#include "progress_indicator.h"
#include "msg_popup.h"
#include "../shared/dir_name.h"
#include "../structures.h"
#include "grid_view.h"
#include "../library/resources.h"
#include "../shared/file_handling.h"
#include "../shared/resolve_path.h"
#include "../shared/recycler.h"
#include "../shared/xml_base.h"
#include "../shared/standard_paths.h"
#include "../shared/toggle_button.h"
#include "folder_pair.h"
#include "../shared/global_func.h"
#include "search.h"
#include "../shared/help_provider.h"
#include "batch_config.h"
#include "../shared/check_exist.h"
#include "../library/lock_holder.h"
#include "../shared/shell_execute.h"

using namespace zen;

namespace
{
struct wxClientDataString : public wxClientData //we need a wxClientData derived class to tell wxWidgets to take object ownership!
{
    wxClientDataString(const wxString& name) : name_(name) {}

    wxString name_;
};
}


class DirectoryNameMainImpl : public DirectoryNameMainDlg
{
public:
    DirectoryNameMainImpl(MainDialog&      mainDlg,
                          wxWindow&        dropWindow1,
                          wxWindow&        dropWindow2,
                          wxDirPickerCtrl& dirPicker,
                          wxComboBox&      dirName,
                          wxStaticBoxSizer& staticBox) :
        DirectoryNameMainDlg(dropWindow1, dropWindow2, dirPicker, dirName, staticBox),
        mainDlg_(mainDlg) {}

    virtual bool AcceptDrop(const std::vector<wxString>& droppedFiles)
    {
        if (droppedFiles.empty())
            return true;

        switch (xmlAccess::getMergeType(droppedFiles))   //throw ()
        {
            case xmlAccess::MERGE_BATCH:
                if (droppedFiles.size() == 1)
                {
                    if (showSyncBatchDlg(droppedFiles[0]) == ReturnBatchConfig::BATCH_FILE_SAVED)
                        mainDlg_.pushStatusInformation(_("Batch file created successfully!"));
                    return false;
                }
                //fall-through for multiple *.ffs_batch files!

            case xmlAccess::MERGE_GUI:
            case xmlAccess::MERGE_GUI_BATCH:
                if (droppedFiles.size() == 1)
                {
                    mainDlg_.loadConfiguration(droppedFiles[0]);
                    return false;
                }
                else
                {
                    xmlAccess::XmlGuiConfig guiCfg;
                    try
                    {
                        convertConfig(droppedFiles, guiCfg); //throw (xmlAccess::XmlError)
                    }
                    catch (const xmlAccess::XmlError& error)
                    {
                        if (error.getSeverity() == xmlAccess::XmlError::WARNING)
                            wxMessageBox(error.msg(), _("Warning"), wxOK | wxICON_WARNING);
                        else
                        {
                            wxMessageBox(error.msg(), _("Error"), wxOK | wxICON_ERROR);
                            return false;
                        }
                    }
                    mainDlg_.setCurrentConfiguration(guiCfg);
                    return false;
                }

            case xmlAccess::MERGE_OTHER:
                //=> return true: change directory selection via drag and drop
                break;
        }


        //disable the sync button
        mainDlg_.syncPreview->enableSynchronization(false);

        //clear grids
        mainDlg_.gridDataView->clearAllRows();
        mainDlg_.updateGuiGrid();

        return true;
    }

private:
    DirectoryNameMainImpl(const DirectoryNameMainImpl&);
    DirectoryNameMainImpl& operator=(const DirectoryNameMainImpl&);

    MainDialog& mainDlg_;
};

//------------------------------------------------------------------
/*    class hierarchy:

           template<>
           FolderPairPanelBasic
                    /|\
                     |
           template<>
           FolderPairCallback   FolderPairGenerated
                    /|\                  /|\
            _________|________    ________|
           |                  |  |
  DirectoryPairFirst      DirectoryPair
*/

template <class GuiPanel>
class FolderPairCallback : public FolderPairPanelBasic<GuiPanel> //implements callback functionality to MainDialog as imposed by FolderPairPanelBasic
{
public:
    FolderPairCallback(GuiPanel& basicPanel, MainDialog& mainDialog) :
        FolderPairPanelBasic<GuiPanel>(basicPanel), //pass FolderPairGenerated part...
        mainDlg(mainDialog) {}

private:
    virtual wxWindow* getParentWindow()
    {
        return &mainDlg;
    }

    virtual MainConfiguration getMainConfig() const
    {
        return mainDlg.getCurrentConfiguration().mainCfg;
    }

    virtual void OnAltSyncCfgChange()
    {
        mainDlg.applySyncConfig();
    }

    virtual void OnAltSyncCfgRemoveConfirm(wxCommandEvent& event)
    {
        FolderPairPanelBasic<GuiPanel>::OnAltSyncCfgRemoveConfirm(event);
        mainDlg.applySyncConfig();
    }

    virtual void OnLocalFilterCfgChange()
    {
        mainDlg.updateFilterConfig();  //re-apply filter
    }

    virtual void OnLocalFilterCfgRemoveConfirm(wxCommandEvent& event)
    {
        FolderPairPanelBasic<GuiPanel>::OnLocalFilterCfgRemoveConfirm(event);
        mainDlg.updateFilterConfig(); //update filter
    }

    MainDialog& mainDlg;
};


class DirectoryPair :
    public FolderPairGenerated, //DirectoryPair "owns" FolderPairGenerated!
    public FolderPairCallback<FolderPairGenerated>
{
public:
    DirectoryPair(wxWindow* parent, MainDialog& mainDialog) :
        FolderPairGenerated(parent),
        FolderPairCallback<FolderPairGenerated>(static_cast<FolderPairGenerated&>(*this), mainDialog), //pass FolderPairGenerated part...
        dirNameLeft (*m_panelLeft,  *m_dirPickerLeft,  *m_directoryLeft),
        dirNameRight(*m_panelRight, *m_dirPickerRight, *m_directoryRight) {}

    void setValues(const wxString& leftDir, const wxString& rightDir, AltSyncCfgPtr syncCfg, const FilterConfig& filter)
    {
        setConfig(syncCfg, filter);
        dirNameLeft.setName(leftDir);
        dirNameRight.setName(rightDir);
    }
    wxString getLeftDir () const { return dirNameLeft .getName(); }
    wxString getRightDir() const { return dirNameRight.getName(); }

private:
    //support for drag and drop
    DirectoryName dirNameLeft;
    DirectoryName dirNameRight;
};


class DirectoryPairFirst : public FolderPairCallback<MainDialogGenerated>
{
public:
    DirectoryPairFirst(MainDialog& mainDialog) :
        FolderPairCallback<MainDialogGenerated>(mainDialog, mainDialog),

        //prepare drag & drop
        dirNameLeft(mainDialog,
                    *mainDialog.m_panelLeft,
                    *mainDialog.m_panelTopLeft,
                    *mainDialog.m_dirPickerLeft,
                    *mainDialog.m_directoryLeft,
                    *mainDialog.sbSizerDirLeft),
        dirNameRight(mainDialog,
                     *mainDialog.m_panelRight,
                     *mainDialog.m_panelTopRight,
                     *mainDialog.m_dirPickerRight,
                     *mainDialog.m_directoryRight,
                     *mainDialog.sbSizerDirRight) {}

    void setValues(const wxString& leftDir, const wxString& rightDir, AltSyncCfgPtr syncCfg, const FilterConfig& filter)
    {
        setConfig(syncCfg, filter);
        dirNameLeft.setName(leftDir);
        dirNameRight.setName(rightDir);
    }
    wxString getLeftDir () const { return dirNameLeft .getName(); }
    wxString getRightDir() const { return dirNameRight.getName(); }

private:
    //support for drag and drop
    DirectoryNameMainImpl dirNameLeft;
    DirectoryNameMainImpl dirNameRight;
};


//workaround for wxWidgets: small hack to update menu items: actually this is a wxWidgets bug (affects Windows- and Linux-build)
class MenuItemUpdater
{
public:
    MenuItemUpdater(wxMenu* menuToUpdate) : menuToUpdate_(menuToUpdate) {}

    ~MenuItemUpdater()
    {
        //start updating menu icons
        const wxMenuItemList& allItems = menuToUpdate_->GetMenuItems();

        //retrieve menu item positions: unfortunately wxMenu doesn't offer a better way
        MenuItemMap::iterator j;
        int index = 0;
        for (wxMenuItemList::const_iterator i = allItems.begin(); i != allItems.end(); ++i, ++index)
            if ((j = menuItems.find(*i)) != menuItems.end())
                j->second = index;

        //finally update items
        for (MenuItemMap::const_iterator i = menuItems.begin(); i != menuItems.end(); ++i)
            if (i->second >= 0)
            {
                menuToUpdate_->Remove(i->first);            //actual workaround
                menuToUpdate_->Insert(i->second, i->first); //
            }
    }

    void addForUpdate(wxMenuItem* newEntry, const wxBitmap& newBitmap)
    {
        newEntry->SetBitmap(newBitmap);
        menuItems.insert(std::pair<wxMenuItem*, int>(newEntry, -1));
    }

private:
    typedef std::map<wxMenuItem*, int> MenuItemMap;
    wxMenu* menuToUpdate_;
    MenuItemMap menuItems;
};


struct DirNotFound
{
    bool operator()(const FolderPairEnh& fp) const
    {
        const Zstring dirFmtLeft  = zen::getFormattedDirectoryName(fp.leftDirectory);
        const Zstring dirFmtRight = zen::getFormattedDirectoryName(fp.rightDirectory);

        if (dirFmtLeft.empty() && dirFmtRight.empty())
            return false;

        return !dirExists(dirFmtLeft) || !dirExists(dirFmtRight);
    }
};


#ifdef FFS_WIN
class PanelMoveWindow : public MouseMoveWindow
{
public:
    PanelMoveWindow(MainDialog& mainDlg) :
        MouseMoveWindow(mainDlg, false), //don't include main dialog itself, thereby prevent various mouse capture lost issues
        mainDlg_(mainDlg) {}

    virtual bool allowMove(const wxMouseEvent& event)
    {
        wxPanel* panel = dynamic_cast<wxPanel*>(event.GetEventObject());

        const wxAuiPaneInfo& paneInfo = mainDlg_.auiMgr.GetPane(panel);
        if (paneInfo.IsOk() &&
            paneInfo.IsFloating())
            return false; //prevent main dialog move

        return true;; //allow dialog move
    }

private:
    MainDialog& mainDlg_;
};
#endif


//##################################################################################################################################
MainDialog::MainDialog(const wxString& cfgFileName, xmlAccess::XmlGlobalSettings& settings) :
    MainDialogGenerated(NULL)
{
    xmlAccess::XmlGuiConfig guiCfg;  //structure to receive gui settings, already defaulted!!

    wxString currentConfigFile = cfgFileName; //this one has priority
    if (currentConfigFile.empty()) currentConfigFile = settings.gui.lastUsedConfigFile; //next: suggest name of last used selection
    if (currentConfigFile.empty() || !fileExists(wxToZ(currentConfigFile))) currentConfigFile = lastRunConfigName(); //if above fails...

    bool loadCfgSuccess = false;
    if (!cfgFileName.empty() || fileExists(wxToZ(currentConfigFile)))
        try
        {
            //load XML
            std::vector<wxString> filenames;
            filenames.push_back(currentConfigFile);

            xmlAccess::convertConfig(filenames, guiCfg); //throw (xmlAccess::XmlError)

            loadCfgSuccess = true;
        }
        catch (const xmlAccess::XmlError& error)
        {
            if (error.getSeverity() == xmlAccess::XmlError::WARNING)
                wxMessageBox(error.msg(), _("Warning"), wxOK | wxICON_WARNING);
            else
                wxMessageBox(error.msg(), _("Error"), wxOK | wxICON_ERROR);
        }
    const bool startComparisonImmediately = loadCfgSuccess && !cfgFileName.empty();

    init(guiCfg,
         settings,
         startComparisonImmediately);

    setLastUsedConfig(currentConfigFile, loadCfgSuccess ? guiCfg : xmlAccess::XmlGuiConfig()); //simulate changed config on parsing errors
}


MainDialog::MainDialog(const xmlAccess::XmlGuiConfig& guiCfg,
                       xmlAccess::XmlGlobalSettings& settings,
                       bool startComparison) :
    MainDialogGenerated(NULL)
{
    init(guiCfg,
         settings,
         startComparison);
}


MainDialog::~MainDialog()
{
    //keep non-inline destructor for std::auto_ptr to work with forward declaration

    cleanUp(true); //do NOT include any other code here! cleanUp() is re-used when switching languages

    auiMgr.UnInit();
}


void MainDialog::init(const xmlAccess::XmlGuiConfig guiCfg,
                      xmlAccess::XmlGlobalSettings& settings,
                      bool startComparison)
{
    wxWindowUpdateLocker dummy(this); //avoid display distortion

    //--------- avoid mirroring this dialog in RTL languages like Hebrew or Arabic --------------------
    m_panelViewFilter    ->SetLayoutDirection(wxLayout_LeftToRight);
    m_panelStatusBar     ->SetLayoutDirection(wxLayout_LeftToRight);
    m_panelGrids         ->SetLayoutDirection(wxLayout_LeftToRight);
    m_panelDirectoryPairs->SetLayoutDirection(wxLayout_LeftToRight);
    //------------------------------------------------------------------------------------------------------

    //---------------- support for dockable gui style --------------------------------
    bSizerPanelHolder->Detach(m_panelTopButtons);
    bSizerPanelHolder->Detach(m_panelDirectoryPairs);
    bSizerPanelHolder->Detach(m_panelGrids);
    bSizerPanelHolder->Detach(m_panelConfig);
    bSizerPanelHolder->Detach(m_panelFilter);
    bSizerPanelHolder->Detach(m_panelViewFilter);
    bSizerPanelHolder->Detach(m_panelStatistics);
    bSizerPanelHolder->Detach(m_panelStatusBar);

    auiMgr.SetManagedWindow(this);
    auiMgr.SetFlags(wxAUI_MGR_DEFAULT | wxAUI_MGR_LIVE_RESIZE);

    //caption required for all panes that can be manipulated by the users => used by context menu
    auiMgr.AddPane(m_panelTopButtons,
                   wxAuiPaneInfo().Name(wxT("Panel1")).Top().Caption(_("Main bar")).CaptionVisible(false).PaneBorder(false).Gripper().MinSize(-1, m_panelTopButtons->GetSize().GetHeight() - 5));
    //note: min height is calculated incorrectly by wxAuiManager if panes with and without caption are in the same row => use smaller min-size

    compareStatus.reset(new CompareStatus(*this)); //integrate the compare status panel (in hidden state)
    auiMgr.AddPane(compareStatus->getAsWindow(),
                   wxAuiPaneInfo().Name(wxT("Panel9")).Top().Row(1).CaptionVisible(false).PaneBorder(false).Hide()); //name "CmpStatus" used by context menu

    auiMgr.AddPane(m_panelDirectoryPairs,
                   wxAuiPaneInfo().Name(wxT("Panel2")).Top().Row(2).Caption(_("Folder pairs")).CaptionVisible(false).PaneBorder(false).Gripper());

    auiMgr.AddPane(m_panelGrids,
                   wxAuiPaneInfo().Name(wxT("Panel3")).CenterPane().PaneBorder(false));

    auiMgr.AddPane(m_panelConfig,
                   wxAuiPaneInfo().Name(wxT("Panel4")).Bottom().Row(1).Position(0).Caption(_("Configuration")).MinSize(m_listBoxHistory->GetSize().GetWidth(), m_panelConfig->GetSize().GetHeight()));

    auiMgr.AddPane(m_panelFilter,
                   wxAuiPaneInfo().Name(wxT("Panel5")).Bottom().Row(1).Position(1).Caption(_("Filter files")).MinSize(-1, m_panelFilter->GetSize().GetHeight()));

    auiMgr.AddPane(m_panelViewFilter,
                   wxAuiPaneInfo().Name(wxT("Panel6")).Bottom().Row(1).Position(2).Caption(_("Select view")).MinSize(m_bpButtonSyncDirNone->GetSize().GetWidth(), m_panelViewFilter->GetSize().GetHeight()));

    auiMgr.AddPane(m_panelStatistics,
                   wxAuiPaneInfo().Name(wxT("Panel7")).Bottom().Row(1).Position(3).Caption(_("Statistics")).MinSize(m_panelStatistics->GetSize().GetWidth() / 2, m_panelStatistics->GetSize().GetHeight()));

    auiMgr.AddPane(m_panelStatusBar,
                   wxAuiPaneInfo().Name(wxT("Panel8")).Bottom().Row(0).CaptionVisible(false).PaneBorder(false).DockFixed());

    auiMgr.Update();

    defaultPerspective = auiMgr.SavePerspective();
    //----------------------------------------------------------------------------------
    //register view layout context menu
    m_panelTopButtons->Connect(wxEVT_RIGHT_DOWN, wxMouseEventHandler(MainDialog::OnContextSetLayout), NULL, this);
    m_panelConfig    ->Connect(wxEVT_RIGHT_DOWN, wxMouseEventHandler(MainDialog::OnContextSetLayout), NULL, this);
    m_panelFilter    ->Connect(wxEVT_RIGHT_DOWN, wxMouseEventHandler(MainDialog::OnContextSetLayout), NULL, this);
    m_panelViewFilter->Connect(wxEVT_RIGHT_DOWN, wxMouseEventHandler(MainDialog::OnContextSetLayout), NULL, this);
    m_panelStatistics->Connect(wxEVT_RIGHT_DOWN, wxMouseEventHandler(MainDialog::OnContextSetLayout), NULL, this);
    m_panelStatusBar ->Connect(wxEVT_RIGHT_DOWN, wxMouseEventHandler(MainDialog::OnContextSetLayout), NULL, this);
    //----------------------------------------------------------------------------------

    globalSettings = &settings;
    gridDataView.reset(new zen::GridView);
    contextMenu.reset(new wxMenu); //initialize right-click context menu; will be dynamically re-created on each R-mouse-click

    cleanedUp = false;
    processingGlobalKeyEvent = false;
    lastSortColumn = -1;
    lastSortGrid = NULL;

    updateFileIcons.reset(new IconUpdater(m_gridLeft, m_gridRight));

#ifdef FFS_WIN
    new PanelMoveWindow(*this); //allow moving main dialog by clicking (nearly) anywhere... //ownership passed to "this"
#endif

    syncPreview.reset(new SyncPreview(this));

    SetTitle(wxString(wxT("FreeFileSync - ")) + _("Folder Comparison and Synchronization"));

    SetIcon(*GlobalResources::instance().programIcon); //set application icon

    //notify about (logical) application main window => program won't quit, but stay on this dialog
    zen::AppMainWindow::setMainWindow(this);

    //init handling of first folder pair
    firstFolderPair.reset(new DirectoryPairFirst(*this));

    initViewFilterButtons();

    //initialize and load configuration
    readGlobalSettings();
    setCurrentConfiguration(guiCfg);

    //set icons for this dialog
    m_buttonCompare->setBitmapFront(GlobalResources::instance().getImage(wxT("compare")));
    m_bpButtonSyncConfig->SetBitmapLabel(GlobalResources::instance().getImage(wxT("syncConfig")));
    m_bpButtonCmpConfig->SetBitmapLabel(GlobalResources::instance().getImage(wxT("cmpConfig")));
    m_bpButtonSave->SetBitmapLabel(GlobalResources::instance().getImage(wxT("save")));
    m_bpButtonLoad->SetBitmapLabel(GlobalResources::instance().getImage(wxT("load")));
    m_bpButtonAddPair->SetBitmapLabel(GlobalResources::instance().getImage(wxT("addFolderPair")));
    m_bitmap15->SetBitmap(GlobalResources::instance().getImage(wxT("statusEdge")));

    m_bitmapCreate->SetBitmap(GlobalResources::instance().getImage(wxT("create")));
    m_bitmapUpdate->SetBitmap(GlobalResources::instance().getImage(wxT("update")));
    m_bitmapDelete->SetBitmap(GlobalResources::instance().getImage(wxT("delete")));
    m_bitmapData  ->SetBitmap(GlobalResources::instance().getImage(wxT("data")));

    m_panelTopButtons->Layout(); //wxButtonWithImage size might have changed

    //menu icons: workaround for wxWidgets: small hack to update menu items: actually this is a wxWidgets bug (affects Windows- and Linux-build)
    MenuItemUpdater updateMenuFile(m_menuFile);
    updateMenuFile.addForUpdate(m_menuItem10,   GlobalResources::instance().getImage(wxT("compareSmall")));
    updateMenuFile.addForUpdate(m_menuItem11,   GlobalResources::instance().getImage(wxT("syncSmall")));
    updateMenuFile.addForUpdate(m_menuItemNew,  GlobalResources::instance().getImage(wxT("newSmall")));
    updateMenuFile.addForUpdate(m_menuItemSave, GlobalResources::instance().getImage(wxT("saveSmall")));
    updateMenuFile.addForUpdate(m_menuItemLoad, GlobalResources::instance().getImage(wxT("loadSmall")));

    MenuItemUpdater updateMenuAdv(m_menuAdvanced);
    updateMenuAdv.addForUpdate(m_menuItemGlobSett, GlobalResources::instance().getImage(wxT("settingsSmall")));
    updateMenuAdv.addForUpdate(m_menuItem7, GlobalResources::instance().getImage(wxT("batchSmall")));

    MenuItemUpdater updateMenuHelp(m_menuHelp);
    updateMenuHelp.addForUpdate(m_menuItemAbout, GlobalResources::instance().getImage(wxT("aboutSmall")));

#ifdef FFS_LINUX
    if (!zen::isPortableVersion()) //disable update check for Linux installer-based version -> handled by .deb
        m_menuItemCheckVer->Enable(false);
#endif

    //create language selection menu
    for (std::vector<ExistingTranslations::Entry>::const_iterator i = ExistingTranslations::get().begin(); i != ExistingTranslations::get().end(); ++i)
    {
        wxMenuItem* newItem = new wxMenuItem(m_menuLanguages, wxID_ANY, i->languageName, wxEmptyString, wxITEM_NORMAL );
        newItem->SetBitmap(GlobalResources::instance().getImage(i->languageFlag));

        //map menu item IDs with language IDs: evaluated when processing event handler
        languageMenuItemMap.insert(std::map<MenuItemID, LanguageID>::value_type(newItem->GetId(), i->languageID));

        //connect event
        Connect(newItem->GetId(), wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(MainDialog::OnMenuLanguageSwitch));
        m_menuLanguages->Append(newItem);
    }

    //support for CTRL + C and DEL on grids
    m_gridLeft  ->Connect(wxEVT_KEY_DOWN, wxKeyEventHandler(MainDialog::onGridLeftButtonEvent),   NULL, this);
    m_gridRight ->Connect(wxEVT_KEY_DOWN, wxKeyEventHandler(MainDialog::onGridRightButtonEvent),  NULL, this);
    m_gridMiddle->Connect(wxEVT_KEY_DOWN, wxKeyEventHandler(MainDialog::onGridMiddleButtonEvent), NULL, this);

    //register global hotkeys (without explicit menu entry)
    wxTheApp->Connect(wxEVT_KEY_DOWN, wxKeyEventHandler(MainDialog::OnGlobalKeyEvent), NULL, this);
    wxTheApp->Connect(wxEVT_CHAR_HOOK, wxKeyEventHandler(MainDialog::OnGlobalKeyEvent), NULL, this); //capture direction keys


    Connect(wxEVT_IDLE, wxEventHandler(MainDialog::OnIdleEvent), NULL, this);

    Connect(wxEVT_SIZE, wxSizeEventHandler(MainDialog::OnResize), NULL, this);
    Connect(wxEVT_MOVE, wxSizeEventHandler(MainDialog::OnResize), NULL, this);

    m_bpButtonFilter->Connect(wxEVT_RIGHT_DOWN, wxCommandEventHandler(MainDialog::OnGlobalFilterOpenContext), NULL, this);

    //calculate witdh of folder pair manually (if scrollbars are visible)
    m_panelTopLeft->Connect(wxEVT_SIZE, wxEventHandler(MainDialog::OnResizeFolderPairs), NULL, this);

    //dynamically change sizer direction depending on size
    //m_panelTopButtons->Connect(wxEVT_SIZE, wxEventHandler(MainDialog::OnResizeTopButtons),      NULL, this);
    m_panelConfig    ->Connect(wxEVT_SIZE, wxEventHandler(MainDialog::OnResizeConfigPanel),     NULL, this);
    m_panelViewFilter->Connect(wxEVT_SIZE, wxEventHandler(MainDialog::OnResizeViewPanel),       NULL, this);
    m_panelStatistics->Connect(wxEVT_SIZE, wxEventHandler(MainDialog::OnResizeStatisticsPanel), NULL, this);
    wxSizeEvent dummy3;
    //OnResizeTopButtons     (dummy3); //call once on window creation
    OnResizeConfigPanel    (dummy3); //
    OnResizeViewPanel      (dummy3); //
    OnResizeStatisticsPanel(dummy3); //

    //event handler for manual (un-)checking of rows and setting of sync direction
    m_gridMiddle->Connect(FFS_CHECK_ROWS_EVENT, FFSCheckRowsEventHandler(MainDialog::OnCheckRows), NULL, this);
    m_gridMiddle->Connect(FFS_SYNC_DIRECTION_EVENT, FFSSyncDirectionEventHandler(MainDialog::OnSetSyncDirection), NULL, this);

    //init grid settings
    m_gridLeft  ->initSettings(m_gridLeft, m_gridMiddle, m_gridRight, gridDataView.get());
    m_gridMiddle->initSettings(m_gridLeft, m_gridMiddle, m_gridRight, gridDataView.get());
    m_gridRight ->initSettings(m_gridLeft, m_gridMiddle, m_gridRight, gridDataView.get());

    //disable sync button as long as "compare" hasn't been triggered.
    syncPreview->enableSynchronization(false);

    //mainly to update row label sizes...
    updateGuiGrid();

    //register regular check for update on next idle event
    Connect(wxEVT_IDLE, wxIdleEventHandler(MainDialog::OnRegularUpdateCheck), NULL, this);

    //asynchronous call to wxWindow::Layout(): fix superfluous frame on right and bottom when FFS is started in fullscreen mode
    Connect(wxEVT_IDLE, wxIdleEventHandler(MainDialog::OnLayoutWindowAsync), NULL, this);

    //----------------------------------------------------------------------------------------------------------------------------------------------------------------
    //some convenience: if FFS is started with a *.ffs_gui file as commandline parameter AND all directories contained exist, comparison shall be started right off
    if (startComparison)
    {
        const zen::MainConfiguration currMainCfg = getCurrentConfiguration().mainCfg;
        const bool allFoldersExist = !DirNotFound()(currMainCfg.firstPair) &&
                                     std::find_if(currMainCfg.additionalPairs.begin(), currMainCfg.additionalPairs.end(),
                                                  DirNotFound()) == currMainCfg.additionalPairs.end();
        if (allFoldersExist)
        {
            wxCommandEvent dummy2(wxEVT_COMMAND_BUTTON_CLICKED);
            m_buttonCompare->GetEventHandler()->AddPendingEvent(dummy2); //simulate button click on "compare"
        }
    }
    //----------------------------------------------------------------------------------------------------------------------------------------------------------------

    addFileToCfgHistory(lastRunConfigName()); //make sure <Last session> is always part of history list
}


void MainDialog::cleanUp(bool saveLastUsedConfig)
{
    if (!cleanedUp)
    {
        cleanedUp = true;

        //important! event source wxTheApp is NOT dependent on this instance -> disconnect!
        wxTheApp->Disconnect(wxEVT_KEY_DOWN,  wxKeyEventHandler(MainDialog::OnGlobalKeyEvent), NULL, this);
        wxTheApp->Disconnect(wxEVT_CHAR_HOOK, wxKeyEventHandler(MainDialog::OnGlobalKeyEvent), NULL, this);


        //no need for wxEventHandler::Disconnect() here; event sources are components of this window and are destroyed, too

        m_gridLeft  ->release(); //handle wxGrid-related callback on grid data after MainDialog has died... (Linux only)
        m_gridMiddle->release();
        m_gridRight ->release();

        writeGlobalSettings(); //set before saving last used config since this will reset "currentConfigFileName"

        //save configuration
        if (saveLastUsedConfig)
            writeConfigurationToXml(lastRunConfigName());   //don't throw exceptions in destructors
    }
}


void MainDialog::readGlobalSettings()
{
    //apply window size and position at program startup ONLY
    widthNotMaximized  = globalSettings->gui.widthNotMaximized;
    heightNotMaximized = globalSettings->gui.heightNotMaximized;
    posXNotMaximized   = globalSettings->gui.posXNotMaximized;
    posYNotMaximized   = globalSettings->gui.posYNotMaximized;

    //apply window size and position
    if (widthNotMaximized  != wxDefaultCoord &&
        heightNotMaximized != wxDefaultCoord &&
        posXNotMaximized   != wxDefaultCoord &&
        posYNotMaximized   != wxDefaultCoord &&
        wxDisplay::GetFromPoint(wxPoint(posXNotMaximized, posYNotMaximized)) != wxNOT_FOUND) //make sure upper left corner is in visible view
        SetSize(posXNotMaximized, posYNotMaximized, widthNotMaximized, heightNotMaximized);
    else
        Centre();

    Maximize(globalSettings->gui.isMaximized);


    //set column attributes
    m_gridLeft->setColumnAttributes(globalSettings->gui.columnAttribLeft);
    m_gridRight->setColumnAttributes(globalSettings->gui.columnAttribRight);

    //load list of last used configuration files (in reverse order)
    std::for_each(globalSettings->gui.cfgFileHistory.rbegin(), globalSettings->gui.cfgFileHistory.rend(),
                  boost::bind(&MainDialog::addFileToCfgHistory, this, _1));

    //load list of last used folders
    std::for_each(globalSettings->gui.folderHistoryLeft.rbegin(),
                  globalSettings->gui.folderHistoryLeft.rend(),
                  boost::bind(&MainDialog::addLeftFolderToHistory, this, _1));

    std::for_each(globalSettings->gui.folderHistoryRight.rbegin(),
                  globalSettings->gui.folderHistoryRight.rend(),
                  boost::bind(&MainDialog::addRightFolderToHistory, this, _1));

    //show/hide file icons
    m_gridLeft->enableFileIcons(globalSettings->gui.showFileIconsLeft);
    m_gridRight->enableFileIcons(globalSettings->gui.showFileIconsRight);

    //------------------------------------------------------------------------------------------------
    //wxAuiManager erroneously loads panel captions, we don't want that
    typedef std::vector<std::pair<wxString, wxString> > CaptionNameMapping;
    CaptionNameMapping captionNameMap;
    const wxAuiPaneInfoArray& paneArray = auiMgr.GetAllPanes();
    for (size_t i = 0; i < paneArray.size(); ++i)
        captionNameMap.push_back(std::make_pair(paneArray[i].caption, paneArray[i].name));

    auiMgr.LoadPerspective(globalSettings->gui.guiPerspectiveLast);

    //restore original captions
    for (CaptionNameMapping::const_iterator i = captionNameMap.begin(); i != captionNameMap.end(); ++i)
        auiMgr.GetPane(i->second).Caption(i->first);
    auiMgr.Update();
}


void MainDialog::writeGlobalSettings()
{
    //write global settings to (global) variable stored in application instance
    globalSettings->gui.widthNotMaximized  = widthNotMaximized;
    globalSettings->gui.heightNotMaximized = heightNotMaximized;
    globalSettings->gui.posXNotMaximized   = posXNotMaximized;
    globalSettings->gui.posYNotMaximized   = posYNotMaximized;
    globalSettings->gui.isMaximized        = IsMaximized();

    //retrieve column attributes
    globalSettings->gui.columnAttribLeft  = m_gridLeft->getColumnAttributes();
    globalSettings->gui.columnAttribRight = m_gridRight->getColumnAttributes();

    //write list of last used configuration files
    std::vector<wxString> cfgFileHistory;
    for (int i = 0; i < static_cast<int>(m_listBoxHistory->GetCount()); ++i)
        if (m_listBoxHistory->GetClientObject(i))
            cfgFileHistory.push_back(static_cast<wxClientDataString*>(m_listBoxHistory->GetClientObject(i))->name_);

    globalSettings->gui.cfgFileHistory     = cfgFileHistory;
    globalSettings->gui.lastUsedConfigFile = currentConfigFileName;

    //write list of last used folders
    globalSettings->gui.folderHistoryLeft.clear();
    const wxArrayString leftFolderHistory = m_directoryLeft->GetStrings();
    for (unsigned i = 0; i < leftFolderHistory.GetCount(); ++i)
        globalSettings->gui.folderHistoryLeft.push_back(leftFolderHistory[i]);

    globalSettings->gui.folderHistoryRight.clear();
    const wxArrayString rightFolderHistory = m_directoryRight->GetStrings();
    for (unsigned i = 0; i < rightFolderHistory.GetCount(); ++i)
        globalSettings->gui.folderHistoryRight.push_back(rightFolderHistory[i]);

    globalSettings->gui.guiPerspectiveLast = auiMgr.SavePerspective();
}


void MainDialog::setSyncDirManually(const std::set<size_t>& rowsToSetOnUiTable, const zen::SyncDirection dir)
{
    if (rowsToSetOnUiTable.size() > 0)
    {
        for (std::set<size_t>::const_iterator i = rowsToSetOnUiTable.begin(); i != rowsToSetOnUiTable.end(); ++i)
        {
            FileSystemObject* fsObj = gridDataView->getObject(*i);
            if (fsObj)
            {
                setSyncDirectionRec(dir, *fsObj); //set new direction (recursively)
                zen::setActiveStatus(true, *fsObj); //works recursively for directories
            }
        }

        updateGuiGrid();
    }
}


void MainDialog::filterRangeManually(const std::set<size_t>& rowsToFilterOnUiTable, int leadingRow)
{
    if (!rowsToFilterOnUiTable.empty())
    {
        bool newSelection = false; //default: deselect range

        //leadingRow determines de-/selection of all other rows
        const FileSystemObject* fsObj = gridDataView->getObject(leadingRow);
        if (!fsObj) fsObj = gridDataView->getObject(*rowsToFilterOnUiTable.begin()); //some fallback (usecase: reverse selection, starting with empty rows)
        if (fsObj)
            newSelection = !fsObj->isActive();

        //if hidefiltered is active, there should be no filtered elements on screen => current element was filtered out
        assert(!currentCfg.hideFilteredElements || !newSelection);

        //get all lines that need to be filtered
        std::vector<FileSystemObject*> compRef;
        gridDataView->getAllFileRef(rowsToFilterOnUiTable, compRef); //everything in compRef is bound

        for (std::vector<FileSystemObject*>::iterator i = compRef.begin(); i != compRef.end(); ++i)
            zen::setActiveStatus(newSelection, **i); //works recursively for directories

        refreshGridAfterFilterChange(400); //call this instead of updateGuiGrid() to add some delay if hideFiltered == true and to handle some graphical artifacts
    }
}


void MainDialog::OnIdleEvent(wxEvent& event)
{
    //small routine to restore status information after some time
    if (stackObjects.size() > 0 )  //check if there is some work to do
    {
        wxMilliClock_t currentTime = wxGetLocalTimeMillis();
        if (currentTime - lastStatusChange > 2500) //restore stackObject after two seconds
        {
            lastStatusChange = currentTime;

            m_staticTextStatusMiddle->SetLabel(stackObjects.top());
            stackObjects.pop();

            if (stackObjects.empty())
                m_staticTextStatusMiddle->SetForegroundColour(wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOWTEXT)); //reset color

            m_panelStatusBar->Layout();
        }
    }

    event.Skip();
}


void MainDialog::copySelectionToClipboard(CustomGrid& selectedGrid)
{
    const std::set<size_t> selectedRows = getSelectedRows(&selectedGrid);
    if (selectedRows.size() > 0)
    {
        zxString clipboardString; //perf: wxString doesn't model exponential growth and so is out

        const int colCount = selectedGrid.GetNumberCols();

        for (std::set<size_t>::const_iterator i = selectedRows.begin(); i != selectedRows.end(); ++i)
        {
            for (int k = 0; k < colCount; ++k)
            {
                clipboardString += wxToZx(selectedGrid.GetCellValue(static_cast<int>(*i), k));
                if (k != colCount - 1)
                    clipboardString += wxT('\t');
            }
            clipboardString += wxT('\n');
        }

        if (!clipboardString.empty())
            // Write text to the clipboard
            if (wxTheClipboard->Open())
            {
                // these data objects are held by the clipboard,
                // so do not delete them in the app.
                wxTheClipboard->SetData(new wxTextDataObject(zxToWx(clipboardString)));
                wxTheClipboard->Close();
            }
    }
}


std::set<size_t> MainDialog::getSelectedRows(const CustomGrid* grid) const
{
    std::set<size_t> output = grid->getAllSelectedRows();

    //remove invalid rows
    output.erase(output.lower_bound(gridDataView->rowsOnView()), output.end());

    return output;
}


std::set<size_t> MainDialog::getSelectedRows() const
{
    //merge selections from left and right grid
    std::set<size_t> selection  = getSelectedRows(m_gridLeft);
    std::set<size_t> additional = getSelectedRows(m_gridRight);
    selection.insert(additional.begin(), additional.end());
    return selection;
}


class ManualDeletionHandler : private wxEvtHandler, public DeleteFilesHandler
{
public:
    ManualDeletionHandler(MainDialog* main) :
        mainDlg(main),
        abortRequested(false),
        ignoreErrors(false),
        deletionCount(0)
    {
        mainDlg->disableAllElements(true); //disable everything except abort button
        mainDlg->clearStatusBar();

        //register abort button
        mainDlg->m_buttonAbort->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( ManualDeletionHandler::OnAbortCompare ), NULL, this );
        mainDlg->Connect(wxEVT_CHAR_HOOK, wxKeyEventHandler(ManualDeletionHandler::OnKeyPressed), NULL, this);
    }

    ~ManualDeletionHandler()
    {
        //de-register abort button
        mainDlg->Disconnect(wxEVT_CHAR_HOOK, wxKeyEventHandler(ManualDeletionHandler::OnKeyPressed), NULL, this);
        mainDlg->m_buttonAbort->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( ManualDeletionHandler::OnAbortCompare ), NULL, this );

        mainDlg->enableAllElements();
    }

    virtual Response reportError(const wxString& errorMessage)
    {
        if (abortRequested)
            throw zen::AbortThisProcess();

        if (ignoreErrors)
            return DeleteFilesHandler::IGNORE_ERROR;

        bool ignoreNextErrors = false;
        switch (showErrorDlg(ReturnErrorDlg::BUTTON_IGNORE | ReturnErrorDlg::BUTTON_RETRY | ReturnErrorDlg::BUTTON_ABORT,
                             errorMessage, ignoreNextErrors))
        {
            case ReturnErrorDlg::BUTTON_IGNORE:
                ignoreErrors = ignoreNextErrors;
                return DeleteFilesHandler::IGNORE_ERROR;
            case ReturnErrorDlg::BUTTON_RETRY:
                return DeleteFilesHandler::RETRY;
            case ReturnErrorDlg::BUTTON_ABORT:
                throw zen::AbortThisProcess();
        }

        assert (false);
        return DeleteFilesHandler::IGNORE_ERROR; //dummy return value
    }

    virtual void notifyDeletion(const Zstring& currentObject)  //called for each file/folder that has been deleted
    {
        ++deletionCount;

        if (updateUiIsAllowed())  //test if specific time span between ui updates is over
        {
            wxString statusMessage = _P("Object deleted successfully!", "%x objects deleted successfully!", deletionCount);
            statusMessage.Replace(wxT("%x"), zen::toStringSep(deletionCount), false);

            if (mainDlg->m_staticTextStatusMiddle->GetLabel() != statusMessage)
            {
                mainDlg->m_staticTextStatusMiddle->SetLabel(statusMessage);
                mainDlg->m_panelStatusBar->Layout();
            }
            updateUiNow();
        }

        if (abortRequested) //test after (implicit) call to wxApp::Yield()
            throw zen::AbortThisProcess();
    }

private:
    void OnAbortCompare(wxCommandEvent& event) //handle abort button click
    {
        abortRequested = true; //don't throw exceptions in a GUI-Callback!!! (throw zen::AbortThisProcess())
    }

    void OnKeyPressed(wxKeyEvent& event)
    {
        const int keyCode = event.GetKeyCode();
        if (keyCode == WXK_ESCAPE)
            abortRequested = true; //don't throw exceptions in a GUI-Callback!!! (throw zen::AbortThisProcess())

        event.Skip();
    }


    MainDialog* const mainDlg;

    bool abortRequested;
    bool ignoreErrors;
    size_t deletionCount;
};


void MainDialog::deleteSelectedFiles(const std::set<size_t>& viewSelectionLeft, const std::set<size_t>& viewSelectionRight)
{
    if (viewSelectionLeft.size() + viewSelectionRight.size())
    {
        //map lines from GUI view to grid line references
        std::vector<FileSystemObject*> compRefLeft;
        gridDataView->getAllFileRef(viewSelectionLeft, compRefLeft);

        std::vector<FileSystemObject*> compRefRight;
        gridDataView->getAllFileRef(viewSelectionRight, compRefRight);


        wxWindow* oldFocus = wxWindow::FindFocus();

        if (zen::showDeleteDialog(compRefLeft,
                                  compRefRight,
                                  globalSettings->gui.deleteOnBothSides,
                                  globalSettings->gui.useRecyclerForManualDeletion) == ReturnSmallDlg::BUTTON_OKAY)
        {
            if (globalSettings->gui.useRecyclerForManualDeletion && !zen::recycleBinExists())
            {
                wxMessageBox(_("Recycle Bin not yet supported for this system!"));
                return;
            }

            try
            {
                //handle errors when deleting files/folders
                ManualDeletionHandler statusHandler(this);

                zen::deleteFromGridAndHD(gridDataView->getDataTentative(),
                                         compRefLeft,
                                         compRefRight,
                                         globalSettings->gui.deleteOnBothSides,
                                         globalSettings->gui.useRecyclerForManualDeletion,
                                         statusHandler);
            }
            catch (zen::AbortThisProcess&) {}

            //remove rows that are empty: just a beautification, invalid rows shouldn't cause issues
            gridDataView->removeInvalidRows();

            //redraw grid neccessary to update new dimensions and for UI-Backend data linkage
            updateGuiGrid(); //call immediately after deleteFromGridAndHD!!!

            m_gridLeft->  ClearSelection();
            m_gridMiddle->ClearSelection();
            m_gridRight-> ClearSelection();
        }

        if (oldFocus)
            oldFocus->SetFocus(); //restore focus before deletion
    }
}


template <SelectedSide side>
void exstractNames(const FileSystemObject& fsObj, wxString& name, wxString& dir)
{
    if (!fsObj.isEmpty<side>())
    {
        struct GetNames : public FSObjectVisitor
        {
            GetNames(wxString& nameIn, wxString& dirIn) : name_(nameIn), dir_(dirIn) {}
            virtual void visit(const FileMapping& fileObj)
            {
                name_ = zToWx(fileObj.getFullName<side>());
                dir_  = zToWx(fileObj.getFullName<side>().BeforeLast(common::FILE_NAME_SEPARATOR));
            }
            virtual void visit(const SymLinkMapping& linkObj)
            {
                name_ = zToWx(linkObj.getFullName<side>());
                dir_  = zToWx(linkObj.getFullName<side>().BeforeLast(common::FILE_NAME_SEPARATOR));
            }
            virtual void visit(const DirMapping& dirObj)
            {
                dir_  = name_ = zToWx(dirObj.getFullName<side>());
            }

            wxString& name_;
            wxString& dir_;
            ;
        } getNames(name, dir);
        fsObj.accept(getNames);
    }
    else
    {
        name.clear();
        dir.clear();
    }
}


void MainDialog::openExternalApplication(const wxString& commandline)
{
    if (m_gridLeft->isLeadGrid() || m_gridRight->isLeadGrid())
    {
        const CustomGrid* leadGrid = m_gridLeft->isLeadGrid() ?
                                     static_cast<CustomGrid*>(m_gridLeft) :
                                     static_cast<CustomGrid*>(m_gridRight);
        std::set<size_t> selection = getSelectedRows(leadGrid);

        if (selection.size() == 1)
            openExternalApplication(*selection.begin(), m_gridLeft->isLeadGrid(), commandline);
    }
}


void MainDialog::openExternalApplication(size_t rowNumber, bool leftSide, const wxString& commandline)
{
    if (commandline.empty())
        return;

    wxString command = commandline;

    wxString name;
    wxString dir;
    wxString nameCo;
    wxString dirCo;

    const FileSystemObject* fsObj = gridDataView->getObject(rowNumber);
    if (fsObj)
    {
        if (leftSide)
        {
            exstractNames<LEFT_SIDE>( *fsObj, name,   dir);
            exstractNames<RIGHT_SIDE>(*fsObj, nameCo, dirCo);
        }
        else
        {
            exstractNames<RIGHT_SIDE>(*fsObj, name,   dir);
            exstractNames<LEFT_SIDE>( *fsObj, nameCo, dirCo);
        }
#ifdef FFS_WIN
        if (name.empty())
        {
            if (leftSide)
                zen::shellExecute(wxString(wxT("explorer ")) + L"\"" + zToWx(fsObj->getBaseDirPf<LEFT_SIDE>()) + L"\"");
            else
                zen::shellExecute(wxString(wxT("explorer ")) + L"\"" + zToWx(fsObj->getBaseDirPf<RIGHT_SIDE>()) + L"\"");
            return;
        }
#endif
    }
    else
    {
        //fallback
        dir   = zToWx(zen::getFormattedDirectoryName(wxToZ(firstFolderPair->getLeftDir())));
        dirCo = zToWx(zen::getFormattedDirectoryName(wxToZ(firstFolderPair->getRightDir())));

        if (!leftSide)
            std::swap(dir, dirCo);

#ifdef FFS_WIN
        zen::shellExecute(wxString(wxT("explorer ")) + L"\"" + dir + L"\""); //default
        return;
#endif
    }

    command.Replace(wxT("%nameCo"), nameCo, true); //attention: replace %nameCo, %dirCo BEFORE %name, %dir to handle dependency
    command.Replace(wxT("%dirCo"),  dirCo,  true);
    command.Replace(wxT("%name"),   name,   true);
    command.Replace(wxT("%dir"),    dir,    true);

    zen::shellExecute(command);
}


void MainDialog::pushStatusInformation(const wxString& text)
{
    lastStatusChange = wxGetLocalTimeMillis();
    stackObjects.push(m_staticTextStatusMiddle->GetLabel());
    m_staticTextStatusMiddle->SetLabel(text);
    m_staticTextStatusMiddle->SetForegroundColour(wxColour(31, 57, 226)); //highlight color: blue
    m_panelStatusBar->Layout();
}


void MainDialog::clearStatusBar()
{
    while (stackObjects.size() > 0)
        stackObjects.pop();

    m_staticTextStatusMiddle->SetForegroundColour(wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOWTEXT)); //reset color
    m_staticTextStatusLeft  ->SetLabel(wxEmptyString);
    m_staticTextStatusMiddle->SetLabel(wxEmptyString);
    m_staticTextStatusRight ->SetLabel(wxEmptyString);
}


void MainDialog::disableAllElements(bool enableAbort)
{
    //disables all elements (except abort button) that might receive user input during long-running processes: comparison, deletion
    m_panelViewFilter    ->Disable();
    m_bpButtonCmpConfig  ->Disable();
    m_panelFilter        ->Disable();
    m_panelConfig        ->Disable();
    m_bpButtonSyncConfig ->Disable();
    m_buttonStartSync    ->Disable();
    m_panelGrids         ->Disable();
    m_panelDirectoryPairs->Disable();
    m_menubar1->EnableTop(0, false);
    m_menubar1->EnableTop(1, false);
    m_menubar1->EnableTop(2, false);

    if (enableAbort)
    {

        //show abort button
        m_buttonAbort->Enable();
        m_buttonAbort->Show();
        m_buttonAbort->SetFocus();
        m_buttonCompare->Disable();
        m_buttonCompare->Hide();
        m_bpButtonCmpConfig ->Disable();
        m_bpButtonSyncConfig->Disable();
        m_buttonStartSync   ->Disable();
        m_panelTopButtons->Layout();
    }
    else
        m_panelTopButtons->Disable();
}


void MainDialog::enableAllElements()
{
    m_panelViewFilter    ->Enable();
    m_bpButtonCmpConfig  ->Enable();
    m_panelFilter        ->Enable();
    m_panelConfig        ->Enable();
    m_bpButtonSyncConfig ->Enable();
    m_buttonStartSync    ->Enable();
    m_panelGrids         ->Enable();
    m_panelDirectoryPairs->Enable();
    m_menubar1->EnableTop(0, true);
    m_menubar1->EnableTop(1, true);
    m_menubar1->EnableTop(2, true);

    //show compare button
    m_buttonAbort->Disable();
    m_buttonAbort->Hide();
    m_buttonCompare->Enable();
    m_buttonCompare->Show();
    m_bpButtonCmpConfig ->Enable();
    m_bpButtonSyncConfig->Enable();
    m_buttonStartSync   ->Enable();

    m_panelTopButtons->Layout();
    m_panelTopButtons->Enable();
}


void MainDialog::OnResize(wxSizeEvent& event)
{
    if (!IsMaximized())
    {
        int width  = 0;
        int height = 0;
        int x      = 0;
        int y      = 0;

        GetSize(&width, &height);
        GetPosition(&x, &y);

        //test ALL parameters at once, since width/height are invalid if the window is minimized (eg x,y == -32000; height = 28, width = 160)
        //note: negative values for x and y are possible when using multiple monitors!
        if (width > 0 && height > 0 && x >= -3360 && y >= -200 &&
            wxDisplay::GetFromPoint(wxPoint(x, y)) != wxNOT_FOUND) //make sure upper left corner is in visible view
        {
            widthNotMaximized  = width;                        //visible coordinates x < 0 and y < 0 are possible with dual monitors!
            heightNotMaximized = height;

            posXNotMaximized = x;
            posYNotMaximized = y;
        }
    }

    event.Skip();
}


namespace
{
void updateSizerOrientation(wxBoxSizer& sizer, wxWindow& window)
{
    if (window.GetSize().GetWidth() > window.GetSize().GetHeight()) //check window NOT sizer width!
    {
        if (sizer.GetOrientation() != wxHORIZONTAL)
        {
            sizer.SetOrientation(wxHORIZONTAL);
            window.Layout();
        }
    }
    else
    {
        if (sizer.GetOrientation() != wxVERTICAL)
        {
            sizer.SetOrientation(wxVERTICAL);
            window.Layout();
        }
    }
}
}


/*void MainDialog::OnResizeTopButtons(wxEvent& event)
{
    updateSizerOrientation(*bSizerTopButtons, *m_panelTopButtons);
    event.Skip();
}*/

void MainDialog::OnResizeConfigPanel(wxEvent& event)
{
    updateSizerOrientation(*bSizerConfig, *m_panelConfig);
    event.Skip();
}

void MainDialog::OnResizeViewPanel(wxEvent& event)
{
    updateSizerOrientation(*bSizerViewFilter, *m_panelViewFilter);
    event.Skip();
}

void MainDialog::OnResizeStatisticsPanel(wxEvent& event)
{
    updateSizerOrientation(*bSizerStatistics, *m_panelStatistics);
    event.Skip();
}


void MainDialog::OnResizeFolderPairs(wxEvent& event)
{
    //adapt left-shift display distortion caused by scrollbars for multiple folder pairs
    const int width = m_panelTopLeft->GetSize().GetWidth();
    for (std::vector<DirectoryPair*>::iterator i = additionalFolderPairs.begin(); i != additionalFolderPairs.end(); ++i)
        (*i)->m_panelLeft->SetMinSize(wxSize(width, -1));

    event.Skip();
}


void MainDialog::onGridLeftButtonEvent(wxKeyEvent& event)
{
    const int keyCode = event.GetKeyCode();

    if (event.ControlDown())
        switch (keyCode)
        {
            case 'C':
            case WXK_INSERT: //CTRL + C || CTRL + INS
                copySelectionToClipboard(*m_gridLeft);
                return; // -> swallow event! don't allow default grid commands!

            case 'A': //CTRL + A
                m_gridLeft->SelectAll();
                return;

            case WXK_NUMPAD_ADD: //CTRL + '+'
                m_gridLeft->autoSizeColumns();
                return;
        }

    else if (event.AltDown())
        switch (keyCode)
        {
            case WXK_LEFT: //ALT + <-
            {
                wxCommandEvent dummy;
                OnContextSyncDirLeft(dummy);
            }
            return;

            case WXK_RIGHT: //ALT + ->
            {
                wxCommandEvent dummy;
                OnContextSyncDirRight(dummy);
            }
            return;

            case WXK_UP:   /* ALT + /|\   */
            case WXK_DOWN: /* ALT + \|/   */
            {
                wxCommandEvent dummy;
                OnContextSyncDirNone(dummy);
            }
            return;
        }

    else
        switch (keyCode)
        {
            case WXK_DELETE:
            case WXK_NUMPAD_DELETE:
            {
                const std::set<size_t> viewSelectionLeft  = getSelectedRows(m_gridLeft);
                const std::set<size_t> viewSelectionRight = getSelectedRows(m_gridRight);
                deleteSelectedFiles(viewSelectionLeft, viewSelectionRight);
            }
            return;

            case WXK_SPACE:
            case WXK_NUMPAD_SPACE:
            {
                wxCommandEvent dummy;
                OnContextFilterTemp(dummy);
            }
            return;

            case WXK_RETURN:
            case WXK_NUMPAD_ENTER:
            {
                if (!globalSettings->gui.externelApplications.empty())
                    openExternalApplication(globalSettings->gui.externelApplications[0].second); //open with first external application
            }
            return;
        }

    event.Skip(); //unknown keypress: propagate
}


void MainDialog::onGridMiddleButtonEvent(wxKeyEvent& event)
{
    const int keyCode = event.GetKeyCode();

    if (event.ControlDown())
        switch (keyCode)
        {
            case 'C':
            case WXK_INSERT: //CTRL + C || CTRL + INS
                copySelectionToClipboard(*m_gridMiddle);
                return;

            case 'A': //CTRL + A
                m_gridMiddle->SelectAll();
                return;
        }

    else if (event.AltDown())
        switch (keyCode)
        {
            case WXK_LEFT: //ALT + <-
            {
                std::set<size_t> selection = getSelectedRows(m_gridMiddle);
                setSyncDirManually(selection, zen::SYNC_DIR_LEFT);
            }
            return;

            case WXK_RIGHT: //ALT + ->
            {
                std::set<size_t> selection = getSelectedRows(m_gridMiddle);
                setSyncDirManually(selection, zen::SYNC_DIR_RIGHT);
            }
            return;

            case WXK_UP:   /* ALT + /|\   */
            case WXK_DOWN: /* ALT + \|/   */
            {
                std::set<size_t> selection = getSelectedRows(m_gridMiddle);
                setSyncDirManually(selection, zen::SYNC_DIR_NONE);
            }
            return;
        }

    else
        switch (keyCode)
        {
            case WXK_DELETE:
            case WXK_NUMPAD_DELETE:
            {
                std::set<size_t> selection = getSelectedRows(m_gridMiddle);
                deleteSelectedFiles(selection, selection);
            }

            return;

            case WXK_SPACE:
            case WXK_NUMPAD_SPACE:
            {
                std::set<size_t> selection = getSelectedRows(m_gridMiddle);
                filterRangeManually(selection, static_cast<int>(*selection.begin()));
            }
            return;
        }

    event.Skip(); //unknown keypress: propagate
}


void MainDialog::onGridRightButtonEvent(wxKeyEvent& event)
{
    const int keyCode = event.GetKeyCode();

    if (event.ControlDown())
        switch (keyCode)
        {
            case 'C':
            case WXK_INSERT: //CTRL + C || CTRL + INS
                copySelectionToClipboard(*m_gridRight);
                return;

            case 'A': //CTRL + A
                m_gridRight->SelectAll();
                return;

            case WXK_NUMPAD_ADD: //CTRL + '+'
                m_gridRight->autoSizeColumns();
                return;
        }

    else if (event.AltDown())
        switch (keyCode)
        {
            case WXK_LEFT: //ALT + <-
            {
                wxCommandEvent dummy;
                OnContextSyncDirLeft(dummy);
            }
            return;

            case WXK_RIGHT: //ALT + ->
            {
                wxCommandEvent dummy;
                OnContextSyncDirRight(dummy);
            }
            return;

            case WXK_UP:   /* ALT + /|\   */
            case WXK_DOWN: /* ALT + \|/   */
            {
                wxCommandEvent dummy;
                OnContextSyncDirNone(dummy);
            }
            return;
        }

    else
        switch (keyCode)
        {
            case WXK_DELETE:
            case WXK_NUMPAD_DELETE:
            {
                const std::set<size_t> viewSelectionLeft  = getSelectedRows(m_gridLeft);
                const std::set<size_t> viewSelectionRight = getSelectedRows(m_gridRight);
                deleteSelectedFiles(viewSelectionLeft, viewSelectionRight);
            }

            return;

            case WXK_SPACE:
            case WXK_NUMPAD_SPACE:
            {
                wxCommandEvent dummy;
                OnContextFilterTemp(dummy);
            }
            return;

            case WXK_RETURN:
            case WXK_NUMPAD_ENTER:
            {
                if (!globalSettings->gui.externelApplications.empty())
                    openExternalApplication(globalSettings->gui.externelApplications[0].second); //open with first external application
            }
            return;
        }

    event.Skip(); //unknown keypress: propagate
}


bool isPartOf(const wxWindow* child, const wxWindow* top)
{
    for (const wxWindow* i = child; i != NULL; i = i->GetParent())
        if (i == top)
            return true;
    return false;
}


void MainDialog::OnGlobalKeyEvent(wxKeyEvent& event) //process key events without explicit menu entry :)
{
    //avoid recursion!!! -> this ugly construct seems to be the only (portable) way to avoid re-entrancy
    //recursion may happen in multiple situations: e.g. modal dialogs, wxGrid::ProcessEvent()!
    if (processingGlobalKeyEvent ||
        !IsShown()               ||
        !IsActive()              ||
        !IsEnabled()             || //only handle if main window is in use
        !m_gridLeft->IsEnabled())   //
    {
        event.Skip();
        return;
    }
    class PreventRecursion
    {
    public:
        PreventRecursion(bool& active) : active_(active) { active_ = true; }
        ~PreventRecursion() { active_ = false; }
    private:
        bool& active_;
    } dummy(processingGlobalKeyEvent);
    //----------------------------------------------------

    const int keyCode = event.GetKeyCode();

    //CTRL + X
    if (event.ControlDown())
        switch (keyCode)
        {
            case 'F': //CTRL + F
                zen::startFind(*this, *m_gridLeft, *m_gridRight, globalSettings->gui.textSearchRespectCase);
                return; //-> swallow event!
        }

    switch (keyCode)
    {
        case WXK_F3:        //F3
        case WXK_NUMPAD_F3: //
            zen::findNext(*this, *m_gridLeft, *m_gridRight, globalSettings->gui.textSearchRespectCase);
            return; //-> swallow event!

            //redirect certain (unhandled) keys directly to grid!
        case WXK_UP:
        case WXK_DOWN:
        case WXK_LEFT:
        case WXK_RIGHT:
        case WXK_NUMPAD_UP:
        case WXK_NUMPAD_DOWN:
        case WXK_NUMPAD_LEFT:
        case WXK_NUMPAD_RIGHT:

        case WXK_PAGEUP:
        case WXK_PAGEDOWN:
        case WXK_HOME:
        case WXK_END:
        case WXK_NUMPAD_PAGEUP:
        case WXK_NUMPAD_PAGEDOWN:
        case WXK_NUMPAD_HOME:
        case WXK_NUMPAD_END:
        {
            const wxWindow* focus = wxWindow::FindFocus();
            if (!isPartOf(focus, m_gridLeft)       && //don't propagate keyboard commands if grid is already in focus
                !isPartOf(focus, m_gridMiddle)     &&
                !isPartOf(focus, m_gridRight)      &&
                !isPartOf(focus, m_listBoxHistory) && //don't propagate if selecting config
                !isPartOf(focus, m_directoryLeft)  && //don't propagate if changing directory field
                !isPartOf(focus, m_directoryRight) &&
                !isPartOf(focus, m_scrolledWindowFolderPairs))
            {
                m_gridLeft->SetFocus();
                m_gridLeft->GetEventHandler()->ProcessEvent(event); //propagating event catched at wxTheApp to child leads to recursion, but we prevented it...
                event.Skip(false); //definitively handled now!
                return;
            }
        }
        break;
    }

    event.Skip();
}


//------------------------------------------------------------
//temporal variables used by exclude via context menu, transport string object to context menu handler
struct CtxtSelectionString : public wxObject
{
    CtxtSelectionString(const wxString& name) : objName(name) {}
    const wxString objName;
};

struct SelectedExtension : public wxObject
{
    SelectedExtension(const Zstring& ext) : extension(ext) {}

    Zstring extension;
};

typedef std::vector<std::pair<Zstring, bool> > FilterObjList; //relative name |-> "is directory flag"

struct FilterObjContainer : public wxObject
{
    FilterObjContainer(const FilterObjList& objList) : selectedObjects(objList) {}

    FilterObjList selectedObjects;
};
//------------------------------------------------------------



void MainDialog::OnContextRim(wxGridEvent& event)
{
    //usability: select row unter right-click if not already selected
    wxGrid* sourceGrid = dynamic_cast<wxGrid*>(event.GetEventObject());
    if (sourceGrid != NULL)
    {
        const int clickedRow = event.GetRow();
        const int clickedCol = event.GetCol();
        if (clickedRow >= 0 && clickedCol >= 0 && !sourceGrid->IsInSelection(clickedRow, 0))
        {
            sourceGrid->SelectRow(clickedRow);
            sourceGrid->SetGridCursor(clickedRow, clickedCol);

            if (sourceGrid == m_gridLeft)
                m_gridRight->ClearSelection();
            else if (sourceGrid == m_gridRight)
                m_gridLeft->ClearSelection();
        }
    }
    //------------------------------------------------------------------------------


    std::set<size_t> selection;

    {
        const std::set<size_t> selectionLeft  = getSelectedRows(m_gridLeft);
        const std::set<size_t> selectionRight = getSelectedRows(m_gridRight);
        selection.insert(selectionLeft .begin(), selectionLeft .end());
        selection.insert(selectionRight.begin(), selectionRight.end());
    }

    const size_t selectionBegin = selection.size() == 0 ? 0 : *selection.begin();

    const FileSystemObject* fsObj = gridDataView->getObject(selectionBegin);

#ifndef _MSC_VER
#warning context menu buttons komplett lokalisieren: ALT+LEFT, SPACE D-Click, ENTER..
#warning statt "Set direction: *-" besser "Set direction: ->"
#endif

    //#######################################################
    //re-create context menu
    contextMenu.reset(new wxMenu);

    if (syncPreview->previewIsEnabled() &&
        fsObj && fsObj->getSyncOperation() != SO_EQUAL)
    {
        if (selection.size() > 0)
        {
            //CONTEXT_SYNC_DIR_LEFT
            wxMenuItem* menuItemSyncDirLeft = new wxMenuItem(contextMenu.get(), wxID_ANY, wxString(_("Set direction:")) +
                                                             wxT(" ") + getSymbol(fsObj->testSyncOperation(true, SYNC_DIR_LEFT)) +
                                                             wxT("\tAlt + Left")); //Linux needs a direction, "<-", because it has no context menu icons!
            menuItemSyncDirLeft->SetBitmap(getSyncOpImage(fsObj->testSyncOperation(true, SYNC_DIR_LEFT)));
            contextMenu->Append(menuItemSyncDirLeft);
            contextMenu->Connect(menuItemSyncDirLeft->GetId(),  wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(MainDialog::OnContextSyncDirLeft),      NULL, this);

            //CONTEXT_SYNC_DIR_NONE
            wxMenuItem* menuItemSyncDirNone = new wxMenuItem(contextMenu.get(), wxID_ANY, wxString(_("Set direction:")) +
                                                             wxT(" ") + getSymbol(fsObj->testSyncOperation(true, SYNC_DIR_NONE)) +
                                                             wxT("\tAlt + Up"));
            menuItemSyncDirNone->SetBitmap(getSyncOpImage(fsObj->testSyncOperation(true, SYNC_DIR_NONE)));
            contextMenu->Append(menuItemSyncDirNone);
            contextMenu->Connect(menuItemSyncDirNone->GetId(),  wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(MainDialog::OnContextSyncDirNone),      NULL, this);

            //CONTEXT_SYNC_DIR_RIGHT
            wxMenuItem* menuItemSyncDirRight = new wxMenuItem(contextMenu.get(), wxID_ANY, wxString(_("Set direction:")) +
                                                              wxT(" ") + getSymbol(fsObj->testSyncOperation(true, SYNC_DIR_RIGHT)) +
                                                              wxT("\tAlt + Right"));
            menuItemSyncDirRight->SetBitmap(getSyncOpImage(fsObj->testSyncOperation(true, SYNC_DIR_RIGHT)));
            contextMenu->Append(menuItemSyncDirRight);
            contextMenu->Connect(menuItemSyncDirRight->GetId(), wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(MainDialog::OnContextSyncDirRight),     NULL, this);

            contextMenu->AppendSeparator();
        }
    }

    //CONTEXT_FILTER_TEMP
    if (fsObj && (selection.size() > 0))
    {
        wxMenuItem* menuItemInExcl = NULL;
        if (fsObj->isActive())
        {
            menuItemInExcl = new wxMenuItem(contextMenu.get(), wxID_ANY, wxString(_("Exclude temporarily")) + wxT("\tSpace"));
            menuItemInExcl->SetBitmap(GlobalResources::instance().getImage(wxT("checkboxFalse")));
        }
        else
        {
            menuItemInExcl = new wxMenuItem(contextMenu.get(), wxID_ANY, wxString(_("Include temporarily")) + wxT("\tSpace"));
            menuItemInExcl->SetBitmap(GlobalResources::instance().getImage(wxT("checkboxTrue")));
        }

        contextMenu->Append(menuItemInExcl);
        contextMenu->Connect(menuItemInExcl->GetId(), wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(MainDialog::OnContextFilterTemp),       NULL, this);
    }
    else
    {
        wxMenuItem* menuItemExcl = contextMenu->Append(wxID_ANY, wxString(_("Exclude temporarily")) + wxT("\tSpace")); //this element should always be visible
        contextMenu->Enable(menuItemExcl->GetId(), false);
    }

    //###############################################################################################
    //get list of relative file/dir-names for filtering
    FilterObjList exFilterCandidateObj;

    {
        class AddFilter : public FSObjectVisitor
        {
        public:
            AddFilter(FilterObjList& fl) : filterList_(fl) {}

            virtual void visit(const FileMapping& fileObj)
            {
                filterList_.push_back(std::make_pair(fileObj.getObjRelativeName(), false));
            }
            virtual void visit(const SymLinkMapping& linkObj)
            {
                filterList_.push_back(std::make_pair(linkObj.getObjRelativeName(), false));
            }
            virtual void visit(const DirMapping& dirObj)
            {
                filterList_.push_back(std::make_pair(dirObj.getObjRelativeName(), true));
            }

        private:
            FilterObjList& filterList_;
        }
        newFilterEntry(exFilterCandidateObj);

        for (std::set<size_t>::const_iterator i = selection.begin(); i != selection.end(); ++i)
        {
            const FileSystemObject* currObj = gridDataView->getObject(*i);
            if (currObj)
                currObj->accept(newFilterEntry);
        }
    }
    //###############################################################################################

    //CONTEXT_EXCLUDE_EXT
    if (exFilterCandidateObj.size() > 0 && !exFilterCandidateObj.begin()->second) //non empty && no directory
    {
        const Zstring filename = exFilterCandidateObj.begin()->first.AfterLast(common::FILE_NAME_SEPARATOR);
        if (filename.find(Zchar('.')) !=  Zstring::npos) //be careful: AfterLast would return the whole string if '.' were not found!
        {
            const Zstring extension = filename.AfterLast(Zchar('.'));

            //add context menu item
            wxMenuItem* menuItemExclExt = new wxMenuItem(contextMenu.get(), wxID_ANY, wxString(_("Exclude via filter:")) + wxT(" ") + wxT("*.") + zToWx(extension));
            menuItemExclExt->SetBitmap(GlobalResources::instance().getImage(wxT("filterSmall")));
            contextMenu->Append(menuItemExclExt);

            //connect event
            contextMenu->Connect(menuItemExclExt->GetId(),
                                 wxEVT_COMMAND_MENU_SELECTED,
                                 wxCommandEventHandler(MainDialog::OnContextExcludeExtension),
                                 new SelectedExtension(extension), //ownership passed!
                                 this);
        }
    }


    //CONTEXT_EXCLUDE_OBJ
    wxMenuItem* menuItemExclObj = NULL;
    if (exFilterCandidateObj.size() == 1)
        menuItemExclObj = new wxMenuItem(contextMenu.get(), wxID_ANY, wxString(_("Exclude via filter:")) + wxT(" ") + zToWx(exFilterCandidateObj.begin()->first.AfterLast(common::FILE_NAME_SEPARATOR)));
    else if (exFilterCandidateObj.size() > 1)
        menuItemExclObj = new wxMenuItem(contextMenu.get(), wxID_ANY, wxString(_("Exclude via filter:")) + wxT(" ") + _("<multiple selection>"));

    if (menuItemExclObj != NULL)
    {
        menuItemExclObj->SetBitmap(GlobalResources::instance().getImage(wxT("filterSmall")));
        contextMenu->Append(menuItemExclObj);

        //connect event
        contextMenu->Connect(menuItemExclObj->GetId(),
                             wxEVT_COMMAND_MENU_SELECTED,
                             wxCommandEventHandler(MainDialog::OnContextExcludeObject),
                             new FilterObjContainer(exFilterCandidateObj), //ownership passed!
                             this);
    }


    //CONTEXT_EXTERNAL_APP
    if (!globalSettings->gui.externelApplications.empty())
    {
        contextMenu->AppendSeparator();

        const bool externalAppEnabled = (m_gridLeft->isLeadGrid() || m_gridRight->isLeadGrid()) &&
                                        selection.size() == 1;

        for (xmlAccess::ExternalApps::iterator i = globalSettings->gui.externelApplications.begin();
             i != globalSettings->gui.externelApplications.end();
             ++i)
        {
            //some trick to translate default external apps on the fly: 1. "open in explorer" 2. "start directly"
            //wxString description = wxGetTranslation(i->first);
            wxString description = zen::translate(i->first);
            if (description.empty())
                description = wxT(" "); //wxWidgets doesn't like empty items

            wxMenuItem* itemExtApp = NULL;
            if (i == globalSettings->gui.externelApplications.begin())
                itemExtApp = contextMenu->Append(wxID_ANY, description + wxT("\t") + wxString(_("D-Click")) + wxT("; Enter"));
            else
                itemExtApp = contextMenu->Append(wxID_ANY, description);
            contextMenu->Enable(itemExtApp->GetId(), externalAppEnabled);

            contextMenu->Connect(itemExtApp->GetId(),
                                 wxEVT_COMMAND_MENU_SELECTED,
                                 wxCommandEventHandler(MainDialog::OnContextOpenWith),
                                 new CtxtSelectionString(i->second), //ownership passed!
                                 this);
        }
    }

    contextMenu->AppendSeparator();


#ifndef _MSC_VER
#warning context menu buttons: nicht mehr all caps
#endif

    //CONTEXT_DELETE_FILES
    wxMenuItem* menuItemDelFiles = contextMenu->Append(wxID_ANY, _("Delete files\tDEL"));
    contextMenu->Enable(menuItemDelFiles->GetId(), selection.size() > 0);
    contextMenu->Connect(menuItemDelFiles->GetId(), wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(MainDialog::OnContextDeleteFiles),      NULL, this);

    //show context menu
    PopupMenu(contextMenu.get());
}


void MainDialog::OnContextFilterTemp(wxCommandEvent& event)
{
    //merge selections from left and right grid
    std::set<size_t> selection = getSelectedRows();
    if (!selection.empty())
        filterRangeManually(selection, static_cast<int>(*selection.begin()));
}


void MainDialog::OnContextExcludeExtension(wxCommandEvent& event)
{
    SelectedExtension* selExtension = dynamic_cast<SelectedExtension*>(event.m_callbackUserData);
    if (selExtension)
    {
        const Zstring newExclude = Zstring(Zstr("*.")) + selExtension->extension;

        //add to filter config
        Zstring& excludeFilter = currentCfg.mainCfg.globalFilter.excludeFilter;
        if (!excludeFilter.empty() && !excludeFilter.EndsWith(Zstr(";")))
            excludeFilter += Zstr("\n");
        excludeFilter += newExclude + Zstr(";"); //';' is appended to 'mark' that next exclude extension entry won't write to new line

        updateFilterButtons();

        //do not fully apply filter, just exclude new items
        addExcludeFiltering(gridDataView->getDataTentative(), newExclude);
        //applyFiltering(getCurrentConfiguration().mainCfg, gridDataView->getDataTentative());
        updateGuiGrid();

        if (currentCfg.hideFilteredElements)
        {
            m_gridLeft->  ClearSelection();
            m_gridRight-> ClearSelection();
            m_gridMiddle->ClearSelection();
        }
    }
}


void MainDialog::OnContextExcludeObject(wxCommandEvent& event)
{
    FilterObjContainer* objCont = dynamic_cast<FilterObjContainer*>(event.m_callbackUserData);
    if (objCont)
    {
        if (objCont->selectedObjects.size() > 0) //check needed to determine if filtering is needed
        {
            Zstring newExclude;
            for (FilterObjList::const_iterator i = objCont->selectedObjects.begin(); i != objCont->selectedObjects.end(); ++i)
            {
                if (i != objCont->selectedObjects.begin())
                    newExclude += Zstr("\n");

                newExclude += common::FILE_NAME_SEPARATOR + i->first;
                if (i->second) //is directory
                    newExclude += common::FILE_NAME_SEPARATOR;
            }

            //add to filter config
            Zstring& excludeFilter = currentCfg.mainCfg.globalFilter.excludeFilter;
            if (!excludeFilter.empty() && !excludeFilter.EndsWith(Zstr("\n")))
                excludeFilter += Zstr("\n");
            excludeFilter += newExclude;

            updateFilterButtons();

            //do not fully apply filter, just exclude new items
            addExcludeFiltering(gridDataView->getDataTentative(), newExclude);
            //applyFiltering(getCurrentConfiguration().mainCfg, gridDataView->getDataTentative());
            updateGuiGrid();

            if (currentCfg.hideFilteredElements)
            {
                m_gridLeft->ClearSelection();
                m_gridRight->ClearSelection();
                m_gridMiddle->ClearSelection();
            }
        }
    }
}



void MainDialog::OnContextOpenWith(wxCommandEvent& event)
{
    CtxtSelectionString* stringObj = dynamic_cast<CtxtSelectionString*>(event.m_callbackUserData);
    if (stringObj)
        openExternalApplication(stringObj->objName);
}



void MainDialog::OnContextDeleteFiles(wxCommandEvent& event)
{
    const std::set<size_t> viewSelectionLeft  = getSelectedRows(m_gridLeft);
    const std::set<size_t> viewSelectionRight = getSelectedRows(m_gridRight);
    deleteSelectedFiles(viewSelectionLeft, viewSelectionRight);
}


void MainDialog::OnContextSyncDirLeft(wxCommandEvent& event)
{
    //merge selections from left and right grid
    const std::set<size_t> selection = getSelectedRows();
    setSyncDirManually(selection, zen::SYNC_DIR_LEFT);
}


void MainDialog::OnContextSyncDirNone(wxCommandEvent& event)
{
    //merge selections from left and right grid
    const std::set<size_t> selection = getSelectedRows();
    setSyncDirManually(selection, zen::SYNC_DIR_NONE);
}


void MainDialog::OnContextSyncDirRight(wxCommandEvent& event)
{
    //merge selections from left and right grid
    const std::set<size_t> selection = getSelectedRows();
    setSyncDirManually(selection, zen::SYNC_DIR_RIGHT);
}


void MainDialog::OnContextRimLabelLeft(wxGridEvent& event)
{
    int ctxtElementId = 1000;

    contextMenu.reset(new wxMenu); //re-create context menu
    contextMenu->Append(++ctxtElementId, _("Customize..."));
    contextMenu->Connect(ctxtElementId, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(MainDialog::OnContextCustColumnLeft), NULL, this);

    contextMenu->AppendSeparator();

    wxMenuItem* itemAutoAdjust = new wxMenuItem(contextMenu.get(), ++ctxtElementId, _("Auto-adjust columns"), wxEmptyString, wxITEM_CHECK);
    contextMenu->Append(itemAutoAdjust);
    itemAutoAdjust->Check(globalSettings->gui.autoAdjustColumnsLeft);
    contextMenu->Connect(ctxtElementId, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(MainDialog::OnContextAutoAdjustLeft), NULL, this);

    PopupMenu(contextMenu.get()); //show context menu
}


void MainDialog::OnContextRimLabelRight(wxGridEvent& event)
{
    int ctxtElementId = 1000;

    contextMenu.reset(new wxMenu); //re-create context menu

    contextMenu->Append(++ctxtElementId, _("Customize..."));
    contextMenu->Connect(ctxtElementId, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(MainDialog::OnContextCustColumnRight), NULL, this);

    contextMenu->AppendSeparator();

    wxMenuItem* itemAutoAdjust = new wxMenuItem(contextMenu.get(), ++ctxtElementId, _("Auto-adjust columns"), wxEmptyString, wxITEM_CHECK);
    contextMenu->Append(itemAutoAdjust);
    itemAutoAdjust->Check(globalSettings->gui.autoAdjustColumnsRight);
    contextMenu->Connect(ctxtElementId, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(MainDialog::OnContextAutoAdjustRight), NULL, this);

    PopupMenu(contextMenu.get()); //show context menu
}


void MainDialog::OnContextCustColumnLeft(wxCommandEvent& event)
{
    xmlAccess::ColumnAttributes colAttr = m_gridLeft->getColumnAttributes();

    if (zen::showCustomizeColsDlg(colAttr) == ReturnSmallDlg::BUTTON_OKAY)
    {
        m_gridLeft->setColumnAttributes(colAttr);

        m_gridLeft->setSortMarker(CustomGrid::SortMarker(-1, CustomGrid::ASCENDING)); //hide sort direction indicator on GUI grids
        m_gridMiddle->setSortMarker(CustomGrid::SortMarker(-1, CustomGrid::ASCENDING));
        m_gridRight->setSortMarker(CustomGrid::SortMarker(-1, CustomGrid::ASCENDING));
    }
}


void MainDialog::OnContextCustColumnRight(wxCommandEvent& event)
{
    xmlAccess::ColumnAttributes colAttr = m_gridRight->getColumnAttributes();

    if (zen::showCustomizeColsDlg(colAttr) == ReturnSmallDlg::BUTTON_OKAY)
    {
        m_gridRight->setColumnAttributes(colAttr);

        m_gridLeft->setSortMarker(CustomGrid::SortMarker(-1, CustomGrid::ASCENDING)); //hide sort direction indicator on GUI grids
        m_gridMiddle->setSortMarker(CustomGrid::SortMarker(-1, CustomGrid::ASCENDING));
        m_gridRight->setSortMarker(CustomGrid::SortMarker(-1, CustomGrid::ASCENDING));
    }
}


void MainDialog::OnContextAutoAdjustLeft(wxCommandEvent& event)
{
    globalSettings->gui.autoAdjustColumnsLeft = !globalSettings->gui.autoAdjustColumnsLeft;
    updateGuiGrid();
}


void MainDialog::OnContextAutoAdjustRight(wxCommandEvent& event)
{
    globalSettings->gui.autoAdjustColumnsRight = !globalSettings->gui.autoAdjustColumnsRight;
    updateGuiGrid();
}


void MainDialog::OnContextMiddle(wxGridEvent& event)
{
    int contextItemID = 2000;

    contextMenu.reset(new wxMenu); //re-create context menu

    contextMenu->Append(++contextItemID, _("Include all rows"));
    if (gridDataView->rowsTotal() == 0)
        contextMenu->Enable(contextItemID, false);
    contextMenu->Connect(contextItemID, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(MainDialog::OnContextIncludeAll),      NULL, this);


    contextMenu->Append(++contextItemID, _("Exclude all rows"));
    if (gridDataView->rowsTotal() == 0)
        contextMenu->Enable(contextItemID, false);
    contextMenu->Connect(contextItemID, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(MainDialog::OnContextExcludeAll),      NULL, this);

    PopupMenu(contextMenu.get()); //show context menu
}


void MainDialog::OnContextMiddleLabel(wxGridEvent& event)
{
    int contextItemID = 2000;

    contextMenu.reset(new wxMenu); //re-create context menu

    wxMenuItem* itemSyncPreview = new wxMenuItem(contextMenu.get(), ++contextItemID, _("Synchronization Preview"));
    contextMenu->Connect(contextItemID, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(MainDialog::OnContextSyncView),       NULL, this);

    wxMenuItem* itemCmpResult   = new wxMenuItem(contextMenu.get(), ++contextItemID, _("Comparison Result"));
    contextMenu->Connect(contextItemID, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(MainDialog::OnContextComparisonView), NULL, this);

    if (syncPreview->previewIsEnabled())
        itemSyncPreview->SetBitmap(GlobalResources::instance().getImage(wxT("syncViewSmall")));
    else
        itemCmpResult->SetBitmap(GlobalResources::instance().getImage(wxT("cmpViewSmall")));

    contextMenu->Append(itemCmpResult);
    contextMenu->Append(itemSyncPreview);

    PopupMenu(contextMenu.get()); //show context menu
}


void MainDialog::OnContextSetLayout(wxMouseEvent& event)
{
    int itemId = 1000;

    contextMenu.reset(new wxMenu); //re-create context menu

    contextMenu->Append(++itemId, _("Reset view"));
    contextMenu->Connect(itemId, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(MainDialog::OnContextSetLayoutReset), NULL, this);

    typedef std::vector<std::pair<wxString, wxString> > CaptionNameMapping;
    CaptionNameMapping captionNameMap;

    const wxAuiPaneInfoArray& paneArray = auiMgr.GetAllPanes();
    for (size_t i = 0; i < paneArray.size(); ++i)
        if (!paneArray[i].IsShown() && !paneArray[i].name.empty() && paneArray[i].window != compareStatus->getAsWindow())
            captionNameMap.push_back(std::make_pair(paneArray[i].caption, paneArray[i].name));

    if (!captionNameMap.empty())
    {
        contextMenu->AppendSeparator();

        for (CaptionNameMapping::const_iterator i = captionNameMap.begin(); i != captionNameMap.end(); ++i)
        {
            wxString entry = _("Show \"%x\"");
            entry.Replace(wxT("%x"), i->first);

            contextMenu->Append(++itemId, entry);
            contextMenu->Connect(itemId,
                                 wxEVT_COMMAND_MENU_SELECTED,
                                 wxCommandEventHandler(MainDialog::OnContextSetLayoutShowPanel),
                                 new CtxtSelectionString(i->second), //ownership passed!
                                 this);
        }
    }

    PopupMenu(contextMenu.get()); //show context menu
}


void MainDialog::OnContextSetLayoutReset(wxCommandEvent& event)
{
    auiMgr.LoadPerspective(defaultPerspective);
}


void MainDialog::OnContextSetLayoutShowPanel(wxCommandEvent& event)
{
    CtxtSelectionString* stringObj = dynamic_cast<CtxtSelectionString*>(event.m_callbackUserData);
    if (stringObj)
    {
        auiMgr.GetPane(stringObj->objName).Show();
        auiMgr.Update();
    }
}


void MainDialog::OnContextIncludeAll(wxCommandEvent& event)
{
    zen::setActiveStatus(true, gridDataView->getDataTentative());
    refreshGridAfterFilterChange(0); //call this instead of updateGuiGrid() to add some delay if hideFiltered == true and to handle some graphical artifacts        break;
}


void MainDialog::OnContextExcludeAll(wxCommandEvent& event)
{
    zen::setActiveStatus(false, gridDataView->getDataTentative());
    refreshGridAfterFilterChange(400); //call this instead of updateGuiGrid() to add some delay if hideFiltered == true and to handle some graphical artifacts
}


void MainDialog::OnContextComparisonView(wxCommandEvent& event)
{
    syncPreview->enablePreview(false); //change view
}


void MainDialog::OnContextSyncView(wxCommandEvent& event)
{
    syncPreview->enablePreview(true); //change view
}


void MainDialog::OnDirSelected(wxFileDirPickerEvent& event)
{
    //left and right directory text-control and dirpicker are synchronized by MainFolderDragDrop automatically

    //disable the sync button
    syncPreview->enableSynchronization(false);

    //clear grids
    gridDataView->clearAllRows();
    updateGuiGrid();

    event.Skip();
}


wxString getFormattedHistoryElement(const wxString& filename)
{
    Zstring output = wxToZ(filename).AfterLast(common::FILE_NAME_SEPARATOR);
    if (output.EndsWith(Zstr(".ffs_gui")))
        output = output.BeforeLast(Zstr('.'));
    return zToWx(output);
}


void MainDialog::addFileToCfgHistory(const wxString& cfgFile)
{
    wxString filename = cfgFile;
    if (filename.empty())
        filename = lastRunConfigName();

    //only (still) existing files should be included in the list
    if (util::fileExists(wxToZ(filename), 200) == util::EXISTING_FALSE) //potentially slow network access: wait 200ms
        return;

    int posFound = -1;

    for (int i = 0; i < static_cast<int>(m_listBoxHistory->GetCount()); ++i)
        if (m_listBoxHistory->GetClientObject(i))
        {
            const wxString& filenameTmp = static_cast<wxClientDataString*>(m_listBoxHistory->GetClientObject(i))->name_;

            //tests if the same filenames are specified, even if they are relative to the current working directory/include symlinks or \\?\ prefix
            if (util::sameFileSpecified(wxToZ(filename), wxToZ(filenameTmp)))
            {
                posFound = i;
                break;
            }
        }

    if (posFound != -1)
    {
        //if entry is in the list, then jump to element
        m_listBoxHistory->SetSelection(posFound);
    }
    else
    {
        int newPos = -1;
        //the default config file should receive another name on GUI
        if (util::sameFileSpecified(wxToZ(lastRunConfigName()), wxToZ(filename)))
            newPos = m_listBoxHistory->Append(_("<Last session>"), new wxClientDataString(filename));  //insert at beginning of list
        else
            newPos = m_listBoxHistory->Append(getFormattedHistoryElement(filename), new wxClientDataString(filename));  //insert at beginning of list

        m_listBoxHistory->SetSelection(newPos);
    }

    //keep maximal size of history list
    //if (m_comboHistory->GetCount() > globalSettings->gui.cfgHistoryMax)
    //    m_comboHistory->Delete(globalSettings->gui.cfgHistoryMax);
}


void MainDialog::addLeftFolderToHistory(const wxString& leftFolder)
{
    m_directoryLeft->addPairToFolderHistory(leftFolder, globalSettings->gui.folderHistMax);
}


void MainDialog::addRightFolderToHistory(const wxString& rightFolder)
{
    m_directoryRight->addPairToFolderHistory(rightFolder, globalSettings->gui.folderHistMax);
}


void MainDialog::OnSaveConfig(wxCommandEvent& event)
{
    trySaveConfig();
}


bool MainDialog::trySaveConfig() //return true if saved successfully
{
    wxString defaultFileName = currentConfigFileName.empty() ? wxT("SyncSettings.ffs_gui") : currentConfigFileName;
    //attention: currentConfigFileName may be an imported *.ffs_batch file! We don't want to overwrite it with a GUI config!
    if (defaultFileName.EndsWith(wxT(".ffs_batch")))
        defaultFileName.Replace(wxT(".ffs_batch"), wxT(".ffs_gui"), false);


    wxFileDialog filePicker(this, wxEmptyString, wxEmptyString, defaultFileName, wxString(_("FreeFileSync configuration")) + wxT(" (*.ffs_gui)|*.ffs_gui"), wxFD_SAVE); //creating this on freestore leads to memleak!
    if (filePicker.ShowModal() == wxID_OK)
    {
        const wxString newFileName = filePicker.GetPath();

        if (zen::fileExists(wxToZ(newFileName)))
        {
            if (showQuestionDlg(ReturnQuestionDlg::BUTTON_YES | ReturnQuestionDlg::BUTTON_CANCEL,
                                wxString(_("File already exists. Overwrite?")) + wxT(" \"") + newFileName + wxT("\"")) != ReturnQuestionDlg::BUTTON_YES)
                return trySaveConfig(); //retry
        }

        if (writeConfigurationToXml(newFileName))
        {
            pushStatusInformation(_("Configuration saved!"));
            return true;
        }
    }

    return false;
}


void MainDialog::OnLoadConfig(wxCommandEvent& event)
{
    wxFileDialog filePicker(this,
                            wxEmptyString,
                            wxEmptyString,
                            wxEmptyString,
                            wxString(_("FreeFileSync configuration")) + wxT(" (*.ffs_gui;*.ffs_batch)|*.ffs_gui;*.ffs_batch"), wxFD_OPEN);

    if (filePicker.ShowModal() == wxID_OK)
        loadConfiguration(filePicker.GetPath());
}


void MainDialog::OnNewConfig(wxCommandEvent& event)
{
    if (!saveOldConfig()) //notify user about changed settings
        return;

    setCurrentConfiguration(xmlAccess::XmlGuiConfig());

    SetTitle(wxString(wxT("FreeFileSync - ")) + _("Folder Comparison and Synchronization"));
    currentConfigFileName.clear();
}


void MainDialog::OnLoadFromHistory(wxCommandEvent& event)
{
    if (event.GetClientObject())
    {
        const wxString filename = static_cast<wxClientDataString*>(event.GetClientObject())->name_;
        loadConfiguration(filename);

        addFileToCfgHistory(currentConfigFileName); //in case user cancelled saving old config: restore selection to currently active config file
    }
}


bool MainDialog::saveOldConfig() //return false on user abort
{
    //notify user about changed settings
    if (globalSettings->optDialogs.popupOnConfigChange && !currentConfigFileName.empty()) //only if check is active and non-default config file loaded
    {
        if (lastConfigurationSaved != getCurrentConfiguration())
        {
            bool dontShowAgain = !globalSettings->optDialogs.popupOnConfigChange;

            switch (showQuestionDlg(ReturnQuestionDlg::BUTTON_YES | ReturnQuestionDlg::BUTTON_NO | ReturnQuestionDlg::BUTTON_CANCEL,
                                    _("Save changes to current configuration?"),
                                    &dontShowAgain))
            {
                case ReturnQuestionDlg::BUTTON_YES:
                    if (!trySaveConfig())
                        return false;
                    break;
                case ReturnQuestionDlg::BUTTON_NO:
                    globalSettings->optDialogs.popupOnConfigChange = !dontShowAgain;
                    break;
                case ReturnQuestionDlg::BUTTON_CANCEL:
                    return false;
            }
        }
    }
    return true;
}


void MainDialog::loadConfiguration(const wxString& filename)
{
    if (!filename.IsEmpty())
    {
        if (!saveOldConfig())
            return;

        if (readConfigurationFromXml(filename))
            pushStatusInformation(_("Configuration loaded!"));
    }
}


void MainDialog::OnCfgHistoryKeyEvent(wxKeyEvent& event)
{
    const int keyCode = event.GetKeyCode();
    if (keyCode == WXK_DELETE || keyCode == WXK_NUMPAD_DELETE)
    {
        //try to delete the currently selected config history item
        //const int selectedItem = m_listBoxHistory->GetCurrentSelection();
        const int selectedItem = m_listBoxHistory->GetSelection();
        if (selectedItem != wxNOT_FOUND)
        {
            m_listBoxHistory->Delete(selectedItem);

            //set selection on next element
            if (m_listBoxHistory->GetCount() > 0)
            {
                int newSelection = selectedItem;
                if (newSelection >= static_cast<int>(m_listBoxHistory->GetCount()))
                    newSelection = m_listBoxHistory->GetCount() - 1;
                m_listBoxHistory->SetSelection(newSelection);
            }
        }

        return; //"swallow" event
    }

    event.Skip();
}


void MainDialog::OnClose(wxCloseEvent& event)
{
    if (m_buttonAbort->IsShown()) //delegate to "abort" button if available
    {
        wxCommandEvent dummy(wxEVT_COMMAND_BUTTON_CLICKED);   //simulate button click
        m_buttonAbort->GetEventHandler()->ProcessEvent(dummy);

        if (event.CanVeto())
        {
            event.Veto(); //that's what we want here
            return;
        }
    }
    else //regular destruction handling
    {
        if (event.CanVeto())
        {
            const bool cancelled = !saveOldConfig(); //notify user about changed settings
            if (cancelled)
            {
                event.Veto();
                return;
            }
        }
    }
    Destroy();
}


void MainDialog::OnCheckRows(FFSCheckRowsEvent& event)
{
    const int lowerBound = std::min(event.rowFrom, event.rowTo);
    const int upperBound = std::max(event.rowFrom, event.rowTo);

    if (0 <= lowerBound)
    {
        std::set<size_t> selectedRowsOnView;

        for (int i = lowerBound; i <= std::min(upperBound, int(gridDataView->rowsOnView()) - 1); ++i)
            selectedRowsOnView.insert(i);

        filterRangeManually(selectedRowsOnView, event.rowFrom);
    }
}


void MainDialog::OnSetSyncDirection(FFSSyncDirectionEvent& event)
{
    const int lowerBound = std::min(event.rowFrom, event.rowTo);
    const int upperBound = std::max(event.rowFrom, event.rowTo);

    if (0 <= lowerBound)
    {
        for (int i = lowerBound; i <= std::min(upperBound, int(gridDataView->rowsOnView()) - 1); ++i)
        {
            FileSystemObject* fsObj = gridDataView->getObject(i);
            if (fsObj)
            {
                setSyncDirectionRec(event.direction, *fsObj); //set new direction (recursively)
                zen::setActiveStatus(true, *fsObj);  //works recursively for directories
            }
        }

        updateGuiGrid();
    }
}


bool MainDialog::readConfigurationFromXml(const wxString& filename)
{
    //load XML
    xmlAccess::XmlGuiConfig newGuiCfg;  //structure to receive gui settings, already defaulted!!
    bool parsingError = true;
    try
    {
        //allow reading batch configurations also
        std::vector<wxString> filenames;
        filenames.push_back(filename);

        xmlAccess::convertConfig(filenames, newGuiCfg); //throw (xmlAccess::XmlError)

        parsingError = false;
    }
    catch (const xmlAccess::XmlError& error)
    {
        if (error.getSeverity() == xmlAccess::XmlError::WARNING)
            wxMessageBox(error.msg(), _("Warning"), wxOK | wxICON_WARNING);
        else
        {
            wxMessageBox(error.msg(), _("Error"), wxOK | wxICON_ERROR);
            return false;
        }
    }

    setCurrentConfiguration(newGuiCfg);

    setLastUsedConfig(filename, parsingError ? xmlAccess::XmlGuiConfig() : newGuiCfg); //simulate changed config on parsing errors

    return !parsingError;
}


void MainDialog::setLastUsedConfig(const wxString& filename, const xmlAccess::XmlGuiConfig& guiConfig)
{
    addFileToCfgHistory(filename); //put filename on list of last used config files

    //set title
    if (filename.empty() || filename == lastRunConfigName())
    {
        SetTitle(wxString(wxT("FreeFileSync - ")) + _("Folder Comparison and Synchronization"));
        currentConfigFileName.clear();
    }
    else
    {
        SetTitle(filename);
        currentConfigFileName = filename;
    }

    lastConfigurationSaved = guiConfig;
}


bool MainDialog::writeConfigurationToXml(const wxString& filename)
{
    const xmlAccess::XmlGuiConfig guiCfg = getCurrentConfiguration();

    //write config to XML
    try
    {
        xmlAccess::writeConfig(guiCfg, filename);
        setLastUsedConfig(filename, guiCfg);
        return true;
    }
    catch (const xmlAccess::XmlError& error)
    {
        wxMessageBox(error.msg().c_str(), _("Error"), wxOK | wxICON_ERROR);
        return false;
    }
}


void MainDialog::setCurrentConfiguration(const xmlAccess::XmlGuiConfig& newGuiCfg)
{
    currentCfg = newGuiCfg;

    //evaluate new settings...

    //disable the sync button
    syncPreview->enableSynchronization(false);

    //clear grids
    gridDataView->clearAllRows();
    updateGuiGrid();

    //(re-)set view filter buttons
    initViewFilterButtons();

    updateFilterButtons();

    //set first folder pair
    firstFolderPair->setValues(zToWx(currentCfg.mainCfg.firstPair.leftDirectory),
                               zToWx(currentCfg.mainCfg.firstPair.rightDirectory),
                               currentCfg.mainCfg.firstPair.altSyncConfig,
                               currentCfg.mainCfg.firstPair.localFilter);

    addLeftFolderToHistory( zToWx(currentCfg.mainCfg.firstPair.leftDirectory));  //another hack: wxCombobox::Insert() asynchronously sends message
    addRightFolderToHistory(zToWx(currentCfg.mainCfg.firstPair.rightDirectory)); //overwriting a later wxCombobox::SetValue()!!! :(

    //clear existing additional folder pairs
    clearAddFolderPairs();

    //set additional pairs
    addFolderPair(currentCfg.mainCfg.additionalPairs);


    //read GUI layout
    m_checkBoxHideFilt->SetValue(currentCfg.hideFilteredElements);

    syncPreview->enablePreview(currentCfg.syncPreviewEnabled);

    //###########################################################
    //update compare variant name
    m_staticTextCmpVariant->SetLabel(wxString(wxT("(")) + getVariantName(currentCfg.mainCfg.compareVar) + wxT(")"));

    //update sync variant name
    m_staticTextSyncVariant->SetLabel(wxString(wxT("(")) + currentCfg.mainCfg.getSyncVariantName() + wxT(")"));
    m_panelTopButtons->Layout(); //adapt layout for variant text
}


inline
FolderPairEnh getEnhancedPair(const DirectoryPair* panel)
{
    return FolderPairEnh(wxToZ(panel->getLeftDir()),
                         wxToZ(panel->getRightDir()),
                         panel->getAltSyncConfig(),
                         panel->getAltFilterConfig());
}


xmlAccess::XmlGuiConfig MainDialog::getCurrentConfiguration() const
{
    xmlAccess::XmlGuiConfig guiCfg = currentCfg;

    //load settings whose ownership lies not in currentCfg:

    //first folder pair
    guiCfg.mainCfg.firstPair = FolderPairEnh(wxToZ(firstFolderPair->getLeftDir()),
                                             wxToZ(firstFolderPair->getRightDir()),
                                             firstFolderPair->getAltSyncConfig(),
                                             firstFolderPair->getAltFilterConfig());

    //add additional pairs
    guiCfg.mainCfg.additionalPairs.clear();
    std::transform(additionalFolderPairs.begin(), additionalFolderPairs.end(),
                   std::back_inserter(guiCfg.mainCfg.additionalPairs), getEnhancedPair);

    //sync preview
    guiCfg.syncPreviewEnabled = syncPreview->previewIsEnabled();

    return guiCfg;
}


const wxString& MainDialog::lastRunConfigName()
{
    static wxString instance = zen::getConfigDir() + wxT("LastRun.ffs_gui");
    return instance;
}


void MainDialog::refreshGridAfterFilterChange(const int delay)
{
    //signal UI that grids need to be refreshed on next Update()
    m_gridLeft  ->ForceRefresh();
    m_gridMiddle->ForceRefresh();
    m_gridRight ->ForceRefresh();
    m_gridLeft  ->Update();   //
    m_gridMiddle->Update();   //show changes resulting from ForceRefresh()
    m_gridRight ->Update();   //

    if (currentCfg.hideFilteredElements)
    {
        wxMilliSleep(delay); //some delay to show user the rows he has filtered out before they are removed
        updateGuiGrid();     //redraw grid to remove excluded elements (keeping sort sequence)

        m_gridLeft->ClearSelection();
        m_gridRight->ClearSelection();
    }
    else
        updateGuiGrid();
}


void MainDialog::OnHideFilteredButton(wxCommandEvent& event)
{
    //toggle showing filtered rows
    currentCfg.hideFilteredElements = !currentCfg.hideFilteredElements;
    //make sure, checkbox and "hideFiltered" are in sync
    m_checkBoxHideFilt->SetValue(currentCfg.hideFilteredElements);

    m_gridLeft->ClearSelection();
    m_gridRight->ClearSelection();
    updateGuiGrid();

    //    event.Skip();
}


void MainDialog::OnConfigureFilter(wxCommandEvent& event)
{
    if (showFilterDialog(true, //is main filter dialog
                         currentCfg.mainCfg.globalFilter) == ReturnSmallDlg::BUTTON_OKAY)
    {
        updateFilterButtons(); //refresh global filter icon
        updateFilterConfig();  //re-apply filter
    }

    //event.Skip()
}


void MainDialog::OnGlobalFilterOpenContext(wxCommandEvent& event)
{
    const int menuId = 1234;
    contextMenu.reset(new wxMenu); //re-create context menu
    contextMenu->Append(menuId, _("Clear filter settings"));
    contextMenu->Connect(wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(MainDialog::OnGlobalFilterRemConfirm), NULL, this);

    if (isNullFilter(currentCfg.mainCfg.globalFilter))
        contextMenu->Enable(menuId, false); //disable menu item, if clicking wouldn't make sense anyway

    PopupMenu(contextMenu.get()); //show context menu
}


void MainDialog::OnGlobalFilterRemConfirm(wxCommandEvent& event)
{
    currentCfg.mainCfg.globalFilter = FilterConfig();

    updateFilterButtons(); //refresh global filter icon
    updateFilterConfig();  //re-apply filter
}


void MainDialog::OnLeftOnlyFiles(wxCommandEvent& event)
{
    m_bpButtonLeftOnly->toggle();
    updateGuiGrid();
}


void MainDialog::OnLeftNewerFiles(wxCommandEvent& event)
{
    m_bpButtonLeftNewer->toggle();
    updateGuiGrid();
}


void MainDialog::OnDifferentFiles(wxCommandEvent& event)
{
    m_bpButtonDifferent->toggle();
    updateGuiGrid();
}


void MainDialog::OnRightNewerFiles(wxCommandEvent& event)
{
    m_bpButtonRightNewer->toggle();
    updateGuiGrid();
}


void MainDialog::OnRightOnlyFiles(wxCommandEvent& event)
{
    m_bpButtonRightOnly->toggle();
    updateGuiGrid();
}


void MainDialog::OnEqualFiles(wxCommandEvent& event)
{
    m_bpButtonEqual->toggle();
    updateGuiGrid();
}


void MainDialog::OnConflictFiles(wxCommandEvent& event)
{
    m_bpButtonConflict->toggle();
    updateGuiGrid();
}


void MainDialog::OnSyncCreateLeft(wxCommandEvent& event)
{
    m_bpButtonSyncCreateLeft->toggle();
    updateGuiGrid();
}


void MainDialog::OnSyncCreateRight(wxCommandEvent& event)
{
    m_bpButtonSyncCreateRight->toggle();
    updateGuiGrid();
}


void MainDialog::OnSyncDeleteLeft(wxCommandEvent& event)
{
    m_bpButtonSyncDeleteLeft->toggle();
    updateGuiGrid();
}


void MainDialog::OnSyncDeleteRight(wxCommandEvent& event)
{
    m_bpButtonSyncDeleteRight->toggle();
    updateGuiGrid();
}


void MainDialog::OnSyncDirLeft(wxCommandEvent& event)
{
    m_bpButtonSyncDirOverwLeft->toggle();
    updateGuiGrid();
}


void MainDialog::OnSyncDirRight(wxCommandEvent& event)
{
    m_bpButtonSyncDirOverwRight->toggle();
    updateGuiGrid();
}


void MainDialog::OnSyncDirNone(wxCommandEvent& event)
{
    m_bpButtonSyncDirNone->toggle();
    updateGuiGrid();
}


void MainDialog::initViewFilterButtons()
{
    //compare result buttons
    m_bpButtonLeftOnly->init(GlobalResources::instance().getImage(wxT("leftOnlyAct")),
                             _("Hide files that exist on left side only"),
                             GlobalResources::instance().getImage(wxT("leftOnlyDeact")),
                             _("Show files that exist on left side only"));

    m_bpButtonRightOnly->init(GlobalResources::instance().getImage(wxT("rightOnlyAct")),
                              _("Hide files that exist on right side only"),
                              GlobalResources::instance().getImage(wxT("rightOnlyDeact")),
                              _("Show files that exist on right side only"));

    m_bpButtonLeftNewer->init(GlobalResources::instance().getImage(wxT("leftNewerAct")),
                              _("Hide files that are newer on left"),
                              GlobalResources::instance().getImage(wxT("leftNewerDeact")),
                              _("Show files that are newer on left"));

    m_bpButtonRightNewer->init(GlobalResources::instance().getImage(wxT("rightNewerAct")),
                               _("Hide files that are newer on right"),
                               GlobalResources::instance().getImage(wxT("rightNewerDeact")),
                               _("Show files that are newer on right"));

    m_bpButtonEqual->init(GlobalResources::instance().getImage(wxT("equalAct")),
                          _("Hide files that are equal"),
                          GlobalResources::instance().getImage(wxT("equalDeact")),
                          _("Show files that are equal"));

    m_bpButtonDifferent->init(GlobalResources::instance().getImage(wxT("differentAct")),
                              _("Hide files that are different"),
                              GlobalResources::instance().getImage(wxT("differentDeact")),
                              _("Show files that are different"));

    m_bpButtonConflict->init(GlobalResources::instance().getImage(wxT("conflictAct")),
                             _("Hide conflicts"),
                             GlobalResources::instance().getImage(wxT("conflictDeact")),
                             _("Show conflicts"));

    //sync preview buttons
    m_bpButtonSyncCreateLeft->init(GlobalResources::instance().getImage(wxT("syncCreateLeftAct")),
                                   _("Hide files that will be created on the left side"),
                                   GlobalResources::instance().getImage(wxT("syncCreateLeftDeact")),
                                   _("Show files that will be created on the left side"));

    m_bpButtonSyncCreateRight->init(GlobalResources::instance().getImage(wxT("syncCreateRightAct")),
                                    _("Hide files that will be created on the right side"),
                                    GlobalResources::instance().getImage(wxT("syncCreateRightDeact")),
                                    _("Show files that will be created on the right side"));

    m_bpButtonSyncDeleteLeft->init(GlobalResources::instance().getImage(wxT("syncDeleteLeftAct")),
                                   _("Hide files that will be deleted on the left side"),
                                   GlobalResources::instance().getImage(wxT("syncDeleteLeftDeact")),
                                   _("Show files that will be deleted on the left side"));

    m_bpButtonSyncDeleteRight->init(GlobalResources::instance().getImage(wxT("syncDeleteRightAct")),
                                    _("Hide files that will be deleted on the right side"),
                                    GlobalResources::instance().getImage(wxT("syncDeleteRightDeact")),
                                    _("Show files that will be deleted on the right side"));

    m_bpButtonSyncDirOverwLeft->init(GlobalResources::instance().getImage(wxT("syncDirLeftAct")),
                                     _("Hide files that will be overwritten on left side"),
                                     GlobalResources::instance().getImage(wxT("syncDirLeftDeact")),
                                     _("Show files that will be overwritten on left side"));

    m_bpButtonSyncDirOverwRight->init(GlobalResources::instance().getImage(wxT("syncDirRightAct")),
                                      _("Hide files that will be overwritten on right side"),
                                      GlobalResources::instance().getImage(wxT("syncDirRightDeact")),
                                      _("Show files that will be overwritten on right side"));

    m_bpButtonSyncDirNone->init(GlobalResources::instance().getImage(wxT("syncDirNoneAct")),
                                _("Hide files that won't be copied"),
                                GlobalResources::instance().getImage(wxT("syncDirNoneDeact")),
                                _("Show files that won't be copied"));

    //compare result buttons
    m_bpButtonLeftOnly->  setActive(true);
    m_bpButtonRightOnly-> setActive(true);
    m_bpButtonLeftNewer-> setActive(true);
    m_bpButtonRightNewer->setActive(true);
    m_bpButtonEqual->     setActive(false);
    m_bpButtonDifferent-> setActive(true);
    m_bpButtonConflict->  setActive(true);

    //sync preview buttons
    m_bpButtonSyncCreateLeft->   setActive(true);
    m_bpButtonSyncCreateRight->  setActive(true);
    m_bpButtonSyncDeleteLeft->   setActive(true);
    m_bpButtonSyncDeleteRight->  setActive(true);
    m_bpButtonSyncDirOverwLeft-> setActive(true);
    m_bpButtonSyncDirOverwRight->setActive(true);
    m_bpButtonSyncDirNone->      setActive(true);
}


void MainDialog::updateFilterButtons()
{
    //global filter: test for Null-filter
    if (isNullFilter(currentCfg.mainCfg.globalFilter))
    {
        m_bpButtonFilter->SetBitmapLabel(GlobalResources::instance().getImage(wxT("filterOff")));
        m_bpButtonFilter->SetToolTip(_("No filter selected"));
    }
    else
    {
        m_bpButtonFilter->SetBitmapLabel(GlobalResources::instance().getImage(wxT("filterOn")));
        m_bpButtonFilter->SetToolTip(_("Filter is active"));
    }

    //update main local filter
    firstFolderPair->refreshButtons();

    //update folder pairs
    for (std::vector<DirectoryPair*>::const_iterator i = additionalFolderPairs.begin(); i != additionalFolderPairs.end(); ++i)
    {
        DirectoryPair* dirPair = *i;
        dirPair->refreshButtons();
    }
}


void MainDialog::OnCompare(wxCommandEvent& event)
{
    //PERF_START;
    clearStatusBar();

    wxBusyCursor dummy; //show hourglass cursor

    //prevent temporary memory peak by clearing old result list
    gridDataView->clearAllRows();
    //updateGuiGrid(); -> don't resize grid to keep scroll position!

    try
    {
        //class handling status display and error messages
        CompareStatusHandler statusHandler(*this);

        const std::vector<zen::FolderPairCfg> cmpConfig = zen::extractCompareCfg(getCurrentConfiguration().mainCfg);

        //GUI mode: place directory locks on directories isolated(!) during both comparison and synchronization
        LockHolder dummy2;
        for (std::vector<FolderPairCfg>::const_iterator i = cmpConfig.begin(); i != cmpConfig.end(); ++i)
        {
            dummy2.addDir(i->leftDirectoryFmt,  statusHandler);
            dummy2.addDir(i->rightDirectoryFmt, statusHandler);
        }

        //begin comparison
        zen::CompareProcess comparison(currentCfg.mainCfg.handleSymlinks,
                                       globalSettings->fileTimeTolerance,
                                       globalSettings->optDialogs,
                                       statusHandler);

        //technical representation of comparison data
        zen::FolderComparison newCompareData;
        comparison.startCompareProcess(cmpConfig, //call getCurrentCfg() to get current values for directory pairs!
                                       currentCfg.mainCfg.compareVar,
                                       newCompareData);

        gridDataView->setData(newCompareData); //newCompareData is invalidated after this call

        //play (optional) sound notification after sync has completed (GUI and batch mode)
        const wxString soundFile = zen::getResourceDir() + wxT("Compare_Complete.wav");
        if (fileExists(wxToZ(soundFile)))
            wxSound::Play(soundFile, wxSOUND_ASYNC);
    }
    catch (AbortThisProcess&)
    {
        //disable the sync button
        syncPreview->enableSynchronization(false);
        m_buttonCompare->SetFocus();
        updateGuiGrid(); //refresh grid in ANY case! (also on abort)
        return;
    }

    //once compare is finished enable the sync button
    syncPreview->enableSynchronization(true);
    m_buttonStartSync->SetFocus();

    //hide sort direction indicator on GUI grids
    m_gridLeft  ->setSortMarker(CustomGrid::SortMarker(-1, CustomGrid::ASCENDING));
    m_gridMiddle->setSortMarker(CustomGrid::SortMarker(-1, CustomGrid::ASCENDING));
    m_gridRight ->setSortMarker(CustomGrid::SortMarker(-1, CustomGrid::ASCENDING));

    //reset last sort selection: used for determining sort direction
    lastSortColumn = -1;
    lastSortGrid   = NULL;

    m_gridLeft->  ClearSelection();
    m_gridMiddle->ClearSelection();
    m_gridRight-> ClearSelection();

    //add to folder history after successful comparison only
    addLeftFolderToHistory( m_directoryLeft->GetValue());
    addRightFolderToHistory(m_directoryRight->GetValue());

    //refresh grid in ANY case! (also on abort)
    updateGuiGrid();

    //prepare status information
    if (allElementsEqual(gridDataView->getDataTentative()))
        pushStatusInformation(_("All directories in sync!"));
}


void MainDialog::updateGuiGrid()
{
    wxWindowUpdateLocker dummy(m_panelStatusBar); //avoid display distortion

    updateGridViewData(); //update gridDataView and write status information

    //all three grids retrieve their data directly via gridDataView
    //the only thing left to do is notify the grids to updafte their sizes (nr of rows), since this has to be communicated by the grids via messages
    m_gridLeft  ->updateGridSizes();
    m_gridMiddle->updateGridSizes();
    m_gridRight ->updateGridSizes();

    //enlarge label width to display row numbers correctly
    const int nrOfRows = m_gridLeft->GetNumberRows();
    if (nrOfRows >= 0)
    {
#ifdef FFS_WIN
        const size_t digitWidth = 8;
#elif defined FFS_LINUX
        const size_t digitWidth = 10;
#endif
        const size_t nrOfDigits = common::getDigitCount(static_cast<size_t>(nrOfRows));
        m_gridLeft ->SetRowLabelSize(static_cast<int>(nrOfDigits * digitWidth + 4));
        m_gridRight->SetRowLabelSize(static_cast<int>(nrOfDigits * digitWidth + 4));
    }

    //support for column auto adjustment
    if (globalSettings->gui.autoAdjustColumnsLeft)
        m_gridLeft->autoSizeColumns();
    if (globalSettings->gui.autoAdjustColumnsRight)
        m_gridRight->autoSizeColumns();

    //update sync preview statistics
    updateStatistics();

    auiMgr.Update(); //fix small display distortion, if view filter panel is empty

    m_gridLeft  ->Refresh();
    m_gridMiddle->Refresh();
    m_gridRight ->Refresh();
}


void MainDialog::updateStatistics()
{
    //update preview of bytes to be transferred:
    const SyncStatistics st(gridDataView->getDataTentative());
    const wxString toCreate = zen::toStringSep(st.getCreate());
    const wxString toUpdate = zen::toStringSep(st.getOverwrite());
    const wxString toDelete = zen::toStringSep(st.getDelete());
    const wxString data     = zen::formatFilesizeToShortString(st.getDataToProcess());

    m_textCtrlCreate->SetValue(toCreate);
    m_textCtrlUpdate->SetValue(toUpdate);
    m_textCtrlDelete->SetValue(toDelete);
    m_textCtrlData  ->SetValue(data);
}


void MainDialog::OnSwitchView(wxCommandEvent& event)
{
    //toggle view
    syncPreview->enablePreview(!syncPreview->previewIsEnabled());
}


void MainDialog::OnSyncSettings(wxCommandEvent& event)
{
    if (showSyncConfigDlg(currentCfg.mainCfg.compareVar,
                          currentCfg.mainCfg.syncConfiguration,
                          currentCfg.mainCfg.handleDeletion,
                          currentCfg.mainCfg.customDeletionDirectory,
                          &currentCfg.handleError) == ReturnSyncConfig::BUTTON_OKAY) //optional input parameter
    {
        applySyncConfig();
    }
}


void MainDialog::OnCmpSettings(wxCommandEvent& event)
{
    //show window right next to the compare-config button
    wxPoint windowPos = m_bpButtonCmpConfig->GetScreenPosition();
    windowPos.x += m_bpButtonCmpConfig->GetSize().GetWidth() + 5;

    const CompareVariant  compareVarOld     = currentCfg.mainCfg.compareVar;
    const SymLinkHandling handleSymlinksOld = currentCfg.mainCfg.handleSymlinks;

    if (zen::showCompareCfgDialog(windowPos,
                                  currentCfg.mainCfg.compareVar,
                                  currentCfg.mainCfg.handleSymlinks) == ReturnSmallDlg::BUTTON_OKAY &&
        //check if settings were changed at all
        (compareVarOld     != currentCfg.mainCfg.compareVar ||
         handleSymlinksOld != currentCfg.mainCfg.handleSymlinks))
    {
        //update compare variant name
        m_staticTextCmpVariant->SetLabel(wxString(wxT("(")) + getVariantName(currentCfg.mainCfg.compareVar) + wxT(")"));
        m_panelTopButtons->Layout(); //adapt layout for variant text

        //disable the sync button
        syncPreview->enableSynchronization(false);

        //clear grids
        gridDataView->clearAllRows();
        updateGuiGrid();

        //convenience: change sync view
        switch (currentCfg.mainCfg.compareVar)
        {
            case CMP_BY_TIME_SIZE:
                syncPreview->enablePreview(true);
                break;
            case CMP_BY_CONTENT:
                syncPreview->enablePreview(false);
                break;
        }

        m_buttonCompare->SetFocus();
    }
}


void MainDialog::OnStartSync(wxCommandEvent& event)
{
    if (!syncPreview->synchronizationIsEnabled())
    {
        //quick sync: simulate button click on "compare"
        wxCommandEvent dummy2(wxEVT_COMMAND_BUTTON_CLICKED);
        m_buttonCompare->GetEventHandler()->ProcessEvent(dummy2); //synchronous call

        if (!syncPreview->synchronizationIsEnabled()) //check if user aborted or error occured, ect...
            return;
        //pushStatusInformation(_("Please run a Compare first before synchronizing!"));
        //return;
    }

    //show sync preview screen
    if (globalSettings->optDialogs.showSummaryBeforeSync)
    {
        bool dontShowAgain = false;

        if (zen::showSyncPreviewDlg(
                getCurrentConfiguration().mainCfg.getSyncVariantName(),
                zen::SyncStatistics(gridDataView->getDataTentative()),
                dontShowAgain) != ReturnSmallDlg::BUTTON_OKAY)
            return;

        globalSettings->optDialogs.showSummaryBeforeSync = !dontShowAgain;
    }

    wxBusyCursor dummy; //show hourglass cursor

    clearStatusBar();
    try
    {
        //PERF_START;

        //class handling status updates and error messages
        SyncStatusHandler statusHandler(this, currentCfg.handleError, zen::extractJobName(currentConfigFileName));

        FolderComparison& dataToSync = gridDataView->getDataTentative();

        //GUI mode: place directory locks on directories isolated(!) during both comparison and synchronization
        LockHolder dummy2;
        for (FolderComparison::const_iterator i = dataToSync.begin(); i != dataToSync.end(); ++i)
        {
            dummy2.addDir(i->getBaseDir<LEFT_SIDE >(), statusHandler);
            dummy2.addDir(i->getBaseDir<RIGHT_SIDE>(), statusHandler);
        }

        //start synchronization and mark all elements processed
        zen::SyncProcess synchronization(
            globalSettings->optDialogs,
            globalSettings->verifyFileCopy,
            globalSettings->copyLockedFiles,
            globalSettings->copyFilePermissions,
            statusHandler);

        const std::vector<zen::FolderPairSyncCfg> syncProcessCfg = zen::extractSyncCfg(getCurrentConfiguration().mainCfg);

        //make sure syncProcessCfg and dataToSync have same size and correspond!
        if (syncProcessCfg.size() != dataToSync.size())
            throw std::logic_error("Programming Error: Contract violation!"); //should never happen: sync button is deactivated if they are not in sync

        synchronization.startSynchronizationProcess(syncProcessCfg, dataToSync);

        //play (optional) sound notification after sync has completed (GUI and batch mode)
        const wxString soundFile = zen::getResourceDir() + wxT("Sync_Complete.wav");
        if (fileExists(wxToZ(soundFile)))
            wxSound::Play(soundFile, wxSOUND_ASYNC);
    }
    catch (AbortThisProcess&)
    {
        //do NOT disable the sync button: user might want to try to sync the REMAINING rows
    }   //enableSynchronization(false);

    //remove rows that empty: just a beautification, invalid rows shouldn't cause issues
    gridDataView->removeInvalidRows();

    updateGuiGrid();

    m_gridLeft->  ClearSelection();
    m_gridMiddle->ClearSelection();
    m_gridRight-> ClearSelection();
}


void MainDialog::OnLeftGridDoubleClick(wxGridEvent& event)
{
    if (!globalSettings->gui.externelApplications.empty())
        openExternalApplication(event.GetRow(), true, globalSettings->gui.externelApplications[0].second);
    //  event.Skip();
}


void MainDialog::OnRightGridDoubleClick(wxGridEvent& event)
{
    if (!globalSettings->gui.externelApplications.empty())
        openExternalApplication(event.GetRow(), false, globalSettings->gui.externelApplications[0].second);
    //    event.Skip();
}


void MainDialog::OnSortLeftGrid(wxGridEvent& event)
{
    //determine direction for std::sort()
    const int currentSortColumn = event.GetCol();
    if (0 <= currentSortColumn && currentSortColumn < int(xmlAccess::COLUMN_TYPE_COUNT))
    {
        static bool sortDefault = true;
        if (lastSortColumn != currentSortColumn || lastSortGrid != m_gridLeft)
            sortDefault = true;
        else
            sortDefault = !sortDefault;

        lastSortColumn = currentSortColumn;
        lastSortGrid   = m_gridLeft;

        GridView::SortType st = GridView::SORT_BY_REL_NAME;

        const xmlAccess::ColumnTypes columnType = m_gridLeft->getTypeAtPos(currentSortColumn);
        switch (columnType)
        {
            case xmlAccess::FULL_PATH:
                st = GridView::SORT_BY_REL_NAME;
                break;
            case xmlAccess::FILENAME:
                st = GridView::SORT_BY_FILENAME;
                break;
            case xmlAccess::REL_PATH:
                st = GridView::SORT_BY_REL_NAME;
                break;
            case xmlAccess::DIRECTORY:
                st = GridView::SORT_BY_DIRECTORY;
                break;
            case xmlAccess::SIZE:
                st = GridView::SORT_BY_FILESIZE;
                break;
            case xmlAccess::DATE:
                st = GridView::SORT_BY_DATE;
                break;
            case xmlAccess::EXTENSION:
                st = GridView::SORT_BY_EXTENSION;
                break;
        }

        const bool sortAscending = sortDefault ?
                                   GridView::getDefaultDirection(st) :
                                   !GridView::getDefaultDirection(st);

        gridDataView->sortView(st, true, sortAscending);

        updateGuiGrid(); //refresh gridDataView

        //set sort direction indicator on UI
        m_gridMiddle->setSortMarker(CustomGrid::SortMarker(-1, CustomGrid::ASCENDING));
        m_gridRight->setSortMarker(CustomGrid::SortMarker(-1, CustomGrid::ASCENDING));
        m_gridLeft->setSortMarker(CustomGrid::SortMarker(currentSortColumn, sortAscending ? CustomGrid::ASCENDING : CustomGrid::DESCENDING));
    }
}


void MainDialog::OnSortMiddleGrid(wxGridEvent& event)
{
    //sorting middle grid is more or less useless: therefore let's toggle view instead!
    syncPreview->enablePreview(!syncPreview->previewIsEnabled()); //toggle view

    //    //determine direction for std::sort()
    //    static bool sortDefault = true;
    //    if (lastSortColumn != 0 || lastSortGrid != m_gridMiddle)
    //        sortDefault = true;
    //    else
    //        sortDefault = !sortDefault;
    //    lastSortColumn = 0;
    //    lastSortGrid   = m_gridMiddle;
    //
    //    //start sort
    //    if (syncPreview->previewIsEnabled())
    //        gridDataView->sortView(GridView::SORT_BY_SYNC_DIRECTION, true, sortDefault);
    //    else
    //        gridDataView->sortView(GridView::SORT_BY_CMP_RESULT, true, sortDefault);
    //
    //    updateGuiGrid(); //refresh gridDataView
    //
    //    //set sort direction indicator on UI
    //    m_gridLeft->setSortMarker(CustomGrid::SortMarker(-1, CustomGrid::ASCENDING));
    //    m_gridRight->setSortMarker(CustomGrid::SortMarker(-1, CustomGrid::ASCENDING));
    //    m_gridMiddle->setSortMarker(CustomGrid::SortMarker(0, sortDefault ? CustomGrid::ASCENDING : CustomGrid::DESCENDING));
}


void MainDialog::OnSortRightGrid(wxGridEvent& event)
{
    //determine direction for std::sort()
    const int currentSortColumn = event.GetCol();
    if (0 <= currentSortColumn && currentSortColumn < int(xmlAccess::COLUMN_TYPE_COUNT))
    {
        static bool sortDefault = true;
        if (lastSortColumn != currentSortColumn || lastSortGrid != m_gridRight)
            sortDefault = true;
        else
            sortDefault = !sortDefault;

        lastSortColumn = currentSortColumn;
        lastSortGrid   = m_gridRight;

        GridView::SortType st = GridView::SORT_BY_REL_NAME;

        const xmlAccess::ColumnTypes columnType = m_gridRight->getTypeAtPos(currentSortColumn);
        switch (columnType)
        {
            case xmlAccess::FULL_PATH:
                st = GridView::SORT_BY_REL_NAME;
                break;
            case xmlAccess::FILENAME:
                st = GridView::SORT_BY_FILENAME;
                break;
            case xmlAccess::REL_PATH:
                st = GridView::SORT_BY_REL_NAME;
                break;
            case xmlAccess::DIRECTORY:
                st = GridView::SORT_BY_DIRECTORY;
                break;
            case xmlAccess::SIZE:
                st = GridView::SORT_BY_FILESIZE;
                break;
            case xmlAccess::DATE:
                st = GridView::SORT_BY_DATE;
                break;
            case xmlAccess::EXTENSION:
                st = GridView::SORT_BY_EXTENSION;
                break;
        }

        const bool sortAscending = sortDefault ?
                                   GridView::getDefaultDirection(st) :
                                   !GridView::getDefaultDirection(st);

        gridDataView->sortView(st, false, sortAscending);

        updateGuiGrid(); //refresh gridDataView

        //set sort direction indicator on UI
        m_gridLeft->setSortMarker(CustomGrid::SortMarker(-1, CustomGrid::ASCENDING));
        m_gridMiddle->setSortMarker(CustomGrid::SortMarker(-1, CustomGrid::ASCENDING));
        m_gridRight->setSortMarker(CustomGrid::SortMarker(currentSortColumn, sortAscending ? CustomGrid::ASCENDING : CustomGrid::DESCENDING));
    }
}


void MainDialog::OnSwapSides(wxCommandEvent& event)
{
    //swap directory names: first pair
    firstFolderPair->setValues(firstFolderPair->getRightDir(), // swap directories
                               firstFolderPair->getLeftDir(),  //
                               firstFolderPair->getAltSyncConfig(),
                               firstFolderPair->getAltFilterConfig());

    //additional pairs
    for (std::vector<DirectoryPair*>::const_iterator i = additionalFolderPairs.begin(); i != additionalFolderPairs.end(); ++i)
    {
        DirectoryPair* dirPair = *i;
        dirPair->setValues(dirPair->getRightDir(), // swap directories
                           dirPair->getLeftDir(),  //
                           dirPair->getAltSyncConfig(),
                           dirPair->getAltFilterConfig());
    }

    //swap view filter
    bool tmp = m_bpButtonLeftOnly->isActive();
    m_bpButtonLeftOnly->setActive(m_bpButtonRightOnly->isActive());
    m_bpButtonRightOnly->setActive(tmp);

    tmp = m_bpButtonLeftNewer->isActive();
    m_bpButtonLeftNewer->setActive(m_bpButtonRightNewer->isActive());
    m_bpButtonRightNewer->setActive(tmp);


    tmp = m_bpButtonSyncCreateLeft->isActive();
    m_bpButtonSyncCreateLeft->setActive(m_bpButtonSyncCreateRight->isActive());
    m_bpButtonSyncCreateRight->setActive(tmp);

    tmp = m_bpButtonSyncDeleteLeft->isActive();
    m_bpButtonSyncDeleteLeft->setActive(m_bpButtonSyncDeleteRight->isActive());
    m_bpButtonSyncDeleteRight->setActive(tmp);

    tmp = m_bpButtonSyncDirOverwLeft->isActive();
    m_bpButtonSyncDirOverwLeft->setActive(m_bpButtonSyncDirOverwRight->isActive());
    m_bpButtonSyncDirOverwRight->setActive(tmp);

    //swap grid information
    zen::swapGrids(getCurrentConfiguration().mainCfg, gridDataView->getDataTentative());
    updateGuiGrid();
}


void MainDialog::updateGridViewData()
{
    size_t filesOnLeftView    = 0;
    size_t foldersOnLeftView  = 0;
    size_t filesOnRightView   = 0;
    size_t foldersOnRightView = 0;
    zen::UInt64 filesizeLeftView;
    zen::UInt64 filesizeRightView;

    //disable all buttons per default
    m_bpButtonLeftOnly->  Show(false);
    m_bpButtonRightOnly-> Show(false);
    m_bpButtonLeftNewer-> Show(false);
    m_bpButtonRightNewer->Show(false);
    m_bpButtonDifferent-> Show(false);
    m_bpButtonEqual->     Show(false);
    m_bpButtonConflict->  Show(false);

    m_bpButtonSyncCreateLeft->   Show(false);
    m_bpButtonSyncCreateRight->  Show(false);
    m_bpButtonSyncDeleteLeft->   Show(false);
    m_bpButtonSyncDeleteRight->  Show(false);
    m_bpButtonSyncDirOverwLeft-> Show(false);
    m_bpButtonSyncDirOverwRight->Show(false);
    m_bpButtonSyncDirNone->      Show(false);


    if (syncPreview->previewIsEnabled())
    {
        const GridView::StatusSyncPreview result = gridDataView->updateSyncPreview(currentCfg.hideFilteredElements,
                                                                                   m_bpButtonSyncCreateLeft->   isActive(),
                                                                                   m_bpButtonSyncCreateRight->  isActive(),
                                                                                   m_bpButtonSyncDeleteLeft->   isActive(),
                                                                                   m_bpButtonSyncDeleteRight->  isActive(),
                                                                                   m_bpButtonSyncDirOverwLeft-> isActive(),
                                                                                   m_bpButtonSyncDirOverwRight->isActive(),
                                                                                   m_bpButtonSyncDirNone->      isActive(),
                                                                                   m_bpButtonEqual->            isActive(),
                                                                                   m_bpButtonConflict->         isActive());

        filesOnLeftView    = result.filesOnLeftView;
        foldersOnLeftView  = result.foldersOnLeftView;
        filesOnRightView   = result.filesOnRightView;
        foldersOnRightView = result.foldersOnRightView;
        filesizeLeftView   = result.filesizeLeftView;
        filesizeRightView  = result.filesizeRightView;


        //sync preview buttons
        m_bpButtonSyncCreateLeft->   Show(result.existsSyncCreateLeft);
        m_bpButtonSyncCreateRight->  Show(result.existsSyncCreateRight);
        m_bpButtonSyncDeleteLeft->   Show(result.existsSyncDeleteLeft);
        m_bpButtonSyncDeleteRight->  Show(result.existsSyncDeleteRight);
        m_bpButtonSyncDirOverwLeft-> Show(result.existsSyncDirLeft);
        m_bpButtonSyncDirOverwRight->Show(result.existsSyncDirRight);
        m_bpButtonSyncDirNone->      Show(result.existsSyncDirNone);
        m_bpButtonEqual->            Show(result.existsSyncEqual);
        m_bpButtonConflict->         Show(result.existsConflict);

        if (m_bpButtonSyncCreateLeft->   IsShown() ||
            m_bpButtonSyncCreateRight->  IsShown() ||
            m_bpButtonSyncDeleteLeft->   IsShown() ||
            m_bpButtonSyncDeleteRight->  IsShown() ||
            m_bpButtonSyncDirOverwLeft-> IsShown() ||
            m_bpButtonSyncDirOverwRight->IsShown() ||
            m_bpButtonSyncDirNone->      IsShown() ||
            m_bpButtonEqual->            IsShown() ||
            m_bpButtonConflict->         IsShown())
        {
            m_panelViewFilter->Show();
            m_panelViewFilter->Layout();
        }
        else
            m_panelViewFilter->Hide();

    }
    else
    {
        const GridView::StatusCmpResult result = gridDataView->updateCmpResult(currentCfg.hideFilteredElements,
                                                                               m_bpButtonLeftOnly->  isActive(),
                                                                               m_bpButtonRightOnly-> isActive(),
                                                                               m_bpButtonLeftNewer-> isActive(),
                                                                               m_bpButtonRightNewer->isActive(),
                                                                               m_bpButtonDifferent-> isActive(),
                                                                               m_bpButtonEqual->     isActive(),
                                                                               m_bpButtonConflict->  isActive());

        filesOnLeftView    = result.filesOnLeftView;
        foldersOnLeftView  = result.foldersOnLeftView;
        filesOnRightView   = result.filesOnRightView;
        foldersOnRightView = result.foldersOnRightView;
        filesizeLeftView   = result.filesizeLeftView;
        filesizeRightView  = result.filesizeRightView;

        //comparison result view buttons
        m_bpButtonLeftOnly->  Show(result.existsLeftOnly);
        m_bpButtonRightOnly-> Show(result.existsRightOnly);
        m_bpButtonLeftNewer-> Show(result.existsLeftNewer);
        m_bpButtonRightNewer->Show(result.existsRightNewer);
        m_bpButtonDifferent-> Show(result.existsDifferent);
        m_bpButtonEqual->     Show(result.existsEqual);
        m_bpButtonConflict->  Show(result.existsConflict);

        if (m_bpButtonLeftOnly->  IsShown() ||
            m_bpButtonRightOnly-> IsShown() ||
            m_bpButtonLeftNewer-> IsShown() ||
            m_bpButtonRightNewer->IsShown() ||
            m_bpButtonDifferent-> IsShown() ||
            m_bpButtonEqual->     IsShown() ||
            m_bpButtonConflict->  IsShown())
        {
            m_panelViewFilter->Show();
            m_panelViewFilter->Layout();
        }
        else
            m_panelViewFilter->Hide();
    }

    //clear status information
    clearStatusBar();

    wxString statusLeftNew;
    wxString statusMiddleNew;
    wxString statusRightNew;

    //#################################################
    //format numbers to text:

    //show status information on "root" level.
    if (foldersOnLeftView)
    {
        wxString tmp = _P("1 directory", "%x directories", foldersOnLeftView);
        tmp.Replace(wxT("%x"), zen::toStringSep(foldersOnLeftView), false);
        statusLeftNew += tmp;

        if (filesOnLeftView)
            statusLeftNew += wxT(" - ");
    }

    if (filesOnLeftView)
    {
        wxString tmp = _P("1 file", "%x files", filesOnLeftView);
        tmp.Replace(wxT("%x"), zen::toStringSep(filesOnLeftView), false);
        statusLeftNew += tmp;

        statusLeftNew += wxT(" - ");
        statusLeftNew += zen::formatFilesizeToShortString(filesizeLeftView);
    }

    {
        wxString tmp = _P("%x of 1 row in view", "%x of %y rows in view", gridDataView->rowsTotal());
        tmp.Replace(wxT("%x"), toStringSep(gridDataView->rowsOnView()), false);
        tmp.Replace(wxT("%y"), toStringSep(gridDataView->rowsTotal()), false);
        statusMiddleNew = tmp;
    }

    if (foldersOnRightView)
    {
        wxString tmp = _P("1 directory", "%x directories", foldersOnRightView);
        tmp.Replace(wxT("%x"), zen::toStringSep(foldersOnRightView), false);
        statusRightNew += tmp;

        if (filesOnRightView)
            statusRightNew += wxT(" - ");
    }

    if (filesOnRightView)
    {
        wxString tmp = _P("1 file", "%x files", filesOnRightView);
        tmp.Replace(wxT("%x"), zen::toStringSep(filesOnRightView), false);
        statusRightNew += tmp;

        statusRightNew += wxT(" - ");
        statusRightNew += zen::formatFilesizeToShortString(filesizeRightView);
    }


    //avoid screen flicker
    if (m_staticTextStatusLeft->GetLabel() != statusLeftNew)
        m_staticTextStatusLeft->SetLabel(statusLeftNew);
    if (m_staticTextStatusMiddle->GetLabel() != statusMiddleNew)
        m_staticTextStatusMiddle->SetLabel(statusMiddleNew);
    if (m_staticTextStatusRight->GetLabel() != statusRightNew)
        m_staticTextStatusRight->SetLabel(statusRightNew);

    m_panelStatusBar->Layout();
}


void MainDialog::OnAddFolderPair(wxCommandEvent& event)
{
    wxWindowUpdateLocker dummy(this); //avoid display distortion

    std::vector<FolderPairEnh> newPairs;
    newPairs.push_back(getCurrentConfiguration().mainCfg.firstPair);

    addFolderPair(newPairs, true); //add pair in front of additonal pairs

    //clear first pair
    const FolderPairEnh cfgEmpty;
    firstFolderPair->setValues(zToWx(cfgEmpty.leftDirectory),
                               zToWx(cfgEmpty.rightDirectory),
                               cfgEmpty.altSyncConfig,
                               cfgEmpty.localFilter);

    //disable the sync button
    syncPreview->enableSynchronization(false);

    //clear grids
    gridDataView->clearAllRows();
    applySyncConfig(); //mainly to update sync dir description text
}


void MainDialog::updateFilterConfig()
{
    applyFiltering(gridDataView->getDataTentative(), getCurrentConfiguration().mainCfg);
    refreshGridAfterFilterChange(400);
}


void MainDialog::applySyncConfig()
{
    //update sync variant name
    m_staticTextSyncVariant->SetLabel(wxString(wxT("(")) + getCurrentConfiguration().mainCfg.getSyncVariantName() + wxT(")"));
    m_panelTopButtons->Layout(); //adapt layout for variant text


    class RedetermineCallback : public DeterminationProblem
    {
    public:
        RedetermineCallback(bool& warningSyncDatabase, wxWindow* parent) :
            warningSyncDatabase_(warningSyncDatabase),
            parent_(parent) {}

        virtual void reportWarning(const wxString& text)
        {
            if (warningSyncDatabase_)
            {
                bool dontWarnAgain = false;
                if (showWarningDlg(ReturnWarningDlg::BUTTON_IGNORE,
                                   text,
                                   dontWarnAgain) == ReturnWarningDlg::BUTTON_IGNORE)
                    warningSyncDatabase_ = !dontWarnAgain;
            }
        }
    private:
        bool& warningSyncDatabase_;
        wxWindow* parent_;
    } redetCallback(globalSettings->optDialogs.warningSyncDatabase, this);

    zen::redetermineSyncDirection(getCurrentConfiguration().mainCfg, gridDataView->getDataTentative(), &redetCallback);
    updateGuiGrid();
}


void MainDialog::OnRemoveTopFolderPair(wxCommandEvent& event)
{
    if (additionalFolderPairs.size() > 0)
    {
        wxWindowUpdateLocker dummy(this); //avoid display distortion

        //get settings from second folder pair
        const FolderPairEnh cfgSecond = getEnhancedPair(additionalFolderPairs[0]);

        //reset first pair
        firstFolderPair->setValues(zToWx(cfgSecond.leftDirectory),
                                   zToWx(cfgSecond.rightDirectory),
                                   cfgSecond.altSyncConfig,
                                   cfgSecond.localFilter);

        removeAddFolderPair(0); //remove second folder pair (first of additional folder pairs)

        //------------------------------------------------------------------
        //disable the sync button
        syncPreview->enableSynchronization(false);

        //clear grids
        gridDataView->clearAllRows();
        applySyncConfig(); //mainly to update sync dir description text
    }
}


void MainDialog::OnRemoveFolderPair(wxCommandEvent& event)
{
    wxWindowUpdateLocker dummy(this); //avoid display distortion

    const wxObject* const eventObj = event.GetEventObject(); //find folder pair originating the event
    for (std::vector<DirectoryPair*>::const_iterator i = additionalFolderPairs.begin(); i != additionalFolderPairs.end(); ++i)
        if (eventObj == (*i)->m_bpButtonRemovePair)
        {
            removeAddFolderPair(i - additionalFolderPairs.begin());

            //------------------------------------------------------------------
            //disable the sync button
            syncPreview->enableSynchronization(false);

            //clear grids
            gridDataView->clearAllRows();
            applySyncConfig(); //mainly to update sync dir description text
            return;
        }
}


void MainDialog::updateGuiForFolderPair()
{
    //adapt delete top folder pair button
    if (additionalFolderPairs.size() == 0)
        m_bpButtonRemovePair->Hide();
    else
        m_bpButtonRemovePair->Show();
    m_panelTopRight->Layout();

    //adapt local filter and sync cfg for first folder pair
    if (additionalFolderPairs.size() == 0 &&
        firstFolderPair->getAltSyncConfig().get() == NULL &&
        isNullFilter(firstFolderPair->getAltFilterConfig()))
    {
        m_bpButtonLocalFilter->Hide();
        m_bpButtonAltSyncCfg->Hide();

        m_bpButtonSwapSides->SetBitmapLabel(GlobalResources::instance().getImage(wxT("swap")));
    }
    else
    {
        m_bpButtonLocalFilter->Show();
        m_bpButtonAltSyncCfg->Show();

        m_bpButtonSwapSides->SetBitmapLabel(GlobalResources::instance().getImage(wxT("swapSlim")));
    }
    m_panelTopMiddle->Layout();


    int addPairHeight = 0;
    if (additionalFolderPairs.size() > 0)
        addPairHeight = std::min<double>(1.5, additionalFolderPairs.size()) * //have 0.5 * height indicate that more folders are there
                        additionalFolderPairs[0]->GetSize().GetHeight();

    //ensure additional folder pairs are at least partially visible
    auiMgr.GetPane(m_panelDirectoryPairs).MinSize(-1, m_panelTopLeft->GetSize().GetHeight() + addPairHeight);
    auiMgr.Update();

    m_scrolledWindowFolderPairs->Fit();    //adjust scrolled window size

    //m_scrolledWindowFolderPairs->Layout(); //adjust stuff inside scrolled window
    m_panelDirectoryPairs->Layout();

    /*
    #warning test
    auiMgr.GetPane(m_panelDirectoryPairs).MaxSize(20, 20);
        auiMgr.Update();
        */
}


void MainDialog::addFolderPair(const std::vector<FolderPairEnh>& newPairs, bool addFront)
{
    wxWindowUpdateLocker dummy(m_panelDirectoryPairs); //avoid display distortion

    if (!newPairs.empty())
    {
        for (std::vector<FolderPairEnh>::const_iterator i = newPairs.begin(); i != newPairs.end(); ++i)
        {
            //add new folder pair
            DirectoryPair* newPair = new DirectoryPair(m_scrolledWindowFolderPairs, *this);

            //set width of left folder panel
            const int width = m_panelTopLeft->GetSize().GetWidth();
            newPair->m_panelLeft->SetMinSize(wxSize(width, -1));


            if (addFront)
            {
                bSizerAddFolderPairs->Insert(0, newPair, 0, wxEXPAND, 5);
                additionalFolderPairs.insert(additionalFolderPairs.begin(), newPair);
            }
            else
            {
                bSizerAddFolderPairs->Add(newPair, 0, wxEXPAND, 5);
                additionalFolderPairs.push_back(newPair);
            }

            //register events
            newPair->m_bpButtonRemovePair->Connect(wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler(MainDialog::OnRemoveFolderPair), NULL, this);

            //set alternate configuration
            newPair->setValues(zToWx(i->leftDirectory),
                               zToWx(i->rightDirectory),
                               i->altSyncConfig,
                               i->localFilter);
        }

        //set size of scrolled window
        //m_scrolledWindowFolderPairs->SetMinSize(wxSize( -1, pairHeight * static_cast<int>(visiblePairs)));

        //update controls
        //        m_scrolledWindowFolderPairs->Fit();    //adjust scrolled window size
        //      m_scrolledWindowFolderPairs->Layout(); //adjust stuff inside scrolled window
        //bSizer1->Layout();
    }

    updateGuiForFolderPair();
}


void MainDialog::removeAddFolderPair(size_t pos)
{
    wxWindowUpdateLocker dummy(m_panelDirectoryPairs); //avoid display distortion

    if (pos < additionalFolderPairs.size())
    {
        //remove folder pairs from window
        DirectoryPair* pairToDelete = additionalFolderPairs[pos];
        //const int pairHeight = pairToDelete->GetSize().GetHeight();

        bSizerAddFolderPairs->Detach(pairToDelete); //Remove() does not work on Window*, so do it manually
        pairToDelete->Destroy();                 //
        additionalFolderPairs.erase(additionalFolderPairs.begin() + pos); //remove element from vector

        //set size of scrolled window
        //const size_t additionalRows = additionalFolderPairs.size();
        //if (additionalRows <= globalSettings->gui.addFolderPairCountMax) //up to "addFolderPairCountMax" additional pairs shall be shown
        //  m_scrolledWindowFolderPairs->SetMinSize(wxSize(-1, pairHeight * static_cast<int>(additionalRows)));

        //update controls
        //m_scrolledWindowFolderPairs->Fit();    //adjust scrolled window size
        //m_scrolledWindowFolderPairs->Layout(); //adjust stuff inside scrolled window
        //bSizer1->Layout();
    }

    updateGuiForFolderPair();
}


void MainDialog::clearAddFolderPairs()
{
    wxWindowUpdateLocker dummy(m_panelDirectoryPairs); //avoid display distortion

    additionalFolderPairs.clear();
    bSizerAddFolderPairs->Clear(true);

    updateGuiForFolderPair();
    //m_scrolledWindowFolderPairs->SetMinSize(wxSize(-1, 0));
    //bSizer1->Layout();
}
//########################################################################################################


//menu events
void MainDialog::OnMenuGlobalSettings(wxCommandEvent& event)
{
    zen::showGlobalSettingsDlg(*globalSettings);

    //event.Skip();
}

namespace
{
inline
void addCellValue(zxString& exportString, const wxString& cellVal)
{
    if (cellVal.find(wxT(';')) != wxString::npos)
        exportString += wxT('\"') + wxToZx(cellVal) + wxT('\"');
    else
        exportString += wxToZx(cellVal);
}
}


void MainDialog::OnMenuExportFileList(wxCommandEvent& event)
{
    //get a filename
    const wxString defaultFileName = wxT("FileList.csv"); //proposal
    wxFileDialog filePicker(this, wxEmptyString, wxEmptyString, defaultFileName, wxString(_("Comma separated list")) + wxT(" (*.csv)|*.csv"), wxFD_SAVE); //creating this on freestore leads to memleak!

    if (filePicker.ShowModal() == wxID_OK)
    {
        const wxString newFileName = filePicker.GetPath();
        if (zen::fileExists(wxToZ(newFileName)))
        {
            if (showQuestionDlg(ReturnQuestionDlg::BUTTON_YES | ReturnQuestionDlg::BUTTON_CANCEL,
                                wxString(_("File already exists. Overwrite?")) + wxT(" \"") + newFileName + wxT("\"")) != ReturnQuestionDlg::BUTTON_YES)
            {
                OnMenuExportFileList(event); //retry
                return;
            }
        }

        zxString exportString; //perf: wxString doesn't model exponential growth and so is out

        //write legend
        exportString +=  wxToZx(_("Legend")) + wxT('\n');
        if (syncPreview->previewIsEnabled())
        {
            exportString += wxT("\"") + wxToZx(getDescription(SO_CREATE_NEW_LEFT))     + wxT("\";") + wxToZx(getSymbol(SO_CREATE_NEW_LEFT))     + wxT('\n');
            exportString += wxT("\"") + wxToZx(getDescription(SO_CREATE_NEW_RIGHT))    + wxT("\";") + wxToZx(getSymbol(SO_CREATE_NEW_RIGHT))    + wxT('\n');
            exportString += wxT("\"") + wxToZx(getDescription(SO_DELETE_LEFT))         + wxT("\";") + wxToZx(getSymbol(SO_DELETE_LEFT))         + wxT('\n');
            exportString += wxT("\"") + wxToZx(getDescription(SO_DELETE_RIGHT))        + wxT("\";") + wxToZx(getSymbol(SO_DELETE_RIGHT))        + wxT('\n');
            exportString += wxT("\"") + wxToZx(getDescription(SO_OVERWRITE_LEFT))      + wxT("\";") + wxToZx(getSymbol(SO_OVERWRITE_LEFT))      + wxT('\n');
            exportString += wxT("\"") + wxToZx(getDescription(SO_OVERWRITE_RIGHT))     + wxT("\";") + wxToZx(getSymbol(SO_OVERWRITE_RIGHT))     + wxT('\n');
            exportString += wxT("\"") + wxToZx(getDescription(SO_DO_NOTHING))          + wxT("\";") + wxToZx(getSymbol(SO_DO_NOTHING))          + wxT('\n');
            exportString += wxT("\"") + wxToZx(getDescription(SO_EQUAL))               + wxT("\";") + wxToZx(getSymbol(SO_EQUAL))               + wxT('\n');
            exportString += wxT("\"") + wxToZx(getDescription(SO_UNRESOLVED_CONFLICT)) + wxT("\";") + wxToZx(getSymbol(SO_UNRESOLVED_CONFLICT)) + wxT('\n');
        }
        else
        {
            exportString += wxT("\"") + wxToZx(getDescription(FILE_LEFT_SIDE_ONLY))  + wxT("\";") + wxToZx(getSymbol(FILE_LEFT_SIDE_ONLY))  + wxT('\n');
            exportString += wxT("\"") + wxToZx(getDescription(FILE_RIGHT_SIDE_ONLY)) + wxT("\";") + wxToZx(getSymbol(FILE_RIGHT_SIDE_ONLY)) + wxT('\n');
            exportString += wxT("\"") + wxToZx(getDescription(FILE_LEFT_NEWER))      + wxT("\";") + wxToZx(getSymbol(FILE_LEFT_NEWER))      + wxT('\n');
            exportString += wxT("\"") + wxToZx(getDescription(FILE_RIGHT_NEWER))     + wxT("\";") + wxToZx(getSymbol(FILE_RIGHT_NEWER))     + wxT('\n');
            exportString += wxT("\"") + wxToZx(getDescription(FILE_DIFFERENT))       + wxT("\";") + wxToZx(getSymbol(FILE_DIFFERENT))       + wxT('\n');
            exportString += wxT("\"") + wxToZx(getDescription(FILE_EQUAL))           + wxT("\";") + wxToZx(getSymbol(FILE_EQUAL))           + wxT('\n');
            exportString += wxT("\"") + wxToZx(getDescription(FILE_CONFLICT))        + wxT("\";") + wxToZx(getSymbol(FILE_CONFLICT))        + wxT('\n');
        }
        exportString += wxT('\n');

        //write header
        const int colsLeft = m_gridLeft->GetNumberCols();
        for (int k = 0; k < colsLeft; ++k)
        {
            addCellValue(exportString, m_gridLeft->GetColLabelValue(k));
            exportString += wxT(';');
        }

        const int colsMiddle = m_gridMiddle->GetNumberCols();
        for (int k = 0; k < colsMiddle; ++k)
        {
            addCellValue(exportString, m_gridMiddle->GetColLabelValue(k));
            exportString += wxT(';');
        }

        const int colsRight = m_gridRight->GetNumberCols();
        for (int k = 0; k < colsRight; ++k)
        {
            addCellValue(exportString, m_gridRight->GetColLabelValue(k));
            if (k != m_gridRight->GetNumberCols() - 1)
                exportString += wxT(';');
        }
        exportString += wxT('\n');

        //begin work
        const int rowsTotal = m_gridLeft->GetNumberRows();
        for (int i = 0; i < rowsTotal; ++i)
        {
            for (int k = 0; k < colsLeft; ++k)
            {
                addCellValue(exportString, m_gridLeft->GetCellValue(i, k));
                exportString += wxT(';');
            }

            for (int k = 0; k < colsMiddle; ++k)
            {
                addCellValue(exportString, m_gridMiddle->GetCellValue(i, k));
                exportString += wxT(';');
            }

            for (int k = 0; k < colsRight; ++k)
            {
                addCellValue(exportString, m_gridRight->GetCellValue(i, k));
                if (k != colsRight - 1)
                    exportString += wxT(';');
            }
            exportString += wxT('\n');
        }

        //write export file
        wxFFile output(newFileName.c_str(), wxT("w")); //don't write in binary mode
        if (output.IsOpened())
        {
            //generate UTF8 representation
            size_t bufferSize = 0;
            const wxCharBuffer utf8buffer = wxConvUTF8.cWC2MB(exportString.c_str(), exportString.size(), &bufferSize);

            output.Write(common::BYTE_ORDER_MARK_UTF8, sizeof(common::BYTE_ORDER_MARK_UTF8) - 1);
            output.Write(utf8buffer, bufferSize);
            pushStatusInformation(_("File list exported!"));
        }
        else
        {
            wxMessageBox(wxString(_("Error writing file:")) + wxT(" \"") + newFileName + wxT("\""), _("Error"), wxOK | wxICON_ERROR);
        }
    }
}


void MainDialog::OnMenuBatchJob(wxCommandEvent& event)
{
    //fill batch config structure
    const xmlAccess::XmlGuiConfig currCfg = getCurrentConfiguration(); //get UP TO DATE config, with updated values for main and additional folders!

    const xmlAccess::XmlBatchConfig batchCfg = convertGuiToBatch(currCfg);

    if (showSyncBatchDlg(batchCfg) == ReturnBatchConfig::BATCH_FILE_SAVED)
        pushStatusInformation(_("Batch file created successfully!"));
}


void MainDialog::OnMenuCheckVersion(wxCommandEvent& event)
{
    zen::checkForUpdateNow();
}


void MainDialog::OnRegularUpdateCheck(wxIdleEvent& event)
{
    //execute just once per startup!
    Disconnect(wxEVT_IDLE, wxIdleEventHandler(MainDialog::OnRegularUpdateCheck), NULL, this);

    zen::checkForUpdatePeriodically(globalSettings->gui.lastUpdateCheck);
}


void MainDialog::OnLayoutWindowAsync(wxIdleEvent& event)
{
    //execute just once per startup!
    Disconnect(wxEVT_IDLE, wxIdleEventHandler(MainDialog::OnLayoutWindowAsync), NULL, this);

    wxWindowUpdateLocker dummy(this); //avoid display distortion

    //adjust folder pair distortion on startup
    for (std::vector<DirectoryPair*>::iterator i = additionalFolderPairs.begin(); i != additionalFolderPairs.end(); ++i)
        (*i)->Layout();

    m_panelTopButtons->Layout();
    Layout(); //strangely this layout call works if called in next idle event only
    auiMgr.Update(); //fix view filter distortion
}


void MainDialog::OnMenuAbout(wxCommandEvent& event)
{
    zen::showAboutDialog();
}


void MainDialog::OnShowHelp(wxCommandEvent& event)
{
    zen::displayHelpEntry();
}


void MainDialog::OnMenuQuit(wxCommandEvent& event)
{
    Close();
}

//#########################################################################################################

//language selection
void MainDialog::switchProgramLanguage(const int langID)
{
    //create new dialog with respect to new language
    zen::setLanguage(langID); //language is a global attribute

    const xmlAccess::XmlGuiConfig currentGuiCfg = getCurrentConfiguration();

    cleanUp(false); //destructor's code: includes updating global settings

    //create new main window and delete old one
    MainDialog* frame = new MainDialog(currentGuiCfg, *globalSettings, false);
    frame->Show();

    Destroy();
}


void MainDialog::OnMenuLanguageSwitch(wxCommandEvent& event)
{
    std::map<MenuItemID, LanguageID>::const_iterator it = languageMenuItemMap.find(event.GetId());

    if (it != languageMenuItemMap.end())
        switchProgramLanguage(it->second);
}

//#########################################################################################################

MainDialog::SyncPreview::SyncPreview(MainDialog* mainDlg) :
    mainDlg_(mainDlg),
    syncPreviewEnabled(false),
    synchronizationEnabled(false) {}


bool MainDialog::SyncPreview::previewIsEnabled() const
{
    return syncPreviewEnabled;
}


void MainDialog::SyncPreview::enablePreview(bool value)
{
    if (value)
    {
        syncPreviewEnabled = true;

        //toggle display of sync preview in middle grid
        mainDlg_->m_gridMiddle->enableSyncPreview(true);

        mainDlg_->Refresh();
    }
    else
    {
        syncPreviewEnabled = false;

        //toggle display of sync preview in middle grid
        mainDlg_->m_gridMiddle->enableSyncPreview(false);

        mainDlg_->Refresh();
    }

    mainDlg_->updateGuiGrid();
}


void MainDialog::SyncPreview::enableSynchronization(bool value)
{
    if (value)
    {
        synchronizationEnabled = true;
        mainDlg_->m_buttonStartSync->SetForegroundColour(wxSystemSettings::GetColour(wxSYS_COLOUR_BTNTEXT));
        mainDlg_->m_buttonStartSync->setBitmapFront(GlobalResources::instance().getImage(wxT("sync")));
    }
    else
    {
        synchronizationEnabled = false;
        mainDlg_->m_buttonStartSync->SetForegroundColour(wxColor(128, 128, 128)); //Some colors seem to have problems with 16Bit color depth, well this one hasn't!
        mainDlg_->m_buttonStartSync->setBitmapFront(GlobalResources::instance().getImage(wxT("syncDisabled")));
    }
}


bool MainDialog::SyncPreview::synchronizationIsEnabled() const
{
    return synchronizationEnabled;
}

