///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Nov  6 2013)
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
namespace zen{ class BitmapTextButton; }
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
#include <wx/button.h>
#include <wx/bmpbuttn.h>
#include <wx/sizer.h>
#include <wx/panel.h>
#include <wx/stattext.h>
#include <wx/combobox.h>
#include <wx/scrolwin.h>
#include <wx/statbmp.h>
#include <wx/statline.h>
#include <wx/textctrl.h>
#include <wx/checkbox.h>
#include <wx/listbox.h>
#include <wx/frame.h>
#include <wx/tglbtn.h>
#include <wx/choice.h>
#include <wx/hyperlink.h>
#include <wx/dialog.h>
#include <wx/gauge.h>
#include <wx/animate.h>
#include <wx/notebook.h>
#include <wx/spinctrl.h>
#include <wx/grid.h>
#include <wx/calctrl.h>

#include "zen/i18n.h"

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
		wxMenu* m_menuTools;
		wxMenuItem* m_menuItemGlobSett;
		wxMenu* m_menuLanguages;
		wxMenu* m_menuHelp;
		wxMenuItem* m_menuItemManual;
		wxMenu* m_menuCheckVersion;
		wxMenuItem* m_menuItemCheckVersionNow;
		wxMenuItem* m_menuItemCheckVersionAuto;
		wxMenuItem* m_menuItemAbout;
		wxBoxSizer* bSizerPanelHolder;
		wxPanel* m_panelTopButtons;
		wxBoxSizer* bSizerTopButtons;
		zen::BitmapTextButton* m_buttonCompare;
		zen::BitmapTextButton* m_buttonCancel;
		wxBitmapButton* m_bpButtonCmpConfig;
		wxBitmapButton* m_bpButtonFilter;
		wxBitmapButton* m_bpButtonSyncConfig;
		zen::BitmapTextButton* m_buttonSync;
		wxPanel* m_panelDirectoryPairs;
		wxStaticText* m_staticTextResolvedPathL;
		wxBitmapButton* m_bpButtonAddPair;
		wxButton* m_buttonSelectDirLeft;
		wxPanel* m_panelTopMiddle;
		wxBitmapButton* m_bpButtonSwapSides;
		wxStaticText* m_staticTextResolvedPathR;
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
		wxBoxSizer* bSizerFileStatus;
		wxBoxSizer* bSizerStatusLeft;
		wxBoxSizer* bSizerStatusLeftDirectories;
		wxStaticBitmap* m_bitmapSmallDirectoryLeft;
		wxStaticText* m_staticTextStatusLeftDirs;
		wxBoxSizer* bSizerStatusLeftFiles;
		wxStaticBitmap* m_bitmapSmallFileLeft;
		wxStaticText* m_staticTextStatusLeftFiles;
		wxStaticText* m_staticTextStatusLeftBytes;
		wxStaticLine* m_staticline9;
		wxStaticText* m_staticTextStatusMiddle;
		wxBoxSizer* bSizerStatusRight;
		wxStaticLine* m_staticline10;
		wxBoxSizer* bSizerStatusRightDirectories;
		wxStaticBitmap* m_bitmapSmallDirectoryRight;
		wxStaticText* m_staticTextStatusRightDirs;
		wxBoxSizer* bSizerStatusRightFiles;
		wxStaticBitmap* m_bitmapSmallFileRight;
		wxStaticText* m_staticTextStatusRightFiles;
		wxStaticText* m_staticTextStatusRightBytes;
		wxStaticText* m_staticTextFullStatus;
		wxPanel* m_panelSearch;
		wxBitmapButton* m_bpButtonHideSearch;
		wxStaticText* m_staticText101;
		wxTextCtrl* m_textCtrlSearchTxt;
		wxCheckBox* m_checkBoxMatchCase;
		wxPanel* m_panelConfig;
		wxBoxSizer* bSizerConfig;
		wxBitmapButton* m_bpButtonOpen;
		wxBitmapButton* m_bpButtonSave;
		wxBitmapButton* m_bpButtonBatchJob;
		wxListBox* m_listBoxHistory;
		wxPanel* m_panelViewFilter;
		wxBoxSizer* bSizerViewFilter;
		ToggleButton* m_bpButtonViewTypeSyncAction;
		ToggleButton* m_bpButtonShowExcluded;
		ToggleButton* m_bpButtonShowCreateLeft;
		ToggleButton* m_bpButtonShowUpdateLeft;
		ToggleButton* m_bpButtonShowDeleteLeft;
		ToggleButton* m_bpButtonShowLeftOnly;
		ToggleButton* m_bpButtonShowLeftNewer;
		ToggleButton* m_bpButtonShowEqual;
		ToggleButton* m_bpButtonShowDifferent;
		ToggleButton* m_bpButtonShowDoNothing;
		ToggleButton* m_bpButtonShowRightNewer;
		ToggleButton* m_bpButtonShowRightOnly;
		ToggleButton* m_bpButtonShowDeleteRight;
		ToggleButton* m_bpButtonShowUpdateRight;
		ToggleButton* m_bpButtonShowCreateRight;
		ToggleButton* m_bpButtonShowConflict;
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
		
		// Virtual event handlers, overide them in your derived class
		virtual void OnClose( wxCloseEvent& event ) { event.Skip(); }
		virtual void OnConfigNew( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnConfigLoad( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnConfigSave( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnConfigSaveAs( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnSaveAsBatchJob( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnCompare( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnStartSync( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnMenuQuit( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnMenuGlobalSettings( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnMenuFindItem( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnMenuExportFileList( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnShowHelp( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnMenuCheckVersion( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnMenuCheckVersionAutomatically( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnMenuAbout( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnCmpSettings( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnCompSettingsContext( wxMouseEvent& event ) { event.Skip(); }
		virtual void OnConfigureFilter( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnGlobalFilterContext( wxMouseEvent& event ) { event.Skip(); }
		virtual void OnSyncSettings( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnSyncSettingsContext( wxMouseEvent& event ) { event.Skip(); }
		virtual void OnAddFolderPair( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnRemoveTopFolderPair( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnSwapSides( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnHideSearchPanel( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnSearchGridEnter( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnCfgHistoryKeyEvent( wxKeyEvent& event ) { event.Skip(); }
		virtual void OnLoadFromHistory( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnLoadFromHistoryDoubleClick( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnCfgHistoryRightClick( wxMouseEvent& event ) { event.Skip(); }
		virtual void OnToggleViewType( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnToggleViewButton( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnViewButtonRightClick( wxMouseEvent& event ) { event.Skip(); }
		
	
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
		
		MainDialogGenerated( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = _("dummy"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( 900,600 ), long style = wxDEFAULT_FRAME_STYLE|wxTAB_TRAVERSAL );
		
		~MainDialogGenerated();
	
};

///////////////////////////////////////////////////////////////////////////////
/// Class CmpCfgDlgGenerated
///////////////////////////////////////////////////////////////////////////////
class CmpCfgDlgGenerated : public wxDialog 
{
	private:
	
	protected:
		wxPanel* m_panel36;
		wxStaticText* m_staticText91;
		wxStaticBitmap* m_bitmapByTime;
		wxToggleButton* m_toggleBtnTimeSize;
		wxStaticBitmap* m_bitmapByContent;
		wxToggleButton* m_toggleBtnContent;
		wxStaticLine* m_staticline33;
		wxStaticText* m_staticText92;
		wxChoice* m_choiceHandleSymlinks;
		wxHyperlinkCtrl* m_hyperlink24;
		wxStaticLine* m_staticline14;
		wxBoxSizer* bSizerStdButtons;
		wxButton* m_buttonOkay;
		wxButton* m_buttonCancel;
		
		// Virtual event handlers, overide them in your derived class
		virtual void OnClose( wxCloseEvent& event ) { event.Skip(); }
		virtual void OnTimeSizeDouble( wxMouseEvent& event ) { event.Skip(); }
		virtual void OnTimeSize( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnContentDouble( wxMouseEvent& event ) { event.Skip(); }
		virtual void OnContent( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnChangeErrorHandling( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnHelpComparisonSettings( wxHyperlinkEvent& event ) { event.Skip(); }
		virtual void OnOkay( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnCancel( wxCommandEvent& event ) { event.Skip(); }
		
	
	public:
		
		CmpCfgDlgGenerated( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = _("dummy"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( -1,-1 ), long style = wxDEFAULT_DIALOG_STYLE ); 
		~CmpCfgDlgGenerated();
	
};

///////////////////////////////////////////////////////////////////////////////
/// Class SyncCfgDlgGenerated
///////////////////////////////////////////////////////////////////////////////
class SyncCfgDlgGenerated : public wxDialog 
{
	private:
	
	protected:
		wxPanel* m_panel37;
		wxStaticText* m_staticText86;
		wxToggleButton* m_toggleBtnTwoWay;
		wxStaticText* m_staticTextAutomatic;
		wxToggleButton* m_toggleBtnMirror;
		wxStaticText* m_staticTextMirror;
		wxToggleButton* m_toggleBtnUpdate;
		wxStaticText* m_staticTextUpdate;
		wxToggleButton* m_toggleBtnCustom;
		wxStaticText* m_staticTextCustom;
		wxCheckBox* m_checkBoxDetectMove;
		wxStaticLine* m_staticline32;
		wxStaticText* m_staticText87;
		wxToggleButton* m_toggleBtnPermanent;
		wxToggleButton* m_toggleBtnRecycler;
		wxToggleButton* m_toggleBtnVersioning;
		wxPanel* m_panelVersioning;
		FolderHistoryBox* m_versioningFolder;
		wxButton* m_buttonSelectDirVersioning;
		wxBoxSizer* bSizer192;
		wxStaticText* m_staticText93;
		wxChoice* m_choiceVersioningStyle;
		wxStaticText* m_staticTextNamingCvtPart1;
		wxStaticText* m_staticTextNamingCvtPart2Bold;
		wxStaticText* m_staticTextNamingCvtPart3;
		wxHyperlinkCtrl* m_hyperlink17;
		wxBoxSizer* bSizerExtraConfig;
		wxStaticLine* m_staticline321;
		wxBoxSizer* bSizer179;
		wxStaticText* m_staticText88;
		wxToggleButton* m_toggleBtnErrorIgnore;
		wxToggleButton* m_toggleBtnErrorPopup;
		wxStaticLine* m_staticline36;
		wxBoxSizer* bSizerOnCompletion;
		wxStaticText* m_staticText89;
		ExecFinishedBox* m_comboBoxExecFinished;
		wxStaticLine* m_staticline31;
		wxBoxSizer* bSizerConfig;
		wxStaticText* m_staticTextHeaderCategory1;
		wxStaticText* m_staticTextHeaderAction1;
		wxStaticBitmap* m_bitmapDatabase;
		wxBoxSizer* sbSizerSyncDirections;
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
		wxStaticLine* m_staticline15;
		wxBoxSizer* bSizerStdButtons;
		wxButton* m_buttonOK;
		wxButton* m_buttonCancel;
		
		// Virtual event handlers, overide them in your derived class
		virtual void OnClose( wxCloseEvent& event ) { event.Skip(); }
		virtual void OnSyncTwoWayDouble( wxMouseEvent& event ) { event.Skip(); }
		virtual void OnSyncTwoWay( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnSyncMirrorDouble( wxMouseEvent& event ) { event.Skip(); }
		virtual void OnSyncMirror( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnSyncUpdateDouble( wxMouseEvent& event ) { event.Skip(); }
		virtual void OnSyncUpdate( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnSyncCustomDouble( wxMouseEvent& event ) { event.Skip(); }
		virtual void OnSyncCustom( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnToggleDetectMovedFiles( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnDeletionPermanent( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnDeletionRecycler( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnDeletionVersioning( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnParameterChange( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnHelpVersioning( wxHyperlinkEvent& event ) { event.Skip(); }
		virtual void OnErrorIgnore( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnErrorPopup( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnExLeftSideOnly( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnExRightSideOnly( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnLeftNewer( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnRightNewer( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnDifferent( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnConflict( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnOkay( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnCancel( wxCommandEvent& event ) { event.Skip(); }
		
	
	public:
		
		SyncCfgDlgGenerated( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = _("dummy"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize, long style = wxDEFAULT_DIALOG_STYLE ); 
		~SyncCfgDlgGenerated();
	
};

///////////////////////////////////////////////////////////////////////////////
/// Class SyncConfirmationDlgGenerated
///////////////////////////////////////////////////////////////////////////////
class SyncConfirmationDlgGenerated : public wxDialog 
{
	private:
	
	protected:
		wxStaticBitmap* m_bitmapSync;
		wxStaticText* m_staticTextHeader;
		wxStaticLine* m_staticline371;
		wxPanel* m_panelStatistics;
		wxStaticLine* m_staticline38;
		wxStaticText* m_staticText84;
		wxStaticText* m_staticTextVariant;
		wxStaticLine* m_staticline14;
		wxStaticText* m_staticText83;
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
		wxStaticLine* m_staticline381;
		wxStaticLine* m_staticline12;
		wxCheckBox* m_checkBoxDontShowAgain;
		wxBoxSizer* bSizerStdButtons;
		wxButton* m_buttonStartSync;
		wxButton* m_buttonCancel;
		
		// Virtual event handlers, overide them in your derived class
		virtual void OnClose( wxCloseEvent& event ) { event.Skip(); }
		virtual void OnStartSync( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnCancel( wxCommandEvent& event ) { event.Skip(); }
		
	
	public:
		
		SyncConfirmationDlgGenerated( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = _("FreeFileSync"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize, long style = wxDEFAULT_DIALOG_STYLE ); 
		~SyncConfirmationDlgGenerated();
	
};

///////////////////////////////////////////////////////////////////////////////
/// Class FolderPairPanelGenerated
///////////////////////////////////////////////////////////////////////////////
class FolderPairPanelGenerated : public wxPanel 
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
		
		FolderPairPanelGenerated( wxWindow* parent, wxWindowID id = wxID_ANY, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize, long style = 0 ); 
		~FolderPairPanelGenerated();
	
};

///////////////////////////////////////////////////////////////////////////////
/// Class CompareProgressDlgGenerated
///////////////////////////////////////////////////////////////////////////////
class CompareProgressDlgGenerated : public wxPanel 
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
		
		CompareProgressDlgGenerated( wxWindow* parent, wxWindowID id = wxID_ANY, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize, long style = wxRAISED_BORDER ); 
		~CompareProgressDlgGenerated();
	
};

///////////////////////////////////////////////////////////////////////////////
/// Class SyncProgressPanelGenerated
///////////////////////////////////////////////////////////////////////////////
class SyncProgressPanelGenerated : public wxPanel 
{
	private:
	
	protected:
		wxBoxSizer* bSizer42;
		wxBoxSizer* bSizer171;
		wxStaticText* m_staticText87;
	
	public:
		wxBoxSizer* bSizerRoot;
		wxStaticBitmap* m_bitmapStatus;
		wxStaticText* m_staticTextPhase;
		wxAnimationCtrl* m_animCtrlSyncing;
		wxBitmapButton* m_bpButtonMinimizeToTray;
		wxBoxSizer* bSizerStatusText;
		wxStaticText* m_staticTextStatus;
		wxPanel* m_panelProgress;
		wxPanel* m_panelItemsProcessed;
		wxStaticText* m_staticTextProcessedObj;
		wxStaticText* m_staticTextDataProcessed;
		wxPanel* m_panelItemsRemaining;
		wxStaticText* m_staticTextRemainingObj;
		wxStaticText* m_staticTextDataRemaining;
		wxPanel* m_panelTimeRemaining;
		wxStaticText* m_staticTextRemTime;
		wxStaticText* m_staticTextTimeElapsed;
		zen::Graph2D* m_panelGraphBytes;
		zen::Graph2D* m_panelGraphItems;
		wxNotebook* m_notebookResult;
		wxStaticLine* m_staticlineFooter;
		wxBoxSizer* bSizerStdButtons;
		wxBoxSizer* bSizerExecFinished;
		ExecFinishedBox* m_comboBoxExecFinished;
		wxButton* m_buttonClose;
		wxButton* m_buttonPause;
		wxButton* m_buttonStop;
		
		SyncProgressPanelGenerated( wxWindow* parent, wxWindowID id = wxID_ANY, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( -1,-1 ), long style = wxTAB_TRAVERSAL ); 
		~SyncProgressPanelGenerated();
	
};

///////////////////////////////////////////////////////////////////////////////
/// Class LogPanelGenerated
///////////////////////////////////////////////////////////////////////////////
class LogPanelGenerated : public wxPanel 
{
	private:
	
	protected:
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
		
		LogPanelGenerated( wxWindow* parent, wxWindowID id = wxID_ANY, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize, long style = wxTAB_TRAVERSAL ); 
		~LogPanelGenerated();
	
};

///////////////////////////////////////////////////////////////////////////////
/// Class BatchDlgGenerated
///////////////////////////////////////////////////////////////////////////////
class BatchDlgGenerated : public wxDialog 
{
	private:
	
	protected:
		wxStaticBitmap* m_bitmapBatchJob;
		wxStaticText* m_staticTextDescr;
		wxStaticLine* m_staticline18;
		wxPanel* m_panel35;
		wxStaticText* m_staticText82;
		wxToggleButton* m_toggleBtnErrorIgnore;
		wxToggleButton* m_toggleBtnErrorPopup;
		wxToggleButton* m_toggleBtnErrorStop;
		wxStaticLine* m_staticline26;
		wxCheckBox* m_checkBoxShowProgress;
		wxStaticText* m_staticText81;
		ExecFinishedBox* m_comboBoxExecFinished;
		wxStaticLine* m_staticline25;
		wxCheckBox* m_checkBoxGenerateLogfile;
		wxPanel* m_panelLogfile;
		wxButton* m_buttonSelectLogfileDir;
		wxCheckBox* m_checkBoxLogfilesLimit;
		wxSpinCtrl* m_spinCtrlLogfileLimit;
		wxHyperlinkCtrl* m_hyperlink17;
		wxStaticLine* m_staticline13;
		wxBoxSizer* bSizerStdButtons;
		wxButton* m_buttonSaveAs;
		wxButton* m_buttonCancel;
		
		// Virtual event handlers, overide them in your derived class
		virtual void OnClose( wxCloseEvent& event ) { event.Skip(); }
		virtual void OnErrorIgnore( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnErrorPopup( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnErrorStop( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnToggleGenerateLogfile( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnToggleLogfilesLimit( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnHelpScheduleBatch( wxHyperlinkEvent& event ) { event.Skip(); }
		virtual void OnSaveBatchJob( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnCancel( wxCommandEvent& event ) { event.Skip(); }
		
	
	public:
		FolderHistoryBox* m_logfileDir;
		
		BatchDlgGenerated( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = _("Save as Batch Job"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize, long style = wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER ); 
		~BatchDlgGenerated();
	
};

///////////////////////////////////////////////////////////////////////////////
/// Class DeleteDlgGenerated
///////////////////////////////////////////////////////////////////////////////
class DeleteDlgGenerated : public wxDialog 
{
	private:
	
	protected:
		wxStaticBitmap* m_bitmapDeleteType;
		wxStaticText* m_staticTextHeader;
		wxStaticLine* m_staticline91;
		wxPanel* m_panel31;
		wxStaticLine* m_staticline42;
		wxTextCtrl* m_textCtrlFileList;
		wxStaticLine* m_staticline9;
		wxBoxSizer* bSizerStdButtons;
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
		
		DeleteDlgGenerated( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = _("Delete Items"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( -1,-1 ), long style = wxDEFAULT_DIALOG_STYLE|wxMAXIMIZE_BOX|wxRESIZE_BORDER ); 
		~DeleteDlgGenerated();
	
};

///////////////////////////////////////////////////////////////////////////////
/// Class FilterDlgGenerated
///////////////////////////////////////////////////////////////////////////////
class FilterDlgGenerated : public wxDialog 
{
	private:
	
	protected:
		wxStaticBitmap* m_bitmapFilter;
		wxStaticText* m_staticText44;
		wxStaticLine* m_staticline17;
		wxPanel* m_panel38;
		wxStaticBitmap* m_bitmapInclude;
		wxStaticText* m_staticText78;
		wxTextCtrl* m_textCtrlInclude;
		wxStaticLine* m_staticline22;
		wxStaticBitmap* m_bitmapExclude;
		wxStaticText* m_staticText77;
		wxHyperlinkCtrl* m_hyperlink17;
		wxTextCtrl* m_textCtrlExclude;
		wxStaticLine* m_staticline24;
		wxStaticBitmap* m_bitmapFilterDate;
		wxStaticText* m_staticText79;
		wxSpinCtrl* m_spinCtrlTimespan;
		wxChoice* m_choiceUnitTimespan;
		wxStaticLine* m_staticline23;
		wxStaticBitmap* m_bitmapFilterSize;
		wxStaticText* m_staticText80;
		wxStaticText* m_staticText101;
		wxSpinCtrl* m_spinCtrlMinSize;
		wxChoice* m_choiceUnitMinSize;
		wxStaticText* m_staticText102;
		wxSpinCtrl* m_spinCtrlMaxSize;
		wxChoice* m_choiceUnitMaxSize;
		wxStaticLine* m_staticline16;
		wxBoxSizer* bSizerStdButtons;
		wxButton* m_buttonClear;
		wxButton* m_buttonOk;
		wxButton* m_buttonCancel;
		
		// Virtual event handlers, overide them in your derived class
		virtual void OnClose( wxCloseEvent& event ) { event.Skip(); }
		virtual void OnUpdateNameFilter( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnHelpShowExamples( wxHyperlinkEvent& event ) { event.Skip(); }
		virtual void OnUpdateChoice( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnClear( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnOkay( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnCancel( wxCommandEvent& event ) { event.Skip(); }
		
	
	public:
		
		FilterDlgGenerated( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = _("dummy"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize, long style = wxDEFAULT_DIALOG_STYLE|wxMAXIMIZE_BOX|wxRESIZE_BORDER ); 
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
		wxStaticText* m_staticText44;
		wxStaticLine* m_staticline20;
		wxPanel* m_panel39;
		wxCheckBox* m_checkBoxFailSafe;
		wxStaticText* m_staticText91;
		wxBoxSizer* bSizerLockedFiles;
		wxCheckBox* m_checkBoxCopyLocked;
		wxStaticText* m_staticText92;
		wxCheckBox* m_checkBoxCopyPermissions;
		wxStaticText* m_staticText93;
		wxStaticLine* m_staticline39;
		wxStaticText* m_staticText95;
		wxStaticText* m_staticText96;
		wxSpinCtrl* m_spinCtrlAutoRetryCount;
		wxStaticText* m_staticTextAutoRetryDelay;
		wxSpinCtrl* m_spinCtrlAutoRetryDelay;
		wxStaticLine* m_staticline191;
		wxStaticText* m_staticText85;
		wxGrid* m_gridCustomCommand;
		wxBitmapButton* m_bpButtonAddRow;
		wxBitmapButton* m_bpButtonRemoveRow;
		wxHyperlinkCtrl* m_hyperlink17;
		wxStaticLine* m_staticline192;
		zen::BitmapTextButton* m_buttonResetDialogs;
		wxStaticLine* m_staticline40;
		wxStaticLine* m_staticline36;
		wxBoxSizer* bSizerStdButtons;
		wxButton* m_buttonDefault;
		wxButton* m_buttonOkay;
		wxButton* m_buttonCancel;
		
		// Virtual event handlers, overide them in your derived class
		virtual void OnClose( wxCloseEvent& event ) { event.Skip(); }
		virtual void OnToggleAutoRetryCount( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnAddRow( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnRemoveRow( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnHelpShowExamples( wxHyperlinkEvent& event ) { event.Skip(); }
		virtual void OnResetDialogs( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnDefault( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnOkay( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnCancel( wxCommandEvent& event ) { event.Skip(); }
		
	
	public:
		
		GlobalSettingsDlgGenerated( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = _("Global Settings"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize, long style = wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER ); 
		~GlobalSettingsDlgGenerated();
	
};

///////////////////////////////////////////////////////////////////////////////
/// Class TooltipDialogGenerated
///////////////////////////////////////////////////////////////////////////////
class TooltipDialogGenerated : public wxDialog 
{
	private:
	
	protected:
	
	public:
		wxStaticBitmap* m_bitmapLeft;
		wxStaticText* m_staticTextMain;
		
		TooltipDialogGenerated( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = wxEmptyString, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize, long style = wxDEFAULT_DIALOG_STYLE ); 
		~TooltipDialogGenerated();
	
};

///////////////////////////////////////////////////////////////////////////////
/// Class SelectTimespanDlgGenerated
///////////////////////////////////////////////////////////////////////////////
class SelectTimespanDlgGenerated : public wxDialog 
{
	private:
	
	protected:
		wxPanel* m_panel35;
		wxCalendarCtrl* m_calendarFrom;
		wxCalendarCtrl* m_calendarTo;
		wxStaticLine* m_staticline21;
		wxBoxSizer* bSizerStdButtons;
		wxButton* m_buttonOkay;
		wxButton* m_buttonCancel;
		
		// Virtual event handlers, overide them in your derived class
		virtual void OnClose( wxCloseEvent& event ) { event.Skip(); }
		virtual void OnChangeSelectionFrom( wxCalendarEvent& event ) { event.Skip(); }
		virtual void OnChangeSelectionTo( wxCalendarEvent& event ) { event.Skip(); }
		virtual void OnOkay( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnCancel( wxCommandEvent& event ) { event.Skip(); }
		
	
	public:
		
		SelectTimespanDlgGenerated( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = _("Select Time Span"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize, long style = wxDEFAULT_DIALOG_STYLE ); 
		~SelectTimespanDlgGenerated();
	
};

///////////////////////////////////////////////////////////////////////////////
/// Class AboutDlgGenerated
///////////////////////////////////////////////////////////////////////////////
class AboutDlgGenerated : public wxDialog 
{
	private:
	
	protected:
		wxPanel* m_panel41;
		wxStaticBitmap* m_bitmapLogo;
		wxStaticLine* m_staticline341;
		wxStaticText* m_staticText96;
		wxHyperlinkCtrl* m_hyperlink11;
		wxHyperlinkCtrl* m_hyperlink9;
		wxHyperlinkCtrl* m_hyperlink10;
		wxHyperlinkCtrl* m_hyperlink7;
		wxHyperlinkCtrl* m_hyperlink14;
		wxHyperlinkCtrl* m_hyperlink15;
		wxHyperlinkCtrl* m_hyperlink13;
		wxHyperlinkCtrl* m_hyperlink16;
		wxHyperlinkCtrl* m_hyperlink12;
		wxHyperlinkCtrl* m_hyperlink18;
		wxPanel* m_panelDonate;
		wxPanel* m_panel39;
		wxAnimationCtrl* m_animCtrlWink;
		wxStaticText* m_staticText83;
		wxButton* m_buttonDonate;
		wxStaticText* m_staticText94;
		wxStaticBitmap* m_bitmap9;
		wxHyperlinkCtrl* m_hyperlink1;
		wxStaticBitmap* m_bitmap10;
		wxHyperlinkCtrl* m_hyperlink2;
		wxStaticLine* m_staticline34;
		wxStaticText* m_staticText93;
		wxStaticBitmap* m_bitmap13;
		wxHyperlinkCtrl* m_hyperlink5;
		wxStaticLine* m_staticline37;
		wxStaticText* m_staticText54;
		wxScrolledWindow* m_scrolledWindowTranslators;
		wxFlexGridSizer* fgSizerTranslators;
		wxStaticLine* m_staticline36;
		wxBoxSizer* bSizerStdButtons;
		wxButton* m_buttonClose;
		
		// Virtual event handlers, overide them in your derived class
		virtual void OnClose( wxCloseEvent& event ) { event.Skip(); }
		virtual void OnDonate( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnOK( wxCommandEvent& event ) { event.Skip(); }
		
	
	public:
		
		AboutDlgGenerated( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = _("About"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize, long style = wxDEFAULT_DIALOG_STYLE ); 
		~AboutDlgGenerated();
	
};

#endif //__GUI_GENERATED_H__
