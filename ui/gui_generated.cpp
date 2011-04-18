///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Mar 22 2011)
// http://www.wxformbuilder.org/
//
// PLEASE DO "NOT" EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "../library/custom_grid.h"
#include "../shared/custom_button.h"
#include "../shared/custom_combo_box.h"
#include "../shared/dir_picker_i18n.h"
#include "../shared/toggle_button.h"

#include "gui_generated.h"

///////////////////////////////////////////////////////////////////////////

MainDialogGenerated::MainDialogGenerated( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : wxFrame( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxSize( 640,400 ), wxDefaultSize );
	
	m_menubar1 = new wxMenuBar( 0 );
	m_menuFile = new wxMenu();
	m_menuItem10 = new wxMenuItem( m_menuFile, wxID_ANY, wxString( _("1. &Compare") ) + wxT('\t') + wxT("F5"), wxEmptyString, wxITEM_NORMAL );
	m_menuFile->Append( m_menuItem10 );
	
	m_menuItem11 = new wxMenuItem( m_menuFile, wxID_ANY, wxString( _("2. &Synchronize...") ) + wxT('\t') + wxT("F6"), wxEmptyString, wxITEM_NORMAL );
	m_menuFile->Append( m_menuItem11 );
	
	wxMenuItem* m_separator1;
	m_separator1 = m_menuFile->AppendSeparator();
	
	m_menuItemSwitchView = new wxMenuItem( m_menuFile, wxID_ANY, wxString( _("S&witch view") ) + wxT('\t') + wxT("F8"), wxEmptyString, wxITEM_NORMAL );
	m_menuFile->Append( m_menuItemSwitchView );
	
	wxMenuItem* m_separator2;
	m_separator2 = m_menuFile->AppendSeparator();
	
	m_menuItemNew = new wxMenuItem( m_menuFile, wxID_ANY, wxString( _("&New") ) + wxT('\t') + wxT("CTRL-N"), wxEmptyString, wxITEM_NORMAL );
	m_menuFile->Append( m_menuItemNew );
	
	m_menuItemSave = new wxMenuItem( m_menuFile, wxID_ANY, wxString( _("S&ave configuration...") ) + wxT('\t') + wxT("CTRL-S"), wxEmptyString, wxITEM_NORMAL );
	m_menuFile->Append( m_menuItemSave );
	
	m_menuItemLoad = new wxMenuItem( m_menuFile, wxID_ANY, wxString( _("&Load configuration...") ) + wxT('\t') + wxT("CTRL-L"), wxEmptyString, wxITEM_NORMAL );
	m_menuFile->Append( m_menuItemLoad );
	
	wxMenuItem* m_separator3;
	m_separator3 = m_menuFile->AppendSeparator();
	
	wxMenuItem* m_menuItem4;
	m_menuItem4 = new wxMenuItem( m_menuFile, wxID_EXIT, wxString( _("&Quit") ) + wxT('\t') + wxT("CTRL-Q"), wxEmptyString, wxITEM_NORMAL );
	m_menuFile->Append( m_menuItem4 );
	
	m_menubar1->Append( m_menuFile, _("&Program") ); 
	
	m_menuAdvanced = new wxMenu();
	m_menuLanguages = new wxMenu();
	m_menuAdvanced->Append( -1, _("&Language"), m_menuLanguages );
	
	wxMenuItem* m_separator4;
	m_separator4 = m_menuAdvanced->AppendSeparator();
	
	m_menuItemGlobSett = new wxMenuItem( m_menuAdvanced, wxID_ANY, wxString( _("&Global settings...") ) , wxEmptyString, wxITEM_NORMAL );
	m_menuAdvanced->Append( m_menuItemGlobSett );
	
	m_menuItem7 = new wxMenuItem( m_menuAdvanced, wxID_ANY, wxString( _("&Create batch job...") ) , wxEmptyString, wxITEM_NORMAL );
	m_menuAdvanced->Append( m_menuItem7 );
	
	wxMenuItem* m_menuItem5;
	m_menuItem5 = new wxMenuItem( m_menuAdvanced, wxID_ANY, wxString( _("&Export file list...") ) , wxEmptyString, wxITEM_NORMAL );
	m_menuAdvanced->Append( m_menuItem5 );
	
	m_menubar1->Append( m_menuAdvanced, _("&Advanced") ); 
	
	m_menuHelp = new wxMenu();
	wxMenuItem* m_menuItemReadme;
	m_menuItemReadme = new wxMenuItem( m_menuHelp, wxID_ANY, wxString( _("&Content") ) + wxT('\t') + wxT("F1"), wxEmptyString, wxITEM_NORMAL );
	m_menuHelp->Append( m_menuItemReadme );
	
	m_menuItemCheckVer = new wxMenuItem( m_menuHelp, wxID_ANY, wxString( _("&Check for new version") ) , wxEmptyString, wxITEM_NORMAL );
	m_menuHelp->Append( m_menuItemCheckVer );
	
	wxMenuItem* m_separator5;
	m_separator5 = m_menuHelp->AppendSeparator();
	
	m_menuItemAbout = new wxMenuItem( m_menuHelp, wxID_ABOUT, wxString( _("&About...") ) + wxT('\t') + wxT("SHIFT-F1"), wxEmptyString, wxITEM_NORMAL );
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
	m_staticTextCmpVariant->SetFont( wxFont( 8, 70, 90, 92, false, wxEmptyString ) );
	m_staticTextCmpVariant->SetForegroundColour( wxSystemSettings::GetColour( wxSYS_COLOUR_GRAYTEXT ) );
	
	fgSizer121->Add( m_staticTextCmpVariant, 0, wxALIGN_CENTER_VERTICAL|wxALIGN_CENTER_HORIZONTAL|wxTOP, 1 );
	
	
	fgSizer121->Add( 0, 0, 1, 0, 5 );
	
	wxBoxSizer* bSizer30;
	bSizer30 = new wxBoxSizer( wxHORIZONTAL );
	
	m_buttonCompare = new wxButtonWithImage( m_panelTopButtons, wxID_OK, _("Compare"), wxDefaultPosition, wxSize( 180,42 ), 0 );
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
	
	bSizerTopButtons->Add( fgSizer121, 0, wxALIGN_CENTER_VERTICAL|wxBOTTOM, 5 );
	
	
	bSizerTopButtons->Add( 0, 0, 1, 0, 5 );
	
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
	
	m_buttonStartSync = new wxButtonWithImage( m_panelTopButtons, wxID_ANY, _("Synchronize..."), wxDefaultPosition, wxSize( -1,42 ), 0 );
	m_buttonStartSync->SetFont( wxFont( 14, 74, 90, 92, false, wxEmptyString ) );
	m_buttonStartSync->SetToolTip( _("Start synchronization") );
	
	fgSizer12->Add( m_buttonStartSync, 0, wxALIGN_CENTER_VERTICAL, 5 );
	
	bSizerTopButtons->Add( fgSizer12, 0, wxALIGN_CENTER_VERTICAL|wxBOTTOM, 5 );
	
	
	bSizerTopButtons->Add( 15, 0, 0, wxALIGN_CENTER_VERTICAL, 5 );
	
	m_panelTopButtons->SetSizer( bSizerTopButtons );
	m_panelTopButtons->Layout();
	bSizerTopButtons->Fit( m_panelTopButtons );
	bSizerPanelHolder->Add( m_panelTopButtons, 0, wxALIGN_CENTER_HORIZONTAL|wxEXPAND, 5 );
	
	m_panelDirectoryPairs = new wxPanel( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	wxBoxSizer* bSizer1601;
	bSizer1601 = new wxBoxSizer( wxVERTICAL );
	
	wxBoxSizer* bSizer91;
	bSizer91 = new wxBoxSizer( wxHORIZONTAL );
	
	m_panelTopLeft = new wxPanel( m_panelDirectoryPairs, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	m_panelTopLeft->SetMinSize( wxSize( 1,1 ) );
	
	sbSizerDirLeft = new wxStaticBoxSizer( new wxStaticBox( m_panelTopLeft, wxID_ANY, _("Drag && drop") ), wxHORIZONTAL );
	
	m_directoryLeft = new CustomComboBox( m_panelTopLeft, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0, NULL, 0 ); 
	sbSizerDirLeft->Add( m_directoryLeft, 1, wxALIGN_CENTER_VERTICAL, 5 );
	
	m_dirPickerLeft = new FfsDirPickerCtrl( m_panelTopLeft, wxID_ANY, wxEmptyString, _("Select a folder"), wxDefaultPosition, wxDefaultSize, 0 );
	m_dirPickerLeft->SetToolTip( _("Select a folder") );
	
	sbSizerDirLeft->Add( m_dirPickerLeft, 0, wxALIGN_CENTER_VERTICAL, 5 );
	
	m_panelTopLeft->SetSizer( sbSizerDirLeft );
	m_panelTopLeft->Layout();
	sbSizerDirLeft->Fit( m_panelTopLeft );
	bSizer91->Add( m_panelTopLeft, 1, wxEXPAND|wxALIGN_CENTER_VERTICAL, 5 );
	
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
	
	m_bpButtonLocalFilter = new wxBitmapButton( m_panelTopMiddle, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxSize( 20,20 ), wxBU_AUTODRAW );
	bSizer160->Add( m_bpButtonLocalFilter, 0, wxALIGN_CENTER_VERTICAL, 5 );
	
	
	bSizer160->Add( 5, 0, 0, wxALIGN_CENTER_VERTICAL, 5 );
	
	m_bpButtonAltSyncCfg = new wxBitmapButton( m_panelTopMiddle, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxSize( 20,20 ), wxBU_AUTODRAW );
	bSizer160->Add( m_bpButtonAltSyncCfg, 0, wxALIGN_CENTER_VERTICAL, 5 );
	
	
	bSizer160->Add( 0, 0, 1, wxEXPAND, 5 );
	
	bSizer93->Add( bSizer160, 0, wxALIGN_CENTER_HORIZONTAL, 5 );
	
	m_panelTopMiddle->SetSizer( bSizer93 );
	m_panelTopMiddle->Layout();
	bSizer93->Fit( m_panelTopMiddle );
	bSizer91->Add( m_panelTopMiddle, 0, wxRIGHT|wxLEFT|wxALIGN_CENTER_VERTICAL|wxEXPAND, 5 );
	
	m_panelTopRight = new wxPanel( m_panelDirectoryPairs, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	m_panelTopRight->SetMinSize( wxSize( 1,1 ) );
	
	sbSizerDirRight = new wxStaticBoxSizer( new wxStaticBox( m_panelTopRight, wxID_ANY, _("Drag && drop") ), wxHORIZONTAL );
	
	m_bpButtonAddPair = new wxBitmapButton( m_panelTopRight, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxSize( 19,21 ), wxBU_AUTODRAW );
	m_bpButtonAddPair->SetToolTip( _("Add folder pair") );
	
	sbSizerDirRight->Add( m_bpButtonAddPair, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 3 );
	
	m_bpButtonRemovePair = new wxBitmapButton( m_panelTopRight, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxSize( 19,21 ), wxBU_AUTODRAW );
	m_bpButtonRemovePair->SetToolTip( _("Remove folder pair") );
	
	sbSizerDirRight->Add( m_bpButtonRemovePair, 0, wxALIGN_CENTER_VERTICAL, 5 );
	
	m_directoryRight = new CustomComboBox( m_panelTopRight, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0, NULL, 0 ); 
	sbSizerDirRight->Add( m_directoryRight, 1, wxALIGN_CENTER_VERTICAL, 5 );
	
	m_dirPickerRight = new FfsDirPickerCtrl( m_panelTopRight, wxID_ANY, wxEmptyString, _("Select a folder"), wxDefaultPosition, wxDefaultSize, 0 );
	m_dirPickerRight->SetToolTip( _("Select a folder") );
	
	sbSizerDirRight->Add( m_dirPickerRight, 0, wxALIGN_CENTER_VERTICAL, 5 );
	
	m_panelTopRight->SetSizer( sbSizerDirRight );
	m_panelTopRight->Layout();
	sbSizerDirRight->Fit( m_panelTopRight );
	bSizer91->Add( m_panelTopRight, 1, wxALIGN_CENTER_VERTICAL|wxEXPAND, 5 );
	
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
	
	m_panelGrids = new wxPanel( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	m_panelGrids->SetBackgroundColour( wxSystemSettings::GetColour( wxSYS_COLOUR_APPWORKSPACE ) );
	
	bSizerGridHolder = new wxBoxSizer( wxHORIZONTAL );
	
	m_panelLeft = new wxPanel( m_panelGrids, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	wxBoxSizer* bSizer7;
	bSizer7 = new wxBoxSizer( wxVERTICAL );
	
	m_gridLeft = new CustomGridLeft( m_panelLeft, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0 );
	
	// Grid
	m_gridLeft->CreateGrid( 15, 4 );
	m_gridLeft->EnableEditing( false );
	m_gridLeft->EnableGridLines( true );
	m_gridLeft->EnableDragGridSize( true );
	m_gridLeft->SetMargins( 0, 0 );
	
	// Columns
	m_gridLeft->EnableDragColMove( false );
	m_gridLeft->EnableDragColSize( true );
	m_gridLeft->SetColLabelSize( 20 );
	m_gridLeft->SetColLabelAlignment( wxALIGN_LEFT, wxALIGN_CENTRE );
	
	// Rows
	m_gridLeft->EnableDragRowSize( false );
	m_gridLeft->SetRowLabelSize( 38 );
	m_gridLeft->SetRowLabelAlignment( wxALIGN_CENTRE, wxALIGN_CENTRE );
	
	// Label Appearance
	
	// Cell Defaults
	m_gridLeft->SetDefaultCellAlignment( wxALIGN_LEFT, wxALIGN_CENTRE );
	m_gridLeft->SetMinSize( wxSize( 1,1 ) );
	
	bSizer7->Add( m_gridLeft, 1, wxALIGN_CENTER_VERTICAL|wxEXPAND|wxRIGHT, 5 );
	
	m_panelLeft->SetSizer( bSizer7 );
	m_panelLeft->Layout();
	bSizer7->Fit( m_panelLeft );
	bSizerGridHolder->Add( m_panelLeft, 1, wxEXPAND, 5 );
	
	m_panelMiddle = new wxPanel( m_panelGrids, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	wxBoxSizer* bSizer18;
	bSizer18 = new wxBoxSizer( wxVERTICAL );
	
	m_gridMiddle = new CustomGridMiddle( m_panelMiddle, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0 );
	
	// Grid
	m_gridMiddle->CreateGrid( 15, 1 );
	m_gridMiddle->EnableEditing( false );
	m_gridMiddle->EnableGridLines( true );
	m_gridMiddle->EnableDragGridSize( false );
	m_gridMiddle->SetMargins( 0, 0 );
	
	// Columns
	m_gridMiddle->SetColSize( 0, 60 );
	m_gridMiddle->EnableDragColMove( false );
	m_gridMiddle->EnableDragColSize( false );
	m_gridMiddle->SetColLabelSize( 20 );
	m_gridMiddle->SetColLabelAlignment( wxALIGN_CENTRE, wxALIGN_CENTRE );
	
	// Rows
	m_gridMiddle->EnableDragRowSize( false );
	m_gridMiddle->SetRowLabelSize( 0 );
	m_gridMiddle->SetRowLabelAlignment( wxALIGN_CENTRE, wxALIGN_CENTRE );
	
	// Label Appearance
	
	// Cell Defaults
	m_gridMiddle->SetDefaultCellFont( wxFont( wxNORMAL_FONT->GetPointSize(), 70, 90, 90, false, wxT("Arial") ) );
	m_gridMiddle->SetDefaultCellAlignment( wxALIGN_CENTRE, wxALIGN_CENTRE );
	bSizer18->Add( m_gridMiddle, 1, wxALIGN_CENTER_HORIZONTAL|wxEXPAND, 5 );
	
	m_panelMiddle->SetSizer( bSizer18 );
	m_panelMiddle->Layout();
	bSizer18->Fit( m_panelMiddle );
	bSizerGridHolder->Add( m_panelMiddle, 0, wxEXPAND, 5 );
	
	m_panelRight = new wxPanel( m_panelGrids, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	wxBoxSizer* bSizer10;
	bSizer10 = new wxBoxSizer( wxVERTICAL );
	
	m_gridRight = new CustomGridRight( m_panelRight, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0 );
	
	// Grid
	m_gridRight->CreateGrid( 15, 4 );
	m_gridRight->EnableEditing( false );
	m_gridRight->EnableGridLines( true );
	m_gridRight->EnableDragGridSize( true );
	m_gridRight->SetMargins( 0, 0 );
	
	// Columns
	m_gridRight->EnableDragColMove( false );
	m_gridRight->EnableDragColSize( true );
	m_gridRight->SetColLabelSize( 20 );
	m_gridRight->SetColLabelAlignment( wxALIGN_LEFT, wxALIGN_CENTRE );
	
	// Rows
	m_gridRight->EnableDragRowSize( false );
	m_gridRight->SetRowLabelSize( 38 );
	m_gridRight->SetRowLabelAlignment( wxALIGN_CENTRE, wxALIGN_CENTRE );
	
	// Label Appearance
	
	// Cell Defaults
	m_gridRight->SetDefaultCellAlignment( wxALIGN_LEFT, wxALIGN_CENTRE );
	m_gridRight->SetMinSize( wxSize( 1,1 ) );
	
	bSizer10->Add( m_gridRight, 1, wxALIGN_CENTER_VERTICAL|wxEXPAND|wxLEFT, 5 );
	
	m_panelRight->SetSizer( bSizer10 );
	m_panelRight->Layout();
	bSizer10->Fit( m_panelRight );
	bSizerGridHolder->Add( m_panelRight, 1, wxEXPAND, 5 );
	
	m_panelGrids->SetSizer( bSizerGridHolder );
	m_panelGrids->Layout();
	bSizerGridHolder->Fit( m_panelGrids );
	bSizerPanelHolder->Add( m_panelGrids, 1, wxALIGN_CENTER_HORIZONTAL|wxEXPAND, 5 );
	
	m_panelConfig = new wxPanel( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	bSizerConfig = new wxBoxSizer( wxHORIZONTAL );
	
	
	bSizerConfig->Add( 10, 0, 0, 0, 5 );
	
	wxBoxSizer* bSizer151;
	bSizer151 = new wxBoxSizer( wxHORIZONTAL );
	
	m_bpButtonSave = new wxBitmapButton( m_panelConfig, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxSize( 40,40 ), wxBU_AUTODRAW );
	m_bpButtonSave->SetToolTip( _("Save current configuration to file") );
	
	bSizer151->Add( m_bpButtonSave, 0, wxALIGN_CENTER_VERTICAL, 5 );
	
	m_bpButtonLoad = new wxBitmapButton( m_panelConfig, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxSize( 40,40 ), wxBU_AUTODRAW );
	m_bpButtonLoad->SetToolTip( _("Load configuration from file") );
	
	bSizer151->Add( m_bpButtonLoad, 0, wxALIGN_CENTER_VERTICAL, 5 );
	
	bSizerConfig->Add( bSizer151, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 5 );
	
	m_listBoxHistory = new wxListBox( m_panelConfig, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0, NULL, wxLB_NEEDED_SB|wxLB_SINGLE|wxLB_SORT ); 
	m_listBoxHistory->SetToolTip( _("Last used configurations (press DEL to remove from list)") );
	m_listBoxHistory->SetMinSize( wxSize( -1,40 ) );
	
	bSizerConfig->Add( m_listBoxHistory, 1, wxEXPAND|wxALIGN_CENTER_VERTICAL, 5 );
	
	m_panelConfig->SetSizer( bSizerConfig );
	m_panelConfig->Layout();
	bSizerConfig->Fit( m_panelConfig );
	bSizerPanelHolder->Add( m_panelConfig, 0, 0, 5 );
	
	m_panelFilter = new wxPanel( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	wxBoxSizer* bSizer140;
	bSizer140 = new wxBoxSizer( wxHORIZONTAL );
	
	
	bSizer140->Add( 10, 0, 0, 0, 5 );
	
	m_bpButtonFilter = new wxBitmapButton( m_panelFilter, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxSize( 40,40 ), wxBU_AUTODRAW|wxFULL_REPAINT_ON_RESIZE );
	bSizer140->Add( m_bpButtonFilter, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );
	
	wxBoxSizer* bSizer23;
	bSizer23 = new wxBoxSizer( wxVERTICAL );
	
	m_checkBoxHideFilt = new wxCheckBox( m_panelFilter, wxID_ANY, _("Hide excluded items"), wxDefaultPosition, wxDefaultSize, 0 );
	m_checkBoxHideFilt->SetToolTip( _("Hide filtered or temporarily excluded files") );
	
	bSizer23->Add( m_checkBoxHideFilt, 0, wxEXPAND, 5 );
	
	bSizer140->Add( bSizer23, 0, wxALIGN_CENTER_VERTICAL, 5 );
	
	m_panelFilter->SetSizer( bSizer140 );
	m_panelFilter->Layout();
	bSizer140->Fit( m_panelFilter );
	bSizerPanelHolder->Add( m_panelFilter, 0, 0, 5 );
	
	m_panelStatistics = new wxPanel( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	bSizerStatistics = new wxBoxSizer( wxHORIZONTAL );
	
	
	bSizerStatistics->Add( 0, 0, 1, wxEXPAND, 5 );
	
	wxFlexGridSizer* fgSizer5;
	fgSizer5 = new wxFlexGridSizer( 2, 2, 0, 5 );
	fgSizer5->SetFlexibleDirection( wxHORIZONTAL );
	fgSizer5->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );
	
	m_bitmapCreate = new wxStaticBitmap( m_panelStatistics, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, 0 );
	m_bitmapCreate->SetToolTip( _("Number of files and directories that will be created") );
	
	fgSizer5->Add( m_bitmapCreate, 0, wxALIGN_CENTER_VERTICAL|wxALIGN_CENTER_HORIZONTAL, 5 );
	
	m_textCtrlCreate = new wxTextCtrl( m_panelStatistics, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize( 80,-1 ), wxTE_READONLY );
	m_textCtrlCreate->SetFont( wxFont( 8, 70, 90, 90, false, wxEmptyString ) );
	m_textCtrlCreate->SetBackgroundColour( wxColour( 208, 208, 208 ) );
	m_textCtrlCreate->SetToolTip( _("Number of files and directories that will be created") );
	
	fgSizer5->Add( m_textCtrlCreate, 0, wxALIGN_CENTER_VERTICAL, 5 );
	
	m_bitmapUpdate = new wxStaticBitmap( m_panelStatistics, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, 0 );
	m_bitmapUpdate->SetToolTip( _("Number of files that will be overwritten") );
	
	fgSizer5->Add( m_bitmapUpdate, 0, wxALIGN_CENTER_VERTICAL|wxALIGN_CENTER_HORIZONTAL, 5 );
	
	m_textCtrlUpdate = new wxTextCtrl( m_panelStatistics, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize( 80,-1 ), wxTE_READONLY );
	m_textCtrlUpdate->SetFont( wxFont( 8, 70, 90, 90, false, wxEmptyString ) );
	m_textCtrlUpdate->SetBackgroundColour( wxColour( 208, 208, 208 ) );
	m_textCtrlUpdate->SetToolTip( _("Number of files that will be overwritten") );
	
	fgSizer5->Add( m_textCtrlUpdate, 0, wxALIGN_CENTER_VERTICAL, 5 );
	
	bSizerStatistics->Add( fgSizer5, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT, 5 );
	
	wxFlexGridSizer* fgSizer6;
	fgSizer6 = new wxFlexGridSizer( 2, 2, 0, 5 );
	fgSizer6->SetFlexibleDirection( wxHORIZONTAL );
	fgSizer6->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );
	
	m_bitmapDelete = new wxStaticBitmap( m_panelStatistics, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, 0 );
	m_bitmapDelete->SetToolTip( _("Number of files and directories that will be deleted") );
	
	fgSizer6->Add( m_bitmapDelete, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 5 );
	
	m_textCtrlDelete = new wxTextCtrl( m_panelStatistics, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize( 80,-1 ), wxTE_READONLY );
	m_textCtrlDelete->SetFont( wxFont( 8, 70, 90, 90, false, wxEmptyString ) );
	m_textCtrlDelete->SetBackgroundColour( wxColour( 208, 208, 208 ) );
	m_textCtrlDelete->SetToolTip( _("Number of files and directories that will be deleted") );
	
	fgSizer6->Add( m_textCtrlDelete, 0, wxALIGN_CENTER_VERTICAL, 5 );
	
	m_bitmapData = new wxStaticBitmap( m_panelStatistics, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, 0 );
	m_bitmapData->SetToolTip( _("Total amount of data that will be transferred") );
	
	fgSizer6->Add( m_bitmapData, 0, wxALIGN_CENTER_VERTICAL|wxALIGN_CENTER_HORIZONTAL, 5 );
	
	m_textCtrlData = new wxTextCtrl( m_panelStatistics, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize( 80,-1 ), wxTE_READONLY );
	m_textCtrlData->SetFont( wxFont( 8, 70, 90, 90, false, wxEmptyString ) );
	m_textCtrlData->SetBackgroundColour( wxColour( 208, 208, 208 ) );
	m_textCtrlData->SetToolTip( _("Total amount of data that will be transferred") );
	
	fgSizer6->Add( m_textCtrlData, 0, wxALIGN_CENTER_VERTICAL, 5 );
	
	bSizerStatistics->Add( fgSizer6, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT, 5 );
	
	
	bSizerStatistics->Add( 0, 0, 1, wxEXPAND, 5 );
	
	m_panelStatistics->SetSizer( bSizerStatistics );
	m_panelStatistics->Layout();
	bSizerStatistics->Fit( m_panelStatistics );
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
	
	m_panelStatusBar = new wxPanel( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxSTATIC_BORDER|wxTAB_TRAVERSAL );
	wxBoxSizer* bSizer451;
	bSizer451 = new wxBoxSizer( wxHORIZONTAL );
	
	bSizer451->SetMinSize( wxSize( -1,22 ) ); 
	wxBoxSizer* bSizer53;
	bSizer53 = new wxBoxSizer( wxHORIZONTAL );
	
	
	bSizer53->Add( 0, 0, 1, wxALIGN_CENTER_VERTICAL, 5 );
	
	m_staticTextStatusLeft = new wxStaticText( m_panelStatusBar, wxID_ANY, _("dummy"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextStatusLeft->Wrap( -1 );
	m_staticTextStatusLeft->SetFont( wxFont( 8, 70, 90, 92, false, wxEmptyString ) );
	
	bSizer53->Add( m_staticTextStatusLeft, 0, wxALIGN_CENTER_VERTICAL|wxALIGN_CENTER_HORIZONTAL, 5 );
	
	
	bSizer53->Add( 0, 0, 1, wxALIGN_CENTER_VERTICAL, 5 );
	
	bSizer451->Add( bSizer53, 1, wxALIGN_CENTER_VERTICAL|wxEXPAND, 5 );
	
	m_staticline9 = new wxStaticLine( m_panelStatusBar, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_VERTICAL );
	bSizer451->Add( m_staticline9, 0, wxALIGN_CENTER_VERTICAL|wxTOP|wxEXPAND, 2 );
	
	
	bSizer451->Add( 26, 0, 0, wxALIGN_CENTER_VERTICAL, 5 );
	
	m_staticTextStatusMiddle = new wxStaticText( m_panelStatusBar, wxID_ANY, _("dummy"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextStatusMiddle->Wrap( -1 );
	m_staticTextStatusMiddle->SetFont( wxFont( 8, 70, 90, 92, false, wxEmptyString ) );
	
	bSizer451->Add( m_staticTextStatusMiddle, 0, wxALIGN_CENTER_VERTICAL|wxALIGN_CENTER_HORIZONTAL, 5 );
	
	
	bSizer451->Add( 26, 0, 0, wxALIGN_CENTER_VERTICAL, 5 );
	
	m_staticline10 = new wxStaticLine( m_panelStatusBar, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_VERTICAL );
	bSizer451->Add( m_staticline10, 0, wxEXPAND|wxALIGN_CENTER_VERTICAL|wxTOP, 2 );
	
	wxBoxSizer* bSizer52;
	bSizer52 = new wxBoxSizer( wxHORIZONTAL );
	
	
	bSizer52->Add( 0, 0, 1, wxALIGN_CENTER_VERTICAL, 5 );
	
	m_staticTextStatusRight = new wxStaticText( m_panelStatusBar, wxID_ANY, _("dummy"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextStatusRight->Wrap( -1 );
	m_staticTextStatusRight->SetFont( wxFont( 8, 70, 90, 92, false, wxEmptyString ) );
	
	bSizer52->Add( m_staticTextStatusRight, 0, wxALIGN_CENTER_VERTICAL, 5 );
	
	wxBoxSizer* bSizer50;
	bSizer50 = new wxBoxSizer( wxHORIZONTAL );
	
	
	bSizer50->Add( 0, 0, 1, wxALIGN_CENTER_VERTICAL, 5 );
	
	m_bitmap15 = new wxStaticBitmap( m_panelStatusBar, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxSize( 10,10 ), 0 );
	bSizer50->Add( m_bitmap15, 0, wxALIGN_BOTTOM, 2 );
	
	bSizer52->Add( bSizer50, 1, wxALIGN_BOTTOM|wxEXPAND, 5 );
	
	bSizer451->Add( bSizer52, 1, wxEXPAND|wxALIGN_CENTER_VERTICAL, 5 );
	
	m_panelStatusBar->SetSizer( bSizer451 );
	m_panelStatusBar->Layout();
	bSizer451->Fit( m_panelStatusBar );
	bSizerPanelHolder->Add( m_panelStatusBar, 0, wxALIGN_CENTER_HORIZONTAL|wxEXPAND, 5 );
	
	this->SetSizer( bSizerPanelHolder );
	this->Layout();
	bSizerPanelHolder->Fit( this );
	
	// Connect Events
	this->Connect( wxEVT_CLOSE_WINDOW, wxCloseEventHandler( MainDialogGenerated::OnClose ) );
	this->Connect( m_menuItem10->GetId(), wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( MainDialogGenerated::OnCompare ) );
	this->Connect( m_menuItem11->GetId(), wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( MainDialogGenerated::OnStartSync ) );
	this->Connect( m_menuItemSwitchView->GetId(), wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( MainDialogGenerated::OnSwitchView ) );
	this->Connect( m_menuItemNew->GetId(), wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( MainDialogGenerated::OnNewConfig ) );
	this->Connect( m_menuItemSave->GetId(), wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( MainDialogGenerated::OnSaveConfig ) );
	this->Connect( m_menuItemLoad->GetId(), wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( MainDialogGenerated::OnLoadConfig ) );
	this->Connect( m_menuItem4->GetId(), wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( MainDialogGenerated::OnMenuQuit ) );
	this->Connect( m_menuItemGlobSett->GetId(), wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( MainDialogGenerated::OnMenuGlobalSettings ) );
	this->Connect( m_menuItem7->GetId(), wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( MainDialogGenerated::OnMenuBatchJob ) );
	this->Connect( m_menuItem5->GetId(), wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( MainDialogGenerated::OnMenuExportFileList ) );
	this->Connect( m_menuItemReadme->GetId(), wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( MainDialogGenerated::OnShowHelp ) );
	this->Connect( m_menuItemCheckVer->GetId(), wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( MainDialogGenerated::OnMenuCheckVersion ) );
	this->Connect( m_menuItemAbout->GetId(), wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( MainDialogGenerated::OnMenuAbout ) );
	m_buttonCompare->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( MainDialogGenerated::OnCompare ), NULL, this );
	m_bpButtonCmpConfig->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( MainDialogGenerated::OnCmpSettings ), NULL, this );
	m_bpButtonSyncConfig->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( MainDialogGenerated::OnSyncSettings ), NULL, this );
	m_buttonStartSync->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( MainDialogGenerated::OnStartSync ), NULL, this );
	m_dirPickerLeft->Connect( wxEVT_COMMAND_DIRPICKER_CHANGED, wxFileDirPickerEventHandler( MainDialogGenerated::OnDirSelected ), NULL, this );
	m_bpButtonSwapSides->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( MainDialogGenerated::OnSwapSides ), NULL, this );
	m_bpButtonAddPair->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( MainDialogGenerated::OnAddFolderPair ), NULL, this );
	m_bpButtonRemovePair->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( MainDialogGenerated::OnRemoveTopFolderPair ), NULL, this );
	m_dirPickerRight->Connect( wxEVT_COMMAND_DIRPICKER_CHANGED, wxFileDirPickerEventHandler( MainDialogGenerated::OnDirSelected ), NULL, this );
	m_gridLeft->Connect( wxEVT_GRID_CELL_LEFT_DCLICK, wxGridEventHandler( MainDialogGenerated::OnLeftGridDoubleClick ), NULL, this );
	m_gridLeft->Connect( wxEVT_GRID_CELL_RIGHT_CLICK, wxGridEventHandler( MainDialogGenerated::OnContextRim ), NULL, this );
	m_gridLeft->Connect( wxEVT_GRID_LABEL_LEFT_CLICK, wxGridEventHandler( MainDialogGenerated::OnSortLeftGrid ), NULL, this );
	m_gridLeft->Connect( wxEVT_GRID_LABEL_RIGHT_CLICK, wxGridEventHandler( MainDialogGenerated::OnContextRimLabelLeft ), NULL, this );
	m_gridMiddle->Connect( wxEVT_GRID_CELL_RIGHT_CLICK, wxGridEventHandler( MainDialogGenerated::OnContextMiddle ), NULL, this );
	m_gridMiddle->Connect( wxEVT_GRID_LABEL_LEFT_CLICK, wxGridEventHandler( MainDialogGenerated::OnSortMiddleGrid ), NULL, this );
	m_gridMiddle->Connect( wxEVT_GRID_LABEL_RIGHT_CLICK, wxGridEventHandler( MainDialogGenerated::OnContextMiddleLabel ), NULL, this );
	m_gridRight->Connect( wxEVT_GRID_CELL_LEFT_DCLICK, wxGridEventHandler( MainDialogGenerated::OnRightGridDoubleClick ), NULL, this );
	m_gridRight->Connect( wxEVT_GRID_CELL_RIGHT_CLICK, wxGridEventHandler( MainDialogGenerated::OnContextRim ), NULL, this );
	m_gridRight->Connect( wxEVT_GRID_LABEL_LEFT_CLICK, wxGridEventHandler( MainDialogGenerated::OnSortRightGrid ), NULL, this );
	m_gridRight->Connect( wxEVT_GRID_LABEL_RIGHT_CLICK, wxGridEventHandler( MainDialogGenerated::OnContextRimLabelRight ), NULL, this );
	m_bpButtonSave->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( MainDialogGenerated::OnSaveConfig ), NULL, this );
	m_bpButtonLoad->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( MainDialogGenerated::OnLoadConfig ), NULL, this );
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
	this->Disconnect( wxID_ANY, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( MainDialogGenerated::OnSwitchView ) );
	this->Disconnect( wxID_ANY, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( MainDialogGenerated::OnNewConfig ) );
	this->Disconnect( wxID_ANY, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( MainDialogGenerated::OnSaveConfig ) );
	this->Disconnect( wxID_ANY, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( MainDialogGenerated::OnLoadConfig ) );
	this->Disconnect( wxID_EXIT, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( MainDialogGenerated::OnMenuQuit ) );
	this->Disconnect( wxID_ANY, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( MainDialogGenerated::OnMenuGlobalSettings ) );
	this->Disconnect( wxID_ANY, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( MainDialogGenerated::OnMenuBatchJob ) );
	this->Disconnect( wxID_ANY, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( MainDialogGenerated::OnMenuExportFileList ) );
	this->Disconnect( wxID_ANY, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( MainDialogGenerated::OnShowHelp ) );
	this->Disconnect( wxID_ANY, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( MainDialogGenerated::OnMenuCheckVersion ) );
	this->Disconnect( wxID_ABOUT, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( MainDialogGenerated::OnMenuAbout ) );
	m_buttonCompare->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( MainDialogGenerated::OnCompare ), NULL, this );
	m_bpButtonCmpConfig->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( MainDialogGenerated::OnCmpSettings ), NULL, this );
	m_bpButtonSyncConfig->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( MainDialogGenerated::OnSyncSettings ), NULL, this );
	m_buttonStartSync->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( MainDialogGenerated::OnStartSync ), NULL, this );
	m_dirPickerLeft->Disconnect( wxEVT_COMMAND_DIRPICKER_CHANGED, wxFileDirPickerEventHandler( MainDialogGenerated::OnDirSelected ), NULL, this );
	m_bpButtonSwapSides->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( MainDialogGenerated::OnSwapSides ), NULL, this );
	m_bpButtonAddPair->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( MainDialogGenerated::OnAddFolderPair ), NULL, this );
	m_bpButtonRemovePair->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( MainDialogGenerated::OnRemoveTopFolderPair ), NULL, this );
	m_dirPickerRight->Disconnect( wxEVT_COMMAND_DIRPICKER_CHANGED, wxFileDirPickerEventHandler( MainDialogGenerated::OnDirSelected ), NULL, this );
	m_gridLeft->Disconnect( wxEVT_GRID_CELL_LEFT_DCLICK, wxGridEventHandler( MainDialogGenerated::OnLeftGridDoubleClick ), NULL, this );
	m_gridLeft->Disconnect( wxEVT_GRID_CELL_RIGHT_CLICK, wxGridEventHandler( MainDialogGenerated::OnContextRim ), NULL, this );
	m_gridLeft->Disconnect( wxEVT_GRID_LABEL_LEFT_CLICK, wxGridEventHandler( MainDialogGenerated::OnSortLeftGrid ), NULL, this );
	m_gridLeft->Disconnect( wxEVT_GRID_LABEL_RIGHT_CLICK, wxGridEventHandler( MainDialogGenerated::OnContextRimLabelLeft ), NULL, this );
	m_gridMiddle->Disconnect( wxEVT_GRID_CELL_RIGHT_CLICK, wxGridEventHandler( MainDialogGenerated::OnContextMiddle ), NULL, this );
	m_gridMiddle->Disconnect( wxEVT_GRID_LABEL_LEFT_CLICK, wxGridEventHandler( MainDialogGenerated::OnSortMiddleGrid ), NULL, this );
	m_gridMiddle->Disconnect( wxEVT_GRID_LABEL_RIGHT_CLICK, wxGridEventHandler( MainDialogGenerated::OnContextMiddleLabel ), NULL, this );
	m_gridRight->Disconnect( wxEVT_GRID_CELL_LEFT_DCLICK, wxGridEventHandler( MainDialogGenerated::OnRightGridDoubleClick ), NULL, this );
	m_gridRight->Disconnect( wxEVT_GRID_CELL_RIGHT_CLICK, wxGridEventHandler( MainDialogGenerated::OnContextRim ), NULL, this );
	m_gridRight->Disconnect( wxEVT_GRID_LABEL_LEFT_CLICK, wxGridEventHandler( MainDialogGenerated::OnSortRightGrid ), NULL, this );
	m_gridRight->Disconnect( wxEVT_GRID_LABEL_RIGHT_CLICK, wxGridEventHandler( MainDialogGenerated::OnContextRimLabelRight ), NULL, this );
	m_bpButtonSave->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( MainDialogGenerated::OnSaveConfig ), NULL, this );
	m_bpButtonLoad->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( MainDialogGenerated::OnLoadConfig ), NULL, this );
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
	wxBoxSizer* bSizer134;
	bSizer134 = new wxBoxSizer( wxHORIZONTAL );
	
	m_directoryLeft = new wxTextCtrl( m_panelLeft, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	bSizer134->Add( m_directoryLeft, 1, wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );
	
	m_dirPickerLeft = new FfsDirPickerCtrl( m_panelLeft, wxID_ANY, wxEmptyString, _("Select a folder"), wxDefaultPosition, wxDefaultSize, 0 );
	m_dirPickerLeft->SetToolTip( _("Select a folder") );
	
	bSizer134->Add( m_dirPickerLeft, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );
	
	m_panelLeft->SetSizer( bSizer134 );
	m_panelLeft->Layout();
	bSizer134->Fit( m_panelLeft );
	bSizer74->Add( m_panelLeft, 0, wxALIGN_CENTER_VERTICAL, 5 );
	
	m_panel20 = new wxPanel( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	wxBoxSizer* bSizer95;
	bSizer95 = new wxBoxSizer( wxHORIZONTAL );
	
	
	bSizer95->Add( 0, 0, 1, wxEXPAND, 5 );
	
	m_bpButtonLocalFilter = new wxBitmapButton( m_panel20, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxSize( 20,20 ), wxBU_AUTODRAW );
	bSizer95->Add( m_bpButtonLocalFilter, 0, wxALIGN_CENTER_VERTICAL, 5 );
	
	
	bSizer95->Add( 5, 0, 0, wxALIGN_CENTER_VERTICAL, 5 );
	
	m_bpButtonAltSyncCfg = new wxBitmapButton( m_panel20, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxSize( 20,20 ), wxBU_AUTODRAW );
	bSizer95->Add( m_bpButtonAltSyncCfg, 0, wxALIGN_CENTER_VERTICAL, 5 );
	
	
	bSizer95->Add( 0, 0, 1, wxEXPAND, 5 );
	
	m_panel20->SetSizer( bSizer95 );
	m_panel20->Layout();
	bSizer95->Fit( m_panel20 );
	bSizer74->Add( m_panel20, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT|wxEXPAND, 5 );
	
	m_panelRight = new wxPanel( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	wxBoxSizer* bSizer135;
	bSizer135 = new wxBoxSizer( wxHORIZONTAL );
	
	m_bpButtonRemovePair = new wxBitmapButton( m_panelRight, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxSize( 19,21 ), wxBU_AUTODRAW );
	m_bpButtonRemovePair->SetToolTip( _("Remove folder pair") );
	
	bSizer135->Add( m_bpButtonRemovePair, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );
	
	m_directoryRight = new wxTextCtrl( m_panelRight, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	bSizer135->Add( m_directoryRight, 1, wxALIGN_CENTER_VERTICAL, 5 );
	
	m_dirPickerRight = new FfsDirPickerCtrl( m_panelRight, wxID_ANY, wxEmptyString, _("Select a folder"), wxDefaultPosition, wxDefaultSize, 0 );
	m_dirPickerRight->SetToolTip( _("Select a folder") );
	
	bSizer135->Add( m_dirPickerRight, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );
	
	m_panelRight->SetSizer( bSizer135 );
	m_panelRight->Layout();
	bSizer135->Fit( m_panelRight );
	bSizer74->Add( m_panelRight, 1, wxALIGN_CENTER_VERTICAL, 5 );
	
	this->SetSizer( bSizer74 );
	this->Layout();
	bSizer74->Fit( this );
}

FolderPairGenerated::~FolderPairGenerated()
{
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
	
	m_directoryLeft = new wxTextCtrl( m_panelLeft, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	bSizer114->Add( m_directoryLeft, 1, wxALIGN_CENTER_VERTICAL, 5 );
	
	m_dirPickerLeft = new FfsDirPickerCtrl( m_panelLeft, wxID_ANY, wxEmptyString, _("Select a folder"), wxDefaultPosition, wxDefaultSize, 0 );
	m_dirPickerLeft->SetToolTip( _("Select a folder") );
	
	bSizer114->Add( m_dirPickerLeft, 0, wxALIGN_CENTER_VERTICAL, 5 );
	
	m_bpButtonLocalFilter = new wxBitmapButton( m_panelLeft, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxSize( 20,20 ), wxBU_AUTODRAW );
	bSizer114->Add( m_bpButtonLocalFilter, 0, wxALIGN_CENTER_VERTICAL, 5 );
	
	m_panelLeft->SetSizer( bSizer114 );
	m_panelLeft->Layout();
	bSizer114->Fit( m_panelLeft );
	bSizer144->Add( m_panelLeft, 0, wxALIGN_CENTER_VERTICAL|wxEXPAND, 5 );
	
	m_panelRight = new wxPanel( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	wxBoxSizer* bSizer115;
	bSizer115 = new wxBoxSizer( wxHORIZONTAL );
	
	m_directoryRight = new wxTextCtrl( m_panelRight, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	bSizer115->Add( m_directoryRight, 1, wxALIGN_CENTER_VERTICAL, 5 );
	
	m_dirPickerRight = new FfsDirPickerCtrl( m_panelRight, wxID_ANY, wxEmptyString, _("Select a folder"), wxDefaultPosition, wxDefaultSize, 0 );
	m_dirPickerRight->SetToolTip( _("Select a folder") );
	
	bSizer115->Add( m_dirPickerRight, 0, wxALIGN_CENTER_VERTICAL, 5 );
	
	m_bpButtonAltSyncCfg = new wxBitmapButton( m_panelRight, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxSize( 20,20 ), wxBU_AUTODRAW );
	bSizer115->Add( m_bpButtonAltSyncCfg, 0, wxALIGN_CENTER_VERTICAL, 5 );
	
	m_panelRight->SetSizer( bSizer115 );
	m_panelRight->Layout();
	bSizer115->Fit( m_panelRight );
	bSizer144->Add( m_panelRight, 0, wxEXPAND|wxALIGN_CENTER_VERTICAL, 5 );
	
	bSizer140->Add( bSizer144, 1, wxALIGN_CENTER_VERTICAL, 5 );
	
	bSizer142->Add( bSizer140, 0, wxEXPAND, 5 );
	
	
	bSizer142->Add( 0, 5, 0, 0, 5 );
	
	this->SetSizer( bSizer142 );
	this->Layout();
	bSizer142->Fit( this );
}

BatchFolderPairGenerated::~BatchFolderPairGenerated()
{
}

BatchDlgGenerated::BatchDlgGenerated( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : wxDialog( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxSize( 460,420 ), wxDefaultSize );
	
	wxBoxSizer* bSizer54;
	bSizer54 = new wxBoxSizer( wxVERTICAL );
	
	bSizer69 = new wxBoxSizer( wxVERTICAL );
	
	wxBoxSizer* bSizer87;
	bSizer87 = new wxBoxSizer( wxHORIZONTAL );
	
	m_bitmap27 = new wxStaticBitmap( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxSize( 40,40 ), 0 );
	bSizer87->Add( m_bitmap27, 1, wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );
	
	m_panel8 = new wxPanel( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxSIMPLE_BORDER|wxTAB_TRAVERSAL );
	m_panel8->SetBackgroundColour( wxSystemSettings::GetColour( wxSYS_COLOUR_3DLIGHT ) );
	
	wxBoxSizer* bSizer72;
	bSizer72 = new wxBoxSizer( wxVERTICAL );
	
	m_staticText56 = new wxStaticText( m_panel8, wxID_ANY, _("Batch job"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText56->Wrap( -1 );
	m_staticText56->SetFont( wxFont( 16, 70, 90, 92, false, wxEmptyString ) );
	
	bSizer72->Add( m_staticText56, 0, wxALL|wxALIGN_CENTER_HORIZONTAL, 5 );
	
	m_panel8->SetSizer( bSizer72 );
	m_panel8->Layout();
	bSizer72->Fit( m_panel8 );
	bSizer87->Add( m_panel8, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxALL, 5 );
	
	
	bSizer87->Add( 0, 0, 1, wxALIGN_CENTER_VERTICAL, 5 );
	
	bSizer69->Add( bSizer87, 0, wxALIGN_CENTER_HORIZONTAL, 5 );
	
	
	bSizer69->Add( 0, 5, 0, 0, 5 );
	
	wxBoxSizer* bSizer70;
	bSizer70 = new wxBoxSizer( wxHORIZONTAL );
	
	m_staticText44 = new wxStaticText( this, wxID_ANY, _("Assemble a batch file for automated synchronization. To start in batch mode simply pass the name of the file to the FreeFileSync executable: FreeFileSync.exe <batchfile>. This can also be scheduled in your operating system's task planner."), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText44->Wrap( 500 );
	bSizer70->Add( m_staticText44, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxALL, 5 );
	
	m_bpButtonHelp = new wxBitmapButton( this, wxID_HELP, wxNullBitmap, wxDefaultPosition, wxSize( 40,40 ), wxBU_AUTODRAW );
	m_bpButtonHelp->SetToolTip( _("Help") );
	
	bSizer70->Add( m_bpButtonHelp, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );
	
	bSizer69->Add( bSizer70, 0, wxALIGN_CENTER_HORIZONTAL, 5 );
	
	
	bSizer69->Add( 0, 5, 0, 0, 5 );
	
	m_staticline10 = new wxStaticLine( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	bSizer69->Add( m_staticline10, 0, wxEXPAND|wxTOP, 5 );
	
	m_staticText531 = new wxStaticText( this, wxID_ANY, _("Configuration overview:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText531->Wrap( -1 );
	m_staticText531->SetFont( wxFont( 10, 74, 90, 92, false, wxT("Arial Black") ) );
	
	bSizer69->Add( m_staticText531, 0, wxALL|wxALIGN_CENTER_HORIZONTAL, 5 );
	
	m_notebookSettings = new wxNotebook( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0 );
	m_panelOverview = new wxPanel( m_notebookSettings, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	wxBoxSizer* bSizer67;
	bSizer67 = new wxBoxSizer( wxHORIZONTAL );
	
	wxBoxSizer* bSizer120;
	bSizer120 = new wxBoxSizer( wxVERTICAL );
	
	wxBoxSizer* bSizer175;
	bSizer175 = new wxBoxSizer( wxHORIZONTAL );
	
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
	
	bSizer175->Add( sbSizer241, 0, wxALIGN_CENTER_VERTICAL, 5 );
	
	
	bSizer175->Add( 0, 0, 1, wxEXPAND, 5 );
	
	wxStaticBoxSizer* sbSizer26;
	sbSizer26 = new wxStaticBoxSizer( new wxStaticBox( m_panelOverview, wxID_ANY, _("Filter files") ), wxVERTICAL );
	
	m_bpButtonFilter = new wxBitmapButton( m_panelOverview, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxSize( 40,40 ), wxBU_AUTODRAW|wxFULL_REPAINT_ON_RESIZE );
	sbSizer26->Add( m_bpButtonFilter, 0, wxALIGN_CENTER_VERTICAL|wxALIGN_CENTER_HORIZONTAL|wxRIGHT|wxLEFT, 15 );
	
	bSizer175->Add( sbSizer26, 0, wxALIGN_CENTER_VERTICAL, 5 );
	
	
	bSizer175->Add( 0, 0, 1, wxEXPAND, 5 );
	
	wxStaticBoxSizer* sbSizer252;
	sbSizer252 = new wxStaticBoxSizer( new wxStaticBox( m_panelOverview, wxID_ANY, _("Synchronize...") ), wxHORIZONTAL );
	
	m_staticTextSyncVariant = new wxStaticText( m_panelOverview, wxID_ANY, _("dummy"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextSyncVariant->Wrap( -1 );
	m_staticTextSyncVariant->SetFont( wxFont( 8, 70, 90, 92, false, wxEmptyString ) );
	m_staticTextSyncVariant->SetForegroundColour( wxSystemSettings::GetColour( wxSYS_COLOUR_GRAYTEXT ) );
	
	sbSizer252->Add( m_staticTextSyncVariant, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 5 );
	
	
	sbSizer252->Add( 10, 0, 0, 0, 5 );
	
	m_bpButtonSyncConfig = new wxBitmapButton( m_panelOverview, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxSize( -1,42 ), wxBU_AUTODRAW );
	m_bpButtonSyncConfig->SetToolTip( _("Synchronization settings") );
	
	sbSizer252->Add( m_bpButtonSyncConfig, 0, wxALIGN_CENTER_VERTICAL, 3 );
	
	bSizer175->Add( sbSizer252, 0, wxALIGN_CENTER_VERTICAL, 5 );
	
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
	
	m_directoryLeft = new wxTextCtrl( m_panelLeft, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	bSizer1141->Add( m_directoryLeft, 1, wxALIGN_CENTER_VERTICAL, 5 );
	
	m_dirPickerLeft = new FfsDirPickerCtrl( m_panelLeft, wxID_ANY, wxEmptyString, _("Select a folder"), wxDefaultPosition, wxDefaultSize, 0 );
	m_dirPickerLeft->SetToolTip( _("Select a folder") );
	
	bSizer1141->Add( m_dirPickerLeft, 0, wxALIGN_CENTER_VERTICAL, 5 );
	
	m_bpButtonLocalFilter = new wxBitmapButton( m_panelLeft, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxSize( 20,20 ), wxBU_AUTODRAW );
	bSizer1141->Add( m_bpButtonLocalFilter, 0, wxALIGN_CENTER_VERTICAL, 5 );
	
	m_panelLeft->SetSizer( bSizer1141 );
	m_panelLeft->Layout();
	bSizer1141->Fit( m_panelLeft );
	bSizer158->Add( m_panelLeft, 0, wxALIGN_CENTER_VERTICAL|wxEXPAND, 5 );
	
	m_panelRight = new wxPanel( m_scrolledWindow6, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	wxBoxSizer* bSizer115;
	bSizer115 = new wxBoxSizer( wxHORIZONTAL );
	
	m_directoryRight = new wxTextCtrl( m_panelRight, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	bSizer115->Add( m_directoryRight, 1, wxALIGN_CENTER_VERTICAL, 5 );
	
	m_dirPickerRight = new FfsDirPickerCtrl( m_panelRight, wxID_ANY, wxEmptyString, _("Select a folder"), wxDefaultPosition, wxDefaultSize, 0 );
	m_dirPickerRight->SetToolTip( _("Select a folder") );
	
	bSizer115->Add( m_dirPickerRight, 0, wxALIGN_CENTER_VERTICAL, 5 );
	
	m_bpButtonAltSyncCfg = new wxBitmapButton( m_panelRight, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxSize( 20,20 ), wxBU_AUTODRAW );
	bSizer115->Add( m_bpButtonAltSyncCfg, 0, wxALIGN_CENTER_VERTICAL, 5 );
	
	m_panelRight->SetSizer( bSizer115 );
	m_panelRight->Layout();
	bSizer115->Fit( m_panelRight );
	bSizer158->Add( m_panelRight, 0, wxEXPAND|wxALIGN_CENTER_VERTICAL, 5 );
	
	sbSizerMainPair->Add( bSizer158, 1, wxALIGN_CENTER_VERTICAL|wxBOTTOM, 5 );
	
	bSizer141->Add( sbSizerMainPair, 0, wxEXPAND, 5 );
	
	bSizerAddFolderPairs = new wxBoxSizer( wxVERTICAL );
	
	bSizer141->Add( bSizerAddFolderPairs, 1, wxEXPAND, 5 );
	
	m_scrolledWindow6->SetSizer( bSizer141 );
	m_scrolledWindow6->Layout();
	bSizer141->Fit( m_scrolledWindow6 );
	bSizer120->Add( m_scrolledWindow6, 1, wxEXPAND, 5 );
	
	
	bSizer120->Add( 0, 5, 0, wxALIGN_CENTER_HORIZONTAL, 5 );
	
	wxFlexGridSizer* fgSizer15;
	fgSizer15 = new wxFlexGridSizer( 1, 2, 10, 10 );
	fgSizer15->SetFlexibleDirection( wxBOTH );
	fgSizer15->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );
	
	wxStaticBoxSizer* sbSizer24;
	sbSizer24 = new wxStaticBoxSizer( new wxStaticBox( m_panelOverview, wxID_ANY, _("Status feedback") ), wxVERTICAL );
	
	
	sbSizer24->Add( 0, 0, 1, wxEXPAND, 5 );
	
	m_checkBoxSilent = new wxCheckBox( m_panelOverview, wxID_ANY, _("Silent mode"), wxDefaultPosition, wxDefaultSize, 0 );
	m_checkBoxSilent->SetToolTip( _("Start minimized and write status information to a logfile") );
	
	sbSizer24->Add( m_checkBoxSilent, 0, wxALIGN_CENTER_HORIZONTAL|wxRIGHT|wxLEFT, 14 );
	
	
	sbSizer24->Add( 0, 0, 1, wxEXPAND, 5 );
	
	fgSizer15->Add( sbSizer24, 0, wxALIGN_CENTER_VERTICAL|wxEXPAND, 5 );
	
	wxStaticBoxSizer* sbSizer25;
	sbSizer25 = new wxStaticBoxSizer( new wxStaticBox( m_panelOverview, wxID_ANY, _("Error handling") ), wxHORIZONTAL );
	
	wxArrayString m_choiceHandleErrorChoices;
	m_choiceHandleError = new wxChoice( m_panelOverview, wxID_ANY, wxDefaultPosition, wxDefaultSize, m_choiceHandleErrorChoices, 0 );
	m_choiceHandleError->SetSelection( 0 );
	sbSizer25->Add( m_choiceHandleError, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );
	
	fgSizer15->Add( sbSizer25, 0, wxEXPAND|wxALIGN_CENTER_VERTICAL, 5 );
	
	bSizer120->Add( fgSizer15, 0, 0, 5 );
	
	bSizer67->Add( bSizer120, 1, wxALIGN_CENTER_VERTICAL|wxALL|wxEXPAND, 10 );
	
	m_panelOverview->SetSizer( bSizer67 );
	m_panelOverview->Layout();
	bSizer67->Fit( m_panelOverview );
	m_notebookSettings->AddPage( m_panelOverview, _("Overview"), true );
	m_panelLogging = new wxPanel( m_notebookSettings, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	wxBoxSizer* bSizer117;
	bSizer117 = new wxBoxSizer( wxVERTICAL );
	
	wxBoxSizer* bSizer119;
	bSizer119 = new wxBoxSizer( wxVERTICAL );
	
	m_staticText120 = new wxStaticText( m_panelLogging, wxID_ANY, _("Select logfile directory:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText120->Wrap( -1 );
	bSizer119->Add( m_staticText120, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );
	
	sbSizerLogfileDir = new wxStaticBoxSizer( new wxStaticBox( m_panelLogging, wxID_ANY, _("Drag && drop") ), wxHORIZONTAL );
	
	m_textCtrlLogfileDir = new wxTextCtrl( m_panelLogging, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	sbSizerLogfileDir->Add( m_textCtrlLogfileDir, 1, wxALIGN_CENTER_VERTICAL, 5 );
	
	m_dirPickerLogfileDir = new FfsDirPickerCtrl( m_panelLogging, wxID_ANY, wxEmptyString, _("Select a folder"), wxDefaultPosition, wxDefaultSize, 0 );
	m_dirPickerLogfileDir->SetToolTip( _("Select a folder") );
	
	sbSizerLogfileDir->Add( m_dirPickerLogfileDir, 0, wxALIGN_CENTER_VERTICAL, 5 );
	
	bSizer119->Add( sbSizerLogfileDir, 1, wxEXPAND, 5 );
	
	wxBoxSizer* bSizer152;
	bSizer152 = new wxBoxSizer( wxHORIZONTAL );
	
	m_staticText96 = new wxStaticText( m_panelLogging, wxID_ANY, _("Maximum number of logfiles:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText96->Wrap( -1 );
	bSizer152->Add( m_staticText96, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );
	
	
	bSizer152->Add( 0, 0, 1, wxEXPAND, 5 );
	
	m_spinCtrlLogCountMax = new wxSpinCtrl( m_panelLogging, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 0, 2000000000, 0 );
	bSizer152->Add( m_spinCtrlLogCountMax, 0, wxALL, 5 );
	
	bSizer119->Add( bSizer152, 0, wxEXPAND, 5 );
	
	bSizer117->Add( bSizer119, 0, wxEXPAND|wxALL, 10 );
	
	m_panelLogging->SetSizer( bSizer117 );
	m_panelLogging->Layout();
	bSizer117->Fit( m_panelLogging );
	m_notebookSettings->AddPage( m_panelLogging, _("Logging"), false );
	
	bSizer69->Add( m_notebookSettings, 1, wxEXPAND, 5 );
	
	wxBoxSizer* bSizer68;
	bSizer68 = new wxBoxSizer( wxHORIZONTAL );
	
	m_buttonSave = new wxButton( this, wxID_SAVE, _("&Save"), wxDefaultPosition, wxSize( -1,30 ), 0 );
	m_buttonSave->SetDefault(); 
	m_buttonSave->SetFont( wxFont( 10, 70, 90, 92, false, wxEmptyString ) );
	
	bSizer68->Add( m_buttonSave, 0, wxALL|wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 5 );
	
	m_buttonLoad = new wxButton( this, wxID_OPEN, _("&Load"), wxDefaultPosition, wxSize( -1,30 ), 0 );
	m_buttonLoad->SetFont( wxFont( 10, 70, 90, 90, false, wxEmptyString ) );
	
	bSizer68->Add( m_buttonLoad, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );
	
	m_button6 = new wxButton( this, wxID_CANCEL, _("&Cancel"), wxDefaultPosition, wxSize( -1,30 ), 0 );
	m_button6->SetFont( wxFont( 10, 70, 90, 90, false, wxEmptyString ) );
	
	bSizer68->Add( m_button6, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );
	
	bSizer69->Add( bSizer68, 0, wxALIGN_CENTER_HORIZONTAL, 5 );
	
	bSizer54->Add( bSizer69, 1, wxALIGN_CENTER_HORIZONTAL|wxALL|wxEXPAND, 5 );
	
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
	m_checkBoxSilent->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( BatchDlgGenerated::OnCheckSilent ), NULL, this );
	m_choiceHandleError->Connect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( BatchDlgGenerated::OnChangeErrorHandling ), NULL, this );
	m_spinCtrlLogCountMax->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( BatchDlgGenerated::OnChangeMaxLogCountTxt ), NULL, this );
	m_buttonSave->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( BatchDlgGenerated::OnSaveBatchJob ), NULL, this );
	m_buttonLoad->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( BatchDlgGenerated::OnLoadBatchJob ), NULL, this );
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
	m_checkBoxSilent->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( BatchDlgGenerated::OnCheckSilent ), NULL, this );
	m_choiceHandleError->Disconnect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( BatchDlgGenerated::OnChangeErrorHandling ), NULL, this );
	m_spinCtrlLogCountMax->Disconnect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( BatchDlgGenerated::OnChangeMaxLogCountTxt ), NULL, this );
	m_buttonSave->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( BatchDlgGenerated::OnSaveBatchJob ), NULL, this );
	m_buttonLoad->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( BatchDlgGenerated::OnLoadBatchJob ), NULL, this );
	m_button6->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( BatchDlgGenerated::OnCancel ), NULL, this );
	
}

CompareStatusGenerated::CompareStatusGenerated( wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style ) : wxPanel( parent, id, pos, size, style )
{
	wxBoxSizer* bSizer40;
	bSizer40 = new wxBoxSizer( wxVERTICAL );
	
	
	bSizer40->Add( 0, 0, 1, wxEXPAND, 5 );
	
	bSizer42 = new wxBoxSizer( wxHORIZONTAL );
	
	wxBoxSizer* bSizer157;
	bSizer157 = new wxBoxSizer( wxVERTICAL );
	
	bSizerFilesFound = new wxBoxSizer( wxHORIZONTAL );
	
	m_staticText321 = new wxStaticText( this, wxID_ANY, _("Elements found:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText321->Wrap( -1 );
	m_staticText321->SetFont( wxFont( 10, 70, 90, 90, false, wxEmptyString ) );
	
	bSizerFilesFound->Add( m_staticText321, 0, wxALIGN_BOTTOM, 5 );
	
	m_staticTextScanned = new wxStaticText( this, wxID_ANY, _("dummy"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextScanned->Wrap( -1 );
	m_staticTextScanned->SetFont( wxFont( 9, 74, 90, 92, false, wxT("Arial") ) );
	
	bSizerFilesFound->Add( m_staticTextScanned, 0, wxALIGN_BOTTOM|wxLEFT, 5 );
	
	bSizer157->Add( bSizerFilesFound, 0, 0, 5 );
	
	bSizerFilesRemaining = new wxBoxSizer( wxHORIZONTAL );
	
	m_staticText46 = new wxStaticText( this, wxID_ANY, _("Elements remaining:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText46->Wrap( -1 );
	m_staticText46->SetFont( wxFont( 10, 70, 90, 90, false, wxEmptyString ) );
	
	bSizerFilesRemaining->Add( m_staticText46, 0, wxALIGN_BOTTOM, 5 );
	
	wxBoxSizer* bSizer154;
	bSizer154 = new wxBoxSizer( wxHORIZONTAL );
	
	m_staticTextFilesRemaining = new wxStaticText( this, wxID_ANY, _("dummy"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextFilesRemaining->Wrap( -1 );
	m_staticTextFilesRemaining->SetFont( wxFont( 9, 74, 90, 92, false, wxT("Arial") ) );
	
	bSizer154->Add( m_staticTextFilesRemaining, 0, wxALIGN_BOTTOM, 5 );
	
	m_staticText117 = new wxStaticText( this, wxID_ANY, _("("), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText117->Wrap( -1 );
	m_staticText117->SetFont( wxFont( 9, 74, 90, 90, false, wxT("Arial") ) );
	
	bSizer154->Add( m_staticText117, 0, wxLEFT|wxALIGN_BOTTOM, 5 );
	
	m_staticTextDataRemaining = new wxStaticText( this, wxID_ANY, _("dummy"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextDataRemaining->Wrap( -1 );
	m_staticTextDataRemaining->SetFont( wxFont( 9, 74, 90, 90, false, wxT("Arial") ) );
	
	bSizer154->Add( m_staticTextDataRemaining, 0, wxALIGN_BOTTOM, 5 );
	
	m_staticText118 = new wxStaticText( this, wxID_ANY, _(")"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText118->Wrap( -1 );
	m_staticText118->SetFont( wxFont( 9, 74, 90, 90, false, wxT("Arial") ) );
	
	bSizer154->Add( m_staticText118, 0, wxALIGN_BOTTOM, 5 );
	
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
	m_staticTextSpeed->SetFont( wxFont( 9, 74, 90, 92, false, wxT("Arial") ) );
	
	sSizerSpeed->Add( m_staticTextSpeed, 0, wxLEFT|wxALIGN_BOTTOM, 5 );
	
	bSizer42->Add( sSizerSpeed, 0, wxALIGN_CENTER_VERTICAL, 5 );
	
	
	bSizer42->Add( 10, 0, 0, wxALIGN_CENTER_VERTICAL, 5 );
	
	sSizerTimeRemaining = new wxBoxSizer( wxHORIZONTAL );
	
	m_staticTextTimeRemFixed = new wxStaticText( this, wxID_ANY, _("Time remaining:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextTimeRemFixed->Wrap( -1 );
	m_staticTextTimeRemFixed->SetFont( wxFont( 10, 70, 90, 90, false, wxEmptyString ) );
	
	sSizerTimeRemaining->Add( m_staticTextTimeRemFixed, 0, wxALIGN_BOTTOM, 5 );
	
	m_staticTextTimeRemaining = new wxStaticText( this, wxID_ANY, _("dummy"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextTimeRemaining->Wrap( -1 );
	m_staticTextTimeRemaining->SetFont( wxFont( 9, 74, 90, 92, false, wxT("Arial") ) );
	
	sSizerTimeRemaining->Add( m_staticTextTimeRemaining, 0, wxLEFT|wxALIGN_BOTTOM, 5 );
	
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
	m_staticTextTimeElapsed->SetFont( wxFont( 9, 74, 90, 92, false, wxT("Arial") ) );
	
	sSizerTimeElapsed->Add( m_staticTextTimeElapsed, 0, wxLEFT|wxALIGN_BOTTOM, 5 );
	
	bSizer42->Add( sSizerTimeElapsed, 0, wxALIGN_CENTER_VERTICAL, 5 );
	
	bSizer40->Add( bSizer42, 0, wxALIGN_CENTER_HORIZONTAL|wxEXPAND|wxBOTTOM|wxRIGHT|wxLEFT, 5 );
	
	wxBoxSizer* bSizer48;
	bSizer48 = new wxBoxSizer( wxHORIZONTAL );
	
	m_staticText30 = new wxStaticText( this, wxID_ANY, _("Operation:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText30->Wrap( -1 );
	m_staticText30->SetFont( wxFont( 10, 70, 90, 92, false, wxEmptyString ) );
	
	bSizer48->Add( m_staticText30, 0, wxRIGHT|wxALIGN_CENTER_VERTICAL, 5 );
	
	m_textCtrlStatus = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_READONLY );
	m_textCtrlStatus->SetBackgroundColour( wxColour( 208, 208, 208 ) );
	
	bSizer48->Add( m_textCtrlStatus, 1, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 5 );
	
	bSizer40->Add( bSizer48, 0, wxEXPAND|wxALIGN_CENTER_HORIZONTAL|wxBOTTOM|wxRIGHT|wxLEFT, 5 );
	
	m_gauge2 = new wxGauge( this, wxID_ANY, 100, wxDefaultPosition, wxSize( -1,14 ), wxGA_HORIZONTAL|wxGA_SMOOTH );
	bSizer40->Add( m_gauge2, 0, wxEXPAND|wxRIGHT|wxLEFT, 5 );
	
	
	bSizer40->Add( 0, 0, 1, wxEXPAND, 5 );
	
	this->SetSizer( bSizer40 );
	this->Layout();
	bSizer40->Fit( this );
}

CompareStatusGenerated::~CompareStatusGenerated()
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
	sbSizer7 = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, wxEmptyString ), wxVERTICAL );
	
	m_staticText1 = new wxStaticText( this, wxID_ANY, _("Select variant:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText1->Wrap( -1 );
	m_staticText1->SetFont( wxFont( 10, 70, 90, 92, false, wxEmptyString ) );
	
	sbSizer7->Add( m_staticText1, 0, wxALL, 5 );
	
	wxFlexGridSizer* fgSizer1;
	fgSizer1 = new wxFlexGridSizer( 4, 3, 8, 5 );
	fgSizer1->SetFlexibleDirection( wxHORIZONTAL );
	fgSizer1->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );
	
	m_radioBtnAutomatic = new wxRadioButton( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_radioBtnAutomatic->SetValue( true ); 
	m_radioBtnAutomatic->SetFont( wxFont( 11, 70, 90, 92, false, wxEmptyString ) );
	
	fgSizer1->Add( m_radioBtnAutomatic, 0, wxALIGN_CENTER_VERTICAL, 5 );
	
	m_buttonAutomatic = new wxButton( this, wxID_ANY, _("<Automatic>"), wxDefaultPosition, wxSize( -1,-1 ), 0 );
	m_buttonAutomatic->SetFont( wxFont( 11, 70, 90, 92, false, wxEmptyString ) );
	
	fgSizer1->Add( m_buttonAutomatic, 0, wxALIGN_CENTER_VERTICAL|wxEXPAND, 5 );
	
	m_staticText81 = new wxStaticText( this, wxID_ANY, _("Identify and propagate changes on both sides using a database. Deletions and conflicts are detected automatically."), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText81->Wrap( 300 );
	fgSizer1->Add( m_staticText81, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );
	
	m_radioBtnMirror = new wxRadioButton( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_radioBtnMirror->SetFont( wxFont( 11, 70, 90, 92, false, wxEmptyString ) );
	
	fgSizer1->Add( m_radioBtnMirror, 0, wxALIGN_CENTER_VERTICAL, 5 );
	
	m_buttonOneWay = new wxButton( this, wxID_ANY, _("Mirror ->>"), wxDefaultPosition, wxSize( -1,-1 ), 0 );
	m_buttonOneWay->SetFont( wxFont( 11, 70, 90, 92, false, wxEmptyString ) );
	
	fgSizer1->Add( m_buttonOneWay, 0, wxALIGN_CENTER_VERTICAL|wxEXPAND, 5 );
	
	m_staticText8 = new wxStaticText( this, wxID_ANY, _("Mirror backup of left folder. Right folder is modified to exactly match left folder after synchronization."), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText8->Wrap( 300 );
	fgSizer1->Add( m_staticText8, 0, wxLEFT|wxALIGN_CENTER_VERTICAL, 5 );
	
	m_radioBtnUpdate = new wxRadioButton( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_radioBtnUpdate->SetFont( wxFont( 11, 70, 90, 92, false, wxEmptyString ) );
	
	fgSizer1->Add( m_radioBtnUpdate, 0, wxALIGN_CENTER_VERTICAL, 5 );
	
	m_buttonUpdate = new wxButton( this, wxID_ANY, _("Update ->"), wxDefaultPosition, wxSize( -1,-1 ), 0 );
	m_buttonUpdate->SetFont( wxFont( 11, 70, 90, 92, false, wxEmptyString ) );
	
	fgSizer1->Add( m_buttonUpdate, 0, wxALIGN_CENTER_VERTICAL|wxEXPAND, 5 );
	
	m_staticText101 = new wxStaticText( this, wxID_ANY, _("Copy new or updated files to right folder."), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText101->Wrap( 300 );
	fgSizer1->Add( m_staticText101, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );
	
	m_radioBtnCustom = new wxRadioButton( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_radioBtnCustom->SetFont( wxFont( 11, 70, 90, 92, false, wxEmptyString ) );
	m_radioBtnCustom->Enable( false );
	
	fgSizer1->Add( m_radioBtnCustom, 0, wxALIGN_CENTER_VERTICAL, 5 );
	
	wxBoxSizer* bSizer65;
	bSizer65 = new wxBoxSizer( wxVERTICAL );
	
	
	bSizer65->Add( 0, 0, 1, wxEXPAND, 5 );
	
	m_staticText23 = new wxStaticText( this, wxID_ANY, _("Custom"), wxDefaultPosition, wxSize( -1,-1 ), wxALIGN_CENTRE|wxSTATIC_BORDER );
	m_staticText23->Wrap( -1 );
	m_staticText23->SetFont( wxFont( 11, 70, 90, 92, false, wxEmptyString ) );
	
	bSizer65->Add( m_staticText23, 0, wxALIGN_CENTER_VERTICAL|wxEXPAND, 5 );
	
	
	bSizer65->Add( 0, 0, 1, wxEXPAND, 5 );
	
	fgSizer1->Add( bSizer65, 0, wxALIGN_CENTER_VERTICAL|wxEXPAND, 5 );
	
	m_staticText9 = new wxStaticText( this, wxID_ANY, _("Configure your own synchronization rules."), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText9->Wrap( 300 );
	fgSizer1->Add( m_staticText9, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );
	
	sbSizer7->Add( fgSizer1, 0, 0, 5 );
	
	bSizer29->Add( sbSizer7, 0, wxEXPAND, 5 );
	
	
	bSizer29->Add( 0, 5, 1, 0, 5 );
	
	bSizer201 = new wxBoxSizer( wxHORIZONTAL );
	
	sbSizerErrorHandling = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Error handling") ), wxHORIZONTAL );
	
	wxArrayString m_choiceHandleErrorChoices;
	m_choiceHandleError = new wxChoice( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, m_choiceHandleErrorChoices, 0 );
	m_choiceHandleError->SetSelection( 0 );
	sbSizerErrorHandling->Add( m_choiceHandleError, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );
	
	bSizer201->Add( sbSizerErrorHandling, 0, wxEXPAND|wxRIGHT, 10 );
	
	sbSizerCustDelDir = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Deletion handling") ), wxVERTICAL );
	
	wxArrayString m_choiceHandleDeletionChoices;
	m_choiceHandleDeletion = new wxChoice( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, m_choiceHandleDeletionChoices, 0 );
	m_choiceHandleDeletion->SetSelection( 0 );
	sbSizerCustDelDir->Add( m_choiceHandleDeletion, 0, wxBOTTOM, 5 );
	
	m_panelCustomDeletionDir = new wxPanel( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	wxBoxSizer* bSizer1151;
	bSizer1151 = new wxBoxSizer( wxHORIZONTAL );
	
	m_textCtrlCustomDelFolder = new wxTextCtrl( m_panelCustomDeletionDir, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_textCtrlCustomDelFolder->SetMinSize( wxSize( 200,-1 ) );
	
	bSizer1151->Add( m_textCtrlCustomDelFolder, 1, wxALIGN_CENTER_VERTICAL, 5 );
	
	m_dirPickerCustomDelFolder = new FfsDirPickerCtrl( m_panelCustomDeletionDir, wxID_ANY, wxEmptyString, _("Select a folder"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizer1151->Add( m_dirPickerCustomDelFolder, 0, wxALIGN_CENTER_VERTICAL, 5 );
	
	m_panelCustomDeletionDir->SetSizer( bSizer1151 );
	m_panelCustomDeletionDir->Layout();
	bSizer1151->Fit( m_panelCustomDeletionDir );
	sbSizerCustDelDir->Add( m_panelCustomDeletionDir, 0, 0, 5 );
	
	bSizer201->Add( sbSizerCustDelDir, 0, wxEXPAND, 5 );
	
	bSizer29->Add( bSizer201, 0, wxTOP|wxBOTTOM, 5 );
	
	
	bSizer29->Add( 0, 5, 1, 0, 5 );
	
	wxBoxSizer* bSizer291;
	bSizer291 = new wxBoxSizer( wxHORIZONTAL );
	
	m_buttonOK = new wxButton( this, wxID_OK, _("&OK"), wxDefaultPosition, wxSize( -1,30 ), 0 );
	m_buttonOK->SetDefault(); 
	m_buttonOK->SetFont( wxFont( 10, 70, 90, 92, false, wxEmptyString ) );
	
	bSizer291->Add( m_buttonOK, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );
	
	m_button16 = new wxButton( this, wxID_CANCEL, _("&Cancel"), wxDefaultPosition, wxSize( -1,30 ), 0 );
	m_button16->SetFont( wxFont( 10, 70, 90, 90, false, wxEmptyString ) );
	
	bSizer291->Add( m_button16, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );
	
	
	bSizer291->Add( 20, 0, 1, wxALIGN_CENTER_VERTICAL, 5 );
	
	bSizer29->Add( bSizer291, 0, wxEXPAND, 5 );
	
	bSizer181->Add( bSizer29, 0, wxEXPAND, 5 );
	
	
	bSizer181->Add( 10, 0, 0, 0, 5 );
	
	sbSizerSyncDirections = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Configuration") ), wxVERTICAL );
	
	wxGridSizer* gSizer3;
	gSizer3 = new wxGridSizer( 1, 2, 0, 5 );
	
	m_staticText21 = new wxStaticText( this, wxID_ANY, _("Category"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText21->Wrap( -1 );
	m_staticText21->SetFont( wxFont( 8, 70, 90, 92, false, wxEmptyString ) );
	
	gSizer3->Add( m_staticText21, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 5 );
	
	m_staticText31 = new wxStaticText( this, wxID_ANY, _("Action"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText31->Wrap( -1 );
	m_staticText31->SetFont( wxFont( 8, 70, 90, 92, false, wxEmptyString ) );
	
	gSizer3->Add( m_staticText31, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 5 );
	
	sbSizerSyncDirections->Add( gSizer3, 0, wxEXPAND, 5 );
	
	m_staticline3 = new wxStaticLine( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	sbSizerSyncDirections->Add( m_staticline3, 0, wxALIGN_CENTER_HORIZONTAL|wxEXPAND|wxTOP|wxBOTTOM, 5 );
	
	wxBoxSizer* bSizer121;
	bSizer121 = new wxBoxSizer( wxVERTICAL );
	
	wxBoxSizer* bSizer122;
	bSizer122 = new wxBoxSizer( wxHORIZONTAL );
	
	m_bitmapLeftOnly = new wxStaticBitmap( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxSize( 45,45 ), 0 );
	m_bitmapLeftOnly->SetToolTip( _("Files/folders that exist on left side only") );
	
	bSizer122->Add( m_bitmapLeftOnly, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 5 );
	
	
	bSizer122->Add( 5, 0, 0, wxALIGN_CENTER_VERTICAL, 5 );
	
	m_bpButtonLeftOnly = new wxBitmapButton( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxSize( 42,42 ), wxBU_AUTODRAW );
	bSizer122->Add( m_bpButtonLeftOnly, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 5 );
	
	bSizer121->Add( bSizer122, 0, wxALIGN_CENTER_HORIZONTAL, 5 );
	
	wxBoxSizer* bSizer123;
	bSizer123 = new wxBoxSizer( wxHORIZONTAL );
	
	m_bitmapRightOnly = new wxStaticBitmap( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxSize( 45,45 ), 0 );
	m_bitmapRightOnly->SetToolTip( _("Files/folders that exist on right side only") );
	
	bSizer123->Add( m_bitmapRightOnly, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 5 );
	
	
	bSizer123->Add( 5, 0, 0, 0, 5 );
	
	m_bpButtonRightOnly = new wxBitmapButton( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxSize( 42,42 ), wxBU_AUTODRAW );
	bSizer123->Add( m_bpButtonRightOnly, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 5 );
	
	bSizer121->Add( bSizer123, 0, wxALIGN_CENTER_HORIZONTAL, 5 );
	
	wxBoxSizer* bSizer124;
	bSizer124 = new wxBoxSizer( wxHORIZONTAL );
	
	m_bitmapLeftNewer = new wxStaticBitmap( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxSize( 45,45 ), 0 );
	m_bitmapLeftNewer->SetToolTip( _("Files that exist on both sides, left one is newer") );
	
	bSizer124->Add( m_bitmapLeftNewer, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 5 );
	
	
	bSizer124->Add( 5, 0, 0, 0, 5 );
	
	m_bpButtonLeftNewer = new wxBitmapButton( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxSize( 42,42 ), wxBU_AUTODRAW );
	bSizer124->Add( m_bpButtonLeftNewer, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 5 );
	
	bSizer121->Add( bSizer124, 0, wxALIGN_CENTER_HORIZONTAL, 5 );
	
	wxBoxSizer* bSizer125;
	bSizer125 = new wxBoxSizer( wxHORIZONTAL );
	
	m_bitmapRightNewer = new wxStaticBitmap( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxSize( 45,45 ), 0 );
	m_bitmapRightNewer->SetToolTip( _("Files that exist on both sides, right one is newer") );
	
	bSizer125->Add( m_bitmapRightNewer, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 5 );
	
	
	bSizer125->Add( 5, 0, 0, 0, 5 );
	
	m_bpButtonRightNewer = new wxBitmapButton( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxSize( 42,42 ), wxBU_AUTODRAW );
	bSizer125->Add( m_bpButtonRightNewer, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 5 );
	
	bSizer121->Add( bSizer125, 0, wxALIGN_CENTER_HORIZONTAL, 5 );
	
	wxBoxSizer* bSizer126;
	bSizer126 = new wxBoxSizer( wxHORIZONTAL );
	
	m_bitmapDifferent = new wxStaticBitmap( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxSize( 45,45 ), 0 );
	m_bitmapDifferent->SetToolTip( _("Files that have different content") );
	
	bSizer126->Add( m_bitmapDifferent, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 5 );
	
	
	bSizer126->Add( 5, 0, 0, 0, 5 );
	
	m_bpButtonDifferent = new wxBitmapButton( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxSize( 42,42 ), wxBU_AUTODRAW );
	bSizer126->Add( m_bpButtonDifferent, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 5 );
	
	bSizer121->Add( bSizer126, 0, wxALIGN_CENTER_HORIZONTAL, 5 );
	
	wxBoxSizer* bSizer127;
	bSizer127 = new wxBoxSizer( wxHORIZONTAL );
	
	m_bitmapConflict = new wxStaticBitmap( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxSize( 45,45 ), 0 );
	m_bitmapConflict->SetToolTip( _("Conflicts/files that cannot be categorized") );
	
	bSizer127->Add( m_bitmapConflict, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 5 );
	
	
	bSizer127->Add( 5, 0, 0, 0, 5 );
	
	m_bpButtonConflict = new wxBitmapButton( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxSize( 42,42 ), wxBU_AUTODRAW );
	bSizer127->Add( m_bpButtonConflict, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 5 );
	
	bSizer121->Add( bSizer127, 0, wxALIGN_CENTER_HORIZONTAL, 5 );
	
	sbSizerSyncDirections->Add( bSizer121, 0, wxALIGN_CENTER_HORIZONTAL, 5 );
	
	bSizer181->Add( sbSizerSyncDirections, 0, wxALIGN_CENTER_HORIZONTAL, 5 );
	
	bSizer7->Add( bSizer181, 0, wxALL, 5 );
	
	this->SetSizer( bSizer7 );
	this->Layout();
	bSizer7->Fit( this );
	
	this->Centre( wxBOTH );
	
	// Connect Events
	this->Connect( wxEVT_CLOSE_WINDOW, wxCloseEventHandler( SyncCfgDlgGenerated::OnClose ) );
	m_radioBtnAutomatic->Connect( wxEVT_COMMAND_RADIOBUTTON_SELECTED, wxCommandEventHandler( SyncCfgDlgGenerated::OnSyncAutomatic ), NULL, this );
	m_buttonAutomatic->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( SyncCfgDlgGenerated::OnSyncAutomatic ), NULL, this );
	m_radioBtnMirror->Connect( wxEVT_COMMAND_RADIOBUTTON_SELECTED, wxCommandEventHandler( SyncCfgDlgGenerated::OnSyncLeftToRight ), NULL, this );
	m_buttonOneWay->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( SyncCfgDlgGenerated::OnSyncLeftToRight ), NULL, this );
	m_radioBtnUpdate->Connect( wxEVT_COMMAND_RADIOBUTTON_SELECTED, wxCommandEventHandler( SyncCfgDlgGenerated::OnSyncUpdate ), NULL, this );
	m_buttonUpdate->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( SyncCfgDlgGenerated::OnSyncUpdate ), NULL, this );
	m_radioBtnCustom->Connect( wxEVT_COMMAND_RADIOBUTTON_SELECTED, wxCommandEventHandler( SyncCfgDlgGenerated::OnSyncCustom ), NULL, this );
	m_choiceHandleError->Connect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( SyncCfgDlgGenerated::OnChangeErrorHandling ), NULL, this );
	m_choiceHandleDeletion->Connect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( SyncCfgDlgGenerated::OnChangeDeletionHandling ), NULL, this );
	m_buttonOK->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( SyncCfgDlgGenerated::OnApply ), NULL, this );
	m_button16->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( SyncCfgDlgGenerated::OnCancel ), NULL, this );
	m_bpButtonLeftOnly->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( SyncCfgDlgGenerated::OnExLeftSideOnly ), NULL, this );
	m_bpButtonRightOnly->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( SyncCfgDlgGenerated::OnExRightSideOnly ), NULL, this );
	m_bpButtonLeftNewer->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( SyncCfgDlgGenerated::OnLeftNewer ), NULL, this );
	m_bpButtonRightNewer->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( SyncCfgDlgGenerated::OnRightNewer ), NULL, this );
	m_bpButtonDifferent->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( SyncCfgDlgGenerated::OnDifferent ), NULL, this );
	m_bpButtonConflict->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( SyncCfgDlgGenerated::OnConflict ), NULL, this );
}

SyncCfgDlgGenerated::~SyncCfgDlgGenerated()
{
	// Disconnect Events
	this->Disconnect( wxEVT_CLOSE_WINDOW, wxCloseEventHandler( SyncCfgDlgGenerated::OnClose ) );
	m_radioBtnAutomatic->Disconnect( wxEVT_COMMAND_RADIOBUTTON_SELECTED, wxCommandEventHandler( SyncCfgDlgGenerated::OnSyncAutomatic ), NULL, this );
	m_buttonAutomatic->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( SyncCfgDlgGenerated::OnSyncAutomatic ), NULL, this );
	m_radioBtnMirror->Disconnect( wxEVT_COMMAND_RADIOBUTTON_SELECTED, wxCommandEventHandler( SyncCfgDlgGenerated::OnSyncLeftToRight ), NULL, this );
	m_buttonOneWay->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( SyncCfgDlgGenerated::OnSyncLeftToRight ), NULL, this );
	m_radioBtnUpdate->Disconnect( wxEVT_COMMAND_RADIOBUTTON_SELECTED, wxCommandEventHandler( SyncCfgDlgGenerated::OnSyncUpdate ), NULL, this );
	m_buttonUpdate->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( SyncCfgDlgGenerated::OnSyncUpdate ), NULL, this );
	m_radioBtnCustom->Disconnect( wxEVT_COMMAND_RADIOBUTTON_SELECTED, wxCommandEventHandler( SyncCfgDlgGenerated::OnSyncCustom ), NULL, this );
	m_choiceHandleError->Disconnect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( SyncCfgDlgGenerated::OnChangeErrorHandling ), NULL, this );
	m_choiceHandleDeletion->Disconnect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( SyncCfgDlgGenerated::OnChangeDeletionHandling ), NULL, this );
	m_buttonOK->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( SyncCfgDlgGenerated::OnApply ), NULL, this );
	m_button16->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( SyncCfgDlgGenerated::OnCancel ), NULL, this );
	m_bpButtonLeftOnly->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( SyncCfgDlgGenerated::OnExLeftSideOnly ), NULL, this );
	m_bpButtonRightOnly->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( SyncCfgDlgGenerated::OnExRightSideOnly ), NULL, this );
	m_bpButtonLeftNewer->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( SyncCfgDlgGenerated::OnLeftNewer ), NULL, this );
	m_bpButtonRightNewer->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( SyncCfgDlgGenerated::OnRightNewer ), NULL, this );
	m_bpButtonDifferent->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( SyncCfgDlgGenerated::OnDifferent ), NULL, this );
	m_bpButtonConflict->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( SyncCfgDlgGenerated::OnConflict ), NULL, this );
	
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
	m_radioBtnSizeDate->SetToolTip( _("Files are found equal if\n     - filesize\n     - last write time and date\nare the same") );
	
	fgSizer16->Add( m_radioBtnSizeDate, 0, wxALIGN_CENTER_VERTICAL, 5 );
	
	m_bitmapByTime = new wxStaticBitmap( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, 0 );
	m_bitmapByTime->SetToolTip( _("Files are found equal if\n     - filesize\n     - last write time and date\nare the same") );
	
	fgSizer16->Add( m_bitmapByTime, 0, wxALIGN_CENTER_VERTICAL|wxALIGN_CENTER_HORIZONTAL, 5 );
	
	m_buttonTimeSize = new wxButton( this, wxID_ANY, _("File size and date"), wxDefaultPosition, wxSize( -1,42 ), 0 );
	m_buttonTimeSize->SetFont( wxFont( 11, 70, 90, 92, false, wxEmptyString ) );
	m_buttonTimeSize->SetToolTip( _("Files are found equal if\n     - filesize\n     - last write time and date\nare the same") );
	
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
	
	m_staticline14 = new wxStaticLine( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_VERTICAL );
	sbSizer6->Add( m_staticline14, 0, wxEXPAND|wxRIGHT|wxLEFT, 5 );
	
	m_bpButtonHelp = new wxBitmapButton( this, wxID_HELP, wxNullBitmap, wxDefaultPosition, wxSize( 40,40 ), wxBU_AUTODRAW );
	m_bpButtonHelp->SetToolTip( _("Help") );
	
	sbSizer6->Add( m_bpButtonHelp, 0, wxALIGN_CENTER_VERTICAL, 5 );
	
	bSizer55->Add( sbSizer6, 0, wxALIGN_CENTER_VERTICAL|wxEXPAND, 2 );
	
	
	bSizer55->Add( 0, 4, 0, 0, 5 );
	
	wxStaticBoxSizer* sbSizer25;
	sbSizer25 = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Symbolic Link handling") ), wxVERTICAL );
	
	wxArrayString m_choiceHandleSymlinksChoices;
	m_choiceHandleSymlinks = new wxChoice( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, m_choiceHandleSymlinksChoices, 0 );
	m_choiceHandleSymlinks->SetSelection( 0 );
	sbSizer25->Add( m_choiceHandleSymlinks, 0, wxALIGN_CENTER_VERTICAL|wxALIGN_CENTER_HORIZONTAL, 5 );
	
	bSizer55->Add( sbSizer25, 0, wxEXPAND|wxTOP, 5 );
	
	wxBoxSizer* bSizer22;
	bSizer22 = new wxBoxSizer( wxHORIZONTAL );
	
	m_button10 = new wxButton( this, wxID_OK, _("&OK"), wxDefaultPosition, wxSize( -1,30 ), 0 );
	m_button10->SetDefault(); 
	m_button10->SetFont( wxFont( 10, 70, 90, 92, false, wxEmptyString ) );
	
	bSizer22->Add( m_button10, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );
	
	m_button6 = new wxButton( this, wxID_CANCEL, _("&Cancel"), wxDefaultPosition, wxSize( -1,30 ), 0 );
	m_button6->SetFont( wxFont( 10, 70, 90, 90, false, wxEmptyString ) );
	
	bSizer22->Add( m_button6, 0, wxALL|wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 5 );
	
	bSizer55->Add( bSizer22, 0, wxALIGN_CENTER_HORIZONTAL, 5 );
	
	bSizer136->Add( bSizer55, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );
	
	this->SetSizer( bSizer136 );
	this->Layout();
	bSizer136->Fit( this );
	
	// Connect Events
	this->Connect( wxEVT_CLOSE_WINDOW, wxCloseEventHandler( CmpCfgDlgGenerated::OnClose ) );
	m_radioBtnSizeDate->Connect( wxEVT_COMMAND_RADIOBUTTON_SELECTED, wxCommandEventHandler( CmpCfgDlgGenerated::OnTimeSize ), NULL, this );
	m_buttonTimeSize->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( CmpCfgDlgGenerated::OnTimeSize ), NULL, this );
	m_radioBtnContent->Connect( wxEVT_COMMAND_RADIOBUTTON_SELECTED, wxCommandEventHandler( CmpCfgDlgGenerated::OnContent ), NULL, this );
	m_buttonContent->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( CmpCfgDlgGenerated::OnContent ), NULL, this );
	m_bpButtonHelp->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( CmpCfgDlgGenerated::OnShowHelp ), NULL, this );
	m_choiceHandleSymlinks->Connect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( CmpCfgDlgGenerated::OnChangeErrorHandling ), NULL, this );
	m_button10->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( CmpCfgDlgGenerated::OnOkay ), NULL, this );
	m_button6->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( CmpCfgDlgGenerated::OnCancel ), NULL, this );
}

CmpCfgDlgGenerated::~CmpCfgDlgGenerated()
{
	// Disconnect Events
	this->Disconnect( wxEVT_CLOSE_WINDOW, wxCloseEventHandler( CmpCfgDlgGenerated::OnClose ) );
	m_radioBtnSizeDate->Disconnect( wxEVT_COMMAND_RADIOBUTTON_SELECTED, wxCommandEventHandler( CmpCfgDlgGenerated::OnTimeSize ), NULL, this );
	m_buttonTimeSize->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( CmpCfgDlgGenerated::OnTimeSize ), NULL, this );
	m_radioBtnContent->Disconnect( wxEVT_COMMAND_RADIOBUTTON_SELECTED, wxCommandEventHandler( CmpCfgDlgGenerated::OnContent ), NULL, this );
	m_buttonContent->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( CmpCfgDlgGenerated::OnContent ), NULL, this );
	m_bpButtonHelp->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( CmpCfgDlgGenerated::OnShowHelp ), NULL, this );
	m_choiceHandleSymlinks->Disconnect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( CmpCfgDlgGenerated::OnChangeErrorHandling ), NULL, this );
	m_button10->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( CmpCfgDlgGenerated::OnOkay ), NULL, this );
	m_button6->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( CmpCfgDlgGenerated::OnCancel ), NULL, this );
	
}

SyncStatusDlgGenerated::SyncStatusDlgGenerated( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : wxFrame( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxSize( 470,300 ), wxDefaultSize );
	this->SetBackgroundColour( wxSystemSettings::GetColour( wxSYS_COLOUR_BTNFACE ) );
	
	wxBoxSizer* bSizer27;
	bSizer27 = new wxBoxSizer( wxVERTICAL );
	
	
	bSizer27->Add( 0, 15, 0, wxEXPAND, 5 );
	
	wxBoxSizer* bSizer37;
	bSizer37 = new wxBoxSizer( wxHORIZONTAL );
	
	bSizer27->Add( bSizer37, 0, wxALIGN_CENTER_HORIZONTAL, 5 );
	
	wxBoxSizer* bSizer42;
	bSizer42 = new wxBoxSizer( wxHORIZONTAL );
	
	
	bSizer42->Add( 0, 0, 1, wxEXPAND, 5 );
	
	m_bitmapStatus = new wxStaticBitmap( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxSize( 28,28 ), 0 );
	bSizer42->Add( m_bitmapStatus, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );
	
	m_staticTextStatus = new wxStaticText( this, wxID_ANY, _("Synchronizing..."), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextStatus->Wrap( -1 );
	m_staticTextStatus->SetFont( wxFont( 14, 70, 90, 92, false, wxEmptyString ) );
	
	bSizer42->Add( m_staticTextStatus, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );
	
	m_animationControl1 = new wxAnimationCtrl( this, wxID_ANY, wxNullAnimation, wxDefaultPosition, wxDefaultSize, wxAC_DEFAULT_STYLE ); 
	m_animationControl1->SetMinSize( wxSize( 45,45 ) );
	
	bSizer42->Add( m_animationControl1, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );
	
	
	bSizer42->Add( 0, 0, 1, wxEXPAND, 5 );
	
	bSizer27->Add( bSizer42, 0, wxEXPAND|wxALIGN_CENTER_HORIZONTAL, 5 );
	
	bSizer31 = new wxBoxSizer( wxHORIZONTAL );
	
	wxBoxSizer* bSizer111;
	bSizer111 = new wxBoxSizer( wxVERTICAL );
	
	bSizerObjectsRemaining = new wxBoxSizer( wxHORIZONTAL );
	
	m_staticText25 = new wxStaticText( this, wxID_ANY, _("Elements remaining:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText25->Wrap( -1 );
	m_staticText25->SetFont( wxFont( 10, 70, 90, 90, false, wxEmptyString ) );
	
	bSizerObjectsRemaining->Add( m_staticText25, 0, wxALIGN_BOTTOM, 5 );
	
	m_staticTextRemainingObj = new wxStaticText( this, wxID_ANY, _("dummy"), wxDefaultPosition, wxSize( -1,-1 ), 0 );
	m_staticTextRemainingObj->Wrap( -1 );
	m_staticTextRemainingObj->SetFont( wxFont( 9, 74, 90, 92, false, wxT("Arial") ) );
	
	bSizerObjectsRemaining->Add( m_staticTextRemainingObj, 0, wxLEFT|wxALIGN_BOTTOM, 5 );
	
	m_staticText96 = new wxStaticText( this, wxID_ANY, _("("), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText96->Wrap( -1 );
	m_staticText96->SetFont( wxFont( 9, 74, 90, 90, false, wxT("Arial") ) );
	
	bSizerObjectsRemaining->Add( m_staticText96, 0, wxLEFT|wxALIGN_BOTTOM, 5 );
	
	m_staticTextDataRemaining = new wxStaticText( this, wxID_ANY, _("dummy"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextDataRemaining->Wrap( -1 );
	m_staticTextDataRemaining->SetFont( wxFont( 9, 74, 90, 90, false, wxT("Arial") ) );
	
	bSizerObjectsRemaining->Add( m_staticTextDataRemaining, 0, wxALIGN_BOTTOM, 5 );
	
	m_staticText97 = new wxStaticText( this, wxID_ANY, _(")"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText97->Wrap( -1 );
	m_staticText97->SetFont( wxFont( 9, 74, 90, 90, false, wxT("Arial") ) );
	
	bSizerObjectsRemaining->Add( m_staticText97, 0, wxALIGN_BOTTOM, 5 );
	
	bSizer111->Add( bSizerObjectsRemaining, 0, 0, 5 );
	
	bSizerObjectsProcessed = new wxBoxSizer( wxHORIZONTAL );
	
	m_staticText251 = new wxStaticText( this, wxID_ANY, _("Elements processed:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText251->Wrap( -1 );
	m_staticText251->SetFont( wxFont( 10, 70, 90, 90, false, wxEmptyString ) );
	
	bSizerObjectsProcessed->Add( m_staticText251, 0, wxALIGN_BOTTOM, 5 );
	
	m_staticTextProcessedObj = new wxStaticText( this, wxID_ANY, _("dummy"), wxDefaultPosition, wxSize( -1,-1 ), 0 );
	m_staticTextProcessedObj->Wrap( -1 );
	m_staticTextProcessedObj->SetFont( wxFont( 9, 74, 90, 92, false, wxT("Arial") ) );
	
	bSizerObjectsProcessed->Add( m_staticTextProcessedObj, 0, wxLEFT|wxALIGN_BOTTOM, 5 );
	
	m_staticText98 = new wxStaticText( this, wxID_ANY, _("("), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText98->Wrap( -1 );
	m_staticText98->SetFont( wxFont( 9, 74, 90, 90, false, wxT("Arial") ) );
	
	bSizerObjectsProcessed->Add( m_staticText98, 0, wxLEFT|wxALIGN_BOTTOM, 5 );
	
	m_staticTextDataProcessed = new wxStaticText( this, wxID_ANY, _("dummy"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextDataProcessed->Wrap( -1 );
	m_staticTextDataProcessed->SetFont( wxFont( 9, 74, 90, 90, false, wxT("Arial") ) );
	
	bSizerObjectsProcessed->Add( m_staticTextDataProcessed, 0, wxALIGN_BOTTOM, 5 );
	
	m_staticText99 = new wxStaticText( this, wxID_ANY, _(")"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText99->Wrap( -1 );
	m_staticText99->SetFont( wxFont( 9, 74, 90, 90, false, wxT("Arial") ) );
	
	bSizerObjectsProcessed->Add( m_staticText99, 0, wxALIGN_BOTTOM, 5 );
	
	bSizer111->Add( bSizerObjectsProcessed, 0, 0, 5 );
	
	bSizer31->Add( bSizer111, 0, wxALIGN_CENTER_VERTICAL, 5 );
	
	
	bSizer31->Add( 0, 0, 1, wxEXPAND, 5 );
	
	wxBoxSizer* bSizer114;
	bSizer114 = new wxBoxSizer( wxHORIZONTAL );
	
	m_staticText55 = new wxStaticText( this, wxID_ANY, _("Time elapsed:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText55->Wrap( -1 );
	m_staticText55->SetFont( wxFont( 10, 70, 90, 90, false, wxEmptyString ) );
	
	bSizer114->Add( m_staticText55, 0, wxALIGN_BOTTOM, 5 );
	
	m_staticTextTimeElapsed = new wxStaticText( this, wxID_ANY, _("dummy"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextTimeElapsed->Wrap( -1 );
	m_staticTextTimeElapsed->SetFont( wxFont( 9, 74, 90, 92, false, wxT("Arial") ) );
	
	bSizer114->Add( m_staticTextTimeElapsed, 0, wxLEFT|wxALIGN_BOTTOM, 5 );
	
	bSizer31->Add( bSizer114, 0, wxALIGN_CENTER_VERTICAL, 5 );
	
	bSizer27->Add( bSizer31, 0, wxEXPAND|wxALIGN_CENTER_HORIZONTAL|wxBOTTOM|wxRIGHT|wxLEFT, 5 );
	
	bSizerProgressText = new wxBoxSizer( wxVERTICAL );
	
	m_textCtrlInfo = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_MULTILINE|wxTE_READONLY );
	m_textCtrlInfo->SetBackgroundColour( wxColour( 208, 208, 208 ) );
	
	bSizerProgressText->Add( m_textCtrlInfo, 3, wxEXPAND|wxALL, 5 );
	
	bSizer27->Add( bSizerProgressText, 1, wxEXPAND, 5 );
	
	m_gauge1 = new wxGauge( this, wxID_ANY, 100, wxDefaultPosition, wxSize( -1,20 ), wxGA_HORIZONTAL );
	bSizer27->Add( m_gauge1, 0, wxEXPAND|wxBOTTOM|wxRIGHT|wxLEFT, 5 );
	
	bSizer28 = new wxBoxSizer( wxHORIZONTAL );
	
	bSizerSpeed = new wxBoxSizer( wxHORIZONTAL );
	
	m_staticText108 = new wxStaticText( this, wxID_ANY, _("Speed:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText108->Wrap( -1 );
	m_staticText108->SetFont( wxFont( 10, 70, 90, 90, false, wxEmptyString ) );
	
	bSizerSpeed->Add( m_staticText108, 0, wxALIGN_BOTTOM, 5 );
	
	m_staticTextSpeed = new wxStaticText( this, wxID_ANY, _("dummy"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextSpeed->Wrap( -1 );
	m_staticTextSpeed->SetFont( wxFont( 9, 74, 90, 92, false, wxT("Arial") ) );
	
	bSizerSpeed->Add( m_staticTextSpeed, 0, wxLEFT|wxALIGN_BOTTOM, 5 );
	
	
	bSizerSpeed->Add( 0, 0, 1, wxALIGN_CENTER_VERTICAL, 5 );
	
	bSizer28->Add( bSizerSpeed, 1, wxALIGN_CENTER_VERTICAL, 5 );
	
	
	bSizer28->Add( 0, 0, 1, 0, 5 );
	
	m_buttonOK = new wxButton( this, wxID_OK, _("&OK"), wxDefaultPosition, wxSize( 100,32 ), 0 );
	m_buttonOK->SetFont( wxFont( 10, 70, 90, 90, false, wxEmptyString ) );
	m_buttonOK->Enable( false );
	m_buttonOK->Hide();
	
	bSizer28->Add( m_buttonOK, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );
	
	m_buttonPause = new wxButton( this, wxID_ANY, _("&Pause"), wxDefaultPosition, wxSize( 100,32 ), 0 );
	m_buttonPause->SetFont( wxFont( 10, 70, 90, 90, false, wxEmptyString ) );
	
	bSizer28->Add( m_buttonPause, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );
	
	m_buttonAbort = new wxButton( this, wxID_CANCEL, _("&Abort"), wxDefaultPosition, wxSize( 100,32 ), 0 );
	m_buttonAbort->SetFont( wxFont( 10, 70, 90, 90, false, wxEmptyString ) );
	
	bSizer28->Add( m_buttonAbort, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );
	
	
	bSizer28->Add( 0, 0, 1, 0, 5 );
	
	bSizerRemTime = new wxBoxSizer( wxHORIZONTAL );
	
	
	bSizerRemTime->Add( 0, 0, 1, wxALIGN_CENTER_VERTICAL, 5 );
	
	m_staticText21 = new wxStaticText( this, wxID_ANY, _("Time remaining:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText21->Wrap( -1 );
	m_staticText21->SetFont( wxFont( 10, 70, 90, 90, false, wxEmptyString ) );
	
	bSizerRemTime->Add( m_staticText21, 0, wxALIGN_BOTTOM, 5 );
	
	m_staticTextTimeRemaining = new wxStaticText( this, wxID_ANY, _("dummy"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextTimeRemaining->Wrap( -1 );
	m_staticTextTimeRemaining->SetFont( wxFont( 9, 74, 90, 92, false, wxT("Arial") ) );
	
	bSizerRemTime->Add( m_staticTextTimeRemaining, 0, wxLEFT|wxALIGN_BOTTOM, 5 );
	
	bSizer28->Add( bSizerRemTime, 1, wxALIGN_RIGHT|wxALIGN_CENTER_VERTICAL, 5 );
	
	bSizer27->Add( bSizer28, 0, wxALIGN_CENTER_HORIZONTAL|wxEXPAND|wxALL, 5 );
	
	
	bSizer27->Add( 0, 5, 0, wxEXPAND, 5 );
	
	this->SetSizer( bSizer27 );
	this->Layout();
	
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
	
	bSizer153->Add( bSizer154, 0, wxALIGN_CENTER_VERTICAL, 5 );
	
	m_textCtrlInfo = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_DONTWRAP|wxTE_MULTILINE|wxTE_READONLY );
	m_textCtrlInfo->SetBackgroundColour( wxColour( 208, 208, 208 ) );
	
	bSizer153->Add( m_textCtrlInfo, 1, wxEXPAND|wxALIGN_CENTER_VERTICAL|wxALL, 5 );
	
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

HelpDlgGenerated::HelpDlgGenerated( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : wxDialog( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxDefaultSize, wxDefaultSize );
	
	wxBoxSizer* bSizer20;
	bSizer20 = new wxBoxSizer( wxVERTICAL );
	
	
	bSizer20->Add( 0, 10, 0, wxEXPAND, 5 );
	
	wxBoxSizer* bSizer85;
	bSizer85 = new wxBoxSizer( wxHORIZONTAL );
	
	m_bitmap25 = new wxStaticBitmap( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxSize( 40,40 ), 0 );
	bSizer85->Add( m_bitmap25, 1, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );
	
	m_panel8 = new wxPanel( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxSIMPLE_BORDER|wxTAB_TRAVERSAL );
	m_panel8->SetBackgroundColour( wxSystemSettings::GetColour( wxSYS_COLOUR_3DLIGHT ) );
	
	wxBoxSizer* bSizer72;
	bSizer72 = new wxBoxSizer( wxHORIZONTAL );
	
	
	bSizer72->Add( 20, 0, 0, 0, 5 );
	
	m_staticText56 = new wxStaticText( m_panel8, wxID_ANY, _("Help"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText56->Wrap( -1 );
	m_staticText56->SetFont( wxFont( 16, 70, 90, 92, false, wxEmptyString ) );
	
	bSizer72->Add( m_staticText56, 0, wxALIGN_CENTER_HORIZONTAL|wxALL|wxALIGN_CENTER_VERTICAL, 5 );
	
	
	bSizer72->Add( 20, 0, 0, 0, 5 );
	
	m_panel8->SetSizer( bSizer72 );
	m_panel8->Layout();
	bSizer72->Fit( m_panel8 );
	bSizer85->Add( m_panel8, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxALL, 5 );
	
	
	bSizer85->Add( 0, 0, 1, wxALIGN_CENTER_VERTICAL, 5 );
	
	bSizer20->Add( bSizer85, 0, wxALIGN_CENTER_HORIZONTAL, 5 );
	
	m_notebook1 = new wxNotebook( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0 );
	m_scrolledWindow1 = new wxScrolledWindow( m_notebook1, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxHSCROLL|wxSIMPLE_BORDER|wxVSCROLL );
	m_scrolledWindow1->SetScrollRate( 5, 5 );
	m_scrolledWindow1->SetBackgroundColour( wxSystemSettings::GetColour( wxSYS_COLOUR_INACTIVEBORDER ) );
	
	wxBoxSizer* bSizer70;
	bSizer70 = new wxBoxSizer( wxVERTICAL );
	
	m_staticText59 = new wxStaticText( m_scrolledWindow1, wxID_ANY, _("Compare by \"File size and date\""), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText59->Wrap( 500 );
	m_staticText59->SetFont( wxFont( 10, 70, 90, 92, true, wxEmptyString ) );
	
	bSizer70->Add( m_staticText59, 0, wxALL|wxEXPAND|wxALIGN_CENTER_HORIZONTAL, 5 );
	
	m_staticText60 = new wxStaticText( m_scrolledWindow1, wxID_ANY, _("This variant evaluates two equally named files as being equal when they have the same file size AND the same last write date and time."), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText60->Wrap( 500 );
	bSizer70->Add( m_staticText60, 0, wxALL, 5 );
	
	m_staticText61 = new wxStaticText( m_scrolledWindow1, wxID_ANY, _("When the comparison is started with this option set the following decision tree is processed:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText61->Wrap( 500 );
	bSizer70->Add( m_staticText61, 0, wxALL, 5 );
	
	m_treeCtrl1 = new wxTreeCtrl( m_scrolledWindow1, wxID_ANY, wxDefaultPosition, wxSize( -1,175 ), wxTR_DEFAULT_STYLE );
	m_treeCtrl1->SetBackgroundColour( wxColour( 208, 208, 208 ) );
	
	bSizer70->Add( m_treeCtrl1, 0, wxALL|wxALIGN_CENTER_HORIZONTAL|wxEXPAND, 5 );
	
	m_staticText63 = new wxStaticText( m_scrolledWindow1, wxID_ANY, _("As a result the files are separated into the following categories:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText63->Wrap( 500 );
	bSizer70->Add( m_staticText63, 0, wxALL, 5 );
	
	m_staticText75 = new wxStaticText( m_scrolledWindow1, wxID_ANY, _("- equal"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText75->Wrap( -1 );
	bSizer70->Add( m_staticText75, 0, wxRIGHT|wxLEFT, 5 );
	
	m_staticText76 = new wxStaticText( m_scrolledWindow1, wxID_ANY, _("- left newer"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText76->Wrap( -1 );
	bSizer70->Add( m_staticText76, 0, wxRIGHT|wxLEFT, 5 );
	
	m_staticText77 = new wxStaticText( m_scrolledWindow1, wxID_ANY, _("- right newer"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText77->Wrap( -1 );
	bSizer70->Add( m_staticText77, 0, wxRIGHT|wxLEFT, 5 );
	
	m_staticText79 = new wxStaticText( m_scrolledWindow1, wxID_ANY, _("- exists left only"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText79->Wrap( -1 );
	bSizer70->Add( m_staticText79, 0, wxRIGHT|wxLEFT, 5 );
	
	m_staticText80 = new wxStaticText( m_scrolledWindow1, wxID_ANY, _("- exists right only"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText80->Wrap( -1 );
	bSizer70->Add( m_staticText80, 0, wxRIGHT|wxLEFT, 5 );
	
	m_staticText78 = new wxStaticText( m_scrolledWindow1, wxID_ANY, _("- conflict"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText78->Wrap( -1 );
	bSizer70->Add( m_staticText78, 0, wxRIGHT|wxLEFT, 5 );
	
	m_scrolledWindow1->SetSizer( bSizer70 );
	m_scrolledWindow1->Layout();
	bSizer70->Fit( m_scrolledWindow1 );
	m_notebook1->AddPage( m_scrolledWindow1, _("File size and date"), true );
	m_scrolledWindow5 = new wxScrolledWindow( m_notebook1, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxHSCROLL|wxSIMPLE_BORDER|wxVSCROLL );
	m_scrolledWindow5->SetScrollRate( 5, 5 );
	m_scrolledWindow5->SetBackgroundColour( wxSystemSettings::GetColour( wxSYS_COLOUR_INACTIVEBORDER ) );
	
	wxBoxSizer* bSizer74;
	bSizer74 = new wxBoxSizer( wxVERTICAL );
	
	m_staticText65 = new wxStaticText( m_scrolledWindow5, wxID_ANY, _("Compare by \"File content\""), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText65->Wrap( 500 );
	m_staticText65->SetFont( wxFont( 10, 70, 90, 92, true, wxEmptyString ) );
	
	bSizer74->Add( m_staticText65, 0, wxALL, 5 );
	
	m_staticText66 = new wxStaticText( m_scrolledWindow5, wxID_ANY, _("As the name suggests, two files which share the same name are marked as equal if and only if they have the same content. This option is useful for consistency checks rather than backup operations. Therefore the file times are not taken into account at all.\n\nWith this option enabled the decision tree is smaller:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText66->Wrap( 500 );
	bSizer74->Add( m_staticText66, 0, wxALL, 5 );
	
	m_treeCtrl2 = new wxTreeCtrl( m_scrolledWindow5, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTR_DEFAULT_STYLE );
	m_treeCtrl2->SetBackgroundColour( wxColour( 208, 208, 208 ) );
	m_treeCtrl2->SetMinSize( wxSize( -1,130 ) );
	
	bSizer74->Add( m_treeCtrl2, 0, wxALL|wxEXPAND, 5 );
	
	m_staticText69 = new wxStaticText( m_scrolledWindow5, wxID_ANY, _("As a result the files are separated into the following categories:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText69->Wrap( 500 );
	bSizer74->Add( m_staticText69, 0, wxALL, 5 );
	
	m_staticText81 = new wxStaticText( m_scrolledWindow5, wxID_ANY, _("- equal"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText81->Wrap( -1 );
	bSizer74->Add( m_staticText81, 0, wxRIGHT|wxLEFT, 5 );
	
	m_staticText82 = new wxStaticText( m_scrolledWindow5, wxID_ANY, _("- different"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText82->Wrap( -1 );
	bSizer74->Add( m_staticText82, 0, wxRIGHT|wxLEFT, 5 );
	
	m_staticText83 = new wxStaticText( m_scrolledWindow5, wxID_ANY, _("- exists left only"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText83->Wrap( -1 );
	bSizer74->Add( m_staticText83, 0, wxRIGHT|wxLEFT, 5 );
	
	m_staticText84 = new wxStaticText( m_scrolledWindow5, wxID_ANY, _("- exists right only"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText84->Wrap( -1 );
	bSizer74->Add( m_staticText84, 0, wxRIGHT|wxLEFT, 5 );
	
	m_scrolledWindow5->SetSizer( bSizer74 );
	m_scrolledWindow5->Layout();
	bSizer74->Fit( m_scrolledWindow5 );
	m_notebook1->AddPage( m_scrolledWindow5, _("File content"), false );
	
	bSizer20->Add( m_notebook1, 1, wxEXPAND | wxALL, 5 );
	
	m_button8 = new wxButton( this, wxID_OK, _("&OK"), wxDefaultPosition, wxSize( 100,32 ), 0 );
	m_button8->SetDefault(); 
	m_button8->SetFont( wxFont( 10, 70, 90, 90, false, wxEmptyString ) );
	
	bSizer20->Add( m_button8, 0, wxALL|wxALIGN_CENTER_HORIZONTAL, 5 );
	
	this->SetSizer( bSizer20 );
	this->Layout();
	
	// Connect Events
	this->Connect( wxEVT_CLOSE_WINDOW, wxCloseEventHandler( HelpDlgGenerated::OnClose ) );
	m_button8->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( HelpDlgGenerated::OnOK ), NULL, this );
}

HelpDlgGenerated::~HelpDlgGenerated()
{
	// Disconnect Events
	this->Disconnect( wxEVT_CLOSE_WINDOW, wxCloseEventHandler( HelpDlgGenerated::OnClose ) );
	m_button8->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( HelpDlgGenerated::OnOK ), NULL, this );
	
}

AboutDlgGenerated::AboutDlgGenerated( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : wxDialog( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxDefaultSize, wxDefaultSize );
	
	wxBoxSizer* bSizer31;
	bSizer31 = new wxBoxSizer( wxVERTICAL );
	
	
	bSizer31->Add( 0, 5, 0, 0, 5 );
	
	m_panel5 = new wxPanel( this, wxID_ANY, wxDefaultPosition, wxSize( -1,-1 ), wxSIMPLE_BORDER|wxTAB_TRAVERSAL );
	wxBoxSizer* bSizer36;
	bSizer36 = new wxBoxSizer( wxHORIZONTAL );
	
	m_bitmap11 = new wxStaticBitmap( m_panel5, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxSize( 404,55 ), 0 );
	bSizer36->Add( m_bitmap11, 0, wxALIGN_CENTER_VERTICAL, 5 );
	
	m_panel5->SetSizer( bSizer36 );
	m_panel5->Layout();
	bSizer36->Fit( m_panel5 );
	bSizer31->Add( m_panel5, 0, wxALL|wxALIGN_CENTER_HORIZONTAL, 5 );
	
	m_build = new wxStaticText( this, wxID_ANY, _("dummy"), wxDefaultPosition, wxDefaultSize, 0 );
	m_build->Wrap( -1 );
	m_build->SetFont( wxFont( 8, 70, 90, 90, false, wxEmptyString ) );
	
	bSizer31->Add( m_build, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxALL, 5 );
	
	
	bSizer31->Add( 0, 10, 0, 0, 5 );
	
	wxBoxSizer* bSizer53;
	bSizer53 = new wxBoxSizer( wxVERTICAL );
	
	m_scrolledWindowCodeInfo = new wxScrolledWindow( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxDOUBLE_BORDER|wxHSCROLL|wxVSCROLL );
	m_scrolledWindowCodeInfo->SetScrollRate( 5, 5 );
	m_scrolledWindowCodeInfo->SetBackgroundColour( wxColour( 208, 208, 208 ) );
	m_scrolledWindowCodeInfo->SetMinSize( wxSize( -1,120 ) );
	
	bSizerCodeInfo = new wxBoxSizer( wxVERTICAL );
	
	m_staticText72 = new wxStaticText( m_scrolledWindowCodeInfo, wxID_ANY, _("Source code written completely in C++ utilizing:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText72->Wrap( -1 );
	m_staticText72->SetFont( wxFont( wxNORMAL_FONT->GetPointSize(), 70, 90, 92, false, wxEmptyString ) );
	
	bSizerCodeInfo->Add( m_staticText72, 0, wxTOP|wxBOTTOM|wxLEFT|wxEXPAND, 5 );
	
	m_staticText73 = new wxStaticText( m_scrolledWindowCodeInfo, wxID_ANY, _("  MinGW \t- Windows port of GNU Compiler Collection\n  wxWidgets \t- Open-Source GUI framework\n  wxFormBuilder\t- wxWidgets GUI-builder\n  CodeBlocks \t- Open-Source IDE"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText73->Wrap( -1 );
	bSizerCodeInfo->Add( m_staticText73, 0, wxALL|wxEXPAND, 5 );
	
	m_hyperlink21 = new wxHyperlinkCtrl( m_scrolledWindowCodeInfo, wxID_ANY, _("- ZenJu -"), wxT("mailto:zhnmju123@gmx.de"), wxDefaultPosition, wxDefaultSize, wxHL_DEFAULT_STYLE );
	m_hyperlink21->SetFont( wxFont( 10, 74, 93, 92, false, wxT("Segoe Print") ) );
	m_hyperlink21->SetBackgroundColour( wxColour( 208, 208, 208 ) );
	m_hyperlink21->SetToolTip( _("zhnmju123@gmx.de") );
	
	bSizerCodeInfo->Add( m_hyperlink21, 0, wxALIGN_CENTER_HORIZONTAL, 5 );
	
	m_scrolledWindowCodeInfo->SetSizer( bSizerCodeInfo );
	m_scrolledWindowCodeInfo->Layout();
	bSizerCodeInfo->Fit( m_scrolledWindowCodeInfo );
	bSizer53->Add( m_scrolledWindowCodeInfo, 0, wxALIGN_CENTER_HORIZONTAL|wxEXPAND|wxBOTTOM, 10 );
	
	m_scrolledWindowTranslators = new wxScrolledWindow( this, wxID_ANY, wxDefaultPosition, wxSize( -1,-1 ), wxDOUBLE_BORDER|wxHSCROLL|wxVSCROLL );
	m_scrolledWindowTranslators->SetScrollRate( 5, 5 );
	m_scrolledWindowTranslators->SetBackgroundColour( wxColour( 208, 208, 208 ) );
	m_scrolledWindowTranslators->SetMinSize( wxSize( -1,140 ) );
	m_scrolledWindowTranslators->SetMaxSize( wxSize( -1,145 ) );
	
	bSizerTranslators = new wxBoxSizer( wxVERTICAL );
	
	m_staticText54 = new wxStaticText( m_scrolledWindowTranslators, wxID_ANY, _("Big thanks for localizing FreeFileSync goes out to:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText54->Wrap( -1 );
	m_staticText54->SetFont( wxFont( 8, 70, 90, 92, false, wxEmptyString ) );
	
	bSizerTranslators->Add( m_staticText54, 0, wxALIGN_CENTER_HORIZONTAL|wxTOP|wxBOTTOM, 5 );
	
	
	bSizerTranslators->Add( 0, 5, 0, 0, 5 );
	
	fgSizerTranslators = new wxFlexGridSizer( 50, 3, 5, 20 );
	fgSizerTranslators->SetFlexibleDirection( wxBOTH );
	fgSizerTranslators->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );
	
	bSizerTranslators->Add( fgSizerTranslators, 0, wxALIGN_CENTER_HORIZONTAL, 5 );
	
	m_scrolledWindowTranslators->SetSizer( bSizerTranslators );
	m_scrolledWindowTranslators->Layout();
	bSizerTranslators->Fit( m_scrolledWindowTranslators );
	bSizer53->Add( m_scrolledWindowTranslators, 0, wxALIGN_CENTER_HORIZONTAL|wxEXPAND|wxBOTTOM, 5 );
	
	bSizer31->Add( bSizer53, 0, wxALIGN_CENTER_HORIZONTAL|wxRIGHT|wxLEFT|wxEXPAND, 25 );
	
	m_staticline3 = new wxStaticLine( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	bSizer31->Add( m_staticline3, 0, wxEXPAND|wxTOP|wxBOTTOM, 5 );
	
	m_staticText131 = new wxStaticText( this, wxID_ANY, _("Feedback and suggestions are welcome at:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText131->Wrap( -1 );
	m_staticText131->SetFont( wxFont( 11, 70, 90, 92, false, wxEmptyString ) );
	
	bSizer31->Add( m_staticText131, 0, wxALIGN_CENTER_HORIZONTAL, 5 );
	
	m_staticline12 = new wxStaticLine( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	bSizer31->Add( m_staticline12, 0, wxEXPAND|wxTOP|wxBOTTOM, 5 );
	
	wxBoxSizer* bSizer156;
	bSizer156 = new wxBoxSizer( wxHORIZONTAL );
	
	m_bitmap9 = new wxStaticBitmap( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxSize( 48,48 ), 0 );
	m_bitmap9->SetToolTip( _("FreeFileSync at Sourceforge") );
	
	bSizer156->Add( m_bitmap9, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );
	
	m_hyperlink1 = new wxHyperlinkCtrl( this, wxID_ANY, _("Homepage"), wxT("http://sourceforge.net/projects/freefilesync/"), wxDefaultPosition, wxDefaultSize, wxHL_DEFAULT_STYLE );
	m_hyperlink1->SetFont( wxFont( 10, 70, 90, 92, true, wxEmptyString ) );
	m_hyperlink1->SetToolTip( _("http://sourceforge.net/projects/freefilesync/") );
	
	bSizer156->Add( m_hyperlink1, 0, wxALIGN_CENTER_VERTICAL, 5 );
	
	
	bSizer156->Add( 0, 0, 1, wxEXPAND, 5 );
	
	m_hyperlink3 = new wxHyperlinkCtrl( this, wxID_ANY, _("If you like FFS"), wxT("https://www.paypal.com/cgi-bin/webscr?cmd=_donations&business=zhnmju123%40gmx%2ede&no_shipping=0&no_note=1&tax=0&currency_code=EUR&lc=EN&bn=PP%2dDonationsBF&charset=UTF%2d8"), wxDefaultPosition, wxDefaultSize, wxHL_DEFAULT_STYLE );
	m_hyperlink3->SetFont( wxFont( 10, 70, 90, 92, true, wxEmptyString ) );
	m_hyperlink3->SetToolTip( _("https://www.paypal.com/cgi-bin/webscr?cmd=_donations&business=zhnmju123%40gmx%2ede&no_shipping=0&no_note=1&tax=0&currency_code=EUR&lc=EN&bn=PP%2dDonationsBF&charset=UTF%2d8") );
	
	bSizer156->Add( m_hyperlink3, 0, wxALIGN_CENTER_VERTICAL, 5 );
	
	m_animationControl1 = new wxAnimationCtrl( this, wxID_ANY, wxNullAnimation, wxDefaultPosition, wxSize( -1,-1 ), wxAC_DEFAULT_STYLE ); 
	m_animationControl1->SetToolTip( _("Donate with PayPal") );
	m_animationControl1->SetMinSize( wxSize( 48,48 ) );
	
	bSizer156->Add( m_animationControl1, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );
	
	bSizer31->Add( bSizer156, 0, wxALIGN_CENTER_VERTICAL|wxALIGN_CENTER_HORIZONTAL|wxEXPAND|wxRIGHT|wxLEFT, 10 );
	
	wxBoxSizer* bSizer158;
	bSizer158 = new wxBoxSizer( wxHORIZONTAL );
	
	m_bitmap10 = new wxStaticBitmap( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxSize( 48,48 ), 0 );
	m_bitmap10->SetToolTip( _("Email") );
	
	bSizer158->Add( m_bitmap10, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );
	
	m_hyperlink2 = new wxHyperlinkCtrl( this, wxID_ANY, _("Email"), wxT("mailto:zhnmju123@gmx.de"), wxDefaultPosition, wxDefaultSize, wxHL_DEFAULT_STYLE );
	m_hyperlink2->SetFont( wxFont( 10, 70, 90, 92, true, wxEmptyString ) );
	m_hyperlink2->SetToolTip( _("zhnmju123@gmx.de") );
	
	bSizer158->Add( m_hyperlink2, 0, wxALIGN_CENTER_VERTICAL, 5 );
	
	
	bSizer158->Add( 0, 0, 1, wxEXPAND, 5 );
	
	m_hyperlink6 = new wxHyperlinkCtrl( this, wxID_ANY, _("Report translation error"), wxT("http://sourceforge.net/projects/freefilesync/forums/forum/976976"), wxDefaultPosition, wxDefaultSize, wxHL_DEFAULT_STYLE );
	m_hyperlink6->SetFont( wxFont( 10, 70, 90, 92, true, wxEmptyString ) );
	m_hyperlink6->SetToolTip( _("http://sourceforge.net/projects/freefilesync/forums/forum/976976") );
	
	bSizer158->Add( m_hyperlink6, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 5 );
	
	m_bitmapTransl = new wxStaticBitmap( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxSize( 48,48 ), 0 );
	m_bitmapTransl->SetToolTip( _("Report translation error") );
	
	bSizer158->Add( m_bitmapTransl, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );
	
	bSizer31->Add( bSizer158, 0, wxALIGN_CENTER_VERTICAL|wxALIGN_CENTER_HORIZONTAL|wxEXPAND|wxRIGHT|wxLEFT, 10 );
	
	m_staticline2 = new wxStaticLine( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	bSizer31->Add( m_staticline2, 0, wxEXPAND|wxTOP|wxBOTTOM, 5 );
	
	wxStaticBoxSizer* sbSizer14;
	sbSizer14 = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Published under the GNU General Public License:") ), wxHORIZONTAL );
	
	
	sbSizer14->Add( 0, 0, 1, wxEXPAND|wxALIGN_CENTER_VERTICAL, 5 );
	
	m_bitmap13 = new wxStaticBitmap( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxSize( 88,31 ), 0 );
	sbSizer14->Add( m_bitmap13, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );
	
	m_hyperlink5 = new wxHyperlinkCtrl( this, wxID_ANY, _("http://www.gnu.org/licenses/gpl.html"), wxT("http://www.gnu.org/licenses/gpl.html"), wxDefaultPosition, wxDefaultSize, wxHL_DEFAULT_STYLE );
	sbSizer14->Add( m_hyperlink5, 0, wxALL|wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 5 );
	
	
	sbSizer14->Add( 0, 0, 1, wxEXPAND|wxALIGN_CENTER_VERTICAL, 5 );
	
	bSizer31->Add( sbSizer14, 0, wxALIGN_CENTER_HORIZONTAL|wxEXPAND|wxRIGHT|wxLEFT, 5 );
	
	m_buttonOkay = new wxButton( this, wxID_OK, _("&OK"), wxDefaultPosition, wxSize( 100,32 ), 0 );
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

ErrorDlgGenerated::ErrorDlgGenerated( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : wxDialog( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxDefaultSize, wxDefaultSize );
	
	wxBoxSizer* bSizer24;
	bSizer24 = new wxBoxSizer( wxVERTICAL );
	
	
	bSizer24->Add( 0, 10, 0, wxEXPAND, 5 );
	
	wxBoxSizer* bSizer26;
	bSizer26 = new wxBoxSizer( wxHORIZONTAL );
	
	m_bitmap10 = new wxStaticBitmap( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxSize( 48,48 ), 0 );
	bSizer26->Add( m_bitmap10, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );
	
	m_textCtrl8 = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize( -1,-1 ), wxTE_MULTILINE|wxTE_READONLY );
	m_textCtrl8->SetBackgroundColour( wxColour( 208, 208, 208 ) );
	
	bSizer26->Add( m_textCtrl8, 1, wxALIGN_CENTER_VERTICAL|wxEXPAND|wxRIGHT|wxLEFT, 5 );
	
	bSizer24->Add( bSizer26, 1, wxALIGN_CENTER_HORIZONTAL|wxEXPAND, 5 );
	
	m_checkBoxIgnoreErrors = new wxCheckBox( this, wxID_ANY, _("Ignore subsequent errors"), wxDefaultPosition, wxDefaultSize, 0 );
	m_checkBoxIgnoreErrors->SetToolTip( _("Hide further error messages during the current process") );
	
	bSizer24->Add( m_checkBoxIgnoreErrors, 0, wxALIGN_CENTER_HORIZONTAL|wxTOP, 10 );
	
	
	bSizer24->Add( 0, 5, 0, wxEXPAND, 5 );
	
	wxBoxSizer* bSizer25;
	bSizer25 = new wxBoxSizer( wxHORIZONTAL );
	
	m_buttonIgnore = new wxButton( this, wxID_OK, _("&Ignore"), wxDefaultPosition, wxSize( -1,30 ), 0 );
	m_buttonIgnore->SetDefault(); 
	m_buttonIgnore->SetFont( wxFont( 10, 70, 90, 90, false, wxEmptyString ) );
	
	bSizer25->Add( m_buttonIgnore, 0, wxTOP|wxBOTTOM|wxLEFT, 5 );
	
	m_buttonRetry = new wxButton( this, wxID_RETRY, _("&Retry"), wxDefaultPosition, wxSize( -1,30 ), 0 );
	m_buttonRetry->SetFont( wxFont( 10, 70, 90, 90, false, wxEmptyString ) );
	
	bSizer25->Add( m_buttonRetry, 0, wxTOP|wxBOTTOM|wxLEFT, 5 );
	
	m_buttonAbort = new wxButton( this, wxID_CANCEL, _("&Abort"), wxDefaultPosition, wxSize( -1,30 ), 0 );
	m_buttonAbort->SetFont( wxFont( 10, 70, 90, 90, false, wxEmptyString ) );
	
	bSizer25->Add( m_buttonAbort, 0, wxTOP|wxBOTTOM|wxLEFT, 5 );
	
	
	bSizer25->Add( 5, 0, 0, wxALIGN_CENTER_VERTICAL, 5 );
	
	bSizer24->Add( bSizer25, 0, wxALIGN_CENTER_HORIZONTAL, 5 );
	
	this->SetSizer( bSizer24 );
	this->Layout();
	
	// Connect Events
	this->Connect( wxEVT_CLOSE_WINDOW, wxCloseEventHandler( ErrorDlgGenerated::OnClose ) );
	m_buttonIgnore->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( ErrorDlgGenerated::OnIgnore ), NULL, this );
	m_buttonRetry->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( ErrorDlgGenerated::OnRetry ), NULL, this );
	m_buttonAbort->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( ErrorDlgGenerated::OnAbort ), NULL, this );
}

ErrorDlgGenerated::~ErrorDlgGenerated()
{
	// Disconnect Events
	this->Disconnect( wxEVT_CLOSE_WINDOW, wxCloseEventHandler( ErrorDlgGenerated::OnClose ) );
	m_buttonIgnore->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( ErrorDlgGenerated::OnIgnore ), NULL, this );
	m_buttonRetry->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( ErrorDlgGenerated::OnRetry ), NULL, this );
	m_buttonAbort->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( ErrorDlgGenerated::OnAbort ), NULL, this );
	
}

WarningDlgGenerated::WarningDlgGenerated( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : wxDialog( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxDefaultSize, wxDefaultSize );
	
	wxBoxSizer* bSizer24;
	bSizer24 = new wxBoxSizer( wxVERTICAL );
	
	
	bSizer24->Add( 0, 10, 0, wxEXPAND, 5 );
	
	wxBoxSizer* bSizer26;
	bSizer26 = new wxBoxSizer( wxHORIZONTAL );
	
	m_bitmap10 = new wxStaticBitmap( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxSize( 48,48 ), 0 );
	bSizer26->Add( m_bitmap10, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );
	
	m_textCtrl8 = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize( -1,-1 ), wxTE_MULTILINE|wxTE_READONLY );
	m_textCtrl8->SetBackgroundColour( wxColour( 208, 208, 208 ) );
	
	bSizer26->Add( m_textCtrl8, 1, wxALIGN_CENTER_VERTICAL|wxEXPAND|wxRIGHT|wxLEFT, 5 );
	
	bSizer24->Add( bSizer26, 1, wxALIGN_CENTER_HORIZONTAL|wxEXPAND, 5 );
	
	m_checkBoxDontShowAgain = new wxCheckBox( this, wxID_ANY, _("Do not show this dialog again"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizer24->Add( m_checkBoxDontShowAgain, 0, wxALIGN_CENTER_HORIZONTAL|wxTOP, 10 );
	
	
	bSizer24->Add( 0, 5, 0, wxEXPAND, 5 );
	
	wxBoxSizer* bSizer25;
	bSizer25 = new wxBoxSizer( wxHORIZONTAL );
	
	m_buttonIgnore = new wxButton( this, wxID_IGNORE, _("&Ignore"), wxDefaultPosition, wxSize( -1,30 ), 0 );
	m_buttonIgnore->SetDefault(); 
	m_buttonIgnore->SetFont( wxFont( 10, 70, 90, 90, false, wxEmptyString ) );
	
	bSizer25->Add( m_buttonIgnore, 0, wxTOP|wxBOTTOM|wxLEFT, 5 );
	
	m_buttonSwitch = new wxButton( this, wxID_MORE, _("&Switch"), wxDefaultPosition, wxSize( -1,30 ), 0 );
	m_buttonSwitch->SetFont( wxFont( 10, 70, 90, 90, false, wxEmptyString ) );
	
	bSizer25->Add( m_buttonSwitch, 0, wxTOP|wxBOTTOM|wxLEFT, 5 );
	
	m_buttonAbort = new wxButton( this, wxID_CANCEL, _("&Abort"), wxDefaultPosition, wxSize( -1,30 ), 0 );
	m_buttonAbort->SetFont( wxFont( 10, 70, 90, 90, false, wxEmptyString ) );
	
	bSizer25->Add( m_buttonAbort, 0, wxTOP|wxBOTTOM|wxLEFT, 5 );
	
	
	bSizer25->Add( 5, 0, 0, wxALIGN_CENTER_VERTICAL, 5 );
	
	bSizer24->Add( bSizer25, 0, wxALIGN_CENTER_HORIZONTAL, 5 );
	
	this->SetSizer( bSizer24 );
	this->Layout();
	
	// Connect Events
	this->Connect( wxEVT_CLOSE_WINDOW, wxCloseEventHandler( WarningDlgGenerated::OnClose ) );
	m_buttonIgnore->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( WarningDlgGenerated::OnIgnore ), NULL, this );
	m_buttonSwitch->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( WarningDlgGenerated::OnSwitch ), NULL, this );
	m_buttonAbort->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( WarningDlgGenerated::OnAbort ), NULL, this );
}

WarningDlgGenerated::~WarningDlgGenerated()
{
	// Disconnect Events
	this->Disconnect( wxEVT_CLOSE_WINDOW, wxCloseEventHandler( WarningDlgGenerated::OnClose ) );
	m_buttonIgnore->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( WarningDlgGenerated::OnIgnore ), NULL, this );
	m_buttonSwitch->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( WarningDlgGenerated::OnSwitch ), NULL, this );
	m_buttonAbort->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( WarningDlgGenerated::OnAbort ), NULL, this );
	
}

QuestionDlgGenerated::QuestionDlgGenerated( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : wxDialog( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxDefaultSize, wxDefaultSize );
	
	wxBoxSizer* bSizer24;
	bSizer24 = new wxBoxSizer( wxVERTICAL );
	
	
	bSizer24->Add( 0, 10, 0, wxEXPAND, 5 );
	
	wxBoxSizer* bSizer26;
	bSizer26 = new wxBoxSizer( wxHORIZONTAL );
	
	m_bitmap10 = new wxStaticBitmap( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxSize( 48,48 ), 0 );
	bSizer26->Add( m_bitmap10, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );
	
	m_textCtrl8 = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize( -1,-1 ), wxTE_MULTILINE|wxTE_READONLY );
	m_textCtrl8->SetBackgroundColour( wxColour( 208, 208, 208 ) );
	
	bSizer26->Add( m_textCtrl8, 1, wxALIGN_CENTER_VERTICAL|wxEXPAND|wxRIGHT|wxLEFT, 5 );
	
	bSizer24->Add( bSizer26, 1, wxALIGN_CENTER_HORIZONTAL|wxEXPAND, 5 );
	
	m_checkBoxDontAskAgain = new wxCheckBox( this, wxID_ANY, _("Do not show this dialog again"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizer24->Add( m_checkBoxDontAskAgain, 0, wxALIGN_CENTER_HORIZONTAL|wxTOP, 10 );
	
	
	bSizer24->Add( 0, 5, 0, wxEXPAND, 5 );
	
	wxBoxSizer* bSizer25;
	bSizer25 = new wxBoxSizer( wxHORIZONTAL );
	
	m_buttonYes = new wxButton( this, wxID_YES, _("&Yes"), wxDefaultPosition, wxSize( -1,30 ), 0 );
	m_buttonYes->SetDefault(); 
	m_buttonYes->SetFont( wxFont( 10, 70, 90, 90, false, wxEmptyString ) );
	
	bSizer25->Add( m_buttonYes, 0, wxTOP|wxBOTTOM|wxLEFT, 5 );
	
	m_buttonNo = new wxButton( this, wxID_NO, _("&No"), wxDefaultPosition, wxSize( -1,30 ), 0 );
	m_buttonNo->SetFont( wxFont( 10, 70, 90, 90, false, wxEmptyString ) );
	
	bSizer25->Add( m_buttonNo, 0, wxTOP|wxBOTTOM|wxLEFT, 5 );
	
	m_buttonCancel = new wxButton( this, wxID_CANCEL, _("&Cancel"), wxDefaultPosition, wxSize( -1,30 ), 0 );
	m_buttonCancel->SetFont( wxFont( 10, 70, 90, 90, false, wxEmptyString ) );
	
	bSizer25->Add( m_buttonCancel, 0, wxTOP|wxBOTTOM|wxLEFT, 5 );
	
	
	bSizer25->Add( 5, 0, 0, wxALIGN_CENTER_VERTICAL, 5 );
	
	bSizer24->Add( bSizer25, 0, wxALIGN_CENTER_HORIZONTAL, 5 );
	
	this->SetSizer( bSizer24 );
	this->Layout();
	
	// Connect Events
	this->Connect( wxEVT_CLOSE_WINDOW, wxCloseEventHandler( QuestionDlgGenerated::OnClose ) );
	m_buttonYes->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( QuestionDlgGenerated::OnYes ), NULL, this );
	m_buttonNo->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( QuestionDlgGenerated::OnNo ), NULL, this );
	m_buttonCancel->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( QuestionDlgGenerated::OnCancel ), NULL, this );
}

QuestionDlgGenerated::~QuestionDlgGenerated()
{
	// Disconnect Events
	this->Disconnect( wxEVT_CLOSE_WINDOW, wxCloseEventHandler( QuestionDlgGenerated::OnClose ) );
	m_buttonYes->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( QuestionDlgGenerated::OnYes ), NULL, this );
	m_buttonNo->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( QuestionDlgGenerated::OnNo ), NULL, this );
	m_buttonCancel->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( QuestionDlgGenerated::OnCancel ), NULL, this );
	
}

DeleteDlgGenerated::DeleteDlgGenerated( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : wxDialog( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxDefaultSize, wxDefaultSize );
	
	wxBoxSizer* bSizer24;
	bSizer24 = new wxBoxSizer( wxVERTICAL );
	
	
	bSizer24->Add( 0, 10, 0, wxEXPAND, 5 );
	
	wxBoxSizer* bSizer41;
	bSizer41 = new wxBoxSizer( wxHORIZONTAL );
	
	
	bSizer41->Add( 0, 0, 1, wxEXPAND, 5 );
	
	m_bitmap12 = new wxStaticBitmap( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxSize( 40,40 ), 0 );
	bSizer41->Add( m_bitmap12, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );
	
	m_staticTextHeader = new wxStaticText( this, wxID_ANY, _("dummy"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextHeader->Wrap( -1 );
	m_staticTextHeader->SetFont( wxFont( 10, 70, 90, 90, false, wxEmptyString ) );
	
	bSizer41->Add( m_staticTextHeader, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxALL, 5 );
	
	
	bSizer41->Add( 0, 0, 1, wxEXPAND, 5 );
	
	bSizer24->Add( bSizer41, 0, wxEXPAND, 5 );
	
	wxBoxSizer* bSizer99;
	bSizer99 = new wxBoxSizer( wxHORIZONTAL );
	
	m_checkBoxDeleteBothSides = new wxCheckBox( this, wxID_ANY, _("Delete on both sides"), wxDefaultPosition, wxDefaultSize, 0 );
	m_checkBoxDeleteBothSides->SetToolTip( _("Delete on both sides even if the file is selected on one side only") );
	
	bSizer99->Add( m_checkBoxDeleteBothSides, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );
	
	
	bSizer99->Add( 0, 0, 1, wxEXPAND|wxALIGN_CENTER_VERTICAL, 5 );
	
	m_checkBoxUseRecycler = new wxCheckBox( this, wxID_ANY, _("Use Recycle Bin"), wxDefaultPosition, wxDefaultSize, 0 );
	m_checkBoxUseRecycler->SetValue(true); 
	bSizer99->Add( m_checkBoxUseRecycler, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );
	
	bSizer24->Add( bSizer99, 0, wxEXPAND|wxALIGN_CENTER_HORIZONTAL|wxRIGHT|wxLEFT, 10 );
	
	m_textCtrlMessage = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize( -1,-1 ), wxTE_MULTILINE|wxTE_READONLY );
	m_textCtrlMessage->SetBackgroundColour( wxColour( 208, 208, 208 ) );
	
	bSizer24->Add( m_textCtrlMessage, 1, wxALL|wxALIGN_CENTER_VERTICAL|wxEXPAND, 5 );
	
	wxBoxSizer* bSizer25;
	bSizer25 = new wxBoxSizer( wxHORIZONTAL );
	
	m_buttonOK = new wxButton( this, wxID_OK, _("&OK"), wxDefaultPosition, wxSize( -1,30 ), 0 );
	m_buttonOK->SetDefault(); 
	m_buttonOK->SetFont( wxFont( 10, 70, 90, 92, false, wxEmptyString ) );
	
	bSizer25->Add( m_buttonOK, 0, wxTOP|wxBOTTOM|wxLEFT, 5 );
	
	m_buttonCancel = new wxButton( this, wxID_CANCEL, _("&Cancel"), wxDefaultPosition, wxSize( -1,30 ), 0 );
	m_buttonCancel->SetFont( wxFont( 10, 70, 90, 90, false, wxEmptyString ) );
	
	bSizer25->Add( m_buttonCancel, 0, wxALL, 5 );
	
	bSizer24->Add( bSizer25, 0, wxALIGN_CENTER_HORIZONTAL, 5 );
	
	this->SetSizer( bSizer24 );
	this->Layout();
	
	// Connect Events
	this->Connect( wxEVT_CLOSE_WINDOW, wxCloseEventHandler( DeleteDlgGenerated::OnClose ) );
	m_checkBoxDeleteBothSides->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DeleteDlgGenerated::OnDelOnBothSides ), NULL, this );
	m_checkBoxUseRecycler->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DeleteDlgGenerated::OnUseRecycler ), NULL, this );
	m_buttonOK->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DeleteDlgGenerated::OnOK ), NULL, this );
	m_buttonCancel->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DeleteDlgGenerated::OnCancel ), NULL, this );
}

DeleteDlgGenerated::~DeleteDlgGenerated()
{
	// Disconnect Events
	this->Disconnect( wxEVT_CLOSE_WINDOW, wxCloseEventHandler( DeleteDlgGenerated::OnClose ) );
	m_checkBoxDeleteBothSides->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DeleteDlgGenerated::OnDelOnBothSides ), NULL, this );
	m_checkBoxUseRecycler->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DeleteDlgGenerated::OnUseRecycler ), NULL, this );
	m_buttonOK->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DeleteDlgGenerated::OnOK ), NULL, this );
	m_buttonCancel->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DeleteDlgGenerated::OnCancel ), NULL, this );
	
}

FilterDlgGenerated::FilterDlgGenerated( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : wxDialog( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxDefaultSize, wxDefaultSize );
	
	wxBoxSizer* bSizer21;
	bSizer21 = new wxBoxSizer( wxVERTICAL );
	
	wxBoxSizer* bSizer86;
	bSizer86 = new wxBoxSizer( wxHORIZONTAL );
	
	m_bitmap26 = new wxStaticBitmap( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxSize( 40,40 ), 0 );
	bSizer86->Add( m_bitmap26, 1, wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );
	
	m_panel8 = new wxPanel( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxSIMPLE_BORDER|wxTAB_TRAVERSAL );
	m_panel8->SetBackgroundColour( wxSystemSettings::GetColour( wxSYS_COLOUR_3DLIGHT ) );
	
	wxBoxSizer* bSizer72;
	bSizer72 = new wxBoxSizer( wxVERTICAL );
	
	m_staticTexHeader = new wxStaticText( m_panel8, wxID_ANY, _("dummy"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTexHeader->Wrap( -1 );
	m_staticTexHeader->SetFont( wxFont( 16, 70, 90, 92, false, wxEmptyString ) );
	
	bSizer72->Add( m_staticTexHeader, 0, wxALIGN_CENTER_HORIZONTAL|wxALL, 5 );
	
	m_panel8->SetSizer( bSizer72 );
	m_panel8->Layout();
	bSizer72->Fit( m_panel8 );
	bSizer86->Add( m_panel8, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxALL, 5 );
	
	
	bSizer86->Add( 0, 0, 1, wxALIGN_CENTER_VERTICAL, 5 );
	
	bSizer21->Add( bSizer86, 0, wxALIGN_CENTER_HORIZONTAL|wxBOTTOM, 5 );
	
	wxBoxSizer* bSizer70;
	bSizer70 = new wxBoxSizer( wxHORIZONTAL );
	
	m_staticText44 = new wxStaticText( this, wxID_ANY, _("Only files/directories that pass filtering will be selected for synchronization. The filter will be applied to the name relative(!) to the base synchronization directories."), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText44->Wrap( 400 );
	bSizer70->Add( m_staticText44, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 5 );
	
	m_bpButtonHelp = new wxBitmapButton( this, wxID_HELP, wxNullBitmap, wxDefaultPosition, wxSize( 40,40 ), wxBU_AUTODRAW );
	m_bpButtonHelp->SetToolTip( _("Help") );
	
	bSizer70->Add( m_bpButtonHelp, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );
	
	bSizer21->Add( bSizer70, 0, wxALIGN_CENTER_HORIZONTAL|wxRIGHT|wxLEFT, 10 );
	
	
	bSizer21->Add( 0, 5, 0, 0, 5 );
	
	m_panel13 = new wxPanel( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	wxBoxSizer* bSizer69;
	bSizer69 = new wxBoxSizer( wxVERTICAL );
	
	m_staticline10 = new wxStaticLine( m_panel13, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	bSizer69->Add( m_staticline10, 0, wxEXPAND|wxTOP|wxBOTTOM, 5 );
	
	wxBoxSizer* bSizer52;
	bSizer52 = new wxBoxSizer( wxVERTICAL );
	
	m_staticText45 = new wxStaticText( m_panel13, wxID_ANY, _("Hints:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText45->Wrap( -1 );
	m_staticText45->SetFont( wxFont( 10, 70, 90, 92, true, wxEmptyString ) );
	
	bSizer52->Add( m_staticText45, 0, wxBOTTOM, 5 );
	
	m_staticText83 = new wxStaticText( m_panel13, wxID_ANY, _("1. Enter relative file or directory names separated by ';' or a new line."), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText83->Wrap( -1 );
	bSizer52->Add( m_staticText83, 0, 0, 5 );
	
	m_staticText84 = new wxStaticText( m_panel13, wxID_ANY, _("2. Use wildcard characters '*' and '?'."), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText84->Wrap( -1 );
	bSizer52->Add( m_staticText84, 0, 0, 5 );
	
	m_staticText85 = new wxStaticText( m_panel13, wxID_ANY, _("3. Exclude files directly on main grid via context menu."), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText85->Wrap( -1 );
	bSizer52->Add( m_staticText85, 0, 0, 5 );
	
	bSizer69->Add( bSizer52, 0, wxALIGN_CENTER_HORIZONTAL|wxRIGHT|wxLEFT, 10 );
	
	wxStaticBoxSizer* sbSizer21;
	sbSizer21 = new wxStaticBoxSizer( new wxStaticBox( m_panel13, wxID_ANY, _("Example") ), wxVERTICAL );
	
	wxBoxSizer* bSizer66;
	bSizer66 = new wxBoxSizer( wxHORIZONTAL );
	
	m_staticText181 = new wxStaticText( m_panel13, wxID_ANY, _("Include: *.doc;*.zip;*.exe\nExclude: \\stuff\\temp\\*"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText181->Wrap( -1 );
	bSizer66->Add( m_staticText181, 0, wxRIGHT|wxALIGN_CENTER_VERTICAL, 5 );
	
	m_staticText1811 = new wxStaticText( m_panel13, wxID_ANY, _("Synchronize all .doc, .zip and .exe files except everything in subfolder \"temp\"."), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText1811->Wrap( 250 );
	m_staticText1811->SetFont( wxFont( 8, 70, 93, 90, false, wxEmptyString ) );
	
	bSizer66->Add( m_staticText1811, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );
	
	sbSizer21->Add( bSizer66, 0, wxALIGN_CENTER_HORIZONTAL|wxRIGHT|wxLEFT, 5 );
	
	bSizer69->Add( sbSizer21, 0, wxALIGN_CENTER_HORIZONTAL|wxTOP|wxRIGHT|wxLEFT, 5 );
	
	m_panel13->SetSizer( bSizer69 );
	m_panel13->Layout();
	bSizer69->Fit( m_panel13 );
	bSizer21->Add( m_panel13, 0, wxALIGN_CENTER_HORIZONTAL|wxBOTTOM|wxEXPAND, 5 );
	
	wxStaticBoxSizer* sbSizer8;
	sbSizer8 = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, wxEmptyString ), wxVERTICAL );
	
	wxFlexGridSizer* fgSizer3;
	fgSizer3 = new wxFlexGridSizer( 2, 2, 0, 0 );
	fgSizer3->AddGrowableCol( 1 );
	fgSizer3->AddGrowableRow( 1 );
	fgSizer3->SetFlexibleDirection( wxBOTH );
	fgSizer3->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );
	
	
	fgSizer3->Add( 0, 0, 1, wxEXPAND, 5 );
	
	m_staticText15 = new wxStaticText( this, wxID_ANY, _("Include"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText15->Wrap( -1 );
	m_staticText15->SetFont( wxFont( 10, 70, 90, 92, false, wxEmptyString ) );
	
	fgSizer3->Add( m_staticText15, 0, wxTOP|wxRIGHT|wxLEFT, 5 );
	
	m_bitmap8 = new wxStaticBitmap( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxSize( 30,30 ), 0 );
	fgSizer3->Add( m_bitmap8, 0, wxTOP|wxBOTTOM|wxLEFT|wxALIGN_CENTER_VERTICAL|wxALIGN_CENTER_HORIZONTAL, 5 );
	
	m_textCtrlInclude = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize( -1,-1 ), wxTE_MULTILINE );
	fgSizer3->Add( m_textCtrlInclude, 1, wxALL|wxALIGN_CENTER_VERTICAL|wxEXPAND, 5 );
	
	sbSizer8->Add( fgSizer3, 1, wxEXPAND, 5 );
	
	wxFlexGridSizer* fgSizer4;
	fgSizer4 = new wxFlexGridSizer( 2, 2, 0, 0 );
	fgSizer4->AddGrowableCol( 1 );
	fgSizer4->AddGrowableRow( 1 );
	fgSizer4->SetFlexibleDirection( wxBOTH );
	fgSizer4->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );
	
	
	fgSizer4->Add( 0, 0, 1, wxEXPAND, 5 );
	
	m_staticText16 = new wxStaticText( this, wxID_ANY, _("Exclude"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText16->Wrap( -1 );
	m_staticText16->SetFont( wxFont( 10, 70, 90, 92, false, wxEmptyString ) );
	
	fgSizer4->Add( m_staticText16, 0, wxTOP|wxRIGHT|wxLEFT, 5 );
	
	m_bitmap9 = new wxStaticBitmap( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxSize( 30,30 ), 0 );
	fgSizer4->Add( m_bitmap9, 0, wxTOP|wxBOTTOM|wxLEFT|wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 5 );
	
	m_textCtrlExclude = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize( -1,-1 ), wxTE_MULTILINE );
	fgSizer4->Add( m_textCtrlExclude, 1, wxALL|wxALIGN_CENTER_VERTICAL|wxEXPAND, 5 );
	
	sbSizer8->Add( fgSizer4, 1, wxEXPAND, 5 );
	
	bSizer21->Add( sbSizer8, 1, wxALIGN_CENTER_HORIZONTAL|wxRIGHT|wxLEFT|wxEXPAND, 5 );
	
	
	bSizer21->Add( 0, 0, 0, 0, 5 );
	
	wxBoxSizer* bSizer22;
	bSizer22 = new wxBoxSizer( wxHORIZONTAL );
	
	m_button9 = new wxButton( this, wxID_DEFAULT, _("&Default"), wxDefaultPosition, wxSize( -1,30 ), 0 );
	m_button9->SetFont( wxFont( 10, 70, 90, 90, false, wxEmptyString ) );
	
	bSizer22->Add( m_button9, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );
	
	
	bSizer22->Add( 0, 0, 1, wxALIGN_CENTER_VERTICAL, 5 );
	
	m_button10 = new wxButton( this, wxID_OK, _("&OK"), wxDefaultPosition, wxSize( -1,30 ), 0 );
	m_button10->SetDefault(); 
	m_button10->SetFont( wxFont( 10, 70, 90, 92, false, wxEmptyString ) );
	
	bSizer22->Add( m_button10, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );
	
	m_button17 = new wxButton( this, wxID_CANCEL, _("&Cancel"), wxDefaultPosition, wxSize( -1,30 ), 0 );
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
	m_button9->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( FilterDlgGenerated::OnDefault ), NULL, this );
	m_button10->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( FilterDlgGenerated::OnApply ), NULL, this );
	m_button17->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( FilterDlgGenerated::OnCancel ), NULL, this );
}

FilterDlgGenerated::~FilterDlgGenerated()
{
	// Disconnect Events
	this->Disconnect( wxEVT_CLOSE_WINDOW, wxCloseEventHandler( FilterDlgGenerated::OnClose ) );
	m_bpButtonHelp->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( FilterDlgGenerated::OnHelp ), NULL, this );
	m_button9->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( FilterDlgGenerated::OnDefault ), NULL, this );
	m_button10->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( FilterDlgGenerated::OnApply ), NULL, this );
	m_button17->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( FilterDlgGenerated::OnCancel ), NULL, this );
	
}

CustomizeColsDlgGenerated::CustomizeColsDlgGenerated( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : wxDialog( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxDefaultSize, wxDefaultSize );
	
	wxBoxSizer* bSizer96;
	bSizer96 = new wxBoxSizer( wxVERTICAL );
	
	wxBoxSizer* bSizer99;
	bSizer99 = new wxBoxSizer( wxHORIZONTAL );
	
	wxArrayString m_checkListColumnsChoices;
	m_checkListColumns = new wxCheckListBox( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, m_checkListColumnsChoices, 0 );
	bSizer99->Add( m_checkListColumns, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );
	
	wxBoxSizer* bSizer98;
	bSizer98 = new wxBoxSizer( wxVERTICAL );
	
	m_bpButton29 = new wxBitmapButton( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxSize( 40,40 ), wxBU_AUTODRAW );
	m_bpButton29->SetToolTip( _("Move column up") );
	
	bSizer98->Add( m_bpButton29, 0, wxTOP|wxRIGHT|wxLEFT, 5 );
	
	m_bpButton30 = new wxBitmapButton( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxSize( 40,40 ), wxBU_AUTODRAW );
	m_bpButton30->SetToolTip( _("Move column down") );
	
	bSizer98->Add( m_bpButton30, 0, wxBOTTOM|wxRIGHT|wxLEFT, 5 );
	
	bSizer99->Add( bSizer98, 0, wxALIGN_CENTER_VERTICAL, 5 );
	
	bSizer96->Add( bSizer99, 0, wxALIGN_CENTER_HORIZONTAL, 5 );
	
	wxBoxSizer* bSizer97;
	bSizer97 = new wxBoxSizer( wxHORIZONTAL );
	
	m_button9 = new wxButton( this, wxID_DEFAULT, _("&Default"), wxDefaultPosition, wxSize( -1,30 ), 0 );
	m_button9->SetFont( wxFont( 10, 70, 90, 90, false, wxEmptyString ) );
	
	bSizer97->Add( m_button9, 0, wxALL, 5 );
	
	
	bSizer97->Add( 0, 0, 1, wxALIGN_CENTER_VERTICAL, 5 );
	
	m_button28 = new wxButton( this, wxID_OK, _("&OK"), wxDefaultPosition, wxSize( -1,30 ), 0 );
	m_button28->SetDefault(); 
	m_button28->SetFont( wxFont( 10, 70, 90, 92, false, wxEmptyString ) );
	
	bSizer97->Add( m_button28, 0, wxALL, 5 );
	
	m_button29 = new wxButton( this, wxID_CANCEL, _("&Cancel"), wxDefaultPosition, wxSize( -1,30 ), 0 );
	m_button29->SetFont( wxFont( 10, 70, 90, 90, false, wxEmptyString ) );
	
	bSizer97->Add( m_button29, 0, wxALL, 5 );
	
	bSizer96->Add( bSizer97, 0, wxBOTTOM|wxRIGHT|wxLEFT|wxEXPAND, 5 );
	
	this->SetSizer( bSizer96 );
	this->Layout();
	bSizer96->Fit( this );
	
	// Connect Events
	this->Connect( wxEVT_CLOSE_WINDOW, wxCloseEventHandler( CustomizeColsDlgGenerated::OnClose ) );
	m_bpButton29->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( CustomizeColsDlgGenerated::OnMoveUp ), NULL, this );
	m_bpButton30->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( CustomizeColsDlgGenerated::OnMoveDown ), NULL, this );
	m_button9->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( CustomizeColsDlgGenerated::OnDefault ), NULL, this );
	m_button28->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( CustomizeColsDlgGenerated::OnOkay ), NULL, this );
	m_button29->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( CustomizeColsDlgGenerated::OnCancel ), NULL, this );
}

CustomizeColsDlgGenerated::~CustomizeColsDlgGenerated()
{
	// Disconnect Events
	this->Disconnect( wxEVT_CLOSE_WINDOW, wxCloseEventHandler( CustomizeColsDlgGenerated::OnClose ) );
	m_bpButton29->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( CustomizeColsDlgGenerated::OnMoveUp ), NULL, this );
	m_bpButton30->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( CustomizeColsDlgGenerated::OnMoveDown ), NULL, this );
	m_button9->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( CustomizeColsDlgGenerated::OnDefault ), NULL, this );
	m_button28->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( CustomizeColsDlgGenerated::OnOkay ), NULL, this );
	m_button29->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( CustomizeColsDlgGenerated::OnCancel ), NULL, this );
	
}

GlobalSettingsDlgGenerated::GlobalSettingsDlgGenerated( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : wxDialog( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxSize( 280,230 ), wxDefaultSize );
	
	wxBoxSizer* bSizer95;
	bSizer95 = new wxBoxSizer( wxVERTICAL );
	
	wxBoxSizer* bSizer86;
	bSizer86 = new wxBoxSizer( wxHORIZONTAL );
	
	m_bitmapSettings = new wxStaticBitmap( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxSize( 40,40 ), 0 );
	bSizer86->Add( m_bitmapSettings, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );
	
	
	bSizer86->Add( 0, 0, 1, wxALIGN_CENTER_VERTICAL, 5 );
	
	m_panel8 = new wxPanel( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxSIMPLE_BORDER|wxTAB_TRAVERSAL );
	m_panel8->SetBackgroundColour( wxSystemSettings::GetColour( wxSYS_COLOUR_3DLIGHT ) );
	
	wxBoxSizer* bSizer72;
	bSizer72 = new wxBoxSizer( wxVERTICAL );
	
	m_staticText56 = new wxStaticText( m_panel8, wxID_ANY, _("Global settings"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText56->Wrap( -1 );
	m_staticText56->SetFont( wxFont( 16, 70, 90, 92, false, wxEmptyString ) );
	
	bSizer72->Add( m_staticText56, 0, wxALIGN_CENTER_HORIZONTAL|wxALL, 5 );
	
	m_panel8->SetSizer( bSizer72 );
	m_panel8->Layout();
	bSizer72->Fit( m_panel8 );
	bSizer86->Add( m_panel8, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxALL, 5 );
	
	bSizer95->Add( bSizer86, 0, wxALIGN_CENTER_HORIZONTAL, 5 );
	
	
	bSizer95->Add( 0, 10, 0, 0, 5 );
	
	wxStaticBoxSizer* sbSizer23;
	sbSizer23 = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, wxEmptyString ), wxVERTICAL );
	
	m_checkBoxCopyLocked = new wxCheckBox( this, wxID_ANY, _("Copy locked files"), wxDefaultPosition, wxDefaultSize, 0 );
	m_checkBoxCopyLocked->SetToolTip( _("Copy shared or locked files using Volume Shadow Copy Service\n(Requires Administrator rights)") );
	
	sbSizer23->Add( m_checkBoxCopyLocked, 0, wxALL|wxEXPAND, 5 );
	
	m_checkBoxCopyPermissions = new wxCheckBox( this, wxID_ANY, _("Copy filesystem permissions"), wxDefaultPosition, wxDefaultSize, 0 );
	m_checkBoxCopyPermissions->SetToolTip( _("Transfer file and directory permissions\n(Requires Administrator rights)") );
	
	sbSizer23->Add( m_checkBoxCopyPermissions, 0, wxALL|wxEXPAND, 5 );
	
	m_staticline10 = new wxStaticLine( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	sbSizer23->Add( m_staticline10, 0, wxEXPAND|wxALIGN_CENTER_HORIZONTAL|wxTOP|wxBOTTOM, 5 );
	
	wxBoxSizer* bSizer101;
	bSizer101 = new wxBoxSizer( wxHORIZONTAL );
	
	m_staticText100 = new wxStaticText( this, wxID_ANY, _("Hidden dialogs:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText100->Wrap( -1 );
	bSizer101->Add( m_staticText100, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );
	
	
	bSizer101->Add( 0, 0, 1, wxEXPAND, 5 );
	
	m_buttonResetDialogs = new wxButtonWithImage( this, wxID_ANY, _("Reset"), wxDefaultPosition, wxSize( 80,-1 ), 0 );
	m_buttonResetDialogs->SetFont( wxFont( 10, 70, 90, 90, false, wxEmptyString ) );
	m_buttonResetDialogs->SetToolTip( _("Show hidden dialogs") );
	
	bSizer101->Add( m_buttonResetDialogs, 0, wxALIGN_CENTER_HORIZONTAL|wxRIGHT|wxLEFT|wxALIGN_CENTER_VERTICAL, 5 );
	
	sbSizer23->Add( bSizer101, 0, wxEXPAND, 5 );
	
	bSizer95->Add( sbSizer23, 0, wxALIGN_CENTER_HORIZONTAL|wxRIGHT|wxLEFT|wxEXPAND, 5 );
	
	
	bSizer95->Add( 0, 10, 0, 0, 5 );
	
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
	m_gridCustomCommand->SetColSize( 0, 129 );
	m_gridCustomCommand->SetColSize( 1, 179 );
	m_gridCustomCommand->EnableDragColMove( false );
	m_gridCustomCommand->EnableDragColSize( true );
	m_gridCustomCommand->SetColLabelSize( 20 );
	m_gridCustomCommand->SetColLabelValue( 0, _("Description") );
	m_gridCustomCommand->SetColLabelValue( 1, _("Command line") );
	m_gridCustomCommand->SetColLabelAlignment( wxALIGN_CENTRE, wxALIGN_CENTRE );
	
	// Rows
	m_gridCustomCommand->EnableDragRowSize( false );
	m_gridCustomCommand->SetRowLabelSize( 0 );
	m_gridCustomCommand->SetRowLabelAlignment( wxALIGN_CENTRE, wxALIGN_CENTRE );
	
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
	
	bSizer95->Add( sbSizer26, 1, wxALIGN_CENTER_HORIZONTAL|wxRIGHT|wxLEFT|wxEXPAND, 5 );
	
	wxBoxSizer* bSizer97;
	bSizer97 = new wxBoxSizer( wxHORIZONTAL );
	
	m_button9 = new wxButton( this, wxID_DEFAULT, _("&Default"), wxDefaultPosition, wxSize( -1,30 ), 0 );
	m_button9->SetFont( wxFont( 10, 70, 90, 90, false, wxEmptyString ) );
	
	bSizer97->Add( m_button9, 0, wxALL, 5 );
	
	
	bSizer97->Add( 0, 0, 1, 0, 5 );
	
	m_buttonOkay = new wxButton( this, wxID_OK, _("&OK"), wxDefaultPosition, wxSize( -1,30 ), 0 );
	m_buttonOkay->SetDefault(); 
	m_buttonOkay->SetFont( wxFont( 10, 70, 90, 92, false, wxEmptyString ) );
	
	bSizer97->Add( m_buttonOkay, 0, wxALL, 5 );
	
	m_button29 = new wxButton( this, wxID_CANCEL, _("&Cancel"), wxDefaultPosition, wxSize( -1,30 ), 0 );
	m_button29->SetFont( wxFont( 10, 70, 90, 90, false, wxEmptyString ) );
	
	bSizer97->Add( m_button29, 0, wxALL, 5 );
	
	bSizer95->Add( bSizer97, 0, wxALIGN_CENTER_HORIZONTAL|wxEXPAND, 5 );
	
	this->SetSizer( bSizer95 );
	this->Layout();
	bSizer95->Fit( this );
	
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
	
	m_buttonStartSync = new wxButtonWithImage( this, wxID_ANY, _("Start"), wxDefaultPosition, wxSize( -1,40 ), 0 );
	m_buttonStartSync->SetDefault(); 
	m_buttonStartSync->SetFont( wxFont( 14, 70, 90, 92, false, wxT("Arial Black") ) );
	m_buttonStartSync->SetToolTip( _("Start synchronization") );
	
	bSizer158->Add( m_buttonStartSync, 0, wxALL|wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 5 );
	
	m_staticline16 = new wxStaticLine( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_VERTICAL );
	bSizer158->Add( m_staticline16, 0, wxEXPAND|wxRIGHT|wxLEFT, 5 );
	
	wxStaticBoxSizer* sbSizer28;
	sbSizer28 = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Variant") ), wxVERTICAL );
	
	m_staticTextVariant = new wxStaticText( this, wxID_ANY, _("dummy"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextVariant->Wrap( -1 );
	m_staticTextVariant->SetFont( wxFont( 10, 70, 90, 92, false, wxT("Arial Black") ) );
	
	sbSizer28->Add( m_staticTextVariant, 0, wxALL|wxALIGN_CENTER_VERTICAL|wxALIGN_CENTER_HORIZONTAL, 5 );
	
	bSizer158->Add( sbSizer28, 1, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxBOTTOM|wxRIGHT|wxLEFT, 5 );
	
	bSizer134->Add( bSizer158, 0, wxALIGN_CENTER_HORIZONTAL|wxEXPAND, 5 );
	
	m_staticline14 = new wxStaticLine( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	bSizer134->Add( m_staticline14, 0, wxEXPAND, 5 );
	
	wxBoxSizer* bSizer141;
	bSizer141 = new wxBoxSizer( wxHORIZONTAL );
	
	wxStaticBoxSizer* sbSizer161;
	sbSizer161 = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Statistics") ), wxVERTICAL );
	
	wxBoxSizer* bSizer157;
	bSizer157 = new wxBoxSizer( wxHORIZONTAL );
	
	wxFlexGridSizer* fgSizer5;
	fgSizer5 = new wxFlexGridSizer( 4, 2, 0, 5 );
	fgSizer5->SetFlexibleDirection( wxHORIZONTAL );
	fgSizer5->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );
	
	
	fgSizer5->Add( 0, 0, 0, wxALIGN_CENTER_VERTICAL|wxALIGN_CENTER_HORIZONTAL, 5 );
	
	m_staticText94 = new wxStaticText( this, wxID_ANY, _("Left"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText94->Wrap( -1 );
	m_staticText94->SetFont( wxFont( 9, 70, 90, 92, false, wxEmptyString ) );
	
	fgSizer5->Add( m_staticText94, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 5 );
	
	m_bitmapCreate = new wxStaticBitmap( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, 0 );
	m_bitmapCreate->SetToolTip( _("Number of files and directories that will be created") );
	
	fgSizer5->Add( m_bitmapCreate, 0, wxALIGN_CENTER_VERTICAL|wxALIGN_CENTER_HORIZONTAL, 5 );
	
	m_textCtrlCreateL = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize( 80,-1 ), wxTE_READONLY );
	m_textCtrlCreateL->SetFont( wxFont( 8, 70, 90, 90, false, wxEmptyString ) );
	m_textCtrlCreateL->SetBackgroundColour( wxColour( 208, 208, 208 ) );
	m_textCtrlCreateL->SetToolTip( _("Number of files and directories that will be created") );
	
	fgSizer5->Add( m_textCtrlCreateL, 0, wxALIGN_CENTER_VERTICAL, 5 );
	
	m_bitmapUpdate = new wxStaticBitmap( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, 0 );
	m_bitmapUpdate->SetToolTip( _("Number of files that will be overwritten") );
	
	fgSizer5->Add( m_bitmapUpdate, 0, wxALIGN_CENTER_VERTICAL|wxALIGN_CENTER_HORIZONTAL, 5 );
	
	m_textCtrlUpdateL = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize( 80,-1 ), wxTE_READONLY );
	m_textCtrlUpdateL->SetFont( wxFont( 8, 70, 90, 90, false, wxEmptyString ) );
	m_textCtrlUpdateL->SetBackgroundColour( wxColour( 208, 208, 208 ) );
	m_textCtrlUpdateL->SetToolTip( _("Number of files that will be overwritten") );
	
	fgSizer5->Add( m_textCtrlUpdateL, 0, wxALIGN_CENTER_VERTICAL, 5 );
	
	m_bitmapDelete = new wxStaticBitmap( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, 0 );
	m_bitmapDelete->SetToolTip( _("Number of files and directories that will be deleted") );
	
	fgSizer5->Add( m_bitmapDelete, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 5 );
	
	m_textCtrlDeleteL = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize( 80,-1 ), wxTE_READONLY );
	m_textCtrlDeleteL->SetFont( wxFont( 8, 70, 90, 90, false, wxEmptyString ) );
	m_textCtrlDeleteL->SetBackgroundColour( wxColour( 208, 208, 208 ) );
	m_textCtrlDeleteL->SetToolTip( _("Number of files and directories that will be deleted") );
	
	fgSizer5->Add( m_textCtrlDeleteL, 0, wxALIGN_CENTER_VERTICAL, 5 );
	
	bSizer157->Add( fgSizer5, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );
	
	wxFlexGridSizer* fgSizer51;
	fgSizer51 = new wxFlexGridSizer( 3, 1, 0, 5 );
	fgSizer51->SetFlexibleDirection( wxHORIZONTAL );
	fgSizer51->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );
	
	m_staticText95 = new wxStaticText( this, wxID_ANY, _("Right"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText95->Wrap( -1 );
	m_staticText95->SetFont( wxFont( 9, 70, 90, 92, false, wxEmptyString ) );
	
	fgSizer51->Add( m_staticText95, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 5 );
	
	m_textCtrlCreateR = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize( 80,-1 ), wxTE_READONLY );
	m_textCtrlCreateR->SetFont( wxFont( 8, 70, 90, 90, false, wxEmptyString ) );
	m_textCtrlCreateR->SetBackgroundColour( wxColour( 208, 208, 208 ) );
	m_textCtrlCreateR->SetToolTip( _("Number of files and directories that will be created") );
	
	fgSizer51->Add( m_textCtrlCreateR, 0, wxALIGN_CENTER_VERTICAL, 5 );
	
	m_textCtrlUpdateR = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize( 80,-1 ), wxTE_READONLY );
	m_textCtrlUpdateR->SetFont( wxFont( 8, 70, 90, 90, false, wxEmptyString ) );
	m_textCtrlUpdateR->SetBackgroundColour( wxColour( 208, 208, 208 ) );
	m_textCtrlUpdateR->SetToolTip( _("Number of files that will be overwritten") );
	
	fgSizer51->Add( m_textCtrlUpdateR, 0, wxALIGN_CENTER_VERTICAL, 5 );
	
	m_textCtrlDeleteR = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize( 80,-1 ), wxTE_READONLY );
	m_textCtrlDeleteR->SetFont( wxFont( 8, 70, 90, 90, false, wxEmptyString ) );
	m_textCtrlDeleteR->SetBackgroundColour( wxColour( 208, 208, 208 ) );
	m_textCtrlDeleteR->SetToolTip( _("Number of files and directories that will be deleted") );
	
	fgSizer51->Add( m_textCtrlDeleteR, 0, wxALIGN_CENTER_VERTICAL, 5 );
	
	bSizer157->Add( fgSizer51, 0, wxRIGHT|wxLEFT|wxALIGN_CENTER_VERTICAL, 5 );
	
	sbSizer161->Add( bSizer157, 0, wxALIGN_CENTER_VERTICAL|wxALIGN_CENTER_HORIZONTAL, 5 );
	
	
	sbSizer161->Add( 0, 10, 0, 0, 5 );
	
	wxBoxSizer* bSizer156;
	bSizer156 = new wxBoxSizer( wxHORIZONTAL );
	
	m_bitmapData = new wxStaticBitmap( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, 0 );
	m_bitmapData->SetToolTip( _("Total amount of data that will be transferred") );
	
	bSizer156->Add( m_bitmapData, 0, wxALIGN_CENTER_VERTICAL|wxALIGN_CENTER_HORIZONTAL|wxRIGHT, 5 );
	
	
	bSizer156->Add( 0, 0, 1, wxEXPAND, 5 );
	
	m_textCtrlData = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize( 80,-1 ), wxTE_READONLY );
	m_textCtrlData->SetFont( wxFont( 8, 70, 90, 90, false, wxEmptyString ) );
	m_textCtrlData->SetBackgroundColour( wxColour( 208, 208, 208 ) );
	m_textCtrlData->SetToolTip( _("Total amount of data that will be transferred") );
	
	bSizer156->Add( m_textCtrlData, 0, wxALIGN_CENTER_VERTICAL, 5 );
	
	
	bSizer156->Add( 0, 0, 1, wxEXPAND, 5 );
	
	sbSizer161->Add( bSizer156, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT|wxEXPAND, 5 );
	
	bSizer141->Add( sbSizer161, 0, wxALIGN_CENTER_VERTICAL|wxALIGN_CENTER_HORIZONTAL|wxALL, 5 );
	
	bSizer134->Add( bSizer141, 0, wxALIGN_CENTER_HORIZONTAL, 5 );
	
	m_staticline12 = new wxStaticLine( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	bSizer134->Add( m_staticline12, 0, wxEXPAND|wxTOP|wxBOTTOM, 5 );
	
	wxBoxSizer* bSizer142;
	bSizer142 = new wxBoxSizer( wxHORIZONTAL );
	
	m_checkBoxDontShowAgain = new wxCheckBox( this, wxID_ANY, _("Do not show this dialog again"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizer142->Add( m_checkBoxDontShowAgain, 0, wxALL|wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 5 );
	
	
	bSizer142->Add( 10, 0, 1, 0, 5 );
	
	m_button16 = new wxButton( this, wxID_CANCEL, _("&Cancel"), wxDefaultPosition, wxSize( -1,30 ), 0 );
	m_button16->SetFont( wxFont( 10, 70, 90, 90, false, wxEmptyString ) );
	
	bSizer142->Add( m_button16, 0, wxALL|wxALIGN_CENTER_VERTICAL|wxALIGN_CENTER_HORIZONTAL, 5 );
	
	bSizer134->Add( bSizer142, 0, wxEXPAND, 5 );
	
	this->SetSizer( bSizer134 );
	this->Layout();
	bSizer134->Fit( this );
	
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
	
	
	bSizer166->Add( 0, 10, 0, 0, 5 );
	
	m_checkBoxMatchCase = new wxCheckBox( this, wxID_ANY, _("Match case"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizer166->Add( m_checkBoxMatchCase, 0, wxALL, 5 );
	
	bSizer161->Add( bSizer166, 1, wxALIGN_CENTER_VERTICAL, 5 );
	
	wxBoxSizer* bSizer97;
	bSizer97 = new wxBoxSizer( wxVERTICAL );
	
	m_buttonFindNext = new wxButton( this, wxID_OK, _("&Find next"), wxDefaultPosition, wxSize( -1,30 ), 0 );
	m_buttonFindNext->SetDefault(); 
	m_buttonFindNext->SetFont( wxFont( 10, 70, 90, 92, false, wxEmptyString ) );
	
	bSizer97->Add( m_buttonFindNext, 0, wxALL|wxEXPAND, 5 );
	
	m_button29 = new wxButton( this, wxID_CANCEL, _("&Cancel"), wxDefaultPosition, wxSize( -1,30 ), 0 );
	m_button29->SetFont( wxFont( 10, 70, 90, 90, false, wxEmptyString ) );
	
	bSizer97->Add( m_button29, 0, wxALL|wxEXPAND, 5 );
	
	bSizer161->Add( bSizer97, 0, wxALIGN_CENTER_VERTICAL, 5 );
	
	this->SetSizer( bSizer161 );
	this->Layout();
	bSizer161->Fit( this );
	
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
