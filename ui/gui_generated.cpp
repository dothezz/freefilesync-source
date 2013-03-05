///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Oct  8 2012)
// http://www.wxformbuilder.org/
//
// PLEASE DO "NOT" EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "../wx+/button.h"
#include "../wx+/graph.h"
#include "../wx+/grid.h"
#include "../wx+/toggle_button.h"
#include "exec_finished_box.h"
#include "folder_history_box.h"
#include "triple_splitter.h"
#include "wx_form_build_hide_warnings.h"

#include "gui_generated.h"

///////////////////////////////////////////////////////////////////////////

MainDialogGenerated::MainDialogGenerated( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : wxFrame( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxSize( 640,400 ), wxDefaultSize );
	
	m_menubar1 = new wxMenuBar( 0 );
	m_menuFile = new wxMenu();
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
	
	m_menuItemSaveAs = new wxMenuItem( m_menuFile, wxID_SAVEAS, wxString( _("Save &as...") ) , wxEmptyString, wxITEM_NORMAL );
	m_menuFile->Append( m_menuItemSaveAs );
	
	m_menuItem7 = new wxMenuItem( m_menuFile, wxID_ANY, wxString( _("Save as &batch job...") ) , wxEmptyString, wxITEM_NORMAL );
	m_menuFile->Append( m_menuItem7 );
	
	m_menuFile->AppendSeparator();
	
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
	
	
	bSizerTopButtons->Add( 15, 0, 0, wxALIGN_CENTER_VERTICAL, 5 );
	
	wxFlexGridSizer* fgSizer121;
	fgSizer121 = new wxFlexGridSizer( 2, 2, 0, 0 );
	fgSizer121->SetFlexibleDirection( wxBOTH );
	fgSizer121->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );
	
	m_staticTextCmpVariant = new wxStaticText( m_panelTopButtons, wxID_ANY, _("dummy"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextCmpVariant->Wrap( -1 );
	m_staticTextCmpVariant->SetFont( wxFont( wxNORMAL_FONT->GetPointSize(), 70, 90, 92, false, wxEmptyString ) );
	m_staticTextCmpVariant->SetForegroundColour( wxSystemSettings::GetColour( wxSYS_COLOUR_GRAYTEXT ) );
	
	fgSizer121->Add( m_staticTextCmpVariant, 0, wxALIGN_CENTER_VERTICAL|wxALIGN_CENTER_HORIZONTAL|wxTOP, 1 );
	
	
	fgSizer121->Add( 0, 0, 1, 0, 5 );
	
	wxBoxSizer* bSizer30;
	bSizer30 = new wxBoxSizer( wxHORIZONTAL );
	
	m_buttonCompare = new zen::BitmapButton( m_panelTopButtons, wxID_OK, _("Compare"), wxDefaultPosition, wxSize( 180,-1 ), 0 );
	m_buttonCompare->SetFont( wxFont( wxNORMAL_FONT->GetPointSize(), 70, 90, 92, false, wxEmptyString ) );
	m_buttonCompare->SetToolTip( _("dummy") );
	
	bSizer30->Add( m_buttonCompare, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxEXPAND, 5 );
	
	m_buttonAbort = new zen::BitmapButton( m_panelTopButtons, wxID_CANCEL, _("&Abort"), wxDefaultPosition, wxSize( 180,-1 ), 0 );
	m_buttonAbort->SetFont( wxFont( wxNORMAL_FONT->GetPointSize(), 70, 90, 92, false, wxEmptyString ) );
	m_buttonAbort->Enable( false );
	m_buttonAbort->Hide();
	
	bSizer30->Add( m_buttonAbort, 0, wxALIGN_CENTER_VERTICAL|wxEXPAND, 5 );
	
	
	fgSizer121->Add( bSizer30, 0, wxALIGN_CENTER_VERTICAL|wxEXPAND, 5 );
	
	m_bpButtonCmpConfig = new wxBitmapButton( m_panelTopButtons, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxSize( 46,46 ), wxBU_AUTODRAW );
	m_bpButtonCmpConfig->SetToolTip( _("Comparison settings") );
	
	fgSizer121->Add( m_bpButtonCmpConfig, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 3 );
	
	
	bSizerTopButtons->Add( fgSizer121, 0, wxALIGN_CENTER_VERTICAL|wxBOTTOM, 5 );
	
	
	bSizerTopButtons->Add( 0, 0, 1, wxALIGN_CENTER_VERTICAL, 5 );
	
	wxFlexGridSizer* fgSizer12;
	fgSizer12 = new wxFlexGridSizer( 2, 2, 0, 0 );
	fgSizer12->SetFlexibleDirection( wxBOTH );
	fgSizer12->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );
	
	
	fgSizer12->Add( 0, 0, 1, wxEXPAND, 5 );
	
	m_staticTextSyncVariant = new wxStaticText( m_panelTopButtons, wxID_ANY, _("dummy"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextSyncVariant->Wrap( -1 );
	m_staticTextSyncVariant->SetFont( wxFont( wxNORMAL_FONT->GetPointSize(), 70, 90, 92, false, wxEmptyString ) );
	m_staticTextSyncVariant->SetForegroundColour( wxSystemSettings::GetColour( wxSYS_COLOUR_GRAYTEXT ) );
	
	fgSizer12->Add( m_staticTextSyncVariant, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxTOP, 1 );
	
	m_bpButtonSyncConfig = new wxBitmapButton( m_panelTopButtons, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxSize( 46,46 ), wxBU_AUTODRAW );
	m_bpButtonSyncConfig->SetToolTip( _("Synchronization settings") );
	
	fgSizer12->Add( m_bpButtonSyncConfig, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 3 );
	
	m_buttonSync = new zen::BitmapButton( m_panelTopButtons, wxID_ANY, _("Synchronize"), wxDefaultPosition, wxSize( 180,-1 ), 0 );
	m_buttonSync->SetFont( wxFont( wxNORMAL_FONT->GetPointSize(), 70, 90, 92, false, wxEmptyString ) );
	m_buttonSync->SetToolTip( _("dummy") );
	
	fgSizer12->Add( m_buttonSync, 0, wxALIGN_CENTER_VERTICAL|wxEXPAND, 5 );
	
	
	bSizerTopButtons->Add( fgSizer12, 0, wxALIGN_CENTER_VERTICAL|wxBOTTOM, 5 );
	
	
	bSizerTopButtons->Add( 15, 0, 0, wxALIGN_CENTER_VERTICAL, 5 );
	
	
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
	
	wxFlexGridSizer* fgSizer8;
	fgSizer8 = new wxFlexGridSizer( 0, 2, 0, 0 );
	fgSizer8->AddGrowableCol( 1 );
	fgSizer8->SetFlexibleDirection( wxBOTH );
	fgSizer8->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_ALL );
	
	
	fgSizer8->Add( 0, 0, 1, wxEXPAND, 5 );
	
	m_staticTextResolvedPathL = new wxStaticText( m_panelTopLeft, wxID_ANY, _("dummy"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextResolvedPathL->Wrap( -1 );
	fgSizer8->Add( m_staticTextResolvedPathL, 0, wxALIGN_CENTER_VERTICAL|wxALL, 2 );
	
	wxBoxSizer* bSizer159;
	bSizer159 = new wxBoxSizer( wxHORIZONTAL );
	
	m_bpButtonAddPair = new wxBitmapButton( m_panelTopLeft, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxSize( 25,25 ), wxBU_AUTODRAW );
	m_bpButtonAddPair->SetToolTip( _("Add folder pair") );
	
	bSizer159->Add( m_bpButtonAddPair, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 5 );
	
	m_bpButtonRemovePair = new wxBitmapButton( m_panelTopLeft, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxSize( 25,25 ), wxBU_AUTODRAW );
	m_bpButtonRemovePair->SetToolTip( _("Remove folder pair") );
	
	bSizer159->Add( m_bpButtonRemovePair, 0, wxALIGN_CENTER_VERTICAL, 5 );
	
	
	fgSizer8->Add( bSizer159, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 5 );
	
	wxBoxSizer* bSizer182;
	bSizer182 = new wxBoxSizer( wxHORIZONTAL );
	
	m_directoryLeft = new FolderHistoryBox( m_panelTopLeft, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0, NULL, 0 ); 
	bSizer182->Add( m_directoryLeft, 1, wxALIGN_CENTER_VERTICAL, 5 );
	
	m_buttonSelectDirLeft = new wxButton( m_panelTopLeft, wxID_ANY, _("Browse"), wxDefaultPosition, wxDefaultSize, 0 );
	m_buttonSelectDirLeft->SetToolTip( _("Select a folder") );
	
	bSizer182->Add( m_buttonSelectDirLeft, 0, wxALIGN_CENTER_VERTICAL, 5 );
	
	
	fgSizer8->Add( bSizer182, 0, wxALIGN_CENTER_VERTICAL|wxEXPAND, 5 );
	
	
	m_panelTopLeft->SetSizer( fgSizer8 );
	m_panelTopLeft->Layout();
	fgSizer8->Fit( m_panelTopLeft );
	bSizer91->Add( m_panelTopLeft, 1, wxLEFT|wxALIGN_CENTER_VERTICAL, 5 );
	
	m_panelTopMiddle = new wxPanel( m_panelDirectoryPairs, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	wxBoxSizer* bSizer1771;
	bSizer1771 = new wxBoxSizer( wxVERTICAL );
	
	
	bSizer1771->Add( 0, 0, 1, wxEXPAND, 5 );
	
	m_bpButtonSwapSides = new wxBitmapButton( m_panelTopMiddle, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxSize( -1,-1 ), wxBU_AUTODRAW );
	m_bpButtonSwapSides->SetToolTip( _("Swap sides") );
	
	bSizer1771->Add( m_bpButtonSwapSides, 0, wxALIGN_CENTER_VERTICAL|wxALIGN_CENTER_HORIZONTAL|wxEXPAND, 5 );
	
	wxBoxSizer* bSizer160;
	bSizer160 = new wxBoxSizer( wxHORIZONTAL );
	
	m_bpButtonAltCompCfg = new wxBitmapButton( m_panelTopMiddle, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxSize( 25,25 ), wxBU_AUTODRAW );
	bSizer160->Add( m_bpButtonAltCompCfg, 0, wxALIGN_CENTER_VERTICAL, 5 );
	
	m_bpButtonLocalFilter = new wxBitmapButton( m_panelTopMiddle, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxSize( 25,25 ), wxBU_AUTODRAW );
	bSizer160->Add( m_bpButtonLocalFilter, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 2 );
	
	m_bpButtonAltSyncCfg = new wxBitmapButton( m_panelTopMiddle, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxSize( 25,25 ), wxBU_AUTODRAW );
	bSizer160->Add( m_bpButtonAltSyncCfg, 0, wxALIGN_CENTER_VERTICAL, 5 );
	
	
	bSizer1771->Add( bSizer160, 0, wxALIGN_CENTER_HORIZONTAL, 5 );
	
	
	bSizer1771->Add( 0, 0, 1, wxEXPAND, 5 );
	
	
	m_panelTopMiddle->SetSizer( bSizer1771 );
	m_panelTopMiddle->Layout();
	bSizer1771->Fit( m_panelTopMiddle );
	bSizer91->Add( m_panelTopMiddle, 0, wxRIGHT|wxLEFT|wxALIGN_CENTER_VERTICAL|wxEXPAND, 5 );
	
	m_panelTopRight = new wxPanel( m_panelDirectoryPairs, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	m_panelTopRight->SetMinSize( wxSize( 1,-1 ) );
	
	wxBoxSizer* bSizer183;
	bSizer183 = new wxBoxSizer( wxVERTICAL );
	
	m_staticTextResolvedPathR = new wxStaticText( m_panelTopRight, wxID_ANY, _("dummy"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextResolvedPathR->Wrap( -1 );
	bSizer183->Add( m_staticTextResolvedPathR, 0, wxALIGN_CENTER_VERTICAL|wxALL, 2 );
	
	wxBoxSizer* bSizer179;
	bSizer179 = new wxBoxSizer( wxHORIZONTAL );
	
	m_directoryRight = new FolderHistoryBox( m_panelTopRight, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0, NULL, 0 ); 
	bSizer179->Add( m_directoryRight, 1, wxALIGN_CENTER_VERTICAL, 5 );
	
	m_buttonSelectDirRight = new wxButton( m_panelTopRight, wxID_ANY, _("Browse"), wxDefaultPosition, wxDefaultSize, 0 );
	m_buttonSelectDirRight->SetToolTip( _("Select a folder") );
	
	bSizer179->Add( m_buttonSelectDirRight, 0, wxALIGN_CENTER_VERTICAL, 5 );
	
	
	bSizer183->Add( bSizer179, 0, wxALIGN_CENTER_VERTICAL|wxEXPAND, 5 );
	
	
	m_panelTopRight->SetSizer( bSizer183 );
	m_panelTopRight->Layout();
	bSizer183->Fit( m_panelTopRight );
	bSizer91->Add( m_panelTopRight, 1, wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );
	
	
	bSizer1601->Add( bSizer91, 0, wxEXPAND, 5 );
	
	m_scrolledWindowFolderPairs = new wxScrolledWindow( m_panelDirectoryPairs, wxID_ANY, wxDefaultPosition, wxSize( -1,-1 ), wxHSCROLL|wxVSCROLL );
	m_scrolledWindowFolderPairs->SetScrollRate( 10, 10 );
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
	bSizerFileStatus = new wxBoxSizer( wxHORIZONTAL );
	
	bSizerStatusLeft = new wxBoxSizer( wxHORIZONTAL );
	
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
	
	
	bSizerStatusLeft->Add( bSizer53, 1, wxEXPAND|wxALIGN_CENTER_VERTICAL, 5 );
	
	m_staticline9 = new wxStaticLine( m_panelStatusBar, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_VERTICAL );
	bSizerStatusLeft->Add( m_staticline9, 0, wxALIGN_CENTER_VERTICAL|wxEXPAND|wxTOP, 2 );
	
	
	bSizerFileStatus->Add( bSizerStatusLeft, 1, wxEXPAND|wxALIGN_CENTER_VERTICAL, 5 );
	
	
	bSizerFileStatus->Add( 26, 0, 0, wxALIGN_CENTER_VERTICAL, 5 );
	
	m_staticTextStatusMiddle = new wxStaticText( m_panelStatusBar, wxID_ANY, _("dummy"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextStatusMiddle->Wrap( -1 );
	m_staticTextStatusMiddle->SetFont( wxFont( wxNORMAL_FONT->GetPointSize(), 70, 90, 92, false, wxEmptyString ) );
	
	bSizerFileStatus->Add( m_staticTextStatusMiddle, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 5 );
	
	
	bSizerFileStatus->Add( 26, 0, 0, wxALIGN_CENTER_VERTICAL, 5 );
	
	bSizerStatusRight = new wxBoxSizer( wxHORIZONTAL );
	
	m_staticline10 = new wxStaticLine( m_panelStatusBar, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_VERTICAL );
	bSizerStatusRight->Add( m_staticline10, 0, wxEXPAND|wxALIGN_CENTER_VERTICAL|wxTOP, 2 );
	
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
	
	
	bSizerStatusRight->Add( bSizer52, 1, wxEXPAND|wxALIGN_CENTER_VERTICAL, 5 );
	
	
	bSizerFileStatus->Add( bSizerStatusRight, 1, wxEXPAND|wxALIGN_CENTER_VERTICAL, 5 );
	
	
	bSizer451->Add( bSizerFileStatus, 1, wxALIGN_CENTER_VERTICAL|wxEXPAND, 5 );
	
	m_staticTextFullStatus = new wxStaticText( m_panelStatusBar, wxID_ANY, _("dummy"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextFullStatus->Wrap( -1 );
	m_staticTextFullStatus->SetFont( wxFont( wxNORMAL_FONT->GetPointSize(), 70, 90, 92, false, wxEmptyString ) );
	
	bSizer451->Add( m_staticTextFullStatus, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );
	
	
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
	
	m_bpButtonOpen = new wxBitmapButton( m_panelConfig, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxSize( 42,42 ), wxBU_AUTODRAW );
	m_bpButtonOpen->SetToolTip( _("dummy") );
	
	bSizer151->Add( m_bpButtonOpen, 0, wxALIGN_CENTER_VERTICAL, 5 );
	
	m_bpButtonSave = new wxBitmapButton( m_panelConfig, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxSize( 42,42 ), wxBU_AUTODRAW );
	m_bpButtonSave->SetToolTip( _("dummy") );
	
	bSizer151->Add( m_bpButtonSave, 0, wxALIGN_CENTER_VERTICAL, 5 );
	
	m_bpButtonBatchJob = new wxBitmapButton( m_panelConfig, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxSize( 42,42 ), wxBU_AUTODRAW );
	m_bpButtonBatchJob->SetToolTip( _("Save as batch job") );
	
	bSizer151->Add( m_bpButtonBatchJob, 0, wxALIGN_CENTER_VERTICAL, 5 );
	
	
	bSizerConfig->Add( bSizer151, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 5 );
	
	m_listBoxHistory = new wxListBox( m_panelConfig, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0, NULL, wxLB_EXTENDED|wxLB_NEEDED_SB|wxLB_SORT ); 
	m_listBoxHistory->SetMinSize( wxSize( -1,40 ) );
	
	bSizerConfig->Add( m_listBoxHistory, 1, wxEXPAND|wxALIGN_CENTER_VERTICAL, 5 );
	
	
	m_panelConfig->SetSizer( bSizerConfig );
	m_panelConfig->Layout();
	bSizerConfig->Fit( m_panelConfig );
	bSizerPanelHolder->Add( m_panelConfig, 0, 0, 5 );
	
	m_panelFilter = new wxPanel( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	wxBoxSizer* bSizer171;
	bSizer171 = new wxBoxSizer( wxHORIZONTAL );
	
	m_bpButtonFilter = new wxBitmapButton( m_panelFilter, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxSize( 42,42 ), wxBU_AUTODRAW|wxFULL_REPAINT_ON_RESIZE );
	bSizer171->Add( m_bpButtonFilter, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );
	
	m_checkBoxHideExcluded = new wxCheckBox( m_panelFilter, wxID_ANY, _("Hide excluded items"), wxDefaultPosition, wxDefaultSize, 0 );
	m_checkBoxHideExcluded->SetToolTip( _("Show filtered or temporarily excluded files") );
	
	bSizer171->Add( m_checkBoxHideExcluded, 1, wxALIGN_CENTER_VERTICAL|wxALL, 5 );
	
	
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
	
	m_bpButtonShowCreateLeft = new ToggleButton( m_panelViewFilter, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxSize( 42,42 ), wxBU_AUTODRAW );
	bSizerViewFilter->Add( m_bpButtonShowCreateLeft, 0, wxALIGN_CENTER_VERTICAL|wxALIGN_CENTER_HORIZONTAL, 5 );
	
	m_bpButtonShowUpdateLeft = new ToggleButton( m_panelViewFilter, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxSize( 42,42 ), wxBU_AUTODRAW );
	bSizerViewFilter->Add( m_bpButtonShowUpdateLeft, 0, wxALIGN_CENTER_VERTICAL|wxALIGN_CENTER_HORIZONTAL, 5 );
	
	m_bpButtonShowDeleteLeft = new ToggleButton( m_panelViewFilter, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxSize( 42,42 ), wxBU_AUTODRAW );
	bSizerViewFilter->Add( m_bpButtonShowDeleteLeft, 0, wxALIGN_CENTER_VERTICAL|wxALIGN_CENTER_HORIZONTAL, 5 );
	
	m_bpButtonShowLeftOnly = new ToggleButton( m_panelViewFilter, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxSize( 42,42 ), wxBU_AUTODRAW );
	bSizerViewFilter->Add( m_bpButtonShowLeftOnly, 0, wxALIGN_CENTER_VERTICAL|wxALIGN_CENTER_HORIZONTAL, 5 );
	
	m_bpButtonShowLeftNewer = new ToggleButton( m_panelViewFilter, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxSize( 42,42 ), wxBU_AUTODRAW );
	bSizerViewFilter->Add( m_bpButtonShowLeftNewer, 0, wxALIGN_CENTER_VERTICAL|wxALIGN_CENTER_HORIZONTAL, 5 );
	
	m_bpButtonShowEqual = new ToggleButton( m_panelViewFilter, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxSize( 42,42 ), wxBU_AUTODRAW );
	bSizerViewFilter->Add( m_bpButtonShowEqual, 0, wxALIGN_CENTER_VERTICAL|wxALIGN_CENTER_HORIZONTAL, 5 );
	
	m_bpButtonShowDifferent = new ToggleButton( m_panelViewFilter, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxSize( 42,42 ), wxBU_AUTODRAW );
	bSizerViewFilter->Add( m_bpButtonShowDifferent, 0, wxALIGN_CENTER_VERTICAL|wxALIGN_CENTER_HORIZONTAL, 5 );
	
	m_bpButtonShowDoNothing = new ToggleButton( m_panelViewFilter, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxSize( 42,42 ), wxBU_AUTODRAW );
	bSizerViewFilter->Add( m_bpButtonShowDoNothing, 0, wxALIGN_CENTER_VERTICAL|wxALIGN_CENTER_HORIZONTAL, 5 );
	
	m_bpButtonShowRightNewer = new ToggleButton( m_panelViewFilter, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxSize( 42,42 ), wxBU_AUTODRAW );
	bSizerViewFilter->Add( m_bpButtonShowRightNewer, 0, wxALIGN_CENTER_VERTICAL|wxALIGN_CENTER_HORIZONTAL, 5 );
	
	m_bpButtonShowRightOnly = new ToggleButton( m_panelViewFilter, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxSize( 42,42 ), wxBU_AUTODRAW );
	bSizerViewFilter->Add( m_bpButtonShowRightOnly, 0, wxALIGN_CENTER_VERTICAL|wxALIGN_CENTER_HORIZONTAL, 5 );
	
	m_bpButtonShowDeleteRight = new ToggleButton( m_panelViewFilter, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxSize( 42,42 ), wxBU_AUTODRAW );
	bSizerViewFilter->Add( m_bpButtonShowDeleteRight, 0, wxALIGN_CENTER_VERTICAL|wxALIGN_CENTER_HORIZONTAL, 5 );
	
	m_bpButtonShowUpdateRight = new ToggleButton( m_panelViewFilter, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxSize( 42,42 ), wxBU_AUTODRAW );
	bSizerViewFilter->Add( m_bpButtonShowUpdateRight, 0, wxALIGN_CENTER_VERTICAL|wxALIGN_CENTER_HORIZONTAL, 5 );
	
	m_bpButtonShowCreateRight = new ToggleButton( m_panelViewFilter, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxSize( 42,42 ), wxBU_AUTODRAW );
	bSizerViewFilter->Add( m_bpButtonShowCreateRight, 0, wxALIGN_CENTER_VERTICAL|wxALIGN_CENTER_HORIZONTAL, 5 );
	
	m_bpButtonShowConflict = new ToggleButton( m_panelViewFilter, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxSize( 42,42 ), wxBU_AUTODRAW );
	bSizerViewFilter->Add( m_bpButtonShowConflict, 0, wxALIGN_CENTER_VERTICAL|wxALIGN_CENTER_HORIZONTAL, 5 );
	
	
	bSizerViewFilter->Add( 0, 0, 1, wxEXPAND, 5 );
	
	
	m_panelViewFilter->SetSizer( bSizerViewFilter );
	m_panelViewFilter->Layout();
	bSizerViewFilter->Fit( m_panelViewFilter );
	bSizerPanelHolder->Add( m_panelViewFilter, 0, wxALIGN_CENTER_VERTICAL, 5 );
	
	
	this->SetSizer( bSizerPanelHolder );
	this->Layout();
	
	// Connect Events
	this->Connect( wxEVT_CLOSE_WINDOW, wxCloseEventHandler( MainDialogGenerated::OnClose ) );
	this->Connect( m_menuItemNew->GetId(), wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( MainDialogGenerated::OnConfigNew ) );
	this->Connect( m_menuItemLoad->GetId(), wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( MainDialogGenerated::OnConfigLoad ) );
	this->Connect( m_menuItemSave->GetId(), wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( MainDialogGenerated::OnConfigSave ) );
	this->Connect( m_menuItemSaveAs->GetId(), wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( MainDialogGenerated::OnConfigSaveAs ) );
	this->Connect( m_menuItem7->GetId(), wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( MainDialogGenerated::OnSaveAsBatchJob ) );
	this->Connect( m_menuItem10->GetId(), wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( MainDialogGenerated::OnCompare ) );
	this->Connect( m_menuItem11->GetId(), wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( MainDialogGenerated::OnStartSync ) );
	this->Connect( m_menuItem4->GetId(), wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( MainDialogGenerated::OnMenuQuit ) );
	this->Connect( m_menuItemGlobSett->GetId(), wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( MainDialogGenerated::OnMenuGlobalSettings ) );
	this->Connect( m_menuItem5->GetId(), wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( MainDialogGenerated::OnMenuExportFileList ) );
	this->Connect( m_menuItemManual->GetId(), wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( MainDialogGenerated::OnShowHelp ) );
	this->Connect( m_menuItemCheckVer->GetId(), wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( MainDialogGenerated::OnMenuCheckVersion ) );
	this->Connect( m_menuItemAbout->GetId(), wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( MainDialogGenerated::OnMenuAbout ) );
	m_buttonCompare->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( MainDialogGenerated::OnCompare ), NULL, this );
	m_bpButtonCmpConfig->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( MainDialogGenerated::OnCmpSettings ), NULL, this );
	m_bpButtonCmpConfig->Connect( wxEVT_RIGHT_DOWN, wxMouseEventHandler( MainDialogGenerated::OnCompSettingsContext ), NULL, this );
	m_bpButtonSyncConfig->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( MainDialogGenerated::OnSyncSettings ), NULL, this );
	m_bpButtonSyncConfig->Connect( wxEVT_RIGHT_DOWN, wxMouseEventHandler( MainDialogGenerated::OnSyncSettingsContext ), NULL, this );
	m_buttonSync->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( MainDialogGenerated::OnStartSync ), NULL, this );
	m_bpButtonAddPair->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( MainDialogGenerated::OnAddFolderPair ), NULL, this );
	m_bpButtonRemovePair->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( MainDialogGenerated::OnRemoveTopFolderPair ), NULL, this );
	m_bpButtonSwapSides->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( MainDialogGenerated::OnSwapSides ), NULL, this );
	m_bpButtonOpen->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( MainDialogGenerated::OnConfigLoad ), NULL, this );
	m_bpButtonSave->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( MainDialogGenerated::OnConfigSave ), NULL, this );
	m_bpButtonBatchJob->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( MainDialogGenerated::OnSaveAsBatchJob ), NULL, this );
	m_listBoxHistory->Connect( wxEVT_CHAR, wxKeyEventHandler( MainDialogGenerated::OnCfgHistoryKeyEvent ), NULL, this );
	m_listBoxHistory->Connect( wxEVT_COMMAND_LISTBOX_SELECTED, wxCommandEventHandler( MainDialogGenerated::OnLoadFromHistory ), NULL, this );
	m_listBoxHistory->Connect( wxEVT_COMMAND_LISTBOX_DOUBLECLICKED, wxCommandEventHandler( MainDialogGenerated::OnLoadFromHistoryDoubleClick ), NULL, this );
	m_listBoxHistory->Connect( wxEVT_RIGHT_DOWN, wxMouseEventHandler( MainDialogGenerated::OnCfgHistoryRightClick ), NULL, this );
	m_bpButtonFilter->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( MainDialogGenerated::OnConfigureFilter ), NULL, this );
	m_bpButtonFilter->Connect( wxEVT_RIGHT_DOWN, wxMouseEventHandler( MainDialogGenerated::OnGlobalFilterContext ), NULL, this );
	m_checkBoxHideExcluded->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( MainDialogGenerated::OnShowExcluded ), NULL, this );
	m_bpButtonShowCreateLeft->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( MainDialogGenerated::OnToggleViewButton ), NULL, this );
	m_bpButtonShowCreateLeft->Connect( wxEVT_RIGHT_DOWN, wxMouseEventHandler( MainDialogGenerated::OnViewButtonRightClick ), NULL, this );
	m_bpButtonShowUpdateLeft->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( MainDialogGenerated::OnToggleViewButton ), NULL, this );
	m_bpButtonShowUpdateLeft->Connect( wxEVT_RIGHT_DOWN, wxMouseEventHandler( MainDialogGenerated::OnViewButtonRightClick ), NULL, this );
	m_bpButtonShowDeleteLeft->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( MainDialogGenerated::OnToggleViewButton ), NULL, this );
	m_bpButtonShowDeleteLeft->Connect( wxEVT_RIGHT_DOWN, wxMouseEventHandler( MainDialogGenerated::OnViewButtonRightClick ), NULL, this );
	m_bpButtonShowLeftOnly->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( MainDialogGenerated::OnToggleViewButton ), NULL, this );
	m_bpButtonShowLeftOnly->Connect( wxEVT_RIGHT_DOWN, wxMouseEventHandler( MainDialogGenerated::OnViewButtonRightClick ), NULL, this );
	m_bpButtonShowLeftNewer->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( MainDialogGenerated::OnToggleViewButton ), NULL, this );
	m_bpButtonShowLeftNewer->Connect( wxEVT_RIGHT_DOWN, wxMouseEventHandler( MainDialogGenerated::OnViewButtonRightClick ), NULL, this );
	m_bpButtonShowEqual->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( MainDialogGenerated::OnToggleViewButton ), NULL, this );
	m_bpButtonShowEqual->Connect( wxEVT_RIGHT_DOWN, wxMouseEventHandler( MainDialogGenerated::OnViewButtonRightClick ), NULL, this );
	m_bpButtonShowDifferent->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( MainDialogGenerated::OnToggleViewButton ), NULL, this );
	m_bpButtonShowDifferent->Connect( wxEVT_RIGHT_DOWN, wxMouseEventHandler( MainDialogGenerated::OnViewButtonRightClick ), NULL, this );
	m_bpButtonShowDoNothing->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( MainDialogGenerated::OnToggleViewButton ), NULL, this );
	m_bpButtonShowDoNothing->Connect( wxEVT_RIGHT_DOWN, wxMouseEventHandler( MainDialogGenerated::OnViewButtonRightClick ), NULL, this );
	m_bpButtonShowRightNewer->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( MainDialogGenerated::OnToggleViewButton ), NULL, this );
	m_bpButtonShowRightNewer->Connect( wxEVT_RIGHT_DOWN, wxMouseEventHandler( MainDialogGenerated::OnViewButtonRightClick ), NULL, this );
	m_bpButtonShowRightOnly->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( MainDialogGenerated::OnToggleViewButton ), NULL, this );
	m_bpButtonShowRightOnly->Connect( wxEVT_RIGHT_DOWN, wxMouseEventHandler( MainDialogGenerated::OnViewButtonRightClick ), NULL, this );
	m_bpButtonShowDeleteRight->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( MainDialogGenerated::OnToggleViewButton ), NULL, this );
	m_bpButtonShowDeleteRight->Connect( wxEVT_RIGHT_DOWN, wxMouseEventHandler( MainDialogGenerated::OnViewButtonRightClick ), NULL, this );
	m_bpButtonShowUpdateRight->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( MainDialogGenerated::OnToggleViewButton ), NULL, this );
	m_bpButtonShowUpdateRight->Connect( wxEVT_RIGHT_DOWN, wxMouseEventHandler( MainDialogGenerated::OnViewButtonRightClick ), NULL, this );
	m_bpButtonShowCreateRight->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( MainDialogGenerated::OnToggleViewButton ), NULL, this );
	m_bpButtonShowCreateRight->Connect( wxEVT_RIGHT_DOWN, wxMouseEventHandler( MainDialogGenerated::OnViewButtonRightClick ), NULL, this );
	m_bpButtonShowConflict->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( MainDialogGenerated::OnToggleViewButton ), NULL, this );
	m_bpButtonShowConflict->Connect( wxEVT_RIGHT_DOWN, wxMouseEventHandler( MainDialogGenerated::OnViewButtonRightClick ), NULL, this );
}

MainDialogGenerated::~MainDialogGenerated()
{
	// Disconnect Events
	this->Disconnect( wxEVT_CLOSE_WINDOW, wxCloseEventHandler( MainDialogGenerated::OnClose ) );
	this->Disconnect( wxID_NEW, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( MainDialogGenerated::OnConfigNew ) );
	this->Disconnect( wxID_OPEN, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( MainDialogGenerated::OnConfigLoad ) );
	this->Disconnect( wxID_SAVE, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( MainDialogGenerated::OnConfigSave ) );
	this->Disconnect( wxID_SAVEAS, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( MainDialogGenerated::OnConfigSaveAs ) );
	this->Disconnect( wxID_ANY, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( MainDialogGenerated::OnSaveAsBatchJob ) );
	this->Disconnect( wxID_ANY, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( MainDialogGenerated::OnCompare ) );
	this->Disconnect( wxID_ANY, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( MainDialogGenerated::OnStartSync ) );
	this->Disconnect( wxID_EXIT, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( MainDialogGenerated::OnMenuQuit ) );
	this->Disconnect( wxID_PREFERENCES, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( MainDialogGenerated::OnMenuGlobalSettings ) );
	this->Disconnect( wxID_ANY, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( MainDialogGenerated::OnMenuExportFileList ) );
	this->Disconnect( wxID_HELP, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( MainDialogGenerated::OnShowHelp ) );
	this->Disconnect( wxID_ANY, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( MainDialogGenerated::OnMenuCheckVersion ) );
	this->Disconnect( wxID_ABOUT, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( MainDialogGenerated::OnMenuAbout ) );
	m_buttonCompare->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( MainDialogGenerated::OnCompare ), NULL, this );
	m_bpButtonCmpConfig->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( MainDialogGenerated::OnCmpSettings ), NULL, this );
	m_bpButtonCmpConfig->Disconnect( wxEVT_RIGHT_DOWN, wxMouseEventHandler( MainDialogGenerated::OnCompSettingsContext ), NULL, this );
	m_bpButtonSyncConfig->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( MainDialogGenerated::OnSyncSettings ), NULL, this );
	m_bpButtonSyncConfig->Disconnect( wxEVT_RIGHT_DOWN, wxMouseEventHandler( MainDialogGenerated::OnSyncSettingsContext ), NULL, this );
	m_buttonSync->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( MainDialogGenerated::OnStartSync ), NULL, this );
	m_bpButtonAddPair->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( MainDialogGenerated::OnAddFolderPair ), NULL, this );
	m_bpButtonRemovePair->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( MainDialogGenerated::OnRemoveTopFolderPair ), NULL, this );
	m_bpButtonSwapSides->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( MainDialogGenerated::OnSwapSides ), NULL, this );
	m_bpButtonOpen->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( MainDialogGenerated::OnConfigLoad ), NULL, this );
	m_bpButtonSave->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( MainDialogGenerated::OnConfigSave ), NULL, this );
	m_bpButtonBatchJob->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( MainDialogGenerated::OnSaveAsBatchJob ), NULL, this );
	m_listBoxHistory->Disconnect( wxEVT_CHAR, wxKeyEventHandler( MainDialogGenerated::OnCfgHistoryKeyEvent ), NULL, this );
	m_listBoxHistory->Disconnect( wxEVT_COMMAND_LISTBOX_SELECTED, wxCommandEventHandler( MainDialogGenerated::OnLoadFromHistory ), NULL, this );
	m_listBoxHistory->Disconnect( wxEVT_COMMAND_LISTBOX_DOUBLECLICKED, wxCommandEventHandler( MainDialogGenerated::OnLoadFromHistoryDoubleClick ), NULL, this );
	m_listBoxHistory->Disconnect( wxEVT_RIGHT_DOWN, wxMouseEventHandler( MainDialogGenerated::OnCfgHistoryRightClick ), NULL, this );
	m_bpButtonFilter->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( MainDialogGenerated::OnConfigureFilter ), NULL, this );
	m_bpButtonFilter->Disconnect( wxEVT_RIGHT_DOWN, wxMouseEventHandler( MainDialogGenerated::OnGlobalFilterContext ), NULL, this );
	m_checkBoxHideExcluded->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( MainDialogGenerated::OnShowExcluded ), NULL, this );
	m_bpButtonShowCreateLeft->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( MainDialogGenerated::OnToggleViewButton ), NULL, this );
	m_bpButtonShowCreateLeft->Disconnect( wxEVT_RIGHT_DOWN, wxMouseEventHandler( MainDialogGenerated::OnViewButtonRightClick ), NULL, this );
	m_bpButtonShowUpdateLeft->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( MainDialogGenerated::OnToggleViewButton ), NULL, this );
	m_bpButtonShowUpdateLeft->Disconnect( wxEVT_RIGHT_DOWN, wxMouseEventHandler( MainDialogGenerated::OnViewButtonRightClick ), NULL, this );
	m_bpButtonShowDeleteLeft->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( MainDialogGenerated::OnToggleViewButton ), NULL, this );
	m_bpButtonShowDeleteLeft->Disconnect( wxEVT_RIGHT_DOWN, wxMouseEventHandler( MainDialogGenerated::OnViewButtonRightClick ), NULL, this );
	m_bpButtonShowLeftOnly->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( MainDialogGenerated::OnToggleViewButton ), NULL, this );
	m_bpButtonShowLeftOnly->Disconnect( wxEVT_RIGHT_DOWN, wxMouseEventHandler( MainDialogGenerated::OnViewButtonRightClick ), NULL, this );
	m_bpButtonShowLeftNewer->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( MainDialogGenerated::OnToggleViewButton ), NULL, this );
	m_bpButtonShowLeftNewer->Disconnect( wxEVT_RIGHT_DOWN, wxMouseEventHandler( MainDialogGenerated::OnViewButtonRightClick ), NULL, this );
	m_bpButtonShowEqual->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( MainDialogGenerated::OnToggleViewButton ), NULL, this );
	m_bpButtonShowEqual->Disconnect( wxEVT_RIGHT_DOWN, wxMouseEventHandler( MainDialogGenerated::OnViewButtonRightClick ), NULL, this );
	m_bpButtonShowDifferent->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( MainDialogGenerated::OnToggleViewButton ), NULL, this );
	m_bpButtonShowDifferent->Disconnect( wxEVT_RIGHT_DOWN, wxMouseEventHandler( MainDialogGenerated::OnViewButtonRightClick ), NULL, this );
	m_bpButtonShowDoNothing->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( MainDialogGenerated::OnToggleViewButton ), NULL, this );
	m_bpButtonShowDoNothing->Disconnect( wxEVT_RIGHT_DOWN, wxMouseEventHandler( MainDialogGenerated::OnViewButtonRightClick ), NULL, this );
	m_bpButtonShowRightNewer->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( MainDialogGenerated::OnToggleViewButton ), NULL, this );
	m_bpButtonShowRightNewer->Disconnect( wxEVT_RIGHT_DOWN, wxMouseEventHandler( MainDialogGenerated::OnViewButtonRightClick ), NULL, this );
	m_bpButtonShowRightOnly->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( MainDialogGenerated::OnToggleViewButton ), NULL, this );
	m_bpButtonShowRightOnly->Disconnect( wxEVT_RIGHT_DOWN, wxMouseEventHandler( MainDialogGenerated::OnViewButtonRightClick ), NULL, this );
	m_bpButtonShowDeleteRight->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( MainDialogGenerated::OnToggleViewButton ), NULL, this );
	m_bpButtonShowDeleteRight->Disconnect( wxEVT_RIGHT_DOWN, wxMouseEventHandler( MainDialogGenerated::OnViewButtonRightClick ), NULL, this );
	m_bpButtonShowUpdateRight->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( MainDialogGenerated::OnToggleViewButton ), NULL, this );
	m_bpButtonShowUpdateRight->Disconnect( wxEVT_RIGHT_DOWN, wxMouseEventHandler( MainDialogGenerated::OnViewButtonRightClick ), NULL, this );
	m_bpButtonShowCreateRight->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( MainDialogGenerated::OnToggleViewButton ), NULL, this );
	m_bpButtonShowCreateRight->Disconnect( wxEVT_RIGHT_DOWN, wxMouseEventHandler( MainDialogGenerated::OnViewButtonRightClick ), NULL, this );
	m_bpButtonShowConflict->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( MainDialogGenerated::OnToggleViewButton ), NULL, this );
	m_bpButtonShowConflict->Disconnect( wxEVT_RIGHT_DOWN, wxMouseEventHandler( MainDialogGenerated::OnViewButtonRightClick ), NULL, this );
	
}

FolderPairGenerated::FolderPairGenerated( wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style ) : wxPanel( parent, id, pos, size, style )
{
	wxBoxSizer* bSizer74;
	bSizer74 = new wxBoxSizer( wxHORIZONTAL );
	
	m_panelLeft = new wxPanel( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	m_panelLeft->SetMinSize( wxSize( 1,-1 ) );
	
	wxBoxSizer* bSizer134;
	bSizer134 = new wxBoxSizer( wxHORIZONTAL );
	
	m_bpButtonRemovePair = new wxBitmapButton( m_panelLeft, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxSize( 25,25 ), wxBU_AUTODRAW );
	m_bpButtonRemovePair->SetToolTip( _("Remove folder pair") );
	
	bSizer134->Add( m_bpButtonRemovePair, 0, wxALIGN_CENTER_VERTICAL, 5 );
	
	m_directoryLeft = new FolderHistoryBox( m_panelLeft, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0, NULL, 0 ); 
	bSizer134->Add( m_directoryLeft, 1, wxALIGN_CENTER_VERTICAL, 5 );
	
	m_buttonSelectDirLeft = new wxButton( m_panelLeft, wxID_ANY, _("Browse"), wxDefaultPosition, wxDefaultSize, 0 );
	m_buttonSelectDirLeft->SetToolTip( _("Select a folder") );
	
	bSizer134->Add( m_buttonSelectDirLeft, 0, wxALIGN_CENTER_VERTICAL, 5 );
	
	
	m_panelLeft->SetSizer( bSizer134 );
	m_panelLeft->Layout();
	bSizer134->Fit( m_panelLeft );
	bSizer74->Add( m_panelLeft, 0, wxALIGN_CENTER_VERTICAL|wxLEFT|wxEXPAND, 5 );
	
	m_panel20 = new wxPanel( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	wxBoxSizer* bSizer95;
	bSizer95 = new wxBoxSizer( wxHORIZONTAL );
	
	
	bSizer95->Add( 0, 0, 1, wxEXPAND, 5 );
	
	m_bpButtonAltCompCfg = new wxBitmapButton( m_panel20, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxSize( 25,25 ), wxBU_AUTODRAW );
	bSizer95->Add( m_bpButtonAltCompCfg, 0, wxALIGN_CENTER_VERTICAL, 5 );
	
	m_bpButtonLocalFilter = new wxBitmapButton( m_panel20, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxSize( 25,25 ), wxBU_AUTODRAW );
	bSizer95->Add( m_bpButtonLocalFilter, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 2 );
	
	m_bpButtonAltSyncCfg = new wxBitmapButton( m_panel20, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxSize( 25,25 ), wxBU_AUTODRAW );
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
	
	m_buttonSelectDirRight = new wxButton( m_panelRight, wxID_ANY, _("Browse"), wxDefaultPosition, wxDefaultSize, 0 );
	m_buttonSelectDirRight->SetToolTip( _("Select a folder") );
	
	bSizer135->Add( m_buttonSelectDirRight, 0, wxALIGN_CENTER_VERTICAL, 5 );
	
	
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

CompareProgressDlgGenerated::CompareProgressDlgGenerated( wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style ) : wxPanel( parent, id, pos, size, style )
{
	this->SetBackgroundColour( wxSystemSettings::GetColour( wxSYS_COLOUR_BTNFACE ) );
	
	wxBoxSizer* bSizer40;
	bSizer40 = new wxBoxSizer( wxVERTICAL );
	
	
	bSizer40->Add( 0, 0, 1, wxEXPAND, 5 );
	
	wxBoxSizer* bSizer182;
	bSizer182 = new wxBoxSizer( wxVERTICAL );
	
	m_textCtrlStatus = new wxTextCtrl( this, wxID_ANY, _("dummy"), wxDefaultPosition, wxDefaultSize, wxTE_READONLY );
	m_textCtrlStatus->SetMaxLength( 0 ); 
	m_textCtrlStatus->SetBackgroundColour( wxSystemSettings::GetColour( wxSYS_COLOUR_WINDOW ) );
	
	bSizer182->Add( m_textCtrlStatus, 0, wxEXPAND, 5 );
	
	
	bSizer40->Add( bSizer182, 0, wxRIGHT|wxLEFT|wxEXPAND, 5 );
	
	m_gauge2 = new wxGauge( this, wxID_ANY, 100, wxDefaultPosition, wxSize( -1,14 ), wxGA_HORIZONTAL|wxGA_SMOOTH );
	bSizer40->Add( m_gauge2, 0, wxEXPAND|wxTOP|wxRIGHT|wxLEFT, 5 );
	
	bSizer42 = new wxBoxSizer( wxHORIZONTAL );
	
	wxBoxSizer* bSizer157;
	bSizer157 = new wxBoxSizer( wxVERTICAL );
	
	bSizerFilesFound = new wxBoxSizer( wxHORIZONTAL );
	
	m_staticText321 = new wxStaticText( this, wxID_ANY, _("Items found:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText321->Wrap( -1 );
	m_staticText321->SetFont( wxFont( wxNORMAL_FONT->GetPointSize(), 70, 90, 90, false, wxEmptyString ) );
	
	bSizerFilesFound->Add( m_staticText321, 0, wxALIGN_BOTTOM, 5 );
	
	m_staticTextScanned = new wxStaticText( this, wxID_ANY, _("dummy"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextScanned->Wrap( -1 );
	m_staticTextScanned->SetFont( wxFont( wxNORMAL_FONT->GetPointSize(), 70, 90, 92, false, wxEmptyString ) );
	
	bSizerFilesFound->Add( m_staticTextScanned, 0, wxALIGN_BOTTOM|wxLEFT, 5 );
	
	
	bSizer157->Add( bSizerFilesFound, 0, 0, 5 );
	
	bSizerFilesRemaining = new wxBoxSizer( wxHORIZONTAL );
	
	m_staticText46 = new wxStaticText( this, wxID_ANY, _("Items remaining:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText46->Wrap( -1 );
	m_staticText46->SetFont( wxFont( wxNORMAL_FONT->GetPointSize(), 70, 90, 90, false, wxEmptyString ) );
	
	bSizerFilesRemaining->Add( m_staticText46, 0, wxALIGN_BOTTOM, 5 );
	
	wxBoxSizer* bSizer154;
	bSizer154 = new wxBoxSizer( wxHORIZONTAL );
	
	m_staticTextFilesRemaining = new wxStaticText( this, wxID_ANY, _("dummy"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextFilesRemaining->Wrap( -1 );
	m_staticTextFilesRemaining->SetFont( wxFont( wxNORMAL_FONT->GetPointSize(), 70, 90, 92, false, wxEmptyString ) );
	
	bSizer154->Add( m_staticTextFilesRemaining, 0, wxALIGN_BOTTOM, 5 );
	
	m_staticTextDataRemaining = new wxStaticText( this, wxID_ANY, _("dummy"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextDataRemaining->Wrap( -1 );
	m_staticTextDataRemaining->SetFont( wxFont( wxNORMAL_FONT->GetPointSize(), 70, 90, 90, false, wxEmptyString ) );
	
	bSizer154->Add( m_staticTextDataRemaining, 0, wxALIGN_BOTTOM|wxLEFT, 5 );
	
	
	bSizerFilesRemaining->Add( bSizer154, 0, wxALIGN_BOTTOM|wxLEFT, 5 );
	
	
	bSizer157->Add( bSizerFilesRemaining, 0, 0, 5 );
	
	
	bSizer42->Add( bSizer157, 0, wxALIGN_CENTER_VERTICAL, 5 );
	
	
	bSizer42->Add( 0, 0, 1, wxEXPAND, 5 );
	
	sSizerSpeed = new wxBoxSizer( wxHORIZONTAL );
	
	m_staticText104 = new wxStaticText( this, wxID_ANY, _("Speed:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText104->Wrap( -1 );
	m_staticText104->SetFont( wxFont( wxNORMAL_FONT->GetPointSize(), 70, 90, 90, false, wxEmptyString ) );
	
	sSizerSpeed->Add( m_staticText104, 0, wxALIGN_BOTTOM, 5 );
	
	m_staticTextSpeed = new wxStaticText( this, wxID_ANY, _("dummy"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextSpeed->Wrap( -1 );
	m_staticTextSpeed->SetFont( wxFont( wxNORMAL_FONT->GetPointSize(), 70, 90, 92, false, wxEmptyString ) );
	
	sSizerSpeed->Add( m_staticTextSpeed, 0, wxLEFT|wxALIGN_BOTTOM, 5 );
	
	
	bSizer42->Add( sSizerSpeed, 0, wxALIGN_CENTER_VERTICAL, 5 );
	
	
	bSizer42->Add( 10, 0, 0, wxALIGN_CENTER_VERTICAL, 5 );
	
	sSizerTimeRemaining = new wxBoxSizer( wxHORIZONTAL );
	
	m_staticTextTimeRemFixed = new wxStaticText( this, wxID_ANY, _("Time remaining:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextTimeRemFixed->Wrap( -1 );
	m_staticTextTimeRemFixed->SetFont( wxFont( wxNORMAL_FONT->GetPointSize(), 70, 90, 90, false, wxEmptyString ) );
	
	sSizerTimeRemaining->Add( m_staticTextTimeRemFixed, 0, wxALIGN_BOTTOM, 5 );
	
	m_staticTextRemTime = new wxStaticText( this, wxID_ANY, _("dummy"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextRemTime->Wrap( -1 );
	m_staticTextRemTime->SetFont( wxFont( wxNORMAL_FONT->GetPointSize(), 70, 90, 92, false, wxEmptyString ) );
	
	sSizerTimeRemaining->Add( m_staticTextRemTime, 0, wxLEFT|wxALIGN_BOTTOM, 5 );
	
	
	bSizer42->Add( sSizerTimeRemaining, 0, wxALIGN_CENTER_VERTICAL, 5 );
	
	
	bSizer42->Add( 0, 0, 1, wxEXPAND, 5 );
	
	sSizerTimeElapsed = new wxBoxSizer( wxHORIZONTAL );
	
	wxStaticText* m_staticText37;
	m_staticText37 = new wxStaticText( this, wxID_ANY, _("Time elapsed:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText37->Wrap( -1 );
	m_staticText37->SetFont( wxFont( wxNORMAL_FONT->GetPointSize(), 70, 90, 90, false, wxEmptyString ) );
	
	sSizerTimeElapsed->Add( m_staticText37, 0, wxALIGN_BOTTOM, 5 );
	
	m_staticTextTimeElapsed = new wxStaticText( this, wxID_ANY, _("dummy"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextTimeElapsed->Wrap( -1 );
	m_staticTextTimeElapsed->SetFont( wxFont( wxNORMAL_FONT->GetPointSize(), 70, 90, 92, false, wxEmptyString ) );
	
	sSizerTimeElapsed->Add( m_staticTextTimeElapsed, 0, wxLEFT|wxALIGN_BOTTOM, 5 );
	
	
	bSizer42->Add( sSizerTimeElapsed, 0, wxALIGN_CENTER_VERTICAL, 5 );
	
	
	bSizer40->Add( bSizer42, 0, wxALIGN_CENTER_HORIZONTAL|wxEXPAND|wxTOP|wxRIGHT|wxLEFT, 5 );
	
	
	bSizer40->Add( 0, 0, 1, wxEXPAND, 5 );
	
	
	this->SetSizer( bSizer40 );
	this->Layout();
	bSizer40->Fit( this );
}

CompareProgressDlgGenerated::~CompareProgressDlgGenerated()
{
}

SyncProgressDlgGenerated::SyncProgressDlgGenerated( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : wxFrame( parent, id, title, pos, size, style )
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
	
	m_bitmapStatus = new wxStaticBitmap( m_panelHeader, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxSize( 32,32 ), 0 );
	bSizer42->Add( m_bitmapStatus, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );
	
	m_staticTextPhase = new wxStaticText( m_panelHeader, wxID_ANY, _("Synchronizing..."), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextPhase->Wrap( -1 );
	m_staticTextPhase->SetFont( wxFont( wxNORMAL_FONT->GetPointSize(), 70, 90, 92, false, wxEmptyString ) );
	
	bSizer42->Add( m_staticTextPhase, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxTOP|wxBOTTOM, 5 );
	
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
	
	m_staticTextStatus = new wxStaticText( m_panelProgress, wxID_ANY, _("dummy"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextStatus->Wrap( -1 );
	bSizer173->Add( m_staticTextStatus, 0, wxALL|wxEXPAND, 5 );
	
	m_gauge1 = new wxGauge( m_panelProgress, wxID_ANY, 100, wxDefaultPosition, wxSize( -1,14 ), wxGA_HORIZONTAL );
	bSizer173->Add( m_gauge1, 0, wxEXPAND|wxRIGHT|wxLEFT, 5 );
	
	
	bSizer173->Add( 0, 10, 0, 0, 5 );
	
	bSizer171 = new wxBoxSizer( wxHORIZONTAL );
	
	
	bSizer171->Add( 10, 0, 0, 0, 5 );
	
	wxFlexGridSizer* fgSizer10;
	fgSizer10 = new wxFlexGridSizer( 0, 2, 2, 5 );
	fgSizer10->SetFlexibleDirection( wxBOTH );
	fgSizer10->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );
	
	m_staticTextLabelItemsProc = new wxStaticText( m_panelProgress, wxID_ANY, _("Items processed:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextLabelItemsProc->Wrap( -1 );
	m_staticTextLabelItemsProc->SetFont( wxFont( wxNORMAL_FONT->GetPointSize(), 70, 90, 90, false, wxEmptyString ) );
	
	fgSizer10->Add( m_staticTextLabelItemsProc, 0, wxALIGN_BOTTOM, 5 );
	
	bSizerItemsProc = new wxBoxSizer( wxHORIZONTAL );
	
	m_staticTextProcessedObj = new wxStaticText( m_panelProgress, wxID_ANY, _("dummy"), wxDefaultPosition, wxSize( -1,-1 ), 0 );
	m_staticTextProcessedObj->Wrap( -1 );
	m_staticTextProcessedObj->SetFont( wxFont( wxNORMAL_FONT->GetPointSize(), 70, 90, 92, false, wxEmptyString ) );
	
	bSizerItemsProc->Add( m_staticTextProcessedObj, 0, wxALIGN_BOTTOM, 5 );
	
	m_staticTextDataProcessed = new wxStaticText( m_panelProgress, wxID_ANY, _("dummy"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextDataProcessed->Wrap( -1 );
	m_staticTextDataProcessed->SetFont( wxFont( wxNORMAL_FONT->GetPointSize(), 70, 90, 90, false, wxEmptyString ) );
	
	bSizerItemsProc->Add( m_staticTextDataProcessed, 0, wxALIGN_BOTTOM|wxLEFT, 5 );
	
	
	fgSizer10->Add( bSizerItemsProc, 0, wxALIGN_BOTTOM, 5 );
	
	m_staticTextLabelItemsRem = new wxStaticText( m_panelProgress, wxID_ANY, _("Items remaining:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextLabelItemsRem->Wrap( -1 );
	m_staticTextLabelItemsRem->SetFont( wxFont( wxNORMAL_FONT->GetPointSize(), 70, 90, 90, false, wxEmptyString ) );
	
	fgSizer10->Add( m_staticTextLabelItemsRem, 0, wxALIGN_BOTTOM, 5 );
	
	bSizerItemsRem = new wxBoxSizer( wxHORIZONTAL );
	
	m_staticTextRemainingObj = new wxStaticText( m_panelProgress, wxID_ANY, _("dummy"), wxDefaultPosition, wxSize( -1,-1 ), 0 );
	m_staticTextRemainingObj->Wrap( -1 );
	m_staticTextRemainingObj->SetFont( wxFont( wxNORMAL_FONT->GetPointSize(), 70, 90, 92, false, wxEmptyString ) );
	
	bSizerItemsRem->Add( m_staticTextRemainingObj, 0, wxALIGN_BOTTOM, 5 );
	
	m_staticTextDataRemaining = new wxStaticText( m_panelProgress, wxID_ANY, _("dummy"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextDataRemaining->Wrap( -1 );
	m_staticTextDataRemaining->SetFont( wxFont( wxNORMAL_FONT->GetPointSize(), 70, 90, 90, false, wxEmptyString ) );
	
	bSizerItemsRem->Add( m_staticTextDataRemaining, 0, wxALIGN_BOTTOM|wxLEFT, 5 );
	
	
	fgSizer10->Add( bSizerItemsRem, 0, wxALIGN_BOTTOM, 5 );
	
	m_staticText84 = new wxStaticText( m_panelProgress, wxID_ANY, _("Speed:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText84->Wrap( -1 );
	m_staticText84->SetFont( wxFont( wxNORMAL_FONT->GetPointSize(), 70, 90, 90, false, wxEmptyString ) );
	
	fgSizer10->Add( m_staticText84, 0, wxALIGN_BOTTOM, 5 );
	
	m_staticTextSpeed = new wxStaticText( m_panelProgress, wxID_ANY, _("dummy"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextSpeed->Wrap( -1 );
	m_staticTextSpeed->SetFont( wxFont( wxNORMAL_FONT->GetPointSize(), 70, 90, 92, false, wxEmptyString ) );
	
	fgSizer10->Add( m_staticTextSpeed, 0, wxALIGN_BOTTOM, 5 );
	
	m_staticTextLabelRemTime = new wxStaticText( m_panelProgress, wxID_ANY, _("Time remaining:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextLabelRemTime->Wrap( -1 );
	m_staticTextLabelRemTime->SetFont( wxFont( wxNORMAL_FONT->GetPointSize(), 70, 90, 90, false, wxEmptyString ) );
	
	fgSizer10->Add( m_staticTextLabelRemTime, 0, wxALIGN_BOTTOM, 5 );
	
	m_staticTextRemTime = new wxStaticText( m_panelProgress, wxID_ANY, _("dummy"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextRemTime->Wrap( -1 );
	m_staticTextRemTime->SetFont( wxFont( wxNORMAL_FONT->GetPointSize(), 70, 90, 92, false, wxEmptyString ) );
	
	fgSizer10->Add( m_staticTextRemTime, 0, wxALIGN_BOTTOM, 5 );
	
	m_staticTextLabelElapsedTime = new wxStaticText( m_panelProgress, wxID_ANY, _("Time elapsed:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextLabelElapsedTime->Wrap( -1 );
	m_staticTextLabelElapsedTime->SetFont( wxFont( wxNORMAL_FONT->GetPointSize(), 70, 90, 90, false, wxEmptyString ) );
	
	fgSizer10->Add( m_staticTextLabelElapsedTime, 0, wxALIGN_BOTTOM, 5 );
	
	m_staticTextTimeElapsed = new wxStaticText( m_panelProgress, wxID_ANY, _("dummy"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextTimeElapsed->Wrap( -1 );
	m_staticTextTimeElapsed->SetFont( wxFont( wxNORMAL_FONT->GetPointSize(), 70, 90, 92, false, wxEmptyString ) );
	
	fgSizer10->Add( m_staticTextTimeElapsed, 0, wxALIGN_BOTTOM, 5 );
	
	
	bSizer171->Add( fgSizer10, 0, wxALIGN_CENTER_VERTICAL, 5 );
	
	
	bSizer171->Add( 10, 0, 0, 0, 5 );
	
	m_panelGraph = new zen::Graph2D( m_panelProgress, wxID_ANY, wxDefaultPosition, wxSize( 340,150 ), wxTAB_TRAVERSAL );
	m_panelGraph->SetBackgroundColour( wxSystemSettings::GetColour( wxSYS_COLOUR_WINDOW ) );
	
	bSizer171->Add( m_panelGraph, 1, wxALIGN_CENTER_VERTICAL|wxEXPAND, 5 );
	
	
	bSizer173->Add( bSizer171, 1, wxEXPAND, 5 );
	
	
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
	
	m_staticText87 = new wxStaticText( m_panelFooter, wxID_ANY, _("On completion"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText87->Wrap( -1 );
	m_staticText87->SetFont( wxFont( wxNORMAL_FONT->GetPointSize(), 70, 90, 90, false, wxEmptyString ) );
	
	bSizerExecFinished->Add( m_staticText87, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );
	
	m_comboBoxExecFinished = new ExecFinishedBox( m_panelFooter, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0, NULL, 0 ); 
	bSizerExecFinished->Add( m_comboBoxExecFinished, 1, wxALIGN_CENTER_VERTICAL, 5 );
	
	
	bSizer182->Add( bSizerExecFinished, 0, wxTOP|wxRIGHT|wxLEFT|wxALIGN_CENTER_HORIZONTAL|wxEXPAND, 5 );
	
	bSizer28 = new wxBoxSizer( wxHORIZONTAL );
	
	
	bSizer28->Add( 0, 0, 1, 0, 5 );
	
	m_buttonOK = new wxButton( m_panelFooter, wxID_OK, _("OK"), wxDefaultPosition, wxSize( -1,30 ), 0 );
	m_buttonOK->SetFont( wxFont( wxNORMAL_FONT->GetPointSize(), 70, 90, 90, false, wxEmptyString ) );
	m_buttonOK->Enable( false );
	
	bSizer28->Add( m_buttonOK, 0, wxALIGN_CENTER_VERTICAL|wxTOP|wxBOTTOM|wxLEFT, 5 );
	
	m_buttonPause = new wxButton( m_panelFooter, wxID_ANY, _("&Pause"), wxDefaultPosition, wxSize( -1,30 ), 0 );
	m_buttonPause->SetFont( wxFont( wxNORMAL_FONT->GetPointSize(), 70, 90, 90, false, wxEmptyString ) );
	
	bSizer28->Add( m_buttonPause, 0, wxALIGN_CENTER_VERTICAL|wxTOP|wxBOTTOM|wxLEFT, 5 );
	
	m_buttonAbort = new wxButton( m_panelFooter, wxID_CANCEL, _("Cancel"), wxDefaultPosition, wxSize( -1,30 ), 0 );
	m_buttonAbort->SetFont( wxFont( wxNORMAL_FONT->GetPointSize(), 70, 90, 90, false, wxEmptyString ) );
	
	bSizer28->Add( m_buttonAbort, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );
	
	
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
	this->Connect( wxEVT_CLOSE_WINDOW, wxCloseEventHandler( SyncProgressDlgGenerated::OnClose ) );
	this->Connect( wxEVT_ICONIZE, wxIconizeEventHandler( SyncProgressDlgGenerated::OnIconize ) );
	m_buttonOK->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( SyncProgressDlgGenerated::OnOkay ), NULL, this );
	m_buttonPause->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( SyncProgressDlgGenerated::OnPause ), NULL, this );
	m_buttonAbort->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( SyncProgressDlgGenerated::OnAbort ), NULL, this );
}

SyncProgressDlgGenerated::~SyncProgressDlgGenerated()
{
	// Disconnect Events
	this->Disconnect( wxEVT_CLOSE_WINDOW, wxCloseEventHandler( SyncProgressDlgGenerated::OnClose ) );
	this->Disconnect( wxEVT_ICONIZE, wxIconizeEventHandler( SyncProgressDlgGenerated::OnIconize ) );
	m_buttonOK->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( SyncProgressDlgGenerated::OnOkay ), NULL, this );
	m_buttonPause->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( SyncProgressDlgGenerated::OnPause ), NULL, this );
	m_buttonAbort->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( SyncProgressDlgGenerated::OnAbort ), NULL, this );
	
}

BatchDlgGenerated::BatchDlgGenerated( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : wxDialog( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxSize( 400,260 ), wxDefaultSize );
	this->SetBackgroundColour( wxSystemSettings::GetColour( wxSYS_COLOUR_WINDOW ) );
	
	wxBoxSizer* bSizer54;
	bSizer54 = new wxBoxSizer( wxVERTICAL );
	
	m_panelHeader = new wxPanel( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	m_panelHeader->SetBackgroundColour( wxSystemSettings::GetColour( wxSYS_COLOUR_BTNFACE ) );
	
	wxBoxSizer* bSizer72;
	bSizer72 = new wxBoxSizer( wxHORIZONTAL );
	
	m_bitmapBatchJob = new wxStaticBitmap( m_panelHeader, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxSize( -1,-1 ), 0 );
	bSizer72->Add( m_bitmapBatchJob, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );
	
	m_staticTextHeader = new wxStaticText( m_panelHeader, wxID_ANY, _("Batch job"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextHeader->Wrap( -1 );
	m_staticTextHeader->SetFont( wxFont( wxNORMAL_FONT->GetPointSize(), 70, 90, 92, false, wxEmptyString ) );
	
	bSizer72->Add( m_staticTextHeader, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );
	
	m_staticText44 = new wxStaticText( m_panelHeader, wxID_ANY, _("Create a batch file to automate synchronization. Double-click this file or schedule in your system's task planner: FreeFileSync.exe <job name>.ffs_batch"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText44->Wrap( 480 );
	bSizer72->Add( m_staticText44, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );
	
	
	bSizer72->Add( 0, 0, 1, wxEXPAND, 5 );
	
	m_bpButtonHelp = new wxBitmapButton( m_panelHeader, wxID_HELP, wxNullBitmap, wxDefaultPosition, wxSize( 42,42 ), wxBU_AUTODRAW );
	m_bpButtonHelp->SetToolTip( _("Help") );
	
	bSizer72->Add( m_bpButtonHelp, 0, wxALIGN_CENTER_VERTICAL, 5 );
	
	
	m_panelHeader->SetSizer( bSizer72 );
	m_panelHeader->Layout();
	bSizer72->Fit( m_panelHeader );
	bSizer54->Add( m_panelHeader, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxEXPAND, 5 );
	
	m_staticline18 = new wxStaticLine( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	bSizer54->Add( m_staticline18, 0, wxEXPAND, 5 );
	
	wxBoxSizer* bSizer172;
	bSizer172 = new wxBoxSizer( wxVERTICAL );
	
	wxBoxSizer* bSizer180;
	bSizer180 = new wxBoxSizer( wxHORIZONTAL );
	
	wxBoxSizer* bSizer171;
	bSizer171 = new wxBoxSizer( wxVERTICAL );
	
	m_staticText82 = new wxStaticText( this, wxID_ANY, _("Error handling"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText82->Wrap( -1 );
	bSizer171->Add( m_staticText82, 0, wxBOTTOM, 5 );
	
	wxBoxSizer* bSizer169;
	bSizer169 = new wxBoxSizer( wxHORIZONTAL );
	
	m_toggleBtnErrorIgnore = new wxToggleButton( this, wxID_ANY, _("Ignore"), wxDefaultPosition, wxDefaultSize, 0 );
	m_toggleBtnErrorIgnore->SetToolTip( _("Hide all error and warning messages") );
	
	bSizer169->Add( m_toggleBtnErrorIgnore, 0, wxRIGHT|wxALIGN_CENTER_VERTICAL, 5 );
	
	m_toggleBtnErrorPopup = new wxToggleButton( this, wxID_ANY, _("Pop-up"), wxDefaultPosition, wxDefaultSize, 0 );
	m_toggleBtnErrorPopup->SetToolTip( _("Show pop-up on errors or warnings") );
	
	bSizer169->Add( m_toggleBtnErrorPopup, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );
	
	m_toggleBtnErrorExit = new wxToggleButton( this, wxID_ANY, _("Exit"), wxDefaultPosition, wxDefaultSize, 0 );
	m_toggleBtnErrorExit->SetToolTip( _("Abort synchronization on first error") );
	
	bSizer169->Add( m_toggleBtnErrorExit, 0, wxALIGN_CENTER_VERTICAL, 5 );
	
	
	bSizer171->Add( bSizer169, 0, wxALIGN_CENTER_VERTICAL, 5 );
	
	
	bSizer180->Add( bSizer171, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );
	
	m_staticline26 = new wxStaticLine( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_VERTICAL );
	bSizer180->Add( m_staticline26, 0, wxEXPAND, 5 );
	
	wxBoxSizer* bSizer170;
	bSizer170 = new wxBoxSizer( wxVERTICAL );
	
	m_staticText81 = new wxStaticText( this, wxID_ANY, _("On completion"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText81->Wrap( -1 );
	bSizer170->Add( m_staticText81, 0, wxBOTTOM, 5 );
	
	m_comboBoxExecFinished = new ExecFinishedBox( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0, NULL, 0 ); 
	bSizer170->Add( m_comboBoxExecFinished, 0, wxALIGN_CENTER_VERTICAL|wxEXPAND, 5 );
	
	
	bSizer180->Add( bSizer170, 1, wxEXPAND|wxALIGN_CENTER_VERTICAL|wxALL, 5 );
	
	
	bSizer172->Add( bSizer180, 0, wxEXPAND, 5 );
	
	m_staticline25 = new wxStaticLine( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	bSizer172->Add( m_staticline25, 0, wxEXPAND, 5 );
	
	m_checkBoxShowProgress = new wxCheckBox( this, wxID_ANY, _("Show progress dialog"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizer172->Add( m_checkBoxShowProgress, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxEXPAND, 5 );
	
	m_checkBoxGenerateLogfile = new wxCheckBox( this, wxID_ANY, _("Save log"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizer172->Add( m_checkBoxGenerateLogfile, 0, wxEXPAND|wxALL, 5 );
	
	m_panelLogfile = new wxPanel( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	wxBoxSizer* bSizer1721;
	bSizer1721 = new wxBoxSizer( wxHORIZONTAL );
	
	m_comboBoxLogfileDir = new FolderHistoryBox( m_panelLogfile, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0, NULL, 0 ); 
	bSizer1721->Add( m_comboBoxLogfileDir, 1, wxALIGN_CENTER_VERTICAL, 5 );
	
	m_buttonSelectLogfileDir = new wxButton( m_panelLogfile, wxID_ANY, _("Browse"), wxDefaultPosition, wxDefaultSize, 0 );
	m_buttonSelectLogfileDir->SetToolTip( _("Select folder to save log files") );
	
	bSizer1721->Add( m_buttonSelectLogfileDir, 0, wxALIGN_CENTER_VERTICAL, 5 );
	
	m_checkBoxLogfilesLimit = new wxCheckBox( m_panelLogfile, wxID_ANY, _("Limit"), wxDefaultPosition, wxDefaultSize, 0 );
	m_checkBoxLogfilesLimit->SetToolTip( _("Limit maximum number of log files") );
	
	bSizer1721->Add( m_checkBoxLogfilesLimit, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );
	
	m_spinCtrlLogfileLimit = new wxSpinCtrl( m_panelLogfile, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize( 60,-1 ), wxSP_ARROW_KEYS, 1, 2000000000, 1 );
	m_spinCtrlLogfileLimit->SetToolTip( _("Limit maximum number of log files") );
	
	bSizer1721->Add( m_spinCtrlLogfileLimit, 0, wxALIGN_CENTER_VERTICAL, 5 );
	
	
	m_panelLogfile->SetSizer( bSizer1721 );
	m_panelLogfile->Layout();
	bSizer1721->Fit( m_panelLogfile );
	bSizer172->Add( m_panelLogfile, 0, wxALIGN_CENTER_VERTICAL|wxEXPAND|wxBOTTOM|wxRIGHT|wxLEFT, 5 );
	
	
	bSizer54->Add( bSizer172, 1, wxEXPAND, 10 );
	
	m_staticline13 = new wxStaticLine( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	bSizer54->Add( m_staticline13, 0, wxEXPAND, 5 );
	
	m_panel35 = new wxPanel( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	m_panel35->SetBackgroundColour( wxSystemSettings::GetColour( wxSYS_COLOUR_BTNFACE ) );
	
	wxBoxSizer* bSizer68;
	bSizer68 = new wxBoxSizer( wxHORIZONTAL );
	
	
	bSizer68->Add( 0, 0, 1, wxEXPAND, 5 );
	
	m_buttonSave = new wxButton( m_panel35, wxID_SAVE, _("Save &as..."), wxDefaultPosition, wxSize( -1,30 ), 0 );
	m_buttonSave->SetDefault(); 
	m_buttonSave->SetFont( wxFont( wxNORMAL_FONT->GetPointSize(), 70, 90, 92, false, wxEmptyString ) );
	
	bSizer68->Add( m_buttonSave, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxTOP|wxBOTTOM|wxLEFT, 5 );
	
	m_button6 = new wxButton( m_panel35, wxID_CANCEL, _("Cancel"), wxDefaultPosition, wxSize( -1,30 ), 0 );
	m_button6->SetFont( wxFont( wxNORMAL_FONT->GetPointSize(), 70, 90, 90, false, wxEmptyString ) );
	
	bSizer68->Add( m_button6, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );
	
	
	bSizer68->Add( 0, 0, 1, wxEXPAND, 5 );
	
	
	m_panel35->SetSizer( bSizer68 );
	m_panel35->Layout();
	bSizer68->Fit( m_panel35 );
	bSizer54->Add( m_panel35, 0, wxALIGN_CENTER_HORIZONTAL|wxEXPAND, 5 );
	
	
	this->SetSizer( bSizer54 );
	this->Layout();
	bSizer54->Fit( this );
	
	this->Centre( wxBOTH );
	
	// Connect Events
	this->Connect( wxEVT_CLOSE_WINDOW, wxCloseEventHandler( BatchDlgGenerated::OnClose ) );
	m_bpButtonHelp->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( BatchDlgGenerated::OnHelp ), NULL, this );
	m_toggleBtnErrorIgnore->Connect( wxEVT_COMMAND_TOGGLEBUTTON_CLICKED, wxCommandEventHandler( BatchDlgGenerated::OnErrorIgnore ), NULL, this );
	m_toggleBtnErrorPopup->Connect( wxEVT_COMMAND_TOGGLEBUTTON_CLICKED, wxCommandEventHandler( BatchDlgGenerated::OnErrorPopup ), NULL, this );
	m_toggleBtnErrorExit->Connect( wxEVT_COMMAND_TOGGLEBUTTON_CLICKED, wxCommandEventHandler( BatchDlgGenerated::OnErrorExit ), NULL, this );
	m_checkBoxGenerateLogfile->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( BatchDlgGenerated::OnToggleGenerateLogfile ), NULL, this );
	m_checkBoxLogfilesLimit->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( BatchDlgGenerated::OnToggleLogfilesLimit ), NULL, this );
	m_buttonSave->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( BatchDlgGenerated::OnSaveBatchJob ), NULL, this );
	m_button6->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( BatchDlgGenerated::OnCancel ), NULL, this );
}

BatchDlgGenerated::~BatchDlgGenerated()
{
	// Disconnect Events
	this->Disconnect( wxEVT_CLOSE_WINDOW, wxCloseEventHandler( BatchDlgGenerated::OnClose ) );
	m_bpButtonHelp->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( BatchDlgGenerated::OnHelp ), NULL, this );
	m_toggleBtnErrorIgnore->Disconnect( wxEVT_COMMAND_TOGGLEBUTTON_CLICKED, wxCommandEventHandler( BatchDlgGenerated::OnErrorIgnore ), NULL, this );
	m_toggleBtnErrorPopup->Disconnect( wxEVT_COMMAND_TOGGLEBUTTON_CLICKED, wxCommandEventHandler( BatchDlgGenerated::OnErrorPopup ), NULL, this );
	m_toggleBtnErrorExit->Disconnect( wxEVT_COMMAND_TOGGLEBUTTON_CLICKED, wxCommandEventHandler( BatchDlgGenerated::OnErrorExit ), NULL, this );
	m_checkBoxGenerateLogfile->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( BatchDlgGenerated::OnToggleGenerateLogfile ), NULL, this );
	m_checkBoxLogfilesLimit->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( BatchDlgGenerated::OnToggleLogfilesLimit ), NULL, this );
	m_buttonSave->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( BatchDlgGenerated::OnSaveBatchJob ), NULL, this );
	m_button6->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( BatchDlgGenerated::OnCancel ), NULL, this );
	
}

CmpCfgDlgGenerated::CmpCfgDlgGenerated( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : wxDialog( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxDefaultSize, wxDefaultSize );
	this->SetBackgroundColour( wxSystemSettings::GetColour( wxSYS_COLOUR_WINDOW ) );
	
	wxBoxSizer* bSizer136;
	bSizer136 = new wxBoxSizer( wxVERTICAL );
	
	m_staticText91 = new wxStaticText( this, wxID_ANY, _("Select variant"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText91->Wrap( -1 );
	bSizer136->Add( m_staticText91, 0, wxALL, 5 );
	
	wxFlexGridSizer* fgSizer16;
	fgSizer16 = new wxFlexGridSizer( 2, 2, 8, 5 );
	fgSizer16->SetFlexibleDirection( wxBOTH );
	fgSizer16->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );
	
	m_bitmapByTime = new wxStaticBitmap( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, 0 );
	m_bitmapByTime->SetToolTip( _("Files are found equal if\n     - last write time and date\n     - file size\nare the same") );
	
	fgSizer16->Add( m_bitmapByTime, 0, wxALIGN_CENTER_VERTICAL|wxALIGN_CENTER_HORIZONTAL, 5 );
	
	m_toggleBtnTimeSize = new wxToggleButton( this, wxID_ANY, _("File time and size"), wxDefaultPosition, wxSize( -1,-1 ), 0 );
	m_toggleBtnTimeSize->SetValue( true ); 
	m_toggleBtnTimeSize->SetFont( wxFont( wxNORMAL_FONT->GetPointSize(), 70, 90, 92, false, wxEmptyString ) );
	m_toggleBtnTimeSize->SetToolTip( _("Files are found equal if\n     - last write time and date\n     - file size\nare the same") );
	
	fgSizer16->Add( m_toggleBtnTimeSize, 0, wxALIGN_CENTER_VERTICAL|wxEXPAND, 5 );
	
	m_bitmapByContent = new wxStaticBitmap( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, 0 );
	m_bitmapByContent->SetToolTip( _("Files are found equal if\n     - file content\nis the same") );
	
	fgSizer16->Add( m_bitmapByContent, 0, wxALIGN_CENTER_VERTICAL|wxALIGN_CENTER_HORIZONTAL, 5 );
	
	m_toggleBtnContent = new wxToggleButton( this, wxID_ANY, _("File content"), wxDefaultPosition, wxSize( -1,-1 ), 0 );
	m_toggleBtnContent->SetFont( wxFont( wxNORMAL_FONT->GetPointSize(), 70, 90, 92, false, wxEmptyString ) );
	m_toggleBtnContent->SetToolTip( _("Files are found equal if\n     - file content\nis the same") );
	
	fgSizer16->Add( m_toggleBtnContent, 0, wxALIGN_CENTER_VERTICAL|wxEXPAND, 5 );
	
	
	bSizer136->Add( fgSizer16, 1, wxALIGN_CENTER_VERTICAL|wxEXPAND|wxBOTTOM|wxRIGHT|wxLEFT, 5 );
	
	m_staticline33 = new wxStaticLine( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	bSizer136->Add( m_staticline33, 0, wxEXPAND, 5 );
	
	m_staticText92 = new wxStaticText( this, wxID_ANY, _("Symbolic Link handling"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText92->Wrap( -1 );
	bSizer136->Add( m_staticText92, 0, wxTOP|wxRIGHT|wxLEFT, 5 );
	
	wxBoxSizer* bSizer177;
	bSizer177 = new wxBoxSizer( wxHORIZONTAL );
	
	wxArrayString m_choiceHandleSymlinksChoices;
	m_choiceHandleSymlinks = new wxChoice( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, m_choiceHandleSymlinksChoices, 0 );
	m_choiceHandleSymlinks->SetSelection( -1 );
	bSizer177->Add( m_choiceHandleSymlinks, 1, wxALIGN_CENTER_HORIZONTAL|wxRIGHT|wxALIGN_CENTER_VERTICAL, 5 );
	
	m_bpButtonHelp = new wxBitmapButton( this, wxID_HELP, wxNullBitmap, wxDefaultPosition, wxSize( 42,42 ), wxBU_AUTODRAW );
	m_bpButtonHelp->SetToolTip( _("Help") );
	
	bSizer177->Add( m_bpButtonHelp, 0, wxALIGN_CENTER_VERTICAL, 5 );
	
	
	bSizer136->Add( bSizer177, 0, wxEXPAND|wxBOTTOM|wxRIGHT|wxLEFT, 5 );
	
	m_staticline14 = new wxStaticLine( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	bSizer136->Add( m_staticline14, 0, wxEXPAND, 5 );
	
	m_panel36 = new wxPanel( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	m_panel36->SetBackgroundColour( wxSystemSettings::GetColour( wxSYS_COLOUR_BTNFACE ) );
	
	wxBoxSizer* bSizer22;
	bSizer22 = new wxBoxSizer( wxHORIZONTAL );
	
	
	bSizer22->Add( 0, 0, 1, wxEXPAND, 5 );
	
	m_button10 = new wxButton( m_panel36, wxID_OK, _("OK"), wxDefaultPosition, wxSize( -1,30 ), 0 );
	m_button10->SetDefault(); 
	m_button10->SetFont( wxFont( wxNORMAL_FONT->GetPointSize(), 70, 90, 92, false, wxEmptyString ) );
	
	bSizer22->Add( m_button10, 0, wxALIGN_CENTER_VERTICAL|wxTOP|wxBOTTOM|wxLEFT, 5 );
	
	m_button6 = new wxButton( m_panel36, wxID_CANCEL, _("Cancel"), wxDefaultPosition, wxSize( -1,30 ), 0 );
	m_button6->SetFont( wxFont( wxNORMAL_FONT->GetPointSize(), 70, 90, 90, false, wxEmptyString ) );
	
	bSizer22->Add( m_button6, 0, wxALL|wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 5 );
	
	
	bSizer22->Add( 0, 0, 1, wxEXPAND, 5 );
	
	
	m_panel36->SetSizer( bSizer22 );
	m_panel36->Layout();
	bSizer22->Fit( m_panel36 );
	bSizer136->Add( m_panel36, 0, wxEXPAND|wxALIGN_CENTER_HORIZONTAL, 5 );
	
	
	this->SetSizer( bSizer136 );
	this->Layout();
	bSizer136->Fit( this );
	
	this->Centre( wxBOTH );
	
	// Connect Events
	this->Connect( wxEVT_CLOSE_WINDOW, wxCloseEventHandler( CmpCfgDlgGenerated::OnClose ) );
	m_toggleBtnTimeSize->Connect( wxEVT_LEFT_DCLICK, wxMouseEventHandler( CmpCfgDlgGenerated::OnTimeSizeDouble ), NULL, this );
	m_toggleBtnTimeSize->Connect( wxEVT_COMMAND_TOGGLEBUTTON_CLICKED, wxCommandEventHandler( CmpCfgDlgGenerated::OnTimeSize ), NULL, this );
	m_toggleBtnContent->Connect( wxEVT_LEFT_DCLICK, wxMouseEventHandler( CmpCfgDlgGenerated::OnContentDouble ), NULL, this );
	m_toggleBtnContent->Connect( wxEVT_COMMAND_TOGGLEBUTTON_CLICKED, wxCommandEventHandler( CmpCfgDlgGenerated::OnContent ), NULL, this );
	m_choiceHandleSymlinks->Connect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( CmpCfgDlgGenerated::OnChangeErrorHandling ), NULL, this );
	m_bpButtonHelp->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( CmpCfgDlgGenerated::OnShowHelp ), NULL, this );
	m_button10->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( CmpCfgDlgGenerated::OnOkay ), NULL, this );
	m_button6->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( CmpCfgDlgGenerated::OnCancel ), NULL, this );
}

CmpCfgDlgGenerated::~CmpCfgDlgGenerated()
{
	// Disconnect Events
	this->Disconnect( wxEVT_CLOSE_WINDOW, wxCloseEventHandler( CmpCfgDlgGenerated::OnClose ) );
	m_toggleBtnTimeSize->Disconnect( wxEVT_LEFT_DCLICK, wxMouseEventHandler( CmpCfgDlgGenerated::OnTimeSizeDouble ), NULL, this );
	m_toggleBtnTimeSize->Disconnect( wxEVT_COMMAND_TOGGLEBUTTON_CLICKED, wxCommandEventHandler( CmpCfgDlgGenerated::OnTimeSize ), NULL, this );
	m_toggleBtnContent->Disconnect( wxEVT_LEFT_DCLICK, wxMouseEventHandler( CmpCfgDlgGenerated::OnContentDouble ), NULL, this );
	m_toggleBtnContent->Disconnect( wxEVT_COMMAND_TOGGLEBUTTON_CLICKED, wxCommandEventHandler( CmpCfgDlgGenerated::OnContent ), NULL, this );
	m_choiceHandleSymlinks->Disconnect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( CmpCfgDlgGenerated::OnChangeErrorHandling ), NULL, this );
	m_bpButtonHelp->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( CmpCfgDlgGenerated::OnShowHelp ), NULL, this );
	m_button10->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( CmpCfgDlgGenerated::OnOkay ), NULL, this );
	m_button6->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( CmpCfgDlgGenerated::OnCancel ), NULL, this );
	
}

SyncCfgDlgGenerated::SyncCfgDlgGenerated( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : wxDialog( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxDefaultSize, wxDefaultSize );
	this->SetBackgroundColour( wxSystemSettings::GetColour( wxSYS_COLOUR_WINDOW ) );
	
	wxBoxSizer* bSizer7;
	bSizer7 = new wxBoxSizer( wxVERTICAL );
	
	wxBoxSizer* bSizer181;
	bSizer181 = new wxBoxSizer( wxHORIZONTAL );
	
	wxBoxSizer* bSizer29;
	bSizer29 = new wxBoxSizer( wxVERTICAL );
	
	m_staticText86 = new wxStaticText( this, wxID_ANY, _("Select variant"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText86->Wrap( -1 );
	bSizer29->Add( m_staticText86, 0, wxALL, 5 );
	
	wxFlexGridSizer* fgSizer1;
	fgSizer1 = new wxFlexGridSizer( 4, 2, 6, 8 );
	fgSizer1->SetFlexibleDirection( wxHORIZONTAL );
	fgSizer1->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );
	
	m_toggleBtnAutomatic = new wxToggleButton( this, wxID_ANY, _("<- Two way ->"), wxDefaultPosition, wxDefaultSize, 0 );
	m_toggleBtnAutomatic->SetValue( true ); 
	m_toggleBtnAutomatic->SetFont( wxFont( wxNORMAL_FONT->GetPointSize(), 70, 90, 92, false, wxEmptyString ) );
	
	fgSizer1->Add( m_toggleBtnAutomatic, 0, wxALIGN_CENTER_VERTICAL|wxEXPAND, 5 );
	
	m_staticTextAutomatic = new wxStaticText( this, wxID_ANY, _("Identify and propagate changes on both sides using a database. Deletions, renaming and conflicts are detected automatically."), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextAutomatic->Wrap( 450 );
	fgSizer1->Add( m_staticTextAutomatic, 0, wxALIGN_CENTER_VERTICAL, 5 );
	
	m_toggleBtnMirror = new wxToggleButton( this, wxID_ANY, _("Mirror ->>"), wxDefaultPosition, wxDefaultSize, 0 );
	m_toggleBtnMirror->SetFont( wxFont( wxNORMAL_FONT->GetPointSize(), 70, 90, 92, false, wxEmptyString ) );
	
	fgSizer1->Add( m_toggleBtnMirror, 0, wxALIGN_CENTER_VERTICAL|wxEXPAND, 5 );
	
	m_staticTextMirror = new wxStaticText( this, wxID_ANY, _("Mirror backup of left folder. Right folder is modified to exactly match left folder after synchronization."), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextMirror->Wrap( 450 );
	fgSizer1->Add( m_staticTextMirror, 0, wxALIGN_CENTER_VERTICAL, 5 );
	
	m_toggleBtnUpdate = new wxToggleButton( this, wxID_ANY, _("Update ->"), wxDefaultPosition, wxDefaultSize, 0 );
	m_toggleBtnUpdate->SetFont( wxFont( wxNORMAL_FONT->GetPointSize(), 70, 90, 92, false, wxEmptyString ) );
	
	fgSizer1->Add( m_toggleBtnUpdate, 0, wxALIGN_CENTER_VERTICAL|wxEXPAND, 5 );
	
	m_staticTextUpdate = new wxStaticText( this, wxID_ANY, _("Copy new or updated files to right folder."), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextUpdate->Wrap( 450 );
	fgSizer1->Add( m_staticTextUpdate, 0, wxALIGN_CENTER_VERTICAL, 5 );
	
	m_toggleBtnCustom = new wxToggleButton( this, wxID_ANY, _("Custom"), wxDefaultPosition, wxDefaultSize, 0 );
	m_toggleBtnCustom->SetFont( wxFont( wxNORMAL_FONT->GetPointSize(), 70, 90, 92, false, wxEmptyString ) );
	
	fgSizer1->Add( m_toggleBtnCustom, 0, wxALIGN_CENTER_VERTICAL|wxEXPAND, 5 );
	
	m_staticTextCustom = new wxStaticText( this, wxID_ANY, _("Configure your own synchronization rules."), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextCustom->Wrap( 450 );
	fgSizer1->Add( m_staticTextCustom, 0, wxALIGN_CENTER_VERTICAL, 5 );
	
	
	bSizer29->Add( fgSizer1, 0, wxBOTTOM|wxRIGHT|wxLEFT, 5 );
	
	bSizerExtraConfig = new wxBoxSizer( wxVERTICAL );
	
	m_staticline321 = new wxStaticLine( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	bSizerExtraConfig->Add( m_staticline321, 0, wxEXPAND, 5 );
	
	bSizer179 = new wxBoxSizer( wxHORIZONTAL );
	
	wxBoxSizer* bSizer174;
	bSizer174 = new wxBoxSizer( wxVERTICAL );
	
	m_staticText88 = new wxStaticText( this, wxID_ANY, _("Error handling"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText88->Wrap( -1 );
	bSizer174->Add( m_staticText88, 0, wxBOTTOM, 5 );
	
	wxBoxSizer* bSizer175;
	bSizer175 = new wxBoxSizer( wxHORIZONTAL );
	
	m_toggleBtnErrorIgnore = new wxToggleButton( this, wxID_ANY, _("Ignore"), wxDefaultPosition, wxDefaultSize, 0 );
	m_toggleBtnErrorIgnore->SetToolTip( _("Hide all error and warning messages") );
	
	bSizer175->Add( m_toggleBtnErrorIgnore, 0, wxRIGHT|wxALIGN_CENTER_VERTICAL, 5 );
	
	m_toggleBtnErrorPopup = new wxToggleButton( this, wxID_ANY, _("Pop-up"), wxDefaultPosition, wxDefaultSize, 0 );
	m_toggleBtnErrorPopup->SetToolTip( _("Show pop-up on errors or warnings") );
	
	bSizer175->Add( m_toggleBtnErrorPopup, 0, wxALIGN_CENTER_VERTICAL, 5 );
	
	
	bSizer174->Add( bSizer175, 0, 0, 5 );
	
	
	bSizer179->Add( bSizer174, 0, wxALL, 5 );
	
	m_staticline36 = new wxStaticLine( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_VERTICAL );
	bSizer179->Add( m_staticline36, 0, wxEXPAND, 5 );
	
	bSizerOnCompletion = new wxBoxSizer( wxVERTICAL );
	
	m_staticText89 = new wxStaticText( this, wxID_ANY, _("On completion"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText89->Wrap( -1 );
	bSizerOnCompletion->Add( m_staticText89, 0, wxBOTTOM, 5 );
	
	m_comboBoxExecFinished = new ExecFinishedBox( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0, NULL, 0 ); 
	bSizerOnCompletion->Add( m_comboBoxExecFinished, 0, wxALIGN_CENTER_VERTICAL|wxEXPAND, 5 );
	
	
	bSizer179->Add( bSizerOnCompletion, 1, wxALL, 5 );
	
	
	bSizerExtraConfig->Add( bSizer179, 0, wxEXPAND, 5 );
	
	
	bSizer29->Add( bSizerExtraConfig, 0, wxEXPAND, 5 );
	
	m_staticline32 = new wxStaticLine( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	bSizer29->Add( m_staticline32, 0, wxEXPAND, 5 );
	
	wxBoxSizer* bSizer158;
	bSizer158 = new wxBoxSizer( wxHORIZONTAL );
	
	m_staticText87 = new wxStaticText( this, wxID_ANY, _("Deletion handling"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText87->Wrap( -1 );
	bSizer158->Add( m_staticText87, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );
	
	
	bSizer158->Add( 0, 0, 1, wxEXPAND, 5 );
	
	bSizerVersioningNamingConvention = new wxBoxSizer( wxHORIZONTAL );
	
	m_staticTextNamingCvtPart1 = new wxStaticText( this, wxID_ANY, _("dummy"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextNamingCvtPart1->Wrap( -1 );
	m_staticTextNamingCvtPart1->SetForegroundColour( wxSystemSettings::GetColour( wxSYS_COLOUR_GRAYTEXT ) );
	
	bSizerVersioningNamingConvention->Add( m_staticTextNamingCvtPart1, 0, wxALIGN_CENTER_VERTICAL, 5 );
	
	m_staticTextNamingCvtPart2Bold = new wxStaticText( this, wxID_ANY, _("dummy"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextNamingCvtPart2Bold->Wrap( -1 );
	m_staticTextNamingCvtPart2Bold->SetFont( wxFont( wxNORMAL_FONT->GetPointSize(), 70, 90, 92, false, wxEmptyString ) );
	m_staticTextNamingCvtPart2Bold->SetForegroundColour( wxSystemSettings::GetColour( wxSYS_COLOUR_GRAYTEXT ) );
	
	bSizerVersioningNamingConvention->Add( m_staticTextNamingCvtPart2Bold, 0, wxALIGN_CENTER_VERTICAL, 5 );
	
	m_staticTextNamingCvtPart3 = new wxStaticText( this, wxID_ANY, _("dummy"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextNamingCvtPart3->Wrap( -1 );
	m_staticTextNamingCvtPart3->SetForegroundColour( wxSystemSettings::GetColour( wxSYS_COLOUR_GRAYTEXT ) );
	
	bSizerVersioningNamingConvention->Add( m_staticTextNamingCvtPart3, 0, wxALIGN_CENTER_VERTICAL, 5 );
	
	
	bSizer158->Add( bSizerVersioningNamingConvention, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );
	
	
	bSizer29->Add( bSizer158, 0, wxEXPAND|wxTOP, 5 );
	
	wxBoxSizer* bSizer180;
	bSizer180 = new wxBoxSizer( wxHORIZONTAL );
	
	m_toggleBtnPermanent = new wxToggleButton( this, wxID_ANY, _("Permanent"), wxDefaultPosition, wxDefaultSize, 0 );
	m_toggleBtnPermanent->SetToolTip( _("Delete or overwrite files permanently") );
	
	bSizer180->Add( m_toggleBtnPermanent, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );
	
	m_toggleBtnRecycler = new wxToggleButton( this, wxID_ANY, _("Recycle Bin"), wxDefaultPosition, wxDefaultSize, 0 );
	m_toggleBtnRecycler->SetToolTip( _("Use Recycle Bin for deleted and overwritten files") );
	
	bSizer180->Add( m_toggleBtnRecycler, 0, wxRIGHT|wxALIGN_CENTER_VERTICAL, 5 );
	
	m_toggleBtnVersioning = new wxToggleButton( this, wxID_ANY, _("Versioning"), wxDefaultPosition, wxDefaultSize, 0 );
	m_toggleBtnVersioning->SetToolTip( _("Move time-stamped files into specified folder") );
	
	bSizer180->Add( m_toggleBtnVersioning, 0, wxALIGN_CENTER_VERTICAL, 5 );
	
	
	bSizer180->Add( 0, 0, 1, wxEXPAND, 5 );
	
	bSizerVersioningStyle = new wxBoxSizer( wxHORIZONTAL );
	
	m_staticText93 = new wxStaticText( this, wxID_ANY, _("Naming convention:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText93->Wrap( -1 );
	bSizerVersioningStyle->Add( m_staticText93, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );
	
	wxArrayString m_choiceVersioningStyleChoices;
	m_choiceVersioningStyle = new wxChoice( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, m_choiceVersioningStyleChoices, 0 );
	m_choiceVersioningStyle->SetSelection( 0 );
	bSizerVersioningStyle->Add( m_choiceVersioningStyle, 0, wxALIGN_CENTER_VERTICAL, 5 );
	
	
	bSizer180->Add( bSizerVersioningStyle, 0, wxALIGN_CENTER_VERTICAL, 5 );
	
	
	bSizer29->Add( bSizer180, 0, wxEXPAND|wxALL, 5 );
	
	m_panelVersioning = new wxPanel( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	wxBoxSizer* bSizer156;
	bSizer156 = new wxBoxSizer( wxVERTICAL );
	
	wxBoxSizer* bSizer1151;
	bSizer1151 = new wxBoxSizer( wxHORIZONTAL );
	
	m_versioningFolder = new FolderHistoryBox( m_panelVersioning, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0, NULL, 0 ); 
	bSizer1151->Add( m_versioningFolder, 1, wxALIGN_CENTER_VERTICAL, 5 );
	
	m_buttonSelectDirVersioning = new wxButton( m_panelVersioning, wxID_ANY, _("Browse"), wxDefaultPosition, wxDefaultSize, 0 );
	m_buttonSelectDirVersioning->SetToolTip( _("Select a folder") );
	
	bSizer1151->Add( m_buttonSelectDirVersioning, 0, wxALIGN_CENTER_VERTICAL, 5 );
	
	
	bSizer156->Add( bSizer1151, 0, wxEXPAND, 5 );
	
	
	m_panelVersioning->SetSizer( bSizer156 );
	m_panelVersioning->Layout();
	bSizer156->Fit( m_panelVersioning );
	bSizer29->Add( m_panelVersioning, 0, wxALIGN_CENTER_VERTICAL|wxEXPAND|wxBOTTOM|wxRIGHT|wxLEFT, 5 );
	
	
	bSizer181->Add( bSizer29, 0, wxEXPAND, 5 );
	
	m_staticline31 = new wxStaticLine( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_VERTICAL );
	bSizer181->Add( m_staticline31, 0, wxEXPAND, 5 );
	
	bSizerConfig = new wxBoxSizer( wxVERTICAL );
	
	m_staticText90 = new wxStaticText( this, wxID_ANY, _("Configuration"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText90->Wrap( -1 );
	bSizerConfig->Add( m_staticText90, 0, wxBOTTOM, 5 );
	
	
	bSizerConfig->Add( 0, 5, 0, 0, 5 );
	
	m_bitmapDatabase = new wxStaticBitmap( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxSize( -1,-1 ), 0 );
	bSizerConfig->Add( m_bitmapDatabase, 0, wxALIGN_CENTER_HORIZONTAL, 10 );
	
	sbSizerSyncDirections = new wxBoxSizer( wxVERTICAL );
	
	wxBoxSizer* bSizer1801;
	bSizer1801 = new wxBoxSizer( wxHORIZONTAL );
	
	m_staticTextHeaderCategory = new wxStaticText( this, wxID_ANY, _("Category"), wxDefaultPosition, wxDefaultSize, wxALIGN_RIGHT );
	m_staticTextHeaderCategory->Wrap( -1 );
	m_staticTextHeaderCategory->SetFont( wxFont( wxNORMAL_FONT->GetPointSize(), 70, 90, 92, false, wxEmptyString ) );
	
	bSizer1801->Add( m_staticTextHeaderCategory, 1, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 5 );
	
	
	bSizer1801->Add( 5, 0, 0, 0, 5 );
	
	m_staticTextHeaderAction = new wxStaticText( this, wxID_ANY, _("Action"), wxDefaultPosition, wxDefaultSize, wxALIGN_LEFT );
	m_staticTextHeaderAction->Wrap( -1 );
	m_staticTextHeaderAction->SetFont( wxFont( wxNORMAL_FONT->GetPointSize(), 70, 90, 92, false, wxEmptyString ) );
	
	bSizer1801->Add( m_staticTextHeaderAction, 1, wxALIGN_CENTER_VERTICAL|wxALIGN_CENTER_HORIZONTAL, 5 );
	
	
	sbSizerSyncDirections->Add( bSizer1801, 0, wxEXPAND, 5 );
	
	bSizerLeftOnly = new wxBoxSizer( wxHORIZONTAL );
	
	m_bitmapLeftOnly = new wxStaticBitmap( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxSize( 45,45 ), 0 );
	m_bitmapLeftOnly->SetToolTip( _("Item exists on left side only") );
	
	bSizerLeftOnly->Add( m_bitmapLeftOnly, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 5 );
	
	
	bSizerLeftOnly->Add( 5, 0, 0, wxALIGN_CENTER_VERTICAL, 5 );
	
	m_bpButtonLeftOnly = new wxBitmapButton( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxSize( 46,46 ), wxBU_AUTODRAW );
	bSizerLeftOnly->Add( m_bpButtonLeftOnly, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 5 );
	
	
	sbSizerSyncDirections->Add( bSizerLeftOnly, 0, wxALIGN_CENTER_HORIZONTAL, 5 );
	
	bSizerRightOnly = new wxBoxSizer( wxHORIZONTAL );
	
	m_bitmapRightOnly = new wxStaticBitmap( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxSize( 45,45 ), 0 );
	m_bitmapRightOnly->SetToolTip( _("Item exists on right side only") );
	
	bSizerRightOnly->Add( m_bitmapRightOnly, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 5 );
	
	
	bSizerRightOnly->Add( 5, 0, 0, 0, 5 );
	
	m_bpButtonRightOnly = new wxBitmapButton( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxSize( 46,46 ), wxBU_AUTODRAW );
	bSizerRightOnly->Add( m_bpButtonRightOnly, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 5 );
	
	
	sbSizerSyncDirections->Add( bSizerRightOnly, 0, wxALIGN_CENTER_HORIZONTAL, 5 );
	
	bSizerLeftNewer = new wxBoxSizer( wxHORIZONTAL );
	
	m_bitmapLeftNewer = new wxStaticBitmap( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxSize( 45,45 ), 0 );
	m_bitmapLeftNewer->SetToolTip( _("Left side is newer") );
	
	bSizerLeftNewer->Add( m_bitmapLeftNewer, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 5 );
	
	
	bSizerLeftNewer->Add( 5, 0, 0, 0, 5 );
	
	m_bpButtonLeftNewer = new wxBitmapButton( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxSize( 46,46 ), wxBU_AUTODRAW );
	bSizerLeftNewer->Add( m_bpButtonLeftNewer, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 5 );
	
	
	sbSizerSyncDirections->Add( bSizerLeftNewer, 0, wxALIGN_CENTER_HORIZONTAL, 5 );
	
	bSizerRightNewer = new wxBoxSizer( wxHORIZONTAL );
	
	m_bitmapRightNewer = new wxStaticBitmap( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxSize( 45,45 ), 0 );
	m_bitmapRightNewer->SetToolTip( _("Right side is newer") );
	
	bSizerRightNewer->Add( m_bitmapRightNewer, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 5 );
	
	
	bSizerRightNewer->Add( 5, 0, 0, 0, 5 );
	
	m_bpButtonRightNewer = new wxBitmapButton( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxSize( 46,46 ), wxBU_AUTODRAW );
	bSizerRightNewer->Add( m_bpButtonRightNewer, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 5 );
	
	
	sbSizerSyncDirections->Add( bSizerRightNewer, 0, wxALIGN_CENTER_HORIZONTAL, 5 );
	
	bSizerDifferent = new wxBoxSizer( wxHORIZONTAL );
	
	m_bitmapDifferent = new wxStaticBitmap( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxSize( 45,45 ), 0 );
	m_bitmapDifferent->SetToolTip( _("Items have different content") );
	
	bSizerDifferent->Add( m_bitmapDifferent, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 5 );
	
	
	bSizerDifferent->Add( 5, 0, 0, 0, 5 );
	
	m_bpButtonDifferent = new wxBitmapButton( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxSize( 46,46 ), wxBU_AUTODRAW );
	bSizerDifferent->Add( m_bpButtonDifferent, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 5 );
	
	
	sbSizerSyncDirections->Add( bSizerDifferent, 0, wxALIGN_CENTER_HORIZONTAL, 5 );
	
	bSizerConflict = new wxBoxSizer( wxHORIZONTAL );
	
	m_bitmapConflict = new wxStaticBitmap( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxSize( 45,45 ), 0 );
	m_bitmapConflict->SetToolTip( _("Conflict/item cannot be categorized") );
	
	bSizerConflict->Add( m_bitmapConflict, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 5 );
	
	
	bSizerConflict->Add( 5, 0, 0, 0, 5 );
	
	m_bpButtonConflict = new wxBitmapButton( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxSize( 46,46 ), wxBU_AUTODRAW );
	bSizerConflict->Add( m_bpButtonConflict, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 5 );
	
	
	sbSizerSyncDirections->Add( bSizerConflict, 0, wxALIGN_CENTER_HORIZONTAL, 5 );
	
	
	bSizerConfig->Add( sbSizerSyncDirections, 0, wxEXPAND, 5 );
	
	
	bSizerConfig->Add( 0, 0, 1, wxEXPAND, 5 );
	
	
	bSizer181->Add( bSizerConfig, 0, wxALL|wxEXPAND, 5 );
	
	
	bSizer7->Add( bSizer181, 1, 0, 5 );
	
	m_staticline15 = new wxStaticLine( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	bSizer7->Add( m_staticline15, 0, wxEXPAND, 5 );
	
	m_panel37 = new wxPanel( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	m_panel37->SetBackgroundColour( wxSystemSettings::GetColour( wxSYS_COLOUR_BTNFACE ) );
	
	wxBoxSizer* bSizer291;
	bSizer291 = new wxBoxSizer( wxHORIZONTAL );
	
	
	bSizer291->Add( 0, 0, 1, wxEXPAND, 5 );
	
	m_buttonOK = new wxButton( m_panel37, wxID_OK, _("OK"), wxDefaultPosition, wxSize( -1,30 ), 0 );
	m_buttonOK->SetDefault(); 
	m_buttonOK->SetFont( wxFont( wxNORMAL_FONT->GetPointSize(), 70, 90, 92, false, wxEmptyString ) );
	
	bSizer291->Add( m_buttonOK, 0, wxALIGN_CENTER_VERTICAL|wxTOP|wxBOTTOM|wxLEFT, 5 );
	
	m_button16 = new wxButton( m_panel37, wxID_CANCEL, _("Cancel"), wxDefaultPosition, wxSize( -1,30 ), 0 );
	m_button16->SetFont( wxFont( wxNORMAL_FONT->GetPointSize(), 70, 90, 90, false, wxEmptyString ) );
	
	bSizer291->Add( m_button16, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );
	
	
	bSizer291->Add( 0, 0, 1, wxEXPAND, 5 );
	
	
	m_panel37->SetSizer( bSizer291 );
	m_panel37->Layout();
	bSizer291->Fit( m_panel37 );
	bSizer7->Add( m_panel37, 0, wxALIGN_CENTER_HORIZONTAL|wxEXPAND, 5 );
	
	
	this->SetSizer( bSizer7 );
	this->Layout();
	bSizer7->Fit( this );
	
	this->Centre( wxBOTH );
	
	// Connect Events
	this->Connect( wxEVT_CLOSE_WINDOW, wxCloseEventHandler( SyncCfgDlgGenerated::OnClose ) );
	m_toggleBtnAutomatic->Connect( wxEVT_LEFT_DCLICK, wxMouseEventHandler( SyncCfgDlgGenerated::OnSyncAutomaticDouble ), NULL, this );
	m_toggleBtnAutomatic->Connect( wxEVT_COMMAND_TOGGLEBUTTON_CLICKED, wxCommandEventHandler( SyncCfgDlgGenerated::OnSyncAutomatic ), NULL, this );
	m_toggleBtnMirror->Connect( wxEVT_LEFT_DCLICK, wxMouseEventHandler( SyncCfgDlgGenerated::OnSyncMirrorDouble ), NULL, this );
	m_toggleBtnMirror->Connect( wxEVT_COMMAND_TOGGLEBUTTON_CLICKED, wxCommandEventHandler( SyncCfgDlgGenerated::OnSyncMirror ), NULL, this );
	m_toggleBtnUpdate->Connect( wxEVT_LEFT_DCLICK, wxMouseEventHandler( SyncCfgDlgGenerated::OnSyncUpdateDouble ), NULL, this );
	m_toggleBtnUpdate->Connect( wxEVT_COMMAND_TOGGLEBUTTON_CLICKED, wxCommandEventHandler( SyncCfgDlgGenerated::OnSyncUpdate ), NULL, this );
	m_toggleBtnCustom->Connect( wxEVT_LEFT_DCLICK, wxMouseEventHandler( SyncCfgDlgGenerated::OnSyncCustomDouble ), NULL, this );
	m_toggleBtnCustom->Connect( wxEVT_COMMAND_TOGGLEBUTTON_CLICKED, wxCommandEventHandler( SyncCfgDlgGenerated::OnSyncCustom ), NULL, this );
	m_toggleBtnErrorIgnore->Connect( wxEVT_COMMAND_TOGGLEBUTTON_CLICKED, wxCommandEventHandler( SyncCfgDlgGenerated::OnErrorIgnore ), NULL, this );
	m_toggleBtnErrorPopup->Connect( wxEVT_COMMAND_TOGGLEBUTTON_CLICKED, wxCommandEventHandler( SyncCfgDlgGenerated::OnErrorPopup ), NULL, this );
	m_toggleBtnPermanent->Connect( wxEVT_COMMAND_TOGGLEBUTTON_CLICKED, wxCommandEventHandler( SyncCfgDlgGenerated::OnDeletionPermanent ), NULL, this );
	m_toggleBtnRecycler->Connect( wxEVT_COMMAND_TOGGLEBUTTON_CLICKED, wxCommandEventHandler( SyncCfgDlgGenerated::OnDeletionRecycler ), NULL, this );
	m_toggleBtnVersioning->Connect( wxEVT_COMMAND_TOGGLEBUTTON_CLICKED, wxCommandEventHandler( SyncCfgDlgGenerated::OnDeletionVersioning ), NULL, this );
	m_choiceVersioningStyle->Connect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( SyncCfgDlgGenerated::OnParameterChange ), NULL, this );
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
	m_toggleBtnAutomatic->Disconnect( wxEVT_LEFT_DCLICK, wxMouseEventHandler( SyncCfgDlgGenerated::OnSyncAutomaticDouble ), NULL, this );
	m_toggleBtnAutomatic->Disconnect( wxEVT_COMMAND_TOGGLEBUTTON_CLICKED, wxCommandEventHandler( SyncCfgDlgGenerated::OnSyncAutomatic ), NULL, this );
	m_toggleBtnMirror->Disconnect( wxEVT_LEFT_DCLICK, wxMouseEventHandler( SyncCfgDlgGenerated::OnSyncMirrorDouble ), NULL, this );
	m_toggleBtnMirror->Disconnect( wxEVT_COMMAND_TOGGLEBUTTON_CLICKED, wxCommandEventHandler( SyncCfgDlgGenerated::OnSyncMirror ), NULL, this );
	m_toggleBtnUpdate->Disconnect( wxEVT_LEFT_DCLICK, wxMouseEventHandler( SyncCfgDlgGenerated::OnSyncUpdateDouble ), NULL, this );
	m_toggleBtnUpdate->Disconnect( wxEVT_COMMAND_TOGGLEBUTTON_CLICKED, wxCommandEventHandler( SyncCfgDlgGenerated::OnSyncUpdate ), NULL, this );
	m_toggleBtnCustom->Disconnect( wxEVT_LEFT_DCLICK, wxMouseEventHandler( SyncCfgDlgGenerated::OnSyncCustomDouble ), NULL, this );
	m_toggleBtnCustom->Disconnect( wxEVT_COMMAND_TOGGLEBUTTON_CLICKED, wxCommandEventHandler( SyncCfgDlgGenerated::OnSyncCustom ), NULL, this );
	m_toggleBtnErrorIgnore->Disconnect( wxEVT_COMMAND_TOGGLEBUTTON_CLICKED, wxCommandEventHandler( SyncCfgDlgGenerated::OnErrorIgnore ), NULL, this );
	m_toggleBtnErrorPopup->Disconnect( wxEVT_COMMAND_TOGGLEBUTTON_CLICKED, wxCommandEventHandler( SyncCfgDlgGenerated::OnErrorPopup ), NULL, this );
	m_toggleBtnPermanent->Disconnect( wxEVT_COMMAND_TOGGLEBUTTON_CLICKED, wxCommandEventHandler( SyncCfgDlgGenerated::OnDeletionPermanent ), NULL, this );
	m_toggleBtnRecycler->Disconnect( wxEVT_COMMAND_TOGGLEBUTTON_CLICKED, wxCommandEventHandler( SyncCfgDlgGenerated::OnDeletionRecycler ), NULL, this );
	m_toggleBtnVersioning->Disconnect( wxEVT_COMMAND_TOGGLEBUTTON_CLICKED, wxCommandEventHandler( SyncCfgDlgGenerated::OnDeletionVersioning ), NULL, this );
	m_choiceVersioningStyle->Disconnect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( SyncCfgDlgGenerated::OnParameterChange ), NULL, this );
	m_bpButtonLeftOnly->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( SyncCfgDlgGenerated::OnExLeftSideOnly ), NULL, this );
	m_bpButtonRightOnly->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( SyncCfgDlgGenerated::OnExRightSideOnly ), NULL, this );
	m_bpButtonLeftNewer->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( SyncCfgDlgGenerated::OnLeftNewer ), NULL, this );
	m_bpButtonRightNewer->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( SyncCfgDlgGenerated::OnRightNewer ), NULL, this );
	m_bpButtonDifferent->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( SyncCfgDlgGenerated::OnDifferent ), NULL, this );
	m_bpButtonConflict->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( SyncCfgDlgGenerated::OnConflict ), NULL, this );
	m_buttonOK->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( SyncCfgDlgGenerated::OnApply ), NULL, this );
	m_button16->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( SyncCfgDlgGenerated::OnCancel ), NULL, this );
	
}

LogControlGenerated::LogControlGenerated( wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style ) : wxPanel( parent, id, pos, size, style )
{
	this->SetBackgroundColour( wxSystemSettings::GetColour( wxSYS_COLOUR_BTNFACE ) );
	
	wxBoxSizer* bSizer179;
	bSizer179 = new wxBoxSizer( wxVERTICAL );
	
	m_staticline12 = new wxStaticLine( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	bSizer179->Add( m_staticline12, 0, wxEXPAND, 5 );
	
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
	
	m_gridMessages = new zen::Grid( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxHSCROLL|wxVSCROLL );
	m_gridMessages->SetScrollRate( 5, 5 );
	bSizer153->Add( m_gridMessages, 1, wxEXPAND|wxALIGN_CENTER_VERTICAL, 5 );
	
	
	bSizer179->Add( bSizer153, 1, wxEXPAND, 5 );
	
	
	this->SetSizer( bSizer179 );
	this->Layout();
	bSizer179->Fit( this );
	
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
	this->SetBackgroundColour( wxSystemSettings::GetColour( wxSYS_COLOUR_WINDOW ) );
	
	wxBoxSizer* bSizer31;
	bSizer31 = new wxBoxSizer( wxVERTICAL );
	
	m_bitmapLogo = new wxStaticBitmap( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxSize( -1,-1 ), 0 );
	bSizer31->Add( m_bitmapLogo, 0, wxALIGN_CENTER_VERTICAL, 5 );
	
	m_staticline341 = new wxStaticLine( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	bSizer31->Add( m_staticline341, 0, wxEXPAND, 5 );
	
	m_build = new wxStaticText( this, wxID_ANY, _("dummy"), wxDefaultPosition, wxDefaultSize, 0 );
	m_build->Wrap( -1 );
	bSizer31->Add( m_build, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxALL, 5 );
	
	m_staticline3411 = new wxStaticLine( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	bSizer31->Add( m_staticline3411, 0, wxEXPAND, 5 );
	
	m_panel33 = new wxPanel( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	m_panel33->SetBackgroundColour( wxSystemSettings::GetColour( wxSYS_COLOUR_WINDOW ) );
	
	bSizerCodeInfo = new wxBoxSizer( wxVERTICAL );
	
	m_staticText72 = new wxStaticText( m_panel33, wxID_ANY, _("Source code written in C++ using:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText72->Wrap( -1 );
	m_staticText72->SetFont( wxFont( wxNORMAL_FONT->GetPointSize(), 70, 90, 92, false, wxEmptyString ) );
	
	bSizerCodeInfo->Add( m_staticText72, 0, wxALL, 5 );
	
	wxBoxSizer* bSizer167;
	bSizer167 = new wxBoxSizer( wxVERTICAL );
	
	wxBoxSizer* bSizer171;
	bSizer171 = new wxBoxSizer( wxHORIZONTAL );
	
	m_hyperlink11 = new wxHyperlinkCtrl( m_panel33, wxID_ANY, _("MS Visual C++"), wxT("http://msdn.microsoft.com/library/60k1461a.aspx"), wxDefaultPosition, wxDefaultSize, wxHL_DEFAULT_STYLE );
	m_hyperlink11->SetBackgroundColour( wxSystemSettings::GetColour( wxSYS_COLOUR_WINDOW ) );
	
	bSizer171->Add( m_hyperlink11, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );
	
	m_hyperlink9 = new wxHyperlinkCtrl( m_panel33, wxID_ANY, _("MinGW"), wxT("http://www.mingw.org"), wxDefaultPosition, wxDefaultSize, wxHL_DEFAULT_STYLE );
	m_hyperlink9->SetBackgroundColour( wxSystemSettings::GetColour( wxSYS_COLOUR_WINDOW ) );
	
	bSizer171->Add( m_hyperlink9, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );
	
	m_hyperlink10 = new wxHyperlinkCtrl( m_panel33, wxID_ANY, _("Code::Blocks"), wxT("http://www.codeblocks.org"), wxDefaultPosition, wxDefaultSize, wxHL_DEFAULT_STYLE );
	m_hyperlink10->SetBackgroundColour( wxSystemSettings::GetColour( wxSYS_COLOUR_WINDOW ) );
	
	bSizer171->Add( m_hyperlink10, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );
	
	m_hyperlink7 = new wxHyperlinkCtrl( m_panel33, wxID_ANY, _("wxWidgets"), wxT("http://www.wxwidgets.org"), wxDefaultPosition, wxDefaultSize, wxHL_DEFAULT_STYLE );
	m_hyperlink7->SetBackgroundColour( wxSystemSettings::GetColour( wxSYS_COLOUR_WINDOW ) );
	
	bSizer171->Add( m_hyperlink7, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );
	
	m_hyperlink14 = new wxHyperlinkCtrl( m_panel33, wxID_ANY, _("wxFormBuilder"), wxT("http://wxformbuilder.org"), wxDefaultPosition, wxDefaultSize, wxHL_DEFAULT_STYLE );
	m_hyperlink14->SetBackgroundColour( wxSystemSettings::GetColour( wxSYS_COLOUR_WINDOW ) );
	
	bSizer171->Add( m_hyperlink14, 0, wxALIGN_CENTER_VERTICAL, 5 );
	
	
	bSizer167->Add( bSizer171, 0, wxALIGN_CENTER_HORIZONTAL|wxBOTTOM, 5 );
	
	wxBoxSizer* bSizer172;
	bSizer172 = new wxBoxSizer( wxHORIZONTAL );
	
	m_hyperlink15 = new wxHyperlinkCtrl( m_panel33, wxID_ANY, _("zenXML"), wxT("http://zenxml.sourceforge.net/"), wxDefaultPosition, wxDefaultSize, wxHL_DEFAULT_STYLE );
	m_hyperlink15->SetBackgroundColour( wxSystemSettings::GetColour( wxSYS_COLOUR_WINDOW ) );
	
	bSizer172->Add( m_hyperlink15, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );
	
	m_hyperlink13 = new wxHyperlinkCtrl( m_panel33, wxID_ANY, _("Boost"), wxT("http://www.boost.org"), wxDefaultPosition, wxDefaultSize, wxHL_DEFAULT_STYLE );
	m_hyperlink13->SetBackgroundColour( wxSystemSettings::GetColour( wxSYS_COLOUR_WINDOW ) );
	
	bSizer172->Add( m_hyperlink13, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );
	
	m_hyperlink16 = new wxHyperlinkCtrl( m_panel33, wxID_ANY, _("Artistic Style"), wxT("http://astyle.sourceforge.net"), wxDefaultPosition, wxDefaultSize, wxHL_DEFAULT_STYLE );
	m_hyperlink16->SetBackgroundColour( wxSystemSettings::GetColour( wxSYS_COLOUR_WINDOW ) );
	
	bSizer172->Add( m_hyperlink16, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );
	
	m_hyperlink12 = new wxHyperlinkCtrl( m_panel33, wxID_ANY, _("Google Test"), wxT("http://code.google.com/p/googletest"), wxDefaultPosition, wxDefaultSize, wxHL_DEFAULT_STYLE );
	m_hyperlink12->SetBackgroundColour( wxSystemSettings::GetColour( wxSYS_COLOUR_WINDOW ) );
	
	bSizer172->Add( m_hyperlink12, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );
	
	m_hyperlink18 = new wxHyperlinkCtrl( m_panel33, wxID_ANY, _("Unicode NSIS"), wxT("http://www.scratchpaper.com"), wxDefaultPosition, wxDefaultSize, wxHL_DEFAULT_STYLE );
	m_hyperlink18->SetBackgroundColour( wxSystemSettings::GetColour( wxSYS_COLOUR_WINDOW ) );
	
	bSizer172->Add( m_hyperlink18, 0, wxALIGN_CENTER_VERTICAL, 5 );
	
	
	bSizer167->Add( bSizer172, 0, wxALIGN_CENTER_HORIZONTAL, 5 );
	
	
	bSizerCodeInfo->Add( bSizer167, 0, wxALIGN_CENTER_HORIZONTAL|wxBOTTOM|wxRIGHT|wxLEFT, 5 );
	
	
	m_panel33->SetSizer( bSizerCodeInfo );
	m_panel33->Layout();
	bSizerCodeInfo->Fit( m_panel33 );
	bSizer31->Add( m_panel33, 0, wxALIGN_CENTER_HORIZONTAL|wxEXPAND, 5 );
	
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
	
	m_hyperlinkDonate = new wxHyperlinkCtrl( m_panel39, wxID_ANY, _("Donate with PayPal"), wxT("https://www.paypal.com/cgi-bin/webscr?cmd=_donations&business=zenju@gmx.de&no_shipping=1&lc=US&currency_code=EUR"), wxDefaultPosition, wxDefaultSize, wxHL_DEFAULT_STYLE );
	m_hyperlinkDonate->SetFont( wxFont( wxNORMAL_FONT->GetPointSize(), 70, 90, 92, true, wxEmptyString ) );
	m_hyperlinkDonate->SetBackgroundColour( wxColour( 221, 221, 255 ) );
	m_hyperlinkDonate->SetToolTip( _("https://www.paypal.com/cgi-bin/webscr?cmd=_donations&business=zenju@gmx.de&no_shipping=1&lc=US&currency_code=EUR") );
	
	bSizer178->Add( m_hyperlinkDonate, 0, wxALIGN_CENTER_VERTICAL|wxALIGN_RIGHT|wxBOTTOM|wxRIGHT|wxLEFT, 5 );
	
	
	bSizer184->Add( bSizer178, 1, wxALIGN_CENTER_VERTICAL, 5 );
	
	m_animCtrlWink = new wxAnimationCtrl( m_panel39, wxID_ANY, wxNullAnimation, wxDefaultPosition, wxSize( -1,-1 ), wxAC_DEFAULT_STYLE ); 
	bSizer184->Add( m_animCtrlWink, 0, 0, 5 );
	
	
	m_panel39->SetSizer( bSizer184 );
	m_panel39->Layout();
	bSizer184->Fit( m_panel39 );
	bSizer183->Add( m_panel39, 0, wxEXPAND|wxALL, 5 );
	
	
	m_panel40->SetSizer( bSizer183 );
	m_panel40->Layout();
	bSizer183->Fit( m_panel40 );
	bSizer31->Add( m_panel40, 0, wxALIGN_CENTER_HORIZONTAL|wxEXPAND, 5 );
	
	m_scrolledWindowTranslators = new wxScrolledWindow( this, wxID_ANY, wxDefaultPosition, wxSize( -1,-1 ), wxHSCROLL|wxVSCROLL );
	m_scrolledWindowTranslators->SetScrollRate( 10, 10 );
	m_scrolledWindowTranslators->SetBackgroundColour( wxSystemSettings::GetColour( wxSYS_COLOUR_WINDOW ) );
	m_scrolledWindowTranslators->SetMinSize( wxSize( -1,180 ) );
	
	bSizerTranslators = new wxBoxSizer( wxVERTICAL );
	
	m_staticText54 = new wxStaticText( m_scrolledWindowTranslators, wxID_ANY, _("Many thanks for localization:"), wxDefaultPosition, wxDefaultSize, 0 );
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
	bSizer31->Add( m_scrolledWindowTranslators, 0, wxALIGN_CENTER_HORIZONTAL|wxEXPAND, 5 );
	
	m_staticline43 = new wxStaticLine( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	bSizer31->Add( m_staticline43, 0, wxEXPAND, 5 );
	
	m_staticText94 = new wxStaticText( this, wxID_ANY, _("Feedback and suggestions are welcome"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText94->Wrap( -1 );
	bSizer31->Add( m_staticText94, 0, wxALL, 5 );
	
	wxBoxSizer* bSizer166;
	bSizer166 = new wxBoxSizer( wxHORIZONTAL );
	
	wxBoxSizer* bSizer170;
	bSizer170 = new wxBoxSizer( wxHORIZONTAL );
	
	
	bSizer170->Add( 0, 0, 1, wxEXPAND, 5 );
	
	m_hyperlink1 = new wxHyperlinkCtrl( this, wxID_ANY, _("Homepage"), wxT("http://freefilesync.sourceforge.net/"), wxDefaultPosition, wxDefaultSize, wxHL_DEFAULT_STYLE );
	m_hyperlink1->SetFont( wxFont( wxNORMAL_FONT->GetPointSize(), 70, 90, 92, true, wxEmptyString ) );
	m_hyperlink1->SetBackgroundColour( wxSystemSettings::GetColour( wxSYS_COLOUR_WINDOW ) );
	m_hyperlink1->SetToolTip( _("http://sourceforge.net/projects/freefilesync/") );
	
	bSizer170->Add( m_hyperlink1, 0, wxALIGN_CENTER_VERTICAL, 5 );
	
	m_bitmap9 = new wxStaticBitmap( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxSize( -1,-1 ), 0 );
	m_bitmap9->SetToolTip( _("FreeFileSync at Sourceforge") );
	
	bSizer170->Add( m_bitmap9, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );
	
	
	bSizer170->Add( 0, 0, 1, wxEXPAND, 5 );
	
	
	bSizer166->Add( bSizer170, 1, wxALIGN_CENTER_VERTICAL, 5 );
	
	wxBoxSizer* bSizer1711;
	bSizer1711 = new wxBoxSizer( wxHORIZONTAL );
	
	
	bSizer1711->Add( 0, 0, 1, wxEXPAND, 5 );
	
	m_hyperlink2 = new wxHyperlinkCtrl( this, wxID_ANY, _("Email"), wxT("mailto:zenju@gmx.de"), wxDefaultPosition, wxDefaultSize, wxHL_DEFAULT_STYLE );
	m_hyperlink2->SetFont( wxFont( wxNORMAL_FONT->GetPointSize(), 70, 90, 92, true, wxEmptyString ) );
	m_hyperlink2->SetBackgroundColour( wxSystemSettings::GetColour( wxSYS_COLOUR_WINDOW ) );
	m_hyperlink2->SetToolTip( _("zenju@gmx.de") );
	
	bSizer1711->Add( m_hyperlink2, 0, wxALIGN_CENTER_VERTICAL, 5 );
	
	m_bitmap10 = new wxStaticBitmap( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxSize( -1,-1 ), 0 );
	m_bitmap10->SetToolTip( _("Email") );
	
	bSizer1711->Add( m_bitmap10, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );
	
	
	bSizer1711->Add( 0, 0, 1, wxEXPAND, 5 );
	
	
	bSizer166->Add( bSizer1711, 1, wxALIGN_CENTER_VERTICAL, 5 );
	
	
	bSizer31->Add( bSizer166, 0, wxALIGN_CENTER_HORIZONTAL|wxEXPAND|wxBOTTOM|wxRIGHT|wxLEFT, 5 );
	
	m_staticline34 = new wxStaticLine( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	bSizer31->Add( m_staticline34, 0, wxEXPAND, 5 );
	
	m_staticText93 = new wxStaticText( this, wxID_ANY, _("Published under the GNU General Public License"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText93->Wrap( -1 );
	bSizer31->Add( m_staticText93, 0, wxALL, 5 );
	
	wxBoxSizer* bSizer1671;
	bSizer1671 = new wxBoxSizer( wxHORIZONTAL );
	
	
	bSizer1671->Add( 0, 0, 1, wxEXPAND|wxALIGN_CENTER_VERTICAL, 5 );
	
	m_bitmap13 = new wxStaticBitmap( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxSize( -1,-1 ), 0 );
	bSizer1671->Add( m_bitmap13, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );
	
	m_hyperlink5 = new wxHyperlinkCtrl( this, wxID_ANY, _("http://www.gnu.org/licenses/gpl.html"), wxT("http://www.gnu.org/licenses/gpl.html"), wxDefaultPosition, wxDefaultSize, wxHL_DEFAULT_STYLE );
	m_hyperlink5->SetBackgroundColour( wxSystemSettings::GetColour( wxSYS_COLOUR_WINDOW ) );
	
	bSizer1671->Add( m_hyperlink5, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 5 );
	
	
	bSizer1671->Add( 0, 0, 1, wxEXPAND|wxALIGN_CENTER_VERTICAL, 5 );
	
	
	bSizer31->Add( bSizer1671, 0, wxEXPAND|wxALIGN_CENTER_HORIZONTAL|wxBOTTOM|wxRIGHT|wxLEFT, 5 );
	
	m_staticline36 = new wxStaticLine( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	bSizer31->Add( m_staticline36, 0, wxEXPAND, 5 );
	
	m_panel41 = new wxPanel( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	m_panel41->SetBackgroundColour( wxSystemSettings::GetColour( wxSYS_COLOUR_BTNFACE ) );
	
	wxBoxSizer* bSizer168;
	bSizer168 = new wxBoxSizer( wxVERTICAL );
	
	m_buttonOkay = new wxButton( m_panel41, wxID_OK, _("OK"), wxDefaultPosition, wxSize( -1,30 ), 0 );
	m_buttonOkay->SetDefault(); 
	m_buttonOkay->SetFont( wxFont( wxNORMAL_FONT->GetPointSize(), 70, 90, 90, false, wxEmptyString ) );
	
	bSizer168->Add( m_buttonOkay, 0, wxALIGN_CENTER_HORIZONTAL|wxALL, 5 );
	
	
	m_panel41->SetSizer( bSizer168 );
	m_panel41->Layout();
	bSizer168->Fit( m_panel41 );
	bSizer31->Add( m_panel41, 0, wxEXPAND|wxALIGN_CENTER_HORIZONTAL, 5 );
	
	
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
	bSizer26->Add( m_bitmapMsgType, 0, wxRIGHT|wxLEFT, 5 );
	
	m_textCtrlMessage = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize( 420,150 ), wxTE_MULTILINE|wxTE_READONLY|wxNO_BORDER );
	m_textCtrlMessage->SetMaxLength( 0 ); 
	bSizer26->Add( m_textCtrlMessage, 1, wxALIGN_CENTER_VERTICAL|wxEXPAND, 5 );
	
	
	bSizer24->Add( bSizer26, 1, wxALIGN_CENTER_HORIZONTAL|wxEXPAND, 5 );
	
	m_staticline6 = new wxStaticLine( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	bSizer24->Add( m_staticline6, 0, wxEXPAND|wxALIGN_CENTER_HORIZONTAL, 5 );
	
	m_panel33 = new wxPanel( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	m_panel33->SetBackgroundColour( wxSystemSettings::GetColour( wxSYS_COLOUR_BTNFACE ) );
	
	wxBoxSizer* bSizer177;
	bSizer177 = new wxBoxSizer( wxVERTICAL );
	
	m_checkBoxCustom = new wxCheckBox( m_panel33, wxID_ANY, _("dummy"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizer177->Add( m_checkBoxCustom, 0, wxALIGN_CENTER_HORIZONTAL|wxTOP|wxRIGHT|wxLEFT, 5 );
	
	wxBoxSizer* bSizer25;
	bSizer25 = new wxBoxSizer( wxHORIZONTAL );
	
	m_buttonCustom1 = new wxButton( m_panel33, wxID_ANY, _("dummy"), wxDefaultPosition, wxSize( -1,30 ), 0 );
	m_buttonCustom1->SetFont( wxFont( wxNORMAL_FONT->GetPointSize(), 70, 90, 90, false, wxEmptyString ) );
	
	bSizer25->Add( m_buttonCustom1, 0, wxTOP|wxBOTTOM|wxLEFT|wxALIGN_CENTER_VERTICAL, 5 );
	
	m_buttonCustom2 = new wxButton( m_panel33, wxID_ANY, _("dummy"), wxDefaultPosition, wxSize( -1,30 ), 0 );
	m_buttonCustom2->SetFont( wxFont( wxNORMAL_FONT->GetPointSize(), 70, 90, 90, false, wxEmptyString ) );
	
	bSizer25->Add( m_buttonCustom2, 0, wxTOP|wxBOTTOM|wxLEFT|wxALIGN_CENTER_VERTICAL, 5 );
	
	m_buttonCancel = new wxButton( m_panel33, wxID_CANCEL, _("Cancel"), wxDefaultPosition, wxSize( -1,30 ), 0 );
	m_buttonCancel->SetFont( wxFont( wxNORMAL_FONT->GetPointSize(), 70, 90, 90, false, wxEmptyString ) );
	
	bSizer25->Add( m_buttonCancel, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );
	
	
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
	
	m_bitmapDeleteType = new wxStaticBitmap( m_panelHeader, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxSize( -1,-1 ), 0 );
	bSizer41->Add( m_bitmapDeleteType, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );
	
	m_staticTextHeader = new wxStaticText( m_panelHeader, wxID_ANY, _("dummy"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextHeader->Wrap( -1 );
	m_staticTextHeader->SetFont( wxFont( wxNORMAL_FONT->GetPointSize(), 70, 90, 90, false, wxEmptyString ) );
	
	bSizer41->Add( m_staticTextHeader, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 5 );
	
	
	bSizer181->Add( bSizer41, 0, wxALIGN_CENTER_HORIZONTAL|wxALL, 5 );
	
	
	m_panelHeader->SetSizer( bSizer181 );
	m_panelHeader->Layout();
	bSizer181->Fit( m_panelHeader );
	bSizer24->Add( m_panelHeader, 0, wxALIGN_CENTER_HORIZONTAL|wxEXPAND, 5 );
	
	m_staticline91 = new wxStaticLine( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	bSizer24->Add( m_staticline91, 0, wxEXPAND, 5 );
	
	m_textCtrlFileList = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize( 550,200 ), wxTE_DONTWRAP|wxTE_MULTILINE|wxTE_READONLY|wxNO_BORDER );
	m_textCtrlFileList->SetMaxLength( 0 ); 
	bSizer24->Add( m_textCtrlFileList, 1, wxEXPAND, 5 );
	
	m_staticline9 = new wxStaticLine( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	bSizer24->Add( m_staticline9, 0, wxEXPAND, 5 );
	
	m_panel36 = new wxPanel( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	m_panel36->SetBackgroundColour( wxSystemSettings::GetColour( wxSYS_COLOUR_BTNFACE ) );
	
	wxBoxSizer* bSizer180;
	bSizer180 = new wxBoxSizer( wxHORIZONTAL );
	
	wxBoxSizer* bSizer99;
	bSizer99 = new wxBoxSizer( wxVERTICAL );
	
	m_checkBoxUseRecycler = new wxCheckBox( m_panel36, wxID_ANY, _("Recycle Bin"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizer99->Add( m_checkBoxUseRecycler, 0, wxALIGN_CENTER_VERTICAL|wxEXPAND|wxALL, 5 );
	
	m_checkBoxDeleteBothSides = new wxCheckBox( m_panel36, wxID_ANY, _("Delete on both sides"), wxDefaultPosition, wxDefaultSize, 0 );
	m_checkBoxDeleteBothSides->SetToolTip( _("Delete on both sides even if the file is selected on one side only") );
	
	bSizer99->Add( m_checkBoxDeleteBothSides, 0, wxALIGN_CENTER_VERTICAL|wxEXPAND|wxBOTTOM|wxRIGHT|wxLEFT, 5 );
	
	
	bSizer180->Add( bSizer99, 1, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 5 );
	
	wxBoxSizer* bSizer25;
	bSizer25 = new wxBoxSizer( wxHORIZONTAL );
	
	m_buttonOK = new wxButton( m_panel36, wxID_OK, _("OK"), wxDefaultPosition, wxSize( -1,30 ), 0 );
	m_buttonOK->SetDefault(); 
	m_buttonOK->SetFont( wxFont( wxNORMAL_FONT->GetPointSize(), 70, 90, 92, false, wxEmptyString ) );
	
	bSizer25->Add( m_buttonOK, 0, wxTOP|wxBOTTOM|wxLEFT, 5 );
	
	m_buttonCancel = new wxButton( m_panel36, wxID_CANCEL, _("Cancel"), wxDefaultPosition, wxSize( -1,30 ), 0 );
	m_buttonCancel->SetFont( wxFont( wxNORMAL_FONT->GetPointSize(), 70, 90, 90, false, wxEmptyString ) );
	
	bSizer25->Add( m_buttonCancel, 0, wxALL, 5 );
	
	
	bSizer180->Add( bSizer25, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 5 );
	
	
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
	this->SetSizeHints( wxSize( 500,300 ), wxDefaultSize );
	this->SetBackgroundColour( wxSystemSettings::GetColour( wxSYS_COLOUR_WINDOW ) );
	
	wxBoxSizer* bSizer21;
	bSizer21 = new wxBoxSizer( wxVERTICAL );
	
	m_panelHeader = new wxPanel( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	m_panelHeader->SetBackgroundColour( wxSystemSettings::GetColour( wxSYS_COLOUR_BTNFACE ) );
	
	wxBoxSizer* bSizer72;
	bSizer72 = new wxBoxSizer( wxHORIZONTAL );
	
	m_bitmap26 = new wxStaticBitmap( m_panelHeader, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxSize( -1,-1 ), 0 );
	bSizer72->Add( m_bitmap26, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );
	
	m_staticTextHeader = new wxStaticText( m_panelHeader, wxID_ANY, _("dummy"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextHeader->Wrap( -1 );
	m_staticTextHeader->SetFont( wxFont( wxNORMAL_FONT->GetPointSize(), 70, 90, 92, false, wxEmptyString ) );
	
	bSizer72->Add( m_staticTextHeader, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );
	
	m_staticText44 = new wxStaticText( m_panelHeader, wxID_ANY, _("Only files that match all filter settings will be synchronized.\nNote: File names must be relative to base directories!"), wxDefaultPosition, wxSize( -1,-1 ), 0 );
	m_staticText44->Wrap( 480 );
	bSizer72->Add( m_staticText44, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );
	
	
	bSizer72->Add( 0, 0, 1, wxEXPAND, 5 );
	
	m_bpButtonHelp = new wxBitmapButton( m_panelHeader, wxID_HELP, wxNullBitmap, wxDefaultPosition, wxSize( 42,42 ), wxBU_AUTODRAW );
	m_bpButtonHelp->SetToolTip( _("Help") );
	
	bSizer72->Add( m_bpButtonHelp, 0, wxALIGN_CENTER_VERTICAL, 5 );
	
	
	m_panelHeader->SetSizer( bSizer72 );
	m_panelHeader->Layout();
	bSizer72->Fit( m_panelHeader );
	bSizer21->Add( m_panelHeader, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxEXPAND, 5 );
	
	m_staticline17 = new wxStaticLine( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	bSizer21->Add( m_staticline17, 0, wxEXPAND, 5 );
	
	wxBoxSizer* bSizer159;
	bSizer159 = new wxBoxSizer( wxHORIZONTAL );
	
	wxBoxSizer* bSizer166;
	bSizer166 = new wxBoxSizer( wxVERTICAL );
	
	m_staticText78 = new wxStaticText( this, wxID_ANY, _("Include"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText78->Wrap( -1 );
	bSizer166->Add( m_staticText78, 0, wxALL, 5 );
	
	wxBoxSizer* bSizer1661;
	bSizer1661 = new wxBoxSizer( wxHORIZONTAL );
	
	m_bitmapInclude = new wxStaticBitmap( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxSize( 30,30 ), 0 );
	bSizer1661->Add( m_bitmapInclude, 0, wxALIGN_CENTER_VERTICAL|wxALIGN_CENTER_HORIZONTAL|wxTOP|wxBOTTOM|wxRIGHT, 5 );
	
	m_textCtrlInclude = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize( -1,-1 ), wxTE_MULTILINE );
	m_textCtrlInclude->SetMaxLength( 0 ); 
	bSizer1661->Add( m_textCtrlInclude, 1, wxALIGN_CENTER_VERTICAL|wxALIGN_CENTER_HORIZONTAL|wxEXPAND, 5 );
	
	
	bSizer166->Add( bSizer1661, 0, wxEXPAND|wxALIGN_CENTER_HORIZONTAL|wxLEFT, 5 );
	
	m_staticline22 = new wxStaticLine( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	bSizer166->Add( m_staticline22, 0, wxEXPAND, 5 );
	
	m_staticText77 = new wxStaticText( this, wxID_ANY, _("Exclude"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText77->Wrap( -1 );
	bSizer166->Add( m_staticText77, 0, wxALL, 5 );
	
	wxBoxSizer* bSizer1651;
	bSizer1651 = new wxBoxSizer( wxHORIZONTAL );
	
	m_bitmapExclude = new wxStaticBitmap( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxSize( 30,30 ), 0 );
	bSizer1651->Add( m_bitmapExclude, 0, wxALIGN_CENTER_HORIZONTAL|wxTOP|wxBOTTOM|wxRIGHT|wxALIGN_CENTER_VERTICAL, 5 );
	
	m_textCtrlExclude = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize( -1,-1 ), wxTE_MULTILINE );
	m_textCtrlExclude->SetMaxLength( 0 ); 
	bSizer1651->Add( m_textCtrlExclude, 1, wxALIGN_CENTER_VERTICAL|wxEXPAND|wxALIGN_CENTER_HORIZONTAL, 5 );
	
	
	bSizer166->Add( bSizer1651, 1, wxEXPAND|wxALIGN_CENTER_HORIZONTAL|wxLEFT, 5 );
	
	
	bSizer159->Add( bSizer166, 1, wxALIGN_CENTER_VERTICAL|wxEXPAND, 5 );
	
	m_staticline24 = new wxStaticLine( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_VERTICAL );
	bSizer159->Add( m_staticline24, 0, wxALIGN_CENTER_VERTICAL|wxEXPAND, 5 );
	
	wxBoxSizer* bSizer160;
	bSizer160 = new wxBoxSizer( wxVERTICAL );
	
	m_staticText79 = new wxStaticText( this, wxID_ANY, _("Time span"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText79->Wrap( -1 );
	bSizer160->Add( m_staticText79, 0, wxALL, 5 );
	
	wxBoxSizer* bSizer167;
	bSizer167 = new wxBoxSizer( wxHORIZONTAL );
	
	m_bitmapFilterDate = new wxStaticBitmap( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxSize( 34,34 ), 0 );
	bSizer167->Add( m_bitmapFilterDate, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxBOTTOM|wxRIGHT, 5 );
	
	wxBoxSizer* bSizer165;
	bSizer165 = new wxBoxSizer( wxVERTICAL );
	
	m_spinCtrlTimespan = new wxSpinCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 0, 2000000000, 0 );
	bSizer165->Add( m_spinCtrlTimespan, 0, wxALIGN_CENTER_VERTICAL|wxEXPAND, 5 );
	
	wxArrayString m_choiceUnitTimespanChoices;
	m_choiceUnitTimespan = new wxChoice( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, m_choiceUnitTimespanChoices, 0 );
	m_choiceUnitTimespan->SetSelection( 0 );
	bSizer165->Add( m_choiceUnitTimespan, 0, wxALIGN_CENTER_VERTICAL|wxEXPAND, 5 );
	
	
	bSizer167->Add( bSizer165, 1, wxALIGN_CENTER_VERTICAL|wxBOTTOM, 5 );
	
	
	bSizer160->Add( bSizer167, 0, wxALIGN_CENTER_HORIZONTAL|wxRIGHT|wxLEFT|wxEXPAND, 5 );
	
	m_staticline23 = new wxStaticLine( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	bSizer160->Add( m_staticline23, 0, wxEXPAND, 5 );
	
	m_staticText80 = new wxStaticText( this, wxID_ANY, _("File size"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText80->Wrap( -1 );
	bSizer160->Add( m_staticText80, 0, wxALL, 5 );
	
	wxBoxSizer* bSizer168;
	bSizer168 = new wxBoxSizer( wxHORIZONTAL );
	
	m_bitmapFilterSize = new wxStaticBitmap( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxSize( 32,32 ), 0 );
	bSizer168->Add( m_bitmapFilterSize, 0, wxALIGN_CENTER_VERTICAL|wxALIGN_CENTER_HORIZONTAL|wxTOP|wxBOTTOM|wxRIGHT, 5 );
	
	wxBoxSizer* bSizer158;
	bSizer158 = new wxBoxSizer( wxVERTICAL );
	
	wxBoxSizer* bSizer162;
	bSizer162 = new wxBoxSizer( wxVERTICAL );
	
	m_staticText101 = new wxStaticText( this, wxID_ANY, _("Minimum"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText101->Wrap( -1 );
	bSizer162->Add( m_staticText101, 0, wxBOTTOM, 2 );
	
	m_spinCtrlMinSize = new wxSpinCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 0, 2000000000, 0 );
	bSizer162->Add( m_spinCtrlMinSize, 0, wxALIGN_CENTER_VERTICAL|wxEXPAND, 5 );
	
	wxArrayString m_choiceUnitMinSizeChoices;
	m_choiceUnitMinSize = new wxChoice( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, m_choiceUnitMinSizeChoices, 0 );
	m_choiceUnitMinSize->SetSelection( 0 );
	bSizer162->Add( m_choiceUnitMinSize, 0, wxALIGN_CENTER_VERTICAL|wxEXPAND, 5 );
	
	
	bSizer158->Add( bSizer162, 0, wxBOTTOM|wxEXPAND, 5 );
	
	wxBoxSizer* bSizer163;
	bSizer163 = new wxBoxSizer( wxVERTICAL );
	
	m_staticText102 = new wxStaticText( this, wxID_ANY, _("Maximum"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText102->Wrap( -1 );
	bSizer163->Add( m_staticText102, 0, wxBOTTOM, 2 );
	
	m_spinCtrlMaxSize = new wxSpinCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 0, 2000000000, 0 );
	bSizer163->Add( m_spinCtrlMaxSize, 0, wxALIGN_CENTER_VERTICAL|wxEXPAND, 5 );
	
	wxArrayString m_choiceUnitMaxSizeChoices;
	m_choiceUnitMaxSize = new wxChoice( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, m_choiceUnitMaxSizeChoices, 0 );
	m_choiceUnitMaxSize->SetSelection( 0 );
	bSizer163->Add( m_choiceUnitMaxSize, 0, wxALIGN_CENTER_VERTICAL|wxEXPAND, 5 );
	
	
	bSizer158->Add( bSizer163, 0, wxEXPAND, 5 );
	
	
	bSizer168->Add( bSizer158, 1, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 5 );
	
	
	bSizer160->Add( bSizer168, 1, wxEXPAND|wxALIGN_CENTER_HORIZONTAL|wxBOTTOM|wxRIGHT|wxLEFT, 5 );
	
	
	bSizer159->Add( bSizer160, 0, wxEXPAND, 5 );
	
	
	bSizer21->Add( bSizer159, 1, wxALIGN_CENTER_HORIZONTAL|wxEXPAND, 5 );
	
	m_staticline16 = new wxStaticLine( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	bSizer21->Add( m_staticline16, 0, wxEXPAND, 5 );
	
	m_panel38 = new wxPanel( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	m_panel38->SetForegroundColour( wxSystemSettings::GetColour( wxSYS_COLOUR_WINDOW ) );
	m_panel38->SetBackgroundColour( wxSystemSettings::GetColour( wxSYS_COLOUR_BTNFACE ) );
	
	wxBoxSizer* bSizer22;
	bSizer22 = new wxBoxSizer( wxHORIZONTAL );
	
	m_button9 = new wxButton( m_panel38, wxID_DEFAULT, _("&Default"), wxDefaultPosition, wxSize( -1,30 ), 0 );
	m_button9->SetFont( wxFont( wxNORMAL_FONT->GetPointSize(), 70, 90, 90, false, wxEmptyString ) );
	
	bSizer22->Add( m_button9, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );
	
	
	bSizer22->Add( 0, 0, 1, wxALIGN_CENTER_VERTICAL, 5 );
	
	m_buttonOk = new wxButton( m_panel38, wxID_OK, _("OK"), wxDefaultPosition, wxSize( -1,30 ), 0 );
	m_buttonOk->SetDefault(); 
	m_buttonOk->SetFont( wxFont( wxNORMAL_FONT->GetPointSize(), 70, 90, 92, false, wxEmptyString ) );
	
	bSizer22->Add( m_buttonOk, 0, wxALIGN_CENTER_VERTICAL|wxTOP|wxBOTTOM|wxLEFT, 5 );
	
	m_button17 = new wxButton( m_panel38, wxID_CANCEL, _("Cancel"), wxDefaultPosition, wxSize( -1,30 ), 0 );
	m_button17->SetFont( wxFont( wxNORMAL_FONT->GetPointSize(), 70, 90, 90, false, wxEmptyString ) );
	
	bSizer22->Add( m_button17, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );
	
	
	m_panel38->SetSizer( bSizer22 );
	m_panel38->Layout();
	bSizer22->Fit( m_panel38 );
	bSizer21->Add( m_panel38, 0, wxEXPAND, 5 );
	
	
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
	this->SetSizeHints( wxSize( 320,360 ), wxDefaultSize );
	this->SetBackgroundColour( wxSystemSettings::GetColour( wxSYS_COLOUR_WINDOW ) );
	
	wxBoxSizer* bSizer95;
	bSizer95 = new wxBoxSizer( wxVERTICAL );
	
	m_panelHeader = new wxPanel( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	m_panelHeader->SetBackgroundColour( wxSystemSettings::GetColour( wxSYS_COLOUR_BTNFACE ) );
	
	wxBoxSizer* bSizer72;
	bSizer72 = new wxBoxSizer( wxHORIZONTAL );
	
	m_bitmapSettings = new wxStaticBitmap( m_panelHeader, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxSize( -1,-1 ), 0 );
	bSizer72->Add( m_bitmapSettings, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );
	
	m_staticTextHeader = new wxStaticText( m_panelHeader, wxID_ANY, _("Global settings"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextHeader->Wrap( -1 );
	m_staticTextHeader->SetFont( wxFont( wxNORMAL_FONT->GetPointSize(), 70, 90, 92, false, wxEmptyString ) );
	
	bSizer72->Add( m_staticTextHeader, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 5 );
	
	
	m_panelHeader->SetSizer( bSizer72 );
	m_panelHeader->Layout();
	bSizer72->Fit( m_panelHeader );
	bSizer95->Add( m_panelHeader, 0, wxALIGN_CENTER_VERTICAL|wxEXPAND, 5 );
	
	m_staticline19 = new wxStaticLine( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	bSizer95->Add( m_staticline19, 0, wxEXPAND, 5 );
	
	wxBoxSizer* bSizer160;
	bSizer160 = new wxBoxSizer( wxVERTICAL );
	
	m_checkBoxTransCopy = new wxCheckBox( this, wxID_ANY, _("Fail-safe file copy"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizer160->Add( m_checkBoxTransCopy, 0, wxALL|wxEXPAND, 5 );
	
	m_staticText82 = new wxStaticText( this, wxID_ANY, _("Write to a temporary file (*.ffs_tmp) first then rename it. This guarantees a consistent state even in case of fatal error."), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText82->Wrap( 420 );
	m_staticText82->SetForegroundColour( wxSystemSettings::GetColour( wxSYS_COLOUR_GRAYTEXT ) );
	
	bSizer160->Add( m_staticText82, 0, wxRIGHT|wxLEFT|wxEXPAND, 20 );
	
	m_checkBoxCopyLocked = new wxCheckBox( this, wxID_ANY, _("Copy locked files"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizer160->Add( m_checkBoxCopyLocked, 0, wxALL|wxEXPAND, 5 );
	
	m_staticTextCopyLocked = new wxStaticText( this, wxID_ANY, _("Copy shared or locked files using Volume Shadow Copy Service (Requires Administrator rights)"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextCopyLocked->Wrap( 420 );
	m_staticTextCopyLocked->SetForegroundColour( wxSystemSettings::GetColour( wxSYS_COLOUR_GRAYTEXT ) );
	
	bSizer160->Add( m_staticTextCopyLocked, 0, wxEXPAND|wxRIGHT|wxLEFT, 20 );
	
	m_checkBoxCopyPermissions = new wxCheckBox( this, wxID_ANY, _("Copy file access permissions"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizer160->Add( m_checkBoxCopyPermissions, 0, wxALL|wxEXPAND, 5 );
	
	m_staticText8211 = new wxStaticText( this, wxID_ANY, _("Transfer file and folder permissions (Requires Administrator rights)"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText8211->Wrap( 420 );
	m_staticText8211->SetForegroundColour( wxSystemSettings::GetColour( wxSYS_COLOUR_GRAYTEXT ) );
	
	bSizer160->Add( m_staticText8211, 0, wxEXPAND|wxRIGHT|wxLEFT, 20 );
	
	
	bSizer95->Add( bSizer160, 0, wxEXPAND|wxALL, 5 );
	
	m_staticline191 = new wxStaticLine( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	bSizer95->Add( m_staticline191, 0, wxEXPAND|wxTOP, 5 );
	
	m_buttonResetDialogs = new zen::BitmapButton( this, wxID_ANY, _("Restore hidden dialogs"), wxDefaultPosition, wxSize( -1,-1 ), 0 );
	m_buttonResetDialogs->SetFont( wxFont( wxNORMAL_FONT->GetPointSize(), 70, 90, 90, false, wxEmptyString ) );
	
	bSizer95->Add( m_buttonResetDialogs, 0, wxALIGN_CENTER_VERTICAL|wxALIGN_CENTER_HORIZONTAL|wxALL, 5 );
	
	m_staticline192 = new wxStaticLine( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	bSizer95->Add( m_staticline192, 0, wxEXPAND, 5 );
	
	m_staticText85 = new wxStaticText( this, wxID_ANY, _("External applications"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText85->Wrap( -1 );
	bSizer95->Add( m_staticText85, 0, wxALL, 5 );
	
	wxBoxSizer* bSizer173;
	bSizer173 = new wxBoxSizer( wxHORIZONTAL );
	
	wxBoxSizer* bSizer157;
	bSizer157 = new wxBoxSizer( wxVERTICAL );
	
	m_bpButtonAddRow = new wxBitmapButton( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxSize( 25,25 ), wxBU_AUTODRAW );
	bSizer157->Add( m_bpButtonAddRow, 0, 0, 5 );
	
	m_bpButtonRemoveRow = new wxBitmapButton( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxSize( 25,25 ), wxBU_AUTODRAW );
	bSizer157->Add( m_bpButtonRemoveRow, 0, 0, 5 );
	
	
	bSizer173->Add( bSizer157, 0, wxALIGN_CENTER_HORIZONTAL|wxRIGHT|wxLEFT, 5 );
	
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
	bSizer173->Add( m_gridCustomCommand, 1, wxALIGN_CENTER_VERTICAL|wxALIGN_CENTER_HORIZONTAL|wxEXPAND, 5 );
	
	
	bSizer95->Add( bSizer173, 1, wxEXPAND|wxALIGN_CENTER_HORIZONTAL, 5 );
	
	m_staticline20 = new wxStaticLine( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	bSizer95->Add( m_staticline20, 0, wxEXPAND, 5 );
	
	m_panel39 = new wxPanel( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	m_panel39->SetBackgroundColour( wxSystemSettings::GetColour( wxSYS_COLOUR_BTNFACE ) );
	
	wxBoxSizer* bSizer97;
	bSizer97 = new wxBoxSizer( wxHORIZONTAL );
	
	m_button9 = new wxButton( m_panel39, wxID_DEFAULT, _("&Default"), wxDefaultPosition, wxSize( -1,30 ), 0 );
	m_button9->SetFont( wxFont( wxNORMAL_FONT->GetPointSize(), 70, 90, 90, false, wxEmptyString ) );
	
	bSizer97->Add( m_button9, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );
	
	
	bSizer97->Add( 0, 0, 1, 0, 5 );
	
	m_buttonOkay = new wxButton( m_panel39, wxID_OK, _("OK"), wxDefaultPosition, wxSize( -1,30 ), 0 );
	m_buttonOkay->SetDefault(); 
	m_buttonOkay->SetFont( wxFont( wxNORMAL_FONT->GetPointSize(), 70, 90, 92, false, wxEmptyString ) );
	
	bSizer97->Add( m_buttonOkay, 0, wxTOP|wxBOTTOM|wxLEFT|wxALIGN_CENTER_VERTICAL, 5 );
	
	m_button29 = new wxButton( m_panel39, wxID_CANCEL, _("Cancel"), wxDefaultPosition, wxSize( -1,30 ), 0 );
	m_button29->SetFont( wxFont( wxNORMAL_FONT->GetPointSize(), 70, 90, 90, false, wxEmptyString ) );
	
	bSizer97->Add( m_button29, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );
	
	
	m_panel39->SetSizer( bSizer97 );
	m_panel39->Layout();
	bSizer97->Fit( m_panel39 );
	bSizer95->Add( m_panel39, 0, wxEXPAND|wxALIGN_CENTER_HORIZONTAL, 5 );
	
	
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
	this->SetBackgroundColour( wxSystemSettings::GetColour( wxSYS_COLOUR_WINDOW ) );
	
	wxBoxSizer* bSizer134;
	bSizer134 = new wxBoxSizer( wxVERTICAL );
	
	m_panelHeader = new wxPanel( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	m_panelHeader->SetBackgroundColour( wxSystemSettings::GetColour( wxSYS_COLOUR_BTNFACE ) );
	
	wxBoxSizer* bSizer158;
	bSizer158 = new wxBoxSizer( wxHORIZONTAL );
	
	m_buttonStartSync = new zen::BitmapButton( m_panelHeader, wxID_OK, _("Start"), wxDefaultPosition, wxSize( -1,-1 ), 0 );
	m_buttonStartSync->SetDefault(); 
	m_buttonStartSync->SetFont( wxFont( wxNORMAL_FONT->GetPointSize(), 70, 90, 92, false, wxEmptyString ) );
	m_buttonStartSync->SetToolTip( _("Start synchronization") );
	
	bSizer158->Add( m_buttonStartSync, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxALL, 5 );
	
	m_staticline16 = new wxStaticLine( m_panelHeader, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_VERTICAL );
	bSizer158->Add( m_staticline16, 0, wxEXPAND, 5 );
	
	wxBoxSizer* bSizer172;
	bSizer172 = new wxBoxSizer( wxVERTICAL );
	
	m_staticText84 = new wxStaticText( m_panelHeader, wxID_ANY, _("Variant"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText84->Wrap( -1 );
	bSizer172->Add( m_staticText84, 0, wxALL, 5 );
	
	m_staticTextVariant = new wxStaticText( m_panelHeader, wxID_ANY, _("dummy"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextVariant->Wrap( -1 );
	m_staticTextVariant->SetFont( wxFont( wxNORMAL_FONT->GetPointSize(), 70, 90, 92, false, wxEmptyString ) );
	
	bSizer172->Add( m_staticTextVariant, 0, wxALIGN_CENTER_VERTICAL|wxALIGN_CENTER_HORIZONTAL|wxBOTTOM|wxRIGHT|wxLEFT, 5 );
	
	
	bSizer158->Add( bSizer172, 1, 0, 5 );
	
	
	m_panelHeader->SetSizer( bSizer158 );
	m_panelHeader->Layout();
	bSizer158->Fit( m_panelHeader );
	bSizer134->Add( m_panelHeader, 0, wxEXPAND, 5 );
	
	m_staticline14 = new wxStaticLine( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	bSizer134->Add( m_staticline14, 0, wxEXPAND, 5 );
	
	m_staticText83 = new wxStaticText( this, wxID_ANY, _("Statistics"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText83->Wrap( -1 );
	bSizer134->Add( m_staticText83, 0, wxALL, 5 );
	
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
	
	
	bSizer134->Add( fgSizer11, 0, wxALIGN_CENTER_VERTICAL|wxBOTTOM|wxRIGHT|wxLEFT|wxALIGN_CENTER_HORIZONTAL, 5 );
	
	m_staticline12 = new wxStaticLine( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	bSizer134->Add( m_staticline12, 0, wxEXPAND, 5 );
	
	m_panel42 = new wxPanel( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	m_panel42->SetBackgroundColour( wxSystemSettings::GetColour( wxSYS_COLOUR_BTNFACE ) );
	
	wxBoxSizer* bSizer142;
	bSizer142 = new wxBoxSizer( wxHORIZONTAL );
	
	m_checkBoxDontShowAgain = new wxCheckBox( m_panel42, wxID_ANY, _("Don't show this dialog again"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizer142->Add( m_checkBoxDontShowAgain, 1, wxALIGN_CENTER_HORIZONTAL|wxALL|wxALIGN_CENTER_VERTICAL, 5 );
	
	m_button16 = new wxButton( m_panel42, wxID_CANCEL, _("Cancel"), wxDefaultPosition, wxSize( -1,30 ), 0 );
	m_button16->SetFont( wxFont( wxNORMAL_FONT->GetPointSize(), 70, 90, 90, false, wxEmptyString ) );
	
	bSizer142->Add( m_button16, 0, wxALIGN_CENTER_VERTICAL|wxALIGN_CENTER_HORIZONTAL|wxALL, 5 );
	
	
	m_panel42->SetSizer( bSizer142 );
	m_panel42->Layout();
	bSizer142->Fit( m_panel42 );
	bSizer134->Add( m_panel42, 0, wxEXPAND, 5 );
	
	
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
	m_textCtrlSearchTxt->SetMaxLength( 0 ); 
	bSizer162->Add( m_textCtrlSearchTxt, 1, wxALL|wxALIGN_CENTER_VERTICAL, 5 );
	
	
	bSizer166->Add( bSizer162, 0, wxALIGN_CENTER_VERTICAL|wxEXPAND, 5 );
	
	
	bSizer166->Add( 0, 5, 0, 0, 5 );
	
	m_checkBoxMatchCase = new wxCheckBox( this, wxID_ANY, _("Match case"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizer166->Add( m_checkBoxMatchCase, 0, wxALL|wxEXPAND, 5 );
	
	
	bSizer161->Add( bSizer166, 1, wxALIGN_CENTER_VERTICAL|wxEXPAND, 5 );
	
	wxBoxSizer* bSizer97;
	bSizer97 = new wxBoxSizer( wxVERTICAL );
	
	m_buttonFindNext = new wxButton( this, wxID_OK, _("&Find next"), wxDefaultPosition, wxSize( -1,30 ), 0 );
	m_buttonFindNext->SetDefault(); 
	m_buttonFindNext->SetFont( wxFont( wxNORMAL_FONT->GetPointSize(), 70, 90, 92, false, wxEmptyString ) );
	
	bSizer97->Add( m_buttonFindNext, 0, wxEXPAND|wxTOP|wxRIGHT, 5 );
	
	m_button29 = new wxButton( this, wxID_CANCEL, _("Cancel"), wxDefaultPosition, wxSize( -1,30 ), 0 );
	m_button29->SetFont( wxFont( wxNORMAL_FONT->GetPointSize(), 70, 90, 90, false, wxEmptyString ) );
	
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
	this->SetBackgroundColour( wxSystemSettings::GetColour( wxSYS_COLOUR_WINDOW ) );
	
	wxBoxSizer* bSizer96;
	bSizer96 = new wxBoxSizer( wxVERTICAL );
	
	wxBoxSizer* bSizer98;
	bSizer98 = new wxBoxSizer( wxHORIZONTAL );
	
	m_calendarFrom = new wxCalendarCtrl( this, wxID_ANY, wxDefaultDateTime, wxDefaultPosition, wxDefaultSize, wxCAL_SHOW_HOLIDAYS );
	bSizer98->Add( m_calendarFrom, 0, wxALL, 5 );
	
	m_calendarTo = new wxCalendarCtrl( this, wxID_ANY, wxDefaultDateTime, wxDefaultPosition, wxDefaultSize, wxCAL_SHOW_HOLIDAYS );
	bSizer98->Add( m_calendarTo, 0, wxALL, 5 );
	
	
	bSizer96->Add( bSizer98, 0, wxALIGN_CENTER_VERTICAL, 5 );
	
	m_staticline21 = new wxStaticLine( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	bSizer96->Add( m_staticline21, 0, wxEXPAND, 5 );
	
	m_panel40 = new wxPanel( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	m_panel40->SetForegroundColour( wxSystemSettings::GetColour( wxSYS_COLOUR_WINDOW ) );
	m_panel40->SetBackgroundColour( wxSystemSettings::GetColour( wxSYS_COLOUR_BTNFACE ) );
	
	wxBoxSizer* bSizer97;
	bSizer97 = new wxBoxSizer( wxHORIZONTAL );
	
	
	bSizer97->Add( 0, 0, 1, wxEXPAND, 5 );
	
	m_buttonOkay = new wxButton( m_panel40, wxID_OK, _("OK"), wxDefaultPosition, wxSize( -1,30 ), 0 );
	m_buttonOkay->SetDefault(); 
	m_buttonOkay->SetFont( wxFont( wxNORMAL_FONT->GetPointSize(), 70, 90, 92, false, wxEmptyString ) );
	
	bSizer97->Add( m_buttonOkay, 0, wxTOP|wxBOTTOM|wxLEFT|wxALIGN_CENTER_VERTICAL, 5 );
	
	m_button29 = new wxButton( m_panel40, wxID_CANCEL, _("Cancel"), wxDefaultPosition, wxSize( -1,30 ), 0 );
	m_button29->SetFont( wxFont( wxNORMAL_FONT->GetPointSize(), 70, 90, 90, false, wxEmptyString ) );
	
	bSizer97->Add( m_button29, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );
	
	
	bSizer97->Add( 0, 0, 1, wxEXPAND, 5 );
	
	
	m_panel40->SetSizer( bSizer97 );
	m_panel40->Layout();
	bSizer97->Fit( m_panel40 );
	bSizer96->Add( m_panel40, 0, wxEXPAND|wxALIGN_CENTER_HORIZONTAL, 5 );
	
	
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
