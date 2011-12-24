// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) 2008-2011 ZenJu (zhnmju123 AT gmx.de)                    *
// **************************************************************************

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
#include <wx/dcmemory.h>
#include "folder_history_box.h"
#include "../lib/custom_grid.h"
#include <wx+/button.h>
#include <wx+/dir_picker.h>
#include "../comparison.h"
#include "../synchronization.h"
#include "../algorithm.h"
#include <wx+/app_main.h>
#include <wx+/format_unit.h>
#include "check_version.h"
#include "gui_status_handler.h"
#include "sync_cfg.h"
#include <wx+/string_conv.h>
#include "small_dlgs.h"
#include <wx+/mouse_move_dlg.h>
#include "progress_indicator.h"
#include "msg_popup.h"
#include "../structures.h"
#include "grid_view.h"
#include "../lib/resources.h"
#include <zen/file_handling.h>
#include <zen/file_id.h>
#include "../lib/resolve_path.h"
#include "../lib/recycler.h"
#include "../lib/ffs_paths.h"
#include <wx+/toggle_button.h>
#include "folder_pair.h"
#include "search.h"
#include "../lib/help_provider.h"
#include "batch_config.h"
#include <zen/thread.h>
#include "../lib/lock_holder.h"
#include <wx+/shell_execute.h>
#include "../lib/localization.h"
#include <wx+/image_tools.h>
#include <wx+/no_flicker.h>
#include <wx+/grid.h>


using namespace zen;
using namespace std::rel_ops;


namespace
{
struct wxClientDataString : public wxClientData //we need a wxClientData derived class to tell wxWidgets to take object ownership!
{
    wxClientDataString(const wxString& name) : name_(name) {}

    wxString name_;
};
}


class DirectoryNameMainImpl : public DirectoryName<FolderHistoryBox>
{
public:
    DirectoryNameMainImpl(MainDialog&      mainDlg,
                          wxWindow&        dropWindow1,
                          wxWindow&        dropWindow2,
                          wxDirPickerCtrl& dirPicker,
                          FolderHistoryBox& dirName,
                          wxStaticBoxSizer& staticBox) :
        DirectoryName(dropWindow1, dirPicker, dirName, &staticBox, &dropWindow2),
        mainDlg_(mainDlg) {}

    virtual bool acceptDrop(const std::vector<wxString>& droppedFiles)
    {
        if (droppedFiles.empty())
            return true;

        switch (xmlAccess::getMergeType(droppedFiles))   //throw ()
        {
            case xmlAccess::MERGE_BATCH:
            case xmlAccess::MERGE_GUI:
            case xmlAccess::MERGE_GUI_BATCH:
                mainDlg_.loadConfiguration(droppedFiles);
                return false;

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
        return mainDlg.getConfig().mainCfg;
    }

    virtual void OnAltCompCfgChange()
    {
        mainDlg.applyCompareConfig(false); //false: not global level
    }

    virtual void OnAltSyncCfgChange()
    {
        mainDlg.applySyncConfig();
    }

    virtual void OnAltCompCfgRemoveConfirm(wxCommandEvent& event)
    {
        FolderPairPanelBasic<GuiPanel>::OnAltCompCfgRemoveConfirm(event);
        mainDlg.applyCompareConfig(false); //false: not global level
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

    void setValues(const wxString& leftDir,
                   const wxString& rightDir,
                   AltCompCfgPtr cmpCfg,
                   AltSyncCfgPtr syncCfg,
                   const FilterConfig& filter)
    {
        setConfig(cmpCfg, syncCfg, filter);
        dirNameLeft.setName(leftDir);
        dirNameRight.setName(rightDir);
    }
    wxString getLeftDir () const { return dirNameLeft .getName(); }
    wxString getRightDir() const { return dirNameRight.getName(); }

private:
    //support for drag and drop
    DirectoryName<FolderHistoryBox> dirNameLeft;
    DirectoryName<FolderHistoryBox> dirNameRight;
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

    void setValues(const wxString& leftDir,
                   const wxString& rightDir,
                   AltCompCfgPtr cmpCfg,
                   AltSyncCfgPtr syncCfg,
                   const FilterConfig& filter)
    {
        setConfig(cmpCfg, syncCfg, filter);
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
    MenuItemUpdater(wxMenu& menuToUpdate) : menuToUpdate_(menuToUpdate) {}

    ~MenuItemUpdater()
    {
        wxMenuItemList allItems = menuToUpdate_.GetMenuItems();

        //retrieve menu item positions: unfortunately wxMenu doesn't offer a better way
        int index = 0;
        for (auto itemIter = allItems.begin(); itemIter != allItems.end(); ++itemIter, ++index) //wxMenuItemList + std::for_each screws up with VS2010!
        {
            wxMenuItem* item = *itemIter;

            auto iter = menuItems.find(item);
            if (iter != menuItems.end())
            {
                /*
                    menuToUpdate_.Remove(item);        ->this simple sequence crashes on Kubuntu x64, wxWidgets 2.9.2
                    menuToUpdate_.Insert(index, item);
                    */

                const wxBitmap& bmp = iter->second;

                wxMenuItem* newItem = new wxMenuItem(&menuToUpdate_, item->GetId(), item->GetItemLabel());
                newItem->SetBitmap(bmp);

                menuToUpdate_.Destroy(item);          //actual workaround
                menuToUpdate_.Insert(index, newItem); //
            }
        }
    }

    void markForUpdate(wxMenuItem* newEntry, const wxBitmap& bmp)
    {
        menuItems.insert(std::make_pair(newEntry, bmp));
    }

private:
    wxMenu& menuToUpdate_;
    std::map<wxMenuItem*, wxBitmap> menuItems;
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
MainDialog::MainDialog(const std::vector<wxString>& cfgFileNames, xmlAccess::XmlGlobalSettings& settings) :
    MainDialogGenerated(NULL)
{
    xmlAccess::XmlGuiConfig guiCfg;  //structure to receive gui settings, already defaulted!!

    std::vector<wxString> filenames;
    if (!cfgFileNames.empty()) //1. this one has priority
        filenames = cfgFileNames;
    else //next: use last used selection
    {
        filenames = settings.gui.lastUsedConfigFiles; //2. now try last used files

        //------------------------------------------------------------------------------------------
        //check existence of all directories in parallel!
        std::list<boost::unique_future<bool>> fileEx;

        std::for_each(filenames.begin(), filenames.end(),
                      [&fileEx](const wxString& filename)
        {
            const Zstring filenameFmt = toZ(filename); //convert to Zstring first: we don't want to pass wxString by value and risk MT issues!
            fileEx.push_back(zen::async2<bool>([=]() { return !filenameFmt.empty() && zen::fileExists(filenameFmt); }));
        });
        //potentially slow network access: give all checks 500ms to finish
        wait_for_all_timed(fileEx.begin(), fileEx.end(), boost::posix_time::milliseconds(500));
        //------------------------------------------------------------------------------------------

        //check if one of the files is not existing (this shall not be an error!)
        const bool allFilesExist = std::find_if(fileEx.begin(), fileEx.end(), [](boost::unique_future<bool>& ft) { return !ft.is_ready() || !ft.get(); }) == fileEx.end();
        if (!allFilesExist)
            filenames.clear();

        if (filenames.empty())
        {
            if (zen::fileExists(zen::toZ(lastRunConfigName()))) //3. try to load auto-save config
                filenames.push_back(lastRunConfigName());
        }
    }

    bool loadCfgSuccess = false;
    if (!filenames.empty())
        try
        {
            //load XML
            xmlAccess::convertConfig(filenames, guiCfg); //throw (xmlAccess::FfsXmlError)

            loadCfgSuccess = true;
        }
        catch (const xmlAccess::FfsXmlError& error)
        {
            if (error.getSeverity() == xmlAccess::FfsXmlError::WARNING)
                wxMessageBox(error.toString(), _("Warning"), wxOK | wxICON_WARNING);
            else
                wxMessageBox(error.toString(), _("Error"), wxOK | wxICON_ERROR);
        }
    const bool startComparisonImmediately = !cfgFileNames.empty() && loadCfgSuccess;

    init(guiCfg,
         settings,
         startComparisonImmediately);

    setLastUsedConfig(filenames, loadCfgSuccess ? guiCfg : xmlAccess::XmlGuiConfig()); //simulate changed config on parsing errors
}


MainDialog::MainDialog(const std::vector<wxString>& referenceFiles,
                       const xmlAccess::XmlGuiConfig& guiCfg,
                       xmlAccess::XmlGlobalSettings& settings,
                       bool startComparison) :
    MainDialogGenerated(NULL)
{
    init(guiCfg,
         settings,
         startComparison);

    setLastUsedConfig(referenceFiles, guiCfg);
}


MainDialog::~MainDialog()
{
    writeGlobalSettings(); //set before saving last used config since "activeConfigFiles" will be replaced

    //save "LastRun.ffs_gui" configuration
    const xmlAccess::XmlGuiConfig guiCfg = getConfig();
    try
    {
        xmlAccess::writeConfig(guiCfg, lastRunConfigName());
        //setLastUsedConfig(lastRunConfigName(), guiCfg); -> may be removed!?
    }
    //don't annoy users on read-only drives: no error checking should be fine since this is not a config the user explicitly wanted to save
    catch (const xmlAccess::FfsXmlError&) {}

    //important! event source wxTheApp is NOT dependent on this instance -> disconnect!
    wxTheApp->Disconnect(wxEVT_KEY_DOWN,  wxKeyEventHandler(MainDialog::OnGlobalKeyEvent), NULL, this);
    wxTheApp->Disconnect(wxEVT_CHAR_HOOK, wxKeyEventHandler(MainDialog::OnGlobalKeyEvent), NULL, this);

    //no need for wxEventHandler::Disconnect() here; event sources are components of this window and are destroyed, too

    m_gridLeft  ->release(); //handle wxGrid-related callback on grid data after MainDialog has died... (Linux only)
    m_gridMiddle->release();
    m_gridRight ->release();

    auiMgr.UnInit();
}


void MainDialog::onQueryEndSession()
{
    writeGlobalSettings();
    try { xmlAccess::writeConfig(getConfig(), lastRunConfigName()); }
    catch (const xmlAccess::FfsXmlError&) {}
}


void MainDialog::init(const xmlAccess::XmlGuiConfig& guiCfg,
                      xmlAccess::XmlGlobalSettings& settings,
                      bool startComparison)
{
    folderHistoryLeft  = std::make_shared<FolderHistory>();  //make sure it is always bound
    folderHistoryRight = std::make_shared<FolderHistory>(); //

    m_directoryLeft ->init(folderHistoryLeft);
    m_directoryRight->init(folderHistoryRight);

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
                   wxAuiPaneInfo().Name(wxT("Panel5")).Bottom().Row(1).Position(1).Caption(_("Filter files")).MinSize(m_bpButtonFilter->GetSize().GetWidth(), m_panelFilter->GetSize().GetHeight()));

    auiMgr.AddPane(m_panelViewFilter,
                   wxAuiPaneInfo().Name(wxT("Panel6")).Bottom().Row(1).Position(2).Caption(_("Select view")).MinSize(m_bpButtonSyncDirNone->GetSize().GetWidth(), m_panelViewFilter->GetSize().GetHeight()));

    auiMgr.AddPane(m_panelStatistics,
                   wxAuiPaneInfo().Name(wxT("Panel7")).Bottom().Row(1).Position(3).Caption(_("Statistics")).MinSize(m_panelStatistics->GetSize().GetWidth() / 2, m_panelStatistics->GetSize().GetHeight()));

    auiMgr.AddPane(m_panelStatusBar,
                   wxAuiPaneInfo().Name(wxT("Panel8")).Bottom().Row(0).Layer(4).CaptionVisible(false).PaneBorder(false).DockFixed());

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

    //register context: quick variant selection
    m_bpButtonCmpConfig ->Connect(wxEVT_RIGHT_DOWN, wxMouseEventHandler(MainDialog::OnContextSelectCompVariant), NULL, this);
    m_bpButtonSyncConfig->Connect(wxEVT_RIGHT_DOWN, wxMouseEventHandler(MainDialog::OnContextSelectSyncVariant), NULL, this);


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

    SetIcon(GlobalResources::instance().programIcon); //set application icon

    //notify about (logical) application main window => program won't quit, but stay on this dialog
    zen::setMainWindow(this);

    //init handling of first folder pair
    firstFolderPair.reset(new DirectoryPairFirst(*this));

    initViewFilterButtons();

    //initialize and load configuration
    readGlobalSettings();
    setConfig(guiCfg);

    //set icons for this dialog
    m_buttonCompare     ->setBitmapFront(GlobalResources::getImage(wxT("compare")));
    m_bpButtonSyncConfig->SetBitmapLabel(GlobalResources::getImage(wxT("syncConfig")));
    m_bpButtonCmpConfig ->SetBitmapLabel(GlobalResources::getImage(wxT("cmpConfig")));
    m_bpButtonSave      ->SetBitmapLabel(GlobalResources::getImage(wxT("save")));
    m_bpButtonLoad      ->SetBitmapLabel(GlobalResources::getImage(wxT("load")));
    m_bpButtonAddPair   ->SetBitmapLabel(GlobalResources::getImage(wxT("addFolderPair")));
    m_bitmap15          ->SetBitmap(GlobalResources::getImage(wxT("statusEdge")));
    {
        IconBuffer tmp(IconBuffer::SIZE_SMALL);
        const wxBitmap bmpFile = tmp.genericFileIcon();
        const wxBitmap bmpDir  = tmp.genericDirIcon();

        m_bitmapSmallDirectoryLeft ->SetBitmap(bmpDir);
        m_bitmapSmallFileLeft      ->SetBitmap(bmpFile);
        m_bitmapSmallDirectoryRight->SetBitmap(bmpDir);
        m_bitmapSmallFileRight     ->SetBitmap(bmpFile);
    }

    m_bitmapCreate->SetBitmap(GlobalResources::getImage(wxT("create")));
    m_bitmapUpdate->SetBitmap(GlobalResources::getImage(wxT("update")));
    m_bitmapDelete->SetBitmap(GlobalResources::getImage(wxT("delete")));
    m_bitmapData  ->SetBitmap(GlobalResources::getImage(wxT("data")));

    m_panelTopButtons->Layout(); //wxButtonWithImage size might have changed

    //menu icons: workaround for wxWidgets: small hack to update menu items: actually this is a wxWidgets bug (affects Windows- and Linux-build)
    MenuItemUpdater updateMenuFile(*m_menuFile);
    updateMenuFile.markForUpdate(m_menuItem10,   GlobalResources::getImage(wxT("compareSmall")));
    updateMenuFile.markForUpdate(m_menuItem11,   GlobalResources::getImage(wxT("syncSmall")));
    updateMenuFile.markForUpdate(m_menuItemNew,  GlobalResources::getImage(wxT("newSmall")));
    updateMenuFile.markForUpdate(m_menuItemSave, GlobalResources::getImage(wxT("saveSmall")));
    updateMenuFile.markForUpdate(m_menuItemLoad, GlobalResources::getImage(wxT("loadSmall")));

    MenuItemUpdater updateMenuAdv(*m_menuAdvanced);
    updateMenuAdv.markForUpdate(m_menuItemGlobSett, GlobalResources::getImage(wxT("settingsSmall")));
    updateMenuAdv.markForUpdate(m_menuItem7, GlobalResources::getImage(wxT("batchSmall")));

    MenuItemUpdater updateMenuHelp(*m_menuHelp);
    updateMenuHelp.markForUpdate(m_menuItemAbout, GlobalResources::getImage(wxT("aboutSmall")));

#ifdef FFS_LINUX
    if (!zen::isPortableVersion()) //disable update check for Linux installer-based version -> handled by .deb
        m_menuItemCheckVer->Enable(false);
#endif

    //create language selection menu
    std::for_each(zen::ExistingTranslations::get().begin(), zen::ExistingTranslations::get().end(),
                  [&](const zen::ExistingTranslations::Entry& entry)
    {
        wxMenuItem* newItem = new wxMenuItem(m_menuLanguages, wxID_ANY, entry.languageName, wxEmptyString, wxITEM_NORMAL );
        newItem->SetBitmap(GlobalResources::getImage(entry.languageFlag));

        //map menu item IDs with language IDs: evaluated when processing event handler
        languageMenuItemMap.insert(std::map<MenuItemID, LanguageID>::value_type(newItem->GetId(), entry.languageID));

        //connect event
        this->Connect(newItem->GetId(), wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(MainDialog::OnMenuLanguageSwitch));
        m_menuLanguages->Append(newItem);
    });

    //support for CTRL + C and DEL on grids
    m_gridLeft  ->Connect(wxEVT_KEY_DOWN, wxKeyEventHandler(MainDialog::onGridLeftButtonEvent),   NULL, this);
    m_gridRight ->Connect(wxEVT_KEY_DOWN, wxKeyEventHandler(MainDialog::onGridRightButtonEvent),  NULL, this);
    m_gridMiddle->Connect(wxEVT_KEY_DOWN, wxKeyEventHandler(MainDialog::onGridMiddleButtonEvent), NULL, this);

    //register global hotkeys (without explicit menu entry)
    wxTheApp->Connect(wxEVT_KEY_DOWN,  wxKeyEventHandler(MainDialog::OnGlobalKeyEvent), NULL, this);
    wxTheApp->Connect(wxEVT_CHAR_HOOK, wxKeyEventHandler(MainDialog::OnGlobalKeyEvent), NULL, this); //capture direction keys


    Connect(wxEVT_IDLE, wxEventHandler(MainDialog::OnIdleEvent), NULL, this);

    Connect(wxEVT_SIZE, wxSizeEventHandler(MainDialog::OnResize), NULL, this);
    Connect(wxEVT_MOVE, wxSizeEventHandler(MainDialog::OnResize), NULL, this);

    m_bpButtonFilter->Connect(wxEVT_RIGHT_DOWN, wxCommandEventHandler(MainDialog::OnGlobalFilterOpenContext), NULL, this);

    //calculate witdh of folder pair manually (if scrollbars are visible)
    m_panelTopLeft->Connect(wxEVT_SIZE, wxEventHandler(MainDialog::OnResizeFolderPairs), NULL, this);

    //dynamically change sizer direction depending on size
    m_panelConfig    ->Connect(wxEVT_SIZE, wxEventHandler(MainDialog::OnResizeConfigPanel),     NULL, this);
    m_panelViewFilter->Connect(wxEVT_SIZE, wxEventHandler(MainDialog::OnResizeViewPanel),       NULL, this);
    m_panelStatistics->Connect(wxEVT_SIZE, wxEventHandler(MainDialog::OnResizeStatisticsPanel), NULL, this);
    wxSizeEvent dummy3;
    OnResizeConfigPanel    (dummy3); //call once on window creation
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
    wxCommandEvent evtDummy;       //call once before OnLayoutWindowAsync()
    OnResizeFolderPairs(evtDummy); //

    //----------------------------------------------------------------------------------------------------------------------------------------------------------------
    //some convenience: if FFS is started with a *.ffs_gui file as commandline parameter AND all directories contained exist, comparison shall be started right away
    if (startComparison)
    {
        const zen::MainConfiguration currMainCfg = getConfig().mainCfg;

        //------------------------------------------------------------------------------------------
        //check existence of all directories in parallel!
        std::list<boost::unique_future<bool>> dirEx;

        auto addDirCheck = [&dirEx](const FolderPairEnh& fp)
        {
            const Zstring dirFmtLeft  = zen::getFormattedDirectoryName(fp.leftDirectory);
            const Zstring dirFmtRight = zen::getFormattedDirectoryName(fp.rightDirectory);

            if (dirFmtLeft.empty() && dirFmtRight.empty()) //only skip check if both sides are empty!
                return;

            dirEx.push_back(zen::async2<bool>([=] { return !dirFmtLeft .empty() && zen::dirExists(dirFmtLeft); }));
            dirEx.push_back(zen::async2<bool>([=] { return !dirFmtRight.empty() && zen::dirExists(dirFmtRight); }));
        };
        addDirCheck(currMainCfg.firstPair);
        std::for_each(currMainCfg.additionalPairs.begin(), currMainCfg.additionalPairs.end(), addDirCheck);
        if (!dirEx.empty())
        {
            //potentially slow network access: give all checks 500ms to finish
            wait_for_all_timed(dirEx.begin(), dirEx.end(), boost::posix_time::milliseconds(500));
            //------------------------------------------------------------------------------------------

            const bool allFoldersExist = std::find_if(dirEx.begin(), dirEx.end(), [](boost::unique_future<bool>& ft) { return !ft.is_ready() || !ft.get(); }) == dirEx.end();

            if (allFoldersExist)
            {
                wxCommandEvent dummy2(wxEVT_COMMAND_BUTTON_CLICKED);
                m_buttonCompare->GetEventHandler()->AddPendingEvent(dummy2); //simulate button click on "compare"
            }
        }
    }
    //----------------------------------------------------------------------------------------------------------------------------------------------------------------
}


void MainDialog::readGlobalSettings()
{
    //apply window size and position at program startup ONLY
    //apply window size and position
    if (globalSettings->gui.dlgSize.GetWidth () != wxDefaultCoord &&
        globalSettings->gui.dlgSize.GetHeight() != wxDefaultCoord &&
        globalSettings->gui.dlgPos.x != wxDefaultCoord &&
        globalSettings->gui.dlgPos.y != wxDefaultCoord &&
        wxDisplay::GetFromPoint(globalSettings->gui.dlgPos) != wxNOT_FOUND) //make sure upper left corner is in visible view
        SetSize(wxRect(globalSettings->gui.dlgPos, globalSettings->gui.dlgSize));
    else
        Centre();

    Maximize(globalSettings->gui.isMaximized);


    //set column attributes
    m_gridLeft ->setColumnAttributes(globalSettings->gui.columnAttribLeft);
    m_gridRight->setColumnAttributes(globalSettings->gui.columnAttribRight);

    //load list of last used configuration files
    std::vector<wxString> cfgFileNames = globalSettings->gui.cfgFileHistory;
    cfgFileNames.push_back(lastRunConfigName()); //make sure <Last session> is always part of history list
    addFileToCfgHistory(cfgFileNames);

    //load list of last used folders
    *folderHistoryLeft  = FolderHistory(globalSettings->gui.folderHistoryLeft,  globalSettings->gui.folderHistMax);
    *folderHistoryRight = FolderHistory(globalSettings->gui.folderHistoryRight, globalSettings->gui.folderHistMax);

    //show/hide file icons
    const IconBuffer::IconSize sz = [&]() -> IconBuffer::IconSize
    {
        switch (globalSettings->gui.iconSize)
        {
            case xmlAccess::ICON_SIZE_SMALL:
                return IconBuffer::SIZE_SMALL;
            case xmlAccess::ICON_SIZE_MEDIUM:
                return IconBuffer::SIZE_MEDIUM;
            case xmlAccess::ICON_SIZE_LARGE:
                return IconBuffer::SIZE_LARGE;
        }
        return IconBuffer::SIZE_SMALL;
    }();

    std::shared_ptr<IconBuffer> iconBuffer = std::make_shared<IconBuffer>(sz);
    m_gridLeft  ->setIconManager(iconBuffer);
    m_gridMiddle->setIconManager(iconBuffer);
    m_gridRight ->setIconManager(iconBuffer);

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
    globalSettings->gui.isMaximized = IsMaximized();

    //retrieve column attributes
    globalSettings->gui.columnAttribLeft  = m_gridLeft->getColumnAttributes();
    globalSettings->gui.columnAttribRight = m_gridRight->getColumnAttributes();

    //write list of last used configuration files
    std::vector<wxString> cfgFileHistory;
    for (int i = 0; i < static_cast<int>(m_listBoxHistory->GetCount()); ++i)
        if (m_listBoxHistory->GetClientObject(i))
            cfgFileHistory.push_back(static_cast<wxClientDataString*>(m_listBoxHistory->GetClientObject(i))->name_);

    globalSettings->gui.cfgFileHistory      = cfgFileHistory;
    globalSettings->gui.lastUsedConfigFiles = activeConfigFiles;

    //write list of last used folders
    globalSettings->gui.folderHistoryLeft  = folderHistoryLeft ->getList();
    globalSettings->gui.folderHistoryRight = folderHistoryRight->getList();

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

namespace
{
//fast replacement for wxString modelling exponential growth
typedef Zbase<wchar_t> zxString; //for use with UI texts
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
                clipboardString += copyStringTo<zxString>(selectedGrid.GetCellValue(static_cast<int>(*i), k));
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
                wxTheClipboard->SetData(new wxTextDataObject(copyStringTo<wxString>(clipboardString)));
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


//Exception class used to abort the "compare" and "sync" process
class AbortDeleteProcess {};

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
        mainDlg->m_buttonAbort->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( ManualDeletionHandler::OnAbortDeletion), NULL, this );
        mainDlg->Connect(wxEVT_CHAR_HOOK, wxKeyEventHandler(ManualDeletionHandler::OnKeyPressed), NULL, this);
    }

    ~ManualDeletionHandler()
    {
        //de-register abort button
        mainDlg->Disconnect(wxEVT_CHAR_HOOK, wxKeyEventHandler(ManualDeletionHandler::OnKeyPressed), NULL, this);
        mainDlg->m_buttonAbort->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( ManualDeletionHandler::OnAbortDeletion ), NULL, this );

        mainDlg->enableAllElements();
    }

    virtual Response reportError(const std::wstring& errorMessage)
    {
        if (abortRequested)
            throw AbortDeleteProcess();

        if (ignoreErrors)
            return DeleteFilesHandler::IGNORE_ERROR;

        bool ignoreNextErrors = false;
        switch (showErrorDlg(ReturnErrorDlg::BUTTON_IGNORE | ReturnErrorDlg::BUTTON_RETRY | ReturnErrorDlg::BUTTON_ABORT,
                             errorMessage, &ignoreNextErrors))
        {
            case ReturnErrorDlg::BUTTON_IGNORE:
                ignoreErrors = ignoreNextErrors;
                return DeleteFilesHandler::IGNORE_ERROR;
            case ReturnErrorDlg::BUTTON_RETRY:
                return DeleteFilesHandler::RETRY;
            case ReturnErrorDlg::BUTTON_ABORT:
                throw AbortDeleteProcess();
        }

        assert (false);
        return DeleteFilesHandler::IGNORE_ERROR; //dummy return value
    }

    virtual void notifyDeletion(const Zstring& currentObject)  //called for each file/folder that has been deleted
    {
        ++deletionCount;

        if (updateUiIsAllowed())  //test if specific time span between ui updates is over
        {
            wxString statusMessage = replaceCpy(_P("Object deleted successfully!", "%x objects deleted successfully!", deletionCount),
                                                L"%x", zen::toStringSep(deletionCount), false);

            if (mainDlg->m_staticTextStatusMiddle->GetLabel() != statusMessage)
            {
                mainDlg->m_staticTextStatusMiddle->SetLabel(statusMessage);
                mainDlg->m_panelStatusBar->Layout();
            }
            updateUiNow();
        }

        if (abortRequested) //test after (implicit) call to wxApp::Yield()
            throw AbortDeleteProcess();
    }

private:
    void OnAbortDeletion(wxCommandEvent& event) //handle abort button click
    {
        abortRequested = true; //don't throw exceptions in a GUI-Callback!!! (throw zen::AbortThisProcess())
    }

    void OnKeyPressed(wxKeyEvent& event)
    {
        const int keyCode = event.GetKeyCode();
        if (keyCode == WXK_ESCAPE)
        {
            abortRequested = true; //don't throw exceptions in a GUI-Callback!!! (throw zen::AbortThisProcess())
            return;
        }

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
            try
            {
                //handle errors when deleting files/folders
                ManualDeletionHandler statusHandler(this);

                zen::deleteFromGridAndHD(compRefLeft,
                                         compRefRight,
                                         gridDataView->getDataTentative(),
                                         extractDirectionCfg(getConfig().mainCfg),
                                         globalSettings->gui.deleteOnBothSides,
                                         globalSettings->gui.useRecyclerForManualDeletion,
                                         statusHandler);
            }
            catch (AbortDeleteProcess&) {}

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


template <SelectedSide side>
wxString extractLastValidDir(const FileSystemObject& fsObj)
{
    Zstring fullname = fsObj.getBaseDirPf<side>() + fsObj.getObjRelativeName(); //full name even if FileSystemObject::isEmpty<side>() == true

    while (!fullname.empty() && !dirExists(fullname)) //bad algorithm: this one should better retrieve the status from fsObj
        fullname = beforeLast(fullname, FILE_NAME_SEPARATOR);

    return toWx(fullname);
}


void MainDialog::openExternalApplication(size_t rowNumber, bool leftSide, const wxString& commandline)
{
    if (commandline.empty())
        return;

    wxString name;
    wxString nameCo;
    wxString dir;
    wxString dirCo;

    {
        const FileSystemObject* fsObj = gridDataView->getObject(rowNumber);
        if (fsObj)
        {
            name = toWx(fsObj->getFullName<LEFT_SIDE>()); //empty if obj not existing
            dir  = toWx(beforeLast(fsObj->getFullName<LEFT_SIDE>(), FILE_NAME_SEPARATOR)); //small issue: if obj does not exist but parent exists, this one erronously returns empty

            nameCo = toWx(fsObj->getFullName<RIGHT_SIDE>());
            dirCo  = toWx(beforeLast(fsObj->getFullName<RIGHT_SIDE>(), FILE_NAME_SEPARATOR));
        }
    }

    if (!leftSide)
    {
        std::swap(name, nameCo);
        std::swap(dir,  dirCo);
    }

    wxString command = commandline;

    auto tryReplace = [&](const wxString& phrase, const wxString& replacement) -> bool
    {
        if (command.find(phrase) != wxString::npos)
        {
            replace(command, phrase, replacement);
            if (replacement.empty())
                return false;
        }
        return true;
    };

    bool expandSuccess =
        /**/	        tryReplace(L"%nameCo", nameCo); //attention: replace %nameCo, %dirCo BEFORE %name, %dir to handle dependency
    expandSuccess = tryReplace(L"%dirCo",  dirCo ) && expandSuccess; //
    expandSuccess = tryReplace(L"%name",   name  ) && expandSuccess; //prevent short-cut behavior!
    expandSuccess = tryReplace(L"%dir",    dir   ) && expandSuccess; //

    const bool openFileBrowser = [&]() -> bool
    {
        xmlAccess::XmlGlobalSettings::Gui dummy;
        return !dummy.externelApplications.empty() && dummy.externelApplications[0].second == commandline;
    }();

    if (!openFileBrowser || expandSuccess)
        zen::shellExecute(command); //just execute, show error message if command is malformed
    else //support built-in fallback!
    {
        wxString fallbackDir;
        const FileSystemObject* fsObj = gridDataView->getObject(rowNumber);
        if (fsObj)
            fallbackDir = leftSide ?
                          extractLastValidDir<LEFT_SIDE >(*fsObj) :
                          extractLastValidDir<RIGHT_SIDE>(*fsObj);

        if (fallbackDir.empty())
            fallbackDir = leftSide ?
                          toWx(zen::getFormattedDirectoryName(toZ(firstFolderPair->getLeftDir()))) :
                          toWx(zen::getFormattedDirectoryName(toZ(firstFolderPair->getRightDir())));

#ifdef FFS_WIN
        zen::shellExecute(wxString(L"\"") + fallbackDir + L"\"");
#elif defined FFS_LINUX
        zen::shellExecute(wxString(L"xdg-open \"") + fallbackDir + L"\"");
#endif
    }
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
    while (!stackObjects.empty())
        stackObjects.pop();

    m_staticTextStatusMiddle->SetForegroundColour(wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOWTEXT)); //reset color
    bSizerStatusLeftDirectories->Show(false);
    bSizerStatusLeftFiles      ->Show(false);

    m_staticTextStatusMiddle->SetLabel(wxEmptyString);

    bSizerStatusRightDirectories->Show(false);
    bSizerStatusRightFiles      ->Show(false);
}


void MainDialog::disableAllElements(bool enableAbort)
{
    //when changing consider: comparison,  synchronization, manual deletion

    EnableCloseButton(false); //not allowed for synchronization! progress indicator is top window!

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
        if (m_buttonAbort->IsShownOnScreen()) m_buttonAbort->SetFocus();
        m_buttonCompare->Disable();
        m_buttonCompare->Hide();
        m_panelTopButtons->Layout();
    }
    else
        m_panelTopButtons->Disable();
}


void MainDialog::enableAllElements()
{
    //wxGTK, yet another QOI issue: some stupid bug, keeps moving main dialog to top!!

    EnableCloseButton(true);

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

    m_panelTopButtons->Layout();
    m_panelTopButtons->Enable();
}


void MainDialog::OnResize(wxSizeEvent& event)
{
    if (!IsMaximized())
    {
        wxSize sz  = GetSize();
        wxPoint ps = GetPosition();

        //test ALL parameters at once, since width/height are invalid if the window is minimized (eg x,y == -32000; height = 28, width = 160)
        //note: negative values for x and y are possible when using multiple monitors!
        if (sz.GetWidth() > 0 && sz.GetHeight() > 0 && ps.x >= -3360 && ps.y >= -200 &&
            wxDisplay::GetFromPoint(ps) != wxNOT_FOUND) //make sure upper left corner is in visible view
        {
            globalSettings->gui.dlgSize = sz;
            globalSettings->gui.dlgPos  = ps;
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
    std::for_each(additionalFolderPairs.begin(), additionalFolderPairs.end(),
                  [&](DirectoryPair* dirPair)
    {
        dirPair->m_panelLeft->SetMinSize(wxSize(width, -1));

    });

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

    processingGlobalKeyEvent = true;
    ZEN_ON_BLOCK_EXIT(processingGlobalKeyEvent = false;)
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


struct CtxtSelectionIconSize : public wxObject
{
    CtxtSelectionIconSize(xmlAccess::FileIconSize sz) : iconSize(sz) {}
    xmlAccess::FileIconSize iconSize;
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
                                                             wxT(" <-") + wxT("\tAlt - Left")); //Linux needs a direction, "<-", because it has no context menu icons!
            menuItemSyncDirLeft->SetBitmap(getSyncOpImage(fsObj->testSyncOperation(SYNC_DIR_LEFT, true)));
            contextMenu->Append(menuItemSyncDirLeft);
            contextMenu->Connect(menuItemSyncDirLeft->GetId(),  wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(MainDialog::OnContextSyncDirLeft),      NULL, this);

            //CONTEXT_SYNC_DIR_NONE
            wxMenuItem* menuItemSyncDirNone = new wxMenuItem(contextMenu.get(), wxID_ANY, wxString(_("Set direction:")) +
                                                             wxT(" -") + wxT("\tAlt - Up"));
            menuItemSyncDirNone->SetBitmap(getSyncOpImage(fsObj->testSyncOperation(SYNC_DIR_NONE, true)));
            contextMenu->Append(menuItemSyncDirNone);
            contextMenu->Connect(menuItemSyncDirNone->GetId(),  wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(MainDialog::OnContextSyncDirNone),      NULL, this);

            //CONTEXT_SYNC_DIR_RIGHT
            wxMenuItem* menuItemSyncDirRight = new wxMenuItem(contextMenu.get(), wxID_ANY, wxString(_("Set direction:")) +
                                                              wxT(" ->") + wxT("\tAlt - Right"));
            menuItemSyncDirRight->SetBitmap(getSyncOpImage(fsObj->testSyncOperation(SYNC_DIR_RIGHT, true)));
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
            menuItemInExcl->SetBitmap(GlobalResources::getImage(wxT("checkboxFalse")));
        }
        else
        {
            menuItemInExcl = new wxMenuItem(contextMenu.get(), wxID_ANY, wxString(_("Include temporarily")) + wxT("\tSpace"));
            menuItemInExcl->SetBitmap(GlobalResources::getImage(wxT("checkboxTrue")));
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
        const Zstring filename = afterLast(exFilterCandidateObj.begin()->first, FILE_NAME_SEPARATOR);
        if (filename.find(Zchar('.')) !=  Zstring::npos) //be careful: AfterLast would return the whole string if '.' were not found!
        {
            const Zstring extension = afterLast(filename, Zchar('.'));

            //add context menu item
            wxMenuItem* menuItemExclExt = new wxMenuItem(contextMenu.get(), wxID_ANY, _("Exclude via filter:") + L" *." + extension);
            menuItemExclExt->SetBitmap(GlobalResources::getImage(wxT("filterSmall")));
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
        menuItemExclObj = new wxMenuItem(contextMenu.get(), wxID_ANY, _("Exclude via filter:") + L" " + afterLast(exFilterCandidateObj.begin()->first, FILE_NAME_SEPARATOR));
    else if (exFilterCandidateObj.size() > 1)
        menuItemExclObj = new wxMenuItem(contextMenu.get(), wxID_ANY, _("Exclude via filter:") + L" " + _("<multiple selection>"));

    if (menuItemExclObj != NULL)
    {
        menuItemExclObj->SetBitmap(GlobalResources::getImage(wxT("filterSmall")));
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
            wxString description = zen::implementation::translate(std::wstring(i->first.c_str()));
            if (description.empty())
                description = wxT(" "); //wxWidgets doesn't like empty items

            wxMenuItem* itemExtApp = NULL;
            if (i == globalSettings->gui.externelApplications.begin())
                itemExtApp = contextMenu->Append(wxID_ANY, description + wxT("\t") + wxString(_("D-Click")) + wxT(" Enter"));
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

    //CONTEXT_DELETE_FILES
    wxMenuItem* menuItemDelFiles = contextMenu->Append(wxID_ANY, wxString(_("Delete")) + wxT("\tDel"));
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
        if (!excludeFilter.empty() && !endsWith(excludeFilter, Zstr(";")))
            excludeFilter += Zstr("\n");
        excludeFilter += newExclude + Zstr(";"); //';' is appended to 'mark' that next exclude extension entry won't write to new line

        updateFilterButtons();

        //do not fully apply filter, just exclude new items
        std::for_each(begin(gridDataView->getDataTentative()), end(gridDataView->getDataTentative()),
        [&](BaseDirMapping& baseMap) { addHardFiltering(baseMap, newExclude); });

        //applyFiltering(getConfig().mainCfg, gridDataView->getDataTentative());
        updateGuiGrid();

        if (currentCfg.hideFilteredElements)
        {
            m_gridLeft  ->ClearSelection();
            m_gridRight ->ClearSelection();
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

                newExclude += FILE_NAME_SEPARATOR + i->first;
                if (i->second) //is directory
                    newExclude += FILE_NAME_SEPARATOR;
            }

            //add to filter config
            Zstring& excludeFilter = currentCfg.mainCfg.globalFilter.excludeFilter;
            if (!excludeFilter.empty() && !endsWith(excludeFilter, Zstr("\n")))
                excludeFilter += Zstr("\n");
            excludeFilter += newExclude;

            updateFilterButtons();

            //do not fully apply filter, just exclude new items
            std::for_each(begin(gridDataView->getDataTentative()), end(gridDataView->getDataTentative()),
            [&](BaseDirMapping& baseMap) { addHardFiltering(baseMap, newExclude); });

            //applyFiltering(getConfig().mainCfg, gridDataView->getDataTentative());
            updateGuiGrid();

            if (currentCfg.hideFilteredElements)
            {
                m_gridLeft  ->ClearSelection();
                m_gridRight ->ClearSelection();
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
    contextMenu.reset(new wxMenu); //re-create context menu

    wxMenuItem* menuItemCustomize = new wxMenuItem(contextMenu.get(), wxID_ANY, _("Customize..."));
    contextMenu->Append(menuItemCustomize);
    contextMenu->Connect(menuItemCustomize->GetId(), wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(MainDialog::OnContextCustColumnLeft), NULL, this);

    if (m_gridLeft->getTypeAtPos(event.GetCol()) == xmlAccess::DATE)
    {
        wxMenuItem* menuItemSelectTs = new wxMenuItem(contextMenu.get(), wxID_ANY, _("Select time span..."));
        contextMenu->Append(menuItemSelectTs);
        contextMenu->Connect(menuItemSelectTs->GetId(), wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(MainDialog::OnContextSelectTimeSpan), NULL, this);
    }

    contextMenu->AppendSeparator();

    wxMenuItem* itemAutoAdjust = new wxMenuItem(contextMenu.get(), wxID_ANY, _("Auto-adjust columns"), wxEmptyString, wxITEM_CHECK);
    contextMenu->Append(itemAutoAdjust);
    itemAutoAdjust->Check(globalSettings->gui.autoAdjustColumnsLeft);
    contextMenu->Connect(itemAutoAdjust->GetId(), wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(MainDialog::OnContextAutoAdjustLeft), NULL, this);

    //if (m_gridLeft->getTypeAtPos(event.GetCol()) == xmlAccess::FILENAME)
    {
        contextMenu->AppendSeparator();

        wxMenuItem* header = contextMenu->Append(wxID_ANY, _("Icon size:"));
        header->Enable(false);

        auto addSizeEntry = [&](const wxString& label, xmlAccess::FileIconSize sz)
        {
            wxMenuItem* newItem = contextMenu->Append(wxID_ANY, label, wxEmptyString, wxITEM_RADIO);
            contextMenu->Connect(newItem->GetId(), wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(MainDialog::OnContextSetIconSize),
                                 new CtxtSelectionIconSize(sz), //ownership passed!
                                 this);

            if (globalSettings->gui.iconSize == sz)
                newItem->Check();
        };
        addSizeEntry(_("Small" ), xmlAccess::ICON_SIZE_SMALL);
        addSizeEntry(_("Medium"), xmlAccess::ICON_SIZE_MEDIUM);
        addSizeEntry(_("Large" ), xmlAccess::ICON_SIZE_LARGE);
    }

    PopupMenu(contextMenu.get()); //show context menu
}


void MainDialog::OnContextRimLabelRight(wxGridEvent& event)
{
    contextMenu.reset(new wxMenu); //re-create context menu

    wxMenuItem* menuItemCustomize = new wxMenuItem(contextMenu.get(), wxID_ANY, _("Customize..."));
    contextMenu->Append(menuItemCustomize);
    contextMenu->Connect(menuItemCustomize->GetId(), wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(MainDialog::OnContextCustColumnRight), NULL, this);

    if (m_gridRight->getTypeAtPos(event.GetCol()) == xmlAccess::DATE)
    {
        wxMenuItem* menuItemSelectTs = new wxMenuItem(contextMenu.get(), wxID_ANY, _("Select time span..."));
        contextMenu->Append(menuItemSelectTs);
        contextMenu->Connect(menuItemSelectTs->GetId(), wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(MainDialog::OnContextSelectTimeSpan), NULL, this);
    }

    contextMenu->AppendSeparator();

    wxMenuItem* itemAutoAdjust = new wxMenuItem(contextMenu.get(), wxID_ANY, _("Auto-adjust columns"), wxEmptyString, wxITEM_CHECK);
    contextMenu->Append(itemAutoAdjust);
    itemAutoAdjust->Check(globalSettings->gui.autoAdjustColumnsRight);
    contextMenu->Connect(itemAutoAdjust->GetId(), wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(MainDialog::OnContextAutoAdjustRight), NULL, this);

    //if (m_gridRight->getTypeAtPos(event.GetCol()) == xmlAccess::FILENAME)
    {
        contextMenu->AppendSeparator();

        wxMenuItem* header = contextMenu->Append(wxID_ANY, _("Icon size:"));
        header->Enable(false);

        auto addSizeEntry = [&](const wxString& label, xmlAccess::FileIconSize sz)
        {
            wxMenuItem* newItem = contextMenu->Append(wxID_ANY, label, wxEmptyString, wxITEM_RADIO);
            contextMenu->Connect(newItem->GetId(), wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(MainDialog::OnContextSetIconSize),
                                 new CtxtSelectionIconSize(sz), //ownership passed!
                                 this);
            if (globalSettings->gui.iconSize == sz)
                newItem->Check();
        };
        addSizeEntry(_("Small" ), xmlAccess::ICON_SIZE_SMALL);
        addSizeEntry(_("Medium"), xmlAccess::ICON_SIZE_MEDIUM);
        addSizeEntry(_("Large" ), xmlAccess::ICON_SIZE_LARGE);
    }

    PopupMenu(contextMenu.get()); //show context menu
}


void MainDialog::OnContextCustColumnLeft(wxCommandEvent& event)
{
    xmlAccess::ColumnAttributes colAttr = m_gridLeft->getColumnAttributes();

    if (zen::showCustomizeColsDlg(colAttr) == ReturnSmallDlg::BUTTON_OKAY)
    {
        m_gridLeft->setColumnAttributes(colAttr);

        m_gridLeft  ->setSortMarker(CustomGrid::SortMarker(-1, CustomGrid::ASCENDING)); //hide sort direction indicator on GUI grids
        m_gridMiddle->setSortMarker(CustomGrid::SortMarker(-1, CustomGrid::ASCENDING));
        m_gridRight ->setSortMarker(CustomGrid::SortMarker(-1, CustomGrid::ASCENDING));
    }
}


void MainDialog::OnContextCustColumnRight(wxCommandEvent& event)
{
    xmlAccess::ColumnAttributes colAttr = m_gridRight->getColumnAttributes();

    if (zen::showCustomizeColsDlg(colAttr) == ReturnSmallDlg::BUTTON_OKAY)
    {
        m_gridRight->setColumnAttributes(colAttr);

        m_gridLeft  ->setSortMarker(CustomGrid::SortMarker(-1, CustomGrid::ASCENDING)); //hide sort direction indicator on GUI grids
        m_gridMiddle->setSortMarker(CustomGrid::SortMarker(-1, CustomGrid::ASCENDING));
        m_gridRight ->setSortMarker(CustomGrid::SortMarker(-1, CustomGrid::ASCENDING));
    }
}


void MainDialog::OnContextSelectTimeSpan(wxCommandEvent& event)
{
    if (showSelectTimespanDlg(manualTimeSpanFrom, manualTimeSpanTo) == ReturnSmallDlg::BUTTON_OKAY)
    {
        applyTimeSpanFilter(gridDataView->getDataTentative(), manualTimeSpanFrom, manualTimeSpanTo); //overwrite current active/inactive settings
        refreshGridAfterFilterChange(400);
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


void MainDialog::OnContextSetIconSize(wxCommandEvent& event)
{
    CtxtSelectionIconSize* sizeObj = dynamic_cast<CtxtSelectionIconSize*>(event.m_callbackUserData);
    if (sizeObj)
    {
        globalSettings->gui.iconSize = sizeObj->iconSize;

        const IconBuffer::IconSize sz = [&]() -> IconBuffer::IconSize
        {
            switch (globalSettings->gui.iconSize)
            {
                case xmlAccess::ICON_SIZE_SMALL:
                    return IconBuffer::SIZE_SMALL;
                case xmlAccess::ICON_SIZE_MEDIUM:
                    return IconBuffer::SIZE_MEDIUM;
                case xmlAccess::ICON_SIZE_LARGE:
                    return IconBuffer::SIZE_LARGE;
            }
            return IconBuffer::SIZE_SMALL;
        }();

        std::shared_ptr<IconBuffer> iconBuffer = std::make_shared<IconBuffer>(sz);

        m_gridLeft  ->setIconManager(iconBuffer);
        m_gridMiddle->setIconManager(iconBuffer);
        m_gridRight ->setIconManager(iconBuffer);
    }
}


void MainDialog::OnContextMiddle(wxGridEvent& event)
{
    contextMenu.reset(new wxMenu); //re-create context menu

    wxMenuItem* menuItemInclude = new wxMenuItem(contextMenu.get(), wxID_ANY, _("Include all rows"));
    contextMenu->Append(menuItemInclude);
    if (gridDataView->rowsTotal() == 0)
        menuItemInclude->Enable(false);
    contextMenu->Connect(menuItemInclude->GetId(), wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(MainDialog::OnContextIncludeAll),      NULL, this);

    wxMenuItem* menuItemExclude = new wxMenuItem(contextMenu.get(), wxID_ANY, _("Exclude all rows"));
    contextMenu->Append(menuItemExclude);
    if (gridDataView->rowsTotal() == 0)
        menuItemExclude->Enable(false);
    contextMenu->Connect(menuItemExclude->GetId(), wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(MainDialog::OnContextExcludeAll),      NULL, this);

    PopupMenu(contextMenu.get()); //show context menu
}


void MainDialog::OnContextMiddleLabel(wxGridEvent& event)
{
    contextMenu.reset(new wxMenu); //re-create context menu

    wxMenuItem* itemSyncPreview = new wxMenuItem(contextMenu.get(), wxID_ANY, _("Synchronization Preview"));
    contextMenu->Connect(itemSyncPreview->GetId(), wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(MainDialog::OnContextSyncView),       NULL, this);

    wxMenuItem* itemCmpResult   = new wxMenuItem(contextMenu.get(), wxID_ANY, _("Comparison Result"));
    contextMenu->Connect(itemCmpResult->GetId(), wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(MainDialog::OnContextComparisonView), NULL, this);

    if (syncPreview->previewIsEnabled())
        itemSyncPreview->SetBitmap(GlobalResources::getImage(wxT("syncViewSmall")));
    else
        itemCmpResult->SetBitmap(GlobalResources::getImage(wxT("cmpViewSmall")));

    contextMenu->Append(itemCmpResult);
    contextMenu->Append(itemSyncPreview);

    PopupMenu(contextMenu.get()); //show context menu
}


void MainDialog::OnContextSetLayout(wxMouseEvent& event)
{
    contextMenu.reset(new wxMenu); //re-create context menu

    wxMenuItem* itemReset = contextMenu->Append(wxID_ANY, _("Reset view"));
    contextMenu->Connect(itemReset->GetId(), wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(MainDialog::OnContextSetLayoutReset), NULL, this);

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

            wxMenuItem* newItem = contextMenu->Append(wxID_ANY, entry);
            contextMenu->Connect(newItem->GetId(),
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


struct CtxtSelectionCmpVar : public wxObject
{
    CtxtSelectionCmpVar(CompareVariant var) : compareVar(var) {}
    CompareVariant compareVar;
};


void MainDialog::OnContextSelectCompVariant(wxMouseEvent& event)
{
    contextMenu.reset(new wxMenu); //re-create context menu

    wxMenuItem* itemSizeDate = new wxMenuItem(contextMenu.get(), wxID_ANY, _("File time and size"), L"", wxITEM_RADIO);
    contextMenu->Append(itemSizeDate);
    contextMenu->Connect(itemSizeDate->GetId(),
                         wxEVT_COMMAND_MENU_SELECTED,
                         wxCommandEventHandler(MainDialog::OnSetCompVariant),
                         new CtxtSelectionCmpVar(CMP_BY_TIME_SIZE), //ownership passed!
                         this);

    wxMenuItem* itemContent = new wxMenuItem(contextMenu.get(), wxID_ANY, _("File content"), L"", wxITEM_RADIO);
    contextMenu->Append(itemContent);
    contextMenu->Connect(itemContent->GetId(),
                         wxEVT_COMMAND_MENU_SELECTED,
                         wxCommandEventHandler(MainDialog::OnSetCompVariant),
                         new CtxtSelectionCmpVar(CMP_BY_CONTENT), //ownership passed!
                         this);

    //---------------------------------------------------------------
    xmlAccess::XmlGuiConfig cfg = getConfig();

    switch (cfg.mainCfg.cmpConfig.compareVar)
    {
        case CMP_BY_TIME_SIZE:
            itemSizeDate->Check();
            break;
        case CMP_BY_CONTENT:
            itemContent->Check();
            break;
    }

    PopupMenu(contextMenu.get()); //show context menu
}


struct CtxtSelectionSyncVar : public wxObject
{
    CtxtSelectionSyncVar(DirectionConfig::Variant var) : syncVar(var) {}
    DirectionConfig::Variant syncVar;
};


void MainDialog::OnContextSelectSyncVariant(wxMouseEvent& event)
{
    contextMenu.reset(new wxMenu); //re-create context menu

    wxMenuItem* itemAuto = new wxMenuItem(contextMenu.get(), wxID_ANY, _("<Automatic>"), L"", wxITEM_RADIO);
    contextMenu->Append(itemAuto);
    contextMenu->Connect(itemAuto->GetId(),
                         wxEVT_COMMAND_MENU_SELECTED,
                         wxCommandEventHandler(MainDialog::OnSetSyncVariant),
                         new CtxtSelectionSyncVar(DirectionConfig::AUTOMATIC), //ownership passed!
                         this);

    wxMenuItem* itemMirror = new wxMenuItem(contextMenu.get(), wxID_ANY, _("Mirror ->>"), L"", wxITEM_RADIO);
    contextMenu->Append(itemMirror);
    contextMenu->Connect(itemMirror->GetId(),
                         wxEVT_COMMAND_MENU_SELECTED,
                         wxCommandEventHandler(MainDialog::OnSetSyncVariant),
                         new CtxtSelectionSyncVar(DirectionConfig::MIRROR), //ownership passed!
                         this);

    wxMenuItem* itemUpdate = new wxMenuItem(contextMenu.get(), wxID_ANY, _("Update ->"), L"", wxITEM_RADIO);
    contextMenu->Append(itemUpdate);
    contextMenu->Connect(itemUpdate->GetId(),
                         wxEVT_COMMAND_MENU_SELECTED,
                         wxCommandEventHandler(MainDialog::OnSetSyncVariant),
                         new CtxtSelectionSyncVar(DirectionConfig::UPDATE), //ownership passed!
                         this);

    wxMenuItem* itemCustom = new wxMenuItem(contextMenu.get(), wxID_ANY, _("Custom"), L"", wxITEM_RADIO);
    contextMenu->Append(itemCustom);
    contextMenu->Connect(itemCustom->GetId(),
                         wxEVT_COMMAND_MENU_SELECTED,
                         wxCommandEventHandler(MainDialog::OnSetSyncVariant),
                         new CtxtSelectionSyncVar(DirectionConfig::CUSTOM), //ownership passed!
                         this);

    //---------------------------------------------------------------
    xmlAccess::XmlGuiConfig cfg = getConfig();

    switch (cfg.mainCfg.syncCfg.directionCfg.var)
    {
        case DirectionConfig::AUTOMATIC:
            itemAuto->Check();
            break;
        case DirectionConfig::MIRROR:
            itemMirror->Check();
            break;
        case DirectionConfig::UPDATE:
            itemUpdate->Check();
            break;
        case DirectionConfig::CUSTOM:
            itemCustom->Check();
            break;
    }

    PopupMenu(contextMenu.get()); //show context menu
}


void MainDialog::OnSetCompVariant(wxCommandEvent& event)
{
    CtxtSelectionCmpVar* selection = dynamic_cast<CtxtSelectionCmpVar*>(event.m_callbackUserData);
    if (selection)
    {
        currentCfg.mainCfg.cmpConfig.compareVar = selection->compareVar;
        applyCompareConfig();
    }
}


void MainDialog::OnSetSyncVariant(wxCommandEvent& event)
{
    CtxtSelectionSyncVar* selection = dynamic_cast<CtxtSelectionSyncVar*>(event.m_callbackUserData);
    if (selection)
    {
        currentCfg.mainCfg.syncCfg.directionCfg.var = selection->syncVar;
        applySyncConfig();
    }
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
    wxString output = afterLast(filename, utf8CvrtTo<wxString>(FILE_NAME_SEPARATOR));
    if (endsWith(output, ".ffs_gui"))
        output = beforeLast(output, L'.');
    return output;
}


void MainDialog::addFileToCfgHistory(const std::vector<wxString>& filenames)
{
    //check existence of all config files in parallel!
    std::list<boost::unique_future<bool>> fileEx;
    std::for_each(filenames.begin(), filenames.end(),
                  [&](const wxString& filename)
    {
        const Zstring file = toZ(filename); //convert to Zstring first: we don't want to pass wxString by value and risk MT issues!
        fileEx.push_back(zen::async2<bool>([=]() { return zen::fileExists(file); }));
    });

    //potentially slow network access: give all checks 500ms to finish
    wait_for_all_timed(fileEx.begin(), fileEx.end(), boost::posix_time::milliseconds(500));
    //------------------------------------------------------------------------------------------


    std::deque<bool> selections(m_listBoxHistory->GetCount());

    auto futIter = fileEx.begin();
    for (auto iter = filenames.begin(); iter != filenames.end(); ++iter, ++futIter)
    {
        //only (still) existing files should be included in the list
        if (futIter->is_ready() && !futIter->get())
            continue;

        const wxString& filename = *iter;
        const Zstring file = toZ(filename);

        int posFound = -1;

        for (int i = 0; i < static_cast<int>(m_listBoxHistory->GetCount()); ++i)
        {
            wxClientDataString* cData = dynamic_cast<wxClientDataString*>(m_listBoxHistory->GetClientObject(i));
            if (cData)
            {
                const wxString& filenameTmp = cData->name_;

                //tests if the same filenames are specified, even if they are relative to the current working directory/include symlinks or \\?\ prefix
                if (zen::samePhysicalFile(toZ(filename), toZ(filenameTmp)))
                {
                    posFound = i;
                    break;
                }
            }
        }

        if (posFound != -1)
            selections[posFound] = true;
        else
        {
            int newPos = -1;
            //the default config file should receive a different name on GUI
            if (zen::samePhysicalFile(toZ(lastRunConfigName()), toZ(filename)))
                newPos = m_listBoxHistory->Append(_("<Last session>"), new wxClientDataString(filename));
            else
                newPos = m_listBoxHistory->Append(getFormattedHistoryElement(filename), new wxClientDataString(filename));

            selections.insert(selections.begin() + newPos, true);
        }
    }

    assert(selections.size() == m_listBoxHistory->GetCount());

    //do not apply selections immediately but only when needed!
    //this prevents problems with m_listBoxHistory losing keyboard selection focus if identical selection is redundantly reapplied
    for (int pos = 0; pos < static_cast<int>(selections.size()); ++pos)
        if (m_listBoxHistory->IsSelected(pos) != selections[pos])
        {
            if (selections[pos])
                m_listBoxHistory->SetSelection(pos);
            else
                m_listBoxHistory->Deselect(pos);
        }
}


void MainDialog::OnSaveConfig(wxCommandEvent& event)
{
    trySaveConfig(NULL);
}


bool MainDialog::trySaveConfig(const wxString* fileName) //return true if saved successfully
{
    wxString targetFilename;

    if (fileName)
        targetFilename = *fileName;
    else
    {
        wxString defaultFileName = activeConfigFiles.size() == 1 && activeConfigFiles[0] != lastRunConfigName() ? activeConfigFiles[0] : wxT("SyncSettings.ffs_gui");
        //attention: activeConfigFiles may be an imported *.ffs_batch file! We don't want to overwrite it with a GUI config!
        if (defaultFileName.EndsWith(wxT(".ffs_batch")))
            defaultFileName.Replace(wxT(".ffs_batch"), wxT(".ffs_gui"), false);

        wxFileDialog filePicker(this,
                                wxEmptyString,
                                wxEmptyString,
                                defaultFileName,
                                wxString(_("FreeFileSync configuration")) + wxT(" (*.ffs_gui)|*.ffs_gui"),
                                wxFD_SAVE /*| wxFD_OVERWRITE_PROMPT*/); //creating this on freestore leads to memleak!
        if (filePicker.ShowModal() != wxID_OK)
            return false;
        targetFilename = filePicker.GetPath();
    }

    const xmlAccess::XmlGuiConfig guiCfg = getConfig();

    try
    {
        xmlAccess::writeConfig(guiCfg, targetFilename); //write config to XML
        setLastUsedConfig(targetFilename, guiCfg);

        pushStatusInformation(_("Configuration saved!"));
        return true;
    }
    catch (const xmlAccess::FfsXmlError& error)
    {
        wxMessageBox(error.toString().c_str(), _("Error"), wxOK | wxICON_ERROR);
        return false;
    }
}


void MainDialog::OnLoadConfig(wxCommandEvent& event)
{
    wxFileDialog filePicker(this,
                            wxEmptyString,
                            beforeLast(activeConfigFiles.size() == 1 && activeConfigFiles[0] != lastRunConfigName() ? activeConfigFiles[0] : wxString(), utf8CvrtTo<wxString>(FILE_NAME_SEPARATOR)), //set default dir: empty string if "activeConfigFiles" is empty or has no path separator
                            wxEmptyString,
                            wxString(_("FreeFileSync configuration")) + wxT(" (*.ffs_gui;*.ffs_batch)|*.ffs_gui;*.ffs_batch"),
                            wxFD_OPEN | wxFD_MULTIPLE);

    if (filePicker.ShowModal() == wxID_OK)
    {
        wxArrayString tmp;
        filePicker.GetPaths(tmp);
        std::vector<wxString> fileNames(tmp.begin(), tmp.end());

        loadConfiguration(fileNames);
    }
}


void MainDialog::OnNewConfig(wxCommandEvent& event)
{
    if (!saveOldConfig()) //notify user about changed settings
        return;

    xmlAccess::XmlGuiConfig emptyCfg;
    setConfig(emptyCfg);

    setLastUsedConfig(std::vector<wxString>(), emptyCfg);
}


void MainDialog::OnLoadFromHistory(wxCommandEvent& event)
{
    wxArrayInt selections;
    m_listBoxHistory->GetSelections(selections);

    std::vector<wxString> filenames;
    std::for_each(selections.begin(), selections.end(),
                  [&](int pos)
    {
        wxClientDataString* cData = dynamic_cast<wxClientDataString*>(m_listBoxHistory->GetClientObject(pos));
        if (cData)
            filenames.push_back(cData->name_);
    });

    if (!filenames.empty())
    {
        loadConfiguration(filenames);

        //in case user cancelled saving old config, selection is wrong: so reapply it!
        addFileToCfgHistory(activeConfigFiles);
    }
}


bool MainDialog::saveOldConfig() //return false on user abort
{
    if (lastConfigurationSaved != getConfig())
    {
        //notify user about changed settings
        if (!globalSettings->optDialogs.popupOnConfigChange)
            //discard current config selection, this ensures next app start will load <last session> instead of the original non-modified config selection
            setLastUsedConfig(std::vector<wxString>(), getConfig());
        else if (activeConfigFiles.size() == 1 && activeConfigFiles[0] != lastRunConfigName() /*|| activeConfigFiles.size() > 1*/)
            //only if check is active and non-default config file loaded
        {
            const wxString filename = activeConfigFiles[0];

            bool dontShowAgain = !globalSettings->optDialogs.popupOnConfigChange;

            switch (showQuestionDlg(ReturnQuestionDlg::BUTTON_YES | ReturnQuestionDlg::BUTTON_NO | ReturnQuestionDlg::BUTTON_CANCEL,
                                    _("Save changes to current configuration?"),
                                    &dontShowAgain))
            {
                case ReturnQuestionDlg::BUTTON_YES:
                    return trySaveConfig(endsWith(filename, L".ffs_gui") ? &filename : NULL); //don't overwrite .ffs_batch!
                case ReturnQuestionDlg::BUTTON_NO:
                    globalSettings->optDialogs.popupOnConfigChange = !dontShowAgain;
                    //by choosing "no" user actively discards current config selection
                    //this ensures next app start will load <last session> instead of the original non-modified config selection
                    setLastUsedConfig(std::vector<wxString>(), getConfig());
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
    std::vector<wxString> filenames;
    filenames.push_back(filename);

    loadConfiguration(filenames);
}


void MainDialog::loadConfiguration(const std::vector<wxString>& filenames)
{
    if (filenames.empty())
        return;

    if (!saveOldConfig())
        return;

    //load XML
    xmlAccess::XmlGuiConfig newGuiCfg;  //structure to receive gui settings, already defaulted!!
    try
    {
        //allow reading batch configurations also
        xmlAccess::convertConfig(filenames, newGuiCfg); //throw (xmlAccess::FfsXmlError)

        setLastUsedConfig(filenames, newGuiCfg);
        pushStatusInformation(_("Configuration loaded!"));
    }
    catch (const xmlAccess::FfsXmlError& error)
    {
        if (error.getSeverity() == xmlAccess::FfsXmlError::WARNING)
        {
            setLastUsedConfig(filenames, xmlAccess::XmlGuiConfig()); //simulate changed config on parsing errors
            wxMessageBox(error.toString(), _("Warning"), wxOK | wxICON_WARNING);
        }
        else
        {
            wxMessageBox(error.toString(), _("Error"), wxOK | wxICON_ERROR);
            return;
        }
    }

    setConfig(newGuiCfg);
}


void MainDialog::OnCfgHistoryKeyEvent(wxKeyEvent& event)
{
    const int keyCode = event.GetKeyCode();
    if (keyCode == WXK_DELETE || keyCode == WXK_NUMPAD_DELETE)
    {
        //delete currently selected config history items
        wxArrayInt tmp;
        m_listBoxHistory->GetSelections(tmp);

        std::set<int> selections(tmp.begin(), tmp.end()); //sort ascending!

        int shift = 0;
        std::for_each(selections.begin(), selections.end(),
                      [&](int pos)
        {
            m_listBoxHistory->Delete(pos + shift);
            --shift;
        });

        //set active selection on next element to allow "batch-deletion" by holding down DEL key
        if (!selections.empty() && m_listBoxHistory->GetCount() > 0)
        {
            int newSelection = *selections.begin();
            if (newSelection >= static_cast<int>(m_listBoxHistory->GetCount()))
                newSelection = m_listBoxHistory->GetCount() - 1;
            m_listBoxHistory->SetSelection(newSelection);
        }

        return; //"swallow" event
    }

    event.Skip();
}


void MainDialog::OnClose(wxCloseEvent& event)
{
    //attention: system shutdown: is handled in onQueryEndSession()!

    //regular destruction handling
    if (event.CanVeto())
    {
        const bool cancelled = !saveOldConfig(); //notify user about changed settings
        if (cancelled)
        {
            //attention: this Veto() would NOT cancel system shutdown since saveOldConfig() shows modal dialog

            event.Veto();
            return;
        }
    }

    Destroy();
}


void MainDialog::OnCheckRows(FFSCheckRowsEvent& event)
{
    const int lowerBound = std::max(std::min(event.rowFrom, event.rowTo), 0);
    const int upperBound = std::min(std::max(event.rowFrom, event.rowTo), static_cast<int>(gridDataView->rowsOnView()) - 1);

    std::set<size_t> selectedRowsOnView;
    for (int i = lowerBound; i <= upperBound; ++i)
        selectedRowsOnView.insert(i);

    if (!selectedRowsOnView.empty())
        filterRangeManually(selectedRowsOnView, event.rowFrom);
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


void MainDialog::setLastUsedConfig(const wxString& filename, const xmlAccess::XmlGuiConfig& guiConfig)
{
    std::vector<wxString> filenames;
    filenames.push_back(filename);
    setLastUsedConfig(filenames, guiConfig);
}


void MainDialog::setLastUsedConfig(const std::vector<wxString>& filenames,
                                   const xmlAccess::XmlGuiConfig& guiConfig)
{
    activeConfigFiles = filenames;
    lastConfigurationSaved = guiConfig;

    addFileToCfgHistory(activeConfigFiles); //put filename on list of last used config files

    //set title
    if (activeConfigFiles.size() == 1 && activeConfigFiles[0] != lastRunConfigName())
        SetTitle(activeConfigFiles[0]);
    else
        SetTitle(wxString(wxT("FreeFileSync - ")) + _("Folder Comparison and Synchronization"));
}


void MainDialog::setConfig(const xmlAccess::XmlGuiConfig& newGuiCfg)
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
    firstFolderPair->setValues(toWx(currentCfg.mainCfg.firstPair.leftDirectory),
                               toWx(currentCfg.mainCfg.firstPair.rightDirectory),
                               currentCfg.mainCfg.firstPair.altCmpConfig,
                               currentCfg.mainCfg.firstPair.altSyncConfig,
                               currentCfg.mainCfg.firstPair.localFilter);

    //folderHistoryLeft->addItem(currentCfg.mainCfg.firstPair.leftDirectory);
    //folderHistoryRight->addItem(currentCfg.mainCfg.firstPair.rightDirectory);

    //clear existing additional folder pairs
    clearAddFolderPairs();

    //set additional pairs
    addFolderPair(currentCfg.mainCfg.additionalPairs);


    //read GUI layout
    m_checkBoxHideFilt->SetValue(currentCfg.hideFilteredElements);

    syncPreview->enablePreview(currentCfg.syncPreviewEnabled);

    //###########################################################
    //update compare variant name
    m_staticTextCmpVariant->SetLabel(currentCfg.mainCfg.getCompVariantName());

    //update sync variant name
    m_staticTextSyncVariant->SetLabel(currentCfg.mainCfg.getSyncVariantName());
    m_panelTopButtons->Layout(); //adapt layout for variant text
}


inline
FolderPairEnh getEnhancedPair(const DirectoryPair* panel)
{
    return FolderPairEnh(toZ(panel->getLeftDir()),
                         toZ(panel->getRightDir()),
                         panel->getAltCompConfig(),
                         panel->getAltSyncConfig(),
                         panel->getAltFilterConfig());
}


xmlAccess::XmlGuiConfig MainDialog::getConfig() const
{
    xmlAccess::XmlGuiConfig guiCfg = currentCfg;

    //load settings whose ownership lies not in currentCfg:

    //first folder pair
    guiCfg.mainCfg.firstPair = FolderPairEnh(toZ(firstFolderPair->getLeftDir()),
                                             toZ(firstFolderPair->getRightDir()),
                                             firstFolderPair->getAltCompConfig(),
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
    static wxString instance = toWx(zen::getConfigDir()) + wxT("LastRun.ffs_gui");
    return instance;
}


void MainDialog::refreshGridAfterFilterChange(int delay)
{
    //signal UI that grids need to be refreshed on next Update()
    m_gridLeft  ->ForceRefresh();
    m_gridMiddle->ForceRefresh();
    m_gridRight ->ForceRefresh();

    m_gridLeft  ->Update();   //
    m_gridMiddle->Update();   //show changes resulting from ForceRefresh() - required for last, possibly half-visible, row only
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
    contextMenu.reset(new wxMenu); //re-create context menu

    wxMenuItem* itemClear = new wxMenuItem(contextMenu.get(), wxID_ANY, _("Clear filter settings"));
    contextMenu->Append(itemClear);
    contextMenu->Connect(itemClear->GetId(), wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(MainDialog::OnGlobalFilterRemConfirm), NULL, this);

    if (isNullFilter(currentCfg.mainCfg.globalFilter))
        contextMenu->Enable(itemClear->GetId(), false); //disable menu item, if clicking wouldn't make sense anyway

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


inline
wxBitmap buttonPressed(const std::string& name)
{
    wxBitmap background = GlobalResources::getImage(wxT("buttonPressed"));
    return layOver(GlobalResources::getImage(utf8CvrtTo<wxString>(name)), background);
}


inline
wxBitmap buttonReleased(const std::string& name)
{
    wxImage output = GlobalResources::getImage(utf8CvrtTo<wxString>(name)).ConvertToImage().ConvertToGreyscale(1.0/3, 1.0/3, 1.0/3); //treat all channels equally!
    zen::move(output, 0, -1); //move image right one pixel

    brighten(output, 80);
    return output;
}


void MainDialog::initViewFilterButtons()
{
    //compare result buttons
    m_bpButtonLeftOnly->init(buttonPressed("leftOnly"),
                             _("Hide files that exist on left side only"),
                             buttonReleased("leftOnly"),
                             _("Show files that exist on left side only"));

    m_bpButtonRightOnly->init(buttonPressed("rightOnly"),
                              _("Hide files that exist on right side only"),
                              buttonReleased("rightOnly"),
                              _("Show files that exist on right side only"));

    m_bpButtonLeftNewer->init(buttonPressed("leftNewer"),
                              _("Hide files that are newer on left"),
                              buttonReleased("leftNewer"),
                              _("Show files that are newer on left"));

    m_bpButtonRightNewer->init(buttonPressed("rightNewer"),
                               _("Hide files that are newer on right"),
                               buttonReleased("rightNewer"),
                               _("Show files that are newer on right"));

    m_bpButtonEqual->init(buttonPressed("equal"),
                          _("Hide files that are equal"),
                          buttonReleased("equal"),
                          _("Show files that are equal"));

    m_bpButtonDifferent->init(buttonPressed("different"),
                              _("Hide files that are different"),
                              buttonReleased("different"),
                              _("Show files that are different"));

    m_bpButtonConflict->init(buttonPressed("conflict"),
                             _("Hide conflicts"),
                             buttonReleased("conflict"),
                             _("Show conflicts"));

    //sync preview buttons
    m_bpButtonSyncCreateLeft->init(buttonPressed("createLeft"),
                                   _("Hide files that will be created on the left side"),
                                   buttonReleased("createLeft"),
                                   _("Show files that will be created on the left side"));

    m_bpButtonSyncCreateRight->init(buttonPressed("createRight"),
                                    _("Hide files that will be created on the right side"),
                                    buttonReleased("createRight"),
                                    _("Show files that will be created on the right side"));

    m_bpButtonSyncDeleteLeft->init(buttonPressed("deleteLeft"),
                                   _("Hide files that will be deleted on the left side"),
                                   buttonReleased("deleteLeft"),
                                   _("Show files that will be deleted on the left side"));

    m_bpButtonSyncDeleteRight->init(buttonPressed("deleteRight"),
                                    _("Hide files that will be deleted on the right side"),
                                    buttonReleased("deleteRight"),
                                    _("Show files that will be deleted on the right side"));

    m_bpButtonSyncDirOverwLeft->init(buttonPressed("updateLeft"),
                                     _("Hide files that will be overwritten on left side"),
                                     buttonReleased("updateLeft"),
                                     _("Show files that will be overwritten on left side"));

    m_bpButtonSyncDirOverwRight->init(buttonPressed("updateRight"),
                                      _("Hide files that will be overwritten on right side"),
                                      buttonReleased("updateRight"),
                                      _("Show files that will be overwritten on right side"));

    m_bpButtonSyncDirNone->init(buttonPressed("none"),
                                _("Hide files that won't be copied"),
                                buttonReleased("none"),
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
        setImage(*m_bpButtonFilter, GlobalResources::getImage(wxT("filterOff")));
        m_bpButtonFilter->SetToolTip(_("No filter selected"));
    }
    else
    {
        setImage(*m_bpButtonFilter, GlobalResources::getImage(wxT("filterOn")));
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

        const std::vector<zen::FolderPairCfg> cmpConfig = zen::extractCompareCfg(getConfig().mainCfg);

        //GUI mode: place directory locks on directories isolated(!) during both comparison and synchronization
        LockHolder dummy2(true); //allow pw prompt
        for (std::vector<FolderPairCfg>::const_iterator i = cmpConfig.begin(); i != cmpConfig.end(); ++i)
        {
            dummy2.addDir(i->leftDirectoryFmt,  statusHandler);
            dummy2.addDir(i->rightDirectoryFmt, statusHandler);
        }

        //begin comparison
        zen::CompareProcess compProc(globalSettings->fileTimeTolerance,
                                     globalSettings->optDialogs,
                                     true, //allow pw prompt
                                     globalSettings->runWithBackgroundPriority,
                                     statusHandler);

        //technical representation of comparison data
        zen::FolderComparison newCompareData;
        compProc.startCompareProcess(cmpConfig, newCompareData);

        gridDataView->setData(newCompareData); //newCompareData is invalidated after this call

        //play (optional) sound notification after sync has completed (GUI and batch mode)
        const wxString soundFile = toWx(zen::getResourceDir()) + wxT("Compare_Complete.wav");
        if (fileExists(toZ(soundFile)))
            wxSound::Play(soundFile, wxSOUND_ASYNC);
    }
    catch (GuiAbortProcess&)
    {
        //disable the sync button
        syncPreview->enableSynchronization(false);
        if (m_buttonCompare->IsShownOnScreen()) m_buttonCompare->SetFocus();
        updateGuiGrid(); //refresh grid in ANY case! (also on abort)
        return;
    }

    //once compare is finished enable the sync button
    syncPreview->enableSynchronization(true);
    if (m_buttonStartSync->IsShownOnScreen()) m_buttonStartSync->SetFocus();

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
    folderHistoryLeft ->addItem(toZ(m_directoryLeft->GetValue()));
    folderHistoryRight->addItem(toZ(m_directoryRight->GetValue()));

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
    const wxString toUpdate = zen::toStringSep(st.getUpdate());
    const wxString toDelete = zen::toStringSep(st.getDelete());
    const wxString data     = zen::filesizeToShortString(st.getDataToProcess());

    m_textCtrlCreate->SetValue(toCreate);
    m_textCtrlUpdate->SetValue(toUpdate);
    m_textCtrlDelete->SetValue(toDelete);
    m_textCtrlData  ->SetValue(data);
}


void MainDialog::OnSwitchView(wxCommandEvent& event)
{
    syncPreview->enablePreview(!syncPreview->previewIsEnabled());
}


void MainDialog::OnSyncSettings(wxCommandEvent& event)
{
    ExecWhenFinishedCfg ewfCfg = { &currentCfg.mainCfg.onCompletion,
                                   &globalSettings->gui.onCompletionHistory,
                                   globalSettings->gui.onCompletionHistoryMax
                                 };

    if (showSyncConfigDlg(currentCfg.mainCfg.cmpConfig.compareVar,
                          currentCfg.mainCfg.syncCfg,
                          &currentCfg.handleError,
                          &ewfCfg) == ReturnSyncConfig::BUTTON_OKAY) //optional input parameter
    {
        applySyncConfig();
    }
}


void MainDialog::applyCompareConfig(bool globalLevel)
{
    //update compare variant name
    m_staticTextCmpVariant->SetLabel(getConfig().mainCfg.getCompVariantName());
    m_panelTopButtons->Layout(); //adapt layout for variant text

    if (globalLevel)
    {
        //disable the sync button
        syncPreview->enableSynchronization(false);

        //clear grids
        gridDataView->clearAllRows();
        updateGuiGrid();

        //convenience: change sync view
        switch (currentCfg.mainCfg.cmpConfig.compareVar)
        {
            case CMP_BY_TIME_SIZE:
                syncPreview->enablePreview(true);
                break;
            case CMP_BY_CONTENT:
                syncPreview->enablePreview(false);
                break;
        }

        if (m_buttonCompare->IsShownOnScreen()) m_buttonCompare->SetFocus();
    }
}


void MainDialog::OnCmpSettings(wxCommandEvent& event)
{
    //show window right next to the compare-config button
    //wxPoint windowPos = m_bpButtonCmpConfig->GetScreenPosition();
    //windowPos.x += m_bpButtonCmpConfig->GetSize().GetWidth() + 5;

    CompConfig cmpConfigNew = currentCfg.mainCfg.cmpConfig;

    if (zen::showCompareCfgDialog(cmpConfigNew) == ReturnSmallDlg::BUTTON_OKAY &&
        //check if settings were changed at all
        cmpConfigNew != currentCfg.mainCfg.cmpConfig)
    {
        currentCfg.mainCfg.cmpConfig = cmpConfigNew;

        applyCompareConfig();
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
                getConfig().mainCfg.getSyncVariantName(),
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

        wxString activeFileName = activeConfigFiles.size() == 1 && activeConfigFiles[0] != lastRunConfigName() ? activeConfigFiles[0] : wxString();

        const auto& guiCfg = getConfig();

        //class handling status updates and error messages
        SyncStatusHandler statusHandler(this,
                                        currentCfg.handleError,
                                        xmlAccess::extractJobName(activeFileName),
                                        guiCfg.mainCfg.onCompletion,
                                        globalSettings->gui.onCompletionHistory);

        FolderComparison& dataToSync = gridDataView->getDataTentative();

        //GUI mode: place directory locks on directories isolated(!) during both comparison and synchronization
        LockHolder dummy2(true); //allow pw prompt

        for (auto i = begin(dataToSync); i != end(dataToSync); ++i)
        {
            dummy2.addDir(i->getBaseDirPf<LEFT_SIDE >(), statusHandler);
            dummy2.addDir(i->getBaseDirPf<RIGHT_SIDE>(), statusHandler);
        }

        //start synchronization and mark all elements processed
        zen::SyncProcess syncProc(globalSettings->optDialogs,
                                  globalSettings->verifyFileCopy,
                                  globalSettings->copyLockedFiles,
                                  globalSettings->copyFilePermissions,
                                  globalSettings->transactionalFileCopy,
                                  globalSettings->runWithBackgroundPriority,
                                  statusHandler);

        const std::vector<zen::FolderPairSyncCfg> syncProcessCfg = zen::extractSyncCfg(guiCfg.mainCfg);

        //make sure syncProcessCfg and dataToSync have same size and correspond!
        if (syncProcessCfg.size() != dataToSync.size())
            throw std::logic_error("Programming Error: Contract violation!"); //should never happen: sync button is deactivated if they are not in sync

        syncProc.startSynchronizationProcess(syncProcessCfg, dataToSync);

        //play (optional) sound notification after sync has completed (GUI and batch mode)
        const wxString soundFile = toWx(zen::getResourceDir()) + wxT("Sync_Complete.wav");
        if (fileExists(toZ(soundFile)))
            wxSound::Play(soundFile, wxSOUND_ASYNC);
    }
    catch (GuiAbortProcess&)
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

        //set sort direction indicator on UI
        m_gridMiddle->setSortMarker(CustomGrid::SortMarker(-1, CustomGrid::ASCENDING));
        m_gridRight ->setSortMarker(CustomGrid::SortMarker(-1, CustomGrid::ASCENDING));
        m_gridLeft  ->setSortMarker(CustomGrid::SortMarker(currentSortColumn, sortAscending ? CustomGrid::ASCENDING : CustomGrid::DESCENDING));

        updateGuiGrid(); //refresh gridDataView
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

        //set sort direction indicator on UI
        m_gridLeft->setSortMarker(CustomGrid::SortMarker(-1, CustomGrid::ASCENDING));
        m_gridMiddle->setSortMarker(CustomGrid::SortMarker(-1, CustomGrid::ASCENDING));
        m_gridRight->setSortMarker(CustomGrid::SortMarker(currentSortColumn, sortAscending ? CustomGrid::ASCENDING : CustomGrid::DESCENDING));

        updateGuiGrid(); //refresh gridDataView
    }
}


void MainDialog::OnSwapSides(wxCommandEvent& event)
{
    //swap directory names: first pair
    firstFolderPair->setValues(firstFolderPair->getRightDir(), // swap directories
                               firstFolderPair->getLeftDir(),  //
                               firstFolderPair->getAltCompConfig(),
                               firstFolderPair->getAltSyncConfig(),
                               firstFolderPair->getAltFilterConfig());

    //additional pairs
    for (std::vector<DirectoryPair*>::const_iterator i = additionalFolderPairs.begin(); i != additionalFolderPairs.end(); ++i)
    {
        DirectoryPair* dirPair = *i;
        dirPair->setValues(dirPair->getRightDir(), // swap directories
                           dirPair->getLeftDir(),  //
                           dirPair->getAltCompConfig(),
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
    zen::swapGrids(getConfig().mainCfg, gridDataView->getDataTentative());
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

    //#################################################
    //format numbers to text:

    //show status information on "root" level.
    bSizerStatusLeftDirectories->Show(foldersOnLeftView > 0);
    bSizerStatusLeftFiles      ->Show(filesOnLeftView   > 0);

    setText(*m_staticTextStatusLeftDirs,  replaceCpy(_P("1 directory", "%x directories", foldersOnLeftView), L"%x", zen::toStringSep(foldersOnLeftView), false));
    setText(*m_staticTextStatusLeftFiles, replaceCpy(_P("1 file", "%x files", filesOnLeftView), L"%x", zen::toStringSep(filesOnLeftView), false));
    setText(*m_staticTextStatusLeftBytes, zen::filesizeToShortString(to<Int64>(filesizeLeftView)));

    {
        wxString statusMiddleNew = _P("%x of 1 row in view", "%x of %y rows in view", gridDataView->rowsTotal());
        replace(statusMiddleNew, L"%x", toStringSep(gridDataView->rowsOnView()), false);
        replace(statusMiddleNew, L"%y", toStringSep(gridDataView->rowsTotal()), false);
        setText(*m_staticTextStatusMiddle, statusMiddleNew);
    }

    bSizerStatusRightDirectories->Show(foldersOnRightView > 0);
    bSizerStatusRightFiles      ->Show(filesOnRightView   > 0);

    setText(*m_staticTextStatusRightDirs,  replaceCpy(_P("1 directory", "%x directories", foldersOnRightView), L"%x", zen::toStringSep(foldersOnRightView), false));
    setText(*m_staticTextStatusRightFiles, replaceCpy(_P("1 file", "%x files", filesOnRightView), L"%x", zen::toStringSep(filesOnRightView), false));
    setText(*m_staticTextStatusRightBytes, zen::filesizeToShortString(to<Int64>(filesizeRightView)));

    m_panelStatusBar->Layout();
}


void MainDialog::updateFilterConfig()
{
    applyFiltering(gridDataView->getDataTentative(), getConfig().mainCfg);
    refreshGridAfterFilterChange(400);
}


void MainDialog::applySyncConfig()
{
    //update sync variant name
    m_staticTextSyncVariant->SetLabel(getConfig().mainCfg.getSyncVariantName());
    m_panelTopButtons->Layout(); //adapt layout for variant text


    class RedetermineCallback : public DeterminationProblem
    {
    public:
        RedetermineCallback(bool& warningSyncDatabase, wxWindow* parent) :
            warningSyncDatabase_(warningSyncDatabase),
            parent_(parent) {}

        virtual void reportWarning(const std::wstring& text)
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

    zen::redetermineSyncDirection(getConfig().mainCfg, gridDataView->getDataTentative(), &redetCallback);
    updateGuiGrid();
}


void MainDialog::OnAddFolderPair(wxCommandEvent& event)
{
    wxWindowUpdateLocker dummy(this); //avoid display distortion

    std::vector<FolderPairEnh> newPairs;
    newPairs.push_back(getConfig().mainCfg.firstPair);

    //clear first pair
    const FolderPairEnh cfgEmpty;
    firstFolderPair->setValues(toWx(cfgEmpty.leftDirectory),
                               toWx(cfgEmpty.rightDirectory),
                               cfgEmpty.altCmpConfig,
                               cfgEmpty.altSyncConfig,
                               cfgEmpty.localFilter);

    //keep sequence to update GUI as last step
    addFolderPair(newPairs, true); //add pair in front of additonal pairs
}


void MainDialog::OnRemoveTopFolderPair(wxCommandEvent& event)
{
    if (additionalFolderPairs.size() > 0)
    {
        wxWindowUpdateLocker dummy(this); //avoid display distortion

        //get settings from second folder pair
        const FolderPairEnh cfgSecond = getEnhancedPair(additionalFolderPairs[0]);

        //reset first pair
        firstFolderPair->setValues(toWx(cfgSecond.leftDirectory),
                                   toWx(cfgSecond.rightDirectory),
                                   cfgSecond.altCmpConfig,
                                   cfgSecond.altSyncConfig,
                                   cfgSecond.localFilter);

        removeAddFolderPair(0); //remove second folder pair (first of additional folder pairs)
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
    m_panelTopLeft->Layout();

    //adapt local filter and sync cfg for first folder pair
    if (additionalFolderPairs.size() == 0 &&
        firstFolderPair->getAltCompConfig().get() == NULL &&
        firstFolderPair->getAltSyncConfig().get() == NULL &&
        isNullFilter(firstFolderPair->getAltFilterConfig()))
    {
        m_bpButtonAltCompCfg ->Hide();
        m_bpButtonAltSyncCfg ->Hide();
        m_bpButtonLocalFilter->Hide();

        setImage(*m_bpButtonSwapSides, GlobalResources::getImage(wxT("swap")));
    }
    else
    {
        m_bpButtonAltCompCfg ->Show();
        m_bpButtonAltSyncCfg ->Show();
        m_bpButtonLocalFilter->Show();

        setImage(*m_bpButtonSwapSides, GlobalResources::getImage(wxT("swapSlim")));
    }
    m_panelTopMiddle->Layout();

    int addPairMinimalHeight = 0;
    int addPairOptimalHeight = 0;
    if (additionalFolderPairs.size() > 0)
    {
        int pairHeight = additionalFolderPairs[0]->GetSize().GetHeight();
        addPairMinimalHeight = std::min<double>(1.5, additionalFolderPairs.size()) * pairHeight; //have 0.5 * height indicate that more folders are there
        addPairOptimalHeight = std::min<double>(globalSettings->gui.maxFolderPairsVisible - 1 + 0.5, //subtract first/main folder pair and add 0.5 to indicate additional folders
                                                additionalFolderPairs.size()) * pairHeight;

        addPairOptimalHeight = std::max(addPairOptimalHeight, addPairMinimalHeight); //implicitly handle corrupted values for "maxFolderPairsVisible"
    }

    //########################################################################################################################
    //wxAUI hack: set minimum height to desired value, then call wxAuiPaneInfo::Fixed() to apply it
    auiMgr.GetPane(m_panelDirectoryPairs).MinSize(-1, m_panelTopLeft->GetSize().GetHeight() + addPairOptimalHeight);
    auiMgr.GetPane(m_panelDirectoryPairs).Fixed();
    auiMgr.Update();

    //now make resizable again
    auiMgr.GetPane(m_panelDirectoryPairs).Resizable();
    auiMgr.Update();
    //########################################################################################################################

    //ensure additional folder pairs are at least partially visible
    auiMgr.GetPane(m_panelDirectoryPairs).MinSize(-1, m_panelTopLeft->GetSize().GetHeight() + addPairMinimalHeight);
    auiMgr.Update();

    m_scrolledWindowFolderPairs->Fit();    //adjust scrolled window size

    //m_scrolledWindowFolderPairs->Layout(); //adjust stuff inside scrolled window
    m_panelDirectoryPairs->Layout();
}


void MainDialog::addFolderPair(const std::vector<FolderPairEnh>& newPairs, bool addFront)
{
    wxWindowUpdateLocker dummy(m_panelDirectoryPairs); //avoid display distortion
    wxWindowUpdateLocker dummy2(m_panelGrids);         //

    std::for_each(newPairs.begin(), newPairs.end(),
                  [&](const FolderPairEnh& enhPair)
    {
        //add new folder pair
        DirectoryPair* newPair = new DirectoryPair(m_scrolledWindowFolderPairs, *this);

        //init dropdown history
        newPair->m_directoryLeft ->init(folderHistoryLeft);
        newPair->m_directoryRight->init(folderHistoryRight);

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
        newPair->setValues(toWx(enhPair.leftDirectory),
                           toWx(enhPair.rightDirectory),
                           enhPair.altCmpConfig,
                           enhPair.altSyncConfig,
                           enhPair.localFilter);
    });

    updateGuiForFolderPair();

    //disable the sync button
    syncPreview->enableSynchronization(false);

    //clear grids
    gridDataView->clearAllRows();
    applySyncConfig(); //mainly to update sync dir description text
    applyCompareConfig(false); //false: not global level
}


void MainDialog::removeAddFolderPair(size_t pos)
{
    wxWindowUpdateLocker dummy(m_panelDirectoryPairs); //avoid display distortion
    wxWindowUpdateLocker dummy2(m_panelGrids);         //

    if (pos < additionalFolderPairs.size())
    {
        //remove folder pairs from window
        DirectoryPair* pairToDelete = additionalFolderPairs[pos];
        //const int pairHeight = pairToDelete->GetSize().GetHeight();

        bSizerAddFolderPairs->Detach(pairToDelete); //Remove() does not work on Window*, so do it manually
        pairToDelete->Destroy();                    //
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

    //------------------------------------------------------------------
    //disable the sync button
    syncPreview->enableSynchronization(false);

    //clear grids
    gridDataView->clearAllRows();
    applySyncConfig(); //mainly to update sync dir description text
    applyCompareConfig(false); //false: not global level
}


void MainDialog::clearAddFolderPairs()
{
    wxWindowUpdateLocker dummy(m_panelDirectoryPairs); //avoid display distortion
    wxWindowUpdateLocker dummy2(m_panelGrids);         //

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
        exportString += wxT('\"') + copyStringTo<zxString>(cellVal) + wxT('\"');
    else
        exportString += copyStringTo<zxString>(cellVal);
}
}


void MainDialog::OnMenuExportFileList(wxCommandEvent& event)
{
    //get a filename
    const wxString defaultFileName = wxT("FileList.csv"); //proposal
    wxFileDialog filePicker(this,
                            wxEmptyString,
                            wxEmptyString,
                            defaultFileName,
                            wxString(_("Comma separated list")) + wxT(" (*.csv)|*.csv"),
                            wxFD_SAVE /*| wxFD_OVERWRITE_PROMPT*/); //creating this on freestore leads to memleak!

    if (filePicker.ShowModal() == wxID_OK)
    {
        const wxString newFileName = filePicker.GetPath();

        zxString exportString; //perf: wxString doesn't model exponential growth and so is out

        //write legend
        exportString +=  copyStringTo<zxString>(_("Legend")) + wxT('\n');
        if (syncPreview->previewIsEnabled())
        {
            exportString += wxT("\"") + copyStringTo<zxString>(getSyncOpDescription(SO_EQUAL))               + wxT("\";") + copyStringTo<zxString>(getSymbol(SO_EQUAL))               + wxT('\n');
            exportString += wxT("\"") + copyStringTo<zxString>(getSyncOpDescription(SO_CREATE_NEW_LEFT))     + wxT("\";") + copyStringTo<zxString>(getSymbol(SO_CREATE_NEW_LEFT))     + wxT('\n');
            exportString += wxT("\"") + copyStringTo<zxString>(getSyncOpDescription(SO_CREATE_NEW_RIGHT))    + wxT("\";") + copyStringTo<zxString>(getSymbol(SO_CREATE_NEW_RIGHT))    + wxT('\n');
            exportString += wxT("\"") + copyStringTo<zxString>(getSyncOpDescription(SO_OVERWRITE_LEFT))      + wxT("\";") + copyStringTo<zxString>(getSymbol(SO_OVERWRITE_LEFT))      + wxT('\n');
            exportString += wxT("\"") + copyStringTo<zxString>(getSyncOpDescription(SO_OVERWRITE_RIGHT))     + wxT("\";") + copyStringTo<zxString>(getSymbol(SO_OVERWRITE_RIGHT))     + wxT('\n');
            exportString += wxT("\"") + copyStringTo<zxString>(getSyncOpDescription(SO_DELETE_LEFT))         + wxT("\";") + copyStringTo<zxString>(getSymbol(SO_DELETE_LEFT))         + wxT('\n');
            exportString += wxT("\"") + copyStringTo<zxString>(getSyncOpDescription(SO_DELETE_RIGHT))        + wxT("\";") + copyStringTo<zxString>(getSymbol(SO_DELETE_RIGHT))        + wxT('\n');
            exportString += wxT("\"") + copyStringTo<zxString>(getSyncOpDescription(SO_DO_NOTHING))          + wxT("\";") + copyStringTo<zxString>(getSymbol(SO_DO_NOTHING))          + wxT('\n');
            exportString += wxT("\"") + copyStringTo<zxString>(getSyncOpDescription(SO_UNRESOLVED_CONFLICT)) + wxT("\";") + copyStringTo<zxString>(getSymbol(SO_UNRESOLVED_CONFLICT)) + wxT('\n');
        }
        else
        {
            exportString += wxT("\"") + copyStringTo<zxString>(getCategoryDescription(FILE_EQUAL))           + wxT("\";") + copyStringTo<zxString>(getSymbol(FILE_EQUAL))           + wxT('\n');
            exportString += wxT("\"") + copyStringTo<zxString>(getCategoryDescription(FILE_DIFFERENT))       + wxT("\";") + copyStringTo<zxString>(getSymbol(FILE_DIFFERENT))       + wxT('\n');
            exportString += wxT("\"") + copyStringTo<zxString>(getCategoryDescription(FILE_LEFT_SIDE_ONLY))  + wxT("\";") + copyStringTo<zxString>(getSymbol(FILE_LEFT_SIDE_ONLY))  + wxT('\n');
            exportString += wxT("\"") + copyStringTo<zxString>(getCategoryDescription(FILE_RIGHT_SIDE_ONLY)) + wxT("\";") + copyStringTo<zxString>(getSymbol(FILE_RIGHT_SIDE_ONLY)) + wxT('\n');
            exportString += wxT("\"") + copyStringTo<zxString>(getCategoryDescription(FILE_LEFT_NEWER))      + wxT("\";") + copyStringTo<zxString>(getSymbol(FILE_LEFT_NEWER))      + wxT('\n');
            exportString += wxT("\"") + copyStringTo<zxString>(getCategoryDescription(FILE_RIGHT_NEWER))     + wxT("\";") + copyStringTo<zxString>(getSymbol(FILE_RIGHT_NEWER))     + wxT('\n');
            exportString += wxT("\"") + copyStringTo<zxString>(getCategoryDescription(FILE_CONFLICT))        + wxT("\";") + copyStringTo<zxString>(getSymbol(FILE_CONFLICT))        + wxT('\n');
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

            output.Write(BYTE_ORDER_MARK_UTF8, sizeof(BYTE_ORDER_MARK_UTF8) - 1);
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
    const xmlAccess::XmlGuiConfig currCfg = getConfig(); //get UP TO DATE config, with updated values for main and additional folders!

    const wxString referenceFile = activeConfigFiles.size() == 1 && activeConfigFiles[0] != lastRunConfigName() ? activeConfigFiles[0] : wxString();

    const xmlAccess::XmlBatchConfig batchCfg = convertGuiToBatch(currCfg, referenceFile);

    if (showSyncBatchDlg(referenceFile, batchCfg,
                         folderHistoryLeft,
                         folderHistoryRight,
                         globalSettings->gui.onCompletionHistory,
                         globalSettings->gui.onCompletionHistoryMax) == ReturnBatchConfig::BATCH_FILE_SAVED)
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
    std::for_each(additionalFolderPairs.begin(), additionalFolderPairs.end(),
    [](DirectoryPair* dirPair) { dirPair->Layout(); });

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

    const xmlAccess::XmlGuiConfig currentGuiCfg = getConfig();
    auto activeFiles = activeConfigFiles;

    writeGlobalSettings(); //updating global settings before creating new dialog

    //create new main window and delete old one
    MainDialog* frame = new MainDialog(activeFiles, currentGuiCfg, *globalSettings, false);
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
    syncPreviewEnabled = value;

    //toggle display of sync preview in middle grid
    mainDlg_->m_gridMiddle->enableSyncPreview(value);
    //        mainDlg_->Refresh();

    mainDlg_->updateGuiGrid();
}


void MainDialog::SyncPreview::enableSynchronization(bool value)
{
    synchronizationEnabled = value;

    if (value)
    {
        mainDlg_->m_buttonStartSync->SetForegroundColour(wxSystemSettings::GetColour(wxSYS_COLOUR_BTNTEXT));
        mainDlg_->m_buttonStartSync->setBitmapFront(GlobalResources::getImage(wxT("sync")));
    }
    else
    {
        mainDlg_->m_buttonStartSync->SetForegroundColour(wxColor(128, 128, 128)); //Some colors seem to have problems with 16Bit color depth, well this one hasn't!
        mainDlg_->m_buttonStartSync->setBitmapFront(GlobalResources::getImage(wxT("syncDisabled")));
    }
}


bool MainDialog::SyncPreview::synchronizationIsEnabled() const
{
    return synchronizationEnabled;
}

