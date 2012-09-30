///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Apr 10 2012)
// http://www.wxformbuilder.org/
//
// PLEASE DO "NOT" EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "gui_generated.h"

///////////////////////////////////////////////////////////////////////////

MainDlgGenerated::MainDlgGenerated( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : wxFrame( parent, id, title, pos, size, style )
{
    this->SetSizeHints( wxSize( 420,350 ), wxDefaultSize );

    m_menubar1 = new wxMenuBar( 0 );
    m_menuFile = new wxMenu();
    wxMenuItem* m_menuItem13;
    m_menuItem13 = new wxMenuItem( m_menuFile, wxID_OPEN, wxString( _("&Open...") ) + wxT('\t') + wxT("CTRL+O"), wxEmptyString, wxITEM_NORMAL );
    m_menuFile->Append( m_menuItem13 );

    wxMenuItem* m_menuItem14;
    m_menuItem14 = new wxMenuItem( m_menuFile, wxID_SAVEAS, wxString( _("Save &as...") ) , wxEmptyString, wxITEM_NORMAL );
    m_menuFile->Append( m_menuItem14 );

    m_menuFile->AppendSeparator();

    wxMenuItem* m_menuItem4;
    m_menuItem4 = new wxMenuItem( m_menuFile, wxID_EXIT, wxString( _("&Quit") ) , wxEmptyString, wxITEM_NORMAL );
    m_menuFile->Append( m_menuItem4 );

    m_menubar1->Append( m_menuFile, _("&Program") );

    m_menuHelp = new wxMenu();
    wxMenuItem* m_menuItemContent;
    m_menuItemContent = new wxMenuItem( m_menuHelp, wxID_HELP, wxString( _("&Content") ) + wxT('\t') + wxT("F1"), wxEmptyString, wxITEM_NORMAL );
    m_menuHelp->Append( m_menuItemContent );

    m_menuHelp->AppendSeparator();

    m_menuItemAbout = new wxMenuItem( m_menuHelp, wxID_ABOUT, wxString( _("&About") ) + wxT('\t') + wxT("SHIFT+F1"), wxEmptyString, wxITEM_NORMAL );
    m_menuHelp->Append( m_menuItemAbout );

    m_menubar1->Append( m_menuHelp, _("&Help") );

    this->SetMenuBar( m_menubar1 );

    bSizerMain = new wxBoxSizer( wxVERTICAL );

    m_panelMain = new wxPanel( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
    wxBoxSizer* bSizer1;
    bSizer1 = new wxBoxSizer( wxVERTICAL );

    wxBoxSizer* bSizer13;
    bSizer13 = new wxBoxSizer( wxVERTICAL );

    wxStaticBoxSizer* sbSizer41;
    sbSizer41 = new wxStaticBoxSizer( new wxStaticBox( m_panelMain, wxID_ANY, _("Usage:") ), wxVERTICAL );

    m_staticText3 = new wxStaticText( m_panelMain, wxID_ANY, _("1. Select folders to watch."), wxDefaultPosition, wxDefaultSize, 0 );
    m_staticText3->Wrap( -1 );
    sbSizer41->Add( m_staticText3, 0, wxLEFT, 10 );

    m_staticText4 = new wxStaticText( m_panelMain, wxID_ANY, _("2. Enter a command line."), wxDefaultPosition, wxDefaultSize, 0 );
    m_staticText4->Wrap( -1 );
    sbSizer41->Add( m_staticText4, 0, wxLEFT, 10 );

    m_staticText5 = new wxStaticText( m_panelMain, wxID_ANY, _("3. Press 'Start'."), wxDefaultPosition, wxDefaultSize, 0 );
    m_staticText5->Wrap( -1 );
    sbSizer41->Add( m_staticText5, 0, wxLEFT, 10 );


    bSizer13->Add( sbSizer41, 0, wxALIGN_CENTER_HORIZONTAL|wxEXPAND|wxTOP|wxBOTTOM, 5 );

    m_staticText811 = new wxStaticText( m_panelMain, wxID_ANY, _("To get started just import a .ffs_batch file."), wxDefaultPosition, wxDefaultSize, 0 );
    m_staticText811->Wrap( -1 );
    m_staticText811->SetForegroundColour( wxSystemSettings::GetColour( wxSYS_COLOUR_GRAYTEXT ) );

    bSizer13->Add( m_staticText811, 0, wxLEFT, 20 );


    bSizer1->Add( bSizer13, 0, wxEXPAND|wxRIGHT|wxLEFT, 20 );

    m_staticline2 = new wxStaticLine( m_panelMain, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
    bSizer1->Add( m_staticline2, 0, wxEXPAND|wxTOP|wxBOTTOM, 10 );

    sbSizerDirToWatch2 = new wxStaticBoxSizer( new wxStaticBox( m_panelMain, wxID_ANY, _("Folders to watch") ), wxVERTICAL );

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

    m_bpButtonAddFolder = new wxBitmapButton( m_panelMainFolder, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxSize( 23,23 ), wxBU_AUTODRAW );
    m_bpButtonAddFolder->SetToolTip( _("Add folder") );

    bSizer781->Add( m_bpButtonAddFolder, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 5 );

    m_bpButtonRemoveTopFolder = new wxBitmapButton( m_panelMainFolder, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxSize( 23,23 ), wxBU_AUTODRAW );
    m_bpButtonRemoveTopFolder->SetToolTip( _("Remove folder") );

    bSizer781->Add( m_bpButtonRemoveTopFolder, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 5 );

    m_txtCtrlDirectoryMain = new wxTextCtrl( m_panelMainFolder, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize( 300,-1 ), 0 );
    bSizer781->Add( m_txtCtrlDirectoryMain, 1, wxALIGN_CENTER_VERTICAL, 5 );

    m_buttonSelectDirMain = new wxButton( m_panelMainFolder, wxID_ANY, _("Browse"), wxDefaultPosition, wxDefaultSize, 0 );
    m_buttonSelectDirMain->SetToolTip( _("Select a folder") );

    bSizer781->Add( m_buttonSelectDirMain, 0, wxALIGN_CENTER_VERTICAL, 5 );


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

    wxStaticBoxSizer* sbSizer4;
    sbSizer4 = new wxStaticBoxSizer( new wxStaticBox( m_panelMain, wxID_ANY, _("Delay [seconds]") ), wxVERTICAL );

    m_spinCtrlDelay = new wxSpinCtrl( m_panelMain, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 0, 2000000000, 0 );
    m_spinCtrlDelay->SetToolTip( _("Idle time between last detected change and execution of command") );

    sbSizer4->Add( m_spinCtrlDelay, 0, wxALIGN_CENTER_HORIZONTAL, 5 );


    bSizer1->Add( sbSizer4, 0, wxEXPAND|wxTOP|wxRIGHT|wxLEFT, 5 );

    wxStaticBoxSizer* sbSizer3;
    sbSizer3 = new wxStaticBoxSizer( new wxStaticBox( m_panelMain, wxID_ANY, _("Command line") ), wxVERTICAL );

    m_textCtrlCommand = new wxTextCtrl( m_panelMain, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
    m_textCtrlCommand->SetToolTip( _("The command is triggered if:\n- files or subfolders change\n- new folders arrive (e.g. USB stick insert)") );

    sbSizer3->Add( m_textCtrlCommand, 0, wxEXPAND, 5 );


    bSizer1->Add( sbSizer3, 0, wxEXPAND|wxTOP|wxRIGHT|wxLEFT, 5 );

    m_buttonStart = new zen::BitmapButton( m_panelMain, wxID_OK, _("Start"), wxDefaultPosition, wxSize( -1,50 ), 0 );
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
    this->Connect( m_menuItem13->GetId(), wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( MainDlgGenerated::OnConfigLoad ) );
    this->Connect( m_menuItem14->GetId(), wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( MainDlgGenerated::OnConfigSave ) );
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
    this->Disconnect( wxID_OPEN, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( MainDlgGenerated::OnConfigLoad ) );
    this->Disconnect( wxID_SAVEAS, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( MainDlgGenerated::OnConfigSave ) );
    this->Disconnect( wxID_EXIT, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( MainDlgGenerated::OnQuit ) );
    this->Disconnect( wxID_HELP, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( MainDlgGenerated::OnShowHelp ) );
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

    m_bpButtonRemoveFolder = new wxBitmapButton( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxSize( 23,23 ), wxBU_AUTODRAW );
    m_bpButtonRemoveFolder->SetToolTip( _("Remove folder") );

    bSizer114->Add( m_bpButtonRemoveFolder, 0, wxALIGN_CENTER_VERTICAL, 5 );

    wxBoxSizer* bSizer20;
    bSizer20 = new wxBoxSizer( wxHORIZONTAL );

    m_txtCtrlDirectory = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
    bSizer20->Add( m_txtCtrlDirectory, 1, wxALIGN_CENTER_VERTICAL, 5 );

    m_buttonSelectDir = new wxButton( this, wxID_ANY, _("Browse"), wxDefaultPosition, wxDefaultSize, 0 );
    m_buttonSelectDir->SetToolTip( _("Select a folder") );

    bSizer20->Add( m_buttonSelectDir, 0, wxALIGN_CENTER_VERTICAL, 5 );


    bSizer114->Add( bSizer20, 1, wxALIGN_CENTER_VERTICAL, 5 );


    this->SetSizer( bSizer114 );
    this->Layout();
    bSizer114->Fit( this );
}

FolderGenerated::~FolderGenerated()
{
}

ErrorDlgGenerated::ErrorDlgGenerated( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : wxDialog( parent, id, title, pos, size, style )
{
    this->SetSizeHints( wxSize( 300,160 ), wxDefaultSize );
    this->SetBackgroundColour( wxSystemSettings::GetColour( wxSYS_COLOUR_WINDOW ) );

    wxBoxSizer* bSizer24;
    bSizer24 = new wxBoxSizer( wxVERTICAL );


    bSizer24->Add( 0, 10, 0, wxEXPAND, 5 );

    wxBoxSizer* bSizer26;
    bSizer26 = new wxBoxSizer( wxHORIZONTAL );

    m_bitmap10 = new wxStaticBitmap( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxSize( -1,-1 ), 0 );
    bSizer26->Add( m_bitmap10, 0, wxBOTTOM|wxRIGHT|wxLEFT, 5 );

    m_textCtrl8 = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize( 400,150 ), wxTE_MULTILINE|wxTE_READONLY|wxNO_BORDER );
    bSizer26->Add( m_textCtrl8, 1, wxEXPAND|wxALIGN_CENTER_VERTICAL, 5 );


    bSizer24->Add( bSizer26, 1, wxALIGN_CENTER_HORIZONTAL|wxEXPAND, 5 );

    m_staticline2 = new wxStaticLine( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
    bSizer24->Add( m_staticline2, 0, wxEXPAND|wxTOP, 5 );

    m_panel3 = new wxPanel( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
    m_panel3->SetBackgroundColour( wxSystemSettings::GetColour( wxSYS_COLOUR_BTNFACE ) );

    wxBoxSizer* bSizer13;
    bSizer13 = new wxBoxSizer( wxVERTICAL );

    wxBoxSizer* bSizer25;
    bSizer25 = new wxBoxSizer( wxHORIZONTAL );

    m_buttonRetry = new wxButton( m_panel3, wxID_RETRY, _("&Retry"), wxDefaultPosition, wxSize( -1,30 ), 0 );
    m_buttonRetry->SetDefault();
    m_buttonRetry->SetFont( wxFont( 10, 70, 90, 90, false, wxEmptyString ) );

    bSizer25->Add( m_buttonRetry, 0, wxTOP|wxBOTTOM|wxLEFT|wxALIGN_CENTER_VERTICAL, 5 );

    m_buttonAbort = new wxButton( m_panel3, wxID_CANCEL, _("Cancel"), wxDefaultPosition, wxSize( -1,30 ), 0 );
    m_buttonAbort->SetFont( wxFont( 10, 70, 90, 90, false, wxEmptyString ) );

    bSizer25->Add( m_buttonAbort, 0, wxALIGN_CENTER_VERTICAL|wxTOP|wxBOTTOM|wxLEFT, 5 );


    bSizer13->Add( bSizer25, 0, wxALIGN_CENTER_HORIZONTAL, 5 );


    m_panel3->SetSizer( bSizer13 );
    m_panel3->Layout();
    bSizer13->Fit( m_panel3 );
    bSizer24->Add( m_panel3, 0, wxEXPAND, 5 );


    this->SetSizer( bSizer24 );
    this->Layout();
    bSizer24->Fit( this );

    this->Centre( wxBOTH );

    // Connect Events
    this->Connect( wxEVT_CLOSE_WINDOW, wxCloseEventHandler( ErrorDlgGenerated::OnClose ) );
    m_buttonRetry->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( ErrorDlgGenerated::OnRetry ), NULL, this );
    m_buttonAbort->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( ErrorDlgGenerated::OnAbort ), NULL, this );
}

ErrorDlgGenerated::~ErrorDlgGenerated()
{
    // Disconnect Events
    this->Disconnect( wxEVT_CLOSE_WINDOW, wxCloseEventHandler( ErrorDlgGenerated::OnClose ) );
    m_buttonRetry->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( ErrorDlgGenerated::OnRetry ), NULL, this );
    m_buttonAbort->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( ErrorDlgGenerated::OnAbort ), NULL, this );

}
