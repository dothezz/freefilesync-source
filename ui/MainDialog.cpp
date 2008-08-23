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
#include <wx/wfstream.h>
#include <wx/zipstrm.h>
#include <fstream>
#include <wx/clipbrd.h>
#include "..\library\CustomGrid.h"
#include <cmath>
#include <wx/msgdlg.h>

using namespace GlobalFunctions;

int leadingPanel = 0;

MainDialog::MainDialog(wxFrame* frame)
        : GUI_Generated(frame), parent(frame)
{
    //initialize sync configuration
    readConfigurationFromHD();

    loadResourceFiles();
    //set icons for this dialog
    m_bpButton11->SetBitmapLabel(*bitmapAbout);
    m_bpButton10->SetBitmapLabel(*bitmapExit);
    m_bpButton12->SetBitmapLabel(*bitmapCompare);
    m_bpButton111->SetBitmapLabel(*bitmapSync);
    m_bpButton111->SetBitmapDisabled(*bitmapSyncDisabled);
    m_bpButton13->SetBitmapLabel(*bitmapSwap);
    m_bpButton14->SetBitmapLabel(*bitmapHelp);

    m_panel1->DragAcceptFiles(true);
    m_panel2->DragAcceptFiles(true);
    m_panel1->Connect(wxEVT_DROP_FILES, wxDropFilesEventHandler(MainDialog::onFilesDroppedPanel1), NULL, this);
    m_panel2->Connect(wxEVT_DROP_FILES, wxDropFilesEventHandler(MainDialog::onFilesDroppedPanel2), NULL, this);

    //support for CTRL+C
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
    m_grid2->GetGridWindow()->Connect(wxEVT_LEFT_DOWN, wxEventHandler(MainDialog::onGrid2access), NULL, this);

    m_grid3->Connect(wxEVT_KEY_DOWN,               wxEventHandler(MainDialog::onGrid3access), NULL, this);
    m_grid3->Connect(wxEVT_SCROLLWIN_LINEUP,       wxEventHandler(MainDialog::onGrid3access), NULL, this);
    m_grid3->Connect(wxEVT_SCROLLWIN_LINEDOWN,     wxEventHandler(MainDialog::onGrid3access), NULL, this);
    m_grid3->GetGridWindow()->Connect(wxEVT_LEFT_DOWN, wxEventHandler(MainDialog::onGrid3access), NULL, this);

    m_grid3->GetGridWindow()->Connect(wxEVT_LEFT_UP, wxEventHandler(MainDialog::OnMarkRangeOnGrid3), NULL, this);


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
    m_bpButton111->Enable(false);

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
    m_checkBox2->SetValue(hideFiltered);

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

    m_grid2->Disconnect(wxEVT_KEY_DOWN,               wxEventHandler(MainDialog::onGrid2access), NULL, this);
    m_grid2->Disconnect(wxEVT_SCROLLWIN_TOP,          wxEventHandler(MainDialog::onGrid2access), NULL, this);
    m_grid2->Disconnect(wxEVT_SCROLLWIN_BOTTOM,       wxEventHandler(MainDialog::onGrid2access), NULL, this);
    m_grid2->Disconnect(wxEVT_SCROLLWIN_LINEUP,       wxEventHandler(MainDialog::onGrid2access), NULL, this);
    m_grid2->Disconnect(wxEVT_SCROLLWIN_LINEDOWN,     wxEventHandler(MainDialog::onGrid2access), NULL, this);
    m_grid2->Disconnect(wxEVT_SCROLLWIN_PAGEUP,       wxEventHandler(MainDialog::onGrid2access), NULL, this);
    m_grid2->Disconnect(wxEVT_SCROLLWIN_PAGEDOWN,     wxEventHandler(MainDialog::onGrid2access), NULL, this);
    m_grid2->Disconnect(wxEVT_SCROLLWIN_THUMBTRACK,   wxEventHandler(MainDialog::onGrid2access), NULL, this);
    m_grid2->Disconnect(wxEVT_SCROLLWIN_THUMBRELEASE, wxEventHandler(MainDialog::onGrid2access), NULL, this);

    m_grid3->Disconnect(wxEVT_KEY_DOWN,               wxEventHandler(MainDialog::onGrid3access), NULL, this);
    m_grid3->Disconnect(wxEVT_SCROLLWIN_LINEUP,       wxEventHandler(MainDialog::onGrid3access), NULL, this);
    m_grid3->Disconnect(wxEVT_SCROLLWIN_LINEDOWN,     wxEventHandler(MainDialog::onGrid3access), NULL, this);

    m_grid3->GetGridWindow()->Disconnect(wxEVT_LEFT_UP, wxEventHandler(MainDialog::OnMarkRangeOnGrid3), NULL, this);

    try
    {
        unloadResourceFiles();

        writeConfigurationToHD();
    }
    catch (std::runtime_error& theException)
    {
        wxMessageBox(_(theException.what()), _("An exception occured!"), wxOK | wxICON_ERROR);
    }
}


void MainDialog::onGrid1access(wxEvent& event)
{
    leadingPanel = 1;
    event.Skip();
}

void MainDialog::onGrid2access(wxEvent& event)
{
    leadingPanel = 2;
    event.Skip();
}

void MainDialog::onGrid3access(wxEvent& event)
{
    leadingPanel = 3;
    event.Skip();
}


//disable marking ranges on grid3: when user releases left mouse, the selection is cleared
void MainDialog::OnMarkRangeOnGrid3( wxEvent& event )
{
    m_grid3->ClearSelection();
    m_grid3->ForceRefresh();
    event.Skip();
}


void MainDialog::OnDeselectRow(wxGridEvent& event)
{
    int rowSelected = event.GetRow();
    if (unsigned(rowSelected)  < currentUI_View.size())
    {
        bool& currentSelection = currentGridData[currentUI_View[rowSelected].linkToCurrentGridData].selectedForSynchronization;
        CompareFilesResult currentCmpResult = currentGridData[currentUI_View[rowSelected].linkToCurrentGridData].cmpResult;

        //toggle selection of current row
        currentSelection = !currentSelection;

        //update currentUI_View, in case of sorting without refresh (mapping of griddata to ui model
        currentUI_View[rowSelected].cmpResult = evaluateCmpResult(currentCmpResult, currentSelection);

        //signal UI that grids need to be refreshed on next Update()
        m_grid1->ForceRefresh();
        m_grid2->ForceRefresh();
        m_grid3->ForceRefresh();

        if (hideFiltered)
        {
            assert (!currentSelection); //if hidefiltered is active, there should be no filtered elements on screen => current element was filtered out

            //some delay to show user the row he has filtered out before it is removed
            Update();   //show changes resulting from ForceRefresh()
            wxMilliSleep(400);

            //delete row, that is filtered out:
            currentUI_View.erase(currentUI_View.begin() + rowSelected);
            //redraw grid neccessary to update new dimensions
            writeGrid(currentGridData, true);   //use UI buffer, no mapping from currentGridData to UI model again, just a re-dimensioning of grids
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
    m_bpButton111->Enable(false);

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
    m_bpButton111->Enable(false);
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
        m_bpButton111->Enable(false);
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
        m_bpButton111->Enable(false);
        //clear grids
        currentGridData.clear();
        writeGrid(currentGridData);
    }
    event.Skip();
}

void MainDialog::OnChangeCompareVariant(wxCommandEvent& event)
{
    //disable the sync button
    m_bpButton111->Enable(false);
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


void MainDialog::loadResourceFiles()
{
    //map, allocate and initialize pictures
    bitmapResource["left arrow.png"]        = (bitmapLeftArrow       = new wxBitmap(wxNullBitmap));
    bitmapResource["start sync.png"]        = (bitmapStartSync       = new wxBitmap(wxNullBitmap));
    bitmapResource["right arrow.png"]       = (bitmapRightArrow      = new wxBitmap(wxNullBitmap));
    bitmapResource["delete.png"]            = (bitmapDelete          = new wxBitmap(wxNullBitmap));
    bitmapResource["email.png"]             = (bitmapEmail           = new wxBitmap(wxNullBitmap));
    bitmapResource["about.png"]             = (bitmapAbout           = new wxBitmap(wxNullBitmap));
    bitmapResource["website.png"]           = (bitmapWebsite         = new wxBitmap(wxNullBitmap));
    bitmapResource["exit.png"]              = (bitmapExit            = new wxBitmap(wxNullBitmap));
    bitmapResource["sync.png"]              = (bitmapSync            = new wxBitmap(wxNullBitmap));
    bitmapResource["compare.png"]           = (bitmapCompare         = new wxBitmap(wxNullBitmap));
    bitmapResource["sync disabled.png"]     = (bitmapSyncDisabled    = new wxBitmap(wxNullBitmap));
    bitmapResource["swap.png"]              = (bitmapSwap            = new wxBitmap(wxNullBitmap));
    bitmapResource["help.png"]              = (bitmapHelp            = new wxBitmap(wxNullBitmap));
    bitmapResource["leftOnly.png"]          = (bitmapLeftOnly        = new wxBitmap(wxNullBitmap));
    bitmapResource["leftNewer.png"]         = (bitmapLeftNewer       = new wxBitmap(wxNullBitmap));
    bitmapResource["different.png"]         = (bitmapDifferent       = new wxBitmap(wxNullBitmap));
    bitmapResource["rightNewer.png"]        = (bitmapRightNewer      = new wxBitmap(wxNullBitmap));
    bitmapResource["rightOnly.png"]         = (bitmapRightOnly       = new wxBitmap(wxNullBitmap));
    bitmapResource["leftOnlyDeact.png"]     = (bitmapLeftOnlyDeact   = new wxBitmap(wxNullBitmap));
    bitmapResource["leftNewerDeact.png"]    = (bitmapLeftNewerDeact  = new wxBitmap(wxNullBitmap));
    bitmapResource["differentDeact.png"]    = (bitmapDifferentDeact  = new wxBitmap(wxNullBitmap));
    bitmapResource["rightNewerDeact.png"]   = (bitmapRightNewerDeact = new wxBitmap(wxNullBitmap));
    bitmapResource["rightOnlyDeact.png"]    = (bitmapRightOnlyDeact  = new wxBitmap(wxNullBitmap));
    bitmapResource["equal.png"]             = (bitmapEqual           = new wxBitmap(wxNullBitmap));
    bitmapResource["equalDeact.png"]        = (bitmapEqualDeact      = new wxBitmap(wxNullBitmap));
    bitmapResource["include.png"]           = (bitmapInclude         = new wxBitmap(wxNullBitmap));
    bitmapResource["exclude.png"]           = (bitmapExclude         = new wxBitmap(wxNullBitmap));
    bitmapResource["filter active.png"]     = (bitmapFilterOn        = new wxBitmap(wxNullBitmap));
    bitmapResource["filter not active.png"] = (bitmapFilterOff       = new wxBitmap(wxNullBitmap));
    bitmapResource["Warning.png"]           = (bitmapWarning         = new wxBitmap(wxNullBitmap));
    bitmapResource["small arrow up.png"]    = (bitmapSmallUp         = new wxBitmap(wxNullBitmap));
    bitmapResource["small arrow down.png"]  = (bitmapSmallDown       = new wxBitmap(wxNullBitmap));

    animationMoney = new wxAnimation(wxNullAnimation);


    wxFileInputStream input("Resources.dat");
    if (!input.IsOk()) throw runtime_error(_("Unable to load Resources.dat!"));

    wxZipInputStream resourceFile(input);

    wxZipEntry* entry;
    map<wxString, wxBitmap*>::iterator bmp;
    while (entry = resourceFile.GetNextEntry())
    {
        wxString name = entry->GetName();

        //just to be sure: search if entry is available in map
        if ((bmp = bitmapResource.find(name)) != bitmapResource.end())
            *(bmp->second) = wxBitmap(wxImage(resourceFile, wxBITMAP_TYPE_PNG));
    }

    animationMoney->LoadFile("Resources.a01");
}


void MainDialog::unloadResourceFiles()
{
    //free bitmap resources
    for (map<wxString, wxBitmap*>::iterator i = bitmapResource.begin(); i != bitmapResource.end(); ++i)
        delete i->second;

    //free other resources
    delete animationMoney;
}

void MainDialog::readConfigurationFromHD()
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

    includeFilter = "*";  //include all files/folders
    excludeFilter = "";   //exlude nothing
    hideFiltered  = false; //show filtered items
    filterIsActive = false; //do not filter by default

//#####################################################

    ifstream config("config.dat");
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
        m_radioBtn4->SetValue(true);
        break;
    case CompareByMD5:
        m_radioBtn5->SetValue(true);
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

    //exlude
    config.getline(bigBuffer, 10000, char(0));
    excludeFilter = bigBuffer;

    config.close();
}

void MainDialog::writeConfigurationToHD()
{
    char* configFile = "config.dat";
    DWORD dwFileAttributes;

    //make config file visible: needed for ofstream
    if (wxFileExists(configFile))
    {
        dwFileAttributes = FILE_ATTRIBUTE_NORMAL;
        if (!SetFileAttributes(
                    configFile,	// address of filename
                    dwFileAttributes 	// address of attributes to set
                )) throw runtime_error(_("Could not change attributes of ""config.dat""!"));
    }

    ofstream config(configFile);
    if (!config)
        throw runtime_error(_("Could not open ""config.dat"" for write access!"));

    //write sync configuration
    config<<char(syncConfiguration.exLeftSideOnly)
    <<char(syncConfiguration.exRightSideOnly)
    <<char(syncConfiguration.leftNewer)
    <<char(syncConfiguration.rightNewer)
    <<char(syncConfiguration.different);

    //write find method
    if (m_radioBtn4->GetValue())
        config<<char(CompareByTimeAndSize);
    else if (m_radioBtn5->GetValue())
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

    config.close();

    //hide config file
    dwFileAttributes = FILE_ATTRIBUTE_HIDDEN;
    if (!SetFileAttributes(
                configFile,	// address of filename
                dwFileAttributes 	// address of attributes to set
            )) throw runtime_error(_("Could not change attributes of ""config.dat""!"));
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

    //make sure, checkbox and "hideFiltered" are in sync.
    m_checkBox2->SetValue(hideFiltered);
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
        m_bpButton20->SetBitmapLabel(*bitmapLeftOnly);
    else
        m_bpButton20->SetBitmapLabel(*bitmapLeftOnlyDeact);

    if (leftNewerFilesActive)
        m_bpButton21->SetBitmapLabel(*bitmapLeftNewer);
    else
        m_bpButton21->SetBitmapLabel(*bitmapLeftNewerDeact);

    if (differentFilesActive)
        m_bpButton22->SetBitmapLabel(*bitmapDifferent);
    else
        m_bpButton22->SetBitmapLabel(*bitmapDifferentDeact);

    if (rightNewerFilesActive)
        m_bpButton23->SetBitmapLabel(*bitmapRightNewer);
    else
        m_bpButton23->SetBitmapLabel(*bitmapRightNewerDeact);

    if (rightOnlyFilesActive)
        m_bpButton24->SetBitmapLabel(*bitmapRightOnly);
    else
        m_bpButton24->SetBitmapLabel(*bitmapRightOnlyDeact);

    if (equalFilesActive)
        m_bpButton25->SetBitmapLabel(*bitmapEqual);
    else
        m_bpButton25->SetBitmapLabel(*bitmapEqualDeact);
}


void MainDialog::updateFilterButton()
{
    if (filterIsActive)
    {
        m_bpButtonFilter->SetBitmapLabel(*bitmapFilterOn);
        m_bpButtonFilter->SetToolTip(_("Filter active: Press again to deactivate"));
    }
    else
    {
        m_bpButtonFilter->SetBitmapLabel(*bitmapFilterOff);
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

    //unsigned int startTime = GetTickCount();

    CompareVariant cmpVar = CompareByTimeAndSize;   //assign a value to suppress compiler warning
    if (m_radioBtn4->GetValue())
        cmpVar  = CompareByTimeAndSize;
    else if (m_radioBtn5->GetValue())
        cmpVar  = CompareByMD5;
    else assert (false);

    try
    {
        //unsigned int startTime3 = GetTickCount();
        FreeFileSync::getModelDiff(currentGridData,
                                   FreeFileSync::getFormattedDirectoryName(m_directoryPanel1->GetValue()),
                                   FreeFileSync::getFormattedDirectoryName(m_directoryPanel2->GetValue()),
                                   cmpVar);
        //wxMessageBox(numberToWxString(unsigned(GetTickCount()) - startTime3));

        //filter currentGridData if option is set
        if (filterIsActive)
            FreeFileSync::filterCurrentGridData(currentGridData, includeFilter, excludeFilter);

        //unsigned int startTime4 = GetTickCount();
        writeGrid(currentGridData);
        //wxMessageBox(numberToWxString(unsigned(GetTickCount()) - startTime4));

        //once compare is finished enable the sync button
        m_bpButton111->Enable(true);

        //m_statusBar1->SetStatusText(wxString(_("Benchmark: ")) + numberToWxString(unsigned(GetTickCount()) - startTime) + " ms", 2);
    }
    catch (std::runtime_error& theException)
    {
        wxMessageBox(_(theException.what()), _("An exception occured!"), wxOK | wxICON_ERROR);
        //disable the sync button
        m_bpButton111->Enable(false);
    }

    wxEndBusyCursor();
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
        mapFileModelToUI(currentUI_View, gridData);

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
    if (syncDlg->ShowModal() == StartSynchronizationProcess)
    {
        wxBeginBusyCursor();

        m_statusBar1->SetStatusText("");
        m_statusBar1->SetStatusText("", 1);
        m_statusBar1->SetStatusText("", 2);
        m_statusBar1->SetStatusText("", 3);
        m_statusBar1->SetStatusText("", 4);

        //unsigned int startTime = GetTickCount();

        synchronizeFolders(currentGridData, syncConfiguration);

        wxString gridSizeFormatted = numberToWxString(currentGridData.size());
        GlobalFunctions::includeNumberSeparator(gridSizeFormatted);
        m_statusBar1->SetStatusText(gridSizeFormatted + _(" item(s) remaining"), 2);

        //  m_statusBar1->SetStatusText(wxString("Benchmark: ") + numberToWxString(unsigned(GetTickCount()) - startTime) + " ms", 2);

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
            m_grid1->setSortMarker(currentSortColumn, bitmapSmallUp);
        else
            m_grid1->setSortMarker(currentSortColumn, bitmapSmallDown);
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
            m_grid2->setSortMarker(currentSortColumn, bitmapSmallUp);
        else
            m_grid2->setSortMarker(currentSortColumn, bitmapSmallDown);
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
    FreeFileSync fileSyncObject;    //currently only needed for use of recycle bin
    mpz_t largeTmpInt;
    mpz_init(largeTmpInt);

    FileCompareResult resultsGrid;

    //prepare progress indicator:
    wxGauge* m_gauge1 = 0;
    m_gauge1 = new wxGauge( this, wxID_ANY, 100, wxDefaultPosition, wxSize( -1,15 ), wxGA_HORIZONTAL );
    bSizer1->Add( m_gauge1, 0, wxALL|wxEXPAND, 5 );
    Layout();

    bool suppressErrormessages = false;

    try
    {
        double totalElements   = 0;  // each element represents one byte for proper progress indicator scaling
        double currentElements = 0;
        double scalingFactor   = 0;  //nr of elements has to be normalized to smaller nr. because of range of int limitation

        //read total nr. of bytes that will be transferred into totalElements
        totalElements = FreeFileSync::calcTotalBytesToTransfer(grid, config).get_d();

        if (totalElements != 0)
            scalingFactor = 50000 / totalElements; //let's normalize to 50000
        m_gauge1->SetRange(50000);
        m_gauge1->SetValue(0);

//wxMessageBox(numberToWxString(float(totalElements)));

        // it should never happen, that a directory on left side has same name as file on right side. GetModelDiff should take care of this
        // and split into two "exists on one side only" cases
        // Note: this case is not handled by this tool as this is considered to be a bug and must be solved by the user

        //synchronize folders:
        for (FileCompareResult::const_iterator i = grid.begin(); i != grid.end(); ++i)
        {
            try
            {
                if (i->fileDescrLeft.objType == IsDirectory || i->fileDescrRight.objType == IsDirectory)
                {
                    if (!fileSyncObject.synchronizeFolder(*i, config))
                        resultsGrid.push_back(*i); //append folders that have not been synced according to configuration
                }
            }
            catch (fileError& error)
            {
                if (!suppressErrormessages)
                {
                    wxString errorMessage = error.show() + _("\n\nSkip this directory and continue synchronization?");

                    SyncErrorDlg* errorDlg = new SyncErrorDlg(this, errorMessage, suppressErrormessages);

                    if (errorDlg->ShowModal() == AbortButtonPressed)
                        throw error;
                    //else if ContinueButtonPressed: right now there are only two return values from errorDlg
                }
            }
        }

        //synchronize files:
        for (FileCompareResult::const_iterator i = grid.begin(); i != grid.end(); ++i)
        {
            try
            {
                if (i->fileDescrLeft.objType == IsFile || i->fileDescrRight.objType == IsFile)
                {
                    if (!fileSyncObject.synchronizeFile(*i, config))
                    {
                        resultsGrid.push_back(*i); //append files that have not been synced according to configuration
                    }
                    else
                    {   //progress indicator update
                        FreeFileSync::getBytesToTransfer(largeTmpInt, *i, config);
                        currentElements+= mpz_get_d(largeTmpInt);
                        m_gauge1->SetValue(int(currentElements * scalingFactor));
                        Update();
                    }
                }
            }
            catch (fileError& error)
            {
                if (!suppressErrormessages)
                {
                    wxString errorMessage = error.show() + _("\n\nSkip this file and continue synchronization?");

                    SyncErrorDlg* errorDlg = new SyncErrorDlg(this, errorMessage, suppressErrormessages);

                    if (errorDlg->ShowModal() == AbortButtonPressed)
                        throw error;
                    //else if ContinueButtonPressed: right now there are only two return values from errorDlg
                }
            }
        }

        //display files that couldn't be processed
        currentGridData = resultsGrid;
        writeGrid(currentGridData);
    }
    catch (fileError& error)
    {
        //don't show error message, this was already done before

        //disable the sync button
        m_bpButton111->Enable(false);

        //no return here to clean up data in the next lines of code
    }
    //clean up
    //remove progress indicator from window
    m_gauge1->Destroy();
    Layout();

    mpz_clear(largeTmpInt);

    //do not include other lines of code here than clean up
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


void MainDialog::mapFileModelToUI(UI_Grid& output, const FileCompareResult& fileCmpResult)
{
    output.clear();
    unsigned int objectsOnLeftView = 0;
    unsigned int objectsOnRightView = 0;
    mpz_t filesizeLeftView, filesizeRightView, tmpInt;
    mpz_init(filesizeLeftView);
    mpz_init(filesizeRightView);
    mpz_init(tmpInt);

    int returnValue = 0;

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

        gridline.leftFilename      = i->fileDescrLeft.relFilename.AfterLast('\\');
        gridline.leftRelativePath  = i->fileDescrLeft.relFilename.BeforeLast('\\');
        if (i->fileDescrLeft.objType == IsDirectory)
            gridline.leftSize = _("<Directory>");
        else
            gridline.leftSize      = GlobalFunctions::includeNumberSeparator(fileSize = i->fileDescrLeft.fileSize);
        gridline.leftDate          = i->fileDescrLeft.lastWriteTime;

        gridline.cmpResult         = evaluateCmpResult(i->cmpResult, i->selectedForSynchronization);
        gridline.linkToCurrentGridData = currentRow;

        gridline.rightFilename     = i->fileDescrRight.relFilename.AfterLast('\\');
        gridline.rightRelativePath = i->fileDescrRight.relFilename.BeforeLast('\\');
        if (i->fileDescrRight.objType == IsDirectory)
            gridline.rightSize = _("<Directory>");
        else
            gridline.rightSize      = GlobalFunctions::includeNumberSeparator(fileSize = i->fileDescrRight.fileSize);
        gridline.rightDate         = i->fileDescrRight.lastWriteTime;

        output.push_back(gridline);

        //prepare status information
        if (i->fileDescrLeft.objType != IsNothing)
        {
            mpz_set_ui(tmpInt, 0);
            returnValue = mpz_set_str(tmpInt, i->fileDescrLeft.fileSize.c_str(), 10);
            mpz_add(filesizeLeftView, filesizeLeftView, tmpInt);
            assert (returnValue == 0);

            objectsOnLeftView++;
        }

        if (i->fileDescrRight.objType != IsNothing)
        {
            mpz_set_ui(tmpInt, 0);
            returnValue = mpz_set_str(tmpInt, i->fileDescrRight.fileSize.c_str(), 10);
            mpz_add(filesizeRightView, filesizeRightView, tmpInt);
            assert (returnValue == 0);

            objectsOnRightView++;
        }
    }

    //sorting is expensive: ca. 50% bigger runtime for large grids; unsorted doesn't look too bad, so it's disabled
    // sort(output.begin(), output.end(), cmpGridSmallerThan);


    //show status information on "root" level. This cannot be accomplished in writeGrid since filesizes are already formatted for display there
    wxString objectsViewLeft = numberToWxString(objectsOnLeftView);
    GlobalFunctions::includeNumberSeparator(objectsViewLeft);
    if (objectsOnLeftView != 1)
        m_statusBar1->SetStatusText(objectsViewLeft + _(" items on left, ") + FreeFileSync::formatFilesizeToShortString(mpz_class(filesizeLeftView)), 0);
    else
        m_statusBar1->SetStatusText(wxString(_("1 item on left, ")) + FreeFileSync::formatFilesizeToShortString(mpz_class(filesizeLeftView)), 0);


    wxString objectsTotal = numberToWxString(fileCmpResult.size());
    GlobalFunctions::includeNumberSeparator(objectsTotal);
    wxString objectsView = numberToWxString(output.size());
    GlobalFunctions::includeNumberSeparator(objectsView);
    if (fileCmpResult.size() != 1)
        m_statusBar1->SetStatusText(objectsView + _(" of ") + objectsTotal + _(" rows in view"), 2);
    else
        m_statusBar1->SetStatusText(objectsView + _(" of ") + objectsTotal + _(" row in view"), 2);


    wxString objectsViewRight = numberToWxString(objectsOnRightView);
    GlobalFunctions::includeNumberSeparator(objectsViewRight);
    if (objectsOnRightView != 1)
        m_statusBar1->SetStatusText(objectsViewRight + _(" items on right, ") + FreeFileSync::formatFilesizeToShortString(mpz_class(filesizeRightView)), 4);
    else
        m_statusBar1->SetStatusText(wxString(_("1 item on right, ")) + FreeFileSync::formatFilesizeToShortString(mpz_class(filesizeRightView)), 4);

    //-----------------------------------------------------
    mpz_clear(filesizeLeftView);
    mpz_clear(filesizeRightView);
    mpz_clear(tmpInt);
}

