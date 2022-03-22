///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 3.10.1-0-g8feb16b3)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#pragma once

#include <wx/artprov.h>
#include <wx/xrc/xmlres.h>
#include <wx/bitmap.h>
#include <wx/image.h>
#include <wx/icon.h>
#include <wx/statbmp.h>
#include <wx/gdicmn.h>
#include <wx/font.h>
#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/string.h>
#include <wx/stattext.h>
#include <wx/sizer.h>
#include <wx/hyperlink.h>
#include <wx/richtext/richtextctrl.h>
#include <wx/button.h>
#include <wx/dialog.h>
#include <wx/notebook.h>
#include <wx/frame.h>
#include <wx/tglbtn.h>
#include <wx/listctrl.h>
#include <wx/panel.h>
#include <wx/toolbar.h>

///////////////////////////////////////////////////////////////////////////

#define wxIDB_NWSCRIPTLOGO 10000
#define wxIDC_LBLVERSION 10001
#define wxIDC_LBLCOPYRIGHT 10002
#define wxIDC_LNKHOMEPAGE 10003
#define wxIDC_TXTABOUT 10004
#define wxIDOK 10005
#define wxIDTOGGLEERRORS 10006
#define wxIDTOGGLEWARNINGS 10007
#define wxIDTOGGLEMESSAGES 10008
#define wxIDC_LSTERRORS 10009

///////////////////////////////////////////////////////////////////////////////
/// Class AboutDialog
///////////////////////////////////////////////////////////////////////////////
class AboutDialog : public wxDialog
{
	private:

	protected:
		wxStaticBitmap* m_bitmap2;
		wxStaticText* lblTitle;
		wxStaticText* lblVersion;
		wxStaticText* lblCopyright;
		wxStaticText* lblSpecialThanks;
		wxStaticText* lblSpecialThanksText;
		wxStaticText* lblHomepage;
		wxHyperlinkCtrl* lnkHomepage;
		wxRichTextCtrl* txtAbout;
		wxButton* btOk;

	public:

		AboutDialog( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = wxT("About NWScript Tools Plugin"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( 656,550 ), long style = wxDEFAULT_DIALOG_STYLE );

		~AboutDialog();

};

///////////////////////////////////////////////////////////////////////////////
/// Class ConsoleWindow
///////////////////////////////////////////////////////////////////////////////
class ConsoleWindow : public wxFrame
{
	private:

	protected:
		wxNotebook* tabConsole;

		// Virtual event handlers, override them in your derived class
		virtual void OnPageChanged( wxNotebookEvent& event ) { event.Skip(); }


	public:

		ConsoleWindow( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = wxT("NWScript Tools Compiler"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( 701,300 ), long style = wxCAPTION|wxFRAME_FLOAT_ON_PARENT|wxRESIZE_BORDER|wxTAB_TRAVERSAL );

		~ConsoleWindow();

};

///////////////////////////////////////////////////////////////////////////////
/// Class ErrorsPanel
///////////////////////////////////////////////////////////////////////////////
class ErrorsPanel : public wxPanel
{
	private:

	protected:
		wxToggleButton* m_toggleBtn1;
		wxToggleButton* m_toggleBtn2;
		wxToggleButton* m_toggleBtn3;
		wxListCtrl* m_listCtrl1;

	public:

		ErrorsPanel( wxWindow* parent, wxWindowID id = wxID_ANY, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( 729,278 ), long style = wxTAB_TRAVERSAL, const wxString& name = wxEmptyString );

		~ErrorsPanel();

};

///////////////////////////////////////////////////////////////////////////////
/// Class ConsolePanel
///////////////////////////////////////////////////////////////////////////////
class ConsolePanel : public wxPanel
{
	private:

	protected:
		wxStaticText* m_staticText7;
		wxToolBar* m_toolBar1;
		wxRichTextCtrl* m_richText2;

	public:

		ConsolePanel( wxWindow* parent, wxWindowID id = wxID_ANY, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( 500,300 ), long style = wxTAB_TRAVERSAL, const wxString& name = wxEmptyString );

		~ConsolePanel();

};

