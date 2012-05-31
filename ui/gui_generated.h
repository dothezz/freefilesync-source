///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Apr 10 2012)
// http://www.wxformbuilder.org/
//
// PLEASE DO "NOT" EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#ifndef __GUI_GENERATED_H__
#define __GUI_GENERATED_H__

#include <wx/artprov.h>
#include <wx/xrc/xmlres.h>
#include <wx/intl.h>
#include "wx_form_build_hide_warnings.h"
#include "../wx+/button.h"
#include "folder_history_box.h"
#include "../wx+/dir_picker.h"
#include "../wx+/grid.h"
#include "../wx+/toggle_button.h"
#include "exec_finished_box.h"
#include "../wx+/graph.h"
#include <wx/string.h>
#include <wx/bitmap.h>
#include <wx/image.h>
#include <wx/icon.h>
#include <wx/menu.h>
#include <wx/gdicmn.h>
#include <wx/font.h>
#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/stattext.h>
#include <wx/button.h>
#include <wx/sizer.h>
#include <wx/bmpbuttn.h>
#include <wx/panel.h>
#include <wx/combobox.h>
#include <wx/filepicker.h>
#include <wx/scrolwin.h>
#include <wx/statbmp.h>
#include <wx/statline.h>
#include <wx/listbox.h>
#include <wx/checkbox.h>
#include <wx/frame.h>
#include <wx/textctrl.h>
#include <wx/gauge.h>
#include <wx/statbox.h>
#include <wx/choice.h>
#include <wx/spinctrl.h>
#include <wx/notebook.h>
#include <wx/dialog.h>
#include <wx/radiobut.h>
#include <wx/animate.h>
#include <wx/listbook.h>
#include <wx/listctrl.h>
#include <wx/imaglist.h>
#include <wx/hyperlink.h>
#include <wx/grid.h>
#include <wx/calctrl.h>

#include "../zen/i18n.h"

///////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////
/// Class MainDialogGenerated
///////////////////////////////////////////////////////////////////////////////
class MainDialogGenerated : public wxFrame
{
private:

protected:
    wxMenuBar* m_menubar1;
    wxMenu* m_menuFile;
    wxMenuItem* m_menuItem10;
    wxMenuItem* m_menuItem11;
    wxMenuItem* m_menuItemNew;
    wxMenuItem* m_menuItemLoad;
    wxMenuItem* m_menuItemSave;
    wxMenu* m_menuAdvanced;
    wxMenu* m_menuLanguages;
    wxMenuItem* m_menuItemGlobSett;
    wxMenuItem* m_menuItem7;
    wxMenu* m_menuHelp;
    wxMenuItem* m_menuItemManual;
    wxMenuItem* m_menuItemCheckVer;
    wxMenuItem* m_menuItemAbout;
    wxBoxSizer* bSizerPanelHolder;
    wxPanel* m_panelTopButtons;
    wxBoxSizer* bSizerTopButtons;
    wxStaticText* m_staticTextCmpVariant;
    zen::BitmapButton* m_buttonCompare;
    wxButton* m_buttonAbort;
    wxBitmapButton* m_bpButtonCmpConfig;
    wxStaticText* m_staticTextSyncVariant;
    wxBitmapButton* m_bpButtonSyncConfig;
    zen::BitmapButton* m_buttonStartSync;
    wxPanel* m_panelDirectoryPairs;
    wxStaticText* m_staticTextFinalPathLeft;
    wxBitmapButton* m_bpButtonAddPair;
    wxPanel* m_panelTopMiddle;
    wxBitmapButton* m_bpButtonSwapSides;
    wxStaticText* m_staticTextFinalPathRight;
    wxScrolledWindow* m_scrolledWindowFolderPairs;
    wxBoxSizer* bSizerAddFolderPairs;
    zen::Grid* m_gridNavi;
    wxPanel* m_panelCenter;
    zen::Grid* m_gridMain;
    wxPanel* m_panelStatusBar;
    wxBoxSizer* bSizerStatusLeftDirectories;
    wxStaticBitmap* m_bitmapSmallDirectoryLeft;
    wxStaticText* m_staticTextStatusLeftDirs;
    wxBoxSizer* bSizerStatusLeftFiles;
    wxStaticBitmap* m_bitmapSmallFileLeft;
    wxStaticText* m_staticTextStatusLeftFiles;
    wxStaticText* m_staticTextStatusLeftBytes;
    wxStaticLine* m_staticline9;
    wxStaticText* m_staticTextStatusMiddle;
    wxStaticLine* m_staticline10;
    wxBoxSizer* bSizerStatusRightDirectories;
    wxStaticBitmap* m_bitmapSmallDirectoryRight;
    wxStaticText* m_staticTextStatusRightDirs;
    wxBoxSizer* bSizerStatusRightFiles;
    wxStaticBitmap* m_bitmapSmallFileRight;
    wxStaticText* m_staticTextStatusRightFiles;
    wxStaticText* m_staticTextStatusRightBytes;
    wxPanel* m_panelConfig;
    wxBoxSizer* bSizerConfig;
    wxBitmapButton* m_bpButtonLoad;
    wxBitmapButton* m_bpButtonSave;
    wxListBox* m_listBoxHistory;
    wxPanel* m_panelFilter;
    wxBitmapButton* m_bpButtonFilter;
    wxCheckBox* m_checkBoxHideFilt;
    wxPanel* m_panelStatistics;
    wxBoxSizer* bSizer1801;
    wxStaticBitmap* m_bitmapCreateLeft;
    wxStaticText* m_staticTextCreateLeft;
    wxStaticBitmap* m_bitmapUpdateLeft;
    wxStaticText* m_staticTextUpdateLeft;
    wxStaticBitmap* m_bitmapDeleteLeft;
    wxStaticText* m_staticTextDeleteLeft;
    wxStaticBitmap* m_bitmapData;
    wxStaticText* m_staticTextData;
    wxStaticBitmap* m_bitmapDeleteRight;
    wxStaticText* m_staticTextDeleteRight;
    wxStaticBitmap* m_bitmapUpdateRight;
    wxStaticText* m_staticTextUpdateRight;
    wxStaticBitmap* m_bitmapCreateRight;
    wxStaticText* m_staticTextCreateRight;
    wxPanel* m_panelViewFilter;
    wxBoxSizer* bSizerViewFilter;
    ToggleButton* m_bpButtonSyncCreateLeft;
    ToggleButton* m_bpButtonSyncDirOverwLeft;
    ToggleButton* m_bpButtonSyncDeleteLeft;
    ToggleButton* m_bpButtonLeftOnly;
    ToggleButton* m_bpButtonLeftNewer;
    ToggleButton* m_bpButtonEqual;
    ToggleButton* m_bpButtonDifferent;
    ToggleButton* m_bpButtonSyncDirNone;
    ToggleButton* m_bpButtonRightNewer;
    ToggleButton* m_bpButtonRightOnly;
    ToggleButton* m_bpButtonSyncDeleteRight;
    ToggleButton* m_bpButtonSyncDirOverwRight;
    ToggleButton* m_bpButtonSyncCreateRight;
    ToggleButton* m_bpButtonConflict;

    // Virtual event handlers, overide them in your derived class
    virtual void OnClose( wxCloseEvent& event ) { event.Skip(); }
    virtual void OnCompare( wxCommandEvent& event ) { event.Skip(); }
    virtual void OnStartSync( wxCommandEvent& event ) { event.Skip(); }
    virtual void OnNewConfig( wxCommandEvent& event ) { event.Skip(); }
    virtual void OnLoadConfig( wxCommandEvent& event ) { event.Skip(); }
    virtual void OnSaveConfig( wxCommandEvent& event ) { event.Skip(); }
    virtual void OnMenuQuit( wxCommandEvent& event ) { event.Skip(); }
    virtual void OnMenuGlobalSettings( wxCommandEvent& event ) { event.Skip(); }
    virtual void OnMenuBatchJob( wxCommandEvent& event ) { event.Skip(); }
    virtual void OnMenuExportFileList( wxCommandEvent& event ) { event.Skip(); }
    virtual void OnShowHelp( wxCommandEvent& event ) { event.Skip(); }
    virtual void OnMenuCheckVersion( wxCommandEvent& event ) { event.Skip(); }
    virtual void OnMenuAbout( wxCommandEvent& event ) { event.Skip(); }
    virtual void OnCmpSettings( wxCommandEvent& event ) { event.Skip(); }
    virtual void OnSyncSettings( wxCommandEvent& event ) { event.Skip(); }
    virtual void OnAddFolderPair( wxCommandEvent& event ) { event.Skip(); }
    virtual void OnRemoveTopFolderPair( wxCommandEvent& event ) { event.Skip(); }
    virtual void OnDirSelected( wxFileDirPickerEvent& event ) { event.Skip(); }
    virtual void OnSwapSides( wxCommandEvent& event ) { event.Skip(); }
    virtual void OnCfgHistoryKeyEvent( wxKeyEvent& event ) { event.Skip(); }
    virtual void OnLoadFromHistory( wxCommandEvent& event ) { event.Skip(); }
    virtual void OnConfigureFilter( wxCommandEvent& event ) { event.Skip(); }
    virtual void OnHideFilteredButton( wxCommandEvent& event ) { event.Skip(); }
    virtual void OnSyncCreateLeft( wxCommandEvent& event ) { event.Skip(); }
    virtual void OnSyncDirLeft( wxCommandEvent& event ) { event.Skip(); }
    virtual void OnSyncDeleteLeft( wxCommandEvent& event ) { event.Skip(); }
    virtual void OnLeftOnlyFiles( wxCommandEvent& event ) { event.Skip(); }
    virtual void OnLeftNewerFiles( wxCommandEvent& event ) { event.Skip(); }
    virtual void OnEqualFiles( wxCommandEvent& event ) { event.Skip(); }
    virtual void OnDifferentFiles( wxCommandEvent& event ) { event.Skip(); }
    virtual void OnSyncDirNone( wxCommandEvent& event ) { event.Skip(); }
    virtual void OnRightNewerFiles( wxCommandEvent& event ) { event.Skip(); }
    virtual void OnRightOnlyFiles( wxCommandEvent& event ) { event.Skip(); }
    virtual void OnSyncDeleteRight( wxCommandEvent& event ) { event.Skip(); }
    virtual void OnSyncDirRight( wxCommandEvent& event ) { event.Skip(); }
    virtual void OnSyncCreateRight( wxCommandEvent& event ) { event.Skip(); }
    virtual void OnConflictFiles( wxCommandEvent& event ) { event.Skip(); }


public:
    wxPanel* m_panelTopLeft;
    wxBitmapButton* m_bpButtonRemovePair;
    FolderHistoryBox* m_directoryLeft;
    zen::DirPickerCtrl* m_dirPickerLeft;
    wxBitmapButton* m_bpButtonAltCompCfg;
    wxBitmapButton* m_bpButtonLocalFilter;
    wxBitmapButton* m_bpButtonAltSyncCfg;
    wxPanel* m_panelTopRight;
    FolderHistoryBox* m_directoryRight;
    zen::DirPickerCtrl* m_dirPickerRight;
    wxBoxSizer* bSizerStatistics;
    wxBoxSizer* bSizerData;

    MainDialogGenerated( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = wxEmptyString, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( 702,522 ), long style = wxDEFAULT_FRAME_STYLE|wxTAB_TRAVERSAL );

    ~MainDialogGenerated();

};

///////////////////////////////////////////////////////////////////////////////
/// Class FolderPairGenerated
///////////////////////////////////////////////////////////////////////////////
class FolderPairGenerated : public wxPanel
{
private:

protected:

public:
    wxPanel* m_panelLeft;
    wxBitmapButton* m_bpButtonRemovePair;
    FolderHistoryBox* m_directoryLeft;
    zen::DirPickerCtrl* m_dirPickerLeft;
    wxPanel* m_panel20;
    wxBitmapButton* m_bpButtonAltCompCfg;
    wxBitmapButton* m_bpButtonLocalFilter;
    wxBitmapButton* m_bpButtonAltSyncCfg;
    wxPanel* m_panelRight;
    FolderHistoryBox* m_directoryRight;
    zen::DirPickerCtrl* m_dirPickerRight;

    FolderPairGenerated( wxWindow* parent, wxWindowID id = wxID_ANY, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( -1,-1 ), long style = wxTAB_TRAVERSAL );
    ~FolderPairGenerated();

};

///////////////////////////////////////////////////////////////////////////////
/// Class CompareStatusGenerated
///////////////////////////////////////////////////////////////////////////////
class CompareStatusGenerated : public wxPanel
{
private:

protected:
    wxTextCtrl* m_textCtrlStatus;
    wxGauge* m_gauge2;
    wxBoxSizer* bSizer42;
    wxBoxSizer* bSizerFilesFound;
    wxStaticText* m_staticText321;
    wxStaticText* m_staticTextScanned;
    wxBoxSizer* bSizerFilesRemaining;
    wxStaticText* m_staticText46;
    wxStaticText* m_staticTextFilesRemaining;
    wxStaticText* m_staticTextDataRemaining;
    wxBoxSizer* sSizerSpeed;
    wxStaticText* m_staticText104;
    wxStaticText* m_staticTextSpeed;
    wxBoxSizer* sSizerTimeRemaining;
    wxStaticText* m_staticTextTimeRemFixed;
    wxStaticText* m_staticTextRemTime;
    wxBoxSizer* sSizerTimeElapsed;
    wxStaticText* m_staticTextTimeElapsed;

public:

    CompareStatusGenerated( wxWindow* parent, wxWindowID id = wxID_ANY, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( -1,-1 ), long style = wxRAISED_BORDER|wxTAB_TRAVERSAL );
    ~CompareStatusGenerated();

};

///////////////////////////////////////////////////////////////////////////////
/// Class BatchDlgGenerated
///////////////////////////////////////////////////////////////////////////////
class BatchDlgGenerated : public wxDialog
{
private:

protected:
    wxStaticBitmap* m_bitmap27;
    wxPanel* m_panel8;
    wxStaticText* m_staticText56;
    wxStaticText* m_staticText44;
    wxBitmapButton* m_bpButtonHelp;
    wxNotebook* m_notebook1;
    wxPanel* m_panelOverview;
    wxBitmapButton* m_bpButtonCmpConfig;
    wxStaticText* m_staticTextCmpVariant;
    wxBitmapButton* m_bpButtonFilter;
    wxStaticText* m_staticTextSyncVariant;
    wxBitmapButton* m_bpButtonSyncConfig;
    wxBoxSizer* sbSizerMainPair;
    wxPanel* m_panelMainPair;
    wxStaticText* m_staticText532;
    wxStaticText* m_staticText5411;
    wxBoxSizer* bSizerAddFolderPairs;
    wxPanel* m_panelBatchSettings;
    wxCheckBox* m_checkBoxShowProgress;
    wxStaticText* m_staticText961;
    wxChoice* m_choiceHandleError;
    wxStaticBoxSizer* sbSizerLogfileDir;
    wxStaticText* m_staticText96;
    wxSpinCtrl* m_spinCtrlLogCountMax;
    wxPanel* m_panelLogfile;
    wxStaticText* m_staticText94;
    zen::DirPickerCtrl* m_dirPickerLogfileDir;
    wxButton* m_buttonSave;
    wxButton* m_buttonLoad;
    wxButton* m_button6;

    // Virtual event handlers, overide them in your derived class
    virtual void OnClose( wxCloseEvent& event ) { event.Skip(); }
    virtual void OnHelp( wxCommandEvent& event ) { event.Skip(); }
    virtual void OnCmpSettings( wxCommandEvent& event ) { event.Skip(); }
    virtual void OnConfigureFilter( wxCommandEvent& event ) { event.Skip(); }
    virtual void OnSyncSettings( wxCommandEvent& event ) { event.Skip(); }
    virtual void OnAddFolderPair( wxCommandEvent& event ) { event.Skip(); }
    virtual void OnRemoveTopFolderPair( wxCommandEvent& event ) { event.Skip(); }
    virtual void OnChangeErrorHandling( wxCommandEvent& event ) { event.Skip(); }
    virtual void OnChangeMaxLogCountTxt( wxCommandEvent& event ) { event.Skip(); }
    virtual void OnSaveBatchJob( wxCommandEvent& event ) { event.Skip(); }
    virtual void OnLoadBatchJob( wxCommandEvent& event ) { event.Skip(); }
    virtual void OnCancel( wxCommandEvent& event ) { event.Skip(); }


public:
    wxScrolledWindow* m_scrolledWindow6;
    wxBitmapButton* m_bpButtonAddPair;
    wxBitmapButton* m_bpButtonRemovePair;
    wxPanel* m_panelLeft;
    FolderHistoryBox* m_directoryLeft;
    zen::DirPickerCtrl* m_dirPickerLeft;
    wxPanel* m_panelRight;
    FolderHistoryBox* m_directoryRight;
    zen::DirPickerCtrl* m_dirPickerRight;
    wxBitmapButton* m_bpButtonAltCompCfg;
    wxBitmapButton* m_bpButtonLocalFilter;
    wxBitmapButton* m_bpButtonAltSyncCfg;
    FolderHistoryBox* m_comboBoxLogfileDir;

    BatchDlgGenerated( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = _("Create a batch job"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( -1,-1 ), long style = wxDEFAULT_DIALOG_STYLE|wxMAXIMIZE_BOX|wxMINIMIZE_BOX|wxRESIZE_BORDER );
    ~BatchDlgGenerated();

};

///////////////////////////////////////////////////////////////////////////////
/// Class BatchFolderPairGenerated
///////////////////////////////////////////////////////////////////////////////
class BatchFolderPairGenerated : public wxPanel
{
private:

protected:
    wxPanel* m_panel32;
    wxStaticText* m_staticText53;
    wxStaticText* m_staticText541;
    wxPanel* m_panelLeft;
    wxPanel* m_panelRight;

public:
    wxBitmapButton* m_bpButtonRemovePair;
    FolderHistoryBox* m_directoryLeft;
    zen::DirPickerCtrl* m_dirPickerLeft;
    FolderHistoryBox* m_directoryRight;
    zen::DirPickerCtrl* m_dirPickerRight;
    wxBitmapButton* m_bpButtonAltCompCfg;
    wxBitmapButton* m_bpButtonLocalFilter;
    wxBitmapButton* m_bpButtonAltSyncCfg;

    BatchFolderPairGenerated( wxWindow* parent, wxWindowID id = wxID_ANY, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( -1,-1 ), long style = wxTAB_TRAVERSAL );
    ~BatchFolderPairGenerated();

};

///////////////////////////////////////////////////////////////////////////////
/// Class SyncCfgDlgGenerated
///////////////////////////////////////////////////////////////////////////////
class SyncCfgDlgGenerated : public wxDialog
{
private:

protected:
    wxRadioButton* m_radioBtnAutomatic;
    wxButton* m_buttonAutomatic;
    wxStaticText* m_staticTextAutomatic;
    wxRadioButton* m_radioBtnMirror;
    wxButton* m_buttonMirror;
    wxStaticText* m_staticTextMirror;
    wxRadioButton* m_radioBtnUpdate;
    wxButton* m_buttonUpdate;
    wxStaticText* m_staticTextUpdate;
    wxRadioButton* m_radioBtnCustom;
    wxButton* m_buttonCustom;
    wxStaticText* m_staticTextCustom;
    wxStaticBoxSizer* sbSizerCustDelDir;
    wxChoice* m_choiceHandleDeletion;
    wxPanel* m_panelCustomDeletionDir;
    FolderHistoryBox* m_customDelFolder;
    zen::DirPickerCtrl* m_dirPickerCustomDelFolder;
    wxBoxSizer* bSizer201;
    wxStaticBoxSizer* sbSizerErrorHandling;
    wxChoice* m_choiceHandleError;
    wxStaticBoxSizer* sbSizerExecFinished;
    ExecFinishedBox* m_comboBoxExecFinished;
    wxStaticBitmap* m_bitmapDatabase;
    wxBoxSizer* sbSizerSyncDirections;
    wxStaticText* m_staticText21;
    wxStaticText* m_staticText31;
    wxBoxSizer* bSizerLeftOnly;
    wxStaticBitmap* m_bitmapLeftOnly;
    wxBitmapButton* m_bpButtonLeftOnly;
    wxBoxSizer* bSizerRightOnly;
    wxStaticBitmap* m_bitmapRightOnly;
    wxBitmapButton* m_bpButtonRightOnly;
    wxBoxSizer* bSizerLeftNewer;
    wxStaticBitmap* m_bitmapLeftNewer;
    wxBitmapButton* m_bpButtonLeftNewer;
    wxBoxSizer* bSizerRightNewer;
    wxStaticBitmap* m_bitmapRightNewer;
    wxBitmapButton* m_bpButtonRightNewer;
    wxBoxSizer* bSizerDifferent;
    wxStaticBitmap* m_bitmapDifferent;
    wxBitmapButton* m_bpButtonDifferent;
    wxBoxSizer* bSizerConflict;
    wxStaticBitmap* m_bitmapConflict;
    wxBitmapButton* m_bpButtonConflict;
    wxButton* m_buttonOK;
    wxButton* m_button16;

    // Virtual event handlers, overide them in your derived class
    virtual void OnClose( wxCloseEvent& event ) { event.Skip(); }
    virtual void OnSyncAutomatic( wxCommandEvent& event ) { event.Skip(); }
    virtual void OnSyncAutomaticDouble( wxMouseEvent& event ) { event.Skip(); }
    virtual void OnSyncMirror( wxCommandEvent& event ) { event.Skip(); }
    virtual void OnSyncMirrorDouble( wxMouseEvent& event ) { event.Skip(); }
    virtual void OnSyncUpdate( wxCommandEvent& event ) { event.Skip(); }
    virtual void OnSyncUpdateDouble( wxMouseEvent& event ) { event.Skip(); }
    virtual void OnSyncCustom( wxCommandEvent& event ) { event.Skip(); }
    virtual void OnSyncCustomDouble( wxMouseEvent& event ) { event.Skip(); }
    virtual void OnChangeDeletionHandling( wxCommandEvent& event ) { event.Skip(); }
    virtual void OnChangeErrorHandling( wxCommandEvent& event ) { event.Skip(); }
    virtual void OnExLeftSideOnly( wxCommandEvent& event ) { event.Skip(); }
    virtual void OnExRightSideOnly( wxCommandEvent& event ) { event.Skip(); }
    virtual void OnLeftNewer( wxCommandEvent& event ) { event.Skip(); }
    virtual void OnRightNewer( wxCommandEvent& event ) { event.Skip(); }
    virtual void OnDifferent( wxCommandEvent& event ) { event.Skip(); }
    virtual void OnConflict( wxCommandEvent& event ) { event.Skip(); }
    virtual void OnApply( wxCommandEvent& event ) { event.Skip(); }
    virtual void OnCancel( wxCommandEvent& event ) { event.Skip(); }


public:

    SyncCfgDlgGenerated( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = _("Synchronization settings"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( -1,-1 ), long style = wxDEFAULT_DIALOG_STYLE );
    ~SyncCfgDlgGenerated();

};

///////////////////////////////////////////////////////////////////////////////
/// Class CmpCfgDlgGenerated
///////////////////////////////////////////////////////////////////////////////
class CmpCfgDlgGenerated : public wxDialog
{
private:

protected:
    wxRadioButton* m_radioBtnSizeDate;
    wxStaticBitmap* m_bitmapByTime;
    wxButton* m_buttonTimeSize;
    wxRadioButton* m_radioBtnContent;
    wxStaticBitmap* m_bitmapByContent;
    wxButton* m_buttonContent;
    wxChoice* m_choiceHandleSymlinks;
    wxBitmapButton* m_bpButtonHelp;
    wxButton* m_button10;
    wxButton* m_button6;

    // Virtual event handlers, overide them in your derived class
    virtual void OnClose( wxCloseEvent& event ) { event.Skip(); }
    virtual void OnTimeSize( wxCommandEvent& event ) { event.Skip(); }
    virtual void OnTimeSizeDouble( wxMouseEvent& event ) { event.Skip(); }
    virtual void OnContent( wxCommandEvent& event ) { event.Skip(); }
    virtual void OnContentDouble( wxMouseEvent& event ) { event.Skip(); }
    virtual void OnChangeErrorHandling( wxCommandEvent& event ) { event.Skip(); }
    virtual void OnShowHelp( wxCommandEvent& event ) { event.Skip(); }
    virtual void OnOkay( wxCommandEvent& event ) { event.Skip(); }
    virtual void OnCancel( wxCommandEvent& event ) { event.Skip(); }


public:

    CmpCfgDlgGenerated( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = _("Comparison settings"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize, long style = wxDEFAULT_DIALOG_STYLE );
    ~CmpCfgDlgGenerated();

};

///////////////////////////////////////////////////////////////////////////////
/// Class SyncStatusDlgGenerated
///////////////////////////////////////////////////////////////////////////////
class SyncStatusDlgGenerated : public wxFrame
{
private:

protected:
    wxBoxSizer* bSizerTop;
    wxPanel* m_panelHeader;
    wxStaticBitmap* m_bitmapStatus;
    wxStaticText* m_staticTextStatus;
    wxAnimationCtrl* m_animationControl1;
    wxStaticLine* m_staticlineHeader;
    wxPanel* m_panelProgress;
    wxTextCtrl* m_textCtrlStatus;
    wxBoxSizer* bSizer171;
    wxStaticText* m_staticTextLabelItemsProc;
    wxBoxSizer* bSizerItemsProc;
    wxStaticText* m_staticTextProcessedObj;
    wxStaticText* m_staticTextDataProcessed;
    wxStaticText* m_staticTextLabelItemsRem;
    wxBoxSizer* bSizerItemsRem;
    wxStaticText* m_staticTextRemainingObj;
    wxStaticText* m_staticTextDataRemaining;
    wxStaticText* m_staticText84;
    wxStaticText* m_staticTextSpeed;
    wxStaticText* m_staticTextLabelRemTime;
    wxStaticText* m_staticTextRemTime;
    wxStaticText* m_staticTextLabelElapsedTime;
    wxStaticText* m_staticTextTimeElapsed;
    zen::Graph2D* m_panelGraph;
    wxBoxSizer* bSizerFinalStat;
    wxListbook* m_listbookResult;
    wxStaticLine* m_staticline12;
    wxPanel* m_panelFooter;
    wxBoxSizer* bSizerExecFinished;
    wxStaticText* m_staticText87;
    ExecFinishedBox* m_comboBoxExecFinished;
    wxBoxSizer* bSizer28;
    wxButton* m_buttonOK;
    wxButton* m_buttonPause;
    wxButton* m_buttonAbort;

    // Virtual event handlers, overide them in your derived class
    virtual void OnClose( wxCloseEvent& event ) { event.Skip(); }
    virtual void OnIconize( wxIconizeEvent& event ) { event.Skip(); }
    virtual void OnOkay( wxCommandEvent& event ) { event.Skip(); }
    virtual void OnPause( wxCommandEvent& event ) { event.Skip(); }
    virtual void OnAbort( wxCommandEvent& event ) { event.Skip(); }


public:
    wxGauge* m_gauge1;

    SyncStatusDlgGenerated( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = wxEmptyString, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( -1,-1 ), long style = wxDEFAULT_FRAME_STYLE|wxTAB_TRAVERSAL );

    ~SyncStatusDlgGenerated();

};

///////////////////////////////////////////////////////////////////////////////
/// Class LogControlGenerated
///////////////////////////////////////////////////////////////////////////////
class LogControlGenerated : public wxPanel
{
private:

protected:
    ToggleButton* m_bpButtonErrors;
    ToggleButton* m_bpButtonWarnings;
    ToggleButton* m_bpButtonInfo;
    wxStaticLine* m_staticline13;
    wxTextCtrl* m_textCtrlInfo;

    // Virtual event handlers, overide them in your derived class
    virtual void OnErrors( wxCommandEvent& event ) { event.Skip(); }
    virtual void OnWarnings( wxCommandEvent& event ) { event.Skip(); }
    virtual void OnInfo( wxCommandEvent& event ) { event.Skip(); }


public:

    LogControlGenerated( wxWindow* parent, wxWindowID id = wxID_ANY, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( -1,-1 ), long style = wxTAB_TRAVERSAL );
    ~LogControlGenerated();

};

///////////////////////////////////////////////////////////////////////////////
/// Class AboutDlgGenerated
///////////////////////////////////////////////////////////////////////////////
class AboutDlgGenerated : public wxDialog
{
private:

protected:
    wxPanel* m_panel5;
    wxStaticBitmap* m_bitmap11;
    wxStaticText* m_build;
    wxPanel* m_panel33;
    wxBoxSizer* bSizerCodeInfo;
    wxStaticText* m_staticText72;
    wxHyperlinkCtrl* m_hyperlink9;
    wxHyperlinkCtrl* m_hyperlink11;
    wxHyperlinkCtrl* m_hyperlink10;
    wxHyperlinkCtrl* m_hyperlink13;
    wxHyperlinkCtrl* m_hyperlink7;
    wxHyperlinkCtrl* m_hyperlink16;
    wxHyperlinkCtrl* m_hyperlink8;
    wxHyperlinkCtrl* m_hyperlink15;
    wxHyperlinkCtrl* m_hyperlink12;
    wxHyperlinkCtrl* m_hyperlink18;
    wxHyperlinkCtrl* m_hyperlink14;
    wxHyperlinkCtrl* m_hyperlink21;
    wxHyperlinkCtrl* m_hyperlink1;
    wxStaticBitmap* m_bitmap9;
    wxHyperlinkCtrl* m_hyperlink2;
    wxStaticBitmap* m_bitmap10;
    wxScrolledWindow* m_scrolledWindowTranslators;
    wxBoxSizer* bSizerTranslators;
    wxStaticText* m_staticText54;
    wxFlexGridSizer* fgSizerTranslators;
    wxPanel* m_panel40;
    wxPanel* m_panel39;
    wxStaticText* m_staticText83;
    wxHyperlinkCtrl* m_hyperlink3;
    wxStaticBitmap* m_bitmapPaypal;
    wxStaticBitmap* m_bitmap13;
    wxHyperlinkCtrl* m_hyperlink5;
    wxButton* m_buttonOkay;

    // Virtual event handlers, overide them in your derived class
    virtual void OnClose( wxCloseEvent& event ) { event.Skip(); }
    virtual void OnOK( wxCommandEvent& event ) { event.Skip(); }


public:

    AboutDlgGenerated( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = _("About"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( -1,-1 ), long style = wxDEFAULT_DIALOG_STYLE );
    ~AboutDlgGenerated();

};

///////////////////////////////////////////////////////////////////////////////
/// Class ErrorDlgGenerated
///////////////////////////////////////////////////////////////////////////////
class ErrorDlgGenerated : public wxDialog
{
private:

protected:
    wxStaticBitmap* m_bitmap10;
    wxTextCtrl* m_textCtrl8;
    wxStaticLine* m_staticline6;
    wxPanel* m_panel33;
    wxCheckBox* m_checkBoxIgnoreErrors;
    wxButton* m_buttonIgnore;
    wxButton* m_buttonRetry;
    wxButton* m_buttonAbort;

    // Virtual event handlers, overide them in your derived class
    virtual void OnClose( wxCloseEvent& event ) { event.Skip(); }
    virtual void OnIgnore( wxCommandEvent& event ) { event.Skip(); }
    virtual void OnRetry( wxCommandEvent& event ) { event.Skip(); }
    virtual void OnAbort( wxCommandEvent& event ) { event.Skip(); }


public:

    ErrorDlgGenerated( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = _("Error"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( -1,-1 ), long style = wxDEFAULT_DIALOG_STYLE|wxMAXIMIZE_BOX|wxMINIMIZE_BOX|wxRESIZE_BORDER );
    ~ErrorDlgGenerated();

};

///////////////////////////////////////////////////////////////////////////////
/// Class WarningDlgGenerated
///////////////////////////////////////////////////////////////////////////////
class WarningDlgGenerated : public wxDialog
{
private:

protected:
    wxTextCtrl* m_textCtrl8;
    wxStaticLine* m_staticline7;
    wxPanel* m_panel34;
    wxCheckBox* m_checkBoxDontShowAgain;
    wxButton* m_buttonIgnore;
    wxButton* m_buttonSwitch;
    wxButton* m_buttonAbort;

    // Virtual event handlers, overide them in your derived class
    virtual void OnClose( wxCloseEvent& event ) { event.Skip(); }
    virtual void OnIgnore( wxCommandEvent& event ) { event.Skip(); }
    virtual void OnSwitch( wxCommandEvent& event ) { event.Skip(); }
    virtual void OnAbort( wxCommandEvent& event ) { event.Skip(); }


public:
    wxStaticBitmap* m_bitmap10;

    WarningDlgGenerated( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = _("Warning"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( -1,-1 ), long style = wxDEFAULT_DIALOG_STYLE|wxMAXIMIZE_BOX|wxMINIMIZE_BOX|wxRESIZE_BORDER );
    ~WarningDlgGenerated();

};

///////////////////////////////////////////////////////////////////////////////
/// Class QuestionDlgGenerated
///////////////////////////////////////////////////////////////////////////////
class QuestionDlgGenerated : public wxDialog
{
private:

protected:
    wxStaticBitmap* m_bitmap10;
    wxTextCtrl* m_textCtrl8;
    wxStaticLine* m_staticline8;
    wxPanel* m_panel35;
    wxCheckBox* m_checkBox;
    wxButton* m_buttonYes;
    wxButton* m_buttonNo;
    wxButton* m_buttonCancel;

    // Virtual event handlers, overide them in your derived class
    virtual void OnClose( wxCloseEvent& event ) { event.Skip(); }
    virtual void OnYes( wxCommandEvent& event ) { event.Skip(); }
    virtual void OnNo( wxCommandEvent& event ) { event.Skip(); }
    virtual void OnCancel( wxCommandEvent& event ) { event.Skip(); }


public:

    QuestionDlgGenerated( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = _("Question"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( -1,-1 ), long style = wxDEFAULT_DIALOG_STYLE|wxMAXIMIZE_BOX|wxMINIMIZE_BOX|wxRESIZE_BORDER );
    ~QuestionDlgGenerated();

};

///////////////////////////////////////////////////////////////////////////////
/// Class DeleteDlgGenerated
///////////////////////////////////////////////////////////////////////////////
class DeleteDlgGenerated : public wxDialog
{
private:

protected:
    wxPanel* m_panelHeader;
    wxStaticBitmap* m_bitmap12;
    wxStaticText* m_staticTextHeader;
    wxStaticLine* m_staticline91;
    wxTextCtrl* m_textCtrlFileList;
    wxStaticLine* m_staticline9;
    wxPanel* m_panel36;
    wxCheckBox* m_checkBoxUseRecycler;
    wxCheckBox* m_checkBoxDeleteBothSides;
    wxButton* m_buttonOK;
    wxButton* m_buttonCancel;

    // Virtual event handlers, overide them in your derived class
    virtual void OnClose( wxCloseEvent& event ) { event.Skip(); }
    virtual void OnUseRecycler( wxCommandEvent& event ) { event.Skip(); }
    virtual void OnDelOnBothSides( wxCommandEvent& event ) { event.Skip(); }
    virtual void OnOK( wxCommandEvent& event ) { event.Skip(); }
    virtual void OnCancel( wxCommandEvent& event ) { event.Skip(); }


public:

    DeleteDlgGenerated( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = _("Confirm"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( -1,-1 ), long style = wxDEFAULT_DIALOG_STYLE|wxMAXIMIZE_BOX|wxMINIMIZE_BOX|wxRESIZE_BORDER );
    ~DeleteDlgGenerated();

};

///////////////////////////////////////////////////////////////////////////////
/// Class FilterDlgGenerated
///////////////////////////////////////////////////////////////////////////////
class FilterDlgGenerated : public wxDialog
{
private:

protected:
    wxStaticBitmap* m_bitmap26;
    wxPanel* m_panel8;
    wxStaticText* m_staticTexHeader;
    wxStaticText* m_staticText44;
    wxBitmapButton* m_bpButtonHelp;
    wxStaticBitmap* m_bitmapInclude;
    wxTextCtrl* m_textCtrlInclude;
    wxStaticBitmap* m_bitmapExclude;
    wxTextCtrl* m_textCtrlExclude;
    wxStaticBitmap* m_bitmapFilterDate;
    wxSpinCtrl* m_spinCtrlTimespan;
    wxChoice* m_choiceUnitTimespan;
    wxStaticBitmap* m_bitmapFilterSize;
    wxStaticText* m_staticText101;
    wxSpinCtrl* m_spinCtrlMinSize;
    wxChoice* m_choiceUnitMinSize;
    wxStaticText* m_staticText102;
    wxSpinCtrl* m_spinCtrlMaxSize;
    wxChoice* m_choiceUnitMaxSize;
    wxButton* m_button9;
    wxButton* m_buttonOk;
    wxButton* m_button17;

    // Virtual event handlers, overide them in your derived class
    virtual void OnClose( wxCloseEvent& event ) { event.Skip(); }
    virtual void OnHelp( wxCommandEvent& event ) { event.Skip(); }
    virtual void OnUpdateNameFilter( wxCommandEvent& event ) { event.Skip(); }
    virtual void OnUpdateChoice( wxCommandEvent& event ) { event.Skip(); }
    virtual void OnDefault( wxCommandEvent& event ) { event.Skip(); }
    virtual void OnApply( wxCommandEvent& event ) { event.Skip(); }
    virtual void OnCancel( wxCommandEvent& event ) { event.Skip(); }


public:

    FilterDlgGenerated( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = _("Configure filter"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( -1,-1 ), long style = wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER );
    ~FilterDlgGenerated();

};

///////////////////////////////////////////////////////////////////////////////
/// Class GlobalSettingsDlgGenerated
///////////////////////////////////////////////////////////////////////////////
class GlobalSettingsDlgGenerated : public wxDialog
{
private:

protected:
    wxStaticBitmap* m_bitmapSettings;
    wxPanel* m_panel8;
    wxStaticText* m_staticText56;
    wxCheckBox* m_checkBoxTransCopy;
    wxStaticText* m_staticText82;
    wxCheckBox* m_checkBoxCopyLocked;
    wxStaticText* m_staticTextCopyLocked;
    wxCheckBox* m_checkBoxCopyPermissions;
    wxStaticText* m_staticText8211;
    zen::BitmapButton* m_buttonResetDialogs;
    wxGrid* m_gridCustomCommand;
    wxBitmapButton* m_bpButtonAddRow;
    wxBitmapButton* m_bpButtonRemoveRow;
    wxButton* m_button9;
    wxButton* m_buttonOkay;
    wxButton* m_button29;

    // Virtual event handlers, overide them in your derived class
    virtual void OnClose( wxCloseEvent& event ) { event.Skip(); }
    virtual void OnResetDialogs( wxCommandEvent& event ) { event.Skip(); }
    virtual void OnAddRow( wxCommandEvent& event ) { event.Skip(); }
    virtual void OnRemoveRow( wxCommandEvent& event ) { event.Skip(); }
    virtual void OnDefault( wxCommandEvent& event ) { event.Skip(); }
    virtual void OnOkay( wxCommandEvent& event ) { event.Skip(); }
    virtual void OnCancel( wxCommandEvent& event ) { event.Skip(); }


public:

    GlobalSettingsDlgGenerated( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = _("Global settings"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( -1,-1 ), long style = wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER );
    ~GlobalSettingsDlgGenerated();

};

///////////////////////////////////////////////////////////////////////////////
/// Class SyncPreviewDlgGenerated
///////////////////////////////////////////////////////////////////////////////
class SyncPreviewDlgGenerated : public wxDialog
{
private:

protected:
    zen::BitmapButton* m_buttonStartSync;
    wxStaticLine* m_staticline16;
    wxStaticText* m_staticTextVariant;
    wxStaticLine* m_staticline14;
    wxStaticBitmap* m_bitmapCreateLeft;
    wxStaticBitmap* m_bitmapUpdateLeft;
    wxStaticBitmap* m_bitmapDeleteLeft;
    wxStaticBitmap* m_bitmapData;
    wxStaticBitmap* m_bitmapDeleteRight;
    wxStaticBitmap* m_bitmapUpdateRight;
    wxStaticBitmap* m_bitmapCreateRight;
    wxStaticText* m_staticTextCreateLeft;
    wxStaticText* m_staticTextUpdateLeft;
    wxStaticText* m_staticTextDeleteLeft;
    wxStaticText* m_staticTextData;
    wxStaticText* m_staticTextDeleteRight;
    wxStaticText* m_staticTextUpdateRight;
    wxStaticText* m_staticTextCreateRight;
    wxStaticLine* m_staticline12;
    wxCheckBox* m_checkBoxDontShowAgain;
    wxButton* m_button16;

    // Virtual event handlers, overide them in your derived class
    virtual void OnClose( wxCloseEvent& event ) { event.Skip(); }
    virtual void OnStartSync( wxCommandEvent& event ) { event.Skip(); }
    virtual void OnCancel( wxCommandEvent& event ) { event.Skip(); }


public:

    SyncPreviewDlgGenerated( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = _("Summary"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize, long style = wxDEFAULT_DIALOG_STYLE );
    ~SyncPreviewDlgGenerated();

};

///////////////////////////////////////////////////////////////////////////////
/// Class PopupFrameGenerated1
///////////////////////////////////////////////////////////////////////////////
class PopupFrameGenerated1 : public wxFrame
{
private:

protected:

public:
    wxStaticBitmap* m_bitmapLeft;
    wxStaticText* m_staticTextMain;

    PopupFrameGenerated1( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = wxEmptyString, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( -1,-1 ), long style = wxFRAME_NO_TASKBAR|wxSTAY_ON_TOP|wxSTATIC_BORDER );

    ~PopupFrameGenerated1();

};

///////////////////////////////////////////////////////////////////////////////
/// Class SearchDialogGenerated
///////////////////////////////////////////////////////////////////////////////
class SearchDialogGenerated : public wxDialog
{
private:

protected:
    wxStaticText* m_staticText101;
    wxTextCtrl* m_textCtrlSearchTxt;
    wxCheckBox* m_checkBoxMatchCase;
    wxButton* m_buttonFindNext;
    wxButton* m_button29;

    // Virtual event handlers, overide them in your derived class
    virtual void OnClose( wxCloseEvent& event ) { event.Skip(); }
    virtual void OnText( wxCommandEvent& event ) { event.Skip(); }
    virtual void OnFindNext( wxCommandEvent& event ) { event.Skip(); }
    virtual void OnCancel( wxCommandEvent& event ) { event.Skip(); }


public:

    SearchDialogGenerated( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = _("Find"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( -1,-1 ), long style = wxDEFAULT_DIALOG_STYLE );
    ~SearchDialogGenerated();

};

///////////////////////////////////////////////////////////////////////////////
/// Class SelectTimespanDlgGenerated
///////////////////////////////////////////////////////////////////////////////
class SelectTimespanDlgGenerated : public wxDialog
{
private:

protected:
    wxCalendarCtrl* m_calendarFrom;
    wxCalendarCtrl* m_calendarTo;
    wxButton* m_buttonOkay;
    wxButton* m_button29;

    // Virtual event handlers, overide them in your derived class
    virtual void OnClose( wxCloseEvent& event ) { event.Skip(); }
    virtual void OnChangeSelectionFrom( wxCalendarEvent& event ) { event.Skip(); }
    virtual void OnChangeSelectionTo( wxCalendarEvent& event ) { event.Skip(); }
    virtual void OnOkay( wxCommandEvent& event ) { event.Skip(); }
    virtual void OnCancel( wxCommandEvent& event ) { event.Skip(); }


public:

    SelectTimespanDlgGenerated( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = _("Select time span"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize, long style = wxDEFAULT_DIALOG_STYLE );
    ~SelectTimespanDlgGenerated();

};

#endif //__GUI_GENERATED_H__
