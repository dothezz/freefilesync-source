// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) 2008-2010 ZenJu (zhnmju123 AT gmx.de)                    *
// **************************************************************************
//
#include "main_dlg.h"
#include <wx/filename.h>
#include <stdexcept>
#include "../shared/system_constants.h"
#include <wx/clipbrd.h>
#include <wx/dataobj.h>
#include <iterator>
#include <wx/ffile.h>
#include "../library/custom_grid.h"
#include "../shared/custom_button.h"
#include "../shared/custom_combo_box.h"
#include <wx/msgdlg.h>
#include "../comparison.h"
#include "../synchronization.h"
#include "../algorithm.h"
#include "../shared/app_main.h"
#include "../shared/util.h"
#include "check_version.h"
#include "gui_status_handler.h"
#include "sync_cfg.h"
#include "../shared/localization.h"
#include "../shared/string_conv.h"
#include "small_dlgs.h"
#include "mouse_move_dlg.h"
#include "progress_indicator.h"
#include "msg_popup.h"
#include "../shared/dir_name.h"
#include "../library/filter.h"
#include "../structures.h"
#include <wx/imaglist.h>
#include <wx/wupdlock.h>
#include "grid_view.h"
#include "../library/resources.h"
#include "../shared/file_handling.h"
#include "../shared/recycler.h"
#include "../shared/xml_base.h"
#include "../shared/standard_paths.h"
#include "../shared/toggle_button.h"
#include "folder_pair.h"
#include "../shared/global_func.h"
#include <wx/sound.h>
#include "search.h"
#include "../shared/help_provider.h"
#include "is_null_filter.h"
#include "batch_config.h"
#include "../shared/check_exist.h"
#include <wx/display.h>

using namespace ffs3;
using ffs3::CustomLocale;


class DirectoryNameMainImpl : public DirectoryNameMainDlg
{
public:
    DirectoryNameMainImpl(MainDialog&      mainDlg,
                          wxWindow*        dropWindow1,
                          wxWindow*        dropWindow2,
                          wxDirPickerCtrl* dirPicker,
                          wxComboBox*      dirName,
                          wxStaticBoxSizer* staticBox) :
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
                BatchDialog* batchDlg = new BatchDialog(&mainDlg_, droppedFiles[0]);
                if (batchDlg->ShowModal() == BatchDialog::BATCH_FILE_SAVED)
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
    virtual void OnLocalFilterCfgRemoveConfirm(wxCommandEvent& event)
    {
        FolderPairPanelBasic<GuiPanel>::OnLocalFilterCfgRemoveConfirm(event);
        mainDlg.updateFilterConfig(); //update filter
    }

    virtual void OnAltSyncCfgRemoveConfirm(wxCommandEvent& event)
    {
        FolderPairPanelBasic<GuiPanel>::OnAltSyncCfgRemoveConfirm(event);
        mainDlg.updateSyncConfig();
    }

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
        mainDlg.updateSyncConfig();
    }

    virtual void OnLocalFilterCfgChange()
    {
        mainDlg.updateFilterConfig();  //re-apply filter
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
        dirNameLeft( m_panelLeft,  m_dirPickerLeft,  m_directoryLeft),
        dirNameRight(m_panelRight, m_dirPickerRight, m_directoryRight) {}

    void setValues(const Zstring& leftDir, const Zstring& rightDir, AltSyncCfgPtr syncCfg, const FilterConfig& filter)
    {
        setConfig(syncCfg, filter);
        dirNameLeft.setName(leftDir);
        dirNameRight.setName(rightDir);
    }
    Zstring getLeftDir() const
    {
        return dirNameLeft.getName();
    }
    Zstring getRightDir() const
    {
        return dirNameRight.getName();
    }

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
                    mainDialog.m_panelLeft,
                    mainDialog.m_panelTopLeft,
                    mainDialog.m_dirPickerLeft,
                    mainDialog.m_directoryLeft,
                    mainDialog.sbSizerDirLeft),
        dirNameRight(mainDialog,
                     mainDialog.m_panelRight,
                     mainDialog.m_panelTopRight,
                     mainDialog.m_dirPickerRight,
                     mainDialog.m_directoryRight,
                     mainDialog.sbSizerDirRight) {}

    void setValues(const Zstring& leftDir, const Zstring& rightDir, AltSyncCfgPtr syncCfg, const FilterConfig& filter)
    {
        setConfig(syncCfg, filter);
        dirNameLeft.setName(leftDir);
        dirNameRight.setName(rightDir);
    }
    Zstring getLeftDir() const
    {
        return dirNameLeft.getName();
    }
    Zstring getRightDir() const
    {
        return dirNameRight.getName();
    }

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
        return !dirExists(ffs3::getFormattedDirectoryName(fp.leftDirectory)) ||
               !dirExists(ffs3::getFormattedDirectoryName(fp.rightDirectory));
    }
};


//##################################################################################################################################
MainDialog::MainDialog(const wxString& cfgFileName, xmlAccess::XmlGlobalSettings& settings) :
    MainDialogGenerated(NULL)
{
    xmlAccess::XmlGuiConfig guiCfg;  //structure to receive gui settings, already defaulted!!

    const wxString currentConfigFile = cfgFileName.empty() ? lastConfigFileName() : cfgFileName;

    bool loadCfgSuccess = false;
    if (!cfgFileName.empty() || fileExists(wxToZ(lastConfigFileName())))
    {
        //load XML
        try
        {
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
    }

    init(guiCfg,
         settings,
         !cfgFileName.empty() && loadCfgSuccess);

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
}


void MainDialog::init(const xmlAccess::XmlGuiConfig guiCfg,
                      xmlAccess::XmlGlobalSettings& settings,
                      bool startComparison)
{
    wxWindowUpdateLocker dummy(this); //avoid display distortion

    //--------- avoid mirroring this dialog in RTL languages like Hebrew or Arabic --------------------
    m_panelViewFilter->SetLayoutDirection(wxLayout_LeftToRight);
    m_panelStatusBar->SetLayoutDirection(wxLayout_LeftToRight);
//           if (GetLayoutDirection() == wxLayout_RightToLeft)
//{
//    bSizerGridHolder->Detach(m_panelRight);
//    bSizerGridHolder->Detach(m_panelLeft);
//    bSizerGridHolder->Add(m_panelLeft);
//    bSizerGridHolder->Prepend(m_panelRight);
//bSizerGridHolder->Fit(this);
//
//    bSizerGridHolder->Layout();
//}
//------------------------------------------------------------------------------------------------------

    globalSettings = &settings;
    gridDataView.reset(new ffs3::GridView);
    contextMenu.reset(new wxMenu); //initialize right-click context menu; will be dynamically re-created on each R-mouse-click

    compareStatus.reset(new CompareStatus(*this));
    cleanedUp = false;
    lastSortColumn = -1;
    lastSortGrid = NULL;

    updateFileIcons.reset(new IconUpdater(m_gridLeft, m_gridRight));

#ifdef FFS_WIN
    new MouseMoveWindow(*this, //allow moving main dialog by clicking (nearly) anywhere...
                        m_panel71,
                        m_panelBottom,
                        m_panelStatusBar); //ownership passed to "this"
#endif

    syncPreview.reset(new SyncPreview(this));

    SetTitle(wxString(wxT("FreeFileSync - ")) + _("Folder Comparison and Synchronization"));

    SetIcon(*GlobalResources::getInstance().programIcon); //set application icon


    //notify about (logical) application main window => program won't quit, but stay on this dialog
    ffs3::AppMainWindow::setMainWindow(this);

    //init handling of first folder pair
    firstFolderPair.reset(new DirectoryPairFirst(*this));

    initViewFilterButtons();

    //initialize and load configuration
    readGlobalSettings();
    setCurrentConfiguration(guiCfg);

    //set icons for this dialog
    m_bpButton10->SetBitmapLabel(GlobalResources::getInstance().getImageByName(wxT("exit")));
    m_buttonCompare->setBitmapFront(GlobalResources::getInstance().getImageByName(wxT("compare")));
    m_bpButtonSyncConfig->SetBitmapLabel(GlobalResources::getInstance().getImageByName(wxT("syncConfig")));
    m_bpButtonCmpConfig->SetBitmapLabel(GlobalResources::getInstance().getImageByName(wxT("cmpConfig")));
    m_bpButtonSave->SetBitmapLabel(GlobalResources::getInstance().getImageByName(wxT("save")));
    m_bpButtonLoad->SetBitmapLabel(GlobalResources::getInstance().getImageByName(wxT("load")));
    m_bpButtonAddPair->SetBitmapLabel(GlobalResources::getInstance().getImageByName(wxT("addFolderPair")));
    m_bitmap15->SetBitmap(GlobalResources::getInstance().getImageByName(wxT("statusEdge")));

    m_bitmapCreate->SetBitmap(GlobalResources::getInstance().getImageByName(wxT("create")));
    m_bitmapUpdate->SetBitmap(GlobalResources::getInstance().getImageByName(wxT("update")));
    m_bitmapDelete->SetBitmap(GlobalResources::getInstance().getImageByName(wxT("delete")));
    m_bitmapData->SetBitmap(GlobalResources::getInstance().getImageByName(wxT("data")));

    bSizer6->Layout(); //wxButtonWithImage size might have changed

    //menu icons: workaround for wxWidgets: small hack to update menu items: actually this is a wxWidgets bug (affects Windows- and Linux-build)
    MenuItemUpdater updateMenuFile(m_menuFile);
    updateMenuFile.addForUpdate(m_menuItem10,   GlobalResources::getInstance().getImageByName(wxT("compareSmall")));
    updateMenuFile.addForUpdate(m_menuItem11,   GlobalResources::getInstance().getImageByName(wxT("syncSmall")));
    updateMenuFile.addForUpdate(m_menuItemNew,  GlobalResources::getInstance().getImageByName(wxT("newSmall")));
    updateMenuFile.addForUpdate(m_menuItemSave, GlobalResources::getInstance().getImageByName(wxT("saveSmall")));
    updateMenuFile.addForUpdate(m_menuItemLoad, GlobalResources::getInstance().getImageByName(wxT("loadSmall")));

    MenuItemUpdater updateMenuAdv(m_menuAdvanced);
    updateMenuAdv.addForUpdate(m_menuItemGlobSett, GlobalResources::getInstance().getImageByName(wxT("settingsSmall")));
    updateMenuAdv.addForUpdate(m_menuItem7, GlobalResources::getInstance().getImageByName(wxT("batchSmall")));

    MenuItemUpdater updateMenuHelp(m_menuHelp);
    updateMenuHelp.addForUpdate(m_menuItemAbout, GlobalResources::getInstance().getImageByName(wxT("aboutSmall")));

#ifdef FFS_LINUX
    if (!ffs3::isPortableVersion()) //disable update check for Linux installer-based version -> handled by .deb
        m_menuItemCheckVer->Enable(false);
#endif

    //create language selection menu
    for (std::vector<LocInfoLine>::const_iterator i = LocalizationInfo::getMapping().begin(); i != LocalizationInfo::getMapping().end(); ++i)
    {
        wxMenuItem* newItem = new wxMenuItem(m_menuLanguages, wxID_ANY, i->languageName, wxEmptyString, wxITEM_NORMAL );
        newItem->SetBitmap(GlobalResources::getInstance().getImageByName(i->languageFlag));

        //map menu item IDs with language IDs: evaluated when processing event handler
        languageMenuItemMap.insert(std::map<MenuItemID, LanguageID>::value_type(newItem->GetId(), i->languageID));

        //connect event
        Connect(newItem->GetId(), wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(MainDialog::OnMenuLanguageSwitch));
        m_menuLanguages->Append(newItem);
    }

    //support for CTRL + C and DEL on grids
    m_gridLeft->Connect(wxEVT_KEY_DOWN, wxKeyEventHandler(MainDialog::onGridLeftButtonEvent), NULL, this);
    m_gridRight->Connect(wxEVT_KEY_DOWN, wxKeyEventHandler(MainDialog::onGridRightButtonEvent), NULL, this);
    m_gridMiddle->Connect(wxEVT_KEY_DOWN, wxKeyEventHandler(MainDialog::onGridMiddleButtonEvent), NULL, this);

    Connect(wxEVT_IDLE, wxEventHandler(MainDialog::OnIdleEvent), NULL, this);
    Connect(wxEVT_SIZE, wxSizeEventHandler(MainDialog::OnResize), NULL, this);
    Connect(wxEVT_MOVE, wxSizeEventHandler(MainDialog::OnResize), NULL, this);

    m_bpButtonFilter->Connect(wxEVT_RIGHT_DOWN, wxCommandEventHandler(MainDialog::OnGlobalFilterOpenContext), NULL, this);

    //calculate witdh of folder pair manually (if scrollbars are visible)
    m_scrolledWindowFolderPairs->Connect(wxEVT_SIZE, wxSizeEventHandler(MainDialog::OnResizeFolderPairs), NULL, this);

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

    //integrate the compare status panel (in hidden state)
    bSizer1->Insert(1, compareStatus->getAsWindow(), 0, wxEXPAND | wxBOTTOM, 5 );
    Layout();   //avoid screen flicker when panel is shown later
    compareStatus->getAsWindow()->Hide();

    //correct width of swap button above middle grid
    const wxSize source = m_gridMiddle->GetSize();
    const wxSize target = m_bpButtonSwapSides->GetSize();
    const int spaceToAdd = source.GetX() - target.GetX();
    bSizerMiddle->Insert(1, spaceToAdd / 2, 0, 0);
    bSizerMiddle->Insert(0, spaceToAdd - (spaceToAdd / 2), 0, 0);

    //register regular check for update on next idle event
    Connect(wxEVT_IDLE, wxIdleEventHandler(MainDialog::OnRegularUpdateCheck), NULL, this);

    //asynchronous call to wxWindow::Layout(): fix superfluous frame on right and bottom when FFS is started in fullscreen mode
    Connect(wxEVT_IDLE, wxIdleEventHandler(MainDialog::OnLayoutWindowAsync), NULL, this);

//----------------------------------------------------------------------------------------------------------------------------------------------------------------
    //some convenience: if FFS is started with a *.ffs_gui file as commandline parameter AND all directories contained exist, comparison shall be started right off
    if (startComparison)
    {
        const ffs3::MainConfiguration currMainCfg = getCurrentConfiguration().mainCfg;
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
}


void MainDialog::cleanUp(bool saveLastUsedConfig)
{
    if (!cleanedUp)
    {
        cleanedUp = true;

        //no need for wxEventHandler::Disconnect() here; done automatically when window is destoyed!

        m_gridLeft  ->release(); //handle wxGrid-related callback on grid data after MainDialog has died... (Linux only)
        m_gridMiddle->release();
        m_gridRight ->release();

        //save configuration
        if (saveLastUsedConfig)
            writeConfigurationToXml(lastConfigFileName());   //don't throw exceptions in destructors
        writeGlobalSettings();
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
    if (    widthNotMaximized  != wxDefaultCoord &&
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
    for (std::vector<wxString>::reverse_iterator i = globalSettings->gui.cfgFileHistory.rbegin();
            i != globalSettings->gui.cfgFileHistory.rend();
            ++i)
        addFileToCfgHistory(*i);

    //load list of last used folders
    for (std::vector<wxString>::reverse_iterator i = globalSettings->gui.folderHistoryLeft.rbegin();
            i != globalSettings->gui.folderHistoryLeft.rend();
            ++i)
        addLeftFolderToHistory(*i);
    for (std::vector<wxString>::reverse_iterator i = globalSettings->gui.folderHistoryRight.rbegin();
            i != globalSettings->gui.folderHistoryRight.rend();
            ++i)
        addRightFolderToHistory(*i);

    //show/hide file icons
    m_gridLeft->enableFileIcons(globalSettings->gui.showFileIconsLeft);
    m_gridRight->enableFileIcons(globalSettings->gui.showFileIconsRight);

    //set selected tab
    m_notebookBottomLeft->ChangeSelection(globalSettings->gui.selectedTabBottomLeft);
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
    globalSettings->gui.cfgFileHistory = cfgFileNames;

    //write list of last used folders
    globalSettings->gui.folderHistoryLeft.clear();
    const wxArrayString leftFolderHistory = m_directoryLeft->GetStrings();
    for (unsigned i = 0; i < leftFolderHistory.GetCount(); ++i)
        globalSettings->gui.folderHistoryLeft.push_back(leftFolderHistory[i]);

    globalSettings->gui.folderHistoryRight.clear();
    const wxArrayString rightFolderHistory = m_directoryRight->GetStrings();
    for (unsigned i = 0; i < rightFolderHistory.GetCount(); ++i)
        globalSettings->gui.folderHistoryRight.push_back(rightFolderHistory[i]);

    //get selected tab
    globalSettings->gui.selectedTabBottomLeft = m_notebookBottomLeft->GetSelection();
}


void MainDialog::setSyncDirManually(const std::set<size_t>& rowsToSetOnUiTable, const ffs3::SyncDirection dir)
{
    if (rowsToSetOnUiTable.size() > 0)
    {
        for (std::set<size_t>::const_iterator i = rowsToSetOnUiTable.begin(); i != rowsToSetOnUiTable.end(); ++i)
        {
            FileSystemObject* fsObj = gridDataView->getObject(*i);
            if (fsObj)
            {
                setSyncDirectionRec(dir, *fsObj); //set new direction (recursively)
                ffs3::setActiveStatus(true, *fsObj); //works recursively for directories
            }
        }

        updateGuiGrid();
    }
}


void MainDialog::filterRangeManually(const std::set<size_t>& rowsToFilterOnUiTable, int leadingRow)
{
    if (rowsToFilterOnUiTable.size() > 0)
    {
        bool newSelection = false; //default: deselect range

        //leadingRow determines de-/selection of all other rows
        const FileSystemObject* fsObj = gridDataView->getObject(leadingRow);
        if (fsObj)
            newSelection = !fsObj->isActive();

        //if hidefiltered is active, there should be no filtered elements on screen => current element was filtered out
        assert(!currentCfg.hideFilteredElements || !newSelection);

        //get all lines that need to be filtered
        std::vector<FileSystemObject*> compRef;
        gridDataView->getAllFileRef(rowsToFilterOnUiTable, compRef); //everything in compRef is bound

        for (std::vector<FileSystemObject*>::iterator i = compRef.begin(); i != compRef.end(); ++i)
            ffs3::setActiveStatus(newSelection, **i); //works recursively for directories

        refreshGridAfterFilterChange(400); //call this instead of updateGuiGrid() to add some delay if hideFiltered == true and to handle some graphical artifacts
    }
}


void MainDialog::OnIdleEvent(wxEvent& event)
{
    //small routine to restore status information after some time
    if (stackObjects.size() > 0 )  //check if there is some work to do
    {
        wxLongLong currentTime = wxGetLocalTimeMillis();
        if (currentTime - lastStatusChange > 2500) //restore stackObject after two seconds
        {
            lastStatusChange = currentTime;

            m_staticTextStatusMiddle->SetLabel(stackObjects.top());
            stackObjects.pop();

            if (stackObjects.empty())
                m_staticTextStatusMiddle->SetForegroundColour(*wxBLACK); //reset color

            m_panelStatusBar->Layout();
        }
    }

    event.Skip();
}


void MainDialog::copySelectionToClipboard(const CustomGrid* selectedGrid)
{
    const std::set<size_t> selectedRows = getSelectedRows(selectedGrid);
    if (selectedRows.size() > 0)
    {
        wxString clipboardString;

        for (std::set<size_t>::const_iterator i = selectedRows.begin(); i != selectedRows.end(); ++i)
        {
            for (int k = 0; k < const_cast<CustomGrid*>(selectedGrid)->GetNumberCols(); ++k)
            {
                clipboardString+= const_cast<CustomGrid*>(selectedGrid)->GetCellValue(static_cast<int>(*i), k);
                if (k != const_cast<CustomGrid*>(selectedGrid)->GetNumberCols() - 1)
                    clipboardString+= '\t';
            }
            clipboardString+= '\n';
        }

        if (!clipboardString.IsEmpty())
            // Write text to the clipboard
            if (wxTheClipboard->Open())
            {
                // these data objects are held by the clipboard,
                // so do not delete them in the app.
                wxTheClipboard->SetData( new wxTextDataObject(clipboardString) );
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
    ManualDeletionHandler(MainDialog* main, size_t totalObjToDel) :
        mainDlg(main),
        totalObjToDelete(totalObjToDel),
        abortRequested(false),
        ignoreErrors(false),
        deletionCount(0)
    {
        mainDlg->disableAllElements(); //disable everything except abort button
        mainDlg->clearStatusBar();

        //register abort button
        mainDlg->m_buttonAbort->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( ManualDeletionHandler::OnAbortCompare ), NULL, this );
    }

    ~ManualDeletionHandler()
    {
        //de-register abort button
        mainDlg->m_buttonAbort->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( ManualDeletionHandler::OnAbortCompare ), NULL, this );

        mainDlg->enableAllElements();
    }

    virtual Response reportError(const wxString& errorMessage)
    {
        if (abortRequested)
            throw ffs3::AbortThisProcess();

        if (ignoreErrors)
            return DeleteFilesHandler::IGNORE_ERROR;

        bool ignoreNextErrors = false;
        ErrorDlg* errorDlg = new ErrorDlg(NULL,
                                          ErrorDlg::BUTTON_IGNORE |  ErrorDlg::BUTTON_RETRY | ErrorDlg::BUTTON_ABORT,
                                          errorMessage, ignoreNextErrors);
        const int rv = errorDlg->ShowModal();
        errorDlg->Destroy();
        switch (static_cast<ErrorDlg::ReturnCodes>(rv))
        {
        case ErrorDlg::BUTTON_IGNORE:
            ignoreErrors = ignoreNextErrors;
            return DeleteFilesHandler::IGNORE_ERROR;
        case ErrorDlg::BUTTON_RETRY:
            return DeleteFilesHandler::RETRY;
        case ErrorDlg::BUTTON_ABORT:
            throw ffs3::AbortThisProcess();
        }

        assert (false);
        return DeleteFilesHandler::IGNORE_ERROR; //dummy return value
    }

    virtual void deletionSuccessful(size_t deletedItems)  //called for each file/folder that has been deleted
    {
        deletionCount += deletedItems;

        if (updateUiIsAllowed())  //test if specific time span between ui updates is over
        {
            wxString statusMessage = _("%x / %y objects deleted successfully");
            statusMessage.Replace(wxT("%x"), ffs3::numberToStringSep(deletionCount), false);
            statusMessage.Replace(wxT("%y"), ffs3::numberToStringSep(totalObjToDelete), false);

            mainDlg->m_staticTextStatusMiddle->SetLabel(statusMessage);
            mainDlg->m_panelStatusBar->Layout();

            updateUiNow();
        }

        if (abortRequested) //test after (implicit) call to wxApp::Yield()
            throw ffs3::AbortThisProcess();
    }

private:
    void OnAbortCompare(wxCommandEvent& event) //handle abort button click
    {
        abortRequested = true; //don't throw exceptions in a GUI-Callback!!! (throw ffs3::AbortThisProcess())
    }

    MainDialog* const mainDlg;
    const size_t totalObjToDelete;

    bool abortRequested;
    bool ignoreErrors;
    size_t deletionCount;
};


void MainDialog::deleteSelectedFiles()
{
    //get set of selected rows on view
    const std::set<size_t> viewSelectionLeft  = getSelectedRows(m_gridLeft);
    const std::set<size_t> viewSelectionRight = getSelectedRows(m_gridRight);

    if (viewSelectionLeft.size() + viewSelectionRight.size())
    {
        //map lines from GUI view to grid line references
        std::vector<FileSystemObject*> compRefLeft;
        gridDataView->getAllFileRef(viewSelectionLeft, compRefLeft);

        std::vector<FileSystemObject*> compRefRight;
        gridDataView->getAllFileRef(viewSelectionRight, compRefRight);


        int totalDeleteCount = 0;

        if (ffs3::showDeleteDialog(compRefLeft,
                                   compRefRight,
                                   globalSettings->gui.deleteOnBothSides,
                                   globalSettings->gui.useRecyclerForManualDeletion,
                                   totalDeleteCount) == DefaultReturnCode::BUTTON_OKAY)
        {
            if (globalSettings->gui.useRecyclerForManualDeletion && !ffs3::recycleBinExists())
            {
                wxMessageBox(_("Recycle Bin not yet supported for this system!"));
                return;
            }

            try
            {
                //handle errors when deleting files/folders
                ManualDeletionHandler statusHandler(this, totalDeleteCount);

                ffs3::deleteFromGridAndHD(gridDataView->getDataTentative(),
                                          compRefLeft,
                                          compRefRight,
                                          globalSettings->gui.deleteOnBothSides,
                                          globalSettings->gui.useRecyclerForManualDeletion,
                                          getCurrentConfiguration().mainCfg,
                                          statusHandler);
            }
            catch (ffs3::AbortThisProcess&) {}

            //remove rows that empty: just a beautification, invalid rows shouldn't cause issues
            gridDataView->removeInvalidRows();

            //redraw grid neccessary to update new dimensions and for UI-Backend data linkage
            updateGuiGrid(); //call immediately after deleteFromGridAndHD!!!

            m_gridLeft->  ClearSelection();
            m_gridMiddle->ClearSelection();
            m_gridRight-> ClearSelection();
        }
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
                wxExecute(wxString(wxT("explorer ")) + zToWx(fsObj->getBaseDirPf<LEFT_SIDE>()));
            else
                wxExecute(wxString(wxT("explorer ")) + zToWx(fsObj->getBaseDirPf<RIGHT_SIDE>()));
            return;
        }
#endif
    }
    else
    {
        //fallback
        dir   = zToWx(ffs3::getFormattedDirectoryName(firstFolderPair->getLeftDir()));
        dirCo = zToWx(ffs3::getFormattedDirectoryName(firstFolderPair->getRightDir()));

        if (!leftSide)
            std::swap(dir, dirCo);

#ifdef FFS_WIN
        wxExecute(wxString(wxT("explorer ")) + dir); //default
        return;
#endif
    }

    command.Replace(wxT("%nameCo"), nameCo, true); //attention: replace %nameCo, %dirCo BEFORE %name, %dir to handle dependency
    command.Replace(wxT("%dirCo"),  dirCo,  true);
    command.Replace(wxT("%name"),   name,   true);
    command.Replace(wxT("%dir"),    dir,    true);

    wxExecute(command);
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

    m_staticTextStatusMiddle->SetForegroundColour(*wxBLACK); //reset color
    m_staticTextStatusLeft->SetLabel(wxEmptyString);
    m_staticTextStatusMiddle->SetLabel(wxEmptyString);
    m_staticTextStatusRight->SetLabel(wxEmptyString);
}


void MainDialog::disableAllElements()
{
    //disenables all elements (except abort button) that might receive user input during long-running processes: comparison, deletion
    m_bpButtonCmpConfig-> Disable();
    m_notebookBottomLeft->Disable();
    m_checkBoxHideFilt->  Disable();
    m_bpButtonSyncConfig->Disable();
    m_buttonStartSync->   Disable();
    m_dirPickerLeft->     Disable();
    m_dirPickerRight->    Disable();
    m_bpButtonSwapSides-> Disable();
    m_bpButtonLeftOnly->  Disable();
    m_bpButtonLeftNewer-> Disable();
    m_bpButtonEqual->     Disable();
    m_bpButtonDifferent-> Disable();
    m_bpButtonRightNewer->Disable();
    m_bpButtonRightOnly-> Disable();
    m_panelLeft->         Disable();
    m_panelMiddle->       Disable();
    m_panelRight->        Disable();
    m_panelTopLeft->      Disable();
    m_panelTopMiddle->    Disable();
    m_panelTopRight->     Disable();
    m_bpButton10->        Disable();
    m_scrolledWindowFolderPairs->Disable();
    m_menubar1->EnableTop(0, false);
    m_menubar1->EnableTop(1, false);
    m_menubar1->EnableTop(2, false);
    EnableCloseButton(false);

    //show abort button
    m_buttonAbort->Enable();
    m_buttonAbort->Show();
    m_buttonCompare->Disable();
    m_buttonCompare->Hide();
    m_buttonAbort->SetFocus();
}


void MainDialog::enableAllElements()
{
    m_bpButtonCmpConfig-> Enable();
    m_notebookBottomLeft->Enable();
    m_checkBoxHideFilt->  Enable();
    m_bpButtonSyncConfig->Enable();
    m_buttonStartSync->   Enable();
    m_dirPickerLeft->     Enable();
    m_dirPickerRight->    Enable();
    m_bpButtonSwapSides-> Enable();
    m_bpButtonLeftOnly->  Enable();
    m_bpButtonLeftNewer-> Enable();
    m_bpButtonEqual->     Enable();
    m_bpButtonDifferent-> Enable();
    m_bpButtonRightNewer->Enable();
    m_bpButtonRightOnly-> Enable();
    m_panelLeft->         Enable();
    m_panelMiddle->       Enable();
    m_panelRight->        Enable();
    m_panelTopLeft->      Enable();
    m_panelTopMiddle->    Enable();
    m_panelTopRight->     Enable();
    m_bpButton10->        Enable();
    m_scrolledWindowFolderPairs->Enable();
    m_menubar1->EnableTop(0, true);
    m_menubar1->EnableTop(1, true);
    m_menubar1->EnableTop(2, true);
    EnableCloseButton(true);

    //show compare button
    m_buttonAbort->Disable();
    m_buttonAbort->Hide();
    m_buttonCompare->Enable();
    m_buttonCompare->Show();
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
        if (width > 0 && height > 0 && x >= -3360 && y >= -200)
        {
            widthNotMaximized  = width;                        //visible coordinates x < 0 and y < 0 are possible with dual monitors!
            heightNotMaximized = height;

            posXNotMaximized = x;
            posYNotMaximized = y;
        }
    }

    event.Skip();
}


void MainDialog::OnResizeFolderPairs(wxSizeEvent& event)
{
    //adapt left-shift display distortion caused by scrollbars for multiple folder pairs
    if (additionalFolderPairs.size() > 0)
    {
        const int width = m_panelTopLeft->GetSize().GetWidth();

        for (std::vector<DirectoryPair*>::iterator i = additionalFolderPairs.begin(); i != additionalFolderPairs.end(); ++i)
            (*i)->m_panelLeft->SetMinSize(wxSize(width, -1));
    }

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
            copySelectionToClipboard(m_gridLeft);
            break;

        case 'A': //CTRL + A
            m_gridLeft->SelectAll();
            break;

        case 'F': //CTRL + F
            ffs3::startFind(*this, *m_gridLeft, *m_gridRight, globalSettings->gui.textSearchRespectCase);
            break;

        case WXK_NUMPAD_ADD: //CTRL + '+'
            m_gridLeft->autoSizeColumns();
            break;
        }

    else if (event.AltDown())
        switch (keyCode)
        {
        case WXK_LEFT: //ALT + <-
        {
            wxCommandEvent dummy;
            OnContextSyncDirLeft(dummy);
        }
        break;

        case WXK_RIGHT: //ALT + ->
        {
            wxCommandEvent dummy;
            OnContextSyncDirRight(dummy);
        }
        break;

        case WXK_UP:   /* ALT + /|\   */
        case WXK_DOWN: /* ALT + \|/   */
        {
            wxCommandEvent dummy;
            OnContextSyncDirNone(dummy);
        }
        break;
        }

    else
        switch (keyCode)
        {
        case WXK_DELETE:
        case WXK_NUMPAD_DELETE:
            deleteSelectedFiles();
            break;

        case WXK_SPACE:
        {
            wxCommandEvent dummy;
            OnContextFilterTemp(dummy);
        }
        break;

        case WXK_RETURN:
        case WXK_NUMPAD_ENTER:
        {
            wxCommandEvent dummy(wxEVT_NULL, externalAppIDFirst); //open with first external application
            OnContextOpenWith(dummy);
        }
        break;

        case WXK_F3:        //F3
        case WXK_NUMPAD_F3: //
            ffs3::findNext(*this, *m_gridLeft, *m_gridRight, globalSettings->gui.textSearchRespectCase);
            break;
        }

    //event.Skip(); -> swallow event! don't allow default grid commands!
}


void MainDialog::onGridMiddleButtonEvent(wxKeyEvent& event)
{
    const int keyCode = event.GetKeyCode();

    if (event.ControlDown())
    {
        if (keyCode == 67 || keyCode == WXK_INSERT) //CTRL + C || CTRL + INS
            copySelectionToClipboard(m_gridMiddle);
    }

    //event.Skip(); -> swallow event! don't allow default grid commands!
}


void MainDialog::onGridRightButtonEvent(wxKeyEvent& event)
{
    const int keyCode = event.GetKeyCode();

    if (event.ControlDown())
        switch (keyCode)
        {
        case 'C':
        case WXK_INSERT: //CTRL + C || CTRL + INS
            copySelectionToClipboard(m_gridRight);
            break;

        case 'A': //CTRL + A
            m_gridRight->SelectAll();
            break;

        case 'F': //CTRL + F
            ffs3::startFind(*this, *m_gridLeft, *m_gridRight, globalSettings->gui.textSearchRespectCase);
            break;

        case WXK_NUMPAD_ADD: //CTRL + '+'
            m_gridRight->autoSizeColumns();
            break;
        }

    else if (event.AltDown())
        switch (keyCode)
        {
        case WXK_LEFT: //ALT + <-
        {
            wxCommandEvent dummy;
            OnContextSyncDirLeft(dummy);
        }
        break;

        case WXK_RIGHT: //ALT + ->
        {
            wxCommandEvent dummy;
            OnContextSyncDirRight(dummy);
        }
        break;

        case WXK_UP:   /* ALT + /|\   */
        case WXK_DOWN: /* ALT + \|/   */
        {
            wxCommandEvent dummy;
            OnContextSyncDirNone(dummy);
        }
        break;
        }

    else
        switch (keyCode)
        {
        case WXK_DELETE:
        case WXK_NUMPAD_DELETE:
            deleteSelectedFiles();
            break;

        case WXK_SPACE:
        {
            wxCommandEvent dummy;
            OnContextFilterTemp(dummy);
        }
        break;

        case WXK_RETURN:
        case WXK_NUMPAD_ENTER:
        {
            wxCommandEvent dummy(wxEVT_NULL, externalAppIDFirst); //open with first external application
            OnContextOpenWith(dummy);
        }
        break;

        case WXK_F3:        //F3
        case WXK_NUMPAD_F3: //
            ffs3::findNext(*this, *m_gridLeft, *m_gridRight, globalSettings->gui.textSearchRespectCase);
            break;
        }

    //event.Skip(); -> swallow event! don't allow default grid commands!
}



//------------------------------------------------------------
//temporal variables used by exclude via context menu
struct SelectedExtension : public wxObject
{
    SelectedExtension(const Zstring& ext) : extension(ext) {}

    Zstring extension;
};

struct FilterObject
{
    FilterObject(const Zstring& relName, bool isDirectory) :
        relativeName(relName),
        isDir(isDirectory) {}
    Zstring relativeName;
    bool isDir;
};

typedef std::vector<FilterObject> FilterObjList;

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


    const std::set<size_t> selectionLeft  = getSelectedRows(m_gridLeft);
    const std::set<size_t> selectionRight = getSelectedRows(m_gridRight);

    const size_t selectionBegin = selectionLeft.size() + selectionRight.size() == 0 ? 0 :
                                  selectionLeft.size()  == 0 ? *selectionRight.begin() :
                                  selectionRight.size() == 0 ? *selectionLeft.begin()  :
                                  std::min(*selectionLeft.begin(), *selectionRight.begin());

    const FileSystemObject* fsObj = gridDataView->getObject(selectionBegin);


    //#######################################################
    //re-create context menu
    contextMenu.reset(new wxMenu);

    if (syncPreview->previewIsEnabled() &&
            fsObj && fsObj->getSyncOperation() != SO_EQUAL)
    {
        if (selectionLeft.size() + selectionRight.size() > 0)
        {
            //CONTEXT_SYNC_DIR_LEFT
            wxMenuItem* menuItemSyncDirLeft = new wxMenuItem(contextMenu.get(), CONTEXT_SYNC_DIR_LEFT, wxString(_("Set direction:")) +
                    wxT(" ") + getSymbol(fsObj->testSyncOperation(true, SYNC_DIR_LEFT)) +
                    wxT("\tALT + LEFT")); //Linux needs a direction, "<-", because it has no context menu icons!
            menuItemSyncDirLeft->SetBitmap(getSyncOpImage(fsObj->testSyncOperation(true, SYNC_DIR_LEFT)));
            contextMenu->Append(menuItemSyncDirLeft);

            //CONTEXT_SYNC_DIR_NONE
            wxMenuItem* menuItemSyncDirNone = new wxMenuItem(contextMenu.get(), CONTEXT_SYNC_DIR_NONE, wxString(_("Set direction:")) +
                    wxT(" ") + getSymbol(fsObj->testSyncOperation(true, SYNC_DIR_NONE)) +
                    wxT("\tALT + UP"));
            menuItemSyncDirNone->SetBitmap(getSyncOpImage(fsObj->testSyncOperation(true, SYNC_DIR_NONE)));
            contextMenu->Append(menuItemSyncDirNone);

            //CONTEXT_SYNC_DIR_RIGHT
            wxMenuItem* menuItemSyncDirRight = new wxMenuItem(contextMenu.get(), CONTEXT_SYNC_DIR_RIGHT, wxString(_("Set direction:")) +
                    wxT(" ") + getSymbol(fsObj->testSyncOperation(true, SYNC_DIR_RIGHT)) +
                    wxT("\tALT + RIGHT"));
            menuItemSyncDirRight->SetBitmap(getSyncOpImage(fsObj->testSyncOperation(true, SYNC_DIR_RIGHT)));
            contextMenu->Append(menuItemSyncDirRight);

            contextMenu->AppendSeparator();
        }
    }


    //CONTEXT_FILTER_TEMP
    if (fsObj && (selectionLeft.size() + selectionRight.size() > 0))
    {
        if (fsObj->isActive())
        {
            wxMenuItem* menuItemExclTemp = new wxMenuItem(contextMenu.get(), CONTEXT_FILTER_TEMP, wxString(_("Exclude temporarily")) + wxT("\tSPACE"));
            menuItemExclTemp->SetBitmap(GlobalResources::getInstance().getImageByName(wxT("checkboxFalse")));
            contextMenu->Append(menuItemExclTemp);
        }
        else
        {
            wxMenuItem* menuItemInclTemp = new wxMenuItem(contextMenu.get(), CONTEXT_FILTER_TEMP, wxString(_("Include temporarily")) + wxT("\tSPACE"));
            menuItemInclTemp->SetBitmap(GlobalResources::getInstance().getImageByName(wxT("checkboxTrue")));
            contextMenu->Append(menuItemInclTemp);
        }
    }
    else
    {
        contextMenu->Append(CONTEXT_FILTER_TEMP, wxString(_("Exclude temporarily")) + wxT("\tSPACE")); //this element should always be visible
        contextMenu->Enable(CONTEXT_FILTER_TEMP, false);
    }

    //###############################################################################################
    //get list of relative file/dir-names for filtering
    FilterObjList exFilterCandidateObj;

    class AddFilter : public FSObjectVisitor
    {
    public:
        AddFilter(FilterObjList& fl) : filterList_(fl) {}
        virtual void visit(const FileMapping& fileObj)
        {
            filterList_.push_back(FilterObject(fileObj.getObjRelativeName(), false));
        }
        virtual void visit(const SymLinkMapping& linkObj)
        {
            filterList_.push_back(FilterObject(linkObj.getObjRelativeName(), false));
        }
        virtual void visit(const DirMapping& dirObj)
        {
            filterList_.push_back(FilterObject(dirObj.getObjRelativeName(), true));
        }

    private:
        FilterObjList& filterList_;
    } newFilterEntry(exFilterCandidateObj);


    for (std::set<size_t>::const_iterator i = selectionLeft.begin(); i != selectionLeft.end(); ++i)
    {
        const FileSystemObject* currObj = gridDataView->getObject(*i);
        if (currObj && !currObj->isEmpty<LEFT_SIDE>())
            currObj->accept(newFilterEntry);
    }
    for (std::set<size_t>::const_iterator i = selectionRight.begin(); i != selectionRight.end(); ++i)
    {
        const FileSystemObject* currObj = gridDataView->getObject(*i);
        if (currObj && !currObj->isEmpty<RIGHT_SIDE>())
            currObj->accept(newFilterEntry);
    }
    //###############################################################################################

    //CONTEXT_EXCLUDE_EXT
    if (exFilterCandidateObj.size() > 0 && !exFilterCandidateObj[0].isDir)
    {
        const Zstring filename = exFilterCandidateObj[0].relativeName.AfterLast(common::FILE_NAME_SEPARATOR);
        if (filename.find(Zchar('.')) !=  Zstring::npos) //be careful: AfterLast would return the whole string if '.' were not found!
        {
            const Zstring extension = filename.AfterLast(Zchar('.'));

            //add context menu item
            wxMenuItem* menuItemExclExt = new wxMenuItem(contextMenu.get(), CONTEXT_EXCLUDE_EXT, wxString(_("Exclude via filter:")) + wxT(" ") + wxT("*.") + zToWx(extension));
            menuItemExclExt->SetBitmap(GlobalResources::getInstance().getImageByName(wxT("filterSmall")));
            contextMenu->Append(menuItemExclExt);

            //connect event
            contextMenu->Connect(CONTEXT_EXCLUDE_EXT,
                                 wxEVT_COMMAND_MENU_SELECTED,
                                 wxCommandEventHandler(MainDialog::OnContextExcludeExtension),
                                 new SelectedExtension(extension), //ownership passed!
                                 this);
        }
    }


    //CONTEXT_EXCLUDE_OBJ
    wxMenuItem* menuItemExclObj = NULL;
    if (exFilterCandidateObj.size() == 1)
        menuItemExclObj = new wxMenuItem(contextMenu.get(), CONTEXT_EXCLUDE_OBJ, wxString(_("Exclude via filter:")) + wxT(" ") + zToWx(exFilterCandidateObj[0].relativeName.AfterLast(common::FILE_NAME_SEPARATOR)));
    else if (exFilterCandidateObj.size() > 1)
        menuItemExclObj = new wxMenuItem(contextMenu.get(), CONTEXT_EXCLUDE_OBJ, wxString(_("Exclude via filter:")) + wxT(" ") + _("<multiple selection>"));

    if (menuItemExclObj != NULL)
    {
        menuItemExclObj->SetBitmap(GlobalResources::getInstance().getImageByName(wxT("filterSmall")));
        contextMenu->Append(menuItemExclObj);

        //connect event
        contextMenu->Connect(CONTEXT_EXCLUDE_OBJ,
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
                                        (selectionLeft.size() + selectionRight.size() == 1);

        int newID = externalAppIDFirst;
        for (xmlAccess::ExternalApps::iterator i = globalSettings->gui.externelApplications.begin();
                i != globalSettings->gui.externelApplications.end();
                ++i, ++newID)
        {
            //some trick to translate default external apps on the fly: 1. "open in explorer" 2. "start directly"
            wxString description = wxGetTranslation(i->first);
            if (description.empty())
                description = wxT(" "); //wxWidgets doesn't like empty items

            if (i == globalSettings->gui.externelApplications.begin())
                contextMenu->Append(newID, description + wxT("\t") + wxString(_("D-Click")) + wxT("; ENTER"));
            else
                contextMenu->Append(newID, description);

            contextMenu->Enable(newID, externalAppEnabled);

            //register event
            contextMenu->Connect(newID, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(MainDialog::OnContextOpenWith), NULL, this);
        }
    }


    contextMenu->AppendSeparator();


    //CONTEXT_CLIPBOARD
    contextMenu->Append(CONTEXT_CLIPBOARD, _("Copy to clipboard\tCTRL+C"));

    if (    (m_gridLeft->isLeadGrid()  && selectionLeft.size()) ||
            (m_gridRight->isLeadGrid() && selectionRight.size()))
        contextMenu->Enable(CONTEXT_CLIPBOARD, true);
    else
        contextMenu->Enable(CONTEXT_CLIPBOARD, false);


    //CONTEXT_DELETE_FILES
    contextMenu->Append(CONTEXT_DELETE_FILES, _("Delete files\tDEL"));

    if (selectionLeft.size() + selectionRight.size() == 0)
        contextMenu->Enable(CONTEXT_DELETE_FILES, false);


    //###############################################################################################
    //connect events
    contextMenu->Connect(CONTEXT_FILTER_TEMP,    wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(MainDialog::OnContextFilterTemp),       NULL, this);
    contextMenu->Connect(CONTEXT_CLIPBOARD,      wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(MainDialog::OnContextCopyClipboard),    NULL, this);
    contextMenu->Connect(CONTEXT_DELETE_FILES,   wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(MainDialog::OnContextDeleteFiles),      NULL, this);
    contextMenu->Connect(CONTEXT_SYNC_DIR_LEFT,  wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(MainDialog::OnContextSyncDirLeft),      NULL, this);
    contextMenu->Connect(CONTEXT_SYNC_DIR_NONE,  wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(MainDialog::OnContextSyncDirNone),      NULL, this);
    contextMenu->Connect(CONTEXT_SYNC_DIR_RIGHT, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(MainDialog::OnContextSyncDirRight),     NULL, this);

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
        addExcludeFiltering(newExclude, gridDataView->getDataTentative());
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
            for (std::vector<FilterObject>::const_iterator i = objCont->selectedObjects.begin(); i != objCont->selectedObjects.end(); ++i)
            {
                if (i != objCont->selectedObjects.begin())
                    newExclude += Zstr("\n");

                newExclude += common::FILE_NAME_SEPARATOR + i->relativeName;
                if (i->isDir)
                    newExclude += common::FILE_NAME_SEPARATOR;
            }

            //add to filter config
            Zstring& excludeFilter = currentCfg.mainCfg.globalFilter.excludeFilter;
            if (!excludeFilter.empty() && !excludeFilter.EndsWith(Zstr("\n")))
                excludeFilter += Zstr("\n");
            excludeFilter += newExclude;

            updateFilterButtons();

            //do not fully apply filter, just exclude new items
            addExcludeFiltering(newExclude, gridDataView->getDataTentative());
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


void MainDialog::OnContextCopyClipboard(wxCommandEvent& event)
{
    if (m_gridLeft->isLeadGrid())
        copySelectionToClipboard(m_gridLeft);
    else if (m_gridRight->isLeadGrid())
        copySelectionToClipboard(m_gridRight);
}


void MainDialog::OnContextOpenWith(wxCommandEvent& event)
{
    if (m_gridLeft->isLeadGrid() || m_gridRight->isLeadGrid())
    {
        const CustomGrid* leadGrid = m_gridLeft->isLeadGrid() ?
                                     static_cast<CustomGrid*>(m_gridLeft) :
                                     static_cast<CustomGrid*>(m_gridRight);
        std::set<size_t> selection = getSelectedRows(leadGrid);

        const int index = event.GetId() - externalAppIDFirst;

        if (        selection.size() == 1     &&
                    0 <= index && static_cast<size_t>(index) <  globalSettings->gui.externelApplications.size())
            openExternalApplication(*selection.begin(), m_gridLeft->isLeadGrid(), globalSettings->gui.externelApplications[index].second);
    }
}


void MainDialog::OnContextDeleteFiles(wxCommandEvent& event)
{
    deleteSelectedFiles();
}


void MainDialog::OnContextSyncDirLeft(wxCommandEvent& event)
{
    //merge selections from left and right grid
    const std::set<size_t> selection = getSelectedRows();
    setSyncDirManually(selection, ffs3::SYNC_DIR_LEFT);
}


void MainDialog::OnContextSyncDirNone(wxCommandEvent& event)
{
    //merge selections from left and right grid
    const std::set<size_t> selection = getSelectedRows();
    setSyncDirManually(selection, ffs3::SYNC_DIR_NONE);
}


void MainDialog::OnContextSyncDirRight(wxCommandEvent& event)
{
    //merge selections from left and right grid
    const std::set<size_t> selection = getSelectedRows();
    setSyncDirManually(selection, ffs3::SYNC_DIR_RIGHT);
}


void MainDialog::OnContextRimLabelLeft(wxGridEvent& event)
{
    contextMenu.reset(new wxMenu); //re-create context menu
    contextMenu->Append(CONTEXT_CUSTOMIZE_COLUMN_LEFT, _("Customize..."));

    contextMenu->AppendSeparator();

    wxMenuItem* itemAutoAdjust = new wxMenuItem(contextMenu.get(), CONTEXT_AUTO_ADJUST_COLUMN_LEFT, _("Auto-adjust columns"), wxEmptyString, wxITEM_CHECK);
    contextMenu->Append(itemAutoAdjust);
    itemAutoAdjust->Check(globalSettings->gui.autoAdjustColumnsLeft);

    contextMenu->Connect(CONTEXT_CUSTOMIZE_COLUMN_LEFT, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(MainDialog::OnContextCustColumnLeft), NULL, this);
    contextMenu->Connect(CONTEXT_AUTO_ADJUST_COLUMN_LEFT, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(MainDialog::OnContextAutoAdjustLeft), NULL, this);

    PopupMenu(contextMenu.get()); //show context menu
}


void MainDialog::OnContextRimLabelRight(wxGridEvent& event)
{
    contextMenu.reset(new wxMenu); //re-create context menu
    contextMenu->Append(CONTEXT_CUSTOMIZE_COLUMN_RIGHT, _("Customize..."));

    contextMenu->AppendSeparator();

    wxMenuItem* itemAutoAdjust = new wxMenuItem(contextMenu.get(), CONTEXT_AUTO_ADJUST_COLUMN_RIGHT, _("Auto-adjust columns"), wxEmptyString, wxITEM_CHECK);
    contextMenu->Append(itemAutoAdjust);
    itemAutoAdjust->Check(globalSettings->gui.autoAdjustColumnsRight);

    contextMenu->Connect(CONTEXT_CUSTOMIZE_COLUMN_RIGHT, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(MainDialog::OnContextCustColumnRight), NULL, this);
    contextMenu->Connect(CONTEXT_AUTO_ADJUST_COLUMN_RIGHT, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(MainDialog::OnContextAutoAdjustRight), NULL, this);

    PopupMenu(contextMenu.get()); //show context menu
}


void MainDialog::OnContextCustColumnLeft(wxCommandEvent& event)
{
    xmlAccess::ColumnAttributes colAttr = m_gridLeft->getColumnAttributes();

    if (ffs3::showCustomizeColsDlg(colAttr) == DefaultReturnCode::BUTTON_OKAY)
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

    if (ffs3::showCustomizeColsDlg(colAttr) == DefaultReturnCode::BUTTON_OKAY)
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
    contextMenu.reset(new wxMenu); //re-create context menu

    contextMenu->Append(CONTEXT_CHECK_ALL, _("Include all rows"));
    contextMenu->Append(CONTEXT_UNCHECK_ALL, _("Exclude all rows"));

    if (gridDataView->rowsTotal() == 0)
    {
        contextMenu->Enable(CONTEXT_CHECK_ALL, false);
        contextMenu->Enable(CONTEXT_UNCHECK_ALL, false);
    }

    contextMenu->Connect(CONTEXT_CHECK_ALL,    wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(MainDialog::OnContextIncludeAll),      NULL, this);
    contextMenu->Connect(CONTEXT_UNCHECK_ALL,  wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(MainDialog::OnContextExcludeAll),      NULL, this);

    PopupMenu(contextMenu.get()); //show context menu
}


void MainDialog::OnContextMiddleLabel(wxGridEvent& event)
{
    contextMenu.reset(new wxMenu); //re-create context menu

    wxMenuItem* itemSyncPreview = new wxMenuItem(contextMenu.get(), CONTEXT_SYNC_PREVIEW, _("Synchronization Preview"));
    wxMenuItem* itemCmpResult   = new wxMenuItem(contextMenu.get(), CONTEXT_COMPARISON_RESULT, _("Comparison Result"));

    if (syncPreview->previewIsEnabled())
        itemSyncPreview->SetBitmap(GlobalResources::getInstance().getImageByName(wxT("syncViewSmall")));
    else
        itemCmpResult->SetBitmap(GlobalResources::getInstance().getImageByName(wxT("cmpViewSmall")));

    contextMenu->Append(itemCmpResult);
    contextMenu->Append(itemSyncPreview);

    contextMenu->Connect(CONTEXT_SYNC_PREVIEW,       wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(MainDialog::OnContextSyncView),       NULL, this);
    contextMenu->Connect(CONTEXT_COMPARISON_RESULT,  wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(MainDialog::OnContextComparisonView), NULL, this);

    PopupMenu(contextMenu.get()); //show context menu
}


void MainDialog::OnContextIncludeAll(wxCommandEvent& event)
{
    ffs3::setActiveStatus(true, gridDataView->getDataTentative());
    refreshGridAfterFilterChange(0); //call this instead of updateGuiGrid() to add some delay if hideFiltered == true and to handle some graphical artifacts        break;
}


void MainDialog::OnContextExcludeAll(wxCommandEvent& event)
{
    ffs3::setActiveStatus(false, gridDataView->getDataTentative());
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
    wxString output = wxFileName(filename).GetFullName();
    if (output.EndsWith(wxT(".ffs_gui")))
        output = output.BeforeLast('.');
    return output;
}


//tests if the same filenames are specified, even if they are relative to the current working directory/include symlinks or \\?\ prefix
class FindDuplicates
{
public:
    FindDuplicates(const Zstring& name) : m_name(name) {}

    bool operator()(const wxString& other) const
    {
        return util::sameFileSpecified(m_name, wxToZ(other));
    }

private:
    const Zstring& m_name;
};


void MainDialog::addFileToCfgHistory(const wxString& filename)
{
    //only (still) existing files should be included in the list
    if (util::fileExists(wxToZ(filename), 200) == util::EXISTING_FALSE) //potentially slow network access: wait 200ms
        return;

    std::vector<wxString>::const_iterator i = find_if(cfgFileNames.begin(), cfgFileNames.end(), FindDuplicates(wxToZ(filename)));
    if (i != cfgFileNames.end())
    {
        //if entry is in the list, then jump to element
        m_choiceHistory->SetSelection(i - cfgFileNames.begin());
    }
    else
    {
        cfgFileNames.insert(cfgFileNames.begin(), filename);

        //the default config file should receive another name on GUI
        if (util::sameFileSpecified(wxToZ(lastConfigFileName()), wxToZ(filename)))
            m_choiceHistory->Insert(_("<Last session>"), 0);  //insert at beginning of list
        else
            m_choiceHistory->Insert(getFormattedHistoryElement(filename), 0);  //insert at beginning of list

        m_choiceHistory->SetSelection(0);
    }

    //keep maximal size of history list
    if (cfgFileNames.size() > globalSettings->gui.cfgHistoryMax)
    {
        //delete last rows
        cfgFileNames.pop_back();
        m_choiceHistory->Delete(globalSettings->gui.cfgHistoryMax);
    }
}


void MainDialog::addLeftFolderToHistory(const wxString& leftFolder)
{
    m_directoryLeft->addPairToFolderHistory(leftFolder, globalSettings->gui.folderHistLeftMax);
}


void MainDialog::addRightFolderToHistory(const wxString& rightFolder)
{
    m_directoryRight->addPairToFolderHistory(rightFolder, globalSettings->gui.folderHistRightMax);
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


    wxFileDialog* filePicker = new wxFileDialog(this, wxEmptyString, wxEmptyString, defaultFileName, wxString(_("FreeFileSync configuration")) + wxT(" (*.ffs_gui)|*.ffs_gui"), wxFD_SAVE);
    if (filePicker->ShowModal() == wxID_OK)
    {
        const wxString newFileName = filePicker->GetPath();

        if (ffs3::fileExists(wxToZ(newFileName)))
        {
            QuestionDlg* messageDlg = new QuestionDlg(this,
                    QuestionDlg::BUTTON_YES | QuestionDlg::BUTTON_CANCEL,
                    wxString(_("File already exists. Overwrite?")) + wxT(" \"") + newFileName + wxT("\""));

            if (messageDlg->ShowModal() != QuestionDlg::BUTTON_YES)
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
    wxFileDialog* filePicker = new wxFileDialog(this,
            wxEmptyString,
            wxEmptyString,
            wxEmptyString,
            wxString(_("FreeFileSync configuration")) + wxT(" (*.ffs_gui;*.ffs_batch)|*.ffs_gui;*.ffs_batch"), wxFD_OPEN);

    if (filePicker->ShowModal() == wxID_OK)
        loadConfiguration(filePicker->GetPath());
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
    const int selectedItem = m_choiceHistory->GetSelection();
    if (0 <= selectedItem && unsigned(selectedItem) < cfgFileNames.size())
        loadConfiguration(cfgFileNames[selectedItem]);
}


bool MainDialog::saveOldConfig() //return false on user abort
{
    //notify user about changed settings
    if (globalSettings->optDialogs.popupOnConfigChange && !currentConfigFileName.empty()) //only if check is active and non-default config file loaded
    {
        if (lastConfigurationSaved != getCurrentConfiguration())
        {
            bool dontShowAgain = !globalSettings->optDialogs.popupOnConfigChange;

            QuestionDlg* notifyChangeDlg = new QuestionDlg(this,
                    QuestionDlg::BUTTON_YES | QuestionDlg::BUTTON_NO | QuestionDlg::BUTTON_CANCEL,
                    _("Save changes to current configuration?"),
                    &dontShowAgain);

            switch (notifyChangeDlg->ShowModal())
            {
            case QuestionDlg::BUTTON_YES:
                if (!trySaveConfig())
                    return false;
                break;
            case QuestionDlg::BUTTON_NO:
                globalSettings->optDialogs.popupOnConfigChange = !dontShowAgain;
                break;
            case QuestionDlg::BUTTON_CANCEL:
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
        const int selectedItem = m_choiceHistory->GetCurrentSelection();
        if (    0 <= selectedItem &&
                selectedItem < int(m_choiceHistory->GetCount()) &&
                selectedItem < int(cfgFileNames.size()))
        {
            //delete selected row
            cfgFileNames.erase(cfgFileNames.begin() + selectedItem);
            m_choiceHistory->Delete(selectedItem);
        }
    }
    event.Skip();
}


void MainDialog::OnClose(wxCloseEvent &event)
{
    if (!saveOldConfig()) //notify user about changed settings
        return;

    Destroy();
}


void MainDialog::OnQuit(wxCommandEvent &event)
{
    if (!saveOldConfig()) //notify user about changed settings
        return;

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
                ffs3::setActiveStatus(true, *fsObj);  //works recursively for directories
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
    if (filename == lastConfigFileName())
    {
        SetTitle(wxString(wxT("FreeFileSync - ")) + _("Folder Comparison and Synchronization"));
        currentConfigFileName.clear();
    }
    else
    {
        SetTitle(wxString(wxT("FreeFileSync - ")) + filename);
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
    }
    catch (const xmlAccess::XmlError& error)
    {
        wxMessageBox(error.msg().c_str(), _("Error"), wxOK | wxICON_ERROR);
        return false;
    }

    setLastUsedConfig(filename, guiCfg);

    return true;
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
    firstFolderPair->setValues(currentCfg.mainCfg.firstPair.leftDirectory,
                               currentCfg.mainCfg.firstPair.rightDirectory,
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
    bSizer6->Layout(); //adapt layout for variant text
}


inline
FolderPairEnh getEnahncedPair(const DirectoryPair* panel)
{
    return FolderPairEnh(panel->getLeftDir(),
                         panel->getRightDir(),
                         panel->getAltSyncConfig(),
                         panel->getAltFilterConfig());
}


xmlAccess::XmlGuiConfig MainDialog::getCurrentConfiguration() const
{
    xmlAccess::XmlGuiConfig guiCfg = currentCfg;

    //load settings whose ownership lies not in currentCfg:

    //first folder pair
    guiCfg.mainCfg.firstPair = FolderPairEnh(firstFolderPair->getLeftDir(),
                               firstFolderPair->getRightDir(),
                               firstFolderPair->getAltSyncConfig(),
                               firstFolderPair->getAltFilterConfig());

    //add additional pairs
    guiCfg.mainCfg.additionalPairs.clear();
    std::transform(additionalFolderPairs.begin(), additionalFolderPairs.end(),
                   std::back_inserter(guiCfg.mainCfg.additionalPairs), getEnahncedPair);

    //sync preview
    guiCfg.syncPreviewEnabled = syncPreview->previewIsEnabled();

    return guiCfg;
}


const wxString& MainDialog::lastConfigFileName()
{
    static wxString instance = ffs3::getConfigDir() + wxT("LastRun.ffs_gui");
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


void MainDialog::OnHideFilteredButton(wxCommandEvent &event)
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


void MainDialog::OnConfigureFilter(wxCommandEvent &event)
{
    if (showFilterDialog(true, //is main filter dialog
                         currentCfg.mainCfg.globalFilter.includeFilter,
                         currentCfg.mainCfg.globalFilter.excludeFilter) == DefaultReturnCode::BUTTON_OKAY)
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
    m_bpButtonLeftOnly->init(GlobalResources::getInstance().getImageByName(wxT("leftOnlyAct")),
                             _("Hide files that exist on left side only"),
                             GlobalResources::getInstance().getImageByName(wxT("leftOnlyDeact")),
                             _("Show files that exist on left side only"));

    m_bpButtonRightOnly->init(GlobalResources::getInstance().getImageByName(wxT("rightOnlyAct")),
                              _("Hide files that exist on right side only"),
                              GlobalResources::getInstance().getImageByName(wxT("rightOnlyDeact")),
                              _("Show files that exist on right side only"));

    m_bpButtonLeftNewer->init(GlobalResources::getInstance().getImageByName(wxT("leftNewerAct")),
                              _("Hide files that are newer on left"),
                              GlobalResources::getInstance().getImageByName(wxT("leftNewerDeact")),
                              _("Show files that are newer on left"));

    m_bpButtonRightNewer->init(GlobalResources::getInstance().getImageByName(wxT("rightNewerAct")),
                               _("Hide files that are newer on right"),
                               GlobalResources::getInstance().getImageByName(wxT("rightNewerDeact")),
                               _("Show files that are newer on right"));

    m_bpButtonEqual->init(GlobalResources::getInstance().getImageByName(wxT("equalAct")),
                          _("Hide files that are equal"),
                          GlobalResources::getInstance().getImageByName(wxT("equalDeact")),
                          _("Show files that are equal"));

    m_bpButtonDifferent->init(GlobalResources::getInstance().getImageByName(wxT("differentAct")),
                              _("Hide files that are different"),
                              GlobalResources::getInstance().getImageByName(wxT("differentDeact")),
                              _("Show files that are different"));

    m_bpButtonConflict->init(GlobalResources::getInstance().getImageByName(wxT("conflictAct")),
                             _("Hide conflicts"),
                             GlobalResources::getInstance().getImageByName(wxT("conflictDeact")),
                             _("Show conflicts"));

    //sync preview buttons
    m_bpButtonSyncCreateLeft->init(GlobalResources::getInstance().getImageByName(wxT("syncCreateLeftAct")),
                                   _("Hide files that will be created on the left side"),
                                   GlobalResources::getInstance().getImageByName(wxT("syncCreateLeftDeact")),
                                   _("Show files that will be created on the left side"));

    m_bpButtonSyncCreateRight->init(GlobalResources::getInstance().getImageByName(wxT("syncCreateRightAct")),
                                    _("Hide files that will be created on the right side"),
                                    GlobalResources::getInstance().getImageByName(wxT("syncCreateRightDeact")),
                                    _("Show files that will be created on the right side"));

    m_bpButtonSyncDeleteLeft->init(GlobalResources::getInstance().getImageByName(wxT("syncDeleteLeftAct")),
                                   _("Hide files that will be deleted on the left side"),
                                   GlobalResources::getInstance().getImageByName(wxT("syncDeleteLeftDeact")),
                                   _("Show files that will be deleted on the left side"));

    m_bpButtonSyncDeleteRight->init(GlobalResources::getInstance().getImageByName(wxT("syncDeleteRightAct")),
                                    _("Hide files that will be deleted on the right side"),
                                    GlobalResources::getInstance().getImageByName(wxT("syncDeleteRightDeact")),
                                    _("Show files that will be deleted on the right side"));

    m_bpButtonSyncDirOverwLeft->init(GlobalResources::getInstance().getImageByName(wxT("syncDirLeftAct")),
                                     _("Hide files that will be overwritten on left side"),
                                     GlobalResources::getInstance().getImageByName(wxT("syncDirLeftDeact")),
                                     _("Show files that will be overwritten on left side"));

    m_bpButtonSyncDirOverwRight->init(GlobalResources::getInstance().getImageByName(wxT("syncDirRightAct")),
                                      _("Hide files that will be overwritten on right side"),
                                      GlobalResources::getInstance().getImageByName(wxT("syncDirRightDeact")),
                                      _("Show files that will be overwritten on right side"));

    m_bpButtonSyncDirNone->init(GlobalResources::getInstance().getImageByName(wxT("syncDirNoneAct")),
                                _("Hide files that won't be copied"),
                                GlobalResources::getInstance().getImageByName(wxT("syncDirNoneDeact")),
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
    //prepare filter icon
    if (m_notebookBottomLeft->GetImageList() == NULL)
    {
        wxImageList* panelIcons = new wxImageList(16, 16);
        panelIcons->Add(wxBitmap(GlobalResources::getInstance().getImageByName(wxT("filterSmall"))));
        panelIcons->Add(wxBitmap(GlobalResources::getInstance().getImageByName(wxT("filterSmallGrey"))));
        m_notebookBottomLeft->AssignImageList(panelIcons); //pass ownership
    }

    //global filter: test for Null-filter
    if (isNullFilter(currentCfg.mainCfg.globalFilter))
    {
        m_bpButtonFilter->SetBitmapLabel(GlobalResources::getInstance().getImageByName(wxT("filterOff")));
        m_bpButtonFilter->SetToolTip(_("No filter selected"));

        //additional filter icon
        m_notebookBottomLeft->SetPageImage(1, 1);
    }
    else
    {
        m_bpButtonFilter->SetBitmapLabel(GlobalResources::getInstance().getImageByName(wxT("filterOn")));
        m_bpButtonFilter->SetToolTip(_("Filter is active"));

        //show filter icon
        m_notebookBottomLeft->SetPageImage(1, 0);
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


void MainDialog::OnCompare(wxCommandEvent &event)
{
    //PERF_START;
    clearStatusBar();

    wxBusyCursor dummy; //show hourglass cursor

    //prevent temporary memory peak by clearing old result list
    gridDataView->clearAllRows();
    updateGuiGrid(); //refresh GUI grid

    bool aborted = false;
    try
    {
        //class handling status display and error messages
        CompareStatusHandler statusHandler(this);

        //begin comparison
        ffs3::CompareProcess comparison(currentCfg.mainCfg.handleSymlinks,
                                        globalSettings->fileTimeTolerance,
                                        globalSettings->optDialogs,
                                        &statusHandler);

        //technical representation of comparison data
        ffs3::FolderComparison newCompareData;

        comparison.startCompareProcess(
            ffs3::extractCompareCfg(getCurrentConfiguration().mainCfg), //call getCurrentCfg() to get current values for directory pairs!
            currentCfg.mainCfg.compareVar,
            newCompareData);

        gridDataView->setData(newCompareData); //newCompareData is invalidated after this call

        //play (optional) sound notification after sync has completed (GUI and batch mode)
        const wxString soundFile = ffs3::getResourceDir() + wxT("Compare_Complete.wav");
        if (fileExists(wxToZ(soundFile)))
            wxSound::Play(soundFile, wxSOUND_ASYNC);
    }
    catch (AbortThisProcess&)
    {
        aborted = true;
    }

    if (aborted)
    {
        //disable the sync button
        syncPreview->enableSynchronization(false);
        m_buttonCompare->SetFocus();
        updateGuiGrid(); //refresh grid in ANY case! (also on abort)
    }
    else
    {
        //once compare is finished enable the sync button
        syncPreview->enableSynchronization(true);
        m_buttonStartSync->SetFocus();

        //hide sort direction indicator on GUI grids
        m_gridLeft->setSortMarker(CustomGrid::SortMarker(-1, CustomGrid::ASCENDING));
        m_gridMiddle->setSortMarker(CustomGrid::SortMarker(-1, CustomGrid::ASCENDING));
        m_gridRight->setSortMarker(CustomGrid::SortMarker(-1, CustomGrid::ASCENDING));

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
        wxString statusInfo;
        if (allElementsEqual(gridDataView->getDataTentative()))
            statusInfo += _("All directories in sync!");
        pushStatusInformation(statusInfo);
    }
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
    calculatePreview();

    m_gridLeft  ->Refresh();
    m_gridMiddle->Refresh();
    m_gridRight ->Refresh();
}


void MainDialog::calculatePreview()
{
    //update preview of bytes to be transferred:
    const SyncStatistics st(gridDataView->getDataTentative());
    const wxString toCreate = ffs3::numberToStringSep(st.getCreate());
    const wxString toUpdate = ffs3::numberToStringSep(st.getOverwrite());
    const wxString toDelete = ffs3::numberToStringSep(st.getDelete());
    const wxString data     = ffs3::formatFilesizeToShortString(st.getDataToProcess());

    m_textCtrlCreate->SetValue(toCreate);
    m_textCtrlUpdate->SetValue(toUpdate);
    m_textCtrlDelete->SetValue(toDelete);
    m_textCtrlData->SetValue(data);
}


void MainDialog::OnSwitchView(wxCommandEvent& event)
{
    //toggle view
    syncPreview->enablePreview(!syncPreview->previewIsEnabled());
}


void MainDialog::OnSyncSettings(wxCommandEvent& event)
{
    SyncCfgDialog* syncDlg = new SyncCfgDialog(this,
            currentCfg.mainCfg.compareVar,
            currentCfg.mainCfg.syncConfiguration,
            currentCfg.mainCfg.handleDeletion,
            currentCfg.mainCfg.customDeletionDirectory,
            &currentCfg.ignoreErrors);
    if (syncDlg->ShowModal() == SyncCfgDialog::BUTTON_APPLY)
        updateSyncConfig();
}


void MainDialog::OnCmpSettings(wxCommandEvent& event)
{
    //show window right next to the compare-config button
    wxPoint windowPos = m_bpButtonCmpConfig->GetScreenPosition();
    windowPos.x += m_bpButtonCmpConfig->GetSize().GetWidth() + 5;

    const CompareVariant  compareVarOld     = currentCfg.mainCfg.compareVar;
    const SymLinkHandling handleSymlinksOld = currentCfg.mainCfg.handleSymlinks;

    if (ffs3::showCompareCfgDialog(windowPos,
                                   currentCfg.mainCfg.compareVar,
                                   currentCfg.mainCfg.handleSymlinks) == DefaultReturnCode::BUTTON_OKAY &&
            //check if settings were changed at all
            (compareVarOld     != currentCfg.mainCfg.compareVar ||
             handleSymlinksOld != currentCfg.mainCfg.handleSymlinks))
    {
        //update compare variant name
        m_staticTextCmpVariant->SetLabel(wxString(wxT("(")) + getVariantName(currentCfg.mainCfg.compareVar) + wxT(")"));
        bSizer6->Layout(); //adapt layout for variant text

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
        pushStatusInformation(_("Please run a Compare first before synchronizing!"));
        return;
    }

    //show sync preview screen
    if (globalSettings->optDialogs.showSummaryBeforeSync)
    {
        bool dontShowAgain = false;

        if (ffs3::showSyncPreviewDlg(
                    getCurrentConfiguration().mainCfg.getSyncVariantName(),
                    ffs3::SyncStatistics(gridDataView->getDataTentative()),
                    dontShowAgain) != DefaultReturnCode::BUTTON_OKAY)
            return;

        globalSettings->optDialogs.showSummaryBeforeSync = !dontShowAgain;
    }

    wxBusyCursor dummy; //show hourglass cursor

    clearStatusBar();
    try
    {
        //PERF_START;

        //class handling status updates and error messages
        SyncStatusHandler statusHandler(this, currentCfg.ignoreErrors);

        //check if there are files/folders to be sync'ed at all
        if (!synchronizationNeeded(gridDataView->getDataTentative()))
            statusHandler.logInfo(_("Nothing to synchronize according to configuration!")); //inform about this special case

        //start synchronization and mark all elements processed
        ffs3::SyncProcess synchronization(
            globalSettings->optDialogs,
            globalSettings->verifyFileCopy,
            globalSettings->copyLockedFiles,
            globalSettings->copyFilePermissions,
            statusHandler);

        const std::vector<ffs3::FolderPairSyncCfg> syncProcessCfg = ffs3::extractSyncCfg(getCurrentConfiguration().mainCfg);
        FolderComparison& dataToSync = gridDataView->getDataTentative();

        //make sure syncProcessCfg and dataToSync have same size and correspond!
        if (syncProcessCfg.size() != dataToSync.size())
            throw std::logic_error("Programming Error: Contract violation!"); //should never happen: sync button is deactivated if they are not in sync

        synchronization.startSynchronizationProcess(syncProcessCfg, dataToSync);

        //play (optional) sound notification after sync has completed (GUI and batch mode)
        const wxString soundFile = ffs3::getResourceDir() + wxT("Sync_Complete.wav");
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
    ffs3::swapGrids(getCurrentConfiguration().mainCfg, gridDataView->getDataTentative());
    updateGuiGrid();
}


void MainDialog::updateGridViewData()
{
    size_t filesOnLeftView    = 0;
    size_t foldersOnLeftView  = 0;
    size_t filesOnRightView   = 0;
    size_t foldersOnRightView = 0;
    wxULongLong filesizeLeftView;
    wxULongLong filesizeRightView;

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

        if (    m_bpButtonSyncCreateLeft->   IsShown() ||
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

        if (    m_bpButtonLeftOnly->  IsShown() ||
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


    bSizer3->Layout();

    //update status information
    clearStatusBar();


    wxString statusLeftNew;
    wxString statusMiddleNew;
    wxString statusRightNew;

//#################################################
//format numbers to text:

//show status information on "root" level.
    if (foldersOnLeftView)
    {
        if (foldersOnLeftView == 1)
            statusLeftNew += _("1 directory");
        else
        {
            wxString folderCount = ffs3::numberToStringSep(foldersOnLeftView);

            wxString outputString = _("%x directories");
            outputString.Replace(wxT("%x"), folderCount, false);
            statusLeftNew += outputString;
        }

        if (filesOnLeftView)
            statusLeftNew += wxT(" - ");
    }

    if (filesOnLeftView)
    {
        if (filesOnLeftView == 1)
            statusLeftNew += _("1 file");
        else
        {
            wxString fileCount = ffs3::numberToStringSep(filesOnLeftView);

            wxString outputString = _("%x files");
            outputString.Replace(wxT("%x"), fileCount, false);
            statusLeftNew += outputString;
        }
        statusLeftNew += wxT(" - ");
        statusLeftNew += ffs3::formatFilesizeToShortString(filesizeLeftView);
    }

    const wxString objectsView = ffs3::numberToStringSep(gridDataView->rowsOnView());
    if (gridDataView->rowsTotal() == 1)
    {
        wxString outputString = _("%x of 1 row in view");
        outputString.Replace(wxT("%x"), objectsView, false);
        statusMiddleNew = outputString;
    }
    else
    {
        const wxString objectsTotal = ffs3::numberToStringSep(gridDataView->rowsTotal());

        wxString outputString = _("%x of %y rows in view");
        outputString.Replace(wxT("%x"), objectsView, false);
        outputString.Replace(wxT("%y"), objectsTotal, false);
        statusMiddleNew = outputString;
    }

    if (foldersOnRightView)
    {
        if (foldersOnRightView == 1)
            statusRightNew += _("1 directory");
        else
        {
            wxString folderCount = ffs3::numberToStringSep(foldersOnRightView);

            wxString outputString = _("%x directories");
            outputString.Replace(wxT("%x"), folderCount, false);
            statusRightNew += outputString;
        }

        if (filesOnRightView)
            statusRightNew += wxT(" - ");
    }

    if (filesOnRightView)
    {
        if (filesOnRightView == 1)
            statusRightNew += _("1 file");
        else
        {
            wxString fileCount = ffs3::numberToStringSep(filesOnRightView);

            wxString outputString = _("%x files");
            outputString.Replace(wxT("%x"), fileCount, false);
            statusRightNew += outputString;
        }

        statusRightNew += wxT(" - ");
        statusRightNew += ffs3::formatFilesizeToShortString(filesizeRightView);
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
    firstFolderPair->setValues(cfgEmpty.leftDirectory,
                               cfgEmpty.rightDirectory,
                               cfgEmpty.altSyncConfig,
                               cfgEmpty.localFilter);

    //disable the sync button
    syncPreview->enableSynchronization(false);

    //clear grids
    gridDataView->clearAllRows();
    updateSyncConfig(); //mainly to update sync dir description text
}


void MainDialog::updateFilterConfig()
{
    applyFiltering(getCurrentConfiguration().mainCfg, gridDataView->getDataTentative());
    refreshGridAfterFilterChange(400);
}


void MainDialog::updateSyncConfig()
{
    //update sync variant name
    m_staticTextSyncVariant->SetLabel(wxString(wxT("(")) + getCurrentConfiguration().mainCfg.getSyncVariantName() + wxT(")"));
    bSizer6->Layout(); //adapt layout for variant text


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
                WarningDlg* warningDlg = new WarningDlg(parent_, //show popup and ask user how to handle warning
                                                        WarningDlg::BUTTON_IGNORE,
                                                        text,
                                                        dontWarnAgain);
                if (warningDlg->ShowModal() == WarningDlg::BUTTON_IGNORE)
                    warningSyncDatabase_ = !dontWarnAgain;
            }
        }
    private:
        bool& warningSyncDatabase_;
        wxWindow* parent_;
    } redetCallback(globalSettings->optDialogs.warningSyncDatabase, this);

    ffs3::redetermineSyncDirection(getCurrentConfiguration().mainCfg, gridDataView->getDataTentative(), &redetCallback);
    updateGuiGrid();
}


void MainDialog::OnRemoveTopFolderPair(wxCommandEvent& event)
{
    if (additionalFolderPairs.size() > 0)
    {
        //get settings from second folder pair
        const FolderPairEnh cfgSecond = getEnahncedPair(additionalFolderPairs[0]);

        //reset first pair
        firstFolderPair->setValues(cfgSecond.leftDirectory,
                                   cfgSecond.rightDirectory,
                                   cfgSecond.altSyncConfig,
                                   cfgSecond.localFilter);

        removeAddFolderPair(0); //remove second folder pair (first of additional folder pairs)

//------------------------------------------------------------------
        //disable the sync button
        syncPreview->enableSynchronization(false);

        //clear grids
        gridDataView->clearAllRows();
        updateSyncConfig(); //mainly to update sync dir description text
    }
}


void MainDialog::OnRemoveFolderPair(wxCommandEvent& event)
{
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
            updateSyncConfig(); //mainly to update sync dir description text
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
    if (    additionalFolderPairs.size() == 0 &&
            firstFolderPair->getAltSyncConfig().get() == NULL &&
            NameFilter(firstFolderPair->getAltFilterConfig().includeFilter,
                       firstFolderPair->getAltFilterConfig().excludeFilter).isNull())
    {
        m_bpButtonLocalFilter->Hide();
        m_bpButtonAltSyncCfg->Hide();

        m_bpButtonSwapSides->SetBitmapLabel(GlobalResources::getInstance().getImageByName(wxT("swap")));
    }
    else
    {
        m_bpButtonLocalFilter->Show();
        m_bpButtonAltSyncCfg->Show();

        m_bpButtonSwapSides->SetBitmapLabel(GlobalResources::getInstance().getImageByName(wxT("swapSlim")));
    }

    m_panelTopMiddle->Layout();
}


void MainDialog::addFolderPair(const std::vector<FolderPairEnh>& newPairs, bool addFront)
{
    wxWindowUpdateLocker dummy(this); //avoid display distortion

    if (!newPairs.empty())
    {
        int pairHeight = 0;
        for (std::vector<FolderPairEnh>::const_iterator i = newPairs.begin(); i != newPairs.end(); ++i)
        {
            //add new folder pair
            DirectoryPair* newPair = new DirectoryPair(m_scrolledWindowFolderPairs, *this);

            //correct width of middle block
            newPair->m_panel21->SetMinSize(wxSize(m_gridMiddle->GetSize().GetWidth(), -1));

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

            //get size of scrolled window
            pairHeight = newPair->GetSize().GetHeight();

            //register events
            newPair->m_bpButtonRemovePair->Connect(wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler(MainDialog::OnRemoveFolderPair), NULL, this);

            //set alternate configuration
            newPair->setValues(i->leftDirectory,
                               i->rightDirectory,
                               i->altSyncConfig,
                               i->localFilter);
        }

        //set size of scrolled window
        const size_t visiblePairs = std::min(additionalFolderPairs.size(), globalSettings->gui.addFolderPairCountMax); //up to "addFolderPairCountMax" additional pairs shall be shown
        m_scrolledWindowFolderPairs->SetMinSize(wxSize( -1, pairHeight * static_cast<int>(visiblePairs)));

        //update controls
        m_scrolledWindowFolderPairs->Fit();    //adjust scrolled window size
        m_scrolledWindowFolderPairs->Layout(); //adjust stuff inside scrolled window
        bSizer1->Layout();
    }

    updateGuiForFolderPair();
}


void MainDialog::removeAddFolderPair(size_t pos)
{
    wxWindowUpdateLocker dummy(this); //avoid display distortion

    if (pos < additionalFolderPairs.size())
    {
        //remove folder pairs from window
        DirectoryPair* pairToDelete = additionalFolderPairs[pos];
        const int pairHeight = pairToDelete->GetSize().GetHeight();

        bSizerAddFolderPairs->Detach(pairToDelete); //Remove() does not work on Window*, so do it manually
        pairToDelete->Destroy();                 //
        additionalFolderPairs.erase(additionalFolderPairs.begin() + pos); //remove element from vector

        //set size of scrolled window
        const size_t additionalRows = additionalFolderPairs.size();
        if (additionalRows <= globalSettings->gui.addFolderPairCountMax) //up to "addFolderPairCountMax" additional pairs shall be shown
            m_scrolledWindowFolderPairs->SetMinSize(wxSize(-1, pairHeight * static_cast<int>(additionalRows)));

        //update controls
        m_scrolledWindowFolderPairs->Fit();    //adjust scrolled window size
        m_scrolledWindowFolderPairs->Layout(); //adjust stuff inside scrolled window
        bSizer1->Layout();
    }

    updateGuiForFolderPair();
}


void MainDialog::clearAddFolderPairs()
{
    wxWindowUpdateLocker dummy(this); //avoid display distortion

    additionalFolderPairs.clear();
    bSizerAddFolderPairs->Clear(true);

    m_scrolledWindowFolderPairs->SetMinSize(wxSize(-1, 0));
    bSizer1->Layout();
}
//########################################################################################################


//menu events
void MainDialog::OnMenuGlobalSettings(wxCommandEvent& event)
{
    ffs3::showGlobalSettingsDlg(*globalSettings);

    //event.Skip();
}


void MainDialog::OnMenuExportFileList(wxCommandEvent& event)
{
    //get a filename
    const wxString defaultFileName = wxT("FileList.csv"); //proposal
    wxFileDialog* filePicker = new wxFileDialog(this, wxEmptyString, wxEmptyString, defaultFileName, wxString(_("Comma separated list")) + wxT(" (*.csv)|*.csv"), wxFD_SAVE);

    if (filePicker->ShowModal() == wxID_OK)
    {
        const wxString newFileName = filePicker->GetPath();
        if (ffs3::fileExists(wxToZ(newFileName)))
        {
            QuestionDlg* messageDlg = new QuestionDlg(this,
                    QuestionDlg::BUTTON_YES | QuestionDlg::BUTTON_CANCEL,
                    wxString(_("File already exists. Overwrite?")) + wxT(" \"") + newFileName + wxT("\""));

            if (messageDlg->ShowModal() != QuestionDlg::BUTTON_YES)
            {
                OnMenuExportFileList(event); //retry
                return;
            }
        }

        wxString exportString;
        //write legend
        exportString +=  wxString(_("Legend")) + wxT('\n');
        if (syncPreview->previewIsEnabled())
        {
            exportString += wxString(wxT("\"")) + getDescription(SO_CREATE_NEW_LEFT)     + wxT("\";") + getSymbol(SO_CREATE_NEW_LEFT)     + wxT('\n');
            exportString += wxString(wxT("\"")) + getDescription(SO_CREATE_NEW_RIGHT)    + wxT("\";") + getSymbol(SO_CREATE_NEW_RIGHT)    + wxT('\n');
            exportString += wxString(wxT("\"")) + getDescription(SO_DELETE_LEFT)         + wxT("\";") + getSymbol(SO_DELETE_LEFT)         + wxT('\n');
            exportString += wxString(wxT("\"")) + getDescription(SO_DELETE_RIGHT)        + wxT("\";") + getSymbol(SO_DELETE_RIGHT)        + wxT('\n');
            exportString += wxString(wxT("\"")) + getDescription(SO_OVERWRITE_LEFT)      + wxT("\";") + getSymbol(SO_OVERWRITE_LEFT)      + wxT('\n');
            exportString += wxString(wxT("\"")) + getDescription(SO_OVERWRITE_RIGHT)     + wxT("\";") + getSymbol(SO_OVERWRITE_RIGHT)     + wxT('\n');
            exportString += wxString(wxT("\"")) + getDescription(SO_DO_NOTHING)          + wxT("\";") + getSymbol(SO_DO_NOTHING)          + wxT('\n');
            exportString += wxString(wxT("\"")) + getDescription(SO_EQUAL)               + wxT("\";") + getSymbol(SO_EQUAL)               + wxT('\n');
            exportString += wxString(wxT("\"")) + getDescription(SO_UNRESOLVED_CONFLICT) + wxT("\";") + getSymbol(SO_UNRESOLVED_CONFLICT) + wxT('\n');
        }
        else
        {
            exportString += wxString(wxT("\"")) + getDescription(FILE_LEFT_SIDE_ONLY)  + wxT("\";") + getSymbol(FILE_LEFT_SIDE_ONLY)  + wxT('\n');
            exportString += wxString(wxT("\"")) + getDescription(FILE_RIGHT_SIDE_ONLY) + wxT("\";") + getSymbol(FILE_RIGHT_SIDE_ONLY) + wxT('\n');
            exportString += wxString(wxT("\"")) + getDescription(FILE_LEFT_NEWER)      + wxT("\";") + getSymbol(FILE_LEFT_NEWER)      + wxT('\n');
            exportString += wxString(wxT("\"")) + getDescription(FILE_RIGHT_NEWER)     + wxT("\";") + getSymbol(FILE_RIGHT_NEWER)     + wxT('\n');
            exportString += wxString(wxT("\"")) + getDescription(FILE_DIFFERENT)       + wxT("\";") + getSymbol(FILE_DIFFERENT)       + wxT('\n');
            exportString += wxString(wxT("\"")) + getDescription(FILE_EQUAL)           + wxT("\";") + getSymbol(FILE_EQUAL)           + wxT('\n');
            exportString += wxString(wxT("\"")) + getDescription(FILE_CONFLICT)        + wxT("\";") + getSymbol(FILE_CONFLICT)        + wxT('\n');
        }
        exportString += '\n';

        //write header
        for (int k = 0; k < m_gridLeft->GetNumberCols(); ++k)
        {
            exportString += m_gridLeft->GetColLabelValue(k);
            exportString += ';';
        }

        for (int k = 0; k < m_gridMiddle->GetNumberCols(); ++k)
        {
            exportString += m_gridMiddle->GetColLabelValue(k);
            exportString += ';';
        }

        for (int k = 0; k < m_gridRight->GetNumberCols(); ++k)
        {
            exportString += m_gridRight->GetColLabelValue(k);
            if (k != m_gridRight->GetNumberCols() - 1)
                exportString += ';';
        }
        exportString += '\n';

        //begin work
        for (int i = 0; i < m_gridLeft->GetNumberRows(); ++i)
        {
            for (int k = 0; k < m_gridLeft->GetNumberCols(); ++k)
            {
                exportString += m_gridLeft->GetCellValue(i, k);
                exportString += ';';
            }

            for (int k = 0; k < m_gridMiddle->GetNumberCols(); ++k)
            {
                exportString += m_gridMiddle->GetCellValue(i, k);
                exportString += ';';
            }

            for (int k = 0; k < m_gridRight->GetNumberCols(); ++k)
            {
                exportString += m_gridRight->GetCellValue(i, k);
                if (k != m_gridRight->GetNumberCols() - 1)
                    exportString+= ';';
            }
            exportString+= '\n';
        }

        //write export file
        wxFFile output(newFileName.c_str(), wxT("w")); //don't write in binary mode
        if (output.IsOpened())
        {
            output.Write(common::BYTE_ORDER_MARK_UTF8, sizeof(common::BYTE_ORDER_MARK_UTF8) - 1);
            output.Write(exportString);
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

    BatchDialog* batchDlg = new BatchDialog(this, batchCfg);
    if (batchDlg->ShowModal() == BatchDialog::BATCH_FILE_SAVED)
        pushStatusInformation(_("Batch file created successfully!"));
}


void MainDialog::OnMenuCheckVersion(wxCommandEvent& event)
{
    ffs3::checkForUpdateNow();
}


void MainDialog::OnRegularUpdateCheck(wxIdleEvent& event)
{
    //execute just once per startup!
    Disconnect(wxEVT_IDLE, wxIdleEventHandler(MainDialog::OnRegularUpdateCheck), NULL, this);

    ffs3::checkForUpdatePeriodically(globalSettings->gui.lastUpdateCheck);
}


void MainDialog::OnLayoutWindowAsync(wxIdleEvent& event)
{
    //execute just once per startup!
    Disconnect(wxEVT_IDLE, wxIdleEventHandler(MainDialog::OnLayoutWindowAsync), NULL, this);

    wxWindowUpdateLocker dummy(this); //avoid display distortion

    //adjust folder pair distortion on startup
    m_scrolledWindowFolderPairs->Fit();

    Layout(); //strangely this layout call works if called in next idle event only
    Refresh();
}


void MainDialog::OnMenuAbout(wxCommandEvent& event)
{
    ffs3::showAboutDialog();
}


void MainDialog::OnShowHelp(wxCommandEvent& event)
{
    ffs3::displayHelpEntry();
}


void MainDialog::OnMenuQuit(wxCommandEvent& event)
{
    if (!saveOldConfig()) //notify user about changed settings
        return;

    Destroy();
}

//#########################################################################################################

//language selection
void MainDialog::switchProgramLanguage(const int langID)
{
    //create new dialog with respect to new language
    CustomLocale::getInstance().setLanguage(langID); //language is a global attribute

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
        mainDlg_->m_buttonStartSync->SetForegroundColour(*wxBLACK);
        mainDlg_->m_buttonStartSync->setBitmapFront(GlobalResources::getInstance().getImageByName(wxT("sync")));
    }
    else
    {
        synchronizationEnabled = false;
        mainDlg_->m_buttonStartSync->SetForegroundColour(wxColor(128, 128, 128)); //Some colors seem to have problems with 16Bit color depth, well this one hasn't!
        mainDlg_->m_buttonStartSync->setBitmapFront(GlobalResources::getInstance().getImageByName(wxT("syncDisabled")));
    }
}


bool MainDialog::SyncPreview::synchronizationIsEnabled() const
{
    return synchronizationEnabled;
}
