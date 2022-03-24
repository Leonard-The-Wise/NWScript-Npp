#pragma once

#include "NWScriptForms.h"

namespace NWScriptPlugin {

	class LoggerDialogEx : wxConsoleWindow {
	public:


		LoggerDialogEx(wxWindow* parent) :
			wxConsoleWindow(parent) 
		{
			//this->AdoptAttributesFromHWND();
			_ErrorPanel = std::make_unique<wxErrorsPanel>(dynamic_cast<wxWindow*>(tabConsole));
			_ConsolePanel = std::make_unique<wxConsolePanel>(dynamic_cast<wxWindow*>(tabConsole));
			tabConsole->AddPage(dynamic_cast<wxWindow*>(_ErrorPanel.get()), "Errors", true);
			tabConsole->AddPage(dynamic_cast<wxWindow*>(_ConsolePanel.get()), "Console", false);			
			//_ErrorPanel->AdoptAttributesFromHWND();
			//_ConsolePanel->AdoptAttributesFromHWND();
		}

		void doDialog(bool toShow = true) {
			this->Show(toShow);
		}

		virtual bool isVisible() {
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
		std::unique_ptr<wxErrorsPanel> _ErrorPanel;
		std::unique_ptr<wxConsolePanel> _ConsolePanel;
	};
}
