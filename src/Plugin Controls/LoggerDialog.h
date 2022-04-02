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

		virtual void display(bool toShow = true) 
		{
			DockingDlgInterface::display(toShow);
			if (toShow)
				::SetFocus(::GetDlgItem(_hSelf, IDC_TABLOGGER));
		}

		void setParent(HWND parent2set) 
		{
			_hParent = parent2set;
		}

		void switchToConsole()
		{
			TabCtrl_SetCurSel(_mainTabHwnd, 1);
		}

		void switchToErrors()
		{
			TabCtrl_SetCurSel(_mainTabHwnd, 0);
		}

	protected:
		// Main window dialog procedure call
		virtual intptr_t CALLBACK run_dlgProc(UINT message, WPARAM wParam, LPARAM lParam);

		// Proxy for children dialog processing
		static INT_PTR CALLBACK dlgProxy(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
		// Real children message-processing 
		intptr_t CALLBACK childrenDlgProc(UINT message, WPARAM wParam, LPARAM lParam);

	private:
		DECLARE_ANCHOR_MAP()

		bool anchorsPrepared = false;
		void SetupDockingAnchors();
		void SetupListView();
		void ResizeList();

		HWND _mainTabHwnd = nullptr;
		HWND _errorDlgHwnd = nullptr;
		HWND _consoleDlgHwnd = nullptr;
		HWND _toolBar = nullptr;
		HIMAGELIST _iconList = nullptr;
	};
}