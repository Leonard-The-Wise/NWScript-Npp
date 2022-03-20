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


BEGIN_ANCHOR_MAP(LoggerDialog)

	ANCHOR_MAP_ENTRY(_hSelf, IDC_TABLOGGER, ANF_ALL)
	ANCHOR_MAP_CHILDWINDOW(_consoleDlgHwnd, ANF_ALL)
	ANCHOR_MAP_CHILDWINDOW(_errorDlgHwnd, ANF_ALL)
	ANCHOR_MAP_ENTRY(_consoleDlgHwnd, IDC_LBLCONSOLE, ANF_TOPLEFT)
	ANCHOR_MAP_ENTRY(_consoleDlgHwnd, IDC_TXTCONSOLE, ANF_ALL)

END_ANCHOR_MAP(_hSelf)

intptr_t CALLBACK LoggerDialog::run_dlgProc(UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
		case WM_INITDIALOG:
		{
			INITCOMMONCONTROLSEX ix;
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

			// Default tab selected item
			TabCtrl_HighlightItem(_mainTabHwnd, 1, 1);

			// Default tab to display
			//ShowWindow(_consoleDlgHwnd, SW_NORMAL);
		}
		
		case WM_SIZE:
		{
			// Do auto-resize to controls
			if (anchorsPrepared)
				HandleAnchors();
		}

		case WM_COMMAND:
		{

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
	if (m_bpfxAnchorMap.IsInitialized())
		m_bpfxAnchorMap.Reset();

	// The tab control is inside a bordered dialog with a title (because Notepad++ needs the title), 
	// hence we first get the original height OFFSET between the client with borders to apply to the new 
	// dialog window without borders
	HWND originalLoggerDlg = CreateDialog(_hInst, MAKEINTRESOURCE(IDD_LOGGER), _hSelf, 0);
	RECT loggerWindow, loggerClient;
	GetWindowRect(originalLoggerDlg, &loggerWindow);
	ScreenToClient(originalLoggerDlg, &loggerWindow);
	GetClientRect(originalLoggerDlg, &loggerClient);
	// Get rid of the extra window.
	DestroyWindow(originalLoggerDlg);

	// Now grab the original margins...
	OFFSETRECT loggerMargins = ControlAnchorMap::calculateMargins(loggerWindow, loggerClient);
	// turn it upside down (cause we want the window title to become the bottom expansion)...
	ControlAnchorMap::invertOffsetRect(loggerMargins, INVERT_VERTICAL);
	// displace it on the Y axis twice (because we got a negative top now)...
	ControlAnchorMap::moveOffsetRect(loggerMargins, { 0, -loggerMargins.topMargin * 2 });

	// And then resize and reposition the TAB control into the new sized docked window keeping original offsets
	ControlAnchorMap::repositControl(_mainTabHwnd, _hSelf, _hInst, 
		IDD_LOGGER, IDC_TABLOGGER, ANF_ALL, loggerMargins);

	// Resize and reposition children dialog windows inside the tab control with a fixed margin

	// Get tab Client area
	GetClientRect(_mainTabHwnd, &rcTabClient);
	TabCtrl_AdjustRect(_mainTabHwnd, false, &rcTabClient);
	// Now apply some fixed margins
	OFFSETRECT margins = { 2, 2, -3, -4 };
	ControlAnchorMap::applyMargins(margins, rcTabClient);

	// Now accomodate windowses inside the tab control
	SetWindowPos(_consoleDlgHwnd, _mainTabHwnd, rcTabClient.left, rcTabClient.top,
		rcTabClient.right, rcTabClient.bottom, SWP_NOREDRAW | SWP_NOZORDER);
	SetWindowPos(_errorDlgHwnd, _mainTabHwnd, rcTabClient.left + 3, rcTabClient.top,
		rcTabClient.right, rcTabClient.bottom, SWP_NOREDRAW | SWP_NOZORDER);

	// Reposition tabbed child controls on their destination windowses
	// maintaining original proportions.
	ControlAnchorMap::repositControl(GetDlgItem(_consoleDlgHwnd, IDC_LBLCONSOLE), _consoleDlgHwnd,
		_hInst, IDD_LOGGER_CONSOLE, IDC_LBLCONSOLE, ANF_TOPLEFT);
	ControlAnchorMap::repositControl(GetDlgItem(_consoleDlgHwnd, IDC_TXTCONSOLE), _consoleDlgHwnd,
		_hInst, IDD_LOGGER_CONSOLE, IDC_TXTCONSOLE, ANF_ALL);

	// Initialize anchors for auto-resize
	InitAnchors();
	anchorsPrepared = true;
}

