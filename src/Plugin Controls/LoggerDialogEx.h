#pragma once

#include "NWScriptForms.h"

namespace NWScriptPlugin {

	class LoggerDialogEx : wxConsoleWindow {
	public:


		LoggerDialogEx(wxWindow* parent) :
			wxConsoleWindow(parent) {
			//_ErrorPanel = std::make_unique<wxErrorsPanel>(this);
			//_ErrorPanel->Show();
		}

		void doDialog(bool toShow = true) {
			this->Show(toShow);
		}

		bool isVisible() {
			return this->isVisible();
		}

		generic_string getTitle() {
			generic_string t = this->GetTitle().ToStdWstring();
			return t;
		}

		HWND getHWND() {
			return reinterpret_cast<HWND>(this->GetHWND());
		}

	protected:
		//std::unique_ptr<wxErrorsPanel> _ErrorPanel;
	};
}
