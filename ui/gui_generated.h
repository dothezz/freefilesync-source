///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Oct  8 2012)
// http://www.wxformbuilder.org/
//
// PLEASE DO "NOT" EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#ifndef __GUI_GENERATED_H__
#define __GUI_GENERATED_H__

#include <wx/artprov.h>
#include <wx/xrc/xmlres.h>
#include <wx/intl.h>
class ExecFinishedBox;
class FolderHistoryBox;
class ToggleButton;
class wxStaticText;
namespace zen{ class BitmapButton; }
namespace zen{ class Graph2D; }
namespace zen{ class Grid; }
namespace zen{ class TripleSplitter; }

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
#include <wx/scrolwin.h>
#include <wx/statbmp.h>
#include <wx/statline.h>
#include <wx/listbox.h>
#include <wx/checkbox.h>
#include <wx/frame.h>
#include <wx/textctrl.h>
#include <wx/gauge.h>
#include <wx/statbox.h>
#include <wx/tglbtn.h>
#include <wx/spinctrl.h>
#include <wx/notebook.h>
#include <wx/dialog.h>
#include <wx/choice.h>
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
		wxMenuItem* m_menuItemNew;
		wxMenuItem* m_menuItemLoad;
		wxMenuItem* m_menuItemSave;
		wxMenuItem* m_menuItemSaveAs;
		wxMenuItem* m_menuItem7;
		wxMenuItem* m_menuItem10;
		wxMenuItem* m_menuItem11;
		wxMenu* m_menuAdvanced;
		wxMenu* m_menuLanguages;
		wxMenuItem* m_menuItemGlobSett;
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
		wxButton* m_buttonSelectDirLeft;
		wxPanel* m_panelTopMiddle;
		wxBitmapButton* m_bpButtonSwapSides;
		wxStaticText* m_staticTextFinalPathRight;
		wxButton* m_buttonSelectDirRight;
		wxScrolledWindow* m_scrolledWindowFolderPairs;
		wxBoxSizer* bSizerAddFolderPairs;
		zen::Grid* m_gridNavi;
		wxPanel* m_panelCenter;
		zen::TripleSplitter* m_splitterMain;
		zen::Grid* m_gridMainL;
		zen::Grid* m_gridMainC;
		zen::Grid* m_gridMainR;
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
		wxBitmapButton* m_bpButtonBatchJob;
		wxListBox* m_listBoxHistory;
		wxPanel* m_panelFilter;
		wxBitmapButton* m_bpButtonFilter;
		wxCheckBox* m_checkBoxShowExcluded;
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
		virtual void OnConfigNew( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnConfigLoad( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnConfigSave( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnConfigSaveAs( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnMenuBatchJob( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnCompare( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnStartSync( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnMenuQuit( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnMenuGlobalSettings( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnMenuExportFileList( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnShowHelp( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnMenuCheckVersion( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnMenuAbout( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnCmpSettings( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnSyncSettings( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnAddFolderPair( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnRemoveTopFolderPair( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnSwapSides( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnCfgHistoryKeyEvent( wxKeyEvent& event ) { event.Skip(); }
		virtual void OnLoadFromHistory( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnLoadFromHistoryDoubleClick( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnConfigureFilter( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnShowExcluded( wxCommandEvent& event ) { event.Skip(); }
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
		wxBitmapButton* m_bpButtonAltCompCfg;
		wxBitmapButton* m_bpButtonLocalFilter;
		wxBitmapButton* m_bpButtonAltSyncCfg;
		wxPanel* m_panelTopRight;
		FolderHistoryBox* m_directoryRight;
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
		wxButton* m_buttonSelectDirLeft;
		wxButton* m_buttonSelectDirRight;
	
	public:
		wxPanel* m_panelLeft;
		wxBitmapButton* m_bpButtonRemovePair;
		FolderHistoryBox* m_directoryLeft;
		wxPanel* m_panel20;
		wxBitmapButton* m_bpButtonAltCompCfg;
		wxBitmapButton* m_bpButtonLocalFilter;
		wxBitmapButton* m_bpButtonAltSyncCfg;
		wxPanel* m_panelRight;
		FolderHistoryBox* m_directoryRight;
		
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
		wxPanel* m_panel8;
		wxStaticBitmap* m_bitmap27;
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
		wxButton* m_buttonSelectDirLeft;
		wxButton* m_buttonSelectDirRight;
		wxBoxSizer* bSizerAddFolderPairs;
		wxPanel* m_panelBatchSettings;
		wxStaticBoxSizer* sbSizerErrorHandling;
		wxToggleButton* m_toggleBtnErrorIgnore;
		wxToggleButton* m_toggleBtnErrorPopup;
		wxToggleButton* m_toggleBtnErrorExit;
		wxStaticBoxSizer* sbSizerExecFinished;
		ExecFinishedBox* m_comboBoxExecFinished;
		wxCheckBox* m_checkBoxShowProgress;
		wxCheckBox* m_checkBoxGenerateLogfile;
		wxPanel* m_panelLogfile;
		wxButton* m_buttonSelectLogfileDir;
		wxCheckBox* m_checkBoxLogfilesLimit;
		wxSpinCtrl* m_spinCtrlLogfileLimit;
		wxButton* m_buttonLoad;
		wxButton* m_buttonSave;
		wxButton* m_button6;
		
		// Virtual event handlers, overide them in your derived class
		virtual void OnClose( wxCloseEvent& event ) { event.Skip(); }
		virtual void OnHelp( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnCmpSettings( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnConfigureFilter( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnSyncSettings( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnAddFolderPair( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnRemoveTopFolderPair( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnErrorIgnore( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnErrorPopup( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnErrorExit( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnToggleGenerateLogfile( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnToggleLogfilesLimit( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnLoadBatchJob( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnSaveBatchJob( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnCancel( wxCommandEvent& event ) { event.Skip(); }
		
	
	public:
		wxScrolledWindow* m_scrolledWindow6;
		wxBitmapButton* m_bpButtonAddPair;
		wxBitmapButton* m_bpButtonRemovePair;
		wxPanel* m_panelLeft;
		FolderHistoryBox* m_directoryLeft;
		wxPanel* m_panelRight;
		FolderHistoryBox* m_directoryRight;
		wxBitmapButton* m_bpButtonAltCompCfg;
		wxBitmapButton* m_bpButtonLocalFilter;
		wxBitmapButton* m_bpButtonAltSyncCfg;
		FolderHistoryBox* m_comboBoxLogfileDir;
		
		BatchDlgGenerated( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = _("Save as batch job"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( -1,-1 ), long style = wxDEFAULT_DIALOG_STYLE|wxMAXIMIZE_BOX|wxMINIMIZE_BOX|wxRESIZE_BORDER ); 
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
		wxButton* m_buttonSelectDirLeft;
		wxPanel* m_panelRight;
		wxButton* m_buttonSelectDirRight;
	
	public:
		wxBitmapButton* m_bpButtonRemovePair;
		FolderHistoryBox* m_directoryLeft;
		FolderHistoryBox* m_directoryRight;
		wxBitmapButton* m_bpButtonAltCompCfg;
		wxBitmapButton* m_bpButtonLocalFilter;
		wxBitmapButton* m_bpButtonAltSyncCfg;
		
		BatchFolderPairGenerated( wxWindow* parent, wxWindowID id = wxID_ANY, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( -1,-1 ), long style = wxTAB_TRAVERSAL ); 
		~BatchFolderPairGenerated();
	
};

///////////////////////////////////////////////////////////////////////////////
/// Class CmpCfgDlgGenerated
///////////////////////////////////////////////////////////////////////////////
class CmpCfgDlgGenerated : public wxDialog 
{
	private:
	
	protected:
		wxStaticBitmap* m_bitmapByTime;
		wxToggleButton* m_toggleBtnTimeSize;
		wxStaticBitmap* m_bitmapByContent;
		wxToggleButton* m_toggleBtnContent;
		wxChoice* m_choiceHandleSymlinks;
		wxBitmapButton* m_bpButtonHelp;
		wxButton* m_button10;
		wxButton* m_button6;
		
		// Virtual event handlers, overide them in your derived class
		virtual void OnClose( wxCloseEvent& event ) { event.Skip(); }
		virtual void OnTimeSizeDouble( wxMouseEvent& event ) { event.Skip(); }
		virtual void OnTimeSize( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnContentDouble( wxMouseEvent& event ) { event.Skip(); }
		virtual void OnContent( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnChangeErrorHandling( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnShowHelp( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnOkay( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnCancel( wxCommandEvent& event ) { event.Skip(); }
		
	
	public:
		
		CmpCfgDlgGenerated( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = _("Comparison settings"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize, long style = wxDEFAULT_DIALOG_STYLE ); 
		~CmpCfgDlgGenerated();
	
};

///////////////////////////////////////////////////////////////////////////////
/// Class SyncCfgDlgGenerated
///////////////////////////////////////////////////////////////////////////////
class SyncCfgDlgGenerated : public wxDialog 
{
	private:
	
	protected:
		wxToggleButton* m_toggleBtnAutomatic;
		wxStaticText* m_staticTextAutomatic;
		wxToggleButton* m_toggleBtnMirror;
		wxStaticText* m_staticTextMirror;
		wxToggleButton* m_toggleBtnUpdate;
		wxStaticText* m_staticTextUpdate;
		wxToggleButton* m_toggleBtnCustom;
		wxStaticText* m_staticTextCustom;
		wxToggleButton* m_toggleBtnPermanent;
		wxToggleButton* m_toggleBtnRecycler;
		wxToggleButton* m_toggleBtnVersioning;
		wxCheckBox* m_checkBoxVersionsLimit;
		wxSpinCtrl* m_spinCtrlVersionsLimit;
		wxPanel* m_panelVersioning;
		FolderHistoryBox* m_versioningFolder;
		wxButton* m_buttonSelectDirVersioning;
		wxBoxSizer* bSizer201;
		wxStaticBoxSizer* sbSizerErrorHandling;
		wxToggleButton* m_toggleBtnErrorIgnore;
		wxToggleButton* m_toggleBtnErrorPopup;
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
		virtual void OnSyncAutomaticDouble( wxMouseEvent& event ) { event.Skip(); }
		virtual void OnSyncAutomatic( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnSyncMirrorDouble( wxMouseEvent& event ) { event.Skip(); }
		virtual void OnSyncMirror( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnSyncUpdateDouble( wxMouseEvent& event ) { event.Skip(); }
		virtual void OnSyncUpdate( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnSyncCustomDouble( wxMouseEvent& event ) { event.Skip(); }
		virtual void OnSyncCustom( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnDeletionPermanent( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnDeletionRecycler( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnDeletionVersioning( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnToggleVersionsLimit( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnErrorIgnore( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnErrorPopup( wxCommandEvent& event ) { event.Skip(); }
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
		wxStaticLine* m_staticline12;
		ToggleButton* m_bpButtonErrors;
		ToggleButton* m_bpButtonWarnings;
		ToggleButton* m_bpButtonInfo;
		wxStaticLine* m_staticline13;
		zen::Grid* m_gridMessages;
		
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
		wxPanel* m_panelLogo;
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
		wxPanel* m_panel40;
		wxPanel* m_panel39;
		wxStaticText* m_staticText83;
		wxHyperlinkCtrl* m_hyperlink3;
		wxAnimationCtrl* m_animCtrlWink;
		wxScrolledWindow* m_scrolledWindowTranslators;
		wxBoxSizer* bSizerTranslators;
		wxStaticText* m_staticText54;
		wxFlexGridSizer* fgSizerTranslators;
		wxHyperlinkCtrl* m_hyperlink1;
		wxStaticBitmap* m_bitmap9;
		wxHyperlinkCtrl* m_hyperlink2;
		wxStaticBitmap* m_bitmap10;
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
/// Class MessageDlgGenerated
///////////////////////////////////////////////////////////////////////////////
class MessageDlgGenerated : public wxDialog 
{
	private:
	
	protected:
		wxStaticBitmap* m_bitmapMsgType;
		wxTextCtrl* m_textCtrlMessage;
		wxStaticLine* m_staticline6;
		wxPanel* m_panel33;
		wxCheckBox* m_checkBoxCustom;
		wxButton* m_buttonCustom1;
		wxButton* m_buttonCustom2;
		wxButton* m_buttonCancel;
		
		// Virtual event handlers, overide them in your derived class
		virtual void OnClose( wxCloseEvent& event ) { event.Skip(); }
		virtual void OnButton1( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnButton2( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnCancel( wxCommandEvent& event ) { event.Skip(); }
		
	
	public:
		
		MessageDlgGenerated( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = _("dummy"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( -1,-1 ), long style = wxDEFAULT_DIALOG_STYLE|wxMAXIMIZE_BOX|wxMINIMIZE_BOX|wxRESIZE_BORDER ); 
		~MessageDlgGenerated();
	
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
		wxPanel* m_panel8;
		wxStaticBitmap* m_bitmap26;
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
		wxPanel* m_panel8;
		wxStaticBitmap* m_bitmapSettings;
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
