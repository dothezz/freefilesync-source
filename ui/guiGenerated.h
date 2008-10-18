///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Oct 18 2008)
// http://www.wxformbuilder.org/
//
// PLEASE DO "NOT" EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#ifndef __guiGenerated__
#define __guiGenerated__

#include <wx/intl.h>

class CustomGrid;

#include <wx/string.h>
#include <wx/bitmap.h>
#include <wx/image.h>
#include <wx/icon.h>
#include <wx/menu.h>
#include <wx/gdicmn.h>
#include <wx/font.h>
#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/bmpbuttn.h>
#include <wx/button.h>
#include <wx/sizer.h>
#include <wx/radiobut.h>
#include <wx/statbox.h>
#include <wx/hyperlink.h>
#include <wx/checkbox.h>
#include <wx/panel.h>
#include <wx/textctrl.h>
#include <wx/filepicker.h>
#include <wx/grid.h>
#include <wx/choice.h>
#include <wx/stattext.h>
#include <wx/statline.h>
#include <wx/statbmp.h>
#include <wx/frame.h>
#include <wx/gauge.h>
#include <wx/dialog.h>
#include <wx/animate.h>

///////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////
/// Class GuiGenerated
///////////////////////////////////////////////////////////////////////////////
class GuiGenerated : public wxFrame 
{
	private:
	
	protected:
		wxMenuBar* m_menubar1;
		wxMenu* m_menu1;
		wxMenu* m_menu3;
		wxMenu* m_menu31;
		wxMenuItem* m_menuItemEnglish;
		wxMenuItem* m_menuItemGerman;
		wxMenuItem* m_menuItemFrench;
		wxMenu* m_menu2;
		wxBoxSizer* bSizer1;
		wxPanel* m_panel71;
		wxBoxSizer* bSizer6;
		
		wxBitmapButton* m_bpButtonCompare;
		wxButton* m_buttonAbort;
		wxRadioButton* m_radioBtnSizeDate;
		wxRadioButton* m_radioBtnContent;
		wxBitmapButton* m_bpButton14;
		
		
		wxBitmapButton* m_bpButtonFilter;
		wxHyperlinkCtrl* m_hyperlinkCfgFilter;
		wxCheckBox* m_checkBoxHideFilt;
		
		wxBitmapButton* m_bpButtonSync;
		
		wxPanel* m_panel1;
		wxTextCtrl* m_directoryPanel1;
		wxDirPickerCtrl* m_dirPicker1;
		CustomGrid* m_grid1;
		wxPanel* m_panel3;
		wxBitmapButton* m_bpButtonSwap;
		CustomGrid* m_grid3;
		wxPanel* m_panel2;
		wxTextCtrl* m_directoryPanel2;
		wxDirPickerCtrl* m_dirPicker2;
		CustomGrid* m_grid2;
		wxPanel* m_panel4;
		wxBoxSizer* bSizer3;
		wxBitmapButton* m_bpButton201;
		wxChoice* m_choiceLoad;
		
		wxPanel* m_panel12;
		
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
		virtual void OnMenuQuit( wxCommandEvent& event ){ event.Skip(); }
		virtual void OnMenuExportFileList( wxCommandEvent& event ){ event.Skip(); }
		virtual void OnMenuLangEnglish( wxCommandEvent& event ){ event.Skip(); }
		virtual void OnMenuLangGerman( wxCommandEvent& event ){ event.Skip(); }
		virtual void OnMenuLangFrench( wxCommandEvent& event ){ event.Skip(); }
		virtual void OnMenuBatchJob( wxCommandEvent& event ){ event.Skip(); }
		virtual void OnMenuAbout( wxCommandEvent& event ){ event.Skip(); }
		virtual void OnCompare( wxCommandEvent& event ){ event.Skip(); }
		virtual void OnAbortCompare( wxCommandEvent& event ){ event.Skip(); }
		virtual void OnCompareByTimeSize( wxCommandEvent& event ){ event.Skip(); }
		virtual void OnCompareByContent( wxCommandEvent& event ){ event.Skip(); }
		virtual void OnShowHelpDialog( wxCommandEvent& event ){ event.Skip(); }
		virtual void OnFilterButton( wxCommandEvent& event ){ event.Skip(); }
		virtual void OnConfigureFilter( wxHyperlinkEvent& event ){ event.Skip(); }
		virtual void OnHideFilteredButton( wxCommandEvent& event ){ event.Skip(); }
		virtual void OnSync( wxCommandEvent& event ){ event.Skip(); }
		virtual void OnEnterLeftDir( wxCommandEvent& event ){ event.Skip(); }
		virtual void OnDirChangedPanel1( wxFileDirPickerEvent& event ){ event.Skip(); }
		virtual void OnLeftGridDoubleClick( wxGridEvent& event ){ event.Skip(); }
		virtual void OnOpenContextMenu( wxGridEvent& event ){ event.Skip(); }
		virtual void OnSortLeftGrid( wxGridEvent& event ){ event.Skip(); }
		virtual void OnSwapDirs( wxCommandEvent& event ){ event.Skip(); }
		virtual void OnEnterRightDir( wxCommandEvent& event ){ event.Skip(); }
		virtual void OnDirChangedPanel2( wxFileDirPickerEvent& event ){ event.Skip(); }
		virtual void OnRightGridDoubleClick( wxGridEvent& event ){ event.Skip(); }
		virtual void OnSortRightGrid( wxGridEvent& event ){ event.Skip(); }
		virtual void OnSaveConfig( wxCommandEvent& event ){ event.Skip(); }
		virtual void OnChoiceKeyEvent( wxKeyEvent& event ){ event.Skip(); }
		virtual void OnLoadConfiguration( wxCommandEvent& event ){ event.Skip(); }
		virtual void OnLeftOnlyFiles( wxCommandEvent& event ){ event.Skip(); }
		virtual void OnLeftNewerFiles( wxCommandEvent& event ){ event.Skip(); }
		virtual void OnEqualFiles( wxCommandEvent& event ){ event.Skip(); }
		virtual void OnDifferentFiles( wxCommandEvent& event ){ event.Skip(); }
		virtual void OnRightNewerFiles( wxCommandEvent& event ){ event.Skip(); }
		virtual void OnRightOnlyFiles( wxCommandEvent& event ){ event.Skip(); }
		virtual void OnQuit( wxCommandEvent& event ){ event.Skip(); }
		
	
	public:
		GuiGenerated( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = _("FreeFileSync - Folder Comparison and Synchronization"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( 933,612 ), long style = wxDEFAULT_FRAME_STYLE|wxTAB_TRAVERSAL );
		~GuiGenerated();
	
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
		
		wxStaticBoxSizer* sbSizer11;
		wxStaticText* m_staticText32;
		wxStaticText* m_staticTextDataToCompare;
		
		wxStaticText* m_staticText37;
		wxStaticText* m_staticTextRemainingTime;
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
		wxBitmapButton* m_bpButton18;
		
		wxCheckBox* m_checkBoxUseRecycler;
		wxCheckBox* m_checkBoxContinueError;
		
		wxStaticText* m_staticText1;
		wxRadioButton* m_radioBtn1;
		wxButton* m_buttonOneWay;
		wxStaticText* m_staticText8;
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
		
		wxPanel* m_panel8;
		wxStaticText* m_staticText56;
		wxAnimationCtrl* m_animationControl1;
		
		wxStaticBitmap* m_bitmapStatus;
		wxStaticText* m_staticTextStatus;
		
		wxBoxSizer* bSizer31;
		wxStaticText* m_staticText21;
		
		wxStaticText* m_staticText55;
		wxStaticText* m_staticTextTimeElapsed;
		wxTextCtrl* m_textCtrlInfo;
		wxBoxSizer* bSizer28;
		wxStaticText* m_staticText26;
		wxStaticText* m_staticTextDataRemaining;
		
		wxButton* m_buttonOK;
		wxButton* m_buttonPause;
		wxButton* m_buttonAbort;
		
		wxStaticText* m_staticText25;
		wxStaticText* m_staticTextRemainingObj;
		
		
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
		
		wxPanel* m_panel8;
		
		wxStaticText* m_staticText56;
		
		
		wxTextCtrl* m_textCtrl8;
		wxButton* m_button8;
		
		// Virtual event handlers, overide them in your derived class
		virtual void OnClose( wxCloseEvent& event ){ event.Skip(); }
		virtual void OnOK( wxCommandEvent& event ){ event.Skip(); }
		
	
	public:
		HelpDlgGenerated( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = wxEmptyString, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( 557,385 ), long style = wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER );
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
		
		wxStaticText* m_staticText54;
		wxStaticLine* m_staticline3;
		wxStaticText* m_staticText131;
		wxStaticBitmap* m_bitmap9;
		wxStaticText* m_staticText11;
		wxHyperlinkCtrl* m_hyperlink1;
		wxStaticBitmap* m_bitmap10;
		wxStaticText* m_staticText13;
		wxHyperlinkCtrl* m_hyperlink2;
		wxAnimationCtrl* m_animationControl1;
		wxStaticText* m_staticText151;
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
		
		wxCheckBox* m_checkBoxContinueError;
		
		wxButton* m_buttonContinue;
		wxButton* m_buttonRetry;
		wxButton* m_buttonAbort;
		
		// Virtual event handlers, overide them in your derived class
		virtual void OnClose( wxCloseEvent& event ){ event.Skip(); }
		virtual void OnContinue( wxCommandEvent& event ){ event.Skip(); }
		virtual void OnRetry( wxCommandEvent& event ){ event.Skip(); }
		virtual void OnAbort( wxCommandEvent& event ){ event.Skip(); }
		
	
	public:
		ErrorDlgGenerated( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = _("An error occured"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( 445,293 ), long style = wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER );
		~ErrorDlgGenerated();
	
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
		
		wxTextCtrl* m_textCtrlMessage;
		wxButton* m_buttonOK;
		wxButton* m_buttonCancel;
		
		// Virtual event handlers, overide them in your derived class
		virtual void OnClose( wxCloseEvent& event ){ event.Skip(); }
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
		wxPanel* m_panel8;
		wxStaticText* m_staticText56;
		wxStaticText* m_staticText44;
		
		wxStaticText* m_staticText45;
		wxStaticText* m_staticText18;
		
		wxStaticText* m_staticText15;
		wxStaticBitmap* m_bitmap8;
		wxTextCtrl* m_textCtrlInclude;
		
		wxStaticText* m_staticText16;
		wxStaticBitmap* m_bitmap9;
		wxTextCtrl* m_textCtrlExclude;
		
		wxButton* m_button9;
		
		wxButton* m_button17;
		wxButton* m_button10;
		
		// Virtual event handlers, overide them in your derived class
		virtual void OnClose( wxCloseEvent& event ){ event.Skip(); }
		virtual void OnDefault( wxCommandEvent& event ){ event.Skip(); }
		virtual void OnCancel( wxCommandEvent& event ){ event.Skip(); }
		virtual void OnOK( wxCommandEvent& event ){ event.Skip(); }
		
	
	public:
		FilterDlgGenerated( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = _("Configure filter"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( -1,-1 ), long style = wxDEFAULT_DIALOG_STYLE );
		~FilterDlgGenerated();
	
};

///////////////////////////////////////////////////////////////////////////////
/// Class BatchDlgGenerated
///////////////////////////////////////////////////////////////////////////////
class BatchDlgGenerated : public wxDialog 
{
	private:
	
	protected:
		wxPanel* m_panel8;
		wxStaticText* m_staticText56;
		
		wxStaticText* m_staticText54;
		
		wxStaticLine* m_staticline10;
		wxStaticText* m_staticText531;
		wxStaticText* m_staticText53;
		wxTextCtrl* m_directoryPanel1;
		wxStaticText* m_staticText541;
		wxTextCtrl* m_directoryPanel2;
		wxRadioButton* m_radioBtnSizeDate;
		wxRadioButton* m_radioBtnContent;
		
		wxCheckBox* m_checkBoxUseRecycler;
		wxCheckBox* m_checkBoxContinueError;
		wxCheckBox* m_checkBoxSilent;
		
		
		wxBitmapButton* m_bpButtonFilter;
		wxStaticBitmap* m_bitmap8;
		wxStaticText* m_staticText15;
		wxTextCtrl* m_textCtrlInclude;
		
		wxStaticBitmap* m_bitmap9;
		wxStaticText* m_staticText16;
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
		wxButton* m_button6;
		wxButton* m_buttonCreate;
		
		// Virtual event handlers, overide them in your derived class
		virtual void OnClose( wxCloseEvent& event ){ event.Skip(); }
		virtual void OnEnterLeftDir( wxCommandEvent& event ){ event.Skip(); }
		virtual void OnEnterRightDir( wxCommandEvent& event ){ event.Skip(); }
		virtual void OnChangeCompareVar( wxCommandEvent& event ){ event.Skip(); }
		virtual void OnSelectRecycleBin( wxCommandEvent& event ){ event.Skip(); }
		virtual void OnFilterButton( wxCommandEvent& event ){ event.Skip(); }
		virtual void OnExLeftSideOnly( wxCommandEvent& event ){ event.Skip(); }
		virtual void OnExRightSideOnly( wxCommandEvent& event ){ event.Skip(); }
		virtual void OnLeftNewer( wxCommandEvent& event ){ event.Skip(); }
		virtual void OnRightNewer( wxCommandEvent& event ){ event.Skip(); }
		virtual void OnDifferent( wxCommandEvent& event ){ event.Skip(); }
		virtual void OnCancel( wxCommandEvent& event ){ event.Skip(); }
		virtual void OnCreateJob( wxCommandEvent& event ){ event.Skip(); }
		
	
	public:
		BatchDlgGenerated( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = wxEmptyString, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( -1,-1 ), long style = wxDEFAULT_DIALOG_STYLE );
		~BatchDlgGenerated();
	
};

#endif //__guiGenerated__
