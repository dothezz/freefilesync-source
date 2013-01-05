///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Oct  8 2012)
// http://www.wxformbuilder.org/
//
// PLEASE DO "NOT" EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "../wx+/button.h"

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
    m_panelMain->SetBackgroundColour( wxSystemSettings::GetColour( wxSYS_COLOUR_WINDOW ) );

    wxBoxSizer* bSizer1;
    bSizer1 = new wxBoxSizer( wxVERTICAL );

    m_panel5 = new wxPanel( m_panelMain, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
    m_panel5->SetBackgroundColour( wxSystemSettings::GetColour( wxSYS_COLOUR_BTNFACE ) );

    wxBoxSizer* bSizer17;
    bSizer17 = new wxBoxSizer( wxVERTICAL );

    wxBoxSizer* bSizer16;
    bSizer16 = new wxBoxSizer( wxHORIZONTAL );

    m_staticText9 = new wxStaticText( m_panel5, wxID_ANY, _("Usage:"), wxDefaultPosition, wxDefaultSize, 0 );
    m_staticText9->Wrap( -1 );
    m_staticText9->SetFont( wxFont( wxNORMAL_FONT->GetPointSize(), 70, 90, 92, false, wxEmptyString ) );

    bSizer16->Add( m_staticText9, 0, wxALL, 5 );

    wxBoxSizer* bSizer15;
    bSizer15 = new wxBoxSizer( wxVERTICAL );

    m_staticText3 = new wxStaticText( m_panel5, wxID_ANY, _("1. Select folders to watch."), wxDefaultPosition, wxDefaultSize, 0 );
    m_staticText3->Wrap( -1 );
    bSizer15->Add( m_staticText3, 0, wxLEFT, 10 );

    m_staticText4 = new wxStaticText( m_panel5, wxID_ANY, _("2. Enter a command line."), wxDefaultPosition, wxDefaultSize, 0 );
    m_staticText4->Wrap( -1 );
    bSizer15->Add( m_staticText4, 0, wxLEFT, 10 );

    m_staticText5 = new wxStaticText( m_panel5, wxID_ANY, _("3. Press 'Start'."), wxDefaultPosition, wxDefaultSize, 0 );
    m_staticText5->Wrap( -1 );
    bSizer15->Add( m_staticText5, 0, wxLEFT, 10 );


    bSizer16->Add( bSizer15, 0, wxTOP|wxLEFT, 5 );


    bSizer17->Add( bSizer16, 0, 0, 5 );

    m_staticText811 = new wxStaticText( m_panel5, wxID_ANY, _("To get started just import a .ffs_batch file."), wxDefaultPosition, wxDefaultSize, 0 );
    m_staticText811->Wrap( -1 );
    m_staticText811->SetForegroundColour( wxSystemSettings::GetColour( wxSYS_COLOUR_GRAYTEXT ) );

    bSizer17->Add( m_staticText811, 0, wxALL|wxALIGN_CENTER_HORIZONTAL, 5 );


    m_panel5->SetSizer( bSizer17 );
    m_panel5->Layout();
    bSizer17->Fit( m_panel5 );
    bSizer1->Add( m_panel5, 0, wxEXPAND, 5 );

    m_staticline2 = new wxStaticLine( m_panelMain, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
    bSizer1->Add( m_staticline2, 0, wxEXPAND, 5 );

    m_staticText7 = new wxStaticText( m_panelMain, wxID_ANY, _("Folders to watch"), wxDefaultPosition, wxDefaultSize, 0 );
    m_staticText7->Wrap( -1 );
    bSizer1->Add( m_staticText7, 0, wxALL, 5 );

    m_panelMainFolder = new wxPanel( m_panelMain, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
    wxFlexGridSizer* fgSizer1;
    fgSizer1 = new wxFlexGridSizer( 0, 2, 0, 0 );
    fgSizer1->AddGrowableCol( 1 );
    fgSizer1->SetFlexibleDirection( wxBOTH );
    fgSizer1->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_ALL );


    fgSizer1->Add( 0, 0, 1, wxEXPAND, 5 );

    m_staticTextFinalPath = new wxStaticText( m_panelMainFolder, wxID_ANY, _("dummy"), wxDefaultPosition, wxDefaultSize, 0 );
    m_staticTextFinalPath->Wrap( -1 );
    fgSizer1->Add( m_staticTextFinalPath, 0, wxALIGN_CENTER_VERTICAL|wxALL, 2 );

    wxBoxSizer* bSizer20;
    bSizer20 = new wxBoxSizer( wxHORIZONTAL );

    m_bpButtonAddFolder = new wxBitmapButton( m_panelMainFolder, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxSize( 24,24 ), wxBU_AUTODRAW );
    m_bpButtonAddFolder->SetToolTip( _("Add folder") );

    bSizer20->Add( m_bpButtonAddFolder, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 5 );

    m_bpButtonRemoveTopFolder = new wxBitmapButton( m_panelMainFolder, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxSize( 24,24 ), wxBU_AUTODRAW );
    m_bpButtonRemoveTopFolder->SetToolTip( _("Remove folder") );

    bSizer20->Add( m_bpButtonRemoveTopFolder, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 5 );


    fgSizer1->Add( bSizer20, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 5 );

    wxBoxSizer* bSizer19;
    bSizer19 = new wxBoxSizer( wxHORIZONTAL );

    m_txtCtrlDirectoryMain = new wxTextCtrl( m_panelMainFolder, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize( 300,-1 ), 0 );
    m_txtCtrlDirectoryMain->SetMaxLength( 0 );
    bSizer19->Add( m_txtCtrlDirectoryMain, 1, wxALIGN_CENTER_VERTICAL, 5 );

    m_buttonSelectDirMain = new wxButton( m_panelMainFolder, wxID_ANY, _("Browse"), wxDefaultPosition, wxDefaultSize, 0 );
    m_buttonSelectDirMain->SetToolTip( _("Select a folder") );

    bSizer19->Add( m_buttonSelectDirMain, 0, wxALIGN_CENTER_VERTICAL, 5 );


    fgSizer1->Add( bSizer19, 0, wxEXPAND|wxALIGN_CENTER_VERTICAL, 5 );


    m_panelMainFolder->SetSizer( fgSizer1 );
    m_panelMainFolder->Layout();
    fgSizer1->Fit( m_panelMainFolder );
    bSizer1->Add( m_panelMainFolder, 0, wxRIGHT|wxLEFT|wxEXPAND, 5 );

    m_scrolledWinFolders = new wxScrolledWindow( m_panelMain, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxHSCROLL|wxVSCROLL );
    m_scrolledWinFolders->SetScrollRate( 10, 10 );
    bSizerFolders = new wxBoxSizer( wxVERTICAL );


    m_scrolledWinFolders->SetSizer( bSizerFolders );
    m_scrolledWinFolders->Layout();
    bSizerFolders->Fit( m_scrolledWinFolders );
    bSizer1->Add( m_scrolledWinFolders, 1, wxEXPAND|wxBOTTOM|wxRIGHT|wxLEFT, 5 );

    m_staticline212 = new wxStaticLine( m_panelMain, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
    bSizer1->Add( m_staticline212, 0, wxEXPAND, 5 );

    wxBoxSizer* bSizer14;
    bSizer14 = new wxBoxSizer( wxHORIZONTAL );

    m_staticText8 = new wxStaticText( m_panelMain, wxID_ANY, _("Idle time [seconds]"), wxDefaultPosition, wxDefaultSize, 0 );
    m_staticText8->Wrap( -1 );
    bSizer14->Add( m_staticText8, 0, wxALL, 5 );


    bSizer14->Add( 0, 0, 1, wxEXPAND, 5 );

    m_spinCtrlDelay = new wxSpinCtrl( m_panelMain, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 0, 2000000000, 0 );
    m_spinCtrlDelay->SetToolTip( _("Idle time between last detected change and execution of command") );

    bSizer14->Add( m_spinCtrlDelay, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxALL, 5 );


    bSizer1->Add( bSizer14, 0, wxALIGN_RIGHT|wxEXPAND, 5 );

    m_staticline211 = new wxStaticLine( m_panelMain, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
    bSizer1->Add( m_staticline211, 0, wxEXPAND, 5 );

    m_staticText6 = new wxStaticText( m_panelMain, wxID_ANY, _("Command line"), wxDefaultPosition, wxDefaultSize, 0 );
    m_staticText6->Wrap( -1 );
    bSizer1->Add( m_staticText6, 0, wxALL, 5 );

    m_textCtrlCommand = new wxTextCtrl( m_panelMain, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
    m_textCtrlCommand->SetMaxLength( 0 );
    m_textCtrlCommand->SetToolTip( _("The command is triggered if:\n- files or subfolders change\n- new folders arrive (e.g. USB stick insert)") );

    bSizer1->Add( m_textCtrlCommand, 0, wxEXPAND|wxBOTTOM|wxRIGHT|wxLEFT, 5 );

    m_staticline5 = new wxStaticLine( m_panelMain, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
    bSizer1->Add( m_staticline5, 0, wxEXPAND, 5 );

    m_panel4 = new wxPanel( m_panelMain, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
    m_panel4->SetBackgroundColour( wxSystemSettings::GetColour( wxSYS_COLOUR_BTNFACE ) );

    wxBoxSizer* bSizer141;
    bSizer141 = new wxBoxSizer( wxHORIZONTAL );


    bSizer141->Add( 0, 0, 1, wxEXPAND, 5 );

    m_buttonStart = new zen::BitmapButton( m_panel4, wxID_OK, _("Start"), wxDefaultPosition, wxSize( -1,50 ), 0 );
    m_buttonStart->SetDefault();
    m_buttonStart->SetFont( wxFont( 14, 74, 90, 92, false, wxT("Arial Black") ) );

    bSizer141->Add( m_buttonStart, 0, wxALIGN_CENTER_HORIZONTAL|wxALL, 5 );


    bSizer141->Add( 0, 0, 1, wxEXPAND, 5 );


    m_panel4->SetSizer( bSizer141 );
    m_panel4->Layout();
    bSizer141->Fit( m_panel4 );
    bSizer1->Add( m_panel4, 0, wxEXPAND, 5 );


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

}

FolderGenerated::FolderGenerated( wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style ) : wxPanel( parent, id, pos, size, style )
{
    wxBoxSizer* bSizer114;
    bSizer114 = new wxBoxSizer( wxHORIZONTAL );

    m_bpButtonRemoveFolder = new wxBitmapButton( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxSize( 24,24 ), wxBU_AUTODRAW );
    m_bpButtonRemoveFolder->SetToolTip( _("Remove folder") );

    bSizer114->Add( m_bpButtonRemoveFolder, 0, wxALIGN_CENTER_VERTICAL, 5 );

    m_txtCtrlDirectory = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
    m_txtCtrlDirectory->SetMaxLength( 0 );
    bSizer114->Add( m_txtCtrlDirectory, 1, wxALIGN_CENTER_VERTICAL, 5 );

    m_buttonSelectDir = new wxButton( this, wxID_ANY, _("Browse"), wxDefaultPosition, wxDefaultSize, 0 );
    m_buttonSelectDir->SetToolTip( _("Select a folder") );

    bSizer114->Add( m_buttonSelectDir, 0, wxALIGN_CENTER_VERTICAL, 5 );


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
    m_textCtrl8->SetMaxLength( 0 );
    bSizer26->Add( m_textCtrl8, 1, wxEXPAND|wxALIGN_CENTER_VERTICAL, 5 );


    bSizer24->Add( bSizer26, 1, wxALIGN_CENTER_HORIZONTAL|wxEXPAND, 5 );

    m_staticline2 = new wxStaticLine( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
    bSizer24->Add( m_staticline2, 0, wxEXPAND, 5 );

    m_panel3 = new wxPanel( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
    m_panel3->SetBackgroundColour( wxSystemSettings::GetColour( wxSYS_COLOUR_BTNFACE ) );

    wxBoxSizer* bSizer13;
    bSizer13 = new wxBoxSizer( wxHORIZONTAL );


    bSizer13->Add( 0, 0, 1, wxEXPAND, 5 );

    m_buttonRetry = new wxButton( m_panel3, wxID_RETRY, _("&Retry"), wxDefaultPosition, wxSize( -1,30 ), 0 );
    m_buttonRetry->SetDefault();
    m_buttonRetry->SetFont( wxFont( 10, 70, 90, 90, false, wxEmptyString ) );

    bSizer13->Add( m_buttonRetry, 0, wxTOP|wxBOTTOM|wxLEFT|wxALIGN_CENTER_VERTICAL, 5 );

    m_buttonAbort = new wxButton( m_panel3, wxID_CANCEL, _("Cancel"), wxDefaultPosition, wxSize( -1,30 ), 0 );
    m_buttonAbort->SetFont( wxFont( 10, 70, 90, 90, false, wxEmptyString ) );

    bSizer13->Add( m_buttonAbort, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );


    bSizer13->Add( 0, 0, 1, wxEXPAND, 5 );


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
