/***************************************************************
 * Name:      FreeFileSyncMain.cpp
 * Purpose:   Code for Application Frame
 * Author:    ZenJu (zhnmju123@gmx.de)
 * Created:   2008-07-16
 * Copyright: ZenJu ()
 * License:
 **************************************************************/

#include "mainDialog.h"
#include <wx/filename.h>
#include "../library/globalFunctions.h"
#include <fstream>
#include <wx/clipbrd.h>
#include <wx/file.h>
#include "../library/customGrid.h"

using namespace globalFunctions;

int leadingPanel = 0;

MainDialog::MainDialog(wxFrame* frame, const wxString& cfgFileName, CustomLocale* language) :
        GuiGenerated(frame),
        programLanguage(language),
        filteringInitialized(false),
        filteringPending(false),
        synchronizationEnabled(false),
        restartOnExit(false),
        cmpStatusUpdaterTmp(0)
{
    m_bpButtonCompare->SetLabel(_("&Compare"));
    m_bpButtonSync->SetLabel(_("&Synchronize"));

    //initialize sync configuration
    readConfigurationFromHD(cfgFileName, true);

    leftOnlyFilesActive   = true;
    leftNewerFilesActive  = true;
    differentFilesActive  = true;
    rightNewerFilesActive = true;  //do not put these bool values into config.dat!
    rightOnlyFilesActive  = true;  //it's more convenient to have them defaulted at startup
    equalFilesActive      = false;
    updateViewFilterButtons();

    //set icons for this dialog
    m_bpButton10->SetBitmapLabel(*GlobalResources::bitmapExit);
    m_bpButtonCompare->SetBitmapLabel(*GlobalResources::bitmapCompare);
    m_bpButtonSync->SetBitmapLabel(*GlobalResources::bitmapSync);
    m_bpButtonSwap->SetBitmapLabel(*GlobalResources::bitmapSwap);
    m_bpButton14->SetBitmapLabel(*GlobalResources::bitmapHelp);
    m_bpButton201->SetBitmapLabel(*GlobalResources::bitmapSave);
    m_bitmap15->SetBitmap(*GlobalResources::bitmapStatusEdge);

    //prepare drag & drop
    m_panel1->SetDropTarget(new FileDropEvent(this, 1));
    m_panel2->SetDropTarget(new FileDropEvent(this, 2));

    //create a right-click context menu
    contextMenu = new wxMenu;
    contextMenu->Append(CONTEXT_MANUAL_FILTER, _("Filter manually"));
    contextMenu->Append(CONTEXT_CLIPBOARD, _("Copy to clipboard\tCTRL+C"));
#ifdef FFS_WIN
    contextMenu->Append(CONTEXT_EXPLORER, _("Open with Explorer\tD-Click"));
#endif
    contextMenu->AppendSeparator();
    contextMenu->Append(CONTEXT_DELETE_FILES, _("Delete files\tDEL"));

    contextMenu->Connect(wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(MainDialog::onContextMenuSelection), NULL, this);

    //support for CTRL + C and DEL
    m_grid1->Connect(wxEVT_KEY_DOWN, wxKeyEventHandler(MainDialog::onGrid1ButtonEvent), NULL, this);
    m_grid2->Connect(wxEVT_KEY_DOWN, wxKeyEventHandler(MainDialog::onGrid2ButtonEvent), NULL, this);
    m_grid3->Connect(wxEVT_KEY_DOWN, wxKeyEventHandler(MainDialog::onGrid3ButtonEvent), NULL, this);

    //identify leading grid by keyboard input or scroll action
    m_grid1->Connect(wxEVT_KEY_DOWN,               wxEventHandler(MainDialog::onGrid1access), NULL, this);
    m_grid1->Connect(wxEVT_SCROLLWIN_TOP,          wxEventHandler(MainDialog::onGrid1access), NULL, this);
    m_grid1->Connect(wxEVT_SCROLLWIN_BOTTOM,       wxEventHandler(MainDialog::onGrid1access), NULL, this);
    m_grid1->Connect(wxEVT_SCROLLWIN_LINEUP,       wxEventHandler(MainDialog::onGrid1access), NULL, this);
    m_grid1->Connect(wxEVT_SCROLLWIN_LINEDOWN,     wxEventHandler(MainDialog::onGrid1access), NULL, this);
    m_grid1->Connect(wxEVT_SCROLLWIN_PAGEUP,       wxEventHandler(MainDialog::onGrid1access), NULL, this);
    m_grid1->Connect(wxEVT_SCROLLWIN_PAGEDOWN,     wxEventHandler(MainDialog::onGrid1access), NULL, this);
    m_grid1->Connect(wxEVT_SCROLLWIN_THUMBTRACK,   wxEventHandler(MainDialog::onGrid1access), NULL, this);
    m_grid1->Connect(wxEVT_SCROLLWIN_THUMBRELEASE, wxEventHandler(MainDialog::onGrid1access), NULL, this);
    m_grid1->Connect(wxEVT_GRID_LABEL_LEFT_CLICK,  wxEventHandler(MainDialog::onGrid1access), NULL, this);
    m_grid1->GetGridWindow()->Connect(wxEVT_LEFT_DOWN, wxEventHandler(MainDialog::onGrid1access), NULL, this);

    m_grid2->Connect(wxEVT_KEY_DOWN,               wxEventHandler(MainDialog::onGrid2access), NULL, this);
    m_grid2->Connect(wxEVT_SCROLLWIN_TOP,          wxEventHandler(MainDialog::onGrid2access), NULL, this);
    m_grid2->Connect(wxEVT_SCROLLWIN_BOTTOM,       wxEventHandler(MainDialog::onGrid2access), NULL, this);
    m_grid2->Connect(wxEVT_SCROLLWIN_LINEUP,       wxEventHandler(MainDialog::onGrid2access), NULL, this);
    m_grid2->Connect(wxEVT_SCROLLWIN_LINEDOWN,     wxEventHandler(MainDialog::onGrid2access), NULL, this);
    m_grid2->Connect(wxEVT_SCROLLWIN_PAGEUP,       wxEventHandler(MainDialog::onGrid2access), NULL, this);
    m_grid2->Connect(wxEVT_SCROLLWIN_PAGEDOWN,     wxEventHandler(MainDialog::onGrid2access), NULL, this);
    m_grid2->Connect(wxEVT_SCROLLWIN_THUMBTRACK,   wxEventHandler(MainDialog::onGrid2access), NULL, this);
    m_grid2->Connect(wxEVT_SCROLLWIN_THUMBRELEASE, wxEventHandler(MainDialog::onGrid2access), NULL, this);
    m_grid2->Connect(wxEVT_GRID_LABEL_LEFT_CLICK,  wxEventHandler(MainDialog::onGrid2access), NULL, this);
    m_grid2->GetGridWindow()->Connect(wxEVT_LEFT_DOWN, wxEventHandler(MainDialog::onGrid2access), NULL, this);

    m_grid3->Connect(wxEVT_KEY_DOWN,               wxEventHandler(MainDialog::onGrid3access), NULL, this);
    m_grid3->Connect(wxEVT_SCROLLWIN_LINEUP,       wxEventHandler(MainDialog::onGrid3access), NULL, this);
    m_grid3->Connect(wxEVT_SCROLLWIN_LINEDOWN,     wxEventHandler(MainDialog::onGrid3access), NULL, this);
    m_grid3->GetGridWindow()->Connect(wxEVT_LEFT_DOWN, wxEventHandler(MainDialog::onGrid3access), NULL, this);

    Connect(wxEVT_IDLE, wxEventHandler(MainDialog::OnIdleEvent), NULL, this);
    m_grid3->GetGridWindow()->Connect(wxEVT_LEFT_UP, wxEventHandler(MainDialog::OnGrid3LeftMouseUp), NULL, this);
    m_grid3->GetGridWindow()->Connect(wxEVT_LEFT_DOWN, wxEventHandler(MainDialog::OnGrid3LeftMouseDown), NULL, this);

    Connect(wxEVT_SIZE, wxEventHandler(MainDialog::onResizeMainWindow), NULL, this);
    Connect(wxEVT_MOVE, wxEventHandler(MainDialog::onResizeMainWindow), NULL, this);

    wxString toolTip = wxString(_("Legend\n")) +
                       _("---------\n") +
                       _("<|	file on left side only\n") +
                       _("|>	file on right side only\n") +
                       _("<<	left file is newer\n") +
                       _(">>	right file is newer\n") +
                       _("!=	files are different\n") +
                       _("==	files are equal\n\n") +
                       _("(-)	filtered out from sync-process\n");
    m_grid3->GetGridWindow()->SetToolTip(toolTip);

    //enable parallel scrolling
    m_grid1->setScrollFriends(m_grid1, m_grid2, m_grid3);
    m_grid2->setScrollFriends(m_grid1, m_grid2, m_grid3);
    m_grid3->setScrollFriends(m_grid1, m_grid2, m_grid3);

    //share UI grid data with grids
    m_grid1->setGridDataTable(&gridRefUI, &currentGridData);
    m_grid2->setGridDataTable(&gridRefUI, &currentGridData);
    m_grid3->setGridDataTable(&gridRefUI, &currentGridData);

    //disable sync button as long as "compare" hasn't been triggered.
    enableSynchronization(false);

    //make filesize right justified on grids
    wxGridCellAttr* cellAttributes = m_grid1->GetOrCreateCellAttr(0, 2);
    cellAttributes->SetAlignment(wxALIGN_RIGHT,wxALIGN_CENTRE);
    m_grid1->SetColAttr(2, cellAttributes);

    cellAttributes = m_grid2->GetOrCreateCellAttr(0, 2);        //leave these two rows, might be necessary 'cause wxGridCellAttr is ref-counting
    cellAttributes->SetAlignment(wxALIGN_RIGHT,wxALIGN_CENTRE); //and SetColAttr takes ownership (means: it will call DecRef())
    m_grid2->SetColAttr(2, cellAttributes);

    //as the name says: disable them
    m_grid3->deactivateScrollbars();

    //mainly to update row label sizes...
    writeGrid(currentGridData);

    //load list of last used configuration files
    cfgFileHistory = new wxConfig(wxT("FreeFileSync"));
    for (int i = CfgHistroyLength - 1; i >= 0; --i) //put files in reverse order to history
    {
        const wxString key = wxString(wxT("Selection")) + numberToWxString(i);

        wxString value;
        if (cfgFileHistory->Read(key, &value))
            addCfgFileToHistory(value);
    }
    m_choiceLoad->SetSelection(0);

    //select rows only
    m_grid1->SetSelectionMode(wxGrid::wxGridSelectRows);
    m_grid2->SetSelectionMode(wxGrid::wxGridSelectRows);
    m_grid3->SetSelectionMode(wxGrid::wxGridSelectRows);

    //set color of selections
    wxColour darkBlue(40, 35, 140);
    m_grid1->SetSelectionBackground(darkBlue);
    m_grid2->SetSelectionBackground(darkBlue);
    m_grid3->SetSelectionBackground(darkBlue);
    m_grid1->SetSelectionForeground(*wxWHITE);
    m_grid2->SetSelectionForeground(*wxWHITE);
    m_grid3->SetSelectionForeground(*wxWHITE);

    enableSynchronization(false);

    //initialize language selection
    switch (programLanguage->getLanguage())
    {
    case wxLANGUAGE_GERMAN:
        m_menuItemGerman->Check();
        break;

    default:
        m_menuItemEnglish->Check();
    }
}


MainDialog::~MainDialog()
{
    m_grid1->setGridDataTable(0, 0);
    m_grid2->setGridDataTable(0, 0);
    m_grid3->setGridDataTable(0, 0);

    m_grid1->setSortMarker(-1);
    m_grid2->setSortMarker(-1);

    m_grid1->Disconnect(wxEVT_KEY_DOWN, wxKeyEventHandler(MainDialog::onGrid1ButtonEvent), NULL, this);
    m_grid2->Disconnect(wxEVT_KEY_DOWN, wxKeyEventHandler(MainDialog::onGrid2ButtonEvent), NULL, this);
    m_grid3->Disconnect(wxEVT_KEY_DOWN, wxKeyEventHandler(MainDialog::onGrid3ButtonEvent), NULL, this);

    m_grid1->Disconnect(wxEVT_KEY_DOWN,               wxEventHandler(MainDialog::onGrid1access), NULL, this);
    m_grid1->Disconnect(wxEVT_SCROLLWIN_TOP,          wxEventHandler(MainDialog::onGrid1access), NULL, this);
    m_grid1->Disconnect(wxEVT_SCROLLWIN_BOTTOM,       wxEventHandler(MainDialog::onGrid1access), NULL, this);
    m_grid1->Disconnect(wxEVT_SCROLLWIN_LINEUP,       wxEventHandler(MainDialog::onGrid1access), NULL, this);
    m_grid1->Disconnect(wxEVT_SCROLLWIN_LINEDOWN,     wxEventHandler(MainDialog::onGrid1access), NULL, this);
    m_grid1->Disconnect(wxEVT_SCROLLWIN_PAGEUP,       wxEventHandler(MainDialog::onGrid1access), NULL, this);
    m_grid1->Disconnect(wxEVT_SCROLLWIN_PAGEDOWN,     wxEventHandler(MainDialog::onGrid1access), NULL, this);
    m_grid1->Disconnect(wxEVT_SCROLLWIN_THUMBTRACK,   wxEventHandler(MainDialog::onGrid1access), NULL, this);
    m_grid1->Disconnect(wxEVT_SCROLLWIN_THUMBRELEASE, wxEventHandler(MainDialog::onGrid1access), NULL, this);
    m_grid1->Disconnect(wxEVT_GRID_LABEL_LEFT_CLICK,  wxEventHandler(MainDialog::onGrid1access), NULL, this);
    m_grid1->GetGridWindow()->Disconnect(wxEVT_LEFT_DOWN, wxEventHandler(MainDialog::onGrid1access), NULL, this);

    m_grid2->Disconnect(wxEVT_KEY_DOWN,               wxEventHandler(MainDialog::onGrid2access), NULL, this);
    m_grid2->Disconnect(wxEVT_SCROLLWIN_TOP,          wxEventHandler(MainDialog::onGrid2access), NULL, this);
    m_grid2->Disconnect(wxEVT_SCROLLWIN_BOTTOM,       wxEventHandler(MainDialog::onGrid2access), NULL, this);
    m_grid2->Disconnect(wxEVT_SCROLLWIN_LINEUP,       wxEventHandler(MainDialog::onGrid2access), NULL, this);
    m_grid2->Disconnect(wxEVT_SCROLLWIN_LINEDOWN,     wxEventHandler(MainDialog::onGrid2access), NULL, this);
    m_grid2->Disconnect(wxEVT_SCROLLWIN_PAGEUP,       wxEventHandler(MainDialog::onGrid2access), NULL, this);
    m_grid2->Disconnect(wxEVT_SCROLLWIN_PAGEDOWN,     wxEventHandler(MainDialog::onGrid2access), NULL, this);
    m_grid2->Disconnect(wxEVT_SCROLLWIN_THUMBTRACK,   wxEventHandler(MainDialog::onGrid2access), NULL, this);
    m_grid2->Disconnect(wxEVT_SCROLLWIN_THUMBRELEASE, wxEventHandler(MainDialog::onGrid2access), NULL, this);
    m_grid2->Disconnect(wxEVT_GRID_LABEL_LEFT_CLICK,  wxEventHandler(MainDialog::onGrid2access), NULL, this);
    m_grid2->GetGridWindow()->Disconnect(wxEVT_LEFT_DOWN, wxEventHandler(MainDialog::onGrid2access), NULL, this);

    m_grid3->Disconnect(wxEVT_KEY_DOWN,               wxEventHandler(MainDialog::onGrid3access), NULL, this);
    m_grid3->Disconnect(wxEVT_SCROLLWIN_LINEUP,       wxEventHandler(MainDialog::onGrid3access), NULL, this);
    m_grid3->Disconnect(wxEVT_SCROLLWIN_LINEDOWN,     wxEventHandler(MainDialog::onGrid3access), NULL, this);
    m_grid3->GetGridWindow()->Disconnect(wxEVT_LEFT_DOWN, wxEventHandler(MainDialog::onGrid3access), NULL, this);

    Disconnect(wxEVT_IDLE, wxEventHandler(MainDialog::OnIdleEvent), NULL, this);
    m_grid3->GetGridWindow()->Disconnect(wxEVT_LEFT_UP, wxEventHandler(MainDialog::OnGrid3LeftMouseUp), NULL, this);
    m_grid3->GetGridWindow()->Disconnect(wxEVT_LEFT_DOWN, wxEventHandler(MainDialog::OnGrid3LeftMouseDown), NULL, this);

    Disconnect(wxEVT_SIZE, wxEventHandler(MainDialog::onResizeMainWindow), NULL, this);
    Disconnect(wxEVT_MOVE, wxEventHandler(MainDialog::onResizeMainWindow), NULL, this);

    contextMenu->Disconnect(wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(MainDialog::onContextMenuSelection), NULL, this);

    delete contextMenu;

    //write list of last used configuration files
    int vectorSize = cfgFileNames.size();
    for (int i = 0; i < CfgHistroyLength; ++i)
    {
        const wxString key = wxString(wxT("Selection")) + numberToWxString(i);

        if (i < vectorSize)
            cfgFileHistory->Write(key, cfgFileNames[i]);
        else
        {
            if (cfgFileHistory->Exists(key))
                cfgFileHistory->DeleteEntry(key);
        }
    }
    delete cfgFileHistory;

    writeConfigurationToHD(FreeFileSync::FfsLastConfigFile);   //don't trow exceptions in destructors

    if (restartOnExit)
    {   //create new dialog
        MainDialog* frame = new MainDialog(0L, FreeFileSync::FfsLastConfigFile, programLanguage);
        frame->SetIcon(*GlobalResources::programIcon); //set application icon
        frame->Show();
    }
}


void MainDialog::onGrid1access(wxEvent& event)
{
    if (leadingPanel != 1)
    {
        leadingPanel = 1;
        m_grid1->SetFocus();

        m_grid2->ClearSelection(); //clear selection on grid2
    }
    event.Skip();
}


void MainDialog::onGrid2access(wxEvent& event)
{
    if (leadingPanel != 2)
    {
        leadingPanel = 2;
        m_grid2->SetFocus();

        m_grid1->ClearSelection(); //clear selection on grid1
    }
    event.Skip();
}


void MainDialog::onGrid3access(wxEvent& event)
{
    if (leadingPanel != 3)
    {
        leadingPanel = 3;
        m_grid1->ClearSelection(); //clear selection on grid1
        m_grid2->ClearSelection(); //clear selection on grid1
    }
    event.Skip();
}


void MainDialog::enableSynchronization(bool value)
{
    if (value)
    {
        synchronizationEnabled = true;
        m_bpButtonSync->SetBitmapLabel(*GlobalResources::bitmapSync);
    }
    else
    {
        synchronizationEnabled = false;
        m_bpButtonSync->SetBitmapLabel(*GlobalResources::bitmapSyncDisabled);
    }
}


void MainDialog::filterRangeManual(const set<int>& rowsToFilterOnUI_View)
{
    if (rowsToFilterOnUI_View.size() > 0)
    {
        int gridSizeUI = gridRefUI.size();

        bool newSelection = false;   //default: deselect range

        //leadingRow determines de-/selection of all other rows
        int leadingRow = *rowsToFilterOnUI_View.begin();
        if (0 <= leadingRow && leadingRow < gridSizeUI)
            newSelection = !currentGridData[gridRefUI[leadingRow]].selectedForSynchronization;

        if (cfg.hideFiltered)
            assert (!newSelection); //if hidefiltered is active, there should be no filtered elements on screen => current element was filtered out


        //get all lines that need to be filtered (e.g. if a folder is marked, then its subelements should be marked as well)
        set<int> rowsToFilterOnGridData;    //rows to filter in backend

        for (set<int>::iterator i = rowsToFilterOnUI_View.begin(); i != rowsToFilterOnUI_View.end(); ++i)
        {
            if (0 <= *i && *i < gridSizeUI)
            {
                unsigned int gridIndex = gridRefUI[*i];

                rowsToFilterOnGridData.insert(gridIndex);
                FreeFileSync::addSubElements(rowsToFilterOnGridData, currentGridData, currentGridData[gridIndex]);
            }
        }

        //toggle selection of filtered rows
        for (set<int>::iterator i = rowsToFilterOnGridData.begin(); i != rowsToFilterOnGridData.end(); ++i)
            currentGridData[*i].selectedForSynchronization = newSelection;

        //signal UI that grids need to be refreshed on next Update()
        m_grid1->ForceRefresh();
        m_grid2->ForceRefresh();
        m_grid3->ForceRefresh();

        if (cfg.hideFiltered)
        {
            Update();   //show changes resulting from ForceRefresh()

            wxLongLong waitBegin = wxGetLocalTimeMillis();

            //determine rows that are currently filtered out on current UI view (and need to be removed)
            set<int> filteredOutRowsOnUI;
            for (GridView::iterator i = gridRefUI.begin(); i != gridRefUI.end(); ++i)
            {
                const FileCompareLine& gridLine = currentGridData[*i];

                if (!gridLine.selectedForSynchronization)
                    filteredOutRowsOnUI.insert(i - gridRefUI.begin());
            }

            //some delay to show user the rows he has filtered out before they are removed
            unsigned long waitRemaining = max(400 - (wxGetLocalTimeMillis() - waitBegin).GetLo(), unsigned(0));
            wxMilliSleep(waitRemaining);    //400 ms delay before rows are removed from UI

            //delete rows, that are filtered out:
            removeRowsFromVector(gridRefUI, filteredOutRowsOnUI);

            //redraw grid necessary to update new dimensions
            writeGrid(currentGridData, true);   //use UI buffer, just a re-dimensioning of grids
            updateStatusInformation(gridRefUI); //status information has to be recalculated!
        }
    }

    //clear selection on grids
    if (cfg.hideFiltered)
    {
        m_grid1->ClearSelection();
        m_grid2->ClearSelection();
    } //exception for grid 3
    m_grid3->ClearSelection();
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

            filterRangeManual(getSelectedRows());
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


void MainDialog::copySelectionToClipboard(const set<int>& selectedRows, int selectedGrid)
{
    if (selectedRows.size() > 0)
    {
        wxGrid* grid = 0;
        switch (selectedGrid)
        {
        case 1:
            grid = m_grid1;
            break;
        case 2:
            grid = m_grid2;
            break;
        case 3:
            grid = m_grid3;
            break;
        default:
            return;
        }

        wxString clipboardString;

        for (set<int>::iterator i = selectedRows.begin(); i != selectedRows.end(); ++i)
        {
            for (int k = 0; k < grid->GetNumberCols(); ++k)
            {
                clipboardString+= grid->GetCellValue(*i, k);
                if (k != grid->GetNumberCols() - 1)
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


void removeInvalidRows(set<int>& rows, const int currentUI_Size)
{
    set<int> validRows; //temporal table IS needed here
    for (set<int>::iterator i = rows.begin(); i != rows.end(); ++i)
        if (0 <= *i)
        {
            if (*i >= currentUI_Size)   //set is sorted, so no need to continue here
                break;
            validRows.insert(*i);
        }
    rows = validRows;
}


set<int> MainDialog::getSelectedRows()
{
    wxGrid* grid = 0;

    switch (leadingPanel)
    {
    case 1:
        grid = m_grid1;
        break;
    case 2:
        grid = m_grid2;
        break;
    case 3:
        grid = m_grid3;
        break;
    default:
        return set<int>();
    }

    set<int> output;
    int rowTop, rowBottom; //coords of selection

    wxArrayInt selectedRows = grid->GetSelectedRows();
    if (!selectedRows.IsEmpty())
    {
        for (unsigned int i = 0; i < selectedRows.GetCount(); ++i)
            output.insert(selectedRows[i]);
    }

    if (!grid->GetSelectedCols().IsEmpty())    //if a column is selected this is means all rows are marked for deletion
    {
        for (int k = 0; k < grid->GetNumberRows(); ++k)
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

    removeInvalidRows(output, gridRefUI.size());

    return output;
}


class DeleteStatusUpdater : public StatusUpdater
{
public:
    DeleteStatusUpdater(bool& unsolvedErrorOccured) : continueOnError(false), unsolvedErrors(unsolvedErrorOccured) {}
    ~DeleteStatusUpdater() {}

    int reportError(const wxString& text)
    {
        if (continueOnError)
        {
            unsolvedErrors = true;
            return StatusUpdater::continueNext;
        }

        wxString errorMessage = text + _("\n\nInformation: If you skip the error and continue or abort a re-compare will be necessary!");

        ErrorDlg* errorDlg = new ErrorDlg(errorMessage, continueOnError);

        int rv = errorDlg->ShowModal();
        errorDlg->Destroy();

        switch (rv)
        {
        case ErrorDlg::continueButtonPressed:
            unsolvedErrors = true;
            return StatusUpdater::continueNext;
        case ErrorDlg::retryButtonPressed:
            return StatusUpdater::retry;
        case ErrorDlg::abortButtonPressed:
        {
            unsolvedErrors = true;
            throw AbortThisProcess();
        }
        default:
            assert (false);
        }

        return StatusUpdater::continueNext; //dummy return value
    }
    void updateStatusText(const wxString& text) {}
    void initNewProcess(int objectsTotal, double dataTotal, int processID) {}
    void updateProcessedData(int objectsProcessed, double dataProcessed) {}
    void triggerUI_Refresh() {}

private:
    bool continueOnError;
    bool& unsolvedErrors;
};


void MainDialog::deleteFilesOnGrid(const set<int>& rowsToDeleteOnUI)
{
    if (rowsToDeleteOnUI.size())
    {
        //map grid lines from UI to grid lines in backend (gridData)
        set<int> rowsToDeleteOnGrid;
        for (set<int>::iterator i = rowsToDeleteOnUI.begin(); i != rowsToDeleteOnUI.end(); ++i)
            rowsToDeleteOnGrid.insert(gridRefUI[*i]);

        wxString headerText;
        wxString filesToDelete;

        if (cfg.useRecycleBin)
            headerText = _("Do you really want to move the following objects(s) to the recycle bin?");
        else
            headerText = _("Do you really want to delete the following objects(s)?");

        for (set<int>::iterator i = rowsToDeleteOnGrid.begin(); i != rowsToDeleteOnGrid.end(); ++i)
        {
            const FileCompareLine& currentCmpLine = currentGridData[*i];

            if (currentCmpLine.fileDescrLeft.objType != TYPE_NOTHING)
                filesToDelete+= currentCmpLine.fileDescrLeft.filename + wxT("\n");

            if (currentCmpLine.fileDescrRight.objType != TYPE_NOTHING)
                filesToDelete+= currentCmpLine.fileDescrRight.filename + wxT("\n");

            filesToDelete+= wxT("\n");
        }

        DeleteDialog* confirmDeletion = new DeleteDialog(headerText, filesToDelete, this); //no destruction needed; attached to main window

        switch (confirmDeletion->ShowModal())
        {
        case DeleteDialog::okayButtonPressed:
        {
            bool unsolvedErrorOccured = false; //if an error is skipped a re-compare will be necessary!

            try
            {   //class errors when deleting files/folders
                DeleteStatusUpdater deleteStatusUpdater(unsolvedErrorOccured);

                FreeFileSync::deleteOnGridAndHD(currentGridData, rowsToDeleteOnGrid, &deleteStatusUpdater, cfg.useRecycleBin);
            }
            catch (AbortThisProcess& theException)
                {}


            //disable the sync button if errors occured during deletion
            if (unsolvedErrorOccured)
                enableSynchronization(false);


            //redraw grid neccessary to update new dimensions and for UI-Backend data linkage
            writeGrid(currentGridData);  //do NOT use UI buffer here

            m_grid1->ClearSelection(); //clear selection on grid
            m_grid2->ClearSelection(); //clear selection on grid
            m_grid3->ClearSelection(); //clear selection on grid
        }
        break;

        case DeleteDialog::cancelButtonPressed:
        default:
            break;

        }
    }
}


void MainDialog::openWithFileBrowser(int rowNumber, int gridNr)
{
#ifdef FFS_WIN
    if (gridNr == 1)
    {
        wxString command = wxString(wxT("explorer ")) + FreeFileSync::getFormattedDirectoryName(m_directoryPanel1->GetValue()); //default

        if (0 <= rowNumber && rowNumber < int(gridRefUI.size()))
        {
            wxString filename = currentGridData[gridRefUI[rowNumber]].fileDescrLeft.filename;

            if (!filename.IsEmpty())
                command = wxString(wxT("explorer /select,")) + filename;
        }
        wxExecute(command);
    }
    else if (gridNr == 2)
    {
        wxString command = wxString(wxT("explorer ")) + FreeFileSync::getFormattedDirectoryName(m_directoryPanel2->GetValue()); //default

        if (0 <= rowNumber && rowNumber < int(gridRefUI.size()))
        {
            wxString filename = currentGridData[gridRefUI[rowNumber]].fileDescrRight.filename;

            if (!filename.IsEmpty())
                command = wxString(wxT("explorer /select,")) + filename;
        }
        wxExecute(command);
    }
#endif  // FFS_WIN
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


void MainDialog::onGrid1ButtonEvent(wxKeyEvent& event)
{
    //CTRL + C || CTRL + INS
    if (event.ControlDown() && event.GetKeyCode() == 67 ||
            event.ControlDown() && event.GetKeyCode() == WXK_INSERT)
        copySelectionToClipboard(getSelectedRows(), 1);

    else if (event.GetKeyCode() == WXK_DELETE)
        deleteFilesOnGrid(getSelectedRows());

    event.Skip();
}


void MainDialog::onGrid2ButtonEvent(wxKeyEvent& event)
{
    //CTRL + C || CTRL + INS
    if (event.ControlDown() && event.GetKeyCode() == 67 ||
            event.ControlDown() && event.GetKeyCode() == WXK_INSERT)
        copySelectionToClipboard(getSelectedRows(), 2);

    else if (event.GetKeyCode() == WXK_DELETE)
        deleteFilesOnGrid(getSelectedRows());

    event.Skip();
}


void MainDialog::onGrid3ButtonEvent(wxKeyEvent& event)
{
    //CTRL + C || CTRL + INS
    if (event.ControlDown() && event.GetKeyCode() == 67 ||
            event.ControlDown() && event.GetKeyCode() == WXK_INSERT)
        copySelectionToClipboard(getSelectedRows(), 3);

    else if (event.GetKeyCode() == WXK_DELETE)
        deleteFilesOnGrid(getSelectedRows());

    event.Skip();
}


void MainDialog::OnOpenContextMenu( wxGridEvent& event )
{
    set<int> selection = getSelectedRows();

    //enable/disable context menu entries
    if (selection.size() > 0)
    {
        contextMenu->Enable(CONTEXT_MANUAL_FILTER, true);
        contextMenu->Enable(CONTEXT_CLIPBOARD, true);
        contextMenu->Enable(CONTEXT_DELETE_FILES, true);
    }
    else
    {
        contextMenu->Enable(CONTEXT_MANUAL_FILTER, false);
        contextMenu->Enable(CONTEXT_CLIPBOARD, false);
        contextMenu->Enable(CONTEXT_DELETE_FILES, false);
    }

#ifdef FFS_WIN
    if ((leadingPanel == 1 || leadingPanel == 2) && selection.size() <= 1)
        contextMenu->Enable(CONTEXT_EXPLORER, true);
    else
        contextMenu->Enable(CONTEXT_EXPLORER, false);
#endif

    //show context menu
    PopupMenu(contextMenu);
    event.Skip();
}


void MainDialog::onContextMenuSelection(wxCommandEvent& event)
{
    set<int> selection;

    switch (event.GetId())
    {
    case CONTEXT_MANUAL_FILTER:
        filterRangeManual(getSelectedRows());
        break;

    case CONTEXT_CLIPBOARD:
        copySelectionToClipboard(getSelectedRows(), leadingPanel);
        break;

    case CONTEXT_EXPLORER:
        selection = getSelectedRows();

        if (leadingPanel == 1 || leadingPanel == 2)
        {
            if (selection.size() == 1)
                openWithFileBrowser(*selection.begin(), leadingPanel);
            else if (selection.size() == 0)
                openWithFileBrowser(-1, leadingPanel);
        }
        break;

    case CONTEXT_DELETE_FILES:
        deleteFilesOnGrid(getSelectedRows());
        break;
    }

    event.Skip();
}


void MainDialog::OnEnterLeftDir( wxCommandEvent& event )
{
    wxString newDir = m_directoryPanel1->GetValue();
    m_dirPicker1->SetPath(newDir);

    event.Skip();
}


void MainDialog::OnEnterRightDir( wxCommandEvent& event )
{
    wxString newDir = m_directoryPanel2->GetValue();
    m_dirPicker2->SetPath(newDir);

    event.Skip();
}


void MainDialog::OnDirChangedPanel1(wxFileDirPickerEvent& event)
{
    wxString newPath = m_dirPicker1->GetPath();

    m_directoryPanel1->SetValue(newPath);

    //disable the sync button
    enableSynchronization(false);

    //clear grids
    currentGridData.clear();
    writeGrid(currentGridData);

    event.Skip();
}


void MainDialog::OnDirChangedPanel2(wxFileDirPickerEvent& event)
{
    wxString newPath = m_dirPicker2->GetPath();

    m_directoryPanel2->SetValue(newPath);

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
    if (output.EndsWith(wxT(".ffs")))
        output = output.BeforeLast('.');
    return output;
}


//tests if the same filenames are specified, even if they are relative to the current working directory
bool sameFileSpecified(const wxString& file1, const wxString& file2)
{
    wxString file1Full = file1;
    wxString file2Full = file2;

    if (wxFileName(file1).GetPath() == wxEmptyString)
        file1Full = wxFileName::GetCwd() + GlobalResources::fileNameSeparator + file1;

    if (wxFileName(file2).GetPath() == wxEmptyString)
        file2Full = wxFileName::GetCwd() + GlobalResources::fileNameSeparator + file2;

    return (file1Full == file2Full);
}


void MainDialog::addCfgFileToHistory(const wxString& filename)
{
    //the default configFile should not be in the history
    if (sameFileSpecified(FreeFileSync::FfsLastConfigFile, filename))
        return;


    bool duplicateEntry = false;
    unsigned int duplicatePos = 0;
    for (unsigned int i = 0; i < cfgFileNames.size(); ++i)
        if (sameFileSpecified(cfgFileNames[i], filename))
        {
            duplicateEntry = true;
            duplicatePos   = i;
            break;
        }


    if (duplicateEntry)    //if entry is in the list, then jump to element
    {
        m_choiceLoad->SetSelection(duplicatePos + 1);
    }
    else
    {
        cfgFileNames.insert(cfgFileNames.begin(), filename);
        m_choiceLoad->Insert(getFormattedHistoryElement(filename), 1);  //insert after "Load configuration..."
        m_choiceLoad->SetSelection(1);
    }

    //keep maximal size of history list
    if (cfgFileNames.size() > unsigned(CfgHistroyLength))
    {
        //delete last rows
        cfgFileNames.erase(cfgFileNames.end() - 1);
        m_choiceLoad->Delete(CfgHistroyLength);  //don't forget: m_choiceLoad has (cfgHistroyLength + 1) elements
    }
}


void onFilesDropped(const wxString& elementName, wxTextCtrl* txtCtrl, wxDirPickerCtrl* dirPicker)
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


bool FileDropEvent::OnDropFiles(wxCoord x, wxCoord y, const wxArrayString& filenames)
{
    if (filenames.IsEmpty())
        return false;
    else
    {
        //disable the sync button
        mainDlg->enableSynchronization(false);

        //clear grids
        mainDlg->currentGridData.clear();
        mainDlg->writeGrid(mainDlg->currentGridData);

        const wxString droppedFileName = filenames[0];

        //test if ffs config file has been dropped
        if (wxFileExists(droppedFileName) && FreeFileSync::isFFS_ConfigFile(droppedFileName))
        {
            mainDlg->readConfigurationFromHD(droppedFileName);

            mainDlg->pushStatusInformation(_("Configuration loaded!"));
        }

        else if (targetGrid == 1)
            onFilesDropped(droppedFileName, mainDlg->m_directoryPanel1, mainDlg->m_dirPicker1);

        else if (targetGrid == 2)
            onFilesDropped(droppedFileName, mainDlg->m_directoryPanel2, mainDlg->m_dirPicker2);
    }
    return false;
}


void MainDialog::OnSaveConfig(wxCommandEvent& event)
{
    wxString defaultFileName = wxT("SyncSettings.ffs");

    //try to use last selected configuration file as default
    int selectedItem;
    if ((selectedItem = m_choiceLoad->GetSelection()) != wxNOT_FOUND)
        if (1 <= selectedItem && unsigned(selectedItem) < m_choiceLoad->GetCount())
            if (unsigned(selectedItem - 1) < cfgFileNames.size())
                defaultFileName = cfgFileNames[selectedItem - 1];


    wxFileDialog* filePicker = new wxFileDialog(this, wxEmptyString, wxEmptyString, defaultFileName, wxString(_("FreeFileSync configuration")) + wxT(" (*.ffs)|*.ffs"), wxFD_SAVE);

    if (filePicker->ShowModal() == wxID_OK)
    {
        wxString newFileName = filePicker->GetPath();

        if (wxFileExists(newFileName))
        {
            wxMessageDialog* messageDlg = new wxMessageDialog(this, wxString(wxT("\"")) + newFileName + wxT("\"") + _(" already exists. Overwrite?"), _("Warning") , wxOK | wxCANCEL);

            if (messageDlg->ShowModal() != wxID_OK)
            {
                pushStatusInformation(_("Saved aborted!"));
                return;
            }
        }
        writeConfigurationToHD(newFileName);

        pushStatusInformation(_("Configuration saved!"));
    }
}


void MainDialog::OnLoadConfiguration(wxCommandEvent& event)
{
    int selectedItem;
    if ((selectedItem = m_choiceLoad->GetSelection()) != wxNOT_FOUND)
    {
        //clear grids
        currentGridData.clear();
        writeGrid(currentGridData);

        wxString newCfgFile;

        wxFileDialog* filePicker = NULL;
        switch (selectedItem)
        {
        case 0: //load config from file
            filePicker = new wxFileDialog(this, wxEmptyString, wxEmptyString, wxEmptyString, wxString(_("FreeFileSync configuration")) + wxT(" (*.ffs)|*.ffs"), wxFD_OPEN);

            if (filePicker->ShowModal() == wxID_OK)
                newCfgFile = filePicker->GetPath();

            break;
        default:
            if (1 <= selectedItem && unsigned(selectedItem) < m_choiceLoad->GetCount())
            {
                if (unsigned(selectedItem - 1) < cfgFileNames.size())
                    newCfgFile = cfgFileNames[selectedItem - 1];
            }
            break;
        }

        if (!newCfgFile.IsEmpty())
        {
            if (!wxFileExists(newCfgFile))
                wxMessageBox(_("The selected file does not exist anymore!"), _("Warning"), wxOK);
            else if (!FreeFileSync::isFFS_ConfigFile(newCfgFile))
                wxMessageBox(_("The selected file does not contain a valid configuration!"), _("Warning"), wxOK);
            else
            {
                readConfigurationFromHD(newCfgFile);

                pushStatusInformation(_("Configuration loaded!"));
            }
        }
    }
    event.Skip();
}


void MainDialog::OnChoiceKeyEvent(wxKeyEvent& event)
{
    if (event.GetKeyCode() == WXK_DELETE)
    {   //try to delete the currently selected config history item
        int selectedItem;
        if ((selectedItem = m_choiceLoad->GetSelection()) != wxNOT_FOUND)
            if (1 <= selectedItem && unsigned(selectedItem) < m_choiceLoad->GetCount())
                if (unsigned(selectedItem - 1) < cfgFileNames.size())
                {
                    //delete selected row
                    cfgFileNames.erase(cfgFileNames.begin() + selectedItem - 1);
                    m_choiceLoad->Delete(selectedItem);
                }
    }
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


void MainDialog::loadDefaultConfiguration()
{
    //default values
    cfg.syncConfiguration.exLeftSideOnly  = SYNC_DIR_RIGHT;
    cfg.syncConfiguration.exRightSideOnly = SYNC_DIR_RIGHT;
    cfg.syncConfiguration.leftNewer       = SYNC_DIR_RIGHT;
    cfg.syncConfiguration.rightNewer      = SYNC_DIR_RIGHT;
    cfg.syncConfiguration.different       = SYNC_DIR_RIGHT;

    cfg.compareVar = CMP_BY_TIME_SIZE; //compare algorithm
    updateCompareButtons();

    cfg.includeFilter = wxT("*");    //include all files/folders
    cfg.excludeFilter = wxEmptyString;     //exlude nothing

    //set status of filter button
    cfg.filterIsActive = false; //do not filter by default
    updateFilterButton(m_bpButtonFilter, cfg.filterIsActive);

    //set status of "hide filtered items" checkbox
    cfg.hideFiltered  = false;  //show filtered items
    m_checkBoxHideFilt->SetValue(cfg.hideFiltered);

    cfg.useRecycleBin   = false;  //do not use: in case OS doesn't support this, user will have to activate first and then get the error message
    cfg.continueOnError = false;

    widthNotMaximized  = wxDefaultCoord;
    heightNotMaximized = wxDefaultCoord;
    posXNotMaximized   = wxDefaultCoord;
    posYNotMaximized   = wxDefaultCoord;
}


void MainDialog::readConfigurationFromHD(const wxString& filename, bool programStartup)
{
    char bigBuffer[10000];

    ifstream config(filename.c_str());
    if (!config)
    {
        if (programStartup)
            loadDefaultConfiguration();
        else
            wxMessageBox(wxString(_("Could not read configuration file ")) + wxT("\"") + filename + wxT("\""), _("An exception occured!"), wxOK | wxICON_ERROR);

        return;
    }

    //read FFS identifier
    config.get(bigBuffer, FreeFileSync::FfsConfigFileID.size() + 1);

    if (string(bigBuffer) != FreeFileSync::FfsConfigFileID)
    {
        wxMessageBox(_("The selected file does not contain a valid configuration!"), _("Warning"), wxOK);
        config.close();
        return;
    }


    //put filename on list of last used config files
    addCfgFileToHistory(filename);


    //read sync configuration
    cfg.syncConfiguration.exLeftSideOnly  = SyncDirection(config.get());
    cfg.syncConfiguration.exRightSideOnly = SyncDirection(config.get());
    cfg.syncConfiguration.leftNewer       = SyncDirection(config.get());
    cfg.syncConfiguration.rightNewer      = SyncDirection(config.get());
    cfg.syncConfiguration.different       = SyncDirection(config.get());

    //read compare algorithm
    cfg.compareVar = CompareVariant(config.get());
    updateCompareButtons();

    //read column sizes
    for (int i = 0; i < m_grid1->GetNumberCols(); ++i)
    {
        config.getline(bigBuffer, 100, char(0));
        m_grid1->SetColSize(i, atoi(bigBuffer));
    }
    for (int i = 0; i < m_grid2->GetNumberCols(); ++i)
    {
        config.getline(bigBuffer, 100, char(0));
        m_grid2->SetColSize(i, atoi(bigBuffer));
    }

    //read application window size and position
    bool startWindowMaximized = bool(config.get());

    config.getline(bigBuffer, 100, char(0));
    int widthTmp = atoi(bigBuffer);
    config.getline(bigBuffer, 100, char(0));
    int heighthTmp = atoi(bigBuffer);

    config.getline(bigBuffer, 100, char(0));
    int posX_Tmp = atoi(bigBuffer);
    config.getline(bigBuffer, 100, char(0));
    int posY_Tmp = atoi(bigBuffer);

    //apply window size and position at program startup ONLY
    if (programStartup)
    {
        widthNotMaximized  = widthTmp;
        heightNotMaximized = heighthTmp;
        posXNotMaximized   = posX_Tmp;
        posYNotMaximized   = posY_Tmp;

        //apply window size and position
        SetSize(posXNotMaximized, posYNotMaximized, widthNotMaximized, heightNotMaximized);
        Maximize(startWindowMaximized);
    }

    //read last directory selection
    config.getline(bigBuffer, 10000, char(0));
    m_directoryPanel1->SetValue(bigBuffer);
    if (wxDirExists(bigBuffer))
        m_dirPicker1->SetPath(bigBuffer);

    config.getline(bigBuffer, 10000, char(0));
    m_directoryPanel2->SetValue(bigBuffer);
    if (wxDirExists(bigBuffer))
        m_dirPicker2->SetPath(bigBuffer);

    //read filter settings:
    cfg.hideFiltered = bool(config.get());
    m_checkBoxHideFilt->SetValue(cfg.hideFiltered);

    cfg.filterIsActive = bool(config.get());
    updateFilterButton(m_bpButtonFilter, cfg.filterIsActive);

    //include
    config.getline(bigBuffer, 10000, char(0));
    cfg.includeFilter = bigBuffer;

    //exclude
    config.getline(bigBuffer, 10000, char(0));
    cfg.excludeFilter = bigBuffer;

    cfg.useRecycleBin   =  bool(config.get());

    cfg.continueOnError =  bool(config.get());

    config.close();
}


void MainDialog::writeConfigurationToHD(const wxString& filename)
{
    ofstream config(filename.c_str());
    if (!config)
    {
        wxMessageBox(wxString(_("Could not write to ")) + wxT("\"") + filename + wxT("\""), _("An exception occured!"), wxOK | wxICON_ERROR);
        return;
    }

    //put filename on list of last used config files
    addCfgFileToHistory(filename);

    //write FFS identifier
    config<<FreeFileSync::FfsConfigFileID.c_str();

    //write sync configuration
    config<<char(cfg.syncConfiguration.exLeftSideOnly)
    <<char(cfg.syncConfiguration.exRightSideOnly)
    <<char(cfg.syncConfiguration.leftNewer)
    <<char(cfg.syncConfiguration.rightNewer)
    <<char(cfg.syncConfiguration.different);

    //write compare algorithm
    config<<char(cfg.compareVar);

    //write column sizes
    for (int i = 0; i < m_grid1->GetNumberCols(); ++i)
        config<<m_grid1->GetColSize(i)<<char(0);
    for (int i = 0; i < m_grid2->GetNumberCols(); ++i)
        config<<m_grid2->GetColSize(i)<<char(0);

    //write application window size and position
    config<<char(IsMaximized());

    //window size
    config<<widthNotMaximized<<char(0);
    config<<heightNotMaximized<<char(0);

    //window position
    config<<posXNotMaximized<<char(0);
    config<<posYNotMaximized<<char(0);

    //write last directory selection
    config<<m_directoryPanel1->GetValue().c_str()<<char(0)
    <<m_directoryPanel2->GetValue().c_str()<<char(0);

    //write filter settings
    config<<char(cfg.hideFiltered);
    config<<char(cfg.filterIsActive);

    config<<cfg.includeFilter.c_str()<<char(0)
    <<cfg.excludeFilter.c_str()<<char(0);

    config<<char(cfg.useRecycleBin);

    config<<char(cfg.continueOnError);

    config.close();
}


void MainDialog::OnShowHelpDialog(wxCommandEvent &event)
{
    HelpDlg* helpDlg = new HelpDlg(this);
    helpDlg->ShowModal();
    event.Skip();
}


void MainDialog::OnFilterButton(wxCommandEvent &event)
{   //toggle filter on/off
    cfg.filterIsActive = !cfg.filterIsActive;
    //make sure, button-appearance and "filterIsActive" are in sync.
    updateFilterButton(m_bpButtonFilter, cfg.filterIsActive);

    if (cfg.filterIsActive)
        FreeFileSync::filterCurrentGridData(currentGridData, cfg.includeFilter, cfg.excludeFilter);
    else
        FreeFileSync::removeFilterOnCurrentGridData(currentGridData);

    writeGrid(currentGridData);
    event.Skip();
}


void MainDialog::OnHideFilteredButton(wxCommandEvent &event)
{   //toggle showing filtered rows
    cfg.hideFiltered = !cfg.hideFiltered;
    //make sure, checkbox and "hideFiltered" are in sync
    m_checkBoxHideFilt->SetValue(cfg.hideFiltered);

    writeGrid(currentGridData);

    event.Skip();
}


void MainDialog::OnConfigureFilter(wxHyperlinkEvent &event)
{
    wxString beforeImage = cfg.includeFilter + wxChar(1) + cfg.excludeFilter;

    FilterDlg* filterDlg = new FilterDlg(this, cfg.includeFilter, cfg.excludeFilter);
    if (filterDlg->ShowModal() == FilterDlg::okayButtonPressed)
    {
        wxString afterImage = cfg.includeFilter + wxChar(1) + cfg.excludeFilter;

        if (beforeImage != afterImage)  //if filter settings are changed: set filtering to "on"
        {
            if (afterImage == (wxString(wxT("*")) + wxChar(1))) //default
            {
                cfg.filterIsActive = false;
                FreeFileSync::removeFilterOnCurrentGridData(currentGridData);
            }
            else
            {
                cfg.filterIsActive = true;
                FreeFileSync::filterCurrentGridData(currentGridData, cfg.includeFilter, cfg.excludeFilter);
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
    event.Skip();
};

void MainDialog::OnLeftNewerFiles(wxCommandEvent& event)
{
    leftNewerFilesActive = !leftNewerFilesActive;
    updateViewFilterButtons();
    writeGrid(currentGridData);
    event.Skip();
};

void MainDialog::OnDifferentFiles(wxCommandEvent& event)
{
    differentFilesActive = !differentFilesActive;
    updateViewFilterButtons();
    writeGrid(currentGridData);
    event.Skip();
};

void MainDialog::OnRightNewerFiles(wxCommandEvent& event)
{
    rightNewerFilesActive = !rightNewerFilesActive;
    updateViewFilterButtons();
    writeGrid(currentGridData);
    event.Skip();
};

void MainDialog::OnRightOnlyFiles(wxCommandEvent& event)
{
    rightOnlyFilesActive = !rightOnlyFilesActive;
    updateViewFilterButtons();
    writeGrid(currentGridData);
    event.Skip();
};

void MainDialog::OnEqualFiles(wxCommandEvent& event)
{
    equalFilesActive = !equalFilesActive;
    updateViewFilterButtons();
    writeGrid(currentGridData);
    event.Skip();
};

void MainDialog::updateViewFilterButtons()
{
    if (leftOnlyFilesActive)
        m_bpButtonLeftOnly->SetBitmapLabel(*GlobalResources::bitmapLeftOnly);
    else
        m_bpButtonLeftOnly->SetBitmapLabel(*GlobalResources::bitmapLeftOnlyDeact);

    if (leftNewerFilesActive)
        m_bpButtonLeftNewer->SetBitmapLabel(*GlobalResources::bitmapLeftNewer);
    else
        m_bpButtonLeftNewer->SetBitmapLabel(*GlobalResources::bitmapLeftNewerDeact);

    if (equalFilesActive)
        m_bpButtonEqual->SetBitmapLabel(*GlobalResources::bitmapEqual);
    else
        m_bpButtonEqual->SetBitmapLabel(*GlobalResources::bitmapEqualDeact);

    if (differentFilesActive)
        m_bpButtonDifferent->SetBitmapLabel(*GlobalResources::bitmapDifferent);
    else
        m_bpButtonDifferent->SetBitmapLabel(*GlobalResources::bitmapDifferentDeact);

    if (rightNewerFilesActive)
        m_bpButtonRightNewer->SetBitmapLabel(*GlobalResources::bitmapRightNewer);
    else
        m_bpButtonRightNewer->SetBitmapLabel(*GlobalResources::bitmapRightNewerDeact);

    if (rightOnlyFilesActive)
        m_bpButtonRightOnly->SetBitmapLabel(*GlobalResources::bitmapRightOnly);
    else
        m_bpButtonRightOnly->SetBitmapLabel(*GlobalResources::bitmapRightOnlyDeact);
}


void MainDialog::updateFilterButton(wxBitmapButton* filterButton, bool isActive)
{
    if (isActive)
    {
        filterButton->SetBitmapLabel(*GlobalResources::bitmapFilterOn);
        filterButton->SetToolTip(_("Filter active: Press again to deactivate"));
    }
    else
    {
        filterButton->SetBitmapLabel(*GlobalResources::bitmapFilterOff);
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


void MainDialog::OnCompare(wxCommandEvent &event)
{
    if (m_directoryPanel1->GetValue().IsEmpty() || m_directoryPanel2->GetValue().IsEmpty())
    {
        wxMessageBox(_("Please select directories for both sides!"), _("Information"));
        return;
    }

    clearStatusBar();

    //check if directories exist if loaded by config file
    if (!wxDirExists(m_directoryPanel1->GetValue()))
    {
        wxMessageBox(_("Directory on the left does not exist. Please select a new one!"), _("Warning"), wxICON_WARNING);
        return;
    }
    else if (!wxDirExists(m_directoryPanel2->GetValue()))
    {
        wxMessageBox(_("Directory on the right does not exist. Please select a new one!"), _("Warning"), wxICON_WARNING);
        return;
    }

    wxBeginBusyCursor();

    bool aborted = false;
    try
    {   //class handling status display and error messages
        CompareStatusUpdater statusUpdater(this);
        cmpStatusUpdaterTmp = &statusUpdater;

        //unsigned int startTime3 = GetTickCount();
        FreeFileSync::startCompareProcess(currentGridData,
                                          FreeFileSync::getFormattedDirectoryName(m_directoryPanel1->GetValue()),
                                          FreeFileSync::getFormattedDirectoryName(m_directoryPanel2->GetValue()),
                                          cfg.compareVar,
                                          &statusUpdater);
        //wxMessageBox(numberToWxString(unsigned(GetTickCount()) - startTime3));


        //filter currentGridData if option is set
        if (cfg.filterIsActive)
            FreeFileSync::filterCurrentGridData(currentGridData, cfg.includeFilter, cfg.excludeFilter);

        writeGrid(currentGridData); //keep it in try/catch to not overwrite status information if compare is aborted

        cmpStatusUpdaterTmp = 0;
    }
    catch (AbortThisProcess& theException)
    {
        aborted = true;
    }

    if (aborted)
    {   //disable the sync button
        enableSynchronization(false);

        m_bpButtonCompare->SetFocus();
    }
    else
    {   //once compare is finished enable the sync button
        enableSynchronization(true);

        m_bpButtonSync->SetFocus();
    }

    wxEndBusyCursor();
    event.Skip();
}


void MainDialog::OnAbortCompare(wxCommandEvent& event)
{
    if (cmpStatusUpdaterTmp)
        cmpStatusUpdaterTmp->requestAbortion();
    event.Skip();
}


void MainDialog::writeGrid(const FileCompareResult& gridData, bool useUI_GridCache)
{
    m_grid1->BeginBatch();
    m_grid2->BeginBatch();
    m_grid3->BeginBatch();

    if (!useUI_GridCache)
    {
        //unsigned int startTime = GetTickCount();
        mapGridDataToUI(gridRefUI, gridData);  //update gridRefUI
        //wxMessageBox(wxString("Benchmark: ") + numberToWxString(unsigned(GetTickCount()) - startTime) + " ms");

        updateStatusInformation(gridRefUI);    //write status information for gridRefUI
    }

    //all three grids retrieve their data directly from currentGridData!!!
    //the only thing left to do is notify the grids to update their sizes (nr of rows), since this has to be communicated via messages by the grids
    m_grid1->updateGridSizes();
    m_grid2->updateGridSizes();
    m_grid3->updateGridSizes();

    //enlarge label width to display row numbers correctly
    int nrOfRows = m_grid1->GetNumberRows();
    if (nrOfRows >= 1)
    {
        int nrOfDigits = int(floor(log10(double(nrOfRows)) + 1));
        m_grid1->SetRowLabelSize(nrOfDigits * 8 + 4);
        m_grid2->SetRowLabelSize(nrOfDigits * 8 + 4);
    }

    //hide sort direction indicator on UI grids
    m_grid1->setSortMarker(-1);
    m_grid2->setSortMarker(-1);


    m_grid1->EndBatch();
    m_grid2->EndBatch();
    m_grid3->EndBatch();
}


void MainDialog::OnSync(wxCommandEvent& event)
{
    SyncDialog* syncDlg = new SyncDialog(this, currentGridData, cfg, synchronizationEnabled);
    if (syncDlg->ShowModal() == SyncDialog::StartSynchronizationProcess)
    {
        //check if there are files/folders to be sync'ed at all
        int objectsToCreate    = 0;
        int objectsToOverwrite = 0;
        int objectsToDelete    = 0;
        double dataToProcess   = 0;
        FreeFileSync::calcTotalBytesToSync(objectsToCreate,
                                           objectsToOverwrite,
                                           objectsToDelete,
                                           dataToProcess,
                                           currentGridData,
                                           cfg.syncConfiguration);
        if (objectsToCreate + objectsToOverwrite + objectsToDelete == 0)
        {
            wxMessageBox(_("Nothing to synchronize. Both directories adhere to the sync-configuration!"), _("Information"), wxICON_WARNING);
            return;
        }


        wxBeginBusyCursor();

        clearStatusBar();

        try
        {
            //class handling status updates and error messages
            SyncStatusUpdater statusUpdater(this, cfg.continueOnError);

            //start synchronization and return elements that were not sync'ed in currentGridData

            //unsigned int startTime3 = GetTickCount();
            FreeFileSync::startSynchronizationProcess(currentGridData, cfg.syncConfiguration, &statusUpdater, cfg.useRecycleBin);
            //wxMessageBox(numberToWxString(unsigned(GetTickCount()) - startTime3));
        }
        catch (AbortThisProcess& theException)
        {   //do NOT disable the sync button: user might want to try to sync the REMAINING rows
        }   //enableSynchronization(false);


        //display files that were not processed
        writeGrid(currentGridData);

        if (currentGridData.size() > 0)
            pushStatusInformation(_("Not all items were synchronized! Have a look at the list."));
        else
        {
            pushStatusInformation(_("All items have been synchronized!"));
            enableSynchronization(false);
        }


        wxEndBusyCursor();
    }
    event.Skip();
}


void MainDialog::OnLeftGridDoubleClick(wxGridEvent& event)
{
    openWithFileBrowser(event.GetRow(), 1);
    event.Skip();
}


void MainDialog::OnRightGridDoubleClick(wxGridEvent& event)
{
    openWithFileBrowser(event.GetRow(), 2);
    event.Skip();
}


//these three global variables are ONLY used for the sorting in the following methods
unsigned int currentSortColumn = 0;
bool sortAscending = true;
FileCompareResult* currentGridDataPtr = 0;

inline
bool cmpString(const wxString& a, const wxString& b)
{
    if (a.IsEmpty())
        return false;   // if a and b are empty: false, if a empty, b not empty: also false, since empty rows should appear at the end
    else if (b.IsEmpty())
        return true;        // empty rows after filled rows: return true

    //if a and b not empty:
    if (sortAscending)
        return (a < b);
    else return (a > b);
}


inline
bool cmpLargeInt(const wxULongLong& a, const wxULongLong& b)
{
    if (sortAscending)
        return (a < b);

    return (a > b);
}


bool sortGridLeft(const GridViewLine a, const GridViewLine b)
{
    const FileDescrLine& gridDataLineA = (*currentGridDataPtr)[a].fileDescrLeft;
    const FileDescrLine& gridDataLineB = (*currentGridDataPtr)[b].fileDescrLeft;

    wxString stringA;
    wxString stringB;

    switch (currentSortColumn)
    {
    case 0:
        //presort types: first files, then directories then empty rows
        if (gridDataLineA.objType == TYPE_NOTHING)
            return false;  //empty rows always last
        else if (gridDataLineB.objType == TYPE_NOTHING)
            return true;  //empty rows always last
        else if (gridDataLineA.objType == TYPE_DIRECTORY)
            return false;
        else if (gridDataLineB.objType == TYPE_DIRECTORY)
            return true;
        else
            return cmpString(gridDataLineA.relFilename.AfterLast(GlobalResources::fileNameSeparator),
                             gridDataLineB.relFilename.AfterLast(GlobalResources::fileNameSeparator));
    case 1:
        if (gridDataLineA.objType == TYPE_DIRECTORY)
            stringA = gridDataLineA.relFilename;
        else
            stringA = gridDataLineA.relFilename.BeforeLast(GlobalResources::fileNameSeparator);

        if (gridDataLineB.objType == TYPE_DIRECTORY)
            stringB = gridDataLineB.relFilename;
        else
            stringB = gridDataLineB.relFilename.BeforeLast(GlobalResources::fileNameSeparator);

        return cmpString(stringA, stringB);

    case 2:
        //presort types: first files, then directories then empty rows
        if (gridDataLineA.objType == TYPE_NOTHING)
            return false;  //empty rows always last
        else if (gridDataLineB.objType == TYPE_NOTHING)
            return true;  //empty rows always last
        else if (gridDataLineA.objType == TYPE_DIRECTORY)
            return false;
        else if (gridDataLineB.objType == TYPE_DIRECTORY)
            return true;
        else       //use unformatted filesizes and sort by size
            return  cmpLargeInt(gridDataLineA.fileSize, gridDataLineB.fileSize);

    case 3:
        return cmpString(gridDataLineA.lastWriteTime, gridDataLineB.lastWriteTime);

    default:
        assert(false);
        return true; //dummy command
    }
}


bool sortGridRight(const GridViewLine a, const GridViewLine b)
{
    const FileDescrLine& gridDataLineA = (*currentGridDataPtr)[a].fileDescrRight;
    const FileDescrLine& gridDataLineB = (*currentGridDataPtr)[b].fileDescrRight;

    wxString stringA;
    wxString stringB;

    switch (currentSortColumn)
    {
    case 0:
        //presort types: first files, then directories then empty rows
        if (gridDataLineA.objType == TYPE_NOTHING)
            return false;  //empty rows always last
        else if (gridDataLineB.objType == TYPE_NOTHING)
            return true;  //empty rows always last
        else if (gridDataLineA.objType == TYPE_DIRECTORY)
            return false;
        else if (gridDataLineB.objType == TYPE_DIRECTORY)
            return true;
        else
            return cmpString(gridDataLineA.relFilename.AfterLast(GlobalResources::fileNameSeparator),
                             gridDataLineB.relFilename.AfterLast(GlobalResources::fileNameSeparator));

    case 1:
        if (gridDataLineA.objType == TYPE_DIRECTORY)
            stringA = gridDataLineA.relFilename;
        else
            stringA = gridDataLineA.relFilename.BeforeLast(GlobalResources::fileNameSeparator);

        if (gridDataLineB.objType == TYPE_DIRECTORY)
            stringB = gridDataLineB.relFilename;
        else
            stringB = gridDataLineB.relFilename.BeforeLast(GlobalResources::fileNameSeparator);

        return cmpString(stringA, stringB);

    case 2:
        //presort types: first files, then directories then empty rows
        if (gridDataLineA.objType == TYPE_NOTHING)
            return false;  //empty rows always last
        else if (gridDataLineB.objType == TYPE_NOTHING)
            return true;  //empty rows always last
        else if (gridDataLineA.objType == TYPE_DIRECTORY)
            return false;
        else if (gridDataLineB.objType == TYPE_DIRECTORY)
            return true;
        else     //use unformatted filesizes and sort by size
            return  cmpLargeInt(gridDataLineA.fileSize, gridDataLineB.fileSize);

    case 3:
        return cmpString(gridDataLineA.lastWriteTime, gridDataLineB.lastWriteTime);

    default:
        assert(false);
        return true; //dummy command
    }
}


void MainDialog::OnSortLeftGrid(wxGridEvent& event)
{
    static bool columnSortAscending[4] = {true, true, false, true};

    currentSortColumn = event.GetCol();
    currentGridDataPtr = &currentGridData;
    if (0 <= currentSortColumn && currentSortColumn <= 3)
    {
        sortAscending = columnSortAscending[currentSortColumn];
        columnSortAscending[currentSortColumn] = !columnSortAscending[currentSortColumn];
        sort(gridRefUI.begin(), gridRefUI.end(), sortGridLeft);

        m_grid1->ForceRefresh();
        m_grid2->ForceRefresh();
        m_grid3->ForceRefresh();

        //set sort direction indicator on UI
        if (sortAscending)
            m_grid1->setSortMarker(currentSortColumn, GlobalResources::bitmapSmallUp);
        else
            m_grid1->setSortMarker(currentSortColumn, GlobalResources::bitmapSmallDown);
        m_grid2->setSortMarker(-1);
    }
    event.Skip();
}


void MainDialog::OnSortRightGrid(wxGridEvent& event)
{
    static bool columnSortAscending[4] = {true, true, false, true};

    currentSortColumn = event.GetCol();
    currentGridDataPtr = &currentGridData;
    if (0 <= currentSortColumn && currentSortColumn <= 3)
    {
        sortAscending = columnSortAscending[currentSortColumn];
        columnSortAscending[currentSortColumn] = !columnSortAscending[currentSortColumn];
        sort(gridRefUI.begin(), gridRefUI.end(), sortGridRight);

        m_grid1->ForceRefresh();
        m_grid2->ForceRefresh();
        m_grid3->ForceRefresh();

        //set sort direction indicator on UI
        m_grid1->setSortMarker(-1);
        if (sortAscending)
            m_grid2->setSortMarker(currentSortColumn, GlobalResources::bitmapSmallUp);
        else
            m_grid2->setSortMarker(currentSortColumn, GlobalResources::bitmapSmallDown);
    }
    event.Skip();
}


void MainDialog::OnSwapDirs( wxCommandEvent& event )
{
    //swap directory names
    wxString tmp = m_directoryPanel1->GetValue();
    m_directoryPanel1->SetValue(m_directoryPanel2->GetValue());
    m_directoryPanel2->SetValue(tmp);

    //swap grid information
    FreeFileSync::swapGrids(currentGridData);
    writeGrid(currentGridData);
    event.Skip();
}


void MainDialog::updateStatusInformation(const GridView& visibleGrid)
{
    while (stackObjects.size() > 0)
        stackObjects.pop();

    unsigned int objectsOnLeftView = 0;
    unsigned int objectsOnRightView = 0;
    wxULongLong filesizeLeftView;
    wxULongLong filesizeRightView;

    wxString statusLeftNew;
    wxString statusMiddleNew;
    wxString statusRightNew;

    for (GridView::const_iterator i = visibleGrid.begin(); i != visibleGrid.end(); ++i)
    {
        const FileCompareLine& refLine = currentGridData[*i];

        //calculate total number of bytes for each sied
        if (refLine.fileDescrLeft.objType != TYPE_NOTHING)
        {
            filesizeLeftView+= refLine.fileDescrLeft.fileSize;
            ++objectsOnLeftView;
        }

        if (refLine.fileDescrRight.objType != TYPE_NOTHING)
        {
            filesizeRightView+= refLine.fileDescrRight.fileSize;
            ++objectsOnRightView;
        }
    }

    //show status information on "root" level. This cannot be accomplished in writeGrid since filesizes are already formatted for display there
    wxString objectsViewLeft = numberToWxString(objectsOnLeftView);
    globalFunctions::includeNumberSeparator(objectsViewLeft);
    if (objectsOnLeftView == 1)
        statusLeftNew = wxString(_("1 item on left, ")) + FreeFileSync::formatFilesizeToShortString(filesizeLeftView);
    else
        statusLeftNew = objectsViewLeft + _(" items on left, ") + FreeFileSync::formatFilesizeToShortString(filesizeLeftView);

    wxString objectsTotal = numberToWxString(currentGridData.size());
    globalFunctions::includeNumberSeparator(objectsTotal);
    wxString objectsView = numberToWxString(visibleGrid.size());
    globalFunctions::includeNumberSeparator(objectsView);
    if (currentGridData.size() == 1)
        statusMiddleNew = objectsView + _(" of ") + objectsTotal + _(" row in view");
    else
        statusMiddleNew = objectsView + _(" of ") + objectsTotal + _(" rows in view");


    wxString objectsViewRight = numberToWxString(objectsOnRightView);
    globalFunctions::includeNumberSeparator(objectsViewRight);
    if (objectsOnRightView == 1)
        statusRightNew = wxString(_("1 item on right, ")) + FreeFileSync::formatFilesizeToShortString(filesizeRightView);
    else
        statusRightNew = objectsViewRight + _(" items on right, ") + FreeFileSync::formatFilesizeToShortString(filesizeRightView);

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

    //show only those view filter buttons that really need to be displayed
    bool leftOnly, rightOnly, leftNewer, rightNewer, different, equal;
    leftOnly = rightOnly = leftNewer = rightNewer = different = equal = false;

    unsigned int currentRow = 0;
    for (FileCompareResult::const_iterator i = fileCmpResult.begin(); i != fileCmpResult.end(); ++i, ++currentRow)
    {
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

        //hide filtered row, if corresponding option is set
        if (cfg.hideFiltered && !i->selectedForSynchronization)
            continue;

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
        m_panel12->Show();
        m_panel12->Layout();
    }
    else
        m_panel12->Hide();

    bSizer3->Layout();

    //sorting is expensive: do performance measurements before implementing here!
}
//########################################################################################################


CompareStatusUpdater::CompareStatusUpdater(MainDialog* dlg) :
        mainDialog(dlg),
        continueOnError(false),
        currentProcess(-1)
{
    //prevent user input during "compare", do not disable maindialog since abort-button would also be disabled
    //it's not nice, but works - even has the advantage that certain actions are still possible: exit, about..
    mainDialog->m_radioBtnSizeDate->Disable();
    mainDialog->m_radioBtnContent->Disable();
    mainDialog->m_bpButtonFilter->Disable();
    mainDialog->m_hyperlinkCfgFilter->Disable();
    mainDialog->m_checkBoxHideFilt->Disable();
    mainDialog->m_bpButtonSync->Disable();
    mainDialog->m_dirPicker1->Disable();
    mainDialog->m_dirPicker2->Disable();
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
    mainDialog->m_bpButton201->Disable();
    mainDialog->m_choiceLoad->Disable();
    mainDialog->m_bpButton10->Disable();
    mainDialog->m_bpButton14->Disable();
    mainDialog->m_menubar1->EnableTop(0, false);
    mainDialog->m_menubar1->EnableTop(1, false);
    mainDialog->m_menubar1->EnableTop(2, false);

    //display status panel during compare
    statusPanel = new CompareStatus(mainDialog);
    mainDialog->bSizer1->Insert(1, statusPanel, 0, wxEXPAND | wxALL, 5 );


    //show abort button
    mainDialog->m_buttonAbort->Enable();
    mainDialog->m_buttonAbort->Show();
    mainDialog->m_bpButtonCompare->Disable();
    mainDialog->m_bpButtonCompare->Hide();
    mainDialog->m_buttonAbort->SetFocus();

    //updateUI_Now();
    mainDialog->bSizer1->Layout();  //both sizers need to recalculate!
    mainDialog->bSizer6->Layout();
}


CompareStatusUpdater::~CompareStatusUpdater()
{
    //reenable complete main dialog
    mainDialog->m_radioBtnSizeDate->Enable();
    mainDialog->m_radioBtnContent->Enable();
    mainDialog->m_bpButtonFilter->Enable();
    mainDialog->m_hyperlinkCfgFilter->Enable();
    mainDialog->m_checkBoxHideFilt->Enable();
    mainDialog->m_bpButtonSync->Enable();
    mainDialog->m_dirPicker1->Enable();
    mainDialog->m_dirPicker2->Enable();
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
    mainDialog->m_bpButton201->Enable();
    mainDialog->m_choiceLoad->Enable();
    mainDialog->m_bpButton10->Enable();
    mainDialog->m_bpButton14->Enable();
    mainDialog->m_menubar1->EnableTop(0, true);
    mainDialog->m_menubar1->EnableTop(1, true);
    mainDialog->m_menubar1->EnableTop(2, true);

    if (abortionRequested)
        mainDialog->pushStatusInformation(_("Operation aborted!"));

    mainDialog->m_buttonAbort->Disable();
    mainDialog->m_buttonAbort->Hide();
    mainDialog->m_bpButtonCompare->Enable();
    mainDialog->m_bpButtonCompare->Show();

    //remove status panel from main window
    mainDialog->bSizer1->Detach(statusPanel);
    statusPanel->Destroy();
    updateUI_Now();
    mainDialog->Layout();
    mainDialog->Refresh();
}


inline
void CompareStatusUpdater::updateStatusText(const wxString& text)
{
    statusPanel->setStatusText_NoUpdate(text);
}


void CompareStatusUpdater::initNewProcess(int objectsTotal, double dataTotal, int processID)
{
    currentProcess = processID;

    if (currentProcess == FreeFileSync::scanningFilesProcess)
        ;
    else if (currentProcess == FreeFileSync::compareFileContentProcess)
        statusPanel->resetCmpGauge(objectsTotal, dataTotal);
    else assert(false);
}


inline
void CompareStatusUpdater::updateProcessedData(int objectsProcessed, double dataProcessed)
{
    if (currentProcess == FreeFileSync::scanningFilesProcess)
        statusPanel->incScannedFiles_NoUpdate(objectsProcessed);
    else if (currentProcess == FreeFileSync::compareFileContentProcess)
        statusPanel->incProcessedCmpData_NoUpdate(objectsProcessed, dataProcessed);
    else assert(false);
}


int CompareStatusUpdater::reportError(const wxString& text)
{
    if (continueOnError)
        return StatusUpdater::continueNext;

    statusPanel->updateStatusPanelNow();

    wxString errorMessage = text + _("\n\nContinue with next object, retry or abort comparison?");

    ErrorDlg* errorDlg = new ErrorDlg(errorMessage, continueOnError);

    int rv = errorDlg->ShowModal();
    errorDlg->Destroy();

    switch (rv)
    {
    case ErrorDlg::continueButtonPressed:
        return StatusUpdater::continueNext;
    case ErrorDlg::retryButtonPressed:
        return StatusUpdater::retry;
    case ErrorDlg::abortButtonPressed:
    {
        abortionRequested = true;
        throw AbortThisProcess();
    }
    default:
        assert (false);
    }

    return StatusUpdater::continueNext; //dummy return value
}


inline
void CompareStatusUpdater::triggerUI_Refresh()
{
    if (abortionRequested) throw AbortThisProcess();    //abort can be triggered by syncStatusFrame

    if (updateUI_IsAllowed()) //test if specific time span between ui updates is over
        statusPanel->updateStatusPanelNow();
}
//########################################################################################################


SyncStatusUpdater::SyncStatusUpdater(wxWindow* dlg, bool continueOnError) :
        continueError(continueOnError)
{
    syncStatusFrame = new SyncStatus(this, dlg);
    syncStatusFrame->Show();
    updateUI_Now();
}


SyncStatusUpdater::~SyncStatusUpdater()
{
    //print the results list
    unsigned int failedItems = unhandledErrors.GetCount();
    wxString result;
    if (failedItems)
    {
        result = wxString(_("Warning: Synchronization failed for ")) + numberToWxString(failedItems) + _(" item(s):\n\n");
        for (unsigned int j = 0; j < failedItems; ++j)
            result+= unhandledErrors[j] + wxT("\n");
        result+= wxT("\n");
    }

    //notify to syncStatusFrame that current process has ended
    if (abortionRequested)
    {
        result+= wxString(_("Synchronization aborted!")) + _(" You may try to synchronize remaining items again (WITHOUT having to re-compare)!");
        syncStatusFrame->setStatusText_NoUpdate(result);
        syncStatusFrame->processHasFinished(SyncStatus::ABORTED);  //enable okay and close events
    }
    else  if (failedItems)
    {
        result+= wxString(_("Synchronization completed with errors!")) + _(" You may try to synchronize remaining items again (WITHOUT having to re-compare)!");
        syncStatusFrame->setStatusText_NoUpdate(result);
        syncStatusFrame->processHasFinished(SyncStatus::FINISHED_WITH_ERROR);
    }
    else
    {
        result+= _("Synchronization completed successfully.");
        syncStatusFrame->setStatusText_NoUpdate(result);
        syncStatusFrame->processHasFinished(SyncStatus::FINISHED_WITH_SUCCESS);
    }
}


inline
void SyncStatusUpdater::updateStatusText(const wxString& text)
{
    syncStatusFrame->setStatusText_NoUpdate(text);
}


void SyncStatusUpdater::initNewProcess(int objectsTotal, double dataTotal, int processID)
{
    assert (processID == FreeFileSync::synchronizeFilesProcess);

    syncStatusFrame->resetGauge(objectsTotal, dataTotal);
    syncStatusFrame->setCurrentStatus(SyncStatus::SYNCHRONIZING);
}


inline
void SyncStatusUpdater::updateProcessedData(int objectsProcessed, double dataProcessed)
{
    syncStatusFrame->incProgressIndicator_NoUpdate(objectsProcessed, dataProcessed);
}


int SyncStatusUpdater::reportError(const wxString& text)
{
    if (continueError)
    {
        unhandledErrors.Add(text);
        return StatusUpdater::continueNext;
    }

    wxString errorMessage = text + _("\n\nContinue with next object, retry or abort synchronization?");
    ErrorDlg* errorDlg = new ErrorDlg(errorMessage, continueError);

    syncStatusFrame->updateStatusDialogNow();
    int rv = errorDlg->ShowModal();
    errorDlg->Destroy();    //dialog is not connected to any window => needs to be deleted manually

    switch (rv)
    {
    case ErrorDlg::continueButtonPressed:
        unhandledErrors.Add(text);
        return StatusUpdater::continueNext;
    case ErrorDlg::retryButtonPressed:
        return StatusUpdater::retry;
    case ErrorDlg::abortButtonPressed:
    {
        unhandledErrors.Add(text);
        abortionRequested = true;
        throw AbortThisProcess();
    }
    default:
        assert (false);
        return StatusUpdater::continueNext;
    }
}


void SyncStatusUpdater::triggerUI_Refresh()
{
    if (abortionRequested)
        throw AbortThisProcess();  //abort can be triggered by syncStatusFrame

    if (updateUI_IsAllowed())  //test if specific time span between ui updates is over
        syncStatusFrame->updateStatusDialogNow();
}

//########################################################################################################


//menu events
void MainDialog::OnMenuExportFileList(wxCommandEvent& event)
{
    //get a filename
    wxString fileName = wxT("FileList.csv"); //proposal
    wxFileDialog* filePicker = new wxFileDialog(this, wxEmptyString, wxEmptyString, fileName, wxString(_("Comma separated list")) + wxT(" (*.csv)|*.csv"), wxFD_SAVE);

    if (filePicker->ShowModal() == wxID_OK)
    {
        fileName = filePicker->GetPath();
        if (wxFileExists(fileName))
        {
            wxMessageDialog* messageDlg = new wxMessageDialog(this, wxString(wxT("\"")) + fileName + wxT("\"") + _(" already exists. Overwrite?"), _("Warning") , wxOK | wxCANCEL);

            if (messageDlg->ShowModal() != wxID_OK)
            {
                pushStatusInformation(_("Saved aborted!"));
                event.Skip();
                return;
            }
        }

        //begin work
        wxString exportString;
        for (unsigned int i = 0; i < gridRefUI.size(); ++i)
        {
            for (int k = 0; k < m_grid1->GetNumberCols(); ++k)
            {
                exportString+= m_grid1->GetCellValue(i, k);
                exportString+= '\t';
            }

            for (int k = 0; k < m_grid3->GetNumberCols(); ++k)
            {
                exportString+= m_grid3->GetCellValue(i, k);
                exportString+= '\t';
            }

            for (int k = 0; k < m_grid2->GetNumberCols(); ++k)
            {
                exportString+= m_grid2->GetCellValue(i, k);
                if (k != m_grid2->GetNumberCols() - 1)
                    exportString+= '\t';
            }
            exportString+= '\n';
        }

        //write export file
        wxFile output(fileName, wxFile::write);

        if (output.IsOpened())
        {
            output.Write(exportString);
            pushStatusInformation(_("File list exported!"));
        }
        else
        {
            wxMessageBox(wxString(_("Could not write to ")) + wxT("\"") + fileName + wxT("\""), _("An exception occured!"), wxOK | wxICON_ERROR);
        }
    }

    event.Skip();
}


void MainDialog::OnMenuBatchJob(wxCommandEvent& event)
{
    BatchDialog* batchDlg = new BatchDialog(this, cfg, m_directoryPanel1->GetValue(), m_directoryPanel2->GetValue());
    if (batchDlg->ShowModal() == BatchDialog::batchFileCreated)
        pushStatusInformation(_("Batch file created successfully!"));

    event.Skip();
}


void MainDialog::OnMenuAbout(wxCommandEvent& event)
{
    AboutDlg* aboutDlg = new AboutDlg(this);
    aboutDlg->ShowModal();
    event.Skip();
}


void MainDialog::OnMenuQuit(wxCommandEvent& event)
{
    Destroy();
    event.Skip();
}


void MainDialog::OnMenuLangEnglish(wxCommandEvent& event)
{
    programLanguage->loadLanguageFile(wxLANGUAGE_ENGLISH); //language is a global attribute
    restartOnExit = true;
    Destroy();
    event.Skip();
}


void MainDialog::OnMenuLangGerman(wxCommandEvent& event)
{
    programLanguage->loadLanguageFile(wxLANGUAGE_GERMAN); //language is a global attribute
    restartOnExit = true;
    Destroy();
    event.Skip();
}

