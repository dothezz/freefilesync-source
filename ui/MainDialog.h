/***************************************************************
 * Name:      FreeFileSyncMain.h
 * Purpose:   Defines Application Frame
 * Author:    ZenJu (zhnmju123@gmx.de)
 * Created:   2008-07-16
 * Copyright: ZenJu ()
 * License:
 **************************************************************/

#ifndef MAINDIALOG_H
#define MAINDIALOG_H

#ifdef WX_PRECOMP
// For compilers that support precompilation, includes "wx.h".
#include <wx/wxprec.h>
#endif

#ifdef __BORLANDC__
#pragma hdrstop
#endif //__BORLANDC__

#ifndef WX_PRECOMP
// Include your minimal set of headers here, or wx.h
#include <wx/wx.h>
#endif

//#########################################################################################

#include "SyncDialog.h"
#include "GUI_Generated.h"
#include "..\FreeFileSync.h"

using namespace std;

const int StartSynchronizationProcess = 15;

struct UI_GridLine
{
    wxString leftFilename;
    wxString leftRelativePath;
    wxString leftSize;
    wxString leftDate;

    wxString cmpResult;

    wxString rightFilename;
    wxString rightRelativePath;
    wxString rightSize;
    wxString rightDate;

    unsigned int linkToCurrentGridData; //rownumber of corresponding row in currentGridData
};
typedef vector<UI_GridLine> UI_Grid;

extern int leadingPanel;


class MainDialog: public GUI_Generated
{
public:
    MainDialog(wxFrame* frame);
    ~MainDialog();

private:
    friend class SyncDialog;
    friend class AboutDlg;

    void readConfigurationFromHD();
    void writeConfigurationToHD();

    void loadResourceFiles();
    void unloadResourceFiles();

    void onGrid1access(wxEvent& event);
    void onGrid2access(wxEvent& event);
    void onGrid3access(wxEvent& event);

    void onGrid1ButtonEvent(wxKeyEvent& event);
    void onGrid2ButtonEvent(wxKeyEvent& event);

    void OnDirChangedPanel1(wxFileDirPickerEvent& event);
    void OnDirChangedPanel2(wxFileDirPickerEvent& event);
    void onFilesDroppedPanel1(wxDropFilesEvent& event);
    void onFilesDroppedPanel2(wxDropFilesEvent& event);

    void mapFileModelToUI(UI_Grid& output, const FileCompareResult& fileCmpResult);
    void updateFilterButtons();

    void synchronizeFolders(FileCompareResult& grid, const SyncConfiguration config);

static wxString evaluateCmpResult(const CompareFilesResult result, const bool selectedForSynchronization);
    void writeGrid(const FileCompareResult& gridData, bool useUI_GridCache =  false);

    void OnMarkRangeOnGrid3(    wxEvent&     event);
    void OnDeselectRow(         wxGridEvent& event);
    void OnLeftGridDoubleClick( wxGridEvent& event);
    void OnRightGridDoubleClick(wxGridEvent& event);
    void OnSortLeftGrid(        wxGridEvent& event);
    void OnSortRightGrid(       wxGridEvent& event);

    void OnLeftOnlyFiles(       wxCommandEvent& event);
    void OnLeftNewerFiles(      wxCommandEvent& event);
    void OnDifferentFiles(      wxCommandEvent& event);
    void OnRightNewerFiles(     wxCommandEvent& event);
    void OnRightOnlyFiles(      wxCommandEvent& event);
    void OnEqualFiles(          wxCommandEvent& event);

    void OnShowHelpDialog(      wxCommandEvent& event);
    void OnSwapDirs(            wxCommandEvent& event);
    void OnChangeCompareVariant(wxCommandEvent& event);
    void OnCompare(             wxCommandEvent& event);
    void OnSync(                wxCommandEvent& event);
    void OnClose(               wxCloseEvent&   event);
    void OnQuit(                wxCommandEvent& event);
    void OnAbout(               wxCommandEvent& event);

//***********************************************
    //application variables:

    //technical representation of grid-data
    FileCompareResult currentGridData;

    //UI view of currentGridData
    UI_Grid currentUI_View;

    SyncConfiguration syncConfiguration;

    bool leftOnlyFilesActive;
    bool leftNewerFilesActive;
    bool differentFilesActive;
    bool equalFilesActive;
    bool rightNewerFilesActive;
    bool rightOnlyFilesActive;

//***********************************************

    wxFrame* parent;
    wxColour* lightBlue;

    //resources
    wxBitmap* bitmapLeftArrow;
    wxBitmap* bitmapStartSync;
    wxBitmap* bitmapRightArrow;
    wxBitmap* bitmapDelete;
    wxBitmap* bitmapEmail;
    wxBitmap* bitmapAbout;
    wxBitmap* bitmapWebsite;
    wxBitmap* bitmapExit;
    wxBitmap* bitmapSync;
    wxBitmap* bitmapCompare;
    wxBitmap* bitmapSyncDisabled;
    wxBitmap* bitmapSwap;
    wxBitmap* bitmapHelp;
    wxBitmap* bitmapLeftOnly;
    wxBitmap* bitmapLeftNewer;
    wxBitmap* bitmapDifferent;
    wxBitmap* bitmapRightNewer;
    wxBitmap* bitmapRightOnly;
    wxBitmap* bitmapLeftOnlyDeact;
    wxBitmap* bitmapLeftNewerDeact;
    wxBitmap* bitmapDifferentDeact;
    wxBitmap* bitmapRightNewerDeact;
    wxBitmap* bitmapRightOnlyDeact;
    wxBitmap* bitmapEqual;
    wxBitmap* bitmapEqualDeact;
    wxAnimation* animationMoney;
};

class CustomGrid : public wxGrid
{
public:
    CustomGrid( wxWindow *parent,
                wxWindowID id,
                const wxPoint& pos   = wxDefaultPosition,
                const wxSize& size   = wxDefaultSize,
                long style           = wxWANTS_CHARS,
                const wxString& name = wxGridNameStr )
            : wxGrid(parent, id, pos, size, style, name), scrollbarsEnabled(true), m_grid1(0), m_grid2(0), m_grid3(0) {}

    ~CustomGrid() {};

    void deactivateScrollbars()
    {
        scrollbarsEnabled = false;
    }

    //overwrite virtual method to finally get rid of the scrollbars
    void SetScrollbar(int orientation, int position, int thumbSize, int range, bool refresh = true)
    {
        if (scrollbarsEnabled)
            wxWindow::SetScrollbar(orientation, position, thumbSize, range, refresh);
        else
            wxWindow::SetScrollbar(orientation, 0, 0, 0, refresh);
    }

    //this method is called when grid view changes: useful for parallel updating of multiple grids
    void DoPrepareDC(wxDC& dc)
    {
        wxScrollHelper::DoPrepareDC(dc);

        int x, y = 0;
        if (leadingPanel == 1 && this == m_grid1)   //avoid back coupling
        {
            GetViewStart(&x, &y);
            m_grid2->Scroll(x, y);
            m_grid3->Scroll(-1, y); //scroll in y-direction only
        }
        else if (leadingPanel == 2 && this == m_grid2)   //avoid back coupling
        {
            GetViewStart(&x, &y);
            m_grid1->Scroll(x, y);
            m_grid3->Scroll(-1, y);
        }
    }

    void setScrollFriends(CustomGrid* grid1, CustomGrid* grid2, CustomGrid* grid3)
    {
        m_grid1 = grid1;
        m_grid2 = grid2;
        m_grid3 = grid3;
    }

private:
    bool scrollbarsEnabled;

    CustomGrid* m_grid1;
    CustomGrid* m_grid2;
    CustomGrid* m_grid3;
};


//######################################################################################################

class AboutDlg: public AboutDlgGenerated
{
public:
    AboutDlg(MainDialog* window) : AboutDlgGenerated(window), mainDialog(window)
    {
        m_bitmap9->SetBitmap(*mainDialog->bitmapWebsite);
        m_bitmap10->SetBitmap(*mainDialog->bitmapEmail);

        m_animationControl1->SetAnimation(*mainDialog->animationMoney);
        m_animationControl1->Play();
    }
    ~AboutDlg() {}

private:
    void OnClose(wxCloseEvent& event)
    {
        Destroy();
    }
    void OnOK(wxCommandEvent& event)
    {
        Destroy();
    }
    MainDialog* mainDialog;
};

//######################################################################################################

class HelpDlg: public HelpDlgGenerated
{
public:
    HelpDlg(MainDialog* window) : HelpDlgGenerated(window) {}
    ~HelpDlg() {}

private:
    void OnClose(wxCloseEvent& event)
    {
        Destroy();
    }
    void OnOK(wxCommandEvent& event)
    {
        Destroy();
    }
};

#endif // MAINDIALOG_H
