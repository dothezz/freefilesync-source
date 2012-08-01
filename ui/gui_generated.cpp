///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Apr 10 2012)
// http://www.wxformbuilder.org/
//
// PLEASE DO "NOT" EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "gui_generated.h"

///////////////////////////////////////////////////////////////////////////

MainDialogGenerated::MainDialogGenerated( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : wxFrame( parent, id, title, pos, size, style )
{
    this->SetSizeHints( wxSize( 640,400 ), wxDefaultSize );

    m_menubar1 = new wxMenuBar( 0 );
    m_menuFile = new wxMenu();
    m_menuItem10 = new wxMenuItem( m_menuFile, wxID_ANY, wxString( _("1. &Compare") ) + wxT('\t') + wxT("F5"), wxEmptyString, wxITEM_NORMAL );
#ifdef __WXMSW__
    m_menuItem10->SetBitmaps( wxNullBitmap );
#elif defined( __WXGTK__ )
    m_menuItem10->SetBitmap( wxNullBitmap );
#endif
    m_menuFile->Append( m_menuItem10 );

    m_menuItem11 = new wxMenuItem( m_menuFile, wxID_ANY, wxString( _("2. &Synchronize") ) + wxT('\t') + wxT("F6"), wxEmptyString, wxITEM_NORMAL );
#ifdef __WXMSW__
    m_menuItem11->SetBitmaps( wxNullBitmap );
#elif defined( __WXGTK__ )
    m_menuItem11->SetBitmap( wxNullBitmap );
#endif
    m_menuFile->Append( m_menuItem11 );

    m_menuFile->AppendSeparator();

    m_menuItemNew = new wxMenuItem( m_menuFile, wxID_NEW, wxString( _("&New") ) + wxT('\t') + wxT("Ctrl+N"), wxEmptyString, wxITEM_NORMAL );
#ifdef __WXMSW__
    m_menuItemNew->SetBitmaps( wxNullBitmap );
#elif defined( __WXGTK__ )
    m_menuItemNew->SetBitmap( wxNullBitmap );
#endif
    m_menuFile->Append( m_menuItemNew );

    m_menuItemLoad = new wxMenuItem( m_menuFile, wxID_OPEN, wxString( _("&Open...") ) + wxT('\t') + wxT("Ctrl+O"), wxEmptyString, wxITEM_NORMAL );
    m_menuFile->Append( m_menuItemLoad );

    m_menuItemSave = new wxMenuItem( m_menuFile, wxID_SAVE, wxString( _("&Save") ) + wxT('\t') + wxT("Ctrl+S"), wxEmptyString, wxITEM_NORMAL );
    m_menuFile->Append( m_menuItemSave );

    m_menuItemSaveAs = new wxMenuItem( m_menuFile, wxID_SAVEAS, wxString( _("Save &As...") ) , wxEmptyString, wxITEM_NORMAL );
    m_menuFile->Append( m_menuItemSaveAs );

    m_menuFile->AppendSeparator();

    wxMenuItem* m_menuItem4;
    m_menuItem4 = new wxMenuItem( m_menuFile, wxID_EXIT, wxString( _("&Quit") ) , wxEmptyString, wxITEM_NORMAL );
    m_menuFile->Append( m_menuItem4 );

    m_menubar1->Append( m_menuFile, _("&Program") );

    m_menuAdvanced = new wxMenu();
    m_menuLanguages = new wxMenu();
    m_menuAdvanced->Append( -1, _("&Language"), m_menuLanguages );

    m_menuAdvanced->AppendSeparator();

    m_menuItemGlobSett = new wxMenuItem( m_menuAdvanced, wxID_PREFERENCES, wxString( _("&Global settings...") ) , wxEmptyString, wxITEM_NORMAL );
    m_menuAdvanced->Append( m_menuItemGlobSett );

    m_menuItem7 = new wxMenuItem( m_menuAdvanced, wxID_ANY, wxString( _("&Create batch job...") ) , wxEmptyString, wxITEM_NORMAL );
    m_menuAdvanced->Append( m_menuItem7 );

    wxMenuItem* m_menuItem5;
    m_menuItem5 = new wxMenuItem( m_menuAdvanced, wxID_ANY, wxString( _("&Export file list...") ) , wxEmptyString, wxITEM_NORMAL );
    m_menuAdvanced->Append( m_menuItem5 );

    m_menubar1->Append( m_menuAdvanced, _("&Advanced") );

    m_menuHelp = new wxMenu();
    m_menuItemManual = new wxMenuItem( m_menuHelp, wxID_HELP, wxString( _("&Content") ) + wxT('\t') + wxT("F1"), wxEmptyString, wxITEM_NORMAL );
    m_menuHelp->Append( m_menuItemManual );

    m_menuItemCheckVer = new wxMenuItem( m_menuHelp, wxID_ANY, wxString( _("&Check for new version") ) , wxEmptyString, wxITEM_NORMAL );
    m_menuHelp->Append( m_menuItemCheckVer );

    m_menuHelp->AppendSeparator();

    m_menuItemAbout = new wxMenuItem( m_menuHelp, wxID_ABOUT, wxString( _("&About") ) + wxT('\t') + wxT("Shift+F1"), wxEmptyString, wxITEM_NORMAL );
    m_menuHelp->Append( m_menuItemAbout );

    m_menubar1->Append( m_menuHelp, _("&Help") );

    this->SetMenuBar( m_menubar1 );

    bSizerPanelHolder = new wxBoxSizer( wxVERTICAL );

    m_panelTopButtons = new wxPanel( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxRAISED_BORDER|wxTAB_TRAVERSAL );
    bSizerTopButtons = new wxBoxSizer( wxHORIZONTAL );

    wxBoxSizer* bSizer155;
    bSizer155 = new wxBoxSizer( wxHORIZONTAL );


    bSizer155->Add( 15, 0, 0, wxALIGN_CENTER_VERTICAL, 5 );

    wxFlexGridSizer* fgSizer121;
    fgSizer121 = new wxFlexGridSizer( 2, 2, 0, 0 );
    fgSizer121->SetFlexibleDirection( wxBOTH );
    fgSizer121->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

    m_staticTextCmpVariant = new wxStaticText( m_panelTopButtons, wxID_ANY, _("dummy"), wxDefaultPosition, wxDefaultSize, 0 );
    m_staticTextCmpVariant->Wrap( -1 );
    m_staticTextCmpVariant->SetFont( wxFont( 8, 70, 90, 92, false, wxEmptyString ) );
    m_staticTextCmpVariant->SetForegroundColour( wxSystemSettings::GetColour( wxSYS_COLOUR_GRAYTEXT ) );

    fgSizer121->Add( m_staticTextCmpVariant, 0, wxALIGN_CENTER_VERTICAL|wxALIGN_CENTER_HORIZONTAL|wxTOP, 1 );


    fgSizer121->Add( 0, 0, 1, 0, 5 );

    wxBoxSizer* bSizer30;
    bSizer30 = new wxBoxSizer( wxHORIZONTAL );

    m_buttonCompare = new zen::BitmapButton( m_panelTopButtons, wxID_OK, _("Compare"), wxDefaultPosition, wxSize( 180,42 ), 0 );
    m_buttonCompare->SetDefault();
    m_buttonCompare->SetFont( wxFont( 14, 74, 90, 92, false, wxEmptyString ) );
    m_buttonCompare->SetToolTip( _("Compare both sides") );

    bSizer30->Add( m_buttonCompare, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 5 );

    m_buttonAbort = new wxButton( m_panelTopButtons, wxID_CANCEL, _("&Abort"), wxDefaultPosition, wxSize( 180,42 ), 0 );
    m_buttonAbort->SetFont( wxFont( 14, 74, 90, 92, false, wxEmptyString ) );
    m_buttonAbort->Enable( false );
    m_buttonAbort->Hide();

    bSizer30->Add( m_buttonAbort, 0, wxALIGN_CENTER_VERTICAL, 5 );


    fgSizer121->Add( bSizer30, 0, wxALIGN_CENTER_VERTICAL, 5 );

    m_bpButtonCmpConfig = new wxBitmapButton( m_panelTopButtons, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxSize( -1,42 ), wxBU_AUTODRAW );
    m_bpButtonCmpConfig->SetToolTip( _("Comparison settings") );

    fgSizer121->Add( m_bpButtonCmpConfig, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 3 );


    bSizer155->Add( fgSizer121, 0, wxALIGN_CENTER_VERTICAL|wxBOTTOM, 5 );


    bSizerTopButtons->Add( bSizer155, 0, wxALIGN_CENTER_VERTICAL, 5 );


    bSizerTopButtons->Add( 0, 0, 1, wxALIGN_CENTER_VERTICAL, 5 );

    wxBoxSizer* bSizer1551;
    bSizer1551 = new wxBoxSizer( wxHORIZONTAL );

    wxFlexGridSizer* fgSizer12;
    fgSizer12 = new wxFlexGridSizer( 2, 2, 0, 0 );
    fgSizer12->SetFlexibleDirection( wxBOTH );
    fgSizer12->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );


    fgSizer12->Add( 0, 0, 1, wxEXPAND, 5 );

    m_staticTextSyncVariant = new wxStaticText( m_panelTopButtons, wxID_ANY, _("dummy"), wxDefaultPosition, wxDefaultSize, 0 );
    m_staticTextSyncVariant->Wrap( -1 );
    m_staticTextSyncVariant->SetFont( wxFont( 8, 70, 90, 92, false, wxEmptyString ) );
    m_staticTextSyncVariant->SetForegroundColour( wxSystemSettings::GetColour( wxSYS_COLOUR_GRAYTEXT ) );

    fgSizer12->Add( m_staticTextSyncVariant, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxTOP, 1 );

    m_bpButtonSyncConfig = new wxBitmapButton( m_panelTopButtons, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxSize( -1,42 ), wxBU_AUTODRAW );
    m_bpButtonSyncConfig->SetToolTip( _("Synchronization settings") );

    fgSizer12->Add( m_bpButtonSyncConfig, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 3 );

    m_buttonStartSync = new zen::BitmapButton( m_panelTopButtons, wxID_ANY, _("Synchronize"), wxDefaultPosition, wxSize( 180,42 ), 0 );
    m_buttonStartSync->SetFont( wxFont( 14, 74, 90, 92, false, wxEmptyString ) );
    m_buttonStartSync->SetToolTip( _("Start synchronization") );

    fgSizer12->Add( m_buttonStartSync, 0, wxALIGN_CENTER_VERTICAL, 5 );


    bSizer1551->Add( fgSizer12, 0, wxALIGN_CENTER_VERTICAL|wxBOTTOM, 5 );


    bSizer1551->Add( 15, 0, 0, wxALIGN_CENTER_VERTICAL, 5 );


    bSizerTopButtons->Add( bSizer1551, 0, wxALIGN_CENTER_VERTICAL, 5 );


    m_panelTopButtons->SetSizer( bSizerTopButtons );
    m_panelTopButtons->Layout();
    bSizerTopButtons->Fit( m_panelTopButtons );
    bSizerPanelHolder->Add( m_panelTopButtons, 0, wxALIGN_CENTER_HORIZONTAL|wxEXPAND, 5 );

    m_panelDirectoryPairs = new wxPanel( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxSTATIC_BORDER|wxTAB_TRAVERSAL );
    wxBoxSizer* bSizer1601;
    bSizer1601 = new wxBoxSizer( wxVERTICAL );

    wxBoxSizer* bSizer91;
    bSizer91 = new wxBoxSizer( wxHORIZONTAL );

    m_panelTopLeft = new wxPanel( m_panelDirectoryPairs, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
    m_panelTopLeft->SetMinSize( wxSize( 1,-1 ) );

    wxBoxSizer* bSizer180;
    bSizer180 = new wxBoxSizer( wxVERTICAL );

    wxBoxSizer* bSizer181;
    bSizer181 = new wxBoxSizer( wxHORIZONTAL );


    bSizer181->Add( 25, 0, 0, 0, 5 );

    m_staticTextFinalPathLeft = new wxStaticText( m_panelTopLeft, wxID_ANY, _("dummy"), wxDefaultPosition, wxDefaultSize, 0 );
    m_staticTextFinalPathLeft->Wrap( -1 );
    bSizer181->Add( m_staticTextFinalPathLeft, 0, wxALIGN_CENTER_VERTICAL|wxTOP|wxBOTTOM, 2 );


    bSizer180->Add( bSizer181, 0, 0, 5 );

    wxBoxSizer* bSizer182;
    bSizer182 = new wxBoxSizer( wxHORIZONTAL );

    m_bpButtonAddPair = new wxBitmapButton( m_panelTopLeft, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxSize( 19,21 ), wxBU_AUTODRAW );
    m_bpButtonAddPair->SetToolTip( _("Add folder pair") );

    bSizer182->Add( m_bpButtonAddPair, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 5 );

    m_bpButtonRemovePair = new wxBitmapButton( m_panelTopLeft, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxSize( 19,21 ), wxBU_AUTODRAW );
    m_bpButtonRemovePair->SetToolTip( _("Remove folder pair") );

    bSizer182->Add( m_bpButtonRemovePair, 0, wxALIGN_CENTER_VERTICAL, 5 );

    m_directoryLeft = new FolderHistoryBox( m_panelTopLeft, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0, NULL, 0 );
    bSizer182->Add( m_directoryLeft, 1, wxALIGN_CENTER_VERTICAL, 5 );

    m_dirPickerLeft = new zen::DirPickerCtrl( m_panelTopLeft, wxID_ANY, wxEmptyString, _("Select a folder"), wxDefaultPosition, wxDefaultSize, 0 );
    m_dirPickerLeft->SetToolTip( _("Select a folder") );

    bSizer182->Add( m_dirPickerLeft, 0, wxALIGN_CENTER_VERTICAL, 5 );


    bSizer180->Add( bSizer182, 0, wxEXPAND, 5 );


    m_panelTopLeft->SetSizer( bSizer180 );
    m_panelTopLeft->Layout();
    bSizer180->Fit( m_panelTopLeft );
    bSizer91->Add( m_panelTopLeft, 1, wxLEFT|wxALIGN_CENTER_VERTICAL|wxEXPAND, 5 );

    m_panelTopMiddle = new wxPanel( m_panelDirectoryPairs, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
    wxBoxSizer* bSizer93;
    bSizer93 = new wxBoxSizer( wxVERTICAL );


    bSizer93->Add( 0, 3, 0, 0, 5 );

    m_bpButtonSwapSides = new wxBitmapButton( m_panelTopMiddle, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxSize( -1,-1 ), wxBU_AUTODRAW );
    m_bpButtonSwapSides->SetToolTip( _("Swap sides") );

    bSizer93->Add( m_bpButtonSwapSides, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxEXPAND, 5 );

    wxBoxSizer* bSizer160;
    bSizer160 = new wxBoxSizer( wxHORIZONTAL );


    bSizer160->Add( 0, 0, 1, wxEXPAND|wxALIGN_CENTER_VERTICAL, 5 );

    m_bpButtonAltCompCfg = new wxBitmapButton( m_panelTopMiddle, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxSize( 20,20 ), wxBU_AUTODRAW );
    bSizer160->Add( m_bpButtonAltCompCfg, 0, wxALIGN_CENTER_VERTICAL, 5 );

    m_bpButtonLocalFilter = new wxBitmapButton( m_panelTopMiddle, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxSize( 20,20 ), wxBU_AUTODRAW );
    bSizer160->Add( m_bpButtonLocalFilter, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 3 );

    m_bpButtonAltSyncCfg = new wxBitmapButton( m_panelTopMiddle, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxSize( 20,20 ), wxBU_AUTODRAW );
    bSizer160->Add( m_bpButtonAltSyncCfg, 0, wxALIGN_CENTER_VERTICAL, 5 );


    bSizer160->Add( 0, 0, 1, wxEXPAND, 5 );


    bSizer93->Add( bSizer160, 0, wxALIGN_CENTER_HORIZONTAL, 5 );


    m_panelTopMiddle->SetSizer( bSizer93 );
    m_panelTopMiddle->Layout();
    bSizer93->Fit( m_panelTopMiddle );
    bSizer91->Add( m_panelTopMiddle, 0, wxRIGHT|wxLEFT|wxALIGN_CENTER_VERTICAL|wxEXPAND, 5 );

    m_panelTopRight = new wxPanel( m_panelDirectoryPairs, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
    m_panelTopRight->SetMinSize( wxSize( 1,-1 ) );

    wxBoxSizer* bSizer183;
    bSizer183 = new wxBoxSizer( wxVERTICAL );

    wxBoxSizer* bSizer184;
    bSizer184 = new wxBoxSizer( wxHORIZONTAL );


    bSizer184->Add( 25, 0, 0, 0, 5 );

    m_staticTextFinalPathRight = new wxStaticText( m_panelTopRight, wxID_ANY, _("dummy"), wxDefaultPosition, wxDefaultSize, 0 );
    m_staticTextFinalPathRight->Wrap( -1 );
    bSizer184->Add( m_staticTextFinalPathRight, 0, wxTOP|wxBOTTOM|wxALIGN_CENTER_VERTICAL, 2 );


    bSizer183->Add( bSizer184, 0, 0, 5 );

    wxBoxSizer* bSizer179;
    bSizer179 = new wxBoxSizer( wxHORIZONTAL );

    m_directoryRight = new FolderHistoryBox( m_panelTopRight, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0, NULL, 0 );
    bSizer179->Add( m_directoryRight, 1, wxALIGN_CENTER_VERTICAL, 5 );

    m_dirPickerRight = new zen::DirPickerCtrl( m_panelTopRight, wxID_ANY, wxEmptyString, _("Select a folder"), wxDefaultPosition, wxDefaultSize, 0 );
    m_dirPickerRight->SetToolTip( _("Select a folder") );

    bSizer179->Add( m_dirPickerRight, 0, wxALIGN_CENTER_VERTICAL, 5 );


    bSizer183->Add( bSizer179, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxEXPAND, 5 );


    m_panelTopRight->SetSizer( bSizer183 );
    m_panelTopRight->Layout();
    bSizer183->Fit( m_panelTopRight );
    bSizer91->Add( m_panelTopRight, 1, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxEXPAND, 5 );


    bSizer1601->Add( bSizer91, 0, wxEXPAND, 5 );

    m_scrolledWindowFolderPairs = new wxScrolledWindow( m_panelDirectoryPairs, wxID_ANY, wxDefaultPosition, wxSize( -1,-1 ), wxHSCROLL|wxVSCROLL );
    m_scrolledWindowFolderPairs->SetScrollRate( 5, 5 );
    m_scrolledWindowFolderPairs->SetMinSize( wxSize( -1,0 ) );

    bSizerAddFolderPairs = new wxBoxSizer( wxVERTICAL );


    m_scrolledWindowFolderPairs->SetSizer( bSizerAddFolderPairs );
    m_scrolledWindowFolderPairs->Layout();
    bSizerAddFolderPairs->Fit( m_scrolledWindowFolderPairs );
    bSizer1601->Add( m_scrolledWindowFolderPairs, 1, wxALIGN_CENTER_VERTICAL|wxEXPAND, 5 );


    m_panelDirectoryPairs->SetSizer( bSizer1601 );
    m_panelDirectoryPairs->Layout();
    bSizer1601->Fit( m_panelDirectoryPairs );
    bSizerPanelHolder->Add( m_panelDirectoryPairs, 0, wxALIGN_CENTER_HORIZONTAL|wxEXPAND, 5 );

    m_gridNavi = new zen::Grid( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxHSCROLL|wxVSCROLL );
    m_gridNavi->SetScrollRate( 5, 5 );
    bSizerPanelHolder->Add( m_gridNavi, 1, wxEXPAND, 5 );

    m_panelCenter = new wxPanel( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
    wxBoxSizer* bSizer1711;
    bSizer1711 = new wxBoxSizer( wxVERTICAL );

    m_splitterMain = new zen::TripleSplitter( m_panelCenter, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
    wxBoxSizer* bSizer1781;
    bSizer1781 = new wxBoxSizer( wxHORIZONTAL );

    m_gridMainL = new zen::Grid( m_splitterMain, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxHSCROLL|wxVSCROLL );
    m_gridMainL->SetScrollRate( 5, 5 );
    bSizer1781->Add( m_gridMainL, 1, wxEXPAND, 5 );

    m_gridMainC = new zen::Grid( m_splitterMain, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxHSCROLL|wxVSCROLL );
    m_gridMainC->SetScrollRate( 5, 5 );
    bSizer1781->Add( m_gridMainC, 0, wxEXPAND, 5 );

    m_gridMainR = new zen::Grid( m_splitterMain, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxHSCROLL|wxVSCROLL );
    m_gridMainR->SetScrollRate( 5, 5 );
    bSizer1781->Add( m_gridMainR, 1, wxEXPAND, 5 );


    m_splitterMain->SetSizer( bSizer1781 );
    m_splitterMain->Layout();
    bSizer1781->Fit( m_splitterMain );
    bSizer1711->Add( m_splitterMain, 1, wxEXPAND, 5 );

    m_panelStatusBar = new wxPanel( m_panelCenter, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxSTATIC_BORDER|wxTAB_TRAVERSAL );
    wxBoxSizer* bSizer451;
    bSizer451 = new wxBoxSizer( wxHORIZONTAL );

    bSizer451->SetMinSize( wxSize( -1,22 ) );
    wxBoxSizer* bSizer53;
    bSizer53 = new wxBoxSizer( wxHORIZONTAL );


    bSizer53->Add( 0, 0, 1, wxALIGN_CENTER_VERTICAL, 5 );

    bSizerStatusLeftDirectories = new wxBoxSizer( wxHORIZONTAL );

    m_bitmapSmallDirectoryLeft = new wxStaticBitmap( m_panelStatusBar, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, 0 );
    bSizerStatusLeftDirectories->Add( m_bitmapSmallDirectoryLeft, 0, wxALIGN_CENTER_VERTICAL, 5 );


    bSizerStatusLeftDirectories->Add( 2, 0, 0, 0, 5 );

    m_staticTextStatusLeftDirs = new wxStaticText( m_panelStatusBar, wxID_ANY, _("dummy"), wxDefaultPosition, wxDefaultSize, 0 );
    m_staticTextStatusLeftDirs->Wrap( -1 );
    bSizerStatusLeftDirectories->Add( m_staticTextStatusLeftDirs, 0, wxALIGN_CENTER_VERTICAL|wxALIGN_CENTER_HORIZONTAL, 5 );


    bSizer53->Add( bSizerStatusLeftDirectories, 0, wxALIGN_CENTER_VERTICAL, 5 );

    bSizerStatusLeftFiles = new wxBoxSizer( wxHORIZONTAL );


    bSizerStatusLeftFiles->Add( 10, 0, 0, 0, 5 );

    m_bitmapSmallFileLeft = new wxStaticBitmap( m_panelStatusBar, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, 0 );
    bSizerStatusLeftFiles->Add( m_bitmapSmallFileLeft, 0, wxALIGN_CENTER_VERTICAL, 5 );


    bSizerStatusLeftFiles->Add( 2, 0, 0, 0, 5 );

    m_staticTextStatusLeftFiles = new wxStaticText( m_panelStatusBar, wxID_ANY, _("dummy"), wxDefaultPosition, wxDefaultSize, 0 );
    m_staticTextStatusLeftFiles->Wrap( -1 );
    bSizerStatusLeftFiles->Add( m_staticTextStatusLeftFiles, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 5 );


    bSizerStatusLeftFiles->Add( 10, 0, 0, 0, 5 );

    m_staticTextStatusLeftBytes = new wxStaticText( m_panelStatusBar, wxID_ANY, _("dummy"), wxDefaultPosition, wxDefaultSize, 0 );
    m_staticTextStatusLeftBytes->Wrap( -1 );
    bSizerStatusLeftFiles->Add( m_staticTextStatusLeftBytes, 0, wxALIGN_CENTER_VERTICAL, 5 );


    bSizer53->Add( bSizerStatusLeftFiles, 0, wxALIGN_CENTER_VERTICAL, 5 );


    bSizer53->Add( 0, 0, 1, wxALIGN_CENTER_VERTICAL, 5 );


    bSizer451->Add( bSizer53, 1, wxALIGN_BOTTOM|wxEXPAND, 5 );

    m_staticline9 = new wxStaticLine( m_panelStatusBar, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_VERTICAL );
    bSizer451->Add( m_staticline9, 0, wxALIGN_CENTER_VERTICAL|wxTOP|wxEXPAND, 2 );


    bSizer451->Add( 26, 0, 0, wxALIGN_CENTER_VERTICAL, 5 );

    m_staticTextStatusMiddle = new wxStaticText( m_panelStatusBar, wxID_ANY, _("dummy"), wxDefaultPosition, wxDefaultSize, 0 );
    m_staticTextStatusMiddle->Wrap( -1 );
    m_staticTextStatusMiddle->SetFont( wxFont( 8, 70, 90, 92, false, wxEmptyString ) );

    bSizer451->Add( m_staticTextStatusMiddle, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 5 );


    bSizer451->Add( 26, 0, 0, wxALIGN_CENTER_VERTICAL, 5 );

    m_staticline10 = new wxStaticLine( m_panelStatusBar, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_VERTICAL );
    bSizer451->Add( m_staticline10, 0, wxEXPAND|wxALIGN_CENTER_VERTICAL|wxTOP, 2 );

    wxBoxSizer* bSizer52;
    bSizer52 = new wxBoxSizer( wxHORIZONTAL );


    bSizer52->Add( 0, 0, 1, wxALIGN_CENTER_VERTICAL, 5 );

    bSizerStatusRightDirectories = new wxBoxSizer( wxHORIZONTAL );

    m_bitmapSmallDirectoryRight = new wxStaticBitmap( m_panelStatusBar, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, 0 );
    bSizerStatusRightDirectories->Add( m_bitmapSmallDirectoryRight, 0, wxALIGN_CENTER_VERTICAL, 5 );


    bSizerStatusRightDirectories->Add( 2, 0, 0, 0, 5 );

    m_staticTextStatusRightDirs = new wxStaticText( m_panelStatusBar, wxID_ANY, _("dummy"), wxDefaultPosition, wxDefaultSize, 0 );
    m_staticTextStatusRightDirs->Wrap( -1 );
    bSizerStatusRightDirectories->Add( m_staticTextStatusRightDirs, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 5 );


    bSizer52->Add( bSizerStatusRightDirectories, 0, wxALIGN_CENTER_VERTICAL, 5 );

    bSizerStatusRightFiles = new wxBoxSizer( wxHORIZONTAL );


    bSizerStatusRightFiles->Add( 10, 0, 0, 0, 5 );

    m_bitmapSmallFileRight = new wxStaticBitmap( m_panelStatusBar, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, 0 );
    bSizerStatusRightFiles->Add( m_bitmapSmallFileRight, 0, wxALIGN_CENTER_VERTICAL, 5 );


    bSizerStatusRightFiles->Add( 2, 0, 0, 0, 5 );

    m_staticTextStatusRightFiles = new wxStaticText( m_panelStatusBar, wxID_ANY, _("dummy"), wxDefaultPosition, wxDefaultSize, 0 );
    m_staticTextStatusRightFiles->Wrap( -1 );
    bSizerStatusRightFiles->Add( m_staticTextStatusRightFiles, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 5 );


    bSizerStatusRightFiles->Add( 10, 0, 0, 0, 5 );

    m_staticTextStatusRightBytes = new wxStaticText( m_panelStatusBar, wxID_ANY, _("dummy"), wxDefaultPosition, wxDefaultSize, 0 );
    m_staticTextStatusRightBytes->Wrap( -1 );
    bSizerStatusRightFiles->Add( m_staticTextStatusRightBytes, 0, wxALIGN_CENTER_VERTICAL, 5 );


    bSizer52->Add( bSizerStatusRightFiles, 0, wxALIGN_CENTER_VERTICAL, 5 );


    bSizer52->Add( 0, 0, 1, wxALIGN_CENTER_VERTICAL, 5 );


    bSizer451->Add( bSizer52, 1, wxALIGN_BOTTOM|wxEXPAND, 5 );


    m_panelStatusBar->SetSizer( bSizer451 );
    m_panelStatusBar->Layout();
    bSizer451->Fit( m_panelStatusBar );
    bSizer1711->Add( m_panelStatusBar, 0, wxALIGN_CENTER_HORIZONTAL|wxEXPAND, 5 );


    m_panelCenter->SetSizer( bSizer1711 );
    m_panelCenter->Layout();
    bSizer1711->Fit( m_panelCenter );
    bSizerPanelHolder->Add( m_panelCenter, 1, wxEXPAND, 5 );

    m_panelConfig = new wxPanel( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
    bSizerConfig = new wxBoxSizer( wxHORIZONTAL );

    wxBoxSizer* bSizer151;
    bSizer151 = new wxBoxSizer( wxHORIZONTAL );

    m_bpButtonLoad = new wxBitmapButton( m_panelConfig, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxSize( 40,40 ), wxBU_AUTODRAW );
    m_bpButtonLoad->SetToolTip( _("Load configuration from file") );

    bSizer151->Add( m_bpButtonLoad, 0, wxALIGN_CENTER_VERTICAL, 5 );

    m_bpButtonSave = new wxBitmapButton( m_panelConfig, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxSize( 40,40 ), wxBU_AUTODRAW );
    m_bpButtonSave->SetToolTip( _("Save current configuration to file") );

    bSizer151->Add( m_bpButtonSave, 0, wxALIGN_CENTER_VERTICAL, 5 );


    bSizerConfig->Add( bSizer151, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 5 );

    m_listBoxHistory = new wxListBox( m_panelConfig, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0, NULL, wxLB_EXTENDED|wxLB_NEEDED_SB|wxLB_SORT );
    m_listBoxHistory->SetToolTip( _("Last used configurations (press DEL to remove from list)") );
    m_listBoxHistory->SetMinSize( wxSize( -1,40 ) );

    bSizerConfig->Add( m_listBoxHistory, 1, wxEXPAND|wxALIGN_CENTER_VERTICAL, 5 );


    m_panelConfig->SetSizer( bSizerConfig );
    m_panelConfig->Layout();
    bSizerConfig->Fit( m_panelConfig );
    bSizerPanelHolder->Add( m_panelConfig, 0, 0, 5 );

    m_panelFilter = new wxPanel( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
    wxBoxSizer* bSizer171;
    bSizer171 = new wxBoxSizer( wxHORIZONTAL );

    m_bpButtonFilter = new wxBitmapButton( m_panelFilter, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxSize( 40,40 ), wxBU_AUTODRAW|wxFULL_REPAINT_ON_RESIZE );
    bSizer171->Add( m_bpButtonFilter, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );

    m_checkBoxHideFilt = new wxCheckBox( m_panelFilter, wxID_ANY, _("Hide excluded items"), wxDefaultPosition, wxDefaultSize, 0 );
    m_checkBoxHideFilt->SetToolTip( _("Hide filtered or temporarily excluded files") );

    bSizer171->Add( m_checkBoxHideFilt, 0, wxALIGN_CENTER_VERTICAL, 5 );


    m_panelFilter->SetSizer( bSizer171 );
    m_panelFilter->Layout();
    bSizer171->Fit( m_panelFilter );
    bSizerPanelHolder->Add( m_panelFilter, 0, 0, 5 );

    m_panelStatistics = new wxPanel( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
    bSizer1801 = new wxBoxSizer( wxHORIZONTAL );


    bSizer1801->Add( 0, 0, 1, wxEXPAND, 5 );

    bSizerStatistics = new wxBoxSizer( wxHORIZONTAL );

    wxBoxSizer* bSizer1712;
    bSizer1712 = new wxBoxSizer( wxVERTICAL );

    m_bitmapCreateLeft = new wxStaticBitmap( m_panelStatistics, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, 0 );
    m_bitmapCreateLeft->SetToolTip( _("Number of files and folders that will be created") );

    bSizer1712->Add( m_bitmapCreateLeft, 0, wxALIGN_CENTER_VERTICAL|wxALIGN_CENTER_HORIZONTAL, 5 );


    bSizer1712->Add( 5, 2, 0, 0, 5 );


    bSizer1712->Add( 0, 0, 1, wxEXPAND, 5 );

    m_staticTextCreateLeft = new wxStaticText( m_panelStatistics, wxID_ANY, _("dummy"), wxDefaultPosition, wxDefaultSize, 0 );
    m_staticTextCreateLeft->Wrap( -1 );
    m_staticTextCreateLeft->SetToolTip( _("Number of files and folders that will be created") );

    bSizer1712->Add( m_staticTextCreateLeft, 0, wxALIGN_CENTER_VERTICAL|wxALIGN_CENTER_HORIZONTAL, 5 );


    bSizerStatistics->Add( bSizer1712, 0, wxALIGN_CENTER_VERTICAL|wxEXPAND, 5 );


    bSizerStatistics->Add( 5, 5, 0, 0, 5 );

    wxBoxSizer* bSizer172;
    bSizer172 = new wxBoxSizer( wxVERTICAL );

    m_bitmapUpdateLeft = new wxStaticBitmap( m_panelStatistics, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, 0 );
    m_bitmapUpdateLeft->SetToolTip( _("Number of files that will be overwritten") );

    bSizer172->Add( m_bitmapUpdateLeft, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 5 );


    bSizer172->Add( 5, 2, 0, 0, 5 );


    bSizer172->Add( 0, 0, 1, wxEXPAND, 5 );

    m_staticTextUpdateLeft = new wxStaticText( m_panelStatistics, wxID_ANY, _("dummy"), wxDefaultPosition, wxDefaultSize, 0 );
    m_staticTextUpdateLeft->Wrap( -1 );
    m_staticTextUpdateLeft->SetToolTip( _("Number of files that will be overwritten") );

    bSizer172->Add( m_staticTextUpdateLeft, 0, wxALIGN_CENTER_VERTICAL|wxALIGN_CENTER_HORIZONTAL, 5 );


    bSizerStatistics->Add( bSizer172, 0, wxALIGN_CENTER_VERTICAL|wxEXPAND, 5 );


    bSizerStatistics->Add( 5, 5, 0, 0, 5 );

    wxBoxSizer* bSizer173;
    bSizer173 = new wxBoxSizer( wxVERTICAL );

    m_bitmapDeleteLeft = new wxStaticBitmap( m_panelStatistics, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, 0 );
    m_bitmapDeleteLeft->SetToolTip( _("Number of files and folders that will be deleted") );

    bSizer173->Add( m_bitmapDeleteLeft, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 5 );


    bSizer173->Add( 5, 2, 0, 0, 5 );


    bSizer173->Add( 0, 0, 1, wxEXPAND, 5 );

    m_staticTextDeleteLeft = new wxStaticText( m_panelStatistics, wxID_ANY, _("dummy"), wxDefaultPosition, wxDefaultSize, 0 );
    m_staticTextDeleteLeft->Wrap( -1 );
    m_staticTextDeleteLeft->SetToolTip( _("Number of files and folders that will be deleted") );

    bSizer173->Add( m_staticTextDeleteLeft, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 5 );


    bSizerStatistics->Add( bSizer173, 0, wxALIGN_CENTER_VERTICAL|wxEXPAND, 5 );


    bSizerStatistics->Add( 5, 5, 0, 0, 5 );

    bSizerData = new wxBoxSizer( wxVERTICAL );

    m_bitmapData = new wxStaticBitmap( m_panelStatistics, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, 0 );
    m_bitmapData->SetToolTip( _("Total bytes to copy") );

    bSizerData->Add( m_bitmapData, 0, wxALIGN_CENTER_VERTICAL|wxALIGN_CENTER_HORIZONTAL, 5 );


    bSizerData->Add( 5, 2, 0, 0, 5 );


    bSizerData->Add( 0, 0, 1, wxEXPAND, 5 );

    m_staticTextData = new wxStaticText( m_panelStatistics, wxID_ANY, _("dummy"), wxDefaultPosition, wxDefaultSize, 0 );
    m_staticTextData->Wrap( -1 );
    m_staticTextData->SetToolTip( _("Total bytes to copy") );

    bSizerData->Add( m_staticTextData, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 5 );


    bSizerStatistics->Add( bSizerData, 0, wxALIGN_CENTER_VERTICAL|wxEXPAND, 5 );


    bSizerStatistics->Add( 5, 5, 0, 0, 5 );

    wxBoxSizer* bSizer176;
    bSizer176 = new wxBoxSizer( wxVERTICAL );

    m_bitmapDeleteRight = new wxStaticBitmap( m_panelStatistics, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, 0 );
    m_bitmapDeleteRight->SetToolTip( _("Number of files and folders that will be deleted") );

    bSizer176->Add( m_bitmapDeleteRight, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 5 );


    bSizer176->Add( 5, 2, 0, 0, 5 );


    bSizer176->Add( 0, 0, 1, wxEXPAND, 5 );

    m_staticTextDeleteRight = new wxStaticText( m_panelStatistics, wxID_ANY, _("dummy"), wxDefaultPosition, wxDefaultSize, 0 );
    m_staticTextDeleteRight->Wrap( -1 );
    m_staticTextDeleteRight->SetToolTip( _("Number of files and folders that will be deleted") );

    bSizer176->Add( m_staticTextDeleteRight, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 5 );


    bSizerStatistics->Add( bSizer176, 0, wxALIGN_CENTER_VERTICAL|wxEXPAND, 5 );


    bSizerStatistics->Add( 5, 5, 0, 0, 5 );

    wxBoxSizer* bSizer177;
    bSizer177 = new wxBoxSizer( wxVERTICAL );

    m_bitmapUpdateRight = new wxStaticBitmap( m_panelStatistics, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, 0 );
    m_bitmapUpdateRight->SetToolTip( _("Number of files that will be overwritten") );

    bSizer177->Add( m_bitmapUpdateRight, 0, wxALIGN_CENTER_VERTICAL|wxALIGN_CENTER_HORIZONTAL, 5 );


    bSizer177->Add( 5, 2, 0, 0, 5 );


    bSizer177->Add( 0, 0, 1, wxEXPAND, 5 );

    m_staticTextUpdateRight = new wxStaticText( m_panelStatistics, wxID_ANY, _("dummy"), wxDefaultPosition, wxDefaultSize, 0 );
    m_staticTextUpdateRight->Wrap( -1 );
    m_staticTextUpdateRight->SetToolTip( _("Number of files that will be overwritten") );

    bSizer177->Add( m_staticTextUpdateRight, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 5 );


    bSizerStatistics->Add( bSizer177, 0, wxALIGN_CENTER_VERTICAL|wxEXPAND, 5 );


    bSizerStatistics->Add( 5, 5, 0, 0, 5 );

    wxBoxSizer* bSizer178;
    bSizer178 = new wxBoxSizer( wxVERTICAL );

    m_bitmapCreateRight = new wxStaticBitmap( m_panelStatistics, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, 0 );
    m_bitmapCreateRight->SetToolTip( _("Number of files and folders that will be created") );

    bSizer178->Add( m_bitmapCreateRight, 0, wxALIGN_CENTER_VERTICAL|wxALIGN_CENTER_HORIZONTAL, 5 );


    bSizer178->Add( 5, 2, 0, 0, 5 );


    bSizer178->Add( 0, 0, 1, wxEXPAND, 5 );

    m_staticTextCreateRight = new wxStaticText( m_panelStatistics, wxID_ANY, _("dummy"), wxDefaultPosition, wxDefaultSize, 0 );
    m_staticTextCreateRight->Wrap( -1 );
    m_staticTextCreateRight->SetToolTip( _("Number of files and folders that will be created") );

    bSizer178->Add( m_staticTextCreateRight, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 5 );


    bSizerStatistics->Add( bSizer178, 0, wxALIGN_CENTER_VERTICAL|wxEXPAND, 5 );


    bSizer1801->Add( bSizerStatistics, 0, wxALIGN_CENTER_VERTICAL, 5 );


    bSizer1801->Add( 0, 0, 1, wxEXPAND, 5 );


    m_panelStatistics->SetSizer( bSizer1801 );
    m_panelStatistics->Layout();
    bSizer1801->Fit( m_panelStatistics );
    bSizerPanelHolder->Add( m_panelStatistics, 0, wxALIGN_CENTER_VERTICAL, 5 );

    m_panelViewFilter = new wxPanel( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
    bSizerViewFilter = new wxBoxSizer( wxHORIZONTAL );


    bSizerViewFilter->Add( 0, 0, 1, wxEXPAND, 5 );

    m_bpButtonSyncCreateLeft = new ToggleButton( m_panelViewFilter, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxSize( 40,40 ), wxBU_AUTODRAW );
    bSizerViewFilter->Add( m_bpButtonSyncCreateLeft, 0, wxALIGN_CENTER_VERTICAL|wxALIGN_CENTER_HORIZONTAL, 5 );

    m_bpButtonSyncDirOverwLeft = new ToggleButton( m_panelViewFilter, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxSize( 40,40 ), wxBU_AUTODRAW );
    bSizerViewFilter->Add( m_bpButtonSyncDirOverwLeft, 0, wxALIGN_CENTER_VERTICAL|wxALIGN_CENTER_HORIZONTAL, 5 );

    m_bpButtonSyncDeleteLeft = new ToggleButton( m_panelViewFilter, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxSize( 40,40 ), wxBU_AUTODRAW );
    bSizerViewFilter->Add( m_bpButtonSyncDeleteLeft, 0, wxALIGN_CENTER_VERTICAL|wxALIGN_CENTER_HORIZONTAL, 5 );

    m_bpButtonLeftOnly = new ToggleButton( m_panelViewFilter, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxSize( 40,40 ), wxBU_AUTODRAW );
    bSizerViewFilter->Add( m_bpButtonLeftOnly, 0, wxALIGN_CENTER_VERTICAL|wxALIGN_CENTER_HORIZONTAL, 5 );

    m_bpButtonLeftNewer = new ToggleButton( m_panelViewFilter, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxSize( 40,40 ), wxBU_AUTODRAW );
    bSizerViewFilter->Add( m_bpButtonLeftNewer, 0, wxALIGN_CENTER_VERTICAL|wxALIGN_CENTER_HORIZONTAL, 5 );

    m_bpButtonEqual = new ToggleButton( m_panelViewFilter, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxSize( 40,40 ), wxBU_AUTODRAW );
    bSizerViewFilter->Add( m_bpButtonEqual, 0, wxALIGN_CENTER_VERTICAL|wxALIGN_CENTER_HORIZONTAL, 5 );

    m_bpButtonDifferent = new ToggleButton( m_panelViewFilter, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxSize( 40,40 ), wxBU_AUTODRAW );
    bSizerViewFilter->Add( m_bpButtonDifferent, 0, wxALIGN_CENTER_VERTICAL|wxALIGN_CENTER_HORIZONTAL, 5 );

    m_bpButtonSyncDirNone = new ToggleButton( m_panelViewFilter, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxSize( 40,40 ), wxBU_AUTODRAW );
    bSizerViewFilter->Add( m_bpButtonSyncDirNone, 0, wxALIGN_CENTER_VERTICAL|wxALIGN_CENTER_HORIZONTAL, 5 );

    m_bpButtonRightNewer = new ToggleButton( m_panelViewFilter, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxSize( 40,40 ), wxBU_AUTODRAW );
    bSizerViewFilter->Add( m_bpButtonRightNewer, 0, wxALIGN_CENTER_VERTICAL|wxALIGN_CENTER_HORIZONTAL, 5 );

    m_bpButtonRightOnly = new ToggleButton( m_panelViewFilter, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxSize( 40,40 ), wxBU_AUTODRAW );
    bSizerViewFilter->Add( m_bpButtonRightOnly, 0, wxALIGN_CENTER_VERTICAL|wxALIGN_CENTER_HORIZONTAL, 5 );

    m_bpButtonSyncDeleteRight = new ToggleButton( m_panelViewFilter, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxSize( 40,40 ), wxBU_AUTODRAW );
    bSizerViewFilter->Add( m_bpButtonSyncDeleteRight, 0, wxALIGN_CENTER_VERTICAL|wxALIGN_CENTER_HORIZONTAL, 5 );

    m_bpButtonSyncDirOverwRight = new ToggleButton( m_panelViewFilter, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxSize( 40,40 ), wxBU_AUTODRAW );
    bSizerViewFilter->Add( m_bpButtonSyncDirOverwRight, 0, wxALIGN_CENTER_VERTICAL|wxALIGN_CENTER_HORIZONTAL, 5 );

    m_bpButtonSyncCreateRight = new ToggleButton( m_panelViewFilter, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxSize( 40,40 ), wxBU_AUTODRAW );
    bSizerViewFilter->Add( m_bpButtonSyncCreateRight, 0, wxALIGN_CENTER_VERTICAL|wxALIGN_CENTER_HORIZONTAL, 5 );

    m_bpButtonConflict = new ToggleButton( m_panelViewFilter, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxSize( 40,40 ), wxBU_AUTODRAW );
    bSizerViewFilter->Add( m_bpButtonConflict, 0, wxALIGN_CENTER_VERTICAL|wxALIGN_CENTER_HORIZONTAL, 5 );


    bSizerViewFilter->Add( 0, 0, 1, wxEXPAND, 5 );


    m_panelViewFilter->SetSizer( bSizerViewFilter );
    m_panelViewFilter->Layout();
    bSizerViewFilter->Fit( m_panelViewFilter );
    bSizerPanelHolder->Add( m_panelViewFilter, 0, wxALIGN_CENTER_VERTICAL, 5 );


    this->SetSizer( bSizerPanelHolder );
    this->Layout();

    // Connect Events
    this->Connect( wxEVT_CLOSE_WINDOW, wxCloseEventHandler( MainDialogGenerated::OnClose ) );
    this->Connect( m_menuItem10->GetId(), wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( MainDialogGenerated::OnCompare ) );
    this->Connect( m_menuItem11->GetId(), wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( MainDialogGenerated::OnStartSync ) );
    this->Connect( m_menuItemNew->GetId(), wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( MainDialogGenerated::OnConfigNew ) );
    this->Connect( m_menuItemLoad->GetId(), wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( MainDialogGenerated::OnConfigLoad ) );
    this->Connect( m_menuItemSave->GetId(), wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( MainDialogGenerated::OnConfigSave ) );
    this->Connect( m_menuItemSaveAs->GetId(), wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( MainDialogGenerated::OnConfigSaveAs ) );
    this->Connect( m_menuItem4->GetId(), wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( MainDialogGenerated::OnMenuQuit ) );
    this->Connect( m_menuItemGlobSett->GetId(), wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( MainDialogGenerated::OnMenuGlobalSettings ) );
    this->Connect( m_menuItem7->GetId(), wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( MainDialogGenerated::OnMenuBatchJob ) );
    this->Connect( m_menuItem5->GetId(), wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( MainDialogGenerated::OnMenuExportFileList ) );
    this->Connect( m_menuItemManual->GetId(), wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( MainDialogGenerated::OnShowHelp ) );
    this->Connect( m_menuItemCheckVer->GetId(), wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( MainDialogGenerated::OnMenuCheckVersion ) );
    this->Connect( m_menuItemAbout->GetId(), wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( MainDialogGenerated::OnMenuAbout ) );
    m_buttonCompare->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( MainDialogGenerated::OnCompare ), NULL, this );
    m_bpButtonCmpConfig->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( MainDialogGenerated::OnCmpSettings ), NULL, this );
    m_bpButtonSyncConfig->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( MainDialogGenerated::OnSyncSettings ), NULL, this );
    m_buttonStartSync->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( MainDialogGenerated::OnStartSync ), NULL, this );
    m_bpButtonAddPair->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( MainDialogGenerated::OnAddFolderPair ), NULL, this );
    m_bpButtonRemovePair->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( MainDialogGenerated::OnRemoveTopFolderPair ), NULL, this );
    m_dirPickerLeft->Connect( wxEVT_COMMAND_DIRPICKER_CHANGED, wxFileDirPickerEventHandler( MainDialogGenerated::OnDirSelected ), NULL, this );
    m_bpButtonSwapSides->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( MainDialogGenerated::OnSwapSides ), NULL, this );
    m_dirPickerRight->Connect( wxEVT_COMMAND_DIRPICKER_CHANGED, wxFileDirPickerEventHandler( MainDialogGenerated::OnDirSelected ), NULL, this );
    m_bpButtonLoad->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( MainDialogGenerated::OnConfigLoad ), NULL, this );
    m_bpButtonSave->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( MainDialogGenerated::OnConfigSaveAs ), NULL, this );
    m_listBoxHistory->Connect( wxEVT_CHAR, wxKeyEventHandler( MainDialogGenerated::OnCfgHistoryKeyEvent ), NULL, this );
    m_listBoxHistory->Connect( wxEVT_COMMAND_LISTBOX_SELECTED, wxCommandEventHandler( MainDialogGenerated::OnLoadFromHistory ), NULL, this );
    m_bpButtonFilter->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( MainDialogGenerated::OnConfigureFilter ), NULL, this );
    m_checkBoxHideFilt->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( MainDialogGenerated::OnHideFilteredButton ), NULL, this );
    m_bpButtonSyncCreateLeft->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( MainDialogGenerated::OnSyncCreateLeft ), NULL, this );
    m_bpButtonSyncDirOverwLeft->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( MainDialogGenerated::OnSyncDirLeft ), NULL, this );
    m_bpButtonSyncDeleteLeft->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( MainDialogGenerated::OnSyncDeleteLeft ), NULL, this );
    m_bpButtonLeftOnly->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( MainDialogGenerated::OnLeftOnlyFiles ), NULL, this );
    m_bpButtonLeftNewer->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( MainDialogGenerated::OnLeftNewerFiles ), NULL, this );
    m_bpButtonEqual->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( MainDialogGenerated::OnEqualFiles ), NULL, this );
    m_bpButtonDifferent->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( MainDialogGenerated::OnDifferentFiles ), NULL, this );
    m_bpButtonSyncDirNone->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( MainDialogGenerated::OnSyncDirNone ), NULL, this );
    m_bpButtonRightNewer->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( MainDialogGenerated::OnRightNewerFiles ), NULL, this );
    m_bpButtonRightOnly->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( MainDialogGenerated::OnRightOnlyFiles ), NULL, this );
    m_bpButtonSyncDeleteRight->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( MainDialogGenerated::OnSyncDeleteRight ), NULL, this );
    m_bpButtonSyncDirOverwRight->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( MainDialogGenerated::OnSyncDirRight ), NULL, this );
    m_bpButtonSyncCreateRight->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( MainDialogGenerated::OnSyncCreateRight ), NULL, this );
    m_bpButtonConflict->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( MainDialogGenerated::OnConflictFiles ), NULL, this );
}

MainDialogGenerated::~MainDialogGenerated()
{
    // Disconnect Events
    this->Disconnect( wxEVT_CLOSE_WINDOW, wxCloseEventHandler( MainDialogGenerated::OnClose ) );
    this->Disconnect( wxID_ANY, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( MainDialogGenerated::OnCompare ) );
    this->Disconnect( wxID_ANY, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( MainDialogGenerated::OnStartSync ) );
    this->Disconnect( wxID_NEW, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( MainDialogGenerated::OnConfigNew ) );
    this->Disconnect( wxID_OPEN, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( MainDialogGenerated::OnConfigLoad ) );
    this->Disconnect( wxID_SAVE, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( MainDialogGenerated::OnConfigSave ) );
    this->Disconnect( wxID_SAVEAS, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( MainDialogGenerated::OnConfigSaveAs ) );
    this->Disconnect( wxID_EXIT, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( MainDialogGenerated::OnMenuQuit ) );
    this->Disconnect( wxID_PREFERENCES, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( MainDialogGenerated::OnMenuGlobalSettings ) );
    this->Disconnect( wxID_ANY, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( MainDialogGenerated::OnMenuBatchJob ) );
    this->Disconnect( wxID_ANY, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( MainDialogGenerated::OnMenuExportFileList ) );
    this->Disconnect( wxID_HELP, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( MainDialogGenerated::OnShowHelp ) );
    this->Disconnect( wxID_ANY, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( MainDialogGenerated::OnMenuCheckVersion ) );
    this->Disconnect( wxID_ABOUT, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( MainDialogGenerated::OnMenuAbout ) );
    m_buttonCompare->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( MainDialogGenerated::OnCompare ), NULL, this );
    m_bpButtonCmpConfig->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( MainDialogGenerated::OnCmpSettings ), NULL, this );
    m_bpButtonSyncConfig->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( MainDialogGenerated::OnSyncSettings ), NULL, this );
    m_buttonStartSync->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( MainDialogGenerated::OnStartSync ), NULL, this );
    m_bpButtonAddPair->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( MainDialogGenerated::OnAddFolderPair ), NULL, this );
    m_bpButtonRemovePair->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( MainDialogGenerated::OnRemoveTopFolderPair ), NULL, this );
    m_dirPickerLeft->Disconnect( wxEVT_COMMAND_DIRPICKER_CHANGED, wxFileDirPickerEventHandler( MainDialogGenerated::OnDirSelected ), NULL, this );
    m_bpButtonSwapSides->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( MainDialogGenerated::OnSwapSides ), NULL, this );
    m_dirPickerRight->Disconnect( wxEVT_COMMAND_DIRPICKER_CHANGED, wxFileDirPickerEventHandler( MainDialogGenerated::OnDirSelected ), NULL, this );
    m_bpButtonLoad->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( MainDialogGenerated::OnConfigLoad ), NULL, this );
    m_bpButtonSave->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( MainDialogGenerated::OnConfigSaveAs ), NULL, this );
    m_listBoxHistory->Disconnect( wxEVT_CHAR, wxKeyEventHandler( MainDialogGenerated::OnCfgHistoryKeyEvent ), NULL, this );
    m_listBoxHistory->Disconnect( wxEVT_COMMAND_LISTBOX_SELECTED, wxCommandEventHandler( MainDialogGenerated::OnLoadFromHistory ), NULL, this );
    m_bpButtonFilter->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( MainDialogGenerated::OnConfigureFilter ), NULL, this );
    m_checkBoxHideFilt->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( MainDialogGenerated::OnHideFilteredButton ), NULL, this );
    m_bpButtonSyncCreateLeft->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( MainDialogGenerated::OnSyncCreateLeft ), NULL, this );
    m_bpButtonSyncDirOverwLeft->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( MainDialogGenerated::OnSyncDirLeft ), NULL, this );
    m_bpButtonSyncDeleteLeft->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( MainDialogGenerated::OnSyncDeleteLeft ), NULL, this );
    m_bpButtonLeftOnly->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( MainDialogGenerated::OnLeftOnlyFiles ), NULL, this );
    m_bpButtonLeftNewer->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( MainDialogGenerated::OnLeftNewerFiles ), NULL, this );
    m_bpButtonEqual->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( MainDialogGenerated::OnEqualFiles ), NULL, this );
    m_bpButtonDifferent->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( MainDialogGenerated::OnDifferentFiles ), NULL, this );
    m_bpButtonSyncDirNone->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( MainDialogGenerated::OnSyncDirNone ), NULL, this );
    m_bpButtonRightNewer->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( MainDialogGenerated::OnRightNewerFiles ), NULL, this );
    m_bpButtonRightOnly->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( MainDialogGenerated::OnRightOnlyFiles ), NULL, this );
    m_bpButtonSyncDeleteRight->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( MainDialogGenerated::OnSyncDeleteRight ), NULL, this );
    m_bpButtonSyncDirOverwRight->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( MainDialogGenerated::OnSyncDirRight ), NULL, this );
    m_bpButtonSyncCreateRight->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( MainDialogGenerated::OnSyncCreateRight ), NULL, this );
    m_bpButtonConflict->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( MainDialogGenerated::OnConflictFiles ), NULL, this );

}

FolderPairGenerated::FolderPairGenerated( wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style ) : wxPanel( parent, id, pos, size, style )
{
    wxBoxSizer* bSizer74;
    bSizer74 = new wxBoxSizer( wxHORIZONTAL );

    m_panelLeft = new wxPanel( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
    m_panelLeft->SetMinSize( wxSize( 1,-1 ) );

    wxBoxSizer* bSizer134;
    bSizer134 = new wxBoxSizer( wxHORIZONTAL );

    m_bpButtonRemovePair = new wxBitmapButton( m_panelLeft, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxSize( 19,21 ), wxBU_AUTODRAW );
    m_bpButtonRemovePair->SetToolTip( _("Remove folder pair") );

    bSizer134->Add( m_bpButtonRemovePair, 0, wxALIGN_CENTER_VERTICAL, 5 );

    m_directoryLeft = new FolderHistoryBox( m_panelLeft, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0, NULL, 0 );
    bSizer134->Add( m_directoryLeft, 1, wxALIGN_CENTER_VERTICAL, 5 );

    m_dirPickerLeft = new zen::DirPickerCtrl( m_panelLeft, wxID_ANY, wxEmptyString, _("Select a folder"), wxDefaultPosition, wxDefaultSize, 0 );
    m_dirPickerLeft->SetToolTip( _("Select a folder") );

    bSizer134->Add( m_dirPickerLeft, 0, wxALIGN_CENTER_VERTICAL, 5 );


    m_panelLeft->SetSizer( bSizer134 );
    m_panelLeft->Layout();
    bSizer134->Fit( m_panelLeft );
    bSizer74->Add( m_panelLeft, 0, wxALIGN_CENTER_VERTICAL|wxLEFT|wxEXPAND, 5 );

    m_panel20 = new wxPanel( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
    wxBoxSizer* bSizer95;
    bSizer95 = new wxBoxSizer( wxHORIZONTAL );


    bSizer95->Add( 0, 0, 1, wxEXPAND, 5 );

    m_bpButtonAltCompCfg = new wxBitmapButton( m_panel20, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxSize( 20,20 ), wxBU_AUTODRAW );
    bSizer95->Add( m_bpButtonAltCompCfg, 0, wxALIGN_CENTER_VERTICAL, 5 );

    m_bpButtonLocalFilter = new wxBitmapButton( m_panel20, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxSize( 20,20 ), wxBU_AUTODRAW );
    bSizer95->Add( m_bpButtonLocalFilter, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 3 );

    m_bpButtonAltSyncCfg = new wxBitmapButton( m_panel20, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxSize( 20,20 ), wxBU_AUTODRAW );
    bSizer95->Add( m_bpButtonAltSyncCfg, 0, wxALIGN_CENTER_VERTICAL, 5 );


    bSizer95->Add( 0, 0, 1, wxEXPAND, 5 );


    m_panel20->SetSizer( bSizer95 );
    m_panel20->Layout();
    bSizer95->Fit( m_panel20 );
    bSizer74->Add( m_panel20, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT|wxEXPAND, 5 );

    m_panelRight = new wxPanel( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
    m_panelRight->SetMinSize( wxSize( 1,-1 ) );

    wxBoxSizer* bSizer135;
    bSizer135 = new wxBoxSizer( wxHORIZONTAL );

    m_directoryRight = new FolderHistoryBox( m_panelRight, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0, NULL, 0 );
    bSizer135->Add( m_directoryRight, 1, wxALIGN_CENTER_VERTICAL, 5 );

    m_dirPickerRight = new zen::DirPickerCtrl( m_panelRight, wxID_ANY, wxEmptyString, _("Select a folder"), wxDefaultPosition, wxDefaultSize, 0 );
    m_dirPickerRight->SetToolTip( _("Select a folder") );

    bSizer135->Add( m_dirPickerRight, 0, wxALIGN_CENTER_VERTICAL, 5 );


    m_panelRight->SetSizer( bSizer135 );
    m_panelRight->Layout();
    bSizer135->Fit( m_panelRight );
    bSizer74->Add( m_panelRight, 1, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxEXPAND, 5 );


    this->SetSizer( bSizer74 );
    this->Layout();
    bSizer74->Fit( this );
}

FolderPairGenerated::~FolderPairGenerated()
{
}

CompareStatusGenerated::CompareStatusGenerated( wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style ) : wxPanel( parent, id, pos, size, style )
{
    this->SetBackgroundColour( wxSystemSettings::GetColour( wxSYS_COLOUR_WINDOW ) );

    wxBoxSizer* bSizer40;
    bSizer40 = new wxBoxSizer( wxVERTICAL );


    bSizer40->Add( 0, 0, 1, wxEXPAND, 5 );

    wxBoxSizer* bSizer182;
    bSizer182 = new wxBoxSizer( wxVERTICAL );

    m_textCtrlStatus = new wxTextCtrl( this, wxID_ANY, _("dummy"), wxDefaultPosition, wxDefaultSize, wxTE_READONLY );
    m_textCtrlStatus->SetBackgroundColour( wxSystemSettings::GetColour( wxSYS_COLOUR_BTNFACE ) );

    bSizer182->Add( m_textCtrlStatus, 0, wxEXPAND, 5 );


    bSizer40->Add( bSizer182, 0, wxEXPAND|wxRIGHT|wxLEFT, 5 );

    m_gauge2 = new wxGauge( this, wxID_ANY, 100, wxDefaultPosition, wxSize( -1,14 ), wxGA_HORIZONTAL|wxGA_SMOOTH );
    bSizer40->Add( m_gauge2, 0, wxEXPAND|wxTOP|wxRIGHT|wxLEFT, 5 );

    bSizer42 = new wxBoxSizer( wxHORIZONTAL );

    wxBoxSizer* bSizer157;
    bSizer157 = new wxBoxSizer( wxVERTICAL );

    bSizerFilesFound = new wxBoxSizer( wxHORIZONTAL );

    m_staticText321 = new wxStaticText( this, wxID_ANY, _("Items found:"), wxDefaultPosition, wxDefaultSize, 0 );
    m_staticText321->Wrap( -1 );
    m_staticText321->SetFont( wxFont( 10, 70, 90, 90, false, wxEmptyString ) );

    bSizerFilesFound->Add( m_staticText321, 0, wxALIGN_BOTTOM, 5 );

    m_staticTextScanned = new wxStaticText( this, wxID_ANY, _("dummy"), wxDefaultPosition, wxDefaultSize, 0 );
    m_staticTextScanned->Wrap( -1 );
    m_staticTextScanned->SetFont( wxFont( 9, 70, 90, 92, false, wxEmptyString ) );

    bSizerFilesFound->Add( m_staticTextScanned, 0, wxALIGN_BOTTOM|wxLEFT, 5 );


    bSizer157->Add( bSizerFilesFound, 0, 0, 5 );

    bSizerFilesRemaining = new wxBoxSizer( wxHORIZONTAL );

    m_staticText46 = new wxStaticText( this, wxID_ANY, _("Items remaining:"), wxDefaultPosition, wxDefaultSize, 0 );
    m_staticText46->Wrap( -1 );
    m_staticText46->SetFont( wxFont( 10, 70, 90, 90, false, wxEmptyString ) );

    bSizerFilesRemaining->Add( m_staticText46, 0, wxALIGN_BOTTOM, 5 );

    wxBoxSizer* bSizer154;
    bSizer154 = new wxBoxSizer( wxHORIZONTAL );

    m_staticTextFilesRemaining = new wxStaticText( this, wxID_ANY, _("dummy"), wxDefaultPosition, wxDefaultSize, 0 );
    m_staticTextFilesRemaining->Wrap( -1 );
    m_staticTextFilesRemaining->SetFont( wxFont( 9, 70, 90, 92, false, wxEmptyString ) );

    bSizer154->Add( m_staticTextFilesRemaining, 0, wxALIGN_BOTTOM, 5 );

    m_staticTextDataRemaining = new wxStaticText( this, wxID_ANY, _("dummy"), wxDefaultPosition, wxDefaultSize, 0 );
    m_staticTextDataRemaining->Wrap( -1 );
    m_staticTextDataRemaining->SetFont( wxFont( 9, 70, 90, 90, false, wxEmptyString ) );

    bSizer154->Add( m_staticTextDataRemaining, 0, wxALIGN_BOTTOM|wxLEFT, 5 );


    bSizerFilesRemaining->Add( bSizer154, 0, wxALIGN_BOTTOM|wxLEFT, 5 );


    bSizer157->Add( bSizerFilesRemaining, 0, 0, 5 );


    bSizer42->Add( bSizer157, 0, wxALIGN_CENTER_VERTICAL, 5 );


    bSizer42->Add( 0, 0, 1, wxEXPAND, 5 );

    sSizerSpeed = new wxBoxSizer( wxHORIZONTAL );

    m_staticText104 = new wxStaticText( this, wxID_ANY, _("Speed:"), wxDefaultPosition, wxDefaultSize, 0 );
    m_staticText104->Wrap( -1 );
    m_staticText104->SetFont( wxFont( 10, 70, 90, 90, false, wxEmptyString ) );

    sSizerSpeed->Add( m_staticText104, 0, wxALIGN_BOTTOM, 5 );

    m_staticTextSpeed = new wxStaticText( this, wxID_ANY, _("dummy"), wxDefaultPosition, wxDefaultSize, 0 );
    m_staticTextSpeed->Wrap( -1 );
    m_staticTextSpeed->SetFont( wxFont( 9, 70, 90, 92, false, wxEmptyString ) );

    sSizerSpeed->Add( m_staticTextSpeed, 0, wxLEFT|wxALIGN_BOTTOM, 5 );


    bSizer42->Add( sSizerSpeed, 0, wxALIGN_CENTER_VERTICAL, 5 );


    bSizer42->Add( 10, 0, 0, wxALIGN_CENTER_VERTICAL, 5 );

    sSizerTimeRemaining = new wxBoxSizer( wxHORIZONTAL );

    m_staticTextTimeRemFixed = new wxStaticText( this, wxID_ANY, _("Time remaining:"), wxDefaultPosition, wxDefaultSize, 0 );
    m_staticTextTimeRemFixed->Wrap( -1 );
    m_staticTextTimeRemFixed->SetFont( wxFont( 10, 70, 90, 90, false, wxEmptyString ) );

    sSizerTimeRemaining->Add( m_staticTextTimeRemFixed, 0, wxALIGN_BOTTOM, 5 );

    m_staticTextRemTime = new wxStaticText( this, wxID_ANY, _("dummy"), wxDefaultPosition, wxDefaultSize, 0 );
    m_staticTextRemTime->Wrap( -1 );
    m_staticTextRemTime->SetFont( wxFont( 9, 70, 90, 92, false, wxEmptyString ) );

    sSizerTimeRemaining->Add( m_staticTextRemTime, 0, wxLEFT|wxALIGN_BOTTOM, 5 );


    bSizer42->Add( sSizerTimeRemaining, 0, wxALIGN_CENTER_VERTICAL, 5 );


    bSizer42->Add( 0, 0, 1, wxEXPAND, 5 );

    sSizerTimeElapsed = new wxBoxSizer( wxHORIZONTAL );

    wxStaticText* m_staticText37;
    m_staticText37 = new wxStaticText( this, wxID_ANY, _("Time elapsed:"), wxDefaultPosition, wxDefaultSize, 0 );
    m_staticText37->Wrap( -1 );
    m_staticText37->SetFont( wxFont( 10, 70, 90, 90, false, wxEmptyString ) );

    sSizerTimeElapsed->Add( m_staticText37, 0, wxALIGN_BOTTOM, 5 );

    m_staticTextTimeElapsed = new wxStaticText( this, wxID_ANY, _("dummy"), wxDefaultPosition, wxDefaultSize, 0 );
    m_staticTextTimeElapsed->Wrap( -1 );
    m_staticTextTimeElapsed->SetFont( wxFont( 9, 70, 90, 92, false, wxEmptyString ) );

    sSizerTimeElapsed->Add( m_staticTextTimeElapsed, 0, wxLEFT|wxALIGN_BOTTOM, 5 );


    bSizer42->Add( sSizerTimeElapsed, 0, wxALIGN_CENTER_VERTICAL, 5 );


    bSizer40->Add( bSizer42, 0, wxALIGN_CENTER_HORIZONTAL|wxEXPAND|wxTOP|wxRIGHT|wxLEFT, 5 );


    bSizer40->Add( 0, 0, 1, wxEXPAND, 5 );


    this->SetSizer( bSizer40 );
    this->Layout();
    bSizer40->Fit( this );
}

CompareStatusGenerated::~CompareStatusGenerated()
{
}

BatchDlgGenerated::BatchDlgGenerated( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : wxDialog( parent, id, title, pos, size, style )
{
    this->SetSizeHints( wxSize( 560,300 ), wxDefaultSize );

    wxBoxSizer* bSizer54;
    bSizer54 = new wxBoxSizer( wxVERTICAL );

    wxBoxSizer* bSizer87;
    bSizer87 = new wxBoxSizer( wxHORIZONTAL );

    m_bitmap27 = new wxStaticBitmap( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxSize( 40,40 ), 0 );
    bSizer87->Add( m_bitmap27, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );

    m_panel8 = new wxPanel( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxSIMPLE_BORDER|wxTAB_TRAVERSAL );
    m_panel8->SetBackgroundColour( wxSystemSettings::GetColour( wxSYS_COLOUR_3DLIGHT ) );

    wxBoxSizer* bSizer72;
    bSizer72 = new wxBoxSizer( wxVERTICAL );

    m_staticText56 = new wxStaticText( m_panel8, wxID_ANY, _("Batch job"), wxDefaultPosition, wxDefaultSize, 0 );
    m_staticText56->Wrap( -1 );
    m_staticText56->SetFont( wxFont( 14, 70, 90, 92, false, wxEmptyString ) );
    m_staticText56->SetForegroundColour( wxColour( 0, 0, 0 ) );

    bSizer72->Add( m_staticText56, 0, wxALL|wxALIGN_CENTER_HORIZONTAL, 5 );


    m_panel8->SetSizer( bSizer72 );
    m_panel8->Layout();
    bSizer72->Fit( m_panel8 );
    bSizer87->Add( m_panel8, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );

    m_staticText44 = new wxStaticText( this, wxID_ANY, _("Create a batch file to automate synchronization. Double-click this file or schedule in your system's task planner: FreeFileSync.exe <job name>.ffs_batch"), wxDefaultPosition, wxDefaultSize, 0 );
    m_staticText44->Wrap( 480 );
    bSizer87->Add( m_staticText44, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );

    m_bpButtonHelp = new wxBitmapButton( this, wxID_HELP, wxNullBitmap, wxDefaultPosition, wxSize( 40,40 ), wxBU_AUTODRAW );
    m_bpButtonHelp->SetToolTip( _("Help") );

    bSizer87->Add( m_bpButtonHelp, 0, wxALIGN_CENTER_VERTICAL, 5 );


    bSizer54->Add( bSizer87, 0, wxALIGN_CENTER_HORIZONTAL|wxTOP|wxRIGHT|wxLEFT, 5 );

    m_notebook1 = new wxNotebook( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0 );
    m_panelOverview = new wxPanel( m_notebook1, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
    wxBoxSizer* bSizer67;
    bSizer67 = new wxBoxSizer( wxHORIZONTAL );

    wxBoxSizer* bSizer120;
    bSizer120 = new wxBoxSizer( wxVERTICAL );

    wxBoxSizer* bSizer175;
    bSizer175 = new wxBoxSizer( wxHORIZONTAL );

    wxBoxSizer* bSizer1701;
    bSizer1701 = new wxBoxSizer( wxVERTICAL );

    wxStaticBoxSizer* sbSizer241;
    sbSizer241 = new wxStaticBoxSizer( new wxStaticBox( m_panelOverview, wxID_ANY, _("Compare") ), wxHORIZONTAL );

    m_bpButtonCmpConfig = new wxBitmapButton( m_panelOverview, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxSize( -1,42 ), wxBU_AUTODRAW );
    m_bpButtonCmpConfig->SetToolTip( _("Comparison settings") );

    sbSizer241->Add( m_bpButtonCmpConfig, 0, wxALIGN_CENTER_VERTICAL, 3 );


    sbSizer241->Add( 10, 0, 0, 0, 5 );

    m_staticTextCmpVariant = new wxStaticText( m_panelOverview, wxID_ANY, _("dummy"), wxDefaultPosition, wxDefaultSize, 0 );
    m_staticTextCmpVariant->Wrap( -1 );
    m_staticTextCmpVariant->SetFont( wxFont( 8, 70, 90, 92, false, wxEmptyString ) );
    m_staticTextCmpVariant->SetForegroundColour( wxSystemSettings::GetColour( wxSYS_COLOUR_GRAYTEXT ) );

    sbSizer241->Add( m_staticTextCmpVariant, 1, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 5 );


    bSizer1701->Add( sbSizer241, 0, wxALIGN_CENTER_VERTICAL, 5 );


    bSizer175->Add( bSizer1701, 1, wxALIGN_CENTER_VERTICAL, 5 );

    wxStaticBoxSizer* sbSizer26;
    sbSizer26 = new wxStaticBoxSizer( new wxStaticBox( m_panelOverview, wxID_ANY, _("Filter files") ), wxHORIZONTAL );


    sbSizer26->Add( 20, 0, 0, 0, 5 );

    m_bpButtonFilter = new wxBitmapButton( m_panelOverview, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxSize( 40,40 ), wxBU_AUTODRAW|wxFULL_REPAINT_ON_RESIZE );
    sbSizer26->Add( m_bpButtonFilter, 0, wxALIGN_CENTER_VERTICAL|wxALIGN_CENTER_HORIZONTAL|wxRIGHT|wxLEFT, 15 );


    sbSizer26->Add( 20, 0, 0, 0, 5 );


    bSizer175->Add( sbSizer26, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );

    wxBoxSizer* bSizer171;
    bSizer171 = new wxBoxSizer( wxVERTICAL );

    wxStaticBoxSizer* sbSizer252;
    sbSizer252 = new wxStaticBoxSizer( new wxStaticBox( m_panelOverview, wxID_ANY, _("Synchronize") ), wxHORIZONTAL );

    m_staticTextSyncVariant = new wxStaticText( m_panelOverview, wxID_ANY, _("dummy"), wxDefaultPosition, wxDefaultSize, 0 );
    m_staticTextSyncVariant->Wrap( -1 );
    m_staticTextSyncVariant->SetFont( wxFont( 8, 70, 90, 92, false, wxEmptyString ) );
    m_staticTextSyncVariant->SetForegroundColour( wxSystemSettings::GetColour( wxSYS_COLOUR_GRAYTEXT ) );

    sbSizer252->Add( m_staticTextSyncVariant, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 5 );


    sbSizer252->Add( 10, 0, 0, 0, 5 );

    m_bpButtonSyncConfig = new wxBitmapButton( m_panelOverview, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxSize( -1,42 ), wxBU_AUTODRAW );
    m_bpButtonSyncConfig->SetToolTip( _("Synchronization settings") );

    sbSizer252->Add( m_bpButtonSyncConfig, 0, wxALIGN_CENTER_VERTICAL, 3 );


    bSizer171->Add( sbSizer252, 0, wxALIGN_CENTER_VERTICAL|wxALIGN_RIGHT, 5 );


    bSizer175->Add( bSizer171, 1, wxALIGN_CENTER_VERTICAL, 5 );


    bSizer120->Add( bSizer175, 0, wxEXPAND, 5 );


    bSizer120->Add( 0, 5, 0, 0, 5 );

    m_scrolledWindow6 = new wxScrolledWindow( m_panelOverview, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxHSCROLL|wxVSCROLL );
    m_scrolledWindow6->SetScrollRate( 5, 5 );
    wxBoxSizer* bSizer141;
    bSizer141 = new wxBoxSizer( wxVERTICAL );

    sbSizerMainPair = new wxBoxSizer( wxHORIZONTAL );

    m_panelMainPair = new wxPanel( m_scrolledWindow6, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxDOUBLE_BORDER );
    wxBoxSizer* bSizer147;
    bSizer147 = new wxBoxSizer( wxHORIZONTAL );

    wxBoxSizer* bSizer1361;
    bSizer1361 = new wxBoxSizer( wxHORIZONTAL );

    m_bpButtonAddPair = new wxBitmapButton( m_panelMainPair, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxSize( 19,21 ), wxBU_AUTODRAW );
    m_bpButtonAddPair->SetToolTip( _("Add folder pair") );

    bSizer1361->Add( m_bpButtonAddPair, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 3 );

    m_bpButtonRemovePair = new wxBitmapButton( m_panelMainPair, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxSize( 19,21 ), wxBU_AUTODRAW );
    m_bpButtonRemovePair->SetToolTip( _("Remove folder pair") );

    bSizer1361->Add( m_bpButtonRemovePair, 0, wxALIGN_CENTER_HORIZONTAL, 5 );


    bSizer147->Add( bSizer1361, 0, wxALIGN_CENTER_VERTICAL, 5 );

    wxBoxSizer* bSizer143;
    bSizer143 = new wxBoxSizer( wxVERTICAL );

    wxBoxSizer* bSizer145;
    bSizer145 = new wxBoxSizer( wxHORIZONTAL );

    m_staticText532 = new wxStaticText( m_panelMainPair, wxID_ANY, _("Left"), wxDefaultPosition, wxDefaultSize, 0 );
    m_staticText532->Wrap( -1 );
    m_staticText532->SetFont( wxFont( 8, 70, 90, 92, false, wxEmptyString ) );

    bSizer145->Add( m_staticText532, 0, wxALIGN_CENTER_VERTICAL, 5 );


    bSizer143->Add( bSizer145, 1, 0, 5 );

    wxBoxSizer* bSizer146;
    bSizer146 = new wxBoxSizer( wxHORIZONTAL );

    m_staticText5411 = new wxStaticText( m_panelMainPair, wxID_ANY, _("Right"), wxDefaultPosition, wxDefaultSize, 0 );
    m_staticText5411->Wrap( -1 );
    m_staticText5411->SetFont( wxFont( 8, 70, 90, 92, false, wxEmptyString ) );

    bSizer146->Add( m_staticText5411, 0, wxALIGN_CENTER_VERTICAL, 5 );


    bSizer143->Add( bSizer146, 1, 0, 5 );


    bSizer147->Add( bSizer143, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT|wxEXPAND, 5 );


    m_panelMainPair->SetSizer( bSizer147 );
    m_panelMainPair->Layout();
    bSizer147->Fit( m_panelMainPair );
    sbSizerMainPair->Add( m_panelMainPair, 0, wxALIGN_CENTER_VERTICAL|wxEXPAND|wxBOTTOM, 5 );

    wxBoxSizer* bSizer158;
    bSizer158 = new wxBoxSizer( wxVERTICAL );

    m_panelLeft = new wxPanel( m_scrolledWindow6, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
    wxBoxSizer* bSizer1141;
    bSizer1141 = new wxBoxSizer( wxHORIZONTAL );

    m_directoryLeft = new FolderHistoryBox( m_panelLeft, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0, NULL, 0 );
    bSizer1141->Add( m_directoryLeft, 1, wxALIGN_CENTER_VERTICAL, 5 );

    m_dirPickerLeft = new zen::DirPickerCtrl( m_panelLeft, wxID_ANY, wxEmptyString, _("Select a folder"), wxDefaultPosition, wxDefaultSize, 0 );
    m_dirPickerLeft->SetToolTip( _("Select a folder") );

    bSizer1141->Add( m_dirPickerLeft, 0, wxALIGN_CENTER_VERTICAL, 5 );


    m_panelLeft->SetSizer( bSizer1141 );
    m_panelLeft->Layout();
    bSizer1141->Fit( m_panelLeft );
    bSizer158->Add( m_panelLeft, 0, wxALIGN_CENTER_VERTICAL|wxEXPAND, 5 );

    m_panelRight = new wxPanel( m_scrolledWindow6, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
    wxBoxSizer* bSizer115;
    bSizer115 = new wxBoxSizer( wxHORIZONTAL );

    m_directoryRight = new FolderHistoryBox( m_panelRight, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0, NULL, 0 );
    bSizer115->Add( m_directoryRight, 1, wxALIGN_CENTER_VERTICAL, 5 );

    m_dirPickerRight = new zen::DirPickerCtrl( m_panelRight, wxID_ANY, wxEmptyString, _("Select a folder"), wxDefaultPosition, wxDefaultSize, 0 );
    m_dirPickerRight->SetToolTip( _("Select a folder") );

    bSizer115->Add( m_dirPickerRight, 0, wxALIGN_CENTER_VERTICAL, 5 );


    m_panelRight->SetSizer( bSizer115 );
    m_panelRight->Layout();
    bSizer115->Fit( m_panelRight );
    bSizer158->Add( m_panelRight, 0, wxEXPAND|wxALIGN_CENTER_VERTICAL, 5 );


    sbSizerMainPair->Add( bSizer158, 1, wxALIGN_CENTER_VERTICAL|wxBOTTOM, 5 );

    wxBoxSizer* bSizer177;
    bSizer177 = new wxBoxSizer( wxHORIZONTAL );

    m_bpButtonAltCompCfg = new wxBitmapButton( m_scrolledWindow6, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxSize( 20,20 ), wxBU_AUTODRAW );
    bSizer177->Add( m_bpButtonAltCompCfg, 0, wxALIGN_CENTER_VERTICAL, 5 );

    m_bpButtonLocalFilter = new wxBitmapButton( m_scrolledWindow6, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxSize( 20,20 ), wxBU_AUTODRAW );
    bSizer177->Add( m_bpButtonLocalFilter, 0, wxALIGN_CENTER_VERTICAL|wxALIGN_CENTER_HORIZONTAL|wxRIGHT|wxLEFT, 3 );

    m_bpButtonAltSyncCfg = new wxBitmapButton( m_scrolledWindow6, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxSize( 20,20 ), wxBU_AUTODRAW );
    bSizer177->Add( m_bpButtonAltSyncCfg, 0, wxALIGN_CENTER_VERTICAL, 5 );


    sbSizerMainPair->Add( bSizer177, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );


    bSizer141->Add( sbSizerMainPair, 0, wxEXPAND, 5 );

    bSizerAddFolderPairs = new wxBoxSizer( wxVERTICAL );


    bSizer141->Add( bSizerAddFolderPairs, 1, wxEXPAND, 5 );


    m_scrolledWindow6->SetSizer( bSizer141 );
    m_scrolledWindow6->Layout();
    bSizer141->Fit( m_scrolledWindow6 );
    bSizer120->Add( m_scrolledWindow6, 1, wxEXPAND, 5 );


    bSizer120->Add( 0, 5, 0, wxALIGN_CENTER_HORIZONTAL, 5 );


    bSizer67->Add( bSizer120, 1, wxALIGN_CENTER_VERTICAL|wxALL|wxEXPAND, 10 );


    m_panelOverview->SetSizer( bSizer67 );
    m_panelOverview->Layout();
    bSizer67->Fit( m_panelOverview );
    m_notebook1->AddPage( m_panelOverview, _("Synchronization settings"), true );
    m_panelBatchSettings = new wxPanel( m_notebook1, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
    wxBoxSizer* bSizer117;
    bSizer117 = new wxBoxSizer( wxVERTICAL );

    wxBoxSizer* bSizer172;
    bSizer172 = new wxBoxSizer( wxVERTICAL );

    wxBoxSizer* bSizer1722;
    bSizer1722 = new wxBoxSizer( wxHORIZONTAL );

    wxStaticBoxSizer* sbSizer24;
    sbSizer24 = new wxStaticBoxSizer( new wxStaticBox( m_panelBatchSettings, wxID_ANY, _("Status feedback") ), wxHORIZONTAL );

    m_checkBoxShowProgress = new wxCheckBox( m_panelBatchSettings, wxID_ANY, _("Show progress dialog"), wxDefaultPosition, wxDefaultSize, 0 );
    sbSizer24->Add( m_checkBoxShowProgress, 1, wxALIGN_CENTER_VERTICAL|wxALL, 5 );

    wxBoxSizer* bSizer1702;
    bSizer1702 = new wxBoxSizer( wxHORIZONTAL );

    m_staticText961 = new wxStaticText( m_panelBatchSettings, wxID_ANY, _("Error handling"), wxDefaultPosition, wxDefaultSize, 0 );
    m_staticText961->Wrap( -1 );
    bSizer1702->Add( m_staticText961, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );

    wxArrayString m_choiceHandleErrorChoices;
    m_choiceHandleError = new wxChoice( m_panelBatchSettings, wxID_ANY, wxDefaultPosition, wxDefaultSize, m_choiceHandleErrorChoices, 0 );
    m_choiceHandleError->SetSelection( 0 );
    bSizer1702->Add( m_choiceHandleError, 0, wxALIGN_CENTER_VERTICAL, 5 );


    sbSizer24->Add( bSizer1702, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );


    bSizer1722->Add( sbSizer24, 1, wxALIGN_CENTER_VERTICAL, 5 );


    bSizer1722->Add( 10, 0, 0, 0, 5 );


    bSizer172->Add( bSizer1722, 0, wxEXPAND, 5 );


    bSizer172->Add( 0, 5, 0, 0, 5 );

    sbSizerLogfileDir = new wxStaticBoxSizer( new wxStaticBox( m_panelBatchSettings, wxID_ANY, _("Logging") ), wxVERTICAL );

    wxBoxSizer* bSizer152;
    bSizer152 = new wxBoxSizer( wxHORIZONTAL );

    m_staticText96 = new wxStaticText( m_panelBatchSettings, wxID_ANY, _("Maximum number of log files:"), wxDefaultPosition, wxDefaultSize, 0 );
    m_staticText96->Wrap( -1 );
    bSizer152->Add( m_staticText96, 1, wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );

    m_spinCtrlLogCountMax = new wxSpinCtrl( m_panelBatchSettings, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 0, 2000000000, 0 );
    bSizer152->Add( m_spinCtrlLogCountMax, 0, wxALIGN_CENTER_VERTICAL, 5 );


    sbSizerLogfileDir->Add( bSizer152, 0, wxEXPAND|wxTOP|wxRIGHT|wxLEFT, 5 );

    m_panelLogfile = new wxPanel( m_panelBatchSettings, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
    wxBoxSizer* bSizer1721;
    bSizer1721 = new wxBoxSizer( wxVERTICAL );

    m_staticText94 = new wxStaticText( m_panelLogfile, wxID_ANY, _("Select folder to save log files:"), wxDefaultPosition, wxDefaultSize, 0 );
    m_staticText94->Wrap( -1 );
    bSizer1721->Add( m_staticText94, 0, wxTOP|wxBOTTOM|wxLEFT, 5 );

    wxBoxSizer* bSizer170;
    bSizer170 = new wxBoxSizer( wxHORIZONTAL );

    m_comboBoxLogfileDir = new FolderHistoryBox( m_panelLogfile, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0, NULL, 0 );
    bSizer170->Add( m_comboBoxLogfileDir, 1, wxALIGN_CENTER_VERTICAL, 5 );

    m_dirPickerLogfileDir = new zen::DirPickerCtrl( m_panelLogfile, wxID_ANY, wxEmptyString, _("Select a folder"), wxDefaultPosition, wxDefaultSize, 0 );
    m_dirPickerLogfileDir->SetToolTip( _("Select a folder") );

    bSizer170->Add( m_dirPickerLogfileDir, 0, wxALIGN_CENTER_VERTICAL, 5 );


    bSizer1721->Add( bSizer170, 0, wxEXPAND, 5 );


    m_panelLogfile->SetSizer( bSizer1721 );
    m_panelLogfile->Layout();
    bSizer1721->Fit( m_panelLogfile );
    sbSizerLogfileDir->Add( m_panelLogfile, 0, wxEXPAND|wxBOTTOM|wxRIGHT|wxLEFT, 5 );


    bSizer172->Add( sbSizerLogfileDir, 0, wxEXPAND, 5 );


    bSizer117->Add( bSizer172, 1, wxEXPAND|wxALL, 10 );


    m_panelBatchSettings->SetSizer( bSizer117 );
    m_panelBatchSettings->Layout();
    bSizer117->Fit( m_panelBatchSettings );
    m_notebook1->AddPage( m_panelBatchSettings, _("Batch settings"), false );

    bSizer54->Add( m_notebook1, 1, wxEXPAND|wxTOP|wxRIGHT|wxLEFT, 5 );

    wxBoxSizer* bSizer68;
    bSizer68 = new wxBoxSizer( wxHORIZONTAL );

    m_buttonLoad = new wxButton( this, wxID_OPEN, _("&Open..."), wxDefaultPosition, wxSize( -1,30 ), 0 );
    m_buttonLoad->SetFont( wxFont( 10, 70, 90, 90, false, wxEmptyString ) );

    bSizer68->Add( m_buttonLoad, 0, wxALIGN_CENTER_VERTICAL|wxTOP|wxBOTTOM|wxLEFT, 5 );

    m_buttonSave = new wxButton( this, wxID_SAVE, _("Save &As..."), wxDefaultPosition, wxSize( -1,30 ), 0 );
    m_buttonSave->SetDefault();
    m_buttonSave->SetFont( wxFont( 10, 70, 90, 92, false, wxEmptyString ) );

    bSizer68->Add( m_buttonSave, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxALL, 5 );

    m_button6 = new wxButton( this, wxID_CANCEL, _("Cancel"), wxDefaultPosition, wxSize( -1,30 ), 0 );
    m_button6->SetFont( wxFont( 10, 70, 90, 90, false, wxEmptyString ) );

    bSizer68->Add( m_button6, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );


    bSizer54->Add( bSizer68, 0, wxALIGN_CENTER_HORIZONTAL, 5 );


    this->SetSizer( bSizer54 );
    this->Layout();
    bSizer54->Fit( this );

    this->Centre( wxBOTH );

    // Connect Events
    this->Connect( wxEVT_CLOSE_WINDOW, wxCloseEventHandler( BatchDlgGenerated::OnClose ) );
    m_bpButtonHelp->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( BatchDlgGenerated::OnHelp ), NULL, this );
    m_bpButtonCmpConfig->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( BatchDlgGenerated::OnCmpSettings ), NULL, this );
    m_bpButtonFilter->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( BatchDlgGenerated::OnConfigureFilter ), NULL, this );
    m_bpButtonSyncConfig->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( BatchDlgGenerated::OnSyncSettings ), NULL, this );
    m_bpButtonAddPair->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( BatchDlgGenerated::OnAddFolderPair ), NULL, this );
    m_bpButtonRemovePair->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( BatchDlgGenerated::OnRemoveTopFolderPair ), NULL, this );
    m_choiceHandleError->Connect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( BatchDlgGenerated::OnChangeErrorHandling ), NULL, this );
    m_spinCtrlLogCountMax->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( BatchDlgGenerated::OnChangeMaxLogCountTxt ), NULL, this );
    m_buttonLoad->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( BatchDlgGenerated::OnLoadBatchJob ), NULL, this );
    m_buttonSave->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( BatchDlgGenerated::OnSaveBatchJob ), NULL, this );
    m_button6->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( BatchDlgGenerated::OnCancel ), NULL, this );
}

BatchDlgGenerated::~BatchDlgGenerated()
{
    // Disconnect Events
    this->Disconnect( wxEVT_CLOSE_WINDOW, wxCloseEventHandler( BatchDlgGenerated::OnClose ) );
    m_bpButtonHelp->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( BatchDlgGenerated::OnHelp ), NULL, this );
    m_bpButtonCmpConfig->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( BatchDlgGenerated::OnCmpSettings ), NULL, this );
    m_bpButtonFilter->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( BatchDlgGenerated::OnConfigureFilter ), NULL, this );
    m_bpButtonSyncConfig->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( BatchDlgGenerated::OnSyncSettings ), NULL, this );
    m_bpButtonAddPair->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( BatchDlgGenerated::OnAddFolderPair ), NULL, this );
    m_bpButtonRemovePair->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( BatchDlgGenerated::OnRemoveTopFolderPair ), NULL, this );
    m_choiceHandleError->Disconnect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( BatchDlgGenerated::OnChangeErrorHandling ), NULL, this );
    m_spinCtrlLogCountMax->Disconnect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( BatchDlgGenerated::OnChangeMaxLogCountTxt ), NULL, this );
    m_buttonLoad->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( BatchDlgGenerated::OnLoadBatchJob ), NULL, this );
    m_buttonSave->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( BatchDlgGenerated::OnSaveBatchJob ), NULL, this );
    m_button6->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( BatchDlgGenerated::OnCancel ), NULL, this );

}

BatchFolderPairGenerated::BatchFolderPairGenerated( wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style ) : wxPanel( parent, id, pos, size, style )
{
    wxBoxSizer* bSizer142;
    bSizer142 = new wxBoxSizer( wxVERTICAL );

    wxBoxSizer* bSizer140;
    bSizer140 = new wxBoxSizer( wxHORIZONTAL );

    m_panel32 = new wxPanel( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxDOUBLE_BORDER );
    wxBoxSizer* bSizer147;
    bSizer147 = new wxBoxSizer( wxHORIZONTAL );

    wxBoxSizer* bSizer136;
    bSizer136 = new wxBoxSizer( wxVERTICAL );

    m_bpButtonRemovePair = new wxBitmapButton( m_panel32, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxSize( 19,21 ), wxBU_AUTODRAW );
    m_bpButtonRemovePair->SetToolTip( _("Remove folder pair") );

    bSizer136->Add( m_bpButtonRemovePair, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 5 );


    bSizer147->Add( bSizer136, 0, wxALIGN_CENTER_VERTICAL, 5 );

    wxBoxSizer* bSizer143;
    bSizer143 = new wxBoxSizer( wxVERTICAL );

    wxBoxSizer* bSizer145;
    bSizer145 = new wxBoxSizer( wxHORIZONTAL );

    m_staticText53 = new wxStaticText( m_panel32, wxID_ANY, _("Left"), wxDefaultPosition, wxDefaultSize, 0 );
    m_staticText53->Wrap( -1 );
    m_staticText53->SetFont( wxFont( 8, 70, 90, 92, false, wxEmptyString ) );

    bSizer145->Add( m_staticText53, 0, wxALIGN_CENTER_VERTICAL, 5 );


    bSizer143->Add( bSizer145, 1, 0, 5 );

    wxBoxSizer* bSizer146;
    bSizer146 = new wxBoxSizer( wxHORIZONTAL );

    m_staticText541 = new wxStaticText( m_panel32, wxID_ANY, _("Right"), wxDefaultPosition, wxDefaultSize, 0 );
    m_staticText541->Wrap( -1 );
    m_staticText541->SetFont( wxFont( 8, 70, 90, 92, false, wxEmptyString ) );

    bSizer146->Add( m_staticText541, 0, wxALIGN_CENTER_VERTICAL, 5 );


    bSizer143->Add( bSizer146, 1, 0, 5 );


    bSizer147->Add( bSizer143, 0, wxALIGN_CENTER_VERTICAL|wxEXPAND|wxRIGHT|wxLEFT, 5 );


    m_panel32->SetSizer( bSizer147 );
    m_panel32->Layout();
    bSizer147->Fit( m_panel32 );
    bSizer140->Add( m_panel32, 0, wxALIGN_CENTER_VERTICAL|wxEXPAND, 5 );

    wxBoxSizer* bSizer144;
    bSizer144 = new wxBoxSizer( wxVERTICAL );

    m_panelLeft = new wxPanel( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
    wxBoxSizer* bSizer114;
    bSizer114 = new wxBoxSizer( wxHORIZONTAL );

    m_directoryLeft = new FolderHistoryBox( m_panelLeft, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0, NULL, 0 );
    bSizer114->Add( m_directoryLeft, 1, wxALIGN_CENTER_VERTICAL, 5 );

    m_dirPickerLeft = new zen::DirPickerCtrl( m_panelLeft, wxID_ANY, wxEmptyString, _("Select a folder"), wxDefaultPosition, wxDefaultSize, 0 );
    m_dirPickerLeft->SetToolTip( _("Select a folder") );

    bSizer114->Add( m_dirPickerLeft, 0, wxALIGN_CENTER_VERTICAL, 5 );


    m_panelLeft->SetSizer( bSizer114 );
    m_panelLeft->Layout();
    bSizer114->Fit( m_panelLeft );
    bSizer144->Add( m_panelLeft, 0, wxALIGN_CENTER_VERTICAL|wxEXPAND, 5 );

    m_panelRight = new wxPanel( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
    wxBoxSizer* bSizer115;
    bSizer115 = new wxBoxSizer( wxHORIZONTAL );

    m_directoryRight = new FolderHistoryBox( m_panelRight, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0, NULL, 0 );
    bSizer115->Add( m_directoryRight, 1, wxALIGN_CENTER_VERTICAL, 5 );

    m_dirPickerRight = new zen::DirPickerCtrl( m_panelRight, wxID_ANY, wxEmptyString, _("Select a folder"), wxDefaultPosition, wxDefaultSize, 0 );
    m_dirPickerRight->SetToolTip( _("Select a folder") );

    bSizer115->Add( m_dirPickerRight, 0, wxALIGN_CENTER_VERTICAL, 5 );


    m_panelRight->SetSizer( bSizer115 );
    m_panelRight->Layout();
    bSizer115->Fit( m_panelRight );
    bSizer144->Add( m_panelRight, 0, wxEXPAND|wxALIGN_CENTER_VERTICAL, 5 );


    bSizer140->Add( bSizer144, 1, wxALIGN_CENTER_VERTICAL, 5 );

    wxBoxSizer* bSizer176;
    bSizer176 = new wxBoxSizer( wxHORIZONTAL );

    m_bpButtonAltCompCfg = new wxBitmapButton( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxSize( 20,20 ), wxBU_AUTODRAW );
    bSizer176->Add( m_bpButtonAltCompCfg, 0, wxALIGN_CENTER_VERTICAL, 5 );

    m_bpButtonLocalFilter = new wxBitmapButton( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxSize( 20,20 ), wxBU_AUTODRAW );
    bSizer176->Add( m_bpButtonLocalFilter, 0, wxALIGN_CENTER_VERTICAL|wxALIGN_CENTER_HORIZONTAL|wxRIGHT|wxLEFT, 3 );

    m_bpButtonAltSyncCfg = new wxBitmapButton( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxSize( 20,20 ), wxBU_AUTODRAW );
    bSizer176->Add( m_bpButtonAltSyncCfg, 0, wxALIGN_CENTER_VERTICAL, 5 );


    bSizer140->Add( bSizer176, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );


    bSizer142->Add( bSizer140, 0, wxEXPAND, 5 );


    bSizer142->Add( 0, 5, 0, 0, 5 );


    this->SetSizer( bSizer142 );
    this->Layout();
    bSizer142->Fit( this );
}

BatchFolderPairGenerated::~BatchFolderPairGenerated()
{
}

SyncCfgDlgGenerated::SyncCfgDlgGenerated( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : wxDialog( parent, id, title, pos, size, style )
{
    this->SetSizeHints( wxDefaultSize, wxDefaultSize );

    wxBoxSizer* bSizer7;
    bSizer7 = new wxBoxSizer( wxVERTICAL );

    wxBoxSizer* bSizer181;
    bSizer181 = new wxBoxSizer( wxHORIZONTAL );

    wxBoxSizer* bSizer29;
    bSizer29 = new wxBoxSizer( wxVERTICAL );

    wxStaticBoxSizer* sbSizer7;
    sbSizer7 = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Select variant:") ), wxVERTICAL );

    wxFlexGridSizer* fgSizer1;
    fgSizer1 = new wxFlexGridSizer( 4, 3, 8, 5 );
    fgSizer1->SetFlexibleDirection( wxHORIZONTAL );
    fgSizer1->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

    m_radioBtnAutomatic = new wxRadioButton( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxRB_GROUP );
    m_radioBtnAutomatic->SetValue( true );
    m_radioBtnAutomatic->SetFont( wxFont( 11, 70, 90, 92, false, wxEmptyString ) );

    fgSizer1->Add( m_radioBtnAutomatic, 0, wxALIGN_CENTER_VERTICAL, 5 );

    m_buttonAutomatic = new wxButton( this, wxID_ANY, _("<Automatic>"), wxDefaultPosition, wxSize( -1,-1 ), 0 );
    m_buttonAutomatic->SetFont( wxFont( 11, 70, 90, 92, false, wxEmptyString ) );

    fgSizer1->Add( m_buttonAutomatic, 0, wxALIGN_CENTER_VERTICAL|wxEXPAND, 5 );

    m_staticTextAutomatic = new wxStaticText( this, wxID_ANY, _("Identify and propagate changes on both sides using a database. Deletions, renaming and conflicts are detected automatically."), wxDefaultPosition, wxDefaultSize, 0 );
    m_staticTextAutomatic->Wrap( 410 );
    fgSizer1->Add( m_staticTextAutomatic, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );

    m_radioBtnMirror = new wxRadioButton( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
    m_radioBtnMirror->SetFont( wxFont( 11, 70, 90, 92, false, wxEmptyString ) );

    fgSizer1->Add( m_radioBtnMirror, 0, wxALIGN_CENTER_VERTICAL, 5 );

    m_buttonMirror = new wxButton( this, wxID_ANY, _("Mirror ->>"), wxDefaultPosition, wxSize( -1,-1 ), 0 );
    m_buttonMirror->SetFont( wxFont( 11, 70, 90, 92, false, wxEmptyString ) );

    fgSizer1->Add( m_buttonMirror, 0, wxALIGN_CENTER_VERTICAL|wxEXPAND, 5 );

    m_staticTextMirror = new wxStaticText( this, wxID_ANY, _("Mirror backup of left folder. Right folder is modified to exactly match left folder after synchronization."), wxDefaultPosition, wxDefaultSize, 0 );
    m_staticTextMirror->Wrap( 410 );
    fgSizer1->Add( m_staticTextMirror, 0, wxLEFT|wxALIGN_CENTER_VERTICAL, 5 );

    m_radioBtnUpdate = new wxRadioButton( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
    m_radioBtnUpdate->SetFont( wxFont( 11, 70, 90, 92, false, wxEmptyString ) );

    fgSizer1->Add( m_radioBtnUpdate, 0, wxALIGN_CENTER_VERTICAL, 5 );

    m_buttonUpdate = new wxButton( this, wxID_ANY, _("Update ->"), wxDefaultPosition, wxSize( -1,-1 ), 0 );
    m_buttonUpdate->SetFont( wxFont( 11, 70, 90, 92, false, wxEmptyString ) );

    fgSizer1->Add( m_buttonUpdate, 0, wxALIGN_CENTER_VERTICAL|wxEXPAND, 5 );

    m_staticTextUpdate = new wxStaticText( this, wxID_ANY, _("Copy new or updated files to right folder."), wxDefaultPosition, wxDefaultSize, 0 );
    m_staticTextUpdate->Wrap( 410 );
    fgSizer1->Add( m_staticTextUpdate, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );

    m_radioBtnCustom = new wxRadioButton( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
    m_radioBtnCustom->SetFont( wxFont( 11, 70, 90, 92, false, wxEmptyString ) );

    fgSizer1->Add( m_radioBtnCustom, 0, wxALIGN_CENTER_VERTICAL, 5 );

    m_buttonCustom = new wxButton( this, wxID_ANY, _("Custom"), wxDefaultPosition, wxSize( -1,-1 ), 0 );
    m_buttonCustom->SetFont( wxFont( 11, 70, 90, 92, false, wxEmptyString ) );

    fgSizer1->Add( m_buttonCustom, 0, wxALIGN_CENTER_VERTICAL|wxEXPAND, 5 );

    m_staticTextCustom = new wxStaticText( this, wxID_ANY, _("Configure your own synchronization rules."), wxDefaultPosition, wxDefaultSize, 0 );
    m_staticTextCustom->Wrap( 410 );
    fgSizer1->Add( m_staticTextCustom, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );


    sbSizer7->Add( fgSizer1, 0, 0, 5 );


    bSizer29->Add( sbSizer7, 0, wxEXPAND, 5 );


    bSizer29->Add( 0, 5, 1, 0, 5 );

    sbSizerCustDelDir = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Deletion handling") ), wxHORIZONTAL );

    wxArrayString m_choiceHandleDeletionChoices;
    m_choiceHandleDeletion = new wxChoice( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, m_choiceHandleDeletionChoices, 0 );
    m_choiceHandleDeletion->SetSelection( 0 );
    sbSizerCustDelDir->Add( m_choiceHandleDeletion, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );

    m_panelCustomDeletionDir = new wxPanel( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
    wxBoxSizer* bSizer1151;
    bSizer1151 = new wxBoxSizer( wxHORIZONTAL );

    m_customDelFolder = new FolderHistoryBox( m_panelCustomDeletionDir, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0, NULL, 0 );
    bSizer1151->Add( m_customDelFolder, 1, wxALIGN_CENTER_VERTICAL, 5 );

    m_dirPickerCustomDelFolder = new zen::DirPickerCtrl( m_panelCustomDeletionDir, wxID_ANY, wxEmptyString, _("Select a folder"), wxDefaultPosition, wxDefaultSize, 0 );
    bSizer1151->Add( m_dirPickerCustomDelFolder, 0, wxALIGN_CENTER_VERTICAL, 5 );


    m_panelCustomDeletionDir->SetSizer( bSizer1151 );
    m_panelCustomDeletionDir->Layout();
    bSizer1151->Fit( m_panelCustomDeletionDir );
    sbSizerCustDelDir->Add( m_panelCustomDeletionDir, 1, wxALIGN_CENTER_VERTICAL, 5 );


    bSizer29->Add( sbSizerCustDelDir, 0, wxEXPAND, 5 );

    bSizer201 = new wxBoxSizer( wxHORIZONTAL );

    sbSizerErrorHandling = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Error handling") ), wxHORIZONTAL );

    wxArrayString m_choiceHandleErrorChoices;
    m_choiceHandleError = new wxChoice( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, m_choiceHandleErrorChoices, 0 );
    m_choiceHandleError->SetSelection( 0 );
    sbSizerErrorHandling->Add( m_choiceHandleError, 0, wxALIGN_CENTER_VERTICAL, 5 );


    bSizer201->Add( sbSizerErrorHandling, 0, wxRIGHT|wxALIGN_CENTER_VERTICAL, 10 );

    sbSizerExecFinished = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("On completion:") ), wxHORIZONTAL );

    m_comboBoxExecFinished = new ExecFinishedBox( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0, NULL, 0 );
    sbSizerExecFinished->Add( m_comboBoxExecFinished, 1, wxALIGN_CENTER_VERTICAL, 5 );


    bSizer201->Add( sbSizerExecFinished, 1, wxALIGN_CENTER_VERTICAL, 5 );


    bSizer29->Add( bSizer201, 0, wxEXPAND|wxTOP, 5 );


    bSizer181->Add( bSizer29, 0, wxEXPAND, 5 );


    bSizer181->Add( 10, 0, 0, 0, 5 );

    wxStaticBoxSizer* sbSizer2453245;
    sbSizer2453245 = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Configuration") ), wxVERTICAL );


    sbSizer2453245->Add( 0, 0, 1, wxEXPAND, 5 );

    m_bitmapDatabase = new wxStaticBitmap( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxSize( 70,70 ), 0 );
    sbSizer2453245->Add( m_bitmapDatabase, 1, wxALIGN_CENTER_HORIZONTAL|wxRIGHT|wxLEFT, 10 );

    sbSizerSyncDirections = new wxBoxSizer( wxVERTICAL );

    wxGridSizer* gSizer3;
    gSizer3 = new wxGridSizer( 1, 2, 0, 5 );

    m_staticText21 = new wxStaticText( this, wxID_ANY, _("Category"), wxDefaultPosition, wxDefaultSize, 0 );
    m_staticText21->Wrap( -1 );
    m_staticText21->SetFont( wxFont( 8, 70, 90, 92, false, wxEmptyString ) );

    gSizer3->Add( m_staticText21, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 5 );

    m_staticText31 = new wxStaticText( this, wxID_ANY, _("Action"), wxDefaultPosition, wxDefaultSize, 0 );
    m_staticText31->Wrap( -1 );
    m_staticText31->SetFont( wxFont( 8, 70, 90, 92, false, wxEmptyString ) );

    gSizer3->Add( m_staticText31, 0, wxALIGN_CENTER_VERTICAL|wxALIGN_CENTER_HORIZONTAL, 5 );


    sbSizerSyncDirections->Add( gSizer3, 0, wxBOTTOM|wxEXPAND, 5 );

    wxBoxSizer* bSizer121;
    bSizer121 = new wxBoxSizer( wxVERTICAL );

    bSizerLeftOnly = new wxBoxSizer( wxHORIZONTAL );

    m_bitmapLeftOnly = new wxStaticBitmap( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxSize( 45,45 ), 0 );
    m_bitmapLeftOnly->SetToolTip( _("Item exists on left side only") );

    bSizerLeftOnly->Add( m_bitmapLeftOnly, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 5 );


    bSizerLeftOnly->Add( 5, 0, 0, wxALIGN_CENTER_VERTICAL, 5 );

    m_bpButtonLeftOnly = new wxBitmapButton( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxSize( 42,42 ), wxBU_AUTODRAW );
    bSizerLeftOnly->Add( m_bpButtonLeftOnly, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 5 );


    bSizer121->Add( bSizerLeftOnly, 0, wxALIGN_CENTER_HORIZONTAL, 5 );

    bSizerRightOnly = new wxBoxSizer( wxHORIZONTAL );

    m_bitmapRightOnly = new wxStaticBitmap( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxSize( 45,45 ), 0 );
    m_bitmapRightOnly->SetToolTip( _("Item exists on right side only") );

    bSizerRightOnly->Add( m_bitmapRightOnly, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 5 );


    bSizerRightOnly->Add( 5, 0, 0, 0, 5 );

    m_bpButtonRightOnly = new wxBitmapButton( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxSize( 42,42 ), wxBU_AUTODRAW );
    bSizerRightOnly->Add( m_bpButtonRightOnly, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 5 );


    bSizer121->Add( bSizerRightOnly, 0, wxALIGN_CENTER_HORIZONTAL, 5 );

    bSizerLeftNewer = new wxBoxSizer( wxHORIZONTAL );

    m_bitmapLeftNewer = new wxStaticBitmap( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxSize( 45,45 ), 0 );
    m_bitmapLeftNewer->SetToolTip( _("Left side is newer") );

    bSizerLeftNewer->Add( m_bitmapLeftNewer, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 5 );


    bSizerLeftNewer->Add( 5, 0, 0, 0, 5 );

    m_bpButtonLeftNewer = new wxBitmapButton( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxSize( 42,42 ), wxBU_AUTODRAW );
    bSizerLeftNewer->Add( m_bpButtonLeftNewer, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 5 );


    bSizer121->Add( bSizerLeftNewer, 0, wxALIGN_CENTER_HORIZONTAL, 5 );

    bSizerRightNewer = new wxBoxSizer( wxHORIZONTAL );

    m_bitmapRightNewer = new wxStaticBitmap( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxSize( 45,45 ), 0 );
    m_bitmapRightNewer->SetToolTip( _("Right side is newer") );

    bSizerRightNewer->Add( m_bitmapRightNewer, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 5 );


    bSizerRightNewer->Add( 5, 0, 0, 0, 5 );

    m_bpButtonRightNewer = new wxBitmapButton( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxSize( 42,42 ), wxBU_AUTODRAW );
    bSizerRightNewer->Add( m_bpButtonRightNewer, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 5 );


    bSizer121->Add( bSizerRightNewer, 0, wxALIGN_CENTER_HORIZONTAL, 5 );

    bSizerDifferent = new wxBoxSizer( wxHORIZONTAL );

    m_bitmapDifferent = new wxStaticBitmap( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxSize( 45,45 ), 0 );
    m_bitmapDifferent->SetToolTip( _("Items have different content") );

    bSizerDifferent->Add( m_bitmapDifferent, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 5 );


    bSizerDifferent->Add( 5, 0, 0, 0, 5 );

    m_bpButtonDifferent = new wxBitmapButton( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxSize( 42,42 ), wxBU_AUTODRAW );
    bSizerDifferent->Add( m_bpButtonDifferent, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 5 );


    bSizer121->Add( bSizerDifferent, 0, wxALIGN_CENTER_HORIZONTAL, 5 );

    bSizerConflict = new wxBoxSizer( wxHORIZONTAL );

    m_bitmapConflict = new wxStaticBitmap( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxSize( 45,45 ), 0 );
    m_bitmapConflict->SetToolTip( _("Conflict/item cannot be categorized") );

    bSizerConflict->Add( m_bitmapConflict, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 5 );


    bSizerConflict->Add( 5, 0, 0, 0, 5 );

    m_bpButtonConflict = new wxBitmapButton( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxSize( 42,42 ), wxBU_AUTODRAW );
    bSizerConflict->Add( m_bpButtonConflict, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 5 );


    bSizer121->Add( bSizerConflict, 0, wxALIGN_CENTER_HORIZONTAL, 5 );


    sbSizerSyncDirections->Add( bSizer121, 0, wxALIGN_CENTER_HORIZONTAL, 5 );


    sbSizer2453245->Add( sbSizerSyncDirections, 0, wxEXPAND, 5 );


    sbSizer2453245->Add( 0, 0, 1, wxEXPAND, 5 );


    bSizer181->Add( sbSizer2453245, 0, wxALIGN_CENTER_HORIZONTAL, 5 );


    bSizer7->Add( bSizer181, 0, wxTOP|wxRIGHT|wxLEFT, 5 );

    wxBoxSizer* bSizer291;
    bSizer291 = new wxBoxSizer( wxHORIZONTAL );

    m_buttonOK = new wxButton( this, wxID_OK, _("OK"), wxDefaultPosition, wxSize( -1,30 ), 0 );
    m_buttonOK->SetDefault();
    m_buttonOK->SetFont( wxFont( 10, 70, 90, 92, false, wxEmptyString ) );

    bSizer291->Add( m_buttonOK, 0, wxALIGN_CENTER_VERTICAL|wxTOP|wxBOTTOM|wxLEFT, 5 );

    m_button16 = new wxButton( this, wxID_CANCEL, _("Cancel"), wxDefaultPosition, wxSize( -1,30 ), 0 );
    m_button16->SetFont( wxFont( 10, 70, 90, 90, false, wxEmptyString ) );

    bSizer291->Add( m_button16, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );


    bSizer7->Add( bSizer291, 0, wxALIGN_CENTER_HORIZONTAL|wxBOTTOM, 5 );


    this->SetSizer( bSizer7 );
    this->Layout();
    bSizer7->Fit( this );

    this->Centre( wxBOTH );

    // Connect Events
    this->Connect( wxEVT_CLOSE_WINDOW, wxCloseEventHandler( SyncCfgDlgGenerated::OnClose ) );
    m_radioBtnAutomatic->Connect( wxEVT_COMMAND_RADIOBUTTON_SELECTED, wxCommandEventHandler( SyncCfgDlgGenerated::OnSyncAutomatic ), NULL, this );
    m_buttonAutomatic->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( SyncCfgDlgGenerated::OnSyncAutomatic ), NULL, this );
    m_buttonAutomatic->Connect( wxEVT_LEFT_DCLICK, wxMouseEventHandler( SyncCfgDlgGenerated::OnSyncAutomaticDouble ), NULL, this );
    m_radioBtnMirror->Connect( wxEVT_COMMAND_RADIOBUTTON_SELECTED, wxCommandEventHandler( SyncCfgDlgGenerated::OnSyncMirror ), NULL, this );
    m_buttonMirror->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( SyncCfgDlgGenerated::OnSyncMirror ), NULL, this );
    m_buttonMirror->Connect( wxEVT_LEFT_DCLICK, wxMouseEventHandler( SyncCfgDlgGenerated::OnSyncMirrorDouble ), NULL, this );
    m_radioBtnUpdate->Connect( wxEVT_COMMAND_RADIOBUTTON_SELECTED, wxCommandEventHandler( SyncCfgDlgGenerated::OnSyncUpdate ), NULL, this );
    m_buttonUpdate->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( SyncCfgDlgGenerated::OnSyncUpdate ), NULL, this );
    m_buttonUpdate->Connect( wxEVT_LEFT_DCLICK, wxMouseEventHandler( SyncCfgDlgGenerated::OnSyncUpdateDouble ), NULL, this );
    m_radioBtnCustom->Connect( wxEVT_COMMAND_RADIOBUTTON_SELECTED, wxCommandEventHandler( SyncCfgDlgGenerated::OnSyncCustom ), NULL, this );
    m_buttonCustom->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( SyncCfgDlgGenerated::OnSyncCustom ), NULL, this );
    m_buttonCustom->Connect( wxEVT_LEFT_DCLICK, wxMouseEventHandler( SyncCfgDlgGenerated::OnSyncCustomDouble ), NULL, this );
    m_choiceHandleDeletion->Connect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( SyncCfgDlgGenerated::OnChangeDeletionHandling ), NULL, this );
    m_choiceHandleError->Connect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( SyncCfgDlgGenerated::OnChangeErrorHandling ), NULL, this );
    m_bpButtonLeftOnly->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( SyncCfgDlgGenerated::OnExLeftSideOnly ), NULL, this );
    m_bpButtonRightOnly->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( SyncCfgDlgGenerated::OnExRightSideOnly ), NULL, this );
    m_bpButtonLeftNewer->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( SyncCfgDlgGenerated::OnLeftNewer ), NULL, this );
    m_bpButtonRightNewer->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( SyncCfgDlgGenerated::OnRightNewer ), NULL, this );
    m_bpButtonDifferent->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( SyncCfgDlgGenerated::OnDifferent ), NULL, this );
    m_bpButtonConflict->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( SyncCfgDlgGenerated::OnConflict ), NULL, this );
    m_buttonOK->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( SyncCfgDlgGenerated::OnApply ), NULL, this );
    m_button16->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( SyncCfgDlgGenerated::OnCancel ), NULL, this );
}

SyncCfgDlgGenerated::~SyncCfgDlgGenerated()
{
    // Disconnect Events
    this->Disconnect( wxEVT_CLOSE_WINDOW, wxCloseEventHandler( SyncCfgDlgGenerated::OnClose ) );
    m_radioBtnAutomatic->Disconnect( wxEVT_COMMAND_RADIOBUTTON_SELECTED, wxCommandEventHandler( SyncCfgDlgGenerated::OnSyncAutomatic ), NULL, this );
    m_buttonAutomatic->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( SyncCfgDlgGenerated::OnSyncAutomatic ), NULL, this );
    m_buttonAutomatic->Disconnect( wxEVT_LEFT_DCLICK, wxMouseEventHandler( SyncCfgDlgGenerated::OnSyncAutomaticDouble ), NULL, this );
    m_radioBtnMirror->Disconnect( wxEVT_COMMAND_RADIOBUTTON_SELECTED, wxCommandEventHandler( SyncCfgDlgGenerated::OnSyncMirror ), NULL, this );
    m_buttonMirror->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( SyncCfgDlgGenerated::OnSyncMirror ), NULL, this );
    m_buttonMirror->Disconnect( wxEVT_LEFT_DCLICK, wxMouseEventHandler( SyncCfgDlgGenerated::OnSyncMirrorDouble ), NULL, this );
    m_radioBtnUpdate->Disconnect( wxEVT_COMMAND_RADIOBUTTON_SELECTED, wxCommandEventHandler( SyncCfgDlgGenerated::OnSyncUpdate ), NULL, this );
    m_buttonUpdate->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( SyncCfgDlgGenerated::OnSyncUpdate ), NULL, this );
    m_buttonUpdate->Disconnect( wxEVT_LEFT_DCLICK, wxMouseEventHandler( SyncCfgDlgGenerated::OnSyncUpdateDouble ), NULL, this );
    m_radioBtnCustom->Disconnect( wxEVT_COMMAND_RADIOBUTTON_SELECTED, wxCommandEventHandler( SyncCfgDlgGenerated::OnSyncCustom ), NULL, this );
    m_buttonCustom->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( SyncCfgDlgGenerated::OnSyncCustom ), NULL, this );
    m_buttonCustom->Disconnect( wxEVT_LEFT_DCLICK, wxMouseEventHandler( SyncCfgDlgGenerated::OnSyncCustomDouble ), NULL, this );
    m_choiceHandleDeletion->Disconnect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( SyncCfgDlgGenerated::OnChangeDeletionHandling ), NULL, this );
    m_choiceHandleError->Disconnect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( SyncCfgDlgGenerated::OnChangeErrorHandling ), NULL, this );
    m_bpButtonLeftOnly->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( SyncCfgDlgGenerated::OnExLeftSideOnly ), NULL, this );
    m_bpButtonRightOnly->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( SyncCfgDlgGenerated::OnExRightSideOnly ), NULL, this );
    m_bpButtonLeftNewer->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( SyncCfgDlgGenerated::OnLeftNewer ), NULL, this );
    m_bpButtonRightNewer->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( SyncCfgDlgGenerated::OnRightNewer ), NULL, this );
    m_bpButtonDifferent->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( SyncCfgDlgGenerated::OnDifferent ), NULL, this );
    m_bpButtonConflict->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( SyncCfgDlgGenerated::OnConflict ), NULL, this );
    m_buttonOK->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( SyncCfgDlgGenerated::OnApply ), NULL, this );
    m_button16->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( SyncCfgDlgGenerated::OnCancel ), NULL, this );

}

CmpCfgDlgGenerated::CmpCfgDlgGenerated( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : wxDialog( parent, id, title, pos, size, style )
{
    this->SetSizeHints( wxDefaultSize, wxDefaultSize );

    wxBoxSizer* bSizer136;
    bSizer136 = new wxBoxSizer( wxVERTICAL );

    wxBoxSizer* bSizer55;
    bSizer55 = new wxBoxSizer( wxVERTICAL );

    wxStaticBoxSizer* sbSizer6;
    sbSizer6 = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Compare by...") ), wxHORIZONTAL );

    wxFlexGridSizer* fgSizer16;
    fgSizer16 = new wxFlexGridSizer( 2, 3, 0, 0 );
    fgSizer16->SetFlexibleDirection( wxBOTH );
    fgSizer16->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

    m_radioBtnSizeDate = new wxRadioButton( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxRB_GROUP );
    m_radioBtnSizeDate->SetValue( true );
    m_radioBtnSizeDate->SetToolTip( _("Files are found equal if\n     - last write time and date\n     - file size\nare the same") );

    fgSizer16->Add( m_radioBtnSizeDate, 0, wxALIGN_CENTER_VERTICAL, 5 );

    m_bitmapByTime = new wxStaticBitmap( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, 0 );
    m_bitmapByTime->SetToolTip( _("Files are found equal if\n     - last write time and date\n     - file size\nare the same") );

    fgSizer16->Add( m_bitmapByTime, 0, wxALIGN_CENTER_VERTICAL|wxALIGN_CENTER_HORIZONTAL, 5 );

    m_buttonTimeSize = new wxButton( this, wxID_ANY, _("File time and size"), wxDefaultPosition, wxSize( -1,42 ), 0 );
    m_buttonTimeSize->SetFont( wxFont( 11, 70, 90, 92, false, wxEmptyString ) );
    m_buttonTimeSize->SetToolTip( _("Files are found equal if\n     - last write time and date\n     - file size\nare the same") );

    fgSizer16->Add( m_buttonTimeSize, 0, wxALIGN_CENTER_VERTICAL|wxEXPAND|wxALL, 5 );

    m_radioBtnContent = new wxRadioButton( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
    m_radioBtnContent->SetToolTip( _("Files are found equal if\n     - file content\nis the same") );

    fgSizer16->Add( m_radioBtnContent, 0, wxALIGN_CENTER_VERTICAL, 5 );

    m_bitmapByContent = new wxStaticBitmap( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, 0 );
    m_bitmapByContent->SetToolTip( _("Files are found equal if\n     - file content\nis the same") );

    fgSizer16->Add( m_bitmapByContent, 0, wxALIGN_CENTER_VERTICAL|wxALIGN_CENTER_HORIZONTAL, 5 );

    m_buttonContent = new wxButton( this, wxID_ANY, _("File content"), wxDefaultPosition, wxSize( -1,42 ), 0 );
    m_buttonContent->SetFont( wxFont( 11, 70, 90, 92, false, wxEmptyString ) );
    m_buttonContent->SetToolTip( _("Files are found equal if\n     - file content\nis the same") );

    fgSizer16->Add( m_buttonContent, 0, wxALIGN_CENTER_VERTICAL|wxEXPAND|wxBOTTOM|wxRIGHT|wxLEFT, 5 );


    sbSizer6->Add( fgSizer16, 1, wxALIGN_CENTER_VERTICAL|wxEXPAND, 5 );


    bSizer55->Add( sbSizer6, 0, wxALIGN_CENTER_VERTICAL|wxEXPAND, 2 );


    bSizer55->Add( 0, 4, 0, 0, 5 );

    wxBoxSizer* bSizer177;
    bSizer177 = new wxBoxSizer( wxHORIZONTAL );

    wxStaticBoxSizer* sbSizer25;
    sbSizer25 = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Symbolic Link handling") ), wxVERTICAL );

    wxArrayString m_choiceHandleSymlinksChoices;
    m_choiceHandleSymlinks = new wxChoice( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, m_choiceHandleSymlinksChoices, 0 );
    m_choiceHandleSymlinks->SetSelection( -1 );
    sbSizer25->Add( m_choiceHandleSymlinks, 0, wxALIGN_CENTER_VERTICAL|wxALIGN_CENTER_HORIZONTAL, 5 );


    bSizer177->Add( sbSizer25, 1, wxALIGN_CENTER_VERTICAL, 5 );

    m_bpButtonHelp = new wxBitmapButton( this, wxID_HELP, wxNullBitmap, wxDefaultPosition, wxSize( 40,40 ), wxBU_AUTODRAW );
    m_bpButtonHelp->SetToolTip( _("Help") );

    bSizer177->Add( m_bpButtonHelp, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );


    bSizer55->Add( bSizer177, 0, wxEXPAND, 5 );

    wxBoxSizer* bSizer22;
    bSizer22 = new wxBoxSizer( wxHORIZONTAL );

    m_button10 = new wxButton( this, wxID_OK, _("OK"), wxDefaultPosition, wxSize( -1,30 ), 0 );
    m_button10->SetDefault();
    m_button10->SetFont( wxFont( 10, 70, 90, 92, false, wxEmptyString ) );

    bSizer22->Add( m_button10, 0, wxALIGN_CENTER_VERTICAL|wxTOP|wxBOTTOM|wxLEFT, 5 );

    m_button6 = new wxButton( this, wxID_CANCEL, _("Cancel"), wxDefaultPosition, wxSize( -1,30 ), 0 );
    m_button6->SetFont( wxFont( 10, 70, 90, 90, false, wxEmptyString ) );

    bSizer22->Add( m_button6, 0, wxALL|wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 5 );


    bSizer55->Add( bSizer22, 0, wxALIGN_CENTER_HORIZONTAL, 5 );


    bSizer136->Add( bSizer55, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );


    this->SetSizer( bSizer136 );
    this->Layout();
    bSizer136->Fit( this );

    this->Centre( wxBOTH );

    // Connect Events
    this->Connect( wxEVT_CLOSE_WINDOW, wxCloseEventHandler( CmpCfgDlgGenerated::OnClose ) );
    m_radioBtnSizeDate->Connect( wxEVT_COMMAND_RADIOBUTTON_SELECTED, wxCommandEventHandler( CmpCfgDlgGenerated::OnTimeSize ), NULL, this );
    m_buttonTimeSize->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( CmpCfgDlgGenerated::OnTimeSize ), NULL, this );
    m_buttonTimeSize->Connect( wxEVT_LEFT_DCLICK, wxMouseEventHandler( CmpCfgDlgGenerated::OnTimeSizeDouble ), NULL, this );
    m_radioBtnContent->Connect( wxEVT_COMMAND_RADIOBUTTON_SELECTED, wxCommandEventHandler( CmpCfgDlgGenerated::OnContent ), NULL, this );
    m_buttonContent->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( CmpCfgDlgGenerated::OnContent ), NULL, this );
    m_buttonContent->Connect( wxEVT_LEFT_DCLICK, wxMouseEventHandler( CmpCfgDlgGenerated::OnContentDouble ), NULL, this );
    m_choiceHandleSymlinks->Connect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( CmpCfgDlgGenerated::OnChangeErrorHandling ), NULL, this );
    m_bpButtonHelp->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( CmpCfgDlgGenerated::OnShowHelp ), NULL, this );
    m_button10->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( CmpCfgDlgGenerated::OnOkay ), NULL, this );
    m_button6->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( CmpCfgDlgGenerated::OnCancel ), NULL, this );
}

CmpCfgDlgGenerated::~CmpCfgDlgGenerated()
{
    // Disconnect Events
    this->Disconnect( wxEVT_CLOSE_WINDOW, wxCloseEventHandler( CmpCfgDlgGenerated::OnClose ) );
    m_radioBtnSizeDate->Disconnect( wxEVT_COMMAND_RADIOBUTTON_SELECTED, wxCommandEventHandler( CmpCfgDlgGenerated::OnTimeSize ), NULL, this );
    m_buttonTimeSize->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( CmpCfgDlgGenerated::OnTimeSize ), NULL, this );
    m_buttonTimeSize->Disconnect( wxEVT_LEFT_DCLICK, wxMouseEventHandler( CmpCfgDlgGenerated::OnTimeSizeDouble ), NULL, this );
    m_radioBtnContent->Disconnect( wxEVT_COMMAND_RADIOBUTTON_SELECTED, wxCommandEventHandler( CmpCfgDlgGenerated::OnContent ), NULL, this );
    m_buttonContent->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( CmpCfgDlgGenerated::OnContent ), NULL, this );
    m_buttonContent->Disconnect( wxEVT_LEFT_DCLICK, wxMouseEventHandler( CmpCfgDlgGenerated::OnContentDouble ), NULL, this );
    m_choiceHandleSymlinks->Disconnect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( CmpCfgDlgGenerated::OnChangeErrorHandling ), NULL, this );
    m_bpButtonHelp->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( CmpCfgDlgGenerated::OnShowHelp ), NULL, this );
    m_button10->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( CmpCfgDlgGenerated::OnOkay ), NULL, this );
    m_button6->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( CmpCfgDlgGenerated::OnCancel ), NULL, this );

}

SyncStatusDlgGenerated::SyncStatusDlgGenerated( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : wxFrame( parent, id, title, pos, size, style )
{
    this->SetSizeHints( wxSize( 470,260 ), wxDefaultSize );
    this->SetBackgroundColour( wxSystemSettings::GetColour( wxSYS_COLOUR_WINDOW ) );

    bSizerTop = new wxBoxSizer( wxVERTICAL );

    m_panelHeader = new wxPanel( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
    m_panelHeader->SetBackgroundColour( wxSystemSettings::GetColour( wxSYS_COLOUR_BTNFACE ) );

    wxBoxSizer* bSizer181;
    bSizer181 = new wxBoxSizer( wxVERTICAL );

    wxBoxSizer* bSizer42;
    bSizer42 = new wxBoxSizer( wxHORIZONTAL );

    m_bitmapStatus = new wxStaticBitmap( m_panelHeader, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxSize( 28,28 ), 0 );
    bSizer42->Add( m_bitmapStatus, 0, wxALIGN_CENTER_VERTICAL|wxTOP|wxBOTTOM|wxRIGHT, 5 );

    m_staticTextStatus = new wxStaticText( m_panelHeader, wxID_ANY, _("Synchronizing..."), wxDefaultPosition, wxDefaultSize, 0 );
    m_staticTextStatus->Wrap( -1 );
    m_staticTextStatus->SetFont( wxFont( 14, 70, 90, 92, false, wxEmptyString ) );

    bSizer42->Add( m_staticTextStatus, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxTOP|wxBOTTOM, 5 );

    m_animationControl1 = new wxAnimationCtrl( m_panelHeader, wxID_ANY, wxNullAnimation, wxDefaultPosition, wxDefaultSize, wxAC_DEFAULT_STYLE );
    m_animationControl1->SetMinSize( wxSize( 45,45 ) );

    bSizer42->Add( m_animationControl1, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );


    bSizer181->Add( bSizer42, 0, wxALIGN_CENTER_HORIZONTAL|wxRIGHT|wxLEFT, 5 );


    m_panelHeader->SetSizer( bSizer181 );
    m_panelHeader->Layout();
    bSizer181->Fit( m_panelHeader );
    bSizerTop->Add( m_panelHeader, 0, wxEXPAND, 5 );

    m_staticlineHeader = new wxStaticLine( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
    bSizerTop->Add( m_staticlineHeader, 0, wxEXPAND, 5 );

    m_panelProgress = new wxPanel( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
    wxBoxSizer* bSizer173;
    bSizer173 = new wxBoxSizer( wxVERTICAL );

    wxBoxSizer* bSizer1811;
    bSizer1811 = new wxBoxSizer( wxVERTICAL );

    m_textCtrlStatus = new wxTextCtrl( m_panelProgress, wxID_ANY, _("dummy"), wxDefaultPosition, wxSize( -1,-1 ), wxTE_READONLY|wxNO_BORDER );
    m_textCtrlStatus->SetBackgroundColour( wxSystemSettings::GetColour( wxSYS_COLOUR_WINDOW ) );

    bSizer1811->Add( m_textCtrlStatus, 0, wxALIGN_CENTER_VERTICAL|wxEXPAND|wxLEFT, 5 );


    bSizer173->Add( bSizer1811, 0, wxEXPAND|wxTOP|wxRIGHT|wxLEFT, 5 );

    m_gauge1 = new wxGauge( m_panelProgress, wxID_ANY, 100, wxDefaultPosition, wxSize( -1,14 ), wxGA_HORIZONTAL );
    bSizer173->Add( m_gauge1, 0, wxEXPAND|wxRIGHT|wxLEFT, 5 );

    bSizer171 = new wxBoxSizer( wxHORIZONTAL );

    wxFlexGridSizer* fgSizer10;
    fgSizer10 = new wxFlexGridSizer( 0, 2, 0, 5 );
    fgSizer10->SetFlexibleDirection( wxBOTH );
    fgSizer10->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

    m_staticTextLabelItemsProc = new wxStaticText( m_panelProgress, wxID_ANY, _("Items processed:"), wxDefaultPosition, wxDefaultSize, 0 );
    m_staticTextLabelItemsProc->Wrap( -1 );
    m_staticTextLabelItemsProc->SetFont( wxFont( 10, 70, 90, 90, false, wxEmptyString ) );

    fgSizer10->Add( m_staticTextLabelItemsProc, 0, wxALIGN_BOTTOM, 5 );

    bSizerItemsProc = new wxBoxSizer( wxHORIZONTAL );

    m_staticTextProcessedObj = new wxStaticText( m_panelProgress, wxID_ANY, _("dummy"), wxDefaultPosition, wxSize( -1,-1 ), 0 );
    m_staticTextProcessedObj->Wrap( -1 );
    m_staticTextProcessedObj->SetFont( wxFont( 9, 70, 90, 92, false, wxEmptyString ) );

    bSizerItemsProc->Add( m_staticTextProcessedObj, 0, wxALIGN_BOTTOM, 5 );

    m_staticTextDataProcessed = new wxStaticText( m_panelProgress, wxID_ANY, _("dummy"), wxDefaultPosition, wxDefaultSize, 0 );
    m_staticTextDataProcessed->Wrap( -1 );
    m_staticTextDataProcessed->SetFont( wxFont( 9, 70, 90, 90, false, wxEmptyString ) );

    bSizerItemsProc->Add( m_staticTextDataProcessed, 0, wxALIGN_BOTTOM|wxLEFT, 5 );


    fgSizer10->Add( bSizerItemsProc, 0, wxALIGN_BOTTOM, 5 );

    m_staticTextLabelItemsRem = new wxStaticText( m_panelProgress, wxID_ANY, _("Items remaining:"), wxDefaultPosition, wxDefaultSize, 0 );
    m_staticTextLabelItemsRem->Wrap( -1 );
    m_staticTextLabelItemsRem->SetFont( wxFont( 10, 70, 90, 90, false, wxEmptyString ) );

    fgSizer10->Add( m_staticTextLabelItemsRem, 0, wxALIGN_BOTTOM, 5 );

    bSizerItemsRem = new wxBoxSizer( wxHORIZONTAL );

    m_staticTextRemainingObj = new wxStaticText( m_panelProgress, wxID_ANY, _("dummy"), wxDefaultPosition, wxSize( -1,-1 ), 0 );
    m_staticTextRemainingObj->Wrap( -1 );
    m_staticTextRemainingObj->SetFont( wxFont( 9, 70, 90, 92, false, wxEmptyString ) );

    bSizerItemsRem->Add( m_staticTextRemainingObj, 0, wxALIGN_BOTTOM, 5 );

    m_staticTextDataRemaining = new wxStaticText( m_panelProgress, wxID_ANY, _("dummy"), wxDefaultPosition, wxDefaultSize, 0 );
    m_staticTextDataRemaining->Wrap( -1 );
    m_staticTextDataRemaining->SetFont( wxFont( 9, 70, 90, 90, false, wxEmptyString ) );

    bSizerItemsRem->Add( m_staticTextDataRemaining, 0, wxALIGN_BOTTOM|wxLEFT, 5 );


    fgSizer10->Add( bSizerItemsRem, 0, wxALIGN_BOTTOM, 5 );

    m_staticText84 = new wxStaticText( m_panelProgress, wxID_ANY, _("Speed:"), wxDefaultPosition, wxDefaultSize, 0 );
    m_staticText84->Wrap( -1 );
    m_staticText84->SetFont( wxFont( 10, 70, 90, 90, false, wxEmptyString ) );

    fgSizer10->Add( m_staticText84, 0, wxALIGN_BOTTOM, 5 );

    m_staticTextSpeed = new wxStaticText( m_panelProgress, wxID_ANY, _("dummy"), wxDefaultPosition, wxDefaultSize, 0 );
    m_staticTextSpeed->Wrap( -1 );
    m_staticTextSpeed->SetFont( wxFont( 9, 70, 90, 92, false, wxEmptyString ) );

    fgSizer10->Add( m_staticTextSpeed, 0, wxALIGN_BOTTOM, 5 );

    m_staticTextLabelRemTime = new wxStaticText( m_panelProgress, wxID_ANY, _("Time remaining:"), wxDefaultPosition, wxDefaultSize, 0 );
    m_staticTextLabelRemTime->Wrap( -1 );
    m_staticTextLabelRemTime->SetFont( wxFont( 10, 70, 90, 90, false, wxEmptyString ) );

    fgSizer10->Add( m_staticTextLabelRemTime, 0, wxALIGN_BOTTOM, 5 );

    m_staticTextRemTime = new wxStaticText( m_panelProgress, wxID_ANY, _("dummy"), wxDefaultPosition, wxDefaultSize, 0 );
    m_staticTextRemTime->Wrap( -1 );
    m_staticTextRemTime->SetFont( wxFont( 9, 70, 90, 92, false, wxEmptyString ) );

    fgSizer10->Add( m_staticTextRemTime, 0, wxALIGN_BOTTOM, 5 );

    m_staticTextLabelElapsedTime = new wxStaticText( m_panelProgress, wxID_ANY, _("Time elapsed:"), wxDefaultPosition, wxDefaultSize, 0 );
    m_staticTextLabelElapsedTime->Wrap( -1 );
    m_staticTextLabelElapsedTime->SetFont( wxFont( 10, 70, 90, 90, false, wxEmptyString ) );

    fgSizer10->Add( m_staticTextLabelElapsedTime, 0, wxALIGN_BOTTOM, 5 );

    m_staticTextTimeElapsed = new wxStaticText( m_panelProgress, wxID_ANY, _("dummy"), wxDefaultPosition, wxDefaultSize, 0 );
    m_staticTextTimeElapsed->Wrap( -1 );
    m_staticTextTimeElapsed->SetFont( wxFont( 9, 70, 90, 92, false, wxEmptyString ) );

    fgSizer10->Add( m_staticTextTimeElapsed, 0, wxALIGN_BOTTOM, 5 );


    bSizer171->Add( fgSizer10, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 10 );


    bSizer171->Add( 10, 0, 0, 0, 5 );

    m_panelGraph = new zen::Graph2D( m_panelProgress, wxID_ANY, wxDefaultPosition, wxSize( 340,130 ), wxTAB_TRAVERSAL );
    m_panelGraph->SetBackgroundColour( wxSystemSettings::GetColour( wxSYS_COLOUR_WINDOW ) );

    bSizer171->Add( m_panelGraph, 1, wxALIGN_CENTER_VERTICAL|wxEXPAND, 5 );


    bSizer173->Add( bSizer171, 1, wxEXPAND|wxTOP|wxLEFT, 5 );


    m_panelProgress->SetSizer( bSizer173 );
    m_panelProgress->Layout();
    bSizer173->Fit( m_panelProgress );
    bSizerTop->Add( m_panelProgress, 1, wxEXPAND, 5 );

    bSizerFinalStat = new wxBoxSizer( wxVERTICAL );

    m_listbookResult = new wxListbook( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLB_TOP );
    wxSize m_listbookResultImageSize = wxSize( 180,1 );
    int m_listbookResultIndex = 0;
    wxImageList* m_listbookResultImages = new wxImageList( m_listbookResultImageSize.GetWidth(), m_listbookResultImageSize.GetHeight() );
    m_listbookResult->AssignImageList( m_listbookResultImages );
    wxBitmap m_listbookResultBitmap;
    wxImage m_listbookResultImage;

    bSizerFinalStat->Add( m_listbookResult, 1, wxEXPAND, 5 );


    bSizerTop->Add( bSizerFinalStat, 1, wxEXPAND, 5 );

    m_staticline12 = new wxStaticLine( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
    bSizerTop->Add( m_staticline12, 0, wxEXPAND, 5 );

    m_panelFooter = new wxPanel( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
    m_panelFooter->SetBackgroundColour( wxSystemSettings::GetColour( wxSYS_COLOUR_BTNFACE ) );

    wxBoxSizer* bSizer182;
    bSizer182 = new wxBoxSizer( wxVERTICAL );

    bSizerExecFinished = new wxBoxSizer( wxHORIZONTAL );

    m_staticText87 = new wxStaticText( m_panelFooter, wxID_ANY, _("On completion:"), wxDefaultPosition, wxDefaultSize, 0 );
    m_staticText87->Wrap( -1 );
    m_staticText87->SetFont( wxFont( wxNORMAL_FONT->GetPointSize(), 70, 90, 90, false, wxEmptyString ) );

    bSizerExecFinished->Add( m_staticText87, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );

    m_comboBoxExecFinished = new ExecFinishedBox( m_panelFooter, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0, NULL, 0 );
    bSizerExecFinished->Add( m_comboBoxExecFinished, 1, wxALIGN_CENTER_VERTICAL, 5 );


    bSizer182->Add( bSizerExecFinished, 0, wxTOP|wxRIGHT|wxLEFT|wxALIGN_CENTER_HORIZONTAL|wxEXPAND, 5 );

    bSizer28 = new wxBoxSizer( wxHORIZONTAL );


    bSizer28->Add( 0, 0, 1, 0, 5 );

    m_buttonOK = new wxButton( m_panelFooter, wxID_OK, _("OK"), wxDefaultPosition, wxSize( 100,30 ), 0 );
    m_buttonOK->SetFont( wxFont( 10, 70, 90, 90, false, wxEmptyString ) );
    m_buttonOK->Enable( false );

    bSizer28->Add( m_buttonOK, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );

    m_buttonPause = new wxButton( m_panelFooter, wxID_ANY, _("&Pause"), wxDefaultPosition, wxSize( 100,30 ), 0 );
    m_buttonPause->SetFont( wxFont( 10, 70, 90, 90, false, wxEmptyString ) );

    bSizer28->Add( m_buttonPause, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );

    m_buttonAbort = new wxButton( m_panelFooter, wxID_CANCEL, _("Cancel"), wxDefaultPosition, wxSize( 100,30 ), 0 );
    m_buttonAbort->SetFont( wxFont( 10, 70, 90, 90, false, wxEmptyString ) );

    bSizer28->Add( m_buttonAbort, 0, wxALIGN_CENTER_VERTICAL|wxTOP|wxBOTTOM|wxRIGHT, 5 );


    bSizer28->Add( 0, 0, 1, wxEXPAND, 5 );


    bSizer182->Add( bSizer28, 0, wxALIGN_CENTER_HORIZONTAL, 5 );


    m_panelFooter->SetSizer( bSizer182 );
    m_panelFooter->Layout();
    bSizer182->Fit( m_panelFooter );
    bSizerTop->Add( m_panelFooter, 0, wxEXPAND, 5 );


    this->SetSizer( bSizerTop );
    this->Layout();
    bSizerTop->Fit( this );

    this->Centre( wxBOTH );

    // Connect Events
    this->Connect( wxEVT_CLOSE_WINDOW, wxCloseEventHandler( SyncStatusDlgGenerated::OnClose ) );
    this->Connect( wxEVT_ICONIZE, wxIconizeEventHandler( SyncStatusDlgGenerated::OnIconize ) );
    m_buttonOK->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( SyncStatusDlgGenerated::OnOkay ), NULL, this );
    m_buttonPause->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( SyncStatusDlgGenerated::OnPause ), NULL, this );
    m_buttonAbort->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( SyncStatusDlgGenerated::OnAbort ), NULL, this );
}

SyncStatusDlgGenerated::~SyncStatusDlgGenerated()
{
    // Disconnect Events
    this->Disconnect( wxEVT_CLOSE_WINDOW, wxCloseEventHandler( SyncStatusDlgGenerated::OnClose ) );
    this->Disconnect( wxEVT_ICONIZE, wxIconizeEventHandler( SyncStatusDlgGenerated::OnIconize ) );
    m_buttonOK->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( SyncStatusDlgGenerated::OnOkay ), NULL, this );
    m_buttonPause->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( SyncStatusDlgGenerated::OnPause ), NULL, this );
    m_buttonAbort->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( SyncStatusDlgGenerated::OnAbort ), NULL, this );

}

LogControlGenerated::LogControlGenerated( wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style ) : wxPanel( parent, id, pos, size, style )
{
    this->SetBackgroundColour( wxSystemSettings::GetColour( wxSYS_COLOUR_BTNFACE ) );

    wxBoxSizer* bSizer153;
    bSizer153 = new wxBoxSizer( wxHORIZONTAL );

    wxBoxSizer* bSizer154;
    bSizer154 = new wxBoxSizer( wxVERTICAL );

    m_bpButtonErrors = new ToggleButton( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxSize( 48,48 ), wxBU_AUTODRAW );
    bSizer154->Add( m_bpButtonErrors, 0, wxALIGN_CENTER_HORIZONTAL, 5 );

    m_bpButtonWarnings = new ToggleButton( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxSize( 48,48 ), wxBU_AUTODRAW );
    bSizer154->Add( m_bpButtonWarnings, 0, wxALIGN_CENTER_HORIZONTAL, 5 );

    m_bpButtonInfo = new ToggleButton( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxSize( 48,48 ), wxBU_AUTODRAW );
    bSizer154->Add( m_bpButtonInfo, 0, wxALIGN_CENTER_HORIZONTAL, 5 );


    bSizer153->Add( bSizer154, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );

    m_staticline13 = new wxStaticLine( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_VERTICAL );
    bSizer153->Add( m_staticline13, 0, wxEXPAND, 5 );

    m_textCtrlInfo = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize( -1,-1 ), wxTE_DONTWRAP|wxTE_MULTILINE|wxTE_READONLY|wxNO_BORDER );
    bSizer153->Add( m_textCtrlInfo, 1, wxEXPAND|wxALIGN_CENTER_VERTICAL, 5 );


    this->SetSizer( bSizer153 );
    this->Layout();
    bSizer153->Fit( this );

    // Connect Events
    m_bpButtonErrors->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( LogControlGenerated::OnErrors ), NULL, this );
    m_bpButtonWarnings->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( LogControlGenerated::OnWarnings ), NULL, this );
    m_bpButtonInfo->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( LogControlGenerated::OnInfo ), NULL, this );
}

LogControlGenerated::~LogControlGenerated()
{
    // Disconnect Events
    m_bpButtonErrors->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( LogControlGenerated::OnErrors ), NULL, this );
    m_bpButtonWarnings->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( LogControlGenerated::OnWarnings ), NULL, this );
    m_bpButtonInfo->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( LogControlGenerated::OnInfo ), NULL, this );

}

AboutDlgGenerated::AboutDlgGenerated( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : wxDialog( parent, id, title, pos, size, style )
{
    this->SetSizeHints( wxDefaultSize, wxDefaultSize );

    wxBoxSizer* bSizer31;
    bSizer31 = new wxBoxSizer( wxVERTICAL );


    bSizer31->Add( 0, 5, 0, 0, 5 );

    wxBoxSizer* bSizer53;
    bSizer53 = new wxBoxSizer( wxVERTICAL );

    m_panelLogo = new wxPanel( this, wxID_ANY, wxDefaultPosition, wxSize( -1,-1 ), wxSIMPLE_BORDER|wxTAB_TRAVERSAL );
    m_panelLogo->SetBackgroundColour( wxColour( 255, 255, 255 ) );

    wxBoxSizer* bSizer36;
    bSizer36 = new wxBoxSizer( wxHORIZONTAL );

    m_bitmap11 = new wxStaticBitmap( m_panelLogo, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxSize( -1,-1 ), 0 );
    bSizer36->Add( m_bitmap11, 0, wxALIGN_CENTER_VERTICAL, 5 );


    m_panelLogo->SetSizer( bSizer36 );
    m_panelLogo->Layout();
    bSizer36->Fit( m_panelLogo );
    bSizer53->Add( m_panelLogo, 0, wxALIGN_CENTER_HORIZONTAL|wxEXPAND|wxTOP|wxBOTTOM, 5 );

    m_build = new wxStaticText( this, wxID_ANY, _("dummy"), wxDefaultPosition, wxDefaultSize, 0 );
    m_build->Wrap( -1 );
    bSizer53->Add( m_build, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxALL, 5 );


    bSizer53->Add( 0, 5, 0, 0, 5 );

    m_panel33 = new wxPanel( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxDOUBLE_BORDER|wxTAB_TRAVERSAL );
    m_panel33->SetBackgroundColour( wxSystemSettings::GetColour( wxSYS_COLOUR_WINDOW ) );

    bSizerCodeInfo = new wxBoxSizer( wxVERTICAL );

    m_staticText72 = new wxStaticText( m_panel33, wxID_ANY, _("Source code written in C++ utilizing:"), wxDefaultPosition, wxDefaultSize, 0 );
    m_staticText72->Wrap( -1 );
    m_staticText72->SetFont( wxFont( wxNORMAL_FONT->GetPointSize(), 70, 90, 92, false, wxEmptyString ) );

    bSizerCodeInfo->Add( m_staticText72, 0, wxTOP|wxBOTTOM|wxLEFT, 5 );

    wxBoxSizer* bSizer167;
    bSizer167 = new wxBoxSizer( wxVERTICAL );

    wxBoxSizer* bSizer171;
    bSizer171 = new wxBoxSizer( wxHORIZONTAL );

    m_hyperlink9 = new wxHyperlinkCtrl( m_panel33, wxID_ANY, _("MinGW"), wxT("http://www.mingw.org"), wxDefaultPosition, wxDefaultSize, wxHL_DEFAULT_STYLE );
    m_hyperlink9->SetBackgroundColour( wxSystemSettings::GetColour( wxSYS_COLOUR_WINDOW ) );

    bSizer171->Add( m_hyperlink9, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );

    m_hyperlink11 = new wxHyperlinkCtrl( m_panel33, wxID_ANY, _("MS Visual C++"), wxT("http://msdn.microsoft.com/library/60k1461a.aspx"), wxDefaultPosition, wxDefaultSize, wxHL_DEFAULT_STYLE );
    m_hyperlink11->SetBackgroundColour( wxSystemSettings::GetColour( wxSYS_COLOUR_WINDOW ) );

    bSizer171->Add( m_hyperlink11, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );

    m_hyperlink10 = new wxHyperlinkCtrl( m_panel33, wxID_ANY, _("Code::Blocks"), wxT("http://www.codeblocks.org"), wxDefaultPosition, wxDefaultSize, wxHL_DEFAULT_STYLE );
    m_hyperlink10->SetBackgroundColour( wxSystemSettings::GetColour( wxSYS_COLOUR_WINDOW ) );

    bSizer171->Add( m_hyperlink10, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );

    m_hyperlink13 = new wxHyperlinkCtrl( m_panel33, wxID_ANY, _("Boost"), wxT("http://www.boost.org"), wxDefaultPosition, wxDefaultSize, wxHL_DEFAULT_STYLE );
    m_hyperlink13->SetBackgroundColour( wxSystemSettings::GetColour( wxSYS_COLOUR_WINDOW ) );

    bSizer171->Add( m_hyperlink13, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );

    m_hyperlink7 = new wxHyperlinkCtrl( m_panel33, wxID_ANY, _("wxWidgets"), wxT("http://www.wxwidgets.org"), wxDefaultPosition, wxDefaultSize, wxHL_DEFAULT_STYLE );
    m_hyperlink7->SetBackgroundColour( wxSystemSettings::GetColour( wxSYS_COLOUR_WINDOW ) );

    bSizer171->Add( m_hyperlink7, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );

    m_hyperlink16 = new wxHyperlinkCtrl( m_panel33, wxID_ANY, _("Artistic Style"), wxT("http://astyle.sourceforge.net"), wxDefaultPosition, wxDefaultSize, wxHL_DEFAULT_STYLE );
    m_hyperlink16->SetBackgroundColour( wxSystemSettings::GetColour( wxSYS_COLOUR_WINDOW ) );

    bSizer171->Add( m_hyperlink16, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );


    bSizer167->Add( bSizer171, 0, wxALIGN_CENTER_HORIZONTAL|wxBOTTOM, 5 );

    wxBoxSizer* bSizer172;
    bSizer172 = new wxBoxSizer( wxHORIZONTAL );

    m_hyperlink8 = new wxHyperlinkCtrl( m_panel33, wxID_ANY, _("Loki"), wxT("http://loki-lib.sourceforge.net/"), wxDefaultPosition, wxDefaultSize, wxHL_DEFAULT_STYLE );
    m_hyperlink8->SetBackgroundColour( wxSystemSettings::GetColour( wxSYS_COLOUR_WINDOW ) );

    bSizer172->Add( m_hyperlink8, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );

    m_hyperlink15 = new wxHyperlinkCtrl( m_panel33, wxID_ANY, _("zenXML"), wxT("http://zenxml.sourceforge.net/"), wxDefaultPosition, wxDefaultSize, wxHL_DEFAULT_STYLE );
    m_hyperlink15->SetBackgroundColour( wxSystemSettings::GetColour( wxSYS_COLOUR_WINDOW ) );

    bSizer172->Add( m_hyperlink15, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );

    m_hyperlink12 = new wxHyperlinkCtrl( m_panel33, wxID_ANY, _("Google Test"), wxT("http://code.google.com/p/googletest"), wxDefaultPosition, wxDefaultSize, wxHL_DEFAULT_STYLE );
    m_hyperlink12->SetBackgroundColour( wxSystemSettings::GetColour( wxSYS_COLOUR_WINDOW ) );

    bSizer172->Add( m_hyperlink12, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );

    m_hyperlink18 = new wxHyperlinkCtrl( m_panel33, wxID_ANY, _("Unicode NSIS"), wxT("http://www.scratchpaper.com"), wxDefaultPosition, wxDefaultSize, wxHL_DEFAULT_STYLE );
    m_hyperlink18->SetBackgroundColour( wxSystemSettings::GetColour( wxSYS_COLOUR_WINDOW ) );

    bSizer172->Add( m_hyperlink18, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );

    m_hyperlink14 = new wxHyperlinkCtrl( m_panel33, wxID_ANY, _("wxFormBuilder"), wxT("http://wxformbuilder.org"), wxDefaultPosition, wxDefaultSize, wxHL_DEFAULT_STYLE );
    m_hyperlink14->SetBackgroundColour( wxSystemSettings::GetColour( wxSYS_COLOUR_WINDOW ) );

    bSizer172->Add( m_hyperlink14, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );


    bSizer167->Add( bSizer172, 0, wxALIGN_CENTER_HORIZONTAL, 5 );


    bSizerCodeInfo->Add( bSizer167, 0, wxALIGN_CENTER_HORIZONTAL, 5 );

    m_hyperlink21 = new wxHyperlinkCtrl( m_panel33, wxID_ANY, _("- ZenJu -"), wxT("mailto:zhnmju123@gmx.de"), wxDefaultPosition, wxDefaultSize, wxHL_DEFAULT_STYLE );
    m_hyperlink21->SetFont( wxFont( 10, 74, 93, 92, false, wxT("Segoe Print") ) );
    m_hyperlink21->SetBackgroundColour( wxSystemSettings::GetColour( wxSYS_COLOUR_WINDOW ) );
    m_hyperlink21->SetToolTip( _("zhnmju123@gmx.de") );

    bSizerCodeInfo->Add( m_hyperlink21, 0, wxALIGN_CENTER_HORIZONTAL, 5 );


    m_panel33->SetSizer( bSizerCodeInfo );
    m_panel33->Layout();
    bSizerCodeInfo->Fit( m_panel33 );
    bSizer53->Add( m_panel33, 0, wxBOTTOM|wxALIGN_CENTER_HORIZONTAL|wxEXPAND, 5 );

    m_panel40 = new wxPanel( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
    m_panel40->SetBackgroundColour( wxColour( 153, 170, 187 ) );

    wxBoxSizer* bSizer183;
    bSizer183 = new wxBoxSizer( wxVERTICAL );

    m_panel39 = new wxPanel( m_panel40, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
    m_panel39->SetBackgroundColour( wxColour( 221, 221, 255 ) );

    wxBoxSizer* bSizer184;
    bSizer184 = new wxBoxSizer( wxHORIZONTAL );

    wxBoxSizer* bSizer178;
    bSizer178 = new wxBoxSizer( wxVERTICAL );

    m_staticText83 = new wxStaticText( m_panel39, wxID_ANY, _("If you like FreeFileSync"), wxDefaultPosition, wxDefaultSize, 0 );
    m_staticText83->Wrap( -1 );
    m_staticText83->SetFont( wxFont( wxNORMAL_FONT->GetPointSize(), 70, 93, 92, false, wxEmptyString ) );
    m_staticText83->SetForegroundColour( wxColour( 0, 0, 0 ) );

    bSizer178->Add( m_staticText83, 0, wxALL, 5 );

    m_hyperlink3 = new wxHyperlinkCtrl( m_panel39, wxID_ANY, _("Donate with PayPal"), wxT("https://www.paypal.com/cgi-bin/webscr?cmd=_donations&business=zhnmju123@gmx.de&lc=US&currency_code=EUR"), wxDefaultPosition, wxDefaultSize, wxHL_DEFAULT_STYLE );
    m_hyperlink3->SetFont( wxFont( 10, 70, 90, 92, true, wxEmptyString ) );
    m_hyperlink3->SetBackgroundColour( wxColour( 221, 221, 255 ) );
    m_hyperlink3->SetToolTip( _("https://www.paypal.com/cgi-bin/webscr?cmd=_donations&business=zhnmju123@gmx.de&lc=US&currency_code=EUR") );

    bSizer178->Add( m_hyperlink3, 0, wxALIGN_CENTER_VERTICAL|wxALIGN_RIGHT|wxBOTTOM|wxRIGHT|wxLEFT, 5 );


    bSizer184->Add( bSizer178, 1, wxALIGN_CENTER_VERTICAL, 5 );

    m_bitmapPaypal = new wxStaticBitmap( m_panel39, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, 0 );
    m_bitmapPaypal->SetToolTip( _("Donate with PayPal") );

    bSizer184->Add( m_bitmapPaypal, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );


    m_panel39->SetSizer( bSizer184 );
    m_panel39->Layout();
    bSizer184->Fit( m_panel39 );
    bSizer183->Add( m_panel39, 0, wxEXPAND|wxALL, 5 );


    m_panel40->SetSizer( bSizer183 );
    m_panel40->Layout();
    bSizer183->Fit( m_panel40 );
    bSizer53->Add( m_panel40, 0, wxEXPAND|wxBOTTOM|wxALIGN_CENTER_HORIZONTAL, 5 );

    m_scrolledWindowTranslators = new wxScrolledWindow( this, wxID_ANY, wxDefaultPosition, wxSize( -1,-1 ), wxDOUBLE_BORDER|wxHSCROLL|wxVSCROLL );
    m_scrolledWindowTranslators->SetScrollRate( 5, 5 );
    m_scrolledWindowTranslators->SetBackgroundColour( wxSystemSettings::GetColour( wxSYS_COLOUR_WINDOW ) );
    m_scrolledWindowTranslators->SetMinSize( wxSize( -1,180 ) );

    bSizerTranslators = new wxBoxSizer( wxVERTICAL );

    m_staticText54 = new wxStaticText( m_scrolledWindowTranslators, wxID_ANY, _("Big thanks for localizing FreeFileSync goes out to:"), wxDefaultPosition, wxDefaultSize, 0 );
    m_staticText54->Wrap( -1 );
    m_staticText54->SetFont( wxFont( wxNORMAL_FONT->GetPointSize(), 70, 90, 92, false, wxEmptyString ) );

    bSizerTranslators->Add( m_staticText54, 0, wxALIGN_CENTER_HORIZONTAL|wxTOP|wxBOTTOM, 5 );


    bSizerTranslators->Add( 0, 5, 0, 0, 5 );

    fgSizerTranslators = new wxFlexGridSizer( 50, 3, 2, 20 );
    fgSizerTranslators->SetFlexibleDirection( wxBOTH );
    fgSizerTranslators->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );


    bSizerTranslators->Add( fgSizerTranslators, 0, wxALIGN_CENTER_HORIZONTAL, 5 );


    m_scrolledWindowTranslators->SetSizer( bSizerTranslators );
    m_scrolledWindowTranslators->Layout();
    bSizerTranslators->Fit( m_scrolledWindowTranslators );
    bSizer53->Add( m_scrolledWindowTranslators, 0, wxALIGN_CENTER_HORIZONTAL|wxBOTTOM|wxEXPAND, 5 );

    wxStaticBoxSizer* sbSizer29;
    sbSizer29 = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Feedback and suggestions are welcome") ), wxHORIZONTAL );

    wxBoxSizer* bSizer170;
    bSizer170 = new wxBoxSizer( wxHORIZONTAL );


    bSizer170->Add( 0, 0, 1, wxEXPAND, 5 );

    m_hyperlink1 = new wxHyperlinkCtrl( this, wxID_ANY, _("Homepage"), wxT("http://sourceforge.net/projects/freefilesync/"), wxDefaultPosition, wxDefaultSize, wxHL_DEFAULT_STYLE );
    m_hyperlink1->SetFont( wxFont( 10, 70, 90, 92, true, wxEmptyString ) );
    m_hyperlink1->SetToolTip( _("http://sourceforge.net/projects/freefilesync/") );

    bSizer170->Add( m_hyperlink1, 0, wxALIGN_CENTER_VERTICAL, 5 );

    m_bitmap9 = new wxStaticBitmap( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxSize( -1,-1 ), 0 );
    m_bitmap9->SetToolTip( _("FreeFileSync at Sourceforge") );

    bSizer170->Add( m_bitmap9, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );


    bSizer170->Add( 0, 0, 1, wxEXPAND, 5 );


    sbSizer29->Add( bSizer170, 1, wxALIGN_CENTER_VERTICAL, 5 );

    wxBoxSizer* bSizer1711;
    bSizer1711 = new wxBoxSizer( wxHORIZONTAL );


    bSizer1711->Add( 0, 0, 1, wxEXPAND, 5 );

    m_hyperlink2 = new wxHyperlinkCtrl( this, wxID_ANY, _("Email"), wxT("mailto:zhnmju123@gmx.de"), wxDefaultPosition, wxDefaultSize, wxHL_DEFAULT_STYLE );
    m_hyperlink2->SetFont( wxFont( 10, 70, 90, 92, true, wxEmptyString ) );
    m_hyperlink2->SetToolTip( _("zhnmju123@gmx.de") );

    bSizer1711->Add( m_hyperlink2, 0, wxALIGN_CENTER_VERTICAL, 5 );

    m_bitmap10 = new wxStaticBitmap( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxSize( -1,-1 ), 0 );
    m_bitmap10->SetToolTip( _("Email") );

    bSizer1711->Add( m_bitmap10, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );


    bSizer1711->Add( 0, 0, 1, wxEXPAND, 5 );


    sbSizer29->Add( bSizer1711, 1, wxALIGN_CENTER_VERTICAL, 5 );


    bSizer53->Add( sbSizer29, 0, wxEXPAND|wxALIGN_CENTER_HORIZONTAL|wxBOTTOM, 5 );

    wxStaticBoxSizer* sbSizer14;
    sbSizer14 = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Published under the GNU General Public License") ), wxHORIZONTAL );


    sbSizer14->Add( 0, 0, 1, wxEXPAND|wxALIGN_CENTER_VERTICAL, 5 );

    m_bitmap13 = new wxStaticBitmap( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxSize( -1,-1 ), 0 );
    sbSizer14->Add( m_bitmap13, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );

    m_hyperlink5 = new wxHyperlinkCtrl( this, wxID_ANY, _("http://www.gnu.org/licenses/gpl.html"), wxT("http://www.gnu.org/licenses/gpl.html"), wxDefaultPosition, wxDefaultSize, wxHL_DEFAULT_STYLE );
    sbSizer14->Add( m_hyperlink5, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 5 );


    sbSizer14->Add( 0, 0, 1, wxEXPAND|wxALIGN_CENTER_VERTICAL, 5 );


    bSizer53->Add( sbSizer14, 0, wxALIGN_CENTER_HORIZONTAL|wxEXPAND, 5 );


    bSizer31->Add( bSizer53, 0, wxALIGN_CENTER_HORIZONTAL|wxEXPAND|wxRIGHT|wxLEFT, 25 );

    m_buttonOkay = new wxButton( this, wxID_OK, _("OK"), wxDefaultPosition, wxSize( 100,30 ), 0 );
    m_buttonOkay->SetDefault();
    m_buttonOkay->SetFont( wxFont( 10, 70, 90, 90, false, wxEmptyString ) );

    bSizer31->Add( m_buttonOkay, 0, wxALIGN_CENTER_HORIZONTAL|wxALL, 5 );


    this->SetSizer( bSizer31 );
    this->Layout();
    bSizer31->Fit( this );

    this->Centre( wxBOTH );

    // Connect Events
    this->Connect( wxEVT_CLOSE_WINDOW, wxCloseEventHandler( AboutDlgGenerated::OnClose ) );
    m_buttonOkay->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( AboutDlgGenerated::OnOK ), NULL, this );
}

AboutDlgGenerated::~AboutDlgGenerated()
{
    // Disconnect Events
    this->Disconnect( wxEVT_CLOSE_WINDOW, wxCloseEventHandler( AboutDlgGenerated::OnClose ) );
    m_buttonOkay->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( AboutDlgGenerated::OnOK ), NULL, this );

}

MessageDlgGenerated::MessageDlgGenerated( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : wxDialog( parent, id, title, pos, size, style )
{
    this->SetSizeHints( wxSize( 300,160 ), wxDefaultSize );
    this->SetBackgroundColour( wxSystemSettings::GetColour( wxSYS_COLOUR_WINDOW ) );

    wxBoxSizer* bSizer24;
    bSizer24 = new wxBoxSizer( wxVERTICAL );


    bSizer24->Add( 0, 10, 0, wxEXPAND, 5 );

    wxBoxSizer* bSizer26;
    bSizer26 = new wxBoxSizer( wxHORIZONTAL );

    m_bitmapMsgType = new wxStaticBitmap( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxSize( -1,-1 ), 0 );
    bSizer26->Add( m_bitmapMsgType, 0, wxBOTTOM|wxRIGHT|wxLEFT, 5 );

    m_textCtrlMessage = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize( 400,130 ), wxTE_MULTILINE|wxTE_READONLY|wxNO_BORDER );
    bSizer26->Add( m_textCtrlMessage, 1, wxALIGN_CENTER_VERTICAL|wxEXPAND, 5 );


    bSizer24->Add( bSizer26, 1, wxALIGN_CENTER_HORIZONTAL|wxEXPAND, 5 );

    m_staticline6 = new wxStaticLine( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
    bSizer24->Add( m_staticline6, 0, wxEXPAND|wxALIGN_CENTER_HORIZONTAL|wxTOP, 5 );

    m_panel33 = new wxPanel( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
    m_panel33->SetBackgroundColour( wxSystemSettings::GetColour( wxSYS_COLOUR_BTNFACE ) );

    wxBoxSizer* bSizer177;
    bSizer177 = new wxBoxSizer( wxVERTICAL );

    m_checkBoxCustom = new wxCheckBox( m_panel33, wxID_ANY, _("dummy"), wxDefaultPosition, wxDefaultSize, 0 );
    bSizer177->Add( m_checkBoxCustom, 0, wxALIGN_CENTER_HORIZONTAL|wxTOP, 5 );

    wxBoxSizer* bSizer25;
    bSizer25 = new wxBoxSizer( wxHORIZONTAL );

    m_buttonCustom1 = new wxButton( m_panel33, wxID_ANY, _("dummy"), wxDefaultPosition, wxSize( -1,30 ), 0 );
    m_buttonCustom1->SetDefault();
    m_buttonCustom1->SetFont( wxFont( 10, 70, 90, 90, false, wxEmptyString ) );

    bSizer25->Add( m_buttonCustom1, 0, wxTOP|wxBOTTOM|wxLEFT|wxALIGN_CENTER_VERTICAL, 5 );

    m_buttonCustom2 = new wxButton( m_panel33, wxID_ANY, _("dummy"), wxDefaultPosition, wxSize( -1,30 ), 0 );
    m_buttonCustom2->SetFont( wxFont( 10, 70, 90, 90, false, wxEmptyString ) );

    bSizer25->Add( m_buttonCustom2, 0, wxTOP|wxBOTTOM|wxLEFT|wxALIGN_CENTER_VERTICAL, 5 );

    m_buttonCancel = new wxButton( m_panel33, wxID_CANCEL, _("Cancel"), wxDefaultPosition, wxSize( -1,30 ), 0 );
    m_buttonCancel->SetFont( wxFont( 10, 70, 90, 90, false, wxEmptyString ) );

    bSizer25->Add( m_buttonCancel, 0, wxALIGN_CENTER_VERTICAL|wxTOP|wxBOTTOM|wxLEFT, 5 );


    bSizer177->Add( bSizer25, 0, wxALIGN_CENTER_HORIZONTAL, 5 );


    m_panel33->SetSizer( bSizer177 );
    m_panel33->Layout();
    bSizer177->Fit( m_panel33 );
    bSizer24->Add( m_panel33, 0, wxEXPAND, 5 );


    this->SetSizer( bSizer24 );
    this->Layout();
    bSizer24->Fit( this );

    this->Centre( wxBOTH );

    // Connect Events
    this->Connect( wxEVT_CLOSE_WINDOW, wxCloseEventHandler( MessageDlgGenerated::OnClose ) );
    m_buttonCustom1->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( MessageDlgGenerated::OnButton1 ), NULL, this );
    m_buttonCustom2->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( MessageDlgGenerated::OnButton2 ), NULL, this );
    m_buttonCancel->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( MessageDlgGenerated::OnCancel ), NULL, this );
}

MessageDlgGenerated::~MessageDlgGenerated()
{
    // Disconnect Events
    this->Disconnect( wxEVT_CLOSE_WINDOW, wxCloseEventHandler( MessageDlgGenerated::OnClose ) );
    m_buttonCustom1->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( MessageDlgGenerated::OnButton1 ), NULL, this );
    m_buttonCustom2->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( MessageDlgGenerated::OnButton2 ), NULL, this );
    m_buttonCancel->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( MessageDlgGenerated::OnCancel ), NULL, this );

}

DeleteDlgGenerated::DeleteDlgGenerated( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : wxDialog( parent, id, title, pos, size, style )
{
    this->SetSizeHints( wxSize( 300,180 ), wxDefaultSize );
    this->SetBackgroundColour( wxSystemSettings::GetColour( wxSYS_COLOUR_WINDOW ) );

    wxBoxSizer* bSizer24;
    bSizer24 = new wxBoxSizer( wxVERTICAL );

    m_panelHeader = new wxPanel( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
    m_panelHeader->SetBackgroundColour( wxSystemSettings::GetColour( wxSYS_COLOUR_BTNFACE ) );

    wxBoxSizer* bSizer181;
    bSizer181 = new wxBoxSizer( wxVERTICAL );

    wxBoxSizer* bSizer41;
    bSizer41 = new wxBoxSizer( wxHORIZONTAL );

    m_bitmap12 = new wxStaticBitmap( m_panelHeader, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxSize( -1,-1 ), 0 );
    bSizer41->Add( m_bitmap12, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );

    m_staticTextHeader = new wxStaticText( m_panelHeader, wxID_ANY, _("dummy"), wxDefaultPosition, wxDefaultSize, 0 );
    m_staticTextHeader->Wrap( -1 );
    m_staticTextHeader->SetFont( wxFont( 10, 70, 90, 90, false, wxEmptyString ) );

    bSizer41->Add( m_staticTextHeader, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 5 );


    bSizer181->Add( bSizer41, 0, wxALIGN_CENTER_HORIZONTAL|wxALL, 5 );


    m_panelHeader->SetSizer( bSizer181 );
    m_panelHeader->Layout();
    bSizer181->Fit( m_panelHeader );
    bSizer24->Add( m_panelHeader, 0, wxALIGN_CENTER_HORIZONTAL|wxEXPAND, 5 );

    m_staticline91 = new wxStaticLine( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
    bSizer24->Add( m_staticline91, 0, wxEXPAND|wxBOTTOM, 5 );

    m_textCtrlFileList = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize( 550,200 ), wxTE_DONTWRAP|wxTE_MULTILINE|wxTE_READONLY|wxNO_BORDER );
    bSizer24->Add( m_textCtrlFileList, 1, wxEXPAND|wxLEFT, 5 );

    m_staticline9 = new wxStaticLine( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
    bSizer24->Add( m_staticline9, 0, wxEXPAND|wxTOP, 5 );

    m_panel36 = new wxPanel( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
    m_panel36->SetBackgroundColour( wxSystemSettings::GetColour( wxSYS_COLOUR_BTNFACE ) );

    wxBoxSizer* bSizer180;
    bSizer180 = new wxBoxSizer( wxVERTICAL );

    wxBoxSizer* bSizer179;
    bSizer179 = new wxBoxSizer( wxHORIZONTAL );

    wxBoxSizer* bSizer99;
    bSizer99 = new wxBoxSizer( wxVERTICAL );

    m_checkBoxUseRecycler = new wxCheckBox( m_panel36, wxID_ANY, _("Use Recycle Bin"), wxDefaultPosition, wxDefaultSize, 0 );
    bSizer99->Add( m_checkBoxUseRecycler, 0, wxALIGN_CENTER_VERTICAL|wxBOTTOM, 5 );

    m_checkBoxDeleteBothSides = new wxCheckBox( m_panel36, wxID_ANY, _("Delete on both sides"), wxDefaultPosition, wxDefaultSize, 0 );
    m_checkBoxDeleteBothSides->SetToolTip( _("Delete on both sides even if the file is selected on one side only") );

    bSizer99->Add( m_checkBoxDeleteBothSides, 0, wxALIGN_CENTER_VERTICAL, 5 );


    bSizer179->Add( bSizer99, 0, wxALIGN_CENTER_HORIZONTAL|wxALL|wxALIGN_CENTER_VERTICAL, 5 );


    bSizer179->Add( 0, 0, 1, wxEXPAND, 5 );

    wxBoxSizer* bSizer25;
    bSizer25 = new wxBoxSizer( wxHORIZONTAL );

    m_buttonOK = new wxButton( m_panel36, wxID_OK, _("OK"), wxDefaultPosition, wxSize( -1,30 ), 0 );
    m_buttonOK->SetDefault();
    m_buttonOK->SetFont( wxFont( 10, 70, 90, 92, false, wxEmptyString ) );

    bSizer25->Add( m_buttonOK, 0, wxTOP|wxBOTTOM|wxLEFT, 5 );

    m_buttonCancel = new wxButton( m_panel36, wxID_CANCEL, _("Cancel"), wxDefaultPosition, wxSize( -1,30 ), 0 );
    m_buttonCancel->SetFont( wxFont( 10, 70, 90, 90, false, wxEmptyString ) );

    bSizer25->Add( m_buttonCancel, 0, wxALL, 5 );


    bSizer179->Add( bSizer25, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 5 );


    bSizer180->Add( bSizer179, 0, wxEXPAND, 5 );


    m_panel36->SetSizer( bSizer180 );
    m_panel36->Layout();
    bSizer180->Fit( m_panel36 );
    bSizer24->Add( m_panel36, 0, wxEXPAND, 5 );


    this->SetSizer( bSizer24 );
    this->Layout();
    bSizer24->Fit( this );

    this->Centre( wxBOTH );

    // Connect Events
    this->Connect( wxEVT_CLOSE_WINDOW, wxCloseEventHandler( DeleteDlgGenerated::OnClose ) );
    m_checkBoxUseRecycler->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DeleteDlgGenerated::OnUseRecycler ), NULL, this );
    m_checkBoxDeleteBothSides->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DeleteDlgGenerated::OnDelOnBothSides ), NULL, this );
    m_buttonOK->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DeleteDlgGenerated::OnOK ), NULL, this );
    m_buttonCancel->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DeleteDlgGenerated::OnCancel ), NULL, this );
}

DeleteDlgGenerated::~DeleteDlgGenerated()
{
    // Disconnect Events
    this->Disconnect( wxEVT_CLOSE_WINDOW, wxCloseEventHandler( DeleteDlgGenerated::OnClose ) );
    m_checkBoxUseRecycler->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DeleteDlgGenerated::OnUseRecycler ), NULL, this );
    m_checkBoxDeleteBothSides->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DeleteDlgGenerated::OnDelOnBothSides ), NULL, this );
    m_buttonOK->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DeleteDlgGenerated::OnOK ), NULL, this );
    m_buttonCancel->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DeleteDlgGenerated::OnCancel ), NULL, this );

}

FilterDlgGenerated::FilterDlgGenerated( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : wxDialog( parent, id, title, pos, size, style )
{
    this->SetSizeHints( wxSize( 500,380 ), wxDefaultSize );

    wxBoxSizer* bSizer21;
    bSizer21 = new wxBoxSizer( wxVERTICAL );

    wxBoxSizer* bSizer70;
    bSizer70 = new wxBoxSizer( wxHORIZONTAL );

    bSizer70->SetMinSize( wxSize( 550,-1 ) );

    bSizer70->Add( 0, 0, 1, wxEXPAND|wxALIGN_CENTER_VERTICAL, 5 );

    m_bitmap26 = new wxStaticBitmap( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxSize( 40,40 ), 0 );
    bSizer70->Add( m_bitmap26, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );

    m_panel8 = new wxPanel( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxSIMPLE_BORDER|wxTAB_TRAVERSAL );
    m_panel8->SetBackgroundColour( wxSystemSettings::GetColour( wxSYS_COLOUR_3DLIGHT ) );

    wxBoxSizer* bSizer72;
    bSizer72 = new wxBoxSizer( wxVERTICAL );

    m_staticTexHeader = new wxStaticText( m_panel8, wxID_ANY, _("dummy"), wxDefaultPosition, wxDefaultSize, 0 );
    m_staticTexHeader->Wrap( -1 );
    m_staticTexHeader->SetFont( wxFont( 14, 70, 90, 92, false, wxEmptyString ) );
    m_staticTexHeader->SetForegroundColour( wxColour( 0, 0, 0 ) );

    bSizer72->Add( m_staticTexHeader, 0, wxALIGN_CENTER_HORIZONTAL|wxALL, 5 );


    m_panel8->SetSizer( bSizer72 );
    m_panel8->Layout();
    bSizer72->Fit( m_panel8 );
    bSizer70->Add( m_panel8, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );

    m_staticText44 = new wxStaticText( this, wxID_ANY, _("Only files that match all filter settings will be synchronized.\nNote: File names must be relative to base directories!"), wxDefaultPosition, wxSize( -1,-1 ), 0 );
    m_staticText44->Wrap( -1 );
    bSizer70->Add( m_staticText44, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 5 );

    m_bpButtonHelp = new wxBitmapButton( this, wxID_HELP, wxNullBitmap, wxDefaultPosition, wxSize( 40,40 ), wxBU_AUTODRAW );
    m_bpButtonHelp->SetToolTip( _("Help") );

    bSizer70->Add( m_bpButtonHelp, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );


    bSizer70->Add( 0, 0, 1, wxALIGN_CENTER_VERTICAL|wxEXPAND, 5 );


    bSizer21->Add( bSizer70, 0, wxALIGN_CENTER_HORIZONTAL|wxTOP|wxRIGHT|wxLEFT|wxEXPAND, 5 );

    wxBoxSizer* bSizer159;
    bSizer159 = new wxBoxSizer( wxHORIZONTAL );

    wxBoxSizer* bSizer166;
    bSizer166 = new wxBoxSizer( wxVERTICAL );

    wxStaticBoxSizer* sbSizer8;
    sbSizer8 = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Include") ), wxHORIZONTAL );

    m_bitmapInclude = new wxStaticBitmap( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxSize( 30,30 ), 0 );
    sbSizer8->Add( m_bitmapInclude, 0, wxALIGN_CENTER_VERTICAL|wxALIGN_CENTER_HORIZONTAL|wxRIGHT, 5 );

    m_textCtrlInclude = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize( -1,-1 ), wxTE_MULTILINE );
    sbSizer8->Add( m_textCtrlInclude, 1, wxALIGN_CENTER_VERTICAL|wxALIGN_CENTER_HORIZONTAL|wxEXPAND, 5 );


    bSizer166->Add( sbSizer8, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxEXPAND, 5 );

    wxStaticBoxSizer* sbSizer26;
    sbSizer26 = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Exclude") ), wxHORIZONTAL );

    m_bitmapExclude = new wxStaticBitmap( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxSize( 30,30 ), 0 );
    sbSizer26->Add( m_bitmapExclude, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );

    m_textCtrlExclude = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize( -1,-1 ), wxTE_MULTILINE );
    sbSizer26->Add( m_textCtrlExclude, 1, wxALIGN_CENTER_VERTICAL|wxEXPAND|wxALIGN_CENTER_HORIZONTAL, 5 );


    bSizer166->Add( sbSizer26, 1, wxALIGN_CENTER_HORIZONTAL|wxEXPAND, 5 );


    bSizer159->Add( bSizer166, 1, wxALIGN_CENTER_VERTICAL|wxEXPAND, 5 );


    bSizer159->Add( 5, 0, 0, 0, 5 );

    wxBoxSizer* bSizer160;
    bSizer160 = new wxBoxSizer( wxVERTICAL );

    wxStaticBoxSizer* sbSizer25;
    sbSizer25 = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Time span") ), wxHORIZONTAL );

    wxBoxSizer* bSizer169;
    bSizer169 = new wxBoxSizer( wxHORIZONTAL );

    m_bitmapFilterDate = new wxStaticBitmap( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxSize( 34,34 ), 0 );
    bSizer169->Add( m_bitmapFilterDate, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );

    wxBoxSizer* bSizer165;
    bSizer165 = new wxBoxSizer( wxVERTICAL );

    wxBoxSizer* bSizer164;
    bSizer164 = new wxBoxSizer( wxVERTICAL );

    m_spinCtrlTimespan = new wxSpinCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 0, 2000000000, 0 );
    bSizer164->Add( m_spinCtrlTimespan, 0, wxALIGN_CENTER_VERTICAL, 5 );

    wxArrayString m_choiceUnitTimespanChoices;
    m_choiceUnitTimespan = new wxChoice( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, m_choiceUnitTimespanChoices, 0 );
    m_choiceUnitTimespan->SetSelection( 0 );
    bSizer164->Add( m_choiceUnitTimespan, 0, wxALIGN_CENTER_VERTICAL, 5 );


    bSizer165->Add( bSizer164, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 5 );


    bSizer169->Add( bSizer165, 0, wxALIGN_CENTER_VERTICAL, 5 );


    sbSizer25->Add( bSizer169, 0, 0, 5 );


    bSizer160->Add( sbSizer25, 0, wxALIGN_CENTER_HORIZONTAL|wxEXPAND, 5 );

    wxStaticBoxSizer* sbSizer81;
    sbSizer81 = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("File size") ), wxHORIZONTAL );

    wxBoxSizer* bSizer170;
    bSizer170 = new wxBoxSizer( wxHORIZONTAL );

    m_bitmapFilterSize = new wxStaticBitmap( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxSize( 32,32 ), 0 );
    bSizer170->Add( m_bitmapFilterSize, 0, wxALIGN_CENTER_VERTICAL|wxALIGN_CENTER_HORIZONTAL|wxRIGHT, 5 );

    wxBoxSizer* bSizer158;
    bSizer158 = new wxBoxSizer( wxVERTICAL );

    wxBoxSizer* bSizer162;
    bSizer162 = new wxBoxSizer( wxVERTICAL );

    m_staticText101 = new wxStaticText( this, wxID_ANY, _("Minimum"), wxDefaultPosition, wxDefaultSize, 0 );
    m_staticText101->Wrap( -1 );
    bSizer162->Add( m_staticText101, 0, wxBOTTOM, 2 );

    m_spinCtrlMinSize = new wxSpinCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 0, 2000000000, 0 );
    bSizer162->Add( m_spinCtrlMinSize, 0, wxALIGN_CENTER_VERTICAL, 5 );

    wxArrayString m_choiceUnitMinSizeChoices;
    m_choiceUnitMinSize = new wxChoice( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, m_choiceUnitMinSizeChoices, 0 );
    m_choiceUnitMinSize->SetSelection( 0 );
    bSizer162->Add( m_choiceUnitMinSize, 0, wxALIGN_CENTER_VERTICAL, 5 );


    bSizer158->Add( bSizer162, 0, wxBOTTOM, 5 );

    wxBoxSizer* bSizer163;
    bSizer163 = new wxBoxSizer( wxVERTICAL );

    m_staticText102 = new wxStaticText( this, wxID_ANY, _("Maximum"), wxDefaultPosition, wxDefaultSize, 0 );
    m_staticText102->Wrap( -1 );
    bSizer163->Add( m_staticText102, 0, wxBOTTOM, 2 );

    m_spinCtrlMaxSize = new wxSpinCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 0, 2000000000, 0 );
    bSizer163->Add( m_spinCtrlMaxSize, 0, wxALIGN_CENTER_VERTICAL, 5 );

    wxArrayString m_choiceUnitMaxSizeChoices;
    m_choiceUnitMaxSize = new wxChoice( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, m_choiceUnitMaxSizeChoices, 0 );
    m_choiceUnitMaxSize->SetSelection( 0 );
    bSizer163->Add( m_choiceUnitMaxSize, 0, wxALIGN_CENTER_VERTICAL, 5 );


    bSizer158->Add( bSizer163, 0, 0, 5 );


    bSizer170->Add( bSizer158, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 5 );


    sbSizer81->Add( bSizer170, 0, 0, 5 );


    bSizer160->Add( sbSizer81, 1, wxALIGN_CENTER_HORIZONTAL|wxEXPAND, 5 );


    bSizer159->Add( bSizer160, 0, wxEXPAND, 5 );


    bSizer21->Add( bSizer159, 1, wxALIGN_CENTER_HORIZONTAL|wxEXPAND|wxTOP|wxRIGHT|wxLEFT, 5 );

    wxBoxSizer* bSizer22;
    bSizer22 = new wxBoxSizer( wxHORIZONTAL );

    m_button9 = new wxButton( this, wxID_DEFAULT, _("&Default"), wxDefaultPosition, wxSize( -1,30 ), 0 );
    m_button9->SetFont( wxFont( 10, 70, 90, 90, false, wxEmptyString ) );

    bSizer22->Add( m_button9, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );


    bSizer22->Add( 0, 0, 1, wxALIGN_CENTER_VERTICAL, 5 );

    m_buttonOk = new wxButton( this, wxID_OK, _("OK"), wxDefaultPosition, wxSize( -1,30 ), 0 );
    m_buttonOk->SetDefault();
    m_buttonOk->SetFont( wxFont( 10, 70, 90, 92, false, wxEmptyString ) );

    bSizer22->Add( m_buttonOk, 0, wxALIGN_CENTER_VERTICAL|wxTOP|wxBOTTOM|wxLEFT, 5 );

    m_button17 = new wxButton( this, wxID_CANCEL, _("Cancel"), wxDefaultPosition, wxSize( -1,30 ), 0 );
    m_button17->SetFont( wxFont( 10, 70, 90, 90, false, wxEmptyString ) );

    bSizer22->Add( m_button17, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );


    bSizer21->Add( bSizer22, 0, wxALIGN_CENTER_HORIZONTAL|wxTOP|wxBOTTOM|wxEXPAND, 5 );


    this->SetSizer( bSizer21 );
    this->Layout();
    bSizer21->Fit( this );

    this->Centre( wxBOTH );

    // Connect Events
    this->Connect( wxEVT_CLOSE_WINDOW, wxCloseEventHandler( FilterDlgGenerated::OnClose ) );
    m_bpButtonHelp->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( FilterDlgGenerated::OnHelp ), NULL, this );
    m_textCtrlInclude->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( FilterDlgGenerated::OnUpdateNameFilter ), NULL, this );
    m_textCtrlExclude->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( FilterDlgGenerated::OnUpdateNameFilter ), NULL, this );
    m_choiceUnitTimespan->Connect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( FilterDlgGenerated::OnUpdateChoice ), NULL, this );
    m_choiceUnitMinSize->Connect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( FilterDlgGenerated::OnUpdateChoice ), NULL, this );
    m_choiceUnitMaxSize->Connect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( FilterDlgGenerated::OnUpdateChoice ), NULL, this );
    m_button9->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( FilterDlgGenerated::OnDefault ), NULL, this );
    m_buttonOk->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( FilterDlgGenerated::OnApply ), NULL, this );
    m_button17->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( FilterDlgGenerated::OnCancel ), NULL, this );
}

FilterDlgGenerated::~FilterDlgGenerated()
{
    // Disconnect Events
    this->Disconnect( wxEVT_CLOSE_WINDOW, wxCloseEventHandler( FilterDlgGenerated::OnClose ) );
    m_bpButtonHelp->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( FilterDlgGenerated::OnHelp ), NULL, this );
    m_textCtrlInclude->Disconnect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( FilterDlgGenerated::OnUpdateNameFilter ), NULL, this );
    m_textCtrlExclude->Disconnect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( FilterDlgGenerated::OnUpdateNameFilter ), NULL, this );
    m_choiceUnitTimespan->Disconnect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( FilterDlgGenerated::OnUpdateChoice ), NULL, this );
    m_choiceUnitMinSize->Disconnect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( FilterDlgGenerated::OnUpdateChoice ), NULL, this );
    m_choiceUnitMaxSize->Disconnect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( FilterDlgGenerated::OnUpdateChoice ), NULL, this );
    m_button9->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( FilterDlgGenerated::OnDefault ), NULL, this );
    m_buttonOk->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( FilterDlgGenerated::OnApply ), NULL, this );
    m_button17->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( FilterDlgGenerated::OnCancel ), NULL, this );

}

GlobalSettingsDlgGenerated::GlobalSettingsDlgGenerated( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : wxDialog( parent, id, title, pos, size, style )
{
    this->SetSizeHints( wxSize( 300,360 ), wxDefaultSize );

    wxBoxSizer* bSizer95;
    bSizer95 = new wxBoxSizer( wxVERTICAL );

    wxBoxSizer* bSizer86;
    bSizer86 = new wxBoxSizer( wxHORIZONTAL );

    m_bitmapSettings = new wxStaticBitmap( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxSize( 40,40 ), 0 );
    bSizer86->Add( m_bitmapSettings, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );

    m_panel8 = new wxPanel( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxSIMPLE_BORDER|wxTAB_TRAVERSAL );
    m_panel8->SetBackgroundColour( wxSystemSettings::GetColour( wxSYS_COLOUR_3DLIGHT ) );

    wxBoxSizer* bSizer72;
    bSizer72 = new wxBoxSizer( wxVERTICAL );

    m_staticText56 = new wxStaticText( m_panel8, wxID_ANY, _("Global settings"), wxDefaultPosition, wxDefaultSize, 0 );
    m_staticText56->Wrap( -1 );
    m_staticText56->SetFont( wxFont( 14, 70, 90, 92, false, wxEmptyString ) );
    m_staticText56->SetForegroundColour( wxColour( 0, 0, 0 ) );

    bSizer72->Add( m_staticText56, 0, wxALIGN_CENTER_HORIZONTAL|wxALL, 5 );


    m_panel8->SetSizer( bSizer72 );
    m_panel8->Layout();
    bSizer72->Fit( m_panel8 );
    bSizer86->Add( m_panel8, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 5 );


    bSizer95->Add( bSizer86, 0, wxALIGN_CENTER_HORIZONTAL|wxTOP|wxRIGHT|wxLEFT, 5 );

    wxStaticBoxSizer* sbSizer23;
    sbSizer23 = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, wxEmptyString ), wxVERTICAL );

    m_checkBoxTransCopy = new wxCheckBox( this, wxID_ANY, _("Fail-safe file copy"), wxDefaultPosition, wxDefaultSize, 0 );
    sbSizer23->Add( m_checkBoxTransCopy, 0, wxEXPAND|wxALL, 5 );

    m_staticText82 = new wxStaticText( this, wxID_ANY, _("Write to a temporary file (*.ffs_tmp) first then rename it. This guarantees a consistent state even in case of fatal error."), wxDefaultPosition, wxDefaultSize, 0 );
    m_staticText82->Wrap( 420 );
    m_staticText82->SetForegroundColour( wxSystemSettings::GetColour( wxSYS_COLOUR_GRAYTEXT ) );

    sbSizer23->Add( m_staticText82, 0, wxLEFT, 20 );

    m_checkBoxCopyLocked = new wxCheckBox( this, wxID_ANY, _("Copy locked files"), wxDefaultPosition, wxDefaultSize, 0 );
    sbSizer23->Add( m_checkBoxCopyLocked, 0, wxALL|wxEXPAND, 5 );

    m_staticTextCopyLocked = new wxStaticText( this, wxID_ANY, _("Copy shared or locked files using Volume Shadow Copy Service (Requires Administrator rights)"), wxDefaultPosition, wxDefaultSize, 0 );
    m_staticTextCopyLocked->Wrap( 420 );
    m_staticTextCopyLocked->SetForegroundColour( wxSystemSettings::GetColour( wxSYS_COLOUR_GRAYTEXT ) );

    sbSizer23->Add( m_staticTextCopyLocked, 0, wxLEFT|wxEXPAND, 20 );

    m_checkBoxCopyPermissions = new wxCheckBox( this, wxID_ANY, _("Copy file access permissions"), wxDefaultPosition, wxDefaultSize, 0 );
    sbSizer23->Add( m_checkBoxCopyPermissions, 0, wxALL|wxEXPAND, 5 );

    m_staticText8211 = new wxStaticText( this, wxID_ANY, _("Transfer file and folder permissions (Requires Administrator rights)"), wxDefaultPosition, wxDefaultSize, 0 );
    m_staticText8211->Wrap( 420 );
    m_staticText8211->SetForegroundColour( wxSystemSettings::GetColour( wxSYS_COLOUR_GRAYTEXT ) );

    sbSizer23->Add( m_staticText8211, 0, wxLEFT|wxEXPAND, 20 );


    bSizer95->Add( sbSizer23, 0, wxALIGN_CENTER_HORIZONTAL|wxEXPAND|wxRIGHT|wxLEFT, 5 );

    m_buttonResetDialogs = new zen::BitmapButton( this, wxID_ANY, _("Restore hidden dialogs"), wxDefaultPosition, wxSize( -1,-1 ), 0 );
    m_buttonResetDialogs->SetFont( wxFont( 10, 70, 90, 90, false, wxEmptyString ) );

    bSizer95->Add( m_buttonResetDialogs, 0, wxALIGN_CENTER_VERTICAL|wxALIGN_CENTER_HORIZONTAL|wxTOP|wxRIGHT|wxLEFT, 5 );

    wxStaticBoxSizer* sbSizer26;
    sbSizer26 = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("External applications") ), wxHORIZONTAL );


    sbSizer26->Add( 5, 0, 0, 0, 5 );

    m_gridCustomCommand = new wxGrid( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0 );

    // Grid
    m_gridCustomCommand->CreateGrid( 5, 2 );
    m_gridCustomCommand->EnableEditing( true );
    m_gridCustomCommand->EnableGridLines( true );
    m_gridCustomCommand->EnableDragGridSize( false );
    m_gridCustomCommand->SetMargins( 0, 0 );

    // Columns
    m_gridCustomCommand->SetColSize( 0, 165 );
    m_gridCustomCommand->SetColSize( 1, 196 );
    m_gridCustomCommand->EnableDragColMove( false );
    m_gridCustomCommand->EnableDragColSize( true );
    m_gridCustomCommand->SetColLabelSize( 20 );
    m_gridCustomCommand->SetColLabelValue( 0, _("Description") );
    m_gridCustomCommand->SetColLabelValue( 1, _("Command line") );
    m_gridCustomCommand->SetColLabelAlignment( wxALIGN_CENTRE, wxALIGN_CENTRE );

    // Rows
    m_gridCustomCommand->EnableDragRowSize( false );
    m_gridCustomCommand->SetRowLabelSize( 1 );
    m_gridCustomCommand->SetRowLabelAlignment( wxALIGN_LEFT, wxALIGN_CENTRE );

    // Label Appearance

    // Cell Defaults
    m_gridCustomCommand->SetDefaultCellAlignment( wxALIGN_LEFT, wxALIGN_TOP );
    sbSizer26->Add( m_gridCustomCommand, 1, wxALIGN_CENTER_VERTICAL|wxALIGN_CENTER_HORIZONTAL|wxEXPAND, 5 );

    wxBoxSizer* bSizer157;
    bSizer157 = new wxBoxSizer( wxVERTICAL );

    m_bpButtonAddRow = new wxBitmapButton( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxSize( 19,21 ), wxBU_AUTODRAW );
    bSizer157->Add( m_bpButtonAddRow, 0, 0, 5 );

    m_bpButtonRemoveRow = new wxBitmapButton( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxSize( 19,21 ), wxBU_AUTODRAW );
    bSizer157->Add( m_bpButtonRemoveRow, 0, 0, 5 );


    sbSizer26->Add( bSizer157, 0, wxALIGN_CENTER_HORIZONTAL, 5 );


    sbSizer26->Add( 5, 0, 0, 0, 5 );


    bSizer95->Add( sbSizer26, 1, wxALIGN_CENTER_HORIZONTAL|wxEXPAND|wxTOP|wxRIGHT|wxLEFT, 5 );

    wxBoxSizer* bSizer97;
    bSizer97 = new wxBoxSizer( wxHORIZONTAL );

    m_button9 = new wxButton( this, wxID_DEFAULT, _("&Default"), wxDefaultPosition, wxSize( -1,30 ), 0 );
    m_button9->SetFont( wxFont( 10, 70, 90, 90, false, wxEmptyString ) );

    bSizer97->Add( m_button9, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );


    bSizer97->Add( 0, 0, 1, 0, 5 );

    m_buttonOkay = new wxButton( this, wxID_OK, _("OK"), wxDefaultPosition, wxSize( -1,30 ), 0 );
    m_buttonOkay->SetDefault();
    m_buttonOkay->SetFont( wxFont( 10, 70, 90, 92, false, wxEmptyString ) );

    bSizer97->Add( m_buttonOkay, 0, wxTOP|wxBOTTOM|wxLEFT|wxALIGN_CENTER_VERTICAL, 5 );

    m_button29 = new wxButton( this, wxID_CANCEL, _("Cancel"), wxDefaultPosition, wxSize( -1,30 ), 0 );
    m_button29->SetFont( wxFont( 10, 70, 90, 90, false, wxEmptyString ) );

    bSizer97->Add( m_button29, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );


    bSizer95->Add( bSizer97, 0, wxALIGN_CENTER_HORIZONTAL|wxEXPAND, 5 );


    this->SetSizer( bSizer95 );
    this->Layout();
    bSizer95->Fit( this );

    this->Centre( wxBOTH );

    // Connect Events
    this->Connect( wxEVT_CLOSE_WINDOW, wxCloseEventHandler( GlobalSettingsDlgGenerated::OnClose ) );
    m_buttonResetDialogs->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( GlobalSettingsDlgGenerated::OnResetDialogs ), NULL, this );
    m_bpButtonAddRow->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( GlobalSettingsDlgGenerated::OnAddRow ), NULL, this );
    m_bpButtonRemoveRow->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( GlobalSettingsDlgGenerated::OnRemoveRow ), NULL, this );
    m_button9->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( GlobalSettingsDlgGenerated::OnDefault ), NULL, this );
    m_buttonOkay->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( GlobalSettingsDlgGenerated::OnOkay ), NULL, this );
    m_button29->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( GlobalSettingsDlgGenerated::OnCancel ), NULL, this );
}

GlobalSettingsDlgGenerated::~GlobalSettingsDlgGenerated()
{
    // Disconnect Events
    this->Disconnect( wxEVT_CLOSE_WINDOW, wxCloseEventHandler( GlobalSettingsDlgGenerated::OnClose ) );
    m_buttonResetDialogs->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( GlobalSettingsDlgGenerated::OnResetDialogs ), NULL, this );
    m_bpButtonAddRow->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( GlobalSettingsDlgGenerated::OnAddRow ), NULL, this );
    m_bpButtonRemoveRow->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( GlobalSettingsDlgGenerated::OnRemoveRow ), NULL, this );
    m_button9->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( GlobalSettingsDlgGenerated::OnDefault ), NULL, this );
    m_buttonOkay->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( GlobalSettingsDlgGenerated::OnOkay ), NULL, this );
    m_button29->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( GlobalSettingsDlgGenerated::OnCancel ), NULL, this );

}

SyncPreviewDlgGenerated::SyncPreviewDlgGenerated( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : wxDialog( parent, id, title, pos, size, style )
{
    this->SetSizeHints( wxDefaultSize, wxDefaultSize );

    wxBoxSizer* bSizer134;
    bSizer134 = new wxBoxSizer( wxVERTICAL );

    wxBoxSizer* bSizer158;
    bSizer158 = new wxBoxSizer( wxHORIZONTAL );

    m_buttonStartSync = new zen::BitmapButton( this, wxID_OK, _("Start"), wxDefaultPosition, wxSize( -1,40 ), 0 );
    m_buttonStartSync->SetDefault();
    m_buttonStartSync->SetFont( wxFont( 14, 70, 90, 92, false, wxEmptyString ) );
    m_buttonStartSync->SetToolTip( _("Start synchronization") );

    bSizer158->Add( m_buttonStartSync, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxALL, 5 );

    m_staticline16 = new wxStaticLine( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_VERTICAL );
    bSizer158->Add( m_staticline16, 0, wxEXPAND|wxRIGHT|wxLEFT, 5 );

    wxStaticBoxSizer* sbSizer28;
    sbSizer28 = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Variant") ), wxVERTICAL );

    m_staticTextVariant = new wxStaticText( this, wxID_ANY, _("dummy"), wxDefaultPosition, wxDefaultSize, 0 );
    m_staticTextVariant->Wrap( -1 );
    m_staticTextVariant->SetFont( wxFont( 10, 70, 90, 92, false, wxEmptyString ) );

    sbSizer28->Add( m_staticTextVariant, 0, wxALL|wxALIGN_CENTER_VERTICAL|wxALIGN_CENTER_HORIZONTAL, 5 );


    bSizer158->Add( sbSizer28, 1, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxBOTTOM|wxRIGHT|wxLEFT, 5 );


    bSizer134->Add( bSizer158, 0, wxALIGN_CENTER_HORIZONTAL|wxEXPAND, 5 );

    m_staticline14 = new wxStaticLine( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
    bSizer134->Add( m_staticline14, 0, wxEXPAND, 5 );

    wxBoxSizer* bSizer141;
    bSizer141 = new wxBoxSizer( wxHORIZONTAL );

    wxStaticBoxSizer* sbSizer161;
    sbSizer161 = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Statistics") ), wxVERTICAL );

    wxFlexGridSizer* fgSizer11;
    fgSizer11 = new wxFlexGridSizer( 2, 7, 2, 5 );
    fgSizer11->SetFlexibleDirection( wxBOTH );
    fgSizer11->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

    m_bitmapCreateLeft = new wxStaticBitmap( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, 0 );
    m_bitmapCreateLeft->SetToolTip( _("Number of files and folders that will be created") );

    fgSizer11->Add( m_bitmapCreateLeft, 0, wxALIGN_CENTER_VERTICAL|wxALIGN_CENTER_HORIZONTAL, 5 );

    m_bitmapUpdateLeft = new wxStaticBitmap( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, 0 );
    m_bitmapUpdateLeft->SetToolTip( _("Number of files that will be overwritten") );

    fgSizer11->Add( m_bitmapUpdateLeft, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 5 );

    m_bitmapDeleteLeft = new wxStaticBitmap( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, 0 );
    m_bitmapDeleteLeft->SetToolTip( _("Number of files and folders that will be deleted") );

    fgSizer11->Add( m_bitmapDeleteLeft, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 5 );

    m_bitmapData = new wxStaticBitmap( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, 0 );
    m_bitmapData->SetToolTip( _("Total bytes to copy") );

    fgSizer11->Add( m_bitmapData, 0, wxALIGN_CENTER_VERTICAL|wxALIGN_CENTER_HORIZONTAL, 5 );

    m_bitmapDeleteRight = new wxStaticBitmap( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, 0 );
    m_bitmapDeleteRight->SetToolTip( _("Number of files and folders that will be deleted") );

    fgSizer11->Add( m_bitmapDeleteRight, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 5 );

    m_bitmapUpdateRight = new wxStaticBitmap( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, 0 );
    m_bitmapUpdateRight->SetToolTip( _("Number of files that will be overwritten") );

    fgSizer11->Add( m_bitmapUpdateRight, 0, wxALIGN_CENTER_VERTICAL|wxALIGN_CENTER_HORIZONTAL, 5 );

    m_bitmapCreateRight = new wxStaticBitmap( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, 0 );
    m_bitmapCreateRight->SetToolTip( _("Number of files and folders that will be created") );

    fgSizer11->Add( m_bitmapCreateRight, 0, wxALIGN_CENTER_VERTICAL|wxALIGN_CENTER_HORIZONTAL, 5 );

    m_staticTextCreateLeft = new wxStaticText( this, wxID_ANY, _("dummy"), wxDefaultPosition, wxDefaultSize, 0 );
    m_staticTextCreateLeft->Wrap( -1 );
    m_staticTextCreateLeft->SetToolTip( _("Number of files and folders that will be created") );

    fgSizer11->Add( m_staticTextCreateLeft, 0, wxALIGN_CENTER_VERTICAL|wxALIGN_CENTER_HORIZONTAL, 5 );

    m_staticTextUpdateLeft = new wxStaticText( this, wxID_ANY, _("dummy"), wxDefaultPosition, wxDefaultSize, 0 );
    m_staticTextUpdateLeft->Wrap( -1 );
    m_staticTextUpdateLeft->SetToolTip( _("Number of files that will be overwritten") );

    fgSizer11->Add( m_staticTextUpdateLeft, 0, wxALIGN_CENTER_VERTICAL|wxALIGN_CENTER_HORIZONTAL, 5 );

    m_staticTextDeleteLeft = new wxStaticText( this, wxID_ANY, _("dummy"), wxDefaultPosition, wxDefaultSize, 0 );
    m_staticTextDeleteLeft->Wrap( -1 );
    m_staticTextDeleteLeft->SetToolTip( _("Number of files and folders that will be deleted") );

    fgSizer11->Add( m_staticTextDeleteLeft, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 5 );

    m_staticTextData = new wxStaticText( this, wxID_ANY, _("dummy"), wxDefaultPosition, wxDefaultSize, 0 );
    m_staticTextData->Wrap( -1 );
    m_staticTextData->SetToolTip( _("Total bytes to copy") );

    fgSizer11->Add( m_staticTextData, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 5 );

    m_staticTextDeleteRight = new wxStaticText( this, wxID_ANY, _("dummy"), wxDefaultPosition, wxDefaultSize, 0 );
    m_staticTextDeleteRight->Wrap( -1 );
    m_staticTextDeleteRight->SetToolTip( _("Number of files and folders that will be deleted") );

    fgSizer11->Add( m_staticTextDeleteRight, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 5 );

    m_staticTextUpdateRight = new wxStaticText( this, wxID_ANY, _("dummy"), wxDefaultPosition, wxDefaultSize, 0 );
    m_staticTextUpdateRight->Wrap( -1 );
    m_staticTextUpdateRight->SetToolTip( _("Number of files that will be overwritten") );

    fgSizer11->Add( m_staticTextUpdateRight, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 5 );

    m_staticTextCreateRight = new wxStaticText( this, wxID_ANY, _("dummy"), wxDefaultPosition, wxDefaultSize, 0 );
    m_staticTextCreateRight->Wrap( -1 );
    m_staticTextCreateRight->SetToolTip( _("Number of files and folders that will be created") );

    fgSizer11->Add( m_staticTextCreateRight, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 5 );


    sbSizer161->Add( fgSizer11, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );


    bSizer141->Add( sbSizer161, 0, wxALIGN_CENTER_VERTICAL|wxALIGN_CENTER_HORIZONTAL|wxALL, 5 );


    bSizer134->Add( bSizer141, 0, wxALIGN_CENTER_HORIZONTAL, 5 );

    m_staticline12 = new wxStaticLine( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
    bSizer134->Add( m_staticline12, 0, wxEXPAND|wxTOP, 5 );

    wxBoxSizer* bSizer142;
    bSizer142 = new wxBoxSizer( wxHORIZONTAL );

    m_checkBoxDontShowAgain = new wxCheckBox( this, wxID_ANY, _("Do not show this dialog again"), wxDefaultPosition, wxDefaultSize, 0 );
    bSizer142->Add( m_checkBoxDontShowAgain, 0, wxALIGN_CENTER_HORIZONTAL|wxALL|wxALIGN_CENTER_VERTICAL, 5 );


    bSizer142->Add( 10, 0, 1, 0, 5 );

    m_button16 = new wxButton( this, wxID_CANCEL, _("Cancel"), wxDefaultPosition, wxSize( -1,30 ), 0 );
    m_button16->SetFont( wxFont( 10, 70, 90, 90, false, wxEmptyString ) );

    bSizer142->Add( m_button16, 0, wxALIGN_CENTER_VERTICAL|wxALIGN_CENTER_HORIZONTAL|wxALL, 5 );


    bSizer134->Add( bSizer142, 0, wxEXPAND, 5 );


    this->SetSizer( bSizer134 );
    this->Layout();
    bSizer134->Fit( this );

    this->Centre( wxBOTH );

    // Connect Events
    this->Connect( wxEVT_CLOSE_WINDOW, wxCloseEventHandler( SyncPreviewDlgGenerated::OnClose ) );
    m_buttonStartSync->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( SyncPreviewDlgGenerated::OnStartSync ), NULL, this );
    m_button16->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( SyncPreviewDlgGenerated::OnCancel ), NULL, this );
}

SyncPreviewDlgGenerated::~SyncPreviewDlgGenerated()
{
    // Disconnect Events
    this->Disconnect( wxEVT_CLOSE_WINDOW, wxCloseEventHandler( SyncPreviewDlgGenerated::OnClose ) );
    m_buttonStartSync->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( SyncPreviewDlgGenerated::OnStartSync ), NULL, this );
    m_button16->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( SyncPreviewDlgGenerated::OnCancel ), NULL, this );

}

PopupFrameGenerated1::PopupFrameGenerated1( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : wxFrame( parent, id, title, pos, size, style )
{
    this->SetSizeHints( wxDefaultSize, wxDefaultSize );

    wxBoxSizer* bSizer158;
    bSizer158 = new wxBoxSizer( wxHORIZONTAL );

    m_bitmapLeft = new wxStaticBitmap( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, 0 );
    bSizer158->Add( m_bitmapLeft, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );

    m_staticTextMain = new wxStaticText( this, wxID_ANY, _("dummy"), wxDefaultPosition, wxDefaultSize, 0 );
    m_staticTextMain->Wrap( 600 );
    bSizer158->Add( m_staticTextMain, 0, wxALL|wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 5 );


    this->SetSizer( bSizer158 );
    this->Layout();
    bSizer158->Fit( this );
}

PopupFrameGenerated1::~PopupFrameGenerated1()
{
}

SearchDialogGenerated::SearchDialogGenerated( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : wxDialog( parent, id, title, pos, size, style )
{
    this->SetSizeHints( wxDefaultSize, wxDefaultSize );

    wxBoxSizer* bSizer161;
    bSizer161 = new wxBoxSizer( wxHORIZONTAL );

    wxBoxSizer* bSizer166;
    bSizer166 = new wxBoxSizer( wxVERTICAL );

    wxBoxSizer* bSizer162;
    bSizer162 = new wxBoxSizer( wxHORIZONTAL );

    m_staticText101 = new wxStaticText( this, wxID_ANY, _("Find what:"), wxDefaultPosition, wxDefaultSize, 0 );
    m_staticText101->Wrap( -1 );
    bSizer162->Add( m_staticText101, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );

    m_textCtrlSearchTxt = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize( 220,-1 ), 0 );
    bSizer162->Add( m_textCtrlSearchTxt, 1, wxALL|wxALIGN_CENTER_VERTICAL, 5 );


    bSizer166->Add( bSizer162, 0, wxALIGN_CENTER_VERTICAL|wxEXPAND, 5 );


    bSizer166->Add( 0, 5, 0, 0, 5 );

    m_checkBoxMatchCase = new wxCheckBox( this, wxID_ANY, _("Match case"), wxDefaultPosition, wxDefaultSize, 0 );
    bSizer166->Add( m_checkBoxMatchCase, 0, wxALL, 5 );


    bSizer161->Add( bSizer166, 1, wxALIGN_CENTER_VERTICAL|wxEXPAND, 5 );

    wxBoxSizer* bSizer97;
    bSizer97 = new wxBoxSizer( wxVERTICAL );

    m_buttonFindNext = new wxButton( this, wxID_OK, _("&Find next"), wxDefaultPosition, wxSize( -1,30 ), 0 );
    m_buttonFindNext->SetDefault();
    m_buttonFindNext->SetFont( wxFont( 10, 70, 90, 92, false, wxEmptyString ) );

    bSizer97->Add( m_buttonFindNext, 0, wxEXPAND|wxTOP|wxRIGHT, 5 );

    m_button29 = new wxButton( this, wxID_CANCEL, _("Cancel"), wxDefaultPosition, wxSize( -1,30 ), 0 );
    m_button29->SetFont( wxFont( 10, 70, 90, 90, false, wxEmptyString ) );

    bSizer97->Add( m_button29, 0, wxEXPAND|wxTOP|wxBOTTOM|wxRIGHT, 5 );


    bSizer161->Add( bSizer97, 0, wxALIGN_CENTER_VERTICAL, 5 );


    this->SetSizer( bSizer161 );
    this->Layout();
    bSizer161->Fit( this );

    this->Centre( wxBOTH );

    // Connect Events
    this->Connect( wxEVT_CLOSE_WINDOW, wxCloseEventHandler( SearchDialogGenerated::OnClose ) );
    m_textCtrlSearchTxt->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( SearchDialogGenerated::OnText ), NULL, this );
    m_buttonFindNext->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( SearchDialogGenerated::OnFindNext ), NULL, this );
    m_button29->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( SearchDialogGenerated::OnCancel ), NULL, this );
}

SearchDialogGenerated::~SearchDialogGenerated()
{
    // Disconnect Events
    this->Disconnect( wxEVT_CLOSE_WINDOW, wxCloseEventHandler( SearchDialogGenerated::OnClose ) );
    m_textCtrlSearchTxt->Disconnect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( SearchDialogGenerated::OnText ), NULL, this );
    m_buttonFindNext->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( SearchDialogGenerated::OnFindNext ), NULL, this );
    m_button29->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( SearchDialogGenerated::OnCancel ), NULL, this );

}

SelectTimespanDlgGenerated::SelectTimespanDlgGenerated( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : wxDialog( parent, id, title, pos, size, style )
{
    this->SetSizeHints( wxDefaultSize, wxDefaultSize );

    wxBoxSizer* bSizer96;
    bSizer96 = new wxBoxSizer( wxVERTICAL );

    wxBoxSizer* bSizer98;
    bSizer98 = new wxBoxSizer( wxHORIZONTAL );

    m_calendarFrom = new wxCalendarCtrl( this, wxID_ANY, wxDefaultDateTime, wxDefaultPosition, wxDefaultSize, wxCAL_SHOW_HOLIDAYS );
    bSizer98->Add( m_calendarFrom, 0, wxALL, 5 );

    m_calendarTo = new wxCalendarCtrl( this, wxID_ANY, wxDefaultDateTime, wxDefaultPosition, wxDefaultSize, wxCAL_SHOW_HOLIDAYS );
    bSizer98->Add( m_calendarTo, 0, wxALL, 5 );


    bSizer96->Add( bSizer98, 0, wxALIGN_CENTER_VERTICAL, 5 );

    wxBoxSizer* bSizer97;
    bSizer97 = new wxBoxSizer( wxHORIZONTAL );


    bSizer97->Add( 0, 0, 1, wxEXPAND, 5 );

    m_buttonOkay = new wxButton( this, wxID_OK, _("OK"), wxDefaultPosition, wxSize( -1,30 ), 0 );
    m_buttonOkay->SetDefault();
    m_buttonOkay->SetFont( wxFont( 10, 70, 90, 92, false, wxEmptyString ) );

    bSizer97->Add( m_buttonOkay, 0, wxTOP|wxBOTTOM|wxLEFT|wxALIGN_CENTER_VERTICAL, 5 );

    m_button29 = new wxButton( this, wxID_CANCEL, _("Cancel"), wxDefaultPosition, wxSize( -1,30 ), 0 );
    m_button29->SetFont( wxFont( 10, 70, 90, 90, false, wxEmptyString ) );

    bSizer97->Add( m_button29, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );


    bSizer97->Add( 0, 0, 1, wxEXPAND, 5 );


    bSizer96->Add( bSizer97, 0, wxEXPAND|wxALIGN_CENTER_HORIZONTAL|wxBOTTOM|wxRIGHT|wxLEFT, 5 );


    this->SetSizer( bSizer96 );
    this->Layout();
    bSizer96->Fit( this );

    this->Centre( wxBOTH );

    // Connect Events
    this->Connect( wxEVT_CLOSE_WINDOW, wxCloseEventHandler( SelectTimespanDlgGenerated::OnClose ) );
    m_calendarFrom->Connect( wxEVT_CALENDAR_SEL_CHANGED, wxCalendarEventHandler( SelectTimespanDlgGenerated::OnChangeSelectionFrom ), NULL, this );
    m_calendarTo->Connect( wxEVT_CALENDAR_SEL_CHANGED, wxCalendarEventHandler( SelectTimespanDlgGenerated::OnChangeSelectionTo ), NULL, this );
    m_buttonOkay->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( SelectTimespanDlgGenerated::OnOkay ), NULL, this );
    m_button29->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( SelectTimespanDlgGenerated::OnCancel ), NULL, this );
}

SelectTimespanDlgGenerated::~SelectTimespanDlgGenerated()
{
    // Disconnect Events
    this->Disconnect( wxEVT_CLOSE_WINDOW, wxCloseEventHandler( SelectTimespanDlgGenerated::OnClose ) );
    m_calendarFrom->Disconnect( wxEVT_CALENDAR_SEL_CHANGED, wxCalendarEventHandler( SelectTimespanDlgGenerated::OnChangeSelectionFrom ), NULL, this );
    m_calendarTo->Disconnect( wxEVT_CALENDAR_SEL_CHANGED, wxCalendarEventHandler( SelectTimespanDlgGenerated::OnChangeSelectionTo ), NULL, this );
    m_buttonOkay->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( SelectTimespanDlgGenerated::OnOkay ), NULL, this );
    m_button29->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( SelectTimespanDlgGenerated::OnCancel ), NULL, this );

}
