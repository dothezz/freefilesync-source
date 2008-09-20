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
#include "../library/customGrid.h"

using namespace globalFunctions;

int leadingPanel = 0;

MainDialog::MainDialog(wxFrame* frame, const wxString& cfgFileName) :
        GuiGenerated(frame),
        parent(frame),
        stackObjects(0),
        filteringInitialized(false),
        filteringPending(false),
        cmpStatusUpdaterTmp(0)
{
    m_bpButtonCompare->SetLabel(_("&Compare"));
    m_bpButtonSync->SetLabel(_("&Synchronize"));
    m_bpButtonFilter->SetLabel(_("&Filter"));

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
    m_bpButton11->SetBitmapLabel(*GlobalResources::bitmapAbout);
    m_bpButton10->SetBitmapLabel(*GlobalResources::bitmapExit);
    m_bpButtonCompare->SetBitmapLabel(*GlobalResources::bitmapCompare);
    m_bpButtonSync->SetBitmapLabel(*GlobalResources::bitmapSync);
    m_bpButtonSync->SetBitmapDisabled(*GlobalResources::bitmapSyncDisabled);
    m_bpButtonSwap->SetBitmapLabel(*GlobalResources::bitmapSwap);
    m_bpButton14->SetBitmapLabel(*GlobalResources::bitmapHelp);
    m_bpButton201->SetBitmapLabel(*GlobalResources::bitmapSave);

    //prepare drag & drop
    m_panel1->SetDropTarget(new FileDropEvent(this, 1));
    m_panel2->SetDropTarget(new FileDropEvent(this, 2));

    //create a right-click context menu
    contextMenu = new wxMenu;
    contextMenu->Append(contextManualFilter, _("Filter manually"));
    contextMenu->Append(contextCopyClipboard, _("Copy to clipboard\tCTRL+C"));
#ifdef FFS_WIN
    contextMenu->Append(contextOpenExplorer, _("Open with Explorer\tD-Click"));
#endif
    contextMenu->AppendSeparator();
    contextMenu->Append(contextDeleteFiles, _("Delete files\tDEL"));

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
    m_grid1->setGridDataTable(&currentUI_View);
    m_grid2->setGridDataTable(&currentUI_View);
    m_grid3->setGridDataTable(&currentUI_View);

    //disable sync button as long as "compare" hasn't been triggered.
    m_bpButtonSync->Disable();

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
    cfgFileHistory = new wxConfig("FreeFileSync");
    for (int i = CfgHistroyLength - 1; i >= 0; --i) //put files in reverse order to history
    {
        const wxString key = "Selection" + numberToWxString(i);

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
}


MainDialog::~MainDialog()
{
    m_grid1->setGridDataTable(0);
    m_grid2->setGridDataTable(0);
    m_grid3->setGridDataTable(0);

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
        const wxString key = "Selection" + numberToWxString(i);

        if (i < vectorSize)
            cfgFileHistory->Write(key, cfgFileNames[i]);
        else
        {
            if (cfgFileHistory->Exists(key))
                cfgFileHistory->DeleteEntry(key);
        }
    }
    delete cfgFileHistory;

    writeConfigurationToHD(FreeFileSync::FFS_LastConfigFile);   //don't trow exceptions in destructors
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


void MainDialog::filterRangeManual(const set<int>& rowsToFilterOnUI_View)
{
    if (rowsToFilterOnUI_View.size() > 0)
    {
        int currentUI_Size = currentUI_View.size();

        bool newSelection = false;   //default: deselect range

        //leadingRow determines de-/selection of all other rows
        int leadingRow = *rowsToFilterOnUI_View.begin();
        if (0 <= leadingRow && leadingRow < currentUI_Size)
            newSelection = !currentGridData[currentUI_View[leadingRow].linkToCurrentGridData].selectedForSynchronization;

        if (hideFiltered)
            assert (!newSelection); //if hidefiltered is active, there should be no filtered elements on screen => current element was filtered out


        //get all lines that need to be filtered (e.g. if a folder is marked, then its subelements should be marked as well)
        set<int> rowsToFilterOnGridData;    //rows to filter in backend


        for (set<int>::iterator i = rowsToFilterOnUI_View.begin(); i != rowsToFilterOnUI_View.end(); ++i)
        {
            if (0 <= *i && *i < currentUI_Size)
            {
                unsigned int gridIndex = currentUI_View[*i].linkToCurrentGridData;

                rowsToFilterOnGridData.insert(gridIndex);
                FreeFileSync::addSubElements(rowsToFilterOnGridData, currentGridData, currentGridData[gridIndex]);
            }
        }

        //toggle selection of filtered rows
        for (set<int>::iterator i = rowsToFilterOnGridData.begin(); i != rowsToFilterOnGridData.end(); ++i)
            currentGridData[*i].selectedForSynchronization = newSelection;


        set<int> filteredOutRowsOnUI; //determine rows that are currently filtered out on current UI view

        //update currentUI_View
        for (UI_Grid::iterator i = currentUI_View.begin(); i != currentUI_View.end(); ++i)
        {
            const FileCompareLine& gridLine = currentGridData[i->linkToCurrentGridData];

            i->cmpResult = evaluateCmpResult(gridLine.cmpResult, gridLine.selectedForSynchronization);

            if (!gridLine.selectedForSynchronization)
                filteredOutRowsOnUI.insert(i - currentUI_View.begin());
        }

        //signal UI that grids need to be refreshed on next Update()
        m_grid1->ForceRefresh();
        m_grid2->ForceRefresh();
        m_grid3->ForceRefresh();

        if (hideFiltered)
        {
            //some delay to show user the rows he has filtered out before they are removed
            Update();   //show changes resulting from ForceRefresh()
            wxMilliSleep(400);

            //delete rows, that are filtered out:
            removeRowsFromVector(currentUI_View, filteredOutRowsOnUI);

            //redraw grid necessary to update new dimensions
            writeGrid(currentGridData, true);        //use UI buffer, no mapping from currentGridData to UI model again, just a re-dimensioning of grids
            updateStatusInformation(currentUI_View); //status information has to be recalculated!
        }
    }
    //clear selection on grids
    if (hideFiltered)
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
    if (stackObjects > 0 )  //check if there is some work to do
    {
        wxLongLong currentTime = wxGetLocalTimeMillis();
        if (currentTime - lastStatusChange > 3000) //restore stackObject after three seconds
        {
            lastStatusChange = currentTime;

            stackObjects--;
            m_statusBar1->PopStatusText(1);
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

    removeInvalidRows(output, currentUI_View.size());

    return output;
}


class DeleteStatusUpdater : public StatusUpdater
{
public:
    DeleteStatusUpdater(bool& unsolvedErrorOccured) : suppressUI_Errormessages(false), unsolvedErrors(unsolvedErrorOccured) {}
    ~DeleteStatusUpdater() {}

    int reportError(const wxString& text)
    {
        if (suppressUI_Errormessages)
        {
            unsolvedErrors = true;
            return StatusUpdater::continueNext;
        }

        wxString errorMessage = text + _("\n\nInformation: If you skip the error and continue or abort a re-compare will be necessary!");

        ErrorDlg* errorDlg = new ErrorDlg(errorMessage, suppressUI_Errormessages);

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
    bool suppressUI_Errormessages;
    bool& unsolvedErrors;
};


void MainDialog::deleteFilesOnGrid(const set<int>& rowsToDeleteOnUI)
{
    if (rowsToDeleteOnUI.size())
    {
        //map grid lines from UI to grid lines in backend
        set<int> rowsToDeleteOnGrid;
        for (set<int>::iterator i = rowsToDeleteOnUI.begin(); i != rowsToDeleteOnUI.end(); ++i)
            rowsToDeleteOnGrid.insert(currentUI_View[*i].linkToCurrentGridData);

        wxString headerText;
        wxString filesToDelete;

        if (useRecycleBin)
            headerText = _("Do you really want to move the following objects(s) to the recycle bin?");
        else
            headerText = _("Do you really want to delete the following objects(s)?");

        for (set<int>::iterator i = rowsToDeleteOnGrid.begin(); i != rowsToDeleteOnGrid.end(); ++i)
        {
            const FileCompareLine& currentCmpLine = currentGridData[*i];

            if (currentCmpLine.fileDescrLeft.objType != isNothing)
                filesToDelete+= currentCmpLine.fileDescrLeft.filename + "\n";

            if (currentCmpLine.fileDescrRight.objType != isNothing)
                filesToDelete+= currentCmpLine.fileDescrRight.filename + "\n";

            filesToDelete+= "\n";
        }

        DeleteDialog* confirmDeletion = new DeleteDialog(headerText, filesToDelete, this);    //no destruction needed; attached to main window

        switch (confirmDeletion->ShowModal())
        {
        case DeleteDialog::okayButtonPressed:
        {
            bool unsolvedErrorOccured = false; //if an error is skipped a re-compare will be necessary!

            try
            {   //class errors when deleting files/folders
                DeleteStatusUpdater deleteStatusUpdater(unsolvedErrorOccured);

                FreeFileSync::deleteOnGridAndHD(currentGridData, rowsToDeleteOnGrid, &deleteStatusUpdater, useRecycleBin);
            }
            catch (AbortThisProcess& theException)
                {}


            //disable the sync button if errors occured during deletion
            if (unsolvedErrorOccured)
                m_bpButtonSync->Disable();


            //redraw grid neccessary to update new dimensions and for UI-Backend data linkage
            writeGrid(currentGridData);     //do NOT use UI buffer here

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
        wxString command = "explorer " + FreeFileSync::getFormattedDirectoryName(m_directoryPanel1->GetValue()); //default

        if (0 <= rowNumber && rowNumber < int(currentUI_View.size()))
        {
            wxString filename = currentGridData[currentUI_View[rowNumber].linkToCurrentGridData].fileDescrLeft.filename;

            if (!filename.IsEmpty())
                command = "explorer /select," + filename;
        }
        wxExecute(command);
    }
    else if (gridNr == 2)
    {
        wxString command = "explorer " + FreeFileSync::getFormattedDirectoryName(m_directoryPanel2->GetValue()); //default

        if (0 <= rowNumber && rowNumber < int(currentUI_View.size()))
        {
            wxString filename = currentGridData[currentUI_View[rowNumber].linkToCurrentGridData].fileDescrRight.filename;

            if (!filename.IsEmpty())
                command = "explorer /select," + filename;
        }
        wxExecute(command);
    }
#endif  // FFS_WIN
}


void MainDialog::pushStatusInformation(const wxString& text)
{
    lastStatusChange = wxGetLocalTimeMillis();
    ++stackObjects;
    m_statusBar1->PushStatusText(text, 1);
}


void MainDialog::writeStatusInformation(const wxString& text)
{
    stackObjects = 0;
    m_statusBar1->SetStatusText(text, 1);
}


void MainDialog::onResizeMainWindow(wxEvent& event)
{
    if (!IsMaximized())
    {
        int width = 0;
        int height = 0;
        int x = 0;
        int y = 0;

        GetSize(&width, &height);
        GetPosition(&x, &y);

        if (width > 0 && height > 0)
        {
            widthNotMaximized  = width;
            heightNotMaximized = height;
        }

        if (x >= 0 && y >= 0)   //might be < 0 under some strange circumstances
        {
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
        contextMenu->Enable(contextManualFilter, true);
        contextMenu->Enable(contextCopyClipboard, true);
        contextMenu->Enable(contextDeleteFiles, true);
    }
    else
    {
        contextMenu->Enable(contextManualFilter, false);
        contextMenu->Enable(contextCopyClipboard, false);
        contextMenu->Enable(contextDeleteFiles, false);
    }

#ifdef FFS_WIN
    if ((leadingPanel == 1 || leadingPanel == 2) && selection.size() <= 1)
        contextMenu->Enable(contextOpenExplorer, true);
    else
        contextMenu->Enable(contextOpenExplorer, false);
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
    case contextManualFilter:
        filterRangeManual(getSelectedRows());
        break;

    case contextCopyClipboard:
        copySelectionToClipboard(getSelectedRows(), leadingPanel);
        break;

    case contextOpenExplorer:
        selection = getSelectedRows();

        if (leadingPanel == 1 || leadingPanel == 2)
        {
            if (selection.size() == 1)
                openWithFileBrowser(*selection.begin(), leadingPanel);
            else if (selection.size() == 0)
                openWithFileBrowser(-1, leadingPanel);
        }
        break;

    case contextDeleteFiles:
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
    m_bpButtonSync->Enable(false);

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
    m_bpButtonSync->Enable(false);
    //clear grids
    currentGridData.clear();
    writeGrid(currentGridData);

    event.Skip();
}


void MainDialog::clearStatusBar()
{
    stackObjects = 0;   //prevent old stack objects from popping up

    m_statusBar1->SetStatusText("");
    m_statusBar1->SetStatusText("", 1);
    m_statusBar1->SetStatusText("", 2);
}


wxString getFormattedHistoryElement(const wxString& filename)
{
    wxString output = wxFileName(filename).GetFullName();
    if (output.EndsWith(".FFS"))
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
    if (sameFileSpecified(FreeFileSync::FFS_LastConfigFile, filename))
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
        mainDlg->m_bpButtonSync->Disable();

        //clear grids
        mainDlg->currentGridData.clear();
        mainDlg->writeGrid(mainDlg->currentGridData);

        mainDlg->clearStatusBar();

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
    wxString defaultFileName = "SyncSettings.FFS";

    //try to use last selected configuration file as default
    int selectedItem;
    if ((selectedItem = m_choiceLoad->GetSelection()) != wxNOT_FOUND)
        if (1 <= selectedItem && unsigned(selectedItem) < m_choiceLoad->GetCount())
            if (unsigned(selectedItem - 1) < cfgFileNames.size())
                defaultFileName = cfgFileNames[selectedItem - 1];


    clearStatusBar();

    wxFileDialog* filePicker = new wxFileDialog(this, "", "", defaultFileName, wxString(_("FreeFileSync configuration")) + " (*.FFS)|*.FFS", wxFD_SAVE);

    if (filePicker->ShowModal() == wxID_OK)
    {
        wxString newFileName = filePicker->GetFilename();

        if (wxFileExists(newFileName))
        {
            wxMessageDialog* messageDlg = new wxMessageDialog(this, wxString("\"") + newFileName + "\"" + _(" already exists. Overwrite?"), _("Warning") , wxOK | wxCANCEL);

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
        wxString newCfgFile;

        clearStatusBar();

        switch (selectedItem)
        {
        case 0: //load config from file
            wxFileDialog* filePicker = new wxFileDialog(this, "", "", "", wxString(_("FreeFileSync configuration")) + " (*.FFS)|*.FFS", wxFD_OPEN);

            if (filePicker->ShowModal() == wxID_OK)
                newCfgFile = filePicker->GetFilename();

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


void MainDialog::OnChangeCompareVariant(wxCommandEvent& event)
{
    //disable the sync button
    m_bpButtonSync->Enable(false);
    //clear grids
    currentGridData.clear();
    writeGrid(currentGridData);
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
    syncConfiguration.exLeftSideOnly  = syncDirRight;
    syncConfiguration.exRightSideOnly = syncDirRight;
    syncConfiguration.leftNewer       = syncDirRight;
    syncConfiguration.rightNewer      = syncDirRight;
    syncConfiguration.different       = syncDirRight;

    m_radioBtnSizeDate->SetValue(true); //compare algorithm

    includeFilter = "*";    //include all files/folders
    excludeFilter = "";     //exlude nothing

    //set status of filter button
    filterIsActive = false; //do not filter by default
    updateFilterButton();

    //set status of "hide filtered items" checkbox
    hideFiltered  = false;  //show filtered items
    m_checkBoxHideFilt->SetValue(hideFiltered);

    useRecycleBin = false;  //do not use: in case OS doesn't support this, user will have to activate first and then get the error message
    hideErrorMessages = false;

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
            wxMessageBox(wxString(_("Could not read configuration file ")) + "\"" + filename + "\"", _("An exception occured!"), wxOK | wxICON_ERROR);

        return;
    }

    //read FFS identifier
    config.get(bigBuffer, FreeFileSync::FFS_ConfigFileID.Len() + 1);

    if (wxString(bigBuffer) != FreeFileSync::FFS_ConfigFileID)
    {
        wxMessageBox(_("The selected file does not contain a valid configuration!"), _("Warning"), wxOK);
        config.close();
        return;
    }


    //put filename on list of last used config files
    addCfgFileToHistory(filename);


    //read sync configuration
    syncConfiguration.exLeftSideOnly  = SyncDirection(config.get());
    syncConfiguration.exRightSideOnly = SyncDirection(config.get());
    syncConfiguration.leftNewer       = SyncDirection(config.get());
    syncConfiguration.rightNewer      = SyncDirection(config.get());
    syncConfiguration.different       = SyncDirection(config.get());

    //read compare algorithm
    switch (int(config.get()))
    {
    case compareByTimeAndSize:
        m_radioBtnSizeDate->SetValue(true);
        break;
    case compareByContent:
        m_radioBtnContent->SetValue(true);
        break;
    default:
        assert (false);
    }

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
    hideFiltered = bool(config.get());
    m_checkBoxHideFilt->SetValue(hideFiltered);

    filterIsActive = bool(config.get());
    updateFilterButton();

    //include
    config.getline(bigBuffer, 10000, char(0));
    includeFilter = bigBuffer;

    //exclude
    config.getline(bigBuffer, 10000, char(0));
    excludeFilter = bigBuffer;

    useRecycleBin =  bool(config.get());

    hideErrorMessages =  bool(config.get());

    config.close();
}


void MainDialog::writeConfigurationToHD(const wxString& filename)
{
    ofstream config(filename.c_str());
    if (!config)
    {
        wxMessageBox(wxString(_("Could not write to ")) + "\"" + filename + "\"", _("An exception occured!"), wxOK | wxICON_ERROR);
        return;
    }

    //put filename on list of last used config files
    addCfgFileToHistory(filename);

    //write FFS identifier
    config<<FreeFileSync::FFS_ConfigFileID.c_str();

    //write sync configuration
    config<<char(syncConfiguration.exLeftSideOnly)
    <<char(syncConfiguration.exRightSideOnly)
    <<char(syncConfiguration.leftNewer)
    <<char(syncConfiguration.rightNewer)
    <<char(syncConfiguration.different);

    //write find method
    if (m_radioBtnSizeDate->GetValue())
        config<<char(compareByTimeAndSize);
    else if (m_radioBtnContent->GetValue())
        config<<char(compareByContent);
    else assert (false);


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
    config<<char(hideFiltered);
    config<<char(filterIsActive);

    config<<includeFilter.c_str()<<char(0)
    <<excludeFilter.c_str()<<char(0);

    config<<char(useRecycleBin);

    config<<char(hideErrorMessages);

    config.close();
}


void MainDialog::OnAbout(wxCommandEvent &event)
{
    AboutDlg* aboutDlg = new AboutDlg(this);
    aboutDlg->ShowModal();
}


void MainDialog::OnShowHelpDialog(wxCommandEvent &event)
{
    HelpDlg* helpDlg = new HelpDlg(this);
    helpDlg->ShowModal();
}


void MainDialog::OnFilterButton(wxCommandEvent &event)
{   //toggle filter on/off
    filterIsActive = !filterIsActive;

    //make sure, button-appearance and "filterIsActive" are in sync.
    updateFilterButton();

    if (filterIsActive)
        FreeFileSync::filterCurrentGridData(currentGridData, includeFilter, excludeFilter);
    else
        FreeFileSync::removeFilterOnCurrentGridData(currentGridData);

    writeGrid(currentGridData);
}


void MainDialog::OnHideFilteredButton(wxCommandEvent &event)
{   //toggle showing filtered rows
    hideFiltered = !hideFiltered;

    writeGrid(currentGridData);

    //make sure, checkbox and "hideFiltered" are in sync
    m_checkBoxHideFilt->SetValue(hideFiltered);
}


void MainDialog::OnConfigureFilter(wxHyperlinkEvent &event)
{
    wxString beforeImage = includeFilter + wxChar(0) + excludeFilter;

    FilterDlg* filterDlg = new FilterDlg(this, includeFilter, excludeFilter);
    if (filterDlg->ShowModal() == FilterDlg::okayButtonPressed)
    {
        wxString afterImage = includeFilter + wxChar(0) + excludeFilter;

        if (beforeImage != afterImage)  //if filter settings are changed: set filtering to "on"
        {
            filterIsActive = true;
            updateFilterButton();

            FreeFileSync::filterCurrentGridData(currentGridData, includeFilter, excludeFilter);

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
        m_bpButton20->SetBitmapLabel(*GlobalResources::bitmapLeftOnly);
    else
        m_bpButton20->SetBitmapLabel(*GlobalResources::bitmapLeftOnlyDeact);

    if (leftNewerFilesActive)
        m_bpButton21->SetBitmapLabel(*GlobalResources::bitmapLeftNewer);
    else
        m_bpButton21->SetBitmapLabel(*GlobalResources::bitmapLeftNewerDeact);

    if (differentFilesActive)
        m_bpButton22->SetBitmapLabel(*GlobalResources::bitmapDifferent);
    else
        m_bpButton22->SetBitmapLabel(*GlobalResources::bitmapDifferentDeact);

    if (rightNewerFilesActive)
        m_bpButton23->SetBitmapLabel(*GlobalResources::bitmapRightNewer);
    else
        m_bpButton23->SetBitmapLabel(*GlobalResources::bitmapRightNewerDeact);

    if (rightOnlyFilesActive)
        m_bpButton24->SetBitmapLabel(*GlobalResources::bitmapRightOnly);
    else
        m_bpButton24->SetBitmapLabel(*GlobalResources::bitmapRightOnlyDeact);

    if (equalFilesActive)
        m_bpButton25->SetBitmapLabel(*GlobalResources::bitmapEqual);
    else
        m_bpButton25->SetBitmapLabel(*GlobalResources::bitmapEqualDeact);
}


void MainDialog::updateFilterButton()
{
    if (filterIsActive)
    {
        m_bpButtonFilter->SetBitmapLabel(*GlobalResources::bitmapFilterOn);
        m_bpButtonFilter->SetToolTip(_("Filter active: Press again to deactivate"));
    }
    else
    {
        m_bpButtonFilter->SetBitmapLabel(*GlobalResources::bitmapFilterOff);
        m_bpButtonFilter->SetToolTip(_("Press button to activate filter"));
    }
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

    CompareVariant cmpVar = compareByTimeAndSize;   //assign a value to suppress compiler warning
    if (m_radioBtnSizeDate->GetValue())
        cmpVar = compareByTimeAndSize;
    else if (m_radioBtnContent->GetValue())
        cmpVar = compareByContent;
    else assert (false);

    try
    {   //class handling status display and error messages
        CompareStatusUpdater statusUpdater(this);
        cmpStatusUpdaterTmp = &statusUpdater;
        stackObjects = 0;

        //unsigned int startTime3 = GetTickCount();
        FreeFileSync::startCompareProcess(currentGridData,
                                          FreeFileSync::getFormattedDirectoryName(m_directoryPanel1->GetValue()),
                                          FreeFileSync::getFormattedDirectoryName(m_directoryPanel2->GetValue()),
                                          cmpVar,
                                          &statusUpdater);
        //wxMessageBox(numberToWxString(unsigned(GetTickCount()) - startTime3));


        //filter currentGridData if option is set
        if (filterIsActive)
            FreeFileSync::filterCurrentGridData(currentGridData, includeFilter, excludeFilter);

        //once compare is finished enable the sync button
        m_bpButtonSync->Enable();
        m_bpButtonSync->SetFocus();

        writeGrid(currentGridData); //keep it in try/catch to not overwrite status information if compare is abortet

        cmpStatusUpdaterTmp = 0;
    }
    catch (AbortThisProcess& theException)
    {
        //disable the sync button
        m_bpButtonSync->Disable();
    }

    wxEndBusyCursor();
}


void MainDialog::OnAbortCompare(wxCommandEvent& event)
{
    if (cmpStatusUpdaterTmp)
        cmpStatusUpdaterTmp->requestAbortion();
}


inline
wxString MainDialog::evaluateCmpResult(const CompareFilesResult result, const bool selectedForSynchronization)
{
    if (selectedForSynchronization)
        switch (result)
        {
        case fileOnLeftSideOnly:
            return "<|";
            break;
        case fileOnRightSideOnly:
            return "|>";
            break;
        case rightFileNewer:
            return ">>";
            break;
        case leftFileNewer:
            return "<<";
            break;
        case filesDifferent:
            return "!=";
            break;
        case filesEqual:
            return "==";
            break;
        default:
            assert (false);
        }
    else return constFilteredOut;
}


void MainDialog::writeGrid(const FileCompareResult& gridData, bool useUI_GridCache)
{
    if (!useUI_GridCache)
    {
        //unsigned int startTime = GetTickCount();
        mapFileModelToUI(currentUI_View, gridData); //update currentUI_View
        //wxMessageBox(wxString("Benchmark: ") + numberToWxString(unsigned(GetTickCount()) - startTime) + " ms");
        updateStatusInformation(currentUI_View);    //write status information for currentUI_View
    }

    m_grid1->BeginBatch();
    m_grid2->BeginBatch();
    m_grid3->BeginBatch();

    //all three grids retrieve their data directly from currentUI_View via a pointer!!!
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


void MainDialog::OnSync( wxCommandEvent& event )
{
    //check if there are files/folders that can be synced
    bool nothingToSync = true;
    for (FileCompareResult::const_iterator i = currentGridData.begin(); i != currentGridData.end(); ++i)
        if (i->cmpResult != filesEqual)
        {
            nothingToSync = false;
            break;
        }

    if (nothingToSync)
    {
        wxMessageBox(_("Nothing to synchronize. Both directories seem to contain the same data!"), _("Information"), wxICON_WARNING);
        return;
    }

    SyncDialog* syncDlg = new SyncDialog(this);
    if (syncDlg->ShowModal() == SyncDialog::StartSynchronizationProcess)
    {
        wxBeginBusyCursor();

        clearStatusBar();

        try
        {
            //class handling status updates and error messages
            SyncStatusUpdater statusUpdater(this, hideErrorMessages);

            //start synchronization and return elements that were not sync'ed in currentGridData

            //unsigned int startTime3 = GetTickCount();
            FreeFileSync::startSynchronizationProcess(currentGridData, syncConfiguration, &statusUpdater, useRecycleBin);
            //wxMessageBox(numberToWxString(unsigned(GetTickCount()) - startTime3));
        }
        catch (AbortThisProcess& theException)
        {   //do NOT disable the sync button: user might want to try to sync the REMAINING rows
        }   //m_bpButtonSync->Disable();


        //display files that were not processed
        writeGrid(currentGridData);

        if (currentGridData.size() > 0)
            pushStatusInformation(_("Not all items were synchronized! Have a look at the list."));
        else
        {
            pushStatusInformation(_("All items have been synchronized!"));
            m_bpButtonSync->Disable();
        }


        wxEndBusyCursor();
    }
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
        return false;   // if a and b are empty: false, if a empty, b not empty: also false, since empty rows should be put below on grid
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


bool sortGridLeft(const UI_GridLine& a, const UI_GridLine& b)
{
    switch (currentSortColumn)
    {
    case 0:
        return cmpString(a.leftFilename, b.leftFilename);
        break;
    case 1:
        return cmpString(a.leftRelativePath, b.leftRelativePath);
        break;
    case 2:
        ObjectType typeA = (*currentGridDataPtr)[a.linkToCurrentGridData].fileDescrLeft.objType;
        ObjectType typeB = (*currentGridDataPtr)[b.linkToCurrentGridData].fileDescrLeft.objType;

        //presort types: first files, then directories then empty rows
        if (typeA == isNothing)
            return false;  //empty rows always last
        else if (typeB == isNothing)
            return true;  //empty rows always last
        else if (typeA == isDirectory)
            return false;
        else if (typeB == isDirectory)
            return true;
        else       //use unformatted filesizes and sort by size
            return  cmpLargeInt((*currentGridDataPtr)[a.linkToCurrentGridData].fileDescrLeft.fileSize, (*currentGridDataPtr)[b.linkToCurrentGridData].fileDescrLeft.fileSize);

        break;
    case 3:
        return cmpString(a.leftDate, b.leftDate);
        break;
    default:
        assert(false);
    }
    return true; //dummy command
}

bool sortGridRight(const UI_GridLine& a, const UI_GridLine& b)
{
    switch (currentSortColumn)
    {
    case 0:
        return cmpString(a.rightFilename, b.rightFilename);
        break;
    case 1:
        return  cmpString(a.rightRelativePath, b.rightRelativePath);
        break;
    case 2:
        ObjectType typeA = (*currentGridDataPtr)[a.linkToCurrentGridData].fileDescrRight.objType;
        ObjectType typeB = (*currentGridDataPtr)[b.linkToCurrentGridData].fileDescrRight.objType;

        //presort types: first files, then directories then empty rows
        if (typeA == isNothing)
            return false;  //empty rows always last
        else if (typeB == isNothing)
            return true;  //empty rows always last
        else if (typeA == isDirectory)
            return false;
        else if (typeB == isDirectory)
            return true;
        else     //use unformatted filesizes and sort by size
            return  cmpLargeInt((*currentGridDataPtr)[a.linkToCurrentGridData].fileDescrRight.fileSize, (*currentGridDataPtr)[b.linkToCurrentGridData].fileDescrRight.fileSize);

        break;
    case 3:
        return  cmpString(a.rightDate, b.rightDate);
        break;
    default:
        assert(false);
    }
    return true; //dummy command
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
        sort(currentUI_View.begin(), currentUI_View.end(), sortGridLeft);
        writeGrid(currentGridData, true);   //use UI buffer, no mapping from currentGridData to UI model again
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
        sort(currentUI_View.begin(), currentUI_View.end(), sortGridRight);
        writeGrid(currentGridData, true);
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
}


//this sorting method is currently NOT used
bool cmpGridSmallerThan(const UI_GridLine& a, const UI_GridLine& b)
{
    wxString cmpStringA;
    wxString cmpStringB;

    for (int i = 0; i < 4; ++i)
    {
        switch (i)
        {
        case 0:
            cmpStringA = a.leftRelativePath;
            cmpStringB = b.leftRelativePath;
            break;
        case 1:
            cmpStringA = a.leftFilename;
            cmpStringB = b.leftFilename;
            break;
        case 2:
            cmpStringA = a.rightRelativePath;
            cmpStringB = b.rightRelativePath;
            break;
        case 3:
            cmpStringA = a.rightFilename;
            cmpStringB = b.rightFilename;
            break;
        default:
            assert (false);
        }
        if (cmpStringA.IsEmpty())
            cmpStringA = '\255';
        if (cmpStringB.IsEmpty())
            cmpStringB = '\255';

        if (cmpStringA != cmpStringB)
            return (cmpStringA < cmpStringB);
    }
    return (false);
}


void MainDialog::updateStatusInformation(const UI_Grid& visibleGrid)
{
    clearStatusBar();

    unsigned int objectsOnLeftView = 0;
    unsigned int objectsOnRightView = 0;
    wxULongLong filesizeLeftView;
    wxULongLong filesizeRightView;

    for (UI_Grid::const_iterator i = visibleGrid.begin(); i != visibleGrid.end(); ++i)
    {
        const FileCompareLine& refLine = currentGridData[i->linkToCurrentGridData];

        //calculate total number of bytes for each sied
        if (refLine.fileDescrLeft.objType != isNothing)
        {
            filesizeLeftView+= refLine.fileDescrLeft.fileSize;
            ++objectsOnLeftView;
        }

        if (refLine.fileDescrRight.objType != isNothing)
        {
            filesizeRightView+= refLine.fileDescrRight.fileSize;
            ++objectsOnRightView;
        }
    }

    //show status information on "root" level. This cannot be accomplished in writeGrid since filesizes are already formatted for display there
    wxString objectsViewLeft = numberToWxString(objectsOnLeftView);
    globalFunctions::includeNumberSeparator(objectsViewLeft);
    if (objectsOnLeftView == 1)
        m_statusBar1->SetStatusText(wxString(_("1 item on left, ")) + FreeFileSync::formatFilesizeToShortString(filesizeLeftView), 0);
    else
        m_statusBar1->SetStatusText(objectsViewLeft + _(" items on left, ") + FreeFileSync::formatFilesizeToShortString(filesizeLeftView), 0);

    wxString objectsTotal = numberToWxString(currentGridData.size());
    globalFunctions::includeNumberSeparator(objectsTotal);
    wxString objectsView = numberToWxString(visibleGrid.size());
    globalFunctions::includeNumberSeparator(objectsView);
    if (currentGridData.size() == 1)
        m_statusBar1->SetStatusText(objectsView + _(" of ") + objectsTotal + _(" row in view"), 1);
    else
        m_statusBar1->SetStatusText(objectsView + _(" of ") + objectsTotal + _(" rows in view"), 1);


    wxString objectsViewRight = numberToWxString(objectsOnRightView);
    globalFunctions::includeNumberSeparator(objectsViewRight);
    if (objectsOnRightView == 1)
        m_statusBar1->SetStatusText(wxString(_("1 item on right, ")) + FreeFileSync::formatFilesizeToShortString(filesizeRightView), 2);
    else
        m_statusBar1->SetStatusText(objectsViewRight + _(" items on right, ") + FreeFileSync::formatFilesizeToShortString(filesizeRightView), 2);
}


void MainDialog::mapFileModelToUI(UI_Grid& output, const FileCompareResult& fileCmpResult)
{
    output.clear();

    UI_GridLine gridline;
    unsigned int currentRow = 0;
    wxString fileSize; //tmp string
    for (FileCompareResult::const_iterator i = fileCmpResult.begin(); i != fileCmpResult.end(); ++i, ++currentRow)
    {
        //process UI filter settings
        switch (i->cmpResult)
        {
        case fileOnLeftSideOnly:
            if (!leftOnlyFilesActive) continue;
            break;
        case fileOnRightSideOnly:
            if (!rightOnlyFilesActive) continue;
            break;
        case rightFileNewer:
            if (!rightNewerFilesActive) continue;
            break;
        case leftFileNewer:
            if (!leftNewerFilesActive) continue;
            break;
        case filesDifferent:
            if (!differentFilesActive) continue;
            break;
        case filesEqual:
            if (!equalFilesActive) continue;
            break;
        default:
            assert (false);
        }

        //hide filtered row, if corresponding option is set
        if (hideFiltered && !i->selectedForSynchronization)
            continue;

        if (i->fileDescrLeft.objType == isDirectory)
        {
            gridline.leftFilename      = wxEmptyString;
            gridline.leftRelativePath  = i->fileDescrLeft.relFilename;
            gridline.leftSize = _("<Directory>");
        }
        else
        {
            gridline.leftFilename      = i->fileDescrLeft.relFilename.AfterLast(GlobalResources::fileNameSeparator);
            gridline.leftRelativePath  = i->fileDescrLeft.relFilename.BeforeLast(GlobalResources::fileNameSeparator);
            if (i->fileDescrLeft.fileSize != 0)
                gridline.leftSize      = globalFunctions::includeNumberSeparator(fileSize = i->fileDescrLeft.fileSize.ToString());
            else
                gridline.leftSize = "";
        }
        gridline.leftDate              = i->fileDescrLeft.lastWriteTime;

        gridline.cmpResult             = evaluateCmpResult(i->cmpResult, i->selectedForSynchronization);
        gridline.linkToCurrentGridData = currentRow;

        if (i->fileDescrRight.objType == isDirectory)
        {
            gridline.rightFilename     = wxEmptyString;
            gridline.rightRelativePath = i->fileDescrRight.relFilename;
            gridline.rightSize = _("<Directory>");
        }
        else
        {
            gridline.rightFilename     = i->fileDescrRight.relFilename.AfterLast(GlobalResources::fileNameSeparator);
            gridline.rightRelativePath = i->fileDescrRight.relFilename.BeforeLast(GlobalResources::fileNameSeparator);
            if (i->fileDescrRight.fileSize != 0)
                gridline.rightSize     = globalFunctions::includeNumberSeparator(fileSize = i->fileDescrRight.fileSize.ToString());
            else
                gridline.rightSize     = "";
        }
        gridline.rightDate             = i->fileDescrRight.lastWriteTime;

        output.push_back(gridline);
    }

    //sorting is expensive: ca. 50% bigger runtime for large grids; unsorted doesn't look too bad, so it's disabled
    // sort(output.begin(), output.end(), cmpGridSmallerThan);
}
//########################################################################################################


void updateUI_Now()
{
    //process UI events and prevent application from "not responding"   -> NO performance issue!
    wxTheApp->Yield();

//    while (wxTheApp->Pending())
//        wxTheApp->Dispatch();
}


bool updateUI_IsAllowed()
{
    static wxLongLong lastExec = 0;

    wxLongLong newExec = wxGetLocalTimeMillis();

    if (newExec - lastExec >= uiUpdateInterval)  //perform ui updates not more often than necessary
    {
        lastExec = newExec;
        return true;
    }
    return false;
}

//########################################################################################################


CompareStatusUpdater::CompareStatusUpdater(MainDialog* dlg) :
        mainDialog(dlg),
        suppressUI_Errormessages(false),
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
    mainDialog->m_bpButton20->Disable();
    mainDialog->m_bpButton21->Disable();
    mainDialog->m_bpButton22->Disable();
    mainDialog->m_bpButton23->Disable();
    mainDialog->m_bpButton24->Disable();
    mainDialog->m_bpButton25->Disable();
    mainDialog->m_panel1->Disable();
    mainDialog->m_panel2->Disable();
    mainDialog->m_panel3->Disable();
    mainDialog->m_bpButton201->Disable();
    mainDialog->m_choiceLoad->Disable();
    mainDialog->m_bpButton10->Disable();
    mainDialog->m_bpButton14->Disable();

    //show abort button
    mainDialog->m_buttonAbort->Show();
    mainDialog->m_bpButtonCompare->Hide();
    mainDialog->m_buttonAbort->SetFocus();

    //display status panel during compare
    statusPanel = new CompareStatus(mainDialog);
    mainDialog->bSizer1->Insert(1, statusPanel, 0, wxEXPAND | wxALL, 5 );
    updateUI_Now();
    mainDialog->Layout();
    mainDialog->Refresh();
}

CompareStatusUpdater::~CompareStatusUpdater()
{
    //reenable complete main dialog
    mainDialog->m_radioBtnSizeDate->Enable();
    mainDialog->m_radioBtnContent->Enable();
    mainDialog->m_bpButtonFilter->Enable();
    mainDialog->m_hyperlinkCfgFilter->Enable();
    mainDialog->m_checkBoxHideFilt->Enable();
    //mainDialog->m_bpButtonSync->Enable();  don't enable this one, this is up to OnCompare to handle its status
    mainDialog->m_dirPicker1->Enable();
    mainDialog->m_dirPicker2->Enable();
    mainDialog->m_bpButtonSwap->Enable();
    mainDialog->m_bpButton20->Enable();
    mainDialog->m_bpButton21->Enable();
    mainDialog->m_bpButton22->Enable();
    mainDialog->m_bpButton23->Enable();
    mainDialog->m_bpButton24->Enable();
    mainDialog->m_bpButton25->Enable();
    mainDialog->m_panel1->Enable();
    mainDialog->m_panel2->Enable();
    mainDialog->m_panel3->Enable();
    mainDialog->m_bpButton201->Enable();
    mainDialog->m_choiceLoad->Enable();
    mainDialog->m_bpButton10->Enable();
    mainDialog->m_bpButton14->Enable();

    if (abortionRequested)
        mainDialog->pushStatusInformation(_("Operation aborted!"));

    mainDialog->m_buttonAbort->Hide();
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
    if (suppressUI_Errormessages)
        return StatusUpdater::continueNext;

    statusPanel->updateStatusPanelNow();

    wxString errorMessage = text + _("\n\nContinue with next object, retry or abort comparison?");

    ErrorDlg* errorDlg = new ErrorDlg(errorMessage, suppressUI_Errormessages);

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


SyncStatusUpdater::SyncStatusUpdater(wxWindow* dlg, bool hideErrorMessages) :
        suppressUI_Errormessages(hideErrorMessages)
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
            result+= unhandledErrors[j] + "\n";
        result+= "\n";
    }

    if (failedItems)
        result+= _("Not all items have been synchronized! You may try to synchronize the remaining items again (WITHOUT having to re-compare)!");
    else if (abortionRequested)
        result+= _("Synchronization aborted: You may try to synchronize remaining items again (WITHOUT having to re-compare)!");

    syncStatusFrame->setStatusText_NoUpdate(result);

    //notify to syncStatusFrame that current process has ended
    if (abortionRequested)
        syncStatusFrame->processHasFinished(statusAborted);  //enable okay and close events
    else if (failedItems)
        syncStatusFrame->processHasFinished(statusCompletedWithErrors);
    else
        syncStatusFrame->processHasFinished(statusCompletedWithSuccess);
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
    syncStatusFrame->setCurrentStatus(statusSynchronizing);
}


inline
void SyncStatusUpdater::updateProcessedData(int objectsProcessed, double dataProcessed)
{
    syncStatusFrame->incProgressIndicator_NoUpdate(objectsProcessed, dataProcessed);
}


int SyncStatusUpdater::reportError(const wxString& text)
{
    if (suppressUI_Errormessages)
    {
        unhandledErrors.Add(text);
        return StatusUpdater::continueNext;
    }

    syncStatusFrame->updateStatusDialogNow();

    wxString errorMessage = text + _("\n\nContinue with next object, retry or abort synchronization?");

    ErrorDlg* errorDlg = new ErrorDlg(errorMessage, suppressUI_Errormessages);

    int rv = errorDlg->ShowModal();
    errorDlg->Destroy();

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
    }

    return StatusUpdater::continueNext; //dummy return value
}


void SyncStatusUpdater::triggerUI_Refresh()
{
    if (abortionRequested)
        throw AbortThisProcess();    //abort can be triggered by syncStatusFrame

    if (updateUI_IsAllowed()) //test if specific time span between ui updates is over
        syncStatusFrame->updateStatusDialogNow();
}

