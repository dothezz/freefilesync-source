///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Apr 16 2008)
// http://www.wxformbuilder.org/
//
// PLEASE DO "NOT" EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#ifndef __GUI_Generated__
#define __GUI_Generated__

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
#include <wx/radiobut.h>
#include <wx/sizer.h>
#include <wx/statbox.h>
#include <wx/textctrl.h>
#include <wx/filepicker.h>
#include <wx/grid.h>
#include <wx/panel.h>
#include <wx/statusbr.h>
#include <wx/frame.h>
#include <wx/stattext.h>
#include <wx/statline.h>
#include <wx/statbmp.h>
#include <wx/dialog.h>
#include <wx/hyperlink.h>
#include <wx/animate.h>

///////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////
/// Class GUI_Generated
///////////////////////////////////////////////////////////////////////////////
class GUI_Generated : public wxFrame 
{
	private:
	
	protected:
		wxBoxSizer* bSizer1;
		
		wxBitmapButton* m_bpButton12;
		wxRadioButton* m_radioBtn4;
		wxRadioButton* m_radioBtn5;
		wxBitmapButton* m_bpButton14;
		
		wxBitmapButton* m_bpButton111;
		
		wxPanel* m_panel1;
		wxTextCtrl* m_directoryPanel1;
		wxDirPickerCtrl* m_dirPicker1;
		CustomGrid* m_grid1;
		wxPanel* m_panel3;
		wxBitmapButton* m_bpButton13;
		CustomGrid* m_grid3;
		wxPanel* m_panel2;
		wxTextCtrl* m_directoryPanel2;
		wxDirPickerCtrl* m_dirPicker2;
		CustomGrid* m_grid2;
		wxPanel* m_panel4;
		wxBitmapButton* m_bpButton11;
		
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
		virtual void OnChangeCompareVariant( wxCommandEvent& event ){ event.Skip(); }
		virtual void OnShowHelpDialog( wxCommandEvent& event ){ event.Skip(); }
		virtual void OnSync( wxCommandEvent& event ){ event.Skip(); }
		virtual void OnDirChangedPanel1( wxFileDirPickerEvent& event ){ event.Skip(); }
		virtual void OnLeftGridDoubleClick( wxGridEvent& event ){ event.Skip(); }
		virtual void OnSortLeftGrid( wxGridEvent& event ){ event.Skip(); }
		virtual void OnSwapDirs( wxCommandEvent& event ){ event.Skip(); }
		virtual void OnDeselectRow( wxGridEvent& event ){ event.Skip(); }
		virtual void OnDirChangedPanel2( wxFileDirPickerEvent& event ){ event.Skip(); }
		virtual void OnRightGridDoubleClick( wxGridEvent& event ){ event.Skip(); }
		virtual void OnSortRightGrid( wxGridEvent& event ){ event.Skip(); }
		virtual void OnAbout( wxCommandEvent& event ){ event.Skip(); }
		virtual void OnLeftOnlyFiles( wxCommandEvent& event ){ event.Skip(); }
		virtual void OnLeftNewerFiles( wxCommandEvent& event ){ event.Skip(); }
		virtual void OnEqualFiles( wxCommandEvent& event ){ event.Skip(); }
		virtual void OnDifferentFiles( wxCommandEvent& event ){ event.Skip(); }
		virtual void OnRightNewerFiles( wxCommandEvent& event ){ event.Skip(); }
		virtual void OnRightOnlyFiles( wxCommandEvent& event ){ event.Skip(); }
		virtual void OnQuit( wxCommandEvent& event ){ event.Skip(); }
		
	
	public:
		GUI_Generated( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = _("FreeFileSync - Folder Synchronization"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( 930,603 ), long style = wxDEFAULT_FRAME_STYLE|wxTAB_TRAVERSAL );
		~GUI_Generated();
	
};

///////////////////////////////////////////////////////////////////////////////
/// Class SyncDialogGenerated
///////////////////////////////////////////////////////////////////////////////
class SyncDialogGenerated : public wxDialog 
{
	private:
	
	protected:
		
		
		wxBitmapButton* m_bpButton18;
		wxStaticText* m_staticText14;
		wxTextCtrl* m_textCtrl5;
		
		wxStaticText* m_staticText1;
		wxRadioButton* m_radioBtn1;
		wxButton* m_button5;
		wxStaticText* m_staticText8;
		wxRadioButton* m_radioBtn2;
		wxButton* m_button61;
		wxStaticText* m_staticText10;
		wxRadioButton* m_radioBtn3;
		wxButton* m_button7;
		wxStaticText* m_staticText9;
		
		
		wxButton* m_button6;
		
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
		virtual void OnExLeftSideOnly( wxCommandEvent& event ){ event.Skip(); }
		virtual void OnExRightSideOnly( wxCommandEvent& event ){ event.Skip(); }
		virtual void OnLeftNewer( wxCommandEvent& event ){ event.Skip(); }
		virtual void OnRightNewer( wxCommandEvent& event ){ event.Skip(); }
		virtual void OnDifferent( wxCommandEvent& event ){ event.Skip(); }
		
	
	public:
		SyncDialogGenerated( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = _("Synchronization settings"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( 526,329 ), long style = wxDEFAULT_DIALOG_STYLE );
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
		
		wxStaticText* m_staticText14;
		wxStaticText* m_staticText15;
		
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
		AboutDlgGenerated( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = _("About"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( 435,476 ), long style = wxDEFAULT_DIALOG_STYLE );
		~AboutDlgGenerated();
	
};

#endif //__GUI_Generated__
