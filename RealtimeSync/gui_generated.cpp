///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Dec  2 2011)
// http://www.wxformbuilder.org/
//
// PLEASE DO "NOT" EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "../wx+/button.h"
#include "../wx+/dir_picker.h"

#include "gui_generated.h"

///////////////////////////////////////////////////////////////////////////

MainDlgGenerated::MainDlgGenerated( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : wxFrame( parent, id, title, pos, size, style )
{
    this->SetSizeHints( wxSize( 420,440 ), wxDefaultSize );

    m_menubar1 = new wxMenuBar( 0 );
    m_menuFile = new wxMenu();
    wxMenuItem* m_menuItem14;
    m_menuItem14 = new wxMenuItem( m_menuFile, wxID_ANY, wxString( _("S&ave configuration...") ) + wxT('\t') + wxT("CTRL+S"), wxEmptyString, wxITEM_NORMAL );
    m_menuFile->Append( m_menuItem14 );

    wxMenuItem* m_menuItem13;
    m_menuItem13 = new wxMenuItem( m_menuFile, wxID_ANY, wxString( _("&Load configuration...") ) + wxT('\t') + wxT("CTRL+L"), wxEmptyString, wxITEM_NORMAL );
    m_menuFile->Append( m_menuItem13 );

    wxMenuItem* m_separator1;
    m_separator1 = m_menuFile->AppendSeparator();

    wxMenuItem* m_menuItem4;
    m_menuItem4 = new wxMenuItem( m_menuFile, wxID_EXIT, wxString( _("&Quit") ) + wxT('\t') + wxT("CTRL+Q"), wxEmptyString, wxITEM_NORMAL );
    m_menuFile->Append( m_menuItem4 );

    m_menubar1->Append( m_menuFile, _("&File") );

    m_menuHelp = new wxMenu();
    wxMenuItem* m_menuItemContent;
    m_menuItemContent = new wxMenuItem( m_menuHelp, wxID_ANY, wxString( _("&Content") ) + wxT('\t') + wxT("F1"), wxEmptyString, wxITEM_NORMAL );
    m_menuHelp->Append( m_menuItemContent );

    wxMenuItem* m_separator2;
    m_separator2 = m_menuHelp->AppendSeparator();

    m_menuItemAbout = new wxMenuItem( m_menuHelp, wxID_ABOUT, wxString( _("&About...") ) + wxT('\t') + wxT("SHIFT+F1"), wxEmptyString, wxITEM_NORMAL );
    m_menuHelp->Append( m_menuItemAbout );

    m_menubar1->Append( m_menuHelp, _("&Help") );

    this->SetMenuBar( m_menubar1 );

    bSizerMain = new wxBoxSizer( wxVERTICAL );

    m_panelMain = new wxPanel( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
    wxBoxSizer* bSizer1;
    bSizer1 = new wxBoxSizer( wxVERTICAL );


    bSizer1->Add( 0, 5, 0, 0, 5 );

    wxStaticBoxSizer* sbSizer41;
    sbSizer41 = new wxStaticBoxSizer( new wxStaticBox( m_panelMain, wxID_ANY, _("Usage:") ), wxVERTICAL );

    m_staticText3 = new wxStaticText( m_panelMain, wxID_ANY, _("1. Select directories to monitor."), wxDefaultPosition, wxDefaultSize, 0 );
    m_staticText3->Wrap( -1 );
    sbSizer41->Add( m_staticText3, 0, wxLEFT, 10 );

    m_staticText4 = new wxStaticText( m_panelMain, wxID_ANY, _("2. Enter a command line."), wxDefaultPosition, wxDefaultSize, 0 );
    m_staticText4->Wrap( -1 );
    sbSizer41->Add( m_staticText4, 0, wxLEFT, 10 );

    m_staticText5 = new wxStaticText( m_panelMain, wxID_ANY, _("3. Press 'Start'."), wxDefaultPosition, wxDefaultSize, 0 );
    m_staticText5->Wrap( -1 );
    sbSizer41->Add( m_staticText5, 0, wxLEFT, 10 );


    sbSizer41->Add( 0, 15, 0, 0, 5 );

    m_staticText21 = new wxStaticText( m_panelMain, wxID_ANY, _("The command line is executed each time:\n- all directories become available (e.g. USB stick insert)\n- files within these directories or subdirectories are modified"), wxDefaultPosition, wxDefaultSize, 0 );
    m_staticText21->Wrap( -1 );
    sbSizer41->Add( m_staticText21, 0, wxRIGHT|wxLEFT|wxEXPAND, 5 );

    bSizer1->Add( sbSizer41, 0, wxALIGN_CENTER_HORIZONTAL|wxEXPAND|wxRIGHT|wxLEFT, 20 );

    m_staticline2 = new wxStaticLine( m_panelMain, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
    bSizer1->Add( m_staticline2, 0, wxTOP|wxBOTTOM|wxEXPAND, 10 );

    sbSizerDirToWatch2 = new wxStaticBoxSizer( new wxStaticBox( m_panelMain, wxID_ANY, _("Directories to watch") ), wxVERTICAL );

    m_panelMainFolder = new wxPanel( m_panelMain, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
    wxBoxSizer* bSizer10;
    bSizer10 = new wxBoxSizer( wxVERTICAL );

    wxBoxSizer* bSizer11;
    bSizer11 = new wxBoxSizer( wxHORIZONTAL );


    bSizer11->Add( 25, 0, 0, 0, 5 );

    m_staticTextFinalPath = new wxStaticText( m_panelMainFolder, wxID_ANY, _("dummy"), wxDefaultPosition, wxDefaultSize, 0 );
    m_staticTextFinalPath->Wrap( -1 );
    bSizer11->Add( m_staticTextFinalPath, 0, wxALIGN_CENTER_VERTICAL|wxTOP|wxBOTTOM, 2 );

    bSizer10->Add( bSizer11, 0, 0, 5 );

    wxBoxSizer* bSizer781;
    bSizer781 = new wxBoxSizer( wxHORIZONTAL );

    m_bpButtonAddFolder = new wxBitmapButton( m_panelMainFolder, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxSize( 19,21 ), wxBU_AUTODRAW );
    m_bpButtonAddFolder->SetToolTip( _("Add folder") );

    bSizer781->Add( m_bpButtonAddFolder, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 5 );

    m_bpButtonRemoveTopFolder = new wxBitmapButton( m_panelMainFolder, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxSize( 19,21 ), wxBU_AUTODRAW );
    m_bpButtonRemoveTopFolder->SetToolTip( _("Remove folder") );

    bSizer781->Add( m_bpButtonRemoveTopFolder, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 5 );

    m_txtCtrlDirectoryMain = new wxTextCtrl( m_panelMainFolder, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize( 300,-1 ), 0 );
    bSizer781->Add( m_txtCtrlDirectoryMain, 1, wxALIGN_CENTER_VERTICAL, 5 );

    m_dirPickerMain = new zen::DirPickerCtrl( m_panelMainFolder, wxID_ANY, wxEmptyString, _("Select a folder"), wxDefaultPosition, wxDefaultSize, 0 );
    m_dirPickerMain->SetToolTip( _("Select a folder") );

    bSizer781->Add( m_dirPickerMain, 0, wxALIGN_CENTER_VERTICAL, 5 );

    bSizer10->Add( bSizer781, 0, wxALIGN_CENTER_VERTICAL|wxEXPAND, 5 );

    m_panelMainFolder->SetSizer( bSizer10 );
    m_panelMainFolder->Layout();
    bSizer10->Fit( m_panelMainFolder );
    sbSizerDirToWatch2->Add( m_panelMainFolder, 0, wxEXPAND, 5 );

    m_scrolledWinFolders = new wxScrolledWindow( m_panelMain, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxHSCROLL|wxVSCROLL );
    m_scrolledWinFolders->SetScrollRate( 5, 5 );
    bSizerFolders = new wxBoxSizer( wxVERTICAL );

    m_scrolledWinFolders->SetSizer( bSizerFolders );
    m_scrolledWinFolders->Layout();
    bSizerFolders->Fit( m_scrolledWinFolders );
    sbSizerDirToWatch2->Add( m_scrolledWinFolders, 0, wxEXPAND, 5 );

    bSizer1->Add( sbSizerDirToWatch2, 0, wxEXPAND|wxRIGHT|wxLEFT, 5 );

    wxStaticBoxSizer* sbSizer3;
    sbSizer3 = new wxStaticBoxSizer( new wxStaticBox( m_panelMain, wxID_ANY, _("Command line") ), wxVERTICAL );

    m_textCtrlCommand = new wxTextCtrl( m_panelMain, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
    sbSizer3->Add( m_textCtrlCommand, 0, wxEXPAND|wxBOTTOM, 5 );

    bSizer1->Add( sbSizer3, 0, wxEXPAND|wxTOP|wxRIGHT|wxLEFT, 5 );

    wxStaticBoxSizer* sbSizer4;
    sbSizer4 = new wxStaticBoxSizer( new wxStaticBox( m_panelMain, wxID_ANY, _("Minimum Idle Time [seconds]") ), wxVERTICAL );

    m_spinCtrlDelay = new wxSpinCtrl( m_panelMain, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 0, 2000000000, 0 );
    m_spinCtrlDelay->SetToolTip( _("Idle time between detection of last change and execution of command line in seconds") );

    sbSizer4->Add( m_spinCtrlDelay, 0, wxALIGN_CENTER_HORIZONTAL, 5 );

    bSizer1->Add( sbSizer4, 0, wxEXPAND|wxTOP|wxRIGHT|wxLEFT, 5 );

    m_buttonStart = new zen::BitmapButton( m_panelMain, wxID_ANY, _("Start"), wxDefaultPosition, wxSize( -1,40 ), 0 );
    m_buttonStart->SetDefault();
    m_buttonStart->SetFont( wxFont( 14, 74, 90, 92, false, wxT("Arial Black") ) );

    bSizer1->Add( m_buttonStart, 0, wxALIGN_CENTER_HORIZONTAL|wxALL, 5 );

    m_buttonCancel = new wxButton( m_panelMain, wxID_CANCEL, _("dummy"), wxDefaultPosition, wxSize( 0,0 ), 0 );
    bSizer1->Add( m_buttonCancel, 0, 0, 5 );

    m_panelMain->SetSizer( bSizer1 );
    m_panelMain->Layout();
    bSizer1->Fit( m_panelMain );
    bSizerMain->Add( m_panelMain, 1, wxEXPAND, 5 );

    this->SetSizer( bSizerMain );
    this->Layout();
    bSizerMain->Fit( this );

    // Connect Events
    this->Connect( wxEVT_CLOSE_WINDOW, wxCloseEventHandler( MainDlgGenerated::OnClose ) );
    this->Connect( m_menuItem14->GetId(), wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( MainDlgGenerated::OnSaveConfig ) );
    this->Connect( m_menuItem13->GetId(), wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( MainDlgGenerated::OnLoadConfig ) );
    this->Connect( m_menuItem4->GetId(), wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( MainDlgGenerated::OnQuit ) );
    this->Connect( m_menuItemContent->GetId(), wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( MainDlgGenerated::OnShowHelp ) );
    this->Connect( m_menuItemAbout->GetId(), wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( MainDlgGenerated::OnMenuAbout ) );
    m_bpButtonAddFolder->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( MainDlgGenerated::OnAddFolder ), NULL, this );
    m_bpButtonRemoveTopFolder->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( MainDlgGenerated::OnRemoveTopFolder ), NULL, this );
    m_buttonStart->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( MainDlgGenerated::OnStart ), NULL, this );
    m_buttonCancel->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( MainDlgGenerated::OnQuit ), NULL, this );
}

MainDlgGenerated::~MainDlgGenerated()
{
    // Disconnect Events
    this->Disconnect( wxEVT_CLOSE_WINDOW, wxCloseEventHandler( MainDlgGenerated::OnClose ) );
    this->Disconnect( wxID_ANY, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( MainDlgGenerated::OnSaveConfig ) );
    this->Disconnect( wxID_ANY, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( MainDlgGenerated::OnLoadConfig ) );
    this->Disconnect( wxID_EXIT, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( MainDlgGenerated::OnQuit ) );
    this->Disconnect( wxID_ANY, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( MainDlgGenerated::OnShowHelp ) );
    this->Disconnect( wxID_ABOUT, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( MainDlgGenerated::OnMenuAbout ) );
    m_bpButtonAddFolder->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( MainDlgGenerated::OnAddFolder ), NULL, this );
    m_bpButtonRemoveTopFolder->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( MainDlgGenerated::OnRemoveTopFolder ), NULL, this );
    m_buttonStart->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( MainDlgGenerated::OnStart ), NULL, this );
    m_buttonCancel->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( MainDlgGenerated::OnQuit ), NULL, this );

}

FolderGenerated::FolderGenerated( wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style ) : wxPanel( parent, id, pos, size, style )
{
    wxBoxSizer* bSizer114;
    bSizer114 = new wxBoxSizer( wxHORIZONTAL );

    m_bpButtonRemoveFolder = new wxBitmapButton( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxSize( 19,21 ), wxBU_AUTODRAW );
    m_bpButtonRemoveFolder->SetToolTip( _("Remove folder") );

    bSizer114->Add( m_bpButtonRemoveFolder, 0, wxALIGN_CENTER_VERTICAL, 5 );

    wxBoxSizer* bSizer20;
    bSizer20 = new wxBoxSizer( wxHORIZONTAL );

    m_txtCtrlDirectory = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
    bSizer20->Add( m_txtCtrlDirectory, 1, wxALIGN_CENTER_VERTICAL, 5 );

    m_dirPicker = new zen::DirPickerCtrl( this, wxID_ANY, wxEmptyString, _("Select a folder"), wxDefaultPosition, wxDefaultSize, 0 );
    m_dirPicker->SetToolTip( _("Select a folder") );

    bSizer20->Add( m_dirPicker, 0, wxALIGN_CENTER_VERTICAL, 5 );

    bSizer114->Add( bSizer20, 1, wxALIGN_CENTER_VERTICAL, 5 );

    this->SetSizer( bSizer114 );
    this->Layout();
    bSizer114->Fit( this );
}

FolderGenerated::~FolderGenerated()
{
}
