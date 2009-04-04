/***************************************************************
 * Purpose:   Code for main dialog
 * Author:    ZenJu (zhnmju123@gmx.de)
 * Created:   2008-07-16
 **************************************************************/

#include "mainDialog.h"
#include <wx/filename.h>
#include "../library/globalFunctions.h"
#include <wx/wfstream.h>
#include <wx/clipbrd.h>
#include <wx/ffile.h>
#include "../library/customGrid.h"
#include "../library/customButton.h"
#include <algorithm>
#include "../library/sorting.h"
#include <wx/msgdlg.h>
#include "../comparison.h"
#include "../synchronization.h"
#include "../algorithm.h"
#include "checkVersion.h"


MainDialog::MainDialog(wxFrame* frame, const wxString& cfgFileName, CustomLocale* language, xmlAccess::XmlGlobalSettings& settings) :
        MainDialogGenerated(frame),
        globalSettings(settings),
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

    //menu icons
    m_menuItem10->SetBitmap(*globalResource.bitmapCompareSmall);
    m_menuItem11->SetBitmap(*globalResource.bitmapSyncSmall);
    m_menuItem7->SetBitmap(*globalResource.bitmapBatchSmall);
    m_menuItemGlobSett->SetBitmap(*globalResource.bitmapSettingsSmall);
    m_menuItemAbout->SetBitmap(*globalResource.bitmapAboutSmall);

    //workaround for wxWidgets: small hack to update menu items: actually this is a wxWidgets bug (affects Windows- and Linux-build)
    m_menu1->Remove(m_menuItem10);
    m_menu1->Remove(m_menuItem11);
    m_menu1->Insert(0, m_menuItem10);
    m_menu1->Insert(1, m_menuItem11);
    m_menu3->Remove(m_menuItemGlobSett);
    m_menu3->Remove(m_menuItem7);
    m_menu3->Insert(2, m_menuItemGlobSett);
    m_menu3->Insert(3, m_menuItem7);

    m_menu33->Remove(m_menuItemAbout);
    m_menu33->Insert(2, m_menuItemAbout);

    //prepare drag & drop
    m_panel1->SetDropTarget(new MainWindowDropTarget(this, m_panel1));
    m_panel2->SetDropTarget(new MainWindowDropTarget(this, m_panel2));

    m_panel11->SetDropTarget(new MainWindowDropTarget(this, m_panel1));
    m_panel12->SetDropTarget(new MainWindowDropTarget(this, m_panel2));

    //redirect drag & drop event back to main window
    Connect(FFS_DROP_FILE_EVENT, FfsFileDropEventHandler(MainDialog::OnFilesDropped), NULL, this);

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
    m_gridLeft->initSettings(  true, globalSettings.gui.showFileIcons, m_gridLeft, m_gridRight, m_gridMiddle, &gridRefUI, &currentGridData);
    m_gridMiddle->initSettings(false, false, m_gridLeft, m_gridRight, m_gridMiddle, &gridRefUI, &currentGridData);
    m_gridRight->initSettings( true, globalSettings.gui.showFileIcons, m_gridLeft, m_gridRight, m_gridMiddle, &gridRefUI, &currentGridData);

    //disable sync button as long as "compare" hasn't been triggered.
    enableSynchronization(false);

    //mainly to update row label sizes...
    writeGrid(currentGridData);

    enableSynchronization(false);

    //initialize language selection
    switch (programLanguage->getLanguage())
    {
    case wxLANGUAGE_CHINESE_SIMPLIFIED:
        m_menuItemChineseSimple->Check();
        break;
    case wxLANGUAGE_DUTCH:
        m_menuItemDutch->Check();
        break;
    case wxLANGUAGE_FRENCH:
        m_menuItemFrench->Check();
        break;
    case wxLANGUAGE_GERMAN:
        m_menuItemGerman->Check();
        break;
    case wxLANGUAGE_HUNGARIAN:
        m_menuItemHungarian->Check();
        break;
    case wxLANGUAGE_ITALIAN:
        m_menuItemItalian->Check();
        break;
    case wxLANGUAGE_JAPANESE:
        m_menuItemJapanese->Check();
        break;
    case wxLANGUAGE_POLISH:
        m_menuItemPolish->Check();
        break;
    case wxLANGUAGE_PORTUGUESE:
        m_menuItemPortuguese->Check();
        break;
    case wxLANGUAGE_SLOVENIAN:
        m_menuItemSlovenian->Check();
        break;
    case wxLANGUAGE_SPANISH:
        m_menuItemSpanish->Check();
        break;
    default:
        m_menuItemEnglish->Check();
    }

    //create the compare status panel in hidden state
    compareStatus = new CompareStatus(this);
    bSizer1->Insert(1, compareStatus, 0, wxEXPAND | wxBOTTOM, 5 );
    Layout();   //avoid screen flicker when panel is shown later
    compareStatus->Hide();

    //correct width of swap button above middle grid
    wxSize source = m_gridMiddle->GetSize();
    wxSize target = bSizerMiddle->GetSize();
    int spaceToAdd = source.GetX() - target.GetX();
    bSizerMiddle->Insert(1, spaceToAdd / 2, 0, 0);
    bSizerMiddle->Insert(0, spaceToAdd - (spaceToAdd / 2), 0, 0);
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
        writeConfigurationToXml(FreeFileSync::LAST_CONFIG_FILE);   //don't trow exceptions in destructors
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
    const wxArrayString leftFolderHistory = m_comboBoxDirLeft->GetStrings();
    for (unsigned i = 0; i < leftFolderHistory.GetCount(); ++i)
        globalSettings.gui.folderHistoryLeft.push_back(leftFolderHistory[i]);

    globalSettings.gui.folderHistoryRight.clear();
    const wxArrayString rightFolderHistory = m_comboBoxDirRight->GetStrings();
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
        int gridSizeUI = gridRefUI.size();

        bool newSelection = false; //default: deselect range

        //leadingRow determines de-/selection of all other rows
        int leadingRow = *rowsToFilterOnUiTable.begin();
        if (0 <= leadingRow && leadingRow < gridSizeUI)
            newSelection = !currentGridData[gridRefUI[leadingRow]].selectedForSynchronization;

        if (hideFilteredElements)
            assert(!newSelection); //if hidefiltered is active, there should be no filtered elements on screen => current element was filtered out


        //get all lines that need to be filtered (e.g. if a folder is marked, then its subelements should be marked as well)
        std::set<int> rowsToFilterOnGridData;    //rows to filter in backend

        for (std::set<int>::iterator i = rowsToFilterOnUiTable.begin(); i != rowsToFilterOnUiTable.end(); ++i)
        {
            if (0 <= *i && *i < gridSizeUI)
            {
                unsigned int gridIndex = gridRefUI[*i];
                rowsToFilterOnGridData.insert(gridIndex);
                FreeFileSync::addSubElements(currentGridData, currentGridData[gridIndex], rowsToFilterOnGridData);
            }
        }

        //toggle selection of filtered rows
        for (std::set<int>::iterator i = rowsToFilterOnGridData.begin(); i != rowsToFilterOnGridData.end(); ++i)
            currentGridData[*i].selectedForSynchronization = newSelection;

        //signal UI that grids need to be refreshed on next Update()
        m_gridLeft->ForceRefresh();
        m_gridRight->ForceRefresh();
        m_gridMiddle->ForceRefresh();
        Update();   //show changes resulting from ForceRefresh()

        if (hideFilteredElements)
        {
            wxMilliSleep(400); //some delay to show user the rows he has filtered out before they are removed
            writeGrid(currentGridData); //redraw grid to remove excluded elements (keeping sort sequence)
        }
    }

    //clear selection on grids
    if (hideFilteredElements)
    {
        m_gridLeft->ClearSelection();
        m_gridRight->ClearSelection();
    } //exception for grid 3
    m_gridMiddle->ClearSelection();
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


void MainDialog::copySelectionToClipboard(const wxGrid* selectedGrid)
{
    const std::set<int> selectedRows = getSelectedRows(selectedGrid);
    if (selectedRows.size() > 0)
    {
        wxString clipboardString;

        for (std::set<int>::iterator i = selectedRows.begin(); i != selectedRows.end(); ++i)
        {
            for (int k = 0; k < const_cast<wxGrid*>(selectedGrid)->GetNumberCols(); ++k)
            {
                clipboardString+= const_cast<wxGrid*>(selectedGrid)->GetCellValue(*i, k);
                if (k != const_cast<wxGrid*>(selectedGrid)->GetNumberCols() - 1)
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


std::set<int> MainDialog::getSelectedRows(const wxGrid* grid)
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
        for (int k = 0; k < const_cast<wxGrid*>(grid)->GetNumberRows(); ++k) //messy wxGrid implementation...
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
    if (output.empty() && grid == m_gridLeft->getLeadGrid())
        output.insert(const_cast<wxGrid*>(grid)->GetCursorRow()); //messy wxGrid implementation...

    removeInvalidRows(output, gridRefUI.size());

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
            throw AbortThisProcess();
        default:
            assert (false);
            return ErrorHandler::IGNORE_ERROR; //dummy return value
        }
    }

private:
    bool ignoreErrors;
};


void MainDialog::deleteFilesOnGrid(const std::set<int>& selectedRowsLeft, const std::set<int>& selectedRowsRight)
{
    if (selectedRowsLeft.size() + selectedRowsRight.size())
    {
        //map grid lines from UI to grid lines in backend (gridData)
        std::set<int> rowsOnGridLeft;
        for (std::set<int>::iterator i = selectedRowsLeft.begin(); i != selectedRowsLeft.end(); ++i)
            rowsOnGridLeft.insert(gridRefUI[*i]);

        std::set<int> rowsOnGridRight;
        for (std::set<int>::iterator i = selectedRowsRight.begin(); i != selectedRowsRight.end(); ++i)
            rowsOnGridRight.insert(gridRefUI[*i]);

        DeleteDialog* confirmDeletion = new DeleteDialog(this, //no destruction needed; attached to main window
                currentGridData,
                rowsOnGridLeft,
                rowsOnGridRight,
                globalSettings.gui.deleteOnBothSides,
                globalSettings.gui.useRecyclerForManualDeletion);
        if (confirmDeletion->ShowModal() == DeleteDialog::BUTTON_OKAY)
        {
            if (globalSettings.gui.useRecyclerForManualDeletion && !FreeFileSync::recycleBinExists())
            {
                wxMessageBox(_("Unable to initialize Recycle Bin!"));
                return;
            }

            //Attention! Modifying the grid is highly critical! There MUST NOT BE any accesses to gridRefUI until this reference table is updated
            //by writeGrid()!! This is easily missed, e.g. when ClearSelection() or ShowModal() or possibly any other wxWidgets function is called
            //that might want to redraw the UI (which implicitly uses the information in gridRefUI and currentGridData (see CustomGrid)

            try
            {   //handle errors when deleting files/folders
                DeleteErrorHandler errorHandler;

                FreeFileSync::deleteFromGridAndHD(currentGridData,
                                                  rowsOnGridLeft,
                                                  rowsOnGridRight,
                                                  globalSettings.gui.deleteOnBothSides,
                                                  globalSettings.gui.useRecyclerForManualDeletion,
                                                  &errorHandler);
            }
            catch (AbortThisProcess&) {}

            //redraw grid neccessary to update new dimensions and for UI-Backend data linkage
            writeGrid(currentGridData); //call immediately after deleteFromGridAndHD!!!

            m_gridLeft->ClearSelection();
            m_gridMiddle->ClearSelection();
            m_gridRight->ClearSelection();
        }
    }
}


void MainDialog::openWithFileManager(int rowNumber, const wxGrid* grid)
{
    wxString command;
    const FileDescrLine* fileDescr = NULL;
    if (grid == m_gridLeft)
    {
        if (0 <= rowNumber && rowNumber < int(gridRefUI.size()))
            fileDescr = &currentGridData[gridRefUI[rowNumber]].fileDescrLeft;
#ifdef FFS_WIN
        command = wxString(wxT("explorer ")) + FreeFileSync::getFormattedDirectoryName(m_comboBoxDirLeft->GetValue().c_str()).c_str(); //default
#endif  // FFS_WIN
    }
    else if (grid == m_gridRight)
    {
        if (0 <= rowNumber && rowNumber < int(gridRefUI.size()))
            fileDescr = &currentGridData[gridRefUI[rowNumber]].fileDescrRight;
#ifdef FFS_WIN
        command = wxString(wxT("explorer ")) + FreeFileSync::getFormattedDirectoryName(m_comboBoxDirRight->GetValue().c_str()).c_str(); //default
#endif  // FFS_WIN
    }
    else
    {
        assert(false);
        return;
    }

    if (fileDescr)
    {
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
        deleteFilesOnGrid(getSelectedRows(m_gridLeft), getSelectedRows(m_gridRight));

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
        deleteFilesOnGrid(getSelectedRows(m_gridLeft), getSelectedRows(m_gridRight));

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

        const FileCompareLine& firstLine = currentGridData[gridRefUI[selectionBegin]];

        if (firstLine.selectedForSynchronization)
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
        const FileCompareLine& line = currentGridData[gridRefUI[*i]];
        newFilterEntry.relativeName = line.fileDescrLeft.relativeName.c_str();
        newFilterEntry.type         = line.fileDescrLeft.objType;
        if (!newFilterEntry.relativeName.IsEmpty())
            exFilterCandidateObj.push_back(newFilterEntry);
    }
    for (std::set<int>::iterator i = selectionRight.begin(); i != selectionRight.end(); ++i)
    {
        const FileCompareLine& line = currentGridData[gridRefUI[*i]];
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

            writeGrid(currentGridData);
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
                {
                    cfg.excludeFilter+= wxString(wxT("*")) + GlobalResources::FILE_NAME_SEPARATOR + i->relativeName;
                }
                else if (i->type == FileDescrLine::TYPE_DIRECTORY)
                {
                    cfg.excludeFilter+= wxString(wxT("*")) + GlobalResources::FILE_NAME_SEPARATOR + i->relativeName + wxT("\n");
                    cfg.excludeFilter+= wxString(wxT("*")) + GlobalResources::FILE_NAME_SEPARATOR + i->relativeName + GlobalResources::FILE_NAME_SEPARATOR + wxT("*");
                }
                else assert(false);
            }

            cfg.filterIsActive = true;
            updateFilterButton(m_bpButtonFilter, cfg.filterIsActive);

            FreeFileSync::filterGridData(currentGridData, cfg.includeFilter, cfg.excludeFilter);

            writeGrid(currentGridData);
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
            const wxGrid* const leadGrid = m_gridLeft->getLeadGrid();

            std::set<int> selection = getSelectedRows(leadGrid);

            if (selection.size() == 1)
                openWithFileManager(*selection.begin(), leadGrid);
            else if (selection.size() == 0)
                openWithFileManager(-1, leadGrid);
        }
    }

    else if (eventId == CONTEXT_DELETE_FILES)
    {
        deleteFilesOnGrid(getSelectedRows(m_gridLeft), getSelectedRows(m_gridRight));
    }
}


void MainDialog::OnContextMenuMiddle(wxGridEvent& event)
{
    contextMenu.reset(new wxMenu); //re-create context menu
    contextMenu->Append(CONTEXT_CHECK_ALL, _("Check all"));
    contextMenu->Append(CONTEXT_UNCHECK_ALL, _("Uncheck all"));

    if (currentGridData.size() == 0)
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
        writeGrid(currentGridData);
    }
    else if (eventId == CONTEXT_UNCHECK_ALL)
    {
        FreeFileSync::excludeAllRowsOnGrid(currentGridData);
        writeGrid(currentGridData);
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


void MainDialog::OnWriteDirManually(wxCommandEvent& event)
{
    wxString newDir = FreeFileSync::getFormattedDirectoryName(event.GetString().c_str()).c_str();
    if (wxDirExists(newDir))
    {
        wxObject* eventObj = event.GetEventObject();

        //first check if event comes from main folder pair
        if (eventObj == (wxObject*)m_comboBoxDirLeft)
            m_dirPickerLeft->SetPath(newDir);
        else if (eventObj == (wxObject*)m_comboBoxDirRight)
            m_dirPickerRight->SetPath(newDir);
        else
        {
            //check if event comes from additional pairs
            for (std::vector<FolderPairGenerated*>::const_iterator i = additionalFolderPairs.begin(); i != additionalFolderPairs.end(); ++i)
            {
                FolderPairGenerated* dirPair = *i;
                if (eventObj == (wxObject*)(dirPair->m_directoryLeft))
                {
                    dirPair->m_dirPickerLeft->SetPath(newDir);
                    break;
                }
                else if (eventObj == (wxObject*)(dirPair->m_directoryRight))
                {
                    dirPair->m_dirPickerRight->SetPath(newDir);
                    break;
                }
            }
        }
    }
    event.Skip();
}


void MainDialog::OnDirSelected(wxFileDirPickerEvent& event)
{
    const wxString newPath = event.GetPath();
    wxObject* eventObj = event.GetEventObject();

    //first check if event comes from main folder pair
    if (eventObj == (wxObject*)m_dirPickerLeft)
    {
        m_comboBoxDirLeft->SetSelection(wxNOT_FOUND);
        m_comboBoxDirLeft->SetValue(newPath);
    }
    else if (eventObj == (wxObject*)m_dirPickerRight)
    {
        m_comboBoxDirRight->SetSelection(wxNOT_FOUND);
        m_comboBoxDirRight->SetValue(newPath);
    }
    else //check if event comes from additional pairs
    {
        for (std::vector<FolderPairGenerated*>::const_iterator i = additionalFolderPairs.begin(); i != additionalFolderPairs.end(); ++i)
        {
            FolderPairGenerated* dirPair = *i;
            if (eventObj == (wxObject*)(dirPair->m_dirPickerLeft))
            {
                dirPair->m_directoryLeft->SetValue(newPath);
                break;
            }
            else if (eventObj == (wxObject*)(dirPair->m_dirPickerRight))
            {
                dirPair->m_directoryRight->SetValue(newPath);
                break;
            }
        }
    }

    //disable the sync button
    enableSynchronization(false);

    //clear grids
    currentGridData.clear();
    writeGrid(currentGridData);

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
        if (sameFileSpecified(FreeFileSync::LAST_CONFIG_FILE, filename))
            m_choiceHistory->Insert(_("<Last session>"), 0);  //insert at beginning of list
        else
            m_choiceHistory->Insert(getFormattedHistoryElement(filename), 0);  //insert at beginning of list

        m_choiceHistory->SetSelection(0);
    }

    //keep maximal size of history list
    if (cfgFileNames.size() > globalSettings.gui.cfgHistoryMaxItems)
    {
        //delete last rows
        cfgFileNames.pop_back();
        m_choiceHistory->Delete(globalSettings.gui.cfgHistoryMaxItems);
    }
}


bool MainWindowDropTarget::OnDropFiles(wxCoord x, wxCoord y, const wxArrayString& filenames)
{
    if (!filenames.IsEmpty())
    {
        const wxString droppedFileName = filenames[0];

        //create a custom event on main window: execute event after file dropping is completed! (e.g. after mouse is released)
        FfsFileDropEvent evt(droppedFileName, dropTarget);
        mainDlg->GetEventHandler()->AddPendingEvent(evt);
    }
    return false;
}


template <class Ctrl>
void setDirectoryFromDrop(const wxString& elementName, Ctrl* txtCtrl, wxDirPickerCtrl* dirPicker)
{
    wxString fileName = elementName;

    if (wxDirExists(fileName))
    {
        txtCtrl->SetValue(fileName);
        dirPicker->SetPath(fileName);
    }
    else
    {
        fileName = wxFileName(fileName).GetPath();
        if (wxDirExists(fileName))
        {
            txtCtrl->SetValue(fileName);
            dirPicker->SetPath(fileName);
        }
    }
}


void MainDialog::OnFilesDropped(FfsFileDropEvent& event)
{
    const wxString droppedFileName = event.m_nameDropped;
    const wxPanel* dropTarget      = event.m_dropTarget;

    //disable the sync button
    enableSynchronization(false);

    //clear grids
    currentGridData.clear();
    writeGrid(currentGridData);

    xmlAccess::XmlType fileType = xmlAccess::getXmlType(droppedFileName);

    //test if ffs config file has been dropped
    if (fileType == xmlAccess::XML_GUI_CONFIG)
    {
        if (readConfigurationFromXml(droppedFileName))
            pushStatusInformation(_("Configuration loaded!"));
    }
    //...or a ffs batch file
    else if (fileType == xmlAccess::XML_BATCH_CONFIG)
    {
        BatchDialog* batchDlg = new BatchDialog(this, droppedFileName);
        if (batchDlg->ShowModal() == BatchDialog::BATCH_FILE_SAVED)
            pushStatusInformation(_("Batch file created successfully!"));
    }
    //test if main folder pair is drop target
    else if (dropTarget == m_panel1)
    {
        m_comboBoxDirLeft->SetSelection(wxNOT_FOUND);
        setDirectoryFromDrop<wxComboBox>(droppedFileName, m_comboBoxDirLeft, m_dirPickerLeft);
    }

    else if (dropTarget == m_panel2)
    {
        m_comboBoxDirRight->SetSelection(wxNOT_FOUND);
        setDirectoryFromDrop<wxComboBox>(droppedFileName, m_comboBoxDirRight, m_dirPickerRight);
    }

    else //test if additional folder pairs are drop targets
    {
        for (std::vector<FolderPairGenerated*>::const_iterator i = additionalFolderPairs.begin(); i != additionalFolderPairs.end(); ++i)
        {
            FolderPairGenerated* dirPair = *i;
            if (dropTarget == (dirPair->m_panelLeft))
            {
                setDirectoryFromDrop<wxTextCtrl>(droppedFileName, dirPair->m_directoryLeft, dirPair->m_dirPickerLeft);
                break;
            }
            else if (dropTarget == (dirPair->m_panelRight))
            {
                setDirectoryFromDrop<wxTextCtrl>(droppedFileName, dirPair->m_directoryRight, dirPair->m_dirPickerRight);
                break;
            }
        }
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


void addPairToFolderHistory(wxComboBox* comboBox, const wxString& newFolder)
{
    const wxString oldVal = comboBox->GetValue();

    const int pos = findTextPos(comboBox->GetStrings(), newFolder);
    if (pos >= 0)
        comboBox->Delete(pos);

    comboBox->Insert(newFolder, 0);

    //keep maximal size of history list
    const unsigned MAX_FOLDER_HIST_COUNT = 12;
    if (comboBox->GetCount() > MAX_FOLDER_HIST_COUNT)
        comboBox->Delete(MAX_FOLDER_HIST_COUNT);

    comboBox->SetSelection(wxNOT_FOUND);  //don't select anything
    comboBox->SetValue(oldVal);           //but preserve main text!
}


void MainDialog::addLeftFolderToHistory(const wxString& leftFolder)
{
    addPairToFolderHistory(m_comboBoxDirLeft, leftFolder);
}


void MainDialog::addRightFolderToHistory(const wxString& rightFolder)
{
    addPairToFolderHistory(m_comboBoxDirRight, rightFolder);
}


void MainDialog::OnSaveConfig(wxCommandEvent& event)
{
    wxString defaultFileName = wxT("SyncSettings.ffs_gui");

    if (!proposedConfigFileName.empty())
        defaultFileName = proposedConfigFileName;

    wxFileDialog* filePicker = new wxFileDialog(this, wxEmptyString, wxEmptyString, defaultFileName, wxString(_("FreeFileSync configuration")) + wxT(" (*.ffs_gui)|*.ffs_gui"), wxFD_SAVE);
    if (filePicker->ShowModal() == wxID_OK)
    {
        wxString newFileName = filePicker->GetPath();

        if (FreeFileSync::fileExists(newFileName.c_str()))
        {
            wxMessageDialog* messageDlg = new wxMessageDialog(this, wxString(_("File already exists. Overwrite?")) + wxT(" \"") + newFileName + wxT("\""), _("Warning") , wxOK | wxCANCEL);

            if (messageDlg->ShowModal() != wxID_OK)
            {
                pushStatusInformation(_("Save aborted!"));
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
        writeGrid(currentGridData);

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
    Destroy();
}


void MainDialog::OnQuit(wxCommandEvent &event)
{
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
            if (filename == FreeFileSync::LAST_CONFIG_FILE && !wxFileExists(filename)) //do not show error in this case
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
    m_comboBoxDirLeft->SetSelection(wxNOT_FOUND);
    m_comboBoxDirLeft->SetValue(wxEmptyString);
    m_dirPickerLeft->SetPath(wxEmptyString);
    m_comboBoxDirRight->SetSelection(wxNOT_FOUND);
    m_comboBoxDirRight->SetValue(wxEmptyString);
    m_dirPickerRight->SetPath(wxEmptyString);

    clearFolderPairs();

    if (guiCfg.directoryPairs.size() > 0)
    {
        //set main folder pair
        std::vector<FolderPair>::const_iterator main = guiCfg.directoryPairs.begin();

        m_comboBoxDirLeft->SetValue(main->leftDirectory.c_str());
        const wxString leftDirFormatted = FreeFileSync::getFormattedDirectoryName(main->leftDirectory).c_str();
        if (wxDirExists(leftDirFormatted))
            m_dirPickerLeft->SetPath(leftDirFormatted);
        addLeftFolderToHistory(main->leftDirectory.c_str()); //another hack: wxCombobox::Insert() asynchronously sends message overwriting a later wxCombobox::SetValue()!!! :(

        m_comboBoxDirRight->SetValue(main->rightDirectory.c_str());
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

    if (filename == FreeFileSync::LAST_CONFIG_FILE) //set title
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
    xmlAccess::XmlGuiConfig guiCfg;

    //load structure with basic settings "mainCfg"
    guiCfg.mainCfg = cfg;
    guiCfg.directoryPairs = getFolderPairs();

    //load structure with gui settings
    guiCfg.hideFilteredElements = hideFilteredElements;

    guiCfg.ignoreErrors = ignoreErrors;

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

    if (filename == FreeFileSync::LAST_CONFIG_FILE) //set title
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

    writeGrid(currentGridData);
}


void MainDialog::OnHideFilteredButton(wxCommandEvent &event)
{   //toggle showing filtered rows
    hideFilteredElements = !hideFilteredElements;
    //make sure, checkbox and "hideFiltered" are in sync
    m_checkBoxHideFilt->SetValue(hideFilteredElements);

    m_gridLeft->ClearSelection();
    m_gridRight->ClearSelection();

    writeGrid(currentGridData);

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

            writeGrid(currentGridData);
        }
    }
    //no event.Skip() here, to not start browser
}


void MainDialog::OnLeftOnlyFiles(wxCommandEvent& event)
{
    leftOnlyFilesActive = !leftOnlyFilesActive;
    updateViewFilterButtons();
    writeGrid(currentGridData);
};

void MainDialog::OnLeftNewerFiles(wxCommandEvent& event)
{
    leftNewerFilesActive = !leftNewerFilesActive;
    updateViewFilterButtons();
    writeGrid(currentGridData);
};

void MainDialog::OnDifferentFiles(wxCommandEvent& event)
{
    differentFilesActive = !differentFilesActive;
    updateViewFilterButtons();
    writeGrid(currentGridData);
};

void MainDialog::OnRightNewerFiles(wxCommandEvent& event)
{
    rightNewerFilesActive = !rightNewerFilesActive;
    updateViewFilterButtons();
    writeGrid(currentGridData);
};

void MainDialog::OnRightOnlyFiles(wxCommandEvent& event)
{
    rightOnlyFilesActive = !rightOnlyFilesActive;
    updateViewFilterButtons();
    writeGrid(currentGridData);
};

void MainDialog::OnEqualFiles(wxCommandEvent& event)
{
    equalFilesActive = !equalFilesActive;
    updateViewFilterButtons();
    writeGrid(currentGridData);
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
    writeGrid(currentGridData);
}


std::vector<FolderPair> MainDialog::getFolderPairs()
{
    std::vector<FolderPair> output;

    //add main pair
    FolderPair newPair;
    newPair.leftDirectory  = m_comboBoxDirLeft->GetValue().c_str();
    newPair.rightDirectory = m_comboBoxDirRight->GetValue().c_str();
    output.push_back(newPair);

    //add additional pairs
    for (std::vector<FolderPairGenerated*>::const_iterator i = additionalFolderPairs.begin(); i != additionalFolderPairs.end(); ++i)
    {
        FolderPairGenerated* dirPair = *i;
        newPair.leftDirectory  = dirPair->m_directoryLeft->GetValue().c_str();
        newPair.rightDirectory = dirPair->m_directoryRight->GetValue().c_str();
        output.push_back(newPair);
    }

    return output;
}


void MainDialog::OnCompare(wxCommandEvent &event)
{
    //PERF_START;

    clearStatusBar();

    wxBusyCursor dummy; //show hourglass cursor

    //save memory by clearing old result list
    currentGridData.clear();
    writeGrid(currentGridData); //refresh GUI grid

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
        std::sort(currentGridData.begin(), currentGridData.end(), sortByRelativeName<true, SORT_ON_LEFT>);

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
        addLeftFolderToHistory(m_comboBoxDirLeft->GetValue());
        addRightFolderToHistory(m_comboBoxDirRight->GetValue());
    }

    //refresh grid in ANY case! (also on abort)
    writeGrid(currentGridData);
}


void MainDialog::OnAbortCompare(wxCommandEvent& event)
{
    if (cmpStatusHandlerTmp)
        cmpStatusHandlerTmp->requestAbortion();
}


void MainDialog::writeGrid(const FileCompareResult& gridData)
{
    m_gridLeft->BeginBatch();
    m_gridMiddle->BeginBatch();
    m_gridRight->BeginBatch();

    mapGridDataToUI(gridRefUI, gridData);  //update gridRefUI
    updateStatusInformation(gridRefUI);    //write status information for gridRefUI

    //all three grids retrieve their data directly via gridRefUI!!!
    //the only thing left to do is notify the grids to update their sizes (nr of rows), since this has to be communicated by the grids via messages
    m_gridLeft->updateGridSizes();
    m_gridMiddle->updateGridSizes();
    m_gridRight->updateGridSizes();

    //enlarge label width to display row numbers correctly
    int nrOfRows = m_gridLeft->GetNumberRows();
    if (nrOfRows >= 1)
    {
        int nrOfDigits = int(floor(log10(double(nrOfRows)) + 1));
        m_gridLeft->SetRowLabelSize(nrOfDigits * 8 + 4);
        m_gridRight->SetRowLabelSize(nrOfDigits * 8 + 4);
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
                &statusHandler);

            synchronization.startSynchronizationProcess(currentGridData, cfg.syncConfiguration);
        }
        catch (AbortThisProcess& theException)
        {   //do NOT disable the sync button: user might want to try to sync the REMAINING rows
        }   //enableSynchronization(false);


        //show remaining files that have not been processed: put DIRECTLY after startSynchronizationProcess() and DON'T call any wxWidgets functions
        //in between! Else CustomGrid might access the obsolete gridRefUI!
        writeGrid(currentGridData);

        m_gridLeft->ClearSelection();
        m_gridMiddle->ClearSelection();
        m_gridRight->ClearSelection();

        if (currentGridData.size() > 0)
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
    openWithFileManager(event.GetRow(), m_gridLeft);
    event.Skip();
}


void MainDialog::OnRightGridDoubleClick(wxGridEvent& event)
{
    openWithFileManager(event.GetRow(), m_gridRight);
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
        xmlAccess::ColumnTypes columnType = m_gridLeft->getTypeAtPos(currentSortColumn);
        if (columnType == xmlAccess::FULL_NAME)
        {
            if (sortAscending) std::sort(currentGridData.begin(), currentGridData.end(), sortByRelativeName<true, SORT_ON_LEFT>); //sort by rel name here too!
            else               std::sort(currentGridData.begin(), currentGridData.end(), sortByRelativeName<false, SORT_ON_LEFT>);
        }
        else if (columnType == xmlAccess::FILENAME)
        {
            if (sortAscending) std::sort(currentGridData.begin(), currentGridData.end(), sortByFileName<true, SORT_ON_LEFT>);
            else               std::sort(currentGridData.begin(), currentGridData.end(), sortByFileName<false, SORT_ON_LEFT>);
        }
        else if (columnType == xmlAccess::REL_PATH)
        {
            if (sortAscending) std::sort(currentGridData.begin(), currentGridData.end(), sortByRelativeName<true, SORT_ON_LEFT>);
            else               std::sort(currentGridData.begin(), currentGridData.end(), sortByRelativeName<false, SORT_ON_LEFT>);
        }
        else if (columnType == xmlAccess::SIZE)
        {
            if (sortAscending) std::sort(currentGridData.begin(), currentGridData.end(), sortByFileSize<true, SORT_ON_LEFT>);
            else               std::sort(currentGridData.begin(), currentGridData.end(), sortByFileSize<false, SORT_ON_LEFT>);
        }
        else if (columnType == xmlAccess::DATE)
        {
            if (sortAscending) std::sort(currentGridData.begin(), currentGridData.end(), sortByDate<true, SORT_ON_LEFT>);
            else               std::sort(currentGridData.begin(), currentGridData.end(), sortByDate<false, SORT_ON_LEFT>);
        }
        else assert(false);

        writeGrid(currentGridData); //needed to refresh gridRefUI references

        //set sort direction indicator on UI
        m_gridMiddle->setSortMarker(-1);
        m_gridRight->setSortMarker(-1);
        if (sortAscending)
            m_gridLeft->setSortMarker(currentSortColumn, globalResource.bitmapSmallUp);
        else
            m_gridLeft->setSortMarker(currentSortColumn, globalResource.bitmapSmallDown);
    }
    event.Skip();
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
        xmlAccess::ColumnTypes columnType = m_gridRight->getTypeAtPos(currentSortColumn);
        if (columnType == xmlAccess::FULL_NAME)
        {
            if (sortAscending) std::sort(currentGridData.begin(), currentGridData.end(), sortByRelativeName<true, SORT_ON_RIGHT>); //sort by rel name here too!
            else               std::sort(currentGridData.begin(), currentGridData.end(), sortByRelativeName<false, SORT_ON_RIGHT>);
        }
        else if (columnType == xmlAccess::FILENAME)
        {
            if (sortAscending) std::sort(currentGridData.begin(), currentGridData.end(), sortByFileName<true, SORT_ON_RIGHT>);
            else               std::sort(currentGridData.begin(), currentGridData.end(), sortByFileName<false, SORT_ON_RIGHT>);
        }
        else if (columnType == xmlAccess::REL_PATH)
        {
            if (sortAscending) std::sort(currentGridData.begin(), currentGridData.end(), sortByRelativeName<true, SORT_ON_RIGHT>);
            else               std::sort(currentGridData.begin(), currentGridData.end(), sortByRelativeName<false, SORT_ON_RIGHT>);
        }
        else if (columnType == xmlAccess::SIZE)
        {
            if (sortAscending) std::sort(currentGridData.begin(), currentGridData.end(), sortByFileSize<true, SORT_ON_RIGHT>);
            else               std::sort(currentGridData.begin(), currentGridData.end(), sortByFileSize<false, SORT_ON_RIGHT>);
        }
        else if (columnType == xmlAccess::DATE)
        {
            if (sortAscending) std::sort(currentGridData.begin(), currentGridData.end(), sortByDate<true, SORT_ON_RIGHT>);
            else               std::sort(currentGridData.begin(), currentGridData.end(), sortByDate<false, SORT_ON_RIGHT>);
        }
        else assert(false);

        writeGrid(currentGridData); //needed to refresh gridRefUI references

        //set sort direction indicator on UI
        m_gridLeft->setSortMarker(-1);
        m_gridMiddle->setSortMarker(-1);
        if (sortAscending)
            m_gridRight->setSortMarker(currentSortColumn, globalResource.bitmapSmallUp);
        else
            m_gridRight->setSortMarker(currentSortColumn, globalResource.bitmapSmallDown);
    }
    event.Skip();
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
    if (sortAscending) std::sort(currentGridData.begin(), currentGridData.end(), sortByCmpResult<true>);
    else               std::sort(currentGridData.begin(), currentGridData.end(), sortByCmpResult<false>);

    writeGrid(currentGridData); //needed to refresh gridRefUI references

    //set sort direction indicator on UI
    m_gridLeft->setSortMarker(-1);
    m_gridRight->setSortMarker(-1);
    if (sortAscending)
        m_gridMiddle->setSortMarker(0, globalResource.bitmapSmallUp);
    else
        m_gridMiddle->setSortMarker(0, globalResource.bitmapSmallDown);

    event.Skip();
}


void MainDialog::OnSwapDirs( wxCommandEvent& event )
{
    //swap directory names : main pair
    const wxString leftDir  = m_comboBoxDirLeft->GetValue();
    const wxString rightDir = m_comboBoxDirRight->GetValue();
    m_comboBoxDirLeft->SetSelection(wxNOT_FOUND);
    m_comboBoxDirRight->SetSelection(wxNOT_FOUND);

    m_comboBoxDirLeft->SetValue(rightDir);
    m_comboBoxDirRight->SetValue(leftDir);

    //additional pairs
    wxString tmp;
    for (std::vector<FolderPairGenerated*>::const_iterator i = additionalFolderPairs.begin(); i != additionalFolderPairs.end(); ++i)
    {
        FolderPairGenerated* dirPair = *i;
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
    writeGrid(currentGridData);
    event.Skip();
}


void MainDialog::updateStatusInformation(const GridView& visibleGrid)
{
    while (stackObjects.size() > 0)
        stackObjects.pop();

    unsigned int filesOnLeftView    = 0;
    unsigned int foldersOnLeftView  = 0;
    unsigned int filesOnRightView   = 0;
    unsigned int foldersOnRightView = 0;
    wxULongLong filesizeLeftView;
    wxULongLong filesizeRightView;

    wxString statusLeftNew;
    wxString statusMiddleNew;
    wxString statusRightNew;

    for (GridView::const_iterator i = visibleGrid.begin(); i != visibleGrid.end(); ++i)
    {
        const FileCompareLine& refLine = currentGridData[*i];

        //calculate total number of bytes for each side
        if (refLine.fileDescrLeft.objType == FileDescrLine::TYPE_FILE)
        {
            filesizeLeftView+= refLine.fileDescrLeft.fileSize;
            ++filesOnLeftView;
        }
        else if (refLine.fileDescrLeft.objType == FileDescrLine::TYPE_DIRECTORY)
            ++foldersOnLeftView;

        if (refLine.fileDescrRight.objType == FileDescrLine::TYPE_FILE)
        {
            filesizeRightView+= refLine.fileDescrRight.fileSize;
            ++filesOnRightView;
        }
        else if (refLine.fileDescrRight.objType == FileDescrLine::TYPE_DIRECTORY)
            ++foldersOnRightView;
    }
//#################################################
//format numbers to text:

//show status information on "root" level.
    if (foldersOnLeftView)
    {
        if (foldersOnLeftView == 1)
            statusLeftNew+= _("1 directory");
        else
        {
            wxString folderCount = globalFunctions::includeNumberSeparator(globalFunctions::numberToWxString(foldersOnLeftView));

            wxString outputString = _("%x directories");
            outputString.Replace(wxT("%x"), folderCount, false);
            statusLeftNew+= outputString;
        }

        if (filesOnLeftView)
            statusLeftNew+= wxT(", ");
    }

    if (filesOnLeftView)
    {
        if (filesOnLeftView == 1)
            statusLeftNew+= _("1 file,");
        else
        {
            wxString fileCount = globalFunctions::includeNumberSeparator(globalFunctions::numberToWxString(filesOnLeftView));

            wxString outputString = _("%x files,");
            outputString.Replace(wxT("%x"), fileCount, false);
            statusLeftNew+= outputString;
        }
        statusLeftNew+= wxT(" ");
        statusLeftNew+= FreeFileSync::formatFilesizeToShortString(filesizeLeftView);
    }

    wxString objectsView = globalFunctions::includeNumberSeparator(globalFunctions::numberToWxString(visibleGrid.size()));
    if (currentGridData.size() == 1)
    {
        wxString outputString = _("%x of 1 row in view");
        outputString.Replace(wxT("%x"), objectsView, false);
        statusMiddleNew = outputString;
    }
    else
    {
        wxString objectsTotal = globalFunctions::includeNumberSeparator(globalFunctions::numberToWxString(currentGridData.size()));

        wxString outputString = _("%x of %y rows in view");
        outputString.Replace(wxT("%x"), objectsView, false);
        outputString.Replace(wxT("%y"), objectsTotal, false);
        statusMiddleNew = outputString;
    }

    if (foldersOnRightView)
    {
        if (foldersOnRightView == 1)
            statusRightNew+= _("1 directory");
        else
        {
            wxString folderCount = globalFunctions::includeNumberSeparator(globalFunctions::numberToWxString(foldersOnRightView));

            wxString outputString = _("%x directories");
            outputString.Replace(wxT("%x"), folderCount, false);
            statusRightNew+= outputString;
        }

        if (filesOnRightView)
            statusRightNew+= wxT(", ");
    }

    if (filesOnRightView)
    {
        if (filesOnRightView == 1)
            statusRightNew+= _("1 file,");
        else
        {
            wxString fileCount = globalFunctions::includeNumberSeparator(globalFunctions::numberToWxString(filesOnRightView));

            wxString outputString = _("%x files,");
            outputString.Replace(wxT("%x"), fileCount, false);
            statusRightNew+= outputString;
        }

        statusRightNew+= wxT(" ");
        statusRightNew+= FreeFileSync::formatFilesizeToShortString(filesizeRightView);
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


void MainDialog::mapGridDataToUI(GridView& output, const FileCompareResult& fileCmpResult)
{
    output.clear();

    //only show those view filter buttons that really need to be displayed
    bool leftOnly, rightOnly, leftNewer, rightNewer, different, equal;
    leftOnly = rightOnly = leftNewer = rightNewer = different = equal = false;

    unsigned int currentRow = 0;
    for (FileCompareResult::const_iterator i = fileCmpResult.begin(); i != fileCmpResult.end(); ++i, ++currentRow)
    {
        //hide filtered row, if corresponding option is set
        if (hideFilteredElements && !i->selectedForSynchronization)
            continue;

        //process UI filter settings
        switch (i->cmpResult)
        {
        case FILE_LEFT_SIDE_ONLY:
            leftOnly = true;
            if (!leftOnlyFilesActive) continue;
            break;
        case FILE_RIGHT_SIDE_ONLY:
            rightOnly = true;
            if (!rightOnlyFilesActive) continue;
            break;
        case FILE_LEFT_NEWER:
            leftNewer = true;
            if (!leftNewerFilesActive) continue;
            break;
        case FILE_RIGHT_NEWER:
            rightNewer = true;
            if (!rightNewerFilesActive) continue;
            break;
        case FILE_DIFFERENT:
            different = true;
            if (!differentFilesActive) continue;
            break;
        case FILE_EQUAL:
            equal = true;
            if (!equalFilesActive) continue;
            break;
        default:
            assert (false);
        }
        output.push_back(currentRow);
    }

    //hide or enable view filter buttons
    if (leftOnly)
        m_bpButtonLeftOnly->Show();
    else
        m_bpButtonLeftOnly->Hide();

    if (rightOnly)
        m_bpButtonRightOnly->Show();
    else
        m_bpButtonRightOnly->Hide();

    if (leftNewer)
        m_bpButtonLeftNewer->Show();
    else
        m_bpButtonLeftNewer->Hide();

    if (rightNewer)
        m_bpButtonRightNewer->Show();
    else
        m_bpButtonRightNewer->Hide();

    if (different)
        m_bpButtonDifferent->Show();
    else
        m_bpButtonDifferent->Hide();

    if (equal)
        m_bpButtonEqual->Show();
    else
        m_bpButtonEqual->Hide();

    if (leftOnly || rightOnly || leftNewer || rightNewer || different || equal)
    {
        m_panel112->Show();
        m_panel112->Layout();
    }
    else
        m_panel112->Hide();

    bSizer3->Layout();
}


void MainDialog::OnAddFolderPair(wxCommandEvent& event)
{
    addFolderPair(wxEmptyString, wxEmptyString);

    //disable the sync button
    enableSynchronization(false);

    //clear grids
    currentGridData.clear();
    writeGrid(currentGridData);
}


void MainDialog::OnRemoveFolderPair(wxCommandEvent& event)
{
    //find folder pair originating the event
    const wxObject* const eventObj = event.GetEventObject();
    for (std::vector<FolderPairGenerated*>::const_iterator i = additionalFolderPairs.begin(); i != additionalFolderPairs.end(); ++i)
    {
        if (eventObj == static_cast<wxObject*>((*i)->m_bpButtonRemovePair))
        {
            removeFolderPair(i - additionalFolderPairs.begin());

            //disable the sync button
            enableSynchronization(false);

            //clear grids
            currentGridData.clear();
            writeGrid(currentGridData);
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
        FolderPairGenerated* newPair = new FolderPairGenerated(m_scrolledWindowFolderPairs);
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
        newPair->m_dirPickerLeft->Connect(wxEVT_COMMAND_DIRPICKER_CHANGED, wxFileDirPickerEventHandler(MainDialog::OnDirSelected), NULL, this);
        newPair->m_dirPickerRight->Connect(wxEVT_COMMAND_DIRPICKER_CHANGED, wxFileDirPickerEventHandler(MainDialog::OnDirSelected), NULL, this);

        newPair->m_directoryLeft->Connect(wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler(MainDialog::OnWriteDirManually), NULL, this );
        newPair->m_directoryRight->Connect(wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler(MainDialog::OnWriteDirManually), NULL, this );

        newPair->m_bpButtonRemovePair->Connect(wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler(MainDialog::OnRemoveFolderPair), NULL, this );

        //prepare drag & drop
        newPair->m_panelLeft->SetDropTarget(new MainWindowDropTarget(this, newPair->m_panelLeft)); //ownership passed
        newPair->m_panelRight->SetDropTarget(new MainWindowDropTarget(this, newPair->m_panelRight));

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
        FolderPairGenerated* pairToDelete = additionalFolderPairs[pos];
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

CompareStatusHandler::CompareStatusHandler(MainDialog* dlg) :
        mainDialog(dlg),
        ignoreErrors(false),
        currentProcess(StatusHandler::PROCESS_NONE)
{
    //prevent user input during "compare", do not disable maindialog since abort-button would also be disabled
    //it's not nice, but works - even has the advantage that certain actions are still possible: exit, about..
    mainDialog->m_radioBtnSizeDate->Disable();
    mainDialog->m_radioBtnContent->Disable();
    mainDialog->m_bpButtonFilter->Disable();
    mainDialog->m_hyperlinkCfgFilter->Disable();
    mainDialog->m_checkBoxHideFilt->Disable();
    mainDialog->m_buttonSync->Disable();
    mainDialog->m_dirPickerLeft->Disable();
    mainDialog->m_dirPickerRight->Disable();
    mainDialog->m_bpButtonSwap->Disable();
    mainDialog->m_bpButtonLeftOnly->Disable();
    mainDialog->m_bpButtonLeftNewer->Disable();
    mainDialog->m_bpButtonEqual->Disable();
    mainDialog->m_bpButtonDifferent->Disable();
    mainDialog->m_bpButtonRightNewer->Disable();
    mainDialog->m_bpButtonRightOnly->Disable();
    mainDialog->m_panel1->Disable();
    mainDialog->m_panel2->Disable();
    mainDialog->m_panel3->Disable();
    mainDialog->m_panel11->Disable();
    mainDialog->m_panel12->Disable();
    mainDialog->m_panel13->Disable();
    mainDialog->m_bpButtonSave->Disable();
    mainDialog->m_bpButtonLoad->Disable();
    mainDialog->m_choiceHistory->Disable();
    mainDialog->m_bpButton10->Disable();
    mainDialog->m_bpButton14->Disable();
    mainDialog->m_scrolledWindowFolderPairs->Disable();
    mainDialog->m_menubar1->EnableTop(0, false);
    mainDialog->m_menubar1->EnableTop(1, false);
    mainDialog->m_menubar1->EnableTop(2, false);

    //show abort button
    mainDialog->m_buttonAbort->Enable();
    mainDialog->m_buttonAbort->Show();
    mainDialog->m_buttonCompare->Disable();
    mainDialog->m_buttonCompare->Hide();
    mainDialog->m_buttonAbort->SetFocus();

    //display status panel during compare
    mainDialog->compareStatus->init(); //clear old values
    mainDialog->compareStatus->Show();

    //updateUiNow();
    mainDialog->bSizer1->Layout(); //both sizers need to recalculate!
    mainDialog->bSizer6->Layout(); //adapt layout for wxBitmapWithImage
    mainDialog->Refresh();
}


CompareStatusHandler::~CompareStatusHandler()
{
    updateUiNow(); //ui update before enabling buttons again: prevent strange behaviour of delayed button clicks

    //reenable complete main dialog
    mainDialog->m_radioBtnSizeDate->Enable();
    mainDialog->m_radioBtnContent->Enable();
    mainDialog->m_bpButtonFilter->Enable();
    mainDialog->m_hyperlinkCfgFilter->Enable();
    mainDialog->m_checkBoxHideFilt->Enable();
    mainDialog->m_buttonSync->Enable();
    mainDialog->m_dirPickerLeft->Enable();
    mainDialog->m_dirPickerRight->Enable();
    mainDialog->m_bpButtonSwap->Enable();
    mainDialog->m_bpButtonLeftOnly->Enable();
    mainDialog->m_bpButtonLeftNewer->Enable();
    mainDialog->m_bpButtonEqual->Enable();
    mainDialog->m_bpButtonDifferent->Enable();
    mainDialog->m_bpButtonRightNewer->Enable();
    mainDialog->m_bpButtonRightOnly->Enable();
    mainDialog->m_panel1->Enable();
    mainDialog->m_panel2->Enable();
    mainDialog->m_panel3->Enable();
    mainDialog->m_panel11->Enable();
    mainDialog->m_panel12->Enable();
    mainDialog->m_panel13->Enable();
    mainDialog->m_bpButtonSave->Enable();
    mainDialog->m_bpButtonLoad->Enable();
    mainDialog->m_choiceHistory->Enable();
    mainDialog->m_bpButton10->Enable();
    mainDialog->m_bpButton14->Enable();
    mainDialog->m_scrolledWindowFolderPairs->Enable();
    mainDialog->m_menubar1->EnableTop(0, true);
    mainDialog->m_menubar1->EnableTop(1, true);
    mainDialog->m_menubar1->EnableTop(2, true);

    if (abortRequested)
        mainDialog->pushStatusInformation(_("Operation aborted!"));

    mainDialog->m_buttonAbort->Disable();
    mainDialog->m_buttonAbort->Hide();
    mainDialog->m_buttonCompare->Enable(); //enable compare button
    mainDialog->m_buttonCompare->Show();

    //hide status panel from main window
    mainDialog->compareStatus->Hide();

    mainDialog->bSizer6->Layout(); //adapt layout for wxBitmapWithImage

    mainDialog->Layout();
    mainDialog->Refresh();
}


inline
void CompareStatusHandler::updateStatusText(const Zstring& text)
{
    mainDialog->compareStatus->setStatusText_NoUpdate(text);
}


void CompareStatusHandler::initNewProcess(int objectsTotal, double dataTotal, Process processID)
{
    currentProcess = processID;

    if (currentProcess == StatusHandler::PROCESS_SCANNING)
        ;
    else if (currentProcess == StatusHandler::PROCESS_COMPARING_CONTENT)
    {
        mainDialog->compareStatus->switchToCompareBytewise(objectsTotal, dataTotal);
        mainDialog->Layout();
    }

    else assert(false);
}


inline
void CompareStatusHandler::updateProcessedData(int objectsProcessed, double dataProcessed)
{
    if (currentProcess == StatusHandler::PROCESS_SCANNING)
        mainDialog->compareStatus->incScannedObjects_NoUpdate(objectsProcessed);
    else if (currentProcess == StatusHandler::PROCESS_COMPARING_CONTENT)
        mainDialog->compareStatus->incProcessedCmpData_NoUpdate(objectsProcessed, dataProcessed);
    else assert(false);
}


ErrorHandler::Response CompareStatusHandler::reportError(const Zstring& text)
{
    if (ignoreErrors)
        return ErrorHandler::IGNORE_ERROR;

    mainDialog->compareStatus->updateStatusPanelNow();

    bool ignoreNextErrors = false;
    wxString errorMessage = wxString(text.c_str()) + wxT("\n\n") + _("Ignore this error, retry or abort?");
    ErrorDlg* errorDlg = new ErrorDlg(mainDialog,
                                      ErrorDlg::BUTTON_IGNORE |  ErrorDlg::BUTTON_RETRY | ErrorDlg::BUTTON_ABORT,
                                      errorMessage, ignoreNextErrors);
    int rv = errorDlg->ShowModal();
    switch (rv)
    {
    case ErrorDlg::BUTTON_IGNORE:
        ignoreErrors = ignoreNextErrors;
        return ErrorHandler::IGNORE_ERROR;

    case ErrorDlg::BUTTON_RETRY:
        return ErrorHandler::RETRY;

    case ErrorDlg::BUTTON_ABORT:
        abortThisProcess();
    }

    assert(false);
    return ErrorHandler::IGNORE_ERROR; //dummy return value
}


void CompareStatusHandler::reportFatalError(const Zstring& errorMessage)
{
    mainDialog->compareStatus->updateStatusPanelNow();

    bool dummy = false;
    ErrorDlg* errorDlg = new ErrorDlg(mainDialog,
                                      ErrorDlg::BUTTON_ABORT,
                                      errorMessage.c_str(), dummy);
    errorDlg->ShowModal();
    abortThisProcess();
}


void CompareStatusHandler::reportWarning(const Zstring& warningMessage, bool& dontShowAgain)
{
    if (ignoreErrors) //if errors are ignored, then warnings should also
        return;

    mainDialog->compareStatus->updateStatusPanelNow();

    //show popup and ask user how to handle warning
    bool dontWarnAgain = false;
    WarningDlg* warningDlg = new WarningDlg(mainDialog,
                                            WarningDlg::BUTTON_IGNORE | WarningDlg::BUTTON_ABORT,
                                            warningMessage.c_str(),
                                            dontWarnAgain);
    switch (warningDlg->ShowModal())
    {
    case WarningDlg::BUTTON_ABORT:
        abortThisProcess();

    case WarningDlg::BUTTON_IGNORE:
        dontShowAgain = dontWarnAgain;
        return;
    }

    assert(false);
}


inline
void CompareStatusHandler::forceUiRefresh()
{
    mainDialog->compareStatus->updateStatusPanelNow();
}


void CompareStatusHandler::abortThisProcess()
{
    abortRequested = true;
    throw AbortThisProcess();  //abort can be triggered by syncStatusFrame
}
//########################################################################################################


SyncStatusHandler::SyncStatusHandler(wxWindow* dlg, bool ignoreAllErrors) :
        ignoreErrors(ignoreAllErrors)
{
    syncStatusFrame = new SyncStatus(this, dlg);
    syncStatusFrame->Show();
    updateUiNow();
}


SyncStatusHandler::~SyncStatusHandler()
{
    //print the results list
    unsigned int failedItems = unhandledErrors.GetCount();
    wxString result;
    if (failedItems)
    {
        result = wxString(_("Warning: Synchronization failed for %x item(s):")) + wxT("\n\n");
        result.Replace(wxT("%x"), globalFunctions::numberToWxString(failedItems), false);

        for (unsigned int j = 0; j < failedItems; ++j)
        {   //remove linebreaks
            wxString errorMessage = unhandledErrors[j];
            for (wxString::iterator i = errorMessage.begin(); i != errorMessage.end(); ++i)
                if (*i == wxChar('\n'))
                    *i = wxChar(' ');

            result += errorMessage + wxT("\n");
        }
        result+= wxT("\n");
    }

    //notify to syncStatusFrame that current process has ended
    if (abortRequested)
    {
        result+= wxString(_("Synchronization aborted!")) + wxT(" ") + _("You may try to synchronize remaining items again (WITHOUT having to re-compare)!");
        syncStatusFrame->setStatusText_NoUpdate(result.c_str());
        syncStatusFrame->processHasFinished(SyncStatus::ABORTED);  //enable okay and close events
    }
    else  if (failedItems)
    {
        result+= wxString(_("Synchronization completed with errors!")) + wxT(" ") + _("You may try to synchronize remaining items again (WITHOUT having to re-compare)!");
        syncStatusFrame->setStatusText_NoUpdate(result.c_str());
        syncStatusFrame->processHasFinished(SyncStatus::FINISHED_WITH_ERROR);
    }
    else
    {
        result+= _("Synchronization completed successfully!");
        syncStatusFrame->setStatusText_NoUpdate(result.c_str());
        syncStatusFrame->processHasFinished(SyncStatus::FINISHED_WITH_SUCCESS);
    }
}


inline
void SyncStatusHandler::updateStatusText(const Zstring& text)
{
    syncStatusFrame->setStatusText_NoUpdate(text);
}


void SyncStatusHandler::initNewProcess(int objectsTotal, double dataTotal, Process processID)
{
    assert (processID == StatusHandler::PROCESS_SYNCHRONIZING);

    syncStatusFrame->resetGauge(objectsTotal, dataTotal);
    syncStatusFrame->setCurrentStatus(SyncStatus::SYNCHRONIZING);
}


inline
void SyncStatusHandler::updateProcessedData(int objectsProcessed, double dataProcessed)
{
    syncStatusFrame->incProgressIndicator_NoUpdate(objectsProcessed, dataProcessed);
}


ErrorHandler::Response SyncStatusHandler::reportError(const Zstring& text)
{
    //add current time before error message
    wxString errorWithTime = wxString(wxT("[")) + wxDateTime::Now().FormatTime() + wxT("] ") + text.c_str();

    if (ignoreErrors)
    {
        unhandledErrors.Add(errorWithTime);
        return ErrorHandler::IGNORE_ERROR;
    }

    syncStatusFrame->updateStatusDialogNow();

    bool ignoreNextErrors = false;
    ErrorDlg* errorDlg = new ErrorDlg(syncStatusFrame,
                                      ErrorDlg::BUTTON_IGNORE |  ErrorDlg::BUTTON_RETRY | ErrorDlg::BUTTON_ABORT,
                                      wxString(text) + wxT("\n\n") + _("Ignore this error, retry or abort synchronization?"),
                                      ignoreNextErrors);
    int rv = errorDlg->ShowModal();
    switch (rv)
    {
    case ErrorDlg::BUTTON_IGNORE:
        ignoreErrors = ignoreNextErrors;
        unhandledErrors.Add(errorWithTime);
        return ErrorHandler::IGNORE_ERROR;

    case ErrorDlg::BUTTON_RETRY:
        return ErrorHandler::RETRY;

    case ErrorDlg::BUTTON_ABORT:
        unhandledErrors.Add(errorWithTime);
        abortThisProcess();
    }

    assert (false);
    unhandledErrors.Add(errorWithTime);
    return ErrorHandler::IGNORE_ERROR;
}


void SyncStatusHandler::reportFatalError(const Zstring& errorMessage)
{   //add current time before error message
    wxString errorWithTime = wxString(wxT("[")) + wxDateTime::Now().FormatTime() + wxT("] ") + errorMessage.c_str();

    unhandledErrors.Add(errorWithTime);
    abortThisProcess();
}


void SyncStatusHandler::reportWarning(const Zstring& warningMessage, bool& dontShowAgain)
{   //add current time before warning message
    wxString warningWithTime = wxString(wxT("[")) + wxDateTime::Now().FormatTime() + wxT("] ") + warningMessage.c_str();

    if (ignoreErrors) //if errors are ignored, then warnings should also
        return; //no unhandled error situation!

    syncStatusFrame->updateStatusDialogNow();

    //show popup and ask user how to handle warning
    bool dontWarnAgain = false;
    WarningDlg* warningDlg = new WarningDlg(syncStatusFrame,
                                            WarningDlg::BUTTON_IGNORE | WarningDlg::BUTTON_ABORT,
                                            warningMessage.c_str(),
                                            dontWarnAgain);
    switch (warningDlg->ShowModal())
    {
    case WarningDlg::BUTTON_IGNORE: //no unhandled error situation!
        dontShowAgain = dontWarnAgain;
        return;

    case WarningDlg::BUTTON_ABORT:
        unhandledErrors.Add(warningWithTime);
        abortThisProcess();
    }

    assert(false);
}


void SyncStatusHandler::forceUiRefresh()
{
    syncStatusFrame->updateStatusDialogNow();
}


void SyncStatusHandler::abortThisProcess()
{
    abortRequested = true;
    throw AbortThisProcess();  //abort can be triggered by syncStatusFrame
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
    wxString fileName = wxT("FileList.csv"); //proposal
    wxFileDialog* filePicker = new wxFileDialog(this, wxEmptyString, wxEmptyString, fileName, wxString(_("Comma separated list")) + wxT(" (*.csv)|*.csv"), wxFD_SAVE);

    if (filePicker->ShowModal() == wxID_OK)
    {
        fileName = filePicker->GetPath();
        if (FreeFileSync::fileExists(fileName.c_str()))
        {
            wxMessageDialog* messageDlg = new wxMessageDialog(this, wxString(_("File already exists. Overwrite?")) + wxT(" \"") + fileName + wxT("\""), _("Warning") , wxOK | wxCANCEL);

            if (messageDlg->ShowModal() != wxID_OK)
            {
                pushStatusInformation(_("Save aborted!"));
                return;
            }
        }

        //begin work
        wxString exportString;
        for (unsigned int i = 0; i < gridRefUI.size(); ++i)
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
        wxFFile output(fileName.c_str(), wxT("w")); //don't write in binary mode
        if (output.IsOpened())
        {
            output.Write(exportString);
            pushStatusInformation(_("File list exported!"));
        }
        else
        {
            wxMessageBox(wxString(_("Error writing file:")) + wxT(" \"") + fileName + wxT("\""), _("Error"), wxOK | wxICON_ERROR);
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
    FreeFileSync::checkForNewVersion();
}


void MainDialog::OnMenuAbout(wxCommandEvent& event)
{
    AboutDlg* aboutDlg = new AboutDlg(this);
    aboutDlg->ShowModal();
}


void MainDialog::OnMenuQuit(wxCommandEvent& event)
{
    Destroy();
}


//#########################################################################################################
//language selection

void MainDialog::switchProgramLanguage(const int langID)
{
    programLanguage->setLanguage(langID); //language is a global attribute

    //create new main window and delete old one
    cleanUp(); //destructor's code: includes writing settings to HD

    //create new dialog with respect to new language
    MainDialog* frame = new MainDialog(NULL, FreeFileSync::LAST_CONFIG_FILE, programLanguage, globalSettings);
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


void MainDialog::OnMenuLangSlovenian(wxCommandEvent& event)
{
    switchProgramLanguage(wxLANGUAGE_SLOVENIAN);
}


void MainDialog::OnMenuLangSpanish(wxCommandEvent& event)
{
    switchProgramLanguage(wxLANGUAGE_SPANISH);
}

