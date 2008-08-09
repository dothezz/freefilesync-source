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

#include "MainDialog.h"

#include "GUI_Generated.h"

///////////////////////////////////////////////////////////////////////////

GUI_Generated::GUI_Generated( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : wxFrame( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxDefaultSize, wxDefaultSize );
	
	bSizer1 = new wxBoxSizer( wxVERTICAL );
	
	wxBoxSizer* bSizer6;
	bSizer6 = new wxBoxSizer( wxHORIZONTAL );
	
	
	bSizer6->Add( 40, 0, 0, wxEXPAND, 5 );
	
	m_bpButton12 = new wxBitmapButton( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxSize( 190,37 ), wxBU_AUTODRAW );
	m_bpButton12->SetToolTip( _("Compare both sides") );
	
	m_bpButton12->SetToolTip( _("Compare both sides") );
	
	bSizer6->Add( m_bpButton12, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );
	
	wxStaticBoxSizer* sbSizer6;
	sbSizer6 = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Compare by") ), wxVERTICAL );
	
	m_radioBtn4 = new wxRadioButton( this, wxID_ANY, _("File size and date"), wxDefaultPosition, wxDefaultSize, 0 );
	m_radioBtn4->SetValue( true ); 
	m_radioBtn4->SetToolTip( _("Files are found equal if\n     - filesize\n     - last write time (UTC) and date\nare the same.") );
	
	sbSizer6->Add( m_radioBtn4, 0, wxRIGHT|wxLEFT, 5 );
	
	m_radioBtn5 = new wxRadioButton( this, wxID_ANY, _("File content"), wxDefaultPosition, wxDefaultSize, 0 );
	m_radioBtn5->SetToolTip( _("Files are found equal if\n     - file content\nis the same.") );
	
	sbSizer6->Add( m_radioBtn5, 0, wxTOP|wxRIGHT|wxLEFT, 5 );
	
	bSizer6->Add( sbSizer6, 0, wxALIGN_CENTER_VERTICAL|wxBOTTOM, 2 );
	
	m_bpButton14 = new wxBitmapButton( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxSize( 40,40 ), wxBU_AUTODRAW );
	bSizer6->Add( m_bpButton14, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );
	
	
	bSizer6->Add( 0, 0, 1, wxEXPAND, 5 );
	
	m_bpButton111 = new wxBitmapButton( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxSize( 190,37 ), wxBU_AUTODRAW );
	m_bpButton111->SetToolTip( _("Open synchronization dialog") );
	
	m_bpButton111->SetToolTip( _("Open synchronization dialog") );
	
	bSizer6->Add( m_bpButton111, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );
	
	
	bSizer6->Add( 40, 0, 0, wxEXPAND, 5 );
	
	bSizer1->Add( bSizer6, 0, wxEXPAND|wxALIGN_CENTER_HORIZONTAL, 5 );
	
	wxBoxSizer* bSizer2;
	bSizer2 = new wxBoxSizer( wxHORIZONTAL );
	
	m_panel1 = new wxPanel( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	wxBoxSizer* bSizer7;
	bSizer7 = new wxBoxSizer( wxVERTICAL );
	
	wxStaticBoxSizer* sbSizer2;
	sbSizer2 = new wxStaticBoxSizer( new wxStaticBox( m_panel1, wxID_ANY, _("Drag && drop") ), wxHORIZONTAL );
	
	m_directoryPanel1 = new wxTextCtrl( m_panel1, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_READONLY );
	m_directoryPanel1->SetBackgroundColour( wxSystemSettings::GetColour( wxSYS_COLOUR_ACTIVEBORDER ) );
	
	sbSizer2->Add( m_directoryPanel1, 1, wxALIGN_BOTTOM, 5 );
	
	m_dirPicker1 = new wxDirPickerCtrl( m_panel1, wxID_ANY, wxEmptyString, _("Select a folder"), wxDefaultPosition, wxDefaultSize, wxDIRP_DIR_MUST_EXIST );
	sbSizer2->Add( m_dirPicker1, 0, wxALIGN_BOTTOM, 5 );
	
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
	bSizer2->Add( m_panel1, 1, wxEXPAND|wxLEFT, 5 );
	
	m_panel3 = new wxPanel( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	wxBoxSizer* bSizer18;
	bSizer18 = new wxBoxSizer( wxVERTICAL );
	
	m_bpButton13 = new wxBitmapButton( m_panel3, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxSize( 40,40 ), wxBU_AUTODRAW );
	m_bpButton13->SetToolTip( _("Swap sides") );
	
	m_bpButton13->SetToolTip( _("Swap sides") );
	
	bSizer18->Add( m_bpButton13, 0, wxTOP|wxALIGN_CENTER_HORIZONTAL, 3 );
	
	m_grid3 = new CustomGrid( m_panel3, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxSUNKEN_BORDER );
	
	// Grid
	m_grid3->CreateGrid( 15, 1 );
	m_grid3->EnableEditing( false );
	m_grid3->EnableGridLines( true );
	m_grid3->EnableDragGridSize( false );
	m_grid3->SetMargins( 0, 50 );
	
	// Columns
	m_grid3->SetColSize( 0, 45 );
	m_grid3->EnableDragColMove( false );
	m_grid3->EnableDragColSize( false );
	m_grid3->SetColLabelSize( 20 );
	m_grid3->SetColLabelValue( 0, _("Result") );
	m_grid3->SetColLabelAlignment( wxALIGN_CENTRE, wxALIGN_CENTRE );
	
	// Rows
	m_grid3->EnableDragRowSize( false );
	m_grid3->SetRowLabelSize( 0 );
	m_grid3->SetRowLabelAlignment( wxALIGN_CENTRE, wxALIGN_CENTRE );
	
	// Label Appearance
	
	// Cell Defaults
	m_grid3->SetDefaultCellFont( wxFont( 12, 74, 90, 92, false, wxT("Arial") ) );
	m_grid3->SetDefaultCellAlignment( wxALIGN_CENTRE, wxALIGN_CENTRE );
	bSizer18->Add( m_grid3, 1, wxBOTTOM|wxTOP, 5 );
	
	m_panel3->SetSizer( bSizer18 );
	m_panel3->Layout();
	bSizer18->Fit( m_panel3 );
	bSizer2->Add( m_panel3, 0, wxEXPAND|wxRIGHT|wxLEFT, 5 );
	
	m_panel2 = new wxPanel( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	wxBoxSizer* bSizer10;
	bSizer10 = new wxBoxSizer( wxVERTICAL );
	
	wxStaticBoxSizer* sbSizer3;
	sbSizer3 = new wxStaticBoxSizer( new wxStaticBox( m_panel2, wxID_ANY, _("Drag && drop") ), wxHORIZONTAL );
	
	m_directoryPanel2 = new wxTextCtrl( m_panel2, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_READONLY );
	m_directoryPanel2->SetBackgroundColour( wxSystemSettings::GetColour( wxSYS_COLOUR_ACTIVEBORDER ) );
	
	sbSizer3->Add( m_directoryPanel2, 1, wxALIGN_BOTTOM, 5 );
	
	m_dirPicker2 = new wxDirPickerCtrl( m_panel2, wxID_ANY, wxEmptyString, _("Select a folder"), wxDefaultPosition, wxDefaultSize, wxDIRP_DIR_MUST_EXIST );
	sbSizer3->Add( m_dirPicker2, 0, wxALIGN_BOTTOM, 5 );
	
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
	bSizer10->Add( m_grid2, 1, wxALL|wxALIGN_CENTER_VERTICAL|wxEXPAND, 5 );
	
	m_panel2->SetSizer( bSizer10 );
	m_panel2->Layout();
	bSizer10->Fit( m_panel2 );
	bSizer2->Add( m_panel2, 1, wxEXPAND|wxRIGHT, 5 );
	
	bSizer1->Add( bSizer2, 1, wxEXPAND, 5 );
	
	m_panel4 = new wxPanel( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	wxBoxSizer* bSizer3;
	bSizer3 = new wxBoxSizer( wxHORIZONTAL );
	
	m_bpButton11 = new wxBitmapButton( m_panel4, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxSize( 50,50 ), wxBU_AUTODRAW );
	m_bpButton11->SetToolTip( _("About") );
	
	m_bpButton11->SetToolTip( _("About") );
	
	bSizer3->Add( m_bpButton11, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );
	
	
	bSizer3->Add( 0, 0, 1, wxEXPAND, 5 );
	
	wxStaticBoxSizer* sbSizer31;
	sbSizer31 = new wxStaticBoxSizer( new wxStaticBox( m_panel4, wxID_ANY, _("Filter") ), wxHORIZONTAL );
	
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
	
	bSizer3->Add( sbSizer31, 0, wxALIGN_CENTER_VERTICAL, 5 );
	
	
	bSizer3->Add( 0, 0, 1, wxEXPAND, 5 );
	
	m_bpButton10 = new wxBitmapButton( m_panel4, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxSize( 50,50 ), wxBU_AUTODRAW );
	m_bpButton10->SetToolTip( _("Quit") );
	
	m_bpButton10->SetToolTip( _("Quit") );
	
	bSizer3->Add( m_bpButton10, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );
	
	m_panel4->SetSizer( bSizer3 );
	m_panel4->Layout();
	bSizer3->Fit( m_panel4 );
	bSizer1->Add( m_panel4, 0, wxALIGN_CENTER_HORIZONTAL|wxEXPAND|wxRIGHT|wxLEFT, 5 );
	
	this->SetSizer( bSizer1 );
	this->Layout();
	m_statusBar1 = this->CreateStatusBar( 5, wxST_SIZEGRIP, wxID_ANY );
	
	// Connect Events
	this->Connect( wxEVT_CLOSE_WINDOW, wxCloseEventHandler( GUI_Generated::OnClose ) );
	m_bpButton12->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( GUI_Generated::OnCompare ), NULL, this );
	m_radioBtn4->Connect( wxEVT_COMMAND_RADIOBUTTON_SELECTED, wxCommandEventHandler( GUI_Generated::OnChangeCompareVariant ), NULL, this );
	m_radioBtn5->Connect( wxEVT_COMMAND_RADIOBUTTON_SELECTED, wxCommandEventHandler( GUI_Generated::OnChangeCompareVariant ), NULL, this );
	m_bpButton14->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( GUI_Generated::OnShowHelpDialog ), NULL, this );
	m_bpButton111->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( GUI_Generated::OnSync ), NULL, this );
	m_dirPicker1->Connect( wxEVT_COMMAND_DIRPICKER_CHANGED, wxFileDirPickerEventHandler( GUI_Generated::OnDirChangedPanel1 ), NULL, this );
	m_grid1->Connect( wxEVT_GRID_CELL_LEFT_DCLICK, wxGridEventHandler( GUI_Generated::OnLeftGridDoubleClick ), NULL, this );
	m_grid1->Connect( wxEVT_GRID_LABEL_LEFT_CLICK, wxGridEventHandler( GUI_Generated::OnSortLeftGrid ), NULL, this );
	m_bpButton13->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( GUI_Generated::OnSwapDirs ), NULL, this );
	m_grid3->Connect( wxEVT_GRID_CELL_LEFT_CLICK, wxGridEventHandler( GUI_Generated::OnDeselectRow ), NULL, this );
	m_dirPicker2->Connect( wxEVT_COMMAND_DIRPICKER_CHANGED, wxFileDirPickerEventHandler( GUI_Generated::OnDirChangedPanel2 ), NULL, this );
	m_grid2->Connect( wxEVT_GRID_CELL_LEFT_DCLICK, wxGridEventHandler( GUI_Generated::OnRightGridDoubleClick ), NULL, this );
	m_grid2->Connect( wxEVT_GRID_LABEL_LEFT_CLICK, wxGridEventHandler( GUI_Generated::OnSortRightGrid ), NULL, this );
	m_bpButton11->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( GUI_Generated::OnAbout ), NULL, this );
	m_bpButton20->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( GUI_Generated::OnLeftOnlyFiles ), NULL, this );
	m_bpButton21->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( GUI_Generated::OnLeftNewerFiles ), NULL, this );
	m_bpButton25->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( GUI_Generated::OnEqualFiles ), NULL, this );
	m_bpButton22->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( GUI_Generated::OnDifferentFiles ), NULL, this );
	m_bpButton23->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( GUI_Generated::OnRightNewerFiles ), NULL, this );
	m_bpButton24->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( GUI_Generated::OnRightOnlyFiles ), NULL, this );
	m_bpButton10->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( GUI_Generated::OnQuit ), NULL, this );
}

GUI_Generated::~GUI_Generated()
{
	// Disconnect Events
	this->Disconnect( wxEVT_CLOSE_WINDOW, wxCloseEventHandler( GUI_Generated::OnClose ) );
	m_bpButton12->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( GUI_Generated::OnCompare ), NULL, this );
	m_radioBtn4->Disconnect( wxEVT_COMMAND_RADIOBUTTON_SELECTED, wxCommandEventHandler( GUI_Generated::OnChangeCompareVariant ), NULL, this );
	m_radioBtn5->Disconnect( wxEVT_COMMAND_RADIOBUTTON_SELECTED, wxCommandEventHandler( GUI_Generated::OnChangeCompareVariant ), NULL, this );
	m_bpButton14->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( GUI_Generated::OnShowHelpDialog ), NULL, this );
	m_bpButton111->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( GUI_Generated::OnSync ), NULL, this );
	m_dirPicker1->Disconnect( wxEVT_COMMAND_DIRPICKER_CHANGED, wxFileDirPickerEventHandler( GUI_Generated::OnDirChangedPanel1 ), NULL, this );
	m_grid1->Disconnect( wxEVT_GRID_CELL_LEFT_DCLICK, wxGridEventHandler( GUI_Generated::OnLeftGridDoubleClick ), NULL, this );
	m_grid1->Disconnect( wxEVT_GRID_LABEL_LEFT_CLICK, wxGridEventHandler( GUI_Generated::OnSortLeftGrid ), NULL, this );
	m_bpButton13->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( GUI_Generated::OnSwapDirs ), NULL, this );
	m_grid3->Disconnect( wxEVT_GRID_CELL_LEFT_CLICK, wxGridEventHandler( GUI_Generated::OnDeselectRow ), NULL, this );
	m_dirPicker2->Disconnect( wxEVT_COMMAND_DIRPICKER_CHANGED, wxFileDirPickerEventHandler( GUI_Generated::OnDirChangedPanel2 ), NULL, this );
	m_grid2->Disconnect( wxEVT_GRID_CELL_LEFT_DCLICK, wxGridEventHandler( GUI_Generated::OnRightGridDoubleClick ), NULL, this );
	m_grid2->Disconnect( wxEVT_GRID_LABEL_LEFT_CLICK, wxGridEventHandler( GUI_Generated::OnSortRightGrid ), NULL, this );
	m_bpButton11->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( GUI_Generated::OnAbout ), NULL, this );
	m_bpButton20->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( GUI_Generated::OnLeftOnlyFiles ), NULL, this );
	m_bpButton21->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( GUI_Generated::OnLeftNewerFiles ), NULL, this );
	m_bpButton25->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( GUI_Generated::OnEqualFiles ), NULL, this );
	m_bpButton22->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( GUI_Generated::OnDifferentFiles ), NULL, this );
	m_bpButton23->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( GUI_Generated::OnRightNewerFiles ), NULL, this );
	m_bpButton24->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( GUI_Generated::OnRightOnlyFiles ), NULL, this );
	m_bpButton10->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( GUI_Generated::OnQuit ), NULL, this );
}

SyncDialogGenerated::SyncDialogGenerated( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : wxDialog( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxDefaultSize, wxDefaultSize );
	
	wxBoxSizer* bSizer7;
	bSizer7 = new wxBoxSizer( wxVERTICAL );
	
	wxBoxSizer* bSizer181;
	bSizer181 = new wxBoxSizer( wxHORIZONTAL );
	
	
	bSizer181->Add( 5, 0, 0, wxEXPAND, 5 );
	
	wxBoxSizer* bSizer29;
	bSizer29 = new wxBoxSizer( wxVERTICAL );
	
	
	bSizer29->Add( 0, 5, 0, wxEXPAND, 5 );
	
	wxBoxSizer* bSizer201;
	bSizer201 = new wxBoxSizer( wxHORIZONTAL );
	
	m_bpButton18 = new wxBitmapButton( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxSize( 140,58 ), wxBU_AUTODRAW );
	m_bpButton18->SetToolTip( _("Start synchronizing files") );
	
	m_bpButton18->SetToolTip( _("Start synchronizing files") );
	
	bSizer201->Add( m_bpButton18, 0, wxRIGHT, 5 );
	
	wxBoxSizer* bSizer211;
	bSizer211 = new wxBoxSizer( wxHORIZONTAL );
	
	m_staticText14 = new wxStaticText( this, wxID_ANY, _("Data to be transferred:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText14->Wrap( -1 );
	m_staticText14->SetFont( wxFont( 10, 74, 93, 90, false, wxT("Tahoma") ) );
	
	bSizer211->Add( m_staticText14, 0, wxALIGN_CENTER_HORIZONTAL|wxALL|wxALIGN_CENTER_VERTICAL, 5 );
	
	m_textCtrl5 = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize( 55,-1 ), wxTE_READONLY );
	m_textCtrl5->SetBackgroundColour( wxSystemSettings::GetColour( wxSYS_COLOUR_ACTIVEBORDER ) );
	
	bSizer211->Add( m_textCtrl5, 0, wxALL|wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 5 );
	
	bSizer201->Add( bSizer211, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );
	
	bSizer29->Add( bSizer201, 0, wxEXPAND, 5 );
	
	
	bSizer29->Add( 0, 3, 0, wxEXPAND, 5 );
	
	wxBoxSizer* bSizer23;
	bSizer23 = new wxBoxSizer( wxHORIZONTAL );
	
	wxStaticBoxSizer* sbSizer7;
	sbSizer7 = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, wxEmptyString ), wxVERTICAL );
	
	m_staticText1 = new wxStaticText( this, wxID_ANY, _("Select variant:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText1->Wrap( -1 );
	m_staticText1->SetFont( wxFont( 10, 74, 90, 92, false, wxT("Tahoma") ) );
	
	sbSizer7->Add( m_staticText1, 0, wxALL, 5 );
	
	wxFlexGridSizer* fgSizer1;
	fgSizer1 = new wxFlexGridSizer( 3, 3, 0, 0 );
	fgSizer1->SetFlexibleDirection( wxBOTH );
	fgSizer1->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );
	
	m_radioBtn1 = new wxRadioButton( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_radioBtn1->SetValue( true ); 
	m_radioBtn1->SetFont( wxFont( 11, 74, 90, 92, false, wxT("Tahoma") ) );
	
	fgSizer1->Add( m_radioBtn1, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );
	
	m_button5 = new wxButton( this, wxID_ANY, _("Left to right ->"), wxDefaultPosition, wxSize( 130,-1 ), 0 );
	m_button5->SetFont( wxFont( 11, 74, 90, 92, false, wxT("Tahoma") ) );
	
	fgSizer1->Add( m_button5, 0, wxALIGN_CENTER_VERTICAL|wxTOP|wxBOTTOM|wxRIGHT, 5 );
	
	m_staticText8 = new wxStaticText( this, wxID_ANY, _("Mirror backup of left folder: Right folder will be overwritten and exactly match left folder after synchronization."), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText8->Wrap( 200 );
	fgSizer1->Add( m_staticText8, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );
	
	m_radioBtn2 = new wxRadioButton( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_radioBtn2->SetFont( wxFont( 11, 74, 90, 92, false, wxT("Tahoma") ) );
	
	fgSizer1->Add( m_radioBtn2, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );
	
	m_button61 = new wxButton( this, wxID_ANY, _("Both sides <->"), wxDefaultPosition, wxSize( 130,-1 ), 0 );
	m_button61->SetFont( wxFont( 11, 74, 90, 92, false, wxT("Tahoma") ) );
	
	fgSizer1->Add( m_button61, 0, wxALIGN_CENTER_VERTICAL|wxTOP|wxBOTTOM|wxRIGHT, 5 );
	
	m_staticText10 = new wxStaticText( this, wxID_ANY, _("Synchronize both sides simultaneously: Copy newer files or files that exist only in one folder to the other."), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText10->Wrap( 200 );
	fgSizer1->Add( m_staticText10, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );
	
	m_radioBtn3 = new wxRadioButton( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_radioBtn3->SetFont( wxFont( 11, 74, 90, 92, false, wxT("Tahoma") ) );
	
	fgSizer1->Add( m_radioBtn3, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );
	
	m_button7 = new wxButton( this, wxID_ANY, _("Custom"), wxDefaultPosition, wxSize( 130,-1 ), 0 );
	m_button7->SetFont( wxFont( 11, 74, 90, 92, false, wxT("Tahoma") ) );
	
	fgSizer1->Add( m_button7, 0, wxALIGN_CENTER_VERTICAL|wxTOP|wxBOTTOM|wxRIGHT, 5 );
	
	m_staticText9 = new wxStaticText( this, wxID_ANY, _("Configure your own synchronization rules."), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText9->Wrap( 200 );
	fgSizer1->Add( m_staticText9, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );
	
	sbSizer7->Add( fgSizer1, 1, wxEXPAND, 5 );
	
	bSizer23->Add( sbSizer7, 0, wxEXPAND, 5 );
	
	
	bSizer23->Add( 0, 0, 1, wxEXPAND, 5 );
	
	bSizer29->Add( bSizer23, 0, wxEXPAND, 5 );
	
	
	bSizer29->Add( 0, 0, 1, wxEXPAND, 5 );
	
	m_button6 = new wxButton( this, wxID_ANY, _("Back"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizer29->Add( m_button6, 0, wxBOTTOM|wxLEFT, 5 );
	
	bSizer181->Add( bSizer29, 1, wxEXPAND, 5 );
	
	wxBoxSizer* bSizer30;
	bSizer30 = new wxBoxSizer( wxVERTICAL );
	
	
	bSizer30->Add( 0, 0, 1, wxEXPAND, 5 );
	
	wxStaticBoxSizer* sbSizer6;
	sbSizer6 = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Configuration") ), wxVERTICAL );
	
	wxBoxSizer* bSizer21;
	bSizer21 = new wxBoxSizer( wxHORIZONTAL );
	
	m_staticText2 = new wxStaticText( this, wxID_ANY, _("Result"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText2->Wrap( -1 );
	m_staticText2->SetFont( wxFont( 8, 74, 90, 92, false, wxT("Tahoma") ) );
	
	bSizer21->Add( m_staticText2, 1, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 5 );
	
	
	bSizer21->Add( 15, 0, 0, wxEXPAND|wxALIGN_CENTER_VERTICAL, 5 );
	
	m_staticText3 = new wxStaticText( this, wxID_ANY, _("Action"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText3->Wrap( -1 );
	m_staticText3->SetFont( wxFont( 8, 74, 90, 92, false, wxT("Tahoma") ) );
	
	bSizer21->Add( m_staticText3, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 5 );
	
	sbSizer6->Add( bSizer21, 0, wxALIGN_CENTER_HORIZONTAL, 5 );
	
	m_staticline3 = new wxStaticLine( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	sbSizer6->Add( m_staticline3, 0, wxALL|wxALIGN_CENTER_HORIZONTAL|wxEXPAND, 5 );
	
	wxBoxSizer* bSizer9;
	bSizer9 = new wxBoxSizer( wxHORIZONTAL );
	
	wxBoxSizer* bSizer20;
	bSizer20 = new wxBoxSizer( wxVERTICAL );
	
	m_bitmap13 = new wxStaticBitmap( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxSize( 48,48 ), 0 );
	m_bitmap13->SetToolTip( _("Folders/files that exist on left side only") );
	
	bSizer20->Add( m_bitmap13, 0, wxALIGN_CENTER_HORIZONTAL, 5 );
	
	m_bitmap14 = new wxStaticBitmap( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxSize( 48,48 ), 0 );
	m_bitmap14->SetToolTip( _("Folders/files that exist on right side only") );
	
	bSizer20->Add( m_bitmap14, 0, wxALIGN_CENTER_HORIZONTAL, 5 );
	
	m_bitmap15 = new wxStaticBitmap( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxSize( 48,48 ), 0 );
	m_bitmap15->SetToolTip( _("Folders/files that exist on both sides, left one is newer") );
	
	bSizer20->Add( m_bitmap15, 0, wxALIGN_CENTER_HORIZONTAL, 5 );
	
	m_bitmap16 = new wxStaticBitmap( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxSize( 48,48 ), 0 );
	m_bitmap16->SetToolTip( _("Folders/files that exist on both sides, right one is newer") );
	
	bSizer20->Add( m_bitmap16, 0, wxALIGN_CENTER_HORIZONTAL, 5 );
	
	m_bitmap17 = new wxStaticBitmap( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxSize( 48,48 ), 0 );
	m_bitmap17->SetToolTip( _("Folders/files that exist on both sides and are different") );
	
	bSizer20->Add( m_bitmap17, 0, wxALIGN_CENTER_HORIZONTAL, 5 );
	
	bSizer9->Add( bSizer20, 0, wxALIGN_CENTER_VERTICAL, 5 );
	
	
	bSizer9->Add( 5, 0, 0, wxEXPAND, 5 );
	
	wxBoxSizer* bSizer19;
	bSizer19 = new wxBoxSizer( wxVERTICAL );
	
	m_bpButton5 = new wxBitmapButton( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxSize( 42,42 ), wxBU_AUTODRAW );
	bSizer19->Add( m_bpButton5, 0, wxALIGN_CENTER_HORIZONTAL|wxRIGHT|wxLEFT, 5 );
	
	
	bSizer19->Add( 0, 5, 0, wxEXPAND, 5 );
	
	m_bpButton6 = new wxBitmapButton( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxSize( 42,42 ), wxBU_AUTODRAW );
	bSizer19->Add( m_bpButton6, 0, wxRIGHT|wxLEFT|wxALIGN_CENTER_HORIZONTAL, 5 );
	
	
	bSizer19->Add( 0, 5, 1, wxEXPAND, 5 );
	
	m_bpButton7 = new wxBitmapButton( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxSize( 42,42 ), wxBU_AUTODRAW );
	bSizer19->Add( m_bpButton7, 0, wxALIGN_CENTER_HORIZONTAL|wxRIGHT|wxLEFT, 5 );
	
	
	bSizer19->Add( 0, 5, 1, wxEXPAND, 5 );
	
	m_bpButton8 = new wxBitmapButton( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxSize( 42,42 ), wxBU_AUTODRAW );
	bSizer19->Add( m_bpButton8, 0, wxRIGHT|wxLEFT|wxALIGN_CENTER_HORIZONTAL, 5 );
	
	
	bSizer19->Add( 0, 5, 1, wxEXPAND, 5 );
	
	m_bpButton9 = new wxBitmapButton( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxSize( 42,42 ), wxBU_AUTODRAW );
	bSizer19->Add( m_bpButton9, 0, wxRIGHT|wxLEFT|wxALIGN_CENTER_HORIZONTAL, 5 );
	
	bSizer9->Add( bSizer19, 0, wxALIGN_CENTER_VERTICAL, 5 );
	
	
	bSizer9->Add( 5, 0, 0, wxEXPAND, 5 );
	
	sbSizer6->Add( bSizer9, 0, wxALIGN_CENTER_HORIZONTAL, 5 );
	
	bSizer30->Add( sbSizer6, 0, wxALIGN_RIGHT|wxBOTTOM, 5 );
	
	bSizer181->Add( bSizer30, 0, wxEXPAND|wxALIGN_BOTTOM, 5 );
	
	
	bSizer181->Add( 5, 0, 0, wxEXPAND, 5 );
	
	bSizer7->Add( bSizer181, 1, wxEXPAND, 5 );
	
	this->SetSizer( bSizer7 );
	this->Layout();
	
	// Connect Events
	this->Connect( wxEVT_CLOSE_WINDOW, wxCloseEventHandler( SyncDialogGenerated::OnClose ) );
	m_bpButton18->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( SyncDialogGenerated::OnStartSync ), NULL, this );
	m_radioBtn1->Connect( wxEVT_COMMAND_RADIOBUTTON_SELECTED, wxCommandEventHandler( SyncDialogGenerated::OnSyncLeftToRight ), NULL, this );
	m_button5->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( SyncDialogGenerated::OnSyncLeftToRight ), NULL, this );
	m_radioBtn2->Connect( wxEVT_COMMAND_RADIOBUTTON_SELECTED, wxCommandEventHandler( SyncDialogGenerated::OnSyncBothSides ), NULL, this );
	m_button61->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( SyncDialogGenerated::OnSyncBothSides ), NULL, this );
	m_radioBtn3->Connect( wxEVT_COMMAND_RADIOBUTTON_SELECTED, wxCommandEventHandler( SyncDialogGenerated::OnSyncCostum ), NULL, this );
	m_button7->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( SyncDialogGenerated::OnSyncCostum ), NULL, this );
	m_button6->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( SyncDialogGenerated::OnBack ), NULL, this );
	m_bpButton5->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( SyncDialogGenerated::OnExLeftSideOnly ), NULL, this );
	m_bpButton6->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( SyncDialogGenerated::OnExRightSideOnly ), NULL, this );
	m_bpButton7->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( SyncDialogGenerated::OnLeftNewer ), NULL, this );
	m_bpButton8->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( SyncDialogGenerated::OnRightNewer ), NULL, this );
	m_bpButton9->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( SyncDialogGenerated::OnDifferent ), NULL, this );
}

SyncDialogGenerated::~SyncDialogGenerated()
{
	// Disconnect Events
	this->Disconnect( wxEVT_CLOSE_WINDOW, wxCloseEventHandler( SyncDialogGenerated::OnClose ) );
	m_bpButton18->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( SyncDialogGenerated::OnStartSync ), NULL, this );
	m_radioBtn1->Disconnect( wxEVT_COMMAND_RADIOBUTTON_SELECTED, wxCommandEventHandler( SyncDialogGenerated::OnSyncLeftToRight ), NULL, this );
	m_button5->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( SyncDialogGenerated::OnSyncLeftToRight ), NULL, this );
	m_radioBtn2->Disconnect( wxEVT_COMMAND_RADIOBUTTON_SELECTED, wxCommandEventHandler( SyncDialogGenerated::OnSyncBothSides ), NULL, this );
	m_button61->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( SyncDialogGenerated::OnSyncBothSides ), NULL, this );
	m_radioBtn3->Disconnect( wxEVT_COMMAND_RADIOBUTTON_SELECTED, wxCommandEventHandler( SyncDialogGenerated::OnSyncCostum ), NULL, this );
	m_button7->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( SyncDialogGenerated::OnSyncCostum ), NULL, this );
	m_button6->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( SyncDialogGenerated::OnBack ), NULL, this );
	m_bpButton5->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( SyncDialogGenerated::OnExLeftSideOnly ), NULL, this );
	m_bpButton6->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( SyncDialogGenerated::OnExRightSideOnly ), NULL, this );
	m_bpButton7->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( SyncDialogGenerated::OnLeftNewer ), NULL, this );
	m_bpButton8->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( SyncDialogGenerated::OnRightNewer ), NULL, this );
	m_bpButton9->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( SyncDialogGenerated::OnDifferent ), NULL, this );
}

HelpDlgGenerated::HelpDlgGenerated( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : wxDialog( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxDefaultSize, wxDefaultSize );
	
	wxBoxSizer* bSizer20;
	bSizer20 = new wxBoxSizer( wxVERTICAL );
	
	
	bSizer20->Add( 0, 10, 0, wxEXPAND, 5 );
	
	m_staticText12 = new wxStaticText( this, wxID_ANY, _("Help"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText12->Wrap( -1 );
	m_staticText12->SetFont( wxFont( 16, 74, 90, 92, false, wxT("Tahoma") ) );
	
	bSizer20->Add( m_staticText12, 0, wxALIGN_CENTER_HORIZONTAL|wxRIGHT|wxLEFT, 5 );
	
	
	bSizer20->Add( 0, 5, 0, wxEXPAND, 5 );
	
	m_textCtrl8 = new wxTextCtrl( this, wxID_ANY, _("Compare by \"File size and date\"\n----------------------------------------\nThis compare variant evaluates two equally named files as being equal when they have the same file size AND the same last write date and time. For the latter the system's UTC time (coordinated word time) is used internally (although the local file time is displayed on the result list). So there are no problems concerning different time zones or daylight saving time.\n\nWhen \"Compare\" is triggered with this option set the following decision tree is processed:\n\n                    -----------------\n                    |Decision tree|\n                    -----------------\n               ________|___________\n               |                                      |\n file exists on both sides     on one side only\n     _____|______                      __|___\n     |                     |                     |          |\nequal             different            left      right\n          _________|_____________\n         |                  |                         |\n  left newer  right newer  different (but same date)\n\nAs a result 6 different status can be returned to categorize all files:\n\n- exists left only\n- exists right only\n- left newer\n- right newer\n- different (but same date)\n- equal\n\n\nCompare by \"File content\"\n----------------------------------------\nAs the name suggests, two files which share the same name are marked as equal if and only if they have the same content. This option is useful for consistency checks rather than backup operations. Therefore the file times are not taken into account at all.\n\nWith this option enabled the decision tree is smaller:\n\n                    -----------------\n                    |Decision tree|\n                    -----------------\n               ________|___________\n               |                                      |\n file exists on both sides     on one side only\n     _____|______                      __|___\n     |                     |                     |          |\nequal             different            left      right\n\nAs a result the files are separated into the following categories:\n\n- exists left only\n- exists right only\n- different\n- equal"), wxDefaultPosition, wxDefaultSize, wxTE_MULTILINE|wxTE_READONLY );
	m_textCtrl8->SetBackgroundColour( wxSystemSettings::GetColour( wxSYS_COLOUR_ACTIVEBORDER ) );
	
	bSizer20->Add( m_textCtrl8, 1, wxALL|wxALIGN_CENTER_HORIZONTAL|wxEXPAND, 5 );
	
	m_button8 = new wxButton( this, wxID_ANY, _("OK"), wxDefaultPosition, wxDefaultSize, 0 );
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
	
	
	bSizer31->Add( 0, 10, 0, wxEXPAND, 5 );
	
	m_staticText14 = new wxStaticText( this, wxID_ANY, _("FreeFileSync v1.0"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText14->Wrap( -1 );
	m_staticText14->SetFont( wxFont( 16, 74, 90, 92, false, wxT("Tahoma") ) );
	
	bSizer31->Add( m_staticText14, 0, wxALL|wxALIGN_CENTER_HORIZONTAL, 5 );
	
	m_staticText15 = new wxStaticText( this, wxID_ANY, _("-Open-Source file synchronization-"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText15->Wrap( -1 );
	m_staticText15->SetFont( wxFont( 8, 74, 93, 90, false, wxT("Tahoma") ) );
	
	bSizer31->Add( m_staticText15, 0, wxALIGN_CENTER_HORIZONTAL|wxBOTTOM|wxRIGHT|wxLEFT, 5 );
	
	
	bSizer31->Add( 0, 10, 0, wxEXPAND, 5 );
	
	m_textCtrl3 = new wxTextCtrl( this, wxID_ANY, _("Source code written completely in C++ utilizing:\n\nMinGW \t\t- Windows port of GNU Compiler Collection\nwxWidgets \t- Open-Source GUI framework\nwxFormBuilder\t- wxWidgets GUI-builder\nGMP \t\t- arithmetic calculations\nCodeBlocks \t- Open-Source IDE\n\n   by ZenJu - August 2008"), wxDefaultPosition, wxSize( 350,140 ), wxTE_MULTILINE|wxTE_NO_VSCROLL|wxTE_READONLY|wxDOUBLE_BORDER );
	m_textCtrl3->SetBackgroundColour( wxSystemSettings::GetColour( wxSYS_COLOUR_ACTIVEBORDER ) );
	
	bSizer31->Add( m_textCtrl3, 0, wxALL|wxALIGN_CENTER_HORIZONTAL, 0 );
	
	
	bSizer31->Add( 0, 0, 1, wxEXPAND, 5 );
	
	wxStaticBoxSizer* sbSizer7;
	sbSizer7 = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, wxEmptyString ), wxVERTICAL );
	
	m_staticText131 = new wxStaticText( this, wxID_ANY, _("Feedback and suggestions are welcome at:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText131->Wrap( -1 );
	m_staticText131->SetFont( wxFont( 11, 74, 90, 90, false, wxT("Tahoma") ) );
	
	sbSizer7->Add( m_staticText131, 0, wxALIGN_CENTER_HORIZONTAL, 5 );
	
	bSizer31->Add( sbSizer7, 0, wxALIGN_CENTER_HORIZONTAL, 5 );
	
	wxFlexGridSizer* fgSizer2;
	fgSizer2 = new wxFlexGridSizer( 3, 3, 0, 0 );
	fgSizer2->SetFlexibleDirection( wxBOTH );
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
	
	m_animationControl1 = new wxAnimationCtrl(this, wxID_ANY, wxNullAnimation, wxDefaultPosition, wxSize( 48,48 ));
	fgSizer2->Add( m_animationControl1, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );
	
	m_staticText151 = new wxStaticText( this, wxID_ANY, _("If you like FFS:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText151->Wrap( -1 );
	m_staticText151->SetFont( wxFont( 10, 74, 90, 92, false, wxT("Tahoma") ) );
	
	fgSizer2->Add( m_staticText151, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );
	
	m_hyperlink3 = new wxHyperlinkCtrl( this, wxID_ANY, _("Donate with Paypal"), wxT("https://www.paypal.com/cgi-bin/webscr?cmd=_donations&business=florian%2db%40gmx%2enet&no_shipping=0&no_note=1&tax=0&currency_code=EUR&lc=DE&bn=PP%2dDonationsBF&charset=UTF%2d8"), wxDefaultPosition, wxDefaultSize, wxHL_DEFAULT_STYLE );
	fgSizer2->Add( m_hyperlink3, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );
	
	bSizer31->Add( fgSizer2, 0, wxEXPAND|wxLEFT, 10 );
	
	m_button8 = new wxButton( this, wxID_ANY, _("OK"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizer31->Add( m_button8, 0, wxALIGN_CENTER_HORIZONTAL|wxBOTTOM|wxRIGHT|wxLEFT, 5 );
	
	this->SetSizer( bSizer31 );
	this->Layout();
	
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
