///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 3.10.1-0-g8feb16b3)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "pch.h"

#include "NWScriptForms.h"

///////////////////////////////////////////////////////////////////////////
using namespace NWScriptPlugin;

wxAboutDialog::wxAboutDialog( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : wxDialog( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxSize( 656,550 ), wxDefaultSize );
	this->SetFont( wxFont( 10, wxFONTFAMILY_MODERN, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL, false, wxT("Cascadia Mono") ) );

	wxBoxSizer* bSizer1;
	bSizer1 = new wxBoxSizer( wxVERTICAL );

	wxBoxSizer* bSizer5;
	bSizer5 = new wxBoxSizer( wxHORIZONTAL );

	bmpLogo = new wxStaticBitmap( this, wxIDB_NWSCRIPTLOGO, wxNullBitmap, wxDefaultPosition, wxDefaultSize, 0 );
	bSizer5->Add( bmpLogo, 0, wxALL, 5 );

	wxBoxSizer* bSizer10;
	bSizer10 = new wxBoxSizer( wxVERTICAL );

	lblTitle = new wxStaticText( this, wxID_ANY, wxT("NWScript Tools for Notepad++"), wxDefaultPosition, wxDefaultSize, 0 );
	lblTitle->Wrap( -1 );
	lblTitle->SetFont( wxFont( 12, wxFONTFAMILY_MODERN, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD, false, wxT("Cascadia Mono") ) );

	bSizer10->Add( lblTitle, 0, wxALIGN_CENTER|wxALL|wxFIXED_MINSIZE, 5 );

	wxBoxSizer* bSizer11;
	bSizer11 = new wxBoxSizer( wxVERTICAL );

	lblVersion = new wxStaticText( this, wxIDC_LBLVERSION, wxT("(Version X.X.X build XXXX)"), wxDefaultPosition, wxDefaultSize, 0 );
	lblVersion->Wrap( -1 );
	lblVersion->SetFont( wxFont( 10, wxFONTFAMILY_MODERN, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_SEMIBOLD, false, wxT("Cascadia Mono SemiBold") ) );

	bSizer11->Add( lblVersion, 0, wxALIGN_CENTER|wxFIXED_MINSIZE|wxTOP, 15 );

	lblCopyright = new wxStaticText( this, wxIDC_LBLCOPYRIGHT, wxT("Copyright (C) 2022 - Leonardo Silva"), wxDefaultPosition, wxDefaultSize, 0 );
	lblCopyright->Wrap( -1 );
	lblCopyright->SetFont( wxFont( 10, wxFONTFAMILY_MODERN, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL, false, wxT("Cascadia Mono") ) );

	bSizer11->Add( lblCopyright, 0, wxALIGN_CENTER|wxALL|wxFIXED_MINSIZE, 10 );

	lblSpecialThanks = new wxStaticText( this, wxID_ANY, wxT("Special Thanks:"), wxDefaultPosition, wxDefaultSize, 0 );
	lblSpecialThanks->Wrap( -1 );
	lblSpecialThanks->SetFont( wxFont( 10, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD, false, wxEmptyString ) );

	bSizer11->Add( lblSpecialThanks, 0, wxALIGN_CENTER|wxALL|wxFIXED_MINSIZE, 10 );

	lblSpecialThanksText = new wxStaticText( this, wxID_ANY, wxT("Don HO for Notepad++ project\nNeil Hodgson for the Scintilla(R) C/C++ Lexer\nNWScript Compiler Library Team\r\n@Alort, @Deid, @Ikeoh, @Mom (moral support)"), wxDefaultPosition, wxDefaultSize, wxALIGN_CENTER_HORIZONTAL );
	lblSpecialThanksText->Wrap( -1 );
	lblSpecialThanksText->SetFont( wxFont( 10, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL, false, wxEmptyString ) );

	bSizer11->Add( lblSpecialThanksText, 0, wxALIGN_CENTER|wxALL|wxFIXED_MINSIZE, 5 );


	bSizer10->Add( bSizer11, 0, wxEXPAND, 5 );


	bSizer5->Add( bSizer10, 1, 0, 5 );


	bSizer1->Add( bSizer5, 0, wxEXPAND|wxLEFT|wxTOP, 10 );

	wxBoxSizer* bSizer6;
	bSizer6 = new wxBoxSizer( wxHORIZONTAL );

	lblHomepage = new wxStaticText( this, wxID_ANY, wxT("Homepage:"), wxDefaultPosition, wxDefaultSize, 0 );
	lblHomepage->Wrap( -1 );
	lblHomepage->SetFont( wxFont( 10, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL, false, wxT("Arial Rounded MT Bold") ) );

	bSizer6->Add( lblHomepage, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxFIXED_MINSIZE|wxLEFT, 5 );

	lnkHomepage = new wxHyperlinkCtrl( this, wxIDC_LNKHOMEPAGE, wxT("https://github.com/Leonard-The-Wise/NWScript-Npp"), wxT("https://github.com/Leonard-The-Wise/NWScript-Npp"), wxDefaultPosition, wxSize( -1,-1 ), wxHL_DEFAULT_STYLE );
	lnkHomepage->SetFont( wxFont( 10, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD, false, wxEmptyString ) );

	bSizer6->Add( lnkHomepage, 1, wxALIGN_CENTER_VERTICAL|wxALL, 5 );


	bSizer1->Add( bSizer6, 0, wxALIGN_CENTER, 10 );

	wxBoxSizer* bSizer7;
	bSizer7 = new wxBoxSizer( wxVERTICAL );

	txtAbout = new wxRichTextCtrl( this, wxIDC_TXTABOUT, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_AUTO_URL|wxTE_READONLY|wxHSCROLL|wxVSCROLL );
	bSizer7->Add( txtAbout, 1, wxEXPAND | wxALL, 5 );


	bSizer1->Add( bSizer7, 1, wxEXPAND|wxLEFT|wxRIGHT, 10 );

	wxBoxSizer* bSizer8;
	bSizer8 = new wxBoxSizer( wxVERTICAL );

	btOk = new wxButton( this, wxIDOK, wxT("OK"), wxDefaultPosition, wxDefaultSize, 0 );

	btOk->SetDefault();
	btOk->SetMinSize( wxSize( 110,27 ) );

	bSizer8->Add( btOk, 0, wxALIGN_CENTER|wxBOTTOM|wxTOP, 10 );


	bSizer1->Add( bSizer8, 0, wxALIGN_CENTER, 5 );


	this->SetSizer( bSizer1 );
	this->Layout();

	this->Centre( wxBOTH );

	// Connect Events
	this->Connect( wxEVT_KEY_UP, wxKeyEventHandler( wxAboutDialog::OnKeyPress ) );
	btOk->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( wxAboutDialog::OnBtOkClose ), NULL, this );
}

wxAboutDialog::~wxAboutDialog()
{
	// Disconnect Events
	this->Disconnect( wxEVT_KEY_UP, wxKeyEventHandler( wxAboutDialog::OnKeyPress ) );
	btOk->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( wxAboutDialog::OnBtOkClose ), NULL, this );

}

wxConsoleWindow::wxConsoleWindow( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : wxDialog( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxDefaultSize, wxDefaultSize );

	wxBoxSizer* bSizer8;
	bSizer8 = new wxBoxSizer( wxVERTICAL );

	wxBoxSizer* bSizer9;
	bSizer9 = new wxBoxSizer( wxVERTICAL );

	tabConsole = new wxNotebook( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxNB_BOTTOM|wxBORDER_DEFAULT|wxFULL_REPAINT_ON_RESIZE );

	bSizer9->Add( tabConsole, 1, wxEXPAND | wxALL, 3 );


	bSizer8->Add( bSizer9, 1, wxBOTTOM|wxEXPAND|wxTOP, 3 );


	this->SetSizer( bSizer8 );
	this->Layout();

	this->Centre( wxBOTH );

	// Connect Events
	tabConsole->Connect( wxEVT_COMMAND_NOTEBOOK_PAGE_CHANGED, wxNotebookEventHandler( wxConsoleWindow::OnPageChanged ), NULL, this );
}

wxConsoleWindow::~wxConsoleWindow()
{
	// Disconnect Events
	tabConsole->Disconnect( wxEVT_COMMAND_NOTEBOOK_PAGE_CHANGED, wxNotebookEventHandler( wxConsoleWindow::OnPageChanged ), NULL, this );

}

wxErrorsPanel::wxErrorsPanel( wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style, const wxString& name ) : wxPanel( parent, id, pos, size, style, name )
{
	wxBoxSizer* bSizer10;
	bSizer10 = new wxBoxSizer( wxVERTICAL );

	wxBoxSizer* bSizer11;
	bSizer11 = new wxBoxSizer( wxVERTICAL );

	wxBoxSizer* bSizer12;
	bSizer12 = new wxBoxSizer( wxHORIZONTAL );

	btShowErrors = new wxToggleButton( this, wxIDTOGGLEERRORS, wxT("(0) Errors"), wxDefaultPosition, wxDefaultSize, 0 );

	btShowErrors->SetBitmap( wxNullBitmap );
	btShowErrors->SetMinSize( wxSize( 120,27 ) );

	bSizer12->Add( btShowErrors, 0, wxALL, 5 );

	btShowWarnings = new wxToggleButton( this, wxIDTOGGLEWARNINGS, wxT("(0) Warnings"), wxDefaultPosition, wxDefaultSize, 0 );
	btShowWarnings->SetMinSize( wxSize( 120,27 ) );

	bSizer12->Add( btShowWarnings, 0, wxALL, 5 );

	btShowMessages = new wxToggleButton( this, wxIDTOGGLEMESSAGES, wxT("(0) Messages"), wxDefaultPosition, wxDefaultSize, 0 );
	btShowMessages->SetMinSize( wxSize( 120,27 ) );

	bSizer12->Add( btShowMessages, 0, wxALL, 5 );


	bSizer11->Add( bSizer12, 0, wxEXPAND, 5 );

	wxBoxSizer* bSizer13;
	bSizer13 = new wxBoxSizer( wxVERTICAL );

	m_listCtrl1 = new wxListCtrl( this, wxIDC_LSTERRORS, wxDefaultPosition, wxDefaultSize, wxLC_REPORT );
	bSizer13->Add( m_listCtrl1, 1, wxALL|wxEXPAND, 5 );


	bSizer11->Add( bSizer13, 1, wxEXPAND, 5 );


	bSizer10->Add( bSizer11, 1, wxEXPAND, 5 );


	this->SetSizer( bSizer10 );
	this->Layout();
}

wxErrorsPanel::~wxErrorsPanel()
{
}

wxConsolePanel::wxConsolePanel( wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style, const wxString& name ) : wxPanel( parent, id, pos, size, style, name )
{
	wxBoxSizer* bSizer14;
	bSizer14 = new wxBoxSizer( wxVERTICAL );

	wxBoxSizer* bSizer15;
	bSizer15 = new wxBoxSizer( wxVERTICAL );

	wxBoxSizer* bSizer16;
	bSizer16 = new wxBoxSizer( wxHORIZONTAL );

	m_staticText7 = new wxStaticText( this, wxID_ANY, wxT("Console Output:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText7->Wrap( -1 );
	m_staticText7->SetFont( wxFont( 10, wxFONTFAMILY_MODERN, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL, true, wxT("Cascadia Mono") ) );
	m_staticText7->SetMinSize( wxSize( 150,20 ) );

	bSizer16->Add( m_staticText7, 0, wxALL, 5 );


	bSizer15->Add( bSizer16, 0, wxFIXED_MINSIZE, 5 );

	wxBoxSizer* bSizer17;
	bSizer17 = new wxBoxSizer( wxVERTICAL );

	txtConsole = new wxRichTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_READONLY|wxBORDER_THEME|wxHSCROLL|wxVSCROLL );
	bSizer17->Add( txtConsole, 1, wxEXPAND | wxALL, 5 );


	bSizer15->Add( bSizer17, 1, wxEXPAND, 5 );


	bSizer14->Add( bSizer15, 1, wxEXPAND, 5 );


	this->SetSizer( bSizer14 );
	this->Layout();
}

wxConsolePanel::~wxConsolePanel()
{
}
