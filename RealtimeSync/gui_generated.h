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
#include "../wx+/dir_picker.h"
#include "../wx+/button.h"
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
#include <wx/sizer.h>
#include <wx/statbox.h>
#include <wx/statline.h>
#include <wx/bmpbuttn.h>
#include <wx/button.h>
#include <wx/textctrl.h>
#include <wx/filepicker.h>
#include <wx/panel.h>
#include <wx/scrolwin.h>
#include <wx/spinctrl.h>
#include <wx/frame.h>
#include <wx/statbmp.h>
#include <wx/dialog.h>

#include "../zen/i18n.h"

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
    wxStaticText* m_staticText3;
    wxStaticText* m_staticText4;
    wxStaticText* m_staticText5;
    wxStaticText* m_staticText811;
    wxStaticLine* m_staticline2;
    wxStaticBoxSizer* sbSizerDirToWatch2;
    wxPanel* m_panelMainFolder;
    wxStaticText* m_staticTextFinalPath;
    wxBitmapButton* m_bpButtonAddFolder;
    wxBitmapButton* m_bpButtonRemoveTopFolder;
    wxTextCtrl* m_txtCtrlDirectoryMain;
    wxScrolledWindow* m_scrolledWinFolders;
    wxBoxSizer* bSizerFolders;
    wxSpinCtrl* m_spinCtrlDelay;
    wxTextCtrl* m_textCtrlCommand;
    zen::BitmapButton* m_buttonStart;
    wxButton* m_buttonCancel;

    // Virtual event handlers, overide them in your derived class
    virtual void OnClose( wxCloseEvent& event ) { event.Skip(); }
    virtual void OnConfigLoad( wxCommandEvent& event ) { event.Skip(); }
    virtual void OnConfigSave( wxCommandEvent& event ) { event.Skip(); }
    virtual void OnQuit( wxCommandEvent& event ) { event.Skip(); }
    virtual void OnShowHelp( wxCommandEvent& event ) { event.Skip(); }
    virtual void OnMenuAbout( wxCommandEvent& event ) { event.Skip(); }
    virtual void OnAddFolder( wxCommandEvent& event ) { event.Skip(); }
    virtual void OnRemoveTopFolder( wxCommandEvent& event ) { event.Skip(); }
    virtual void OnStart( wxCommandEvent& event ) { event.Skip(); }


public:
    zen::DirPickerCtrl* m_dirPickerMain;

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
    zen::DirPickerCtrl* m_dirPicker;

    FolderGenerated( wxWindow* parent, wxWindowID id = wxID_ANY, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( -1,-1 ), long style = wxTAB_TRAVERSAL );
    ~FolderGenerated();

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
    wxStaticLine* m_staticline2;
    wxPanel* m_panel3;
    wxButton* m_buttonRetry;
    wxButton* m_buttonAbort;

    // Virtual event handlers, overide them in your derived class
    virtual void OnClose( wxCloseEvent& event ) { event.Skip(); }
    virtual void OnRetry( wxCommandEvent& event ) { event.Skip(); }
    virtual void OnAbort( wxCommandEvent& event ) { event.Skip(); }


public:

    ErrorDlgGenerated( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = _("Error"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( -1,-1 ), long style = wxDEFAULT_DIALOG_STYLE|wxMAXIMIZE_BOX|wxMINIMIZE_BOX|wxRESIZE_BORDER );
    ~ErrorDlgGenerated();

};

#endif //__GUI_GENERATED_H__
