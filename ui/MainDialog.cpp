/***************************************************************
 * Name:      FreeFileSyncMain.cpp
 * Purpose:   Code for Application Frame
 * Author:    ZenJu (zhnmju123@gmx.de)
 * Created:   2008-07-16
 * Copyright: ZenJu ()
 * License:
 **************************************************************/

#include "MainDialog.h"
#include <wx/filename.h>
#include <stdexcept> //for std::runtime_error
#include "..\library\globalfunctions.h"
#include <fstream>
#include <wx/clipbrd.h>
#include "..\library\CustomGrid.h"
#include <cmath>
#include <wx/msgdlg.h>

using namespace GlobalFunctions;

int leadingPanel = 0;

MainDialog::MainDialog(wxFrame* frame) :
        GUI_Generated(frame),
        parent(frame),
        selectedRangeBegin(0),
        selectedRangeEnd(0),
        selectionLead(0),
        filteringPending(false),
        cmpStatusUpdaterTmp(0)
{
    //initialize sync configuration
    readConfigurationFromHD("config.dat");

    //set icons for this dialog
    m_bpButton11->SetBitmapLabel(*GlobalResources::bitmapAbout);
    m_bpButton10->SetBitmapLabel(*GlobalResources::bitmapExit);
    m_bpButtonCompare->SetBitmapLabel(*GlobalResources::bitmapCompare);
    m_bpButtonSync->SetBitmapLabel(*GlobalResources::bitmapSync);
    m_bpButtonSync->SetBitmapDisabled(*GlobalResources::bitmapSyncDisabled);
    m_bpButtonSwap->SetBitmapLabel(*GlobalResources::bitmapSwap);
    m_bpButton14->SetBitmapLabel(*GlobalResources::bitmapHelp);

    m_panel1->DragAcceptFiles(true);
    m_panel2->DragAcceptFiles(true);
    m_panel1->Connect(wxEVT_DROP_FILES, wxDropFilesEventHandler(MainDialog::onFilesDroppedPanel1), NULL, this);
    m_panel2->Connect(wxEVT_DROP_FILES, wxDropFilesEventHandler(MainDialog::onFilesDroppedPanel2), NULL, this);

    //support for CTRL + C
    m_grid1->Connect(wxEVT_KEY_DOWN, wxKeyEventHandler(MainDialog::onGrid1ButtonEvent), NULL, this);
    m_grid2->Connect(wxEVT_KEY_DOWN, wxKeyEventHandler(MainDialog::onGrid2ButtonEvent), NULL, this);

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

    m_grid3->GetGridWindow()->Connect(wxEVT_IDLE, wxEventHandler(MainDialog::OnIdleToFilterManually), NULL, this);
    m_grid3->GetGridWindow()->Connect(wxEVT_LEFT_UP, wxEventHandler(MainDialog::OnGrid3LeftMouseUp), NULL, this);

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
    m_bpButtonSync->Enable(false);

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

    m_panel1->Disconnect(wxEVT_DROP_FILES, wxDropFilesEventHandler(MainDialog::onFilesDroppedPanel1), NULL, this);
    m_panel2->Disconnect(wxEVT_DROP_FILES, wxDropFilesEventHandler(MainDialog::onFilesDroppedPanel2), NULL, this);

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

    m_grid3->GetGridWindow()->Disconnect(wxEVT_IDLE, wxEventHandler(MainDialog::OnIdleToFilterManually), NULL, this);
    m_grid3->GetGridWindow()->Disconnect(wxEVT_LEFT_UP, wxEventHandler(MainDialog::OnGrid3LeftMouseUp), NULL, this);

    try
    {
        writeConfigurationToHD("config.dat");   //don't trow exceptions in destructors
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

        //lastSelected Row should be set in OnDeselectRow()
        if (leadingRow < currentUI_Size)
            newSelection = !currentGridData[currentUI_View[leadingRow].linkToCurrentGridData].selectedForSynchronization;

        if (hideFiltered)
            assert (!newSelection); //if hidefiltered is active, there should be no filtered elements on screen => current element was filtered out

        for (int i = topRow; i <= bottomRow; ++ i)
        {
            bool& currentSelection = currentGridData[currentUI_View[i].linkToCurrentGridData].selectedForSynchronization;
            CompareFilesResult currentCmpResult = currentGridData[currentUI_View[i].linkToCurrentGridData].cmpResult;

            //toggle selection of current row
            currentSelection = newSelection;

            //update currentUI_View, in case of sorting without refresh (mapping of griddata to ui model)
            currentUI_View[i].cmpResult = evaluateCmpResult(currentCmpResult, currentSelection);
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
            currentUI_View.erase(currentUI_View.begin() + topRow, currentUI_View.begin() + bottomRow + 1);

            //redraw grid neccessary to update new dimensions
            writeGrid(currentGridData, true);        //use UI buffer, no mapping from currentGridData to UI model again, just a re-dimensioning of grids
            updateStatusInformation(currentUI_View); //status information has to be recalculated!
        }
    }
    //clear selection on grid
    m_grid3->ClearSelection();
}

/*grid event choreography:
1. UI-Mouse-Down => OnGrid3SelectCell
2. UI-Mouse-Up   => OnGrid3SelectRange (if at least two rows are marked)

=> the decision if a range or a single cell is selected can be made only after Mouse-UP. But OnGrid3SelectRange unfortunately is not always
executed (e.g. if single cell selected)

=> new choreography:

1. UI-Mouse-Down => OnGrid3SelectCell
2. UI-Mouse-Up   => OnGrid3LeftMouseUp (notify that filtering shall be started on next idle event
3. UI-Mouse-Up   => OnGrid3SelectRange, possibly
4. Idle event    => OnIdleToFilterManually

    It's !crazy! but it works!
*/

void MainDialog::OnGrid3SelectCell(wxGridEvent& event)
{
    selectionLead = selectedRangeBegin = selectedRangeEnd = event.GetRow();
    event.Skip();
}


void MainDialog::OnGrid3LeftMouseUp(wxEvent& event)
{
    filteringPending = true;
    event.Skip();
}


void MainDialog::OnGrid3SelectRange(wxGridRangeSelectEvent& event)
{
    if (event.Selecting())
    {
        selectedRangeBegin = event.GetTopRow();
        selectedRangeEnd   = event.GetBottomRow();
    }
    event.Skip();
}


void MainDialog::OnIdleToFilterManually(wxEvent& event)
{
    if (filteringPending)
    {
        filteringPending = false;
        filterRangeManual(selectedRangeBegin, selectedRangeEnd, selectionLead);
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


void MainDialog::onGrid1ButtonEvent(wxKeyEvent& event)
{
    //CTRL + C || CTRL + INS
    if (event.ControlDown() && event.GetKeyCode() == 67 ||
            event.ControlDown() && event.GetKeyCode() == WXK_INSERT)
        copySelectionToClipboard(m_grid1);
    event.Skip();
}

void MainDialog::onGrid2ButtonEvent(wxKeyEvent& event)
{
    //CTRL + C || CTRL + INS
    if (event.ControlDown() && event.GetKeyCode() == 67 ||
            event.ControlDown() && event.GetKeyCode() == WXK_INSERT)
        copySelectionToClipboard(m_grid2);
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

void onFilesDropped(wxString elementName, wxTextCtrl* txtCtrl, wxDirPickerCtrl* dirPicker)
{
    if (wxDirExists(elementName))
    {
        txtCtrl->SetValue(elementName);
        dirPicker->SetPath(elementName);
    }
    else
    {
        elementName = wxFileName(elementName).GetPath();
        if (wxDirExists(elementName))
        {
            txtCtrl->SetValue(elementName);
            dirPicker->SetPath(elementName);
        }
    }
}


void MainDialog::onFilesDroppedPanel1(wxDropFilesEvent& event)
{
    if (event.m_noFiles >= 1)
    {
        onFilesDropped(event.GetFiles()[0], m_directoryPanel1, m_dirPicker1);

        //disable the sync button
        m_bpButtonSync->Enable(false);
        //clear grids
        currentGridData.clear();
        writeGrid(currentGridData);
    }
    event.Skip();
}

void MainDialog::onFilesDroppedPanel2(wxDropFilesEvent& event)
{
    if (event.m_noFiles >= 1)
    {
        onFilesDropped(event.GetFiles()[0], m_directoryPanel2, m_dirPicker2);

        //disable the sync button
        m_bpButtonSync->Enable(false);
        //clear grids
        currentGridData.clear();
        writeGrid(currentGridData);
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


void MainDialog::readConfigurationFromHD(const wxString& filename)
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
//#####################################################

    ifstream config(filename.c_str());
    if (!config)
        return;

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

    char bigBuffer[10000];

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
    Maximize(bool(config.get()));

    config.getline(bigBuffer, 100, char(0));
    int width = atoi(bigBuffer);
    config.getline(bigBuffer, 100, char(0));
    int height = atoi(bigBuffer);
    config.getline(bigBuffer, 100, char(0));
    int posX = atoi(bigBuffer);
    config.getline(bigBuffer, 100, char(0));
    int posY = atoi(bigBuffer);
    SetSize(posX, posY, width, height);

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
    DWORD dwFileAttributes;

    //make config file visible: needed for ofstream
    if (wxFileExists(filename))
    {
        dwFileAttributes = FILE_ATTRIBUTE_NORMAL;
        if (!SetFileAttributes(
                    filename.c_str(),	// address of filename
                    dwFileAttributes 	// address of attributes to set
                )) throw runtime_error(string(_("Could not change attributes of ")) + filename.c_str());
    }

    ofstream config(filename.c_str());
    if (!config)
        throw runtime_error(string(_("Could not open ")) + filename.c_str());

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

    //write application window size
    config<<char(IsMaximized());
    //window size
    int width = 0;
    int height = 0;
    GetSize(&width, &height);
    config<<width<<char(0);
    config<<height<<char(0);
    //window position
    int posX = 0;
    int posY = 0;
    GetPosition(&posX, &posY);
    config<<posX<<char(0);
    config<<posY<<char(0);

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

    //hide config file
    dwFileAttributes = FILE_ATTRIBUTE_HIDDEN;
    if (!SetFileAttributes(
                filename.c_str(),	// address of filename
                dwFileAttributes 	// address of attributes to set
            )) throw runtime_error(string(_("Could not change attributes of ")) + filename.c_str());
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
    if (filterDlg->ShowModal() == OkayButtonPressed)
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
    m_statusBar1->SetStatusText("");
    m_statusBar1->SetStatusText("", 1);
    m_statusBar1->SetStatusText("", 2);
    m_statusBar1->SetStatusText("", 3);
    m_statusBar1->SetStatusText("", 4);

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

        cmpStatusUpdaterTmp = 0;
    }
    catch (AbortThisProcess& theException)
    {
        //disable the sync button
        m_bpButtonSync->Enable(false);
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

        m_statusBar1->SetStatusText("");
        m_statusBar1->SetStatusText("", 1);
        m_statusBar1->SetStatusText("", 2);
        m_statusBar1->SetStatusText("", 3);
        m_statusBar1->SetStatusText("", 4);

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
        m_statusBar1->SetStatusText(objectsView + _(" of ") + objectsTotal + _(" row in view"), 2);
    else
        m_statusBar1->SetStatusText(objectsView + _(" of ") + objectsTotal + _(" rows in view"), 2);


    wxString objectsViewRight = numberToWxString(objectsOnRightView);
    GlobalFunctions::includeNumberSeparator(objectsViewRight);
    if (objectsOnRightView == 1)
        m_statusBar1->SetStatusText(wxString(_("1 item on right, ")) + FreeFileSync::formatFilesizeToShortString(mpz_class(filesizeRightView)), 4);
    else
        m_statusBar1->SetStatusText(objectsViewRight + _(" items on right, ") + FreeFileSync::formatFilesizeToShortString(mpz_class(filesizeRightView)), 4);

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
            gridline.leftFilename      = i->fileDescrLeft.relFilename.AfterLast('\\');
            gridline.leftRelativePath  = i->fileDescrLeft.relFilename.BeforeLast('\\');
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
            gridline.rightFilename     = i->fileDescrRight.relFilename.AfterLast('\\');
            gridline.rightRelativePath = i->fileDescrRight.relFilename.BeforeLast('\\');
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
    while (wxTheApp->Pending())
        wxTheApp->Dispatch();
}


bool updateUI_IsAllowed()
{
    static wxLongLong lastExec = 0;

    wxLongLong newExec = wxGetLocalTimeMillis();

    if (newExec - lastExec > 100)  //perform ui updates not more often than necessary, 100 seems to be a good value with only a minimal performance loss
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

    //show abort button
    mainDialog->m_buttonAbort->Show(true);
    mainDialog->m_bpButtonCompare->Show(false);
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

    if (abortionRequested)
        mainDialog->m_statusBar1->SetStatusText(mainDialog->m_statusBar1->GetStatusText(2) + " - " + _("Aborted!"), 2);

    mainDialog->m_buttonAbort->Show(false);
    mainDialog->m_bpButtonCompare->Show(true);
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
        statusBar->SetStatusText(wxString(_("Scanning files/folders: ")) + numberToWxString(numberOfScannedObjects), 2);
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


