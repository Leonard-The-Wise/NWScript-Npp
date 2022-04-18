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

#include "Settings.h"
#include "NWScriptLogger.h"


typedef NWScriptPlugin::NWScriptLogger::CompilerMessage CompilerMessage;
typedef NWScriptPlugin::NWScriptLogger::LogType LogType;

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

		void reset()
		{
			clearConsole();
			clearErrors();
			switchToConsole();
		}

		void appendSettings(Settings* settings) {
			_settings = settings;
		}

		void SetNavigateFunctionCallback(void (*_navigateToFileCallback)(const generic_string& fileName, size_t lineNum, const generic_string& rawMessage,
			const fs::path& filePath))
		{
			navigateToFileCallback = _navigateToFileCallback;
		}

		// HACK: To correct the file navigation issue, we store the current lineNumber being passed
		// to navigateToFileCallback, so the timer on it can refer back to it.
		int getCurrentNavigationLine() const {
			return _currentLine;
		}

		void checkSwitchToErrors()	{
			if (_errorCount > 0)
				switchToErrors();
		}

		void LogMessage(const CompilerMessage& message, const generic_string& filePath = TEXT(""));

		void LockControls(bool toLock);

		void setDarkMode() {
			checkSwitchToDarkMode();
		}

	protected:

		// Main window dialog procedure call
		virtual intptr_t CALLBACK run_dlgProc(UINT message, WPARAM wParam, LPARAM lParam);

		// Proxy for children dialog processing
		static INT_PTR CALLBACK dlgProxy(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
		// Real children message-processing 
		intptr_t CALLBACK childrenDlgProc(UINT message, WPARAM wParam, LPARAM lParam);

		void checkSwitchToDarkMode();

	private:
		DECLARE_ANCHOR_MAP()

		bool anchorsPrepared = false;
		void SetupDockingAnchors();
		void SetupListView();
		void ResizeList();
		void AppendConsoleText(const generic_string& newText);
		void RebuildErrorsList();
		void WriteToErrorsList(const CompilerMessage& message, bool ignoreConsole = false);
		void UpdateToolButtonLabels();
		void RecreateTxtConsole();
		void RecreateIcons();
		HWND CreateToolTip(HWND hDlg, int toolID, PCTSTR pszText);
		void CreateTooltips();

		void switchToConsole()
		{
			TabCtrl_SetCurSel(_mainTabHwnd, 1);
			run_dlgProc(WM_NOTIFY, IDC_TABLOGGER, 0);
		}

		void switchToErrors()
		{
			TabCtrl_SetCurSel(_mainTabHwnd, 0);
			run_dlgProc(WM_NOTIFY, IDC_TABLOGGER, 0);
		}

		void clearConsole()
		{
			SetDlgItemText(_consoleDlgHwnd, IDC_TXTCONSOLE, L"");
		}

		void clearErrors()
		{
			ListView_DeleteAllItems(GetDlgItem(_errorDlgHwnd, IDC_LSTERRORS));
			_errorsList.clear();
			_errorCount = 0;
			_warningCount = 0;
			_infoCount = 0;
		}

		HWND _mainTabHwnd = nullptr;
		HWND _errorDlgHwnd = nullptr;
		HWND _consoleDlgHwnd = nullptr;
		HWND _listErrors = nullptr;
		HWND _toolBar = nullptr;

		static const int _maxToolTips = 5;
		HWND _toolTips[_maxToolTips] = { };

		HWND _txtConsole = nullptr;

		HIMAGELIST _iconList16x16 = nullptr;

		Settings* _settings = nullptr;

		std::vector<CompilerMessage> _errorsList;
		int _errorCount = 0;
		int _warningCount = 0;
		int _infoCount = 0;
		bool _processInputForErrorList = true;

		void (*navigateToFileCallback)(const generic_string& fileName, size_t lineNum, const generic_string& rawMessage,
			const fs::path& filePath) = nullptr;

		// HACK: To correct the file navigation issue, we store the current lineNumber being passed
		// to navigateToFileCallback, so the timer on it can refer back to it.
		int _currentLine = 0;

	};
}