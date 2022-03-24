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
#include <wx/tglbtn.h>
#include <wx/listctrl.h>
#include <wx/panel.h>

///////////////////////////////////////////////////////////////////////////

namespace NWScriptPlugin
{
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
	/// Class wxAboutDialog
	///////////////////////////////////////////////////////////////////////////////
	class wxAboutDialog : public wxDialog
	{
		private:

		protected:
			wxStaticBitmap* bmpLogo;
			wxStaticText* lblTitle;
			wxStaticText* lblVersion;
			wxStaticText* lblCopyright;
			wxStaticText* lblSpecialThanks;
			wxStaticText* lblSpecialThanksText;
			wxStaticText* lblHomepage;
			wxHyperlinkCtrl* lnkHomepage;
			wxRichTextCtrl* txtAbout;
			wxButton* btOk;

			// Virtual event handlers, override them in your derived class
			virtual void OnKeyPress( wxKeyEvent& event ) { event.Skip(); }
			virtual void OnBtOkClose( wxCommandEvent& event ) { event.Skip(); }


		public:

			wxAboutDialog( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = wxT("About NWScript Tools Plugin"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( 656,550 ), long style = wxDEFAULT_DIALOG_STYLE );

			~wxAboutDialog();

	};

	///////////////////////////////////////////////////////////////////////////////
	/// Class wxConsoleWindow
	///////////////////////////////////////////////////////////////////////////////
	class wxConsoleWindow : public wxDialog
	{
		private:

		protected:
			wxNotebook* tabConsole;

			// Virtual event handlers, override them in your derived class
			virtual void OnPageChanged( wxNotebookEvent& event ) { event.Skip(); }


		public:

			wxConsoleWindow( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = wxT("NWScript Panel 2.0"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( 551,360 ), long style = wxCAPTION|wxDEFAULT_DIALOG_STYLE|wxFULL_REPAINT_ON_RESIZE );

			~wxConsoleWindow();

	};

	///////////////////////////////////////////////////////////////////////////////
	/// Class wxErrorsPanel
	///////////////////////////////////////////////////////////////////////////////
	class wxErrorsPanel : public wxPanel
	{
		private:

		protected:
			wxToggleButton* btShowErrors;
			wxToggleButton* btShowWarnings;
			wxToggleButton* btShowMessages;
			wxListCtrl* m_listCtrl1;

		public:

			wxErrorsPanel( wxWindow* parent, wxWindowID id = wxID_ANY, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( 729,278 ), long style = wxFULL_REPAINT_ON_RESIZE, const wxString& name = wxEmptyString );

			~wxErrorsPanel();

	};

	///////////////////////////////////////////////////////////////////////////////
	/// Class wxConsolePanel
	///////////////////////////////////////////////////////////////////////////////
	class wxConsolePanel : public wxPanel
	{
		private:

		protected:
			wxStaticText* m_staticText7;
			wxRichTextCtrl* txtConsole;

		public:

			wxConsolePanel( wxWindow* parent, wxWindowID id = wxID_ANY, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( 500,300 ), long style = wxFULL_REPAINT_ON_RESIZE, const wxString& name = wxEmptyString );

			~wxConsolePanel();

	};

} // namespace NWScriptPlugin

