///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Apr 16 2008)
// http://www.wxformbuilder.org/
//
// PLEASE DO "NOT" EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#ifndef __guiGenerated__
#define __guiGenerated__

#include <wx/intl.h>

class CustomGridLeft;
class CustomGridMiddle;
class CustomGridRight;
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
#include <wx/button.h>
#include <wx/sizer.h>
#include <wx/radiobut.h>
#include <wx/bmpbuttn.h>
#include <wx/statbox.h>
#include <wx/hyperlink.h>
#include <wx/checkbox.h>
#include <wx/panel.h>
#include <wx/combobox.h>
#include <wx/filepicker.h>
#include <wx/statbmp.h>
#include <wx/scrolwin.h>
#include <wx/grid.h>
#include <wx/choice.h>
#include <wx/stattext.h>
#include <wx/statline.h>
#include <wx/frame.h>
#include <wx/textctrl.h>
#include <wx/dialog.h>
#include <wx/gauge.h>
#include <wx/animate.h>
#include <wx/treectrl.h>
#include <wx/notebook.h>
#include <wx/checklst.h>
#include <wx/spinctrl.h>

///////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////
/// Class MainDialogGenerated
///////////////////////////////////////////////////////////////////////////////
class MainDialogGenerated : public wxFrame 
{
	private:
	
	protected:
		wxMenuBar* m_menubar1;
		wxMenu* m_menu1;
		wxMenuItem* m_menuItem10;
		wxMenuItem* m_menuItem11;
		wxMenu* m_menu3;
		wxMenu* m_menu31;
		wxMenuItem* m_menuItemGerman;
		wxMenuItem* m_menuItemEnglish;
		wxMenuItem* m_menuItemSpanish;
		wxMenuItem* m_menuItemFrench;
		wxMenuItem* m_menuItemHungarian;
		wxMenuItem* m_menuItemItalian;
		wxMenuItem* m_menuItemPolish;
		wxMenuItem* m_menuItemDutch;
		wxMenuItem* m_menuItemPortuguese;
		wxMenuItem* m_menuItemSlovenian;
		wxMenuItem* m_menuItemJapanese;
		wxMenuItem* m_menuItemChineseSimple;
		wxMenuItem* m_menuItemGlobSett;
		wxMenuItem* m_menuItem7;
		wxMenu* m_menu33;
		wxMenuItem* m_menuItemAbout;
		wxBoxSizer* bSizer1;
		wxPanel* m_panel71;
		wxBoxSizer* bSizer6;
		
		wxButtonWithImage* m_buttonCompare;
		wxButton* m_buttonAbort;
		wxRadioButton* m_radioBtnSizeDate;
		wxRadioButton* m_radioBtnContent;
		wxBitmapButton* m_bpButton14;
		
		
		wxBitmapButton* m_bpButtonFilter;
		wxHyperlinkCtrl* m_hyperlinkCfgFilter;
		wxCheckBox* m_checkBoxHideFilt;
		
		wxButtonWithImage* m_buttonSync;
		
		wxPanel* m_panel11;
		wxStaticBoxSizer* sbSizer2;
		wxComboBox* m_comboBoxDirLeft;
		wxDirPickerCtrl* m_dirPickerLeft;
		wxPanel* m_panel13;
		
		wxBoxSizer* bSizerMiddle;
		wxBitmapButton* m_bpButtonSwap;
		
		wxPanel* m_panel12;
		
		wxBitmapButton* m_bpButtonAddPair;
		wxComboBox* m_comboBoxDirRight;
		wxDirPickerCtrl* m_dirPickerRight;
		wxBoxSizer* bSizer106;
		wxStaticBitmap* m_bitmapShift;
		wxScrolledWindow* m_scrolledWindowFolderPairs;
		wxBoxSizer* bSizerFolderPairs;
		wxPanel* m_panel1;
		CustomGridLeft* m_gridLeft;
		wxPanel* m_panel3;
		CustomGridMiddle* m_gridMiddle;
		wxPanel* m_panel2;
		CustomGridRight* m_gridRight;
		wxBoxSizer* bSizer3;
		wxBoxSizer* bSizer58;
		wxBitmapButton* m_bpButtonSave;
		wxBitmapButton* m_bpButtonLoad;
		wxChoice* m_choiceHistory;
		
		wxPanel* m_panel112;
		
		wxBitmapButton* m_bpButtonLeftOnly;
		wxBitmapButton* m_bpButtonLeftNewer;
		wxBitmapButton* m_bpButtonEqual;
		wxBitmapButton* m_bpButtonDifferent;
		wxBitmapButton* m_bpButtonRightNewer;
		wxBitmapButton* m_bpButtonRightOnly;
		
		wxBitmapButton* m_bpButton10;
		wxPanel* m_panel7;
		
		wxStaticText* m_staticTextStatusLeft;
		
		wxStaticLine* m_staticline9;
		
		wxStaticText* m_staticTextStatusMiddle;
		
		wxStaticLine* m_staticline10;
		
		wxStaticText* m_staticTextStatusRight;
		
		wxStaticBitmap* m_bitmap15;
		
		// Virtual event handlers, overide them in your derived class
		virtual void OnClose( wxCloseEvent& event ){ event.Skip(); }
		virtual void OnCompare( wxCommandEvent& event ){ event.Skip(); }
		virtual void OnSync( wxCommandEvent& event ){ event.Skip(); }
		virtual void OnMenuSaveConfig( wxCommandEvent& event ){ event.Skip(); }
		virtual void OnMenuLoadConfig( wxCommandEvent& event ){ event.Skip(); }
		virtual void OnMenuQuit( wxCommandEvent& event ){ event.Skip(); }
		virtual void OnMenuLangGerman( wxCommandEvent& event ){ event.Skip(); }
		virtual void OnMenuLangEnglish( wxCommandEvent& event ){ event.Skip(); }
		virtual void OnMenuLangSpanish( wxCommandEvent& event ){ event.Skip(); }
		virtual void OnMenuLangFrench( wxCommandEvent& event ){ event.Skip(); }
		virtual void OnMenuLangHungarian( wxCommandEvent& event ){ event.Skip(); }
		virtual void OnMenuLangItalian( wxCommandEvent& event ){ event.Skip(); }
		virtual void OnMenuLangPolish( wxCommandEvent& event ){ event.Skip(); }
		virtual void OnMenuLangDutch( wxCommandEvent& event ){ event.Skip(); }
		virtual void OnMenuLangPortuguese( wxCommandEvent& event ){ event.Skip(); }
		virtual void OnMenuLangSlovenian( wxCommandEvent& event ){ event.Skip(); }
		virtual void OnMenuLangJapanese( wxCommandEvent& event ){ event.Skip(); }
		virtual void OnMenuLangChineseSimp( wxCommandEvent& event ){ event.Skip(); }
		virtual void OnMenuGlobalSettings( wxCommandEvent& event ){ event.Skip(); }
		virtual void OnMenuBatchJob( wxCommandEvent& event ){ event.Skip(); }
		virtual void OnMenuExportFileList( wxCommandEvent& event ){ event.Skip(); }
		virtual void OnMenuCheckVersion( wxCommandEvent& event ){ event.Skip(); }
		virtual void OnMenuAbout( wxCommandEvent& event ){ event.Skip(); }
		virtual void OnAbortCompare( wxCommandEvent& event ){ event.Skip(); }
		virtual void OnCompareByTimeSize( wxCommandEvent& event ){ event.Skip(); }
		virtual void OnCompareByContent( wxCommandEvent& event ){ event.Skip(); }
		virtual void OnShowHelpDialog( wxCommandEvent& event ){ event.Skip(); }
		virtual void OnFilterButton( wxCommandEvent& event ){ event.Skip(); }
		virtual void OnConfigureFilter( wxHyperlinkEvent& event ){ event.Skip(); }
		virtual void OnHideFilteredButton( wxCommandEvent& event ){ event.Skip(); }
		virtual void OnFolderHistoryKeyEvent( wxKeyEvent& event ){ event.Skip(); }
		virtual void OnWriteDirManually( wxCommandEvent& event ){ event.Skip(); }
		virtual void OnDirSelected( wxFileDirPickerEvent& event ){ event.Skip(); }
		virtual void OnSwapDirs( wxCommandEvent& event ){ event.Skip(); }
		virtual void OnAddFolderPair( wxCommandEvent& event ){ event.Skip(); }
		virtual void OnLeftGridDoubleClick( wxGridEvent& event ){ event.Skip(); }
		virtual void OnContextMenu( wxGridEvent& event ){ event.Skip(); }
		virtual void OnSortLeftGrid( wxGridEvent& event ){ event.Skip(); }
		virtual void OnContextColumnLeft( wxGridEvent& event ){ event.Skip(); }
		virtual void OnContextMenuMiddle( wxGridEvent& event ){ event.Skip(); }
		virtual void OnSortMiddleGrid( wxGridEvent& event ){ event.Skip(); }
		virtual void OnRightGridDoubleClick( wxGridEvent& event ){ event.Skip(); }
		virtual void OnSortRightGrid( wxGridEvent& event ){ event.Skip(); }
		virtual void OnContextColumnRight( wxGridEvent& event ){ event.Skip(); }
		virtual void OnSaveConfig( wxCommandEvent& event ){ event.Skip(); }
		virtual void OnLoadConfig( wxCommandEvent& event ){ event.Skip(); }
		virtual void OnCfgHistoryKeyEvent( wxKeyEvent& event ){ event.Skip(); }
		virtual void OnLoadFromHistory( wxCommandEvent& event ){ event.Skip(); }
		virtual void OnLeftOnlyFiles( wxCommandEvent& event ){ event.Skip(); }
		virtual void OnLeftNewerFiles( wxCommandEvent& event ){ event.Skip(); }
		virtual void OnEqualFiles( wxCommandEvent& event ){ event.Skip(); }
		virtual void OnDifferentFiles( wxCommandEvent& event ){ event.Skip(); }
		virtual void OnRightNewerFiles( wxCommandEvent& event ){ event.Skip(); }
		virtual void OnRightOnlyFiles( wxCommandEvent& event ){ event.Skip(); }
		virtual void OnQuit( wxCommandEvent& event ){ event.Skip(); }
		
	
	public:
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
		wxPanel* m_panel21;
		
		
		// Virtual event handlers, overide them in your derived class
		virtual void OnRemoveFolderPair( wxCommandEvent& event ){ event.Skip(); }
		
	
	public:
		wxPanel* m_panelLeft;
		wxTextCtrl* m_directoryLeft;
		wxDirPickerCtrl* m_dirPickerLeft;
		wxStaticBitmap* m_bitmap23;
		wxPanel* m_panelRight;
		wxBitmapButton* m_bpButtonRemovePair;
		wxTextCtrl* m_directoryRight;
		wxDirPickerCtrl* m_dirPickerRight;
		FolderPairGenerated( wxWindow* parent, wxWindowID id = wxID_ANY, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( -1,-1 ), long style = wxTAB_TRAVERSAL );
		~FolderPairGenerated();
	
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
		
		
		wxStaticText* m_staticText54;
		
		wxStaticLine* m_staticline10;
		wxStaticText* m_staticText531;
		wxScrolledWindow* m_scrolledWindow6;
		wxBoxSizer* bSizerFolderPairs;
		
		wxRadioButton* m_radioBtnSizeDate;
		wxRadioButton* m_radioBtnContent;
		
		wxChoice* m_choiceHandleError;
		
		wxCheckBox* m_checkBoxUseRecycler;
		wxCheckBox* m_checkBoxSilent;
		
		wxBitmapButton* m_bpButtonFilter;
		wxStaticBitmap* m_bitmap8;
		wxTextCtrl* m_textCtrlInclude;
		
		wxStaticBitmap* m_bitmap9;
		wxTextCtrl* m_textCtrlExclude;
		
		wxStaticText* m_staticText211;
		wxStaticText* m_staticText311;
		wxStaticLine* m_staticline3;
		wxStaticBitmap* m_bitmap13;
		wxBitmapButton* m_bpButton5;
		wxStaticBitmap* m_bitmap14;
		wxBitmapButton* m_bpButton6;
		wxStaticBitmap* m_bitmap15;
		wxBitmapButton* m_bpButton7;
		wxStaticBitmap* m_bitmap16;
		wxBitmapButton* m_bpButton8;
		wxStaticBitmap* m_bitmap17;
		wxBitmapButton* m_bpButton9;
		wxStaticLine* m_staticline9;
		wxButton* m_buttonSave;
		wxButton* m_buttonLoad;
		wxButton* m_button6;
		
		// Virtual event handlers, overide them in your derived class
		virtual void OnClose( wxCloseEvent& event ){ event.Skip(); }
		virtual void OnChangeCompareVar( wxCommandEvent& event ){ event.Skip(); }
		virtual void OnChangeErrorHandling( wxCommandEvent& event ){ event.Skip(); }
		virtual void OnSelectRecycleBin( wxCommandEvent& event ){ event.Skip(); }
		virtual void OnFilterButton( wxCommandEvent& event ){ event.Skip(); }
		virtual void OnExLeftSideOnly( wxCommandEvent& event ){ event.Skip(); }
		virtual void OnExRightSideOnly( wxCommandEvent& event ){ event.Skip(); }
		virtual void OnLeftNewer( wxCommandEvent& event ){ event.Skip(); }
		virtual void OnRightNewer( wxCommandEvent& event ){ event.Skip(); }
		virtual void OnDifferent( wxCommandEvent& event ){ event.Skip(); }
		virtual void OnSaveBatchJob( wxCommandEvent& event ){ event.Skip(); }
		virtual void OnLoadBatchJob( wxCommandEvent& event ){ event.Skip(); }
		virtual void OnCancel( wxCommandEvent& event ){ event.Skip(); }
		
	
	public:
		BatchDlgGenerated( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = _("Create a batch job"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( -1,-1 ), long style = wxDEFAULT_DIALOG_STYLE );
		~BatchDlgGenerated();
	
};

///////////////////////////////////////////////////////////////////////////////
/// Class BatchFolderPairGenerated
///////////////////////////////////////////////////////////////////////////////
class BatchFolderPairGenerated : public wxPanel 
{
	private:
	
	protected:
		wxStaticText* m_staticText53;
		wxStaticText* m_staticText541;
		
		// Virtual event handlers, overide them in your derived class
		virtual void OnEnterLeftDir( wxCommandEvent& event ){ event.Skip(); }
		virtual void OnEnterRightDir( wxCommandEvent& event ){ event.Skip(); }
		
	
	public:
		wxTextCtrl* m_directoryLeft;
		wxTextCtrl* m_directoryRight;
		BatchFolderPairGenerated( wxWindow* parent, wxWindowID id = wxID_ANY, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( -1,-1 ), long style = wxTAB_TRAVERSAL );
		~BatchFolderPairGenerated();
	
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
		wxStaticText* m_staticTextFilesToCompare;
		wxStaticText* m_staticText32;
		wxStaticText* m_staticTextDataToCompare;
		
		wxStaticText* m_staticText37;
		wxStaticText* m_staticTextTimeElapsed;
		wxStaticText* m_staticText30;
		wxTextCtrl* m_textCtrlFilename;
		wxGauge* m_gauge2;
	
	public:
		CompareStatusGenerated( wxWindow* parent, wxWindowID id = wxID_ANY, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( -1,-1 ), long style = wxTAB_TRAVERSAL );
		~CompareStatusGenerated();
	
};

///////////////////////////////////////////////////////////////////////////////
/// Class SyncDlgGenerated
///////////////////////////////////////////////////////////////////////////////
class SyncDlgGenerated : public wxDialog 
{
	private:
	
	protected:
		wxBoxSizer* bSizer201;
		wxButtonWithImage* m_button18;
		
		wxCheckBox* m_checkBoxUseRecycler;
		wxCheckBox* m_checkBoxIgnoreErrors;
		
		wxStaticText* m_staticText1;
		wxRadioButton* m_radioBtn1;
		wxButton* m_buttonOneWay;
		wxStaticText* m_staticText8;
		wxRadioButton* m_radioBtnUpdate;
		wxButton* m_buttonUpdate;
		wxStaticText* m_staticText101;
		wxRadioButton* m_radioBtn2;
		wxButton* m_buttonTwoWay;
		wxStaticText* m_staticText10;
		wxRadioButton* m_radioBtn3;
		
		wxStaticText* m_staticText23;
		
		wxStaticText* m_staticText9;
		
		wxButton* m_button6;
		wxButton* m_button16;
		
		wxStaticText* m_staticText37;
		wxTextCtrl* m_textCtrlCreate;
		wxStaticText* m_staticText14;
		wxTextCtrl* m_textCtrlDelete;
		wxStaticText* m_staticText42;
		wxTextCtrl* m_textCtrlUpdate;
		wxStaticText* m_staticText43;
		wxTextCtrl* m_textCtrlData;
		
		wxStaticText* m_staticText21;
		wxStaticText* m_staticText31;
		wxStaticLine* m_staticline3;
		wxStaticBitmap* m_bitmap13;
		wxBitmapButton* m_bpButton5;
		wxStaticBitmap* m_bitmap14;
		wxBitmapButton* m_bpButton6;
		wxStaticBitmap* m_bitmap15;
		wxBitmapButton* m_bpButton7;
		wxStaticBitmap* m_bitmap16;
		wxBitmapButton* m_bpButton8;
		wxStaticBitmap* m_bitmap17;
		wxBitmapButton* m_bpButton9;
		
		// Virtual event handlers, overide them in your derived class
		virtual void OnClose( wxCloseEvent& event ){ event.Skip(); }
		virtual void OnStartSync( wxCommandEvent& event ){ event.Skip(); }
		virtual void OnSelectRecycleBin( wxCommandEvent& event ){ event.Skip(); }
		virtual void OnSyncLeftToRight( wxCommandEvent& event ){ event.Skip(); }
		virtual void OnSyncUpdate( wxCommandEvent& event ){ event.Skip(); }
		virtual void OnSyncBothSides( wxCommandEvent& event ){ event.Skip(); }
		virtual void OnSyncCostum( wxCommandEvent& event ){ event.Skip(); }
		virtual void OnBack( wxCommandEvent& event ){ event.Skip(); }
		virtual void OnCancel( wxCommandEvent& event ){ event.Skip(); }
		virtual void OnExLeftSideOnly( wxCommandEvent& event ){ event.Skip(); }
		virtual void OnExRightSideOnly( wxCommandEvent& event ){ event.Skip(); }
		virtual void OnLeftNewer( wxCommandEvent& event ){ event.Skip(); }
		virtual void OnRightNewer( wxCommandEvent& event ){ event.Skip(); }
		virtual void OnDifferent( wxCommandEvent& event ){ event.Skip(); }
		
	
	public:
		SyncDlgGenerated( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = _("Synchronization settings"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( -1,-1 ), long style = wxDEFAULT_DIALOG_STYLE );
		~SyncDlgGenerated();
	
};

///////////////////////////////////////////////////////////////////////////////
/// Class SyncStatusDlgGenerated
///////////////////////////////////////////////////////////////////////////////
class SyncStatusDlgGenerated : public wxDialog 
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
		wxStaticText* m_staticText25;
		wxStaticText* m_staticTextRemainingObj;
		
		wxButton* m_buttonOK;
		wxButton* m_buttonPause;
		wxButton* m_buttonAbort;
		
		wxStaticText* m_staticText26;
		wxStaticText* m_staticTextDataRemaining;
		
		
		// Virtual event handlers, overide them in your derived class
		virtual void OnClose( wxCloseEvent& event ){ event.Skip(); }
		virtual void OnOkay( wxCommandEvent& event ){ event.Skip(); }
		virtual void OnPause( wxCommandEvent& event ){ event.Skip(); }
		virtual void OnAbort( wxCommandEvent& event ){ event.Skip(); }
		
	
	public:
		wxGauge* m_gauge1;
		SyncStatusDlgGenerated( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = wxEmptyString, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( 614,371 ), long style = wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER );
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
		wxStaticText* m_staticText78;
		wxStaticText* m_staticText79;
		wxStaticText* m_staticText80;
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
		HelpDlgGenerated( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = wxEmptyString, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( 565,501 ), long style = wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER );
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
		
		wxScrolledWindow* m_scrolledWindow4;
		wxStaticText* m_staticText72;
		wxStaticText* m_staticText73;
		wxStaticText* m_staticText74;
		wxScrolledWindow* m_scrolledWindow3;
		wxStaticText* m_staticText54;
		
		wxStaticText* m_staticText68;
		wxStaticText* m_staticText69;
		wxStaticText* m_staticText70;
		wxStaticText* m_staticText71;
		wxStaticText* m_staticText711;
		wxStaticText* m_staticText712;
		wxStaticText* m_staticText91;
		wxStaticText* m_staticText92;
		wxStaticText* m_staticText911;
		wxStaticText* m_staticText921;
		wxStaticText* m_staticText9211;
		wxStaticText* m_staticText9212;
		wxStaticText* m_staticText92121;
		wxStaticText* m_staticText92122;
		wxStaticText* m_staticText921221;
		wxStaticText* m_staticText921222;
		wxStaticText* m_staticText682;
		wxStaticText* m_staticText681;
		wxStaticText* m_staticText683;
		wxStaticText* m_staticText691;
		wxStaticLine* m_staticline3;
		wxPanel* m_panel22;
		wxStaticText* m_staticText131;
		wxStaticLine* m_staticline12;
		wxStaticBitmap* m_bitmap9;
		wxHyperlinkCtrl* m_hyperlink1;
		
		wxStaticBitmap* m_bitmap10;
		wxHyperlinkCtrl* m_hyperlink2;
		wxAnimationCtrl* m_animationControl1;
		wxHyperlinkCtrl* m_hyperlink3;
		wxStaticLine* m_staticline2;
		
		wxStaticBitmap* m_bitmap13;
		wxHyperlinkCtrl* m_hyperlink5;
		
		wxButton* m_button8;
		
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
		ErrorDlgGenerated( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = _("Error"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( 404,268 ), long style = wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER );
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
		WarningDlgGenerated( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = _("Warning"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( 382,249 ), long style = wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER );
		~WarningDlgGenerated();
	
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
		wxStaticText* m_staticText56;
		
		
		wxStaticText* m_staticText44;
		wxBitmapButton* m_bpButtonHelp;
		
		wxPanel* m_panel13;
		wxStaticLine* m_staticline10;
		wxStaticText* m_staticText45;
		wxStaticText* m_staticText83;
		wxStaticText* m_staticText84;
		wxStaticText* m_staticText85;
		wxStaticText* m_staticText86;
		wxStaticText* m_staticText181;
		wxStaticText* m_staticText1811;
		
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
		virtual void OnOK( wxCommandEvent& event ){ event.Skip(); }
		virtual void OnCancel( wxCommandEvent& event ){ event.Skip(); }
		
	
	public:
		FilterDlgGenerated( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = _("Configure filter"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( -1,-1 ), long style = wxDEFAULT_DIALOG_STYLE );
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
		wxButton* m_button28;
		wxButton* m_button9;
		wxButton* m_button29;
		
		
		// Virtual event handlers, overide them in your derived class
		virtual void OnClose( wxCloseEvent& event ){ event.Skip(); }
		virtual void OnMoveUp( wxCommandEvent& event ){ event.Skip(); }
		virtual void OnMoveDown( wxCommandEvent& event ){ event.Skip(); }
		virtual void OnOkay( wxCommandEvent& event ){ event.Skip(); }
		virtual void OnDefault( wxCommandEvent& event ){ event.Skip(); }
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
		
		
		wxStaticText* m_staticText99;
		
		wxSpinCtrl* m_spinCtrlFileTimeTolerance;
		wxStaticLine* m_staticline10;
		wxStaticText* m_staticText97;
		wxTextCtrl* m_textCtrlFileManager;
		wxStaticLine* m_staticline101;
		wxStaticText* m_staticText100;
		
		wxButtonWithImage* m_buttonResetWarnings;
		
		wxButton* m_button28;
		wxButton* m_button9;
		wxButton* m_button29;
		
		// Virtual event handlers, overide them in your derived class
		virtual void OnClose( wxCloseEvent& event ){ event.Skip(); }
		virtual void OnResetWarnings( wxCommandEvent& event ){ event.Skip(); }
		virtual void OnOkay( wxCommandEvent& event ){ event.Skip(); }
		virtual void OnDefault( wxCommandEvent& event ){ event.Skip(); }
		virtual void OnCancel( wxCommandEvent& event ){ event.Skip(); }
		
	
	public:
		GlobalSettingsDlgGenerated( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = _("Global settings"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize, long style = wxDEFAULT_DIALOG_STYLE );
		~GlobalSettingsDlgGenerated();
	
};

#endif //__guiGenerated__
