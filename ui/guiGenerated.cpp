///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Apr 16 2008)
// http://www.wxformbuilder.org/
//
// PLEASE DO "NOT" EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "wx/wxprec.h"

#ifdef __BORLANDC__
#pragma hdrstop
#endif //__BORLANDC__

#ifndef WX_PRECOMP
#include <wx/wx.h>
#endif //WX_PRECOMP

#include "../library/customGrid.h"

#include "guiGenerated.h"

///////////////////////////////////////////////////////////////////////////

GuiGenerated::GuiGenerated( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : wxFrame( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxDefaultSize, wxDefaultSize );
	
	m_menubar1 = new wxMenuBar( 0 );
	m_menu1 = new wxMenu();
	wxMenuItem* m_menuItem4;
	m_menuItem4 = new wxMenuItem( m_menu1, wxID_EXIT, wxString( _("&Quit") ) + wxT('\t') + wxT("CTRL-Q"), wxEmptyString, wxITEM_NORMAL );
	m_menu1->Append( m_menuItem4 );
	
	m_menubar1->Append( m_menu1, _("&File") );
	
	m_menu3 = new wxMenu();
	wxMenuItem* m_menuItem5;
	m_menuItem5 = new wxMenuItem( m_menu3, wxID_ANY, wxString( _("&Export file list") ) , wxEmptyString, wxITEM_NORMAL );
	m_menu3->Append( m_menuItem5 );
	
	m_menu31 = new wxMenu();
	m_menuItemEnglish = new wxMenuItem( m_menu31, wxID_ANY, wxString( _("English") ) , wxEmptyString, wxITEM_RADIO );
	m_menu31->Append( m_menuItemEnglish );
	
	m_menuItemGerman = new wxMenuItem( m_menu31, wxID_ANY, wxString( _("Deutsch") ) , wxEmptyString, wxITEM_RADIO );
	m_menu31->Append( m_menuItemGerman );
	
	m_menu3->Append( -1, _("&Language"), m_menu31 );
	
	m_menu3->AppendSeparator();
	
	wxMenuItem* m_menuItem7;
	m_menuItem7 = new wxMenuItem( m_menu3, wxID_ANY, wxString( _("&Create batch job") ) , wxEmptyString, wxITEM_NORMAL );
	m_menu3->Append( m_menuItem7 );
	
	m_menubar1->Append( m_menu3, _("&Advanced") );
	
	m_menu2 = new wxMenu();
	wxMenuItem* m_menuItem3;
	m_menuItem3 = new wxMenuItem( m_menu2, wxID_ABOUT, wxString( _("&About...") ) + wxT('\t') + wxT("F1"), wxEmptyString, wxITEM_NORMAL );
	m_menu2->Append( m_menuItem3 );
	
	m_menubar1->Append( m_menu2, _("&Help") );
	
	this->SetMenuBar( m_menubar1 );
	
	bSizer1 = new wxBoxSizer( wxVERTICAL );
	
	m_panel71 = new wxPanel( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxSIMPLE_BORDER|wxTAB_TRAVERSAL );
	m_panel71->SetBackgroundColour( wxSystemSettings::GetColour( wxSYS_COLOUR_APPWORKSPACE ) );
	
	bSizer6 = new wxBoxSizer( wxHORIZONTAL );
	
	
	bSizer6->Add( 40, 0, 0, wxALIGN_CENTER_VERTICAL, 5 );
	
	wxBoxSizer* bSizer30;
	bSizer30 = new wxBoxSizer( wxHORIZONTAL );
	
	m_bpButtonCompare = new wxBitmapButton( m_panel71, wxID_OK, wxNullBitmap, wxDefaultPosition, wxSize( 190,37 ), wxBU_AUTODRAW|wxFULL_REPAINT_ON_RESIZE );
	m_bpButtonCompare->SetDefault(); 
	m_bpButtonCompare->SetToolTip( _("Compare both sides") );
	
	m_bpButtonCompare->SetToolTip( _("Compare both sides") );
	
	bSizer30->Add( m_bpButtonCompare, 0, wxALIGN_CENTER_HORIZONTAL|wxRIGHT|wxALIGN_CENTER_VERTICAL, 5 );
	
	m_buttonAbort = new wxButton( m_panel71, wxID_CANCEL, _("Abort"), wxDefaultPosition, wxSize( 190,37 ), 0 );
	m_buttonAbort->SetFont( wxFont( 14, 74, 90, 92, false, wxT("Tahoma") ) );
	m_buttonAbort->Enable( false );
	m_buttonAbort->Hide();
	
	bSizer30->Add( m_buttonAbort, 0, wxALIGN_CENTER_VERTICAL|wxALIGN_CENTER_HORIZONTAL|wxRIGHT, 5 );
	
	bSizer6->Add( bSizer30, 0, wxALIGN_CENTER_VERTICAL, 5 );
	
	wxBoxSizer* bSizer55;
	bSizer55 = new wxBoxSizer( wxVERTICAL );
	
	wxStaticBoxSizer* sbSizer6;
	sbSizer6 = new wxStaticBoxSizer( new wxStaticBox( m_panel71, wxID_ANY, _("Compare by...") ), wxHORIZONTAL );
	
	wxBoxSizer* bSizer45;
	bSizer45 = new wxBoxSizer( wxVERTICAL );
	
	m_radioBtnSizeDate = new wxRadioButton( m_panel71, wxID_ANY, _("File size and date"), wxDefaultPosition, wxDefaultSize, 0 );
	m_radioBtnSizeDate->SetValue( true ); 
	m_radioBtnSizeDate->SetToolTip( _("Files are found equal if\n     - filesize\n     - last write time (UTC) and date\nare the same.") );
	
	bSizer45->Add( m_radioBtnSizeDate, 0, 0, 5 );
	
	m_radioBtnContent = new wxRadioButton( m_panel71, wxID_ANY, _("File content"), wxDefaultPosition, wxDefaultSize, 0 );
	m_radioBtnContent->SetToolTip( _("Files are found equal if\n     - file content\nis the same.") );
	
	bSizer45->Add( m_radioBtnContent, 0, wxTOP, 5 );
	
	sbSizer6->Add( bSizer45, 0, wxALIGN_CENTER_VERTICAL, 5 );
	
	m_bpButton14 = new wxBitmapButton( m_panel71, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxSize( 40,40 ), wxBU_AUTODRAW );
	sbSizer6->Add( m_bpButton14, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );
	
	bSizer55->Add( sbSizer6, 0, wxALIGN_CENTER_VERTICAL, 2 );
	
	
	bSizer55->Add( 0, 4, 0, 0, 5 );
	
	bSizer6->Add( bSizer55, 0, wxALIGN_CENTER_VERTICAL, 5 );
	
	
	bSizer6->Add( 0, 0, 1, wxALIGN_CENTER_VERTICAL, 5 );
	
	wxBoxSizer* bSizer56;
	bSizer56 = new wxBoxSizer( wxVERTICAL );
	
	wxStaticBoxSizer* sbSizer9;
	sbSizer9 = new wxStaticBoxSizer( new wxStaticBox( m_panel71, wxID_ANY, _("Filter files") ), wxHORIZONTAL );
	
	m_bpButtonFilter = new wxBitmapButton( m_panel71, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxSize( 40,40 ), wxBU_AUTODRAW|wxFULL_REPAINT_ON_RESIZE );
	sbSizer9->Add( m_bpButtonFilter, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );
	
	wxBoxSizer* bSizer23;
	bSizer23 = new wxBoxSizer( wxVERTICAL );
	
	m_hyperlinkCfgFilter = new wxHyperlinkCtrl( m_panel71, wxID_ANY, _("Configure filter..."), wxEmptyString, wxDefaultPosition, wxDefaultSize, wxHL_DEFAULT_STYLE );
	
	m_hyperlinkCfgFilter->SetNormalColour( wxColour( 0, 0, 255 ) );
	m_hyperlinkCfgFilter->SetVisitedColour( wxColour( 0, 0, 255 ) );
	m_hyperlinkCfgFilter->SetBackgroundColour( wxColour( 128, 128, 150 ) );
	
	bSizer23->Add( m_hyperlinkCfgFilter, 0, wxALL, 5 );
	
	m_checkBoxHideFilt = new wxCheckBox( m_panel71, wxID_ANY, _("Hide filtered items"), wxDefaultPosition, wxDefaultSize, 0 );
	
	m_checkBoxHideFilt->SetToolTip( _("Choose to hide filtered files/directories from list") );
	
	bSizer23->Add( m_checkBoxHideFilt, 0, 0, 5 );
	
	sbSizer9->Add( bSizer23, 0, 0, 5 );
	
	bSizer56->Add( sbSizer9, 0, wxALIGN_CENTER_VERTICAL, 5 );
	
	
	bSizer56->Add( 0, 4, 0, wxALIGN_CENTER_HORIZONTAL, 5 );
	
	bSizer6->Add( bSizer56, 0, wxALIGN_CENTER_VERTICAL, 5 );
	
	m_bpButtonSync = new wxBitmapButton( m_panel71, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxSize( 190,37 ), wxBU_AUTODRAW|wxFULL_REPAINT_ON_RESIZE );
	m_bpButtonSync->SetToolTip( _("Open synchronization dialog") );
	
	m_bpButtonSync->SetToolTip( _("Open synchronization dialog") );
	
	bSizer6->Add( m_bpButtonSync, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );
	
	
	bSizer6->Add( 40, 0, 0, wxEXPAND|wxALIGN_CENTER_VERTICAL, 5 );
	
	m_panel71->SetSizer( bSizer6 );
	m_panel71->Layout();
	bSizer6->Fit( m_panel71 );
	bSizer1->Add( m_panel71, 0, wxEXPAND|wxALIGN_CENTER_HORIZONTAL, 5 );
	
	wxBoxSizer* bSizer2;
	bSizer2 = new wxBoxSizer( wxHORIZONTAL );
	
	m_panel1 = new wxPanel( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	wxBoxSizer* bSizer7;
	bSizer7 = new wxBoxSizer( wxVERTICAL );
	
	wxStaticBoxSizer* sbSizer2;
	sbSizer2 = new wxStaticBoxSizer( new wxStaticBox( m_panel1, wxID_ANY, _("Drag && drop") ), wxHORIZONTAL );
	
	m_directoryPanel1 = new wxTextCtrl( m_panel1, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	sbSizer2->Add( m_directoryPanel1, 1, wxALIGN_CENTER_VERTICAL, 5 );
	
	m_dirPicker1 = new wxDirPickerCtrl( m_panel1, wxID_ANY, wxEmptyString, _("Select a folder"), wxDefaultPosition, wxDefaultSize, wxDIRP_DIR_MUST_EXIST );
	sbSizer2->Add( m_dirPicker1, 0, wxALIGN_CENTER_VERTICAL, 5 );
	
	bSizer7->Add( sbSizer2, 0, wxEXPAND, 5 );
	
	m_grid1 = new CustomGrid( m_panel1, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0 );
	
	// Grid
	m_grid1->CreateGrid( 15, 4 );
	m_grid1->EnableEditing( false );
	m_grid1->EnableGridLines( true );
	m_grid1->EnableDragGridSize( true );
	m_grid1->SetMargins( 0, 0 );
	
	// Columns
	m_grid1->SetColSize( 0, 138 );
	m_grid1->SetColSize( 1, 118 );
	m_grid1->SetColSize( 2, 67 );
	m_grid1->SetColSize( 3, 113 );
	m_grid1->EnableDragColMove( false );
	m_grid1->EnableDragColSize( true );
	m_grid1->SetColLabelSize( 20 );
	m_grid1->SetColLabelValue( 0, _("Filename") );
	m_grid1->SetColLabelValue( 1, _("Relative path") );
	m_grid1->SetColLabelValue( 2, _("Size") );
	m_grid1->SetColLabelValue( 3, _("Date") );
	m_grid1->SetColLabelAlignment( wxALIGN_LEFT, wxALIGN_CENTRE );
	
	// Rows
	m_grid1->EnableDragRowSize( false );
	m_grid1->SetRowLabelSize( 38 );
	m_grid1->SetRowLabelAlignment( wxALIGN_CENTRE, wxALIGN_CENTRE );
	
	// Label Appearance
	
	// Cell Defaults
	m_grid1->SetDefaultCellAlignment( wxALIGN_LEFT, wxALIGN_CENTRE );
	bSizer7->Add( m_grid1, 1, wxALIGN_CENTER_VERTICAL|wxEXPAND|wxALL, 5 );
	
	m_panel1->SetSizer( bSizer7 );
	m_panel1->Layout();
	bSizer7->Fit( m_panel1 );
	bSizer2->Add( m_panel1, 1, wxEXPAND, 5 );
	
	m_panel3 = new wxPanel( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	wxBoxSizer* bSizer18;
	bSizer18 = new wxBoxSizer( wxVERTICAL );
	
	m_bpButtonSwap = new wxBitmapButton( m_panel3, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxSize( 42,42 ), wxBU_AUTODRAW );
	m_bpButtonSwap->SetToolTip( _("Swap sides") );
	
	m_bpButtonSwap->SetToolTip( _("Swap sides") );
	
	bSizer18->Add( m_bpButtonSwap, 0, wxALIGN_CENTER_HORIZONTAL|wxTOP, 3 );
	
	m_grid3 = new CustomGrid( m_panel3, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0 );
	
	// Grid
	m_grid3->CreateGrid( 15, 1 );
	m_grid3->EnableEditing( false );
	m_grid3->EnableGridLines( true );
	m_grid3->EnableDragGridSize( false );
	m_grid3->SetMargins( 0, 0 );
	
	// Columns
	m_grid3->SetColSize( 0, 45 );
	m_grid3->EnableDragColMove( false );
	m_grid3->EnableDragColSize( false );
	m_grid3->SetColLabelSize( 20 );
	m_grid3->SetColLabelValue( 0, _("Filter") );
	m_grid3->SetColLabelAlignment( wxALIGN_CENTRE, wxALIGN_CENTRE );
	
	// Rows
	m_grid3->EnableDragRowSize( false );
	m_grid3->SetRowLabelSize( 0 );
	m_grid3->SetRowLabelAlignment( wxALIGN_CENTRE, wxALIGN_CENTRE );
	
	// Label Appearance
	
	// Cell Defaults
	m_grid3->SetDefaultCellFont( wxFont( 12, 74, 90, 92, false, wxT("Arial") ) );
	m_grid3->SetDefaultCellAlignment( wxALIGN_CENTRE, wxALIGN_CENTRE );
	bSizer18->Add( m_grid3, 1, wxALIGN_CENTER_HORIZONTAL|wxEXPAND|wxTOP|wxBOTTOM, 5 );
	
	m_panel3->SetSizer( bSizer18 );
	m_panel3->Layout();
	bSizer18->Fit( m_panel3 );
	bSizer2->Add( m_panel3, 0, wxEXPAND|wxRIGHT|wxLEFT, 5 );
	
	m_panel2 = new wxPanel( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	wxBoxSizer* bSizer10;
	bSizer10 = new wxBoxSizer( wxVERTICAL );
	
	wxStaticBoxSizer* sbSizer3;
	sbSizer3 = new wxStaticBoxSizer( new wxStaticBox( m_panel2, wxID_ANY, _("Drag && drop") ), wxHORIZONTAL );
	
	m_directoryPanel2 = new wxTextCtrl( m_panel2, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	sbSizer3->Add( m_directoryPanel2, 1, wxALIGN_CENTER_VERTICAL, 5 );
	
	m_dirPicker2 = new wxDirPickerCtrl( m_panel2, wxID_ANY, wxEmptyString, _("Select a folder"), wxDefaultPosition, wxDefaultSize, wxDIRP_DIR_MUST_EXIST );
	sbSizer3->Add( m_dirPicker2, 0, wxALIGN_CENTER_VERTICAL, 5 );
	
	bSizer10->Add( sbSizer3, 0, wxEXPAND, 5 );
	
	m_grid2 = new CustomGrid( m_panel2, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0 );
	
	// Grid
	m_grid2->CreateGrid( 15, 4 );
	m_grid2->EnableEditing( false );
	m_grid2->EnableGridLines( true );
	m_grid2->EnableDragGridSize( true );
	m_grid2->SetMargins( 0, 0 );
	
	// Columns
	m_grid2->SetColSize( 0, 138 );
	m_grid2->SetColSize( 1, 118 );
	m_grid2->SetColSize( 2, 67 );
	m_grid2->SetColSize( 3, 113 );
	m_grid2->EnableDragColMove( false );
	m_grid2->EnableDragColSize( true );
	m_grid2->SetColLabelSize( 20 );
	m_grid2->SetColLabelValue( 0, _("Filename") );
	m_grid2->SetColLabelValue( 1, _("Relative path") );
	m_grid2->SetColLabelValue( 2, _("Size") );
	m_grid2->SetColLabelValue( 3, _("Date") );
	m_grid2->SetColLabelAlignment( wxALIGN_LEFT, wxALIGN_CENTRE );
	
	// Rows
	m_grid2->EnableDragRowSize( false );
	m_grid2->SetRowLabelSize( 38 );
	m_grid2->SetRowLabelAlignment( wxALIGN_CENTRE, wxALIGN_CENTRE );
	
	// Label Appearance
	
	// Cell Defaults
	m_grid2->SetDefaultCellAlignment( wxALIGN_LEFT, wxALIGN_CENTRE );
	bSizer10->Add( m_grid2, 1, wxALIGN_CENTER_VERTICAL|wxEXPAND|wxALL, 5 );
	
	m_panel2->SetSizer( bSizer10 );
	m_panel2->Layout();
	bSizer10->Fit( m_panel2 );
	bSizer2->Add( m_panel2, 1, wxEXPAND, 5 );
	
	bSizer1->Add( bSizer2, 1, wxEXPAND, 5 );
	
	m_panel4 = new wxPanel( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	bSizer3 = new wxBoxSizer( wxHORIZONTAL );
	
	m_bpButton11 = new wxBitmapButton( m_panel4, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxSize( 50,50 ), wxBU_AUTODRAW );
	m_bpButton11->SetToolTip( _("About") );
	
	m_bpButton11->SetToolTip( _("About") );
	
	bSizer3->Add( m_bpButton11, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );
	
	
	bSizer3->Add( 0, 0, 1, wxEXPAND, 5 );
	
	wxBoxSizer* bSizer58;
	bSizer58 = new wxBoxSizer( wxVERTICAL );
	
	wxStaticBoxSizer* sbSizer16;
	sbSizer16 = new wxStaticBoxSizer( new wxStaticBox( m_panel4, wxID_ANY, _("Configuration") ), wxHORIZONTAL );
	
	m_bpButton201 = new wxBitmapButton( m_panel4, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxSize( 40,40 ), wxBU_AUTODRAW );
	m_bpButton201->SetToolTip( _("Save current configuration to file") );
	
	m_bpButton201->SetToolTip( _("Save current configuration to file") );
	
	sbSizer16->Add( m_bpButton201, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );
	
	wxString m_choiceLoadChoices[] = { _("Load configuration...") };
	int m_choiceLoadNChoices = sizeof( m_choiceLoadChoices ) / sizeof( wxString );
	m_choiceLoad = new wxChoice( m_panel4, wxID_ANY, wxDefaultPosition, wxSize( 140,-1 ), m_choiceLoadNChoices, m_choiceLoadChoices, 0 );
	m_choiceLoad->SetSelection( 0 );
	m_choiceLoad->SetToolTip( _("Load configuration from file:\n     - use this choice box\n     - drag & drop config file to this window\n     - specify config file as first commandline parameter") );
	
	sbSizer16->Add( m_choiceLoad, 0, wxALIGN_CENTER_VERTICAL, 5 );
	
	bSizer58->Add( sbSizer16, 0, wxALIGN_CENTER_VERTICAL, 5 );
	
	
	bSizer58->Add( 0, 4, 0, wxALIGN_CENTER_HORIZONTAL, 5 );
	
	bSizer3->Add( bSizer58, 0, wxALIGN_CENTER_VERTICAL, 5 );
	
	
	bSizer3->Add( 0, 0, 1, wxEXPAND, 5 );
	
	bSizer59 = new wxBoxSizer( wxVERTICAL );
	
	wxStaticBoxSizer* sbSizer31;
	sbSizer31 = new wxStaticBoxSizer( new wxStaticBox( m_panel4, wxID_ANY, _("Filter view") ), wxHORIZONTAL );
	
	m_bpButton20 = new wxBitmapButton( m_panel4, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxSize( 40,40 ), wxBU_AUTODRAW );
	m_bpButton20->SetToolTip( _("Files that exist on left view only") );
	
	m_bpButton20->SetToolTip( _("Files that exist on left view only") );
	
	sbSizer31->Add( m_bpButton20, 0, wxALIGN_CENTER_VERTICAL, 5 );
	
	m_bpButton21 = new wxBitmapButton( m_panel4, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxSize( 40,40 ), wxBU_AUTODRAW );
	m_bpButton21->SetToolTip( _("Files that are newer on left") );
	
	m_bpButton21->SetToolTip( _("Files that are newer on left") );
	
	sbSizer31->Add( m_bpButton21, 0, wxALIGN_CENTER_VERTICAL, 5 );
	
	m_bpButton25 = new wxBitmapButton( m_panel4, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxSize( 40,40 ), wxBU_AUTODRAW );
	m_bpButton25->SetToolTip( _("Files that are equal") );
	
	m_bpButton25->SetToolTip( _("Files that are equal") );
	
	sbSizer31->Add( m_bpButton25, 0, wxALIGN_CENTER_VERTICAL, 5 );
	
	m_bpButton22 = new wxBitmapButton( m_panel4, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxSize( 40,40 ), wxBU_AUTODRAW );
	m_bpButton22->SetToolTip( _("Files that are different") );
	
	m_bpButton22->SetToolTip( _("Files that are different") );
	
	sbSizer31->Add( m_bpButton22, 0, wxALIGN_CENTER_VERTICAL, 5 );
	
	m_bpButton23 = new wxBitmapButton( m_panel4, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxSize( 40,40 ), wxBU_AUTODRAW );
	m_bpButton23->SetToolTip( _("Files that are newer on right") );
	
	m_bpButton23->SetToolTip( _("Files that are newer on right") );
	
	sbSizer31->Add( m_bpButton23, 0, wxALIGN_CENTER_VERTICAL, 5 );
	
	m_bpButton24 = new wxBitmapButton( m_panel4, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxSize( 40,40 ), wxBU_AUTODRAW );
	m_bpButton24->SetToolTip( _("Files that exist on right view only") );
	
	m_bpButton24->SetToolTip( _("Files that exist on right view only") );
	
	sbSizer31->Add( m_bpButton24, 0, wxALIGN_CENTER_VERTICAL, 5 );
	
	bSizer59->Add( sbSizer31, 0, wxALIGN_CENTER_VERTICAL|wxALIGN_CENTER_HORIZONTAL, 5 );
	
	
	bSizer59->Add( 0, 4, 0, wxALIGN_CENTER_HORIZONTAL, 5 );
	
	bSizer3->Add( bSizer59, 0, wxALIGN_CENTER_VERTICAL, 5 );
	
	
	bSizer3->Add( 0, 0, 1, wxEXPAND, 5 );
	
	
	bSizer3->Add( 185, 0, 0, wxALL, 5 );
	
	
	bSizer3->Add( 0, 0, 1, wxEXPAND, 5 );
	
	m_bpButton10 = new wxBitmapButton( m_panel4, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxSize( 50,50 ), wxBU_AUTODRAW );
	m_bpButton10->SetToolTip( _("Quit") );
	
	m_bpButton10->SetToolTip( _("Quit") );
	
	bSizer3->Add( m_bpButton10, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );
	
	m_panel4->SetSizer( bSizer3 );
	m_panel4->Layout();
	bSizer3->Fit( m_panel4 );
	bSizer1->Add( m_panel4, 0, wxALIGN_CENTER_HORIZONTAL|wxEXPAND, 5 );
	
	m_panel7 = new wxPanel( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxSTATIC_BORDER|wxTAB_TRAVERSAL );
	wxBoxSizer* bSizer451;
	bSizer451 = new wxBoxSizer( wxHORIZONTAL );
	
	bSizer451->SetMinSize( wxSize( -1,22 ) ); 
	wxBoxSizer* bSizer53;
	bSizer53 = new wxBoxSizer( wxHORIZONTAL );
	
	
	bSizer53->Add( 0, 0, 1, wxALIGN_CENTER_VERTICAL, 5 );
	
	m_staticTextStatusLeft = new wxStaticText( m_panel7, wxID_ANY, _("Dummy text"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextStatusLeft->Wrap( -1 );
	bSizer53->Add( m_staticTextStatusLeft, 0, wxALIGN_CENTER_VERTICAL|wxALIGN_CENTER_HORIZONTAL, 5 );
	
	
	bSizer53->Add( 0, 0, 1, wxALIGN_CENTER_VERTICAL, 5 );
	
	bSizer451->Add( bSizer53, 1, wxALIGN_CENTER_VERTICAL|wxEXPAND, 5 );
	
	m_staticline9 = new wxStaticLine( m_panel7, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_VERTICAL );
	bSizer451->Add( m_staticline9, 0, wxALIGN_CENTER_VERTICAL|wxTOP|wxEXPAND, 2 );
	
	
	bSizer451->Add( 26, 0, 0, wxALIGN_CENTER_VERTICAL, 5 );
	
	m_staticTextStatusMiddle = new wxStaticText( m_panel7, wxID_ANY, _("Dummy text"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextStatusMiddle->Wrap( -1 );
	bSizer451->Add( m_staticTextStatusMiddle, 0, wxALIGN_CENTER_VERTICAL|wxALIGN_CENTER_HORIZONTAL, 5 );
	
	
	bSizer451->Add( 26, 0, 0, wxALIGN_CENTER_VERTICAL, 5 );
	
	m_staticline10 = new wxStaticLine( m_panel7, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_VERTICAL );
	bSizer451->Add( m_staticline10, 0, wxEXPAND|wxALIGN_CENTER_VERTICAL|wxTOP, 2 );
	
	wxBoxSizer* bSizer52;
	bSizer52 = new wxBoxSizer( wxHORIZONTAL );
	
	
	bSizer52->Add( 0, 0, 1, wxALIGN_CENTER_VERTICAL, 5 );
	
	m_staticTextStatusRight = new wxStaticText( m_panel7, wxID_ANY, _("Dummy text"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextStatusRight->Wrap( -1 );
	bSizer52->Add( m_staticTextStatusRight, 0, wxALIGN_CENTER_VERTICAL, 5 );
	
	wxBoxSizer* bSizer50;
	bSizer50 = new wxBoxSizer( wxHORIZONTAL );
	
	
	bSizer50->Add( 0, 0, 1, wxALIGN_CENTER_VERTICAL, 5 );
	
	m_bitmap15 = new wxStaticBitmap( m_panel7, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxSize( 10,10 ), 0 );
	bSizer50->Add( m_bitmap15, 0, wxALIGN_BOTTOM, 2 );
	
	bSizer52->Add( bSizer50, 1, wxALIGN_BOTTOM|wxEXPAND, 5 );
	
	bSizer451->Add( bSizer52, 1, wxEXPAND|wxALIGN_CENTER_VERTICAL, 5 );
	
	m_panel7->SetSizer( bSizer451 );
	m_panel7->Layout();
	bSizer451->Fit( m_panel7 );
	bSizer1->Add( m_panel7, 0, wxALIGN_CENTER_HORIZONTAL|wxEXPAND, 5 );
	
	this->SetSizer( bSizer1 );
	this->Layout();
	
	// Connect Events
	this->Connect( wxEVT_CLOSE_WINDOW, wxCloseEventHandler( GuiGenerated::OnClose ) );
	this->Connect( m_menuItem4->GetId(), wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( GuiGenerated::OnMenuQuit ) );
	this->Connect( m_menuItem5->GetId(), wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( GuiGenerated::OnMenuExportFileList ) );
	this->Connect( m_menuItemEnglish->GetId(), wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( GuiGenerated::OnMenuLangEnglish ) );
	this->Connect( m_menuItemGerman->GetId(), wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( GuiGenerated::OnMenuLangGerman ) );
	this->Connect( m_menuItem7->GetId(), wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( GuiGenerated::OnMenuBatchJob ) );
	this->Connect( m_menuItem3->GetId(), wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( GuiGenerated::OnMenuAbout ) );
	m_bpButtonCompare->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( GuiGenerated::OnCompare ), NULL, this );
	m_buttonAbort->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( GuiGenerated::OnAbortCompare ), NULL, this );
	m_radioBtnSizeDate->Connect( wxEVT_COMMAND_RADIOBUTTON_SELECTED, wxCommandEventHandler( GuiGenerated::OnCompareByTimeSize ), NULL, this );
	m_radioBtnContent->Connect( wxEVT_COMMAND_RADIOBUTTON_SELECTED, wxCommandEventHandler( GuiGenerated::OnCompareByContent ), NULL, this );
	m_bpButton14->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( GuiGenerated::OnShowHelpDialog ), NULL, this );
	m_bpButtonFilter->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( GuiGenerated::OnFilterButton ), NULL, this );
	m_hyperlinkCfgFilter->Connect( wxEVT_COMMAND_HYPERLINK, wxHyperlinkEventHandler( GuiGenerated::OnConfigureFilter ), NULL, this );
	m_checkBoxHideFilt->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( GuiGenerated::OnHideFilteredButton ), NULL, this );
	m_bpButtonSync->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( GuiGenerated::OnSync ), NULL, this );
	m_directoryPanel1->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( GuiGenerated::OnEnterLeftDir ), NULL, this );
	m_dirPicker1->Connect( wxEVT_COMMAND_DIRPICKER_CHANGED, wxFileDirPickerEventHandler( GuiGenerated::OnDirChangedPanel1 ), NULL, this );
	m_grid1->Connect( wxEVT_GRID_CELL_LEFT_DCLICK, wxGridEventHandler( GuiGenerated::OnLeftGridDoubleClick ), NULL, this );
	m_grid1->Connect( wxEVT_GRID_CELL_RIGHT_CLICK, wxGridEventHandler( GuiGenerated::OnOpenContextMenu ), NULL, this );
	m_grid1->Connect( wxEVT_GRID_LABEL_LEFT_CLICK, wxGridEventHandler( GuiGenerated::OnSortLeftGrid ), NULL, this );
	m_bpButtonSwap->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( GuiGenerated::OnSwapDirs ), NULL, this );
	m_grid3->Connect( wxEVT_GRID_CELL_RIGHT_CLICK, wxGridEventHandler( GuiGenerated::OnOpenContextMenu ), NULL, this );
	m_directoryPanel2->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( GuiGenerated::OnEnterRightDir ), NULL, this );
	m_dirPicker2->Connect( wxEVT_COMMAND_DIRPICKER_CHANGED, wxFileDirPickerEventHandler( GuiGenerated::OnDirChangedPanel2 ), NULL, this );
	m_grid2->Connect( wxEVT_GRID_CELL_LEFT_DCLICK, wxGridEventHandler( GuiGenerated::OnRightGridDoubleClick ), NULL, this );
	m_grid2->Connect( wxEVT_GRID_CELL_RIGHT_CLICK, wxGridEventHandler( GuiGenerated::OnOpenContextMenu ), NULL, this );
	m_grid2->Connect( wxEVT_GRID_LABEL_LEFT_CLICK, wxGridEventHandler( GuiGenerated::OnSortRightGrid ), NULL, this );
	m_bpButton11->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( GuiGenerated::OnAbout ), NULL, this );
	m_bpButton201->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( GuiGenerated::OnSaveConfig ), NULL, this );
	m_choiceLoad->Connect( wxEVT_CHAR, wxKeyEventHandler( GuiGenerated::OnChoiceKeyEvent ), NULL, this );
	m_choiceLoad->Connect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( GuiGenerated::OnLoadConfiguration ), NULL, this );
	m_bpButton20->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( GuiGenerated::OnLeftOnlyFiles ), NULL, this );
	m_bpButton21->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( GuiGenerated::OnLeftNewerFiles ), NULL, this );
	m_bpButton25->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( GuiGenerated::OnEqualFiles ), NULL, this );
	m_bpButton22->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( GuiGenerated::OnDifferentFiles ), NULL, this );
	m_bpButton23->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( GuiGenerated::OnRightNewerFiles ), NULL, this );
	m_bpButton24->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( GuiGenerated::OnRightOnlyFiles ), NULL, this );
	m_bpButton10->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( GuiGenerated::OnQuit ), NULL, this );
}

GuiGenerated::~GuiGenerated()
{
	// Disconnect Events
	this->Disconnect( wxEVT_CLOSE_WINDOW, wxCloseEventHandler( GuiGenerated::OnClose ) );
	this->Disconnect( wxID_ANY, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( GuiGenerated::OnMenuQuit ) );
	this->Disconnect( wxID_ANY, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( GuiGenerated::OnMenuExportFileList ) );
	this->Disconnect( wxID_ANY, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( GuiGenerated::OnMenuLangEnglish ) );
	this->Disconnect( wxID_ANY, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( GuiGenerated::OnMenuLangGerman ) );
	this->Disconnect( wxID_ANY, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( GuiGenerated::OnMenuBatchJob ) );
	this->Disconnect( wxID_ANY, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( GuiGenerated::OnMenuAbout ) );
	m_bpButtonCompare->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( GuiGenerated::OnCompare ), NULL, this );
	m_buttonAbort->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( GuiGenerated::OnAbortCompare ), NULL, this );
	m_radioBtnSizeDate->Disconnect( wxEVT_COMMAND_RADIOBUTTON_SELECTED, wxCommandEventHandler( GuiGenerated::OnCompareByTimeSize ), NULL, this );
	m_radioBtnContent->Disconnect( wxEVT_COMMAND_RADIOBUTTON_SELECTED, wxCommandEventHandler( GuiGenerated::OnCompareByContent ), NULL, this );
	m_bpButton14->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( GuiGenerated::OnShowHelpDialog ), NULL, this );
	m_bpButtonFilter->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( GuiGenerated::OnFilterButton ), NULL, this );
	m_hyperlinkCfgFilter->Disconnect( wxEVT_COMMAND_HYPERLINK, wxHyperlinkEventHandler( GuiGenerated::OnConfigureFilter ), NULL, this );
	m_checkBoxHideFilt->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( GuiGenerated::OnHideFilteredButton ), NULL, this );
	m_bpButtonSync->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( GuiGenerated::OnSync ), NULL, this );
	m_directoryPanel1->Disconnect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( GuiGenerated::OnEnterLeftDir ), NULL, this );
	m_dirPicker1->Disconnect( wxEVT_COMMAND_DIRPICKER_CHANGED, wxFileDirPickerEventHandler( GuiGenerated::OnDirChangedPanel1 ), NULL, this );
	m_grid1->Disconnect( wxEVT_GRID_CELL_LEFT_DCLICK, wxGridEventHandler( GuiGenerated::OnLeftGridDoubleClick ), NULL, this );
	m_grid1->Disconnect( wxEVT_GRID_CELL_RIGHT_CLICK, wxGridEventHandler( GuiGenerated::OnOpenContextMenu ), NULL, this );
	m_grid1->Disconnect( wxEVT_GRID_LABEL_LEFT_CLICK, wxGridEventHandler( GuiGenerated::OnSortLeftGrid ), NULL, this );
	m_bpButtonSwap->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( GuiGenerated::OnSwapDirs ), NULL, this );
	m_grid3->Disconnect( wxEVT_GRID_CELL_RIGHT_CLICK, wxGridEventHandler( GuiGenerated::OnOpenContextMenu ), NULL, this );
	m_directoryPanel2->Disconnect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( GuiGenerated::OnEnterRightDir ), NULL, this );
	m_dirPicker2->Disconnect( wxEVT_COMMAND_DIRPICKER_CHANGED, wxFileDirPickerEventHandler( GuiGenerated::OnDirChangedPanel2 ), NULL, this );
	m_grid2->Disconnect( wxEVT_GRID_CELL_LEFT_DCLICK, wxGridEventHandler( GuiGenerated::OnRightGridDoubleClick ), NULL, this );
	m_grid2->Disconnect( wxEVT_GRID_CELL_RIGHT_CLICK, wxGridEventHandler( GuiGenerated::OnOpenContextMenu ), NULL, this );
	m_grid2->Disconnect( wxEVT_GRID_LABEL_LEFT_CLICK, wxGridEventHandler( GuiGenerated::OnSortRightGrid ), NULL, this );
	m_bpButton11->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( GuiGenerated::OnAbout ), NULL, this );
	m_bpButton201->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( GuiGenerated::OnSaveConfig ), NULL, this );
	m_choiceLoad->Disconnect( wxEVT_CHAR, wxKeyEventHandler( GuiGenerated::OnChoiceKeyEvent ), NULL, this );
	m_choiceLoad->Disconnect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( GuiGenerated::OnLoadConfiguration ), NULL, this );
	m_bpButton20->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( GuiGenerated::OnLeftOnlyFiles ), NULL, this );
	m_bpButton21->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( GuiGenerated::OnLeftNewerFiles ), NULL, this );
	m_bpButton25->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( GuiGenerated::OnEqualFiles ), NULL, this );
	m_bpButton22->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( GuiGenerated::OnDifferentFiles ), NULL, this );
	m_bpButton23->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( GuiGenerated::OnRightNewerFiles ), NULL, this );
	m_bpButton24->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( GuiGenerated::OnRightOnlyFiles ), NULL, this );
	m_bpButton10->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( GuiGenerated::OnQuit ), NULL, this );
}

CompareStatusGenerated::CompareStatusGenerated( wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style ) : wxPanel( parent, id, pos, size, style )
{
	wxBoxSizer* bSizer40;
	bSizer40 = new wxBoxSizer( wxVERTICAL );
	
	bSizer42 = new wxBoxSizer( wxHORIZONTAL );
	
	wxStaticBoxSizer* sbSizer10;
	sbSizer10 = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, wxEmptyString ), wxHORIZONTAL );
	
	m_staticText321 = new wxStaticText( this, wxID_ANY, _("Files scanned:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText321->Wrap( -1 );
	m_staticText321->SetFont( wxFont( 10, 74, 90, 90, false, wxT("Tahoma") ) );
	
	sbSizer10->Add( m_staticText321, 0, wxALIGN_CENTER_VERTICAL|wxALIGN_CENTER_HORIZONTAL|wxRIGHT, 5 );
	
	m_staticTextScanned = new wxStaticText( this, wxID_ANY, _("123456"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextScanned->Wrap( -1 );
	m_staticTextScanned->SetFont( wxFont( 10, 74, 90, 92, false, wxT("Tahoma") ) );
	
	sbSizer10->Add( m_staticTextScanned, 0, wxALIGN_CENTER_VERTICAL, 5 );
	
	bSizer42->Add( sbSizer10, 0, wxEXPAND|wxALIGN_CENTER_VERTICAL, 5 );
	
	
	bSizer42->Add( 0, 0, 1, wxEXPAND, 5 );
	
	sbSizer13 = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, wxEmptyString ), wxHORIZONTAL );
	
	m_staticText46 = new wxStaticText( this, wxID_ANY, _("Files to compare:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText46->Wrap( -1 );
	m_staticText46->SetFont( wxFont( 10, 74, 90, 90, false, wxT("Tahoma") ) );
	
	sbSizer13->Add( m_staticText46, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );
	
	m_staticTextFilesToCompare = new wxStaticText( this, wxID_ANY, _("123456"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextFilesToCompare->Wrap( -1 );
	m_staticTextFilesToCompare->SetFont( wxFont( 10, 74, 90, 92, false, wxT("Tahoma") ) );
	
	sbSizer13->Add( m_staticTextFilesToCompare, 0, wxALIGN_CENTER_VERTICAL|wxALIGN_CENTER_HORIZONTAL, 5 );
	
	bSizer42->Add( sbSizer13, 0, wxEXPAND|wxALIGN_CENTER_VERTICAL, 5 );
	
	
	bSizer42->Add( 0, 0, 1, wxEXPAND, 5 );
	
	sbSizer11 = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, wxEmptyString ), wxHORIZONTAL );
	
	m_staticText32 = new wxStaticText( this, wxID_ANY, _("Data to compare:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText32->Wrap( -1 );
	m_staticText32->SetFont( wxFont( 10, 74, 90, 90, false, wxT("Tahoma") ) );
	
	sbSizer11->Add( m_staticText32, 0, wxALIGN_CENTER_VERTICAL|wxALIGN_CENTER_HORIZONTAL|wxRIGHT, 5 );
	
	m_staticTextDataToCompare = new wxStaticText( this, wxID_ANY, _("123456"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextDataToCompare->Wrap( -1 );
	m_staticTextDataToCompare->SetFont( wxFont( 10, 74, 90, 92, false, wxT("Tahoma") ) );
	
	sbSizer11->Add( m_staticTextDataToCompare, 0, wxALIGN_CENTER_VERTICAL, 5 );
	
	bSizer42->Add( sbSizer11, 0, wxEXPAND|wxALIGN_CENTER_VERTICAL, 5 );
	
	
	bSizer42->Add( 0, 0, 1, wxEXPAND, 5 );
	
	wxStaticBoxSizer* sbSizer131;
	sbSizer131 = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, wxEmptyString ), wxHORIZONTAL );
	
	m_staticText37 = new wxStaticText( this, wxID_ANY, _("Remaining time:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText37->Wrap( -1 );
	m_staticText37->SetFont( wxFont( 10, 74, 90, 90, false, wxT("Tahoma") ) );
	m_staticText37->Hide();
	
	sbSizer131->Add( m_staticText37, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );
	
	m_staticTextRemainingTime = new wxStaticText( this, wxID_ANY, _("--.--"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextRemainingTime->Wrap( -1 );
	m_staticTextRemainingTime->SetFont( wxFont( 10, 74, 90, 92, false, wxT("Tahoma") ) );
	m_staticTextRemainingTime->Hide();
	
	sbSizer131->Add( m_staticTextRemainingTime, 0, wxALIGN_CENTER_VERTICAL, 5 );
	
	bSizer42->Add( sbSizer131, 0, wxEXPAND|wxALIGN_CENTER_VERTICAL, 5 );
	
	bSizer40->Add( bSizer42, 0, wxALIGN_CENTER_HORIZONTAL|wxEXPAND|wxALL, 5 );
	
	wxBoxSizer* bSizer48;
	bSizer48 = new wxBoxSizer( wxHORIZONTAL );
	
	m_staticText30 = new wxStaticText( this, wxID_ANY, _("Operation:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText30->Wrap( -1 );
	m_staticText30->SetFont( wxFont( 10, 74, 90, 92, false, wxT("Tahoma") ) );
	
	bSizer48->Add( m_staticText30, 0, wxRIGHT|wxALIGN_CENTER_VERTICAL, 5 );
	
	m_textCtrlFilename = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_READONLY );
	m_textCtrlFilename->SetBackgroundColour( wxSystemSettings::GetColour( wxSYS_COLOUR_ACTIVEBORDER ) );
	
	bSizer48->Add( m_textCtrlFilename, 1, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 5 );
	
	bSizer40->Add( bSizer48, 0, wxEXPAND|wxALIGN_CENTER_HORIZONTAL|wxBOTTOM|wxRIGHT|wxLEFT, 5 );
	
	m_gauge2 = new wxGauge( this, wxID_ANY, 100, wxDefaultPosition, wxSize( -1,20 ), wxGA_HORIZONTAL );
	bSizer40->Add( m_gauge2, 0, wxALL|wxEXPAND, 5 );
	
	this->SetSizer( bSizer40 );
	this->Layout();
	bSizer40->Fit( this );
}

CompareStatusGenerated::~CompareStatusGenerated()
{
}

SyncDlgGenerated::SyncDlgGenerated( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : wxDialog( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxDefaultSize, wxDefaultSize );
	
	wxBoxSizer* bSizer7;
	bSizer7 = new wxBoxSizer( wxVERTICAL );
	
	wxBoxSizer* bSizer181;
	bSizer181 = new wxBoxSizer( wxHORIZONTAL );
	
	wxBoxSizer* bSizer29;
	bSizer29 = new wxBoxSizer( wxVERTICAL );
	
	wxBoxSizer* bSizer201;
	bSizer201 = new wxBoxSizer( wxHORIZONTAL );
	
	m_bpButton18 = new wxBitmapButton( this, wxID_OK, wxNullBitmap, wxDefaultPosition, wxSize( 140,58 ), wxBU_AUTODRAW );
	m_bpButton18->SetDefault(); 
	m_bpButton18->SetToolTip( _("Start synchronization") );
	
	m_bpButton18->SetToolTip( _("Start synchronization") );
	
	bSizer201->Add( m_bpButton18, 0, wxALIGN_CENTER_VERTICAL, 5 );
	
	
	bSizer201->Add( 18, 0, 0, wxEXPAND, 5 );
	
	wxBoxSizer* bSizer38;
	bSizer38 = new wxBoxSizer( wxVERTICAL );
	
	m_checkBoxUseRecycler = new wxCheckBox( this, wxID_ANY, _("Use Recycle Bin"), wxDefaultPosition, wxDefaultSize, 0 );
	
	m_checkBoxUseRecycler->SetToolTip( _("Use Recycle Bin when deleting or overwriting files during synchronization") );
	
	bSizer38->Add( m_checkBoxUseRecycler, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );
	
	m_checkBoxContinueError = new wxCheckBox( this, wxID_ANY, _("Continue on error"), wxDefaultPosition, wxDefaultSize, 0 );
	
	m_checkBoxContinueError->SetToolTip( _("Hides error messages during synchronization:\nThey are collected and shown as a list at the end of the process") );
	
	bSizer38->Add( m_checkBoxContinueError, 0, wxALL, 5 );
	
	bSizer201->Add( bSizer38, 0, wxALIGN_CENTER_VERTICAL, 5 );
	
	bSizer29->Add( bSizer201, 1, 0, 5 );
	
	
	bSizer29->Add( 0, 5, 0, 0, 5 );
	
	wxStaticBoxSizer* sbSizer7;
	sbSizer7 = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, wxEmptyString ), wxVERTICAL );
	
	m_staticText1 = new wxStaticText( this, wxID_ANY, _("Select variant:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText1->Wrap( -1 );
	m_staticText1->SetFont( wxFont( 10, 74, 90, 92, false, wxT("Tahoma") ) );
	
	sbSizer7->Add( m_staticText1, 0, wxALL, 5 );
	
	wxFlexGridSizer* fgSizer1;
	fgSizer1 = new wxFlexGridSizer( 3, 3, 8, 5 );
	fgSizer1->SetFlexibleDirection( wxHORIZONTAL );
	fgSizer1->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );
	
	m_radioBtn1 = new wxRadioButton( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_radioBtn1->SetValue( true ); 
	m_radioBtn1->SetFont( wxFont( 11, 74, 90, 92, false, wxT("Tahoma") ) );
	
	fgSizer1->Add( m_radioBtn1, 0, wxALIGN_CENTER_VERTICAL, 5 );
	
	m_buttonOneWay = new wxButton( this, wxID_ANY, _("One way ->"), wxDefaultPosition, wxSize( 130,-1 ), 0 );
	m_buttonOneWay->SetFont( wxFont( 11, 74, 90, 92, false, wxT("Tahoma") ) );
	
	fgSizer1->Add( m_buttonOneWay, 0, wxALIGN_CENTER_VERTICAL, 5 );
	
	m_staticText8 = new wxStaticText( this, wxID_ANY, _("Mirror backup of left folder: Right folder will be overwritten and exactly match left folder after synchronization."), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText8->Wrap( 260 );
	fgSizer1->Add( m_staticText8, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );
	
	m_radioBtn2 = new wxRadioButton( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_radioBtn2->SetFont( wxFont( 11, 74, 90, 92, false, wxT("Tahoma") ) );
	
	fgSizer1->Add( m_radioBtn2, 0, wxALIGN_CENTER_VERTICAL, 5 );
	
	m_buttonTwoWay = new wxButton( this, wxID_ANY, _("Two way <->"), wxDefaultPosition, wxSize( 130,-1 ), 0 );
	m_buttonTwoWay->SetFont( wxFont( 11, 74, 90, 92, false, wxT("Tahoma") ) );
	
	fgSizer1->Add( m_buttonTwoWay, 0, wxALIGN_CENTER_VERTICAL, 5 );
	
	m_staticText10 = new wxStaticText( this, wxID_ANY, _("Synchronize both sides simultaneously: Copy new or updated files in both directions."), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText10->Wrap( 250 );
	fgSizer1->Add( m_staticText10, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );
	
	m_radioBtn3 = new wxRadioButton( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_radioBtn3->SetFont( wxFont( 11, 74, 90, 92, false, wxT("Tahoma") ) );
	
	fgSizer1->Add( m_radioBtn3, 0, wxALIGN_CENTER_VERTICAL, 5 );
	
	m_staticText23 = new wxStaticText( this, wxID_ANY, _("Custom"), wxDefaultPosition, wxSize( 130,-1 ), wxALIGN_CENTRE|wxSTATIC_BORDER );
	m_staticText23->Wrap( -1 );
	m_staticText23->SetFont( wxFont( 11, 74, 90, 92, false, wxT("Tahoma") ) );
	
	fgSizer1->Add( m_staticText23, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 5 );
	
	m_staticText9 = new wxStaticText( this, wxID_ANY, _("Configure your own synchronization rules."), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText9->Wrap( 250 );
	fgSizer1->Add( m_staticText9, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );
	
	sbSizer7->Add( fgSizer1, 0, 0, 5 );
	
	bSizer29->Add( sbSizer7, 0, wxEXPAND, 5 );
	
	
	bSizer29->Add( 0, 5, 0, 0, 5 );
	
	wxBoxSizer* bSizer291;
	bSizer291 = new wxBoxSizer( wxHORIZONTAL );
	
	m_button6 = new wxButton( this, wxID_ANY, _("&Back"), wxDefaultPosition, wxSize( 100,32 ), 0 );
	m_button6->SetFont( wxFont( 10, 74, 90, 90, false, wxT("Tahoma") ) );
	
	bSizer291->Add( m_button6, 0, wxALIGN_CENTER_VERTICAL, 5 );
	
	m_button16 = new wxButton( this, wxID_CANCEL, _("dummy"), wxDefaultPosition, wxSize( 0,0 ), 0 );
	bSizer291->Add( m_button16, 0, wxALIGN_BOTTOM, 5 );
	
	
	bSizer291->Add( 20, 0, 1, wxALIGN_CENTER_VERTICAL, 5 );
	
	wxStaticBoxSizer* sbSizer16;
	sbSizer16 = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Preview") ), wxHORIZONTAL );
	
	wxFlexGridSizer* fgSizer5;
	fgSizer5 = new wxFlexGridSizer( 2, 2, 0, 5 );
	fgSizer5->SetFlexibleDirection( wxHORIZONTAL );
	fgSizer5->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );
	
	m_staticText37 = new wxStaticText( this, wxID_ANY, _("Create:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText37->Wrap( -1 );
	m_staticText37->SetFont( wxFont( 10, 74, 93, 90, false, wxT("Tahoma") ) );
	m_staticText37->SetToolTip( _("Number of files or directories that will be created") );
	
	fgSizer5->Add( m_staticText37, 0, wxALIGN_CENTER_VERTICAL, 5 );
	
	m_textCtrlCreate = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize( 80,-1 ), wxTE_READONLY );
	m_textCtrlCreate->SetFont( wxFont( 8, 74, 90, 90, false, wxT("Tahoma") ) );
	m_textCtrlCreate->SetBackgroundColour( wxColour( 222, 222, 236 ) );
	m_textCtrlCreate->SetToolTip( _("Number of files and directories that will be created") );
	
	fgSizer5->Add( m_textCtrlCreate, 0, wxALIGN_CENTER_VERTICAL, 5 );
	
	m_staticText14 = new wxStaticText( this, wxID_ANY, _("Delete:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText14->Wrap( -1 );
	m_staticText14->SetFont( wxFont( 10, 74, 93, 90, false, wxT("Tahoma") ) );
	m_staticText14->SetToolTip( _("Number of files or directories that will be deleted") );
	
	fgSizer5->Add( m_staticText14, 0, wxALIGN_CENTER_VERTICAL, 5 );
	
	m_textCtrlDelete = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize( 80,-1 ), wxTE_READONLY );
	m_textCtrlDelete->SetFont( wxFont( 8, 74, 90, 90, false, wxT("Tahoma") ) );
	m_textCtrlDelete->SetBackgroundColour( wxColour( 222, 222, 236 ) );
	m_textCtrlDelete->SetToolTip( _("Number of files and directories that will be deleted") );
	
	fgSizer5->Add( m_textCtrlDelete, 0, wxALIGN_CENTER_VERTICAL, 5 );
	
	sbSizer16->Add( fgSizer5, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );
	
	wxFlexGridSizer* fgSizer6;
	fgSizer6 = new wxFlexGridSizer( 2, 2, 0, 5 );
	fgSizer6->SetFlexibleDirection( wxHORIZONTAL );
	fgSizer6->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );
	
	m_staticText42 = new wxStaticText( this, wxID_ANY, _("Update:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText42->Wrap( -1 );
	m_staticText42->SetFont( wxFont( 10, 74, 93, 90, false, wxT("Tahoma") ) );
	m_staticText42->SetToolTip( _("Number of files that will be overwritten") );
	
	fgSizer6->Add( m_staticText42, 0, wxALIGN_CENTER_VERTICAL, 5 );
	
	m_textCtrlUpdate = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize( 80,-1 ), wxTE_READONLY );
	m_textCtrlUpdate->SetFont( wxFont( 8, 74, 90, 90, false, wxT("Tahoma") ) );
	m_textCtrlUpdate->SetBackgroundColour( wxColour( 222, 222, 236 ) );
	m_textCtrlUpdate->SetToolTip( _("Number of files that will be overwritten") );
	
	fgSizer6->Add( m_textCtrlUpdate, 0, wxALIGN_CENTER_VERTICAL, 5 );
	
	m_staticText43 = new wxStaticText( this, wxID_ANY, _("Data:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText43->Wrap( -1 );
	m_staticText43->SetFont( wxFont( 10, 74, 93, 90, false, wxT("Tahoma") ) );
	m_staticText43->SetToolTip( _("Total amount of data that will be transferred") );
	
	fgSizer6->Add( m_staticText43, 0, wxALIGN_CENTER_VERTICAL, 5 );
	
	m_textCtrlData = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize( 80,-1 ), wxTE_READONLY );
	m_textCtrlData->SetFont( wxFont( 8, 74, 90, 90, false, wxT("Tahoma") ) );
	m_textCtrlData->SetBackgroundColour( wxColour( 222, 222, 236 ) );
	m_textCtrlData->SetToolTip( _("Total amount of data that will be transferred") );
	
	fgSizer6->Add( m_textCtrlData, 0, wxALIGN_CENTER_VERTICAL, 5 );
	
	sbSizer16->Add( fgSizer6, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );
	
	bSizer291->Add( sbSizer16, 0, wxALIGN_CENTER_VERTICAL|wxEXPAND, 5 );
	
	bSizer29->Add( bSizer291, 0, wxEXPAND, 5 );
	
	bSizer181->Add( bSizer29, 0, 0, 5 );
	
	
	bSizer181->Add( 10, 0, 0, 0, 5 );
	
	wxStaticBoxSizer* sbSizer6;
	sbSizer6 = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Configuration") ), wxVERTICAL );
	
	wxGridSizer* gSizer3;
	gSizer3 = new wxGridSizer( 1, 2, 0, 5 );
	
	m_staticText21 = new wxStaticText( this, wxID_ANY, _("Result"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText21->Wrap( -1 );
	m_staticText21->SetFont( wxFont( 8, 74, 90, 92, false, wxT("Tahoma") ) );
	
	gSizer3->Add( m_staticText21, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 5 );
	
	m_staticText31 = new wxStaticText( this, wxID_ANY, _("Action"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText31->Wrap( -1 );
	m_staticText31->SetFont( wxFont( 8, 74, 90, 92, false, wxT("Tahoma") ) );
	
	gSizer3->Add( m_staticText31, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 5 );
	
	sbSizer6->Add( gSizer3, 0, wxEXPAND, 5 );
	
	m_staticline3 = new wxStaticLine( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	sbSizer6->Add( m_staticline3, 0, wxALIGN_CENTER_HORIZONTAL|wxEXPAND|wxTOP|wxBOTTOM, 5 );
	
	wxGridSizer* gSizer1;
	gSizer1 = new wxGridSizer( 5, 2, 0, 5 );
	
	m_bitmap13 = new wxStaticBitmap( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxSize( 48,48 ), 0 );
	m_bitmap13->SetToolTip( _("Files/folders that exist on left side only") );
	
	gSizer1->Add( m_bitmap13, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 5 );
	
	m_bpButton5 = new wxBitmapButton( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxSize( 42,42 ), wxBU_AUTODRAW );
	gSizer1->Add( m_bpButton5, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 5 );
	
	m_bitmap14 = new wxStaticBitmap( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxSize( 48,48 ), 0 );
	m_bitmap14->SetToolTip( _("Files/folders that exist on right side only") );
	
	gSizer1->Add( m_bitmap14, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 5 );
	
	m_bpButton6 = new wxBitmapButton( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxSize( 42,42 ), wxBU_AUTODRAW );
	gSizer1->Add( m_bpButton6, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 5 );
	
	m_bitmap15 = new wxStaticBitmap( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxSize( 48,48 ), 0 );
	m_bitmap15->SetToolTip( _("Files that exist on both sides, left one is newer") );
	
	gSizer1->Add( m_bitmap15, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 5 );
	
	m_bpButton7 = new wxBitmapButton( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxSize( 42,42 ), wxBU_AUTODRAW );
	gSizer1->Add( m_bpButton7, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 5 );
	
	m_bitmap16 = new wxStaticBitmap( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxSize( 48,48 ), 0 );
	m_bitmap16->SetToolTip( _("Files that exist on both sides, right one is newer") );
	
	gSizer1->Add( m_bitmap16, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 5 );
	
	m_bpButton8 = new wxBitmapButton( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxSize( 42,42 ), wxBU_AUTODRAW );
	gSizer1->Add( m_bpButton8, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 5 );
	
	m_bitmap17 = new wxStaticBitmap( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxSize( 48,48 ), 0 );
	m_bitmap17->SetToolTip( _("Files that exist on both sides and are different") );
	
	gSizer1->Add( m_bitmap17, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 5 );
	
	m_bpButton9 = new wxBitmapButton( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxSize( 42,42 ), wxBU_AUTODRAW );
	gSizer1->Add( m_bpButton9, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 5 );
	
	sbSizer6->Add( gSizer1, 0, wxALIGN_CENTER_HORIZONTAL|wxEXPAND, 5 );
	
	bSizer181->Add( sbSizer6, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_BOTTOM, 5 );
	
	bSizer7->Add( bSizer181, 0, wxALL, 5 );
	
	this->SetSizer( bSizer7 );
	this->Layout();
	bSizer7->Fit( this );
	
	this->Centre( wxBOTH );
	
	// Connect Events
	this->Connect( wxEVT_CLOSE_WINDOW, wxCloseEventHandler( SyncDlgGenerated::OnClose ) );
	m_bpButton18->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( SyncDlgGenerated::OnStartSync ), NULL, this );
	m_checkBoxUseRecycler->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( SyncDlgGenerated::OnSelectRecycleBin ), NULL, this );
	m_radioBtn1->Connect( wxEVT_COMMAND_RADIOBUTTON_SELECTED, wxCommandEventHandler( SyncDlgGenerated::OnSyncLeftToRight ), NULL, this );
	m_buttonOneWay->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( SyncDlgGenerated::OnSyncLeftToRight ), NULL, this );
	m_radioBtn2->Connect( wxEVT_COMMAND_RADIOBUTTON_SELECTED, wxCommandEventHandler( SyncDlgGenerated::OnSyncBothSides ), NULL, this );
	m_buttonTwoWay->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( SyncDlgGenerated::OnSyncBothSides ), NULL, this );
	m_radioBtn3->Connect( wxEVT_COMMAND_RADIOBUTTON_SELECTED, wxCommandEventHandler( SyncDlgGenerated::OnSyncCostum ), NULL, this );
	m_button6->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( SyncDlgGenerated::OnBack ), NULL, this );
	m_button16->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( SyncDlgGenerated::OnCancel ), NULL, this );
	m_bpButton5->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( SyncDlgGenerated::OnExLeftSideOnly ), NULL, this );
	m_bpButton6->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( SyncDlgGenerated::OnExRightSideOnly ), NULL, this );
	m_bpButton7->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( SyncDlgGenerated::OnLeftNewer ), NULL, this );
	m_bpButton8->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( SyncDlgGenerated::OnRightNewer ), NULL, this );
	m_bpButton9->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( SyncDlgGenerated::OnDifferent ), NULL, this );
}

SyncDlgGenerated::~SyncDlgGenerated()
{
	// Disconnect Events
	this->Disconnect( wxEVT_CLOSE_WINDOW, wxCloseEventHandler( SyncDlgGenerated::OnClose ) );
	m_bpButton18->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( SyncDlgGenerated::OnStartSync ), NULL, this );
	m_checkBoxUseRecycler->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( SyncDlgGenerated::OnSelectRecycleBin ), NULL, this );
	m_radioBtn1->Disconnect( wxEVT_COMMAND_RADIOBUTTON_SELECTED, wxCommandEventHandler( SyncDlgGenerated::OnSyncLeftToRight ), NULL, this );
	m_buttonOneWay->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( SyncDlgGenerated::OnSyncLeftToRight ), NULL, this );
	m_radioBtn2->Disconnect( wxEVT_COMMAND_RADIOBUTTON_SELECTED, wxCommandEventHandler( SyncDlgGenerated::OnSyncBothSides ), NULL, this );
	m_buttonTwoWay->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( SyncDlgGenerated::OnSyncBothSides ), NULL, this );
	m_radioBtn3->Disconnect( wxEVT_COMMAND_RADIOBUTTON_SELECTED, wxCommandEventHandler( SyncDlgGenerated::OnSyncCostum ), NULL, this );
	m_button6->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( SyncDlgGenerated::OnBack ), NULL, this );
	m_button16->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( SyncDlgGenerated::OnCancel ), NULL, this );
	m_bpButton5->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( SyncDlgGenerated::OnExLeftSideOnly ), NULL, this );
	m_bpButton6->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( SyncDlgGenerated::OnExRightSideOnly ), NULL, this );
	m_bpButton7->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( SyncDlgGenerated::OnLeftNewer ), NULL, this );
	m_bpButton8->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( SyncDlgGenerated::OnRightNewer ), NULL, this );
	m_bpButton9->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( SyncDlgGenerated::OnDifferent ), NULL, this );
}

SyncStatusDlgGenerated::SyncStatusDlgGenerated( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : wxDialog( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxDefaultSize, wxDefaultSize );
	
	wxBoxSizer* bSizer27;
	bSizer27 = new wxBoxSizer( wxVERTICAL );
	
	
	bSizer27->Add( 0, 15, 0, wxEXPAND, 5 );
	
	wxBoxSizer* bSizer37;
	bSizer37 = new wxBoxSizer( wxHORIZONTAL );
	
	m_panel8 = new wxPanel( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxSIMPLE_BORDER|wxTAB_TRAVERSAL );
	m_panel8->SetBackgroundColour( wxSystemSettings::GetColour( wxSYS_COLOUR_3DLIGHT ) );
	
	wxBoxSizer* bSizer72;
	bSizer72 = new wxBoxSizer( wxVERTICAL );
	
	m_staticText56 = new wxStaticText( m_panel8, wxID_ANY, _("Synchronization status"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText56->Wrap( -1 );
	m_staticText56->SetFont( wxFont( 16, 74, 90, 92, false, wxT("Tahoma") ) );
	
	bSizer72->Add( m_staticText56, 0, wxALIGN_CENTER_HORIZONTAL|wxALL, 5 );
	
	m_panel8->SetSizer( bSizer72 );
	m_panel8->Layout();
	bSizer72->Fit( m_panel8 );
	bSizer37->Add( m_panel8, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );
	
	m_animationControl1 = new wxAnimationCtrl(this, wxID_ANY, wxNullAnimation, wxDefaultPosition, wxSize( 45,45 ));
	bSizer37->Add( m_animationControl1, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );
	
	bSizer27->Add( bSizer37, 0, wxALIGN_CENTER_HORIZONTAL, 5 );
	
	wxBoxSizer* bSizer42;
	bSizer42 = new wxBoxSizer( wxHORIZONTAL );
	
	
	bSizer42->Add( 0, 0, 1, wxEXPAND, 5 );
	
	m_bitmapStatus = new wxStaticBitmap( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxSize( 28,28 ), 0 );
	bSizer42->Add( m_bitmapStatus, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );
	
	m_staticTextStatus = new wxStaticText( this, wxID_ANY, _("Synchronizing..."), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextStatus->Wrap( -1 );
	m_staticTextStatus->SetFont( wxFont( 14, 74, 93, 90, false, wxT("Tahoma") ) );
	
	bSizer42->Add( m_staticTextStatus, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );
	
	
	bSizer42->Add( 0, 0, 1, wxEXPAND, 5 );
	
	bSizer27->Add( bSizer42, 0, wxEXPAND|wxALIGN_CENTER_HORIZONTAL, 5 );
	
	wxBoxSizer* bSizer31;
	bSizer31 = new wxBoxSizer( wxHORIZONTAL );
	
	m_staticText21 = new wxStaticText( this, wxID_ANY, _("Current operation:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText21->Wrap( -1 );
	m_staticText21->SetFont( wxFont( 10, 74, 90, 90, false, wxT("Tahoma") ) );
	
	bSizer31->Add( m_staticText21, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );
	
	
	bSizer31->Add( 0, 0, 1, wxEXPAND, 5 );
	
	bSizer27->Add( bSizer31, 0, wxEXPAND, 5 );
	
	m_textCtrlInfo = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_MULTILINE|wxTE_READONLY );
	m_textCtrlInfo->SetBackgroundColour( wxSystemSettings::GetColour( wxSYS_COLOUR_ACTIVEBORDER ) );
	
	bSizer27->Add( m_textCtrlInfo, 3, wxALL|wxEXPAND, 5 );
	
	m_gauge1 = new wxGauge( this, wxID_ANY, 100, wxDefaultPosition, wxSize( -1,20 ), wxGA_HORIZONTAL );
	bSizer27->Add( m_gauge1, 0, wxALL|wxEXPAND, 5 );
	
	bSizer28 = new wxBoxSizer( wxHORIZONTAL );
	
	wxBoxSizer* bSizer32;
	bSizer32 = new wxBoxSizer( wxHORIZONTAL );
	
	m_staticText26 = new wxStaticText( this, wxID_ANY, _("Data remaining:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText26->Wrap( -1 );
	m_staticText26->SetFont( wxFont( 10, 74, 93, 90, false, wxT("Tahoma") ) );
	
	bSizer32->Add( m_staticText26, 0, wxALIGN_CENTER_VERTICAL|wxTOP|wxBOTTOM|wxLEFT, 5 );
	
	m_staticTextDataRemaining = new wxStaticText( this, wxID_ANY, _("--,- MB"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextDataRemaining->Wrap( -1 );
	m_staticTextDataRemaining->SetFont( wxFont( 10, 74, 90, 92, false, wxT("Tahoma") ) );
	
	bSizer32->Add( m_staticTextDataRemaining, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );
	
	bSizer28->Add( bSizer32, 0, wxALIGN_CENTER_VERTICAL, 5 );
	
	
	bSizer28->Add( 0, 0, 1, 0, 5 );
	
	m_buttonOK = new wxButton( this, wxID_OK, _("&OK"), wxDefaultPosition, wxSize( 100,32 ), 0 );
	m_buttonOK->SetFont( wxFont( 10, 74, 90, 90, false, wxT("Tahoma") ) );
	m_buttonOK->Hide();
	
	bSizer28->Add( m_buttonOK, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );
	
	m_buttonPause = new wxButton( this, wxID_ANY, _("&Pause"), wxDefaultPosition, wxSize( 100,32 ), 0 );
	m_buttonPause->SetFont( wxFont( 10, 74, 90, 90, false, wxT("Tahoma") ) );
	
	bSizer28->Add( m_buttonPause, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );
	
	m_buttonAbort = new wxButton( this, wxID_CANCEL, _("&Abort"), wxDefaultPosition, wxSize( 100,32 ), 0 );
	m_buttonAbort->SetDefault(); 
	m_buttonAbort->SetFont( wxFont( 10, 74, 90, 90, false, wxT("Tahoma") ) );
	
	bSizer28->Add( m_buttonAbort, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );
	
	
	bSizer28->Add( 0, 0, 1, 0, 5 );
	
	wxBoxSizer* bSizer33;
	bSizer33 = new wxBoxSizer( wxHORIZONTAL );
	
	m_staticText25 = new wxStaticText( this, wxID_ANY, _("Files remaining:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText25->Wrap( -1 );
	m_staticText25->SetFont( wxFont( 10, 74, 93, 90, false, wxT("Tahoma") ) );
	
	bSizer33->Add( m_staticText25, 0, wxTOP|wxBOTTOM|wxLEFT|wxALIGN_CENTER_VERTICAL, 5 );
	
	m_staticTextRemainingObj = new wxStaticText( this, wxID_ANY, _("0000000"), wxDefaultPosition, wxSize( -1,-1 ), 0 );
	m_staticTextRemainingObj->Wrap( -1 );
	m_staticTextRemainingObj->SetFont( wxFont( 10, 74, 90, 92, false, wxT("Tahoma") ) );
	
	bSizer33->Add( m_staticTextRemainingObj, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );
	
	bSizer28->Add( bSizer33, 0, wxALIGN_CENTER_VERTICAL, 5 );
	
	bSizer27->Add( bSizer28, 0, wxALIGN_CENTER_HORIZONTAL|wxEXPAND, 5 );
	
	
	bSizer27->Add( 0, 5, 0, wxEXPAND, 5 );
	
	this->SetSizer( bSizer27 );
	this->Layout();
	
	// Connect Events
	this->Connect( wxEVT_CLOSE_WINDOW, wxCloseEventHandler( SyncStatusDlgGenerated::OnClose ) );
	m_buttonOK->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( SyncStatusDlgGenerated::OnOkay ), NULL, this );
	m_buttonPause->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( SyncStatusDlgGenerated::OnPause ), NULL, this );
	m_buttonAbort->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( SyncStatusDlgGenerated::OnAbort ), NULL, this );
}

SyncStatusDlgGenerated::~SyncStatusDlgGenerated()
{
	// Disconnect Events
	this->Disconnect( wxEVT_CLOSE_WINDOW, wxCloseEventHandler( SyncStatusDlgGenerated::OnClose ) );
	m_buttonOK->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( SyncStatusDlgGenerated::OnOkay ), NULL, this );
	m_buttonPause->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( SyncStatusDlgGenerated::OnPause ), NULL, this );
	m_buttonAbort->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( SyncStatusDlgGenerated::OnAbort ), NULL, this );
}

HelpDlgGenerated::HelpDlgGenerated( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : wxDialog( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxDefaultSize, wxDefaultSize );
	
	wxBoxSizer* bSizer20;
	bSizer20 = new wxBoxSizer( wxVERTICAL );
	
	
	bSizer20->Add( 0, 10, 0, wxEXPAND, 5 );
	
	m_panel8 = new wxPanel( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxSIMPLE_BORDER|wxTAB_TRAVERSAL );
	m_panel8->SetBackgroundColour( wxSystemSettings::GetColour( wxSYS_COLOUR_3DLIGHT ) );
	
	wxBoxSizer* bSizer72;
	bSizer72 = new wxBoxSizer( wxHORIZONTAL );
	
	
	bSizer72->Add( 20, 0, 0, 0, 5 );
	
	m_staticText56 = new wxStaticText( m_panel8, wxID_ANY, _("Help"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText56->Wrap( -1 );
	m_staticText56->SetFont( wxFont( 16, 74, 90, 92, false, wxT("Tahoma") ) );
	
	bSizer72->Add( m_staticText56, 0, wxALIGN_CENTER_HORIZONTAL|wxALL, 5 );
	
	
	bSizer72->Add( 20, 0, 0, 0, 5 );
	
	m_panel8->SetSizer( bSizer72 );
	m_panel8->Layout();
	bSizer72->Fit( m_panel8 );
	bSizer20->Add( m_panel8, 0, wxALIGN_CENTER_HORIZONTAL, 5 );
	
	
	bSizer20->Add( 0, 5, 0, wxEXPAND, 5 );
	
	m_textCtrl8 = new wxTextCtrl( this, wxID_ANY, _("Compare by \"File size and date\"\n----------------------------------------\nThis compare variant evaluates two equally named files as being equal when they have the same file size AND the same last write date and time. For the latter the system's UTC time (coordinated word time) is used internally, although the local file time is displayed on the result list. So there are no problems concerning different time zones or daylight saving time.\n\nWhen \"Compare\" is triggered with this option set the following decision tree is processed:\n\n                    -----------------\n                    |Decision tree|\n                    -----------------\n               ________|___________\n               |                                      |\n file exists on both sides     on one side only\n     _____|______                      __|___\n     |                     |                     |          |\nequal             different            left      right\n          _________|_____________\n         |                  |                         |\n  left newer  right newer  different (but same date)\n\nAs a result 6 different status can be returned to categorize all files:\n\n- exists left only\n- exists right only\n- left newer\n- right newer\n- different (but same date)\n- equal\n\n\nCompare by \"File content\"\n----------------------------------------\nAs the name suggests, two files which share the same name are marked as equal if and only if they have the same content. This option is useful for consistency checks rather than backup operations. Therefore the file times are not taken into account at all.\n\nWith this option enabled the decision tree is smaller:\n\n                    -----------------\n                    |Decision tree|\n                    -----------------\n               ________|___________\n               |                                      |\n file exists on both sides     on one side only\n     _____|______                      __|___\n     |                     |                     |          |\nequal             different            left      right\n\nAs a result the files are separated into the following categories:\n\n- exists left only\n- exists right only\n- different\n- equal"), wxDefaultPosition, wxDefaultSize, wxTE_MULTILINE|wxTE_READONLY );
	m_textCtrl8->SetBackgroundColour( wxSystemSettings::GetColour( wxSYS_COLOUR_ACTIVEBORDER ) );
	
	bSizer20->Add( m_textCtrl8, 1, wxALL|wxALIGN_CENTER_HORIZONTAL|wxEXPAND, 5 );
	
	m_button8 = new wxButton( this, wxID_OK, _("&OK"), wxDefaultPosition, wxSize( 100,32 ), 0 );
	m_button8->SetDefault(); 
	m_button8->SetFont( wxFont( 10, 74, 90, 90, false, wxT("Tahoma") ) );
	
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
	m_panel5->SetBackgroundColour( wxColour( 255, 255, 255 ) );
	
	wxBoxSizer* bSizer36;
	bSizer36 = new wxBoxSizer( wxHORIZONTAL );
	
	m_bitmap11 = new wxStaticBitmap( m_panel5, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxSize( 404,55 ), 0 );
	bSizer36->Add( m_bitmap11, 0, wxALIGN_CENTER_VERTICAL, 5 );
	
	m_panel5->SetSizer( bSizer36 );
	m_panel5->Layout();
	bSizer36->Fit( m_panel5 );
	bSizer31->Add( m_panel5, 0, wxALL|wxALIGN_CENTER_HORIZONTAL, 5 );
	
	m_staticText15 = new wxStaticText( this, wxID_ANY, _("-Open-Source file synchronization-"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText15->Wrap( -1 );
	m_staticText15->SetFont( wxFont( 8, 74, 90, 92, false, wxT("Tahoma") ) );
	
	bSizer31->Add( m_staticText15, 0, wxALIGN_CENTER_HORIZONTAL|wxBOTTOM|wxRIGHT|wxLEFT, 5 );
	
	m_build = new wxStaticText( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_build->Wrap( -1 );
	m_build->SetFont( wxFont( 8, 74, 90, 90, false, wxT("Tahoma") ) );
	
	bSizer31->Add( m_build, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxALL, 5 );
	
	
	bSizer31->Add( 0, 10, 0, 0, 5 );
	
	wxBoxSizer* bSizer53;
	bSizer53 = new wxBoxSizer( wxVERTICAL );
	
	wxTextCtrl* m_textCtrl3;
	m_textCtrl3 = new wxTextCtrl( this, wxID_ANY, _("Source code written completely in C++ utilizing:\n\n  MinGW \t\t- Windows port of GNU Compiler Collection\n  wxWidgets \t- Open-Source GUI framework\n  wxFormBuilder\t- wxWidgets GUI-builder\n  CodeBlocks \t- Open-Source IDE\n\n     by ZenJu"), wxDefaultPosition, wxSize( -1,-1 ), wxTE_MULTILINE|wxTE_READONLY|wxDOUBLE_BORDER );
	m_textCtrl3->SetBackgroundColour( wxSystemSettings::GetColour( wxSYS_COLOUR_ACTIVEBORDER ) );
	
	bSizer53->Add( m_textCtrl3, 0, wxALIGN_CENTER_HORIZONTAL|wxEXPAND, 5 );
	
	bSizer31->Add( bSizer53, 0, wxRIGHT|wxLEFT|wxALIGN_CENTER_HORIZONTAL|wxEXPAND, 30 );
	
	
	bSizer31->Add( 0, 7, 0, 0, 5 );
	
	wxStaticBoxSizer* sbSizer7;
	sbSizer7 = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, wxEmptyString ), wxVERTICAL );
	
	m_staticText131 = new wxStaticText( this, wxID_ANY, _("Feedback and suggestions are welcome at:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText131->Wrap( -1 );
	m_staticText131->SetFont( wxFont( 11, 74, 93, 92, false, wxT("Tahoma") ) );
	
	sbSizer7->Add( m_staticText131, 0, wxALIGN_CENTER_HORIZONTAL, 5 );
	
	bSizer31->Add( sbSizer7, 0, wxALIGN_CENTER_HORIZONTAL|wxTOP|wxBOTTOM, 5 );
	
	m_staticline3 = new wxStaticLine( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	bSizer31->Add( m_staticline3, 0, wxEXPAND|wxTOP|wxBOTTOM, 5 );
	
	wxFlexGridSizer* fgSizer2;
	fgSizer2 = new wxFlexGridSizer( 3, 3, 0, 0 );
	fgSizer2->SetFlexibleDirection( wxHORIZONTAL );
	fgSizer2->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );
	
	m_bitmap9 = new wxStaticBitmap( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxSize( 48,48 ), 0 );
	fgSizer2->Add( m_bitmap9, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );
	
	m_staticText11 = new wxStaticText( this, wxID_ANY, _("Homepage:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText11->Wrap( -1 );
	m_staticText11->SetFont( wxFont( 10, 74, 90, 92, false, wxT("Tahoma") ) );
	
	fgSizer2->Add( m_staticText11, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );
	
	m_hyperlink1 = new wxHyperlinkCtrl( this, wxID_ANY, _("FreeFileSync at Sourceforge"), wxT("http://sourceforge.net/projects/freefilesync/"), wxDefaultPosition, wxDefaultSize, wxHL_DEFAULT_STYLE );
	m_hyperlink1->SetToolTip( _("http://sourceforge.net/projects/freefilesync/") );
	
	fgSizer2->Add( m_hyperlink1, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );
	
	m_bitmap10 = new wxStaticBitmap( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxSize( 48,48 ), 0 );
	fgSizer2->Add( m_bitmap10, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );
	
	m_staticText13 = new wxStaticText( this, wxID_ANY, _("Email:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText13->Wrap( -1 );
	m_staticText13->SetFont( wxFont( 10, 74, 90, 92, false, wxT("Tahoma") ) );
	
	fgSizer2->Add( m_staticText13, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );
	
	m_hyperlink2 = new wxHyperlinkCtrl( this, wxID_ANY, _("zhnmju123@gmx.de"), wxT("mailto:zhnmju123@gmx.de"), wxDefaultPosition, wxDefaultSize, wxHL_DEFAULT_STYLE );
	fgSizer2->Add( m_hyperlink2, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );
	
	m_animationControl1 = new wxAnimationCtrl(this, wxID_ANY, wxNullAnimation);
	m_animationControl1->Hide();
	
	fgSizer2->Add( m_animationControl1, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );
	
	m_staticText151 = new wxStaticText( this, wxID_ANY, _("If you like FFS:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText151->Wrap( -1 );
	m_staticText151->SetFont( wxFont( 10, 74, 90, 92, false, wxT("Tahoma") ) );
	
	fgSizer2->Add( m_staticText151, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );
	
	m_hyperlink3 = new wxHyperlinkCtrl( this, wxID_ANY, _("Donate with Paypal"), wxT("https://www.paypal.com/cgi-bin/webscr?cmd=_donations&business=florian%2db%40gmx%2enet&no_shipping=0&no_note=1&tax=0&currency_code=EUR&lc=DE&bn=PP%2dDonationsBF&charset=UTF%2d8"), wxDefaultPosition, wxDefaultSize, wxHL_DEFAULT_STYLE );
	fgSizer2->Add( m_hyperlink3, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );
	
	bSizer31->Add( fgSizer2, 0, wxLEFT|wxEXPAND, 10 );
	
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
	
	m_button8 = new wxButton( this, wxID_OK, _("&OK"), wxDefaultPosition, wxSize( 100,32 ), 0 );
	m_button8->SetDefault(); 
	m_button8->SetFont( wxFont( 10, 74, 90, 90, false, wxT("Tahoma") ) );
	
	bSizer31->Add( m_button8, 0, wxALIGN_CENTER_HORIZONTAL|wxALL, 5 );
	
	this->SetSizer( bSizer31 );
	this->Layout();
	bSizer31->Fit( this );
	
	this->Centre( wxBOTH );
	
	// Connect Events
	this->Connect( wxEVT_CLOSE_WINDOW, wxCloseEventHandler( AboutDlgGenerated::OnClose ) );
	m_button8->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( AboutDlgGenerated::OnOK ), NULL, this );
}

AboutDlgGenerated::~AboutDlgGenerated()
{
	// Disconnect Events
	this->Disconnect( wxEVT_CLOSE_WINDOW, wxCloseEventHandler( AboutDlgGenerated::OnClose ) );
	m_button8->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( AboutDlgGenerated::OnOK ), NULL, this );
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
	m_textCtrl8->SetBackgroundColour( wxSystemSettings::GetColour( wxSYS_COLOUR_ACTIVEBORDER ) );
	
	bSizer26->Add( m_textCtrl8, 1, wxALIGN_CENTER_VERTICAL|wxEXPAND|wxBOTTOM|wxRIGHT|wxLEFT, 5 );
	
	bSizer24->Add( bSizer26, 1, wxALIGN_CENTER_HORIZONTAL|wxEXPAND, 5 );
	
	
	bSizer24->Add( 0, 10, 0, wxEXPAND, 5 );
	
	m_checkBoxContinueError = new wxCheckBox( this, wxID_ANY, _("Continue on next errors"), wxDefaultPosition, wxDefaultSize, 0 );
	
	m_checkBoxContinueError->SetToolTip( _("Hide further error messages during the current process and continue") );
	
	bSizer24->Add( m_checkBoxContinueError, 0, wxALL|wxALIGN_CENTER_HORIZONTAL, 5 );
	
	
	bSizer24->Add( 0, 10, 0, wxEXPAND, 5 );
	
	wxBoxSizer* bSizer25;
	bSizer25 = new wxBoxSizer( wxHORIZONTAL );
	
	m_buttonContinue = new wxButton( this, wxID_OK, _("&Continue"), wxDefaultPosition, wxSize( -1,30 ), 0 );
	m_buttonContinue->SetDefault(); 
	m_buttonContinue->SetFont( wxFont( 10, 74, 90, 90, false, wxT("Tahoma") ) );
	
	bSizer25->Add( m_buttonContinue, 0, wxTOP|wxBOTTOM|wxLEFT, 5 );
	
	m_buttonRetry = new wxButton( this, wxID_RETRY, _("&Retry"), wxDefaultPosition, wxSize( -1,30 ), 0 );
	m_buttonRetry->SetFont( wxFont( 10, 74, 90, 90, false, wxT("Tahoma") ) );
	
	bSizer25->Add( m_buttonRetry, 0, wxTOP|wxBOTTOM|wxLEFT, 5 );
	
	m_buttonAbort = new wxButton( this, wxID_CANCEL, _("&Abort"), wxDefaultPosition, wxSize( -1,30 ), 0 );
	m_buttonAbort->SetFont( wxFont( 10, 74, 90, 90, false, wxT("Tahoma") ) );
	
	bSizer25->Add( m_buttonAbort, 0, wxALL, 5 );
	
	bSizer24->Add( bSizer25, 0, wxALIGN_CENTER_HORIZONTAL, 5 );
	
	this->SetSizer( bSizer24 );
	this->Layout();
	
	// Connect Events
	this->Connect( wxEVT_CLOSE_WINDOW, wxCloseEventHandler( ErrorDlgGenerated::OnClose ) );
	m_buttonContinue->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( ErrorDlgGenerated::OnContinue ), NULL, this );
	m_buttonRetry->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( ErrorDlgGenerated::OnRetry ), NULL, this );
	m_buttonAbort->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( ErrorDlgGenerated::OnAbort ), NULL, this );
}

ErrorDlgGenerated::~ErrorDlgGenerated()
{
	// Disconnect Events
	this->Disconnect( wxEVT_CLOSE_WINDOW, wxCloseEventHandler( ErrorDlgGenerated::OnClose ) );
	m_buttonContinue->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( ErrorDlgGenerated::OnContinue ), NULL, this );
	m_buttonRetry->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( ErrorDlgGenerated::OnRetry ), NULL, this );
	m_buttonAbort->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( ErrorDlgGenerated::OnAbort ), NULL, this );
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
	
	m_staticTextHeader = new wxStaticText( this, wxID_ANY, _("Dummy text"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextHeader->Wrap( -1 );
	m_staticTextHeader->SetFont( wxFont( 10, 74, 90, 90, false, wxT("Tahoma") ) );
	
	bSizer41->Add( m_staticTextHeader, 0, wxALL|wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 5 );
	
	
	bSizer41->Add( 0, 0, 1, wxEXPAND, 5 );
	
	bSizer24->Add( bSizer41, 0, wxEXPAND, 5 );
	
	m_textCtrlMessage = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize( -1,-1 ), wxTE_MULTILINE|wxTE_READONLY );
	m_textCtrlMessage->SetBackgroundColour( wxSystemSettings::GetColour( wxSYS_COLOUR_ACTIVEBORDER ) );
	
	bSizer24->Add( m_textCtrlMessage, 1, wxALL|wxALIGN_CENTER_VERTICAL|wxEXPAND, 5 );
	
	wxBoxSizer* bSizer25;
	bSizer25 = new wxBoxSizer( wxHORIZONTAL );
	
	m_buttonOK = new wxButton( this, wxID_OK, _("&OK"), wxDefaultPosition, wxSize( -1,30 ), 0 );
	m_buttonOK->SetDefault(); 
	m_buttonOK->SetFont( wxFont( 10, 74, 90, 92, false, wxT("Tahoma") ) );
	
	bSizer25->Add( m_buttonOK, 0, wxTOP|wxBOTTOM|wxLEFT, 5 );
	
	m_buttonCancel = new wxButton( this, wxID_CANCEL, _("&Cancel"), wxDefaultPosition, wxSize( -1,30 ), 0 );
	m_buttonCancel->SetFont( wxFont( 10, 74, 90, 90, false, wxT("Tahoma") ) );
	
	bSizer25->Add( m_buttonCancel, 0, wxALL, 5 );
	
	bSizer24->Add( bSizer25, 0, wxALIGN_CENTER_HORIZONTAL, 5 );
	
	this->SetSizer( bSizer24 );
	this->Layout();
	
	// Connect Events
	this->Connect( wxEVT_CLOSE_WINDOW, wxCloseEventHandler( DeleteDlgGenerated::OnClose ) );
	m_buttonOK->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DeleteDlgGenerated::OnOK ), NULL, this );
	m_buttonCancel->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DeleteDlgGenerated::OnCancel ), NULL, this );
}

DeleteDlgGenerated::~DeleteDlgGenerated()
{
	// Disconnect Events
	this->Disconnect( wxEVT_CLOSE_WINDOW, wxCloseEventHandler( DeleteDlgGenerated::OnClose ) );
	m_buttonOK->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DeleteDlgGenerated::OnOK ), NULL, this );
	m_buttonCancel->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DeleteDlgGenerated::OnCancel ), NULL, this );
}

FilterDlgGenerated::FilterDlgGenerated( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : wxDialog( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxDefaultSize, wxDefaultSize );
	
	wxBoxSizer* bSizer21;
	bSizer21 = new wxBoxSizer( wxVERTICAL );
	
	m_panel8 = new wxPanel( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxSIMPLE_BORDER|wxTAB_TRAVERSAL );
	m_panel8->SetBackgroundColour( wxSystemSettings::GetColour( wxSYS_COLOUR_3DLIGHT ) );
	
	wxBoxSizer* bSizer72;
	bSizer72 = new wxBoxSizer( wxVERTICAL );
	
	m_staticText56 = new wxStaticText( m_panel8, wxID_ANY, _("Set filter for synchronization"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText56->Wrap( -1 );
	m_staticText56->SetFont( wxFont( 16, 74, 90, 92, false, wxT("Tahoma") ) );
	
	bSizer72->Add( m_staticText56, 0, wxALIGN_CENTER_HORIZONTAL|wxALL, 5 );
	
	m_panel8->SetSizer( bSizer72 );
	m_panel8->Layout();
	bSizer72->Fit( m_panel8 );
	bSizer21->Add( m_panel8, 0, wxALIGN_CENTER_HORIZONTAL|wxALL, 10 );
	
	m_staticText44 = new wxStaticText( this, wxID_ANY, _("Only files/directories that pass filtering will be selected for synchronization."), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText44->Wrap( -1 );
	bSizer21->Add( m_staticText44, 0, wxALIGN_CENTER_HORIZONTAL, 5 );
	
	
	bSizer21->Add( 0, 10, 0, 0, 5 );
	
	wxBoxSizer* bSizer52;
	bSizer52 = new wxBoxSizer( wxVERTICAL );
	
	m_staticText45 = new wxStaticText( this, wxID_ANY, _("Hints:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText45->Wrap( -1 );
	m_staticText45->SetFont( wxFont( 10, 74, 90, 92, true, wxT("Tahoma") ) );
	
	bSizer52->Add( m_staticText45, 0, wxBOTTOM, 5 );
	
	m_staticText18 = new wxStaticText( this, wxID_ANY, _("1. Enter full file or directory names separated by ';'.\n2. Wildcard characters '*' and '?' are supported.\n3. Case sensitive expressions!\n\nExample: *.tmp;*\\filename.dat;*\\directory\\*"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText18->Wrap( -1 );
	bSizer52->Add( m_staticText18, 0, wxALIGN_CENTER_HORIZONTAL, 5 );
	
	bSizer21->Add( bSizer52, 0, wxALIGN_CENTER_HORIZONTAL, 5 );
	
	wxStaticBoxSizer* sbSizer8;
	sbSizer8 = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, wxEmptyString ), wxVERTICAL );
	
	wxFlexGridSizer* fgSizer3;
	fgSizer3 = new wxFlexGridSizer( 2, 2, 0, 0 );
	fgSizer3->SetFlexibleDirection( wxBOTH );
	fgSizer3->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );
	
	
	fgSizer3->Add( 0, 0, 1, wxEXPAND, 5 );
	
	m_staticText15 = new wxStaticText( this, wxID_ANY, _("Include"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText15->Wrap( -1 );
	m_staticText15->SetFont( wxFont( 10, 74, 90, 92, false, wxT("Tahoma") ) );
	
	fgSizer3->Add( m_staticText15, 0, wxTOP|wxRIGHT|wxLEFT, 5 );
	
	m_bitmap8 = new wxStaticBitmap( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxSize( 30,30 ), 0 );
	fgSizer3->Add( m_bitmap8, 0, wxTOP|wxBOTTOM|wxLEFT, 5 );
	
	m_textCtrlInclude = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize( 400,-1 ), 0 );
	fgSizer3->Add( m_textCtrlInclude, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );
	
	sbSizer8->Add( fgSizer3, 0, 0, 5 );
	
	wxFlexGridSizer* fgSizer4;
	fgSizer4 = new wxFlexGridSizer( 2, 2, 0, 0 );
	fgSizer4->SetFlexibleDirection( wxBOTH );
	fgSizer4->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );
	
	
	fgSizer4->Add( 0, 0, 1, wxEXPAND, 5 );
	
	m_staticText16 = new wxStaticText( this, wxID_ANY, _("Exclude"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText16->Wrap( -1 );
	m_staticText16->SetFont( wxFont( 10, 74, 90, 92, false, wxT("Tahoma") ) );
	
	fgSizer4->Add( m_staticText16, 0, wxTOP|wxRIGHT|wxLEFT, 5 );
	
	m_bitmap9 = new wxStaticBitmap( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxSize( 30,30 ), 0 );
	fgSizer4->Add( m_bitmap9, 0, wxTOP|wxBOTTOM|wxLEFT, 5 );
	
	m_textCtrlExclude = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize( 400,-1 ), 0 );
	fgSizer4->Add( m_textCtrlExclude, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );
	
	sbSizer8->Add( fgSizer4, 0, 0, 5 );
	
	bSizer21->Add( sbSizer8, 0, wxRIGHT|wxLEFT|wxALIGN_CENTER_HORIZONTAL, 5 );
	
	
	bSizer21->Add( 0, 0, 0, 0, 5 );
	
	wxBoxSizer* bSizer22;
	bSizer22 = new wxBoxSizer( wxHORIZONTAL );
	
	m_button9 = new wxButton( this, wxID_DEFAULT, _("&Default"), wxDefaultPosition, wxSize( -1,30 ), 0 );
	m_button9->SetFont( wxFont( 10, 74, 90, 90, false, wxT("Tahoma") ) );
	
	bSizer22->Add( m_button9, 0, wxALL, 5 );
	
	
	bSizer22->Add( 0, 0, 1, wxEXPAND, 5 );
	
	m_button17 = new wxButton( this, wxID_CANCEL, _("dummy"), wxDefaultPosition, wxSize( 0,0 ), 0 );
	bSizer22->Add( m_button17, 0, wxALIGN_BOTTOM, 5 );
	
	m_button10 = new wxButton( this, wxID_OK, _("&OK"), wxDefaultPosition, wxSize( -1,30 ), 0 );
	m_button10->SetDefault(); 
	m_button10->SetFont( wxFont( 10, 74, 90, 92, false, wxT("Tahoma") ) );
	
	bSizer22->Add( m_button10, 0, wxALL, 5 );
	
	bSizer21->Add( bSizer22, 0, wxEXPAND|wxTOP, 5 );
	
	this->SetSizer( bSizer21 );
	this->Layout();
	bSizer21->Fit( this );
	
	this->Centre( wxBOTH );
	
	// Connect Events
	this->Connect( wxEVT_CLOSE_WINDOW, wxCloseEventHandler( FilterDlgGenerated::OnClose ) );
	m_button9->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( FilterDlgGenerated::OnDefault ), NULL, this );
	m_button17->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( FilterDlgGenerated::OnCancel ), NULL, this );
	m_button10->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( FilterDlgGenerated::OnOK ), NULL, this );
}

FilterDlgGenerated::~FilterDlgGenerated()
{
	// Disconnect Events
	this->Disconnect( wxEVT_CLOSE_WINDOW, wxCloseEventHandler( FilterDlgGenerated::OnClose ) );
	m_button9->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( FilterDlgGenerated::OnDefault ), NULL, this );
	m_button17->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( FilterDlgGenerated::OnCancel ), NULL, this );
	m_button10->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( FilterDlgGenerated::OnOK ), NULL, this );
}

BatchDlgGenerated::BatchDlgGenerated( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : wxDialog( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxDefaultSize, wxDefaultSize );
	
	wxBoxSizer* bSizer54;
	bSizer54 = new wxBoxSizer( wxVERTICAL );
	
	wxBoxSizer* bSizer69;
	bSizer69 = new wxBoxSizer( wxVERTICAL );
	
	m_panel8 = new wxPanel( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxSIMPLE_BORDER|wxTAB_TRAVERSAL );
	m_panel8->SetBackgroundColour( wxSystemSettings::GetColour( wxSYS_COLOUR_3DLIGHT ) );
	
	wxBoxSizer* bSizer72;
	bSizer72 = new wxBoxSizer( wxVERTICAL );
	
	m_staticText56 = new wxStaticText( m_panel8, wxID_ANY, _("Create a batch job"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText56->Wrap( -1 );
	m_staticText56->SetFont( wxFont( 16, 74, 90, 92, false, wxT("Tahoma") ) );
	
	bSizer72->Add( m_staticText56, 0, wxALL|wxALIGN_CENTER_HORIZONTAL, 5 );
	
	m_panel8->SetSizer( bSizer72 );
	m_panel8->Layout();
	bSizer72->Fit( m_panel8 );
	bSizer69->Add( m_panel8, 0, wxALIGN_CENTER_HORIZONTAL, 5 );
	
	
	bSizer69->Add( 0, 5, 0, 0, 5 );
	
	m_staticText54 = new wxStaticText( this, wxID_ANY, _("Assemble a batch file with the following settings. To start synchronization in batch mode simply execute this file or schedule it in your operating system's task planner."), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText54->Wrap( 380 );
	m_staticText54->SetFont( wxFont( wxNORMAL_FONT->GetPointSize(), 70, 90, 90, false, wxT("Tahoma") ) );
	
	bSizer69->Add( m_staticText54, 0, wxALL|wxALIGN_CENTER_HORIZONTAL, 5 );
	
	
	bSizer69->Add( 0, 5, 0, 0, 5 );
	
	m_staticline10 = new wxStaticLine( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	bSizer69->Add( m_staticline10, 0, wxEXPAND|wxTOP, 5 );
	
	m_staticText531 = new wxStaticText( this, wxID_ANY, _("Configuration overview:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText531->Wrap( -1 );
	m_staticText531->SetFont( wxFont( 10, 74, 90, 92, false, wxT("Arial Black") ) );
	
	bSizer69->Add( m_staticText531, 0, wxALL|wxALIGN_CENTER_HORIZONTAL, 5 );
	
	wxStaticBoxSizer* sbSizer20;
	sbSizer20 = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, wxEmptyString ), wxVERTICAL );
	
	wxFlexGridSizer* fgSizer9;
	fgSizer9 = new wxFlexGridSizer( 2, 2, 5, 5 );
	fgSizer9->AddGrowableCol( 1 );
	fgSizer9->SetFlexibleDirection( wxHORIZONTAL );
	fgSizer9->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );
	
	m_staticText53 = new wxStaticText( this, wxID_ANY, _("Left folder:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText53->Wrap( -1 );
	m_staticText53->SetFont( wxFont( 8, 74, 90, 92, false, wxT("Tahoma") ) );
	
	fgSizer9->Add( m_staticText53, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );
	
	m_directoryPanel1 = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer9->Add( m_directoryPanel1, 0, wxALIGN_CENTER_VERTICAL|wxEXPAND, 5 );
	
	m_staticText541 = new wxStaticText( this, wxID_ANY, _("Right folder:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText541->Wrap( -1 );
	m_staticText541->SetFont( wxFont( 8, 74, 90, 92, false, wxT("Tahoma") ) );
	
	fgSizer9->Add( m_staticText541, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );
	
	m_directoryPanel2 = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer9->Add( m_directoryPanel2, 0, wxALIGN_CENTER_VERTICAL|wxEXPAND, 5 );
	
	sbSizer20->Add( fgSizer9, 0, wxEXPAND, 5 );
	
	bSizer69->Add( sbSizer20, 1, wxEXPAND|wxBOTTOM|wxRIGHT|wxLEFT, 5 );
	
	wxBoxSizer* bSizer67;
	bSizer67 = new wxBoxSizer( wxHORIZONTAL );
	
	wxBoxSizer* bSizer57;
	bSizer57 = new wxBoxSizer( wxVERTICAL );
	
	wxBoxSizer* bSizer71;
	bSizer71 = new wxBoxSizer( wxHORIZONTAL );
	
	wxBoxSizer* bSizer721;
	bSizer721 = new wxBoxSizer( wxVERTICAL );
	
	wxStaticBoxSizer* sbSizer6;
	sbSizer6 = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Compare by...") ), wxVERTICAL );
	
	m_radioBtnSizeDate = new wxRadioButton( this, wxID_ANY, _("File size and date"), wxDefaultPosition, wxDefaultSize, 0 );
	m_radioBtnSizeDate->SetValue( true ); 
	m_radioBtnSizeDate->SetToolTip( _("Files are found equal if\n     - filesize\n     - last write time (UTC) and date\nare the same.") );
	
	sbSizer6->Add( m_radioBtnSizeDate, 0, 0, 5 );
	
	m_radioBtnContent = new wxRadioButton( this, wxID_ANY, _("File content"), wxDefaultPosition, wxDefaultSize, 0 );
	m_radioBtnContent->SetToolTip( _("Files are found equal if\n     - file content\nis the same.") );
	
	sbSizer6->Add( m_radioBtnContent, 0, wxTOP, 5 );
	
	bSizer721->Add( sbSizer6, 0, wxALIGN_CENTER_VERTICAL, 5 );
	
	
	bSizer721->Add( 0, 10, 1, 0, 5 );
	
	wxBoxSizer* bSizer38;
	bSizer38 = new wxBoxSizer( wxVERTICAL );
	
	m_checkBoxUseRecycler = new wxCheckBox( this, wxID_ANY, _("Use Recycle Bin"), wxDefaultPosition, wxDefaultSize, 0 );
	
	m_checkBoxUseRecycler->SetToolTip( _("Use Recycle Bin when deleting or overwriting files during synchronization") );
	
	bSizer38->Add( m_checkBoxUseRecycler, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );
	
	m_checkBoxContinueError = new wxCheckBox( this, wxID_ANY, _("Continue on error"), wxDefaultPosition, wxDefaultSize, 0 );
	
	m_checkBoxContinueError->SetToolTip( _("Hides error messages during synchronization:\nThey are collected and shown as a list at the end of the process") );
	
	bSizer38->Add( m_checkBoxContinueError, 0, wxALL, 5 );
	
	m_checkBoxSilent = new wxCheckBox( this, wxID_ANY, _("Silent mode"), wxDefaultPosition, wxDefaultSize, 0 );
	
	m_checkBoxSilent->SetToolTip( _("Do not show graphical status and error messages but write to a logfile instead") );
	
	bSizer38->Add( m_checkBoxSilent, 0, wxALL, 5 );
	
	bSizer721->Add( bSizer38, 0, wxALIGN_CENTER_VERTICAL, 5 );
	
	
	bSizer721->Add( 0, 10, 1, 0, 5 );
	
	bSizer71->Add( bSizer721, 0, wxEXPAND, 5 );
	
	
	bSizer71->Add( 0, 0, 1, wxEXPAND, 5 );
	
	m_bpButtonFilter = new wxBitmapButton( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxSize( 40,40 ), wxBU_AUTODRAW|wxFULL_REPAINT_ON_RESIZE );
	bSizer71->Add( m_bpButtonFilter, 0, wxALIGN_BOTTOM|wxRIGHT, 5 );
	
	bSizer57->Add( bSizer71, 0, wxEXPAND, 5 );
	
	wxStaticBoxSizer* sbSizer8;
	sbSizer8 = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, wxEmptyString ), wxVERTICAL );
	
	wxBoxSizer* bSizer671;
	bSizer671 = new wxBoxSizer( wxVERTICAL );
	
	wxBoxSizer* bSizer681;
	bSizer681 = new wxBoxSizer( wxHORIZONTAL );
	
	m_bitmap8 = new wxStaticBitmap( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxSize( 30,30 ), 0 );
	bSizer681->Add( m_bitmap8, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );
	
	m_staticText15 = new wxStaticText( this, wxID_ANY, _("Include"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText15->Wrap( -1 );
	m_staticText15->SetFont( wxFont( 10, 74, 90, 92, false, wxT("Tahoma") ) );
	
	bSizer681->Add( m_staticText15, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );
	
	bSizer671->Add( bSizer681, 1, wxEXPAND, 5 );
	
	m_textCtrlInclude = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize( 250,-1 ), 0 );
	bSizer671->Add( m_textCtrlInclude, 0, wxALL, 5 );
	
	sbSizer8->Add( bSizer671, 0, 0, 5 );
	
	
	sbSizer8->Add( 0, 10, 0, 0, 5 );
	
	wxBoxSizer* bSizer691;
	bSizer691 = new wxBoxSizer( wxVERTICAL );
	
	wxBoxSizer* bSizer70;
	bSizer70 = new wxBoxSizer( wxHORIZONTAL );
	
	m_bitmap9 = new wxStaticBitmap( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxSize( 30,30 ), 0 );
	bSizer70->Add( m_bitmap9, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );
	
	m_staticText16 = new wxStaticText( this, wxID_ANY, _("Exclude"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText16->Wrap( -1 );
	m_staticText16->SetFont( wxFont( 10, 74, 90, 92, false, wxT("Tahoma") ) );
	
	bSizer70->Add( m_staticText16, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );
	
	bSizer691->Add( bSizer70, 1, wxEXPAND, 5 );
	
	m_textCtrlExclude = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize( 250,-1 ), 0 );
	bSizer691->Add( m_textCtrlExclude, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );
	
	sbSizer8->Add( bSizer691, 0, 0, 5 );
	
	bSizer57->Add( sbSizer8, 0, 0, 5 );
	
	bSizer67->Add( bSizer57, 0, 0, 5 );
	
	
	bSizer67->Add( 10, 0, 0, wxALIGN_CENTER_VERTICAL, 5 );
	
	wxStaticBoxSizer* sbSizer61;
	sbSizer61 = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Configuration") ), wxVERTICAL );
	
	wxGridSizer* gSizer3;
	gSizer3 = new wxGridSizer( 1, 2, 0, 5 );
	
	m_staticText211 = new wxStaticText( this, wxID_ANY, _("Result"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText211->Wrap( -1 );
	m_staticText211->SetFont( wxFont( 8, 74, 90, 92, false, wxT("Tahoma") ) );
	
	gSizer3->Add( m_staticText211, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 5 );
	
	m_staticText311 = new wxStaticText( this, wxID_ANY, _("Action"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText311->Wrap( -1 );
	m_staticText311->SetFont( wxFont( 8, 74, 90, 92, false, wxT("Tahoma") ) );
	
	gSizer3->Add( m_staticText311, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 5 );
	
	sbSizer61->Add( gSizer3, 0, wxEXPAND, 5 );
	
	m_staticline3 = new wxStaticLine( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	sbSizer61->Add( m_staticline3, 0, wxALIGN_CENTER_HORIZONTAL|wxEXPAND|wxTOP|wxBOTTOM, 5 );
	
	wxGridSizer* gSizer1;
	gSizer1 = new wxGridSizer( 5, 2, 0, 5 );
	
	m_bitmap13 = new wxStaticBitmap( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxSize( 48,48 ), 0 );
	m_bitmap13->SetToolTip( _("Files/folders that exist on left side only") );
	
	gSizer1->Add( m_bitmap13, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 5 );
	
	m_bpButton5 = new wxBitmapButton( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxSize( 42,42 ), wxBU_AUTODRAW );
	gSizer1->Add( m_bpButton5, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 5 );
	
	m_bitmap14 = new wxStaticBitmap( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxSize( 48,48 ), 0 );
	m_bitmap14->SetToolTip( _("Files/folders that exist on right side only") );
	
	gSizer1->Add( m_bitmap14, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 5 );
	
	m_bpButton6 = new wxBitmapButton( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxSize( 42,42 ), wxBU_AUTODRAW );
	gSizer1->Add( m_bpButton6, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 5 );
	
	m_bitmap15 = new wxStaticBitmap( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxSize( 48,48 ), 0 );
	m_bitmap15->SetToolTip( _("Files that exist on both sides, left one is newer") );
	
	gSizer1->Add( m_bitmap15, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 5 );
	
	m_bpButton7 = new wxBitmapButton( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxSize( 42,42 ), wxBU_AUTODRAW );
	gSizer1->Add( m_bpButton7, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 5 );
	
	m_bitmap16 = new wxStaticBitmap( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxSize( 48,48 ), 0 );
	m_bitmap16->SetToolTip( _("Files that exist on both sides, right one is newer") );
	
	gSizer1->Add( m_bitmap16, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 5 );
	
	m_bpButton8 = new wxBitmapButton( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxSize( 42,42 ), wxBU_AUTODRAW );
	gSizer1->Add( m_bpButton8, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 5 );
	
	m_bitmap17 = new wxStaticBitmap( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxSize( 48,48 ), 0 );
	m_bitmap17->SetToolTip( _("Files that exist on both sides and are different") );
	
	gSizer1->Add( m_bitmap17, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 5 );
	
	m_bpButton9 = new wxBitmapButton( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxSize( 42,42 ), wxBU_AUTODRAW );
	gSizer1->Add( m_bpButton9, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 5 );
	
	sbSizer61->Add( gSizer1, 0, wxALIGN_CENTER_HORIZONTAL|wxEXPAND, 5 );
	
	bSizer67->Add( sbSizer61, 0, 0, 5 );
	
	bSizer69->Add( bSizer67, 0, wxALL, 5 );
	
	m_staticline9 = new wxStaticLine( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	bSizer69->Add( m_staticline9, 0, wxTOP|wxBOTTOM|wxALIGN_CENTER_HORIZONTAL|wxEXPAND, 5 );
	
	wxBoxSizer* bSizer68;
	bSizer68 = new wxBoxSizer( wxHORIZONTAL );
	
	m_button6 = new wxButton( this, wxID_CANCEL, _("&Cancel"), wxDefaultPosition, wxSize( -1,30 ), 0 );
	m_button6->SetFont( wxFont( 10, 74, 90, 90, false, wxT("Tahoma") ) );
	
	bSizer68->Add( m_button6, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );
	
	m_buttonCreate = new wxButton( this, wxID_ANY, _("&Create"), wxDefaultPosition, wxSize( 120,35 ), 0 );
	m_buttonCreate->SetDefault(); 
	m_buttonCreate->SetFont( wxFont( 10, 74, 90, 92, false, wxT("Tahoma") ) );
	
	bSizer68->Add( m_buttonCreate, 0, wxALL|wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 5 );
	
	bSizer69->Add( bSizer68, 0, wxALIGN_CENTER_HORIZONTAL, 5 );
	
	bSizer54->Add( bSizer69, 0, wxALL|wxALIGN_CENTER_HORIZONTAL, 5 );
	
	this->SetSizer( bSizer54 );
	this->Layout();
	bSizer54->Fit( this );
	
	this->Centre( wxBOTH );
	
	// Connect Events
	this->Connect( wxEVT_CLOSE_WINDOW, wxCloseEventHandler( BatchDlgGenerated::OnClose ) );
	m_directoryPanel1->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( BatchDlgGenerated::OnEnterLeftDir ), NULL, this );
	m_directoryPanel2->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( BatchDlgGenerated::OnEnterRightDir ), NULL, this );
	m_checkBoxUseRecycler->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( BatchDlgGenerated::OnSelectRecycleBin ), NULL, this );
	m_bpButtonFilter->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( BatchDlgGenerated::OnFilterButton ), NULL, this );
	m_bpButton5->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( BatchDlgGenerated::OnExLeftSideOnly ), NULL, this );
	m_bpButton6->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( BatchDlgGenerated::OnExRightSideOnly ), NULL, this );
	m_bpButton7->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( BatchDlgGenerated::OnLeftNewer ), NULL, this );
	m_bpButton8->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( BatchDlgGenerated::OnRightNewer ), NULL, this );
	m_bpButton9->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( BatchDlgGenerated::OnDifferent ), NULL, this );
	m_button6->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( BatchDlgGenerated::OnCancel ), NULL, this );
	m_buttonCreate->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( BatchDlgGenerated::OnCreateJob ), NULL, this );
}

BatchDlgGenerated::~BatchDlgGenerated()
{
	// Disconnect Events
	this->Disconnect( wxEVT_CLOSE_WINDOW, wxCloseEventHandler( BatchDlgGenerated::OnClose ) );
	m_directoryPanel1->Disconnect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( BatchDlgGenerated::OnEnterLeftDir ), NULL, this );
	m_directoryPanel2->Disconnect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( BatchDlgGenerated::OnEnterRightDir ), NULL, this );
	m_checkBoxUseRecycler->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( BatchDlgGenerated::OnSelectRecycleBin ), NULL, this );
	m_bpButtonFilter->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( BatchDlgGenerated::OnFilterButton ), NULL, this );
	m_bpButton5->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( BatchDlgGenerated::OnExLeftSideOnly ), NULL, this );
	m_bpButton6->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( BatchDlgGenerated::OnExRightSideOnly ), NULL, this );
	m_bpButton7->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( BatchDlgGenerated::OnLeftNewer ), NULL, this );
	m_bpButton8->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( BatchDlgGenerated::OnRightNewer ), NULL, this );
	m_bpButton9->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( BatchDlgGenerated::OnDifferent ), NULL, this );
	m_button6->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( BatchDlgGenerated::OnCancel ), NULL, this );
	m_buttonCreate->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( BatchDlgGenerated::OnCreateJob ), NULL, this );
}
