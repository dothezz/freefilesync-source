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
#include <stdexcept> //for std::runtime_error
#include "../library/globalFunctions.h"
#include <fstream>
#include <wx/clipbrd.h>
#include "../library/customGrid.h"
#include <cmath>
#include <wx/msgdlg.h>

using namespace GlobalFunctions;

int leadingPanel = 0;

MainDialog::MainDialog(wxFrame* frame, const wxString& cfgFileName) :
        GuiGenerated(frame),
        parent(frame),
        stackObjects(0),
        selectedRange3Begin(0),
        selectedRange3End(0),
        selectionLead(0),
        filteringInitialized(false),
        filteringPending(false),
        cmpStatusUpdaterTmp(0)
{
    //initialize sync configuration
    readConfigurationFromHD(cfgFileName, true);

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

    m_grid3->GetGridWindow()->Connect(wxEVT_IDLE, wxEventHandler(MainDialog::OnIdleEvent), NULL, this);
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

    updateViewFilterButtons();

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

    //set status of filter button
    updateFilterButton();
    //set status of "hide filtered items" checkbox
    m_checkBoxHideFilt->SetValue(hideFiltered);

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

    m_grid3->GetGridWindow()->Disconnect(wxEVT_IDLE, wxEventHandler(MainDialog::OnIdleEvent), NULL, this);
    m_grid3->GetGridWindow()->Disconnect(wxEVT_LEFT_UP, wxEventHandler(MainDialog::OnGrid3LeftMouseUp), NULL, this);
    m_grid3->GetGridWindow()->Disconnect(wxEVT_LEFT_DOWN, wxEventHandler(MainDialog::OnGrid3LeftMouseDown), NULL, this);

    Disconnect(wxEVT_SIZE, wxEventHandler(MainDialog::onResizeMainWindow), NULL, this);
    Disconnect(wxEVT_MOVE, wxEventHandler(MainDialog::onResizeMainWindow), NULL, this);

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

    try
    {
        writeConfigurationToHD(FreeFileSync::FFS_LastConfigFile);   //don't trow exceptions in destructors
    }
    catch (std::runtime_error& theException)
    {
        wxMessageBox(_(theException.what()), _("An exception occured!"), wxOK | wxICON_ERROR);
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


void MainDialog::filterRangeManual(int begin, int end, int leadingRow)
{
    int currentUI_Size = currentUI_View.size();

    int topRow = max(begin, 0);
    int bottomRow = min(end, currentUI_Size - 1);

    if (topRow <= bottomRow) // bottomRow might be -1 ?
    {

        bool newSelection = false;   //default: deselect range

        //leadingRow should be set in OnGridSelectCell()
        if (0 <= leadingRow && leadingRow < currentUI_Size)
            newSelection = !currentGridData[currentUI_View[leadingRow].linkToCurrentGridData].selectedForSynchronization;

        if (hideFiltered)
            assert (!newSelection); //if hidefiltered is active, there should be no filtered elements on screen => current element was filtered out


        //get all lines that need to be filtered (e.g. if a folder is marked, then its subelements should be marked as well)
        set<int> rowsToFilterOnGridData;    //rows to filter in backend
        for (int i = topRow; i <= bottomRow; ++i)
        {
            unsigned int gridDataLine = currentUI_View[i].linkToCurrentGridData;

            rowsToFilterOnGridData.insert(gridDataLine);
            FreeFileSync::addSubElements(rowsToFilterOnGridData, currentGridData, currentGridData[gridDataLine]);
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

            //for (set<int>::reverse_iterator i = filteredOutRowsOnUI.rbegin(); i != filteredOutRowsOnUI.rend(); ++i)
            //  currentUI_View.erase(currentUI_View.begin() + *i);

            //Note: the following lines are a performance optimization for deleting elements from a vector. It is incredibly faster to create a new
            //vector and leave specific elements out than to delete row by row and force recopying of most elements for each single deletion (linear vs quadratic runtime)

            //Note: This is the most time consuming part in this whole method!

            UI_Grid temp;
            int rowNr = 0;
            int rowToSkip = -1;

            set<int>::iterator rowToSkipIndex = filteredOutRowsOnUI.begin();

            if (rowToSkipIndex != filteredOutRowsOnUI.end())
                rowToSkip = *rowToSkipIndex;

            for (UI_Grid::iterator i = currentUI_View.begin(); i != currentUI_View.end(); ++i, ++rowNr)
            {
                if (rowNr != rowToSkip)
                    temp.push_back(*i);
                else
                {
                    rowToSkipIndex++;
                    if (rowToSkipIndex != filteredOutRowsOnUI.end())
                        rowToSkip = *rowToSkipIndex;
                }
            }
            currentUI_View.swap(temp);


            //redraw grid necessary to update new dimensions
            writeGrid(currentGridData, true);        //use UI buffer, no mapping from currentGridData to UI model again, just a re-dimensioning of grids
            updateStatusInformation(currentUI_View); //status information has to be recalculated!
        }
    }
    //clear selection on grid
    m_grid3->ClearSelection();
}

/*grid event choreography:
1. UI-Mouse-Down => OnGridSelectCell
2. UI-Mouse-Up   => OnGrid3SelectRange (if at least two rows are marked)

=> the decision if a range or a single cell is selected can be made only after Mouse-UP. But OnGrid3SelectRange unfortunately is not always
executed (e.g. if single cell selected)

=> new choreography:

1. UI-Mouse-Down => OnGridSelectCell    -> set leading row
2. UI-Mouse-Up   => OnGrid3LeftMouseUp (notify that filtering shall be started on next idle event
3. UI-Mouse-Up   => OnGrid3SelectRange, possibly
4. Idle event    => OnIdleEvent

    It's !crazy! but it works!
*/

void MainDialog::OnGridSelectCell(wxGridEvent& event)
{
    selectionLead = selectedRange3Begin = selectedRange3End = event.GetRow();
    event.Skip();
}


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


void MainDialog::OnGrid3SelectRange(wxGridRangeSelectEvent& event)
{
    if (event.Selecting())  //this range event should only be processed on release left mouse button
    {
        selectedRange3Begin = event.GetTopRow();
        selectedRange3End   = event.GetBottomRow();
    }
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

            filterRangeManual(selectedRange3Begin, selectedRange3End, selectionLead);
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


void copySelectionToClipboard(wxGrid* grid)
{
    int rowTop, rowBottom, colLeft, colRight; //coords of selection
    wxString clipboardString;

    wxArrayInt selectedRows, selectedColumns;
    selectedRows    = grid->GetSelectedRows();
    selectedColumns = grid->GetSelectedCols();

    if (!selectedRows.IsEmpty())
    {
        for (unsigned int i = 0; i < selectedRows.GetCount(); ++i)
        {
            for (int k = 0; k < grid->GetNumberCols(); ++k)
            {
                clipboardString+= grid->GetCellValue(selectedRows[i], k);
                if (k != grid->GetNumberCols() - 1)
                    clipboardString+= '\t';
            }
            clipboardString+= '\n';
        }
    }
    else if (!selectedColumns.IsEmpty())
    {
        for (int k = 0; k < grid->GetNumberRows(); ++k)
        {
            for (unsigned int i = 0; i < selectedColumns.GetCount(); ++i)
            {
                clipboardString+= grid->GetCellValue(k, selectedColumns[i]);
                if (i != selectedColumns.GetCount() - 1)
                    clipboardString+= '\t';
            }
            clipboardString+= '\n';
        }
    }
    else
    {
        wxGridCellCoordsArray tmpArray;

        tmpArray = grid->GetSelectionBlockTopLeft();
        if (!tmpArray.IsEmpty())
        {
            wxGridCellCoords topLeft = tmpArray[0];

            rowTop  = topLeft.GetRow();
            colLeft = topLeft.GetCol();

            tmpArray = grid->GetSelectionBlockBottomRight();
            if (!tmpArray.IsEmpty())
            {
                wxGridCellCoords bottomRight = tmpArray[0];

                rowBottom = bottomRight.GetRow();
                colRight  = bottomRight.GetCol();

                //save selection in one big string
                for (int j = rowTop; j <= rowBottom; ++j)
                {
                    for (int i = colLeft; i <= colRight; ++i)
                    {
                        clipboardString+= grid->GetCellValue(j, i);
                        if (i != colRight)
                            clipboardString+= '\t';
                    }
                    clipboardString+= '\n';
                }
            }
        }
    }

    if (!clipboardString.IsEmpty())
        // Write some text to the clipboard
        if (wxTheClipboard->Open())
        {
            // This data objects are held by the clipboard,
            // so do not delete them in the app.
            wxTheClipboard->SetData( new wxTextDataObject(clipboardString) );
            wxTheClipboard->Close();
        }
}


set<int> getSelectedRows(wxGrid* grid)
{
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

    return output;
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
            return StatusUpdater::Continue;
        }

        wxString errorMessage = text + _("\n\nInformation: If you skip the error and continue or abort a re-compare will be necessary!");

        ErrorDlg* errorDlg = new ErrorDlg(errorMessage, suppressUI_Errormessages);

        int rv = errorDlg->ShowModal();
        errorDlg->Destroy();

        switch (rv)
        {
        case ErrorDlg::ContinueButtonPressed:
            unsolvedErrors = true;
            return StatusUpdater::Continue;
        case ErrorDlg::RetryButtonPressed:
            return StatusUpdater::Retry;
        case ErrorDlg::AbortButtonPressed:
        {
            unsolvedErrors = true;
            throw AbortThisProcess();
        }
        default:
            assert (false);
        }

        return StatusUpdater::Continue; //dummy return value
    }
    void updateStatus(const wxString& text) {}
    void updateProgressIndicator(double number) {}


private:
    bool suppressUI_Errormessages;
    bool& unsolvedErrors;
};


void MainDialog::deleteFilesOnGrid(wxGrid* grid)
{
    set<int> rowsToDeleteOnUI = getSelectedRows(grid);

    if (0 <= selectionLead && unsigned(selectionLead) < currentUI_View.size())
        rowsToDeleteOnUI.insert(selectionLead); //add row of the currently selected cell

    removeInvalidRows(rowsToDeleteOnUI, currentUI_View.size());


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

            if (currentCmpLine.fileDescrLeft.objType != IsNothing)
                filesToDelete+= currentCmpLine.fileDescrLeft.filename + "\n";

            if (currentCmpLine.fileDescrRight.objType != IsNothing)
                filesToDelete+= currentCmpLine.fileDescrRight.filename + "\n";

            filesToDelete+= "\n";
        }

        DeleteDialog* confirmDeletion = new DeleteDialog(headerText, filesToDelete, this);    //no destruction needed; attached to main window

        switch (confirmDeletion->ShowModal())
        {
        case DeleteDialog::OkayButtonPressed:
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
        }
        break;

        case DeleteDialog::CancelButtonPressed:
        default:
            break;

        }
    }
}


void MainDialog::pushStatusInformation(const wxString& text)
{
    lastStatusChange = wxGetLocalTimeMillis();
    stackObjects++;
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
        GetSize(&widthNotMaximized, &heightNotMaximized);
        GetPosition(&posXNotMaximized, &posYNotMaximized);
    }
    event.Skip();
}


void MainDialog::onGrid1ButtonEvent(wxKeyEvent& event)
{
    //CTRL + C || CTRL + INS
    if (event.ControlDown() && event.GetKeyCode() == 67 ||
            event.ControlDown() && event.GetKeyCode() == WXK_INSERT)
        copySelectionToClipboard(m_grid1);

    else if (event.GetKeyCode() == WXK_DELETE)
        deleteFilesOnGrid(m_grid1);

    event.Skip();
}

void MainDialog::onGrid2ButtonEvent(wxKeyEvent& event)
{
    //CTRL + C || CTRL + INS
    if (event.ControlDown() && event.GetKeyCode() == 67 ||
            event.ControlDown() && event.GetKeyCode() == WXK_INSERT)
        copySelectionToClipboard(m_grid2);
    else if (event.GetKeyCode() == WXK_DELETE)
        deleteFilesOnGrid(m_grid2);

    event.Skip();
}


void MainDialog::onGrid3ButtonEvent(wxKeyEvent& event)
{
    //CTRL + C || CTRL + INS
    if (event.ControlDown() && event.GetKeyCode() == 67 ||
            event.ControlDown() && event.GetKeyCode() == WXK_INSERT)
        copySelectionToClipboard(m_grid3);
    else if (event.GetKeyCode() == WXK_DELETE)
        deleteFilesOnGrid(m_grid3);

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
        file1Full = wxFileName::GetCwd() + FileNameSeparator + file1;

    if (wxFileName(file2).GetPath() == wxEmptyString)
        file2Full = wxFileName::GetCwd() + FileNameSeparator + file2;

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

    wxFileDialog* filePicker = new wxFileDialog(this, "", "", defaultFileName, "*.*", wxFD_SAVE);

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
            wxFileDialog* filePicker = new wxFileDialog(this, "", "", "", "*.*", wxFD_OPEN);

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
            if (wxFileExists(newCfgFile) && FreeFileSync::isFFS_ConfigFile(newCfgFile))
            {
                readConfigurationFromHD(newCfgFile);

                pushStatusInformation(_("Configuration loaded!"));
            }
            else
                wxMessageBox(_("The selected file does not contain a valid configuration!"), _("Warning"), wxOK);
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


void MainDialog::readConfigurationFromHD(const wxString& filename, bool programStartup)
{
    //default values
    syncConfiguration.exLeftSideOnly  = SyncDirRight;
    syncConfiguration.exRightSideOnly = SyncDirRight;
    syncConfiguration.leftNewer       = SyncDirRight;
    syncConfiguration.rightNewer      = SyncDirRight;
    syncConfiguration.different       = SyncDirRight;

    leftOnlyFilesActive   = true;
    leftNewerFilesActive  = true;
    differentFilesActive  = true;
    rightNewerFilesActive = true;  //do not put these bool values into config.dat!
    rightOnlyFilesActive  = true;  //it's more convenient to have them defaulted for each execution
    equalFilesActive      = false;

    includeFilter = "*";    //include all files/folders
    excludeFilter = "";     //exlude nothing
    hideFiltered  = false;  //show filtered items
    filterIsActive = false; //do not filter by default

    useRecycleBin = false;  //do not use: in case OS doesn't support this, user will have to activate first and then get the error message

    widthNotMaximized  = wxDefaultCoord;
    heightNotMaximized = wxDefaultCoord;
    posXNotMaximized   = wxDefaultCoord;
    posYNotMaximized   = wxDefaultCoord;
//#####################################################

    ifstream config(filename.c_str());
    if (!config)
        return;

    //put filename on list of last used config files
    addCfgFileToHistory(filename);

    char bigBuffer[10000];


    //read FFS identifier
    config.get(bigBuffer, FreeFileSync::FFS_ConfigFileID.Len() + 1);

    //read sync configuration
    syncConfiguration.exLeftSideOnly  = SyncDirection(config.get());
    syncConfiguration.exRightSideOnly = SyncDirection(config.get());
    syncConfiguration.leftNewer       = SyncDirection(config.get());
    syncConfiguration.rightNewer      = SyncDirection(config.get());
    syncConfiguration.different       = SyncDirection(config.get());

    //read find method
    switch (int(config.get()))
    {
    case CompareByTimeAndSize:
        m_radioBtnSizeDate->SetValue(true);
        break;
    case CompareByMD5:
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
    filterIsActive = bool(config.get());

    //include
    config.getline(bigBuffer, 10000, char(0));
    includeFilter = bigBuffer;

    //exclude
    config.getline(bigBuffer, 10000, char(0));
    excludeFilter = bigBuffer;

    useRecycleBin =  bool(config.get());

    config.close();
}

void MainDialog::writeConfigurationToHD(const wxString& filename)
{
    ofstream config(filename.c_str());
    if (!config)
        throw runtime_error(string(_("Could not write to ")) + filename.c_str());

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
        config<<char(CompareByTimeAndSize);
    else if (m_radioBtnContent->GetValue())
        config<<char(CompareByMD5);
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

    if (filterIsActive)
        FreeFileSync::filterCurrentGridData(currentGridData, includeFilter, excludeFilter);
    else
        FreeFileSync::removeFilterOnCurrentGridData(currentGridData);

    writeGrid(currentGridData);

    //make sure, button-appearance and "filterIsActive" are in sync.
    updateFilterButton();
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
    FilterDlg* filterDlg = new FilterDlg(this);
    if (filterDlg->ShowModal() == FilterDlg::OkayButtonPressed)
    {
        if (filterIsActive)
        {
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

    CompareVariant cmpVar = CompareByTimeAndSize;   //assign a value to suppress compiler warning
    if (m_radioBtnSizeDate->GetValue())
        cmpVar = CompareByTimeAndSize;
    else if (m_radioBtnContent->GetValue())
        cmpVar = CompareByMD5;
    else assert (false);

    try
    {   //class handling status display and error messages
        CompareStatusUpdater statusUpdater(this, m_statusBar1);
        cmpStatusUpdaterTmp = &statusUpdater;
        stackObjects = 0;

        //unsigned int startTime3 = GetTickCount();
        FreeFileSync::getModelDiff(currentGridData,
                                   FreeFileSync::getFormattedDirectoryName(m_directoryPanel1->GetValue()),
                                   FreeFileSync::getFormattedDirectoryName(m_directoryPanel2->GetValue()),
                                   cmpVar,
                                   &statusUpdater);
        //wxMessageBox(numberToWxString(unsigned(GetTickCount()) - startTime3));


        //filter currentGridData if option is set
        if (filterIsActive)
            FreeFileSync::filterCurrentGridData(currentGridData, includeFilter, excludeFilter);

        writeGrid(currentGridData);

        //once compare is finished enable the sync button
        m_bpButtonSync->Enable(true);
        m_bpButtonSync->SetFocus();

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
        case FileOnLeftSideOnly:
            return "<|";
            break;
        case FileOnRightSideOnly:
            return "|>";
            break;
        case RightFileNewer:
            return ">>";
            break;
        case LeftFileNewer:
            return "<<";
            break;
        case FilesDifferent:
            return "!=";
            break;
        case FilesEqual:
            return "==";
            break;
        default:
            assert (false);
        }
    else return ConstFilteredOut;
}


void MainDialog::writeGrid(const FileCompareResult& gridData, bool useUI_GridCache)
{
    //unsigned int startTime = GetTickCount();

    if (!useUI_GridCache)
    {
        mapFileModelToUI(currentUI_View, gridData); //update currentUI_View
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

    //wxMessageBox(wxString("Benchmark: ") + numberToWxString(unsigned(GetTickCount()) - startTime) + " ms");
}


void MainDialog::OnSync( wxCommandEvent& event )
{
    //check if there are files/folders that can be synced
    bool nothingToSync = true;
    for (FileCompareResult::const_iterator i = currentGridData.begin(); i != currentGridData.end(); ++i)
        if (i->cmpResult != FilesEqual)
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

        //unsigned int startTime = GetTickCount();
        synchronizeFolders(currentGridData, syncConfiguration);
        //wxMessageBox(numberToWxString(unsigned(GetTickCount()) - startTime));

        wxEndBusyCursor();
    }
}


void MainDialog::OnLeftGridDoubleClick(wxGridEvent& event)
{   //default
    wxString command = "explorer " + FreeFileSync::getFormattedDirectoryName(m_directoryPanel1->GetValue());

    if (event.GetRow() < int(currentUI_View.size()))
    {
        wxString filename = currentGridData[currentUI_View[event.GetRow()].linkToCurrentGridData].fileDescrLeft.filename;

        if (!filename.IsEmpty())
            command = "explorer /select," + filename;
    }
    wxExecute(command);
}

void MainDialog::OnRightGridDoubleClick(wxGridEvent& event)
{   //default
    wxString command = "explorer " + FreeFileSync::getFormattedDirectoryName(m_directoryPanel2->GetValue());

    if (event.GetRow() < int(currentUI_View.size()))
    {
        wxString filename = currentGridData[currentUI_View[event.GetRow()].linkToCurrentGridData].fileDescrRight.filename;

        if (!filename.IsEmpty())
            command = "explorer /select," + filename;
    }
    wxExecute(command);
}

//these two global variables are ONLY used for the sorting in the following methods
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
bool cmpLargeInt(const wxString& a, const wxString& b)
{
    //if a and b not empty:
    bool result = true;
    wxString tmpString;
    mpz_t largeTempIntegerA;
    mpz_t largeTempIntegerB;
    mpz_init(largeTempIntegerA);
    mpz_init(largeTempIntegerB);
    mpz_set_str(largeTempIntegerA, a.c_str(), 10);
    //return value should be checked: if function fails, largeTempIntegerA is not changed: no issue here
    mpz_set_str(largeTempIntegerB, b.c_str(), 10);
    //return value should be checked: if function fails, largeTempIntegerA is not changed: no issue here
    if (sortAscending)
        result = (mpz_cmp(largeTempIntegerA, largeTempIntegerB) < 0); // true if A < B
    else
        result = (mpz_cmp(largeTempIntegerA, largeTempIntegerB) > 0); // true if A > B
    mpz_clear(largeTempIntegerA);
    mpz_clear(largeTempIntegerB);
    return result;
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
        if (typeA == IsNothing)
            return false;  //empty rows always last
        else if (typeB == IsNothing)
            return true;  //empty rows always last
        else if (typeA == IsDirectory)
            return false;
        else if (typeB == IsDirectory)
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
        if (typeA == IsNothing)
            return false;  //empty rows always last
        else if (typeB == IsNothing)
            return true;  //empty rows always last
        else if (typeA == IsDirectory)
            return false;
        else if (typeB == IsDirectory)
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


void MainDialog::synchronizeFolders(FileCompareResult& grid, const SyncConfiguration config)
{
    try
    {   //class handling status updates and error messages
        SyncStatusUpdater statusUpdater(this, FreeFileSync::calcTotalBytesToTransfer(grid, config).get_d());

        //start synchronization and return elements that were errorneous in "grid"

        //unsigned int startTime3 = GetTickCount();
        FreeFileSync::startSynchronizationProcess(grid, config, &statusUpdater, useRecycleBin);
        //wxMessageBox(numberToWxString(unsigned(GetTickCount()) - startTime3));


        //display files that couldn't be processed
        writeGrid(grid);
    }
    catch (AbortThisProcess& theException)
    {
        //disable the sync button
        m_bpButtonSync->Enable(false);
    }
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
    stackObjects = 0;
    unsigned int objectsOnLeftView = 0;
    unsigned int objectsOnRightView = 0;
    mpz_t filesizeLeftView, filesizeRightView, tmpInt;
    mpz_init(filesizeLeftView);
    mpz_init(filesizeRightView);
    mpz_init(tmpInt);

    int returnValue = 0;

    for (UI_Grid::const_iterator i = visibleGrid.begin(); i != visibleGrid.end(); i++)
    {
        const FileCompareLine& refLine = currentGridData[i->linkToCurrentGridData];

        //calculate total number of bytes for each sied
        if (refLine.fileDescrLeft.objType != IsNothing)
        {
            mpz_set_ui(tmpInt, 0);
            returnValue = mpz_set_str(tmpInt, refLine.fileDescrLeft.fileSize.c_str(), 10);
            mpz_add(filesizeLeftView, filesizeLeftView, tmpInt);
            assert (returnValue == 0);

            objectsOnLeftView++;
        }

        if (refLine.fileDescrRight.objType != IsNothing)
        {
            mpz_set_ui(tmpInt, 0);
            returnValue = mpz_set_str(tmpInt, refLine.fileDescrRight.fileSize.c_str(), 10);
            mpz_add(filesizeRightView, filesizeRightView, tmpInt);
            assert (returnValue == 0);

            objectsOnRightView++;
        }
    }

    //show status information on "root" level. This cannot be accomplished in writeGrid since filesizes are already formatted for display there
    wxString objectsViewLeft = numberToWxString(objectsOnLeftView);
    GlobalFunctions::includeNumberSeparator(objectsViewLeft);
    if (objectsOnLeftView == 1)
        m_statusBar1->SetStatusText(wxString(_("1 item on left, ")) + FreeFileSync::formatFilesizeToShortString(mpz_class(filesizeLeftView)), 0);
    else
        m_statusBar1->SetStatusText(objectsViewLeft + _(" items on left, ") + FreeFileSync::formatFilesizeToShortString(mpz_class(filesizeLeftView)), 0);


    wxString objectsTotal = numberToWxString(currentGridData.size());
    GlobalFunctions::includeNumberSeparator(objectsTotal);
    wxString objectsView = numberToWxString(visibleGrid.size());
    GlobalFunctions::includeNumberSeparator(objectsView);
    if (currentGridData.size() == 1)
        m_statusBar1->SetStatusText(objectsView + _(" of ") + objectsTotal + _(" row in view"), 1);
    else
        m_statusBar1->SetStatusText(objectsView + _(" of ") + objectsTotal + _(" rows in view"), 1);


    wxString objectsViewRight = numberToWxString(objectsOnRightView);
    GlobalFunctions::includeNumberSeparator(objectsViewRight);
    if (objectsOnRightView == 1)
        m_statusBar1->SetStatusText(wxString(_("1 item on right, ")) + FreeFileSync::formatFilesizeToShortString(mpz_class(filesizeRightView)), 2);
    else
        m_statusBar1->SetStatusText(objectsViewRight + _(" items on right, ") + FreeFileSync::formatFilesizeToShortString(mpz_class(filesizeRightView)), 2);

    //-----------------------------------------------------
    mpz_clear(filesizeLeftView);
    mpz_clear(filesizeRightView);
    mpz_clear(tmpInt);
}


void MainDialog::mapFileModelToUI(UI_Grid& output, const FileCompareResult& fileCmpResult)
{
    output.clear();

    UI_GridLine gridline;
    unsigned int currentRow = 0;
    wxString fileSize; //tmp string
    for (FileCompareResult::const_iterator i = fileCmpResult.begin(); i != fileCmpResult.end(); i++, currentRow++)
    {
        //process UI filter settings
        switch (i->cmpResult)
        {
        case FileOnLeftSideOnly:
            if (!leftOnlyFilesActive) continue;
            break;
        case FileOnRightSideOnly:
            if (!rightOnlyFilesActive) continue;
            break;
        case RightFileNewer:
            if (!rightNewerFilesActive) continue;
            break;
        case LeftFileNewer:
            if (!leftNewerFilesActive) continue;
            break;
        case FilesDifferent:
            if (!differentFilesActive) continue;
            break;
        case FilesEqual:
            if (!equalFilesActive) continue;
            break;
        default:
            assert (false);
        }

        //hide filtered row, if corresponding option is set
        if (hideFiltered && !i->selectedForSynchronization)
            continue;

        if (i->fileDescrLeft.objType == IsDirectory)
        {
            gridline.leftFilename      = wxEmptyString;
            gridline.leftRelativePath  = i->fileDescrLeft.relFilename;
            gridline.leftSize = _("<Directory>");
        }
        else
        {
            gridline.leftFilename      = i->fileDescrLeft.relFilename.AfterLast(FileNameSeparator);
            gridline.leftRelativePath  = i->fileDescrLeft.relFilename.BeforeLast(FileNameSeparator);
            gridline.leftSize      = GlobalFunctions::includeNumberSeparator(fileSize = i->fileDescrLeft.fileSize);
        }
        gridline.leftDate          = i->fileDescrLeft.lastWriteTime;

        gridline.cmpResult         = evaluateCmpResult(i->cmpResult, i->selectedForSynchronization);
        gridline.linkToCurrentGridData = currentRow;

        if (i->fileDescrRight.objType == IsDirectory)
        {
            gridline.rightFilename     = wxEmptyString;
            gridline.rightRelativePath = i->fileDescrRight.relFilename;
            gridline.rightSize = _("<Directory>");
        }
        else
        {
            gridline.rightFilename     = i->fileDescrRight.relFilename.AfterLast(FileNameSeparator);
            gridline.rightRelativePath = i->fileDescrRight.relFilename.BeforeLast(FileNameSeparator);
            gridline.rightSize      = GlobalFunctions::includeNumberSeparator(fileSize = i->fileDescrRight.fileSize);
        }
        gridline.rightDate         = i->fileDescrRight.lastWriteTime;

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

    if (newExec - lastExec >= 100)  //perform ui updates not more often than necessary, 100 seems to be a good value with only a minimal performance loss
    {
        lastExec = newExec;
        return true;
    }
    return false;
}

//########################################################################################################


CompareStatusUpdater::CompareStatusUpdater(MainDialog* dlg, wxStatusBar* mainWindowBar) :
        mainDialog(dlg),
        statusBar(mainWindowBar),
        suppressUI_Errormessages(false),
        numberOfScannedObjects(0)
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

    //show abort button
    mainDialog->m_buttonAbort->Show();
    mainDialog->m_bpButtonCompare->Hide();
    mainDialog->Layout();
    mainDialog->Refresh();
    mainDialog->m_buttonAbort->SetFocus();
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

    if (abortionRequested)
        mainDialog->pushStatusInformation(_("Operation aborted!"));

    mainDialog->m_buttonAbort->Hide();
    mainDialog->m_bpButtonCompare->Show();
    mainDialog->Layout();
    mainDialog->Refresh();
}

void CompareStatusUpdater::updateStatus(const wxString& text)
{
    //not relevant for "compare"; it's sufficient to display the number of scanned files/folders
}

inline
void CompareStatusUpdater::updateProgressIndicator(double number)
{
    numberOfScannedObjects+= int(number);   //conversion is harmless, since number == 1 in case of "compare"
}

int CompareStatusUpdater::reportError(const wxString& text)
{
    if (suppressUI_Errormessages)
        return StatusUpdater::Continue;

    wxString errorMessage = text + _("\n\nContinue with next object, retry or abort comparison?");

    ErrorDlg* errorDlg = new ErrorDlg(errorMessage, suppressUI_Errormessages);

    int rv = errorDlg->ShowModal();
    errorDlg->Destroy();

    switch (rv)
    {
    case ErrorDlg::ContinueButtonPressed:
        return StatusUpdater::Continue;
    case ErrorDlg::RetryButtonPressed:
        return StatusUpdater::Retry;
    case ErrorDlg::AbortButtonPressed:
    {
        abortionRequested = true;
        throw AbortThisProcess();
    }
    default:
        assert (false);
    }

    return StatusUpdater::Continue; //dummy return value
}


inline
void CompareStatusUpdater::triggerUI_Refresh()
{
    if (abortionRequested) throw AbortThisProcess();    //abort can be triggered by syncStatusFrame

    if (updateUI_IsAllowed()) //test if specific time span between ui updates is over
    {
        statusBar->SetStatusText(wxString(_("Scanning files/folders: ")) + numberToWxString(numberOfScannedObjects), 1);
        updateUI_Now();
    }
}
//########################################################################################################


SyncStatusUpdater::SyncStatusUpdater(wxWindow* dlg, double gaugeTotalElements) :
        suppressUI_Errormessages(false)
{
    syncStatusFrame = new SyncStatus(this, gaugeTotalElements, dlg);
    syncStatusFrame->Show();
    updateUI_Now();
}

SyncStatusUpdater::~SyncStatusUpdater()
{
    if (abortionRequested)
        syncStatusFrame->processHasFinished(_("Aborted!"));  //enable okay and close events
    else
        syncStatusFrame->processHasFinished(_("Completed"));

    //print the results list
    unsigned int failedItems = unhandledErrors.GetCount();
    wxString result;
    if (failedItems)
    {
        result = wxString(_("Warning: Synchronization failed for ")) + numberToWxString(failedItems) + _(" item(s):\n\n");
        for (unsigned int j = 0; j < failedItems; ++j)
            result+= unhandledErrors[j] + "\n";
    }
    syncStatusFrame->setStatusText_NoUpdate(result);
    syncStatusFrame->updateStatusDialogNow();
}


inline
void SyncStatusUpdater::updateStatus(const wxString& text)
{
    syncStatusFrame->setStatusText_NoUpdate(text);
}


inline
void SyncStatusUpdater::updateProgressIndicator(double number)
{
    syncStatusFrame->incProgressIndicator_NoUpdate(number);
}


int SyncStatusUpdater::reportError(const wxString& text)
{
    if (suppressUI_Errormessages)
    {
        unhandledErrors.Add(text);
        return StatusUpdater::Continue;
    }

    wxString errorMessage = text + _("\n\nContinue with next object, retry or abort synchronization?");

    ErrorDlg* errorDlg = new ErrorDlg(errorMessage, suppressUI_Errormessages);

    int rv = errorDlg->ShowModal();
    errorDlg->Destroy();

    switch (rv)
    {
    case ErrorDlg::ContinueButtonPressed:
        unhandledErrors.Add(text);
        return StatusUpdater::Continue;
    case ErrorDlg::RetryButtonPressed:
        return StatusUpdater::Retry;
    case ErrorDlg::AbortButtonPressed:
    {
        unhandledErrors.Add(text);
        abortionRequested = true;
        throw AbortThisProcess();
    }
    default:
        assert (false);
    }

    return StatusUpdater::Continue; //dummy return value
}


void SyncStatusUpdater::triggerUI_Refresh()
{
    if (abortionRequested)
        throw AbortThisProcess();    //abort can be triggered by syncStatusFrame

    if (updateUI_IsAllowed()) //test if specific time span between ui updates is over
    {
        syncStatusFrame->updateStatusDialogNow();
    }
}


