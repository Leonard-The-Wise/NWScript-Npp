#pragma once

#include "NWScriptForms.h"
#include "PluginControlsRC.h"

namespace NWScriptPlugin {

	class AboutDialogEx : wxAboutDialog {
	public:


		AboutDialogEx(HINSTANCE hInst, wxWindow* parent) :
			_hInst(hInst), wxAboutDialog(parent) {
			_hSelf = this->GetHWND();

			HBITMAP NWLogo = reinterpret_cast<HBITMAP>(LoadImage(hInst, MAKEINTRESOURCE(IDB_NWSCRIPTLOGO), IMAGE_BITMAP, 0, 0, 0));
			
			//b.
			//bmpLogo->SetBitmap(b);
		}

		void doDialog(bool toShow = true) {
			this->Show(toShow);
		}

	protected:
		virtual void OnBtOkClose(wxCommandEvent& event) {
			this->Show(false);			
		}

		virtual void OnKeyPress(wxKeyEvent& event) {
			if (event.GetKeyCode() == wxKeyCode::WXK_ESCAPE)
				this->Show(false);
		}

	private:
		HINSTANCE _hInst;
		HWND _hSelf;
	};
}
