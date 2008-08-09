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

    m_grid2->Connect(wxEVT_KEY_DOWN,               wxEventHandler(MainDialog::onGrid2access), NULL, this);
    m_grid2->Connect(wxEVT_SCROLLWIN_TOP,          wxEventHandler(MainDialog::onGrid2access), NULL, this);
    m_grid2->Connect(wxEVT_SCROLLWIN_BOTTOM,       wxEventHandler(MainDialog::onGrid2access), NULL, this);
    m_grid2->Connect(wxEVT_SCROLLWIN_LINEUP,       wxEventHandler(MainDialog::onGrid2access), NULL, this);
    m_grid2->Connect(wxEVT_SCROLLWIN_LINEDOWN,     wxEventHandler(MainDialog::onGrid2access), NULL, this);
    m_grid2->Connect(wxEVT_SCROLLWIN_PAGEUP,       wxEventHandler(MainDialog::onGrid2access), NULL, this);
    m_grid2->Connect(wxEVT_SCROLLWIN_PAGEDOWN,     wxEventHandler(MainDialog::onGrid2access), NULL, this);
    m_grid2->Connect(wxEVT_SCROLLWIN_THUMBTRACK,   wxEventHandler(MainDialog::onGrid2access), NULL, this);
    m_grid2->Connect(wxEVT_SCROLLWIN_THUMBRELEASE, wxEventHandler(MainDialog::onGrid2access), NULL, this);

    m_grid3->Connect(wxEVT_KEY_DOWN,               wxEventHandler(MainDialog::onGrid3access), NULL, this);
    m_grid3->Connect(wxEVT_SCROLLWIN_LINEUP,       wxEventHandler(MainDialog::onGrid3access), NULL, this);
    m_grid3->Connect(wxEVT_SCROLLWIN_LINEDOWN,     wxEventHandler(MainDialog::onGrid3access), NULL, this);

    m_grid3->GetGridWindow()->Connect(wxEVT_LEFT_UP, wxEventHandler(MainDialog::OnMarkRangeOnGrid3), NULL, this);


    wxString toolTip = wxString(_("Legend\n")) +
                       _("---------\n") +
                       _("<|	file on left side only\n") +
                       _("|>	file on right side only\n") +
                       _("<<	left file is newer\n") +
                       _(">>	right file is newer\n") +
                       _("!=	files are different\n") +
                       _("==	files are equal\n\n") +
                       _("(-)	exclude from sync-process\n");
    m_grid3->GetGridWindow()->SetToolTip(toolTip);

    //enable parallel scrolling
    m_grid1->setScrollFriends(m_grid1, m_grid2, m_grid3);
    m_grid2->setScrollFriends(m_grid1, m_grid2, m_grid3);
    m_grid3->setScrollFriends(m_grid1, m_grid2, m_grid3);

    updateFilterButtons();

    //disable sync button as long as "compare" hasn't been triggered.
    m_bpButton111->Enable(false);

    //make filesize right justified on grids
    wxGridCellAttr* cellAttributes = m_grid1->GetOrCreateCellAttr(0, 2);
    cellAttributes->SetAlignment(wxALIGN_RIGHT,wxALIGN_CENTRE);
    m_grid1->SetColAttr(2, cellAttributes);

    cellAttributes = m_grid2->GetOrCreateCellAttr(0, 2);
    cellAttributes->SetAlignment(wxALIGN_RIGHT,wxALIGN_CENTRE);
    m_grid2->SetColAttr(2, cellAttributes);

    //as the name says: disable them
    m_grid3->deactivateScrollbars();

lightBlue = new wxColour(80, 110, 255);
}

MainDialog::~MainDialog()
{
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

    delete lightBlue;

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

void MainDialog::onGrid3access(wxEvent& event) {}   //keyboard access on grid3: do nothing


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
    if (rowSelected + 1 <= int(currentUI_View.size()))
    {
        bool& currentSelection = currentGridData[currentUI_View[rowSelected].linkToCurrentGridData].selectedForSynchronization;
        CompareFilesResult currentCmpResult = currentGridData[currentUI_View[rowSelected].linkToCurrentGridData].cmpResult;

        //files that are equal aren't sync'ed, so exclude them from selection
        //btw: logic in SynchronizeFile will cause a deselected row to appear on grid after sync, this doesn't make sense for files that are equal
        if (currentCmpResult != filesEqual)
        {
            //toggle selection of current row
            currentSelection = !currentSelection;

            //something for the eye...
            if (currentSelection)
                m_grid3->SetCellBackgroundColour(rowSelected, 0, *wxWHITE);
            else
                m_grid3->SetCellBackgroundColour(rowSelected, 0, *lightBlue);

            //update cell with "(-)" and don't forget currentUI_View, in case of sorting without refresh
            m_grid3->SetCellValue(rowSelected, 0, evaluateCmpResult(currentCmpResult, currentSelection));
            currentUI_View[rowSelected].cmpResult = evaluateCmpResult(currentCmpResult, currentSelection);
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


void MainDialog::OnDirChangedPanel1(wxFileDirPickerEvent& event)
{
    wxString newPath = m_dirPicker1->GetPath();
    if (newPath.Last() != '\\')
        newPath.append("\\");

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
    if (newPath.Last() != '\\')
        newPath.append("\\");

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
        if (elementName.Last() != '\\')
            elementName.append("\\");

        txtCtrl->SetValue(elementName);
        dirPicker->SetPath(elementName);
    }
    else
    {
        elementName = wxFileName(elementName).GetPath();
        if (wxDirExists(elementName))
        {
            if (elementName.Last() != '\\')
                elementName.append("\\");

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

void loadPng(wxBitmap* bitmap, wxZipInputStream& resource)
{
    wxImage* tmpImage = new wxImage(resource, wxBITMAP_TYPE_PNG);
    *bitmap = wxBitmap(*tmpImage);
    delete tmpImage;
}

void MainDialog::loadResourceFiles()
{
    //initialize and allocate pictures in case they are not found in resource file
    bitmapLeftArrow       = new wxBitmap(wxNullBitmap);
    bitmapStartSync       = new wxBitmap(wxNullBitmap);
    bitmapRightArrow      = new wxBitmap(wxNullBitmap);
    bitmapDelete          = new wxBitmap(wxNullBitmap);
    bitmapEmail           = new wxBitmap(wxNullBitmap);
    bitmapAbout           = new wxBitmap(wxNullBitmap);
    bitmapWebsite         = new wxBitmap(wxNullBitmap);
    bitmapExit            = new wxBitmap(wxNullBitmap);
    bitmapSync            = new wxBitmap(wxNullBitmap);
    bitmapCompare         = new wxBitmap(wxNullBitmap);
    bitmapSyncDisabled    = new wxBitmap(wxNullBitmap);
    bitmapSwap            = new wxBitmap(wxNullBitmap);
    bitmapHelp            = new wxBitmap(wxNullBitmap);
    bitmapLeftOnly        = new wxBitmap(wxNullBitmap);
    bitmapLeftNewer       = new wxBitmap(wxNullBitmap);
    bitmapEqual           = new wxBitmap(wxNullBitmap);
    bitmapDifferent       = new wxBitmap(wxNullBitmap);
    bitmapRightNewer      = new wxBitmap(wxNullBitmap);
    bitmapRightOnly       = new wxBitmap(wxNullBitmap);
    bitmapLeftOnlyDeact   = new wxBitmap(wxNullBitmap);
    bitmapLeftNewerDeact  = new wxBitmap(wxNullBitmap);
    bitmapEqualDeact      = new wxBitmap(wxNullBitmap);
    bitmapDifferentDeact  = new wxBitmap(wxNullBitmap);
    bitmapRightNewerDeact = new wxBitmap(wxNullBitmap);
    bitmapRightOnlyDeact  = new wxBitmap(wxNullBitmap);
    animationMoney        = new wxAnimation(wxNullAnimation);


    wxFileInputStream input("Resources.dat");
    if (!input.IsOk()) throw runtime_error(_("Unable to load Resources.dat!"));

    wxZipInputStream resourceFile(input);

    wxZipEntry* entry;
    while (entry = resourceFile.GetNextEntry())
    {
        wxString name = entry->GetName();

        if (name == wxString("left arrow.png"))
            loadPng(bitmapLeftArrow, resourceFile);
        else if (name == wxString("start sync.png"))
            loadPng(bitmapStartSync, resourceFile);
        else if (name == wxString("right arrow.png"))
            loadPng(bitmapRightArrow, resourceFile);
        else if (name == wxString("delete.png"))
            loadPng(bitmapDelete, resourceFile);
        else if (name == wxString("email.png"))
            loadPng(bitmapEmail, resourceFile);
        else if (name == wxString("about.png"))
            loadPng(bitmapAbout, resourceFile);
        else if (name == wxString("website.png"))
            loadPng(bitmapWebsite, resourceFile);
        else if (name == wxString("exit.png"))
            loadPng(bitmapExit, resourceFile);
        else if (name == wxString("sync.png"))
            loadPng(bitmapSync, resourceFile);
        else if (name == wxString("compare.png"))
            loadPng(bitmapCompare, resourceFile);
        else if (name == wxString("sync disabled.png"))
            loadPng(bitmapSyncDisabled, resourceFile);
        else if (name == wxString("swap.png"))
            loadPng(bitmapSwap, resourceFile);
        else if (name == wxString("help.png"))
            loadPng(bitmapHelp, resourceFile);
        else if (name == wxString("leftOnly.png"))
            loadPng(bitmapLeftOnly, resourceFile);
        else if (name == wxString("leftNewer.png"))
            loadPng(bitmapLeftNewer, resourceFile);
        else if (name == wxString("equal.png"))
            loadPng(bitmapEqual, resourceFile);
        else if (name == wxString("different.png"))
            loadPng(bitmapDifferent, resourceFile);
        else if (name == wxString("rightNewer.png"))
            loadPng(bitmapRightNewer, resourceFile);
        else if (name == wxString("rightOnly.png"))
            loadPng(bitmapRightOnly, resourceFile);
        else if (name == wxString("leftOnlyDeact.png"))
            loadPng(bitmapLeftOnlyDeact, resourceFile);
        else if (name == wxString("leftNewerDeact.png"))
            loadPng(bitmapLeftNewerDeact, resourceFile);
        else if (name == wxString("equalDeact.png"))
            loadPng(bitmapEqualDeact, resourceFile);
        else if (name == wxString("differentDeact.png"))
            loadPng(bitmapDifferentDeact, resourceFile);
        else if (name == wxString("rightNewerDeact.png"))
            loadPng(bitmapRightNewerDeact, resourceFile);
        else if (name == wxString("rightOnlyDeact.png"))
            loadPng(bitmapRightOnlyDeact, resourceFile);
    }

    animationMoney->LoadFile("Resources.a01");
}


void MainDialog::unloadResourceFiles()
{
    delete bitmapLeftArrow;
    delete bitmapStartSync;
    delete bitmapRightArrow;
    delete bitmapDelete;
    delete bitmapEmail;
    delete bitmapAbout;
    delete bitmapWebsite;
    delete bitmapExit;
    delete bitmapSync;
    delete bitmapCompare;
    delete bitmapSyncDisabled;
    delete bitmapSwap;
    delete bitmapHelp;
    delete bitmapLeftOnly;
    delete bitmapLeftNewer;
    delete bitmapEqual;
    delete bitmapDifferent;
    delete bitmapRightNewer;
    delete bitmapRightOnly;
    delete bitmapLeftOnlyDeact;
    delete bitmapLeftNewerDeact;
    delete bitmapEqualDeact;
    delete bitmapDifferentDeact;
    delete bitmapRightNewerDeact;
    delete bitmapRightOnlyDeact;
    delete animationMoney;
}

void MainDialog::readConfigurationFromHD()
{
    //default values
    syncConfiguration.exLeftSideOnly  = syncDirRight;
    syncConfiguration.exRightSideOnly = syncDirRight;
    syncConfiguration.leftNewer       = syncDirRight;
    syncConfiguration.rightNewer      = syncDirRight;
    syncConfiguration.different       = syncDirRight;

    leftOnlyFilesActive   = true;
    leftNewerFilesActive  = true;
    differentFilesActive  = true;
    rightNewerFilesActive = true;  //do not put these bool values into config.dat!
    rightOnlyFilesActive  = true;  //it's more convenient to have them defaulted for each execution
    equalFilesActive      = false;

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
    case compareByTimeAndSize:
        m_radioBtn4->SetValue(true);
        break;
    case compareByMD5:
        m_radioBtn5->SetValue(true);
        break;
    default:
        assert (false);
    }

    //read column sizes
    char integer[100];
    for (int i = 0; i < m_grid1->GetNumberCols(); ++i)
    {
        config.getline(integer, 100, char(0));
        m_grid1->SetColSize(i, atoi(integer));
    }
    for (int i = 0; i < m_grid2->GetNumberCols(); ++i)
    {
        config.getline(integer, 100, char(0));
        m_grid2->SetColSize(i, atoi(integer));
    }

    //read application window size
    Maximize(bool(config.get()));

    config.getline(integer, 100, char(0));
    int width = atoi(integer);
    config.getline(integer, 100, char(0));
    int height = atoi(integer);
    SetSize(width, height);

    //read last directory selection
    char directory[2000];
    config.getline(directory, 2000, char(0));
    m_directoryPanel1->SetValue(directory);
    if (wxDirExists(directory))
        m_dirPicker1->SetPath(directory);

    config.getline(directory, 2000, char(0));
    m_directoryPanel2->SetValue(directory);
    if (wxDirExists(directory))
        m_dirPicker2->SetPath(directory);

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
        config<<char(compareByTimeAndSize);
    else if (m_radioBtn5->GetValue())
        config<<char(compareByMD5);
    else assert (false);

    //write column sizes
    for (int i = 0; i < m_grid1->GetNumberCols(); ++i)
        config<<m_grid1->GetColSize(i)<<char(0);
    for (int i = 0; i < m_grid2->GetNumberCols(); ++i)
        config<<m_grid2->GetColSize(i)<<char(0);

    //write application window size
    config<<char(IsMaximized());

    int width = 0;
    int height = 0;
    GetSize(&width, &height);
    config<<width<<char(0);
    config<<height<<char(0);

    //write last directory selection
    config<<m_directoryPanel1->GetValue().c_str()<<char(0)
    <<m_directoryPanel2->GetValue().c_str()<<char(0);

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

void MainDialog::OnLeftOnlyFiles(wxCommandEvent& event)
{
    leftOnlyFilesActive = !leftOnlyFilesActive;
    updateFilterButtons();
    writeGrid(currentGridData);
};

void MainDialog::OnLeftNewerFiles(wxCommandEvent& event)
{
    leftNewerFilesActive = !leftNewerFilesActive;
    updateFilterButtons();
    writeGrid(currentGridData);
};

void MainDialog::OnDifferentFiles(wxCommandEvent& event)
{
    differentFilesActive = !differentFilesActive;
    updateFilterButtons();
    writeGrid(currentGridData);
};

void MainDialog::OnRightNewerFiles(wxCommandEvent& event)
{
    rightNewerFilesActive = !rightNewerFilesActive;
    updateFilterButtons();
    writeGrid(currentGridData);
};

void MainDialog::OnRightOnlyFiles(wxCommandEvent& event)
{
    rightOnlyFilesActive = !rightOnlyFilesActive;
    updateFilterButtons();
    writeGrid(currentGridData);
};

void MainDialog::OnEqualFiles(wxCommandEvent& event)
{
    equalFilesActive = !equalFilesActive;
    updateFilterButtons();
    writeGrid(currentGridData);
};

void MainDialog::updateFilterButtons()
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
        wxMessageBox(_("Directory on the left does not exist anymore. Please select a new one!"), _("Warning"), wxICON_WARNING);
        return;
    }
    else if (!wxDirExists(m_directoryPanel2->GetValue()))
    {
        wxMessageBox(_("Directory on the right does not exist anymore. Please select a new one!"), _("Warning"), wxICON_WARNING);
        return;
    }
    wxBeginBusyCursor();

    unsigned int startTime = GetTickCount();

    CompareVariant cmpVar;
    if (m_radioBtn4->GetValue())
        cmpVar  = compareByTimeAndSize;
    else if (m_radioBtn5->GetValue())
        cmpVar  = compareByMD5;
    else assert (false);

    try
    {
        //unsigned int startTime3 = GetTickCount();
        FreeFileSync::getModelDiff(currentGridData, m_directoryPanel1->GetValue(), m_directoryPanel2->GetValue(), cmpVar);
        //wxMessageBox(numberToString(unsigned(GetTickCount()) - startTime3).c_str());

        //unsigned int startTime4 = GetTickCount();
        writeGrid(currentGridData);
        //wxMessageBox(numberToString(unsigned(GetTickCount()) - startTime4).c_str());

        //once compare is finished enable the sync button
        m_bpButton111->Enable(true);

        m_statusBar1->SetStatusText(wxString(_("Benchmark: ")) + numberToString(unsigned(GetTickCount()) - startTime).c_str() + " ms", 2);
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
    else return "(-)";
}


void MainDialog::writeGrid(const FileCompareResult& gridData, bool useUI_GridCache)
{
    //unsigned int startTime = GetTickCount();

    //currentUI_View is backup of last UI view: no need for new mapping, when user just changes sorting
    //this set is also necessary if user double-clicks on a row when filters are activated
    if (!useUI_GridCache)
        mapFileModelToUI(currentUI_View, gridData);

    const unsigned int minimumRows = 15;

    m_grid1->BeginBatch();
    m_grid2->BeginBatch();
    m_grid3->BeginBatch();

    unsigned int oldGridSize = m_grid1->GetNumberRows();
    unsigned int newGridSize = max(unsigned(currentUI_View.size()), minimumRows);

    if (oldGridSize < newGridSize)
    {
        if (!m_grid1->AppendRows(newGridSize - oldGridSize) ||
                !m_grid2->AppendRows(newGridSize - oldGridSize))
            throw runtime_error(_("Error when modifying GUI grid"));
    }
    else if (oldGridSize > newGridSize)
    {
        if (!m_grid1->DeleteRows(newGridSize, oldGridSize - newGridSize) ||
                !m_grid2->DeleteRows(newGridSize, oldGridSize - newGridSize))
            throw runtime_error(_("Error when modifying GUI grid"));
    }

    //ClearGrid is only necessary in this case, since all entries will be overwritten by the next loop
    if (newGridSize  > currentUI_View.size())
    {
        m_grid1->ClearGrid();
        m_grid2->ClearGrid();
    }

    //handle grid3 separately: cells might be formatted (selected) so delete it completeley and reallocate
    //that's a performance optimization, since grid1 and grid2 need not to be deleted completely (contain no formatted cells)
    m_grid3->DeleteRows(0, oldGridSize);
    if (m_grid3->AppendRows(newGridSize) == false)
        throw runtime_error(_("Error when modifying GUI grid"));

    //enlarge label width to display row numbers correctly
    if (newGridSize >= 100000)
    {
        m_grid1->SetRowLabelSize(50);
        m_grid2->SetRowLabelSize(50);
    }
    else
    {
        m_grid1->SetRowLabelSize(38);
        m_grid2->SetRowLabelSize(38);
    }

    unsigned int rowNumber = 0;
    for (UI_Grid::const_iterator i = currentUI_View.begin(); i != currentUI_View.end(); ++i, rowNumber++)
    {
        m_grid1->SetCellValue(rowNumber, 0, i->leftFilename);
        m_grid1->SetCellValue(rowNumber, 1, i->leftRelativePath);
        m_grid1->SetCellValue(rowNumber, 2, i->leftSize);
        m_grid1->SetCellValue(rowNumber, 3, i->leftDate);

        m_grid2->SetCellValue(rowNumber, 0, i->rightFilename);
        m_grid2->SetCellValue(rowNumber, 1, i->rightRelativePath);
        m_grid2->SetCellValue(rowNumber, 2, i->rightSize);
        m_grid2->SetCellValue(rowNumber, 3, i->rightDate);

        m_grid3->SetCellValue(rowNumber, 0, i->cmpResult);

        //some background color for the eye...
        if (!currentGridData[i->linkToCurrentGridData].selectedForSynchronization)
            m_grid3->SetCellBackgroundColour(rowNumber, 0, *lightBlue);
    }

    m_grid1->EndBatch();
    m_grid2->EndBatch();
    m_grid3->EndBatch();

    //wxMessageBox(wxString("Benchmark: ") + numberToString(unsigned(GetTickCount()) - startTime).c_str() + " ms");
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
    if (syncDlg->ShowModal() == StartSynchronizationProcess)
    {
        wxBeginBusyCursor();

        m_statusBar1->SetStatusText("");
        m_statusBar1->SetStatusText("", 1);
        m_statusBar1->SetStatusText("", 2);
        m_statusBar1->SetStatusText("", 3);
        m_statusBar1->SetStatusText("", 4);

        try
        {
            //unsigned int startTime = GetTickCount();

            synchronizeFolders(currentGridData, syncConfiguration);

            string gridSizeFormatted = numberToString(currentGridData.size());
            GlobalFunctions::includeNumberSeparator(gridSizeFormatted);
            m_statusBar1->SetStatusText(wxString(gridSizeFormatted.c_str()) + _(" item(s) remaining"), 2);

            //  m_statusBar1->SetStatusText(wxString("Benchmark: ") + numberToString(unsigned(GetTickCount()) - startTime).c_str() + " ms", 2);
        }
        catch (fileError& error)
        {
            wxMessageBox(error.show(), _("An error occured"), wxICON_ERROR);
            //disable the sync button
            m_bpButton111->Enable(false);
        }
        wxEndBusyCursor();
    }
}


void MainDialog::OnLeftGridDoubleClick(wxGridEvent& event)
{
    wxString command;

    if (int(currentUI_View.size()) < event.GetRow() + 1)
        command = "explorer " + m_directoryPanel1->GetValue();
    else
    {
        wxString filename = currentGridData[currentUI_View[event.GetRow()].linkToCurrentGridData].fileDescrLeft.filename;

        if (filename.IsEmpty())
            command = "explorer " + m_directoryPanel1->GetValue();
        else
            command = "explorer /select," + filename;
    }
    wxExecute(command);
}

void MainDialog::OnRightGridDoubleClick(wxGridEvent& event)
{
    wxString command;

    if (int(currentUI_View.size()) < event.GetRow() + 1)
        command = "explorer " + m_directoryPanel2->GetValue();
    else
    {
        wxString filename = currentGridData[currentUI_View[event.GetRow()].linkToCurrentGridData].fileDescrRight.filename;

        if (filename.IsEmpty())
            command = "explorer " + m_directoryPanel2->GetValue();
        else
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
    if (a.IsEmpty())
        return false;   // if a and b are empty: false, if a empty, b not empty: also false, since empty rows should be put below on grid
    else if (b.IsEmpty())
        return true;        // empty rows after filled rows: return true

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
        //use unformatted filesizes
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
        //use unformatted filesizes
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
        writeGrid(currentGridData, true);
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
    wxGauge* m_gauge1 = new wxGauge( this, wxID_ANY, 100, wxDefaultPosition, wxSize( -1,15 ), wxGA_HORIZONTAL );
    bSizer1->Add( m_gauge1, 0, wxALL|wxEXPAND, 5 );
    Layout();

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

//wxMessageBox(numberToString(float(totalElements)).c_str());

        // it should never happen, that a directory on left side has same name as file on right side. GetModelDiff should take care of this
        // and split into two "exists on one side only" cases
        // Note: this case is not handled by this tool as this is considered to be a bug and must be solved by the user

        //synchronize folders:
        for (FileCompareResult::const_iterator i = grid.begin(); i != grid.end(); ++i)
        {
            if (i->fileDescrLeft.objType == isDirectory || i->fileDescrRight.objType == isDirectory)
            {
                if (!fileSyncObject.synchronizeFolder(*i, config))
                    resultsGrid.push_back(*i); //append folders that have not been synced according to configuration
            }
        }

        //synchronize files:
        for (FileCompareResult::const_iterator i = grid.begin(); i != grid.end(); ++i)
        {
            if (i->fileDescrLeft.objType == isFile || i->fileDescrRight.objType == isFile)
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
    }
    catch (fileError& error)
    {   //clean up
        //remove progress indicator from window
        m_gauge1->Destroy();
        Layout();

        mpz_clear(largeTmpInt);

        throw error;
    }

    //remove progress indicator from window
    m_gauge1->Destroy();
    Layout();

    //display files that couldn't be processed
    currentGridData = resultsGrid;
    writeGrid(currentGridData);

    mpz_clear(largeTmpInt);
}


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
    string fileSize; //tmp string
    for (FileCompareResult::const_iterator i = fileCmpResult.begin(); i != fileCmpResult.end(); i++, currentRow++)
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

        gridline.leftFilename      = i->fileDescrLeft.relFilename.AfterLast('\\');
        gridline.leftRelativePath  = i->fileDescrLeft.relFilename.BeforeLast('\\');
        if (i->fileDescrLeft.objType == isDirectory)
            gridline.leftSize = _("<directory>");
        else
            gridline.leftSize      = GlobalFunctions::includeNumberSeparator(fileSize = i->fileDescrLeft.fileSize.c_str()).c_str();
        gridline.leftDate          = i->fileDescrLeft.lastWriteTime;

        gridline.cmpResult         = evaluateCmpResult(i->cmpResult, i->selectedForSynchronization);
        gridline.linkToCurrentGridData = currentRow;

        gridline.rightFilename     = i->fileDescrRight.relFilename.AfterLast('\\');
        gridline.rightRelativePath = i->fileDescrRight.relFilename.BeforeLast('\\');
        if (i->fileDescrRight.objType == isDirectory)
            gridline.rightSize = _("<directory>");
        else
            gridline.rightSize      = GlobalFunctions::includeNumberSeparator(fileSize = i->fileDescrRight.fileSize.c_str()).c_str();
        gridline.rightDate         = i->fileDescrRight.lastWriteTime;

        output.push_back(gridline);

        //prepare status information
        if (i->fileDescrLeft.objType != isNothing)
        {
            mpz_set_ui(tmpInt, 0);
            returnValue = mpz_set_str(tmpInt, i->fileDescrLeft.fileSize.c_str(), 10);
            mpz_add(filesizeLeftView, filesizeLeftView, tmpInt);
            assert (returnValue == 0);

            objectsOnLeftView++;
        }

        if (i->fileDescrRight.objType != isNothing)
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
    string objectsViewLeft = numberToString(objectsOnLeftView);
    GlobalFunctions::includeNumberSeparator(objectsViewLeft);
    if (objectsOnLeftView != 1)
        m_statusBar1->SetStatusText(wxString(objectsViewLeft.c_str()) + _(" items on left (") + FreeFileSync::formatFilesizeToShortString(mpz_class(filesizeLeftView)) + ")", 0);
    else
        m_statusBar1->SetStatusText(wxString(_("1 item on left (")) + FreeFileSync::formatFilesizeToShortString(mpz_class(filesizeLeftView)) + ")", 0);


    string objectsTotal = numberToString(fileCmpResult.size());
    GlobalFunctions::includeNumberSeparator(objectsTotal);
    string objectsView = numberToString(output.size());
    GlobalFunctions::includeNumberSeparator(objectsView);
    if (output.size() != 1)
        m_statusBar1->SetStatusText(wxString(objectsView.c_str()) + _(" rows in view (") + objectsTotal + _(" total)")   , 2);
    else
        m_statusBar1->SetStatusText(wxString(_("1 row in view (")) + objectsTotal + _(" total)")   , 2);


    string objectsViewRight = numberToString(objectsOnRightView);
    GlobalFunctions::includeNumberSeparator(objectsViewRight);
    if (objectsOnRightView != 1)
        m_statusBar1->SetStatusText(wxString(objectsViewRight.c_str()) + _(" items on right (") + FreeFileSync::formatFilesizeToShortString(mpz_class(filesizeRightView)) + ")", 4);
    else
        m_statusBar1->SetStatusText(wxString(_("1 item on right (")) + FreeFileSync::formatFilesizeToShortString(mpz_class(filesizeRightView)) + ")", 4);

    //-----------------------------------------------------
    mpz_clear(filesizeLeftView);
    mpz_clear(filesizeRightView);
    mpz_clear(tmpInt);
}



