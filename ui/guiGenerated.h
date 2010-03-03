///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Apr 16 2008)
// http://www.wxformbuilder.org/
//
// PLEASE DO "NOT" EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#ifndef __guiGenerated__
#define __guiGenerated__

#include <wx/intl.h>

class CustomComboBox;
class CustomGridLeft;
class CustomGridMiddle;
class CustomGridRight;
class ToggleButton;
class wxButtonWithImage;

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
#include <wx/statbox.h>
#include <wx/scrolwin.h>
#include <wx/grid.h>
#include <wx/choice.h>
#include <wx/checkbox.h>
#include <wx/notebook.h>
#include <wx/statbmp.h>
#include <wx/textctrl.h>
#include <wx/statline.h>
#include <wx/frame.h>
#include <wx/radiobut.h>
#include <wx/dialog.h>
#include <wx/gauge.h>
#include <wx/animate.h>
#include <wx/treectrl.h>
#include <wx/hyperlink.h>
#include <wx/checklst.h>

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
		wxMenuItem* m_menuItemSwitchView;
		wxMenuItem* m_menuItemNew;
		wxMenuItem* m_menuItemSave;
		wxMenuItem* m_menuItemLoad;
		wxMenu* m_menuAdvanced;
		wxMenu* m_menuLanguages;
		wxMenuItem* m_menuItemGlobSett;
		wxMenuItem* m_menuItem7;
		wxMenu* m_menuHelp;
		wxMenuItem* m_menuItemCheckVer;
		wxMenuItem* m_menuItemAbout;
		wxBoxSizer* bSizer1;
		wxPanel* m_panel71;
		wxBoxSizer* bSizer6;
		
		wxStaticText* m_staticTextCmpVariant;
		
		wxButtonWithImage* m_buttonCompare;
		wxButton* m_buttonAbort;
		wxBitmapButton* m_bpButtonCmpConfig;
		
		
		wxStaticText* m_staticTextSyncVariant;
		wxBitmapButton* m_bpButtonSyncConfig;
		wxButtonWithImage* m_buttonStartSync;
		
		wxStaticBoxSizer* sbSizer2;
		wxPanel* m_panelTopMiddle;
		
		wxBoxSizer* bSizerMiddle;
		wxBitmapButton* m_bpButtonSwapSides;
		
		
		
		wxBitmapButton* m_bpButtonAddPair;
		wxScrolledWindow* m_scrolledWindowFolderPairs;
		wxBoxSizer* bSizerAddFolderPairs;
		CustomGridLeft* m_gridLeft;
		wxPanel* m_panelMiddle;
		CustomGridMiddle* m_gridMiddle;
		CustomGridRight* m_gridRight;
		wxPanel* m_panelBottom;
		wxBoxSizer* bSizer3;
		wxNotebook* m_notebookBottomLeft;
		wxPanel* m_panel30;
		wxBitmapButton* m_bpButtonSave;
		wxBitmapButton* m_bpButtonLoad;
		wxChoice* m_choiceHistory;
		wxPanel* m_panelFilter;
		wxBitmapButton* m_bpButtonFilter;
		wxCheckBox* m_checkBoxActivateFilter;
		wxCheckBox* m_checkBoxHideFilt;
		wxPanel* m_panel112;
		
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
		
		wxBoxSizer* bSizerBottomRight;
		
		wxPanel* m_panelSyncPreview;
		wxStaticBitmap* m_bitmapCreate;
		wxTextCtrl* m_textCtrlCreate;
		wxStaticBitmap* m_bitmapDelete;
		wxTextCtrl* m_textCtrlDelete;
		wxStaticBitmap* m_bitmapUpdate;
		wxTextCtrl* m_textCtrlUpdate;
		wxStaticBitmap* m_bitmapData;
		wxTextCtrl* m_textCtrlData;
		wxBitmapButton* m_bpButton10;
		wxPanel* m_panelStatusBar;
		
		wxStaticText* m_staticTextStatusLeft;
		
		wxStaticLine* m_staticline9;
		
		wxStaticText* m_staticTextStatusMiddle;
		
		wxStaticLine* m_staticline10;
		
		wxStaticText* m_staticTextStatusRight;
		
		wxStaticBitmap* m_bitmap15;
		
		// Virtual event handlers, overide them in your derived class
		virtual void OnClose( wxCloseEvent& event ){ event.Skip(); }
		virtual void OnCompare( wxCommandEvent& event ){ event.Skip(); }
		virtual void OnStartSync( wxCommandEvent& event ){ event.Skip(); }
		virtual void OnSwitchView( wxCommandEvent& event ){ event.Skip(); }
		virtual void OnNewConfig( wxCommandEvent& event ){ event.Skip(); }
		virtual void OnSaveConfig( wxCommandEvent& event ){ event.Skip(); }
		virtual void OnLoadConfig( wxCommandEvent& event ){ event.Skip(); }
		virtual void OnMenuQuit( wxCommandEvent& event ){ event.Skip(); }
		virtual void OnMenuGlobalSettings( wxCommandEvent& event ){ event.Skip(); }
		virtual void OnMenuBatchJob( wxCommandEvent& event ){ event.Skip(); }
		virtual void OnMenuExportFileList( wxCommandEvent& event ){ event.Skip(); }
		virtual void OnShowHelp( wxCommandEvent& event ){ event.Skip(); }
		virtual void OnMenuCheckVersion( wxCommandEvent& event ){ event.Skip(); }
		virtual void OnMenuAbout( wxCommandEvent& event ){ event.Skip(); }
		virtual void OnCmpSettings( wxCommandEvent& event ){ event.Skip(); }
		virtual void OnSyncSettings( wxCommandEvent& event ){ event.Skip(); }
		virtual void OnDirSelected( wxFileDirPickerEvent& event ){ event.Skip(); }
		virtual void OnSwapSides( wxCommandEvent& event ){ event.Skip(); }
		virtual void OnAddFolderPair( wxCommandEvent& event ){ event.Skip(); }
		virtual void OnRemoveTopFolderPair( wxCommandEvent& event ){ event.Skip(); }
		virtual void OnLeftGridDoubleClick( wxGridEvent& event ){ event.Skip(); }
		virtual void OnContextRim( wxGridEvent& event ){ event.Skip(); }
		virtual void OnSortLeftGrid( wxGridEvent& event ){ event.Skip(); }
		virtual void OnContextRimLabelLeft( wxGridEvent& event ){ event.Skip(); }
		virtual void OnContextMiddle( wxGridEvent& event ){ event.Skip(); }
		virtual void OnSortMiddleGrid( wxGridEvent& event ){ event.Skip(); }
		virtual void OnContextMiddleLabel( wxGridEvent& event ){ event.Skip(); }
		virtual void OnRightGridDoubleClick( wxGridEvent& event ){ event.Skip(); }
		virtual void OnSortRightGrid( wxGridEvent& event ){ event.Skip(); }
		virtual void OnContextRimLabelRight( wxGridEvent& event ){ event.Skip(); }
		virtual void OnCfgHistoryKeyEvent( wxKeyEvent& event ){ event.Skip(); }
		virtual void OnLoadFromHistory( wxCommandEvent& event ){ event.Skip(); }
		virtual void OnConfigureFilter( wxCommandEvent& event ){ event.Skip(); }
		virtual void OnFilterButton( wxCommandEvent& event ){ event.Skip(); }
		virtual void OnHideFilteredButton( wxCommandEvent& event ){ event.Skip(); }
		virtual void OnSyncCreateLeft( wxCommandEvent& event ){ event.Skip(); }
		virtual void OnSyncDirLeft( wxCommandEvent& event ){ event.Skip(); }
		virtual void OnSyncDeleteLeft( wxCommandEvent& event ){ event.Skip(); }
		virtual void OnLeftOnlyFiles( wxCommandEvent& event ){ event.Skip(); }
		virtual void OnLeftNewerFiles( wxCommandEvent& event ){ event.Skip(); }
		virtual void OnEqualFiles( wxCommandEvent& event ){ event.Skip(); }
		virtual void OnDifferentFiles( wxCommandEvent& event ){ event.Skip(); }
		virtual void OnSyncDirNone( wxCommandEvent& event ){ event.Skip(); }
		virtual void OnRightNewerFiles( wxCommandEvent& event ){ event.Skip(); }
		virtual void OnRightOnlyFiles( wxCommandEvent& event ){ event.Skip(); }
		virtual void OnSyncDeleteRight( wxCommandEvent& event ){ event.Skip(); }
		virtual void OnSyncDirRight( wxCommandEvent& event ){ event.Skip(); }
		virtual void OnSyncCreateRight( wxCommandEvent& event ){ event.Skip(); }
		virtual void OnConflictFiles( wxCommandEvent& event ){ event.Skip(); }
		virtual void OnQuit( wxCommandEvent& event ){ event.Skip(); }
		
	
	public:
		wxPanel* m_panelTopLeft;
		CustomComboBox* m_directoryLeft;
		wxDirPickerCtrl* m_dirPickerLeft;
		wxBitmapButton* m_bpButtonLocalFilter;
		wxBitmapButton* m_bpButtonAltSyncCfg;
		wxPanel* m_panelTopRight;
		wxBitmapButton* m_bpButtonRemovePair;
		CustomComboBox* m_directoryRight;
		wxDirPickerCtrl* m_dirPickerRight;
		wxPanel* m_panelLeft;
		wxPanel* m_panelRight;
		MainDialogGenerated( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = _("FreeFileSync - Folder Comparison and Synchronization"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( 933,612 ), long style = wxDEFAULT_FRAME_STYLE|wxTAB_TRAVERSAL );
		~MainDialogGenerated();
	
};

///////////////////////////////////////////////////////////////////////////////
/// Class FolderPairGenerated
///////////////////////////////////////////////////////////////////////////////
class FolderPairGenerated : public wxPanel 
{
	private:
	
	protected:
		wxPanel* m_panel20;
		
		
		
	
	public:
		wxPanel* m_panelLeft;
		wxTextCtrl* m_directoryLeft;
		wxDirPickerCtrl* m_dirPickerLeft;
		wxPanel* m_panel21;
		wxBitmapButton* m_bpButtonLocalFilter;
		wxBitmapButton* m_bpButtonAltSyncCfg;
		wxPanel* m_panelRight;
		wxBitmapButton* m_bpButtonRemovePair;
		wxTextCtrl* m_directoryRight;
		wxDirPickerCtrl* m_dirPickerRight;
		FolderPairGenerated( wxWindow* parent, wxWindowID id = wxID_ANY, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( -1,-1 ), long style = wxTAB_TRAVERSAL );
		~FolderPairGenerated();
	
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
		wxTextCtrl* m_directoryLeft;
		wxDirPickerCtrl* m_dirPickerLeft;
		wxBitmapButton* m_bpButtonLocalFilter;
		wxTextCtrl* m_directoryRight;
		wxDirPickerCtrl* m_dirPickerRight;
		wxBitmapButton* m_bpButtonAltSyncCfg;
		BatchFolderPairGenerated( wxWindow* parent, wxWindowID id = wxID_ANY, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( -1,-1 ), long style = wxTAB_TRAVERSAL );
		~BatchFolderPairGenerated();
	
};

///////////////////////////////////////////////////////////////////////////////
/// Class BatchDlgGenerated
///////////////////////////////////////////////////////////////////////////////
class BatchDlgGenerated : public wxDialog 
{
	private:
	
	protected:
		wxBoxSizer* bSizer69;
		wxStaticBitmap* m_bitmap27;
		wxPanel* m_panel8;
		wxStaticText* m_staticText56;
		
		
		wxStaticText* m_staticText44;
		wxBitmapButton* m_bpButtonHelp;
		
		wxStaticLine* m_staticline10;
		wxStaticText* m_staticText531;
		wxNotebook* m_notebookSettings;
		wxPanel* m_panelOverview;
		wxBoxSizer* sbSizerMainPair;
		wxPanel* m_panelMainPair;
		wxStaticText* m_staticText532;
		wxStaticText* m_staticText5411;
		wxBoxSizer* bSizerAddFolderPairs;
		
		
		wxRadioButton* m_radioBtnSizeDate;
		wxRadioButton* m_radioBtnContent;
		
		
		wxCheckBox* m_checkBoxAutomatic;
		wxCheckBox* m_checkBoxFilter;
		wxCheckBox* m_checkBoxSilent;
		
		wxChoice* m_choiceHandleError;
		wxChoice* m_choiceHandleDeletion;
		wxPanel* m_panelCustomDeletionDir;
		wxTextCtrl* m_textCtrlCustomDelFolder;
		wxDirPickerCtrl* m_dirPickerCustomDelFolder;
		
		wxStaticBoxSizer* sbSizerSyncDirections;
		wxStaticText* m_staticText21;
		wxStaticText* m_staticText31;
		wxStaticLine* m_staticline3;
		wxStaticBitmap* m_bitmapLeftOnly;
		
		wxBitmapButton* m_bpButtonLeftOnly;
		wxStaticBitmap* m_bitmapRightOnly;
		
		wxBitmapButton* m_bpButtonRightOnly;
		wxStaticBitmap* m_bitmapLeftNewer;
		
		wxBitmapButton* m_bpButtonLeftNewer;
		wxStaticBitmap* m_bitmapRightNewer;
		
		wxBitmapButton* m_bpButtonRightNewer;
		wxStaticBitmap* m_bitmapDifferent;
		
		wxBitmapButton* m_bpButtonDifferent;
		wxStaticBitmap* m_bitmapConflict;
		
		wxBitmapButton* m_bpButtonConflict;
		wxPanel* m_panelFilter;
		
		wxStaticText* m_staticText15;
		wxStaticBitmap* m_bitmap8;
		wxTextCtrl* m_textCtrlInclude;
		
		wxStaticText* m_staticText16;
		wxStaticBitmap* m_bitmap9;
		wxTextCtrl* m_textCtrlExclude;
		wxPanel* m_panelLogging;
		wxStaticText* m_staticText120;
		wxTextCtrl* m_textCtrlLogfileDir;
		wxDirPickerCtrl* m_dirPickerLogfileDir;
		wxButton* m_buttonSave;
		wxButton* m_buttonLoad;
		wxButton* m_button6;
		
		// Virtual event handlers, overide them in your derived class
		virtual void OnClose( wxCloseEvent& event ){ event.Skip(); }
		virtual void OnHelp( wxCommandEvent& event ){ event.Skip(); }
		virtual void OnAddFolderPair( wxCommandEvent& event ){ event.Skip(); }
		virtual void OnRemoveTopFolderPair( wxCommandEvent& event ){ event.Skip(); }
		virtual void OnChangeCompareVar( wxCommandEvent& event ){ event.Skip(); }
		virtual void OnCheckAutomatic( wxCommandEvent& event ){ event.Skip(); }
		virtual void OnCheckFilter( wxCommandEvent& event ){ event.Skip(); }
		virtual void OnCheckSilent( wxCommandEvent& event ){ event.Skip(); }
		virtual void OnChangeErrorHandling( wxCommandEvent& event ){ event.Skip(); }
		virtual void OnChangeDeletionHandling( wxCommandEvent& event ){ event.Skip(); }
		virtual void OnExLeftSideOnly( wxCommandEvent& event ){ event.Skip(); }
		virtual void OnExRightSideOnly( wxCommandEvent& event ){ event.Skip(); }
		virtual void OnLeftNewer( wxCommandEvent& event ){ event.Skip(); }
		virtual void OnRightNewer( wxCommandEvent& event ){ event.Skip(); }
		virtual void OnDifferent( wxCommandEvent& event ){ event.Skip(); }
		virtual void OnConflict( wxCommandEvent& event ){ event.Skip(); }
		virtual void OnSaveBatchJob( wxCommandEvent& event ){ event.Skip(); }
		virtual void OnLoadBatchJob( wxCommandEvent& event ){ event.Skip(); }
		virtual void OnCancel( wxCommandEvent& event ){ event.Skip(); }
		
	
	public:
		wxScrolledWindow* m_scrolledWindow6;
		wxBitmapButton* m_bpButtonAddPair;
		wxBitmapButton* m_bpButtonRemovePair;
		wxPanel* m_panelLeft;
		wxTextCtrl* m_directoryLeft;
		wxDirPickerCtrl* m_dirPickerLeft;
		wxBitmapButton* m_bpButtonLocalFilter;
		wxPanel* m_panelRight;
		wxTextCtrl* m_directoryRight;
		wxDirPickerCtrl* m_dirPickerRight;
		wxBitmapButton* m_bpButtonAltSyncCfg;
		BatchDlgGenerated( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = _("Create a batch job"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( -1,-1 ), long style = wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER );
		~BatchDlgGenerated();
	
};

///////////////////////////////////////////////////////////////////////////////
/// Class CompareStatusGenerated
///////////////////////////////////////////////////////////////////////////////
class CompareStatusGenerated : public wxPanel 
{
	private:
	
	protected:
		wxBoxSizer* bSizer42;
		wxStaticText* m_staticText321;
		wxStaticText* m_staticTextScanned;
		
		wxStaticBoxSizer* sbSizer13;
		wxStaticText* m_staticText46;
		wxStaticText* m_staticTextFilesRemaining;
		wxStaticText* m_staticText32;
		wxStaticText* m_staticTextDataRemaining;
		wxStaticText* m_staticText104;
		wxStaticText* m_staticTextSpeed;
		wxStaticText* m_staticText103;
		wxStaticText* m_staticTextTimeRemaining;
		
		wxStaticText* m_staticText37;
		wxStaticText* m_staticTextTimeElapsed;
		wxStaticText* m_staticText30;
		wxTextCtrl* m_textCtrlStatus;
		wxGauge* m_gauge2;
	
	public:
		CompareStatusGenerated( wxWindow* parent, wxWindowID id = wxID_ANY, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( -1,-1 ), long style = wxTAB_TRAVERSAL );
		~CompareStatusGenerated();
	
};

///////////////////////////////////////////////////////////////////////////////
/// Class SyncCfgDlgGenerated
///////////////////////////////////////////////////////////////////////////////
class SyncCfgDlgGenerated : public wxDialog 
{
	private:
	
	protected:
		wxStaticText* m_staticText1;
		wxRadioButton* m_radioBtnAutomatic;
		wxButton* m_buttonAutomatic;
		wxStaticText* m_staticText81;
		wxRadioButton* m_radioBtnMirror;
		wxButton* m_buttonOneWay;
		wxStaticText* m_staticText8;
		wxRadioButton* m_radioBtnUpdate;
		wxButton* m_buttonUpdate;
		wxStaticText* m_staticText101;
		wxRadioButton* m_radioBtnCustom;
		
		wxStaticText* m_staticText23;
		
		wxStaticText* m_staticText9;
		
		wxBoxSizer* bSizer201;
		wxStaticBoxSizer* sbSizerErrorHandling;
		wxChoice* m_choiceHandleError;
		wxChoice* m_choiceHandleDeletion;
		wxPanel* m_panelCustomDeletionDir;
		wxTextCtrl* m_textCtrlCustomDelFolder;
		wxDirPickerCtrl* m_dirPickerCustomDelFolder;
		
		wxButton* m_buttonOK;
		wxButton* m_button16;
		
		
		wxStaticBoxSizer* sbSizerSyncDirections;
		wxStaticText* m_staticText21;
		wxStaticText* m_staticText31;
		wxStaticLine* m_staticline3;
		wxStaticBitmap* m_bitmapLeftOnly;
		
		wxBitmapButton* m_bpButtonLeftOnly;
		wxStaticBitmap* m_bitmapRightOnly;
		
		wxBitmapButton* m_bpButtonRightOnly;
		wxStaticBitmap* m_bitmapLeftNewer;
		
		wxBitmapButton* m_bpButtonLeftNewer;
		wxStaticBitmap* m_bitmapRightNewer;
		
		wxBitmapButton* m_bpButtonRightNewer;
		wxStaticBitmap* m_bitmapDifferent;
		
		wxBitmapButton* m_bpButtonDifferent;
		wxStaticBitmap* m_bitmapConflict;
		
		wxBitmapButton* m_bpButtonConflict;
		
		// Virtual event handlers, overide them in your derived class
		virtual void OnClose( wxCloseEvent& event ){ event.Skip(); }
		virtual void OnSyncAutomatic( wxCommandEvent& event ){ event.Skip(); }
		virtual void OnSyncLeftToRight( wxCommandEvent& event ){ event.Skip(); }
		virtual void OnSyncUpdate( wxCommandEvent& event ){ event.Skip(); }
		virtual void OnSyncCustom( wxCommandEvent& event ){ event.Skip(); }
		virtual void OnChangeErrorHandling( wxCommandEvent& event ){ event.Skip(); }
		virtual void OnChangeDeletionHandling( wxCommandEvent& event ){ event.Skip(); }
		virtual void OnApply( wxCommandEvent& event ){ event.Skip(); }
		virtual void OnCancel( wxCommandEvent& event ){ event.Skip(); }
		virtual void OnExLeftSideOnly( wxCommandEvent& event ){ event.Skip(); }
		virtual void OnExRightSideOnly( wxCommandEvent& event ){ event.Skip(); }
		virtual void OnLeftNewer( wxCommandEvent& event ){ event.Skip(); }
		virtual void OnRightNewer( wxCommandEvent& event ){ event.Skip(); }
		virtual void OnDifferent( wxCommandEvent& event ){ event.Skip(); }
		virtual void OnConflict( wxCommandEvent& event ){ event.Skip(); }
		
	
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
		wxStaticLine* m_staticline14;
		wxBitmapButton* m_bpButtonHelp;
		
		wxButton* m_button6;
		
		// Virtual event handlers, overide them in your derived class
		virtual void OnClose( wxCloseEvent& event ){ event.Skip(); }
		virtual void OnTimeSize( wxCommandEvent& event ){ event.Skip(); }
		virtual void OnContent( wxCommandEvent& event ){ event.Skip(); }
		virtual void OnShowHelp( wxCommandEvent& event ){ event.Skip(); }
		virtual void OnCancel( wxCommandEvent& event ){ event.Skip(); }
		
	
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
		
		wxAnimationCtrl* m_animationControl1;
		wxPanel* m_panel8;
		wxStaticText* m_staticText56;
		
		wxStaticBitmap* m_bitmapStatus;
		wxStaticText* m_staticTextStatus;
		
		wxBoxSizer* bSizer31;
		wxStaticText* m_staticText21;
		
		wxStaticText* m_staticText55;
		wxStaticText* m_staticTextTimeElapsed;
		wxTextCtrl* m_textCtrlInfo;
		wxBoxSizer* bSizer28;
		wxBoxSizer* bSizerObjectsRemaining;
		wxStaticText* m_staticText25;
		wxStaticText* m_staticTextRemainingObj;
		wxBoxSizer* bSizerObjectsProcessed;
		wxStaticText* m_staticText251;
		wxStaticText* m_staticTextProcessedObj;
		wxBoxSizer* bSizerSpeed;
		wxStaticText* m_staticText108;
		wxStaticText* m_staticTextSpeed;
		
		wxButton* m_buttonOK;
		wxButton* m_buttonPause;
		wxButton* m_buttonAbort;
		
		wxBoxSizer* bSizerDataRemaining;
		wxStaticText* m_staticText26;
		wxStaticText* m_staticTextDataRemaining;
		wxBoxSizer* bSizerDataProcessed;
		wxStaticText* m_staticText261;
		wxStaticText* m_staticTextDataProcessed;
		wxBoxSizer* bSizerRemTime;
		wxStaticText* m_staticText106;
		wxStaticText* m_staticTextTimeRemaining;
		
		
		// Virtual event handlers, overide them in your derived class
		virtual void OnClose( wxCloseEvent& event ){ event.Skip(); }
		virtual void OnIconize( wxIconizeEvent& event ){ event.Skip(); }
		virtual void OnOkay( wxCommandEvent& event ){ event.Skip(); }
		virtual void OnPause( wxCommandEvent& event ){ event.Skip(); }
		virtual void OnAbort( wxCommandEvent& event ){ event.Skip(); }
		
	
	public:
		wxGauge* m_gauge1;
		SyncStatusDlgGenerated( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = wxEmptyString, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( 638,376 ), long style = wxDEFAULT_FRAME_STYLE|wxTAB_TRAVERSAL );
		~SyncStatusDlgGenerated();
	
};

///////////////////////////////////////////////////////////////////////////////
/// Class HelpDlgGenerated
///////////////////////////////////////////////////////////////////////////////
class HelpDlgGenerated : public wxDialog 
{
	private:
	
	protected:
		
		wxStaticBitmap* m_bitmap25;
		wxPanel* m_panel8;
		
		wxStaticText* m_staticText56;
		
		
		wxNotebook* m_notebook1;
		wxScrolledWindow* m_scrolledWindow1;
		wxStaticText* m_staticText59;
		wxStaticText* m_staticText60;
		wxStaticText* m_staticText61;
		wxTreeCtrl* m_treeCtrl1;
		wxStaticText* m_staticText63;
		wxStaticText* m_staticText75;
		wxStaticText* m_staticText76;
		wxStaticText* m_staticText77;
		wxStaticText* m_staticText79;
		wxStaticText* m_staticText80;
		wxStaticText* m_staticText78;
		wxScrolledWindow* m_scrolledWindow5;
		wxStaticText* m_staticText65;
		wxStaticText* m_staticText66;
		wxTreeCtrl* m_treeCtrl2;
		wxStaticText* m_staticText69;
		wxStaticText* m_staticText81;
		wxStaticText* m_staticText82;
		wxStaticText* m_staticText83;
		wxStaticText* m_staticText84;
		wxButton* m_button8;
		
		// Virtual event handlers, overide them in your derived class
		virtual void OnClose( wxCloseEvent& event ){ event.Skip(); }
		virtual void OnOK( wxCommandEvent& event ){ event.Skip(); }
		
	
	public:
		HelpDlgGenerated( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = wxEmptyString, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( 579,543 ), long style = wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER );
		~HelpDlgGenerated();
	
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
		wxStaticText* m_staticText15;
		wxStaticText* m_build;
		
		wxScrolledWindow* m_scrolledWindowCodeInfo;
		wxBoxSizer* bSizerCodeInfo;
		wxStaticText* m_staticText72;
		wxStaticText* m_staticText73;
		wxStaticText* m_staticText74;
		wxScrolledWindow* m_scrolledWindowTranslators;
		wxBoxSizer* bSizerTranslators;
		wxStaticText* m_staticText54;
		
		wxFlexGridSizer* fgSizerTranslators;
		wxStaticLine* m_staticline3;
		wxStaticText* m_staticText131;
		wxStaticLine* m_staticline12;
		wxStaticBitmap* m_bitmap9;
		wxHyperlinkCtrl* m_hyperlink1;
		
		wxHyperlinkCtrl* m_hyperlink6;
		wxStaticBitmap* m_bitmap10;
		wxHyperlinkCtrl* m_hyperlink2;
		
		wxAnimationCtrl* m_animationControl1;
		wxHyperlinkCtrl* m_hyperlink3;
		wxStaticLine* m_staticline2;
		
		wxStaticBitmap* m_bitmap13;
		wxHyperlinkCtrl* m_hyperlink5;
		
		wxButton* m_buttonOkay;
		
		// Virtual event handlers, overide them in your derived class
		virtual void OnClose( wxCloseEvent& event ){ event.Skip(); }
		virtual void OnOK( wxCommandEvent& event ){ event.Skip(); }
		
	
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
		wxCheckBox* m_checkBoxIgnoreErrors;
		
		wxButton* m_buttonIgnore;
		wxButton* m_buttonRetry;
		wxButton* m_buttonAbort;
		
		
		// Virtual event handlers, overide them in your derived class
		virtual void OnClose( wxCloseEvent& event ){ event.Skip(); }
		virtual void OnIgnore( wxCommandEvent& event ){ event.Skip(); }
		virtual void OnRetry( wxCommandEvent& event ){ event.Skip(); }
		virtual void OnAbort( wxCommandEvent& event ){ event.Skip(); }
		
	
	public:
		ErrorDlgGenerated( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = _("Error"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( 421,228 ), long style = wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER );
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
		wxCheckBox* m_checkBoxDontShowAgain;
		
		wxButton* m_buttonIgnore;
		wxButton* m_buttonAbort;
		
		
		// Virtual event handlers, overide them in your derived class
		virtual void OnClose( wxCloseEvent& event ){ event.Skip(); }
		virtual void OnIgnore( wxCommandEvent& event ){ event.Skip(); }
		virtual void OnAbort( wxCommandEvent& event ){ event.Skip(); }
		
	
	public:
		wxStaticBitmap* m_bitmap10;
		WarningDlgGenerated( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = _("Warning"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( 421,231 ), long style = wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER );
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
		wxCheckBox* m_checkBoxDontAskAgain;
		
		wxButton* m_buttonYes;
		wxButton* m_buttonNo;
		wxButton* m_buttonCancel;
		
		
		// Virtual event handlers, overide them in your derived class
		virtual void OnClose( wxCloseEvent& event ){ event.Skip(); }
		virtual void OnYes( wxCommandEvent& event ){ event.Skip(); }
		virtual void OnNo( wxCommandEvent& event ){ event.Skip(); }
		virtual void OnCancel( wxCommandEvent& event ){ event.Skip(); }
		
	
	public:
		QuestionDlgGenerated( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = _("Question"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( 420,198 ), long style = wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER );
		~QuestionDlgGenerated();
	
};

///////////////////////////////////////////////////////////////////////////////
/// Class DeleteDlgGenerated
///////////////////////////////////////////////////////////////////////////////
class DeleteDlgGenerated : public wxDialog 
{
	private:
	
	protected:
		
		
		wxStaticBitmap* m_bitmap12;
		wxStaticText* m_staticTextHeader;
		
		wxCheckBox* m_checkBoxDeleteBothSides;
		
		wxCheckBox* m_checkBoxUseRecycler;
		wxTextCtrl* m_textCtrlMessage;
		wxButton* m_buttonOK;
		wxButton* m_buttonCancel;
		
		// Virtual event handlers, overide them in your derived class
		virtual void OnClose( wxCloseEvent& event ){ event.Skip(); }
		virtual void OnDelOnBothSides( wxCommandEvent& event ){ event.Skip(); }
		virtual void OnUseRecycler( wxCommandEvent& event ){ event.Skip(); }
		virtual void OnOK( wxCommandEvent& event ){ event.Skip(); }
		virtual void OnCancel( wxCommandEvent& event ){ event.Skip(); }
		
	
	public:
		DeleteDlgGenerated( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = _("Confirm"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( 553,336 ), long style = wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER );
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
		
		wxPanel* m_panel13;
		wxStaticLine* m_staticline10;
		wxStaticText* m_staticText45;
		wxStaticText* m_staticText83;
		wxStaticText* m_staticText84;
		wxStaticText* m_staticText85;
		wxStaticText* m_staticText181;
		wxStaticText* m_staticText1811;
		wxStaticText* m_staticTextFilteringInactive;
		
		wxStaticText* m_staticText15;
		wxStaticBitmap* m_bitmap8;
		wxTextCtrl* m_textCtrlInclude;
		
		wxStaticText* m_staticText16;
		wxStaticBitmap* m_bitmap9;
		wxTextCtrl* m_textCtrlExclude;
		
		wxButton* m_button9;
		
		wxButton* m_button10;
		wxButton* m_button17;
		
		// Virtual event handlers, overide them in your derived class
		virtual void OnClose( wxCloseEvent& event ){ event.Skip(); }
		virtual void OnHelp( wxCommandEvent& event ){ event.Skip(); }
		virtual void OnDefault( wxCommandEvent& event ){ event.Skip(); }
		virtual void OnApply( wxCommandEvent& event ){ event.Skip(); }
		virtual void OnCancel( wxCommandEvent& event ){ event.Skip(); }
		
	
	public:
		FilterDlgGenerated( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = _("Configure filter"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( -1,-1 ), long style = wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER );
		~FilterDlgGenerated();
	
};

///////////////////////////////////////////////////////////////////////////////
/// Class CustomizeColsDlgGenerated
///////////////////////////////////////////////////////////////////////////////
class CustomizeColsDlgGenerated : public wxDialog 
{
	private:
	
	protected:
		wxCheckListBox* m_checkListColumns;
		wxBitmapButton* m_bpButton29;
		wxBitmapButton* m_bpButton30;
		wxButton* m_button9;
		
		wxButton* m_button28;
		wxButton* m_button29;
		
		// Virtual event handlers, overide them in your derived class
		virtual void OnClose( wxCloseEvent& event ){ event.Skip(); }
		virtual void OnMoveUp( wxCommandEvent& event ){ event.Skip(); }
		virtual void OnMoveDown( wxCommandEvent& event ){ event.Skip(); }
		virtual void OnDefault( wxCommandEvent& event ){ event.Skip(); }
		virtual void OnOkay( wxCommandEvent& event ){ event.Skip(); }
		virtual void OnCancel( wxCommandEvent& event ){ event.Skip(); }
		
	
	public:
		CustomizeColsDlgGenerated( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = _("Customize columns"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize, long style = wxDEFAULT_DIALOG_STYLE );
		~CustomizeColsDlgGenerated();
	
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
		
		wxStaticText* m_staticText114;
		
		wxCheckBox* m_checkBoxIgnoreOneHour;
		wxStaticText* m_staticTextCopyLocked;
		
		wxCheckBox* m_checkBoxCopyLocked;
		wxStaticLine* m_staticline10;
		wxStaticText* m_staticText100;
		
		wxButtonWithImage* m_buttonResetDialogs;
		
		
		wxGrid* m_gridCustomCommand;
		wxBitmapButton* m_bpButtonAddRow;
		wxBitmapButton* m_bpButtonRemoveRow;
		
		wxButton* m_button9;
		
		wxButton* m_buttonOkay;
		wxButton* m_button29;
		
		// Virtual event handlers, overide them in your derived class
		virtual void OnClose( wxCloseEvent& event ){ event.Skip(); }
		virtual void OnResetDialogs( wxCommandEvent& event ){ event.Skip(); }
		virtual void OnAddRow( wxCommandEvent& event ){ event.Skip(); }
		virtual void OnRemoveRow( wxCommandEvent& event ){ event.Skip(); }
		virtual void OnDefault( wxCommandEvent& event ){ event.Skip(); }
		virtual void OnOkay( wxCommandEvent& event ){ event.Skip(); }
		virtual void OnCancel( wxCommandEvent& event ){ event.Skip(); }
		
	
	public:
		GlobalSettingsDlgGenerated( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = _("Global settings"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize, long style = wxDEFAULT_DIALOG_STYLE );
		~GlobalSettingsDlgGenerated();
	
};

///////////////////////////////////////////////////////////////////////////////
/// Class SyncPreviewDlgGenerated
///////////////////////////////////////////////////////////////////////////////
class SyncPreviewDlgGenerated : public wxDialog 
{
	private:
	
	protected:
		wxButtonWithImage* m_buttonStartSync;
		wxStaticLine* m_staticline16;
		wxStaticText* m_staticTextVariant;
		wxStaticLine* m_staticline14;
		
		wxStaticText* m_staticText94;
		wxStaticBitmap* m_bitmapCreate;
		wxTextCtrl* m_textCtrlCreateL;
		wxStaticBitmap* m_bitmapUpdate;
		wxTextCtrl* m_textCtrlUpdateL;
		wxStaticBitmap* m_bitmapDelete;
		wxTextCtrl* m_textCtrlDeleteL;
		wxStaticText* m_staticText95;
		wxTextCtrl* m_textCtrlCreateR;
		wxTextCtrl* m_textCtrlUpdateR;
		wxTextCtrl* m_textCtrlDeleteR;
		
		wxStaticBitmap* m_bitmapData;
		
		wxTextCtrl* m_textCtrlData;
		
		wxStaticLine* m_staticline12;
		wxCheckBox* m_checkBoxDontShowAgain;
		
		wxButton* m_button16;
		
		// Virtual event handlers, overide them in your derived class
		virtual void OnClose( wxCloseEvent& event ){ event.Skip(); }
		virtual void OnStartSync( wxCommandEvent& event ){ event.Skip(); }
		virtual void OnCancel( wxCommandEvent& event ){ event.Skip(); }
		
	
	public:
		SyncPreviewDlgGenerated( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = _("Synchronization Preview"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize, long style = wxDEFAULT_DIALOG_STYLE );
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
		virtual void OnClose( wxCloseEvent& event ){ event.Skip(); }
		virtual void OnText( wxCommandEvent& event ){ event.Skip(); }
		virtual void OnFindNext( wxCommandEvent& event ){ event.Skip(); }
		virtual void OnCancel( wxCommandEvent& event ){ event.Skip(); }
		
	
	public:
		SearchDialogGenerated( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = _("Find"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( -1,-1 ), long style = wxDEFAULT_DIALOG_STYLE );
		~SearchDialogGenerated();
	
};

#endif //__guiGenerated__
