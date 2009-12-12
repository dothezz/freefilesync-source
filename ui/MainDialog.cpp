#include "mainDialog.h"
#include <wx/filename.h>
#include <stdexcept>
#include "../shared/systemConstants.h"
#include <wx/clipbrd.h>
#include <wx/dataobj.h>
#include <wx/ffile.h>
#include "../library/customGrid.h"
#include "../shared/customButton.h"
#include "../shared/customComboBox.h"
#include <wx/msgdlg.h>
#include "../comparison.h"
#include "../synchronization.h"
#include "../algorithm.h"
#include "util.h"
#include "checkVersion.h"
#include "guiStatusHandler.h"
#include "settingsDialog.h"
#include "../shared/localization.h"
#include "../shared/stringConv.h"
#include "smallDialogs.h"
#include "../shared/dragAndDrop.h"
#include "../library/filter.h"
#include "../structures.h"
#include <wx/imaglist.h>
#include <wx/wupdlock.h>
#include "gridView.h"
#include "../library/resources.h"
#include "../shared/fileHandling.h"
#include "../shared/xmlBase.h"
#include "../shared/standardPaths.h"
#include "../shared/toggleButton.h"
#include "folderPair.h"
#include "../shared/globalFunctions.h"
#include <wx/sound.h>

using namespace FreeFileSync;
using FreeFileSync::CustomLocale;


class MainFolderDragDrop : public DragDropOnMainDlg
{
public:
    MainFolderDragDrop(MainDialog&      mainDlg,
                       wxWindow*        dropWindow1,
                       wxWindow*        dropWindow2,
                       wxDirPickerCtrl* dirPicker,
                       wxComboBox*      dirName) :

        DragDropOnMainDlg(dropWindow1, dropWindow2, dirPicker, dirName),
        mainDlg_(mainDlg) {}

    virtual bool AcceptDrop(const wxString& dropName)
    {
        const xmlAccess::XmlType fileType = xmlAccess::getXmlType(dropName);

        //test if ffs config file has been dropped
        if (fileType == xmlAccess::XML_GUI_CONFIG)
        {
            mainDlg_.loadConfiguration(dropName);
            return false;
        }
        //...or a ffs batch file
        else if (fileType == xmlAccess::XML_BATCH_CONFIG)
        {
            BatchDialog* batchDlg = new BatchDialog(&mainDlg_, dropName);
            if (batchDlg->ShowModal() == BatchDialog::BATCH_FILE_SAVED)
                mainDlg_.pushStatusInformation(_("Batch file created successfully!"));
            return false;
        }

        //disable the sync button
        mainDlg_.syncPreview.enableSynchronization(false);

        //clear grids
        mainDlg_.gridDataView->clearAllRows();
        mainDlg_.updateGuiGrid();

        return true;
    }

private:
    MainFolderDragDrop(const MainFolderDragDrop&);

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
  FirstFolderPairCfg    FolderPairPanel
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


class FolderPairPanel :
    public FolderPairGenerated, //FolderPairPanel "owns" FolderPairGenerated!
    public FolderPairCallback<FolderPairGenerated>
{
public:
    FolderPairPanel(wxWindow* parent, MainDialog& mainDialog) :
        FolderPairGenerated(parent),
        FolderPairCallback<FolderPairGenerated>(static_cast<FolderPairGenerated&>(*this), mainDialog), //pass FolderPairGenerated part...
        dragDropOnLeft( m_panelLeft,  m_dirPickerLeft,  m_directoryLeft),
        dragDropOnRight(m_panelRight, m_dirPickerRight, m_directoryRight) {}

private:
    //support for drag and drop
    DragDropOnDlg dragDropOnLeft;
    DragDropOnDlg dragDropOnRight;
};


class FirstFolderPairCfg : public FolderPairCallback<MainDialogGenerated>
{
public:
    FirstFolderPairCfg(MainDialog& mainDialog) :
        FolderPairCallback<MainDialogGenerated>(mainDialog, mainDialog),

        //prepare drag & drop
        dragDropOnLeft(mainDialog,
                       mainDialog.m_panelLeft,
                       mainDialog.m_panelTopLeft,
                       mainDialog.m_dirPickerLeft,
                       mainDialog.m_directoryLeft),
        dragDropOnRight(mainDialog,
                        mainDialog.m_panelRight,
                        mainDialog.m_panelTopRight,
                        mainDialog.m_dirPickerRight,
                        mainDialog.m_directoryRight) {}

private:
    //support for drag and drop
    MainFolderDragDrop dragDropOnLeft;
    MainFolderDragDrop dragDropOnRight;
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


//##################################################################################################################################
MainDialog::MainDialog(wxFrame* frame,
                       const wxString& cfgFileName,
                       xmlAccess::XmlGlobalSettings& settings,
                       wxHelpController& helpController) :
    MainDialogGenerated(frame),
    globalSettings(settings),
    gridDataView(new FreeFileSync::GridView()),
    contextMenu(new wxMenu), //initialize right-click context menu; will be dynamically re-created on each R-mouse-click
    cleanedUp(false),
    lastSortColumn(-1),
    lastSortGrid(NULL),
#ifdef FFS_WIN
    updateFileIcons(new IconUpdater(m_gridLeft, m_gridRight)),
#endif
    helpController_(helpController),
    syncPreview(this)
{
    wxWindowUpdateLocker dummy(this); //avoid display distortion

    //init handling of first folder pair
    firstFolderPair.reset(new FirstFolderPairCfg(*this));

    initViewFilterButtons();

    //initialize and load configuration
    readGlobalSettings();

    if (cfgFileName.empty())
        readConfigurationFromXml(lastConfigFileName(), true);
    else
        readConfigurationFromXml(cfgFileName, true);

    //set icons for this dialog
    m_bpButton10->SetBitmapLabel(*GlobalResources::getInstance().bitmapExit);
    m_buttonCompare->setBitmapFront(*GlobalResources::getInstance().bitmapCompare);
    m_bpButtonSyncConfig->SetBitmapLabel(*GlobalResources::getInstance().bitmapSyncCfg);
    m_bpButtonCmpConfig->SetBitmapLabel(*GlobalResources::getInstance().bitmapCmpCfg);
    m_bpButtonSave->SetBitmapLabel(*GlobalResources::getInstance().bitmapSave);
    m_bpButtonLoad->SetBitmapLabel(*GlobalResources::getInstance().bitmapLoad);
    m_bpButtonAddPair->SetBitmapLabel(*GlobalResources::getInstance().bitmapAddFolderPair);
    m_bitmap15->SetBitmap(*GlobalResources::getInstance().bitmapStatusEdge);

    m_bitmapCreate->SetBitmap(*GlobalResources::getInstance().bitmapCreate);
    m_bitmapUpdate->SetBitmap(*GlobalResources::getInstance().bitmapUpdate);
    m_bitmapDelete->SetBitmap(*GlobalResources::getInstance().bitmapDelete);
    m_bitmapData->SetBitmap(*GlobalResources::getInstance().bitmapData);

    bSizer6->Layout(); //wxButtonWithImage size might have changed

    //menu icons: workaround for wxWidgets: small hack to update menu items: actually this is a wxWidgets bug (affects Windows- and Linux-build)
    MenuItemUpdater updateMenuFile(m_menuFile);
    updateMenuFile.addForUpdate(m_menuItem10,   *GlobalResources::getInstance().bitmapCompareSmall);
    updateMenuFile.addForUpdate(m_menuItem11,   *GlobalResources::getInstance().bitmapSyncSmall);
    updateMenuFile.addForUpdate(m_menuItemNew,  *GlobalResources::getInstance().bitmapNewSmall);
    updateMenuFile.addForUpdate(m_menuItemSave, *GlobalResources::getInstance().bitmapSaveSmall);
    updateMenuFile.addForUpdate(m_menuItemLoad, *GlobalResources::getInstance().bitmapLoadSmall);

    MenuItemUpdater updateMenuAdv(m_menuAdvanced);
    updateMenuAdv.addForUpdate(m_menuItemGlobSett, *GlobalResources::getInstance().bitmapSettingsSmall);
    updateMenuAdv.addForUpdate(m_menuItem7, *GlobalResources::getInstance().bitmapBatchSmall);

    MenuItemUpdater updateMenuHelp(m_menuHelp);
    updateMenuHelp.addForUpdate(m_menuItemAbout, *GlobalResources::getInstance().bitmapAboutSmall);


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

    //support for CTRL + C and DEL
    m_gridLeft->Connect(wxEVT_KEY_DOWN, wxKeyEventHandler(MainDialog::onGridLeftButtonEvent), NULL, this);
    m_gridRight->Connect(wxEVT_KEY_DOWN, wxKeyEventHandler(MainDialog::onGridRightButtonEvent), NULL, this);
    m_gridMiddle->Connect(wxEVT_KEY_DOWN, wxKeyEventHandler(MainDialog::onGridMiddleButtonEvent), NULL, this);

    Connect(wxEVT_IDLE, wxEventHandler(MainDialog::OnIdleEvent), NULL, this);
    Connect(wxEVT_SIZE, wxSizeEventHandler(MainDialog::OnResize), NULL, this);
    Connect(wxEVT_MOVE, wxSizeEventHandler(MainDialog::OnResize), NULL, this);

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
    syncPreview.enableSynchronization(false);

    //mainly to update row label sizes...
    updateGuiGrid();

    //create the compare status panel in hidden state
    compareStatus = new CompareStatus(this);
    bSizer1->Insert(1, compareStatus, 0, wxEXPAND | wxBOTTOM, 5 );
    Layout();   //avoid screen flicker when panel is shown later
    compareStatus->Hide();

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
}


MainDialog::~MainDialog()
{
    cleanUp(); //do NOT include any other code here! cleanUp() is re-used when switching languages
}


void MainDialog::cleanUp()
{
    if (!cleanedUp)
    {
        cleanedUp = true;

        //no need for wxEventHandler::Disconnect() here; done automatically when window is destoyed!

        //save configuration
        writeConfigurationToXml(lastConfigFileName());   //don't trow exceptions in destructors
        writeGlobalSettings();
    }
}


void MainDialog::readGlobalSettings()
{
    //apply window size and position at program startup ONLY
    widthNotMaximized  = globalSettings.gui.widthNotMaximized;
    heightNotMaximized = globalSettings.gui.heightNotMaximized;
    posXNotMaximized   = globalSettings.gui.posXNotMaximized;
    posYNotMaximized   = globalSettings.gui.posYNotMaximized;

    //apply window size and position
    if (    widthNotMaximized  != wxDefaultCoord &&
            heightNotMaximized != wxDefaultCoord &&
            posXNotMaximized   != wxDefaultCoord &&
            posYNotMaximized   != wxDefaultCoord)
        SetSize(posXNotMaximized, posYNotMaximized, widthNotMaximized, heightNotMaximized);
    else
        Centre();

    Maximize(globalSettings.gui.isMaximized);

    //set column attributes
    m_gridLeft->setColumnAttributes(globalSettings.gui.columnAttribLeft);
    m_gridRight->setColumnAttributes(globalSettings.gui.columnAttribRight);

    //load list of last used configuration files (in reverse order)
    for (std::vector<wxString>::reverse_iterator i = globalSettings.gui.cfgFileHistory.rbegin();
            i != globalSettings.gui.cfgFileHistory.rend();
            ++i)
        addFileToCfgHistory(*i);

    //load list of last used folders
    for (std::vector<wxString>::reverse_iterator i = globalSettings.gui.folderHistoryLeft.rbegin();
            i != globalSettings.gui.folderHistoryLeft.rend();
            ++i)
        addLeftFolderToHistory(*i);
    for (std::vector<wxString>::reverse_iterator i = globalSettings.gui.folderHistoryRight.rbegin();
            i != globalSettings.gui.folderHistoryRight.rend();
            ++i)
        addRightFolderToHistory(*i);

    //show/hide file icons
#ifdef FFS_WIN
    m_gridLeft->enableFileIcons(globalSettings.gui.showFileIconsLeft);
    m_gridRight->enableFileIcons(globalSettings.gui.showFileIconsRight);
#endif

    //set selected tab
    m_notebookBottomLeft->ChangeSelection(globalSettings.gui.selectedTabBottomLeft);
}


void MainDialog::writeGlobalSettings()
{
    //write global settings to (global) variable stored in application instance
    globalSettings.gui.widthNotMaximized  = widthNotMaximized;
    globalSettings.gui.heightNotMaximized = heightNotMaximized;
    globalSettings.gui.posXNotMaximized   = posXNotMaximized;
    globalSettings.gui.posYNotMaximized   = posYNotMaximized;
    globalSettings.gui.isMaximized        = IsMaximized();

    //retrieve column attributes
    globalSettings.gui.columnAttribLeft  = m_gridLeft->getColumnAttributes();
    globalSettings.gui.columnAttribRight = m_gridRight->getColumnAttributes();

    //write list of last used configuration files
    globalSettings.gui.cfgFileHistory = cfgFileNames;

    //write list of last used folders
    globalSettings.gui.folderHistoryLeft.clear();
    const wxArrayString leftFolderHistory = m_directoryLeft->GetStrings();
    for (unsigned i = 0; i < leftFolderHistory.GetCount(); ++i)
        globalSettings.gui.folderHistoryLeft.push_back(leftFolderHistory[i]);

    globalSettings.gui.folderHistoryRight.clear();
    const wxArrayString rightFolderHistory = m_directoryRight->GetStrings();
    for (unsigned i = 0; i < rightFolderHistory.GetCount(); ++i)
        globalSettings.gui.folderHistoryRight.push_back(rightFolderHistory[i]);

    //get selected tab
    globalSettings.gui.selectedTabBottomLeft = m_notebookBottomLeft->GetSelection();
}


void MainDialog::setSyncDirManually(const std::set<unsigned int>& rowsToSetOnUiTable, const FreeFileSync::SyncDirection dir)
{
    if (rowsToSetOnUiTable.size() > 0)
    {
        for (std::set<unsigned int>::const_iterator i = rowsToSetOnUiTable.begin(); i != rowsToSetOnUiTable.end(); ++i)
        {
            FileSystemObject* fsObj = gridDataView->getObject(*i);
            if (fsObj)
            {
                setSyncDirectionRec(dir, *fsObj); //set new direction (recursively)
                FreeFileSync::setActiveStatus(true, *fsObj); //works recursively for directories
            }
        }

        updateGuiGrid();
    }
}


void MainDialog::filterRangeManually(const std::set<unsigned int>& rowsToFilterOnUiTable, const int leadingRow)
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
            FreeFileSync::setActiveStatus(newSelection, **i); //works recursively for directories

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

            m_panel7->Layout();
        }
    }

    event.Skip();
}


void MainDialog::copySelectionToClipboard(const CustomGrid* selectedGrid)
{
    const std::set<unsigned int> selectedRows = getSelectedRows(selectedGrid);
    if (selectedRows.size() > 0)
    {
        wxString clipboardString;

        for (std::set<unsigned int>::const_iterator i = selectedRows.begin(); i != selectedRows.end(); ++i)
        {
            for (int k = 0; k < const_cast<CustomGrid*>(selectedGrid)->GetNumberCols(); ++k)
            {
                clipboardString+= const_cast<CustomGrid*>(selectedGrid)->GetCellValue(*i, k);
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


std::set<unsigned int> MainDialog::getSelectedRows(const CustomGrid* grid) const
{
    std::set<unsigned int> output = grid->getAllSelectedRows();

    //remove invalid rows
    output.erase(output.lower_bound(gridDataView->rowsOnView()), output.end());

    return output;
}


std::set<unsigned int> MainDialog::getSelectedRows() const
{
    //merge selections from left and right grid
    std::set<unsigned int> selection  = getSelectedRows(m_gridLeft);
    std::set<unsigned int> additional = getSelectedRows(m_gridRight);
    selection.insert(additional.begin(), additional.end());
    return selection;
}


class ManualDeletionHandler : private wxEvtHandler, public DeleteFilesHandler
{
public:
    ManualDeletionHandler(MainDialog* main, int totalObjToDel) :
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
            throw FreeFileSync::AbortThisProcess();

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
            throw FreeFileSync::AbortThisProcess();
        }

        assert (false);
        return DeleteFilesHandler::IGNORE_ERROR; //dummy return value
    }

    virtual void deletionSuccessful()  //called for each file/folder that has been deleted
    {
        ++deletionCount;

        if (updateUiIsAllowed())  //test if specific time span between ui updates is over
        {
            wxString statusMessage = _("%x / %y objects deleted successfully");
            statusMessage.Replace(wxT("%x"), globalFunctions::numberToWxString(deletionCount), false);
            statusMessage.Replace(wxT("%y"), globalFunctions::numberToWxString(totalObjToDelete), false);

            mainDlg->m_staticTextStatusMiddle->SetLabel(statusMessage);
            mainDlg->m_panel7->Layout();

            updateUiNow();
        }

        if (abortRequested) //test after (implicit) call to wxApp::Yield()
            throw FreeFileSync::AbortThisProcess();
    }

private:
    void OnAbortCompare(wxCommandEvent& event) //handle abort button click
    {
        abortRequested = true; //don't throw exceptions in a GUI-Callback!!! (throw FreeFileSync::AbortThisProcess())
    }

    MainDialog* const mainDlg;
    const int totalObjToDelete;

    bool abortRequested;
    bool ignoreErrors;
    unsigned int deletionCount;
};


void MainDialog::deleteSelectedFiles()
{
    //get set of selected rows on view
    const std::set<unsigned int> viewSelectionLeft  = getSelectedRows(m_gridLeft);
    const std::set<unsigned int> viewSelectionRight = getSelectedRows(m_gridRight);

    if (viewSelectionLeft.size() + viewSelectionRight.size())
    {
        //map lines from GUI view to grid line references
        std::vector<FileSystemObject*> compRefLeft;
        gridDataView->getAllFileRef(viewSelectionLeft, compRefLeft);

        std::vector<FileSystemObject*> compRefRight;
        gridDataView->getAllFileRef(viewSelectionRight, compRefRight);


        int totalDeleteCount = 0;

        DeleteDialog* confirmDeletion = new DeleteDialog(this, //no destruction needed; attached to main window
                compRefLeft,
                compRefRight,
                globalSettings.gui.deleteOnBothSides,
                globalSettings.gui.useRecyclerForManualDeletion,
                totalDeleteCount);
        if (confirmDeletion->ShowModal() == DeleteDialog::BUTTON_OKAY)
        {
            if (globalSettings.gui.useRecyclerForManualDeletion && !FreeFileSync::recycleBinExists())
            {
                wxMessageBox(_("Unable to initialize Recycle Bin!"));
                return;
            }

            try
            {
                //handle errors when deleting files/folders
                ManualDeletionHandler statusHandler(this, totalDeleteCount);

                FreeFileSync::deleteFromGridAndHD(gridDataView->getDataTentative(),
                                                  compRefLeft,
                                                  compRefRight,
                                                  globalSettings.gui.deleteOnBothSides,
                                                  globalSettings.gui.useRecyclerForManualDeletion,
                                                  getCurrentConfiguration().mainCfg,
                                                  &statusHandler);
            }
            catch (FreeFileSync::AbortThisProcess&) {}

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
        const FileMapping* fileObj = dynamic_cast<const FileMapping*>(&fsObj);
        if (fileObj != NULL)
        {
            name = zToWx(fsObj.getFullName<side>());
            dir  = zToWx(fsObj.getFullName<side>().BeforeLast(globalFunctions::FILE_NAME_SEPARATOR));
        }
        else
        {
            const DirMapping* dirObj = dynamic_cast<const DirMapping*>(&fsObj);
            if (dirObj != NULL)
            {
                name = zToWx(fsObj.getFullName<side>());
                dir  = name;
            }
        }
    }
    else
    {
        name.clear();
        dir.clear();
    }
}


void MainDialog::openExternalApplication(unsigned int rowNumber, bool leftSide, const wxString& commandline)
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
        dir   = zToWx(FreeFileSync::getFormattedDirectoryName(firstFolderPair->getLeftDir()));
        dirCo = zToWx(FreeFileSync::getFormattedDirectoryName(firstFolderPair->getRightDir()));

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
    m_panel7->Layout();
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

        if (width > 0 && height > 0 && x >= 0 && y >= 0) //test ALL parameters at once, since width/height are invalid if
        {
            //the window is minimized (eg x,y == -32000; height = 28, width = 160)
            widthNotMaximized  = width;
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

        for (std::vector<FolderPairPanel*>::iterator i = additionalFolderPairs.begin(); i != additionalFolderPairs.end(); ++i)
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
        case 67:
        case WXK_INSERT: //CTRL + C || CTRL + INS
            copySelectionToClipboard(m_gridLeft);
            break;

        case 65: //CTRL + A
            m_gridLeft->SelectAll();
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
        case 67:
        case WXK_INSERT: //CTRL + C || CTRL + INS
            copySelectionToClipboard(m_gridRight);
            break;

        case 65: //CTRL + A
            m_gridRight->SelectAll();
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


    const std::set<unsigned int> selectionLeft  = getSelectedRows(m_gridLeft);
    const std::set<unsigned int> selectionRight = getSelectedRows(m_gridRight);

    const unsigned int selectionBegin = selectionLeft.size() + selectionRight.size() == 0 ? 0 :
                                        selectionLeft.size()  == 0 ? *selectionRight.begin() :
                                        selectionRight.size() == 0 ? *selectionLeft.begin()  :
                                        std::min(*selectionLeft.begin(), *selectionRight.begin());

    const FileSystemObject* fsObj = gridDataView->getObject(selectionBegin);


    //#######################################################
    //re-create context menu
    contextMenu.reset(new wxMenu);

    if (syncPreview.previewIsEnabled())
    {
        if (fsObj && (selectionLeft.size() + selectionRight.size() > 0))
        {
            //CONTEXT_SYNC_DIR_LEFT
            wxMenuItem* menuItemSyncDirLeft = new wxMenuItem(contextMenu.get(), CONTEXT_SYNC_DIR_LEFT, wxString(_("Change direction")) + wxT("\tALT + LEFT"));
            menuItemSyncDirLeft->SetBitmap(getSyncOpImage(fsObj->testSyncOperation(true, SYNC_DIR_LEFT)));
            contextMenu->Append(menuItemSyncDirLeft);

            //CONTEXT_SYNC_DIR_NONE
            wxMenuItem* menuItemSyncDirNone = new wxMenuItem(contextMenu.get(), CONTEXT_SYNC_DIR_NONE, wxString(_("Change direction")) + wxT("\tALT + UP"));
            menuItemSyncDirNone->SetBitmap(getSyncOpImage(fsObj->testSyncOperation(true, SYNC_DIR_NONE)));
            contextMenu->Append(menuItemSyncDirNone);

            //CONTEXT_SYNC_DIR_RIGHT
            wxMenuItem* menuItemSyncDirRight = new wxMenuItem(contextMenu.get(), CONTEXT_SYNC_DIR_RIGHT, wxString(_("Change direction")) + wxT("\tALT + RIGHT"));
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
            menuItemExclTemp->SetBitmap(*GlobalResources::getInstance().bitmapCheckBoxFalse);
            contextMenu->Append(menuItemExclTemp);
        }
        else
        {
            wxMenuItem* menuItemInclTemp = new wxMenuItem(contextMenu.get(), CONTEXT_FILTER_TEMP, wxString(_("Include temporarily")) + wxT("\tSPACE"));
            menuItemInclTemp->SetBitmap(*GlobalResources::getInstance().bitmapCheckBoxTrue);
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
    for (std::set<unsigned int>::const_iterator i = selectionLeft.begin(); i != selectionLeft.end(); ++i)
    {
        const FileSystemObject* currObj = gridDataView->getObject(*i);
        if (currObj && !currObj->isEmpty<LEFT_SIDE>())
            exFilterCandidateObj.push_back(
                FilterObject(currObj->getRelativeName<LEFT_SIDE>(),
                             dynamic_cast<const DirMapping*>(currObj) != NULL));
    }
    for (std::set<unsigned int>::const_iterator i = selectionRight.begin(); i != selectionRight.end(); ++i)
    {
        const FileSystemObject* currObj = gridDataView->getObject(*i);
        if (currObj && !currObj->isEmpty<RIGHT_SIDE>())
            exFilterCandidateObj.push_back(
                FilterObject(currObj->getRelativeName<RIGHT_SIDE>(),
                             dynamic_cast<const DirMapping*>(currObj) != NULL));
    }
    //###############################################################################################

    //CONTEXT_EXCLUDE_EXT
    if (exFilterCandidateObj.size() > 0 && !exFilterCandidateObj[0].isDir)
    {
        const Zstring filename = exFilterCandidateObj[0].relativeName.AfterLast(globalFunctions::FILE_NAME_SEPARATOR);
        if (filename.Find(wxChar('.'), false) !=  Zstring::npos) //be careful: AfterLast will return the whole string if '.' is not found!
        {
            const Zstring extension = filename.AfterLast(DefaultChar('.'));

            //add context menu item
            wxMenuItem* menuItemExclExt = new wxMenuItem(contextMenu.get(), CONTEXT_EXCLUDE_EXT, wxString(_("Exclude via filter:")) + wxT(" ") + wxT("*.") + zToWx(extension));
            menuItemExclExt->SetBitmap(*GlobalResources::getInstance().bitmapFilterSmall);
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
        menuItemExclObj = new wxMenuItem(contextMenu.get(), CONTEXT_EXCLUDE_OBJ, wxString(_("Exclude via filter:")) + wxT(" ") + zToWx(exFilterCandidateObj[0].relativeName.AfterLast(globalFunctions::FILE_NAME_SEPARATOR)));
    else if (exFilterCandidateObj.size() > 1)
        menuItemExclObj = new wxMenuItem(contextMenu.get(), CONTEXT_EXCLUDE_OBJ, wxString(_("Exclude via filter:")) + wxT(" ") + _("<multiple selection>"));

    if (menuItemExclObj != NULL)
    {
        menuItemExclObj->SetBitmap(*GlobalResources::getInstance().bitmapFilterSmall);
        contextMenu->Append(menuItemExclObj);

        //connect event
        contextMenu->Connect(CONTEXT_EXCLUDE_OBJ,
                             wxEVT_COMMAND_MENU_SELECTED,
                             wxCommandEventHandler(MainDialog::OnContextExcludeObject),
                             new FilterObjContainer(exFilterCandidateObj), //ownership passed!
                             this);
    }



    //CONTEXT_EXTERNAL_APP
    if (!globalSettings.gui.externelApplications.empty())
    {
        contextMenu->AppendSeparator();

        const bool externalAppEnabled = (m_gridLeft->isLeadGrid() || m_gridRight->isLeadGrid()) &&
                                        (selectionLeft.size() + selectionRight.size() == 1);

        int newID = externalAppIDFirst;
        for (xmlAccess::ExternalApps::iterator i = globalSettings.gui.externelApplications.begin();
                i != globalSettings.gui.externelApplications.end();
                ++i, ++newID)
        {
            if (i == globalSettings.gui.externelApplications.begin())
                contextMenu->Append(newID, i->first + wxT("\t") + wxString(_("D-Click")) + wxT("; ENTER"));
            else
                contextMenu->Append(newID, i->first.empty() ? wxT(" ") : i->first); //wxWidgets doesn't like empty items

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
    std::set<unsigned int> selection = getSelectedRows();
    if (!selection.empty())
        filterRangeManually(selection, *selection.begin());
}


void MainDialog::OnContextExcludeExtension(wxCommandEvent& event)
{
    SelectedExtension* selExtension = dynamic_cast<SelectedExtension*>(event.m_callbackUserData);
    if (selExtension)
    {
        if (!currentCfg.mainCfg.excludeFilter.empty() && !currentCfg.mainCfg.excludeFilter.EndsWith(DefaultStr(";")))
            currentCfg.mainCfg.excludeFilter += DefaultStr("\n");

        currentCfg.mainCfg.excludeFilter += Zstring(DefaultStr("*.")) + selExtension->extension + DefaultStr(";"); //';' is appended to 'mark' that next exclude extension entry won't write to new line

        m_checkBoxActivateFilter->SetValue(true);
        updateFilterButtons();

        applyFiltering(getCurrentConfiguration().mainCfg, gridDataView->getDataTentative());
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
            for (std::vector<FilterObject>::const_iterator i = objCont->selectedObjects.begin(); i != objCont->selectedObjects.end(); ++i)
            {
                if (!currentCfg.mainCfg.excludeFilter.empty() && !currentCfg.mainCfg.excludeFilter.EndsWith(DefaultStr("\n")))
                    currentCfg.mainCfg.excludeFilter += DefaultStr("\n");

                if (!i->isDir)
                    currentCfg.mainCfg.excludeFilter += Zstring() + globalFunctions::FILE_NAME_SEPARATOR + i->relativeName;
                else
                    currentCfg.mainCfg.excludeFilter += Zstring() + globalFunctions::FILE_NAME_SEPARATOR + i->relativeName + globalFunctions::FILE_NAME_SEPARATOR + DefaultStr("*");
            }

            m_checkBoxActivateFilter->SetValue(true);
            updateFilterButtons();

            applyFiltering(getCurrentConfiguration().mainCfg, gridDataView->getDataTentative());
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
        std::set<unsigned int> selection = getSelectedRows(leadGrid);

        const int index = event.GetId() - externalAppIDFirst;

        if (        selection.size() == 1     &&
                    0 <= index && static_cast<unsigned>(index) <  globalSettings.gui.externelApplications.size())
            openExternalApplication(*selection.begin(), m_gridLeft->isLeadGrid(), globalSettings.gui.externelApplications[index].second);
    }
}


void MainDialog::OnContextDeleteFiles(wxCommandEvent& event)
{
    deleteSelectedFiles();
}


void MainDialog::OnContextSyncDirLeft(wxCommandEvent& event)
{
    //merge selections from left and right grid
    const std::set<unsigned int> selection = getSelectedRows();
    setSyncDirManually(selection, FreeFileSync::SYNC_DIR_LEFT);
}


void MainDialog::OnContextSyncDirNone(wxCommandEvent& event)
{
    //merge selections from left and right grid
    const std::set<unsigned int> selection = getSelectedRows();
    setSyncDirManually(selection, FreeFileSync::SYNC_DIR_NONE);
}


void MainDialog::OnContextSyncDirRight(wxCommandEvent& event)
{
    //merge selections from left and right grid
    const std::set<unsigned int> selection = getSelectedRows();
    setSyncDirManually(selection, FreeFileSync::SYNC_DIR_RIGHT);
}


void MainDialog::OnContextRimLabelLeft(wxGridEvent& event)
{
    contextMenu.reset(new wxMenu); //re-create context menu
    contextMenu->Append(CONTEXT_CUSTOMIZE_COLUMN_LEFT, _("Customize..."));

    contextMenu->AppendSeparator();

    wxMenuItem* itemAutoAdjust = new wxMenuItem(contextMenu.get(), CONTEXT_AUTO_ADJUST_COLUMN_LEFT, _("Auto-adjust columns"), wxEmptyString, wxITEM_CHECK);
    contextMenu->Append(itemAutoAdjust);
    itemAutoAdjust->Check(globalSettings.gui.autoAdjustColumnsLeft);

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
    itemAutoAdjust->Check(globalSettings.gui.autoAdjustColumnsRight);

    contextMenu->Connect(CONTEXT_CUSTOMIZE_COLUMN_RIGHT, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(MainDialog::OnContextCustColumnRight), NULL, this);
    contextMenu->Connect(CONTEXT_AUTO_ADJUST_COLUMN_RIGHT, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(MainDialog::OnContextAutoAdjustRight), NULL, this);

    PopupMenu(contextMenu.get()); //show context menu
}


void MainDialog::OnContextCustColumnLeft(wxCommandEvent& event)
{
    xmlAccess::ColumnAttributes colAttr = m_gridLeft->getColumnAttributes();
    CustomizeColsDlg* customizeDlg = new CustomizeColsDlg(this, colAttr, globalSettings.gui.showFileIconsLeft);
    if (customizeDlg->ShowModal() == CustomizeColsDlg::BUTTON_OKAY)
    {
        m_gridLeft->setColumnAttributes(colAttr);
#ifdef FFS_WIN
        m_gridLeft->enableFileIcons(globalSettings.gui.showFileIconsLeft);
#endif

        m_gridLeft->setSortMarker(CustomGrid::SortMarker(-1, CustomGrid::ASCENDING)); //hide sort direction indicator on GUI grids
        m_gridMiddle->setSortMarker(CustomGrid::SortMarker(-1, CustomGrid::ASCENDING));
        m_gridRight->setSortMarker(CustomGrid::SortMarker(-1, CustomGrid::ASCENDING));
    }
}


void MainDialog::OnContextCustColumnRight(wxCommandEvent& event)
{
    xmlAccess::ColumnAttributes colAttr = m_gridRight->getColumnAttributes();
    CustomizeColsDlg* customizeDlg = new CustomizeColsDlg(this, colAttr, globalSettings.gui.showFileIconsRight);
    if (customizeDlg->ShowModal() == CustomizeColsDlg::BUTTON_OKAY)
    {
        m_gridRight->setColumnAttributes(colAttr);
#ifdef FFS_WIN
        m_gridRight->enableFileIcons(globalSettings.gui.showFileIconsRight);
#endif

        m_gridLeft->setSortMarker(CustomGrid::SortMarker(-1, CustomGrid::ASCENDING)); //hide sort direction indicator on GUI grids
        m_gridMiddle->setSortMarker(CustomGrid::SortMarker(-1, CustomGrid::ASCENDING));
        m_gridRight->setSortMarker(CustomGrid::SortMarker(-1, CustomGrid::ASCENDING));
    }
}


void MainDialog::OnContextAutoAdjustLeft(wxCommandEvent& event)
{
    globalSettings.gui.autoAdjustColumnsLeft = !globalSettings.gui.autoAdjustColumnsLeft;
    updateGuiGrid();
}


void MainDialog::OnContextAutoAdjustRight(wxCommandEvent& event)
{
    globalSettings.gui.autoAdjustColumnsRight = !globalSettings.gui.autoAdjustColumnsRight;
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

    if (syncPreview.previewIsEnabled())
        itemSyncPreview->SetBitmap(*GlobalResources::getInstance().bitmapSyncViewSmall);
    else
        itemCmpResult->SetBitmap(*GlobalResources::getInstance().bitmapCmpViewSmall);

    contextMenu->Append(itemCmpResult);
    contextMenu->Append(itemSyncPreview);

    contextMenu->Connect(CONTEXT_SYNC_PREVIEW,       wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(MainDialog::OnContextSyncView),       NULL, this);
    contextMenu->Connect(CONTEXT_COMPARISON_RESULT,  wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(MainDialog::OnContextComparisonView), NULL, this);

    PopupMenu(contextMenu.get()); //show context menu
}


void MainDialog::OnContextIncludeAll(wxCommandEvent& event)
{
    FreeFileSync::setActiveStatus(true, gridDataView->getDataTentative());
    refreshGridAfterFilterChange(0); //call this instead of updateGuiGrid() to add some delay if hideFiltered == true and to handle some graphical artifacts        break;
}


void MainDialog::OnContextExcludeAll(wxCommandEvent& event)
{
    FreeFileSync::setActiveStatus(false, gridDataView->getDataTentative());
    refreshGridAfterFilterChange(400); //call this instead of updateGuiGrid() to add some delay if hideFiltered == true and to handle some graphical artifacts
}


void MainDialog::OnContextComparisonView(wxCommandEvent& event)
{
    syncPreview.enablePreview(false); //change view
}


void MainDialog::OnContextSyncView(wxCommandEvent& event)
{
    syncPreview.enablePreview(true); //change view
}


void MainDialog::OnDirSelected(wxFileDirPickerEvent& event)
{
    //left and right directory text-control and dirpicker are synchronized by MainFolderDragDrop automatically

    //disable the sync button
    syncPreview.enableSynchronization(false);

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


wxString getFullFilename(const wxString& name)
{
    //resolve relative names to avoid problems after working directory is changed
    wxFileName filename(name);
    if (!filename.Normalize())
        return name; //fallback

    return filename.GetFullPath();
}


//tests if the same filenames are specified, even if they are relative to the current working directory
inline
bool sameFileSpecified(const wxString& file1, const wxString& file2)
{
    const wxString file1Full = getFullFilename(file1);
    const wxString file2Full = getFullFilename(file2);

#ifdef FFS_WIN //don't respect case in windows build
    return file1Full.CmpNoCase(file2Full) == 0;
#elif defined FFS_LINUX
    return file1Full == file2Full;
#endif
}


class FindDuplicates
{
public:
    FindDuplicates(const wxString& name) : m_name(name) {}

    bool operator()(const wxString& other) const
    {
        return sameFileSpecified(m_name, other);
    }

private:
    const wxString& m_name;
};


void MainDialog::addFileToCfgHistory(const wxString& filename)
{
    //only (still) existing files should be included in the list
    if (!wxFileExists(filename))
        return;

    std::vector<wxString>::const_iterator i = find_if(cfgFileNames.begin(), cfgFileNames.end(), FindDuplicates(filename));
    if (i != cfgFileNames.end())
    {
        //if entry is in the list, then jump to element
        m_choiceHistory->SetSelection(i - cfgFileNames.begin());
    }
    else
    {
        cfgFileNames.insert(cfgFileNames.begin(), filename);

        //the default config file should receive another name on GUI
        if (sameFileSpecified(lastConfigFileName(), filename))
            m_choiceHistory->Insert(_("<Last session>"), 0);  //insert at beginning of list
        else
            m_choiceHistory->Insert(getFormattedHistoryElement(filename), 0);  //insert at beginning of list

        m_choiceHistory->SetSelection(0);
    }

    //keep maximal size of history list
    if (cfgFileNames.size() > globalSettings.gui.cfgHistoryMax)
    {
        //delete last rows
        cfgFileNames.pop_back();
        m_choiceHistory->Delete(globalSettings.gui.cfgHistoryMax);
    }
}


void MainDialog::addLeftFolderToHistory(const wxString& leftFolder)
{
    m_directoryLeft->addPairToFolderHistory(leftFolder, globalSettings.gui.folderHistLeftMax);
}


void MainDialog::addRightFolderToHistory(const wxString& rightFolder)
{
    m_directoryRight->addPairToFolderHistory(rightFolder, globalSettings.gui.folderHistRightMax);
}


void MainDialog::OnSaveConfig(wxCommandEvent& event)
{
    trySaveConfig();
}


bool MainDialog::trySaveConfig() //return true if saved successfully
{
    const wxString defaultFileName = currentConfigFileName.empty() ? wxT("SyncSettings.ffs_gui") : currentConfigFileName;

    wxFileDialog* filePicker = new wxFileDialog(this, wxEmptyString, wxEmptyString, defaultFileName, wxString(_("FreeFileSync configuration")) + wxT(" (*.ffs_gui)|*.ffs_gui"), wxFD_SAVE);
    if (filePicker->ShowModal() == wxID_OK)
    {
        const wxString newFileName = filePicker->GetPath();

        if (FreeFileSync::fileExists(wxToZ(newFileName)))
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
    if (globalSettings.optDialogs.popupOnConfigChange && !currentConfigFileName.empty()) //only if check is active and non-default config file loaded
    {
        if (lastConfigurationSaved != getCurrentConfiguration())
        {
            bool dontShowAgain = !globalSettings.optDialogs.popupOnConfigChange;

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
                globalSettings.optDialogs.popupOnConfigChange = !dontShowAgain;
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
        std::set<unsigned int> selectedRowsOnView;

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
                FreeFileSync::setActiveStatus(true, *fsObj); //works recursively for directories
            }
        }

        updateGuiGrid();
    }
}


bool MainDialog::readConfigurationFromXml(const wxString& filename, bool programStartup)
{
    //load XML
    xmlAccess::XmlGuiConfig newGuiCfg;  //structure to receive gui settings, already defaulted!!
    bool parsingError = false;
    try
    {
        xmlAccess::readGuiOrBatchConfig(filename, newGuiCfg); //allow reading batch configurations also
    }
    catch (const xmlAccess::XmlError& error)
    {
        if (programStartup && filename == lastConfigFileName() && !wxFileExists(filename)) //do not show error message in this case
            ;
        else
        {
            if (error.getSeverity() == xmlAccess::XmlError::WARNING)
            {
                wxMessageBox(error.show(), _("Warning"), wxOK | wxICON_WARNING);
                parsingError = true;
            }
            else
            {
                wxMessageBox(error.show(), _("Error"), wxOK | wxICON_ERROR);
                return false;
            }
        }
    }

    setCurrentConfiguration(newGuiCfg);

    //###########################################################
    addFileToCfgHistory(filename); //put filename on list of last used config files

    lastConfigurationSaved = parsingError ? xmlAccess::XmlGuiConfig() : currentCfg; //simulate changed config on parsing errors

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

    return true;
}


bool MainDialog::writeConfigurationToXml(const wxString& filename)
{
    const xmlAccess::XmlGuiConfig guiCfg = getCurrentConfiguration();

    //write config to XML
    try
    {
        xmlAccess::writeGuiConfig(guiCfg, filename);
    }
    catch (const xmlAccess::XmlError& error)
    {
        wxMessageBox(error.show().c_str(), _("Error"), wxOK | wxICON_ERROR);
        return false;
    }

    //###########################################################
    addFileToCfgHistory(filename); //put filename on list of last used config files

    lastConfigurationSaved = guiCfg;

    if (filename == lastConfigFileName()) //set title
    {
        SetTitle(wxString(wxT("FreeFileSync - ")) + _("Folder Comparison and Synchronization"));
        currentConfigFileName.clear();
    }
    else
    {
        SetTitle(wxString(wxT("FreeFileSync - ")) + filename);
        currentConfigFileName = filename;
    }

    return true;
}


void MainDialog::setCurrentConfiguration(const xmlAccess::XmlGuiConfig& newGuiCfg)
{
    currentCfg = newGuiCfg;

    //evaluate new settings...

    //disable the sync button
    syncPreview.enableSynchronization(false);

    //clear grids
    gridDataView->clearAllRows();
    updateGuiGrid();

    //(re-)set view filter buttons
    initViewFilterButtons();

    m_checkBoxActivateFilter->SetValue(currentCfg.mainCfg.filterIsActive);
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

    syncPreview.enablePreview(currentCfg.syncPreviewEnabled);

    //###########################################################
    //update compare variant name
    m_staticTextCmpVariant->SetLabel(wxString(wxT("(")) + getVariantName(currentCfg.mainCfg.compareVar) + wxT(")"));

    //update sync variant name
    m_staticTextSyncVariant->SetLabel(wxString(wxT("(")) + currentCfg.mainCfg.getSyncVariantName() + wxT(")"));
    bSizer6->Layout(); //adapt layout for variant text
}


inline
FolderPairEnh getEnahncedPair(const FolderPairPanel* panel)
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

    //filter active status
    guiCfg.mainCfg.filterIsActive = m_checkBoxActivateFilter->GetValue();

    //sync preview
    guiCfg.syncPreviewEnabled = syncPreview.previewIsEnabled();

    return guiCfg;
}


const wxString& MainDialog::lastConfigFileName()
{
    static wxString instance = FreeFileSync::getConfigDir().EndsWith(zToWx(globalFunctions::FILE_NAME_SEPARATOR)) ?
                               FreeFileSync::getConfigDir() + wxT("LastRun.ffs_gui") :
                               FreeFileSync::getConfigDir() + zToWx(globalFunctions::FILE_NAME_SEPARATOR) + wxT("LastRun.ffs_gui");
    return instance;
}


void MainDialog::refreshGridAfterFilterChange(const int delay)
{
    //signal UI that grids need to be refreshed on next Update()
    m_gridLeft->ForceRefresh();
    m_gridMiddle->ForceRefresh();
    m_gridRight->ForceRefresh();
    Update();   //show changes resulting from ForceRefresh()

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


void MainDialog::OnFilterButton(wxCommandEvent &event)
{
    //make sure, button-appearance and "m_checkBoxActivateFilter" are in sync.
    updateFilterButtons();

    updateFilterConfig(); //refresh filtering
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
    FilterDlg* filterDlg = new FilterDlg(this,
                                         true, //is main filter dialog
                                         currentCfg.mainCfg.includeFilter,
                                         currentCfg.mainCfg.excludeFilter,
                                         m_checkBoxActivateFilter->GetValue());
    if (filterDlg->ShowModal() == FilterDlg::BUTTON_APPLY)
    {
        updateFilterButtons(); //refresh global filter icon
        updateFilterConfig();  //re-apply filter
    }

    //event.Skip()
}


void MainDialog::OnLeftOnlyFiles(wxCommandEvent& event)
{
    m_bpButtonLeftOnly->toggle();
    updateGuiGrid();
};


void MainDialog::OnLeftNewerFiles(wxCommandEvent& event)
{
    m_bpButtonLeftNewer->toggle();
    updateGuiGrid();
};


void MainDialog::OnDifferentFiles(wxCommandEvent& event)
{
    m_bpButtonDifferent->toggle();
    updateGuiGrid();
};


void MainDialog::OnRightNewerFiles(wxCommandEvent& event)
{
    m_bpButtonRightNewer->toggle();
    updateGuiGrid();
};


void MainDialog::OnRightOnlyFiles(wxCommandEvent& event)
{
    m_bpButtonRightOnly->toggle();
    updateGuiGrid();
};


void MainDialog::OnEqualFiles(wxCommandEvent& event)
{
    m_bpButtonEqual->toggle();
    updateGuiGrid();
};


void MainDialog::OnConflictFiles(wxCommandEvent& event)
{
    m_bpButtonConflict->toggle();
    updateGuiGrid();
};


void MainDialog::OnSyncCreateLeft(wxCommandEvent& event)
{
    m_bpButtonSyncCreateLeft->toggle();
    updateGuiGrid();
};


void MainDialog::OnSyncCreateRight(wxCommandEvent& event)
{
    m_bpButtonSyncCreateRight->toggle();
    updateGuiGrid();
};


void MainDialog::OnSyncDeleteLeft(wxCommandEvent& event)
{
    m_bpButtonSyncDeleteLeft->toggle();
    updateGuiGrid();
};


void MainDialog::OnSyncDeleteRight(wxCommandEvent& event)
{
    m_bpButtonSyncDeleteRight->toggle();
    updateGuiGrid();
};


void MainDialog::OnSyncDirLeft(wxCommandEvent& event)
{
    m_bpButtonSyncDirOverwLeft->toggle();
    updateGuiGrid();
};


void MainDialog::OnSyncDirRight(wxCommandEvent& event)
{
    m_bpButtonSyncDirOverwRight->toggle();
    updateGuiGrid();
};


void MainDialog::OnSyncDirNone(wxCommandEvent& event)
{
    m_bpButtonSyncDirNone->toggle();
    updateGuiGrid();
};


void MainDialog::initViewFilterButtons()
{
    //compare result buttons
    m_bpButtonLeftOnly->init(*GlobalResources::getInstance().bitmapLeftOnlyAct,
                             _("Hide files that exist on left side only"),
                             *GlobalResources::getInstance().bitmapLeftOnlyDeact,
                             _("Show files that exist on left side only"));

    m_bpButtonRightOnly->init(*GlobalResources::getInstance().bitmapRightOnlyAct,
                              _("Hide files that exist on right side only"),
                              *GlobalResources::getInstance().bitmapRightOnlyDeact,
                              _("Show files that exist on right side only"));

    m_bpButtonLeftNewer->init(*GlobalResources::getInstance().bitmapLeftNewerAct,
                              _("Hide files that are newer on left"),
                              *GlobalResources::getInstance().bitmapLeftNewerDeact,
                              _("Show files that are newer on left"));

    m_bpButtonRightNewer->init(*GlobalResources::getInstance().bitmapRightNewerAct,
                               _("Hide files that are newer on right"),
                               *GlobalResources::getInstance().bitmapRightNewerDeact,
                               _("Show files that are newer on right"));

    m_bpButtonEqual->init(*GlobalResources::getInstance().bitmapEqualAct,
                          _("Hide files that are equal"),
                          *GlobalResources::getInstance().bitmapEqualDeact,
                          _("Show files that are equal"));

    m_bpButtonDifferent->init(*GlobalResources::getInstance().bitmapDifferentAct,
                              _("Hide files that are different"),
                              *GlobalResources::getInstance().bitmapDifferentDeact,
                              _("Show files that are different"));

    m_bpButtonConflict->init(*GlobalResources::getInstance().bitmapConflictAct,
                             _("Hide conflicts"),
                             *GlobalResources::getInstance().bitmapConflictDeact,
                             _("Show conflicts"));

    //sync preview buttons
    m_bpButtonSyncCreateLeft->init(*GlobalResources::getInstance().bitmapSyncCreateLeftAct,
                                   _("Hide files that will be created on the left side"),
                                   *GlobalResources::getInstance().bitmapSyncCreateLeftDeact,
                                   _("Show files that will be created on the left side"));

    m_bpButtonSyncCreateRight->init(*GlobalResources::getInstance().bitmapSyncCreateRightAct,
                                    _("Hide files that will be created on the right side"),
                                    *GlobalResources::getInstance().bitmapSyncCreateRightDeact,
                                    _("Show files that will be created on the right side"));

    m_bpButtonSyncDeleteLeft->init(*GlobalResources::getInstance().bitmapSyncDeleteLeftAct,
                                   _("Hide files that will be deleted on the left side"),
                                   *GlobalResources::getInstance().bitmapSyncDeleteLeftDeact,
                                   _("Show files that will be deleted on the left side"));

    m_bpButtonSyncDeleteRight->init(*GlobalResources::getInstance().bitmapSyncDeleteRightAct,
                                    _("Hide files that will be deleted on the right side"),
                                    *GlobalResources::getInstance().bitmapSyncDeleteRightDeact,
                                    _("Show files that will be deleted on the right side"));

    m_bpButtonSyncDirOverwLeft->init(*GlobalResources::getInstance().bitmapSyncDirLeftAct,
                                     _("Hide files that will be overwritten on left side"),
                                     *GlobalResources::getInstance().bitmapSyncDirLeftDeact,
                                     _("Show files that will be overwritten on left side"));

    m_bpButtonSyncDirOverwRight->init(*GlobalResources::getInstance().bitmapSyncDirRightAct,
                                      _("Hide files that will be overwritten on right side"),
                                      *GlobalResources::getInstance().bitmapSyncDirRightDeact,
                                      _("Show files that will be overwritten on right side"));

    m_bpButtonSyncDirNone->init(*GlobalResources::getInstance().bitmapSyncDirNoneAct,
                                _("Hide files that won't be copied"),
                                *GlobalResources::getInstance().bitmapSyncDirNoneDeact,
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
        panelIcons->Add(wxBitmap(*GlobalResources::getInstance().bitmapFilterSmall));
        panelIcons->Add(wxBitmap(*GlobalResources::getInstance().bitmapFilterSmallGrey));
        m_notebookBottomLeft->AssignImageList(panelIcons); //pass ownership
    }

    //global filter
    if (m_checkBoxActivateFilter->GetValue()) //filter active?
    {
        //test for Null-filter
        const bool isNullFilter = NameFilter(currentCfg.mainCfg.includeFilter, currentCfg.mainCfg.excludeFilter).isNull();
        if (isNullFilter)
        {
            m_bpButtonFilter->SetBitmapLabel(*GlobalResources::getInstance().bitmapFilterOff);
            m_bpButtonFilter->SetToolTip(_("No filter selected"));

            //additional filter icon
            m_notebookBottomLeft->SetPageImage(1, 1);
        }
        else
        {
            m_bpButtonFilter->SetBitmapLabel(*GlobalResources::getInstance().bitmapFilterOn);
            m_bpButtonFilter->SetToolTip(_("Filter has been selected"));

            //show filter icon
            m_notebookBottomLeft->SetPageImage(1, 0);
        }
    }
    else
    {
        m_bpButtonFilter->SetBitmapLabel(*GlobalResources::getInstance().bitmapFilterOff);
        m_bpButtonFilter->SetToolTip(_("Filtering is deactivated"));

        //additional filter icon
        m_notebookBottomLeft->SetPageImage(1, 1);
    }

    //update main local filter
    firstFolderPair->refreshButtons();

    //update folder pairs
    for (std::vector<FolderPairPanel*>::const_iterator i = additionalFolderPairs.begin(); i != additionalFolderPairs.end(); ++i)
    {
        FolderPairPanel* dirPair = *i;
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
        FreeFileSync::CompareProcess comparison(currentCfg.mainCfg.hidden.traverseDirectorySymlinks,
                                                currentCfg.mainCfg.hidden.fileTimeTolerance,
                                                globalSettings.ignoreOneHourDiff,
                                                globalSettings.optDialogs,
                                                &statusHandler);

        //technical representation of comparison data
        FreeFileSync::FolderComparison newCompareData;

        comparison.startCompareProcess(
            FreeFileSync::extractCompareCfg(getCurrentConfiguration().mainCfg), //call getCurrentCfg() to get current values for directory pairs!
            currentCfg.mainCfg.compareVar,
            newCompareData);

        gridDataView->setData(newCompareData); //newCompareData is invalidated after this call
    }
    catch (AbortThisProcess&)
    {
        aborted = true;
    }

    if (aborted)
    {
        //disable the sync button
        syncPreview.enableSynchronization(false);
        m_buttonCompare->SetFocus();
        updateGuiGrid(); //refresh grid in ANY case! (also on abort)
    }
    else
    {
        //once compare is finished enable the sync button
        syncPreview.enableSynchronization(true);
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

        if (allElementsEqual(gridDataView->getDataTentative()))
            pushStatusInformation(_("All directories in sync!"));
    }
}


void MainDialog::updateGuiGrid()
{
    m_gridLeft  ->BeginBatch(); //necessary??
    m_gridMiddle->BeginBatch();
    m_gridRight ->BeginBatch();

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
        const unsigned int digitWidth = 8;
#elif defined FFS_LINUX
        const unsigned int digitWidth = 10;
#endif
        const unsigned int nrOfDigits = globalFunctions::getDigitCount(static_cast<unsigned int>(nrOfRows));
        m_gridLeft ->SetRowLabelSize(nrOfDigits * digitWidth + 4);
        m_gridRight->SetRowLabelSize(nrOfDigits * digitWidth + 4);
    }

    //support for column auto adjustment
    if (globalSettings.gui.autoAdjustColumnsLeft)
        m_gridLeft->autoSizeColumns();
    if (globalSettings.gui.autoAdjustColumnsRight)
        m_gridRight->autoSizeColumns();

    //update sync preview statistics
    calculatePreview();

    m_gridLeft  ->EndBatch();
    m_gridMiddle->EndBatch();
    m_gridRight ->EndBatch();
}


void MainDialog::calculatePreview()
{
    //update preview of bytes to be transferred:
    const SyncStatistics st(gridDataView->getDataTentative());
    const wxString toCreate = FreeFileSync::includeNumberSeparator(globalFunctions::numberToWxString(st.getCreate()));
    const wxString toUpdate = FreeFileSync::includeNumberSeparator(globalFunctions::numberToWxString(st.getOverwrite()));
    const wxString toDelete = FreeFileSync::includeNumberSeparator(globalFunctions::numberToWxString(st.getDelete()));
    const wxString data     = FreeFileSync::formatFilesizeToShortString(st.getDataToProcess());

    m_textCtrlCreate->SetValue(toCreate);
    m_textCtrlUpdate->SetValue(toUpdate);
    m_textCtrlDelete->SetValue(toDelete);
    m_textCtrlData->SetValue(data);
}


void MainDialog::OnSwitchView(wxCommandEvent& event)
{
    //toggle view
    syncPreview.enablePreview(!syncPreview.previewIsEnabled());
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
    {
        updateSyncConfig();
    }
}


void MainDialog::OnCmpSettings(wxCommandEvent& event)
{
    CompareVariant newCmpVariant = currentCfg.mainCfg.compareVar;

    //show window right next to the compare-config button
    wxPoint windowPos = m_bpButtonCmpConfig->GetScreenPosition();
    windowPos.x += m_bpButtonCmpConfig->GetSize().GetWidth() + 5;

    CompareCfgDialog* syncDlg = new CompareCfgDialog(this, windowPos, newCmpVariant);
    if (syncDlg->ShowModal() == CompareCfgDialog::BUTTON_OKAY)
    {
        if (currentCfg.mainCfg.compareVar != newCmpVariant)
        {
            currentCfg.mainCfg.compareVar = newCmpVariant;

            //update compare variant name
            m_staticTextCmpVariant->SetLabel(wxString(wxT("(")) + getVariantName(currentCfg.mainCfg.compareVar) + wxT(")"));
            bSizer6->Layout(); //adapt layout for variant text

            //disable the sync button
            syncPreview.enableSynchronization(false);

            //clear grids
            gridDataView->clearAllRows();
            updateGuiGrid();

            m_buttonCompare->SetFocus();
        }
    }
}


void MainDialog::OnStartSync(wxCommandEvent& event)
{
    if (!syncPreview.synchronizationIsEnabled())
    {
        pushStatusInformation(_("Please run a Compare first before synchronizing!"));
        return;
    }

    //show sync preview screen
    if (globalSettings.optDialogs.showSummaryBeforeSync)
    {
        bool dontShowAgain = false;

        SyncPreviewDlg* preview = new SyncPreviewDlg(
            this,
            getCurrentConfiguration().mainCfg.getSyncVariantName(),
            FreeFileSync::SyncStatistics(gridDataView->getDataTentative()),
            dontShowAgain);

        if (preview->ShowModal() != SyncPreviewDlg::BUTTON_START)
            return;

        globalSettings.optDialogs.showSummaryBeforeSync = !dontShowAgain;
    }

    //check if there are files/folders to be sync'ed at all
    if (!synchronizationNeeded(gridDataView->getDataTentative()))
    {
        wxMessageBox(_("Nothing to synchronize according to configuration!"), _("Information"), wxICON_WARNING);
        return;
    }

    wxBusyCursor dummy; //show hourglass cursor

    clearStatusBar();
    try
    {
        //PERF_START;

        //class handling status updates and error messages
        SyncStatusHandler statusHandler(this, currentCfg.ignoreErrors);

        //start synchronization and mark all elements processed
        FreeFileSync::SyncProcess synchronization(
            currentCfg.mainCfg.hidden.copyFileSymlinks,
            currentCfg.mainCfg.hidden.traverseDirectorySymlinks,
            globalSettings.optDialogs,
            currentCfg.mainCfg.hidden.verifyFileCopy,
            globalSettings.copyLockedFiles,
            statusHandler);

        const std::vector<FreeFileSync::FolderPairSyncCfg> syncProcessCfg = FreeFileSync::extractSyncCfg(getCurrentConfiguration().mainCfg);
        FolderComparison& dataToSync = gridDataView->getDataTentative();

        //make sure syncProcessCfg and dataToSync have same size and correspond!
        if (syncProcessCfg.size() != dataToSync.size())
            throw std::logic_error("Programming Error: Contract violation!"); //should never happen: sync button is deactivated if they are not in sync

        synchronization.startSynchronizationProcess(syncProcessCfg, dataToSync);

        //play (optional) sound notification after sync has completed (GUI and batch mode)
        const wxString soundFile = FreeFileSync::getInstallationDir() + wxT("Sync_Complete.wav");
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
    if (!globalSettings.gui.externelApplications.empty())
        openExternalApplication(event.GetRow(), true, globalSettings.gui.externelApplications[0].second);
    //  event.Skip();
}


void MainDialog::OnRightGridDoubleClick(wxGridEvent& event)
{
    if (!globalSettings.gui.externelApplications.empty())
        openExternalApplication(event.GetRow(), false, globalSettings.gui.externelApplications[0].second);
//    event.Skip();
}


void MainDialog::OnSortLeftGrid(wxGridEvent& event)
{
    //determine direction for std::sort()
    const int currentSortColumn = event.GetCol();
    if (0 <= currentSortColumn && currentSortColumn < int(xmlAccess::COLUMN_TYPE_COUNT))
    {
        static bool sortAscending = true;
        if (lastSortColumn != currentSortColumn || lastSortGrid != m_gridLeft)
            sortAscending = true;
        else
            sortAscending = !sortAscending;

        lastSortColumn = currentSortColumn;
        lastSortGrid   = m_gridLeft;

        //start sort
        const xmlAccess::ColumnTypes columnType = m_gridLeft->getTypeAtPos(currentSortColumn);
        switch (columnType)
        {
        case xmlAccess::FULL_PATH:
            gridDataView->sortView(GridView::SORT_BY_REL_NAME, true, sortAscending);
            break;
        case xmlAccess::FILENAME:
            gridDataView->sortView(GridView::SORT_BY_FILENAME, true, sortAscending);
            break;
        case xmlAccess::REL_PATH:
            gridDataView->sortView(GridView::SORT_BY_REL_NAME, true, sortAscending);
            break;
        case xmlAccess::DIRECTORY:
            gridDataView->sortView(GridView::SORT_BY_DIRECTORY, true, sortAscending);
            break;
        case xmlAccess::SIZE:
            gridDataView->sortView(GridView::SORT_BY_FILESIZE, true, sortAscending);
            break;
        case xmlAccess::DATE:
            gridDataView->sortView(GridView::SORT_BY_DATE, true, sortAscending);
            break;
        }

        updateGuiGrid(); //refresh gridDataView

        //set sort direction indicator on UI
        m_gridMiddle->setSortMarker(CustomGrid::SortMarker(-1, CustomGrid::ASCENDING));
        m_gridRight->setSortMarker(CustomGrid::SortMarker(-1, CustomGrid::ASCENDING));
        m_gridLeft->setSortMarker(CustomGrid::SortMarker(currentSortColumn, sortAscending ? CustomGrid::ASCENDING : CustomGrid::DESCENDING));
    }
}


void MainDialog::OnSortMiddleGrid(wxGridEvent& event)
{
    //determine direction for std::sort()
    static bool sortAscending = true;
    if (lastSortColumn != 0 || lastSortGrid != m_gridMiddle)
        sortAscending = true;
    else
        sortAscending = !sortAscending;
    lastSortColumn = 0;
    lastSortGrid   = m_gridMiddle;

    //start sort
    if (syncPreview.previewIsEnabled())
        gridDataView->sortView(GridView::SORT_BY_SYNC_DIRECTION, true, sortAscending);
    else
        gridDataView->sortView(GridView::SORT_BY_CMP_RESULT, true, sortAscending);

    updateGuiGrid(); //refresh gridDataView

    //set sort direction indicator on UI
    m_gridLeft->setSortMarker(CustomGrid::SortMarker(-1, CustomGrid::ASCENDING));
    m_gridRight->setSortMarker(CustomGrid::SortMarker(-1, CustomGrid::ASCENDING));
    m_gridMiddle->setSortMarker(CustomGrid::SortMarker(0, sortAscending ? CustomGrid::ASCENDING : CustomGrid::DESCENDING));
}


void MainDialog::OnSortRightGrid(wxGridEvent& event)
{
    //determine direction for std::sort()
    const int currentSortColumn = event.GetCol();
    if (0 <= currentSortColumn && currentSortColumn < int(xmlAccess::COLUMN_TYPE_COUNT))
    {
        static bool sortAscending = true;
        if (lastSortColumn != currentSortColumn || lastSortGrid != m_gridRight)
            sortAscending = true;
        else
            sortAscending = !sortAscending;

        lastSortColumn = currentSortColumn;
        lastSortGrid   = m_gridRight;

        //start sort
        const xmlAccess::ColumnTypes columnType = m_gridRight->getTypeAtPos(currentSortColumn);
        switch (columnType)
        {
        case xmlAccess::FULL_PATH:
            gridDataView->sortView(GridView::SORT_BY_REL_NAME, false, sortAscending);
            break;
        case xmlAccess::FILENAME:
            gridDataView->sortView(GridView::SORT_BY_FILENAME, false, sortAscending);
            break;
        case xmlAccess::REL_PATH:
            gridDataView->sortView(GridView::SORT_BY_REL_NAME, false, sortAscending);
            break;
        case xmlAccess::DIRECTORY:
            gridDataView->sortView(GridView::SORT_BY_DIRECTORY, false, sortAscending);
            break;
        case xmlAccess::SIZE:
            gridDataView->sortView(GridView::SORT_BY_FILESIZE, false, sortAscending);
            break;
        case xmlAccess::DATE:
            gridDataView->sortView(GridView::SORT_BY_DATE, false, sortAscending);
            break;
        }

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
    for (std::vector<FolderPairPanel*>::const_iterator i = additionalFolderPairs.begin(); i != additionalFolderPairs.end(); ++i)
    {
        FolderPairPanel* dirPair = *i;
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
    FreeFileSync::swapGrids(getCurrentConfiguration().mainCfg, gridDataView->getDataTentative());
    updateGuiGrid();
}


void MainDialog::updateGridViewData()
{
    unsigned int filesOnLeftView    = 0;
    unsigned int foldersOnLeftView  = 0;
    unsigned int filesOnRightView   = 0;
    unsigned int foldersOnRightView = 0;
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



    if (syncPreview.previewIsEnabled())
    {
        const GridView::StatusSyncPreview result = gridDataView->updateSyncPreview(currentCfg.hideFilteredElements,
                m_bpButtonSyncCreateLeft->   isActive(),
                m_bpButtonSyncCreateRight->  isActive(),
                m_bpButtonSyncDeleteLeft->   isActive(),
                m_bpButtonSyncDeleteRight->  isActive(),
                m_bpButtonSyncDirOverwLeft-> isActive(),
                m_bpButtonSyncDirOverwRight->isActive(),
                m_bpButtonSyncDirNone->      isActive(),
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
        m_bpButtonConflict->         Show(result.existsConflict);

        if (    m_bpButtonSyncCreateLeft->   IsShown() ||
                m_bpButtonSyncCreateRight->  IsShown() ||
                m_bpButtonSyncDeleteLeft->   IsShown() ||
                m_bpButtonSyncDeleteRight->  IsShown() ||
                m_bpButtonSyncDirOverwLeft-> IsShown() ||
                m_bpButtonSyncDirOverwRight->IsShown() ||
                m_bpButtonSyncDirNone->      IsShown() ||
                m_bpButtonConflict->         IsShown())
        {
            m_panel112->Show();
            m_panel112->Layout();
        }
        else
            m_panel112->Hide();

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
            m_panel112->Show();
            m_panel112->Layout();
        }
        else
            m_panel112->Hide();
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
            wxString folderCount = FreeFileSync::includeNumberSeparator(globalFunctions::numberToWxString(foldersOnLeftView));

            wxString outputString = _("%x directories");
            outputString.Replace(wxT("%x"), folderCount, false);
            statusLeftNew += outputString;
        }

        if (filesOnLeftView)
            statusLeftNew += wxT(", ");
    }

    if (filesOnLeftView)
    {
        if (filesOnLeftView == 1)
            statusLeftNew += _("1 file,");
        else
        {
            wxString fileCount = FreeFileSync::includeNumberSeparator(globalFunctions::numberToWxString(filesOnLeftView));

            wxString outputString = _("%x files,");
            outputString.Replace(wxT("%x"), fileCount, false);
            statusLeftNew += outputString;
        }
        statusLeftNew += wxT(" ");
        statusLeftNew += FreeFileSync::formatFilesizeToShortString(filesizeLeftView);
    }

    const wxString objectsView = FreeFileSync::includeNumberSeparator(
                                     globalFunctions::numberToWxString(static_cast<unsigned int>(gridDataView->rowsOnView())));
    if (gridDataView->rowsTotal() == 1)
    {
        wxString outputString = _("%x of 1 row in view");
        outputString.Replace(wxT("%x"), objectsView, false);
        statusMiddleNew = outputString;
    }
    else
    {
        const wxString objectsTotal = FreeFileSync::includeNumberSeparator(
                                          globalFunctions::numberToWxString(static_cast<unsigned int>(gridDataView->rowsTotal())));

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
            wxString folderCount = FreeFileSync::includeNumberSeparator(globalFunctions::numberToWxString(foldersOnRightView));

            wxString outputString = _("%x directories");
            outputString.Replace(wxT("%x"), folderCount, false);
            statusRightNew += outputString;
        }

        if (filesOnRightView)
            statusRightNew += wxT(", ");
    }

    if (filesOnRightView)
    {
        if (filesOnRightView == 1)
            statusRightNew += _("1 file,");
        else
        {
            wxString fileCount = FreeFileSync::includeNumberSeparator(globalFunctions::numberToWxString(filesOnRightView));

            wxString outputString = _("%x files,");
            outputString.Replace(wxT("%x"), fileCount, false);
            statusRightNew += outputString;
        }

        statusRightNew += wxT(" ");
        statusRightNew += FreeFileSync::formatFilesizeToShortString(filesizeRightView);
    }


    //avoid screen flicker
    if (m_staticTextStatusLeft->GetLabel() != statusLeftNew)
        m_staticTextStatusLeft->SetLabel(statusLeftNew);
    if (m_staticTextStatusMiddle->GetLabel() != statusMiddleNew)
        m_staticTextStatusMiddle->SetLabel(statusMiddleNew);
    if (m_staticTextStatusRight->GetLabel() != statusRightNew)
        m_staticTextStatusRight->SetLabel(statusRightNew);

    m_panel7->Layout();
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
    syncPreview.enableSynchronization(false);

    //clear grids
    gridDataView->clearAllRows();
    updateSyncConfig(); //mainly to update sync dir description text
}


void MainDialog::updateFilterConfig()
{
    if (m_checkBoxActivateFilter->GetValue())
    {
        applyFiltering(getCurrentConfiguration().mainCfg, gridDataView->getDataTentative());
        refreshGridAfterFilterChange(400);
    }
    else
    {
        FreeFileSync::setActiveStatus(true, gridDataView->getDataTentative());
        refreshGridAfterFilterChange(0);
    }
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
    } redetCallback(globalSettings.optDialogs.warningSyncDatabase, this);

    FreeFileSync::redetermineSyncDirection(getCurrentConfiguration().mainCfg, gridDataView->getDataTentative(), &redetCallback);
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
        syncPreview.enableSynchronization(false);

        //clear grids
        gridDataView->clearAllRows();
        updateSyncConfig(); //mainly to update sync dir description text
    }
}


void MainDialog::OnRemoveFolderPair(wxCommandEvent& event)
{
    const wxObject* const eventObj = event.GetEventObject(); //find folder pair originating the event
    for (std::vector<FolderPairPanel*>::const_iterator i = additionalFolderPairs.begin(); i != additionalFolderPairs.end(); ++i)
        if (eventObj == (*i)->m_bpButtonRemovePair)
        {
            removeAddFolderPair(i - additionalFolderPairs.begin());

//------------------------------------------------------------------
            //disable the sync button
            syncPreview.enableSynchronization(false);

            //clear grids
            gridDataView->clearAllRows();
            updateSyncConfig(); //mainly to update sync dir description text
            return;
        }
}


const size_t MAX_ADD_FOLDER_PAIRS = 5;


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

        m_bpButtonSwapSides->SetBitmapLabel(*GlobalResources::getInstance().bitmapSwap);
    }
    else
    {
        m_bpButtonLocalFilter->Show();
        m_bpButtonAltSyncCfg->Show();

        m_bpButtonSwapSides->SetBitmapLabel(*GlobalResources::getInstance().bitmapSwapSlim);
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
            FolderPairPanel* newPair = new FolderPairPanel(m_scrolledWindowFolderPairs, *this);

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
        const int visiblePairs = std::min(additionalFolderPairs.size(), MAX_ADD_FOLDER_PAIRS); //up to MAX_ADD_FOLDER_PAIRS additional pairs shall be shown
        m_scrolledWindowFolderPairs->SetMinSize(wxSize( -1, pairHeight * visiblePairs));

        //update controls
        m_scrolledWindowFolderPairs->Fit();    //adjust scrolled window size
        m_scrolledWindowFolderPairs->Layout(); //adjust stuff inside scrolled window
        bSizer1->Layout();
    }

    updateGuiForFolderPair();
}


void MainDialog::removeAddFolderPair(const unsigned int pos)
{
    wxWindowUpdateLocker dummy(this); //avoid display distortion

    if (pos < additionalFolderPairs.size())
    {
        //remove folder pairs from window
        FolderPairPanel* pairToDelete = additionalFolderPairs[pos];
        const int pairHeight = pairToDelete->GetSize().GetHeight();

        bSizerAddFolderPairs->Detach(pairToDelete); //Remove() does not work on Window*, so do it manually
        pairToDelete->Destroy();                 //
        additionalFolderPairs.erase(additionalFolderPairs.begin() + pos); //remove element from vector

        //set size of scrolled window
        const size_t additionalRows = additionalFolderPairs.size();
        if (additionalRows <= MAX_ADD_FOLDER_PAIRS) //up to MAX_ADD_FOLDER_PAIRS additional pairs shall be shown
            m_scrolledWindowFolderPairs->SetMinSize(wxSize(-1, pairHeight * additionalRows));

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
    wxDialog* settingsDlg = new GlobalSettingsDlg(this, globalSettings);
    settingsDlg->ShowModal();

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
        if (FreeFileSync::fileExists(wxToZ(newFileName)))
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
        if (syncPreview.previewIsEnabled())
        {
            exportString += wxString(wxT("\"")) + getDescription(SO_CREATE_NEW_LEFT)     + wxT("\";") + getSymbol(SO_CREATE_NEW_LEFT)     + wxT('\n');
            exportString += wxString(wxT("\"")) + getDescription(SO_CREATE_NEW_RIGHT)    + wxT("\";") + getSymbol(SO_CREATE_NEW_RIGHT)    + wxT('\n');
            exportString += wxString(wxT("\"")) + getDescription(SO_DELETE_LEFT)         + wxT("\";") + getSymbol(SO_DELETE_LEFT)         + wxT('\n');
            exportString += wxString(wxT("\"")) + getDescription(SO_DELETE_RIGHT)        + wxT("\";") + getSymbol(SO_DELETE_RIGHT)        + wxT('\n');
            exportString += wxString(wxT("\"")) + getDescription(SO_OVERWRITE_LEFT)      + wxT("\";") + getSymbol(SO_OVERWRITE_LEFT)      + wxT('\n');
            exportString += wxString(wxT("\"")) + getDescription(SO_OVERWRITE_RIGHT)     + wxT("\";") + getSymbol(SO_OVERWRITE_RIGHT)     + wxT('\n');
            exportString += wxString(wxT("\"")) + getDescription(SO_DO_NOTHING)          + wxT("\";") + getSymbol(SO_DO_NOTHING)          + wxT('\n');
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
    xmlAccess::XmlGuiConfig currCfg = getCurrentConfiguration(); //get UP TO DATE config, with updated values for main and additional folders!

    xmlAccess::XmlBatchConfig batchCfg;
    batchCfg.mainCfg = currCfg.mainCfg;

    if (currentCfg.ignoreErrors)
        batchCfg.handleError = xmlAccess::ON_ERROR_IGNORE;
    else
        batchCfg.handleError = xmlAccess::ON_ERROR_POPUP;

    BatchDialog* batchDlg = new BatchDialog(this, batchCfg);
    if (batchDlg->ShowModal() == BatchDialog::BATCH_FILE_SAVED)
        pushStatusInformation(_("Batch file created successfully!"));
}


void MainDialog::OnMenuCheckVersion(wxCommandEvent& event)
{
    FreeFileSync::checkForUpdateNow();
}


void MainDialog::OnRegularUpdateCheck(wxIdleEvent& event)
{
    //execute just once per startup!
    Disconnect(wxEVT_IDLE, wxIdleEventHandler(MainDialog::OnRegularUpdateCheck), NULL, this);

    FreeFileSync::checkForUpdatePeriodically(globalSettings.lastUpdateCheck);
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
    AboutDlg* aboutDlg = new AboutDlg(this);
    aboutDlg->ShowModal();
}



void MainDialog::OnShowHelp(wxCommandEvent& event)
{
    helpController_.DisplayContents();
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
    CustomLocale::getInstance().setLanguage(langID); //language is a global attribute

    //create new main window and delete old one
    cleanUp(); //destructor's code: includes writing settings to HD

    //create new dialog with respect to new language
    MainDialog* frame = new MainDialog(NULL, wxEmptyString, globalSettings, helpController_);
    frame->SetIcon(*GlobalResources::getInstance().programIcon); //set application icon
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
        mainDlg_->m_buttonStartSync->setBitmapFront(*GlobalResources::getInstance().bitmapSync);
    }
    else
    {
        synchronizationEnabled = false;
        mainDlg_->m_buttonStartSync->SetForegroundColour(wxColor(128, 128, 128)); //Some colors seem to have problems with 16Bit color depth, well this one hasn't!
        mainDlg_->m_buttonStartSync->setBitmapFront(*GlobalResources::getInstance().bitmapSyncDisabled);
    }
}


bool MainDialog::SyncPreview::synchronizationIsEnabled() const
{
    return synchronizationEnabled;
}




