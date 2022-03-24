/** @file LoggerDialog.cpp
 * Logger Dialog Box
 *
 **/
 // Copyright (C) 2022 - Leonardo Silva 
 // The License.txt file describes the conditions under which this software may be distributed.


#include "pch.h"
#pragma comment (lib, "comctl32")          // Must use to create Image List controls

#include "LoggerDialog.h"


using namespace NWScriptPlugin;

#define POSTINITSETUP

BEGIN_ANCHOR_MAP(LoggerDialog)
#ifdef DEBUG_ANCHORLIB
	ANCHOR_MAP_ENTRY(_hSelf, IDC_TABLOGGER, ANF_ALL, "Dialog Control: TABLOGGER (main window child)")
	ANCHOR_MAP_CHILDWINDOW(_consoleDlgHwnd, ANF_ALL, "Dialog Box: Console")
	ANCHOR_MAP_ENTRY(_consoleDlgHwnd, IDC_LBLCONSOLE, ANF_TOPLEFT, "Dialog Control: LBLCONSOLE (console child)")
	ANCHOR_MAP_ENTRY(_consoleDlgHwnd, IDC_TXTCONSOLE, ANF_ALL, "Dialog Control: TXTCONSOLE (console child)")
	ANCHOR_MAP_CHILDWINDOW(_errorDlgHwnd, ANF_ALL, "Dialog Box: Error")
	ANCHOR_MAP_ENTRY(_errorDlgHwnd, IDC_BTERRORSTOGGLE, ANF_TOPLEFT, "Dialog Control: BTERRORSTOGGLE (errors child)")
	ANCHOR_MAP_ENTRY(_errorDlgHwnd, IDC_BTWARNINGSTOGGLE, ANF_TOPLEFT, "Dialog Control: BTWARNINGSTOGGLE (errors child)")
	ANCHOR_MAP_ENTRY(_errorDlgHwnd, IDC_BTMESSGESTOGGLE, ANF_TOPLEFT, "Dialog Control: BTMESSGESTOGGLE (errors child)")
	ANCHOR_MAP_ENTRY(_errorDlgHwnd, IDC_ERRORGROUPBOX, ANF_ALL, "Dialog Control: ERRORGROUPBOX (errors child)")
	ANCHOR_MAP_ENTRY(_errorDlgHwnd, IDC_LSTERRORS, ANF_ALL, "Dialog Control: LSTERRORS (errors child)")
#else
	ANCHOR_MAP_ENTRY(_hSelf, IDC_TABLOGGER, ANF_ALL)
	ANCHOR_MAP_CHILDWINDOW(_consoleDlgHwnd, ANF_ALL)
	ANCHOR_MAP_ENTRY(_consoleDlgHwnd, IDC_LBLCONSOLE, ANF_TOPLEFT)
	ANCHOR_MAP_ENTRY(_consoleDlgHwnd, IDC_TXTCONSOLE, ANF_ALL)
	ANCHOR_MAP_CHILDWINDOW(_errorDlgHwnd, ANF_ALL)
	ANCHOR_MAP_ENTRY(_errorDlgHwnd, IDC_BTERRORSTOGGLE, ANF_TOPLEFT)
	ANCHOR_MAP_ENTRY(_errorDlgHwnd, IDC_BTWARNINGSTOGGLE, ANF_TOPLEFT)
	ANCHOR_MAP_ENTRY(_errorDlgHwnd, IDC_BTMESSGESTOGGLE, ANF_TOPLEFT)
	ANCHOR_MAP_ENTRY(_errorDlgHwnd, IDC_ERRORGROUPBOX, ANF_ALL)
	ANCHOR_MAP_ENTRY(_errorDlgHwnd, IDC_LSTERRORS, ANF_ALL)
#endif
END_ANCHOR_MAP(_hSelf)


intptr_t CALLBACK LoggerDialog::run_dlgProc(UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
		case WM_INITDIALOG:
		{
			INITCOMMONCONTROLSEX ix = { 0, 0 };
			ix.dwSize = sizeof(INITCOMMONCONTROLSEX);
			ix.dwICC = ICC_TAB_CLASSES;
			InitCommonControlsEx(&ix);

			_mainTabHwnd = GetDlgItem(_hSelf, IDC_TABLOGGER);

			// Populate tab control
			TCITEM tie = { 0 };
			tie.mask = TCIF_TEXT | TCIF_IMAGE;
			tie.iImage = -1;
			tie.pszText = (TCHAR*)TEXT("Errors List");
			TabCtrl_InsertItem(_mainTabHwnd, 0, &tie);
			tie.pszText = (TCHAR*)TEXT("Console Output");
			TabCtrl_InsertItem(_mainTabHwnd, 1, &tie);

			// Create our child dialogs
			_consoleDlgHwnd = CreateDialog(_hInst, MAKEINTRESOURCE(IDD_LOGGER_CONSOLE), _hSelf, 0);
			_errorDlgHwnd = CreateDialog(_hInst, MAKEINTRESOURCE(IDD_LOGGER_ERRORS), _hSelf, 0);

			// Create an image list to associate with errors and warnings, etc
			_iconList = ImageList_Create(24, 24, ILC_COLOR32, 3, 1);
			ImageList_AddIcon(_iconList, getStockIcon(SHSTOCKICONID::SIID_ERROR, IconSize::Size24x24));
			ImageList_AddIcon(_iconList, getStockIcon(SHSTOCKICONID::SIID_WARNING, IconSize::Size24x24));
			ImageList_AddIcon(_iconList, getStockIcon(SHSTOCKICONID::SIID_INFO, IconSize::Size24x24));

			// Icons for buttons
			HBITMAP hErrorsBmp = getStockIconBitmap(SHSTOCKICONID::SIID_ERROR, IconSize::Size16x16);
			HBITMAP hWarningBmp = getStockIconBitmap(SHSTOCKICONID::SIID_WARNING, IconSize::Size16x16);
			HBITMAP hInfoBmp = getStockIconBitmap(SHSTOCKICONID::SIID_INFO, IconSize::Size16x16);

			::SendMessage(GetDlgItem(_errorDlgHwnd, IDC_BTERRORSTOGGLE), BM_SETIMAGE, static_cast<WPARAM>(IMAGE_BITMAP), reinterpret_cast<LPARAM>(hErrorsBmp));
			::SendMessage(GetDlgItem(_errorDlgHwnd, IDC_BTERRORSTOGGLE), BM_SETIMAGE, static_cast<WPARAM>(IMAGE_BITMAP), reinterpret_cast<LPARAM>(hErrorsBmp));

#ifndef POSTINITSETUP
			SetupDockingAnchors();
#endif
			ShowWindow(_consoleDlgHwnd, SW_NORMAL);
			// Default tab selected item
			TabCtrl_SetCurSel(_mainTabHwnd, 1);
		}
		
		case WM_SIZE:
		{
			// Do auto-resize to controls
			if (anchorsPrepared)
				return handleSizers();
		}

		case WM_COMMAND:
		{

		}

		case WM_NOTIFY:
		{
			if (wParam != IDC_TABLOGGER)
				break;

			if (_mainTabHwnd)
			{
				int tabSelection = TabCtrl_GetCurSel(_mainTabHwnd);
				if (tabSelection == 0)
				{
					ShowWindow(_errorDlgHwnd, SW_NORMAL);
					ShowWindow(_consoleDlgHwnd, SW_HIDE);
				}
				else
				{
					ShowWindow(_errorDlgHwnd, SW_HIDE);
					ShowWindow(_consoleDlgHwnd, SW_NORMAL);
				}
			}
		}

		default:
			return DockingDlgInterface::run_dlgProc(message, wParam, lParam);
	}

	return FALSE;
}

// Moved all repositioning code from WM_INITIALIZE to this function.
// The reason is simple: Notepad++ will MESS UP with our control's
// lengths if we initialize anchors before displaying a docked window
// because it does SEVERAL repositionings and movins internally to create
// the docked window... so we wait until it finishes internal processing
// and THEN we properly resize our controls to fit the docked dialog
// and then initialize anchors... (yeah, took SOME time to figure that out)
void LoggerDialog::SetupDockingAnchors() 
{
	RECT rcTabClient;

	// We reset anchors here, so we can reposition at will...
	if (m_bpfxAnchorMap.isInitialized())
		m_bpfxAnchorMap.reset();

	// The tab control is inside a bordered dialog with a title (because Notepad++ needs the title), 
	// hence we first get the original height OFFSETS between the client with borders to apply to the new 
	// dialog window without borders. The title bar will become our new bottom limit to expand.

	// So, first grab the original designed dialog
	HWND originalLoggerDlg = CreateDialog(_hInst, MAKEINTRESOURCE(IDD_LOGGER), _hSelf, 0);
	RECT loggerWindow, loggerClient;
	GetWindowRect(originalLoggerDlg, &loggerWindow);
	ScreenToClient(originalLoggerDlg, &loggerWindow);
	GetClientRect(originalLoggerDlg, &loggerClient);

	// Now grab the original margins...
	OFFSETRECT loggerMargins = ControlAnchorMap::calculateMargins(loggerWindow, loggerClient);
	// turn it upside down (cause we want the window title to become the bottom expansion)...
	ControlAnchorMap::invertOffsetRect(loggerMargins, INVERT_VERTICAL);
	// displace it on the Y axis twice the current top (because we got a negative top margin now)...
	ControlAnchorMap::moveOffsetRect(loggerMargins, { 0, -loggerMargins.topMargin * 2 });

	// And then resize and reposition the TAB control into the new sized docked window while keeping the original offsets...
	// ding!
	ControlAnchorMap::repositControl(_mainTabHwnd, originalLoggerDlg, IDC_TABLOGGER, ANF_ALL, loggerMargins);

	// (now get rid of the extra window)
	DestroyWindow(originalLoggerDlg);

	// Resize and reposition children dialog windows inside the tab control with a fixed margin

	// Get tab Client area
	GetClientRect(_mainTabHwnd, &rcTabClient);
	TabCtrl_AdjustRect(_mainTabHwnd, false, &rcTabClient);

	// And since we don't want our child windowses completely overlapping the Tab borders
	// apply a *small* extra margin to them...
	ControlAnchorMap::applyMargins(rcTabClient, { 1, 1, -1, -2 });

	// Now accomodate windowses inside the tab control
	SetWindowPos(_consoleDlgHwnd, _mainTabHwnd, rcTabClient.left, rcTabClient.top,
		rcTabClient.right, rcTabClient.bottom, SWP_NOREDRAW | SWP_NOZORDER);
	SetWindowPos(_errorDlgHwnd, _mainTabHwnd, rcTabClient.left, rcTabClient.top,
		rcTabClient.right, rcTabClient.bottom, SWP_NOREDRAW | SWP_NOZORDER);

	// Create temporary handlers for resource economy...
	HWND originalConsoleDlg = CreateDialog(_hInst, MAKEINTRESOURCE(IDD_LOGGER_CONSOLE), _hSelf, 0);
	HWND originalErrorsDlg = CreateDialog(_hInst, MAKEINTRESOURCE(IDD_LOGGER_ERRORS), _hSelf, 0);

	// Reposition tabbed child controls on their destination windowses
	// maintaining original proportions.
		// Console window items
	ControlAnchorMap::repositControl(GetDlgItem(_consoleDlgHwnd, IDC_LBLCONSOLE), 
		originalConsoleDlg, IDC_LBLCONSOLE, ANF_TOPLEFT);
	ControlAnchorMap::repositControl(GetDlgItem(_consoleDlgHwnd, IDC_TXTCONSOLE), 
		originalConsoleDlg, IDC_TXTCONSOLE, ANF_ALL);
		// Errors window items
	ControlAnchorMap::repositControl(GetDlgItem(_errorDlgHwnd, IDC_BTERRORSTOGGLE), 
		originalErrorsDlg, IDC_BTERRORSTOGGLE, ANF_TOPLEFT);
	ControlAnchorMap::repositControl(GetDlgItem(_errorDlgHwnd, IDC_BTWARNINGSTOGGLE),
		originalErrorsDlg, IDC_BTWARNINGSTOGGLE, ANF_TOPLEFT);
	ControlAnchorMap::repositControl(GetDlgItem(_errorDlgHwnd, IDC_BTMESSGESTOGGLE), 
		originalErrorsDlg, IDC_BTMESSGESTOGGLE, ANF_TOPLEFT);
	ControlAnchorMap::repositControl(GetDlgItem(_errorDlgHwnd, IDC_ERRORGROUPBOX), 
		originalErrorsDlg, IDC_ERRORGROUPBOX, ANF_ALL);
	ControlAnchorMap::repositControl(GetDlgItem(_errorDlgHwnd, IDC_LSTERRORS), 
		originalErrorsDlg, IDC_LSTERRORS, ANF_ALL);

	// Remove temporary objects
	DestroyWindow(originalConsoleDlg);
	DestroyWindow(originalErrorsDlg);

	// initialize anchors for auto-resize and mark ready for WM_SIZE processing
	InitAnchors();
	anchorsPrepared = true;
}
