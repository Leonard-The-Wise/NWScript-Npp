#pragma once

#include "NWScriptForms.h"

namespace NWScriptPlugin {

	class AboutDialogEx : wxAboutDialog {
	public:


		AboutDialogEx(wxWindow* parent) :
			wxAboutDialog(parent) {			
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


	};
}
