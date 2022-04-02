/** @file LoggerDialog.cpp
 * Logger Dialog Box
 *
 **/
 // Copyright (C) 2022 - Leonardo Silva 
 // The License.txt file describes the conditions under which this software may be distributed.


#include "pch.h"
//#pragma comment (lib, "comctl32")          // Must use to create Image List controls (moved to pch.h)

#include "LoggerDialog.h"

#define IDM_ERRORTOGGLE 4001
#define IDM_WARNINGTOGGLE 4002
#define IDM_MESSAGETOGGLE 4003

using namespace NWScriptPlugin;

BEGIN_ANCHOR_MAP(LoggerDialog)
#ifdef DEBUG_ANCHORLIB
	ANCHOR_MAP_ENTRY(_hSelf, IDC_TABLOGGER, ANF_ALL, "Dialog Control: TABLOGGER (main window child)")
	ANCHOR_MAP_CHILDWINDOW(_consoleDlgHwnd, ANF_ALL, "Dialog Box: Console")
	ANCHOR_MAP_ENTRY(_consoleDlgHwnd, IDC_LBLCONSOLE, ANF_TOPLEFT, "Dialog Control: LBLCONSOLE (console child)")
	ANCHOR_MAP_ENTRY(_consoleDlgHwnd, IDC_TXTCONSOLE, ANF_ALL, "Dialog Control: TXTCONSOLE (console child)")
	ANCHOR_MAP_CHILDWINDOW(_errorDlgHwnd, ANF_ALL, "Dialog Box: Error")
	ANCHOR_MAP_ENTRY(_errorDlgHwnd, IDC_ERRORGROUPBOX, ANF_ALL, "Dialog Control: ERRORGROUPBOX (errors child)")
	ANCHOR_MAP_ENTRY(_errorDlgHwnd, IDC_LSTERRORS, ANF_ALL, "Dialog Control: LSTERRORS (errors child)")
#else
	ANCHOR_MAP_ENTRY(_hSelf, IDC_TABLOGGER, ANF_ALL)
	ANCHOR_MAP_CHILDWINDOW(_consoleDlgHwnd, ANF_ALL)
	ANCHOR_MAP_ENTRY(_consoleDlgHwnd, IDC_LBLCONSOLE, ANF_TOPLEFT)
	ANCHOR_MAP_ENTRY(_consoleDlgHwnd, IDC_TXTCONSOLE, ANF_ALL)
	ANCHOR_MAP_CHILDWINDOW(_errorDlgHwnd, ANF_ALL)
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
			_consoleDlgHwnd = CreateDialogParam(_hInst, MAKEINTRESOURCE(IDD_LOGGER_CONSOLE), _hSelf, dlgProxy, reinterpret_cast<LPARAM>(this));
			_errorDlgHwnd = CreateDialogParam(_hInst, MAKEINTRESOURCE(IDD_LOGGER_ERRORS), _hSelf, dlgProxy, reinterpret_cast<LPARAM>(this));

			// Create an image list to associate with errors and warnings, etc
			_iconList = ImageList_Create(16, 16, ILC_COLOR32, 3, 1);
			ImageList_AddIcon(_iconList, getStockIcon(SHSTOCKICONID::SIID_ERROR, IconSize::Size16x16));
			ImageList_AddIcon(_iconList, getStockIcon(SHSTOCKICONID::SIID_WARNING, IconSize::Size16x16));
			ImageList_AddIcon(_iconList, getStockIcon(SHSTOCKICONID::SIID_INFO, IconSize::Size16x16));

			// Create toolbar
			int ImageListID = 0;
			_toolBar = CreateWindowEx(TBSTYLE_EX_MIXEDBUTTONS, TOOLBARCLASSNAME, 
				NULL, WS_CHILD | TBSTYLE_WRAPABLE | TBSTYLE_LIST | CCS_NOPARENTALIGN | CCS_NODIVIDER,
				6, 11, 360, 25,	_errorDlgHwnd, NULL, _hInst, NULL);
			SendMessage(_toolBar, TB_SETIMAGELIST, (WPARAM)ImageListID, (LPARAM)_iconList);

			const int numButtons = 3;
			BYTE buttonStyles = BTNS_AUTOSIZE | BTNS_CHECK;
			TBBUTTON tbButtons[numButtons] =
			{
//				{ -1, 0, 0, BTNS_SEP, {0}, 0, (INT_PTR)L"" },
				{ MAKELONG(0, ImageListID), IDM_ERRORTOGGLE,   TBSTATE_ENABLED, buttonStyles, {0}, 0, (INT_PTR)L"  (0) Errors" },
				{ MAKELONG(1, ImageListID), IDM_WARNINGTOGGLE, TBSTATE_ENABLED, buttonStyles, {0}, 0, (INT_PTR)L"  (0) Warnings"},
				{ MAKELONG(2, ImageListID), IDM_MESSAGETOGGLE, TBSTATE_ENABLED, buttonStyles, {0}, 0, (INT_PTR)L"  (0) Messages"}
			};

			// Add buttons.
			SendMessage(_toolBar, TB_BUTTONSTRUCTSIZE, (WPARAM)sizeof(TBBUTTON), 0);
			SendMessage(_toolBar, TB_ADDBUTTONS, (WPARAM)numButtons, (LPARAM)&tbButtons);

			// Resize the toolbar, and then show it.
			TBMETRICS tbM;
			tbM.cbSize = sizeof(tbM);
			tbM.cxButtonSpacing = 6;
			tbM.dwMask = TBMF_BUTTONSPACING;
			SendMessage(_toolBar, TB_SETMETRICS, 0, (LPARAM)&tbM);
			ShowWindow(_toolBar, TRUE);

			// Setup anchors
			SetupDockingAnchors();

			// Add columns to listview
			SetupListView();

			// Default tab selected item
			TabCtrl_SetCurSel(_mainTabHwnd, 1);
			// Show default window (Console)
			ShowWindow(_consoleDlgHwnd, SW_NORMAL);

			break;
		}
		
		case WM_SIZE:
		{
			// Do auto-resize to controls
			if (anchorsPrepared)
				std::ignore = handleSizers();
			ResizeList();
			break;
		}

		case WM_COMMAND:
		{
			switch (wParam)
			{
				case IDCANCEL:
					display(false);
					break;
			}
			break;
		}

		case WM_NOTIFY:
		{
			if (wParam == IDC_TABLOGGER)
			{
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

			break;
		}

		default:
			return DockingDlgInterface::run_dlgProc(message, wParam, lParam);
	}

	return FALSE;
}

// Proxy Errors and Console subdialogs messages
INT_PTR LoggerDialog::dlgProxy(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
		case WM_INITDIALOG:
		{
			LoggerDialog* pLoggerDlg = reinterpret_cast<LoggerDialog*>(lParam);
			::SetWindowLongPtr(hWnd, GWLP_USERDATA, static_cast<LONG_PTR>(lParam));
			pLoggerDlg->childrenDlgProc(message, wParam, lParam);
			return TRUE;
		}

		default:
		{
			LoggerDialog* pLoggerDlg = reinterpret_cast<LoggerDialog*>(::GetWindowLongPtr(hWnd, GWLP_USERDATA));
			if (!pLoggerDlg)
				return FALSE;
			return pLoggerDlg->childrenDlgProc(message, wParam, lParam);
		}
	}
}

intptr_t LoggerDialog::childrenDlgProc(UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
		case WM_COMMAND:
		{
			switch (wParam)
			{
				case IDCANCEL:
				{
					display(false);
					break;
				}

				case IDC_BTERRORSTOGGLE:
				{
					break;
				}
			}
		}
	}

	return FALSE;
}


void LoggerDialog::ResizeList()
{
	// Calculate Description column size
	HWND listErrorsHWND = GetDlgItem(_errorDlgHwnd, IDC_LSTERRORS);
	LV_COLUMN adjustColumn;
	ZeroMemory(&adjustColumn, sizeof(adjustColumn));

	RECT rcList;
	GetClientRect(listErrorsHWND, &rcList);
	rcList.right;

	// Get column sizes
	size_t lstWidth = ListView_GetColumnWidth(listErrorsHWND, 0);
	lstWidth += ListView_GetColumnWidth(listErrorsHWND, 1);
	lstWidth += ListView_GetColumnWidth(listErrorsHWND, 2);
	lstWidth += ListView_GetColumnWidth(listErrorsHWND, 4);
	lstWidth += ListView_GetColumnWidth(listErrorsHWND, 5);
	lstWidth += ListView_GetColumnWidth(listErrorsHWND, 6);

	// Set "Description" column width
	size_t finalSize = rcList.right - lstWidth;
	ListView_SetColumnWidth(listErrorsHWND, 3, finalSize);

	//Auto-size last column
	ListView_SetColumnWidth(listErrorsHWND, 6, LVSCW_AUTOSIZE);
}

void LoggerDialog::SetupListView()
{
	HWND listErrorsHWND = GetDlgItem(_errorDlgHwnd, IDC_LSTERRORS);
	TCHAR columnName[64];

	// Extended Styles
	ListView_SetExtendedListViewStyle(listErrorsHWND, LVS_EX_DOUBLEBUFFER | LVS_EX_FULLROWSELECT);

	// Column generics
	LV_COLUMN newColumn;
	ZeroMemory(&newColumn, sizeof(newColumn));
	ZeroMemory(columnName, std::size(columnName));
	newColumn.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_ORDER | LVCF_DEFAULTWIDTH | LVCF_IDEALWIDTH;
	newColumn.fmt = LVCFMT_FIXED_WIDTH | LVCFMT_LEFT | LVCFMT_IMAGE | LVCFMT_FIXED_RATIO;
	newColumn.pszText = columnName;

	// First column (a dummy empty one)
	newColumn.mask = LVCF_FMT | LVCF_WIDTH;
	newColumn.fmt = LVCFMT_FIXED_WIDTH;
	newColumn.cx = 25;
	ListView_InsertColumn(listErrorsHWND, 0, &newColumn);

	// Icon of message
	newColumn.mask = LVCF_FMT | LVCF_WIDTH;
	newColumn.fmt = LVCFMT_FIXED_WIDTH | LVCFMT_IMAGE | LVCFMT_CENTER;
	ListView_InsertColumn(listErrorsHWND, 1, &newColumn);

	// Message Code
	newColumn.mask = LVCF_FMT | LVCF_TEXT | LVCF_MINWIDTH |LVCF_DEFAULTWIDTH | LVCF_IDEALWIDTH | LVCF_WIDTH;
	newColumn.fmt = LVCFMT_LEFT;
	newColumn.cxMin = 93;
	newColumn.cxDefault = 95;
	newColumn.cxIdeal = 95;
	newColumn.cx = 95;
	memcpy(columnName, TEXT("Code"), std::size(TEXT("Code")) * sizeof(TCHAR));
	newColumn.cchTextMax = std::size(TEXT("Code"));
	ListView_InsertColumn(listErrorsHWND, 2, &newColumn);

	// Message Description
	newColumn.mask = LVCF_FMT | LVCF_TEXT | LVCF_MINWIDTH | LVCF_DEFAULTWIDTH | LVCF_WIDTH;
	newColumn.fmt = LVCFMT_LEFT;
	newColumn.cxMin = 90;
	newColumn.cxDefault = 300;
	newColumn.cx = 300;
	memcpy(columnName, TEXT("Description"), std::size(TEXT("Description")) * sizeof(TCHAR));
	newColumn.cchTextMax = std::size(TEXT("Description"));
	ListView_InsertColumn(listErrorsHWND, 3, &newColumn);

	// File
	newColumn.mask = LVCF_FMT | LVCF_TEXT | LVCF_DEFAULTWIDTH | LVCF_IDEALWIDTH | LVCF_WIDTH;
	newColumn.fmt = LVCFMT_LEFT;
	newColumn.cxDefault = 120;
	newColumn.cxIdeal = 120;
	newColumn.cx = 120;
	memcpy(columnName, TEXT("File"), std::size(TEXT("File")) * sizeof(TCHAR));
	newColumn.cchTextMax = std::size(TEXT("File"));
	ListView_InsertColumn(listErrorsHWND, 4, &newColumn);

	// Line
	newColumn.mask = LVCF_FMT | LVCF_TEXT | LVCF_DEFAULTWIDTH | LVCF_IDEALWIDTH | LVCF_WIDTH;
	newColumn.fmt = LVCFMT_LEFT;
	newColumn.cxDefault = 80;
	newColumn.cxIdeal = 80;
	newColumn.cx = 80;
	memcpy(columnName, TEXT("Line"), std::size(TEXT("Line")) * sizeof(TCHAR));
	newColumn.cchTextMax = std::size(TEXT("Line"));
	ListView_InsertColumn(listErrorsHWND, 5, &newColumn);

	// Last column (a dummy empty terminator)
	newColumn.mask = LVCF_FMT | LVCF_WIDTH;
	newColumn.fmt = LVCFMT_RIGHT;
	newColumn.cx = 25;
	ListView_InsertColumn(listErrorsHWND, 6, &newColumn);

	// Do a first resize to fit the window...
	ResizeList();
}

// Setup the docking anchors
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

