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
#include <wx/animate.h>
#include <wx/notebook.h>
#include <wx/tglbtn.h>
#include <wx/choice.h>
#include <wx/dialog.h>
#include <wx/spinctrl.h>
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
		wxMenu* m_menuTools;
		wxMenu* m_menuLanguages;
		wxMenuItem* m_menuItemGlobSett;
		wxMenu* m_menuHelp;
		wxMenuItem* m_menuItemManual;
		wxMenu* m_menuCheckVersion;
		wxMenuItem* m_menuItemCheckVersionNow;
		wxMenuItem* m_menuItemCheckVersionAuto;
		wxMenuItem* m_menuItemAbout;
		wxBoxSizer* bSizerPanelHolder;
		wxPanel* m_panelTopButtons;
		wxBoxSizer* bSizerTopButtons;
		wxStaticText* m_staticTextCmpVariant;
		zen::BitmapButton* m_buttonCompare;
		zen::BitmapButton* m_buttonCancel;
		wxBitmapButton* m_bpButtonCmpConfig;
		wxStaticText* m_staticTextSyncVariant;
		wxBitmapButton* m_bpButtonSyncConfig;
		zen::BitmapButton* m_buttonSync;
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
		wxPanel* m_panelConfig;
		wxBoxSizer* bSizerConfig;
		wxBitmapButton* m_bpButtonOpen;
		wxBitmapButton* m_bpButtonSave;
		wxBitmapButton* m_bpButtonBatchJob;
		wxListBox* m_listBoxHistory;
		wxPanel* m_panelFilter;
		wxBitmapButton* m_bpButtonFilter;
		wxCheckBox* m_checkBoxHideExcluded;
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
		ToggleButton* m_bpButtonViewTypeSyncAction;
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
		virtual void OnMenuExportFileList( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnMenuGlobalSettings( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnShowHelp( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnMenuCheckVersion( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnMenuCheckVersionAutomatically( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnMenuAbout( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnCmpSettings( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnCompSettingsContext( wxMouseEvent& event ) { event.Skip(); }
		virtual void OnSyncSettings( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnSyncSettingsContext( wxMouseEvent& event ) { event.Skip(); }
		virtual void OnAddFolderPair( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnRemoveTopFolderPair( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnSwapSides( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnCfgHistoryKeyEvent( wxKeyEvent& event ) { event.Skip(); }
		virtual void OnLoadFromHistory( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnLoadFromHistoryDoubleClick( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnCfgHistoryRightClick( wxMouseEvent& event ) { event.Skip(); }
		virtual void OnConfigureFilter( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnGlobalFilterContext( wxMouseEvent& event ) { event.Skip(); }
		virtual void OnShowExcluded( wxCommandEvent& event ) { event.Skip(); }
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
		
		MainDialogGenerated( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = wxEmptyString, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( 900,600 ), long style = wxDEFAULT_FRAME_STYLE|wxTAB_TRAVERSAL );
		
		~MainDialogGenerated();
	
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
/// Class SyncProgressDlgGenerated
///////////////////////////////////////////////////////////////////////////////
class SyncProgressDlgGenerated : public wxFrame 
{
	private:
	
	protected:
		wxBoxSizer* bSizerRoot;
		wxBoxSizer* bSizer42;
		wxStaticBitmap* m_bitmapStatus;
		wxStaticText* m_staticTextPhase;
		wxAnimationCtrl* m_animCtrlSyncing;
		wxStaticLine* m_staticlineHeader;
		wxPanel* m_panelProgress;
		wxStaticText* m_staticTextStatus;
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
		wxNotebook* m_notebookResult;
		wxStaticLine* m_staticlineFooter;
		wxBoxSizer* bSizerStdButtons;
		wxBoxSizer* bSizerExecFinished;
		wxStaticText* m_staticText87;
		ExecFinishedBox* m_comboBoxExecFinished;
		wxButton* m_buttonClose;
		wxButton* m_buttonPause;
		wxButton* m_buttonCancel;
		
		// Virtual event handlers, overide them in your derived class
		virtual void OnClose( wxCloseEvent& event ) { event.Skip(); }
		virtual void OnIconize( wxIconizeEvent& event ) { event.Skip(); }
		virtual void OnOkay( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnPause( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnCancel( wxCommandEvent& event ) { event.Skip(); }
		
	
	public:
		wxGauge* m_gauge1;
		
		SyncProgressDlgGenerated( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = wxEmptyString, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize, long style = wxDEFAULT_FRAME_STYLE|wxTAB_TRAVERSAL );
		
		~SyncProgressDlgGenerated();
	
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
		wxBitmapButton* m_bpButtonHelp;
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
		wxStaticLine* m_staticline32;
		wxStaticText* m_staticText87;
		wxBoxSizer* bSizerVersioningNamingConvention;
		wxStaticText* m_staticTextNamingCvtPart1;
		wxStaticText* m_staticTextNamingCvtPart2Bold;
		wxStaticText* m_staticTextNamingCvtPart3;
		wxToggleButton* m_toggleBtnPermanent;
		wxToggleButton* m_toggleBtnRecycler;
		wxToggleButton* m_toggleBtnVersioning;
		wxBoxSizer* bSizerVersioningStyle;
		wxStaticText* m_staticText93;
		wxChoice* m_choiceVersioningStyle;
		wxPanel* m_panelVersioning;
		FolderHistoryBox* m_versioningFolder;
		wxButton* m_buttonSelectDirVersioning;
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
		virtual void OnErrorIgnore( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnErrorPopup( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnDeletionPermanent( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnDeletionRecycler( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnDeletionVersioning( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnParameterChange( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnExLeftSideOnly( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnExRightSideOnly( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnLeftNewer( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnRightNewer( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnDifferent( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnConflict( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnApply( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnCancel( wxCommandEvent& event ) { event.Skip(); }
		
	
	public:
		wxBoxSizer* bSizerNamingConvention;
		
		SyncCfgDlgGenerated( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = _("Synchronization settings"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize, long style = wxDEFAULT_DIALOG_STYLE ); 
		~SyncCfgDlgGenerated();
	
};

///////////////////////////////////////////////////////////////////////////////
/// Class BatchDlgGenerated
///////////////////////////////////////////////////////////////////////////////
class BatchDlgGenerated : public wxDialog 
{
	private:
	
	protected:
		wxStaticBitmap* m_bitmapBatchJob;
		wxStaticText* m_staticTextHeader;
		wxStaticText* m_staticText44;
		wxBitmapButton* m_bpButtonHelp;
		wxStaticLine* m_staticline18;
		wxPanel* m_panel35;
		wxStaticText* m_staticText82;
		wxToggleButton* m_toggleBtnErrorIgnore;
		wxToggleButton* m_toggleBtnErrorPopup;
		wxToggleButton* m_toggleBtnErrorExit;
		wxStaticLine* m_staticline26;
		wxStaticText* m_staticText81;
		ExecFinishedBox* m_comboBoxExecFinished;
		wxStaticLine* m_staticline25;
		wxCheckBox* m_checkBoxShowProgress;
		wxCheckBox* m_checkBoxGenerateLogfile;
		wxPanel* m_panelLogfile;
		wxButton* m_buttonSelectLogfileDir;
		wxCheckBox* m_checkBoxLogfilesLimit;
		wxSpinCtrl* m_spinCtrlLogfileLimit;
		wxStaticLine* m_staticline13;
		wxBoxSizer* bSizerStdButtons;
		wxButton* m_buttonSaveAs;
		wxButton* m_buttonCancel;
		
		// Virtual event handlers, overide them in your derived class
		virtual void OnClose( wxCloseEvent& event ) { event.Skip(); }
		virtual void OnHelp( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnErrorIgnore( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnErrorPopup( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnErrorExit( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnToggleGenerateLogfile( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnToggleLogfilesLimit( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnSaveBatchJob( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnCancel( wxCommandEvent& event ) { event.Skip(); }
		
	
	public:
		FolderHistoryBox* m_logfileDir;
		
		BatchDlgGenerated( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = _("Save as batch job"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize, long style = wxDEFAULT_DIALOG_STYLE|wxMAXIMIZE_BOX|wxMINIMIZE_BOX|wxRESIZE_BORDER ); 
		~BatchDlgGenerated();
	
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
		wxStaticText* m_build;
		wxStaticLine* m_staticline3411;
		wxStaticText* m_staticText72;
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
		wxPanel* m_panel40;
		wxPanel* m_panel39;
		wxStaticText* m_staticText83;
		wxButton* m_buttonDonate;
		wxAnimationCtrl* m_animCtrlWink;
		wxScrolledWindow* m_scrolledWindowTranslators;
		wxBoxSizer* bSizerTranslators;
		wxStaticText* m_staticText54;
		wxFlexGridSizer* fgSizerTranslators;
		wxStaticLine* m_staticline43;
		wxStaticText* m_staticText94;
		wxHyperlinkCtrl* m_hyperlink1;
		wxStaticBitmap* m_bitmap9;
		wxHyperlinkCtrl* m_hyperlink2;
		wxStaticBitmap* m_bitmap10;
		wxStaticLine* m_staticline34;
		wxStaticText* m_staticText93;
		wxStaticBitmap* m_bitmap13;
		wxHyperlinkCtrl* m_hyperlink5;
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

///////////////////////////////////////////////////////////////////////////////
/// Class MessageDlgGenerated
///////////////////////////////////////////////////////////////////////////////
class MessageDlgGenerated : public wxDialog 
{
	private:
	
	protected:
		wxPanel* m_panel33;
		wxStaticBitmap* m_bitmapMsgType;
		wxTextCtrl* m_textCtrlMessage;
		wxStaticLine* m_staticline6;
		wxCheckBox* m_checkBoxCustom;
		wxBoxSizer* bSizerStdButtons;
		wxButton* m_buttonAffirmative;
		wxButton* m_buttonNegative;
		wxButton* m_buttonCancel;
		
		// Virtual event handlers, overide them in your derived class
		virtual void OnClose( wxCloseEvent& event ) { event.Skip(); }
		virtual void OnCheckBoxClick( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnButtonAffirmative( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnButtonNegative( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnCancel( wxCommandEvent& event ) { event.Skip(); }
		
	
	public:
		
		MessageDlgGenerated( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = _("dummy"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize, long style = wxDEFAULT_DIALOG_STYLE|wxMAXIMIZE_BOX|wxMINIMIZE_BOX|wxRESIZE_BORDER ); 
		~MessageDlgGenerated();
	
};

///////////////////////////////////////////////////////////////////////////////
/// Class DeleteDlgGenerated
///////////////////////////////////////////////////////////////////////////////
class DeleteDlgGenerated : public wxDialog 
{
	private:
	
	protected:
		wxPanel* m_panel31;
		wxStaticBitmap* m_bitmapDeleteType;
		wxStaticText* m_staticTextHeader;
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
		
		DeleteDlgGenerated( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = _("Delete"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize, long style = wxDEFAULT_DIALOG_STYLE|wxMAXIMIZE_BOX|wxMINIMIZE_BOX|wxRESIZE_BORDER ); 
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
		wxStaticText* m_staticTextHeader;
		wxStaticText* m_staticText44;
		wxBitmapButton* m_bpButtonHelp;
		wxStaticLine* m_staticline17;
		wxPanel* m_panel38;
		wxStaticText* m_staticText78;
		wxStaticBitmap* m_bitmapInclude;
		wxTextCtrl* m_textCtrlInclude;
		wxStaticLine* m_staticline22;
		wxStaticText* m_staticText77;
		wxStaticBitmap* m_bitmapExclude;
		wxTextCtrl* m_textCtrlExclude;
		wxStaticLine* m_staticline24;
		wxStaticText* m_staticText79;
		wxStaticBitmap* m_bitmapFilterDate;
		wxSpinCtrl* m_spinCtrlTimespan;
		wxChoice* m_choiceUnitTimespan;
		wxStaticLine* m_staticline23;
		wxStaticText* m_staticText80;
		wxStaticBitmap* m_bitmapFilterSize;
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
		virtual void OnHelp( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnUpdateNameFilter( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnUpdateChoice( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnClear( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnApply( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnCancel( wxCommandEvent& event ) { event.Skip(); }
		
	
	public:
		
		FilterDlgGenerated( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = _("Configure filter"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize, long style = wxDEFAULT_DIALOG_STYLE|wxMAXIMIZE_BOX|wxMINIMIZE_BOX|wxRESIZE_BORDER ); 
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
		wxStaticText* m_staticTextHeader;
		wxStaticLine* m_staticline19;
		wxPanel* m_panel39;
		wxCheckBox* m_checkBoxTransCopy;
		wxStaticText* m_staticText82;
		wxCheckBox* m_checkBoxCopyLocked;
		wxStaticText* m_staticTextCopyLocked;
		wxCheckBox* m_checkBoxCopyPermissions;
		wxStaticText* m_staticText8211;
		wxStaticLine* m_staticline191;
		zen::BitmapButton* m_buttonResetDialogs;
		wxStaticLine* m_staticline192;
		wxStaticText* m_staticText85;
		wxBitmapButton* m_bpButtonAddRow;
		wxBitmapButton* m_bpButtonRemoveRow;
		wxGrid* m_gridCustomCommand;
		wxStaticLine* m_staticline20;
		wxBoxSizer* bSizerStdButtons;
		wxButton* m_buttonDefault;
		wxButton* m_buttonOkay;
		wxButton* m_buttonCancel;
		
		// Virtual event handlers, overide them in your derived class
		virtual void OnClose( wxCloseEvent& event ) { event.Skip(); }
		virtual void OnResetDialogs( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnAddRow( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnRemoveRow( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnDefault( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnOkay( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnCancel( wxCommandEvent& event ) { event.Skip(); }
		
	
	public:
		
		GlobalSettingsDlgGenerated( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = _("Global settings"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize, long style = wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER ); 
		~GlobalSettingsDlgGenerated();
	
};

///////////////////////////////////////////////////////////////////////////////
/// Class SyncConfirmationDlgGenerated
///////////////////////////////////////////////////////////////////////////////
class SyncConfirmationDlgGenerated : public wxDialog 
{
	private:
	
	protected:
		wxPanel* m_panelStatistics;
		wxStaticBitmap* m_bitmapSync;
		wxStaticLine* m_staticline39;
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
		
		SyncConfirmationDlgGenerated( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = _("Start synchronization"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize, long style = wxDEFAULT_DIALOG_STYLE ); 
		~SyncConfirmationDlgGenerated();
	
};

///////////////////////////////////////////////////////////////////////////////
/// Class PopupDialogGenerated
///////////////////////////////////////////////////////////////////////////////
class PopupDialogGenerated : public wxDialog 
{
	private:
	
	protected:
	
	public:
		wxStaticBitmap* m_bitmapLeft;
		wxStaticText* m_staticTextMain;
		
		PopupDialogGenerated( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = wxEmptyString, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize, long style = wxDEFAULT_DIALOG_STYLE ); 
		~PopupDialogGenerated();
	
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
		
		SearchDialogGenerated( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = _("Find"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize, long style = wxDEFAULT_DIALOG_STYLE ); 
		~SearchDialogGenerated();
	
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
		
		SelectTimespanDlgGenerated( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = _("Select time span"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize, long style = wxDEFAULT_DIALOG_STYLE ); 
		~SelectTimespanDlgGenerated();
	
};

#endif //__GUI_GENERATED_H__
