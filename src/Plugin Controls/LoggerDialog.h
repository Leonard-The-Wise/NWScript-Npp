/** @file LoggerDialog.h
 * Logger Dialog Box
 *
 **/
 // Copyright (C) 2022 - Leonardo Silva 
 // The License.txt file describes the conditions under which this software may be distributed.

#pragma once

#define USE_ANF_SCREEN_TO_CLIENT

#include "AnchorMap.h"
#include "Common.h"
#include "DockingDlgInterface.h"
#include "PluginControlsRC.h"

namespace NWScriptPlugin {

	class LoggerDialog : public DockingDlgInterface
	{
	public:
		LoggerDialog() : DockingDlgInterface(IDD_LOGGER) {};

		// Notepad calls this on registering the class...
		// DON'T use it directly UNLESS on class initialization 
		// [to override Notepad++ auto-show for instance, you may call display(false) 
		// so no auto-anchoring setup will be done].
		// On normal uses, call doDialog() instead because we must setup anchors first...
		// read the SetupDockingAnchors() implement description on LoggerDialog.cpp for more
		// info.
		virtual void display(bool toShow = true) 
		{
			DockingDlgInterface::display(toShow);
			if (toShow)
				::SetFocus(::GetDlgItem(_hSelf, IDC_TABLOGGER));
		};

		void doDialog(bool toShow = true)
		{
			if (!anchorsPrepared && toShow)
				SetupDockingAnchors();

			display(toShow);
			anchorsPrepared = toShow;
		}

		void setParent(HWND parent2set) 
		{
			_hParent = parent2set;
		};


	protected:
		virtual intptr_t CALLBACK run_dlgProc(UINT message, WPARAM wParam, LPARAM lParam);
	private:
		DECLARE_ANCHOR_MAP()

		bool anchorsPrepared = false;
		void SetupDockingAnchors();

		HWND _mainTabHwnd = nullptr;
		HWND _errorDlgHwnd = nullptr;
		HWND _consoleDlgHwnd = nullptr;
		HIMAGELIST _iconList = nullptr;
	};
}