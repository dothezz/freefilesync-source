///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Apr 16 2008)
// http://www.wxformbuilder.org/
//
// PLEASE DO "NOT" EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#ifndef __guiGenerated__
#define __guiGenerated__

#include <wx/intl.h>

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
#include <wx/statline.h>
#include <wx/bmpbuttn.h>
#include <wx/button.h>
#include <wx/sizer.h>
#include <wx/textctrl.h>
#include <wx/filepicker.h>
#include <wx/panel.h>
#include <wx/scrolwin.h>
#include <wx/statbox.h>
#include <wx/spinctrl.h>
#include <wx/frame.h>

///////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
/// Class MainDlgGenerated
///////////////////////////////////////////////////////////////////////////////
class MainDlgGenerated : public wxFrame 
{
	private:
	
	protected:
		wxMenuBar* m_menubar1;
		wxMenu* m_menuFile;
		wxMenu* m_menuHelp;
		wxMenuItem* m_menuItemAbout;
		wxBoxSizer* bSizerMain;
		wxPanel* m_panelMain;
		
		wxStaticText* m_staticText2;
		wxStaticLine* m_staticline2;
		wxPanel* m_panelMainFolder;
		wxBitmapButton* m_bpButtonAddFolder;
		wxBitmapButton* m_bpButtonRemoveTopFolder;
		wxTextCtrl* m_txtCtrlDirectoryMain;
		wxScrolledWindow* m_scrolledWinFolders;
		wxBoxSizer* bSizerFolders;
		wxTextCtrl* m_textCtrlCommand;
		wxStaticLine* m_staticline1;
		wxSpinCtrl* m_spinCtrlDelay;
		wxButtonWithImage* m_buttonStart;
		wxButton* m_buttonCancel;
		
		// Virtual event handlers, overide them in your derived class
		virtual void OnClose( wxCloseEvent& event ){ event.Skip(); }
		virtual void OnSaveConfig( wxCommandEvent& event ){ event.Skip(); }
		virtual void OnLoadConfig( wxCommandEvent& event ){ event.Skip(); }
		virtual void OnQuit( wxCommandEvent& event ){ event.Skip(); }
		virtual void OnMenuAbout( wxCommandEvent& event ){ event.Skip(); }
		virtual void OnAddFolder( wxCommandEvent& event ){ event.Skip(); }
		virtual void OnRemoveTopFolder( wxCommandEvent& event ){ event.Skip(); }
		virtual void OnStart( wxCommandEvent& event ){ event.Skip(); }
		
	
	public:
		wxDirPickerCtrl* m_dirPickerMain;
		MainDlgGenerated( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = _("RealtimeSync - Automated Synchronization"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( -1,-1 ), long style = wxDEFAULT_FRAME_STYLE|wxTAB_TRAVERSAL );
		~MainDlgGenerated();
	
};

///////////////////////////////////////////////////////////////////////////////
/// Class FolderGenerated
///////////////////////////////////////////////////////////////////////////////
class FolderGenerated : public wxPanel 
{
	private:
	
	protected:
	
	public:
		wxBitmapButton* m_bpButtonRemoveFolder;
		wxTextCtrl* m_txtCtrlDirectory;
		wxDirPickerCtrl* m_dirPicker;
		FolderGenerated( wxWindow* parent, wxWindowID id = wxID_ANY, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( -1,-1 ), long style = wxTAB_TRAVERSAL );
		~FolderGenerated();
	
};

#endif //__guiGenerated__
