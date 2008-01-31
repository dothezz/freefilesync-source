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
#include <wx/wfstream.h>
#include <wx/clipbrd.h>
#include <wx/ffile.h>
#include "../library/customGrid.h"
#include <algorithm>
#include "../library/sorting.h"
#include <wx/msgdlg.h>
#include "../comparison.h"
#include "../synchronization.h"
#include "../algorithm.h"

using namespace globalFunctions;
using namespace xmlAccess;


extern const wxGrid* leadGrid = NULL;

MainDialog::MainDialog(wxFrame* frame, const wxString& cfgFileName, CustomLocale* language, xmlAccess::XmlGlobalSettings& settings) :
        MainDialogGenerated(frame),
        globalSettings(settings),
        programLanguage(language),
        filteringInitialized(false),
        filteringPending(false),
        synchronizationEnabled(false),
        restartOnExit(false),
        cmpStatusHandlerTmp(0)
{
    m_bpButtonCompare->SetLabel(_("&Compare"));
    m_bpButtonSync->SetLabel(_("&Synchronize"));

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

    //initialize and load configuration
    readConfigurationFromXml(cfgFileName, true);
    readGlobalSettings();

    leftOnlyFilesActive   = true;
    leftNewerFilesActive  = true;
    differentFilesActive  = true;
    rightNewerFilesActive = true;  //do not put these bool values into config.dat!
    rightOnlyFilesActive  = true;  //it's more convenient to have them defaulted at startup
    equalFilesActive      = false;
    updateViewFilterButtons();

    //set icons for this dialog
    m_bpButton10->SetBitmapLabel(*globalResource.bitmapExit);
    m_bpButtonCompare->SetBitmapLabel(*globalResource.bitmapCompare);
    m_bpButtonSync->SetBitmapLabel(*globalResource.bitmapSync);
    m_bpButtonSwap->SetBitmapLabel(*globalResource.bitmapSwap);
    m_bpButton14->SetBitmapLabel(*globalResource.bitmapHelp);
    m_bpButton201->SetBitmapLabel(*globalResource.bitmapSave);
    m_bpButtonAddPair->SetBitmapLabel(*globalResource.bitmapAddFolderPair);
    m_bpButtonRemovePair->SetBitmapLabel(*globalResource.bitmapRemoveFolderPair);
    m_bpButtonRemovePair->SetBitmapDisabled(*globalResource.bitmapRemoveFolderPairD);
    m_bitmap15->SetBitmap(*globalResource.bitmapStatusEdge);

    //menu icons
    m_menuItem10->SetBitmap(*globalResource.bitmapCompareSmall);
    m_menuItem11->SetBitmap(*globalResource.bitmapSyncSmall);
    m_menuItem7->SetBitmap(*globalResource.bitmapBatchSmall);
    m_menuItemAdjustTimes->SetBitmap(*globalResource.bitmapClockSmall);

    //Workaround for wxWidgets: small hack to update menu items: actually this is a wxWidgets bug (affects Windows- and Linux-build)
    m_menu1->Remove(m_menuItem10);
    m_menu1->Remove(m_menuItem11);
    m_menu1->Insert(0, m_menuItem10);
    m_menu1->Insert(1, m_menuItem11);
    m_menu3->Remove(m_menuItem7);
    m_menu3->Remove(m_menuItemAdjustTimes);
    m_menu3->Insert(2, m_menuItem7);
    m_menu3->Insert(3, m_menuItemAdjustTimes);

#ifdef FFS_LINUX //file time adjustment not needed for Linux build
    m_menu3->Remove(m_menuItemAdjustTimes);
#endif

    //prepare drag & drop
    m_panel1->SetDropTarget(new FileDropEvent(this, m_panel1));
    m_panel2->SetDropTarget(new FileDropEvent(this, m_panel2));

    m_panel11->SetDropTarget(new FileDropEvent(this, m_panel1));
    m_panel12->SetDropTarget(new FileDropEvent(this, m_panel2));

    //initialize right-click context menu; will be dynamically re-created on each R-mouse-click
    contextMenu = new wxMenu;

    //support for CTRL + C and DEL
    m_gridLeft->Connect(wxEVT_KEY_DOWN, wxKeyEventHandler(MainDialog::onGridLeftButtonEvent), NULL, this);
    m_gridRight->Connect(wxEVT_KEY_DOWN, wxKeyEventHandler(MainDialog::onGridRightButtonEvent), NULL, this);
    m_gridMiddle->Connect(wxEVT_KEY_DOWN, wxKeyEventHandler(MainDialog::onGridMiddleButtonEvent), NULL, this);

    //identify leading grid by keyboard input or scroll action
    m_gridLeft->Connect(wxEVT_KEY_DOWN,               wxEventHandler(MainDialog::onGridLeftAccess), NULL, this);
    m_gridLeft->Connect(wxEVT_SCROLLWIN_TOP,          wxEventHandler(MainDialog::onGridLeftAccess), NULL, this);
    m_gridLeft->Connect(wxEVT_SCROLLWIN_BOTTOM,       wxEventHandler(MainDialog::onGridLeftAccess), NULL, this);
    m_gridLeft->Connect(wxEVT_SCROLLWIN_LINEUP,       wxEventHandler(MainDialog::onGridLeftAccess), NULL, this);
    m_gridLeft->Connect(wxEVT_SCROLLWIN_LINEDOWN,     wxEventHandler(MainDialog::onGridLeftAccess), NULL, this);
    m_gridLeft->Connect(wxEVT_SCROLLWIN_PAGEUP,       wxEventHandler(MainDialog::onGridLeftAccess), NULL, this);
    m_gridLeft->Connect(wxEVT_SCROLLWIN_PAGEDOWN,     wxEventHandler(MainDialog::onGridLeftAccess), NULL, this);
    m_gridLeft->Connect(wxEVT_SCROLLWIN_THUMBTRACK,   wxEventHandler(MainDialog::onGridLeftAccess), NULL, this);
    m_gridLeft->Connect(wxEVT_SCROLLWIN_THUMBRELEASE, wxEventHandler(MainDialog::onGridLeftAccess), NULL, this);
    m_gridLeft->Connect(wxEVT_GRID_LABEL_LEFT_CLICK,  wxEventHandler(MainDialog::onGridLeftAccess), NULL, this);
    m_gridLeft->GetGridWindow()->Connect(wxEVT_LEFT_DOWN, wxEventHandler(MainDialog::onGridLeftAccess), NULL, this);

    m_gridRight->Connect(wxEVT_KEY_DOWN,               wxEventHandler(MainDialog::onGridRightAccess), NULL, this);
    m_gridRight->Connect(wxEVT_SCROLLWIN_TOP,          wxEventHandler(MainDialog::onGridRightAccess), NULL, this);
    m_gridRight->Connect(wxEVT_SCROLLWIN_BOTTOM,       wxEventHandler(MainDialog::onGridRightAccess), NULL, this);
    m_gridRight->Connect(wxEVT_SCROLLWIN_LINEUP,       wxEventHandler(MainDialog::onGridRightAccess), NULL, this);
    m_gridRight->Connect(wxEVT_SCROLLWIN_LINEDOWN,     wxEventHandler(MainDialog::onGridRightAccess), NULL, this);
    m_gridRight->Connect(wxEVT_SCROLLWIN_PAGEUP,       wxEventHandler(MainDialog::onGridRightAccess), NULL, this);
    m_gridRight->Connect(wxEVT_SCROLLWIN_PAGEDOWN,     wxEventHandler(MainDialog::onGridRightAccess), NULL, this);
    m_gridRight->Connect(wxEVT_SCROLLWIN_THUMBTRACK,   wxEventHandler(MainDialog::onGridRightAccess), NULL, this);
    m_gridRight->Connect(wxEVT_SCROLLWIN_THUMBRELEASE, wxEventHandler(MainDialog::onGridRightAccess), NULL, this);
    m_gridRight->Connect(wxEVT_GRID_LABEL_LEFT_CLICK,  wxEventHandler(MainDialog::onGridRightAccess), NULL, this);
    m_gridRight->GetGridWindow()->Connect(wxEVT_LEFT_DOWN, wxEventHandler(MainDialog::onGridRightAccess), NULL, this);

    m_gridMiddle->Connect(wxEVT_KEY_DOWN,               wxEventHandler(MainDialog::onGridMiddleAccess), NULL, this);
    m_gridMiddle->Connect(wxEVT_SCROLLWIN_LINEUP,       wxEventHandler(MainDialog::onGridMiddleAccess), NULL, this);
    m_gridMiddle->Connect(wxEVT_SCROLLWIN_LINEDOWN,     wxEventHandler(MainDialog::onGridMiddleAccess), NULL, this);
    m_gridMiddle->GetGridWindow()->Connect(wxEVT_LEFT_DOWN, wxEventHandler(MainDialog::onGridMiddleAccess), NULL, this);

    Connect(wxEVT_IDLE, wxEventHandler(MainDialog::OnIdleEvent), NULL, this);
    m_gridMiddle->GetGridWindow()->Connect(wxEVT_LEFT_UP, wxEventHandler(MainDialog::OnGrid3LeftMouseUp), NULL, this);
    m_gridMiddle->GetGridWindow()->Connect(wxEVT_LEFT_DOWN, wxEventHandler(MainDialog::OnGrid3LeftMouseDown), NULL, this);

    Connect(wxEVT_SIZE, wxEventHandler(MainDialog::onResizeMainWindow), NULL, this);
    Connect(wxEVT_MOVE, wxEventHandler(MainDialog::onResizeMainWindow), NULL, this);

    const wxString header = _("Legend");
    wxString toolTip = header + wxT("\n") +
                       wxString().Pad(header.Len(), wxChar('-')) + wxT("\n") +
                       _("<|	file on left side only\n") +
                       _("|>	file on right side only\n") +
                       _("<<	left file is newer\n") +
                       _(">>	right file is newer\n") +
                       _("!=	files are different\n") +
                       _("==	files are equal\n\n") +
                       _("(-)	filtered out from sync-process\n");
    m_gridMiddle->GetGridWindow()->SetToolTip(toolTip);

    //enable parallel scrolling
    m_gridLeft->setScrollFriends(m_gridLeft, m_gridRight, m_gridMiddle);
    m_gridRight->setScrollFriends(m_gridLeft, m_gridRight, m_gridMiddle);
    m_gridMiddle->setScrollFriends(m_gridLeft, m_gridRight, m_gridMiddle);

    //share data with GUI grids
    m_gridLeft->setGridDataTable(&gridRefUI, &currentGridData);
    m_gridRight->setGridDataTable(&gridRefUI, &currentGridData);
    m_gridMiddle->setGridDataTable(&gridRefUI, &currentGridData);

    //disable sync button as long as "compare" hasn't been triggered.
    enableSynchronization(false);

    //make filesize right justified on grids
    wxGridCellAttr* cellAttributes = m_gridLeft->GetOrCreateCellAttr(0, 2);
    cellAttributes->SetAlignment(wxALIGN_RIGHT,wxALIGN_CENTRE);
    m_gridLeft->SetColAttr(2, cellAttributes);

    cellAttributes = m_gridRight->GetOrCreateCellAttr(0, 2);    //leave these two rows, might be necessary 'cause wxGridCellAttr is ref-counting
    cellAttributes->SetAlignment(wxALIGN_RIGHT,wxALIGN_CENTRE); //and SetColAttr takes ownership (means: it will call DecRef())
    m_gridRight->SetColAttr(2, cellAttributes);

    //as the name says: disable them
    m_gridMiddle->deactivateScrollbars();

    //mainly to update row label sizes...
    writeGrid(currentGridData);

    //select rows only
    m_gridLeft->SetSelectionMode(wxGrid::wxGridSelectRows);
    m_gridRight->SetSelectionMode(wxGrid::wxGridSelectRows);
    m_gridMiddle->SetSelectionMode(wxGrid::wxGridSelectRows);

    //set color of selections
    wxColour darkBlue(40, 35, 140);
    m_gridLeft->SetSelectionBackground(darkBlue);
    m_gridRight->SetSelectionBackground(darkBlue);
    m_gridMiddle->SetSelectionBackground(darkBlue);
    m_gridLeft->SetSelectionForeground(*wxWHITE);
    m_gridRight->SetSelectionForeground(*wxWHITE);
    m_gridMiddle->SetSelectionForeground(*wxWHITE);

    enableSynchronization(false);

    //initialize language selection
    switch (programLanguage->getLanguage())
    {
    case wxLANGUAGE_GERMAN:
        m_menuItemGerman->Check();
        break;
    case wxLANGUAGE_FRENCH:
        m_menuItemFrench->Check();
        break;
    case wxLANGUAGE_JAPANESE:
        m_menuItemJapanese->Check();
        break;
    case wxLANGUAGE_DUTCH:
        m_menuItemDutch->Check();
        break;
    case wxLANGUAGE_CHINESE_SIMPLIFIED:
        m_menuItemChineseSimple->Check();
        break;
    default:
        m_menuItemEnglish->Check();
    }

    //create the compare status panel in hidden state
    compareStatus = new CompareStatus(this);
    bSizer1->Insert(1, compareStatus, 0, wxEXPAND | wxBOTTOM, 5 );
    Layout();   //avoid screen flicker when panel is shown later
    compareStatus->Hide();

    //correct width of middle grid
    wxSize source = m_gridMiddle->GetSize();
    wxSize target = bSizerMiddle->GetSize();
    int spaceToAdd = source.GetX() - target.GetX();
    bSizerMiddle->Insert(1, spaceToAdd / 2, 0, 0);
    bSizerMiddle->Insert(0, spaceToAdd - (spaceToAdd / 2), 0, 0);
}


MainDialog::~MainDialog()
{
    m_gridLeft->setGridDataTable(0, 0);
    m_gridRight->setGridDataTable(0, 0);
    m_gridMiddle->setGridDataTable(0, 0);

    m_gridLeft->setSortMarker(-1);
    m_gridRight->setSortMarker(-1);

    //no need for event disconnect here; done automatically

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
                cfgFileHistory->DeleteEntry(key, false);
        }
    }
    delete cfgFileHistory;

    //save configuration
    writeConfigurationToXml(FreeFileSync::LAST_CONFIG_FILE);   //don't trow exceptions in destructors
    writeGlobalSettings();

    if (restartOnExit)  //this is needed so that restart happens AFTER configuration was written!
    {   //create new dialog
        MainDialog* frame = new MainDialog(NULL, FreeFileSync::LAST_CONFIG_FILE, programLanguage, globalSettings);
        frame->SetIcon(*globalResource.programIcon); //set application icon
        frame->Show();
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
    if (    widthNotMaximized != wxDefaultCoord &&
            heightNotMaximized != wxDefaultCoord &&
            posXNotMaximized != wxDefaultCoord &&
            posYNotMaximized != wxDefaultCoord)
        SetSize(posXNotMaximized, posYNotMaximized, widthNotMaximized, heightNotMaximized);

    Maximize(globalSettings.gui.isMaximized);

    //read column widths
    for (int i = 0; i < m_gridLeft->GetNumberCols() && i < int(globalSettings.gui.columnWidthLeft.size()); ++i)
        m_gridLeft->SetColSize(i, globalSettings.gui.columnWidthLeft[i]);

    for (int i = 0; i < m_gridRight->GetNumberCols() && i < int(globalSettings.gui.columnWidthRight.size()); ++i)
        m_gridRight->SetColSize(i, globalSettings.gui.columnWidthRight[i]);

/*    //read column positions
    for (int i = 0; i < m_gridLeft->GetNumberCols() && i < int(globalSettings.gui.columnPositionsLeft.size()); ++i)
    {
        const int newPosition = globalSettings.gui.columnPositionsLeft[i];
        if (0 <= newPosition && newPosition < m_gridLeft->GetNumberCols())
            m_gridLeft->SetColPos(i, newPosition);
    }

    for (int i = 0; i < m_gridRight->GetNumberCols() && i < int(globalSettings.gui.columnPositionsRight.size()); ++i)
    {
        const int newPosition = globalSettings.gui.columnPositionsRight[i];
        if (0 <= newPosition && newPosition < m_gridRight->GetNumberCols())
            m_gridRight->SetColPos(i, newPosition);
    }
    */
}


void MainDialog::writeGlobalSettings()
{
    //write global settings to (global) variable stored in application instance
    globalSettings.gui.widthNotMaximized  = widthNotMaximized;
    globalSettings.gui.heightNotMaximized = heightNotMaximized;
    globalSettings.gui.posXNotMaximized   = posXNotMaximized;
    globalSettings.gui.posYNotMaximized   = posYNotMaximized;
    globalSettings.gui.isMaximized        = IsMaximized();

    //write column widths
    globalSettings.gui.columnWidthLeft.clear();
    for (int i = 0; i < m_gridLeft->GetNumberCols(); ++i)
        globalSettings.gui.columnWidthLeft.push_back(m_gridLeft->GetColSize(i));

    globalSettings.gui.columnWidthRight.clear();
    for (int i = 0; i < m_gridRight->GetNumberCols(); ++i)
        globalSettings.gui.columnWidthRight.push_back(m_gridRight->GetColSize(i));

/*
    //write column positions
    globalSettings.gui.columnPositionsLeft.clear();
    for (int i = 0; i < m_gridLeft->GetNumberCols(); ++i)
        globalSettings.gui.columnPositionsLeft.push_back(m_gridLeft->GetColPos(i));

    globalSettings.gui.columnPositionsRight.clear();
    for (int i = 0; i < m_gridRight->GetNumberCols(); ++i)
        globalSettings.gui.columnPositionsRight.push_back(m_gridRight->GetColPos(i));*/
}


void MainDialog::onGridLeftAccess(wxEvent& event)
{
    if (leadGrid != m_gridLeft)
    {
        leadGrid = m_gridLeft;
        m_gridLeft->SetFocus();

        m_gridRight->ClearSelection();
    }
    event.Skip();
}


void MainDialog::onGridRightAccess(wxEvent& event)
{
    if (leadGrid != m_gridRight)
    {
        leadGrid = m_gridRight;
        m_gridRight->SetFocus();

        m_gridLeft->ClearSelection();
    }
    event.Skip();
}


void MainDialog::onGridMiddleAccess(wxEvent& event)
{
    if (leadGrid != m_gridMiddle)
    {
        leadGrid = m_gridMiddle;
        m_gridLeft->ClearSelection();
        m_gridRight->ClearSelection();
    }
    event.Skip();
}


void MainDialog::enableSynchronization(bool value)
{
    if (value)
    {
        synchronizationEnabled = true;
        m_bpButtonSync->SetBitmapLabel(*globalResource.bitmapSync);
    }
    else
    {
        synchronizationEnabled = false;
        m_bpButtonSync->SetBitmapLabel(*globalResource.bitmapSyncDisabled);
    }
}


void MainDialog::filterRangeTemp(const set<int>& rowsToFilterOnUI_View)
{
    if (rowsToFilterOnUI_View.size() > 0)
    {
        int gridSizeUI = gridRefUI.size();

        bool newSelection = false; //default: deselect range

        //leadingRow determines de-/selection of all other rows
        int leadingRow = *rowsToFilterOnUI_View.begin();
        if (0 <= leadingRow && leadingRow < gridSizeUI)
            newSelection = !currentGridData[gridRefUI[leadingRow]].selectedForSynchronization;

        if (hideFilteredElements)
            assert(!newSelection); //if hidefiltered is active, there should be no filtered elements on screen => current element was filtered out


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

            if (leadGrid)
                filterRangeTemp(getSelectedRows(leadGrid));
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
    const set<int> selectedRows = getSelectedRows(selectedGrid);
    if (selectedRows.size() > 0)
    {
        wxString clipboardString;

        for (set<int>::iterator i = selectedRows.begin(); i != selectedRows.end(); ++i)
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


set<int> MainDialog::getSelectedRows(const wxGrid* grid)
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
        for (int k = 0; k < const_cast<wxGrid*>(grid)->GetNumberRows(); ++k)
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


class DeleteErrorHandler : public ErrorHandler
{
public:
    DeleteErrorHandler(wxWindow* parentWindow, bool& unsolvedErrorOccured) :
            parent(parentWindow),
            ignoreErrors(false),
            unsolvedErrors(unsolvedErrorOccured) {}
    ~DeleteErrorHandler() {}

    Response reportError(const wxString& text)
    {
        if (ignoreErrors)
        {
            unsolvedErrors = true;
            return ErrorHandler::IGNORE_ERROR;
        }

        bool ignoreNextErrors = false;
        wxString errorMessage = text + wxT("\n\n") + _("Information: If you ignore the error or abort a re-compare will be necessary!");
        ErrorDlg* errorDlg = new ErrorDlg(parent, errorMessage, ignoreNextErrors);

        int rv = errorDlg->ShowModal();
        switch (rv)
        {
        case ErrorDlg::BUTTON_IGNORE:
            ignoreErrors = ignoreNextErrors;
            unsolvedErrors = true;
            return ErrorHandler::IGNORE_ERROR;
        case ErrorDlg::BUTTON_RETRY:
            return ErrorHandler::RETRY;
        case ErrorDlg::BUTTON_ABORT:
        {
            unsolvedErrors = true;
            throw AbortThisProcess();
        }
        default:
            assert (false);
        }

        return ErrorHandler::IGNORE_ERROR; //dummy return value
    }
private:

    wxWindow* parent;
    bool ignoreErrors;
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

            if (currentCmpLine.fileDescrLeft.objType != FileDescrLine::TYPE_NOTHING)
                filesToDelete += (currentCmpLine.fileDescrLeft.fullName + wxT("\n")).c_str();

            if (currentCmpLine.fileDescrRight.objType != FileDescrLine::TYPE_NOTHING)
                filesToDelete += (currentCmpLine.fileDescrRight.fullName + wxT("\n")).c_str();

            filesToDelete+= wxT("\n");
        }

        DeleteDialog* confirmDeletion = new DeleteDialog(headerText, filesToDelete, this); //no destruction needed; attached to main window

        switch (confirmDeletion->ShowModal())
        {
        case DeleteDialog::BUTTON_OKAY:
        {
            bool unsolvedErrorOccured = false; //if an error is skipped a re-compare will be necessary!

            try
            {   //handle errors when deleting files/folders
                DeleteErrorHandler errorHandler(this, unsolvedErrorOccured);

                FreeFileSync::deleteOnGridAndHD(currentGridData, rowsToDeleteOnGrid, &errorHandler, cfg.useRecycleBin);
            }
            catch (AbortThisProcess& theException)
                {}

            //disable the sync button if errors occured during deletion
            if (unsolvedErrorOccured)
                enableSynchronization(false);

            //redraw grid neccessary to update new dimensions and for UI-Backend data linkage
            writeGrid(currentGridData);

            m_gridLeft->ClearSelection();
            m_gridRight->ClearSelection();
            m_gridMiddle->ClearSelection();
        }
        break;

        case DeleteDialog::BUTTON_CANCEL:
        default:
            break;
        }
    }
}


void MainDialog::openWithFileBrowser(int rowNumber, const wxGrid* grid)
{
#ifdef FFS_WIN
    if (grid == m_gridLeft)
    {
        wxString command = wxString(wxT("explorer ")) + FreeFileSync::getFormattedDirectoryName(m_directoryLeft->GetValue()).c_str(); //default

        if (0 <= rowNumber && rowNumber < int(gridRefUI.size()))
        {
            const FileDescrLine& fileDescr = currentGridData[gridRefUI[rowNumber]].fileDescrLeft;
            if (fileDescr.objType != FileDescrLine::TYPE_NOTHING)
                command = wxString(wxT("explorer /select,")) + fileDescr.fullName.c_str();
        }
        wxExecute(command);
    }
    else if (grid == m_gridRight)
    {
        wxString command = wxString(wxT("explorer ")) + FreeFileSync::getFormattedDirectoryName(m_directoryRight->GetValue()).c_str(); //default

        if (0 <= rowNumber && rowNumber < int(gridRefUI.size()))
        {
            const FileDescrLine& fileDescr = currentGridData[gridRefUI[rowNumber]].fileDescrRight;
            if (fileDescr.objType != FileDescrLine::TYPE_NOTHING)
                command = wxString(wxT("explorer /select,")) + fileDescr.fullName.c_str();
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


void MainDialog::onGridLeftButtonEvent(wxKeyEvent& event)
{
    //CTRL + C || CTRL + INS
    if (    event.ControlDown() && event.GetKeyCode() == 67 ||
            event.ControlDown() && event.GetKeyCode() == WXK_INSERT)
        copySelectionToClipboard(m_gridLeft);

    else if (event.GetKeyCode() == WXK_DELETE)
        deleteFilesOnGrid(getSelectedRows(m_gridLeft));

    event.Skip();
}


void MainDialog::onGridRightButtonEvent(wxKeyEvent& event)
{
    //CTRL + C || CTRL + INS
    if (    event.ControlDown() && event.GetKeyCode() == 67 ||
            event.ControlDown() && event.GetKeyCode() == WXK_INSERT)
        copySelectionToClipboard(m_gridRight);

    else if (event.GetKeyCode() == WXK_DELETE)
        deleteFilesOnGrid(getSelectedRows(m_gridRight));

    event.Skip();
}


void MainDialog::onGridMiddleButtonEvent(wxKeyEvent& event)
{
    //CTRL + C || CTRL + INS
    if (    event.ControlDown() && event.GetKeyCode() == 67 ||
            event.ControlDown() && event.GetKeyCode() == WXK_INSERT)
        copySelectionToClipboard(m_gridMiddle);

    else if (event.GetKeyCode() == WXK_DELETE)
        deleteFilesOnGrid(getSelectedRows(m_gridMiddle));

    event.Skip();
}


void MainDialog::OnOpenContextMenu(wxGridEvent& event)
{
    set<int> selection;

    if (leadGrid)
        selection = getSelectedRows(leadGrid);

    exFilterCandidateExtension.Clear();
    exFilterCandidateObj.clear();

//#######################################################
//re-create context menu
    delete contextMenu;
    contextMenu = new wxMenu;

    //dynamic filter determination
    if (selection.size() > 0)
    {
        const FileCompareLine& firstLine = currentGridData[gridRefUI[*selection.begin()]];

        if (firstLine.selectedForSynchronization)
            contextMenu->Append(CONTEXT_FILTER_TEMP, _("Exclude temporarily"));
        else
            contextMenu->Append(CONTEXT_FILTER_TEMP, _("Include temporarily"));

        //get list of relative file/dir-names into vectors
        FilterObject newFilterEntry;
        if (leadGrid == m_gridLeft)
            for (set<int>::iterator i = selection.begin(); i != selection.end(); ++i)
            {
                const FileCompareLine& line = currentGridData[gridRefUI[*i]];
                newFilterEntry.relativeName = line.fileDescrLeft.relativeName.c_str();
                newFilterEntry.type         = line.fileDescrLeft.objType;
                if (!newFilterEntry.relativeName.IsEmpty())
                    exFilterCandidateObj.push_back(newFilterEntry);
            }
        else if (leadGrid == m_gridRight)
            for (set<int>::iterator i = selection.begin(); i != selection.end(); ++i)
            {
                const FileCompareLine& line = currentGridData[gridRefUI[*i]];
                newFilterEntry.relativeName = line.fileDescrRight.relativeName.c_str();
                newFilterEntry.type         = line.fileDescrRight.objType;
                if (!newFilterEntry.relativeName.IsEmpty())
                    exFilterCandidateObj.push_back(newFilterEntry);
            }


        if (exFilterCandidateObj.size() > 0 && exFilterCandidateObj[0].type == FileDescrLine::TYPE_FILE)
        {
            const wxString filename = exFilterCandidateObj[0].relativeName.AfterLast(GlobalResources::FILE_NAME_SEPARATOR);
            if (filename.Find(wxChar('.')) !=  wxNOT_FOUND) //be careful: AfterLast will return the whole string if '.' is not found!
            {
                exFilterCandidateExtension = filename.AfterLast(wxChar('.'));
                contextMenu->Append(CONTEXT_EXCLUDE_EXT, wxString(_("Add to exclude filter:")) + wxT(" ") + wxT("*.") + exFilterCandidateExtension);
            }
        }

        if (exFilterCandidateObj.size() == 1)
            contextMenu->Append(CONTEXT_EXCLUDE_OBJ, wxString(_("Add to exclude filter:")) + wxT(" ") + exFilterCandidateObj[0].relativeName.AfterLast(GlobalResources::FILE_NAME_SEPARATOR));
        else if (exFilterCandidateObj.size() > 1)
            contextMenu->Append(CONTEXT_EXCLUDE_OBJ, wxString(_("Add to exclude filter:")) + wxT(" ") + _("<multiple selection>"));
    }
    else
        contextMenu->Append(CONTEXT_FILTER_TEMP, _("Exclude temporarily")); //this element should always be visible

    contextMenu->AppendSeparator();
    contextMenu->Append(CONTEXT_CLIPBOARD, _("Copy to clipboard\tCTRL+C"));
#ifdef FFS_WIN
    contextMenu->Append(CONTEXT_EXPLORER, _("Open with Explorer\tD-Click"));
#endif
    contextMenu->AppendSeparator();
    contextMenu->Append(CONTEXT_DELETE_FILES, _("Delete files\tDEL"));

    contextMenu->Connect(wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(MainDialog::onContextMenuSelection), NULL, this);

//#######################################################
//enable/disable context menu entries
    if (selection.size() > 0)
    {
        contextMenu->Enable(CONTEXT_FILTER_TEMP, true);
        contextMenu->Enable(CONTEXT_CLIPBOARD, true);
        contextMenu->Enable(CONTEXT_DELETE_FILES, true);
    }
    else
    {
        contextMenu->Enable(CONTEXT_FILTER_TEMP, false);
        contextMenu->Enable(CONTEXT_CLIPBOARD, false);
        contextMenu->Enable(CONTEXT_DELETE_FILES, false);
    }

#ifdef FFS_WIN
    if ((leadGrid == m_gridLeft || leadGrid == m_gridRight) && selection.size() <= 1)
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
    int eventId = event.GetId();
    if (eventId == CONTEXT_FILTER_TEMP)
    {
        if (leadGrid)
        {
            set<int> selection = getSelectedRows(leadGrid);
            filterRangeTemp(selection);
        }
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

            FreeFileSync::filterCurrentGridData(currentGridData, cfg.includeFilter, cfg.excludeFilter);

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
            for (vector<FilterObject>::const_iterator i = exFilterCandidateObj.begin(); i != exFilterCandidateObj.end(); ++i)
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

            FreeFileSync::filterCurrentGridData(currentGridData, cfg.includeFilter, cfg.excludeFilter);

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
        if (leadGrid)
            copySelectionToClipboard(leadGrid);
    }
    else if (eventId == CONTEXT_EXPLORER)
    {
        if (leadGrid == m_gridLeft || leadGrid == m_gridRight)
        {
            set<int> selection = getSelectedRows(leadGrid);

            if (selection.size() == 1)
                openWithFileBrowser(*selection.begin(), leadGrid);
            else if (selection.size() == 0)
                openWithFileBrowser(-1, leadGrid);
        }
    }
    else if (eventId == CONTEXT_DELETE_FILES)
    {
        if (leadGrid)
        {
            set<int> selection = getSelectedRows(leadGrid);
            deleteFilesOnGrid(selection);
        }
    }

    event.Skip();
}


void MainDialog::OnColumnMenuLeft(wxGridEvent& event)
{
    event.Skip();
}


void MainDialog::OnColumnMenuRight(wxGridEvent& event)
{
    event.Skip();
}


void MainDialog::OnWriteDirManually(wxCommandEvent& event)
{
    wxString newDir = FreeFileSync::getFormattedDirectoryName(event.GetString());
    if (wxDirExists(newDir))
    {
        wxObject* eventObj = event.GetEventObject();

        //first check if event comes from main folder pair
        if (eventObj == (wxObject*)m_directoryLeft)
            m_dirPickerLeft->SetPath(newDir);
        else if (eventObj == (wxObject*)m_directoryRight)
            m_dirPickerRight->SetPath(newDir);
        else
        {
            //check if event comes from additional pairs
            for (vector<FolderPairGenerated*>::const_iterator i = additionalFolderPairs.begin(); i != additionalFolderPairs.end(); ++i)
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
        m_directoryLeft->SetValue(newPath);
    else if (eventObj == (wxObject*)m_dirPickerRight)
        m_directoryRight->SetValue(newPath);
    else //check if event comes from additional pairs
    {
        for (vector<FolderPairGenerated*>::const_iterator i = additionalFolderPairs.begin(); i != additionalFolderPairs.end(); ++i)
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
bool sameFileSpecified(const wxString& file1, const wxString& file2)
{
    wxString file1Full = file1;
    wxString file2Full = file2;

    if (wxFileName(file1).GetPath() == wxEmptyString)
        file1Full = wxFileName::GetCwd() + GlobalResources::FILE_NAME_SEPARATOR + file1;

    if (wxFileName(file2).GetPath() == wxEmptyString)
        file2Full = wxFileName::GetCwd() + GlobalResources::FILE_NAME_SEPARATOR + file2;

    return (file1Full == file2Full);
}


void MainDialog::addCfgFileToHistory(const wxString& filename)
{
    //the default configFile should not be in the history
    if (sameFileSpecified(FreeFileSync::LAST_CONFIG_FILE, filename))
        return;

    //only still existing files should be included in the list
    if (!wxFileExists(filename))
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

    if (duplicateEntry) //if entry is in the list, then jump to element
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
    if (!filenames.IsEmpty())
    {
        //disable the sync button
        mainDlg->enableSynchronization(false);

        //clear grids
        mainDlg->currentGridData.clear();
        mainDlg->writeGrid(mainDlg->currentGridData);

        const wxString droppedFileName = filenames[0];

        //test if ffs config file has been dropped
        if (xmlAccess::getXmlType(droppedFileName) == XML_GUI_CONFIG)
        {
            if (mainDlg->readConfigurationFromXml(droppedFileName))
                mainDlg->pushStatusInformation(_("Configuration loaded!"));
        }

        //test if main folder pair is drop target
        else if (dropTarget == mainDlg->m_panel1)
            onFilesDropped(droppedFileName, mainDlg->m_directoryLeft, mainDlg->m_dirPickerLeft);

        else if (dropTarget == mainDlg->m_panel2)
            onFilesDropped(droppedFileName, mainDlg->m_directoryRight, mainDlg->m_dirPickerRight);

        else //test if additional folder pairs are drop targets
        {
            for (vector<FolderPairGenerated*>::const_iterator i = mainDlg->additionalFolderPairs.begin(); i != mainDlg->additionalFolderPairs.end(); ++i)
            {
                FolderPairGenerated* dirPair = *i;
                if (dropTarget == (dirPair->m_panelLeft))
                {
                    onFilesDropped(droppedFileName, dirPair->m_directoryLeft, dirPair->m_dirPickerLeft);
                    break;
                }
                else if (dropTarget == (dirPair->m_panelRight))
                {
                    onFilesDropped(droppedFileName, dirPair->m_directoryRight, dirPair->m_dirPickerRight);
                    break;
                }
            }
        }
    }
    return false;
}


void MainDialog::OnSaveConfig(wxCommandEvent& event)
{
    wxString defaultFileName = wxT("SyncSettings.ffs_gui");

    //try to use currently selected configuration file as default
    int selectedItem;
    if ((selectedItem = m_choiceLoad->GetSelection()) != wxNOT_FOUND)
        if (1 <= selectedItem && unsigned(selectedItem) < m_choiceLoad->GetCount())
            if (unsigned(selectedItem - 1) < cfgFileNames.size())
                defaultFileName = cfgFileNames[selectedItem - 1];


    wxFileDialog* filePicker = new wxFileDialog(this, wxEmptyString, wxEmptyString, defaultFileName, wxString(_("FreeFileSync configuration")) + wxT(" (*.ffs_gui)|*.ffs_gui"), wxFD_SAVE);
    if (filePicker->ShowModal() == wxID_OK)
    {
        wxString newFileName = filePicker->GetPath();

        if (wxFileExists(newFileName))
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
    event.Skip();
}


void MainDialog::OnLoadConfig(wxCommandEvent& event)
{
    int selectedItem;
    if ((selectedItem = m_choiceLoad->GetSelection()) != wxNOT_FOUND)
    {
        wxFileDialog* filePicker = NULL;
        switch (selectedItem)
        {
        case 0: //load config from file
            filePicker = new wxFileDialog(this, wxEmptyString, wxEmptyString, wxEmptyString, wxString(_("FreeFileSync configuration")) + wxT(" (*.ffs_gui)|*.ffs_gui"), wxFD_OPEN);

            if (filePicker->ShowModal() == wxID_OK)
                loadConfiguration(filePicker->GetPath());
            break;

        default:
            if (1 <= selectedItem && unsigned(selectedItem) < m_choiceLoad->GetCount())
            {
                if (unsigned(selectedItem - 1) < cfgFileNames.size())
                    loadConfiguration(cfgFileNames[selectedItem - 1]);
            }
            break;
        }
    }
    event.Skip();
}


void MainDialog::OnMenuSaveConfig(wxCommandEvent& event)
{
    OnSaveConfig(event);
    event.Skip();
}


void MainDialog::OnMenuLoadConfig(wxCommandEvent& event)
{
    wxFileDialog* filePicker = new wxFileDialog(this, wxEmptyString, wxEmptyString, wxEmptyString, wxString(_("FreeFileSync configuration")) + wxT(" (*.ffs_gui)|*.ffs_gui"), wxFD_OPEN);
    if (filePicker->ShowModal() == wxID_OK)
        loadConfiguration(filePicker->GetPath());
}


void MainDialog::loadConfiguration(const wxString& filename)
{
    if (!filename.IsEmpty())
    {
        if (!wxFileExists(filename))
            wxMessageBox(wxString(_("The selected file does not exist:")) + wxT(" \"") + filename + wxT("\""), _("Warning"), wxOK);
        else if (xmlAccess::getXmlType(filename) != XML_GUI_CONFIG)
            wxMessageBox(wxString(_("The file does not contain a valid configuration:")) + wxT(" \"") + filename + wxT("\""), _("Warning"), wxOK);
        else
        {   //clear grids
            currentGridData.clear();
            writeGrid(currentGridData);

            if (readConfigurationFromXml(filename))
                pushStatusInformation(_("Configuration loaded!"));
        }
    }
}


void MainDialog::OnChoiceKeyEvent(wxKeyEvent& event)
{
    if (event.GetKeyCode() == WXK_DELETE)
    {   //try to delete the currently selected config history item
        int selectedItem;
        if ((selectedItem = m_choiceLoad->GetCurrentSelection()) != wxNOT_FOUND)
            if (1 <= selectedItem && unsigned(selectedItem) < m_choiceLoad->GetCount())
                if (unsigned(selectedItem - 1) < cfgFileNames.size())
                {   //delete selected row
                    cfgFileNames.erase(cfgFileNames.begin() + selectedItem - 1);
                    m_choiceLoad->Delete(selectedItem);
                    m_choiceLoad->SetSelection(0);
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


bool MainDialog::readConfigurationFromXml(const wxString& filename, bool programStartup)
{
    //load XML
    XmlGuiConfig guiCfg;  //structure to receive gui settings, already defaulted!!
    try
    {
        guiCfg = xmlAccess::readGuiConfig(filename);
    }
    catch (const FileError& error)
    {
        if (programStartup && filename == FreeFileSync::LAST_CONFIG_FILE && !wxFileExists(filename)) //do not show error in this case
            ;
        else if (!programStartup)
        {
            wxMessageBox(error.show(), _("Error"), wxOK | wxICON_ERROR);
            return false;
        }
        else //program startup: show error message and load defaults
            wxMessageBox(error.show(), _("Error"), wxOK | wxICON_ERROR);
    }

    //load main configuration into instance
    cfg = guiCfg.mainCfg;

    //update visible config on main window
    updateCompareButtons();
    updateFilterButton(m_bpButtonFilter, cfg.filterIsActive);

    //read folder pairs:
    //clear existing pairs first
    m_directoryLeft->SetValue(wxEmptyString);
    m_dirPickerLeft->SetPath(wxEmptyString);
    m_directoryRight->SetValue(wxEmptyString);
    m_dirPickerRight->SetPath(wxEmptyString);

    removeFolderPair(true);

    //set main folder pair
    const unsigned int folderPairCount = guiCfg.directoryPairs.size();
    if (folderPairCount > 0)
    {
        vector<FolderPair>::const_iterator i = guiCfg.directoryPairs.begin();

        m_directoryLeft->SetValue(i->leftDirectory);
        wxString leftDirFormatted = FreeFileSync::getFormattedDirectoryName(i->leftDirectory);
        if (wxDirExists(leftDirFormatted))
            m_dirPickerLeft->SetPath(leftDirFormatted);

        m_directoryRight->SetValue(i->rightDirectory);
        wxString rightDirFormatted = FreeFileSync::getFormattedDirectoryName(i->rightDirectory);
        if (wxDirExists(rightDirFormatted))
            m_dirPickerRight->SetPath(rightDirFormatted);

        //set additional pairs
        for (vector<FolderPair>::const_iterator i = guiCfg.directoryPairs.begin() + 1; i != guiCfg.directoryPairs.end(); ++i)
            addFolderPair(i->leftDirectory, i->rightDirectory);

        //adjust folder pair buttons
        const int additionalFolderCount = folderPairCount - 1;
        if (additionalFolderCount <= 0)
            m_bpButtonRemovePair->Disable();
    }

    //read GUI layout (optional!)
    hideFilteredElements = guiCfg.hideFilteredElements;
    m_checkBoxHideFilt->SetValue(hideFilteredElements);


    //###########################################################
    addCfgFileToHistory(filename); //put filename on list of last used config files
    return true;
}


bool MainDialog::writeConfigurationToXml(const wxString& filename)
{
    XmlGuiConfig guiCfg;

    //load structure with basic settings "mainCfg"
    guiCfg.mainCfg = cfg;
    getFolderPairs(guiCfg.directoryPairs);

    //load structure with gui settings
    guiCfg.hideFilteredElements = hideFilteredElements;

    //write config to XML
    try
    {
        xmlAccess::writeGuiConfig(filename, guiCfg);
    }
    catch (const FileError& error)
    {
        wxMessageBox(error.show(), _("Error"), wxOK | wxICON_ERROR);
        return false;
    }

    //put filename on list of last used config files
    addCfgFileToHistory(filename);
    return true;
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
    hideFilteredElements = !hideFilteredElements;
    //make sure, checkbox and "hideFiltered" are in sync
    m_checkBoxHideFilt->SetValue(hideFilteredElements);

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


void MainDialog::getFolderPairs(vector<FolderPair>& output, bool formatted)
{
    output.clear();

    //add main pair
    FolderPair newPair;
    if (formatted)
    {
        newPair.leftDirectory  = FreeFileSync::getFormattedDirectoryName(m_directoryLeft->GetValue()).c_str();
        newPair.rightDirectory = FreeFileSync::getFormattedDirectoryName(m_directoryRight->GetValue()).c_str();
    }
    else
    {
        newPair.leftDirectory  = m_directoryLeft->GetValue().c_str();
        newPair.rightDirectory = m_directoryRight->GetValue().c_str();
    }
    output.push_back(newPair);

    //add additional pairs
    for (vector<FolderPairGenerated*>::const_iterator i = additionalFolderPairs.begin(); i != additionalFolderPairs.end(); ++i)
    {
        FolderPairGenerated* dirPair = *i;
        if (formatted)
        {
            newPair.leftDirectory  = FreeFileSync::getFormattedDirectoryName(dirPair->m_directoryLeft->GetValue());
            newPair.rightDirectory = FreeFileSync::getFormattedDirectoryName(dirPair->m_directoryRight->GetValue());
        }
        else
        {
            newPair.leftDirectory  = dirPair->m_directoryLeft->GetValue().c_str();
            newPair.rightDirectory = dirPair->m_directoryRight->GetValue().c_str();
        }

        output.push_back(newPair);
    }
}


void MainDialog::OnCompare(wxCommandEvent &event)
{
    //assemble vector of formatted folder pairs
    vector<FolderPair> directoryPairsFormatted;
    getFolderPairs(directoryPairsFormatted, true);

    //check if folders are valid
    wxString errorMessage;
    if (!FreeFileSync::foldersAreValidForComparison(directoryPairsFormatted, errorMessage))
    {
        wxMessageBox(errorMessage, _("Warning"));
        return;
    }

    //check if folders have dependencies
    if (globalSettings.global.folderDependCheckActive)
    {
        wxString warningMessage;
        if (FreeFileSync::foldersHaveDependencies(directoryPairsFormatted, warningMessage))
        {
            bool hideThisDialog = false;
            wxString messageText = warningMessage + wxT("\n\n") +
                                   _("Consider this when setting up synchronization rules: You might want to avoid write access to these directories so that synchronization of both does not interfere.");

            //show popup and ask user how to handle warning
            WarningDlg* warningDlg = new WarningDlg(this, WarningDlg::BUTTON_IGNORE | WarningDlg::BUTTON_ABORT, messageText, hideThisDialog);
            if (warningDlg->ShowModal() == WarningDlg::BUTTON_ABORT)
                return;
            else
                globalSettings.global.folderDependCheckActive = !hideThisDialog;
        }
    }
//----------------------------------------------

    clearStatusBar();

    wxBusyCursor dummy; //show hourglass cursor

    bool aborted = false;
    try
    {   //class handling status display and error messages
        CompareStatusHandler statusHandler(this);
        cmpStatusHandlerTmp = &statusHandler;

        FreeFileSync::CompareProcess comparison(false, &statusHandler);
        comparison.startCompareProcess(directoryPairsFormatted,
                                       cfg.compareVar,
                                       currentGridData);

        //filter currentGridData if option is set
        if (cfg.filterIsActive)
            FreeFileSync::filterCurrentGridData(currentGridData, cfg.includeFilter, cfg.excludeFilter);

        writeGrid(currentGridData); //keep it in try/catch to not overwrite status information if compare is aborted

#ifdef FFS_WIN
        //check if DST time correction needs to be applied
        if (globalSettings.global.dstCheckActive)
        {
            int timeShift = 0;
            wxString driveName;
            FreeFileSync::checkForDSTChange(currentGridData, directoryPairsFormatted, timeShift, driveName);
            if (timeShift)
            {
                bool hideThisDialog = false;
                wxString errorMessage = wxString(_("A file time shift due to a daylight saving time change was detected for a FAT/FAT32 drive.")) + wxT("\n")
                                        + _("You can adjust the file times accordingly to resolve the issue:");
                errorMessage+= wxString(wxT("\n\n")) + _("Drive:") + wxT(" ") + driveName + wxT("\n")
                               + _("Time shift:") + wxT(" ") + globalFunctions::numberToWxString(timeShift);

                //show popup and ask user how to handle the DST change
                WarningDlg* warningDlg = new WarningDlg(this, WarningDlg::BUTTON_RESOLVE | WarningDlg::BUTTON_IGNORE, errorMessage, hideThisDialog);
                warningDlg->m_bitmap10->SetBitmap(*globalResource.bitmapClock);
                if (warningDlg->ShowModal() == WarningDlg::BUTTON_RESOLVE)
                {
                    ModifyFilesDlg* modifyDlg = new ModifyFilesDlg(this, driveName, timeShift);
                    if (modifyDlg->ShowModal() == ModifyFilesDlg::BUTTON_APPLY)
                        throw AbortThisProcess();
                }
                else
                    globalSettings.global.dstCheckActive = !hideThisDialog;
            }
        }
#endif  //FFS_WIN
    }
    catch (AbortThisProcess& theException)
    {
        aborted = true;
    }
    cmpStatusHandlerTmp = 0;

    if (aborted)
    {   //disable the sync button
        enableSynchronization(false);
        m_bpButtonCompare->SetFocus();
    }
    else
    {   //once compare is finished enable the sync button
        enableSynchronization(true);
        m_bpButtonSync->SetFocus();

        //hide sort direction indicator on GUI grids
        m_gridLeft->setSortMarker(-1);
        m_gridRight->setSortMarker(-1);
    }

    event.Skip();
}


void MainDialog::OnAbortCompare(wxCommandEvent& event)
{
    if (cmpStatusHandlerTmp)
        cmpStatusHandlerTmp->requestAbortion();
    event.Skip();
}


void MainDialog::writeGrid(const FileCompareResult& gridData)
{
    m_gridLeft->BeginBatch();
    m_gridRight->BeginBatch();
    m_gridMiddle->BeginBatch();

    mapGridDataToUI(gridRefUI, gridData);  //update gridRefUI
    updateStatusInformation(gridRefUI);    //write status information for gridRefUI

    //all three grids retrieve their data directly via gridRefUI!!!
    //the only thing left to do is notify the grids to update their sizes (nr of rows), since this has to be communicated by the grids via messages
    m_gridLeft->updateGridSizes();
    m_gridRight->updateGridSizes();
    m_gridMiddle->updateGridSizes();

    //enlarge label width to display row numbers correctly
    int nrOfRows = m_gridLeft->GetNumberRows();
    if (nrOfRows >= 1)
    {
        int nrOfDigits = int(floor(log10(double(nrOfRows)) + 1));
        m_gridLeft->SetRowLabelSize(nrOfDigits * 8 + 4);
        m_gridRight->SetRowLabelSize(nrOfDigits * 8 + 4);
    }

    m_gridLeft->EndBatch();
    m_gridRight->EndBatch();
    m_gridMiddle->EndBatch();
}


void MainDialog::OnSync(wxCommandEvent& event)
{
    SyncDialog* syncDlg = new SyncDialog(this, currentGridData, cfg, synchronizationEnabled);
    if (syncDlg->ShowModal() == SyncDialog::BUTTON_START)
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
            wxMessageBox(_("Nothing to synchronize according to configuration!"), _("Information"), wxICON_WARNING);
            return;
        }


        wxBusyCursor dummy; //show hourglass cursor

        clearStatusBar();
        try
        {
            //class handling status updates and error messages
            SyncStatusHandler statusHandler(this, cfg.ignoreErrors);

            //start synchronization and return elements that were not sync'ed in currentGridData
            FreeFileSync::SyncProcess synchronization(cfg.useRecycleBin, true, &statusHandler);
            synchronization.startSynchronizationProcess(currentGridData, cfg.syncConfiguration);
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
    }
    event.Skip();
}


void MainDialog::OnLeftGridDoubleClick(wxGridEvent& event)
{
    openWithFileBrowser(event.GetRow(), m_gridLeft);
    event.Skip();
}


void MainDialog::OnRightGridDoubleClick(wxGridEvent& event)
{
    openWithFileBrowser(event.GetRow(), m_gridRight);
    event.Skip();
}


void MainDialog::OnSortLeftGrid(wxGridEvent& event)
{
    static bool columnSortAscending[4] = {true, true, false, true};

    int currentSortColumn = event.GetCol();
    if (0 <= currentSortColumn && currentSortColumn <= 3)
    {
        bool& sortAscending = columnSortAscending[currentSortColumn];

        if (currentSortColumn == 0)
        {
            if (sortAscending) sort(currentGridData.begin(), currentGridData.end(), sortByFileName<true, SORT_ON_LEFT>);
            else               sort(currentGridData.begin(), currentGridData.end(), sortByFileName<false, SORT_ON_LEFT>);
        }
        else if (currentSortColumn == 1)
        {
            if (sortAscending) sort(currentGridData.begin(), currentGridData.end(), sortByRelativeName<true, SORT_ON_LEFT>);
            else               sort(currentGridData.begin(), currentGridData.end(), sortByRelativeName<false, SORT_ON_LEFT>);
        }
        else if (currentSortColumn == 2)
        {
            if (sortAscending) sort(currentGridData.begin(), currentGridData.end(), sortByFileSize<true, SORT_ON_LEFT>);
            else               sort(currentGridData.begin(), currentGridData.end(), sortByFileSize<false, SORT_ON_LEFT>);
        }
        else if (currentSortColumn == 3)
        {
            if (sortAscending) sort(currentGridData.begin(), currentGridData.end(), sortByDate<true, SORT_ON_LEFT>);
            else               sort(currentGridData.begin(), currentGridData.end(), sortByDate<false, SORT_ON_LEFT>);
        }

        writeGrid(currentGridData); //needed to refresh gridRefUI references

        //set sort direction indicator on UI
        if (sortAscending)
            m_gridLeft->setSortMarker(currentSortColumn, globalResource.bitmapSmallUp);
        else
            m_gridLeft->setSortMarker(currentSortColumn, globalResource.bitmapSmallDown);
        m_gridRight->setSortMarker(-1);

        sortAscending = !sortAscending;
    }
    event.Skip();
}


void MainDialog::OnSortRightGrid(wxGridEvent& event)
{
    static bool columnSortAscending[4] = {true, true, false, true};

    int currentSortColumn = event.GetCol();
    if (0 <= currentSortColumn && currentSortColumn <= 3)
    {
        bool& sortAscending = columnSortAscending[currentSortColumn];

        if (currentSortColumn == 0)
        {
            if (sortAscending) sort(currentGridData.begin(), currentGridData.end(), sortByFileName<true, SORT_ON_RIGHT>);
            else               sort(currentGridData.begin(), currentGridData.end(), sortByFileName<false, SORT_ON_RIGHT>);
        }
        else if (currentSortColumn == 1)
        {
            if (sortAscending) sort(currentGridData.begin(), currentGridData.end(), sortByRelativeName<true, SORT_ON_RIGHT>);
            else               sort(currentGridData.begin(), currentGridData.end(), sortByRelativeName<false, SORT_ON_RIGHT>);
        }
        else if (currentSortColumn == 2)
        {
            if (sortAscending) sort(currentGridData.begin(), currentGridData.end(), sortByFileSize<true, SORT_ON_RIGHT>);
            else               sort(currentGridData.begin(), currentGridData.end(), sortByFileSize<false, SORT_ON_RIGHT>);
        }
        else if (currentSortColumn == 3)
        {
            if (sortAscending) sort(currentGridData.begin(), currentGridData.end(), sortByDate<true, SORT_ON_RIGHT>);
            else               sort(currentGridData.begin(), currentGridData.end(), sortByDate<false, SORT_ON_RIGHT>);
        }

        writeGrid(currentGridData); //needed to refresh gridRefUI references

        //set sort direction indicator on UI
        m_gridLeft->setSortMarker(-1);
        if (sortAscending)
            m_gridRight->setSortMarker(currentSortColumn, globalResource.bitmapSmallUp);
        else
            m_gridRight->setSortMarker(currentSortColumn, globalResource.bitmapSmallDown);

        sortAscending = !sortAscending;
    }
    event.Skip();
}


void MainDialog::OnSwapDirs( wxCommandEvent& event )
{
    //swap directory names : main pair
    wxString tmp = m_directoryLeft->GetValue();
    m_directoryLeft->SetValue(m_directoryRight->GetValue());
    m_directoryRight->SetValue(tmp);

    //additional pairs
    for (vector<FolderPairGenerated*>::const_iterator i = additionalFolderPairs.begin(); i != additionalFolderPairs.end(); ++i)
    {
        FolderPairGenerated* dirPair = *i;
        tmp = dirPair->m_directoryLeft->GetValue();
        dirPair->m_directoryLeft->SetValue(dirPair->m_directoryRight->GetValue());
        dirPair->m_directoryRight->SetValue(tmp);
    }

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
            wxString folderCount = numberToWxString(foldersOnLeftView);
            globalFunctions::includeNumberSeparator(folderCount);

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
            wxString fileCount = numberToWxString(filesOnLeftView);
            globalFunctions::includeNumberSeparator(fileCount);

            wxString outputString = _("%x files,");
            outputString.Replace(wxT("%x"), fileCount, false);
            statusLeftNew+= outputString;
        }
        statusLeftNew+= wxT(" ");
        statusLeftNew+= FreeFileSync::formatFilesizeToShortString(filesizeLeftView);
    }

    wxString objectsView = numberToWxString(visibleGrid.size());
    globalFunctions::includeNumberSeparator(objectsView);
    if (currentGridData.size() == 1)
    {
        wxString outputString = _("%x of 1 row in view");
        outputString.Replace(wxT("%x"), objectsView, false);
        statusMiddleNew = outputString;
    }
    else
    {
        wxString objectsTotal = numberToWxString(currentGridData.size());
        globalFunctions::includeNumberSeparator(objectsTotal);

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
            wxString folderCount = numberToWxString(foldersOnRightView);
            globalFunctions::includeNumberSeparator(folderCount);

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
            wxString fileCount = numberToWxString(filesOnRightView);
            globalFunctions::includeNumberSeparator(fileCount);

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
    event.Skip();
}


void MainDialog::OnRemoveFolderPair(wxCommandEvent& event)
{
    removeFolderPair();
    event.Skip();
}


void MainDialog::addFolderPair(const wxString& leftDir, const wxString& rightDir)
{
    //add new folder pair
    FolderPairGenerated* newPair = new FolderPairGenerated(m_scrolledWindowFolderPairs);
    newPair->m_bitmap23->SetBitmap(*globalResource.bitmapLink);

    bSizerFolderPairs->Add(newPair, 0, wxEXPAND, 5);
    additionalFolderPairs.push_back(newPair);

    //set size of scrolled window
    wxSize pairSize = newPair->GetSize();
    const int additionalRows = additionalFolderPairs.size();
    if (additionalRows <= 3) //up to 3 additional pairs shall be shown
        m_scrolledWindowFolderPairs->SetMinSize(wxSize( -1, pairSize.GetHeight() * additionalRows));
    else //adjust scrollbars
        m_scrolledWindowFolderPairs->Fit();

    //adjust remove button
    if (additionalRows > 0)
        m_bpButtonRemovePair->Enable();

    m_scrolledWindowFolderPairs->Layout();
    bSizer1->Layout();
    m_bpButtonSwap->Refresh();

    //register events
    newPair->m_dirPickerLeft->Connect(wxEVT_COMMAND_DIRPICKER_CHANGED, wxFileDirPickerEventHandler(MainDialog::OnDirSelected), NULL, this);
    newPair->m_dirPickerRight->Connect(wxEVT_COMMAND_DIRPICKER_CHANGED, wxFileDirPickerEventHandler(MainDialog::OnDirSelected), NULL, this);

    newPair->m_directoryLeft->Connect(wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler(MainDialog::OnWriteDirManually), NULL, this );
    newPair->m_directoryRight->Connect(wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler(MainDialog::OnWriteDirManually), NULL, this );

    //prepare drag & drop
    newPair->m_panelLeft->SetDropTarget(new FileDropEvent(this, newPair->m_panelLeft));
    newPair->m_panelRight->SetDropTarget(new FileDropEvent(this, newPair->m_panelRight));

    //insert directory names if provided
    newPair->m_directoryLeft->SetValue(leftDir);
    wxString leftDirFormatted = FreeFileSync::getFormattedDirectoryName(leftDir);
    if (wxDirExists(leftDirFormatted))
        newPair->m_dirPickerLeft->SetPath(leftDirFormatted);

    newPair->m_directoryRight->SetValue(rightDir);
    wxString rightDirFormatted = FreeFileSync::getFormattedDirectoryName(rightDir);
    if (wxDirExists(rightDirFormatted))
        newPair->m_dirPickerRight->SetPath(rightDirFormatted);
}


void MainDialog::removeFolderPair(bool removeAll)
{
    if (additionalFolderPairs.size() > 0)
    {
        wxSize pairSize;
        do
        {   //remove folder pairs from window
            FolderPairGenerated* pairToDelete = additionalFolderPairs.back();
            pairSize = pairToDelete->GetSize();

            bSizerFolderPairs->Detach(pairToDelete); //Remove() does not work on Window*, so do it manually
            pairToDelete->Destroy();                 //
            additionalFolderPairs.pop_back(); //remove last element in vector
        }
        while (removeAll && additionalFolderPairs.size() > 0);

        //set size of scrolled window
        const int additionalRows = additionalFolderPairs.size();
        if (additionalRows <= 3) //up to 3 additional pairs shall be shown
            m_scrolledWindowFolderPairs->SetMinSize(wxSize(-1, pairSize.GetHeight() * additionalRows));
        //adjust scrollbars (do not put in else clause)
        m_scrolledWindowFolderPairs->Fit();

        //adjust remove button
        if (additionalRows <= 0)
            m_bpButtonRemovePair->Disable();

        m_scrolledWindowFolderPairs->Layout();
        bSizer1->Layout();
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
    mainDialog->m_bpButtonSync->Disable();
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
    mainDialog->m_bpButton201->Disable();
    mainDialog->m_choiceLoad->Disable();
    mainDialog->m_bpButton10->Disable();
    mainDialog->m_bpButton14->Disable();
    mainDialog->m_scrolledWindowFolderPairs->Disable();
    mainDialog->m_menubar1->EnableTop(0, false);
    mainDialog->m_menubar1->EnableTop(1, false);
    mainDialog->m_menubar1->EnableTop(2, false);

    //display status panel during compare
    mainDialog->compareStatus->init(); //clear old values
    mainDialog->compareStatus->Show();

    //show abort button
    mainDialog->m_buttonAbort->Enable();
    mainDialog->m_buttonAbort->Show();
    mainDialog->m_bpButtonCompare->Disable();
    mainDialog->m_bpButtonCompare->Hide();
    mainDialog->m_buttonAbort->SetFocus();

    //updateUiNow();
    mainDialog->bSizer1->Layout();  //both sizers need to recalculate!
    mainDialog->bSizer6->Layout();
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
    mainDialog->m_bpButtonSync->Enable();
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
    mainDialog->m_bpButton201->Enable();
    mainDialog->m_choiceLoad->Enable();
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

    mainDialog->m_bpButtonCompare->Enable(); //enable compare button
    mainDialog->m_bpButtonCompare->Show();

    //hide status panel from main window
    mainDialog->compareStatus->Hide();

    mainDialog->Layout();
    mainDialog->Refresh();
}


inline
void CompareStatusHandler::updateStatusText(const wxString& text)
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


ErrorHandler::Response CompareStatusHandler::reportError(const wxString& text)
{
    if (ignoreErrors)
        return ErrorHandler::IGNORE_ERROR;

    mainDialog->compareStatus->updateStatusPanelNow();

    bool ignoreNextErrors = false;
    wxString errorMessage = text + wxT("\n\n") + _("Ignore this error, retry or abort?");
    ErrorDlg* errorDlg = new ErrorDlg(mainDialog, errorMessage, ignoreNextErrors);

    int rv = errorDlg->ShowModal();
    switch (rv)
    {
    case ErrorDlg::BUTTON_IGNORE:
        ignoreErrors = ignoreNextErrors;
        return ErrorHandler::IGNORE_ERROR;
    case ErrorDlg::BUTTON_RETRY:
        return ErrorHandler::RETRY;
    case ErrorDlg::BUTTON_ABORT:
    {
        abortRequested = true;
        throw AbortThisProcess();
    }
    default:
        assert (false);
    }

    return ErrorHandler::IGNORE_ERROR; //dummy return value
}


inline
void CompareStatusHandler::forceUiRefresh()
{
    mainDialog->compareStatus->updateStatusPanelNow();
}


void CompareStatusHandler::abortThisProcess()
{
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
            result+= unhandledErrors[j] + wxT("\n");
        result+= wxT("\n");
    }

    //notify to syncStatusFrame that current process has ended
    if (abortRequested)
    {
        result+= wxString(_("Synchronization aborted!")) + wxT(" ") + _("You may try to synchronize remaining items again (WITHOUT having to re-compare)!");
        syncStatusFrame->setStatusText_NoUpdate(result);
        syncStatusFrame->processHasFinished(SyncStatus::ABORTED);  //enable okay and close events
    }
    else  if (failedItems)
    {
        result+= wxString(_("Synchronization completed with errors!")) + wxT(" ") + _("You may try to synchronize remaining items again (WITHOUT having to re-compare)!");
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
void SyncStatusHandler::updateStatusText(const wxString& text)
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


ErrorHandler::Response SyncStatusHandler::reportError(const wxString& text)
{
    if (ignoreErrors)
    {
        unhandledErrors.Add(text);
        return ErrorHandler::IGNORE_ERROR;
    }

    syncStatusFrame->updateStatusDialogNow();

    bool ignoreNextErrors = false;
    wxString errorMessage = text + wxT("\n\n") + _("Ignore this error, retry or abort synchronization?");
    ErrorDlg* errorDlg = new ErrorDlg(syncStatusFrame, errorMessage, ignoreNextErrors);

    int rv = errorDlg->ShowModal();
    switch (rv)
    {
    case ErrorDlg::BUTTON_IGNORE:
        ignoreErrors = ignoreNextErrors;
        unhandledErrors.Add(text);
        return ErrorHandler::IGNORE_ERROR;
    case ErrorDlg::BUTTON_RETRY:
        return ErrorHandler::RETRY;
    case ErrorDlg::BUTTON_ABORT:
    {
        unhandledErrors.Add(text);
        abortRequested = true;
        throw AbortThisProcess();
    }
    default:
        assert (false);
        return ErrorHandler::IGNORE_ERROR;
    }
}


void SyncStatusHandler::forceUiRefresh()
{
    syncStatusFrame->updateStatusDialogNow();
}


void SyncStatusHandler::abortThisProcess()
{
    throw AbortThisProcess();  //abort can be triggered by syncStatusFrame
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
            wxMessageDialog* messageDlg = new wxMessageDialog(this, wxString(_("File already exists. Overwrite?")) + wxT(" \"") + fileName + wxT("\""), _("Warning") , wxOK | wxCANCEL);

            if (messageDlg->ShowModal() != wxID_OK)
            {
                pushStatusInformation(_("Save aborted!"));
                event.Skip();
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

    event.Skip();
}


void MainDialog::OnMenuAdjustFileTimes(wxCommandEvent& event)
{
    ModifyFilesDlg* modifyDlg = new ModifyFilesDlg(this, FreeFileSync::getFormattedDirectoryName(m_directoryLeft->GetValue()), 0);
    if (modifyDlg->ShowModal() == ModifyFilesDlg::BUTTON_APPLY)
    {   //disable the sync button
        enableSynchronization(false);
        m_bpButtonCompare->SetFocus();
    }

    event.Skip();
}


void MainDialog::OnMenuBatchJob(wxCommandEvent& event)
{
    vector<FolderPair> folderPairs;
    getFolderPairs(folderPairs);

    BatchDialog* batchDlg = new BatchDialog(this, cfg, folderPairs);
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
    programLanguage->setLanguage(wxLANGUAGE_ENGLISH); //language is a global attribute
    restartOnExit = true;
    Destroy();
    event.Skip();
}


void MainDialog::OnMenuLangGerman(wxCommandEvent& event)
{
    programLanguage->setLanguage(wxLANGUAGE_GERMAN); //language is a global attribute
    restartOnExit = true;
    Destroy();
    event.Skip();
}


void MainDialog::OnMenuLangFrench(wxCommandEvent& event)
{
    programLanguage->setLanguage(wxLANGUAGE_FRENCH); //language is a global attribute
    restartOnExit = true;
    Destroy();
    event.Skip();
}


void MainDialog::OnMenuLangJapanese(wxCommandEvent& event)
{
    programLanguage->setLanguage(wxLANGUAGE_JAPANESE); //language is a global attribute
    restartOnExit = true;
    Destroy();
    event.Skip();
}


void MainDialog::OnMenuLangDutch(wxCommandEvent& event)
{
    programLanguage->setLanguage(wxLANGUAGE_DUTCH); //language is a global attribute
    restartOnExit = true;
    Destroy();
    event.Skip();
}


void MainDialog::OnMenuLangChineseSimp(wxCommandEvent& event)
{
    programLanguage->setLanguage(wxLANGUAGE_CHINESE_SIMPLIFIED); //language is a global attribute
    restartOnExit = true;
    Destroy();
    event.Skip();
}

