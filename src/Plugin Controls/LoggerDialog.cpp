/** @file LoggerDialog.cpp
 * Logger Dialog Box
 *
 **/
 // Copyright (C) 2022 - Leonardo Silva 
 // The License.txt file describes the conditions under which this software may be distributed.


#include "pch.h"
//#pragma comment (lib, "comctl32")          // Must use to create Image List controls (moved to pch.h)

#include "LoggerDialog.h"
#include "Common.h"

#define IDM_ERRORTOGGLE 4001
#define IDM_WARNINGTOGGLE 4002
#define IDM_MESSAGETOGGLE 4003

#define ERRORCOLUMN_ITEMID 0
#define ERRORCOLUMN_MESSAGECODE 2
#define ERRORCOLUMN_MESSAGETEXT 3
#define ERRORCOLUMN_FILENAME 4
#define ERRORCOLUMN_FILELINE 5


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
	ANCHOR_MAP_ENTRY(_consoleDlgHwnd, IDC_BTFILTERERRORS, ANF_TOPRIGHT)
	ANCHOR_MAP_ENTRY(_consoleDlgHwnd, IDC_BTFILTERWARNINGS, ANF_TOPRIGHT)
	ANCHOR_MAP_ENTRY(_consoleDlgHwnd, IDC_BTFILTERINFO, ANF_TOPRIGHT)
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
			_iconList16x16 = ImageList_Create(16, 16, ILC_COLOR32, 4, 1);
			ImageList_AddIcon(_iconList16x16, getStockIcon(SHSTOCKICONID::SIID_ERROR, IconSize::Size16x16));
			ImageList_AddIcon(_iconList16x16, LoadIcon(_hInst, MAKEINTRESOURCE(IDI_ERRORSQUIGGLE)));               // compiler error
			ImageList_AddIcon(_iconList16x16, getStockIcon(SHSTOCKICONID::SIID_WARNING, IconSize::Size16x16));
			ImageList_AddIcon(_iconList16x16, getStockIcon(SHSTOCKICONID::SIID_INFO, IconSize::Size16x16));

			// Create toolbar
			int ImageListID = 0;
			_toolBar = CreateWindowEx(TBSTYLE_EX_MIXEDBUTTONS, TOOLBARCLASSNAME, 
				NULL, WS_CHILD | TBSTYLE_WRAPABLE | TBSTYLE_LIST | CCS_NOPARENTALIGN | CCS_NODIVIDER,
				6, 11, 360, 29,	_errorDlgHwnd, NULL, _hInst, NULL);
			SendMessage(_toolBar, TB_SETIMAGELIST, (WPARAM)ImageListID, (LPARAM)_iconList16x16);

			const int numButtons = 3;
			BYTE buttonStyles = BTNS_AUTOSIZE | BTNS_CHECK;
			BYTE errorToggle = TBSTATE_ENABLED | (_settings->compilerWindowShowErrors ? TBSTATE_CHECKED : (BYTE)0);
			BYTE warningToggle = TBSTATE_ENABLED | (_settings->compilerWindowShowWarnings ? TBSTATE_CHECKED : (BYTE)0);
			BYTE infoToggle = TBSTATE_ENABLED | (_settings->compilerWindowShowInfos ? TBSTATE_CHECKED : (BYTE)0);

			TBBUTTON tbButtons[numButtons] =
			{
				{ MAKELONG(0, ImageListID), IDM_ERRORTOGGLE, errorToggle, buttonStyles, {0}, 0, (INT_PTR)L" (0) Errors"},
				{ MAKELONG(2, ImageListID), IDM_WARNINGTOGGLE, warningToggle, buttonStyles, {0}, 0, (INT_PTR)L" (0) Warnings"},
				{ MAKELONG(3, ImageListID), IDM_MESSAGETOGGLE, infoToggle, buttonStyles, {0}, 0, (INT_PTR)L" (0) Messages"}
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

			// Setup icons to buttons in console log (LoadIcon is bugging, using LoadImage instead).
			HBITMAP clearWindow = iconToBitmap(reinterpret_cast<HICON>(LoadImage(_hInst, MAKEINTRESOURCE(IDI_CLEARWINDOW), IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR)));
			::SendMessage(GetDlgItem(_consoleDlgHwnd, IDC_BTCLEARCONSOLE), BM_SETIMAGE, static_cast<WPARAM>(IMAGE_BITMAP), reinterpret_cast<LPARAM>(clearWindow));
			HBITMAP wordWrap = iconToBitmap(reinterpret_cast<HICON>(LoadImage(_hInst, MAKEINTRESOURCE(IDI_WORDWRAP), IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR)));
			::SendMessage(GetDlgItem(_consoleDlgHwnd, IDC_BTTOGGLEWORDWRAP), BM_SETIMAGE, static_cast<WPARAM>(IMAGE_BITMAP), reinterpret_cast<LPARAM>(wordWrap));
			HBITMAP errorFilter = getStockIconBitmap(SHSTOCKICONID::SIID_ERROR, IconSize::Size16x16);
			HBITMAP warningFilter = getStockIconBitmap(SHSTOCKICONID::SIID_WARNING, IconSize::Size16x16);
			HBITMAP infoFilter = getStockIconBitmap(SHSTOCKICONID::SIID_INFO, IconSize::Size16x16);
			::SendMessage(GetDlgItem(_consoleDlgHwnd, IDC_BTFILTERERRORS), BM_SETIMAGE, static_cast<WPARAM>(IMAGE_BITMAP), reinterpret_cast<LPARAM>(errorFilter));
			::SendMessage(GetDlgItem(_consoleDlgHwnd, IDC_BTFILTERWARNINGS), BM_SETIMAGE, static_cast<WPARAM>(IMAGE_BITMAP), reinterpret_cast<LPARAM>(warningFilter));
			::SendMessage(GetDlgItem(_consoleDlgHwnd, IDC_BTFILTERINFO), BM_SETIMAGE, static_cast<WPARAM>(IMAGE_BITMAP), reinterpret_cast<LPARAM>(infoFilter));

			// Set Toggle Word Wrap state (default is ON. If it's off, we need to rebuild the control)
			CheckDlgButton(_consoleDlgHwnd, IDC_BTTOGGLEWORDWRAP, _settings->compilerWindowConsoleWordWrap);
			if (!_settings->compilerWindowConsoleWordWrap)
				ToggleWordWrap();

			// Set filters checkboxes (buttons) state
			CheckDlgButton(_consoleDlgHwnd, IDC_BTFILTERERRORS, _settings->compilerWindowConsoleShowErrors);
			CheckDlgButton(_consoleDlgHwnd, IDC_BTFILTERWARNINGS, _settings->compilerWindowConsoleShowWarnings);
			CheckDlgButton(_consoleDlgHwnd, IDC_BTFILTERINFO, _settings->compilerWindowConsoleShowInfos);			

			// Setup anchors
			SetupDockingAnchors();

			// Add columns to listview
			SetupListView();

			// Select saved tab
			if (_settings->compilerWindowSelectedTab == 0)
				switchToErrors();
			else
				switchToConsole();

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
					_settings->compilerWindowSelectedTab = tabSelection;
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

				case IDM_ERRORTOGGLE:
				case IDM_WARNINGTOGGLE:
				case IDM_MESSAGETOGGLE:
				{
					// Get changed states
					_settings->compilerWindowShowErrors = ::SendMessage(_toolBar, TB_ISBUTTONCHECKED, IDM_ERRORTOGGLE, 0);
					_settings->compilerWindowShowWarnings = ::SendMessage(_toolBar, TB_ISBUTTONCHECKED, IDM_WARNINGTOGGLE, 0);
					_settings->compilerWindowShowInfos = ::SendMessage(_toolBar, TB_ISBUTTONCHECKED, IDM_MESSAGETOGGLE, 0);

					// Rebuild errors list
					RebuildErrorsList();
					break;
				}

				case IDC_BTCLEARCONSOLE:
				{
					SetDlgItemText(_consoleDlgHwnd, IDC_TXTCONSOLE, TEXT(""));
					break;
				}

				case IDC_BTTOGGLEWORDWRAP:
				{
					ToggleWordWrap();
					break;
				}

				case IDC_BTFILTERERRORS:
				case IDC_BTFILTERWARNINGS:
				case IDC_BTFILTERINFO:
				{
					_settings->compilerWindowConsoleShowErrors = IsDlgButtonChecked(_consoleDlgHwnd, IDC_BTFILTERERRORS);
					_settings->compilerWindowConsoleShowWarnings = IsDlgButtonChecked(_consoleDlgHwnd, IDC_BTFILTERWARNINGS);
					_settings->compilerWindowConsoleShowInfos = IsDlgButtonChecked(_consoleDlgHwnd, IDC_BTFILTERINFO);
				}
			}
		}

		case WM_NOTIFY:
		{
			if (wParam == IDC_LSTERRORS)
			{
				LPNMITEMACTIVATE lpnmia = (LPNMITEMACTIVATE)lParam;
				NMHDR hdr = lpnmia->hdr;
#pragma warning (push)
#pragma warning (disable : 26454)
				if (hdr.code == NM_CLICK)
#pragma warning (pop)
				{
    				// Invalid item or list is "locked" for user input at the time.
					if (lpnmia->iItem < 0 || !_processInputForErrorList)
						return FALSE;

					HWND lstErrors = GetDlgItem(_errorDlgHwnd, IDC_LSTERRORS);

					// Gather the item ID stored previously
					int messageItem = -1;
					LVITEM itemInfo;
					itemInfo.mask = LVIF_PARAM;
					itemInfo.iItem = lpnmia->iItem;
					ListView_GetItem(lstErrors, &itemInfo);
					messageItem = static_cast<int>(itemInfo.lParam);

					CompilerMessage& r = _errorsList[messageItem];

					// Dispatch to Plugin for processing.
					generic_string fileName = r.fileName.empty() ? TEXT("") : r.fileName + TEXT(".") + r.fileExt;
					int lineNumber = r.lineNumber.empty() ? -1 : stoi(r.lineNumber);

					// Only dispatch messages with valid line numbers
					if (navigateToFileCallback && lineNumber > -1)
					{
						// HACK: To correct the file navigation issue, we store the current lineNumber being passed
						// to navigateToFileCallback, so the timer on it can refer back to it.
						_currentLine = lineNumber;
						navigateToFileCallback(fileName, lineNumber, r.messageText, r.filePath);
					}
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
	ZeroMemory(columnName, std::size(columnName));

	// Extended Styles
	ListView_SetExtendedListViewStyle(listErrorsHWND, LVS_EX_DOUBLEBUFFER | LVS_EX_FULLROWSELECT | LVS_EX_TRACKSELECT |
		LVS_EX_SUBITEMIMAGES | LVS_EX_TWOCLICKACTIVATE | LVS_EX_JUSTIFYCOLUMNS | LVS_EX_UNDERLINEHOT);

	// Associate icons list with errors list
	ListView_SetImageList(GetDlgItem(_errorDlgHwnd, IDC_LSTERRORS), _iconList16x16, LVSIL_SMALL);

	// Column generics
	LVCOLUMN newColumn;
	ZeroMemory(&newColumn, sizeof(newColumn));
	newColumn.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_ORDER | LVCF_DEFAULTWIDTH | LVCF_IDEALWIDTH;
	newColumn.fmt = LVCFMT_FIXED_WIDTH | LVCFMT_LEFT | LVCFMT_IMAGE | LVCFMT_FIXED_RATIO;
	newColumn.pszText = columnName;

	// Column 0 is hidden (displays icon)
	newColumn.mask = LVCF_FMT | LVCF_WIDTH;
	newColumn.fmt = LVCFMT_FIXED_WIDTH;
	newColumn.cx = 0;
	ListView_InsertColumn(listErrorsHWND, 0, &newColumn);

	// First column (a dummy empty one)
	newColumn.mask = LVCF_FMT | LVCF_WIDTH;
	newColumn.fmt = LVCFMT_FIXED_WIDTH;
	newColumn.cx = 5;
	ListView_InsertColumn(listErrorsHWND, 1, &newColumn);

	// Message Code
	newColumn.mask = LVCF_FMT | LVCF_TEXT | LVCF_MINWIDTH | LVCF_DEFAULTWIDTH | LVCF_WIDTH;
	newColumn.fmt = LVCFMT_LEFT;
	newColumn.cxMin = 93;
	newColumn.cxDefault = 95;
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

void LoggerDialog::LogMessage(const CompilerMessage& message, const generic_string& filePath)
{
	// Create a more complete version of the log message, appended the full file path.
	// (because at the time of creation the logger don't have this info avaliable)
	CompilerMessage fullMessage = message;
	fullMessage.filePath = filePath;

	if (message.messageType != LogType::ConsoleMessage)
		_errorsList.push_back(fullMessage);

	// Write message to console and/or errors list
	WriteToErrorsList(message, false);

	// Update counters
	if (message.messageType == LogType::Critical || message.messageType == LogType::Error)
		_errorCount++;
	if (message.messageType == LogType::Warning)
		_warningCount++;
	if (message.messageType == LogType::Info)
		_infoCount++;

	UpdateToolButtonLabels();
}

void LoggerDialog::WriteToErrorsList(const CompilerMessage& message, bool ignoreConsole)
{
	HWND lstErrors = GetDlgItem(_errorDlgHwnd, IDC_LSTERRORS);

	// Write raw text to console
	if (!ignoreConsole && message.messageType == LogType::ConsoleMessage)
	{
		AppendConsoleText(message.messageText + TEXT("\r\n"));
		return;
	}

	// Format message
	generic_string messageType = message.messageType == LogType::Error ? TEXT("Error") :
		message.messageType == LogType::Warning ? TEXT("Warning") : TEXT("Info");
	generic_string output = message.fileName + TEXT(".") + message.fileExt + TEXT("(") + message.lineNumber + TEXT("): ")
		+ messageType + TEXT(": ") + message.messageCode + TEXT(": ") + message.messageText;

	// Critical messages - override output message
	if (message.messageType == LogType::Critical)
		output = TEXT("Critical Error [") + message.messageCode + TEXT("]: ") + message.messageText;

	// Pure infos -  override output message
	if (message.messageType == LogType::Info && message.fileName.empty())
		output = TEXT("Info: ") + message.messageText;

	// Write message to console 
	if (!ignoreConsole)
	{
		if ((message.messageType == LogType::Critical || message.messageType == LogType::Error) && _settings->compilerWindowConsoleShowErrors)
			AppendConsoleText(output + TEXT("\r\n"));
		if (message.messageType == LogType::Warning && _settings->compilerWindowConsoleShowWarnings)
			AppendConsoleText(output + TEXT("\r\n"));
		if (message.messageType == LogType::Info && _settings->compilerWindowConsoleShowInfos)
			AppendConsoleText(output + TEXT("\r\n"));
	}

	// Don't show filtered messages (error filters are different from console filters)
	if ((message.messageType == LogType::Critical || message.messageType == LogType::Error) && !_settings->compilerWindowShowErrors)
		return;
	if (message.messageType == LogType::Warning && !_settings->compilerWindowShowWarnings)
		return;
	if (message.messageType == LogType::Info && !_settings->compilerWindowShowInfos)
		return;

	// Retrieve the message index on list and store for future reference.
	int itemIndex = 0;
	for (; itemIndex < _errorsList.size(); itemIndex++)
	{
		if (_errorsList[itemIndex] == message)
			break;
	}

	// Create the list item
	size_t currentItem = ListView_GetItemCount(lstErrors);
	LVITEM newItem;
	ZeroMemory(&newItem, sizeof(LVITEM));
	newItem.mask = LVIF_PARAM;
	newItem.iItem = currentItem;
	newItem.lParam = (LPARAM)itemIndex;
	currentItem = ListView_InsertItem(lstErrors, &newItem);

	// Update all visible columns
	newItem.iItem = currentItem;
	newItem.mask = LVIF_TEXT | LVIF_IMAGE;  // Icon column
	newItem.iSubItem = ERRORCOLUMN_MESSAGECODE;
	newItem.iImage = (int)message.messageType;
	generic_string code = message.messageCode.empty() ? messageType : message.messageCode;
	newItem.pszText = (LPWSTR)code.c_str();
	ListView_SetItem(lstErrors, &newItem);
	ListView_SetItemText(lstErrors, currentItem, ERRORCOLUMN_MESSAGETEXT, (LPWSTR)message.messageText.c_str());
	generic_string fileName = message.fileName.empty() ? TEXT("-") : message.fileName + TEXT(".") + message.fileExt;
	ListView_SetItemText(lstErrors, currentItem, ERRORCOLUMN_FILENAME, (LPWSTR)fileName.c_str());
	generic_string lineNumber = message.lineNumber.empty() ? TEXT("-") : message.lineNumber;
	ListView_SetItemText(lstErrors, currentItem, ERRORCOLUMN_FILELINE, (LPWSTR)lineNumber.c_str());

}

void LoggerDialog::AppendConsoleText(const generic_string& newText)
{
	CHARRANGE cr;
	cr.cpMin = -1;
	cr.cpMax = -1;
	HWND editControl = GetDlgItem(_consoleDlgHwnd, IDC_TXTCONSOLE);
	SendMessage(editControl, EM_EXSETSEL, 0, reinterpret_cast<LPARAM>(&cr));
	SendMessage(editControl, EM_REPLACESEL, 0, reinterpret_cast<LPARAM>(newText.c_str()));
	SendMessage(editControl, WM_VSCROLL, SB_BOTTOM, 0);
}

void LoggerDialog::RebuildErrorsList()
{
	// Clear current Errors List
	ListView_DeleteAllItems(GetDlgItem(_errorDlgHwnd, IDC_LSTERRORS));

	// Redo with filters (also ignore console messages)
	for (CompilerMessage m : _errorsList)
	{
		if ((m.messageType == LogType::Critical || m.messageType == LogType::Error) && _settings->compilerWindowShowErrors)
			WriteToErrorsList(m, true);
		if (m.messageType == LogType::Warning && _settings->compilerWindowShowWarnings)
			WriteToErrorsList(m, true);
		if (m.messageType == LogType::Info && _settings->compilerWindowShowInfos)
			WriteToErrorsList(m, true);
	}

	// Update tool buttons captions.
	UpdateToolButtonLabels();
}

void LoggerDialog::UpdateToolButtonLabels()
{
	TBBUTTONINFO tbButton;
	ZeroMemory(&tbButton, sizeof(TBBUTTONINFO));

	tbButton.dwMask = TBIF_TEXT;
	tbButton.cbSize = sizeof(TBBUTTONINFO);

	// Errors count
	generic_string labelText = TEXT(" ") + ((_settings->compilerWindowShowErrors || _errorCount == 0 ? TEXT("(") + std::to_wstring(_errorCount) + TEXT(") ") :
		TEXT("(0 of ") + std::to_wstring(_errorCount) + TEXT(") ")) + TEXT("Errors"));
	tbButton.pszText = (LPWSTR)labelText.c_str();
	::SendMessage(_toolBar, TB_SETBUTTONINFO, IDM_ERRORTOGGLE, (LPARAM)&tbButton);

	// Warnings count
	labelText = TEXT(" ") + ((_settings->compilerWindowShowWarnings || _warningCount == 0 ? TEXT("(") + std::to_wstring(_warningCount) + TEXT(") ") :
		TEXT("(0 of ") + std::to_wstring(_warningCount) + TEXT(") ")) + TEXT("Warnings"));
	tbButton.pszText = (LPWSTR)labelText.c_str();
	::SendMessage(_toolBar, TB_SETBUTTONINFO, IDM_WARNINGTOGGLE, (LPARAM)&tbButton);

	// Messages (Info) count
	labelText = TEXT(" ") + ((_settings->compilerWindowShowInfos || _infoCount == 0 ? TEXT("(") + std::to_wstring(_infoCount) + TEXT(") ") :
		TEXT("(0 of ") + std::to_wstring(_infoCount) + TEXT(") ")) + TEXT("Messages"));
	tbButton.pszText = (LPWSTR)labelText.c_str();
	::SendMessage(_toolBar, TB_SETBUTTONINFO, IDM_MESSAGETOGGLE, (LPARAM)&tbButton);
}

void LoggerDialog::LockControls(bool toLock)
{
	// Processing of error list clicks (lock/unlock)
	_processInputForErrorList = !toLock;

	// Lock/unlock the toolbar
	EnableWindow(_toolBar, !toLock);

	// Enable/Disable clear window and word wrap;
	EnableWindow(GetDlgItem(_consoleDlgHwnd, IDC_BTCLEARCONSOLE), !toLock);
	EnableWindow(GetDlgItem(_consoleDlgHwnd, IDC_BTTOGGLEWORDWRAP), !toLock);

	EnableWindow(GetDlgItem(_consoleDlgHwnd, IDC_BTFILTERERRORS), !toLock);
	EnableWindow(GetDlgItem(_consoleDlgHwnd, IDC_BTFILTERWARNINGS), !toLock);
	EnableWindow(GetDlgItem(_consoleDlgHwnd, IDC_BTFILTERINFO), !toLock);
}

// There's no toggle word wrap message to text boxes, we must recreate the control.
// https://stackoverflow.com/questions/56781359/how-do-i-toggle-word-wrap-in-a-editbox
void LoggerDialog::ToggleWordWrap()
{
	RECT editRect;
	generic_string sTextBuffer;
	HWND editControl = GetDlgItem(_consoleDlgHwnd, IDC_TXTCONSOLE);

	// Retrieve text for swapping control
	GETTEXTLENGTHEX tl = { GTL_NUMCHARS, 1200 };
	sTextBuffer.resize(SendMessage(editControl, EM_GETTEXTLENGTHEX, (WPARAM)&tl, 0) + 1);
	GETTEXTEX tex = { (DWORD)sTextBuffer.size() * sizeof(TCHAR), GT_RAWTEXT, 1200, NULL, NULL };
	SendMessage(editControl, EM_GETTEXTEX, (WPARAM)&tex, (LPARAM)sTextBuffer.data());

	// Get previous windows measures
	GetWindowRect(editControl, &editRect);
	ScreenToClient(_consoleDlgHwnd, &editRect);

	// Destroy current Control (and remove from anchor map)
	ANCHOR_MAP_REMOVE(editControl);
	DestroyWindow(editControl);

	// New edit will have or not WS_HCROLL style depending if word wrap is on.
	_settings->compilerWindowConsoleWordWrap = IsDlgButtonChecked(_consoleDlgHwnd, IDC_BTTOGGLEWORDWRAP);
	DWORD defaultStyles = WS_CHILD | ES_SUNKEN | WS_VSCROLL | WS_TABSTOP | 0x804 | (_settings->compilerWindowConsoleWordWrap ? 0 : WS_HSCROLL);

	// Using HMENU trick to preserve control ID.
	// https://social.msdn.microsoft.com/Forums/vstudio/en-US/aa81f991-85c9-431a-8804-2a580aa6a293/assigning-a-control-id-to-a-win32-button?forum=vcgeneral
	SetLastError(0);
	editControl = CreateWindowEx(0, MSFTEDIT_CLASS, 0, defaultStyles, editRect.left, editRect.top, 
		editRect.right - editRect.left, editRect.bottom - editRect.top, _consoleDlgHwnd, (HMENU)IDC_TXTCONSOLE, _hInst, 0);
	DWORD test = GetLastError();

	// Restore default font style
	HFONT editFont = CreateFont(15, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
		OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH, TEXT("Consolas"));
	::SendMessage(editControl, WM_SETFONT, reinterpret_cast<WPARAM>(editFont), 0);

	// Redo visibility and anchor map
	ShowWindow(editControl, SW_NORMAL);
	ANCHOR_MAP_DYNAMICCONTROL(editControl, ANF_ALL);

	// Restore old text
	SETTEXTEX stex = { ST_DEFAULT, 1200 };
	SendMessage(editControl, EM_SETTEXTEX, (WPARAM)&stex, (LPARAM)sTextBuffer.c_str());
}