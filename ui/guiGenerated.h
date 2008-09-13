///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Apr 16 2008)
// http://www.wxformbuilder.org/
//
// PLEASE DO "NOT" EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#ifndef __guiGenerated__
#define __guiGenerated__

#include <wx/intl.h>

class CustomGrid;

#include <wx/bitmap.h>
#include <wx/image.h>
#include <wx/icon.h>
#include <wx/bmpbuttn.h>
#include <wx/gdicmn.h>
#include <wx/font.h>
#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/string.h>
#include <wx/button.h>
#include <wx/sizer.h>
#include <wx/radiobut.h>
#include <wx/statbox.h>
#include <wx/hyperlink.h>
#include <wx/checkbox.h>
#include <wx/textctrl.h>
#include <wx/filepicker.h>
#include <wx/grid.h>
#include <wx/panel.h>
#include <wx/choice.h>
#include <wx/statusbr.h>
#include <wx/frame.h>
#include <wx/stattext.h>
#include <wx/gauge.h>
#include <wx/statline.h>
#include <wx/statbmp.h>
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
		wxBoxSizer* bSizer1;
		
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
		wxBitmapButton* m_bpButton11;
		
		wxBitmapButton* m_bpButton201;
		wxChoice* m_choiceLoad;
		
		wxBitmapButton* m_bpButton20;
		wxBitmapButton* m_bpButton21;
		wxBitmapButton* m_bpButton25;
		wxBitmapButton* m_bpButton22;
		wxBitmapButton* m_bpButton23;
		wxBitmapButton* m_bpButton24;
		
		
		
		wxBitmapButton* m_bpButton10;
		wxStatusBar* m_statusBar1;
		
		// Virtual event handlers, overide them in your derived class
		virtual void OnClose( wxCloseEvent& event ){ event.Skip(); }
		virtual void OnCompare( wxCommandEvent& event ){ event.Skip(); }
		virtual void OnAbortCompare( wxCommandEvent& event ){ event.Skip(); }
		virtual void OnChangeCompareVariant( wxCommandEvent& event ){ event.Skip(); }
		virtual void OnShowHelpDialog( wxCommandEvent& event ){ event.Skip(); }
		virtual void OnFilterButton( wxCommandEvent& event ){ event.Skip(); }
		virtual void OnConfigureFilter( wxHyperlinkEvent& event ){ event.Skip(); }
		virtual void OnHideFilteredButton( wxCommandEvent& event ){ event.Skip(); }
		virtual void OnSync( wxCommandEvent& event ){ event.Skip(); }
		virtual void OnEnterLeftDir( wxCommandEvent& event ){ event.Skip(); }
		virtual void OnDirChangedPanel1( wxFileDirPickerEvent& event ){ event.Skip(); }
		virtual void OnLeftGridDoubleClick( wxGridEvent& event ){ event.Skip(); }
		virtual void OnSortLeftGrid( wxGridEvent& event ){ event.Skip(); }
		virtual void OnGridSelectCell( wxGridEvent& event ){ event.Skip(); }
		virtual void OnSwapDirs( wxCommandEvent& event ){ event.Skip(); }
		virtual void OnGrid3SelectRange( wxGridRangeSelectEvent& event ){ event.Skip(); }
		virtual void OnEnterRightDir( wxCommandEvent& event ){ event.Skip(); }
		virtual void OnDirChangedPanel2( wxFileDirPickerEvent& event ){ event.Skip(); }
		virtual void OnRightGridDoubleClick( wxGridEvent& event ){ event.Skip(); }
		virtual void OnSortRightGrid( wxGridEvent& event ){ event.Skip(); }
		virtual void OnAbout( wxCommandEvent& event ){ event.Skip(); }
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
		GuiGenerated( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = _("FreeFileSync - Folder Synchronization"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( 933,612 ), long style = wxDEFAULT_FRAME_STYLE|wxTAB_TRAVERSAL );
		~GuiGenerated();
	
};

///////////////////////////////////////////////////////////////////////////////
/// Class CompareStatusGenerated
///////////////////////////////////////////////////////////////////////////////
class CompareStatusGenerated : public wxPanel 
{
	private:
	
	protected:
		wxStaticText* m_staticText321;
		wxStaticText* m_staticTextScanned;
		
		wxStaticText* m_staticText46;
		wxStaticText* m_staticTextFilesToCompare;
		
		wxStaticText* m_staticText32;
		wxStaticText* m_staticTextDataToCompare;
		wxStaticText* m_staticText30;
		wxTextCtrl* m_textCtrlFilename;
		wxGauge* m_gauge2;
	
	public:
		CompareStatusGenerated( wxWindow* parent, wxWindowID id = wxID_ANY, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( -1,-1 ), long style = wxTAB_TRAVERSAL );
		~CompareStatusGenerated();
	
};

///////////////////////////////////////////////////////////////////////////////
/// Class SyncDialogGenerated
///////////////////////////////////////////////////////////////////////////////
class SyncDialogGenerated : public wxDialog 
{
	private:
	
	protected:
		
		
		wxBitmapButton* m_bpButton18;
		
		wxStaticText* m_staticText37;
		wxTextCtrl* m_textCtrl12;
		wxStaticText* m_staticText14;
		wxTextCtrl* m_textCtrl5;
		
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
		
		wxCheckBox* m_checkBoxUseRecycler;
		wxCheckBox* m_checkBoxHideErrors;
		
		wxStaticText* m_staticText2;
		
		wxStaticText* m_staticText3;
		wxStaticLine* m_staticline3;
		wxStaticBitmap* m_bitmap13;
		wxStaticBitmap* m_bitmap14;
		wxStaticBitmap* m_bitmap15;
		wxStaticBitmap* m_bitmap16;
		wxStaticBitmap* m_bitmap17;
		
		wxBitmapButton* m_bpButton5;
		
		wxBitmapButton* m_bpButton6;
		
		wxBitmapButton* m_bpButton7;
		
		wxBitmapButton* m_bpButton8;
		
		wxBitmapButton* m_bpButton9;
		
		
		
		// Virtual event handlers, overide them in your derived class
		virtual void OnClose( wxCloseEvent& event ){ event.Skip(); }
		virtual void OnStartSync( wxCommandEvent& event ){ event.Skip(); }
		virtual void OnSyncLeftToRight( wxCommandEvent& event ){ event.Skip(); }
		virtual void OnSyncBothSides( wxCommandEvent& event ){ event.Skip(); }
		virtual void OnSyncCostum( wxCommandEvent& event ){ event.Skip(); }
		virtual void OnBack( wxCommandEvent& event ){ event.Skip(); }
		virtual void OnCancel( wxCommandEvent& event ){ event.Skip(); }
		virtual void OnSelectRecycleBin( wxCommandEvent& event ){ event.Skip(); }
		virtual void OnExLeftSideOnly( wxCommandEvent& event ){ event.Skip(); }
		virtual void OnExRightSideOnly( wxCommandEvent& event ){ event.Skip(); }
		virtual void OnLeftNewer( wxCommandEvent& event ){ event.Skip(); }
		virtual void OnRightNewer( wxCommandEvent& event ){ event.Skip(); }
		virtual void OnDifferent( wxCommandEvent& event ){ event.Skip(); }
		
	
	public:
		SyncDialogGenerated( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = _("Synchronization settings"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( 525,356 ), long style = wxDEFAULT_DIALOG_STYLE );
		~SyncDialogGenerated();
	
};

///////////////////////////////////////////////////////////////////////////////
/// Class HelpDlgGenerated
///////////////////////////////////////////////////////////////////////////////
class HelpDlgGenerated : public wxDialog 
{
	private:
	
	protected:
		
		wxStaticText* m_staticText12;
		
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
		
		wxStaticBitmap* m_bitmap11;
		wxStaticText* m_staticText14;
		
		wxStaticText* m_staticText15;
		wxStaticText* m_build;
		
		wxTextCtrl* m_textCtrl3;
		
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
		wxButton* m_button8;
		
		// Virtual event handlers, overide them in your derived class
		virtual void OnClose( wxCloseEvent& event ){ event.Skip(); }
		virtual void OnOK( wxCommandEvent& event ){ event.Skip(); }
		
	
	public:
		AboutDlgGenerated( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = _("About"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( 439,510 ), long style = wxDEFAULT_DIALOG_STYLE );
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
		
		wxCheckBox* m_checkBoxSuppress;
		
		wxButton* m_buttonContinue;
		wxButton* m_buttonRetry;
		wxButton* m_buttonAbort;
		
		// Virtual event handlers, overide them in your derived class
		virtual void OnClose( wxCloseEvent& event ){ event.Skip(); }
		virtual void OnContinue( wxCommandEvent& event ){ event.Skip(); }
		virtual void OnRetry( wxCommandEvent& event ){ event.Skip(); }
		virtual void OnAbort( wxCommandEvent& event ){ event.Skip(); }
		
	
	public:
		ErrorDlgGenerated( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = _("An error occured"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( 411,266 ), long style = wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER );
		~ErrorDlgGenerated();
	
};

///////////////////////////////////////////////////////////////////////////////
/// Class DeleteDialogGenerated
///////////////////////////////////////////////////////////////////////////////
class DeleteDialogGenerated : public wxDialog 
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
		DeleteDialogGenerated( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = _("Confirm"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( 553,336 ), long style = wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER );
		~DeleteDialogGenerated();
	
};

///////////////////////////////////////////////////////////////////////////////
/// Class FilterDlgGenerated
///////////////////////////////////////////////////////////////////////////////
class FilterDlgGenerated : public wxDialog 
{
	private:
	
	protected:
		wxStaticText* m_staticText17;
		
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
		FilterDlgGenerated( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = _("Configure filter settings"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( 484,350 ), long style = wxDEFAULT_DIALOG_STYLE );
		~FilterDlgGenerated();
	
};

///////////////////////////////////////////////////////////////////////////////
/// Class SyncStatusGenerated
///////////////////////////////////////////////////////////////////////////////
class SyncStatusGenerated : public wxDialog 
{
	private:
	
	protected:
		
		wxAnimationCtrl* m_animationControl1;
		wxStaticText* m_staticText20;
		wxStaticText* m_staticText21;
		
		wxStaticText* m_staticText26;
		wxStaticText* m_staticTextDataRemaining;
		
		
		wxStaticText* m_staticText25;
		wxStaticText* m_staticTextRemainingObj;
		
		
		// Virtual event handlers, overide them in your derived class
		virtual void OnClose( wxCloseEvent& event ){ event.Skip(); }
		virtual void OnOkay( wxCommandEvent& event ){ event.Skip(); }
		virtual void OnAbort( wxCommandEvent& event ){ event.Skip(); }
		
	
	public:
		wxStaticText* m_staticTextStatus;
		wxTextCtrl* m_textCtrlInfo;
		wxGauge* m_gauge1;
		wxButton* m_buttonOK;
		wxButton* m_buttonAbort;
		SyncStatusGenerated( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = wxEmptyString, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( 614,371 ), long style = wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER );
		~SyncStatusGenerated();
	
};

#endif //__guiGenerated__
