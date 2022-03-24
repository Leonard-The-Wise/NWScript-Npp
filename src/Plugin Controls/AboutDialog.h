/** @file AboutDialog.h
 * About Dialog Box
 *
 **/
 // Copyright (C) 2022 - Leonardo Silva 
 // The License.txt file describes the conditions under which this software may be distributed.


#pragma once

#include "Common.h"
#include "NWScriptForms.h"
#include "PluginControlsRC.h"

namespace NWScriptPlugin {

	class AboutDialog : wxAboutDialog {
	public:


		AboutDialog(HINSTANCE hInst, wxWindow* parent) :
			_hInst(hInst), wxAboutDialog(parent) {
			_hSelf = this->GetHWND();
			
			bmpLogo->SetBitmap(resourcePNG(IDB_NWSCRIPTLOGO));
			this->connectKeyDownEvent(this);
		}

		void doDialog(bool toShow = true) {
			this->Show(toShow);
		}

		void setReplaceStrings(const std::map<generic_string, generic_string>& replaceStrings) {
			_replaceStrings = replaceStrings;
		};

		void setHomePath(const TCHAR* homePath) {
			_homePath = homePath;
		}

		void UpdateInfo();


	protected:
		virtual void OnBtOkClose(wxCommandEvent& event) {
			this->Show(false);			
		}

		virtual void OnKeyPress(wxKeyEvent& event) {
			if (event.GetKeyCode() == wxKeyCode::WXK_ESCAPE || 
				event.GetKeyCode() == wxKeyCode::WXK_RETURN ||
				event.GetKeyCode() == wxKeyCode::WXK_NUMPAD_ENTER)
				this->Show(false);
		}

	private:

		void connectKeyDownEvent(wxWindow* pclComponent);

		generic_string _homePath;
		std::map<generic_string, generic_string> _replaceStrings;

		HINSTANCE _hInst;
		HWND _hSelf;
	};
}
