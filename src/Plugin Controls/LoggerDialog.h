/** @file LoggerDialog.h
 * Logger Dialog Box
 *
 **/
 // Copyright (C) 2022 - Leonardo Silva 
 // The License.txt file describes the conditions under which this software may be distributed.

#pragma once


#include "Common.h"
#include "DockingDlgInterface.h"

#include "NWScriptForms.h"

namespace NWScriptPlugin {


	class LoggerDialog : wxConsoleWindow {
	public:

		LoggerDialog(wxWindow* parent) :
			wxConsoleWindow(parent) 
		{
			_hParent = parent->GetHWND();
			_hSelf = this->GetHWND();
			_ErrorPanel = std::make_unique<wxErrorsPanel>(dynamic_cast<wxWindow*>(tabConsole));
			_ConsolePanel = std::make_unique<wxConsolePanel>(dynamic_cast<wxWindow*>(tabConsole));
			tabConsole->AddPage(dynamic_cast<wxWindow*>(_ErrorPanel.get()), "Errors", false);
			tabConsole->AddPage(dynamic_cast<wxWindow*>(_ConsolePanel.get()), "Console", true);

			// Main list icons
			_listIcons.Add(getStockIconWx(SHSTOCKICONID::SIID_ERROR, IconSize::Size16x16));
			_listIcons.Add(getStockIconWx(SHSTOCKICONID::SIID_WARNING, IconSize::Size16x16));
			_listIcons.Add(getStockIconWx(SHSTOCKICONID::SIID_INFO, IconSize::Size16x16));


			//Set icons to buttons
			wxWindow* button = _ErrorPanel->FindWindowById(wxIDTOGGLEERRORS);
			((wxToggleButton*)button)->SetBitmap(getStockIconBitmapWx(SHSTOCKICONID::SIID_ERROR, 
				IconSize::Size16x16), wxDirection::wxLEFT);			
			button = _ErrorPanel->FindWindowById(wxIDTOGGLEWARNINGS);
			((wxToggleButton*)button)->SetBitmap(getStockIconBitmapWx(SHSTOCKICONID::SIID_WARNING,
				IconSize::Size16x16), wxDirection::wxLEFT);
			button = _ErrorPanel->FindWindowById(wxIDTOGGLEMESSAGES);
			((wxToggleButton*)button)->SetBitmap(getStockIconBitmapWx(SHSTOCKICONID::SIID_INFO,
				IconSize::Size16x16), wxDirection::wxLEFT);

			// Create tab columns
			
			

		}

		void doDialog(bool toShow = true) {
			// Here we must communicate with parent (Notepad++)
			::SendMessage(_hParent, toShow ? NPPM_DMMSHOW : NPPM_DMMHIDE, 0, (LPARAM)_hSelf);
		}

		bool isVisible() {
			return (::IsWindowVisible(_hSelf) ? true : false); // query status by ordinary HWND here
		}

		generic_string getTitle() {
			generic_string t = this->GetTitle().ToStdWstring();
			return t;
		}

		HWND getHWND() {
			return reinterpret_cast<HWND>(this->GetHWND());
		}

	protected:
		wxImageList _listIcons;
		HWND _hSelf;
		HWND _hParent;
		wxIcon _error, _warning, _info;
		std::unique_ptr<wxErrorsPanel> _ErrorPanel;
		std::unique_ptr<wxConsolePanel> _ConsolePanel;
	};
}
