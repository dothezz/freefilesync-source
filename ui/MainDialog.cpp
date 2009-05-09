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
            if (mainDlg_->readConfigurationFromXml(dropName))
                mainDlg_->pushStatusInformation(_("Configuration loaded!"));
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
        mainDlg_->enableSynchronization(false);

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
        filteringInitialized(false),
        filteringPending(false),
        synchronizationEnabled(false),
        cmpStatusHandlerTmp(0),
        cleanedUp(false),
        lastSortColumn(-1),
        lastSortGrid(NULL)
{
    //initialize and load configuration
    readGlobalSettings();
    readConfigurationFromXml(cfgFileName, true);

    //set icons for this dialog
    m_bpButton10->SetBitmapLabel(*globalResource.bitmapExit);
    m_buttonCompare->setBitmapFront(*globalResource.bitmapCompare);
    m_buttonSync->setBitmapFront(*globalResource.bitmapSync);
    m_bpButtonSwap->SetBitmapLabel(*globalResource.bitmapSwap);
    m_bpButton14->SetBitmapLabel(*globalResource.bitmapHelp);
    m_bpButtonSave->SetBitmapLabel(*globalResource.bitmapSave);
    m_bpButtonLoad->SetBitmapLabel(*globalResource.bitmapLoad);
    m_bpButtonAddPair->SetBitmapLabel(*globalResource.bitmapAddFolderPair);
    m_bitmap15->SetBitmap(*globalResource.bitmapStatusEdge);
    m_bitmapShift->SetBitmap(*globalResource.bitmapShift);

    bSizer6->Layout(); //wxButtonWithImage size might have changed

    //menu icons: workaround for wxWidgets: small hack to update menu items: actually this is a wxWidgets bug (affects Windows- and Linux-build)
    MenuItemUpdater updateMenuFile(m_menuFile);
    updateMenuFile.addForUpdate(m_menuItem10, *globalResource.bitmapCompareSmall);
    updateMenuFile.addForUpdate(m_menuItem11, *globalResource.bitmapSyncSmall);

    MenuItemUpdater updateMenuAdv(m_menuAdvanced);
    updateMenuAdv.addForUpdate(m_menuItemGlobSett, *globalResource.bitmapSettingsSmall);
    updateMenuAdv.addForUpdate(m_menuItem7, *globalResource.bitmapBatchSmall);

    MenuItemUpdater updateMenuHelp(m_menuHelp);
    updateMenuHelp.addForUpdate(m_menuItemAbout, *globalResource.bitmapAboutSmall);

    MenuItemUpdater updateMenuAdvLang(m_menuLanguages);
    updateMenuAdvLang.addForUpdate(m_menuItemGerman,           *globalResource.bitmapGermany);
    updateMenuAdvLang.addForUpdate(m_menuItemEnglish,          *globalResource.bitmapEngland);
    updateMenuAdvLang.addForUpdate(m_menuItemSpanish,          *globalResource.bitmapSpain);
    updateMenuAdvLang.addForUpdate(m_menuItemFrench,           *globalResource.bitmapFrance);
    updateMenuAdvLang.addForUpdate(m_menuItemItalian,          *globalResource.bitmapItaly);
    updateMenuAdvLang.addForUpdate(m_menuItemHungarian,        *globalResource.bitmapHungary);
    updateMenuAdvLang.addForUpdate(m_menuItemDutch,            *globalResource.bitmapHolland);
    updateMenuAdvLang.addForUpdate(m_menuItemPolish,           *globalResource.bitmapPoland);
    updateMenuAdvLang.addForUpdate(m_menuItemPortuguese,       *globalResource.bitmapPortugal);
    updateMenuAdvLang.addForUpdate(m_menuItemPortugueseBrazil, *globalResource.bitmapBrazil);
    updateMenuAdvLang.addForUpdate(m_menuItemSlovenian,        *globalResource.bitmapSlovakia);
    updateMenuAdvLang.addForUpdate(m_menuItemJapanese,         *globalResource.bitmapJapan);
    updateMenuAdvLang.addForUpdate(m_menuItemChineseSimple,    *globalResource.bitmapChina);


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
    m_gridMiddle->GetGridWindow()->Connect(wxEVT_LEFT_UP, wxEventHandler(MainDialog::OnGrid3LeftMouseUp), NULL, this);
    m_gridMiddle->GetGridWindow()->Connect(wxEVT_LEFT_DOWN, wxEventHandler(MainDialog::OnGrid3LeftMouseDown), NULL, this);

    Connect(wxEVT_SIZE, wxEventHandler(MainDialog::onResizeMainWindow), NULL, this);
    Connect(wxEVT_MOVE, wxEventHandler(MainDialog::onResizeMainWindow), NULL, this);

    //init grid settings
    m_gridLeft->initSettings(  globalSettings.gui.showFileIcons,
                               m_gridLeft,
                               m_gridRight,
                               m_gridMiddle,
                               &gridDataView);

    m_gridMiddle->initSettings(m_gridLeft,
                               m_gridRight,
                               m_gridMiddle,
                               &gridDataView);

    m_gridRight->initSettings( globalSettings.gui.showFileIcons,
                               m_gridLeft,
                               m_gridRight,
                               m_gridMiddle,
                               &gridDataView);

    //disable sync button as long as "compare" hasn't been triggered.
    enableSynchronization(false);

    //mainly to update row label sizes...
    updateGuiGrid();

    enableSynchronization(false);

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
        writeConfigurationToXml(xmlAccess::LAST_CONFIG_FILE);   //don't trow exceptions in destructors
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
}


void MainDialog::enableSynchronization(bool value)
{
    if (value)
    {
        synchronizationEnabled = true;
        m_buttonSync->SetForegroundColour(*wxBLACK);
        m_buttonSync->setBitmapFront(*globalResource.bitmapSync);
    }
    else
    {
        synchronizationEnabled = false;
        m_buttonSync->SetForegroundColour(wxColor(94, 94, 94)); //grey
        m_buttonSync->setBitmapFront(*globalResource.bitmapSyncDisabled);
    }
}


void MainDialog::filterRangeManually(const std::set<int>& rowsToFilterOnUiTable)
{
    if (rowsToFilterOnUiTable.size() > 0)
    {
        bool newSelection = false; //default: deselect range

        //leadingRow determines de-/selection of all other rows
        const int leadingRow = *rowsToFilterOnUiTable.begin();
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
            for (std::set<int>::iterator i = markedRows.begin(); i != markedRows.end(); ++i)
            {
                markedRowsTotal.insert(*i);
                FreeFileSync::addSubElements(fileCmp, fileCmp[*i], markedRowsTotal);
            }

            //toggle selection of filtered rows
            for (std::set<int>::iterator i = markedRowsTotal.begin(); i != markedRowsTotal.end(); ++i)
                fileCmp[*i].selectedForSynchronization = newSelection;
        }

        //signal UI that grids need to be refreshed on next Update()
        m_gridLeft->ForceRefresh();
        m_gridMiddle->ForceRefresh();
        m_gridRight->ForceRefresh();
        Update();   //show changes resulting from ForceRefresh()

        if (hideFilteredElements)
        {
            wxMilliSleep(400); //some delay to show user the rows he has filtered out before they are removed
            updateGuiGrid();   //redraw grid to remove excluded elements (keeping sort sequence)

            m_gridLeft->ClearSelection();
            m_gridRight->ClearSelection();
            m_gridMiddle->ClearSelection();
        }
        else
        {   //this second call to ForceRefresh() looks strange, but it actually fixes occasional graphical artifacts on bottom right of the grid
            m_gridLeft->ForceRefresh();
            m_gridMiddle->ForceRefresh();
            m_gridRight->ForceRefresh();

            m_gridMiddle->ClearSelection();
        }
    }
}

/*grid event choreography:
1. UI-Mouse-Down => OnGridSelectCell
2. UI-Mouse-Up   => SelectRangeEvent (if at least two rows are marked)

=> the decision if a range or a single cell is selected can be made only after Mouse-UP. But SelectRangeEvent unfortunately is not always
executed (e.g. if single cell selected)

=> new choreography:
1. UI-Mouse-Down => OnGrid3LeftMouseDown (notify that filtering was initialized: this is needed since under some circumstances it might happen that the program
                    receives a mouse-up without a preceding mouse-down (double-clicks)
2. UI-Mouse-Up   => OnGrid3LeftMouseUp (notify that filtering shall be started on next idle event
3. UI-Mouse-Up   => SelectRangeEvent, possibly
4. Idle event    => OnIdleEvent
*/


void MainDialog::OnGrid3LeftMouseDown(wxEvent& event)
{
    filteringInitialized = true;
    event.Skip();
}


void MainDialog::OnGrid3LeftMouseUp(wxEvent& event)
{
    filteringPending = true;
    event.Skip();
}


void MainDialog::OnIdleEvent(wxEvent& event)
{
    //process manually filtered rows
    if (filteringPending)
    {
        filteringPending = false;

        if (filteringInitialized) //filteringInitialized is being reset after each selection, since strangely it might happen, that the grid receives
        {                         //a mouse up event, but no mouse down! (e.g. when window is maximized and cursor is on grid3)
            filteringInitialized = false;

            if (m_gridMiddle)
                filterRangeManually(getSelectedRows(m_gridMiddle));
        }
    }

    //------------------------------------------------------------------------------

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

        for (std::set<int>::iterator i = selectedRows.begin(); i != selectedRows.end(); ++i)
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


std::set<int> MainDialog::getSelectedRows(const CustomGrid* grid)
{
    std::set<int> output;
    int rowTop, rowBottom; //coords of selection

    wxArrayInt selectedRows = grid->GetSelectedRows();
    if (!selectedRows.IsEmpty())
    {
        for (unsigned int i = 0; i < selectedRows.GetCount(); ++i)
            output.insert(selectedRows[i]);
    }

    if (!grid->GetSelectedCols().IsEmpty())    //if a column is selected this is means all rows are marked for deletion
    {
        for (int k = 0; k < const_cast<CustomGrid*>(grid)->GetNumberRows(); ++k) //messy wxGrid implementation...
            output.insert(k);
    }

    wxGridCellCoordsArray singlySelected = grid->GetSelectedCells();
    if (!singlySelected.IsEmpty())
    {
        for (unsigned int k = 0; k < singlySelected.GetCount(); ++k)
            output.insert(singlySelected[k].GetRow());
    }

    wxGridCellCoordsArray tmpArrayTop = grid->GetSelectionBlockTopLeft();
    if (!tmpArrayTop.IsEmpty())
    {
        wxGridCellCoordsArray tmpArrayBottom = grid->GetSelectionBlockBottomRight();

        unsigned int arrayCount = tmpArrayTop.GetCount();

        if (arrayCount == tmpArrayBottom.GetCount())
        {
            for (unsigned int i = 0; i < arrayCount; ++i)
            {
                rowTop    = tmpArrayTop[i].GetRow();
                rowBottom = tmpArrayBottom[i].GetRow();

                for (int k = rowTop; k <= rowBottom; ++k)
                    output.insert(k);
            }
        }
    }

    //some exception: also add current cursor row to selection if there are no others... hopefully improving usability
    if (output.empty() && grid->isLeadGrid())
        output.insert(const_cast<CustomGrid*>(grid)->GetCursorRow()); //messy wxGrid implementation...

    removeInvalidRows(output, gridDataView.elementsOnView());

    return output;
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

    wxString defaultFolder;
    if (0 <= rowNumber && rowNumber < int(gridDataView.elementsOnView()))
        defaultFolder = leftSide ?
                        gridDataView.getFolderPair(rowNumber).leftDirectory.c_str() :
                        gridDataView.getFolderPair(rowNumber).rightDirectory.c_str();
    else
        defaultFolder = leftSide ?
                        FreeFileSync::getFormattedDirectoryName(m_directoryLeft->GetValue().c_str()).c_str() :
                        FreeFileSync::getFormattedDirectoryName(m_directoryRight->GetValue().c_str()).c_str();

#ifdef FFS_WIN
    command = wxString(wxT("explorer ")) + defaultFolder; //default
#elif defined FFS_LINUX
    command = globalSettings.gui.commandLineFileManager;
    command.Replace(wxT("%name"), defaultFolder);
    command.Replace(wxT("%path"), defaultFolder);
#endif

    if (0 <= rowNumber && rowNumber < int(gridDataView.elementsOnView()))
    {
        const FileDescrLine* fileDescr = leftSide ?
                                         &gridDataView[rowNumber].fileDescrLeft :
                                         &gridDataView[rowNumber].fileDescrRight;

        if (fileDescr->objType == FileDescrLine::TYPE_FILE)
        {
            command = globalSettings.gui.commandLineFileManager;
            command.Replace(wxT("%name"), fileDescr->fullName.c_str());
            command.Replace(wxT("%path"), wxString(fileDescr->fullName.c_str()).BeforeLast(GlobalResources::FILE_NAME_SEPARATOR));
        }
        else if (fileDescr->objType == FileDescrLine::TYPE_DIRECTORY)
        {
            command = globalSettings.gui.commandLineFileManager;
            command.Replace(wxT("%name"), fileDescr->fullName.c_str());
            command.Replace(wxT("%path"), fileDescr->fullName.c_str());
        }
        else if (fileDescr->objType == FileDescrLine::TYPE_NOTHING)
        {
        }
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


void MainDialog::OnContextMenu(wxGridEvent& event)
{
    std::set<int> selectionLeft  = getSelectedRows(m_gridLeft);
    std::set<int> selectionRight = getSelectedRows(m_gridRight);

//#######################################################
    //re-create context menu
    contextMenu.reset(new wxMenu);

    //CONTEXT_FILTER_TEMP
    if (selectionLeft.size() + selectionRight.size() > 0)
    {
        int selectionBegin = 0;
        if (!selectionLeft.size())
            selectionBegin = *selectionRight.begin();
        else if (!selectionRight.size())
            selectionBegin = *selectionLeft.begin();
        else
            selectionBegin = std::min(*selectionLeft.begin(), *selectionRight.begin());

        if (gridDataView[selectionBegin].selectedForSynchronization) //valid access, as getSelectedRows returns valid references only
            contextMenu->Append(CONTEXT_FILTER_TEMP, _("Exclude temporarily"));
        else
            contextMenu->Append(CONTEXT_FILTER_TEMP, _("Include temporarily"));
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
    for (std::set<int>::iterator i = selectionLeft.begin(); i != selectionLeft.end(); ++i)
    {
        const FileCompareLine& line = gridDataView[*i];
        newFilterEntry.relativeName = line.fileDescrLeft.relativeName.c_str();
        newFilterEntry.type         = line.fileDescrLeft.objType;
        if (!newFilterEntry.relativeName.IsEmpty())
            exFilterCandidateObj.push_back(newFilterEntry);
    }
    for (std::set<int>::iterator i = selectionRight.begin(); i != selectionRight.end(); ++i)
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
        const wxString filename = exFilterCandidateObj[0].relativeName.AfterLast(GlobalResources::FILE_NAME_SEPARATOR);
        if (filename.Find(wxChar('.')) !=  wxNOT_FOUND) //be careful: AfterLast will return the whole string if '.' is not found!
        {
            exFilterCandidateExtension = filename.AfterLast(wxChar('.'));
            contextMenu->Append(CONTEXT_EXCLUDE_EXT, wxString(_("Exclude via filter:")) + wxT(" ") + wxT("*.") + exFilterCandidateExtension);
        }
    }


    //CONTEXT_EXCLUDE_OBJ
    if (exFilterCandidateObj.size() == 1)
        contextMenu->Append(CONTEXT_EXCLUDE_OBJ, wxString(_("Exclude via filter:")) + wxT(" ") + exFilterCandidateObj[0].relativeName.AfterLast(GlobalResources::FILE_NAME_SEPARATOR));
    else if (exFilterCandidateObj.size() > 1)
        contextMenu->Append(CONTEXT_EXCLUDE_OBJ, wxString(_("Exclude via filter:")) + wxT(" ") + _("<multiple selection>"));

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

    contextMenu->Connect(wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(MainDialog::OnContextMenuSelection), NULL, this);

    //show context menu
    PopupMenu(contextMenu.get());
    event.Skip();
}


void MainDialog::OnContextMenuSelection(wxCommandEvent& event)
{
    int eventId = event.GetId();
    if (eventId == CONTEXT_FILTER_TEMP)
    {
        //merge selections from left and right grid
        std::set<int> selection = getSelectedRows(m_gridLeft);
        std::set<int> additional = getSelectedRows(m_gridRight);
        for (std::set<int>::const_iterator i = additional.begin(); i != additional.end(); ++i)
            selection.insert(*i);

        filterRangeManually(selection);
    }

    else if (eventId == CONTEXT_EXCLUDE_EXT)
    {
        if (!exFilterCandidateExtension.IsEmpty())
        {
            if (!cfg.excludeFilter.IsEmpty() && !cfg.excludeFilter.EndsWith(wxT(";")))
                cfg.excludeFilter+= wxT("\n");

            cfg.excludeFilter+= wxString(wxT("*.")) + exFilterCandidateExtension + wxT(";"); //';' is appended to 'mark' that next exclude extension entry won't write to new line

            cfg.filterIsActive = true;
            updateFilterButton(m_bpButtonFilter, cfg.filterIsActive);

            FreeFileSync::filterGridData(currentGridData, cfg.includeFilter, cfg.excludeFilter);

            updateGuiGrid();
            if (hideFilteredElements)
            {
                m_gridLeft->ClearSelection();
                m_gridRight->ClearSelection();
                m_gridMiddle->ClearSelection();
            }
        }
    }

    else if (eventId == CONTEXT_EXCLUDE_OBJ)
    {
        if (exFilterCandidateObj.size() > 0) //check needed to determine if filtering is needed
        {
            for (std::vector<FilterObject>::const_iterator i = exFilterCandidateObj.begin(); i != exFilterCandidateObj.end(); ++i)
            {
                if (!cfg.excludeFilter.IsEmpty() && !cfg.excludeFilter.EndsWith(wxT("\n")))
                    cfg.excludeFilter+= wxT("\n");

                if (i->type == FileDescrLine::TYPE_FILE)
                    cfg.excludeFilter+= wxString(wxT("*")) + GlobalResources::FILE_NAME_SEPARATOR + i->relativeName;
                else if (i->type == FileDescrLine::TYPE_DIRECTORY)
                    cfg.excludeFilter+= wxString(wxT("*")) + GlobalResources::FILE_NAME_SEPARATOR + i->relativeName + GlobalResources::FILE_NAME_SEPARATOR + wxT("*");

                else assert(false);
            }

            cfg.filterIsActive = true;
            updateFilterButton(m_bpButtonFilter, cfg.filterIsActive);

            FreeFileSync::filterGridData(currentGridData, cfg.includeFilter, cfg.excludeFilter);

            updateGuiGrid();
            if (hideFilteredElements)
            {
                m_gridLeft->ClearSelection();
                m_gridRight->ClearSelection();
                m_gridMiddle->ClearSelection();
            }
        }
    }

    else if (eventId == CONTEXT_CLIPBOARD)
    {
        if (m_gridLeft->isLeadGrid())
            copySelectionToClipboard(m_gridLeft);
        else if (m_gridRight->isLeadGrid())
            copySelectionToClipboard(m_gridRight);
    }

    else if (eventId == CONTEXT_EXPLORER)
    {
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
    }

    else if (eventId == CONTEXT_DELETE_FILES)
    {
        deleteSelectedFiles();
    }
}


void MainDialog::OnContextMenuMiddle(wxGridEvent& event)
{
    contextMenu.reset(new wxMenu); //re-create context menu
    contextMenu->Append(CONTEXT_CHECK_ALL, _("Check all"));
    contextMenu->Append(CONTEXT_UNCHECK_ALL, _("Uncheck all"));

    if (gridDataView.elementsTotal() == 0)
    {
        contextMenu->Enable(CONTEXT_CHECK_ALL, false);
        contextMenu->Enable(CONTEXT_UNCHECK_ALL, false);
    }

    contextMenu->Connect(wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(MainDialog::OnContextMenuMiddleSelection), NULL, this);
    PopupMenu(contextMenu.get()); //show context menu

    event.Skip();
}


void MainDialog::OnContextMenuMiddleSelection(wxCommandEvent& event)
{
    int eventId = event.GetId();
    if (eventId == CONTEXT_CHECK_ALL)
    {
        FreeFileSync::includeAllRowsOnGrid(currentGridData);
        updateGuiGrid();
    }
    else if (eventId == CONTEXT_UNCHECK_ALL)
    {
        FreeFileSync::excludeAllRowsOnGrid(currentGridData);
        updateGuiGrid();
    }
}


void MainDialog::OnContextColumnLeft(wxGridEvent& event)
{
    contextMenu.reset(new wxMenu); //re-create context menu
    contextMenu->Append(CONTEXT_CUSTOMIZE_COLUMN_LEFT, _("Customize columns"));
    contextMenu->Connect(wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(MainDialog::OnContextColumnSelection), NULL, this);
    PopupMenu(contextMenu.get()); //show context menu

    event.Skip();
}


void MainDialog::OnContextColumnRight(wxGridEvent& event)
{
    contextMenu.reset(new wxMenu); //re-create context menu
    contextMenu->Append(CONTEXT_CUSTOMIZE_COLUMN_RIGHT, _("Customize columns"));
    contextMenu->Connect(wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(MainDialog::OnContextColumnSelection), NULL, this);
    PopupMenu(contextMenu.get()); //show context menu

    event.Skip();
}


void MainDialog::OnContextColumnSelection(wxCommandEvent& event)
{
    int eventId = event.GetId();
    if (eventId == CONTEXT_CUSTOMIZE_COLUMN_LEFT)
    {
        xmlAccess::ColumnAttributes colAttr = m_gridLeft->getColumnAttributes();
        CustomizeColsDlg* customizeDlg = new CustomizeColsDlg(this, colAttr);
        if (customizeDlg->ShowModal() == CustomizeColsDlg::BUTTON_OKAY)
        {
            m_gridLeft->setColumnAttributes(colAttr);

            m_gridLeft->setSortMarker(-1); //hide sort direction indicator on GUI grids
            m_gridMiddle->setSortMarker(-1);
            m_gridRight->setSortMarker(-1);
        }
    }
    else if (eventId == CONTEXT_CUSTOMIZE_COLUMN_RIGHT)
    {
        xmlAccess::ColumnAttributes colAttr = m_gridRight->getColumnAttributes();
        CustomizeColsDlg* customizeDlg = new CustomizeColsDlg(this, colAttr);
        if (customizeDlg->ShowModal() == CustomizeColsDlg::BUTTON_OKAY)
        {
            m_gridRight->setColumnAttributes(colAttr);
            m_gridLeft->setSortMarker(-1); //hide sort direction indicator on GUI grids
            m_gridMiddle->setSortMarker(-1);
            m_gridRight->setSortMarker(-1);
        }
    }
}


void MainDialog::OnDirSelected(wxFileDirPickerEvent& event)
{
    //left and right directory text-control and dirpicker are synchronized by MainFolderDragDrop automatically

    //disable the sync button
    enableSynchronization(false);

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
        file1Full = wxFileName::GetCwd() + GlobalResources::FILE_NAME_SEPARATOR + file1;

    if (wxFileName(file2).GetPath() == wxEmptyString)
        file2Full = wxFileName::GetCwd() + GlobalResources::FILE_NAME_SEPARATOR + file2;

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
        if (sameFileSpecified(xmlAccess::LAST_CONFIG_FILE, filename))
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
    const wxString defaultFileName = proposedConfigFileName.empty() ? wxT("SyncSettings.ffs_gui") : proposedConfigFileName;

    wxFileDialog* filePicker = new wxFileDialog(this, wxEmptyString, wxEmptyString, defaultFileName, wxString(_("FreeFileSync configuration")) + wxT(" (*.ffs_gui)|*.ffs_gui"), wxFD_SAVE);
    if (filePicker->ShowModal() == wxID_OK)
    {
        const wxString newFileName = filePicker->GetPath();

        if (FreeFileSync::fileExists(newFileName.c_str()))
        {
            wxMessageDialog* messageDlg = new wxMessageDialog(this, wxString(_("File already exists. Overwrite?")) + wxT(" \"") + newFileName + wxT("\""), _("Warning") , wxOK | wxCANCEL);

            if (messageDlg->ShowModal() != wxID_OK)
            {
                OnSaveConfig(event); //retry
                return;
            }
        }
        if (writeConfigurationToXml(newFileName))
            pushStatusInformation(_("Configuration saved!"));
    }
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
    /*
    if (globalSettings.gui.popupOnConfigChange)
    {
        if (lastConfigurationSaved != getCurrentConfiguration())
        {
            ...
wxID_OK
            OnSaveConfig(wxCommandEvent& event)

            ;
            wxMessageBox(wxT("ji"));
        }
    }
    */

    Destroy();
}


bool MainDialog::readConfigurationFromXml(const wxString& filename, bool programStartup)
{
    leftOnlyFilesActive   = true;
    leftNewerFilesActive  = true;
    differentFilesActive  = true;
    rightNewerFilesActive = true;  //do not save/load these bool values from harddisk!
    rightOnlyFilesActive  = true;  //it's more convenient to have them defaulted at startup
    equalFilesActive      = false;
    updateViewFilterButtons();


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
            if (filename == xmlAccess::LAST_CONFIG_FILE && !wxFileExists(filename)) //do not show error in this case
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

    //###########################################################
    addFileToCfgHistory(filename); //put filename on list of last used config files

    lastConfigurationSaved = guiCfg;

    if (filename == xmlAccess::LAST_CONFIG_FILE) //set title
    {
        SetTitle(wxString(wxT("FreeFileSync - ")) + _("Folder Comparison and Synchronization"));
        proposedConfigFileName.clear();
    }
    else
    {
        SetTitle(wxString(wxT("FreeFileSync - ")) + filename);
        proposedConfigFileName = filename;
    }

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

    if (filename == xmlAccess::LAST_CONFIG_FILE) //set title
    {
        SetTitle(wxString(wxT("FreeFileSync - ")) + _("Folder Comparison and Synchronization"));
        proposedConfigFileName.clear();
    }
    else
    {
        SetTitle(wxString(wxT("FreeFileSync - ")) + filename);
        proposedConfigFileName = filename;
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

    return guiCfg;
}


void MainDialog::OnShowHelpDialog(wxCommandEvent &event)
{
    HelpDlg* helpDlg = new HelpDlg(this);
    helpDlg->ShowModal();
}


void MainDialog::OnFilterButton(wxCommandEvent &event)
{   //toggle filter on/off
    cfg.filterIsActive = !cfg.filterIsActive;
    //make sure, button-appearance and "filterIsActive" are in sync.
    updateFilterButton(m_bpButtonFilter, cfg.filterIsActive);

    if (cfg.filterIsActive)
        FreeFileSync::filterGridData(currentGridData, cfg.includeFilter, cfg.excludeFilter);
    else
        FreeFileSync::includeAllRowsOnGrid(currentGridData);

    updateGuiGrid();
}


void MainDialog::OnHideFilteredButton(wxCommandEvent &event)
{   //toggle showing filtered rows
    hideFilteredElements = !hideFilteredElements;
    //make sure, checkbox and "hideFiltered" are in sync
    m_checkBoxHideFilt->SetValue(hideFilteredElements);

    m_gridLeft->ClearSelection();
    m_gridRight->ClearSelection();

    updateGuiGrid();

    event.Skip();
}


void MainDialog::OnConfigureFilter(wxHyperlinkEvent &event)
{
    wxString beforeImage = cfg.includeFilter + wxChar(1) + cfg.excludeFilter;

    FilterDlg* filterDlg = new FilterDlg(this, cfg.includeFilter, cfg.excludeFilter);
    if (filterDlg->ShowModal() == FilterDlg::BUTTON_OKAY)
    {
        wxString afterImage = cfg.includeFilter + wxChar(1) + cfg.excludeFilter;

        if (beforeImage != afterImage)  //if filter settings are changed: set filtering to "on"
        {
            if (afterImage == (wxString(wxT("*")) + wxChar(1))) //default
            {
                cfg.filterIsActive = false;
                FreeFileSync::includeAllRowsOnGrid(currentGridData);
            }
            else
            {
                cfg.filterIsActive = true;
                FreeFileSync::filterGridData(currentGridData, cfg.includeFilter, cfg.excludeFilter);
            }

            updateFilterButton(m_bpButtonFilter, cfg.filterIsActive);

            updateGuiGrid();
        }
    }
    //no event.Skip() here, to not start browser
}


void MainDialog::OnLeftOnlyFiles(wxCommandEvent& event)
{
    leftOnlyFilesActive = !leftOnlyFilesActive;
    updateViewFilterButtons();
    updateGuiGrid();
};

void MainDialog::OnLeftNewerFiles(wxCommandEvent& event)
{
    leftNewerFilesActive = !leftNewerFilesActive;
    updateViewFilterButtons();
    updateGuiGrid();
};

void MainDialog::OnDifferentFiles(wxCommandEvent& event)
{
    differentFilesActive = !differentFilesActive;
    updateViewFilterButtons();
    updateGuiGrid();
};

void MainDialog::OnRightNewerFiles(wxCommandEvent& event)
{
    rightNewerFilesActive = !rightNewerFilesActive;
    updateViewFilterButtons();
    updateGuiGrid();
};

void MainDialog::OnRightOnlyFiles(wxCommandEvent& event)
{
    rightOnlyFilesActive = !rightOnlyFilesActive;
    updateViewFilterButtons();
    updateGuiGrid();
};

void MainDialog::OnEqualFiles(wxCommandEvent& event)
{
    equalFilesActive = !equalFilesActive;
    updateViewFilterButtons();
    updateGuiGrid();
};


void MainDialog::updateViewFilterButtons()
{
    if (leftOnlyFilesActive)
    {
        m_bpButtonLeftOnly->SetBitmapLabel(*globalResource.bitmapLeftOnlyAct);
        m_bpButtonLeftOnly->SetToolTip(_("Hide files that exist on left side only"));
    }
    else
    {
        m_bpButtonLeftOnly->SetBitmapLabel(*globalResource.bitmapLeftOnlyDeact);
        m_bpButtonLeftOnly->SetToolTip(_("Show files that exist on left side only"));
    }

    if (leftNewerFilesActive)
    {
        m_bpButtonLeftNewer->SetBitmapLabel(*globalResource.bitmapLeftNewerAct);
        m_bpButtonLeftNewer->SetToolTip(_("Hide files that are newer on left"));
    }
    else
    {
        m_bpButtonLeftNewer->SetBitmapLabel(*globalResource.bitmapLeftNewerDeact);
        m_bpButtonLeftNewer->SetToolTip(_("Show files that are newer on left"));
    }

    if (equalFilesActive)
    {
        m_bpButtonEqual->SetBitmapLabel(*globalResource.bitmapEqualAct);
        m_bpButtonEqual->SetToolTip(_("Hide files that are equal"));
    }
    else
    {
        m_bpButtonEqual->SetBitmapLabel(*globalResource.bitmapEqualDeact);
        m_bpButtonEqual->SetToolTip(_("Show files that are equal"));
    }

    if (differentFilesActive)
    {
        m_bpButtonDifferent->SetBitmapLabel(*globalResource.bitmapDifferentAct);
        m_bpButtonDifferent->SetToolTip(_("Hide files that are different"));
    }
    else
    {
        m_bpButtonDifferent->SetBitmapLabel(*globalResource.bitmapDifferentDeact);
        m_bpButtonDifferent->SetToolTip(_("Show files that are different"));
    }

    if (rightNewerFilesActive)
    {
        m_bpButtonRightNewer->SetBitmapLabel(*globalResource.bitmapRightNewerAct);
        m_bpButtonRightNewer->SetToolTip(_("Hide files that are newer on right"));
    }
    else
    {
        m_bpButtonRightNewer->SetBitmapLabel(*globalResource.bitmapRightNewerDeact);
        m_bpButtonRightNewer->SetToolTip(_("Show files that are newer on right"));
    }

    if (rightOnlyFilesActive)
    {
        m_bpButtonRightOnly->SetBitmapLabel(*globalResource.bitmapRightOnlyAct);
        m_bpButtonRightOnly->SetToolTip(_("Hide files that exist on right side only"));
    }
    else
    {
        m_bpButtonRightOnly->SetBitmapLabel(*globalResource.bitmapRightOnlyDeact);
        m_bpButtonRightOnly->SetToolTip(_("Show files that exist on right side only"));
    }
}


void MainDialog::updateFilterButton(wxBitmapButton* filterButton, bool isActive)
{
    if (isActive)
    {
        filterButton->SetBitmapLabel(*globalResource.bitmapFilterOn);
        filterButton->SetToolTip(_("Filter active: Press again to deactivate"));
    }
    else
    {
        filterButton->SetBitmapLabel(*globalResource.bitmapFilterOff);
        filterButton->SetToolTip(_("Press button to activate filter"));
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
    enableSynchronization(false);

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

        FreeFileSync::CompareProcess comparison(globalSettings.shared.traverseDirectorySymlinks,
                                                globalSettings.shared.fileTimeTolerance,
                                                globalSettings.shared.warningDependentFolders,
                                                &statusHandler);

        comparison.startCompareProcess(getFolderPairs(),
                                       cfg.compareVar,
                                       currentGridData);

        //if (output.size < 50000)
        statusHandler.updateStatusText(_("Sorting file list..."));
        statusHandler.forceUiRefresh(); //keep total number of scanned files up to date

        gridDataView.sortView(GridView::SORT_BY_DIRECTORY, true, true);

        //filter currentGridData if option is set
        if (cfg.filterIsActive)
            FreeFileSync::filterGridData(currentGridData, cfg.includeFilter, cfg.excludeFilter);
    }
    catch (AbortThisProcess& theException)
    {
        aborted = true;
    }
    cmpStatusHandlerTmp = 0;

    if (aborted)
    {   //disable the sync button
        enableSynchronization(false);
        m_buttonCompare->SetFocus();
    }
    else
    {   //once compare is finished enable the sync button
        enableSynchronization(true);
        m_buttonSync->SetFocus();

        //hide sort direction indicator on GUI grids
        m_gridLeft->setSortMarker(-1);
        m_gridMiddle->setSortMarker(-1);
        m_gridRight->setSortMarker(-1);

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

    m_gridLeft->EndBatch();
    m_gridMiddle->EndBatch();
    m_gridRight->EndBatch();
}


void MainDialog::OnSync(wxCommandEvent& event)
{
    SyncDialog* syncDlg = new SyncDialog(this, currentGridData, cfg, ignoreErrors, synchronizationEnabled);
    if (syncDlg->ShowModal() == SyncDialog::BUTTON_START)
    {
        //check if there are files/folders to be sync'ed at all
        if (!synchronizationNeeded(currentGridData, cfg.syncConfiguration))
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
                globalSettings.shared.copyFileSymlinks,
                globalSettings.shared.traverseDirectorySymlinks,
                globalSettings.shared.warningSignificantDifference,
                globalSettings.shared.warningNotEnoughDiskSpace,
                &statusHandler);

            synchronization.startSynchronizationProcess(currentGridData, cfg.syncConfiguration);
        }
        catch (AbortThisProcess& theException)
        {   //do NOT disable the sync button: user might want to try to sync the REMAINING rows
        }   //enableSynchronization(false);


        //show remaining files that have not been processed: put DIRECTLY after startSynchronizationProcess() and DON'T call any wxWidgets functions
        //in between! Else CustomGrid might access obsolete data entries in currentGridData!
        updateGuiGrid();

        m_gridLeft->ClearSelection();
        m_gridMiddle->ClearSelection();
        m_gridRight->ClearSelection();

        if (gridDataView.elementsTotal() > 0)
            pushStatusInformation(_("Not all items were synchronized! Have a look at the list."));
        else
        {
            pushStatusInformation(_("All items have been synchronized!"));
            enableSynchronization(false);
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
        case xmlAccess::FULL_NAME:
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
        m_gridMiddle->setSortMarker(-1);
        m_gridRight->setSortMarker(-1);
        if (sortAscending)
            m_gridLeft->setSortMarker(currentSortColumn, globalResource.bitmapSmallUp);
        else
            m_gridLeft->setSortMarker(currentSortColumn, globalResource.bitmapSmallDown);
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
    gridDataView.sortView(GridView::SORT_BY_CMP_RESULT, true, sortAscending);

    updateGuiGrid(); //refresh gridDataView

    //set sort direction indicator on UI
    m_gridLeft->setSortMarker(-1);
    m_gridRight->setSortMarker(-1);
    if (sortAscending)
        m_gridMiddle->setSortMarker(0, globalResource.bitmapSmallUp);
    else
        m_gridMiddle->setSortMarker(0, globalResource.bitmapSmallDown);
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
        case xmlAccess::FULL_NAME:
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
        m_gridLeft->setSortMarker(-1);
        m_gridMiddle->setSortMarker(-1);
        if (sortAscending)
            m_gridRight->setSortMarker(currentSortColumn, globalResource.bitmapSmallUp);
        else
            m_gridRight->setSortMarker(currentSortColumn, globalResource.bitmapSmallDown);
    }
}


void MainDialog::OnSwapDirs( wxCommandEvent& event )
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
    std::swap(leftOnlyFilesActive, rightOnlyFilesActive);
    std::swap(leftNewerFilesActive, rightNewerFilesActive);
    updateViewFilterButtons();

    //swap grid information
    FreeFileSync::swapGrids(currentGridData);
    updateGuiGrid();
    event.Skip();
}


void MainDialog::updateGridViewData()
{
    const GridView::StatusInfo result = gridDataView.update(
                                            leftOnlyFilesActive,
                                            rightOnlyFilesActive,
                                            leftNewerFilesActive,
                                            rightNewerFilesActive,
                                            differentFilesActive,
                                            equalFilesActive,
                                            hideFilteredElements);

    //hide or enable view filter buttons
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

    if (    result.existsLeftOnly   ||
            result.existsRightOnly  ||
            result.existsLeftNewer  ||
            result.existsRightNewer ||
            result.existsDifferent  ||
            result.existsEqual)
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
    const unsigned int objCount = gridDataView.elementsTotal();
    if (objCount == 1)
    {
        wxString outputString = _("%x of 1 row in view");
        outputString.Replace(wxT("%x"), objectsView, false);
        statusMiddleNew = outputString;
    }
    else
    {
        const wxString objectsTotal = globalFunctions::includeNumberSeparator(globalFunctions::numberToWxString(objCount));

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
    addFolderPair(wxEmptyString, wxEmptyString);

    //disable the sync button
    enableSynchronization(false);

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
            enableSynchronization(false);

            //clear grids
            currentGridData.clear();
            updateGuiGrid();
            return;
        }
    }
}


void MainDialog::addFolderPair(const Zstring& leftDir, const Zstring& rightDir)
{
    std::vector<FolderPair> newPairs;
    FolderPair pair;
    pair.leftDirectory = leftDir;
    pair.rightDirectory = rightDir;
    newPairs.push_back(pair);

    MainDialog::addFolderPair(newPairs);
}


void MainDialog::addFolderPair(const std::vector<FolderPair>& newPairs)
{
    if (newPairs.size() == 0)
        return;

    for (std::vector<FolderPair>::const_iterator i = newPairs.begin(); i != newPairs.end(); ++i)
    {
        //add new folder pair
        FolderPairPanel* newPair = new FolderPairPanel(m_scrolledWindowFolderPairs);
        newPair->m_bitmap23->SetBitmap(*globalResource.bitmapLink);
        newPair->m_bpButtonRemovePair->SetBitmapLabel(*globalResource.bitmapRemoveFolderPair);

        bSizerFolderPairs->Add(newPair, 0, wxEXPAND, 5);
        additionalFolderPairs.push_back(newPair);

        //set size of scrolled window
        wxSize pairSize = newPair->GetSize();

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

    m_scrolledWindowFolderPairs->Layout();
    bSizer1->Layout();
    m_bpButtonSwap->Refresh();
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

        if (refreshLayout)
        {
            //adapt left-shift display distortion caused by scrollbars
            if (additionalFolderPairs.size() <= 3)
                m_bitmapShift->Hide();

            m_scrolledWindowFolderPairs->Layout();
            bSizer1->Layout();
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

    FreeFileSync::checkForUpdatePeriodically(globalSettings.shared.lastUpdateCheck);
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
    MainDialog* frame = new MainDialog(NULL, xmlAccess::LAST_CONFIG_FILE, programLanguage, globalSettings);
    frame->SetIcon(*globalResource.programIcon); //set application icon
    frame->Show();

    Destroy();
}


void MainDialog::OnMenuLangChineseSimp(wxCommandEvent& event)
{
    switchProgramLanguage(wxLANGUAGE_CHINESE_SIMPLIFIED);
}


void MainDialog::OnMenuLangDutch(wxCommandEvent& event)
{
    switchProgramLanguage(wxLANGUAGE_DUTCH);
}


void MainDialog::OnMenuLangEnglish(wxCommandEvent& event)
{
    switchProgramLanguage(wxLANGUAGE_ENGLISH);
}


void MainDialog::OnMenuLangFrench(wxCommandEvent& event)
{
    switchProgramLanguage(wxLANGUAGE_FRENCH);
}


void MainDialog::OnMenuLangGerman(wxCommandEvent& event)
{
    switchProgramLanguage(wxLANGUAGE_GERMAN);
}


void MainDialog::OnMenuLangHungarian(wxCommandEvent& event)
{
    switchProgramLanguage(wxLANGUAGE_HUNGARIAN);
}


void MainDialog::OnMenuLangItalian(wxCommandEvent& event)
{
    switchProgramLanguage(wxLANGUAGE_ITALIAN);
}


void MainDialog::OnMenuLangJapanese(wxCommandEvent& event)
{
    switchProgramLanguage(wxLANGUAGE_JAPANESE);
}


void MainDialog::OnMenuLangPolish(wxCommandEvent& event)
{
    switchProgramLanguage(wxLANGUAGE_POLISH);
}


void MainDialog::OnMenuLangPortuguese(wxCommandEvent& event)
{
    switchProgramLanguage(wxLANGUAGE_PORTUGUESE);
}


void MainDialog::OnMenuLangPortugueseBrazil(wxCommandEvent& event)
{
    switchProgramLanguage(wxLANGUAGE_PORTUGUESE_BRAZILIAN);
}


void MainDialog::OnMenuLangSlovenian(wxCommandEvent& event)
{
    switchProgramLanguage(wxLANGUAGE_SLOVENIAN);
}


void MainDialog::OnMenuLangSpanish(wxCommandEvent& event)
{
    switchProgramLanguage(wxLANGUAGE_SPANISH);
}


