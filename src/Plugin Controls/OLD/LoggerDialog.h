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
		// so, DON'T use it directly UNLESS on your class initialization routines
		// [to override Notepad++ auto-show docked dialogs on initialization for instance, 
		// you may call display(false) to reverse that behavior].
		// On normal uses, call doDialog() instead because we must setup anchors first...
		// read the SetupDockingAnchors() implementation description on LoggerDialog.cpp for more
		// info.
		virtual void display(bool toShow = true) 
		{
			DockingDlgInterface::display(toShow);
			if (toShow)
				::SetFocus(::GetDlgItem(_hSelf, IDC_TABLOGGER));
		};

		void doDialog(bool toShow = true)
		{
			// To work with notepad++ drawing mechanisms, we reverse the process...
			// usually we would first setup anchors and display our dialog, but here
			// we first SHOW the window, so all the drawing messages pass through
			display(toShow);

			// and ONLY THEN we setup anchors... (this is only required on DockingDialogs)
			// or else we may get some unexpected control sizes as a result.
			if (!anchorsPrepared && toShow)
				SetupDockingAnchors();
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