#include "mainDialog.h"
#include <wx/filename.h>
#include "../library/globalFunctions.h"
#include <wx/clipbrd.h>
#include <wx/dataobj.h>
#include <wx/ffile.h>
#include "../library/customGrid.h"
#include "../library/customButton.h"
#include <algorithm>
#include <wx/msgdlg.h>
#include "../comparison.h"
#include "../synchronization.h"
#include "../algorithm.h"
#include "checkVersion.h"
#include "guiStatusHandler.h"
#include "syncDialog.h"
#include "../library/localization.h"
#include "smallDialogs.h"
#include "dragAndDrop.h"
#include "../library/filter.h"
#include "../structures.h"
#include <wx/imaglist.h>

using namespace FreeFileSync;


class FolderPairPanel : public FolderPairGenerated
{
public:
    FolderPairPanel(wxWindow* parent) :
            FolderPairGenerated(parent),
            dragDropOnLeft(new DragDropOnDlg(m_panelLeft, m_dirPickerLeft, m_directoryLeft)),
            dragDropOnRight(new DragDropOnDlg(m_panelRight, m_dirPickerRight, m_directoryRight)) {}

private:
    //support for drag and drop
    std::auto_ptr<DragDropOnDlg> dragDropOnLeft;
    std::auto_ptr<DragDropOnDlg> dragDropOnRight;
};


class MainFolderDragDrop : public DragDropOnMainDlg
{
public:
    MainFolderDragDrop(MainDialog*      mainDlg,
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
            mainDlg_->loadConfiguration(dropName);
            return false;
        }
        //...or a ffs batch file
        else if (fileType == xmlAccess::XML_BATCH_CONFIG)
        {
            BatchDialog* batchDlg = new BatchDialog(mainDlg_, dropName);
            if (batchDlg->ShowModal() == BatchDialog::BATCH_FILE_SAVED)
                mainDlg_->pushStatusInformation(_("Batch file created successfully!"));
            return false;
        }

        //disable the sync button
        mainDlg_->syncPreview->enableSynchronization(false);

        //clear grids
        mainDlg_->currentGridData.clear();
        mainDlg_->updateGuiGrid();

        return true;
    }

private:
    MainDialog* mainDlg_;
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


MainDialog::MainDialog(wxFrame* frame, const wxString& cfgFileName, CustomLocale* language, xmlAccess::XmlGlobalSettings& settings) :
        MainDialogGenerated(frame),
        globalSettings(settings),
        gridDataView(currentGridData),
        contextMenu(new wxMenu), //initialize right-click context menu; will be dynamically re-created on each R-mouse-click
        programLanguage(language),
        cmpStatusHandlerTmp(0),
        cleanedUp(false),
        lastSortColumn(-1),
        lastSortGrid(NULL),
#ifdef FFS_WIN
        updateFileIcons(new IconUpdater(m_gridLeft, m_gridRight)),
#endif
        syncPreview(new SyncPreview(this))
{
    m_bpButtonRemoveTopPair->Hide();


    //initialize and load configuration
    readGlobalSettings();
    readConfigurationFromXml(cfgFileName, true);

    //set icons for this dialog
    m_bpButton10->SetBitmapLabel(*GlobalResources::getInstance().bitmapExit);
    m_bpButtonSwapSides->SetBitmapLabel(*GlobalResources::getInstance().bitmapSwap);
    m_buttonCompare->setBitmapFront(*GlobalResources::getInstance().bitmapCompare);
    m_bpButtonSyncConfig->SetBitmapLabel(*GlobalResources::getInstance().bitmapSyncCfg);
    m_bpButton14->SetBitmapLabel(*GlobalResources::getInstance().bitmapHelp);
    m_bpButtonSave->SetBitmapLabel(*GlobalResources::getInstance().bitmapSave);
    m_bpButtonLoad->SetBitmapLabel(*GlobalResources::getInstance().bitmapLoad);
    m_bpButtonAddPair->SetBitmapLabel(*GlobalResources::getInstance().bitmapAddFolderPair);
    m_bpButtonRemoveTopPair->SetBitmapLabel(*GlobalResources::getInstance().bitmapRemoveFolderPair);
    m_bitmap15->SetBitmap(*GlobalResources::getInstance().bitmapStatusEdge);
    m_bitmapShift->SetBitmap(*GlobalResources::getInstance().bitmapShift);

    m_bitmapCreate->SetBitmap(*GlobalResources::getInstance().bitmapCreate);
    m_bitmapUpdate->SetBitmap(*GlobalResources::getInstance().bitmapUpdate);
    m_bitmapDelete->SetBitmap(*GlobalResources::getInstance().bitmapDelete);
    m_bitmapData->SetBitmap(*GlobalResources::getInstance().bitmapData);

    bSizer6->Layout(); //wxButtonWithImage size might have changed

    //menu icons: workaround for wxWidgets: small hack to update menu items: actually this is a wxWidgets bug (affects Windows- and Linux-build)
    MenuItemUpdater updateMenuFile(m_menuFile);
    updateMenuFile.addForUpdate(m_menuItem10, *GlobalResources::getInstance().bitmapCompareSmall);
    updateMenuFile.addForUpdate(m_menuItem11, *GlobalResources::getInstance().bitmapSyncSmall);

    MenuItemUpdater updateMenuAdv(m_menuAdvanced);
    updateMenuAdv.addForUpdate(m_menuItemGlobSett, *GlobalResources::getInstance().bitmapSettingsSmall);
    updateMenuAdv.addForUpdate(m_menuItem7, *GlobalResources::getInstance().bitmapBatchSmall);

    MenuItemUpdater updateMenuHelp(m_menuHelp);
    updateMenuHelp.addForUpdate(m_menuItemAbout, *GlobalResources::getInstance().bitmapAboutSmall);


    //create language selection menu
    for (std::vector<LocInfoLine>::const_iterator i = LocalizationInfo::getMapping().begin(); i != LocalizationInfo::getMapping().end(); ++i)
    {
        wxMenuItem* newItem = new wxMenuItem(m_menuLanguages, wxID_ANY, i->languageName, wxEmptyString, wxITEM_NORMAL );
        newItem->SetBitmap(*i->languageFlag);

        //map menu item IDs with language IDs: evaluated when processing event handler
        languageMenuItemMap.insert(std::map<MenuItemID, LanguageID>::value_type(newItem->GetId(), i->languageID));

        //connect event
        Connect(newItem->GetId(), wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(MainDialog::OnMenuLanguageSwitch));

        m_menuLanguages->Append(newItem);
    }


    //prepare drag & drop
    dragDropOnLeft.reset(new MainFolderDragDrop(
                             this,
                             m_panelLeft,
                             m_panelTopLeft,
                             m_dirPickerLeft,
                             m_directoryLeft));

    dragDropOnRight.reset(new MainFolderDragDrop(
                              this,
                              m_panelRight,
                              m_panelTopRight,
                              m_dirPickerRight,
                              m_directoryRight));

    //support for CTRL + C and DEL
    m_gridLeft->Connect(wxEVT_KEY_DOWN, wxKeyEventHandler(MainDialog::onGridLeftButtonEvent), NULL, this);
    m_gridRight->Connect(wxEVT_KEY_DOWN, wxKeyEventHandler(MainDialog::onGridRightButtonEvent), NULL, this);
    m_gridMiddle->Connect(wxEVT_KEY_DOWN, wxKeyEventHandler(MainDialog::onGridMiddleButtonEvent), NULL, this);

    Connect(wxEVT_IDLE, wxEventHandler(MainDialog::OnIdleEvent), NULL, this);
    Connect(wxEVT_SIZE, wxEventHandler(MainDialog::onResizeMainWindow), NULL, this);
    Connect(wxEVT_MOVE, wxEventHandler(MainDialog::onResizeMainWindow), NULL, this);

    //event handler for manual (un-)checking of rows and setting of sync direction
    m_gridMiddle->Connect(FFS_CHECK_ROWS_EVENT, FFSCheckRowsEventHandler(MainDialog::OnCheckRows), NULL, this);
    m_gridMiddle->Connect(FFS_SYNC_DIRECTION_EVENT, FFSSyncDirectionEventHandler(MainDialog::OnSetSyncDirection), NULL, this);

    //init grid settings
    m_gridLeft  ->initSettings(m_gridLeft, m_gridMiddle, m_gridRight, &gridDataView);
    m_gridMiddle->initSettings(m_gridLeft, m_gridMiddle, m_gridRight, &gridDataView);
    m_gridRight ->initSettings(m_gridLeft, m_gridMiddle, m_gridRight, &gridDataView);

    //disable sync button as long as "compare" hasn't been triggered.
    syncPreview->enableSynchronization(false);

    //mainly to update row label sizes...
    updateGuiGrid();

    //create the compare status panel in hidden state
    compareStatus = new CompareStatus(this);
    bSizer1->Insert(1, compareStatus, 0, wxEXPAND | wxBOTTOM, 5 );
    Layout();   //avoid screen flicker when panel is shown later
    compareStatus->Hide();

    //correct width of swap button above middle grid
    const wxSize source = m_gridMiddle->GetSize();
    const wxSize target = bSizerMiddle->GetSize();
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

        //no need for event Disconnect() here; done automatically

        //save configuration
        writeConfigurationToXml(FreeFileSync::getLastConfigFile());   //don't trow exceptions in destructors
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


void MainDialog::setSyncDirManually(const std::set<int>& rowsToSetOnUiTable, const FreeFileSync::SyncDirection dir)
{
    if (rowsToSetOnUiTable.size() > 0)
    {
        for (std::set<int>::const_iterator i = rowsToSetOnUiTable.begin(); i != rowsToSetOnUiTable.end(); ++i)
        {
            gridDataView[*i].direction = dir;
            gridDataView[*i].selectedForSynchronization = true;
        }

        updateGuiGrid();
    }
}


void MainDialog::filterRangeManually(const std::set<int>& rowsToFilterOnUiTable, const int leadingRow)
{
    if (rowsToFilterOnUiTable.size() > 0)
    {
        bool newSelection = false; //default: deselect range

        //leadingRow determines de-/selection of all other rows
        if (0 <= leadingRow && leadingRow < int(gridDataView.elementsOnView()))
            newSelection = !gridDataView[leadingRow].selectedForSynchronization;

        //if hidefiltered is active, there should be no filtered elements on screen => current element was filtered out
        assert(!hideFilteredElements || !newSelection);

        //get all lines that need to be filtered (e.g. if a folder is marked, then its subelements should be marked as well)
        FolderCompRef compRef;
        gridDataView.viewRefToFolderRef(rowsToFilterOnUiTable, compRef);

        assert(compRef.size() == currentGridData.size()); //GridView::viewRefToFolderRef() should ensure this!

        for (FolderComparison::iterator i = currentGridData.begin(); i != currentGridData.end(); ++i)
        {
            FileComparison& fileCmp = i->fileCmp;
            const std::set<int>& markedRows = compRef[i - currentGridData.begin()];

            std::set<int> markedRowsTotal; //retrieve additional rows that need to be filtered, too
            for (std::set<int>::const_iterator i = markedRows.begin(); i != markedRows.end(); ++i)
            {
                markedRowsTotal.insert(*i);
                FreeFileSync::addSubElements(fileCmp, fileCmp[*i], markedRowsTotal);
            }

            //toggle selection of filtered rows
            for (std::set<int>::iterator i = markedRowsTotal.begin(); i != markedRowsTotal.end(); ++i)
                fileCmp[*i].selectedForSynchronization = newSelection;
        }

        refreshGridAfterFilterChange(400); //call this instead of updateGuiGrid() to add some delay if hideFiltered == true and to handle some graphical artifacts
    }
}


void MainDialog::OnIdleEvent(wxEvent& event)
{
    //small routine to restore status information after some time
    if (stackObjects.size() > 0 )  //check if there is some work to do
    {
        wxLongLong currentTime = wxGetLocalTimeMillis();
        if (currentTime - lastStatusChange > 2000) //restore stackObject after two seconds
        {
            lastStatusChange = currentTime;

            m_staticTextStatusMiddle->SetLabel(stackObjects.top());
            stackObjects.pop();
            m_panel7->Layout();
        }
    }

    event.Skip();
}


void MainDialog::copySelectionToClipboard(const CustomGrid* selectedGrid)
{
    const std::set<int> selectedRows = getSelectedRows(selectedGrid);
    if (selectedRows.size() > 0)
    {
        wxString clipboardString;

        for (std::set<int>::const_iterator i = selectedRows.begin(); i != selectedRows.end(); ++i)
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


void removeInvalidRows(std::set<int>& rows, const int currentUiTableSize)
{
    std::set<int> validRows; //temporal table IS needed here
    for (std::set<int>::iterator i = rows.begin(); i != rows.end(); ++i)
        if (0 <= *i)
        {
            if (*i >= currentUiTableSize) //set is sorted, so no need to continue here
                break;
            validRows.insert(*i);
        }
    rows.swap(validRows);
}


std::set<int> MainDialog::getSelectedRows(const CustomGrid* grid) const
{
    std::set<int> output = grid->getAllSelectedRows();

    removeInvalidRows(output, gridDataView.elementsOnView());

    return output;
}


std::set<int> MainDialog::getSelectedRows() const
{
    //merge selections from left and right grid
    std::set<int> selection = getSelectedRows(m_gridLeft);
    std::set<int> additional = getSelectedRows(m_gridRight);
    for (std::set<int>::const_iterator i = additional.begin(); i != additional.end(); ++i)
        selection.insert(*i);

    return selection;
}


class DeleteErrorHandler : public ErrorHandler
{
public:
    DeleteErrorHandler() :
            ignoreErrors(false) {}

    ~DeleteErrorHandler() {}

    Response reportError(const Zstring& errorMessage)
    {
        if (ignoreErrors)
            return ErrorHandler::IGNORE_ERROR;

        bool ignoreNextErrors = false;
        ErrorDlg* errorDlg = new ErrorDlg(NULL,
                                          ErrorDlg::BUTTON_IGNORE |  ErrorDlg::BUTTON_RETRY | ErrorDlg::BUTTON_ABORT,
                                          errorMessage.c_str(), ignoreNextErrors);
        int rv = errorDlg->ShowModal();
        errorDlg->Destroy();
        switch (rv)
        {
        case ErrorDlg::BUTTON_IGNORE:
            ignoreErrors = ignoreNextErrors;
            return ErrorHandler::IGNORE_ERROR;
        case ErrorDlg::BUTTON_RETRY:
            return ErrorHandler::RETRY;
        case ErrorDlg::BUTTON_ABORT:
            throw FreeFileSync::AbortThisProcess();
        default:
            assert (false);
            return ErrorHandler::IGNORE_ERROR; //dummy return value
        }
    }

private:
    bool ignoreErrors;
};


void MainDialog::deleteSelectedFiles()
{
    //get set of selected rows on view
    const std::set<int> viewSelectionLeft  = getSelectedRows(m_gridLeft);
    const std::set<int> viewSelectionRight = getSelectedRows(m_gridRight);

    if (viewSelectionLeft.size() + viewSelectionRight.size())
    {
        //map lines from GUI view to grid line references for "currentGridData"
        FolderCompRef compRefLeft;
        gridDataView.viewRefToFolderRef(viewSelectionLeft, compRefLeft);

        FolderCompRef compRefRight;
        gridDataView.viewRefToFolderRef(viewSelectionRight, compRefRight);


        DeleteDialog* confirmDeletion = new DeleteDialog(this, //no destruction needed; attached to main window
                currentGridData,
                compRefLeft,
                compRefRight,
                globalSettings.gui.deleteOnBothSides,
                globalSettings.gui.useRecyclerForManualDeletion);
        if (confirmDeletion->ShowModal() == DeleteDialog::BUTTON_OKAY)
        {
            if (globalSettings.gui.useRecyclerForManualDeletion && !FreeFileSync::recycleBinExists())
            {
                wxMessageBox(_("Unable to initialize Recycle Bin!"));
                return;
            }

            //Attention! Modifying the grid is highly critical! There MUST NOT BE any accesses to gridDataView until this reference table is updated
            //by updateGuiGrid()!! This is easily missed, e.g. when ClearSelection() or ShowModal() or possibly any other wxWidgets function is called
            //that might want to redraw the UI (which implicitly uses the information in gridDataView (see CustomGrid)

            try
            {   //handle errors when deleting files/folders
                DeleteErrorHandler errorHandler;

                assert(compRefLeft.size() == currentGridData.size()); //GridView::viewRefToFolderRef() should ensure this!
                assert(compRefRight.size() == currentGridData.size());

                for (FolderComparison::iterator i = currentGridData.begin(); i != currentGridData.end(); ++i)
                {
                    const int folderPairNr = i - currentGridData.begin();

                    const std::set<int>& rowsToDeleteOnLeft  = compRefLeft[folderPairNr];
                    const std::set<int>& rowsToDeleteOnRight = compRefRight[folderPairNr];

                    FreeFileSync::deleteFromGridAndHD(i->fileCmp,
                                                      rowsToDeleteOnLeft,
                                                      rowsToDeleteOnRight,
                                                      globalSettings.gui.deleteOnBothSides,
                                                      globalSettings.gui.useRecyclerForManualDeletion,
                                                      cfg.syncConfiguration,
                                                      &errorHandler);
                }

            }
            catch (FreeFileSync::AbortThisProcess&) {}

            //redraw grid neccessary to update new dimensions and for UI-Backend data linkage
            updateGuiGrid(); //call immediately after deleteFromGridAndHD!!!

            m_gridLeft->ClearSelection();
            m_gridMiddle->ClearSelection();
            m_gridRight->ClearSelection();
        }
    }
}


void MainDialog::openWithFileManager(const int rowNumber, const bool leftSide)
{
    wxString command;

    if (0 <= rowNumber && rowNumber < int(gridDataView.elementsOnView()))
    {
        const FileDescrLine* fileDescr = leftSide ?
                                         &gridDataView[rowNumber].fileDescrLeft :
                                         &gridDataView[rowNumber].fileDescrRight;

        switch (fileDescr->objType)
        {
        case FileDescrLine::TYPE_FILE:
            command = globalSettings.gui.commandLineFileManager;
            command.Replace(wxT("%name"), fileDescr->fullName.c_str());
            command.Replace(wxT("%path"), wxString(fileDescr->fullName.c_str()).BeforeLast(FreeFileSync::FILE_NAME_SEPARATOR));
            break;
        case FileDescrLine::TYPE_DIRECTORY:
            command = globalSettings.gui.commandLineFileManager;
            command.Replace(wxT("%name"), fileDescr->fullName.c_str());
            command.Replace(wxT("%path"), fileDescr->fullName.c_str());
            break;
        case FileDescrLine::TYPE_NOTHING:
        {
            //use path from other side
            const FileDescrLine* fileDescrOther = leftSide ?
                                                  &gridDataView[rowNumber].fileDescrRight :
                                                  &gridDataView[rowNumber].fileDescrLeft;

            const wxString syncFolder = leftSide ?
                                        gridDataView.getFolderPair(rowNumber).leftDirectory.c_str() :
                                        gridDataView.getFolderPair(rowNumber).rightDirectory.c_str();

            wxString alternateFolder;
            switch (fileDescrOther->objType)
            {
            case FileDescrLine::TYPE_FILE:
                alternateFolder = syncFolder + wxString(fileDescrOther->relativeName.c_str()).BeforeLast(FreeFileSync::FILE_NAME_SEPARATOR);
                break;
            case FileDescrLine::TYPE_DIRECTORY:
                alternateFolder = syncFolder + wxString(fileDescrOther->relativeName.c_str());
                break;
            case FileDescrLine::TYPE_NOTHING:
                assert(false);
                break;
            }

            if (!wxDirExists(alternateFolder))
                alternateFolder = syncFolder;

#ifdef FFS_WIN
            command = wxString(wxT("explorer ")) + alternateFolder; //default
#elif defined FFS_LINUX
            command = globalSettings.gui.commandLineFileManager;
            command.Replace(wxT("%name"), alternateFolder);
            command.Replace(wxT("%path"), alternateFolder);
#endif
        }
        break;
        }
    }
    else
    {   //fallback
        const wxString defaultFolder = leftSide ?
                                       FreeFileSync::getFormattedDirectoryName(m_directoryLeft->GetValue().c_str()).c_str() :
                                       FreeFileSync::getFormattedDirectoryName(m_directoryRight->GetValue().c_str()).c_str();
#ifdef FFS_WIN
        command = wxString(wxT("explorer ")) + defaultFolder; //default
#elif defined FFS_LINUX
        command = globalSettings.gui.commandLineFileManager;
        command.Replace(wxT("%name"), defaultFolder);
        command.Replace(wxT("%path"), defaultFolder);
#endif
    }

    if (!command.empty())
        wxExecute(command.c_str());
}


void MainDialog::pushStatusInformation(const wxString& text)
{
    lastStatusChange = wxGetLocalTimeMillis();
    stackObjects.push(m_staticTextStatusMiddle->GetLabel());
    m_staticTextStatusMiddle->SetLabel(text);
    m_panel7->Layout();
}


void MainDialog::clearStatusBar()
{
    while (stackObjects.size() > 0)
        stackObjects.pop();

    m_staticTextStatusLeft->SetLabel(wxEmptyString);
    m_staticTextStatusMiddle->SetLabel(wxEmptyString);
    m_staticTextStatusRight->SetLabel(wxEmptyString);
}


void MainDialog::onResizeMainWindow(wxEvent& event)
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
        {                                                //the window is minimized (eg x,y == -32000; height = 28, width = 160)
            widthNotMaximized  = width;
            heightNotMaximized = height;

            posXNotMaximized = x;
            posYNotMaximized = y;
        }
    }
    event.Skip();
}


void MainDialog::onGridLeftButtonEvent(wxKeyEvent& event)
{
    const int keyCode = event.GetKeyCode();

    if (event.ControlDown())
    {
        if (keyCode == 67 || keyCode == WXK_INSERT) //CTRL + C || CTRL + INS
            copySelectionToClipboard(m_gridLeft);
        else if (keyCode == 65) //CTRL + A
            m_gridLeft->SelectAll();
    }
    else if (keyCode == WXK_DELETE || keyCode == WXK_NUMPAD_DELETE)
        deleteSelectedFiles();

    event.Skip();
}


void MainDialog::onGridMiddleButtonEvent(wxKeyEvent& event)
{
    const int keyCode = event.GetKeyCode();

    if (event.ControlDown())
    {
        if (keyCode == 67 || keyCode == WXK_INSERT) //CTRL + C || CTRL + INS
            copySelectionToClipboard(m_gridMiddle);
    }

    event.Skip();
}


void MainDialog::onGridRightButtonEvent(wxKeyEvent& event)
{
    const int keyCode = event.GetKeyCode();

    if (event.ControlDown())
    {
        if (keyCode == 67 || keyCode == WXK_INSERT) //CTRL + C || CTRL + INS
            copySelectionToClipboard(m_gridRight);
        else if (keyCode == 65) //CTRL + A
            m_gridRight->SelectAll();
    }
    else if (keyCode == WXK_DELETE || keyCode == WXK_NUMPAD_DELETE)
        deleteSelectedFiles();

    event.Skip();
}


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


    const std::set<int> selectionLeft  = getSelectedRows(m_gridLeft);
    const std::set<int> selectionRight = getSelectedRows(m_gridRight);

    const int selectionBegin = selectionLeft.size() + selectionRight.size() == 0 ? 0 :
                               selectionLeft.size()  == 0 ? *selectionRight.begin() :
                               selectionRight.size() == 0 ? *selectionLeft.begin()  :
                               std::min(*selectionLeft.begin(), *selectionRight.begin());

//#######################################################
    //re-create context menu
    contextMenu.reset(new wxMenu);

    if (syncPreview->previewIsEnabled())
    {
        if (selectionLeft.size() + selectionRight.size() > 0)
        {
            //CONTEXT_SYNC_DIR_LEFT
            wxMenuItem* menuItemSyncDirLeft = new wxMenuItem(contextMenu.get(), CONTEXT_SYNC_DIR_LEFT, _("Change direction"));
            menuItemSyncDirLeft->SetBitmap(getSyncOpImage(gridDataView[selectionBegin].cmpResult, true, SYNC_DIR_LEFT));
            contextMenu->Append(menuItemSyncDirLeft);

            //CONTEXT_SYNC_DIR_NONE
            wxMenuItem* menuItemSyncDirNone = new wxMenuItem(contextMenu.get(), CONTEXT_SYNC_DIR_NONE, _("Change direction"));
            menuItemSyncDirNone->SetBitmap(getSyncOpImage(gridDataView[selectionBegin].cmpResult, true, SYNC_DIR_NONE));
            contextMenu->Append(menuItemSyncDirNone);

            //CONTEXT_SYNC_DIR_RIGHT
            wxMenuItem* menuItemSyncDirRight = new wxMenuItem(contextMenu.get(), CONTEXT_SYNC_DIR_RIGHT, _("Change direction"));
            menuItemSyncDirRight->SetBitmap(getSyncOpImage(gridDataView[selectionBegin].cmpResult, true, SYNC_DIR_RIGHT));
            contextMenu->Append(menuItemSyncDirRight);

            contextMenu->AppendSeparator();
        }
    }


    //CONTEXT_FILTER_TEMP
    if (selectionLeft.size() + selectionRight.size() > 0)
    {
        if (gridDataView[selectionBegin].selectedForSynchronization) //valid access, as getSelectedRows returns valid references only
        {
            wxMenuItem* menuItemExclTemp = new wxMenuItem(contextMenu.get(), CONTEXT_FILTER_TEMP, _("Exclude temporarily"));
            menuItemExclTemp->SetBitmap(*GlobalResources::getInstance().bitmapCheckBoxFalse);
            contextMenu->Append(menuItemExclTemp);
        }
        else
        {
            wxMenuItem* menuItemInclTemp = new wxMenuItem(contextMenu.get(), CONTEXT_FILTER_TEMP, _("Include temporarily"));
            menuItemInclTemp->SetBitmap(*GlobalResources::getInstance().bitmapCheckBoxTrue);
            contextMenu->Append(menuItemInclTemp);
        }
    }
    else
    {
        contextMenu->Append(CONTEXT_FILTER_TEMP, _("Exclude temporarily")); //this element should always be visible
        contextMenu->Enable(CONTEXT_FILTER_TEMP, false);
    }

//###############################################################################################
    //get list of relative file/dir-names for filtering
    exFilterCandidateObj.clear();
    FilterObject newFilterEntry;
    for (std::set<int>::const_iterator i = selectionLeft.begin(); i != selectionLeft.end(); ++i)
    {
        const FileCompareLine& line = gridDataView[*i];
        newFilterEntry.relativeName = line.fileDescrLeft.relativeName.c_str();
        newFilterEntry.type         = line.fileDescrLeft.objType;
        if (!newFilterEntry.relativeName.IsEmpty())
            exFilterCandidateObj.push_back(newFilterEntry);
    }
    for (std::set<int>::const_iterator i = selectionRight.begin(); i != selectionRight.end(); ++i)
    {
        const FileCompareLine& line = gridDataView[*i];
        newFilterEntry.relativeName = line.fileDescrRight.relativeName.c_str();
        newFilterEntry.type         = line.fileDescrRight.objType;
        if (!newFilterEntry.relativeName.IsEmpty())
            exFilterCandidateObj.push_back(newFilterEntry);
    }
//###############################################################################################

    //CONTEXT_EXCLUDE_EXT
    exFilterCandidateExtension.clear();
    if (exFilterCandidateObj.size() > 0 && exFilterCandidateObj[0].type == FileDescrLine::TYPE_FILE)
    {
        const wxString filename = exFilterCandidateObj[0].relativeName.AfterLast(FreeFileSync::FILE_NAME_SEPARATOR);
        if (filename.Find(wxChar('.')) !=  wxNOT_FOUND) //be careful: AfterLast will return the whole string if '.' is not found!
        {
            exFilterCandidateExtension = filename.AfterLast(wxChar('.'));

            //add context menu item
            wxMenuItem* menuItemExclExt = new wxMenuItem(contextMenu.get(), CONTEXT_EXCLUDE_EXT, wxString(_("Exclude via filter:")) + wxT(" ") + wxT("*.") + exFilterCandidateExtension);
            menuItemExclExt->SetBitmap(*GlobalResources::getInstance().bitmapFilterSmall);
            contextMenu->Append(menuItemExclExt);
        }
    }


    //CONTEXT_EXCLUDE_OBJ
    wxMenuItem* menuItemExclObj = NULL;
    if (exFilterCandidateObj.size() == 1)
        menuItemExclObj = new wxMenuItem(contextMenu.get(), CONTEXT_EXCLUDE_OBJ, wxString(_("Exclude via filter:")) + wxT(" ") + exFilterCandidateObj[0].relativeName.AfterLast(FreeFileSync::FILE_NAME_SEPARATOR));
    else if (exFilterCandidateObj.size() > 1)
        menuItemExclObj = new wxMenuItem(contextMenu.get(), CONTEXT_EXCLUDE_OBJ, wxString(_("Exclude via filter:")) + wxT(" ") + _("<multiple selection>"));

    if (menuItemExclObj != NULL)
    {
        menuItemExclObj->SetBitmap(*GlobalResources::getInstance().bitmapFilterSmall);
        contextMenu->Append(menuItemExclObj);
    }


    contextMenu->AppendSeparator();


    //CONTEXT_CLIPBOARD
    contextMenu->Append(CONTEXT_CLIPBOARD, _("Copy to clipboard\tCTRL+C"));

    if (    (m_gridLeft->isLeadGrid()  && selectionLeft.size()) ||
            (m_gridRight->isLeadGrid() && selectionRight.size()))
        contextMenu->Enable(CONTEXT_CLIPBOARD, true);
    else
        contextMenu->Enable(CONTEXT_CLIPBOARD, false);


    //CONTEXT_EXPLORER
    contextMenu->Append(CONTEXT_EXPLORER, _("Open with File Manager\tD-Click"));

    if (    (m_gridLeft->isLeadGrid()  && selectionLeft.size() <= 1) ||
            (m_gridRight->isLeadGrid() && selectionRight.size() <= 1))
        contextMenu->Enable(CONTEXT_EXPLORER, true);
    else
        contextMenu->Enable(CONTEXT_EXPLORER, false);

    contextMenu->AppendSeparator();


    //CONTEXT_DELETE_FILES
    contextMenu->Append(CONTEXT_DELETE_FILES, _("Delete files\tDEL"));

    if (selectionLeft.size() + selectionRight.size() == 0)
        contextMenu->Enable(CONTEXT_DELETE_FILES, false);


//###############################################################################################

    contextMenu->Connect(wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(MainDialog::OnContextRimSelection), NULL, this);

    //show context menu
    PopupMenu(contextMenu.get());
}


void MainDialog::OnContextRimSelection(wxCommandEvent& event)
{
    const ContextIDRim eventId = static_cast<ContextIDRim>(event.GetId());
    switch (eventId)
    {
    case CONTEXT_SYNC_DIR_LEFT:
    {
        //merge selections from left and right grid
        const std::set<int> selection = getSelectedRows();
        setSyncDirManually(selection, FreeFileSync::SYNC_DIR_LEFT);
    }
    break;

    case CONTEXT_SYNC_DIR_NONE:
    {
        //merge selections from left and right grid
        const std::set<int> selection = getSelectedRows();
        setSyncDirManually(selection, FreeFileSync::SYNC_DIR_NONE);
    }
    break;

    case CONTEXT_SYNC_DIR_RIGHT:
    {
        //merge selections from left and right grid
        const std::set<int> selection = getSelectedRows();
        setSyncDirManually(selection, FreeFileSync::SYNC_DIR_RIGHT);
    }
    break;

    case CONTEXT_FILTER_TEMP:
    {
        //merge selections from left and right grid
        std::set<int> selection = getSelectedRows();
        filterRangeManually(selection, *selection.begin());
    }
    break;

    case CONTEXT_EXCLUDE_EXT:
        if (!exFilterCandidateExtension.IsEmpty())
        {
            if (!cfg.excludeFilter.IsEmpty() && !cfg.excludeFilter.EndsWith(wxT(";")))
                cfg.excludeFilter+= wxT("\n");

            cfg.excludeFilter+= wxString(wxT("*.")) + exFilterCandidateExtension + wxT(";"); //';' is appended to 'mark' that next exclude extension entry won't write to new line

            cfg.filterIsActive = true;
            updateFilterButton(m_bpButtonFilter, cfg.filterIsActive);

            FreeFileSync::FilterProcess filterInstance(cfg.includeFilter, cfg.excludeFilter);
            filterInstance.filterGridData(currentGridData);

            updateGuiGrid();
            if (hideFilteredElements)
            {
                m_gridLeft->ClearSelection();
                m_gridRight->ClearSelection();
                m_gridMiddle->ClearSelection();
            }
        }
        break;

    case CONTEXT_EXCLUDE_OBJ:
        if (exFilterCandidateObj.size() > 0) //check needed to determine if filtering is needed
        {
            for (std::vector<FilterObject>::const_iterator i = exFilterCandidateObj.begin(); i != exFilterCandidateObj.end(); ++i)
            {
                if (!cfg.excludeFilter.IsEmpty() && !cfg.excludeFilter.EndsWith(wxT("\n")))
                    cfg.excludeFilter+= wxT("\n");

                if (i->type == FileDescrLine::TYPE_FILE)
                    cfg.excludeFilter+= wxString(FreeFileSync::FILE_NAME_SEPARATOR) + i->relativeName;
                else if (i->type == FileDescrLine::TYPE_DIRECTORY)
                    cfg.excludeFilter+= wxString(FreeFileSync::FILE_NAME_SEPARATOR) + i->relativeName + FreeFileSync::FILE_NAME_SEPARATOR + wxT("*");
                else assert(false);
            }

            cfg.filterIsActive = true;
            updateFilterButton(m_bpButtonFilter, cfg.filterIsActive);

            FreeFileSync::FilterProcess filterInstance(cfg.includeFilter, cfg.excludeFilter);
            filterInstance.filterGridData(currentGridData);

            updateGuiGrid();
            if (hideFilteredElements)
            {
                m_gridLeft->ClearSelection();
                m_gridRight->ClearSelection();
                m_gridMiddle->ClearSelection();
            }
        }
        break;

    case CONTEXT_CLIPBOARD:
        if (m_gridLeft->isLeadGrid())
            copySelectionToClipboard(m_gridLeft);
        else if (m_gridRight->isLeadGrid())
            copySelectionToClipboard(m_gridRight);
        break;

    case CONTEXT_EXPLORER:
        if (m_gridLeft->isLeadGrid() || m_gridRight->isLeadGrid())
        {
            const CustomGrid* leadGrid = NULL;
            if (m_gridLeft->isLeadGrid())
                leadGrid = m_gridLeft;
            else
                leadGrid = m_gridRight;

            std::set<int> selection = getSelectedRows(leadGrid);

            if (selection.size() == 1)
                openWithFileManager(*selection.begin(), m_gridLeft->isLeadGrid());
            else if (selection.size() == 0)
                openWithFileManager(-1, m_gridLeft->isLeadGrid());
        }
        break;

    case CONTEXT_DELETE_FILES:
        deleteSelectedFiles();
        break;
    }
}


void MainDialog::OnContextRimLabelLeft(wxGridEvent& event)
{
    contextMenu.reset(new wxMenu); //re-create context menu
    contextMenu->Append(CONTEXT_CUSTOMIZE_COLUMN_LEFT, _("Customize columns"));
    contextMenu->Connect(wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(MainDialog::OnContextRimLabelSelection), NULL, this);
    PopupMenu(contextMenu.get()); //show context menu
}


void MainDialog::OnContextRimLabelRight(wxGridEvent& event)
{
    contextMenu.reset(new wxMenu); //re-create context menu
    contextMenu->Append(CONTEXT_CUSTOMIZE_COLUMN_RIGHT, _("Customize columns"));
    contextMenu->Connect(wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(MainDialog::OnContextRimLabelSelection), NULL, this);
    PopupMenu(contextMenu.get()); //show context menu
}


void MainDialog::OnContextRimLabelSelection(wxCommandEvent& event)
{
    const ContextIDRimLabel eventId = static_cast<ContextIDRimLabel>(event.GetId());
    switch (eventId)
    {
    case CONTEXT_CUSTOMIZE_COLUMN_LEFT:
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
    break;
    case CONTEXT_CUSTOMIZE_COLUMN_RIGHT:
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
    break;
    }
}


void MainDialog::OnContextMiddle(wxGridEvent& event)
{
    contextMenu.reset(new wxMenu); //re-create context menu

    contextMenu->Append(CONTEXT_CHECK_ALL, _("Check all"));
    contextMenu->Append(CONTEXT_UNCHECK_ALL, _("Uncheck all"));

    if (gridDataView.refGridIsEmpty())
    {
        contextMenu->Enable(CONTEXT_CHECK_ALL, false);
        contextMenu->Enable(CONTEXT_UNCHECK_ALL, false);
    }

    contextMenu->Connect(wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(MainDialog::OnContextMiddleSelection), NULL, this);
    PopupMenu(contextMenu.get()); //show context menu
}


void MainDialog::OnContextMiddleSelection(wxCommandEvent& event)
{
    const ContextIDMiddle eventId = static_cast<ContextIDMiddle>(event.GetId());
    switch (eventId)
    {
    case CONTEXT_CHECK_ALL:
        FreeFileSync::FilterProcess::includeAllRowsOnGrid(currentGridData);
        refreshGridAfterFilterChange(0); //call this instead of updateGuiGrid() to add some delay if hideFiltered == true and to handle some graphical artifacts        break;
        break;
    case CONTEXT_UNCHECK_ALL:
        FreeFileSync::FilterProcess::excludeAllRowsOnGrid(currentGridData);
        refreshGridAfterFilterChange(400); //call this instead of updateGuiGrid() to add some delay if hideFiltered == true and to handle some graphical artifacts
        break;
    }
}


void MainDialog::OnContextMiddleLabel(wxGridEvent& event)
{
    contextMenu.reset(new wxMenu); //re-create context menu

    wxMenuItem* itemSyncPreview = new wxMenuItem(contextMenu.get(), CONTEXT_SYNC_PREVIEW, _("Synchronization Preview"));
    wxMenuItem* itemCmpResult   = new wxMenuItem(contextMenu.get(), CONTEXT_COMPARISON_RESULT, _("Comparison Result"));

    if (syncPreview->previewIsEnabled())
        itemSyncPreview->SetBitmap(*GlobalResources::getInstance().bitmapSyncViewSmall);
    else
        itemCmpResult->SetBitmap(*GlobalResources::getInstance().bitmapCmpViewSmall);

    contextMenu->Append(itemCmpResult);
    contextMenu->Append(itemSyncPreview);

    contextMenu->Connect(wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(MainDialog::OnContextMiddleLabelSelection), NULL, this);
    PopupMenu(contextMenu.get()); //show context menu
}


void MainDialog::OnContextMiddleLabelSelection(wxCommandEvent& event)
{
    const ContextIDMiddleLabel eventId = static_cast<ContextIDMiddleLabel>(event.GetId());
    switch (eventId)
    {
    case CONTEXT_COMPARISON_RESULT:
        //change view
        syncPreview->enablePreview(false);
        break;
    case CONTEXT_SYNC_PREVIEW:
        //change view
        syncPreview->enablePreview(true);
        break;
    }
}


void MainDialog::OnDirSelected(wxFileDirPickerEvent& event)
{
    //left and right directory text-control and dirpicker are synchronized by MainFolderDragDrop automatically

    //disable the sync button
    syncPreview->enableSynchronization(false);

    //clear grids
    currentGridData.clear();
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


//tests if the same filenames are specified, even if they are relative to the current working directory
inline
bool sameFileSpecified(const wxString& file1, const wxString& file2)
{
    wxString file1Full = file1;
    wxString file2Full = file2;

    if (wxFileName(file1).GetPath() == wxEmptyString)
        file1Full = wxFileName::GetCwd() + FreeFileSync::FILE_NAME_SEPARATOR + file1;

    if (wxFileName(file2).GetPath() == wxEmptyString)
        file2Full = wxFileName::GetCwd() + FreeFileSync::FILE_NAME_SEPARATOR + file2;

#ifdef FFS_WIN //don't respect case in windows build
    return FreeFileSync::compareStringsWin32(file1Full.c_str(), file2Full.c_str()) == 0;
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

    std::vector<wxString>::const_iterator i;
    if ((i = find_if(cfgFileNames.begin(), cfgFileNames.end(), FindDuplicates(filename))) != cfgFileNames.end())
    {    //if entry is in the list, then jump to element
        m_choiceHistory->SetSelection(i - cfgFileNames.begin());
    }
    else
    {
        cfgFileNames.insert(cfgFileNames.begin(), filename);

        //the default config file should receive another name on GUI
        if (sameFileSpecified(FreeFileSync::getLastConfigFile(), filename))
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


int findTextPos(const wxArrayString& array, const wxString& text)
{
    for (unsigned int i = 0; i < array.GetCount(); ++i)
#ifdef FFS_WIN //don't respect case in windows build
        if (FreeFileSync::compareStringsWin32(array[i].c_str(), text.c_str()) == 0)
#elif defined FFS_LINUX
        if (array[i] == text)
#endif
            return i;

    return -1;
}


void addPairToFolderHistory(wxComboBox* comboBox, const wxString& newFolder, unsigned int maxHistSize)
{
    const wxString oldVal = comboBox->GetValue();

    const int pos = findTextPos(comboBox->GetStrings(), newFolder);
    if (pos >= 0)
        comboBox->Delete(pos);

    comboBox->Insert(newFolder, 0);

    //keep maximal size of history list
    if (comboBox->GetCount() > maxHistSize)
        comboBox->Delete(maxHistSize);

    comboBox->SetSelection(wxNOT_FOUND);  //don't select anything
    comboBox->SetValue(oldVal);           //but preserve main text!
}


void MainDialog::addLeftFolderToHistory(const wxString& leftFolder)
{
    addPairToFolderHistory(m_directoryLeft, leftFolder, globalSettings.gui.folderHistLeftMax);
}


void MainDialog::addRightFolderToHistory(const wxString& rightFolder)
{
    addPairToFolderHistory(m_directoryRight, rightFolder, globalSettings.gui.folderHistRightMax);
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

        if (FreeFileSync::fileExists(newFileName.c_str()))
        {
            wxMessageDialog* messageDlg = new wxMessageDialog(this, wxString(_("File already exists. Overwrite?")) + wxT(" \"") + newFileName + wxT("\""), _("Warning") , wxOK | wxCANCEL);

            if (messageDlg->ShowModal() != wxID_OK)
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
    wxFileDialog* filePicker = new wxFileDialog(this, wxEmptyString, wxEmptyString, wxEmptyString, wxString(_("FreeFileSync configuration")) + wxT(" (*.ffs_gui)|*.ffs_gui"), wxFD_OPEN);
    if (filePicker->ShowModal() == wxID_OK)
        loadConfiguration(filePicker->GetPath());
}


void MainDialog::OnLoadFromHistory(wxCommandEvent& event)
{
    const int selectedItem = m_choiceHistory->GetSelection();
    if (0 <= selectedItem && unsigned(selectedItem) < cfgFileNames.size())
        loadConfiguration(cfgFileNames[selectedItem]);
}


void MainDialog::OnMenuSaveConfig(wxCommandEvent& event)
{
    OnSaveConfig(event);
}


void MainDialog::OnMenuLoadConfig(wxCommandEvent& event)
{
    OnLoadConfig(event);
}


void MainDialog::loadConfiguration(const wxString& filename)
{
    //notify user about changed settings
    if (globalSettings.gui.popupOnConfigChange && !currentConfigFileName.empty()) //only if check is active and non-default config file loaded
    {
        if (lastConfigurationSaved != getCurrentConfiguration())
        {
            bool dontShowAgain = !globalSettings.gui.popupOnConfigChange;

            QuestionDlg* notifyChangeDlg = new QuestionDlg(this,
                    QuestionDlg::BUTTON_YES | QuestionDlg::BUTTON_NO | QuestionDlg::BUTTON_CANCEL,
                    _("Save changes to current configuration?"),
                    dontShowAgain);

            switch (notifyChangeDlg->ShowModal())
            {
            case QuestionDlg::BUTTON_YES:
                if (!trySaveConfig())
                    return;
                break;
            case QuestionDlg::BUTTON_NO:
                globalSettings.gui.popupOnConfigChange = !dontShowAgain;
                break;
            case QuestionDlg::BUTTON_CANCEL:
                return;
            }
        }
    }
    //------------------------------------------------------------------------------------

    if (!filename.IsEmpty())
    {   //clear grids
        currentGridData.clear();
        updateGuiGrid();

        if (readConfigurationFromXml(filename))
            pushStatusInformation(_("Configuration loaded!"));
    }
}


void MainDialog::OnCfgHistoryKeyEvent(wxKeyEvent& event)
{
    const int keyCode = event.GetKeyCode();
    if (keyCode == WXK_DELETE || keyCode == WXK_NUMPAD_DELETE)
    {   //try to delete the currently selected config history item
        const int selectedItem = m_choiceHistory->GetCurrentSelection();
        if (    0 <= selectedItem &&
                selectedItem < int(m_choiceHistory->GetCount()) &&
                selectedItem < int(cfgFileNames.size()))
        {   //delete selected row
            cfgFileNames.erase(cfgFileNames.begin() + selectedItem);
            m_choiceHistory->Delete(selectedItem);
        }
    }
    event.Skip();
}


void removeSelectedElement(wxComboBox* control, wxEvent& event)
{
    const int selectedItem = control->GetCurrentSelection();
    if (0 <= selectedItem && selectedItem < int(control->GetCount()))
    {
        const wxString oldVal = control->GetValue();
        control->Delete(selectedItem);
        control->SetSelection(wxNOT_FOUND);
        control->SetValue(oldVal);
    }
    else
        event.Skip();
}


void MainDialog::OnFolderHistoryKeyEvent(wxKeyEvent& event)
{

    /*
        const int keyCode = event.GetKeyCode();
        if (keyCode == WXK_DELETE || keyCode == WXK_NUMPAD_DELETE)
        {
            wxObject* eventObj = event.GetEventObject();
            if (eventObj == (wxObject*)m_comboBoxDirLeft)
            {
                removeSelectedElement(m_comboBoxDirLeft, event);
                return; //no event.Skip() here!
            }
            else if (eventObj == (wxObject*)m_comboBoxDirRight)
            {
                removeSelectedElement(m_comboBoxDirRight, event);
                return;
            }
        }*/
    event.Skip();
}


void MainDialog::OnCompareByTimeSize(wxCommandEvent& event)
{
    cfg.compareVar = CMP_BY_TIME_SIZE;
    updateCompareButtons();
}


void MainDialog::OnCompareByContent(wxCommandEvent& event)
{
    cfg.compareVar = CMP_BY_CONTENT;
    updateCompareButtons();
}


void MainDialog::OnClose(wxCloseEvent &event)
{
    requestShutdown();
}


void MainDialog::OnQuit(wxCommandEvent &event)
{
    requestShutdown();
}


void MainDialog::requestShutdown()
{
    //notify user about changed settings
    if (globalSettings.gui.popupOnConfigChange && !currentConfigFileName.empty()) //only if check is active and non-default config file loaded
    {
        if (lastConfigurationSaved != getCurrentConfiguration())
        {
            bool dontShowAgain = !globalSettings.gui.popupOnConfigChange;

            QuestionDlg* notifyChangeDlg = new QuestionDlg(this,
                    QuestionDlg::BUTTON_YES | QuestionDlg::BUTTON_NO | QuestionDlg::BUTTON_CANCEL,
                    _("Save changes to current configuration?"),
                    dontShowAgain);

            switch (notifyChangeDlg->ShowModal())
            {
            case QuestionDlg::BUTTON_YES:
                if (!trySaveConfig())
                    return;
                break;
            case QuestionDlg::BUTTON_NO:
                globalSettings.gui.popupOnConfigChange = !dontShowAgain;
                break;
            case QuestionDlg::BUTTON_CANCEL:
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
        std::set<int> selectedRowsOnView;

        for (int i = lowerBound; i <= std::min(upperBound, int(gridDataView.elementsOnView()) - 1); ++i)
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
        for (int i = lowerBound; i <= std::min(upperBound, int(gridDataView.elementsOnView()) - 1); ++i)
        {
            gridDataView[i].direction = event.direction;
            gridDataView[i].selectedForSynchronization = true;
        }

        updateGuiGrid();
    }
}


bool MainDialog::readConfigurationFromXml(const wxString& filename, bool programStartup)
{
    //load XML
    xmlAccess::XmlGuiConfig guiCfg;  //structure to receive gui settings, already defaulted!!
    try
    {
        guiCfg = xmlAccess::readGuiConfig(filename);
    }
    catch (const FileError& error)
    {
        if (programStartup)
        {
            if (filename == FreeFileSync::getLastConfigFile() && !wxFileExists(filename)) //do not show error in this case
                ;
            else //program startup: show error message and load defaults
                wxMessageBox(error.show().c_str(), _("Error"), wxOK | wxICON_ERROR);
        }
        else
        {
            wxMessageBox(error.show().c_str(), _("Error"), wxOK | wxICON_ERROR);
            return false;
        }
    }

    //(re-)set view filter buttons
    gridDataView.leftOnlyFilesActive   = true;
    gridDataView.leftNewerFilesActive  = true;
    gridDataView.differentFilesActive  = true;
    gridDataView.rightNewerFilesActive = true;  //do not save/load these bool values from harddisk!
    gridDataView.rightOnlyFilesActive  = true;  //it's more convenient to have them defaulted at startup
    gridDataView.equalFilesActive      = false;

    gridDataView.conflictFilesActive   = true;

    gridDataView.syncDirLeftActive     = true;
    gridDataView.syncDirRightActive    = true;
    gridDataView.syncDirNoneActive     = true;

    updateViewFilterButtons();


    //load main configuration into instance
    cfg = guiCfg.mainCfg;

    //update visible config on main window
    updateCompareButtons();

    updateFilterButton(m_bpButtonFilter, cfg.filterIsActive);

    //read folder pairs:
    //clear existing pairs first
    m_directoryLeft->SetSelection(wxNOT_FOUND);
    m_directoryLeft->SetValue(wxEmptyString);
    m_dirPickerLeft->SetPath(wxEmptyString);
    m_directoryRight->SetSelection(wxNOT_FOUND);
    m_directoryRight->SetValue(wxEmptyString);
    m_dirPickerRight->SetPath(wxEmptyString);

    clearFolderPairs();

    if (guiCfg.directoryPairs.size() > 0)
    {
        //set main folder pair
        std::vector<FolderPair>::const_iterator main = guiCfg.directoryPairs.begin();

        m_directoryLeft->SetValue(main->leftDirectory.c_str());
        const wxString leftDirFormatted = FreeFileSync::getFormattedDirectoryName(main->leftDirectory).c_str();
        if (wxDirExists(leftDirFormatted))
            m_dirPickerLeft->SetPath(leftDirFormatted);
        addLeftFolderToHistory(main->leftDirectory.c_str()); //another hack: wxCombobox::Insert() asynchronously sends message overwriting a later wxCombobox::SetValue()!!! :(

        m_directoryRight->SetValue(main->rightDirectory.c_str());
        const wxString rightDirFormatted = FreeFileSync::getFormattedDirectoryName(main->rightDirectory).c_str();
        if (wxDirExists(rightDirFormatted))
            m_dirPickerRight->SetPath(rightDirFormatted);
        addRightFolderToHistory(main->rightDirectory.c_str()); //another hack...

        //set additional pairs
        std::vector<FolderPair> additionalPairs; //don't modify guiCfg.directoryPairs!
        for (std::vector<FolderPair>::const_iterator i = guiCfg.directoryPairs.begin() + 1; i != guiCfg.directoryPairs.end(); ++i)
            additionalPairs.push_back(*i);
        addFolderPair(additionalPairs);
    }

    //read GUI layout
    hideFilteredElements = guiCfg.hideFilteredElements;
    m_checkBoxHideFilt->SetValue(hideFilteredElements);

    ignoreErrors = guiCfg.ignoreErrors;

    syncPreview->enablePreview(guiCfg.syncPreviewEnabled);

    //###########################################################
    addFileToCfgHistory(filename); //put filename on list of last used config files

    lastConfigurationSaved = guiCfg;

    if (filename == FreeFileSync::getLastConfigFile()) //set title
    {
        SetTitle(wxString(wxT("FreeFileSync - ")) + _("Folder Comparison and Synchronization"));
        currentConfigFileName.clear();
    }
    else
    {
        SetTitle(wxString(wxT("FreeFileSync - ")) + filename);
        currentConfigFileName = filename;
    }

    //update variant name
    m_staticTextVariant->SetLabel(wxString(wxT("(")) + cfg.syncConfiguration.getVariantName() + wxT(")"));
    bSizer6->Layout(); //adapt layout for variant text

    return true;
}


bool MainDialog::writeConfigurationToXml(const wxString& filename)
{
    const xmlAccess::XmlGuiConfig guiCfg = getCurrentConfiguration();

    //write config to XML
    try
    {
        xmlAccess::writeGuiConfig(filename, guiCfg);
    }
    catch (const FileError& error)
    {
        wxMessageBox(error.show().c_str(), _("Error"), wxOK | wxICON_ERROR);
        return false;
    }

    //put filename on list of last used config files
    addFileToCfgHistory(filename);

    lastConfigurationSaved = guiCfg;

    if (filename == FreeFileSync::getLastConfigFile()) //set title
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


xmlAccess::XmlGuiConfig MainDialog::getCurrentConfiguration() const
{
    xmlAccess::XmlGuiConfig guiCfg;

    //load structure with basic settings "mainCfg"
    guiCfg.mainCfg = cfg;
    guiCfg.directoryPairs = getFolderPairs();

    //load structure with gui settings
    guiCfg.hideFilteredElements = hideFilteredElements;

    guiCfg.ignoreErrors = ignoreErrors;

    guiCfg.syncPreviewEnabled = syncPreview->previewIsEnabled();

    return guiCfg;
}


void MainDialog::OnShowHelpDialog(wxCommandEvent &event)
{
    HelpDlg* helpDlg = new HelpDlg(this);
    helpDlg->ShowModal();
}


void MainDialog::refreshGridAfterFilterChange(const int delay)
{
    //signal UI that grids need to be refreshed on next Update()
    m_gridLeft->ForceRefresh();
    m_gridMiddle->ForceRefresh();
    m_gridRight->ForceRefresh();
    Update();   //show changes resulting from ForceRefresh()

    if (hideFilteredElements)
    {
        wxMilliSleep(delay); //some delay to show user the rows he has filtered out before they are removed
        updateGuiGrid();   //redraw grid to remove excluded elements (keeping sort sequence)

        m_gridLeft->ClearSelection();
        m_gridRight->ClearSelection();
    }
    else
        updateGuiGrid();
}


void MainDialog::OnFilterButton(wxCommandEvent &event)
{   //toggle filter on/off
    cfg.filterIsActive = !cfg.filterIsActive;
    //make sure, button-appearance and "filterIsActive" are in sync.
    updateFilterButton(m_bpButtonFilter, cfg.filterIsActive);

    if (cfg.filterIsActive)
    {
        FreeFileSync::FilterProcess filterInstance(cfg.includeFilter, cfg.excludeFilter);
        filterInstance.filterGridData(currentGridData);
        refreshGridAfterFilterChange(400); //call this instead of updateGuiGrid() to add some delay if hideFiltered == true and to handle some graphical artifacts
    }
    else
    {
        FreeFileSync::FilterProcess::includeAllRowsOnGrid(currentGridData);
        refreshGridAfterFilterChange(0); //call this instead of updateGuiGrid() to add some delay if hideFiltered == true and to handle some graphical artifacts        }
    }
}


void MainDialog::OnHideFilteredButton(wxCommandEvent &event)
{   //toggle showing filtered rows
    hideFilteredElements = !hideFilteredElements;
    //make sure, checkbox and "hideFiltered" are in sync
    m_checkBoxHideFilt->SetValue(hideFilteredElements);

    m_gridLeft->ClearSelection();
    m_gridRight->ClearSelection();

    refreshGridAfterFilterChange(0);

    event.Skip();
}


void MainDialog::OnConfigureFilter(wxHyperlinkEvent &event)
{
    const wxString beforeImage = cfg.includeFilter + wxChar(1) + cfg.excludeFilter;

    FilterDlg* filterDlg = new FilterDlg(this, cfg.includeFilter, cfg.excludeFilter);
    if (filterDlg->ShowModal() == FilterDlg::BUTTON_OKAY)
    {
        const wxString afterImage = cfg.includeFilter + wxChar(1) + cfg.excludeFilter;

        if (beforeImage != afterImage)  //if filter settings are changed: set filtering to "on"
        {
            if (afterImage == (wxString(wxT("*")) + wxChar(1))) //default
            {
                cfg.filterIsActive = false;
                FreeFileSync::FilterProcess::includeAllRowsOnGrid(currentGridData);
            }
            else
            {
                cfg.filterIsActive = true;

                FreeFileSync::FilterProcess filterInstance(cfg.includeFilter, cfg.excludeFilter);
                filterInstance.filterGridData(currentGridData);
            }

            updateFilterButton(m_bpButtonFilter, cfg.filterIsActive);

            updateGuiGrid();
        }
    }

    //no event.Skip() here, to not start browser
}


void MainDialog::OnLeftOnlyFiles(wxCommandEvent& event)
{
    gridDataView.leftOnlyFilesActive = !gridDataView.leftOnlyFilesActive;
    updateViewFilterButtons();
    updateGuiGrid();
};

void MainDialog::OnLeftNewerFiles(wxCommandEvent& event)
{
    gridDataView.leftNewerFilesActive = !gridDataView.leftNewerFilesActive;
    updateViewFilterButtons();
    updateGuiGrid();
};

void MainDialog::OnDifferentFiles(wxCommandEvent& event)
{
    gridDataView.differentFilesActive = !gridDataView.differentFilesActive;
    updateViewFilterButtons();
    updateGuiGrid();
};

void MainDialog::OnRightNewerFiles(wxCommandEvent& event)
{
    gridDataView.rightNewerFilesActive = !gridDataView.rightNewerFilesActive;
    updateViewFilterButtons();
    updateGuiGrid();
};

void MainDialog::OnRightOnlyFiles(wxCommandEvent& event)
{
    gridDataView.rightOnlyFilesActive = !gridDataView.rightOnlyFilesActive;
    updateViewFilterButtons();
    updateGuiGrid();
};


void MainDialog::OnEqualFiles(wxCommandEvent& event)
{
    gridDataView.equalFilesActive = !gridDataView.equalFilesActive;
    updateViewFilterButtons();
    updateGuiGrid();
};


void MainDialog::OnConflictFiles(wxCommandEvent& event)
{
    gridDataView.conflictFilesActive = !gridDataView.conflictFilesActive;
    updateViewFilterButtons();
    updateGuiGrid();
};


void MainDialog::OnSyncDirLeft(wxCommandEvent& event)
{
    gridDataView.syncDirLeftActive = !gridDataView.syncDirLeftActive;
    updateViewFilterButtons();
    updateGuiGrid();
};


void MainDialog::OnSyncDirRight(wxCommandEvent& event)
{
    gridDataView.syncDirRightActive = !gridDataView.syncDirRightActive;
    updateViewFilterButtons();
    updateGuiGrid();
};


void MainDialog::OnSyncDirNone(wxCommandEvent& event)
{
    gridDataView.syncDirNoneActive = !gridDataView.syncDirNoneActive;
    updateViewFilterButtons();
    updateGuiGrid();
};


void MainDialog::updateViewFilterButtons()
{
    //compare result buttons
    if (gridDataView.leftOnlyFilesActive)
    {
        m_bpButtonLeftOnly->SetBitmapLabel(*GlobalResources::getInstance().bitmapLeftOnlyAct);
        m_bpButtonLeftOnly->SetToolTip(_("Hide files that exist on left side only"));
    }
    else
    {
        m_bpButtonLeftOnly->SetBitmapLabel(*GlobalResources::getInstance().bitmapLeftOnlyDeact);
        m_bpButtonLeftOnly->SetToolTip(_("Show files that exist on left side only"));
    }

    if (gridDataView.rightOnlyFilesActive)
    {
        m_bpButtonRightOnly->SetBitmapLabel(*GlobalResources::getInstance().bitmapRightOnlyAct);
        m_bpButtonRightOnly->SetToolTip(_("Hide files that exist on right side only"));
    }
    else
    {
        m_bpButtonRightOnly->SetBitmapLabel(*GlobalResources::getInstance().bitmapRightOnlyDeact);
        m_bpButtonRightOnly->SetToolTip(_("Show files that exist on right side only"));
    }

    if (gridDataView.leftNewerFilesActive)
    {
        m_bpButtonLeftNewer->SetBitmapLabel(*GlobalResources::getInstance().bitmapLeftNewerAct);
        m_bpButtonLeftNewer->SetToolTip(_("Hide files that are newer on left"));
    }
    else
    {
        m_bpButtonLeftNewer->SetBitmapLabel(*GlobalResources::getInstance().bitmapLeftNewerDeact);
        m_bpButtonLeftNewer->SetToolTip(_("Show files that are newer on left"));
    }

    if (gridDataView.rightNewerFilesActive)
    {
        m_bpButtonRightNewer->SetBitmapLabel(*GlobalResources::getInstance().bitmapRightNewerAct);
        m_bpButtonRightNewer->SetToolTip(_("Hide files that are newer on right"));
    }
    else
    {
        m_bpButtonRightNewer->SetBitmapLabel(*GlobalResources::getInstance().bitmapRightNewerDeact);
        m_bpButtonRightNewer->SetToolTip(_("Show files that are newer on right"));
    }

    if (gridDataView.equalFilesActive)
    {
        m_bpButtonEqual->SetBitmapLabel(*GlobalResources::getInstance().bitmapEqualAct);
        m_bpButtonEqual->SetToolTip(_("Hide files that are equal"));
    }
    else
    {
        m_bpButtonEqual->SetBitmapLabel(*GlobalResources::getInstance().bitmapEqualDeact);
        m_bpButtonEqual->SetToolTip(_("Show files that are equal"));
    }

    if (gridDataView.differentFilesActive)
    {
        m_bpButtonDifferent->SetBitmapLabel(*GlobalResources::getInstance().bitmapDifferentAct);
        m_bpButtonDifferent->SetToolTip(_("Hide files that are different"));
    }
    else
    {
        m_bpButtonDifferent->SetBitmapLabel(*GlobalResources::getInstance().bitmapDifferentDeact);
        m_bpButtonDifferent->SetToolTip(_("Show files that are different"));
    }

    if (gridDataView.conflictFilesActive)
    {
        m_bpButtonConflict->SetBitmapLabel(*GlobalResources::getInstance().bitmapConflictAct);
        m_bpButtonConflict->SetToolTip(_("Hide conflicts"));
    }
    else
    {
        m_bpButtonConflict->SetBitmapLabel(*GlobalResources::getInstance().bitmapConflictDeact);
        m_bpButtonConflict->SetToolTip(_("Show conflicts"));
    }

    //sync preview buttons
    if (gridDataView.syncDirLeftActive)
    {
        m_bpButtonSyncDirLeft->SetBitmapLabel(*GlobalResources::getInstance().bitmapSyncDirLeftAct);
        m_bpButtonSyncDirLeft->SetToolTip(_("Hide files that will be copied to the left side"));
    }
    else
    {
        m_bpButtonSyncDirLeft->SetBitmapLabel(*GlobalResources::getInstance().bitmapSyncDirLeftDeact);
        m_bpButtonSyncDirLeft->SetToolTip(_("Show files that will be copied to the left side"));
    }

    if (gridDataView.syncDirRightActive)
    {
        m_bpButtonSyncDirRight->SetBitmapLabel(*GlobalResources::getInstance().bitmapSyncDirRightAct);
        m_bpButtonSyncDirRight->SetToolTip(_("Hide files that will be copied to the right side"));
    }
    else
    {
        m_bpButtonSyncDirRight->SetBitmapLabel(*GlobalResources::getInstance().bitmapSyncDirRightDeact);
        m_bpButtonSyncDirRight->SetToolTip(_("Show files that will be copied to the right side"));
    }

    if (gridDataView.syncDirNoneActive)
    {
        m_bpButtonSyncDirNone->SetBitmapLabel(*GlobalResources::getInstance().bitmapSyncDirNoneAct);
        m_bpButtonSyncDirNone->SetToolTip(_("Hide files that won't be copied"));
    }
    else
    {
        m_bpButtonSyncDirNone->SetBitmapLabel(*GlobalResources::getInstance().bitmapSyncDirNoneDeact);
        m_bpButtonSyncDirNone->SetToolTip(_("Show files that won't be copied"));
    }
}


void MainDialog::updateFilterButton(wxBitmapButton* filterButton, bool isActive)
{
    if (isActive)
    {
        filterButton->SetBitmapLabel(*GlobalResources::getInstance().bitmapFilterOn);
        filterButton->SetToolTip(_("Filter active: Press again to deactivate"));

        //show filter icon
        if (m_notebookBottomLeft->GetImageList() == NULL)
        {
            wxImageList* panelIcons = new wxImageList(16, 16);
            panelIcons->Add(wxBitmap(*GlobalResources::getInstance().bitmapFilterSmall));
            m_notebookBottomLeft->AssignImageList(panelIcons); //pass ownership
        }
        m_notebookBottomLeft->SetPageImage(1, 0);

    }
    else
    {
        filterButton->SetBitmapLabel(*GlobalResources::getInstance().bitmapFilterOff);
        filterButton->SetToolTip(_("Press button to activate filter"));

        //hide filter icon
        m_notebookBottomLeft->SetPageImage(1, -1);
    }
}


void MainDialog::updateCompareButtons()
{
    switch (cfg.compareVar)
    {
    case CMP_BY_TIME_SIZE:
        m_radioBtnSizeDate->SetValue(true);
        break;
    case CMP_BY_CONTENT:
        m_radioBtnContent->SetValue(true);
        break;
    default:
        assert (false);
    }

    //disable the sync button
    syncPreview->enableSynchronization(false);

    //clear grids
    currentGridData.clear();
    updateGuiGrid();
}


std::vector<FolderPair> MainDialog::getFolderPairs() const
{
    std::vector<FolderPair> output;

    //add main pair
    output.push_back(FolderPair(m_directoryLeft->GetValue().c_str(),
                                m_directoryRight->GetValue().c_str()));

    //add additional pairs
    for (std::vector<FolderPairPanel*>::const_iterator i = additionalFolderPairs.begin(); i != additionalFolderPairs.end(); ++i)
        output.push_back(FolderPair((*i)->m_directoryLeft->GetValue().c_str(),
                                    (*i)->m_directoryRight->GetValue().c_str()));
    return output;
}


void MainDialog::OnCompare(wxCommandEvent &event)
{
    //PERF_START;

    clearStatusBar();

    wxBusyCursor dummy; //show hourglass cursor

    //prevent temporary memory peak by clearing old result list
    currentGridData.clear();
    updateGuiGrid(); //refresh GUI grid

    bool aborted = false;
    try
    {   //class handling status display and error messages
        CompareStatusHandler statusHandler(this);
        cmpStatusHandlerTmp = &statusHandler;

        //prepare filter
        std::auto_ptr<FreeFileSync::FilterProcess> filterInstance(NULL);
        if (cfg.filterIsActive)
            filterInstance.reset(new FreeFileSync::FilterProcess(cfg.includeFilter, cfg.excludeFilter));

        //begin comparison
        FreeFileSync::CompareProcess comparison(globalSettings.traverseDirectorySymlinks,
                                                globalSettings.fileTimeTolerance,
                                                globalSettings.ignoreOneHourDiff,
                                                globalSettings.warnings,
                                                filterInstance.get(),
                                                &statusHandler);

        comparison.startCompareProcess(getFolderPairs(),
                                       cfg.compareVar,
                                       cfg.syncConfiguration,
                                       currentGridData);

        //if (output.size < 50000)
        statusHandler.updateStatusText(_("Sorting file list..."));
        statusHandler.forceUiRefresh(); //keep total number of scanned files up to date

        gridDataView.sortView(GridView::SORT_BY_DIRECTORY, true, true);
    }
    catch (AbortThisProcess&)
    {
        aborted = true;
    }
    cmpStatusHandlerTmp = NULL;

    if (aborted)
    {   //disable the sync button
        syncPreview->enableSynchronization(false);
        m_buttonCompare->SetFocus();
    }
    else
    {   //once compare is finished enable the sync button
        syncPreview->enableSynchronization(true);
        m_buttonStartSync->SetFocus();

        //hide sort direction indicator on GUI grids
        m_gridLeft->setSortMarker(CustomGrid::SortMarker(-1, CustomGrid::ASCENDING));
        m_gridMiddle->setSortMarker(CustomGrid::SortMarker(-1, CustomGrid::ASCENDING));
        m_gridRight->setSortMarker(CustomGrid::SortMarker(-1, CustomGrid::ASCENDING));

        //reset last sort selection: used for determining sort direction
        lastSortColumn = -1;
        lastSortGrid   = NULL;

        m_gridLeft->ClearSelection();
        m_gridMiddle->ClearSelection();
        m_gridRight->ClearSelection();

        //add to folder history after successful comparison only
        addLeftFolderToHistory(m_directoryLeft->GetValue());
        addRightFolderToHistory(m_directoryRight->GetValue());
    }

    //refresh grid in ANY case! (also on abort)
    updateGuiGrid();
}


void MainDialog::OnAbortCompare(wxCommandEvent& event)
{
    if (cmpStatusHandlerTmp)
        cmpStatusHandlerTmp->requestAbortion();
}


void MainDialog::updateGuiGrid()
{
    m_gridLeft->BeginBatch(); //necessary??
    m_gridMiddle->BeginBatch();
    m_gridRight->BeginBatch();

    updateGridViewData(); //update gridDataView and write status information

    //all three grids retrieve their data directly via currentGridData!!!
    //the only thing left to do is notify the grids to update their sizes (nr of rows), since this has to be communicated by the grids via messages
    m_gridLeft->updateGridSizes();
    m_gridMiddle->updateGridSizes();
    m_gridRight->updateGridSizes();

    //enlarge label width to display row numbers correctly
    const int nrOfRows = m_gridLeft->GetNumberRows();

    if (nrOfRows >= 1)
    {
#ifdef FFS_WIN
        const int digitWidth = 8;
#elif defined FFS_LINUX
        const int digitWidth = 10;
#endif
        const int nrOfDigits = int(floor(log10(double(nrOfRows)) + 1));
        m_gridLeft->SetRowLabelSize(nrOfDigits * digitWidth + 4);
        m_gridRight->SetRowLabelSize(nrOfDigits * digitWidth + 4);
    }

    //update sync preview statistics
    calculatePreview();

    m_gridLeft->EndBatch();
    m_gridMiddle->EndBatch();
    m_gridRight->EndBatch();
}


void MainDialog::calculatePreview()
{
    //update preview of bytes to be transferred:
    int objectsToCreate    = 0;
    int objectsToOverwrite = 0;
    int objectsToDelete    = 0;
    int conflictsDummy     = 0;
    wxULongLong dataToProcess;
    FreeFileSync::calcTotalBytesToSync(currentGridData,
                                       objectsToCreate,
                                       objectsToOverwrite,
                                       objectsToDelete,
                                       conflictsDummy,
                                       dataToProcess);

    const wxString toCreate = globalFunctions::includeNumberSeparator(globalFunctions::numberToWxString(objectsToCreate));
    const wxString toUpdate = globalFunctions::includeNumberSeparator(globalFunctions::numberToWxString(objectsToOverwrite));
    const wxString toDelete = globalFunctions::includeNumberSeparator(globalFunctions::numberToWxString(objectsToDelete));
    const wxString data     = FreeFileSync::formatFilesizeToShortString(dataToProcess);

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
    SyncDialog* syncDlg = new SyncDialog(this, currentGridData, cfg, ignoreErrors);
    if (syncDlg->ShowModal() == SyncDialog::BUTTON_OKAY)
    {
        //update variant name
        m_staticTextVariant->SetLabel(wxString(wxT("(")) + cfg.syncConfiguration.getVariantName() + wxT(")"));
        bSizer6->Layout(); //adapt layout for variant text

        redetermineSyncDirection(cfg.syncConfiguration, currentGridData);

        updateGuiGrid();
    }
}


void MainDialog::OnStartSync(wxCommandEvent& event)
{
    if (syncPreview->synchronizationIsEnabled())
    {
        //show sync preview screen
        if (globalSettings.gui.showSummaryBeforeSync)
        {
            //prepare preview screen
            int objectsToCreate    = 0;
            int objectsToOverwrite = 0;
            int objectsToDelete    = 0;
            int conflictsDummy     = 0;
            wxULongLong dataToProcess;
            FreeFileSync::calcTotalBytesToSync(currentGridData,
                                               objectsToCreate,
                                               objectsToOverwrite,
                                               objectsToDelete,
                                               conflictsDummy,
                                               dataToProcess);

            const wxString toCreate = globalFunctions::includeNumberSeparator(globalFunctions::numberToWxString(objectsToCreate));
            const wxString toUpdate = globalFunctions::includeNumberSeparator(globalFunctions::numberToWxString(objectsToOverwrite));
            const wxString toDelete = globalFunctions::includeNumberSeparator(globalFunctions::numberToWxString(objectsToDelete));
            const wxString data     = FreeFileSync::formatFilesizeToShortString(dataToProcess);

            bool dontShowAgain = false;

            SyncPreviewDlg* preview = new SyncPreviewDlg(
                this, cfg.syncConfiguration.getVariantName(),
                toCreate, toUpdate, toDelete, data,
                dontShowAgain);

            if (preview->ShowModal() != SyncPreviewDlg::BUTTON_START)
                return;

            globalSettings.gui.showSummaryBeforeSync = !dontShowAgain;
        }

        //check if there are files/folders to be sync'ed at all
        if (!synchronizationNeeded(currentGridData))
        {
            wxMessageBox(_("Nothing to synchronize according to configuration!"), _("Information"), wxICON_WARNING);
            return;
        }

        wxBusyCursor dummy; //show hourglass cursor

        clearStatusBar();
        try
        {
            //class handling status updates and error messages
            SyncStatusHandler statusHandler(this, ignoreErrors);

            //start synchronization and return elements that were not sync'ed in currentGridData
            FreeFileSync::SyncProcess synchronization(
                cfg.useRecycleBin,
                globalSettings.copyFileSymlinks,
                globalSettings.traverseDirectorySymlinks,
                globalSettings.warnings,
                &statusHandler);

            synchronization.startSynchronizationProcess(currentGridData);
        }
        catch (AbortThisProcess&)
        {   //do NOT disable the sync button: user might want to try to sync the REMAINING rows
        }   //enableSynchronization(false);


        //show remaining files that have not been processed: put DIRECTLY after startSynchronizationProcess() and DON'T call any wxWidgets functions
        //in between! Else CustomGrid might access obsolete data entries in currentGridData!
        updateGuiGrid();

        m_gridLeft->ClearSelection();
        m_gridMiddle->ClearSelection();
        m_gridRight->ClearSelection();

        if (!gridDataView.refGridIsEmpty())
            pushStatusInformation(_("Not all items have been synchronized! Have a look at the list."));
        else
        {
            pushStatusInformation(_("All items have been synchronized!"));
            syncPreview->enableSynchronization(false);
        }
    }
}


void MainDialog::OnLeftGridDoubleClick(wxGridEvent& event)
{
    openWithFileManager(event.GetRow(), true);
    event.Skip();
}


void MainDialog::OnRightGridDoubleClick(wxGridEvent& event)
{
    openWithFileManager(event.GetRow(), false);
    event.Skip();
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
            gridDataView.sortView(GridView::SORT_BY_DIRECTORY, true, sortAscending);
            break;
        case xmlAccess::FILENAME:
            gridDataView.sortView(GridView::SORT_BY_FILENAME, true, sortAscending);
            break;
        case xmlAccess::REL_PATH:
            gridDataView.sortView(GridView::SORT_BY_REL_NAME, true, sortAscending);
            break;
        case xmlAccess::DIRECTORY:
            gridDataView.sortView(GridView::SORT_BY_DIRECTORY, true, sortAscending);
            break;
        case xmlAccess::SIZE:
            gridDataView.sortView(GridView::SORT_BY_FILESIZE, true, sortAscending);
            break;
        case xmlAccess::DATE:
            gridDataView.sortView(GridView::SORT_BY_DATE, true, sortAscending);
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
    if (syncPreview->previewIsEnabled())
        gridDataView.sortView(GridView::SORT_BY_SYNC_DIRECTION, true, sortAscending);
    else
        gridDataView.sortView(GridView::SORT_BY_CMP_RESULT, true, sortAscending);

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
            gridDataView.sortView(GridView::SORT_BY_DIRECTORY, false, sortAscending);
            break;
        case xmlAccess::FILENAME:
            gridDataView.sortView(GridView::SORT_BY_FILENAME, false, sortAscending);
            break;
        case xmlAccess::REL_PATH:
            gridDataView.sortView(GridView::SORT_BY_REL_NAME, false, sortAscending);
            break;
        case xmlAccess::DIRECTORY:
            gridDataView.sortView(GridView::SORT_BY_DIRECTORY, false, sortAscending);
            break;
        case xmlAccess::SIZE:
            gridDataView.sortView(GridView::SORT_BY_FILESIZE, false, sortAscending);
            break;
        case xmlAccess::DATE:
            gridDataView.sortView(GridView::SORT_BY_DATE, false, sortAscending);
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
    //swap directory names: main pair
    const wxString leftDir  = m_directoryLeft->GetValue();
    const wxString rightDir = m_directoryRight->GetValue();
    m_directoryLeft->SetSelection(wxNOT_FOUND);
    m_directoryRight->SetSelection(wxNOT_FOUND);

    m_directoryLeft->SetValue(rightDir);
    m_directoryRight->SetValue(leftDir);

    //additional pairs
    wxString tmp;
    for (std::vector<FolderPairPanel*>::const_iterator i = additionalFolderPairs.begin(); i != additionalFolderPairs.end(); ++i)
    {
        FolderPairPanel* dirPair = *i;
        tmp = dirPair->m_directoryLeft->GetValue();
        dirPair->m_directoryLeft->SetValue(dirPair->m_directoryRight->GetValue());
        dirPair->m_directoryRight->SetValue(tmp);
    }

    //swap view filter
    std::swap(gridDataView.leftOnlyFilesActive, gridDataView.rightOnlyFilesActive);
    std::swap(gridDataView.leftNewerFilesActive, gridDataView.rightNewerFilesActive);
    std::swap(gridDataView.syncDirLeftActive, gridDataView.syncDirRightActive);

    updateViewFilterButtons();

    //swap grid information
    FreeFileSync::swapGrids(cfg.syncConfiguration, currentGridData);
    updateGuiGrid();
    event.Skip();
}


void MainDialog::updateGridViewData()
{
    const GridView::StatusInfo result = gridDataView.update(hideFilteredElements, syncPreview->previewIsEnabled());

    //hide or enable view filter buttons

    //comparison result view buttons
    if (result.existsLeftOnly)
        m_bpButtonLeftOnly->Show();
    else
        m_bpButtonLeftOnly->Hide();

    if (result.existsRightOnly)
        m_bpButtonRightOnly->Show();
    else
        m_bpButtonRightOnly->Hide();

    if (result.existsLeftNewer)
        m_bpButtonLeftNewer->Show();
    else
        m_bpButtonLeftNewer->Hide();

    if (result.existsRightNewer)
        m_bpButtonRightNewer->Show();
    else
        m_bpButtonRightNewer->Hide();

    if (result.existsDifferent)
        m_bpButtonDifferent->Show();
    else
        m_bpButtonDifferent->Hide();

    if (result.existsEqual)
        m_bpButtonEqual->Show();
    else
        m_bpButtonEqual->Hide();

    if (result.existsConflict)
        m_bpButtonConflict->Show();
    else
        m_bpButtonConflict->Hide();

    //sync preview buttons
    if (result.existsSyncDirLeft)
        m_bpButtonSyncDirLeft->Show();
    else
        m_bpButtonSyncDirLeft->Hide();

    if (result.existsSyncDirRight)
        m_bpButtonSyncDirRight->Show();
    else
        m_bpButtonSyncDirRight->Hide();

    if (result.existsSyncDirNone)
        m_bpButtonSyncDirNone->Show();
    else
        m_bpButtonSyncDirNone->Hide();


    if (    result.existsLeftOnly     ||
            result.existsRightOnly    ||
            result.existsLeftNewer    ||
            result.existsRightNewer   ||
            result.existsDifferent    ||
            result.existsEqual        ||
            result.existsConflict     ||
            result.existsSyncDirLeft  ||
            result.existsSyncDirRight ||
            result.existsSyncDirNone)
    {
        m_panel112->Show();
        m_panel112->Layout();
    }
    else
        m_panel112->Hide();

    bSizer3->Layout();


    //update status information
    while (stackObjects.size() > 0)
        stackObjects.pop();

    wxString statusLeftNew;
    wxString statusMiddleNew;
    wxString statusRightNew;

//#################################################
//format numbers to text:

//show status information on "root" level.
    if (result.foldersOnLeftView)
    {
        if (result.foldersOnLeftView == 1)
            statusLeftNew += _("1 directory");
        else
        {
            wxString folderCount = globalFunctions::includeNumberSeparator(globalFunctions::numberToWxString(result.foldersOnLeftView));

            wxString outputString = _("%x directories");
            outputString.Replace(wxT("%x"), folderCount, false);
            statusLeftNew += outputString;
        }

        if (result.filesOnLeftView)
            statusLeftNew += wxT(", ");
    }

    if (result.filesOnLeftView)
    {
        if (result.filesOnLeftView == 1)
            statusLeftNew += _("1 file,");
        else
        {
            wxString fileCount = globalFunctions::includeNumberSeparator(globalFunctions::numberToWxString(result.filesOnLeftView));

            wxString outputString = _("%x files,");
            outputString.Replace(wxT("%x"), fileCount, false);
            statusLeftNew += outputString;
        }
        statusLeftNew += wxT(" ");
        statusLeftNew += FreeFileSync::formatFilesizeToShortString(result.filesizeLeftView);
    }

    const wxString objectsView = globalFunctions::includeNumberSeparator(globalFunctions::numberToWxString(gridDataView.elementsOnView()));
    if (result.objectsTotal == 1)
    {
        wxString outputString = _("%x of 1 row in view");
        outputString.Replace(wxT("%x"), objectsView, false);
        statusMiddleNew = outputString;
    }
    else
    {
        const wxString objectsTotal = globalFunctions::includeNumberSeparator(globalFunctions::numberToWxString(result.objectsTotal));

        wxString outputString = _("%x of %y rows in view");
        outputString.Replace(wxT("%x"), objectsView, false);
        outputString.Replace(wxT("%y"), objectsTotal, false);
        statusMiddleNew = outputString;
    }

    if (result.foldersOnRightView)
    {
        if (result.foldersOnRightView == 1)
            statusRightNew += _("1 directory");
        else
        {
            wxString folderCount = globalFunctions::includeNumberSeparator(globalFunctions::numberToWxString(result.foldersOnRightView));

            wxString outputString = _("%x directories");
            outputString.Replace(wxT("%x"), folderCount, false);
            statusRightNew += outputString;
        }

        if (result.filesOnRightView)
            statusRightNew += wxT(", ");
    }

    if (result.filesOnRightView)
    {
        if (result.filesOnRightView == 1)
            statusRightNew += _("1 file,");
        else
        {
            wxString fileCount = globalFunctions::includeNumberSeparator(globalFunctions::numberToWxString(result.filesOnRightView));

            wxString outputString = _("%x files,");
            outputString.Replace(wxT("%x"), fileCount, false);
            statusRightNew += outputString;
        }

        statusRightNew += wxT(" ");
        statusRightNew += FreeFileSync::formatFilesizeToShortString(result.filesizeRightView);
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
    const wxString topPairLeft  = m_directoryLeft->GetValue();
    const wxString topPairRight = m_directoryRight->GetValue();

    //clear existing pairs first
    m_directoryLeft->SetSelection(wxNOT_FOUND);
    m_directoryLeft->SetValue(wxEmptyString);
    m_dirPickerLeft->SetPath(wxEmptyString);
    m_directoryRight->SetSelection(wxNOT_FOUND);
    m_directoryRight->SetValue(wxEmptyString);
    m_dirPickerRight->SetPath(wxEmptyString);

    addFolderPair(topPairLeft.c_str(), topPairRight.c_str(), true); //add pair in front of additonal pairs

    //disable the sync button
    syncPreview->enableSynchronization(false);

    //clear grids
    currentGridData.clear();
    updateGuiGrid();
}


void MainDialog::OnRemoveFolderPair(wxCommandEvent& event)
{
    //find folder pair originating the event
    const wxObject* const eventObj = event.GetEventObject();
    for (std::vector<FolderPairPanel*>::const_iterator i = additionalFolderPairs.begin(); i != additionalFolderPairs.end(); ++i)
    {
        if (eventObj == static_cast<wxObject*>((*i)->m_bpButtonRemovePair))
        {
            removeFolderPair(i - additionalFolderPairs.begin());

            //disable the sync button
            syncPreview->enableSynchronization(false);

            //clear grids
            currentGridData.clear();
            updateGuiGrid();
            return;
        }
    }
}


void MainDialog::OnRemoveTopFolderPair(wxCommandEvent& event)
{
    if (additionalFolderPairs.size() > 0)
    {
        m_directoryLeft->SetSelection(wxNOT_FOUND);
        m_directoryRight->SetSelection(wxNOT_FOUND);

        m_directoryLeft->SetValue((*additionalFolderPairs.begin())->m_directoryLeft->GetValue());
        m_directoryRight->SetValue((*additionalFolderPairs.begin())->m_directoryRight->GetValue());

        removeFolderPair(0); //remove first of additional folder pairs

        //disable the sync button
        syncPreview->enableSynchronization(false);

        //clear grids
        currentGridData.clear();
        updateGuiGrid();
    }
}


void MainDialog::addFolderPair(const Zstring& leftDir, const Zstring& rightDir, bool addFront)
{
    std::vector<FolderPair> newPairs;
    newPairs.push_back(FolderPair(leftDir, rightDir));

    MainDialog::addFolderPair(newPairs, addFront);
}


void scrollToBottom(wxScrolledWindow* scrWindow)
{
    int height = 0;
    scrWindow->GetClientSize(NULL, &height);

    int pixelPerLine = 0;
    scrWindow->GetScrollPixelsPerUnit(NULL, &pixelPerLine);

    if (height > 0 && pixelPerLine > 0)
    {
        const int scrollLinesTotal    = scrWindow->GetScrollLines(wxVERTICAL);
        const int scrollLinesOnScreen = height / pixelPerLine;
        const int scrollPosBottom     = scrollLinesTotal - scrollLinesOnScreen;

        if (0 <= scrollPosBottom)
            scrWindow->Scroll(0, scrollPosBottom);
    }
}


void MainDialog::addFolderPair(const std::vector<FolderPair>& newPairs, bool addFront)
{
    if (newPairs.size() == 0)
        return;

    for (std::vector<FolderPair>::const_iterator i = newPairs.begin(); i != newPairs.end(); ++i)
    {
        //add new folder pair
        FolderPairPanel* newPair = new FolderPairPanel(m_scrolledWindowFolderPairs);
        newPair->m_bitmap23->SetBitmap(*GlobalResources::getInstance().bitmapLink);
        newPair->m_bpButtonRemovePair->SetBitmapLabel(*GlobalResources::getInstance().bitmapRemoveFolderPair);

        if (addFront)
        {
            bSizerFolderPairs->Insert(0, newPair, 0, wxEXPAND, 5);
            additionalFolderPairs.insert(additionalFolderPairs.begin(), newPair);
        }
        else
        {
            bSizerFolderPairs->Add(newPair, 0, wxEXPAND, 5);
            additionalFolderPairs.push_back(newPair);
        }

        //set size of scrolled window
        const wxSize pairSize = newPair->GetSize();

        const int additionalRows = additionalFolderPairs.size();
        if (additionalRows <= 3) //up to 3 additional pairs shall be shown
            m_scrolledWindowFolderPairs->SetMinSize(wxSize( -1, pairSize.GetHeight() * additionalRows));
        else //adjust scrollbars
            m_scrolledWindowFolderPairs->Fit();

        //register events
        newPair->m_bpButtonRemovePair->Connect(wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler(MainDialog::OnRemoveFolderPair), NULL, this );

        //insert directory names if provided
        newPair->m_directoryLeft->SetValue(i->leftDirectory.c_str());
        const wxString leftDirFormatted = FreeFileSync::getFormattedDirectoryName(i->leftDirectory).c_str();
        if (wxDirExists(leftDirFormatted))
            newPair->m_dirPickerLeft->SetPath(leftDirFormatted);

        newPair->m_directoryRight->SetValue(i->rightDirectory.c_str());
        const wxString rightDirFormatted = FreeFileSync::getFormattedDirectoryName(i->rightDirectory).c_str();
        if (wxDirExists(rightDirFormatted))
            newPair->m_dirPickerRight->SetPath(rightDirFormatted);
    }

    //adapt left-shift display distortion caused by scrollbars
    if (additionalFolderPairs.size() > 3 && !m_bitmapShift->IsShown())
    {
        //m_scrolledWindowFolderPairs and bSizerFolderPairs need to be up to date
        m_scrolledWindowFolderPairs->Layout();
        bSizer1->Layout();

        //calculate scrollbar width
        const int shiftToRight = m_scrolledWindowFolderPairs->GetSize().GetWidth() - bSizerFolderPairs->GetSize().GetWidth();
        m_bitmapShift->SetMinSize(wxSize(shiftToRight, -1));
        m_bitmapShift->Show();
    }

    //adapt delete top folder pair button
    if (additionalFolderPairs.size() > 0)
    {
        m_bpButtonRemoveTopPair->Show();
        m_panelTopRight->Layout();
    }

    //scroll to the bottom of wxScrolledWindow
    //scrollToBottom(m_scrolledWindowFolderPairs);

    m_scrolledWindowFolderPairs->Layout();
    bSizer1->Layout();
    m_bpButtonSwapSides->Refresh();
}


void MainDialog::removeFolderPair(const int pos, bool refreshLayout)
{
    if (0 <= pos && pos < int(additionalFolderPairs.size()))
    {
        wxSize pairSize;
        //remove folder pairs from window
        FolderPairPanel* pairToDelete = additionalFolderPairs[pos];
        pairSize = pairToDelete->GetSize();

        bSizerFolderPairs->Detach(pairToDelete); //Remove() does not work on Window*, so do it manually
        pairToDelete->Destroy();                 //
        additionalFolderPairs.erase(additionalFolderPairs.begin() + pos); //remove last element in vector

        //set size of scrolled window
        const int additionalRows = additionalFolderPairs.size();
        if (additionalRows <= 3) //up to 3 additional pairs shall be shown
            m_scrolledWindowFolderPairs->SetMinSize(wxSize(-1, pairSize.GetHeight() * additionalRows));
        //adjust scrollbars (do not put in else clause)
        m_scrolledWindowFolderPairs->Fit();

        //adapt left-shift display distortion caused by scrollbars
        if (refreshLayout)
        {
            if (additionalRows <= 3)
                m_bitmapShift->Hide();

            m_scrolledWindowFolderPairs->Layout();
            bSizer1->Layout();
        }

        //adapt delete top folder pair button
        if (additionalFolderPairs.size() == 0)
        {
            m_bpButtonRemoveTopPair->Hide();
            if (refreshLayout)
                m_panelTopRight->Layout();
        }

    }
}


void MainDialog::clearFolderPairs()
{
    if (additionalFolderPairs.size() > 0)
    {
        for (int i = int(additionalFolderPairs.size()) - 1; i > 0; --i)
            MainDialog::removeFolderPair(i, false);

        //refresh layout just once for improved performance!
        MainDialog::removeFolderPair(0, true);
    }
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
        if (FreeFileSync::fileExists(newFileName.c_str()))
        {
            wxMessageDialog* messageDlg = new wxMessageDialog(this, wxString(_("File already exists. Overwrite?")) + wxT(" \"") + newFileName + wxT("\""), _("Warning") , wxOK | wxCANCEL);

            if (messageDlg->ShowModal() != wxID_OK)
            {
                OnMenuExportFileList(event); //retry
                return;
            }
        }

        //begin work
        wxString exportString;
        for (int i = 0; i < m_gridLeft->GetNumberRows(); ++i)
        {
            for (int k = 0; k < m_gridLeft->GetNumberCols(); ++k)
            {
                exportString+= m_gridLeft->GetCellValue(i, k);
                exportString+= ';';
            }

            for (int k = 0; k < m_gridMiddle->GetNumberCols(); ++k)
            {
                exportString+= m_gridMiddle->GetCellValue(i, k);
                exportString+= ';';
            }

            for (int k = 0; k < m_gridRight->GetNumberCols(); ++k)
            {
                exportString+= m_gridRight->GetCellValue(i, k);
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
    xmlAccess::XmlBatchConfig batchCfg;
    batchCfg.mainCfg = cfg;
    batchCfg.directoryPairs = getFolderPairs();
    batchCfg.silent = false;

    if (ignoreErrors)
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

    Layout(); //strangely this layout call works if called in next idle event only
    Refresh();
}


void MainDialog::OnMenuAbout(wxCommandEvent& event)
{
    AboutDlg* aboutDlg = new AboutDlg(this);
    aboutDlg->ShowModal();
}


void MainDialog::OnMenuQuit(wxCommandEvent& event)
{
    requestShutdown();
}

//#########################################################################################################

//language selection
void MainDialog::switchProgramLanguage(const int langID)
{
    programLanguage->setLanguage(langID); //language is a global attribute

    //create new main window and delete old one
    cleanUp(); //destructor's code: includes writing settings to HD

    //create new dialog with respect to new language
    MainDialog* frame = new MainDialog(NULL, FreeFileSync::getLastConfigFile(), programLanguage, globalSettings);
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


SyncPreview::SyncPreview(MainDialog* mainDlg) :
        mainDlg_(mainDlg),
        syncPreviewEnabled(false),
        synchronizationEnabled(false) {}


bool SyncPreview::previewIsEnabled()
{
    return syncPreviewEnabled;
}


void SyncPreview::enablePreview(bool value)
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


void SyncPreview::enableSynchronization(bool value)
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
        mainDlg_->m_buttonStartSync->SetForegroundColour(wxColor(94, 94, 94)); //grey
        mainDlg_->m_buttonStartSync->setBitmapFront(*GlobalResources::getInstance().bitmapSyncDisabled);
    }
}


bool SyncPreview::synchronizationIsEnabled()
{
    return synchronizationEnabled;
}
