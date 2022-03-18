/** @file LoggerDialog.h
 * Logger Dialog Box
 *
 **/
 // Copyright (C) 2022 - Leonardo Silva 
 // The License.txt file describes the conditions under which this software may be distributed.

#pragma once

#include "Common.h"
#include "DockingDlgInterface.h"
#include "PluginControlsRC.h"

namespace NWScriptPlugin {

	class LoggerDialog : public DockingDlgInterface
	{
	public:
		LoggerDialog() : DockingDlgInterface(IDD_LOGGER) {};

		virtual void display(bool toShow = true) const {
			DockingDlgInterface::display(toShow);
			if (toShow)
				::SetFocus(::GetDlgItem(_hSelf, IDC_TABLOGGER));
		};

		void setParent(HWND parent2set) {
			_hParent = parent2set;
		};

	protected:
		virtual intptr_t CALLBACK run_dlgProc(UINT message, WPARAM wParam, LPARAM lParam);
	private:
		HWND _mainTabHwnd = nullptr;
		HWND _tabErrorHwnd = nullptr;
		HWND _tabConsoleHwnd = nullptr;		
	};
}