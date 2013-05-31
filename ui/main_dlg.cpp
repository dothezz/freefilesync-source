// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) Zenju (zenju AT gmx DOT de) - All Rights Reserved        *
// **************************************************************************

#include "main_dlg.h"
#include <zen/format_unit.h>
#include <zen/file_handling.h>
#include <zen/serialize.h>
//#include <zen/file_id.h>
//#include <zen/perf.h>
#include <zen/thread.h>
#include <wx/clipbrd.h>
#include <wx/wupdlock.h>
#include <wx/msgdlg.h>
#include <wx/sound.h>
#include <wx/filedlg.h>
#include <wx/display.h>
#include <wx+/context_menu.h>
#include <wx+/string_conv.h>
#include <wx+/button.h>
#include <wx+/shell_execute.h>
#include <wx+/app_main.h>
#include <wx+/toggle_button.h>
#include <wx+/mouse_move_dlg.h>
#include <wx+/no_flicker.h>
#include <wx+/rtl.h>
#include <wx+/font_size.h>
#include "check_version.h"
#include "gui_status_handler.h"
#include "sync_cfg.h"
#include "small_dlgs.h"
#include "progress_indicator.h"
#include "msg_popup.h"
#include "folder_pair.h"
#include "search.h"
#include "batch_config.h"
#include "triple_splitter.h"
#include "../comparison.h"
#include "../synchronization.h"
#include "../algorithm.h"
#include "../lib/resources.h"
#include "../lib/resolve_path.h"
#include "../lib/ffs_paths.h"
#include "../lib/help_provider.h"
#include "../lib/lock_holder.h"
#include "../lib/localization.h"

using namespace zen;
using namespace std::rel_ops;


namespace
{
struct wxClientHistoryData : public wxClientData //we need a wxClientData derived class to tell wxWidgets to take object ownership!
{
    wxClientHistoryData(const Zstring& cfgFile, int lastUseIndex) : cfgFile_(cfgFile), lastUseIndex_(lastUseIndex) {}

    //~wxClientHistoryData()
    //{
    // std::cerr << cfgFile_.c_str() << "\n";
    //}

    Zstring cfgFile_;
    int lastUseIndex_; //support sorting history by last usage, the higher the index the more recent the usage
};

IconBuffer::IconSize convert(xmlAccess::FileIconSize isize)
{
    using namespace xmlAccess;
    switch (isize)
    {
        case ICON_SIZE_SMALL:
            return IconBuffer::SIZE_SMALL;
        case ICON_SIZE_MEDIUM:
            return IconBuffer::SIZE_MEDIUM;
        case ICON_SIZE_LARGE:
            return IconBuffer::SIZE_LARGE;
    }
    return IconBuffer::SIZE_SMALL;
}
}


class DirectoryNameMainImpl : public DirectoryName<FolderHistoryBox>
{
public:
    DirectoryNameMainImpl(MainDialog&      mainDlg,
                          wxWindow&        dropWindow1,
                          Grid&            dropGrid,
                          wxButton&        dirSelectButton,
                          FolderHistoryBox& dirName,
                          wxStaticText&    staticText) :
        DirectoryName(dropWindow1, dirSelectButton, dirName, &staticText, &dropGrid.getMainWin()),
        mainDlg_(mainDlg) {}

    virtual bool acceptDrop(const std::vector<wxString>& droppedFiles, const wxPoint& clientPos, const wxWindow& wnd)
    {
        if (droppedFiles.empty())
            return false;

        switch (xmlAccess::getMergeType(toZ(droppedFiles))) //throw()
        {
            case xmlAccess::MERGE_BATCH:
            case xmlAccess::MERGE_GUI:
            case xmlAccess::MERGE_GUI_BATCH:
                mainDlg_.loadConfiguration(toZ(droppedFiles));
                return false;

            case xmlAccess::MERGE_OTHER:
                //=> return true: change directory selection via drag and drop
                break;
        }

        //mainDlg_.clearGrid();
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
    virtual MainConfiguration getMainConfig() const { return mainDlg.getConfig().mainCfg; }
    virtual wxWindow* getParentWindow() { return &mainDlg; }
    virtual std::unique_ptr<FilterConfig>& getFilterCfgOnClipboardRef() { return mainDlg.filterCfgOnClipboard; }

    virtual void onAltCompCfgChange    () { mainDlg.applyCompareConfig(); }
    virtual void onAltSyncCfgChange    () { mainDlg.applySyncConfig(); }
    virtual void onLocalFilterCfgChange() { mainDlg.applyFilterConfig(); } //re-apply filter

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
        dirNameLeft (*m_panelLeft,  *m_buttonSelectDirLeft,  *m_directoryLeft),
        dirNameRight(*m_panelRight, *m_buttonSelectDirRight, *m_directoryRight)
    {
        dirNameLeft .Connect(EVENT_ON_DIR_SELECTED, wxCommandEventHandler(MainDialog::onDirSelected), nullptr, &mainDialog);
        dirNameRight.Connect(EVENT_ON_DIR_SELECTED, wxCommandEventHandler(MainDialog::onDirSelected), nullptr, &mainDialog);

        dirNameLeft .Connect(EVENT_ON_DIR_MANUAL_CORRECTION, wxCommandEventHandler(MainDialog::onDirManualCorrection), nullptr, &mainDialog);
        dirNameRight.Connect(EVENT_ON_DIR_MANUAL_CORRECTION, wxCommandEventHandler(MainDialog::onDirManualCorrection), nullptr, &mainDialog);
    }

    void setValues(const Zstring& leftDir,
                   const Zstring& rightDir,
                   AltCompCfgPtr cmpCfg,
                   AltSyncCfgPtr syncCfg,
                   const FilterConfig& filter)
    {
        setConfig(cmpCfg, syncCfg, filter);
        dirNameLeft .setName(toWx(leftDir));
        dirNameRight.setName(toWx(rightDir));
    }
    Zstring getLeftDir () const { return toZ(dirNameLeft .getName()); }
    Zstring getRightDir() const { return toZ(dirNameRight.getName()); }

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
                    *mainDialog.m_panelTopLeft,
                    *mainDialog.m_gridMainL,
                    *mainDialog.m_buttonSelectDirLeft,
                    *mainDialog.m_directoryLeft,
                    *mainDialog.m_staticTextResolvedPathL),
        dirNameRight(mainDialog,
                     *mainDialog.m_panelTopRight,
                     *mainDialog.m_gridMainR,
                     *mainDialog.m_buttonSelectDirRight,
                     *mainDialog.m_directoryRight,
                     *mainDialog.m_staticTextResolvedPathR)
    {
        dirNameLeft .Connect(EVENT_ON_DIR_SELECTED, wxCommandEventHandler(MainDialog::onDirSelected), nullptr, &mainDialog);
        dirNameRight.Connect(EVENT_ON_DIR_SELECTED, wxCommandEventHandler(MainDialog::onDirSelected), nullptr, &mainDialog);

        dirNameLeft .Connect(EVENT_ON_DIR_MANUAL_CORRECTION, wxCommandEventHandler(MainDialog::onDirManualCorrection), nullptr, &mainDialog);
        dirNameRight.Connect(EVENT_ON_DIR_MANUAL_CORRECTION, wxCommandEventHandler(MainDialog::onDirManualCorrection), nullptr, &mainDialog);
    }

    void setValues(const Zstring& leftDir,
                   const Zstring& rightDir,
                   AltCompCfgPtr cmpCfg,
                   AltSyncCfgPtr syncCfg,
                   const FilterConfig& filter)
    {
        setConfig(cmpCfg, syncCfg, filter);
        dirNameLeft .setName(toWx(leftDir));
        dirNameRight.setName(toWx(rightDir));
    }
    Zstring getLeftDir () const { return toZ(dirNameLeft .getName()); }
    Zstring getRightDir() const { return toZ(dirNameRight.getName()); }

private:
    //support for drag and drop
    DirectoryNameMainImpl dirNameLeft;
    DirectoryNameMainImpl dirNameRight;
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
        if (wxPanel* panel = dynamic_cast<wxPanel*>(event.GetEventObject()))
        {
            const wxAuiPaneInfo& paneInfo = mainDlg_.auiMgr.GetPane(panel);
            if (paneInfo.IsOk() &&
                paneInfo.IsFloating())
                return false; //prevent main dialog move
        }

        return true; //allow dialog move
    }

private:
    MainDialog& mainDlg_;
};
#endif


namespace
{
//workaround for wxWidgets: small hack to update menu items: actually this is a wxWidgets bug (affects Windows- and Linux-build)
void setMenuItemImage(wxMenuItem*& menuItem, const wxBitmap& bmp)
{
    assert(menuItem->GetKind() == wxITEM_NORMAL);

    //support polling
    if (isEqual(bmp, menuItem->GetBitmap()))
        return;

    if (wxMenu* menu = menuItem->GetMenu())
    {
        int pos = menu->GetMenuItems().IndexOf(menuItem);
        if (pos != wxNOT_FOUND)
        {
            /*
                menu->Remove(item);        ->this simple sequence crashes on Kubuntu x64, wxWidgets 2.9.2
                menu->Insert(index, item);
                */
            const bool enabled = menuItem->IsEnabled();
            wxMenuItem* newItem = new wxMenuItem(menu, menuItem->GetId(), menuItem->GetItemLabel());
            newItem->SetBitmap(bmp);

            menu->Destroy(menuItem);          //actual workaround
            menuItem = menu->Insert(pos, newItem); //don't forget to update input item pointer!

            if (!enabled)
                menuItem->Enable(false); //do not enable BEFORE appending item! wxWidgets screws up for yet another crappy reason
        }
    }
}

//##################################################################################################################################

xmlAccess::XmlGlobalSettings retrieveGlobalCfgFromDisk() //blocks on GUI on errors!
{
    using namespace xmlAccess;
    XmlGlobalSettings globalCfg;
    try
    {
        if (fileExists(getGlobalConfigFile()))
            readConfig(globalCfg); //throw FfsXmlError
        //else: globalCfg already has default values
    }
    catch (const FfsXmlError& e)
    {
        assert(false);
        if (e.getSeverity() != FfsXmlError::WARNING) //ignore parsing errors: should be migration problems only *cross-fingers*
            wxMessageBox(e.toString(), _("Error"), wxOK | wxICON_ERROR);
    }
    return globalCfg;
}
}


void MainDialog::create(const std::vector<Zstring>& cfgFileNames)
{
    using namespace xmlAccess;
    const XmlGlobalSettings globalSettings = retrieveGlobalCfgFromDisk();

    std::vector<Zstring> filenames;
    if (!cfgFileNames.empty()) //1. this one has priority
        filenames = cfgFileNames;
    else //FFS default startup: use last used selection
    {
        filenames = globalSettings.gui.lastUsedConfigFiles; //2. now try last used files

        //------------------------------------------------------------------------------------------
        //check existence of all directories in parallel!

        RunUntilFirstHit<NullType> findFirstMissing;

        std::for_each(filenames.begin(), filenames.end(), [&](const Zstring& filename)
        {
            findFirstMissing.addJob([=] { return filename.empty() /*ever empty??*/ || !fileExists(filename) ? zen::make_unique<NullType>() : nullptr; });
        });
        //potentially slow network access: give all checks 500ms to finish
        const bool allFilesExist = findFirstMissing.timedWait(boost::posix_time::milliseconds(500)) && //false: time elapsed
                                   !findFirstMissing.get(); //no missing
        if (!allFilesExist)
            filenames.clear(); //we do NOT want to show an error due to last config file missing on application start!
        //------------------------------------------------------------------------------------------

        if (filenames.empty())
        {
            if (zen::fileExists(lastRunConfigName())) //3. try to load auto-save config
                filenames.push_back(lastRunConfigName());
        }
    }

    XmlGuiConfig guiCfg; //structure to receive gui settings with default values

    bool loadCfgSuccess = false;
    if (filenames.empty())
    {
        //add default exclusion filter: this is only ever relevant when creating new configurations!
        //a default XmlGuiConfig does not need these user-specific exclusions!
        Zstring& excludeFilter = guiCfg.mainCfg.globalFilter.excludeFilter;
        if (!excludeFilter.empty() && !endsWith(excludeFilter, Zstr("\n")))
            excludeFilter += Zstr("\n");
        excludeFilter += globalSettings.gui.defaultExclusionFilter;
    }
    else
        try
        {
            readAnyConfig(filenames, guiCfg); //throw FfsXmlError
            loadCfgSuccess = true;
        }
        catch (const FfsXmlError& error)
        {
            if (error.getSeverity() == FfsXmlError::WARNING)
                wxMessageBox(error.toString(), _("Warning"), wxOK | wxICON_WARNING);
            //what about simulating changed config on parsing errors????
            else
                wxMessageBox(error.toString(), _("Error"), wxOK | wxICON_ERROR);
        }

    const bool startComparisonImmediately = !cfgFileNames.empty() && loadCfgSuccess;

    //------------------------------------------------------------------------------------------

    create_impl(guiCfg, filenames, globalSettings, startComparisonImmediately);
}


void MainDialog::create(const xmlAccess::XmlGuiConfig& guiCfg,
                        bool startComparison)
{
    create_impl(guiCfg, std::vector<Zstring>(), retrieveGlobalCfgFromDisk(), startComparison);
}


void MainDialog::create(const xmlAccess::XmlGuiConfig& guiCfg,
                        const std::vector<Zstring>& referenceFiles,
                        const xmlAccess::XmlGlobalSettings& globalSettings,
                        bool startComparison)
{
    create_impl(guiCfg, referenceFiles, globalSettings, startComparison);
}


void MainDialog::create_impl(const xmlAccess::XmlGuiConfig& guiCfg,
                             const std::vector<Zstring>& referenceFiles,
                             const xmlAccess::XmlGlobalSettings& globalSettings,
                             bool startComparison)
{
    try
    {
        //we need to set language *before* creating MainDialog!
        setLanguage(globalSettings.programLanguage); //throw FileError
    }
    catch (const FileError& e)
    {
        wxMessageBox(e.toString().c_str(), _("Error"), wxOK | wxICON_ERROR);
        //continue!
    }

    MainDialog* frame = new MainDialog(guiCfg, referenceFiles, globalSettings, startComparison);
    frame->Show();
}


MainDialog::MainDialog(const xmlAccess::XmlGuiConfig& guiCfg,
                       const std::vector<Zstring>& referenceFiles,
                       const xmlAccess::XmlGlobalSettings& globalSettings,
                       bool startComparison) :
    MainDialogGenerated(nullptr),
    folderHistoryLeft (std::make_shared<FolderHistory>()), //make sure it is always bound
    folderHistoryRight(std::make_shared<FolderHistory>())  //
{
    m_directoryLeft ->init(folderHistoryLeft);
    m_directoryRight->init(folderHistoryRight);

    //setup sash: detach + reparent:
    m_splitterMain->SetSizer(nullptr); //alas wxFormbuilder doesn't allow us to have child windows without a sizer, so we have to remove it here
    m_splitterMain->setupWindows(m_gridMainL, m_gridMainC, m_gridMainR);

    wxWindowUpdateLocker dummy(this); //avoid display distortion

    setRelativeFontSize(*m_buttonCompare, 1.5);
    setRelativeFontSize(*m_buttonSync,    1.5);
    setRelativeFontSize(*m_buttonCancel,  1.5);
    m_buttonCancel->refreshButtonLabel(); //required after font change!

    //---------------- support for dockable gui style --------------------------------
    bSizerPanelHolder->Detach(m_panelTopButtons);
    bSizerPanelHolder->Detach(m_panelDirectoryPairs);
    bSizerPanelHolder->Detach(m_gridNavi);
    bSizerPanelHolder->Detach(m_panelCenter);
    bSizerPanelHolder->Detach(m_panelConfig);
    bSizerPanelHolder->Detach(m_panelFilter);
    bSizerPanelHolder->Detach(m_panelViewFilter);
    bSizerPanelHolder->Detach(m_panelStatistics);

    auiMgr.SetManagedWindow(this);
    auiMgr.SetFlags(wxAUI_MGR_DEFAULT | wxAUI_MGR_LIVE_RESIZE);

    compareStatus = make_unique<CompareProgressDialog>(*this); //integrate the compare status panel (in hidden state)

    //caption required for all panes that can be manipulated by the users => used by context menu
    auiMgr.AddPane(m_panelCenter,
                   wxAuiPaneInfo().Name(L"Panel3").CenterPane().PaneBorder(false));

    auiMgr.AddPane(m_panelDirectoryPairs,
                   wxAuiPaneInfo().Name(L"Panel2").Layer(2).Top().Caption(_("Folder pairs")).CaptionVisible(false).PaneBorder(false).Gripper());

    auiMgr.AddPane(m_gridNavi,
                   wxAuiPaneInfo().Name(L"Panel10").Layer(3).Left().Caption(_("Overview")).MinSize(300, m_gridNavi->GetSize().GetHeight())); //MinSize(): just default size, see comment below

    auiMgr.AddPane(m_panelConfig,
                   wxAuiPaneInfo().Name(L"Panel4").Layer(3).Left().Caption(_("Configuration")).MinSize(m_listBoxHistory->GetSize().GetWidth(), m_panelConfig->GetSize().GetHeight()));

    auiMgr.AddPane(m_panelTopButtons,
                   wxAuiPaneInfo().Name(L"Panel1").Layer(4).Top().Row(1).Caption(_("Main bar")).CaptionVisible(false).PaneBorder(false).Gripper().MinSize(-1, m_panelTopButtons->GetSize().GetHeight()));
    //note: min height is calculated incorrectly by wxAuiManager if panes with and without caption are in the same row => use smaller min-size

    auiMgr.AddPane(compareStatus->getAsWindow(),
                   wxAuiPaneInfo().Name(L"Panel9").Layer(4).Top().Row(2).CaptionVisible(false).PaneBorder(false).Hide());

    auiMgr.AddPane(m_panelFilter,
                   wxAuiPaneInfo().Name(L"Panel5").Layer(4).Bottom().Position(1).Caption(_("Filter files")).MinSize(m_bpButtonFilter->GetSize().GetWidth(), m_panelFilter->GetSize().GetHeight()));

    auiMgr.AddPane(m_panelViewFilter,
                   wxAuiPaneInfo().Name(L"Panel6").Layer(4).Bottom().Position(2).Caption(_("Select view")).MinSize(m_bpButtonShowDoNothing->GetSize().GetWidth(), m_panelViewFilter->GetSize().GetHeight()));

    auiMgr.AddPane(m_panelStatistics,
                   wxAuiPaneInfo().Name(L"Panel7").Layer(4).Bottom().Position(3).Caption(_("Statistics")).MinSize(m_bitmapData->GetSize().GetWidth() + m_staticTextData->GetSize().GetWidth(), m_panelStatistics->GetSize().GetHeight()));

    auiMgr.Update();

    //give panel captions bold typeface
    if (wxAuiDockArt* artProvider = auiMgr.GetArtProvider())
    {
        wxFont font = artProvider->GetFont(wxAUI_DOCKART_CAPTION_FONT);
        font.SetWeight(wxFONTWEIGHT_BOLD);
        font.SetPointSize(wxNORMAL_FONT->GetPointSize()); //= larger than the wxAuiDockArt default; looks better on OS X
        artProvider->SetFont(wxAUI_DOCKART_CAPTION_FONT, font);

        //accessibility: fix wxAUI drawing black text on black background on high-contrast color schemes:
        artProvider->SetColor(wxAUI_DOCKART_INACTIVE_CAPTION_TEXT_COLOUR, wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOWTEXT));
    }

    auiMgr.GetPane(m_gridNavi).MinSize(-1, -1); //we successfully tricked wxAuiManager into setting an initial Window size :> incomplete API anyone??
    auiMgr.Update(); //

    defaultPerspective = auiMgr.SavePerspective();
    //----------------------------------------------------------------------------------
    //register view layout context menu
    m_panelTopButtons->Connect(wxEVT_RIGHT_DOWN, wxMouseEventHandler(MainDialog::OnContextSetLayout), nullptr, this);
    m_panelConfig    ->Connect(wxEVT_RIGHT_DOWN, wxMouseEventHandler(MainDialog::OnContextSetLayout), nullptr, this);
    m_panelFilter    ->Connect(wxEVT_RIGHT_DOWN, wxMouseEventHandler(MainDialog::OnContextSetLayout), nullptr, this);
    m_panelViewFilter->Connect(wxEVT_RIGHT_DOWN, wxMouseEventHandler(MainDialog::OnContextSetLayout), nullptr, this);
    m_panelStatistics->Connect(wxEVT_RIGHT_DOWN, wxMouseEventHandler(MainDialog::OnContextSetLayout), nullptr, this);
    m_panelStatusBar ->Connect(wxEVT_RIGHT_DOWN, wxMouseEventHandler(MainDialog::OnContextSetLayout), nullptr, this);
    //----------------------------------------------------------------------------------

    //sort grids
    m_gridMainL->Connect(EVENT_GRID_COL_LABEL_MOUSE_LEFT,  GridClickEventHandler(MainDialog::onGridLabelLeftClickL ), nullptr, this);
    m_gridMainC->Connect(EVENT_GRID_COL_LABEL_MOUSE_LEFT,  GridClickEventHandler(MainDialog::onGridLabelLeftClickC ), nullptr, this);
    m_gridMainR->Connect(EVENT_GRID_COL_LABEL_MOUSE_LEFT,  GridClickEventHandler(MainDialog::onGridLabelLeftClickR ), nullptr, this);

    m_gridMainL->Connect(EVENT_GRID_COL_LABEL_MOUSE_RIGHT, GridClickEventHandler(MainDialog::onGridLabelContextL   ), nullptr, this);
    m_gridMainC->Connect(EVENT_GRID_COL_LABEL_MOUSE_RIGHT, GridClickEventHandler(MainDialog::onGridLabelContextC   ), nullptr, this);
    m_gridMainR->Connect(EVENT_GRID_COL_LABEL_MOUSE_RIGHT, GridClickEventHandler(MainDialog::onGridLabelContextR   ), nullptr, this);

    //grid context menu
    m_gridMainL->Connect(EVENT_GRID_MOUSE_RIGHT_UP, GridClickEventHandler(MainDialog::onMainGridContextL), nullptr, this);
    m_gridMainC->Connect(EVENT_GRID_MOUSE_RIGHT_UP, GridClickEventHandler(MainDialog::onMainGridContextC), nullptr, this);
    m_gridMainR->Connect(EVENT_GRID_MOUSE_RIGHT_UP, GridClickEventHandler(MainDialog::onMainGridContextR), nullptr, this);
    m_gridNavi ->Connect(EVENT_GRID_MOUSE_RIGHT_UP, GridClickEventHandler(MainDialog::onNaviGridContext ), nullptr, this);

    m_gridMainL->Connect(EVENT_GRID_MOUSE_LEFT_DOUBLE, GridClickEventHandler(MainDialog::onGridDoubleClickL), nullptr, this );
    m_gridMainR->Connect(EVENT_GRID_MOUSE_LEFT_DOUBLE, GridClickEventHandler(MainDialog::onGridDoubleClickR), nullptr, this );

    m_gridNavi->Connect(EVENT_GRID_SELECT_RANGE, GridRangeSelectEventHandler(MainDialog::onNaviSelection), nullptr, this);

    //set tool tips with (non-translated!) short cut hint
    m_bpButtonOpen ->SetToolTip(_("Open...") + L" (Ctrl+O)");
    m_bpButtonSave ->SetToolTip(_("Save")    + L" (Ctrl+S)");
    m_buttonCompare->SetToolTip(_("Compare both sides")    + L" (F5)");
    m_buttonSync   ->SetToolTip(_("Start synchronization") + L" (F6)");

    gridDataView = std::make_shared<GridView>();
    treeDataView = std::make_shared<TreeView>();

    cleanedUp = false;
    processingGlobalKeyEvent = false;

#ifdef FFS_WIN
    new PanelMoveWindow(*this); //allow moving main dialog by clicking (nearly) anywhere... //ownership passed to "this"
#endif

    SetIcon(GlobalResources::instance().programIconFFS); //set application icon

    //notify about (logical) application main window => program won't quit, but stay on this dialog
    zen::setMainWindow(this);

    //init handling of first folder pair
    firstFolderPair = make_unique<DirectoryPairFirst>(*this);

    initViewFilterButtons();

    //init grid settings
    gridview::init(*m_gridMainL, *m_gridMainC, *m_gridMainR, gridDataView);
    treeview::init(*m_gridNavi, treeDataView);

    //initialize and load configuration
    setGlobalCfgOnInit(globalSettings);
    setConfig(guiCfg, referenceFiles);

    //set icons for this dialog
    m_buttonCompare     ->setBitmapFront(getResourceImage(L"compare"), 5);
    m_bpButtonSyncConfig->SetBitmapLabel(getResourceImage(L"syncConfig"));
    m_bpButtonCmpConfig ->SetBitmapLabel(getResourceImage(L"cmpConfig"));
    m_bpButtonOpen      ->SetBitmapLabel(getResourceImage(L"load"));
    m_bpButtonBatchJob  ->SetBitmapLabel(getResourceImage(L"batch"));
    m_bpButtonAddPair   ->SetBitmapLabel(getResourceImage(L"item_add"));
    {
        IconBuffer tmp(IconBuffer::SIZE_SMALL);
        const wxBitmap& bmpFile = tmp.genericFileIcon();
        const wxBitmap& bmpDir  = tmp.genericDirIcon();

        m_bitmapSmallDirectoryLeft ->SetBitmap(bmpDir);
        m_bitmapSmallFileLeft      ->SetBitmap(bmpFile);
        m_bitmapSmallDirectoryRight->SetBitmap(bmpDir);
        m_bitmapSmallFileRight     ->SetBitmap(bmpFile);
    }

    m_panelTopButtons->Layout(); //wxButtonWithImage size might have changed

    const int dummySize = 5;
    wxImage dummyImg(dummySize, dummySize);
    if (!dummyImg.HasAlpha())
        dummyImg.InitAlpha();
    std::fill(dummyImg.GetAlpha(), dummyImg.GetAlpha() + dummySize * dummySize, wxIMAGE_ALPHA_TRANSPARENT);

    //menu icons: workaround for wxWidgets: small hack to update menu items: actually this is a wxWidgets bug (affects Windows- and Linux-build)
    setMenuItemImage(m_menuItem10,  getResourceImage(L"compareSmall"));
    setMenuItemImage(m_menuItem11,  getResourceImage(L"syncSmall"));

    setMenuItemImage(m_menuItemNew,  dummyImg); //it's ridiculous, but wxWidgets screws up aligning short-cut label texts if we don't set an image!
    setMenuItemImage(m_menuItemSaveAs, dummyImg);
    setMenuItemImage(m_menuItemLoad,  getResourceImage(L"loadSmall"));
    setMenuItemImage(m_menuItemSave,  getResourceImage(L"saveSmall"));

    setMenuItemImage(m_menuItemGlobSett, getResourceImage(L"settingsSmall"));
    setMenuItemImage(m_menuItem7,        getResourceImage(L"batchSmall"));

    setMenuItemImage(m_menuItemManual, getResourceImage(L"helpSmall"));
    setMenuItemImage(m_menuItemAbout,  getResourceImage(L"aboutSmall"));

    if (!manualProgramUpdateRequired())
    {
        m_menuItemCheckVersionNow ->Enable(false);
        m_menuItemCheckVersionAuto->Enable(false);

        //wxFormbuilder doesn't give us a wxMenuItem for m_menuCheckVersion, so we need this abomination:
        wxMenuItemList& items = m_menuHelp->GetMenuItems();
        for (auto it = items.begin(); it != items.end(); ++it)
            if ((*it)->GetSubMenu() == m_menuCheckVersion)
                (*it)->Enable(false);
    }

    //create language selection menu
    std::for_each(zen::ExistingTranslations::get().begin(), ExistingTranslations::get().end(),
                  [&](const ExistingTranslations::Entry& entry)
    {
        wxMenuItem* newItem = new wxMenuItem(m_menuLanguages, wxID_ANY, entry.languageName);
        newItem->SetBitmap(getResourceImage(entry.languageFlag));

        //map menu item IDs with language IDs: evaluated when processing event handler
        languageMenuItemMap.insert(std::make_pair(newItem->GetId(), entry.languageID));

        //connect event
        this->Connect(newItem->GetId(), wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(MainDialog::OnMenuLanguageSwitch));
        m_menuLanguages->Append(newItem);
    });

    //support for CTRL + C and DEL on grids
    m_gridMainL->getMainWin().Connect(wxEVT_KEY_DOWN, wxKeyEventHandler(MainDialog::onGridButtonEventL), nullptr, this);
    m_gridMainC->getMainWin().Connect(wxEVT_KEY_DOWN, wxKeyEventHandler(MainDialog::onGridButtonEventC), nullptr, this);
    m_gridMainR->getMainWin().Connect(wxEVT_KEY_DOWN, wxKeyEventHandler(MainDialog::onGridButtonEventR), nullptr, this);

    m_gridNavi->getMainWin().Connect(wxEVT_KEY_DOWN, wxKeyEventHandler(MainDialog::onTreeButtonEvent), nullptr, this);

    //register global hotkeys (without explicit menu entry)
    wxTheApp->Connect(wxEVT_KEY_DOWN,  wxKeyEventHandler(MainDialog::OnGlobalKeyEvent), nullptr, this);
    wxTheApp->Connect(wxEVT_CHAR_HOOK, wxKeyEventHandler(MainDialog::OnGlobalKeyEvent), nullptr, this); //capture direction keys

    //drag & drop on navi panel
    setupFileDrop(*m_gridNavi);
    m_gridNavi->Connect(EVENT_DROP_FILE, FileDropEventHandler(MainDialog::onNaviPanelFilesDropped), nullptr, this);

    timerForAsyncTasks.Connect(wxEVT_TIMER, wxEventHandler(MainDialog::onProcessAsyncTasks), nullptr, this);

    //Connect(wxEVT_SIZE, wxSizeEventHandler(MainDialog::OnResize), nullptr, this);
    //Connect(wxEVT_MOVE, wxSizeEventHandler(MainDialog::OnResize), nullptr, this);

    //calculate witdh of folder pair manually (if scrollbars are visible)
    m_panelTopLeft->Connect(wxEVT_SIZE, wxEventHandler(MainDialog::OnResizeLeftFolderWidth), nullptr, this);

    //dynamically change sizer direction depending on size
    m_panelConfig    ->Connect(wxEVT_SIZE, wxEventHandler(MainDialog::OnResizeConfigPanel),     nullptr, this);
    m_panelViewFilter->Connect(wxEVT_SIZE, wxEventHandler(MainDialog::OnResizeViewPanel),       nullptr, this);
    m_panelStatistics->Connect(wxEVT_SIZE, wxEventHandler(MainDialog::OnResizeStatisticsPanel), nullptr, this);
    wxSizeEvent dummy3;
    OnResizeConfigPanel    (dummy3); //call once on window creation
    OnResizeViewPanel      (dummy3); //
    OnResizeStatisticsPanel(dummy3); //

    //event handler for manual (un-)checking of rows and setting of sync direction
    m_gridMainC->Connect(EVENT_GRID_CHECK_ROWS,     CheckRowsEventHandler    (MainDialog::onCheckRows), nullptr, this);
    m_gridMainC->Connect(EVENT_GRID_SYNC_DIRECTION, SyncDirectionEventHandler(MainDialog::onSetSyncDirection), nullptr, this);

    //mainly to update row label sizes...
    updateGui();

    //register regular check for update on next idle event
    Connect(wxEVT_IDLE, wxIdleEventHandler(MainDialog::OnRegularUpdateCheck), nullptr, this);

    //asynchronous call to wxWindow::Layout(): fix superfluous frame on right and bottom when FFS is started in fullscreen mode
    Connect(wxEVT_IDLE, wxIdleEventHandler(MainDialog::OnLayoutWindowAsync), nullptr, this);
    wxCommandEvent evtDummy;       //call once before OnLayoutWindowAsync()
    OnResizeLeftFolderWidth(evtDummy); //

    //----------------------------------------------------------------------------------------------------------------------------------------------------------------
    //some convenience: if FFS is started with a *.ffs_gui file as commandline parameter AND all directories contained exist, comparison shall be started right away
    if (startComparison)
    {
        const zen::MainConfiguration currMainCfg = getConfig().mainCfg;

        //------------------------------------------------------------------------------------------
        //check existence of all directories in parallel!
        RunUntilFirstHit<NullType> findFirstMissing;

        //harmonize checks with comparison.cpp:: checkForIncompleteInput()
        //we're really doing two checks: 1. check directory existence 2. check config validity -> don't mix them!
        bool havePartialPair = false;
        bool haveFullPair    = false;

        auto addDirCheck = [&](const FolderPairEnh& fp)
        {
            const Zstring dirLeft  = getFormattedDirectoryName(fp.leftDirectory ); //should not block!?
            const Zstring dirRight = getFormattedDirectoryName(fp.rightDirectory); //

            if (dirLeft.empty() != dirRight.empty()) //only skip check if both sides are empty!
                havePartialPair = true;
            else if (!dirLeft.empty())
                haveFullPair = true;

            if (!dirLeft.empty())
                findFirstMissing.addJob([=] { return !dirExists(dirLeft ) ? zen::make_unique<NullType>() : nullptr; });
            if (!dirRight.empty())
                findFirstMissing.addJob([=] { return !dirExists(dirRight) ? zen::make_unique<NullType>() : nullptr; });
        };

        addDirCheck(currMainCfg.firstPair);
        std::for_each(currMainCfg.additionalPairs.begin(), currMainCfg.additionalPairs.end(), addDirCheck);
        //------------------------------------------------------------------------------------------

        if (havePartialPair != haveFullPair) //either all pairs full or all half-filled -> validity check!
        {
            //potentially slow network access: give all checks 500ms to finish
            const bool allFilesExist = findFirstMissing.timedWait(boost::posix_time::milliseconds(500)) && //true: have result
                                       !findFirstMissing.get(); //no missing
            if (allFilesExist)
                if (wxEvtHandler* evtHandler = m_buttonCompare->GetEventHandler())
                {
                    wxCommandEvent dummy2(wxEVT_COMMAND_BUTTON_CLICKED);
                    evtHandler->AddPendingEvent(dummy2); //simulate button click on "compare"
                }
        }
    }
}


MainDialog::~MainDialog()
{
    try //save "GlobalSettings.xml"
    {
        xmlAccess::writeConfig(getGlobalCfgBeforeExit()); //throw FfsXmlError
    }
    catch (const xmlAccess::FfsXmlError& e)
    {
        wxMessageBox(e.toString().c_str(), _("Error"), wxOK | wxICON_ERROR, this);
    }

    try //save "LastRun.ffs_gui"
    {
        xmlAccess::writeConfig(getConfig(), lastRunConfigName()); //throw FfsXmlError
    }
    //don't annoy users on read-only drives: it's enough to show a single error message when saving global config
    catch (const xmlAccess::FfsXmlError&) {}

    //important! event source wxTheApp is NOT dependent on this instance -> disconnect!
    wxTheApp->Disconnect(wxEVT_KEY_DOWN,  wxKeyEventHandler(MainDialog::OnGlobalKeyEvent), nullptr, this);
    wxTheApp->Disconnect(wxEVT_CHAR_HOOK, wxKeyEventHandler(MainDialog::OnGlobalKeyEvent), nullptr, this);

#ifdef FFS_MAC
    //more (non-portable) wxWidgets crap: wxListBox leaks wxClientData, both of the following functions fail to clean up:
    //	src/common/ctrlsub.cpp:: wxItemContainer::~wxItemContainer() -> empty function body!!!
    //	src/osx/listbox_osx.cpp: wxListBox::~wxListBox()
    //=> finally a manual wxItemContainer::Clear() will render itself useful:
    m_listBoxHistory->Clear();
#endif

    auiMgr.UnInit();

    //no need for wxEventHandler::Disconnect() here; event sources are components of this window and are destroyed, too
}

//-------------------------------------------------------------------------------------------------------------------------------------

void MainDialog::onQueryEndSession()
{
    try { xmlAccess::writeConfig(getGlobalCfgBeforeExit()); }
    catch (const xmlAccess::FfsXmlError&) {} //we try our best do to something useful in this extreme situation - no reason to notify or even log errors here!

    try { xmlAccess::writeConfig(getConfig(), lastRunConfigName()); }
    catch (const xmlAccess::FfsXmlError&) {}
}


void MainDialog::setGlobalCfgOnInit(const xmlAccess::XmlGlobalSettings& globalSettings)
{
    globalCfg = globalSettings;

    //caveat set/get language asymmmetry! setLanguage(globalSettings.programLanguage); //throw FileError
    //we need to set langugabe before creating this class!

    //set dialog size and position:
    // - width/height are invalid if the window is minimized (eg x,y == -32000; height = 28, width = 160)
    // - multi-monitor setups: dialog may be placed on second monitor which is currently turned off
    if (globalSettings.gui.dlgSize.GetWidth () > 0 &&
        globalSettings.gui.dlgSize.GetHeight() > 0)
    {
        //calculate how much of the dialog will be visible on screen
        const int dialogAreaTotal = globalSettings.gui.dlgSize.GetWidth() * globalSettings.gui.dlgSize.GetHeight();
        int dialogAreaVisible = 0;

        const int monitorCount = wxDisplay::GetCount();
        for (int i = 0; i < monitorCount; ++i)
        {
            wxRect intersection = wxDisplay(i).GetClientArea().Intersect(wxRect(globalSettings.gui.dlgPos, globalSettings.gui.dlgSize));
            dialogAreaVisible = std::max(dialogAreaVisible, intersection.GetWidth() * intersection.GetHeight());
        }

        if (dialogAreaVisible > 0.1 * dialogAreaTotal) //at least 10% of the dialog should be visible!
            SetSize(wxRect(globalSettings.gui.dlgPos, globalSettings.gui.dlgSize));
        else
        {
            SetSize(wxRect(globalSettings.gui.dlgSize));
            Centre();
        }
    }
    else
        Centre();

    Maximize(globalSettings.gui.isMaximized);

    //set column attributes
    m_gridMainL   ->setColumnConfig(gridview::convertConfig(globalSettings.gui.columnAttribLeft));
    m_gridMainR   ->setColumnConfig(gridview::convertConfig(globalSettings.gui.columnAttribRight));
    m_splitterMain->setSashOffset(globalSettings.gui.sashOffset);

    m_gridNavi->setColumnConfig(treeview::convertConfig(globalSettings.gui.columnAttribNavi));
    treeview::setShowPercentage(*m_gridNavi, globalSettings.gui.showPercentBar);

    treeDataView->setSortDirection(globalSettings.gui.naviLastSortColumn, globalSettings.gui.naviLastSortAscending);

    //--------------------------------------------------------------------------------
    //load list of last used configuration files
    std::vector<Zstring> cfgFileNames = globalSettings.gui.cfgFileHistory;
    std::reverse(cfgFileNames.begin(), cfgFileNames.end()); //list is stored with last used files first in xml, however addFileToCfgHistory() needs them last!!!

    cfgFileNames.push_back(lastRunConfigName()); //make sure <Last session> is always part of history list (if existing)
    addFileToCfgHistory(cfgFileNames);
    removeObsoleteCfgHistoryItems(cfgFileNames); //remove non-existent items (we need this only on startup)
    //--------------------------------------------------------------------------------

    //load list of last used folders
    *folderHistoryLeft  = FolderHistory(globalSettings.gui.folderHistoryLeft,  globalSettings.gui.folderHistMax);
    *folderHistoryRight = FolderHistory(globalSettings.gui.folderHistoryRight, globalSettings.gui.folderHistMax);

    //show/hide file icons
    gridview::setupIcons(*m_gridMainL, *m_gridMainC, *m_gridMainR, globalSettings.gui.showIcons, convert(globalSettings.gui.iconSize));

    //------------------------------------------------------------------------------------------------
    //wxAuiManager erroneously loads panel captions, we don't want that
    typedef std::vector<std::pair<wxString, wxString> > CaptionNameMapping;
    CaptionNameMapping captionNameMap;
    const wxAuiPaneInfoArray& paneArray = auiMgr.GetAllPanes();
    for (size_t i = 0; i < paneArray.size(); ++i)
        captionNameMap.push_back(std::make_pair(paneArray[i].caption, paneArray[i].name));

    auiMgr.LoadPerspective(globalSettings.gui.guiPerspectiveLast);

    //restore original captions
    for (auto it = captionNameMap.begin(); it != captionNameMap.end(); ++it)
        auiMgr.GetPane(it->second).Caption(it->first);

    //if MainDialog::onQueryEndSession() is called while comparison is active, this panel is saved and restored as "visible"
    auiMgr.GetPane(compareStatus->getAsWindow()).Hide();

    m_menuItemCheckVersionAuto->Check(globalCfg.gui.lastUpdateCheck != -1);

    auiMgr.Update();
}


xmlAccess::XmlGlobalSettings MainDialog::getGlobalCfgBeforeExit()
{
    Freeze(); //no need to Thaw() again!!
    //	    wxWindowUpdateLocker dummy(this);

    xmlAccess::XmlGlobalSettings globalSettings = globalCfg;

    globalSettings.programLanguage = getLanguage();

    //retrieve column attributes
    globalSettings.gui.columnAttribLeft  = gridview::convertConfig(m_gridMainL->getColumnConfig());
    globalSettings.gui.columnAttribRight = gridview::convertConfig(m_gridMainR->getColumnConfig());
    globalSettings.gui.sashOffset        = m_splitterMain->getSashOffset();

    globalSettings.gui.columnAttribNavi = treeview::convertConfig(m_gridNavi->getColumnConfig());
    globalSettings.gui.showPercentBar   = treeview::getShowPercentage(*m_gridNavi);

    const std::pair<ColumnTypeNavi, bool> sortInfo = treeDataView->getSortDirection();
    globalSettings.gui.naviLastSortColumn    = sortInfo.first;
    globalSettings.gui.naviLastSortAscending = sortInfo.second;

    //--------------------------------------------------------------------------------
    //write list of last used configuration files
    std::map<int, Zstring> historyDetail; //(cfg-file/last use index)
    for (unsigned int i = 0; i < m_listBoxHistory->GetCount(); ++i)
        if (auto clientString = dynamic_cast<const wxClientHistoryData*>(m_listBoxHistory->GetClientObject(i)))
            historyDetail.insert(std::make_pair(clientString->lastUseIndex_, clientString->cfgFile_));

    //sort by last use; put most recent items *first* (looks better in xml than the reverse)
    std::vector<Zstring> history;
    std::transform(historyDetail.rbegin(), historyDetail.rend(), std::back_inserter(history), [](const std::pair<int, Zstring>& item) { return item.second; });

    if (history.size() > globalSettings.gui.cfgFileHistMax) //erase oldest elements
        history.resize(globalSettings.gui.cfgFileHistMax);

    globalSettings.gui.cfgFileHistory = history;
    //--------------------------------------------------------------------------------
    globalSettings.gui.lastUsedConfigFiles = activeConfigFiles;

    //write list of last used folders
    globalSettings.gui.folderHistoryLeft  = folderHistoryLeft ->getList();
    globalSettings.gui.folderHistoryRight = folderHistoryRight->getList();

    globalSettings.gui.guiPerspectiveLast = auiMgr.SavePerspective();

    //we need to portably retrieve non-iconized, non-maximized size and position (non-portable: GetWindowPlacement())
    //call *after* wxAuiManager::SavePerspective()!
    if (IsIconized())
        Iconize(false);

    globalSettings.gui.isMaximized = IsMaximized(); //evaluate AFTER uniconizing!

    if (IsMaximized())
        Maximize(false);

    globalSettings.gui.dlgSize = GetSize();
    globalSettings.gui.dlgPos  = GetPosition();

    return globalSettings;
}


void MainDialog::setSyncDirManually(const std::vector<FileSystemObject*>& selection, SyncDirection direction)
{
    if (!selection.empty())
    {
        std::for_each(selection.begin(), selection.end(),
                      [&](FileSystemObject* fsObj)
        {
            setSyncDirectionRec(direction, *fsObj); //set new direction (recursively)
            zen::setActiveStatus(true, *fsObj); //works recursively for directories
        });

        updateGui();
    }
}


void MainDialog::setFilterManually(const std::vector<FileSystemObject*>& selection, bool setIncluded)
{
    //if hidefiltered is active, there should be no filtered elements on screen => current element was filtered out
    assert(!currentCfg.hideExcludedItems || !setIncluded);

    if (!selection.empty())
    {
        std::for_each(selection.begin(), selection.end(),
        [&](FileSystemObject* fsObj) { zen::setActiveStatus(setIncluded, *fsObj); }); //works recursively for directories

        updateGuiDelayedIf(currentCfg.hideExcludedItems); //show update GUI before removing rows
    }
}


namespace
{
//perf: wxString doesn't model exponential growth and so is unusable for large data sets
typedef Zbase<wchar_t> zxString; //guaranteed exponential growth
}

void MainDialog::copySelectionToClipboard(const std::vector<const Grid*>& gridRefs)
{
    try
    {
        zxString clipboardString;

        auto addSelection = [&](const Grid& grid)
        {
            if (auto prov = grid.getDataProvider())
            {
                std::vector<Grid::ColumnAttribute> colAttr = grid.getColumnConfig();
                vector_remove_if(colAttr, [](const Grid::ColumnAttribute& ca) { return !ca.visible_; });
                if (!colAttr.empty())
                {
                    const std::vector<size_t> selection = grid.getSelectedRows();
                    std::for_each(selection.begin(), selection.end(),
                                  [&](size_t row)
                    {
                        std::for_each(colAttr.begin(), colAttr.end() - 1,
                                      [&](const Grid::ColumnAttribute& ca)
                        {
                            clipboardString += copyStringTo<zxString>(prov->getValue(row, ca.type_));
                            clipboardString += L'\t';
                        });
                        clipboardString += copyStringTo<zxString>(prov->getValue(row, colAttr.back().type_));
                        clipboardString += L'\n';
                    });
                }
            }
        };

        for (auto it = gridRefs.begin(); it != gridRefs.end(); ++it)
            addSelection(**it);

        //finally write to clipboard
        if (!clipboardString.empty())
            if (wxClipboard::Get()->Open())
            {
                ZEN_ON_SCOPE_EXIT(wxClipboard::Get()->Close());
                wxClipboard::Get()->SetData(new wxTextDataObject(copyStringTo<wxString>(clipboardString))); //ownership passed
            }
    }
    catch (const std::bad_alloc& e)
    {
        wxMessageBox(_("Out of memory!") + L" " + utfCvrtTo<std::wstring>(e.what()), _("Error"), wxOK | wxICON_ERROR, this);
    }
}


std::vector<FileSystemObject*> MainDialog::getGridSelection(bool fromLeft, bool fromRight) const
{
    std::set<size_t> selectedRows;

    auto addSelection = [&](const Grid& grid)
    {
        const std::vector<size_t>& sel = grid.getSelectedRows();
        selectedRows.insert(sel.begin(), sel.end());
    };

    if (fromLeft)
        addSelection(*m_gridMainL);

    if (fromRight)
        addSelection(*m_gridMainR);

    return gridDataView->getAllFileRef(selectedRows);
}


std::vector<FileSystemObject*> MainDialog::getTreeSelection() const
{
    std::vector<FileSystemObject*> output;

    const std::vector<size_t>& sel = m_gridNavi->getSelectedRows();
    std::for_each(sel.begin(), sel.end(),
                  [&](size_t row)
    {
        if (std::unique_ptr<TreeView::Node> node = treeDataView->getLine(row))
        {
            if (auto root = dynamic_cast<const TreeView::RootNode*>(node.get()))
            {
                //select first level of child elements
                std::transform(root->baseMap_.refSubDirs ().begin(), root->baseMap_.refSubDirs ().end(), std::back_inserter(output), [](FileSystemObject& fsObj) { return &fsObj; });
                std::transform(root->baseMap_.refSubFiles().begin(), root->baseMap_.refSubFiles().end(), std::back_inserter(output), [](FileSystemObject& fsObj) { return &fsObj; });
                std::transform(root->baseMap_.refSubLinks().begin(), root->baseMap_.refSubLinks().end(), std::back_inserter(output), [](FileSystemObject& fsObj) { return &fsObj; });
                //for (auto& fsObj : root->baseMap_.refSubLinks()) output.push_back(&fsObj); -> seriously MSVC, stop this disgrace and implement "range for"!
            }
            else if (auto dir = dynamic_cast<const TreeView::DirNode*>(node.get()))
                output.push_back(&(dir->dirObj_));
            else if (auto file = dynamic_cast<const TreeView::FilesNode*>(node.get()))
                output.insert(output.end(), file->filesAndLinks_.begin(), file->filesAndLinks_.end());
        }
    });
    return output;
}


//Exception class used to abort the "compare" and "sync" process
class AbortDeleteProcess {};

class ManualDeletionHandler : private wxEvtHandler, public DeleteFilesHandler //throw AbortDeleteProcess
{
public:
    ManualDeletionHandler(MainDialog& main) :
        mainDlg(main),
        abortRequested(false),
        ignoreErrors(false)
    {
        mainDlg.disableAllElements(true); //disable everything except abort button

        //register abort button
        mainDlg.m_buttonCancel->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( ManualDeletionHandler::OnAbortDeletion), nullptr, this );
        mainDlg.Connect(wxEVT_CHAR_HOOK, wxKeyEventHandler(ManualDeletionHandler::OnKeyPressed), nullptr, this);
    }

    ~ManualDeletionHandler()
    {
        //de-register abort button
        mainDlg.Disconnect(wxEVT_CHAR_HOOK, wxKeyEventHandler(ManualDeletionHandler::OnKeyPressed), nullptr, this);
        mainDlg.m_buttonCancel->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( ManualDeletionHandler::OnAbortDeletion ), nullptr, this );

        mainDlg.enableAllElements();
    }

    virtual Response reportError(const std::wstring& msg)
    {
        if (ignoreErrors)
            return DeleteFilesHandler::IGNORE_ERROR;

        forceUiRefresh();
        bool ignoreNextErrors = false;
        switch (showErrorDlg(&mainDlg,
                             ReturnErrorDlg::BUTTON_IGNORE | ReturnErrorDlg::BUTTON_RETRY | ReturnErrorDlg::BUTTON_CANCEL,
                             msg, &ignoreNextErrors))
        {
            case ReturnErrorDlg::BUTTON_IGNORE:
                ignoreErrors = ignoreNextErrors;
                return DeleteFilesHandler::IGNORE_ERROR;
            case ReturnErrorDlg::BUTTON_RETRY:
                return DeleteFilesHandler::RETRY;
            case ReturnErrorDlg::BUTTON_CANCEL:
                throw AbortDeleteProcess();
        }

        assert (false);
        return DeleteFilesHandler::IGNORE_ERROR; //dummy return value
    }

    virtual void reportWarning(const std::wstring& msg, bool& warningActive)
    {
        if (!warningActive || ignoreErrors)
            return;

        forceUiRefresh();
        bool dontWarnAgain = false;
        switch (showWarningDlg(&mainDlg, ReturnWarningDlg::BUTTON_IGNORE | ReturnWarningDlg::BUTTON_CANCEL, msg, dontWarnAgain))
        {
            case ReturnWarningDlg::BUTTON_SWITCH:
                assert(false);
            case ReturnWarningDlg::BUTTON_CANCEL:
                throw AbortDeleteProcess();

            case ReturnWarningDlg::BUTTON_IGNORE:
                warningActive = !dontWarnAgain;
                break;
        }
    }

    virtual void reportStatus (const std::wstring& msg)
    {
        statusMsg = msg;
        requestUiRefresh();
    }

private:
    virtual void requestUiRefresh()
    {
        if (updateUiIsAllowed())  //test if specific time span between ui updates is over
            forceUiRefresh();

        if (abortRequested) //test after (implicit) call to wxApp::Yield()
            throw AbortDeleteProcess();
    }

    void forceUiRefresh()
    {
        //std::wstring msg = toGuiString(deletionCount) +
        mainDlg.setStatusBarFullText(statusMsg);
        updateUiNow();
    }

    //context: C callstack message loop => throw()!
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

    MainDialog& mainDlg;

    bool abortRequested;
    bool ignoreErrors;
    //size_t deletionCount;   //
    std::wstring statusMsg; //status reporting
};


void MainDialog::deleteSelectedFiles(const std::vector<FileSystemObject*>& selectionLeft,
                                     const std::vector<FileSystemObject*>& selectionRight)
{
    if (!selectionLeft.empty() || !selectionRight.empty())
    {
        wxBusyCursor dummy; //show hourglass cursor

        wxWindow* oldFocus = wxWindow::FindFocus();
        ZEN_ON_SCOPE_EXIT(if (oldFocus) oldFocus->SetFocus();)

            bool deleteOnBothSides = false; //let's keep this disabled by default -> don't save

        if (zen::showDeleteDialog(this,
                                  selectionLeft,
                                  selectionRight,
                                  deleteOnBothSides,
                                  globalCfg.gui.useRecyclerForManualDeletion) == ReturnSmallDlg::BUTTON_OKAY)
        {
            try
            {
                //handle errors when deleting files/folders
                ManualDeletionHandler statusHandler(*this);

                zen::deleteFromGridAndHD(selectionLeft,
                                         selectionRight,
                                         folderCmp,
                                         extractDirectionCfg(getConfig().mainCfg),
                                         deleteOnBothSides,
                                         globalCfg.gui.useRecyclerForManualDeletion,
                                         statusHandler,
                                         globalCfg.optDialogs.warningRecyclerMissing);

                gridview::clearSelection(*m_gridMainL, *m_gridMainC, *m_gridMainR); //do not clear, if aborted!
            }
            catch (AbortDeleteProcess&) {}

            //remove rows that are empty: just a beautification, invalid rows shouldn't cause issues
            gridDataView->removeInvalidRows();

            //redraw grid neccessary to update new dimensions and for UI-Backend data linkage
            updateGui(); //call immediately after deleteFromGridAndHD!!!
        }
    }
}

namespace
{
template <SelectedSide side>
Zstring getExistingParentFolder(const FileSystemObject& fsObj)
{
    const DirMapping* dirObj = dynamic_cast<const DirMapping*>(&fsObj);
    if (!dirObj)
        dirObj = dynamic_cast<const DirMapping*>(&fsObj.parent());

    while (dirObj)
    {
        if (!dirObj->isEmpty<side>())
            return dirObj->getFullName<side>();

        dirObj = dynamic_cast<const DirMapping*>(&dirObj->parent());
    }
    return fsObj.getBaseDirPf<side>();
}
}

void MainDialog::openExternalApplication(const wxString& commandline, const std::vector<FileSystemObject*>& selection, bool leftSide)
{
    if (commandline.empty())
        return;

    auto selectionTmp = selection;

    const bool openFileBrowserRequested = [&]() -> bool
    {
        xmlAccess::XmlGlobalSettings::Gui dummy;
        return !dummy.externelApplications.empty() && dummy.externelApplications[0].second == commandline;
    }();

    //support fallback instead of an error in this special case
    if (openFileBrowserRequested)
    {
        if (selectionTmp.size() > 1) //do not open more than one explorer instance!
            selectionTmp.resize(1);  //

        if (selectionTmp.empty() ||
            (leftSide  && selectionTmp[0]->isEmpty<LEFT_SIDE >()) ||
            (!leftSide && selectionTmp[0]->isEmpty<RIGHT_SIDE>()))
        {
            Zstring fallbackDir;
            if (selectionTmp.empty())
                fallbackDir = leftSide ?
                              getFormattedDirectoryName(firstFolderPair->getLeftDir()) :
                              getFormattedDirectoryName(firstFolderPair->getRightDir());

            else
                fallbackDir = leftSide ?
                              getExistingParentFolder<LEFT_SIDE >(*selectionTmp[0]) :
                              getExistingParentFolder<RIGHT_SIDE>(*selectionTmp[0]);
#ifdef FFS_WIN
            zen::shellExecute(L"\"" + fallbackDir + L"\"");
#elif defined FFS_LINUX
            zen::shellExecute("xdg-open \"" + fallbackDir + "\"");
#elif defined FFS_MAC
            zen::shellExecute("open \"" + fallbackDir + "\"");
#endif
            return;
        }
    }

    //regular command evaluation
    for (auto it = selectionTmp.begin(); it != selectionTmp.end(); ++it) //context menu calls this function only if selection is not empty!
    {
        const FileSystemObject* fsObj = *it;

        Zstring path1 = fsObj->getBaseDirPf<LEFT_SIDE>() + fsObj->getObjRelativeName(); //full path, even if item is not existing!
        Zstring dir1  = beforeLast(path1, FILE_NAME_SEPARATOR); //Win: wrong for root paths like "C:\file.txt"

        Zstring path2 = fsObj->getBaseDirPf<RIGHT_SIDE>() + fsObj->getObjRelativeName();
        Zstring dir2  = beforeLast(path2, FILE_NAME_SEPARATOR);

        if (!leftSide)
        {
            std::swap(path1, path2);
            std::swap(dir1,  dir2);
        }

        Zstring command = utfCvrtTo<Zstring>(commandline);
        replace(command, Zstr("%item_path%"),    path1);
        replace(command, Zstr("%item2_path%"),   path2);
        replace(command, Zstr("%item_folder%"),  dir1 );
        replace(command, Zstr("%item2_folder%"), dir2 );

        auto cmdExp = expandMacros(command);
        zen::shellExecute(cmdExp); //shows error message if command is malformed
    }
}


void MainDialog::setStatusBarFileStatistics(size_t filesOnLeftView,
                                            size_t foldersOnLeftView,
                                            size_t filesOnRightView,
                                            size_t foldersOnRightView,
                                            UInt64 filesizeLeftView,
                                            UInt64 filesizeRightView)
{
    wxWindowUpdateLocker dummy(m_panelStatusBar); //avoid display distortion

    //select state
    bSizerFileStatus->Show(true);
    m_staticTextFullStatus->Hide();

    //fill statistics
    //update status information
    bSizerStatusLeftDirectories->Show(foldersOnLeftView > 0);
    bSizerStatusLeftFiles      ->Show(filesOnLeftView   > 0);

    setText(*m_staticTextStatusLeftDirs,  replaceCpy(_P("1 directory", "%x directories", foldersOnLeftView), L"%x", toGuiString(foldersOnLeftView), false));
    setText(*m_staticTextStatusLeftFiles, replaceCpy(_P("1 file", "%x files", filesOnLeftView), L"%x", toGuiString(filesOnLeftView), false));
    setText(*m_staticTextStatusLeftBytes, filesizeToShortString(to<Int64>(filesizeLeftView)));

    wxString statusMiddleNew;
    if (gridDataView->rowsTotal() > 0)
    {
        statusMiddleNew = _P("%y of 1 row in view", "%y of %x rows in view", gridDataView->rowsTotal());
        replace(statusMiddleNew, L"%x", toGuiString(gridDataView->rowsTotal ()), false); //%x is required to be the plural form placeholder!
        replace(statusMiddleNew, L"%y", toGuiString(gridDataView->rowsOnView()), false); //%y is secondary placeholder
    }

    bSizerStatusRightDirectories->Show(foldersOnRightView > 0);
    bSizerStatusRightFiles      ->Show(filesOnRightView   > 0);

    setText(*m_staticTextStatusRightDirs,  replaceCpy(_P("1 directory", "%x directories", foldersOnRightView), L"%x", toGuiString(foldersOnRightView), false));
    setText(*m_staticTextStatusRightFiles, replaceCpy(_P("1 file", "%x files", filesOnRightView), L"%x", toGuiString(filesOnRightView), false));
    setText(*m_staticTextStatusRightBytes, filesizeToShortString(to<Int64>(filesizeRightView)));

    //fill middle text (considering flashStatusInformation())
    if (oldStatusMsgs.empty())
        setText(*m_staticTextStatusMiddle, statusMiddleNew);
    else
        oldStatusMsgs.front() = statusMiddleNew;

    m_panelStatusBar->Layout();
}


void MainDialog::setStatusBarFullText(const wxString& msg)
{
    const bool needLayoutUpdate = !m_staticTextFullStatus->IsShown();
    //select state
    bSizerFileStatus->Show(false);
    m_staticTextFullStatus->Show();

    //update status information
    setText(*m_staticTextFullStatus, msg);
    m_panelStatusBar->Layout();

    if (needLayoutUpdate)
        auiMgr.Update(); //fix status bar height (needed on OS X)
}


void MainDialog::flashStatusInformation(const wxString& text)
{
    oldStatusMsgs.push_back(m_staticTextStatusMiddle->GetLabel());

    m_staticTextStatusMiddle->SetLabel(text);
    m_staticTextStatusMiddle->SetForegroundColour(wxColour(31, 57, 226)); //highlight color: blue
    m_panelStatusBar->Layout();
    //if (needLayoutUpdate) auiMgr.Update(); -> not needed here, this is called anyway in updateGui()

    processAsync2([] { boost::this_thread::sleep(boost::posix_time::millisec(2500)); },
                  [this] { this->restoreStatusInformation(); });
}


void MainDialog::restoreStatusInformation()
{
    if (!oldStatusMsgs.empty())
    {
        wxString oldMsg = oldStatusMsgs.back();
        oldStatusMsgs.pop_back();

        if (oldStatusMsgs.empty()) //restore original status text
        {
            m_staticTextStatusMiddle->SetLabel(oldMsg);
            m_staticTextStatusMiddle->SetForegroundColour(wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOWTEXT)); //reset color
            m_panelStatusBar->Layout();
        }
    }
}


void MainDialog::onProcessAsyncTasks(wxEvent& event)
{
    //schedule and run long-running tasks asynchronously
    asyncTasks.evalResults(); //process results on GUI queue
    if (asyncTasks.empty())
        timerForAsyncTasks.Stop();
}


void MainDialog::disableAllElements(bool enableAbort)
{
    //when changing consider: comparison, synchronization, manual deletion

    EnableCloseButton(false); //not allowed for synchronization! progress indicator is top window! -> not honored on OS X!

    //disables all elements (except abort button) that might receive user input during long-running processes: comparison, deletion
    m_panelViewFilter    ->Disable();
    m_bpButtonCmpConfig  ->Disable();
    m_panelFilter        ->Disable();
    m_panelConfig        ->Disable();
    m_bpButtonSyncConfig ->Disable();
    m_buttonSync         ->Disable();
    m_gridMainL          ->Disable();
    m_gridMainC          ->Disable();
    m_gridMainR          ->Disable();
    m_panelStatistics    ->Disable();
    m_gridNavi           ->Disable();
    m_panelDirectoryPairs->Disable();
    m_splitterMain       ->Disable();
    m_menubar1->EnableTop(0, false);
    m_menubar1->EnableTop(1, false);
    m_menubar1->EnableTop(2, false);

    if (enableAbort)
    {
        //show abort button
        m_buttonCancel->Enable();
        m_buttonCancel->Show();
        if (m_buttonCancel->IsShownOnScreen())
            m_buttonCancel->SetFocus();
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
    m_buttonSync         ->Enable();
    m_gridMainL          ->Enable();
    m_gridMainC          ->Enable();
    m_gridMainR          ->Enable();
    m_panelStatistics    ->Enable();
    m_gridNavi           ->Enable();
    m_panelDirectoryPairs->Enable();
    m_splitterMain       ->Enable();
    m_menubar1->EnableTop(0, true);
    m_menubar1->EnableTop(1, true);
    m_menubar1->EnableTop(2, true);

    //show compare button
    m_buttonCancel->Disable();
    m_buttonCancel->Hide();
    m_buttonCompare->Enable();
    m_buttonCompare->Show();

    m_panelTopButtons->Layout();
    m_panelTopButtons->Enable();
}


namespace
{
void updateSizerOrientation(wxBoxSizer& sizer, wxWindow& window)
{
    const int newOrientation = window.GetSize().GetWidth() > window.GetSize().GetHeight() ? wxHORIZONTAL : wxVERTICAL; //check window NOT sizer width!
    if (sizer.GetOrientation() != newOrientation)
    {
        sizer.SetOrientation(newOrientation);
        window.Layout();
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
    //updateSizerOrientation(*bSizerStatistics, *m_panelStatistics);

    //we need something more fancy:
    const int parentOrient = m_panelStatistics->GetSize().GetWidth() > m_panelStatistics->GetSize().GetHeight() ? wxHORIZONTAL : wxVERTICAL; //check window NOT sizer width!
    if (bSizerStatistics->GetOrientation() != parentOrient)
    {
        bSizerStatistics->SetOrientation(parentOrient);

        //apply opposite orientation for child sizers
        const int childOrient = parentOrient == wxHORIZONTAL ? wxVERTICAL : wxHORIZONTAL;
        wxSizerItemList& sl = bSizerStatistics->GetChildren();
        for (auto it = sl.begin(); it != sl.end(); ++it) //yet another wxWidgets bug keeps us from using std::for_each
        {
            wxSizerItem& szItem = **it;
            if (auto sizerChild = dynamic_cast<wxBoxSizer*>(szItem.GetSizer()))
                if (sizerChild->GetOrientation() != childOrient)
                    sizerChild->SetOrientation(childOrient);
        }
        m_panelStatistics->Layout();
    }

    event.Skip();
}


void MainDialog::OnResizeLeftFolderWidth(wxEvent& event)
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


void MainDialog::onTreeButtonEvent(wxKeyEvent& event)
{
    int keyCode = event.GetKeyCode();
    if (m_gridNavi->GetLayoutDirection() == wxLayout_RightToLeft)
    {
        if (keyCode == WXK_LEFT)
            keyCode = WXK_RIGHT;
        else if (keyCode == WXK_RIGHT)
            keyCode = WXK_LEFT;
        else if (keyCode == WXK_NUMPAD_LEFT)
            keyCode = WXK_NUMPAD_RIGHT;
        else if (keyCode == WXK_NUMPAD_RIGHT)
            keyCode = WXK_NUMPAD_LEFT;
    }

    if (event.ControlDown())
        switch (keyCode)
        {
            case 'C':
            case WXK_INSERT: //CTRL + C || CTRL + INS
            {
                std::vector<const Grid*> gridRefs;
                gridRefs.push_back(m_gridNavi);
                copySelectionToClipboard(gridRefs);
            }
            return;
        }
    else if (event.AltDown())
        switch (keyCode)
        {
            case WXK_NUMPAD_LEFT:
            case WXK_LEFT: //ALT + <-
                setSyncDirManually(getTreeSelection(), SYNC_DIR_LEFT);
                return;

            case WXK_NUMPAD_RIGHT:
            case WXK_RIGHT: //ALT + ->
                setSyncDirManually(getTreeSelection(), SYNC_DIR_RIGHT);
                return;

            case WXK_NUMPAD_UP:
            case WXK_NUMPAD_DOWN:
            case WXK_UP:   /* ALT + /|\   */
            case WXK_DOWN: /* ALT + \|/   */
                setSyncDirManually(getTreeSelection(), SYNC_DIR_NONE);
                return;
        }

    else
        switch (keyCode)
        {
            case WXK_SPACE:
            case WXK_NUMPAD_SPACE:
            {
                const std::vector<FileSystemObject*>& selection = getTreeSelection();
                if (!selection.empty())
                    setFilterManually(selection, !selection[0]->isActive());
            }
            return;

            case WXK_DELETE:
            case WXK_NUMPAD_DELETE:
                deleteSelectedFiles(getTreeSelection(), getTreeSelection());
                return;
        }

    event.Skip(); //unknown keypress: propagate
}


void MainDialog::onGridButtonEventL(wxKeyEvent& event)
{
    onGridButtonEvent(event, *m_gridMainL, true);
}
void MainDialog::onGridButtonEventC(wxKeyEvent& event)
{
    onGridButtonEvent(event, *m_gridMainC, true);
}
void MainDialog::onGridButtonEventR(wxKeyEvent& event)
{
    onGridButtonEvent(event, *m_gridMainR, false);
}

void MainDialog::onGridButtonEvent(wxKeyEvent& event, Grid& grid, bool leftSide)
{
    int keyCode = event.GetKeyCode();
    if (grid.GetLayoutDirection() == wxLayout_RightToLeft)
    {
        if (keyCode == WXK_LEFT)
            keyCode = WXK_RIGHT;
        else if (keyCode == WXK_RIGHT)
            keyCode = WXK_LEFT;
        else if (keyCode == WXK_NUMPAD_LEFT)
            keyCode = WXK_NUMPAD_RIGHT;
        else if (keyCode == WXK_NUMPAD_RIGHT)
            keyCode = WXK_NUMPAD_LEFT;
    }

    if (event.ControlDown())
        switch (keyCode)
        {
            case 'C':
            case WXK_INSERT: //CTRL + C || CTRL + INS
            {
                std::vector<const Grid*> gridRefs;
                gridRefs.push_back(m_gridMainL);
                gridRefs.push_back(m_gridMainR);
                copySelectionToClipboard(gridRefs);
            }
            return; // -> swallow event! don't allow default grid commands!
        }

    else if (event.AltDown())
        switch (keyCode)
        {
            case WXK_NUMPAD_LEFT:
            case WXK_LEFT: //ALT + <-
                setSyncDirManually(getGridSelection(), SYNC_DIR_LEFT);
                return;

            case WXK_NUMPAD_RIGHT:
            case WXK_RIGHT: //ALT + ->
                setSyncDirManually(getGridSelection(), SYNC_DIR_RIGHT);
                return;

            case WXK_NUMPAD_UP:
            case WXK_NUMPAD_DOWN:
            case WXK_UP:   /* ALT + /|\   */
            case WXK_DOWN: /* ALT + \|/   */
                setSyncDirManually(getGridSelection(), SYNC_DIR_NONE);
                return;
        }

    else
        switch (keyCode)
        {
            case WXK_DELETE:
            case WXK_NUMPAD_DELETE:
                deleteSelectedFiles(getGridSelection(true, false),
                                    getGridSelection(false, true));
                return;

            case WXK_SPACE:
            case WXK_NUMPAD_SPACE:
            {
                const std::vector<FileSystemObject*>& selection = getGridSelection();
                if (!selection.empty())
                    setFilterManually(selection, !selection[0]->isActive());
            }
            return;

            case WXK_RETURN:
            case WXK_NUMPAD_ENTER:
                if (!globalCfg.gui.externelApplications.empty())
                    openExternalApplication(globalCfg.gui.externelApplications[0].second, //open with first external application
                                            getGridSelection(), leftSide);
                return;
        }

    event.Skip(); //unknown keypress: propagate
}


bool isPartOf(const wxWindow* child, const wxWindow* top)
{
    for (const wxWindow* wnd = child; wnd != nullptr; wnd = wnd->GetParent())
        if (wnd == top)
            return true;
    return false;
}


void MainDialog::OnGlobalKeyEvent(wxKeyEvent& event) //process key events without explicit menu entry :)
{
    //avoid recursion!!! -> this ugly construct seems to be the only (portable) way to avoid re-entrancy
    //recursion may happen in multiple situations: e.g. modal dialogs, wxGrid::ProcessEvent()!
    if (processingGlobalKeyEvent  ||
        !IsShown()                ||
        !IsActive()               ||
        !IsEnabled()              ||
        !m_gridMainL->IsEnabled() || //
        !m_gridMainC->IsEnabled() || //only handle if main window is in use
        !m_gridMainR->IsEnabled())   //
    {
        event.Skip();
        return;
    }

    processingGlobalKeyEvent = true;
    ZEN_ON_SCOPE_EXIT(processingGlobalKeyEvent = false;)
    //----------------------------------------------------

    const int keyCode = event.GetKeyCode();

    //CTRL + X
    if (event.ControlDown())
        switch (keyCode)
        {
            case 'F': //CTRL + F
                zen::startFind(this, *m_gridMainL, *m_gridMainR, globalCfg.gui.textSearchRespectCase);
                return; //-> swallow event!
        }

    switch (keyCode)
    {
        case WXK_F3:        //F3
        case WXK_NUMPAD_F3: //
            zen::findNext(this, *m_gridMainL, *m_gridMainR, globalCfg.gui.textSearchRespectCase);
            return; //-> swallow event!

        case WXK_F8:        //F8
            setViewTypeSyncAction(!m_bpButtonViewTypeSyncAction->isActive());
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
            if (!isPartOf(focus, m_gridMainL     ) && //
                !isPartOf(focus, m_gridMainC     ) && //don't propagate keyboard commands if grid is already in focus
                !isPartOf(focus, m_gridMainR     ) && //
                !isPartOf(focus, m_gridNavi      ) &&
                !isPartOf(focus, m_listBoxHistory) && //don't propagate if selecting config
                !isPartOf(focus, m_directoryLeft)  && //don't propagate if changing directory field
                !isPartOf(focus, m_directoryRight) &&
                !isPartOf(focus, m_scrolledWindowFolderPairs))
                if (wxEvtHandler* evtHandler = m_gridMainL->getMainWin().GetEventHandler())
                {
                    m_gridMainL->SetFocus();

                    event.SetEventType(wxEVT_KEY_DOWN); //the grid event handler doesn't expect wxEVT_CHAR_HOOK!
                    evtHandler->ProcessEvent(event); //propagating event catched at wxTheApp to child leads to recursion, but we prevented it...
                    event.Skip(false); //definitively handled now!
                    return;
                }
        }
        break;
    }

    event.Skip();
}


void MainDialog::onNaviSelection(GridRangeSelectEvent& event)
{
    //scroll m_gridMain to user's new selection on m_gridNavi
    ptrdiff_t leadRow = -1;
    if (event.rowFirst_ != event.rowLast_)
        if (std::unique_ptr<TreeView::Node> node = treeDataView->getLine(event.rowFirst_))
        {
            if (const TreeView::RootNode* root = dynamic_cast<const TreeView::RootNode*>(node.get()))
                leadRow = gridDataView->findRowFirstChild(&(root->baseMap_));
            else if (const TreeView::DirNode* dir = dynamic_cast<const TreeView::DirNode*>(node.get()))
            {
                leadRow = gridDataView->findRowDirect(&(dir->dirObj_));
                if (leadRow < 0) //directory was filtered out! still on tree view (but NOT on grid view)
                    leadRow = gridDataView->findRowFirstChild(&(dir->dirObj_));
            }
            else if (const TreeView::FilesNode* files = dynamic_cast<const TreeView::FilesNode*>(node.get()))
            {
                assert(!files->filesAndLinks_.empty());
                if (!files->filesAndLinks_.empty())
                    leadRow = gridDataView->findRowDirect(files->filesAndLinks_[0]->getId());
            }
        }

    if (leadRow >= 0)
    {
        leadRow = std::max<ptrdiff_t>(0, leadRow - 1); //scroll one more row

        m_gridMainL->scrollTo(leadRow);
        m_gridMainC->scrollTo(leadRow);
        m_gridMainR->scrollTo(leadRow);

        m_gridNavi->getMainWin().Update(); //draw cursor immediately rather than on next idle event (required for slow CPUs, netbook)

    }

    //get selection on navigation tree and set corresponding markers on main grid
    hash_set<const FileSystemObject*> markedFilesAndLinks; //mark files/symlinks directly
    hash_set<const HierarchyObject*>  markedContainer;     //mark full container including child-objects

    const std::vector<size_t>& selection = m_gridNavi->getSelectedRows();
    std::for_each(selection.begin(), selection.end(),
                  [&](size_t row)
    {
        if (std::unique_ptr<TreeView::Node> node = treeDataView->getLine(row))
        {
            if (const TreeView::RootNode* root = dynamic_cast<const TreeView::RootNode*>(node.get()))
                markedContainer.insert(&(root->baseMap_));
            else if (const TreeView::DirNode* dir = dynamic_cast<const TreeView::DirNode*>(node.get()))
                markedContainer.insert(&(dir->dirObj_));
            else if (const TreeView::FilesNode* files = dynamic_cast<const TreeView::FilesNode*>(node.get()))
                markedFilesAndLinks.insert(files->filesAndLinks_.begin(), files->filesAndLinks_.end());
        }
    });

    gridview::setNavigationMarker(*m_gridMainL, std::move(markedFilesAndLinks), std::move(markedContainer));

    event.Skip();
}


void MainDialog::onNaviGridContext(GridClickEvent& event)
{
    const auto& selection = getTreeSelection(); //referenced by lambdas!
    ContextMenu menu;

    //----------------------------------------------------------------------------------------------------
    if (!selection.empty())
        //std::any_of(selection.begin(), selection.end(), [](const FileSystemObject* fsObj){ return fsObj->getSyncOperation() != SO_EQUAL; })) -> doesn't consider directories
    {
        auto getImage = [&](SyncDirection dir, SyncOperation soDefault)
        {
            return mirrorIfRtl(getSyncOpImage(selection[0]->getSyncOperation() != SO_EQUAL ?
                                              selection[0]->testSyncOperation(dir) : soDefault));
        };
        const wxBitmap opRight = getImage(SYNC_DIR_RIGHT, SO_OVERWRITE_RIGHT);
        const wxBitmap opNone  = getImage(SYNC_DIR_NONE,  SO_DO_NOTHING     );
        const wxBitmap opLeft  = getImage(SYNC_DIR_LEFT,  SO_OVERWRITE_LEFT );

        wxString shortCutLeft  = L"\tAlt+Left";
        wxString shortCutRight = L"\tAlt+Right";
        if (wxTheApp->GetLayoutDirection() == wxLayout_RightToLeft)
            std::swap(shortCutLeft, shortCutRight);

        menu.addItem(_("Set direction:") + L" ->" + shortCutRight, [this, &selection] { setSyncDirManually(selection, SYNC_DIR_RIGHT); }, &opRight);
        menu.addItem(_("Set direction:") + L" -" L"\tAlt+Down",    [this, &selection] { setSyncDirManually(selection, SYNC_DIR_NONE);  }, &opNone);
        menu.addItem(_("Set direction:") + L" <-" + shortCutLeft,  [this, &selection] { setSyncDirManually(selection, SYNC_DIR_LEFT);  }, &opLeft);
        //Gtk needs a direction, "<-", because it has no context menu icons!
        //Gtk requires "no spaces" for shortcut identifiers!
        menu.addSeparator();
    }
    //----------------------------------------------------------------------------------------------------
    if (!selection.empty())
    {
        if (selection[0]->isActive())
            menu.addItem(_("Exclude temporarily") + L"\tSpace", [this, &selection] { setFilterManually(selection, false); }, &getResourceImage(L"checkboxFalse"));
        else
            menu.addItem(_("Include temporarily") + L"\tSpace", [this, &selection] { setFilterManually(selection, true); }, &getResourceImage(L"checkboxTrue"));
    }
    else
        menu.addItem(_("Exclude temporarily") + L"\tSpace", [] {}, nullptr, false);

    //----------------------------------------------------------------------------------------------------
    //EXCLUDE FILTER
    if (selection.size() == 1)
    {
        //by relative path
        menu.addItem(_("Exclude via filter:") + L" " + (FILE_NAME_SEPARATOR + selection[0]->getObjRelativeName()),
                     [this, &selection] { excludeItems(selection); }, &getResourceImage(L"filterSmall"));
    }
    else if (selection.size() > 1)
    {
        //by relative path
        menu.addItem(_("Exclude via filter:") + L" " + _("<multiple selection>"),
                     [this, &selection] { excludeItems(selection); }, &getResourceImage(L"filterSmall"));
    }

    //----------------------------------------------------------------------------------------------------
    //CONTEXT_DELETE_FILES
    menu.addSeparator();
    menu.addItem(_("Delete") + L"\tDelete", [&] { deleteSelectedFiles(selection, selection); }, nullptr, !selection.empty());

    menu.popup(*this);
}


void MainDialog::onMainGridContextC(GridClickEvent& event)
{
    ContextMenu menu;

    menu.addItem(_("Include all"), [&]
    {
        zen::setActiveStatus(true, folderCmp);
        updateGui();
    }, nullptr, gridDataView->rowsTotal() > 0);

    menu.addItem(_("Exclude all"), [&]
    {
        zen::setActiveStatus(false, folderCmp);
        updateGuiDelayedIf(currentCfg.hideExcludedItems); //show update GUI before removing rows
    }, nullptr, gridDataView->rowsTotal() > 0);

    menu.popup(*this);
}

void MainDialog::onMainGridContextL(GridClickEvent& event)
{
    onMainGridContextRim(true);
}

void MainDialog::onMainGridContextR(GridClickEvent& event)
{
    onMainGridContextRim(false);
}

void MainDialog::onMainGridContextRim(bool leftSide)
{
    const auto& selection = getGridSelection(); //referenced by lambdas!
    ContextMenu menu;

    if (!selection.empty())
    {
        auto getImage = [&](SyncDirection dir, SyncOperation soDefault)
        {
            return mirrorIfRtl(getSyncOpImage(selection[0]->getSyncOperation() != SO_EQUAL ?
                                              selection[0]->testSyncOperation(dir) : soDefault));
        };
        const wxBitmap opRight = getImage(SYNC_DIR_RIGHT, SO_OVERWRITE_RIGHT);
        const wxBitmap opNone  = getImage(SYNC_DIR_NONE,  SO_DO_NOTHING     );
        const wxBitmap opLeft  = getImage(SYNC_DIR_LEFT,  SO_OVERWRITE_LEFT );

        wxString shortCutLeft  = L"\tAlt+Left";
        wxString shortCutRight = L"\tAlt+Right";
        if (wxTheApp->GetLayoutDirection() == wxLayout_RightToLeft)
            std::swap(shortCutLeft, shortCutRight);

        menu.addItem(_("Set direction:") + L" ->" + shortCutRight, [this, &selection] { setSyncDirManually(selection, SYNC_DIR_RIGHT); }, &opRight);
        menu.addItem(_("Set direction:") + L" -" L"\tAlt+Down",    [this, &selection] { setSyncDirManually(selection, SYNC_DIR_NONE);  }, &opNone);
        menu.addItem(_("Set direction:") + L" <-" + shortCutLeft,  [this, &selection] { setSyncDirManually(selection, SYNC_DIR_LEFT);  }, &opLeft);
        //Gtk needs a direction, "<-", because it has no context menu icons!
        //Gtk requires "no spaces" for shortcut identifiers!
        menu.addSeparator();
    }
    //----------------------------------------------------------------------------------------------------
    if (!selection.empty())
    {
        if (selection[0]->isActive())
            menu.addItem(_("Exclude temporarily") + L"\tSpace", [this, &selection] { setFilterManually(selection, false); }, &getResourceImage(L"checkboxFalse"));
        else
            menu.addItem(_("Include temporarily") + L"\tSpace", [this, &selection] { setFilterManually(selection, true); }, &getResourceImage(L"checkboxTrue"));
    }
    else
        menu.addItem(_("Exclude temporarily") + L"\tSpace", [] {}, nullptr, false);

    //----------------------------------------------------------------------------------------------------
    //EXCLUDE FILTER
    if (selection.size() == 1)
    {
        ContextMenu submenu;

        //by extension
        if (dynamic_cast<const DirMapping*>(selection[0]) == nullptr) //non empty && no directory
        {
            const Zstring filename = afterLast(selection[0]->getObjRelativeName(), FILE_NAME_SEPARATOR);
            if (contains(filename, Zchar('.'))) //be careful: AfterLast would return the whole string if '.' were not found!
            {
                const Zstring extension = afterLast(filename, Zchar('.'));

                submenu.addItem(L"*." + utfCvrtTo<wxString>(extension),
                                [this, extension] { excludeExtension(extension); });
            }
        }

        //by short name
        submenu.addItem(utfCvrtTo<wxString>(Zstring(Zstr("*")) + FILE_NAME_SEPARATOR + selection[0]->getObjShortName()),
                        [this, &selection] { excludeShortname(*selection[0]); });

        //by relative path
        submenu.addItem(utfCvrtTo<wxString>(FILE_NAME_SEPARATOR + selection[0]->getObjRelativeName()),
                        [this, &selection] { excludeItems(selection); });

        menu.addSubmenu(_("Exclude via filter:"), submenu, &getResourceImage(L"filterSmall"));
    }
    else if (selection.size() > 1)
    {
        //by relative path
        menu.addItem(_("Exclude via filter:") + L" " + _("<multiple selection>"),
                     [this, &selection] { excludeItems(selection); }, &getResourceImage(L"filterSmall"));
    }

    //----------------------------------------------------------------------------------------------------
    //CONTEXT_EXTERNAL_APP
    if (!globalCfg.gui.externelApplications.empty())
    {
        menu.addSeparator();

        for (auto it = globalCfg.gui.externelApplications.begin();
             it != globalCfg.gui.externelApplications.end();
             ++it)
        {
            //translate default external apps on the fly: 1. "open in explorer" 2. "start directly"
            wxString description = zen::implementation::translate(it->first);
            if (description.empty())
                description = L" "; //wxWidgets doesn't like empty items

            const wxString command = it->second; //COPY into lambda

            auto openApp = [this, &selection, leftSide, command] { openExternalApplication(command, selection, leftSide); };

            if (it == globalCfg.gui.externelApplications.begin())
                description += L"\tEnter";

            menu.addItem(description, openApp, nullptr, !selection.empty());
        }
    }
    //----------------------------------------------------------------------------------------------------
    //CONTEXT_DELETE_FILES
    menu.addSeparator();

    menu.addItem(_("Delete") + L"\tDelete", [this]
    {
        deleteSelectedFiles(
            getGridSelection(true, false),
            getGridSelection(false, true));
    }, nullptr, !selection.empty());

    menu.popup(*this);
}


void MainDialog::excludeExtension(const Zstring& extension)
{
    const Zstring newExclude = Zstr("*.") + extension;

    //add to filter config
    Zstring& excludeFilter = currentCfg.mainCfg.globalFilter.excludeFilter;
    if (!excludeFilter.empty() && !endsWith(excludeFilter, Zstr(";")) && !endsWith(excludeFilter, Zstr("\n")))
        excludeFilter += Zstr("\n");
    excludeFilter += newExclude + Zstr(";"); //';' is appended to 'mark' that next exclude extension entry won't write to new line

    updateGlobalFilterButton();

    //do not fully apply filter, just exclude new items
    std::for_each(begin(folderCmp), end(folderCmp), [&](BaseDirMapping& baseMap) { addHardFiltering(baseMap, newExclude); });
    updateGui();
}


void MainDialog::excludeShortname(const FileSystemObject& fsObj)
{
    Zstring newExclude = Zstring(Zstr("*")) + FILE_NAME_SEPARATOR + fsObj.getObjShortName();
    const bool isDir = dynamic_cast<const DirMapping*>(&fsObj) != nullptr;
    if (isDir)
        newExclude += FILE_NAME_SEPARATOR;

    //add to filter config
    Zstring& excludeFilter = currentCfg.mainCfg.globalFilter.excludeFilter;
    if (!excludeFilter.empty() && !endsWith(excludeFilter, Zstr("\n")))
        excludeFilter += Zstr("\n");
    excludeFilter += newExclude;

    updateGlobalFilterButton();

    //do not fully apply filter, just exclude new items
    std::for_each(begin(folderCmp), end(folderCmp), [&](BaseDirMapping& baseMap) { addHardFiltering(baseMap, newExclude); });
    updateGui();
}


void MainDialog::excludeItems(const std::vector<FileSystemObject*>& selection)
{
    if (!selection.empty()) //check needed to determine if filtering is needed
    {
        Zstring newExclude;
        for (auto it = selection.begin(); it != selection.end(); ++it)
        {
            FileSystemObject* fsObj = *it;

            if (it != selection.begin())
                newExclude += Zstr("\n");

            //#pragma warning(suppress: 6011) -> fsObj bound in this context!
            newExclude += FILE_NAME_SEPARATOR + fsObj->getObjRelativeName();

            const bool isDir = dynamic_cast<const DirMapping*>(fsObj) != nullptr;
            if (isDir)
                newExclude += FILE_NAME_SEPARATOR;
        }

        //add to filter config
        Zstring& excludeFilter = currentCfg.mainCfg.globalFilter.excludeFilter;
        if (!excludeFilter.empty() && !endsWith(excludeFilter, Zstr("\n")))
            excludeFilter += Zstr("\n");
        excludeFilter += newExclude;

        updateGlobalFilterButton();

        //do not fully apply filter, just exclude new items
        std::for_each(begin(folderCmp), end(folderCmp), [&](BaseDirMapping& baseMap) { addHardFiltering(baseMap, newExclude); });
        updateGui();
    }
}


void MainDialog::onGridLabelContextC(GridClickEvent& event)
{
    ContextMenu menu;

    const bool actionView = m_bpButtonViewTypeSyncAction->isActive();
    menu.addRadio(_("Category") + (actionView  ? L"\tF8" : L""), [&] { setViewTypeSyncAction(false); }, !actionView);
    menu.addRadio(_("Action")   + (!actionView ? L"\tF8" : L""), [&] { setViewTypeSyncAction(true ); },  actionView);

    //menu.addItem(_("Category") + L"\tF8", [&] { setViewTypeSyncAction(false); }, m_bpButtonViewTypeSyncAction->isActive() ? nullptr : &getResourceImage(L"compareSmall"));
    //menu.addItem(_("Action"),             [&] { setViewTypeSyncAction(true ); }, m_bpButtonViewTypeSyncAction->isActive() ? &getResourceImage(L"syncSmall") : nullptr);
    menu.popup(*this);
}


void MainDialog::onGridLabelContextL(GridClickEvent& event)
{
    onGridLabelContext(*m_gridMainL, static_cast<ColumnTypeRim>(event.colType_), getDefaultColumnAttributesLeft());
}
void MainDialog::onGridLabelContextR(GridClickEvent& event)
{
    onGridLabelContext(*m_gridMainR, static_cast<ColumnTypeRim>(event.colType_), getDefaultColumnAttributesRight());
}


void MainDialog::onGridLabelContext(Grid& grid, ColumnTypeRim type, const std::vector<ColumnAttributeRim>& defaultColumnAttributes)
{
    ContextMenu menu;

    auto toggleColumn = [&](const Grid::ColumnAttribute& ca)
    {
        auto colAttr = grid.getColumnConfig();

        for (auto it = colAttr.begin(); it != colAttr.end(); ++it)
            if (it->type_ == ca.type_)
            {
                it->visible_ = !ca.visible_;
                grid.setColumnConfig(colAttr);
                return;
            }
    };

    if (auto prov = grid.getDataProvider())
    {
        const auto& colAttr = grid.getColumnConfig();
        for (auto it = colAttr.begin(); it != colAttr.end(); ++it)
        {
            const Grid::ColumnAttribute& ca = *it;

            menu.addCheckBox(prov->getColumnLabel(ca.type_), [ca, toggleColumn] { toggleColumn(ca); },
                             ca.visible_, ca.type_ != static_cast<ColumnType>(COL_TYPE_FILENAME)); //do not allow user to hide file name column!
        }
    }
    //----------------------------------------------------------------------------------------------
    menu.addSeparator();

    auto setDefault = [&]
    {
        grid.setColumnConfig(gridview::convertConfig(defaultColumnAttributes));
    };
    menu.addItem(_("&Default"), setDefault); //'&' -> reuse text from "default" buttons elsewhere
    //----------------------------------------------------------------------------------------------
    menu.addSeparator();
    menu.addCheckBox(_("Show icons:"), [&]
    {
        globalCfg.gui.showIcons = !globalCfg.gui.showIcons;
        gridview::setupIcons(*m_gridMainL, *m_gridMainC, *m_gridMainR, globalCfg.gui.showIcons, convert(globalCfg.gui.iconSize));

    }, globalCfg.gui.showIcons);

    auto setIconSize = [&](xmlAccess::FileIconSize sz)
    {
        globalCfg.gui.iconSize = sz;
        gridview::setupIcons(*m_gridMainL, *m_gridMainC, *m_gridMainR, globalCfg.gui.showIcons, convert(sz));
    };
    auto addSizeEntry = [&](const wxString& label, xmlAccess::FileIconSize sz)
    {
        auto setIconSize2 = setIconSize; //bring into scope
        menu.addRadio(label, [sz, setIconSize2] { setIconSize2(sz); }, globalCfg.gui.iconSize == sz, globalCfg.gui.showIcons);
    };
    addSizeEntry(L"    " + _("Small" ), xmlAccess::ICON_SIZE_SMALL );
    addSizeEntry(L"    " + _("Medium"), xmlAccess::ICON_SIZE_MEDIUM);
    addSizeEntry(L"    " + _("Large" ), xmlAccess::ICON_SIZE_LARGE );
    //----------------------------------------------------------------------------------------------
    if (type == COL_TYPE_DATE)
    {
        menu.addSeparator();

        auto selectTimeSpan = [&]
        {
            if (showSelectTimespanDlg(this, manualTimeSpanFrom, manualTimeSpanTo) == ReturnSmallDlg::BUTTON_OKAY)
            {
                applyTimeSpanFilter(folderCmp, manualTimeSpanFrom, manualTimeSpanTo); //overwrite current active/inactive settings
                //updateGuiDelayedIf(currentCfg.hideExcludedItems); //show update GUI before removing rows
                updateGui();
            }
        };
        menu.addItem(_("Select time span..."), selectTimeSpan);
    }

    menu.popup(*this);
}


void MainDialog::OnContextSetLayout(wxMouseEvent& event)
{
    ContextMenu menu;

    menu.addItem(_("Default view"), [&]
    {
        m_splitterMain->setSashOffset(0);
        auiMgr.LoadPerspective(defaultPerspective);
        updateGuiForFolderPair();
    });
    //----------------------------------------------------------------------------------------
    std::vector<std::pair<wxString, wxString>> captionNameMap; //(caption, identifier)

    const wxAuiPaneInfoArray& paneArray = auiMgr.GetAllPanes();
    for (size_t i = 0; i < paneArray.size(); ++i)
        if (!paneArray[i].IsShown() && !paneArray[i].name.empty() && paneArray[i].window != compareStatus->getAsWindow())
            captionNameMap.push_back(std::make_pair(paneArray[i].caption, paneArray[i].name));

    if (!captionNameMap.empty())
        menu.addSeparator();

    auto showPanel = [&](const wxString& identifier)
    {
        auiMgr.GetPane(identifier).Show();
        auiMgr.Update();
    };

    for (auto it = captionNameMap.begin(); it != captionNameMap.end(); ++it)
    {
        const wxString label = replaceCpy(_("Show \"%x\""), L"%x", it->first);
        const wxString identifier = it->second;

        menu.addItem(label, [showPanel, identifier] { showPanel(identifier); });
    }

    menu.popup(*this);
}


void MainDialog::OnCompSettingsContext(wxMouseEvent& event)
{
    ContextMenu menu;

    auto setVariant = [&](CompareVariant var)
    {
        currentCfg.mainCfg.cmpConfig.compareVar = var;
        applyCompareConfig(true); //true: switchMiddleGrid
    };

    auto currentVar = getConfig().mainCfg.cmpConfig.compareVar;

    menu.addRadio(_("File time and size"), [&] { setVariant(CMP_BY_TIME_SIZE); }, currentVar == CMP_BY_TIME_SIZE);
    menu.addRadio(_("File content"      ), [&] { setVariant(CMP_BY_CONTENT);   }, currentVar == CMP_BY_CONTENT);

    menu.popup(*this);
}


void MainDialog::OnSyncSettingsContext(wxMouseEvent& event)
{
    ContextMenu menu;

    auto setVariant = [&](DirectionConfig::Variant var)
    {
        currentCfg.mainCfg.syncCfg.directionCfg.var = var;
        applySyncConfig();
    };

    const auto currentVar = getConfig().mainCfg.syncCfg.directionCfg.var;

    menu.addRadio(_("<- Two way ->"), [&] { setVariant(DirectionConfig::AUTOMATIC); }, currentVar == DirectionConfig::AUTOMATIC);
    menu.addRadio(_("Mirror ->>")   , [&] { setVariant(DirectionConfig::MIRROR);    }, currentVar == DirectionConfig::MIRROR);
    menu.addRadio(_("Update ->")    , [&] { setVariant(DirectionConfig::UPDATE);    }, currentVar == DirectionConfig::UPDATE);
    menu.addRadio(_("Custom")       , [&] { setVariant(DirectionConfig::CUSTOM);    }, currentVar == DirectionConfig::CUSTOM);

    menu.popup(*this);
}


void MainDialog::onNaviPanelFilesDropped(FileDropEvent& event)
{
    loadConfiguration(toZ(event.getFiles()));
    event.Skip();
}


void MainDialog::onDirSelected(wxCommandEvent& event)
{
    //left and right directory text-control and dirpicker are synchronized by MainFolderDragDrop automatically
    clearGrid(); //disable the sync button
    event.Skip();
}


void MainDialog::onDirManualCorrection(wxCommandEvent& event)
{
    updateUnsavedCfgStatus();
    event.Skip();
}


wxString getFormattedHistoryElement(const Zstring& filename)
{
    Zstring output = afterLast(filename, FILE_NAME_SEPARATOR);
    if (endsWith(output, Zstr(".ffs_gui")))
        output = beforeLast(output, Zstr('.'));
    return utfCvrtTo<wxString>(output);
}


void MainDialog::addFileToCfgHistory(const std::vector<Zstring>& filenames)
{
    //determine highest "last use" index number of m_listBoxHistory
    int lastUseIndexMax = 0;
    for (int i = 0; i < static_cast<int>(m_listBoxHistory->GetCount()); ++i)
        if (auto histData = dynamic_cast<const wxClientHistoryData*>(m_listBoxHistory->GetClientObject(i)))
            if (histData->lastUseIndex_ > lastUseIndexMax)
                lastUseIndexMax = histData->lastUseIndex_;

    std::deque<bool> selections(m_listBoxHistory->GetCount()); //items to select after update of history list

    for (auto it = filenames.begin(); it != filenames.end(); ++it)
    {
        const Zstring& filename = *it;

        //Do we need to additionally check for aliases of the same physical files here? (and aliases for lastRunConfigName?)

        const int itemPos = [&]() -> int
        {
            const int itemCount = static_cast<int>(m_listBoxHistory->GetCount());
            for (int i = 0; i < itemCount; ++i)
                if (auto histData = dynamic_cast<const wxClientHistoryData*>(m_listBoxHistory->GetClientObject(i)))
                    if (EqualFilename()(filename, histData->cfgFile_))
                        return i;
            return -1;
        }();

        if (itemPos >= 0) //update
        {
            if (auto histData = dynamic_cast<wxClientHistoryData*>(m_listBoxHistory->GetClientObject(itemPos)))
                histData->lastUseIndex_ = ++lastUseIndexMax;
            selections[itemPos] = true;
        }
        else //insert
        {
            const wxString label = EqualFilename()(filename, lastRunConfigName()) ? //give default config file a different name
                                   _("<Last session>") : getFormattedHistoryElement(filename);
            const int newPos = m_listBoxHistory->Append(label, new wxClientHistoryData(filename, ++lastUseIndexMax)); //*insert* into sorted list
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


void MainDialog::removeObsoleteCfgHistoryItems(const std::vector<Zstring>& filenames)
{
    //don't use wxString: NOT thread-safe! (e.g. non-atomic ref-count)

    auto getMissingFilesAsync = [filenames]() -> std::vector<Zstring>
    {
        //boost::this_thread::sleep(boost::posix_time::millisec(5000));

        //check existence of all config files in parallel!
        std::list<boost::unique_future<bool>> fileEx;

        for (auto it = filenames.begin(); it != filenames.end(); ++it) //avoid VC11 compiler issues with std::for_each
        {
            const Zstring filename = *it; //don't reference iterator in lambda!
            fileEx.push_back(zen::async2<bool>([=] { return fileExists(filename); }));
        }

        //potentially slow network access => limit maximum wait time!
        wait_for_all_timed(fileEx.begin(), fileEx.end(), boost::posix_time::milliseconds(1000));

        std::vector<Zstring> missingFiles;

        auto itFut = fileEx.begin();
        for (auto it = filenames.begin(); it != filenames.end(); ++it, ++itFut)
            if (itFut->is_ready() && !itFut->get()) //remove only files that are confirmed to be non-existent
                missingFiles.push_back(*it);

        return missingFiles;
    };

    processAsync(getMissingFilesAsync, [this](const std::vector<Zstring>& files) { removeCfgHistoryItems(files); });
}


void MainDialog::removeCfgHistoryItems(const std::vector<Zstring>& filenames)
{
    std::for_each(filenames.begin(), filenames.end(), [&](const Zstring& filename)
    {
        const int histSize = m_listBoxHistory->GetCount();
        for (int i = 0; i < histSize; ++i)
            if (auto histData = dynamic_cast<wxClientHistoryData*>(m_listBoxHistory->GetClientObject(i)))
                if (EqualFilename()(filename, histData->cfgFile_))
                {
                    m_listBoxHistory->Delete(i);
                    break;
                }
    });
}


void MainDialog::updateUnsavedCfgStatus()
{
    const Zstring activeCfgFilename = activeConfigFiles.size() == 1 && activeConfigFiles[0] != lastRunConfigName() ? activeConfigFiles[0] : Zstring();

    const bool haveUnsavedCfg = lastConfigurationSaved != getConfig();

    //update save config button
    const bool allowSave = haveUnsavedCfg ||
                           activeConfigFiles.size() > 1;

    auto makeBrightGrey = [](const wxBitmap& bmp) -> wxBitmap
    {
        wxImage img = bmp.ConvertToImage().ConvertToGreyscale(1.0/3, 1.0/3, 1.0/3); //treat all channels equally!
        brighten(img, 80);
        return img;
    };
    //setImage(*m_bpButtonSave, greyScale(getResourceImage(L"save")));

    setImage(*m_bpButtonSave, allowSave ? getResourceImage(L"save") : makeBrightGrey(getResourceImage(L"save")));
    m_bpButtonSave->Enable(allowSave);
    m_menuItemSave->Enable(allowSave); //bitmap is automatically greyscaled on Win7 (introducing a crappy looking shift), but not on XP

    //set main dialog title
    wxString title;
    if (haveUnsavedCfg)
        title += L'*';

    if (!activeCfgFilename.empty())
        title += toWx(activeCfgFilename);
    else if (activeConfigFiles.size() > 1)
    {
#ifdef _MSC_VER
#pragma warning(disable:4428)	// VC wrongly issues warning C4428: universal-character-name encountered in source
#endif
        const wchar_t* EM_DASH = L" \u2014 ";
        title += xmlAccess::extractJobName(activeConfigFiles[0]);
        std::for_each(activeConfigFiles.begin() + 1, activeConfigFiles.end(), [&](const Zstring& filename) { title += EM_DASH + xmlAccess::extractJobName(filename); });
    }
    else
        title += L"FreeFileSync - " + _("Folder Comparison and Synchronization");

    SetTitle(title);
}


void MainDialog::OnConfigSave(wxCommandEvent& event)
{
    const Zstring activeCfgFilename = activeConfigFiles.size() == 1 && activeConfigFiles[0] != lastRunConfigName() ? activeConfigFiles[0] : Zstring();

    //if we work on a single named configuration document: save directly if changed
    //else: always show file dialog
    if (!activeCfgFilename.empty())
    {
        using namespace xmlAccess;

        switch (getXmlType(activeCfgFilename)) //throw()
        {
            case XML_TYPE_GUI:
                trySaveConfig(&activeCfgFilename);
                return;
            case XML_TYPE_BATCH:
                trySaveBatchConfig(&activeCfgFilename);
                return;
            case XML_TYPE_GLOBAL:
            case XML_TYPE_OTHER:
                assert(false);
                return;
        }
    }

    trySaveConfig(nullptr);
}


void MainDialog::OnConfigSaveAs(wxCommandEvent& event)
{
    trySaveConfig(nullptr);
}


void MainDialog::OnSaveAsBatchJob(wxCommandEvent& event)
{
    trySaveBatchConfig(nullptr);
}


bool MainDialog::trySaveConfig(const Zstring* fileNameGui) //return true if saved successfully
{
    Zstring targetFilename;

    if (fileNameGui)
    {
        targetFilename = *fileNameGui;
        assert(endsWith(targetFilename, Zstr(".ffs_gui")));
    }
    else
    {
        wxString defaultFileName = activeConfigFiles.size() == 1 && activeConfigFiles[0] != lastRunConfigName() ? toWx(activeConfigFiles[0]) : L"SyncSettings.ffs_gui";
        //attention: activeConfigFiles may be an imported *.ffs_batch file! We don't want to overwrite it with a GUI config!
        if (endsWith(defaultFileName, L".ffs_batch"))
            replace(defaultFileName, L".ffs_batch", L".ffs_gui", false);

        wxFileDialog filePicker(this, //put modal dialog on stack: creating this on freestore leads to memleak!
                                wxEmptyString,
                                wxEmptyString,
                                defaultFileName,
                                wxString(L"FreeFileSync (*.ffs_gui)|*.ffs_gui") + L"|" +_("All files") + L" (*.*)|*",
                                wxFD_SAVE | wxFD_OVERWRITE_PROMPT);
        if (filePicker.ShowModal() != wxID_OK)
            return false;
        targetFilename = toZ(filePicker.GetPath());
    }

    const xmlAccess::XmlGuiConfig guiCfg = getConfig();

    try
    {
        xmlAccess::writeConfig(guiCfg, targetFilename); //throw FfsXmlError
        setLastUsedConfig(targetFilename, guiCfg);

        flashStatusInformation(_("Configuration saved!"));
        return true;
    }
    catch (const xmlAccess::FfsXmlError& e)
    {
        wxMessageBox(e.toString().c_str(), _("Error"), wxOK | wxICON_ERROR, this);
        return false;
    }
}


bool MainDialog::trySaveBatchConfig(const Zstring* fileNameBatch)
{
    //essentially behave like trySaveConfig(): the collateral damage of not saving GUI-only settings "hideExcludedItems, m_bpButtonViewTypeSyncAction" is negliable

    const xmlAccess::XmlGuiConfig guiCfg = getConfig();

    Zstring targetFilename;
    xmlAccess::XmlBatchConfig batchCfg;

    if (fileNameBatch)
    {
        targetFilename = *fileNameBatch;
        batchCfg = convertGuiToBatchPreservingExistingBatch(guiCfg, *fileNameBatch);
        assert(endsWith(targetFilename, Zstr(".ffs_batch")));
    }
    else
    {
        const Zstring activeCfgFilename = activeConfigFiles.size() == 1 && activeConfigFiles[0] != lastRunConfigName() ? activeConfigFiles[0] : Zstring();
        batchCfg = convertGuiToBatchPreservingExistingBatch(guiCfg, activeCfgFilename);

        //let user change batch config: this should change batch-exclusive settings only, else the "setLastUsedConfig" below would be somewhat of a lie
        if (!customizeBatchConfig(this,
                                  batchCfg, //in/out
                                  globalCfg.gui.onCompletionHistory,
                                  globalCfg.gui.onCompletionHistoryMax))
            return false;

        wxString defaultFileName = !activeCfgFilename.empty() ? toWx(activeCfgFilename) : L"BatchRun.ffs_batch";
        //attention: activeConfigFiles may be a *.ffs_gui file! We don't want to overwrite it with a BATCH config!
        if (endsWith(defaultFileName, L".ffs_gui"))
            replace(defaultFileName, L".ffs_gui", L".ffs_batch");

        wxFileDialog filePicker(this, //put modal dialog on stack: creating this on freestore leads to memleak!
                                wxEmptyString,
                                wxEmptyString,
                                defaultFileName,
                                _("FreeFileSync batch") + L" (*.ffs_batch)|*.ffs_batch" + L"|" +_("All files") + L" (*.*)|*",
                                wxFD_SAVE | wxFD_OVERWRITE_PROMPT);
        if (filePicker.ShowModal() != wxID_OK)
            return false;
        targetFilename = toZ(filePicker.GetPath());
    }

    try
    {
        xmlAccess::writeConfig(batchCfg, targetFilename); //throw FfsXmlError

        setLastUsedConfig(targetFilename, guiCfg); //[!] behave as if we had saved guiCfg
        flashStatusInformation(_("Configuration saved!"));
        return true;
    }
    catch (const xmlAccess::FfsXmlError& e)
    {
        wxMessageBox(e.toString().c_str(), _("Error"), wxOK | wxICON_ERROR, this);
        return false;
    }
}


bool MainDialog::saveOldConfig() //return false on user abort
{
    if (lastConfigurationSaved != getConfig())
    {
        const Zstring activeCfgFilename = activeConfigFiles.size() == 1 && activeConfigFiles[0] != lastRunConfigName() ? activeConfigFiles[0] : Zstring();

        //notify user about changed settings
        if (globalCfg.optDialogs.popupOnConfigChange)
            if (!activeCfgFilename.empty())
                //only if check is active and non-default config file loaded
            {
                bool neverSave = !globalCfg.optDialogs.popupOnConfigChange;

                switch (showQuestionDlg(this,
                                        ReturnQuestionDlg::BUTTON_YES | ReturnQuestionDlg::BUTTON_NO | ReturnQuestionDlg::BUTTON_CANCEL,
                                        replaceCpy(_("Do you want to save changes to %x?"), L"%x", fmtFileName(afterLast(activeCfgFilename, FILE_NAME_SEPARATOR))),
                                        QuestConfig().setCaption(toWx(activeCfgFilename)).
                                        setLabelYes(_("&Save")).
                                        setLabelNo(_("Do&n't save")).
                                        showCheckBox(neverSave, _("Never save changes"), ReturnQuestionDlg::BUTTON_YES)))
                {
                    case ReturnQuestionDlg::BUTTON_YES:

                        using namespace xmlAccess;
                        switch (getXmlType(activeCfgFilename)) //throw()
                        {
                            case XML_TYPE_GUI:
                                return trySaveConfig(&activeCfgFilename);
                            case XML_TYPE_BATCH:
                                return trySaveBatchConfig(&activeCfgFilename);
                            case XML_TYPE_GLOBAL:
                            case XML_TYPE_OTHER:
                                assert(false);
                                return false;
                        }

                    case ReturnQuestionDlg::BUTTON_NO:
                        globalCfg.optDialogs.popupOnConfigChange = !neverSave;
                        break;

                    case ReturnQuestionDlg::BUTTON_CANCEL:
                        return false;
                }
            }

        //discard current reference file(s), this ensures next app start will load <last session> instead of the original non-modified config selection
        setLastUsedConfig(std::vector<Zstring>(), lastConfigurationSaved);
        //this seems to make theoretical sense also: the job of this function is to make sure current (volatile) config and reference file name are in sync
        // => if user does not save cfg, it is not attached to a physical file names anymore!
    }
    return true;
}


void MainDialog::OnConfigLoad(wxCommandEvent& event)
{
    const Zstring activeCfgFilename = activeConfigFiles.size() == 1 && activeConfigFiles[0] != lastRunConfigName() ? activeConfigFiles[0] : Zstring();

    wxFileDialog filePicker(this,
                            wxEmptyString,
                            toWx(beforeLast(activeCfgFilename, FILE_NAME_SEPARATOR)), //set default dir: empty string if "activeConfigFiles" is empty or has no path separator
                            wxEmptyString,
                            wxString(L"FreeFileSync (*.ffs_gui;*.ffs_batch)|*.ffs_gui;*.ffs_batch") + L"|" +_("All files") + L" (*.*)|*",
                            wxFD_OPEN | wxFD_MULTIPLE);

    if (filePicker.ShowModal() == wxID_OK)
    {
        wxArrayString tmp;
        filePicker.GetPaths(tmp);
        std::vector<wxString> filenames(tmp.begin(), tmp.end());

        loadConfiguration(toZ(filenames));
    }
}


void MainDialog::OnConfigNew(wxCommandEvent& event)
{
    if (!saveOldConfig()) //notify user about changed settings
        return;

    xmlAccess::XmlGuiConfig newConfig;

    //add default exclusion filter: this is only ever relevant when creating new configurations!
    //a default XmlGuiConfig does not need these user-specific exclusions!
    Zstring& excludeFilter = newConfig.mainCfg.globalFilter.excludeFilter;
    if (!excludeFilter.empty() && !endsWith(excludeFilter, Zstr("\n")))
        excludeFilter += Zstr("\n");
    excludeFilter += globalCfg.gui.defaultExclusionFilter;

    setConfig(newConfig, std::vector<Zstring>());
}


void MainDialog::OnLoadFromHistory(wxCommandEvent& event)
{
    wxArrayInt selections;
    m_listBoxHistory->GetSelections(selections);

    std::vector<Zstring> filenames;
    std::for_each(selections.begin(), selections.end(),
                  [&](int pos)
    {
        if (auto histData = dynamic_cast<const wxClientHistoryData*>(m_listBoxHistory->GetClientObject(pos)))
            filenames.push_back(histData->cfgFile_);
    });

    if (!filenames.empty())
        loadConfiguration(filenames);

    //user changed m_listBoxHistory selection so it's this method's responsibility to synchronize with activeConfigFiles:
    //- if user cancelled saving old config
    //- there's an error loading new config
    //- filenames is empty and user tried to unselect the current config
    addFileToCfgHistory(activeConfigFiles);
}


void MainDialog::OnLoadFromHistoryDoubleClick(wxCommandEvent& event)
{
    wxArrayInt selections;
    m_listBoxHistory->GetSelections(selections);

    std::vector<Zstring> filenames;
    std::for_each(selections.begin(), selections.end(), [&](int pos)
    {
        if (auto histData = dynamic_cast<const wxClientHistoryData*>(m_listBoxHistory->GetClientObject(pos)))
            filenames.push_back(histData->cfgFile_);
    });

    if (!filenames.empty())
        if (loadConfiguration(filenames))
        {
            //simulate button click on "compare"
            wxCommandEvent dummy2(wxEVT_COMMAND_BUTTON_CLICKED);
            if (wxEvtHandler* evtHandler = m_buttonCompare->GetEventHandler())
                evtHandler->ProcessEvent(dummy2); //synchronous call
        }

    //synchronize m_listBoxHistory and activeConfigFiles, see OnLoadFromHistory()
    addFileToCfgHistory(activeConfigFiles);
}


bool MainDialog::loadConfiguration(const std::vector<Zstring>& filenames)
{
    if (filenames.empty())
        return true;

    if (!saveOldConfig())
        return false; //cancelled by user

    //load XML
    xmlAccess::XmlGuiConfig newGuiCfg;  //structure to receive gui settings, already defaulted!!
    try
    {
        //allow reading batch configurations also
        xmlAccess::readAnyConfig(filenames, newGuiCfg); //throw FfsXmlError

        setConfig(newGuiCfg, filenames);
        //flashStatusInformation(_("Configuration loaded!")); -> irrelevant!?
        return true;
    }
    catch (const xmlAccess::FfsXmlError& error)
    {
        if (error.getSeverity() == xmlAccess::FfsXmlError::WARNING)
        {
            wxMessageBox(error.toString(), _("Warning"), wxOK | wxICON_WARNING, this);
            setConfig(newGuiCfg, filenames);
            setLastUsedConfig(filenames, xmlAccess::XmlGuiConfig()); //simulate changed config due to parsing errors
        }
        else
            wxMessageBox(error.toString(), _("Error"), wxOK | wxICON_ERROR, this);
        return false;
    }
}

void MainDialog::deleteSelectedCfgHistoryItems()
{
    wxArrayInt tmp;
    m_listBoxHistory->GetSelections(tmp);

    std::set<int> selections(tmp.begin(), tmp.end()); //sort ascending!
    //delete starting with high positions:
    std::for_each(selections.rbegin(), selections.rend(), [&](int pos) { m_listBoxHistory->Delete(pos); });

    //set active selection on next element to allow "batch-deletion" by holding down DEL key
    if (!selections.empty() && m_listBoxHistory->GetCount() > 0)
    {
        int newSelection = *selections.begin();
        if (newSelection >= static_cast<int>(m_listBoxHistory->GetCount()))
            newSelection = m_listBoxHistory->GetCount() - 1;
        m_listBoxHistory->SetSelection(newSelection);
    }
}


void MainDialog::OnCfgHistoryRightClick(wxMouseEvent& event)
{
    ContextMenu menu;
    menu.addItem(_("Delete"), [this] { deleteSelectedCfgHistoryItems(); });
    menu.popup(*this);
}


void MainDialog::OnCfgHistoryKeyEvent(wxKeyEvent& event)
{
    const int keyCode = event.GetKeyCode();
    if (keyCode == WXK_DELETE || keyCode == WXK_NUMPAD_DELETE)
    {
        deleteSelectedCfgHistoryItems();
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
            //attention: this Veto() will NOT cancel system shutdown since saveOldConfig() blocks on modal dialog

            event.Veto();
            return;
        }
    }

    Destroy();
}


void MainDialog::onCheckRows(CheckRowsEvent& event)
{
    std::set<size_t> selectedRows;

    const size_t rowLast = std::min(event.rowLast_, gridDataView->rowsOnView()); //consider dummy rows
    for (size_t i = event.rowFirst_; i < rowLast; ++i)
        selectedRows.insert(i);

    if (!selectedRows.empty())
    {
        std::vector<FileSystemObject*> objects = gridDataView->getAllFileRef(selectedRows);
        setFilterManually(objects, event.setIncluded_);
    }
}


void MainDialog::onSetSyncDirection(SyncDirectionEvent& event)
{
    std::set<size_t> selectedRows;

    const size_t rowLast = std::min(event.rowLast_, gridDataView->rowsOnView()); //consider dummy rows
    for (size_t i = event.rowFirst_; i < rowLast; ++i)
        selectedRows.insert(i);

    if (!selectedRows.empty())
    {
        std::vector<FileSystemObject*> objects = gridDataView->getAllFileRef(selectedRows);
        setSyncDirManually(objects, event.direction_);
    }
}


void MainDialog::setLastUsedConfig(const Zstring& filename, const xmlAccess::XmlGuiConfig& guiConfig)
{
    std::vector<Zstring> filenames;
    filenames.push_back(filename);
    setLastUsedConfig(filenames, guiConfig);
}


void MainDialog::setLastUsedConfig(const std::vector<Zstring>& filenames,
                                   const xmlAccess::XmlGuiConfig& guiConfig)
{
    activeConfigFiles = filenames;
    lastConfigurationSaved = guiConfig;

    addFileToCfgHistory(activeConfigFiles); //put filename on list of last used config files

    updateUnsavedCfgStatus();
}


void MainDialog::setConfig(const xmlAccess::XmlGuiConfig& newGuiCfg, const std::vector<Zstring>& referenceFiles)
{
    currentCfg = newGuiCfg;

    //evaluate new settings...

    //(re-)set view filter buttons
    setViewFilterDefault();

    updateGlobalFilterButton();

    //set first folder pair
    firstFolderPair->setValues(currentCfg.mainCfg.firstPair.leftDirectory,
                               currentCfg.mainCfg.firstPair.rightDirectory,
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
    m_checkBoxHideExcluded->SetValue(currentCfg.hideExcludedItems);

    setViewTypeSyncAction(currentCfg.highlightSyncAction);

    //###########################################################
    //update compare variant name
    m_staticTextCmpVariant->SetLabel(currentCfg.mainCfg.getCompVariantName());

    //update sync variant name
    m_staticTextSyncVariant->SetLabel(currentCfg.mainCfg.getSyncVariantName());
    m_panelTopButtons->Layout(); //adapt layout for variant text

    clearGrid(); //+ update GUI

    setLastUsedConfig(referenceFiles, newGuiCfg);
}


inline
FolderPairEnh getEnhancedPair(const DirectoryPair* panel)
{
    return FolderPairEnh(panel->getLeftDir(),
                         panel->getRightDir(),
                         panel->getAltCompConfig(),
                         panel->getAltSyncConfig(),
                         panel->getAltFilterConfig());
}


xmlAccess::XmlGuiConfig MainDialog::getConfig() const
{
    xmlAccess::XmlGuiConfig guiCfg = currentCfg;

    //load settings whose ownership lies not in currentCfg:

    //first folder pair
    guiCfg.mainCfg.firstPair = FolderPairEnh(firstFolderPair->getLeftDir(),
                                             firstFolderPair->getRightDir(),
                                             firstFolderPair->getAltCompConfig(),
                                             firstFolderPair->getAltSyncConfig(),
                                             firstFolderPair->getAltFilterConfig());

    //add additional pairs
    guiCfg.mainCfg.additionalPairs.clear();
    std::transform(additionalFolderPairs.begin(), additionalFolderPairs.end(),
                   std::back_inserter(guiCfg.mainCfg.additionalPairs), getEnhancedPair);

    //sync preview
    guiCfg.highlightSyncAction = m_bpButtonViewTypeSyncAction->isActive();

    return guiCfg;
}


const Zstring& MainDialog::lastRunConfigName()
{
    static Zstring instance = zen::getConfigDir() + Zstr("LastRun.ffs_gui");
    return instance;
}


void MainDialog::updateGuiDelayedIf(bool condition)
{
    const int delay = 400;

    if (condition)
    {
        gridview::refresh(*m_gridMainL, *m_gridMainC, *m_gridMainR);
        m_gridMainL->Update();
        m_gridMainC->Update();
        m_gridMainR->Update();

        wxMilliSleep(delay); //some delay to show the changed GUI before removing rows from sight
    }

    updateGui();
}


void MainDialog::OnShowExcluded(wxCommandEvent& event)
{
    //toggle showing filtered rows
    currentCfg.hideExcludedItems = !currentCfg.hideExcludedItems;
    //make sure, checkbox and value are in sync
    m_checkBoxHideExcluded->SetValue(currentCfg.hideExcludedItems);

    updateGui();
}


void MainDialog::OnConfigureFilter(wxCommandEvent& event)
{
    if (showFilterDialog(this,
                         true, //is main filter dialog
                         currentCfg.mainCfg.globalFilter) == ReturnSmallDlg::BUTTON_OKAY)
    {
        updateGlobalFilterButton(); //refresh global filter icon
        applyFilterConfig();  //re-apply filter
    }

    //event.Skip()
}


void MainDialog::OnGlobalFilterContext(wxMouseEvent& event)
{
    auto clearFilter = [&]
    {
        currentCfg.mainCfg.globalFilter = FilterConfig();
        updateGlobalFilterButton(); //refresh global filter icon
        applyFilterConfig();  //re-apply filter
    };
    auto copyFilter  = [&] { filterCfgOnClipboard = make_unique<FilterConfig>(currentCfg.mainCfg.globalFilter); };
    auto pasteFilter = [&]
    {
        if (filterCfgOnClipboard)
        {
            currentCfg.mainCfg.globalFilter = *filterCfgOnClipboard;
            updateGlobalFilterButton(); //refresh global filter icon
            applyFilterConfig();  //re-apply filter
        }
    };

    ContextMenu menu;
    menu.addItem( _("Clear filter settings"), clearFilter, nullptr, !isNullFilter(currentCfg.mainCfg.globalFilter));
    menu.addSeparator();
    menu.addItem( _("Copy"),  copyFilter,  nullptr, !isNullFilter(currentCfg.mainCfg.globalFilter));
    menu.addItem( _("Paste"), pasteFilter, nullptr, filterCfgOnClipboard.get() != nullptr);
    menu.popup(*this);
}


void MainDialog::OnToggleViewType(wxCommandEvent& event)
{
    setViewTypeSyncAction(!m_bpButtonViewTypeSyncAction->isActive()); //toggle view
}


void MainDialog::OnToggleViewButton(wxCommandEvent& event)
{
    if (auto button = dynamic_cast<ToggleButton*>(event.GetEventObject()))
    {
        button->toggle();
        updateGui();
    }
    else
        assert(false);
}


inline
wxBitmap buttonPressed(const std::string& name)
{
    wxBitmap background = getResourceImage(L"buttonPressed");
    return mirrorIfRtl(
               layOver(getResourceImage(utfCvrtTo<wxString>(name)), background));
}


inline
wxBitmap buttonReleased(const std::string& name)
{
    wxImage output = getResourceImage(utfCvrtTo<wxString>(name)).ConvertToImage().ConvertToGreyscale(1.0/3, 1.0/3, 1.0/3); //treat all channels equally!
    zen::move(output, 0, -1); //move image right one pixel

    brighten(output, 80);
    return mirrorIfRtl(output);
}


void MainDialog::initViewFilterButtons()
{
    m_bpButtonViewTypeSyncAction->init(getResourceImage(L"viewTypeSyncAction"), getResourceImage(L"viewTypeCmpResult"));
    //tooltip is updated dynamically in setViewTypeSyncAction()

    auto initButton = [](ToggleButton& btn, const char* imgName, const wxString& tooltip) { btn.init(buttonPressed(imgName), buttonReleased(imgName)); btn.SetToolTip(tooltip); };

    //compare result buttons
    initButton(*m_bpButtonShowLeftOnly,   "leftOnly",   _("Show files that exist on left side only"));
    initButton(*m_bpButtonShowRightOnly,  "rightOnly",  _("Show files that exist on right side only"));
    initButton(*m_bpButtonShowLeftNewer,  "leftNewer",  _("Show files that are newer on left"));
    initButton(*m_bpButtonShowRightNewer, "rightNewer", _("Show files that are newer on right"));
    initButton(*m_bpButtonShowEqual,      "equal",      _("Show files that are equal"));
    initButton(*m_bpButtonShowDifferent,  "different",  _("Show files that are different"));
    initButton(*m_bpButtonShowConflict,   "conflict",   _("Show conflicts"));

    //sync preview buttons
    initButton(*m_bpButtonShowCreateLeft,  "createLeft",  _("Show files that will be created on the left side"));
    initButton(*m_bpButtonShowCreateRight, "createRight", _("Show files that will be created on the right side"));
    initButton(*m_bpButtonShowDeleteLeft,  "deleteLeft",  _("Show files that will be deleted on the left side"));
    initButton(*m_bpButtonShowDeleteRight, "deleteRight", _("Show files that will be deleted on the right side"));
    initButton(*m_bpButtonShowUpdateLeft,  "updateLeft",  _("Show files that will be overwritten on left side"));
    initButton(*m_bpButtonShowUpdateRight, "updateRight", _("Show files that will be overwritten on right side"));
    initButton(*m_bpButtonShowDoNothing,   "none",        _("Show files that won't be copied"));
}


void MainDialog::setViewFilterDefault()
{
    auto setButton = [](ToggleButton* tb, bool value) { tb->setActive(value); };

    const auto& def = globalCfg.gui.viewFilterDefault;
    setButton(m_bpButtonShowLeftOnly,	def.leftOnly);
    setButton(m_bpButtonShowRightOnly,	def.rightOnly);
    setButton(m_bpButtonShowLeftNewer,	def.leftNewer);
    setButton(m_bpButtonShowRightNewer,	def.rightNewer);
    setButton(m_bpButtonShowEqual,		def.equal);
    setButton(m_bpButtonShowDifferent,	def.different);
    setButton(m_bpButtonShowConflict,	def.conflict);

    setButton(m_bpButtonShowCreateLeft,	def.createLeft);
    setButton(m_bpButtonShowCreateRight,def.createRight);
    setButton(m_bpButtonShowUpdateLeft,	def.updateLeft);
    setButton(m_bpButtonShowUpdateRight,def.updateRight);
    setButton(m_bpButtonShowDeleteLeft,	def.deleteLeft);
    setButton(m_bpButtonShowDeleteRight,def.deleteRight);
    setButton(m_bpButtonShowDoNothing,	def.doNothing);
}


void MainDialog::OnViewButtonRightClick(wxMouseEvent& event)
{
    auto setButtonDefault = [](const ToggleButton* tb, bool& defaultValue)
    {
        if (tb->IsShown())
            defaultValue = tb->isActive();
    };

    auto setDefault = [&]
    {
        auto& def = globalCfg.gui.viewFilterDefault;
        setButtonDefault(m_bpButtonShowLeftOnly,   def.leftOnly);
        setButtonDefault(m_bpButtonShowRightOnly,  def.rightOnly);
        setButtonDefault(m_bpButtonShowLeftNewer,  def.leftNewer);
        setButtonDefault(m_bpButtonShowRightNewer, def.rightNewer);
        setButtonDefault(m_bpButtonShowEqual,      def.equal);
        setButtonDefault(m_bpButtonShowDifferent,  def.different);
        setButtonDefault(m_bpButtonShowConflict,   def.conflict);

        setButtonDefault(m_bpButtonShowCreateLeft,  def.createLeft);
        setButtonDefault(m_bpButtonShowCreateRight, def.createRight);
        setButtonDefault(m_bpButtonShowUpdateLeft,  def.updateLeft);
        setButtonDefault(m_bpButtonShowUpdateRight, def.updateRight);
        setButtonDefault(m_bpButtonShowDeleteLeft,  def.deleteLeft);
        setButtonDefault(m_bpButtonShowDeleteRight, def.deleteRight);
        setButtonDefault(m_bpButtonShowDoNothing,   def.doNothing);
    };

    ContextMenu menu;
    menu.addItem( _("Set as default"), setDefault);
    menu.popup(*this);
}


void MainDialog::updateGlobalFilterButton()
{
    //global filter: test for Null-filter
    if (!isNullFilter(currentCfg.mainCfg.globalFilter))
    {
        setImage(*m_bpButtonFilter, getResourceImage(L"filter"));
        m_bpButtonFilter->SetToolTip(_("Filter is active"));
    }
    else
    {
        setImage(*m_bpButtonFilter, greyScale(getResourceImage(L"filter")));
        m_bpButtonFilter->SetToolTip(_("No filter selected"));
    }
}


void MainDialog::OnCompare(wxCommandEvent& event)
{
    //PERF_START;

    wxBusyCursor dummy; //show hourglass cursor

    wxWindow* oldFocus = wxWindow::FindFocus();
    ZEN_ON_SCOPE_EXIT(if (oldFocus) oldFocus->SetFocus();); //e.g. keep focus on main grid after pressing F5

    int scrollPosX = 0;
    int scrollPosY = 0;
    m_gridMainL->GetViewStart(&scrollPosX, &scrollPosY); //preserve current scroll position
    ZEN_ON_SCOPE_EXIT(
        m_gridMainL->Scroll(scrollPosX, scrollPosY); //
        m_gridMainR->Scroll(scrollPosX, scrollPosY); //restore
        m_gridMainC->Scroll(-1, scrollPosY); )       //

    clearGrid(); //avoid memory peak by clearing old data first

    disableAllElements(true); //CompareStatusHandler will internally process Window messages, so avoid unexpected callbacks!
    ZEN_ON_SCOPE_EXIT(updateUiNow(); enableAllElements()); //ui update before enabling buttons again: prevent strange behaviour of delayed button clicks

    try
    {
        //class handling status display and error messages
        CompareStatusHandler statusHandler(*this);

        const std::vector<zen::FolderPairCfg> cmpConfig = zen::extractCompareCfg(getConfig().mainCfg);

        //GUI mode: place directory locks on directories isolated(!) during both comparison and synchronization
        std::unique_ptr<LockHolder> dirLocks;

        //COMPARE DIRECTORIES
        compare(globalCfg.fileTimeTolerance,
                globalCfg.optDialogs,
                true, //allow pw prompt
                globalCfg.runWithBackgroundPriority,
                globalCfg.createLockFile,
                dirLocks,
                cmpConfig,
                folderCmp,
                statusHandler); //throw GuiAbortProcess
    }
    catch (GuiAbortProcess&)
    {
        flashStatusInformation(_("Operation aborted!"));
        //        if (m_buttonCompare->IsShownOnScreen()) m_buttonCompare->SetFocus();
        updateGui(); //refresh grid in ANY case! (also on abort)
        return;
    }

    gridDataView->setData(folderCmp); //update view on data
    treeDataView->setData(folderCmp); //
    updateGui();

    //    if (m_buttonSync->IsShownOnScreen()) m_buttonSync->SetFocus();

    gridview::clearSelection(*m_gridMainL, *m_gridMainC, *m_gridMainR);
    m_gridNavi->clearSelection();

    //play (optional) sound notification after sync has completed (GUI and batch mode)
    //const Zstring soundFile = zen::getResourceDir() + Zstr("Compare_Complete.wav");
    //if (fileExists(soundFile))
    //  wxSound::Play(toWx(soundFile), wxSOUND_ASYNC);

    //add to folder history after successful comparison only
    folderHistoryLeft ->addItem(toZ(m_directoryLeft ->GetValue()));
    folderHistoryRight->addItem(toZ(m_directoryRight->GetValue()));

    //prepare status information
    if (allElementsEqual(folderCmp))
        flashStatusInformation(_("All folders are in sync!"));
}


void MainDialog::updateGui()
{
    updateGridViewData(); //update gridDataView and write status information

    //update sync preview statistics
    updateStatistics();

    updateUnsavedCfgStatus();

    //update sync and comparison variant names
    m_staticTextSyncVariant->SetLabel(getConfig().mainCfg.getSyncVariantName());
    m_staticTextCmpVariant ->SetLabel(getConfig().mainCfg.getCompVariantName());
    m_panelTopButtons->Layout();

    //update sync button enabled/disabled status
    if (!folderCmp.empty())
    {
        m_buttonSync->SetForegroundColour(wxSystemSettings::GetColour(wxSYS_COLOUR_BTNTEXT));
        m_buttonSync->setBitmapFront(getResourceImage(L"sync"), 5);
    }
    else
    {
        m_buttonSync->SetForegroundColour(wxColor(128, 128, 128)); //Some colors seem to have problems with 16-bit desktop color, well this one hasn't!
        m_buttonSync->setBitmapFront(greyScale(getResourceImage(L"sync")), 5);
    }

    auiMgr.Update(); //fix small display distortion, if view filter panel is empty
}


void MainDialog::clearGrid()
{
    folderCmp.clear();
    gridDataView->setData(folderCmp);
    treeDataView->setData(folderCmp);

    updateGui();
}


void MainDialog::updateStatistics()
{
    //update preview of item count and bytes to be transferred:
    const SyncStatistics st(folderCmp);

    setText(*m_staticTextData, filesizeToShortString(st.getDataToProcess()));
    if (st.getDataToProcess() == 0)
        m_bitmapData->SetBitmap(greyScale(getResourceImage(L"data")));
    else
        m_bitmapData->SetBitmap(getResourceImage(L"data"));

    auto setValue = [](wxStaticText& txtControl, int value, wxStaticBitmap& bmpControl, const wchar_t* bmpName)
    {
        setText(txtControl, toGuiString(value));

        if (value == 0)
            bmpControl.SetBitmap(greyScale(mirrorIfRtl(getResourceImage(bmpName))));
        else
            bmpControl.SetBitmap(mirrorIfRtl(getResourceImage(bmpName)));
    };

    setValue(*m_staticTextCreateLeft,  st.getCreate<LEFT_SIDE >(), *m_bitmapCreateLeft,  L"createLeftSmall");
    setValue(*m_staticTextUpdateLeft,  st.getUpdate<LEFT_SIDE >(), *m_bitmapUpdateLeft,  L"updateLeftSmall");
    setValue(*m_staticTextDeleteLeft,  st.getDelete<LEFT_SIDE >(), *m_bitmapDeleteLeft,  L"deleteLeftSmall");
    setValue(*m_staticTextCreateRight, st.getCreate<RIGHT_SIDE>(), *m_bitmapCreateRight, L"createRightSmall");
    setValue(*m_staticTextUpdateRight, st.getUpdate<RIGHT_SIDE>(), *m_bitmapUpdateRight, L"updateRightSmall");
    setValue(*m_staticTextDeleteRight, st.getDelete<RIGHT_SIDE>(), *m_bitmapDeleteRight, L"deleteRightSmall");

    m_panelStatistics->Layout();
    m_panelStatistics->Refresh(); //fix small mess up on RTL layout
}


void MainDialog::OnSyncSettings(wxCommandEvent& event)
{
    ExecWhenFinishedCfg ewfCfg = { &currentCfg.mainCfg.onCompletion,
                                   &globalCfg.gui.onCompletionHistory,
                                   globalCfg.gui.onCompletionHistoryMax
                                 };

    if (showSyncConfigDlg(this,
                          currentCfg.mainCfg.cmpConfig.compareVar,
                          currentCfg.mainCfg.syncCfg,
                          &currentCfg.handleError,
                          &ewfCfg) == ReturnSyncConfig::BUTTON_OKAY) //optional input parameter
    {
        applySyncConfig();
    }
}


void MainDialog::applyCompareConfig(bool switchMiddleGrid)
{
    clearGrid(); //+ GUI update

    //convenience: change sync view
    if (switchMiddleGrid)
        switch (currentCfg.mainCfg.cmpConfig.compareVar)
        {
            case CMP_BY_TIME_SIZE:
                setViewTypeSyncAction(true);
                break;
            case CMP_BY_CONTENT:
                setViewTypeSyncAction(false);
                break;
        }
}


void MainDialog::OnCmpSettings(wxCommandEvent& event)
{
    //show window right next to the compare-config button
    //wxPoint windowPos = m_bpButtonCmpConfig->GetScreenPosition();
    //windowPos.x += m_bpButtonCmpConfig->GetSize().GetWidth() + 5;

    CompConfig cmpConfigNew = currentCfg.mainCfg.cmpConfig;

    if (zen::showCompareCfgDialog(this, cmpConfigNew) == ReturnSmallDlg::BUTTON_OKAY &&
        //check if settings were changed at all
        cmpConfigNew != currentCfg.mainCfg.cmpConfig)
    {
        currentCfg.mainCfg.cmpConfig = cmpConfigNew;

        applyCompareConfig(true); //true: switchMiddleGrid
    }
}


void MainDialog::OnStartSync(wxCommandEvent& event)
{
    if (folderCmp.empty())
    {
        //quick sync: simulate button click on "compare"
        wxCommandEvent dummy2(wxEVT_COMMAND_BUTTON_CLICKED);
        if (wxEvtHandler* evtHandler = m_buttonCompare->GetEventHandler())
            evtHandler->ProcessEvent(dummy2); //synchronous call

        if (folderCmp.empty()) //check if user aborted or error occurred, ect...
            return;
    }

    //show sync preview/confirmation dialog
    if (globalCfg.optDialogs.confirmSyncStart)
    {
        bool dontShowAgain = false;

        if (zen::showSyncPreviewDlg(this,
                                    getConfig().mainCfg.getSyncVariantName(),
                                    zen::SyncStatistics(folderCmp),
                                    dontShowAgain) != ReturnSmallDlg::BUTTON_OKAY)
            return;

        globalCfg.optDialogs.confirmSyncStart = !dontShowAgain;
    }

    wxBusyCursor dummy; //show hourglass cursor

    try
    {
        //PERF_START;
        const Zstring activeCfgFilename = activeConfigFiles.size() == 1 && activeConfigFiles[0] != lastRunConfigName() ? activeConfigFiles[0] : Zstring();

        const auto& guiCfg = getConfig();

        disableAllElements(false); //SyncStatusHandler will internally process Window messages, so avoid unexpected callbacks!
        ZEN_ON_SCOPE_EXIT(enableAllElements());

        //class handling status updates and error messages
        SyncStatusHandler statusHandler(this, //throw GuiAbortProcess
                                        globalCfg.lastSyncsLogFileSizeMax,
                                        currentCfg.handleError,
                                        xmlAccess::extractJobName(activeCfgFilename),
                                        guiCfg.mainCfg.onCompletion,
                                        globalCfg.gui.onCompletionHistory);

        //GUI mode: place directory locks on directories isolated(!) during both comparison and synchronization
        std::unique_ptr<LockHolder> dirLocks;
        if (globalCfg.createLockFile)
        {
            std::set<Zstring, LessFilename> dirnamesExisting;
            for (auto it = begin(folderCmp); it != end(folderCmp); ++it)
            {
                if (it->isExisting<LEFT_SIDE>()) //do NOT check directory existence again!
                    dirnamesExisting.insert(it->getBaseDirPf<LEFT_SIDE >());
                if (it->isExisting<RIGHT_SIDE>())
                    dirnamesExisting.insert(it->getBaseDirPf<RIGHT_SIDE>());
            }
            dirLocks = make_unique<LockHolder>(dirnamesExisting, globalCfg.optDialogs.warningDirectoryLockFailed, statusHandler);
        }

        //START SYNCHRONIZATION
        const std::vector<zen::FolderPairSyncCfg> syncProcessCfg = zen::extractSyncCfg(guiCfg.mainCfg);
        if (syncProcessCfg.size() != folderCmp.size())
            throw std::logic_error("Programming Error: Contract violation! " + std::string(__FILE__) + ":" + numberTo<std::string>(__LINE__));
        //should never happen: sync button is deactivated if they are not in sync

        synchronize(localTime(),
                    globalCfg.optDialogs,
                    globalCfg.verifyFileCopy,
                    globalCfg.copyLockedFiles,
                    globalCfg.copyFilePermissions,
                    globalCfg.transactionalFileCopy,
                    globalCfg.runWithBackgroundPriority,
                    syncProcessCfg,
                    folderCmp,
                    statusHandler);
    }
    catch (GuiAbortProcess&)
    {
        //do NOT disable the sync button: user might want to try to sync the REMAINING rows
    }   //enableSynchronization(false);

    //remove empty rows: just a beautification, invalid rows shouldn't cause issues
    gridDataView->removeInvalidRows();

    updateGui();
}


void MainDialog::onGridDoubleClickL(GridClickEvent& event)
{
    onGridDoubleClickRim(event.row_, true);
}
void MainDialog::onGridDoubleClickR(GridClickEvent& event)
{
    onGridDoubleClickRim(event.row_, false);
}

void MainDialog::onGridDoubleClickRim(size_t row, bool leftSide)
{
    if (!globalCfg.gui.externelApplications.empty())
    {
        std::vector<FileSystemObject*> selection;
        if (FileSystemObject* fsObj = gridDataView->getObject(row)) //selection must be a list of BOUND pointers!
            selection.push_back(fsObj);

        openExternalApplication(globalCfg.gui.externelApplications[0].second, selection, leftSide);
    }
}


void MainDialog::onGridLabelLeftClick(bool onLeft, ColumnTypeRim type)
{
    auto sortInfo = gridDataView->getSortInfo();

    bool sortAscending = GridView::getDefaultSortDirection(type);
    if (sortInfo && sortInfo->onLeft_ == onLeft && sortInfo->type_ == type)
        sortAscending = !sortInfo->ascending_;

    gridDataView->sortView(type, onLeft, sortAscending);

    gridview::clearSelection(*m_gridMainL, *m_gridMainC, *m_gridMainR);
    updateGui(); //refresh gridDataView
}

void MainDialog::onGridLabelLeftClickL(GridClickEvent& event)
{
    onGridLabelLeftClick(true, static_cast<ColumnTypeRim>(event.colType_));
}
void MainDialog::onGridLabelLeftClickR(GridClickEvent& event)
{
    onGridLabelLeftClick(false, static_cast<ColumnTypeRim>(event.colType_));
}


void MainDialog::onGridLabelLeftClickC(GridClickEvent& event)
{
    //sorting middle grid is more or less useless: therefore let's toggle view instead!
    setViewTypeSyncAction(!m_bpButtonViewTypeSyncAction->isActive()); //toggle view
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
    for (auto it = additionalFolderPairs.begin(); it != additionalFolderPairs.end(); ++it)
    {
        DirectoryPair* dirPair = *it;
        dirPair->setValues(dirPair->getRightDir(), // swap directories
                           dirPair->getLeftDir(),  //
                           dirPair->getAltCompConfig(),
                           dirPair->getAltSyncConfig(),
                           dirPair->getAltFilterConfig());
    }

    //swap view filter
    bool tmp = m_bpButtonShowLeftOnly->isActive();
    m_bpButtonShowLeftOnly->setActive(m_bpButtonShowRightOnly->isActive());
    m_bpButtonShowRightOnly->setActive(tmp);

    tmp = m_bpButtonShowLeftNewer->isActive();
    m_bpButtonShowLeftNewer->setActive(m_bpButtonShowRightNewer->isActive());
    m_bpButtonShowRightNewer->setActive(tmp);

    /* for sync preview and "mirror" variant swapping may create strange effect:
    tmp = m_bpButtonShowCreateLeft->isActive();
    m_bpButtonShowCreateLeft->setActive(m_bpButtonShowCreateRight->isActive());
    m_bpButtonShowCreateRight->setActive(tmp);

    tmp = m_bpButtonShowDeleteLeft->isActive();
    m_bpButtonShowDeleteLeft->setActive(m_bpButtonShowDeleteRight->isActive());
    m_bpButtonShowDeleteRight->setActive(tmp);

    tmp = m_bpButtonShowUpdateLeft->isActive();
    m_bpButtonShowUpdateLeft->setActive(m_bpButtonShowUpdateRight->isActive());
    m_bpButtonShowUpdateRight->setActive(tmp);
    */

    //swap grid information
    zen::swapGrids(getConfig().mainCfg, folderCmp);

    updateGui();
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
    m_bpButtonShowLeftOnly  ->Show(false);
    m_bpButtonShowRightOnly ->Show(false);
    m_bpButtonShowLeftNewer ->Show(false);
    m_bpButtonShowRightNewer->Show(false);
    m_bpButtonShowDifferent ->Show(false);
    m_bpButtonShowEqual     ->Show(false);
    m_bpButtonShowConflict  ->Show(false);

    m_bpButtonShowCreateLeft ->Show(false);
    m_bpButtonShowCreateRight->Show(false);
    m_bpButtonShowDeleteLeft ->Show(false);
    m_bpButtonShowDeleteRight->Show(false);
    m_bpButtonShowUpdateLeft ->Show(false);
    m_bpButtonShowUpdateRight->Show(false);
    m_bpButtonShowDoNothing  ->Show(false);

    if (m_bpButtonViewTypeSyncAction->isActive())
    {
        const GridView::StatusSyncPreview result = gridDataView->updateSyncPreview(currentCfg.hideExcludedItems,
                                                                                   m_bpButtonShowCreateLeft ->isActive(),
                                                                                   m_bpButtonShowCreateRight->isActive(),
                                                                                   m_bpButtonShowDeleteLeft ->isActive(),
                                                                                   m_bpButtonShowDeleteRight->isActive(),
                                                                                   m_bpButtonShowUpdateLeft ->isActive(),
                                                                                   m_bpButtonShowUpdateRight->isActive(),
                                                                                   m_bpButtonShowDoNothing  ->isActive(),
                                                                                   m_bpButtonShowEqual            ->isActive(),
                                                                                   m_bpButtonShowConflict         ->isActive());
        filesOnLeftView    = result.filesOnLeftView;
        foldersOnLeftView  = result.foldersOnLeftView;
        filesOnRightView   = result.filesOnRightView;
        foldersOnRightView = result.foldersOnRightView;
        filesizeLeftView   = result.filesizeLeftView;
        filesizeRightView  = result.filesizeRightView;


        //sync preview buttons
        m_bpButtonShowCreateLeft  ->Show(result.existsSyncCreateLeft);
        m_bpButtonShowCreateRight ->Show(result.existsSyncCreateRight);
        m_bpButtonShowDeleteLeft  ->Show(result.existsSyncDeleteLeft);
        m_bpButtonShowDeleteRight ->Show(result.existsSyncDeleteRight);
        m_bpButtonShowUpdateLeft  ->Show(result.existsSyncDirLeft);
        m_bpButtonShowUpdateRight ->Show(result.existsSyncDirRight);
        m_bpButtonShowDoNothing   ->Show(result.existsSyncDirNone);
        m_bpButtonShowEqual       ->Show(result.existsSyncEqual);
        m_bpButtonShowConflict    ->Show(result.existsConflict);

        const bool anyViewFilterButtonShown = m_bpButtonShowCreateLeft ->IsShown() ||
                                              m_bpButtonShowCreateRight->IsShown() ||
                                              m_bpButtonShowDeleteLeft ->IsShown() ||
                                              m_bpButtonShowDeleteRight->IsShown() ||
                                              m_bpButtonShowUpdateLeft ->IsShown() ||
                                              m_bpButtonShowUpdateRight->IsShown() ||
                                              m_bpButtonShowDoNothing  ->IsShown() ||
                                              m_bpButtonShowEqual      ->IsShown() ||
                                              m_bpButtonShowConflict   ->IsShown();
        m_bpButtonViewTypeSyncAction->Show(anyViewFilterButtonShown);

        if (anyViewFilterButtonShown)
        {
            m_panelViewFilter->Show();
            m_panelViewFilter->Layout();
        }
        else
            m_panelViewFilter->Hide();
    }
    else
    {
        const GridView::StatusCmpResult result = gridDataView->updateCmpResult(currentCfg.hideExcludedItems,
                                                                               m_bpButtonShowLeftOnly  ->isActive(),
                                                                               m_bpButtonShowRightOnly ->isActive(),
                                                                               m_bpButtonShowLeftNewer ->isActive(),
                                                                               m_bpButtonShowRightNewer->isActive(),
                                                                               m_bpButtonShowDifferent ->isActive(),
                                                                               m_bpButtonShowEqual     ->isActive(),
                                                                               m_bpButtonShowConflict  ->isActive());
        filesOnLeftView    = result.filesOnLeftView;
        foldersOnLeftView  = result.foldersOnLeftView;
        filesOnRightView   = result.filesOnRightView;
        foldersOnRightView = result.foldersOnRightView;
        filesizeLeftView   = result.filesizeLeftView;
        filesizeRightView  = result.filesizeRightView;

        //comparison result view buttons
        m_bpButtonShowLeftOnly  ->Show(result.existsLeftOnly);
        m_bpButtonShowRightOnly ->Show(result.existsRightOnly);
        m_bpButtonShowLeftNewer ->Show(result.existsLeftNewer);
        m_bpButtonShowRightNewer->Show(result.existsRightNewer);
        m_bpButtonShowDifferent ->Show(result.existsDifferent);
        m_bpButtonShowEqual     ->Show(result.existsEqual);
        m_bpButtonShowConflict  ->Show(result.existsConflict);

        const bool anyViewFilterButtonShown = m_bpButtonShowLeftOnly  ->IsShown() ||
                                              m_bpButtonShowRightOnly ->IsShown() ||
                                              m_bpButtonShowLeftNewer ->IsShown() ||
                                              m_bpButtonShowRightNewer->IsShown() ||
                                              m_bpButtonShowDifferent ->IsShown() ||
                                              m_bpButtonShowEqual     ->IsShown() ||
                                              m_bpButtonShowConflict  ->IsShown();
        m_bpButtonViewTypeSyncAction->Show(anyViewFilterButtonShown);

        if (anyViewFilterButtonShown)
        {
            m_panelViewFilter->Show();
            m_panelViewFilter->Layout();
        }
        else
            m_panelViewFilter->Hide();
    }
    //all three grids retrieve their data directly via gridDataView
    gridview::refresh(*m_gridMainL, *m_gridMainC, *m_gridMainR);

    //navigation tree
    if (m_bpButtonViewTypeSyncAction->isActive())
        treeDataView->updateSyncPreview(currentCfg.hideExcludedItems,
                                        m_bpButtonShowCreateLeft ->isActive(),
                                        m_bpButtonShowCreateRight->isActive(),
                                        m_bpButtonShowDeleteLeft ->isActive(),
                                        m_bpButtonShowDeleteRight->isActive(),
                                        m_bpButtonShowUpdateLeft ->isActive(),
                                        m_bpButtonShowUpdateRight->isActive(),
                                        m_bpButtonShowDoNothing  ->isActive(),
                                        m_bpButtonShowEqual      ->isActive(),
                                        m_bpButtonShowConflict   ->isActive());
    else
        treeDataView->updateCmpResult(currentCfg.hideExcludedItems,
                                      m_bpButtonShowLeftOnly  ->isActive(),
                                      m_bpButtonShowRightOnly ->isActive(),
                                      m_bpButtonShowLeftNewer ->isActive(),
                                      m_bpButtonShowRightNewer->isActive(),
                                      m_bpButtonShowDifferent ->isActive(),
                                      m_bpButtonShowEqual     ->isActive(),
                                      m_bpButtonShowConflict  ->isActive());
    m_gridNavi->Refresh();

    //update status bar information
    setStatusBarFileStatistics(filesOnLeftView,
                               foldersOnLeftView,
                               filesOnRightView,
                               foldersOnRightView,
                               filesizeLeftView,
                               filesizeRightView);
}


void MainDialog::applyFilterConfig()
{
    applyFiltering(folderCmp, getConfig().mainCfg);

    updateGui();
    //updateGuiDelayedIf(currentCfg.hideExcludedItems); //show update GUI before removing rows
}


void MainDialog::applySyncConfig()
{
    zen::redetermineSyncDirection(getConfig().mainCfg, folderCmp,
                                  [&](const std::wstring& warning)
    {
        bool& warningActive = globalCfg.optDialogs.warningDatabaseError;
        if (warningActive)
        {
            bool dontWarnAgain = false;
            if (showWarningDlg(this,
                               ReturnWarningDlg::BUTTON_IGNORE, warning, dontWarnAgain) == ReturnWarningDlg::BUTTON_IGNORE)
                warningActive = !dontWarnAgain;
        }
    });

    updateGui();
}


void MainDialog::OnAddFolderPair(wxCommandEvent& event)
{
    wxWindowUpdateLocker dummy(this); //avoid display distortion

    std::vector<FolderPairEnh> newPairs;
    newPairs.push_back(getConfig().mainCfg.firstPair);

    //clear first pair
    const FolderPairEnh cfgEmpty;
    firstFolderPair->setValues(cfgEmpty.leftDirectory,
                               cfgEmpty.rightDirectory,
                               cfgEmpty.altCmpConfig,
                               cfgEmpty.altSyncConfig,
                               cfgEmpty.localFilter);

    //keep sequence to update GUI as last step
    addFolderPair(newPairs, true); //add pair in front of additonal pairs
}


void MainDialog::OnRemoveTopFolderPair(wxCommandEvent& event)
{
    if (!additionalFolderPairs.empty())
    {
        wxWindowUpdateLocker dummy(this); //avoid display distortion

        //get settings from second folder pair
        const FolderPairEnh cfgSecond = getEnhancedPair(additionalFolderPairs[0]);

        //reset first pair
        firstFolderPair->setValues(cfgSecond.leftDirectory,
                                   cfgSecond.rightDirectory,
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
    for (auto it = additionalFolderPairs.begin(); it != additionalFolderPairs.end(); ++it)
        if (eventObj == (*it)->m_bpButtonRemovePair)
        {
            removeAddFolderPair(it - additionalFolderPairs.begin());
            break;
        }
}


void MainDialog::updateGuiForFolderPair()
{
    wxWindowUpdateLocker dummy(this);

    //adapt delete top folder pair button
    m_bpButtonRemovePair->Show(!additionalFolderPairs.empty());
    m_panelTopLeft->Layout();

    //adapt local filter and sync cfg for first folder pair
    const bool showLocalCfgFirstPair = !additionalFolderPairs.empty()                       ||
                                       firstFolderPair->getAltCompConfig().get() != nullptr ||
                                       firstFolderPair->getAltSyncConfig().get() != nullptr ||
                                       !isNullFilter(firstFolderPair->getAltFilterConfig());

    m_bpButtonAltCompCfg ->Show(showLocalCfgFirstPair);
    m_bpButtonAltSyncCfg ->Show(showLocalCfgFirstPair);
    m_bpButtonLocalFilter->Show(showLocalCfgFirstPair);
    setImage(*m_bpButtonSwapSides, getResourceImage(showLocalCfgFirstPair ? L"swapSlim" : L"swap"));
    m_panelTopMiddle->Layout();      //both required to update button size for calculations below!!!
    m_panelDirectoryPairs->Layout(); //   -> updates size of stretched m_panelTopLeft!

    int addPairMinimalHeight = 0;
    int addPairOptimalHeight = 0;
    if (!additionalFolderPairs.empty())
    {
        const int pairHeight = additionalFolderPairs[0]->GetSize().GetHeight();
        addPairMinimalHeight = std::min<double>(1.5, additionalFolderPairs.size()) * pairHeight; //have 1.5 * height indicate that more folders are there
        addPairOptimalHeight = std::min<double>(globalCfg.gui.maxFolderPairsVisible - 1 + 0.5, //subtract first/main folder pair and add 0.5 to indicate additional folders
                                                additionalFolderPairs.size()) * pairHeight;

        addPairOptimalHeight = std::max(addPairOptimalHeight, addPairMinimalHeight); //implicitly handle corrupted values for "maxFolderPairsVisible"
    }

    const int firstPairHeight = std::max(m_panelDirectoryPairs->ClientToWindowSize(m_panelTopLeft  ->GetSize()).GetHeight(),  //include m_panelDirectoryPairs window borders!
                                         m_panelDirectoryPairs->ClientToWindowSize(m_panelTopMiddle->GetSize()).GetHeight()); //

    //########################################################################################################################
    //wxAUI hack: set minimum height to desired value, then call wxAuiPaneInfo::Fixed() to apply it
    auiMgr.GetPane(m_panelDirectoryPairs).MinSize(-1, firstPairHeight + addPairOptimalHeight);
    auiMgr.GetPane(m_panelDirectoryPairs).Fixed();
    auiMgr.Update();

    //now make resizable again
    auiMgr.GetPane(m_panelDirectoryPairs).Resizable();
    auiMgr.Update();
    //########################################################################################################################

    //make sure user cannot fully shrink additional folder pairs
    auiMgr.GetPane(m_panelDirectoryPairs).MinSize(-1, firstPairHeight + addPairMinimalHeight);
    auiMgr.Update();

    m_scrolledWindowFolderPairs->Fit();    //adjust scrolled window size

    //m_scrolledWindowFolderPairs->Layout(); //adjust stuff inside scrolled window
    m_panelDirectoryPairs->Layout();
}


void MainDialog::addFolderPair(const std::vector<FolderPairEnh>& newPairs, bool addFront)
{
    wxWindowUpdateLocker dummy(m_panelDirectoryPairs); //avoid display distortion
    //wxWindowUpdateLocker dummy2(m_panelGrids);         //

    std::vector<DirectoryPair*> newEntries;

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
            bSizerAddFolderPairs->Insert(0, newPair, 0, wxEXPAND);
            additionalFolderPairs.insert(additionalFolderPairs.begin(), newPair);
        }
        else
        {
            bSizerAddFolderPairs->Add(newPair, 0, wxEXPAND);
            additionalFolderPairs.push_back(newPair);
        }
        newEntries.push_back(newPair);

        //register events
        newPair->m_bpButtonRemovePair->Connect(wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler(MainDialog::OnRemoveFolderPair), nullptr, this);
    });

    updateGuiForFolderPair();

    //wxComboBox screws up miserably if width/height is smaller than the magic number 4! Problem occurs when trying to set tooltip
    //so we have to update window sizes before setting configuration:
    for (auto it = newPairs.begin(); it != newPairs.end(); ++it)//set alternate configuration
        newEntries[it - newPairs.begin()]->setValues(it->leftDirectory,
                                                     it->rightDirectory,
                                                     it->altCmpConfig,
                                                     it->altSyncConfig,
                                                     it->localFilter);
    clearGrid(); //+ GUI update
}


void MainDialog::removeAddFolderPair(size_t pos)
{
    wxWindowUpdateLocker dummy(m_panelDirectoryPairs); //avoid display distortion
    //wxWindowUpdateLocker dummy2(m_panelGrids);         //

    if (pos < additionalFolderPairs.size())
    {
        //remove folder pairs from window
        DirectoryPair* pairToDelete = additionalFolderPairs[pos];
        //const int pairHeight = pairToDelete->GetSize().GetHeight();

        bSizerAddFolderPairs->Detach(pairToDelete); //Remove() does not work on Window*, so do it manually
        additionalFolderPairs.erase(additionalFolderPairs.begin() + pos); //remove element from vector
        //more (non-portable) wxWidgets bullshit: on OS X wxWindow::Destroy() screws up and calls "operator delete" directly rather than
        //the deferred deletion it is expected to do (and which is implemented correctly on Windows and Linux)
        //http://bb10.com/python-wxpython-devel/2012-09/msg00004.html
        //=> since we're in a mouse button callback of a sub-component of "pairToDelete" we need to delay deletion ourselves:
        processAsync2([] {}, [pairToDelete] { pairToDelete->Destroy(); });

        //set size of scrolled window
        //const size_t additionalRows = additionalFolderPairs.size();
        //if (additionalRows <= globalCfg.gui.addFolderPairCountMax) //up to "addFolderPairCountMax" additional pairs shall be shown
        //  m_scrolledWindowFolderPairs->SetMinSize(wxSize(-1, pairHeight * static_cast<int>(additionalRows)));

        //update controls
        //m_scrolledWindowFolderPairs->Fit();    //adjust scrolled window size
        //m_scrolledWindowFolderPairs->Layout(); //adjust stuff inside scrolled window
        //bSizer1->Layout();
    }

    updateGuiForFolderPair();

    clearGrid(); //+ GUI update
}


void MainDialog::clearAddFolderPairs()
{
    wxWindowUpdateLocker dummy(m_panelDirectoryPairs); //avoid display distortion
    //wxWindowUpdateLocker dummy2(m_panelGrids);         //

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
    zen::showGlobalSettingsDlg(this, globalCfg);
}


void MainDialog::OnMenuExportFileList(wxCommandEvent& event)
{
    //get a filename
    const wxString defaultFileName = L"FileList.csv"; //proposal
    wxFileDialog filePicker(this, //creating this on freestore leads to memleak!
                            wxEmptyString,
                            wxEmptyString,
                            defaultFileName,
                            _("Comma separated list") + L" (*.csv)|*.csv" + L"|" +_("All files") + L" (*.*)|*",
                            wxFD_SAVE | wxFD_OVERWRITE_PROMPT);
    if (filePicker.ShowModal() != wxID_OK)
        return;

    const Zstring filename = utfCvrtTo<Zstring>(filePicker.GetPath());

    //http://en.wikipedia.org/wiki/Comma-separated_values
    const lconv* localInfo = ::localeconv(); //always bound according to doc
    const bool haveCommaAsDecimalSep = std::string(localInfo->decimal_point) == ",";

    const char CSV_SEP = haveCommaAsDecimalSep ? ';' : ',';

    auto fmtValue = [&](const wxString& val) -> Utf8String
    {
        Utf8String&& tmp = utfCvrtTo<Utf8String>(val);

        if (contains(tmp, CSV_SEP))
            return '\"' + tmp + '\"';
        else
            return tmp;
    };

    Utf8String header; //perf: wxString doesn't model exponential growth and so is out, std::string doesn't give performance guarantee!
    header += BYTE_ORDER_MARK_UTF8;

    //base folders
    header += fmtValue(_("Folder pairs")) + '\n' ;
    std::for_each(begin(folderCmp), end(folderCmp),
                  [&](BaseDirMapping& baseMap)
    {
        header += utfCvrtTo<Utf8String>(baseMap.getBaseDirPf<LEFT_SIDE >()) + CSV_SEP;
        header += utfCvrtTo<Utf8String>(baseMap.getBaseDirPf<RIGHT_SIDE>()) + '\n';
    });
    header += '\n';

    //write header
    auto provLeft   = m_gridMainL->getDataProvider();
    auto provMiddle = m_gridMainC->getDataProvider();
    auto provRight  = m_gridMainR->getDataProvider();

    auto colAttrLeft   = m_gridMainL->getColumnConfig();
    auto colAttrMiddle = m_gridMainC->getColumnConfig();
    auto colAttrRight  = m_gridMainR->getColumnConfig();

    vector_remove_if(colAttrLeft  , [](const Grid::ColumnAttribute& ca) { return !ca.visible_; });
    vector_remove_if(colAttrMiddle, [](const Grid::ColumnAttribute& ca) { return !ca.visible_ || static_cast<ColumnTypeMiddle>(ca.type_) == COL_TYPE_CHECKBOX; });
    vector_remove_if(colAttrRight , [](const Grid::ColumnAttribute& ca) { return !ca.visible_; });

    if (provLeft && provMiddle && provRight)
    {
        std::for_each(colAttrLeft.begin(), colAttrLeft.end(),
                      [&](const Grid::ColumnAttribute& ca)
        {
            header += fmtValue(provLeft->getColumnLabel(ca.type_));
            header += CSV_SEP;
        });
        std::for_each(colAttrMiddle.begin(), colAttrMiddle.end(),
                      [&](const Grid::ColumnAttribute& ca)
        {
            header += fmtValue(provMiddle->getColumnLabel(ca.type_));
            header += CSV_SEP;
        });
        if (!colAttrRight.empty())
        {
            std::for_each(colAttrRight.begin(), colAttrRight.end() - 1,
                          [&](const Grid::ColumnAttribute& ca)
            {
                header += fmtValue(provRight->getColumnLabel(ca.type_));
                header += CSV_SEP;
            });
            header += fmtValue(provRight->getColumnLabel(colAttrRight.back().type_));
        }
        header += '\n';

        try
        {
            //write file
            FileOutput fileOut(filename, zen::FileOutput::ACC_OVERWRITE); //throw FileError

            replace(header, '\n', LINE_BREAK);
            fileOut.write(&*header.begin(), header.size()); //throw FileError

            //main grid: write rows one after the other instead of creating one big string: memory allocation might fail; think 1 million rows!
            /*
            performance test case "export 600.000 rows" to CSV:
            aproach 1. assemble single temporary string, then write file:	4.6s
            aproach 2. write to buffered file output directly for each row: 6.4s
            */
            const size_t rowCount = m_gridMainL->getRowCount();
            for (size_t row = 0; row < rowCount; ++row)
            {
                Utf8String tmp;

                std::for_each(colAttrLeft.begin(), colAttrLeft.end(),
                              [&](const Grid::ColumnAttribute& ca)
                {
                    tmp += fmtValue(provLeft->getValue(row, ca.type_));
                    tmp += CSV_SEP;
                });
                std::for_each(colAttrMiddle.begin(), colAttrMiddle.end(),
                              [&](const Grid::ColumnAttribute& ca)
                {
                    tmp += fmtValue(provMiddle->getValue(row, ca.type_));
                    tmp += CSV_SEP;
                });
                std::for_each(colAttrRight.begin(), colAttrRight.end(),
                              [&](const Grid::ColumnAttribute& ca)
                {
                    tmp += fmtValue(provRight->getValue(row, ca.type_));
                    tmp += CSV_SEP;
                });
                tmp += '\n';

                replace(tmp, '\n', LINE_BREAK);
                fileOut.write(&*tmp.begin(), tmp.size()); //throw FileError
            }

            flashStatusInformation(_("File list exported!"));
        }
        catch (const FileError& e)
        {
            wxMessageBox(e.toString(), _("Error"), wxOK | wxICON_ERROR, this);
        }
    }
}


void MainDialog::OnMenuCheckVersion(wxCommandEvent& event)
{
    zen::checkForUpdateNow(this);
}


void MainDialog::OnMenuCheckVersionAutomatically(wxCommandEvent& event)
{
    globalCfg.gui.lastUpdateCheck = globalCfg.gui.lastUpdateCheck == -1 ? 0 : -1;
    m_menuItemCheckVersionAuto->Check(globalCfg.gui.lastUpdateCheck != -1);
    zen::checkForUpdatePeriodically(this, globalCfg.gui.lastUpdateCheck, [&] { flashStatusInformation(_("Searching for program updates...")); });
}


void MainDialog::OnRegularUpdateCheck(wxIdleEvent& event)
{
    //execute just once per startup!
    Disconnect(wxEVT_IDLE, wxIdleEventHandler(MainDialog::OnRegularUpdateCheck), nullptr, this);

    if (manualProgramUpdateRequired())
        zen::checkForUpdatePeriodically(this, globalCfg.gui.lastUpdateCheck, [&] { flashStatusInformation(_("Searching for program updates...")); });
}


void MainDialog::OnLayoutWindowAsync(wxIdleEvent& event)
{
    //execute just once per startup!
    Disconnect(wxEVT_IDLE, wxIdleEventHandler(MainDialog::OnLayoutWindowAsync), nullptr, this);

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
    zen::showAboutDialog(this);
}


void MainDialog::OnShowHelp(wxCommandEvent& event)
{
    zen::displayHelpEntry(this);
}

//#########################################################################################################

//language selection
void MainDialog::switchProgramLanguage(int langID)
{
    //create new dialog with respect to new language
    xmlAccess::XmlGlobalSettings newGlobalCfg = getGlobalCfgBeforeExit();
    newGlobalCfg.programLanguage = langID;

    //show new dialog, then delete old one
    MainDialog::create(getConfig(), activeConfigFiles, newGlobalCfg, false);

    //we don't use Close():
    //1. we don't want to show the prompt to save current config in OnClose()
    //2. after getGlobalCfgBeforeExit() the old main dialog is invalid so we want to force deletion
    Destroy();
}


void MainDialog::OnMenuLanguageSwitch(wxCommandEvent& event)
{
    std::map<MenuItemID, LanguageID>::const_iterator it = languageMenuItemMap.find(event.GetId());
    if (it != languageMenuItemMap.end())
        switchProgramLanguage(it->second);
}

//#########################################################################################################

void MainDialog::setViewTypeSyncAction(bool value)
{
    //if (m_bpButtonViewTypeSyncAction->isActive() == value) return; support polling -> what about initialization?

    m_bpButtonViewTypeSyncAction->setActive(value);
    m_bpButtonViewTypeSyncAction->SetToolTip((value ? _("Action") : _("Category")) +  L" (F8)");

    //toggle display of sync preview in middle grid
    gridview::highlightSyncAction(*m_gridMainC, value);

    updateGui();
}
